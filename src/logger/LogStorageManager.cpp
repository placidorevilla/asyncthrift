#include "LogStorageManager.h"
#include "LogStorageManager_p.h"

#include "AsyncThriftLogger.h"
#include "ThriftDispatcher.h"
#include "NBRingByteBuffer.h"
#include "LogEndian.h"
#include "HBaseOperations.h"

#include <QSettings>
#include <QDir>
#include <QMutexLocker>
#include <QTemporaryFile>
#include <QLocalSocket>

#include <lzma.h>

#include <fcntl.h>
#include <time.h>

#include "config.h"

log4cxx::LoggerPtr LogStorageManager::logger(log4cxx::Logger::getLogger(LogStorageManager::staticMetaObject.className()));
log4cxx::LoggerPtr LogStorageManagerPrivate::logger(log4cxx::Logger::getLogger(LogStorageManager::staticMetaObject.className()));
log4cxx::LoggerPtr LogStorage::logger(log4cxx::Logger::getLogger(LogStorage::staticMetaObject.className()));
log4cxx::LoggerPtr LogWriteThread::logger(log4cxx::Logger::getLogger(LogWriteThread::staticMetaObject.className()));
log4cxx::LoggerPtr LogSyncThread::logger(log4cxx::Logger::getLogger(LogSyncThread::staticMetaObject.className()));
log4cxx::LoggerPtr LogAllocateThread::logger(log4cxx::Logger::getLogger(LogAllocateThread::staticMetaObject.className()));
log4cxx::LoggerPtr LogReadThread::logger(log4cxx::Logger::getLogger(LogReadThread::staticMetaObject.className()));
log4cxx::LoggerPtr StorageReadContext::logger(log4cxx::Logger::getLogger(LogReadThread::staticMetaObject.className()));

static const int RINGBUFFER_READ_TIMEOUT = 500;
// TODO: this should be config
static const char* LOGGER_SOCKET_NAME = "logger";
static const int MAX_CLIENT_BUFFER = 1 * 1024 * 1024;
static const int MAX_TRANSACTIONS_SENT_PER_LOOP = 32;

LogStorageManager::LogStorageManager(QObject* parent) : QObject(parent), d(new LogStorageManagerPrivate(this))
{
}

LogStorageManager::~LogStorageManager()
{
}

bool LogStorageManager::configure(unsigned int max_log_size, unsigned int sync_period, const QStringList& dirs)
{
	LOG4CXX_DEBUG(logger, "Configuring storages");

	d->set_max_log_size(max_log_size);
	d->create_storages(dirs);
	d->set_sync_period(sync_period);
	d->listen();

	return true;
}

LogStorageManagerPrivate::LogStorageManagerPrivate(QObject* parent) : QObject(parent), max_log_size_(0)
{
	this->connect(&sync_timer, SIGNAL(timeout()), SLOT(sync_timeout()));	
}

LogStorageManagerPrivate::~LogStorageManagerPrivate()
{
	foreach (LogReadThread* read_thread, read_threads) {
		read_thread->quit();
		read_thread->wait();
		delete read_thread;
	}
}

void LogStorageManagerPrivate::create_storages(const QStringList& dirs)
{
	QSet<QString> unique_storages;

	foreach (const QString& dir, dirs) {
		QString canonical = QDir(dir).canonicalPath();
		if (unique_storages.contains(canonical)) {
			LOG4CXX_WARN(logger, "Configuration has repeated logging dirs. Ignoring.");
			continue;
		}
		unique_storages.insert(canonical);
	}

	if (storages.size() != 0) {
		foreach (LogStorage* storage, storages) {
			QString dir(storage->path());
			if (!unique_storages.contains(dir)) {
				LOG4CXX_WARN(logger, "Cannot reconfigure storages while running");
				return;
			}
			unique_storages.remove(dir);
		}
		if (!unique_storages.isEmpty())
			LOG4CXX_WARN(logger, "Cannot reconfigure storages while running");
		return;
	}

	for (auto canonical = unique_storages.begin(); canonical != unique_storages.end(); canonical++) {
		LogStorage* storage = new LogStorage(this, *canonical);
		storages.append(storage);
	}
}

void LogStorageManagerPrivate::listen()
{
	if (!logger_server.isListening()) {
		QDir server_dir(PKGSTATEDIR);

		if (server_dir.exists(LOGGER_SOCKET_NAME))
			server_dir.remove(LOGGER_SOCKET_NAME);

		connect(&logger_server, SIGNAL(newConnection()), SLOT(new_logger_connection()));

		logger_server.listen(server_dir.absoluteFilePath(LOGGER_SOCKET_NAME));
	}
}

void LogStorageManagerPrivate::new_logger_connection()
{
	while (true) {
		QLocalSocket* logger_connection = logger_server.nextPendingConnection();
		if (!logger_connection)
			break;

		LogReadThread* read_thread = new LogReadThread(logger_connection, this);
		connect(read_thread, SIGNAL(finished()), SLOT(finished_logger_connection()));
		read_threads.append(read_thread);
		read_thread->start();
	}
}

void LogStorageManagerPrivate::finished_logger_connection()
{
	LogReadThread* read_thread(qobject_cast<LogReadThread*>(sender()));

	delete read_thread;
	read_threads.remove(read_threads.indexOf(read_thread));

	LOG4CXX_DEBUG(logger, "Finished read thread");
}

void LogStorageManagerPrivate::set_sync_period(unsigned int sync_period)
{
	sync_timer.setInterval(sync_period / storages.size());
	if (!sync_timer.isActive())
		sync_timer.start();
}

void LogStorageManagerPrivate::sync_timeout()
{
	static unsigned int current_storage = 0;

	int nstorages = storages.size();
	
	if (!nstorages)
		return;

	QMetaObject::invokeMethod(storages.at((current_storage++) % nstorages), "sync", Qt::QueuedConnection);
}

uint64_t LogStorageManagerPrivate::transaction(uint64_t new_transaction)
{
	static uint64_t cur_transaction = 0;

	if (new_transaction != 0) {
		uint64_t transaction = cur_transaction;
		if (new_transaction < transaction)
			return cur_transaction;
		while (!__sync_bool_compare_and_swap(&cur_transaction, transaction, new_transaction)) {
			__sync_synchronize();
			transaction = cur_transaction;
			if (new_transaction < transaction)
				return cur_transaction;
		}
		return new_transaction;
	}
	return __sync_add_and_fetch(&cur_transaction, 1);
}

LogReadContext* LogStorageManagerPrivate::begin_read(uint64_t transaction)
{
	QList<StorageReadContext*> storage_contexts;

	foreach (LogStorage* storage, storages)
		storage_contexts.append(storage->begin_read(transaction));

	return new LogReadContext(this, storage_contexts);
}

void LogStorageManagerPrivate::end_read(LogReadContext* context)
{
	delete context;
}

bool LogStorageManagerPrivate::read_next_transaction(LogReadContext* context, char** buffer, size_t* size)
{
	return context->read_next_transaction(buffer, size);
}

LogReadContext::LogReadContext(LogStorageManagerPrivate* manager, const QList<StorageReadContext*>& storage_contexts) : manager(manager), storage_contexts(storage_contexts)
{
}

LogReadContext::~LogReadContext()
{
	foreach (StorageReadContext* context, storage_contexts)
		delete context;
}

bool LogReadContext::read_next_transaction(char** buffer, size_t* size)
{
	uint64_t transaction = UINT64_MAX;
	StorageReadContext* context_to_advance = 0;
	bool any_valid = false;
	
	foreach (StorageReadContext* context, storage_contexts) {
		if (!context->peek_next_transaction(buffer, size)) {
			// TODO: check if we are blocked waiting for events
			context->advance();
			if (!context->peek_next_transaction(buffer, size))
				continue;
		}
		if (**(uint64_t**)buffer < transaction) {
			transaction = LOG_ENDIAN(**(uint64_t**)buffer);
			context_to_advance = context;
			any_valid = true;
		}
	}

//	printf("tx: %lu, any: %d\n", transaction, any_valid);

	if (context_to_advance)
		context_to_advance->advance();
	return any_valid;
}

StorageReadContext::StorageReadContext(LogStorage* storage, int index, TMemFile* file) : storage(storage), index(index), file(file)
{
}

StorageReadContext::~StorageReadContext()
{
	delete file;
}

bool StorageReadContext::peek_next_transaction(char** buffer, size_t* size)
{
	// TODO: this can't happen
	if (!file)
		return false;
	char* buf = (char*)file->buffer();
	if (LOG_ENDIAN(*(uint64_t *)(buf + file->pos())) == 0)
		return false;
	*buffer = buf + file->pos();
	*size = (LOG_ENDIAN(*((*(uint64_t**)buffer) + 1)) >> (8 * sizeof(uint32_t))) + 3 * sizeof(uint64_t);  // 3 * uint64_t : txid, timestamp_len, crc
	return true;
}

bool StorageReadContext::advance(uint64_t transaction)
{
	char* buffer;
	uint64_t current_transaction;
	size_t size;

	// TODO: this can't happen
	if (!file)
		return false;

	do {
		buffer = (char*)file->buffer() + file->pos();
		if (((size_t)(file->size() - file->pos()) < (3 * sizeof(uint64_t))) || ((current_transaction = LOG_ENDIAN(*(uint64_t*)buffer)) == 0)) {
			delete file;
			file = storage->advance_next_file(&index);
			if (file)
				return true;
			return false;
		}
		if (transaction == 0 || transaction > current_transaction) {
			size = (LOG_ENDIAN(*(((uint64_t*)buffer) + 1)) >> (8 * sizeof(uint32_t))) + 3 * sizeof(uint64_t);  // 3 * uint64_t : txid, timestamp_len, crc
			file->seek(file->pos() + size);
		}
	} while (transaction != 0 && transaction > current_transaction);
	
	return true;
}

LogStorage::LogStorage(LogStorageManagerPrivate* manager, const QString& dir) : QObject(manager), storage_dir(dir), write_thread(new LogWriteThread(this)), sync_thread(new LogSyncThread(this)), manager(manager), need_sync(false)
{
	get_next_file();
}

LogStorage::~LogStorage()
{
	write_thread->quit();
	write_thread->wait();
	delete write_thread;
	sync_thread->quit();
	sync_thread->wait();
	delete sync_thread;

	current_log.close();
}

StorageReadContext* LogStorage::begin_read(uint64_t transaction)
{
	QMutexLocker locker(&file_guard);
	TMemFile* file = 0;
	int index = -1;

	// TODO: check if the current writing file is the one we need
	
	QList<int> known_files = file_to_transaction_map.keys();
	if (known_files.size() > 0)
		index = *known_files.begin();
	for (auto log_index = known_files.end();;) {
		if (log_index == known_files.begin())
			break;
		log_index--;
		if (file_to_transaction_map[*log_index].first == 0)
			map_log_file(*log_index, true);
		if (file_to_transaction_map.contains(*log_index) && file_to_transaction_map[*log_index].first <= transaction) {
			index = *log_index;
			break;
		}
	}

	// TODO: this can't happen
	if (index == -1) {
		LOG4CXX_DEBUG(logger, "should have opened the current file");
		return 0;
	}

	file = new TMemFile(storage_dir.absoluteFilePath(QString("asyncthrift.%1.log").arg(index, 4, 10, QChar('0'))));
	file->open(QIODevice::ReadWrite);

	StorageReadContext* context = new StorageReadContext(this, index, file);
	context->advance(transaction);
	return context;
}

TMemFile* LogStorage::advance_next_file(int* index)
{
	TMemFile* file = 0;

	LOG4CXX_DEBUG(logger, "advance_next_file");

	// TODO: this can't happen
	if (*index == -1)
		return 0;

	auto log_index = file_to_transaction_map.find(*index) + 1;

	if (log_index == file_to_transaction_map.end()) {
		LOG4CXX_DEBUG(logger, "We need to open the current file");
		// TODO: open the current writing file
		*index = -1;
		return 0;
	}

	file = new TMemFile(storage_dir.absoluteFilePath(QString("asyncthrift.%1.log").arg(log_index.key(), 4, 10, QChar('0'))));
	file->open(QIODevice::ReadWrite);
	*index = log_index.key();
	return file;
}

void LogStorage::map_log_file(int log_index, bool only_start)
{
	if (invalid_files_map.contains(log_index))
		return;

	TMemFile log_file(storage_dir.absoluteFilePath(QString("asyncthrift.%1.log").arg(log_index, 4, 10, QChar('0'))));
	log_file.open(QIODevice::ReadOnly);
	uint64_t transaction, first_good_transaction = 0, last_good_transaction = 0;
	uint64_t timestamp_and_len;
	if (log_file.read((char*)&transaction, sizeof(transaction)) != sizeof(transaction)) {
		LOG4CXX_WARN(logger, qPrintable(QString("Corrupt log file: '%1'").arg(log_file.file()->fileName())));
	} else {
		first_good_transaction = transaction = LOG_ENDIAN(transaction);
		LOG4CXX_INFO(logger, qPrintable(QString("First good transaction for file '%1' is %2").arg(log_file.file()->fileName()).arg(first_good_transaction)));
		while(!only_start && transaction != 0) {
			last_good_transaction = transaction;
			if (log_file.read((char*)&timestamp_and_len, sizeof(timestamp_and_len)) != sizeof(timestamp_and_len))
				break;
			timestamp_and_len = LOG_ENDIAN(timestamp_and_len);
			if (!log_file.seek(log_file.pos() + (timestamp_and_len >> (sizeof(uint32_t) * 8)) + sizeof(uint64_t)))
				break;
			if (log_file.read((char*)&transaction, sizeof(transaction)) != sizeof(transaction))
				break;
			transaction = LOG_ENDIAN(transaction);
		}
		LOG4CXX_INFO(logger, qPrintable(QString("Last good transaction for file '%1' is %2").arg(log_file.file()->fileName()).arg(last_good_transaction)));
		if (first_good_transaction != 0) {
			file_to_transaction_map.insert(log_index, qMakePair(first_good_transaction, last_good_transaction));
		} else {
			file_to_transaction_map.remove(log_index);
			invalid_files_map.insert(log_index);
		}
		log_file.close();
	}
}

void LogStorage::get_next_file()
{
	QMutexLocker locker(&file_guard);
	int next_log_index = -1;

	foreach(const QString& log_filename, storage_dir.entryList(QStringList() << "asyncthrift.[0-9][0-9][0-9][0-9].log", QDir::Files, QDir::NoSort)) {
		int log_index = log_filename.mid(12, 4).toInt();
		next_log_index = qMax(next_log_index, log_index);
		if (!file_to_transaction_map.contains(log_index) && !invalid_files_map.contains(log_index))
			file_to_transaction_map.insert(log_index, qMakePair(0UL, 0UL));
	}

	QList<int> known_files = file_to_transaction_map.keys();
	for (auto log_index = known_files.end();;) {
		if (log_index == known_files.begin())
			break;
		log_index--;
		if (file_to_transaction_map[*log_index].first == 0)
			map_log_file(*log_index);
		if (file_to_transaction_map.contains(*log_index)) {
			manager->transaction(file_to_transaction_map[*log_index].second);
			break;
		}
	}

	// If the most recent log file is empty use it instead of creating a new one
	if (!invalid_files_map.isEmpty()) {
		int potential_index = *std::max_element(invalid_files_map.begin(), invalid_files_map.end());
		if (file_to_transaction_map.isEmpty() || (file_to_transaction_map.end() - 1).key() < potential_index) {
			if (current_log.isOpen())
				current_log.close();
			current_log.file()->setFileName(storage_dir.absoluteFilePath(QString("asyncthrift.%1.log").arg(potential_index, 4, 10, QChar('0'))));
			current_log.open(QIODevice::ReadWrite);
			return;
		}
	}

	next_log_index++;

	if (!storage_dir.exists("asyncthrift.next.log")) {
		alloc_thread = new LogAllocateThread(storage_dir.canonicalPath(), manager->max_log_size());
		alloc_thread->start();
		alloc_thread->wait();
	}

	if (current_log.isOpen())
		current_log.close();

	current_log.file()->setFileName(storage_dir.absoluteFilePath("asyncthrift.next.log"));
	current_log.file()->rename(storage_dir.absoluteFilePath(QString("asyncthrift.%1.log").arg(next_log_index, 4, 10, QChar('0'))));
	current_log.open(QIODevice::ReadWrite);

	alloc_thread = new LogAllocateThread(storage_dir.canonicalPath(), manager->max_log_size());
	alloc_thread->start();
}

void LogStorage::sync()
{
	__sync_synchronize();
	if (!need_sync)
		return;
	QMutexLocker locker(&file_guard);
	LOG4CXX_DEBUG(logger, "Syncing storage");
	current_log.sync();
	need_sync = false;
}

void LogStorage::finished_allocation()
{
	delete alloc_thread;
	alloc_thread = 0;
}

void LogStorage::write(void* buffer, size_t size)
{
	static char padding_buffer[sizeof(uint64_t)] = { 0 };

	size_t rounded_size;
	uint64_t transaction, crc, timestamp_and_len;

	struct timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts);
	// This is the current time in tenths of a second
	uint32_t now = (uint32_t) (ts.tv_sec * 10 + ts.tv_nsec / 100000000);

	rounded_size = ((size + sizeof(uint64_t) - 1) / sizeof(uint64_t)) * sizeof(uint64_t);

	if (qint64(current_log.pos() + sizeof(transaction) + rounded_size + sizeof(crc)) >= current_log.size())
		get_next_file();

	transaction = LOG_ENDIAN(manager->transaction());
//	printf("tx: %lu\n", LOG_ENDIAN(transaction));
	timestamp_and_len = LOG_ENDIAN(now | (rounded_size << (sizeof(uint32_t) * 8)));
//	printf("ts: %x, size: %x, val: %lx\n", (uint32_t) now, (uint32_t) rounded_size, timestamp_and_len);
	crc = LOG_ENDIAN(lzma_crc64(reinterpret_cast<uint8_t*>(buffer), size, 0));
	current_log.write((const char*)&transaction, sizeof(transaction));
	current_log.write((const char*)&timestamp_and_len, sizeof(timestamp_and_len));
	if ((size_t)current_log.write((const char*)buffer, size) != size)
		LOG4CXX_ERROR(logger, "Short write on log. This will CORRUPT the log file!");
	if (rounded_size != size)
		current_log.write(padding_buffer, rounded_size - size);
	current_log.write((const char*)&crc, sizeof(crc));

	need_sync = true;
	LOG4CXX_DEBUG(logger, "Log transaction");
}

LogAllocateThread::LogAllocateThread(const QString& dir, size_t size) : dir(dir), size(size)
{
}

void LogAllocateThread::run()
{
	QDir storage_dir(dir);
	QTemporaryFile tmp_file(storage_dir.absoluteFilePath("asyncthrift.next.log"));
	tmp_file.setAutoRemove(false);
	tmp_file.open();

	if (posix_fallocate(tmp_file.handle(), 0, size))
		LOG4CXX_WARN(logger, "Error allocating space for next LogFile");
	tmp_file.rename(storage_dir.absoluteFilePath("asyncthrift.next.log"));
}

LogSyncThread::LogSyncThread(LogStorage* storage) : storage(storage)
{
	start();
	moveToThread(this);
}

LogSyncThread::~LogSyncThread()
{
}

void LogSyncThread::sync()
{
	storage->sync();
}

LogWriteThread::LogWriteThread(LogStorage* storage) : quitNow(false), buffer(AsyncThriftLogger::instance()->dispatcher()->buffer()), storage(storage)
{
	start();
	moveToThread(this);
}

LogWriteThread::~LogWriteThread()
{
}

void LogWriteThread::run()
{
	void* request;
	size_t request_size;
	unsigned long buf_transaction;
	char local_buffer[4096];

	while (true) {
		request = buffer->fetch_read(&request_size, &buf_transaction, RINGBUFFER_READ_TIMEOUT);
		if (!request) {
			QCoreApplication::processEvents();
			if (quitNow) {
				LOG4CXX_DEBUG(logger, "Exiting write thread");
				return;
			}
			continue;
		}
		
		if (request_size <= sizeof(local_buffer)) {
			memcpy(local_buffer, request, request_size);
			request = local_buffer;
			buffer->commit_read(buf_transaction);
		}

		storage->write(request, request_size);

		if (request_size > sizeof(local_buffer))
			buffer->commit_read(buf_transaction);
	}
}

void LogWriteThread::quit()
{
	quitNow = true;
}

LogReadThread::LogReadThread(QLocalSocket* socket, LogStorageManagerPrivate* manager) : socket(socket), manager(manager), stream(socket), transaction(0), read_context(0)
{
	moveToThread(this);
	socket->setParent(0);
	socket->moveToThread(this);
	socket->setParent(this);
}

void LogReadThread::run()
{
	LOG4CXX_DEBUG(logger, "Start read thread");
	connect(socket, SIGNAL(disconnected()), SLOT(quit()));
	connect(socket, SIGNAL(readyRead()), SLOT(handle_ready_read()));
	connect(socket, SIGNAL(bytesWritten(qint64)), SLOT(handle_bytes_written(qint64)));

	exec();

	socket->close();

	if (read_context)
		manager->end_read(read_context);
}

void LogReadThread::write_transactions()
{
	char* buffer;
	size_t size;
	int max_to_write = MAX_TRANSACTIONS_SENT_PER_LOOP;

	while (max_to_write-- && stream.device()->bytesToWrite() < MAX_CLIENT_BUFFER && manager->read_next_transaction(read_context, &buffer, &size)) {
		stream.writeBytes(buffer, size);
	}
}

void LogReadThread::handle_ready_read()
{
	uint64_t tx;
	
	if ((size_t)socket->bytesAvailable() < sizeof(tx))
		return;

	while ((size_t)socket->bytesAvailable() >= sizeof(tx))
		stream.readRawData((char*)& tx, sizeof(tx));
	tx = LOG_ENDIAN(tx);

	LOG4CXX_DEBUG(logger, qPrintable(QString("Pending transaction %1").arg(tx)));

	if (transaction == 0) {
		transaction = tx;
		read_context = manager->begin_read(transaction);
		write_transactions();
	}
}

void LogReadThread::handle_bytes_written(qint64 bytes)
{
	Q_UNUSED(bytes);
	write_transactions();
}

