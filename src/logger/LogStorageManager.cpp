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

#include "config.h"

log4cxx::LoggerPtr LogStorageManager::logger(log4cxx::Logger::getLogger(LogStorageManager::staticMetaObject.className()));
log4cxx::LoggerPtr LogStorageManagerPrivate::logger(log4cxx::Logger::getLogger(LogStorageManager::staticMetaObject.className()));
log4cxx::LoggerPtr LogStorage::logger(log4cxx::Logger::getLogger(LogStorage::staticMetaObject.className()));
log4cxx::LoggerPtr LogWriteThread::logger(log4cxx::Logger::getLogger(LogWriteThread::staticMetaObject.className()));
log4cxx::LoggerPtr LogSyncThread::logger(log4cxx::Logger::getLogger(LogSyncThread::staticMetaObject.className()));
log4cxx::LoggerPtr LogAllocateThread::logger(log4cxx::Logger::getLogger(LogAllocateThread::staticMetaObject.className()));
log4cxx::LoggerPtr LogReadThread::logger(log4cxx::Logger::getLogger(LogReadThread::staticMetaObject.className()));

static const int RINGBUFFER_READ_TIMEOUT = 500;

static const char* LOGGER_SOCKET_NAME = "logger";

LogStorageManager::LogStorageManager(QObject* parent) : QObject(parent), d(new LogStorageManagerPrivate(this))
{
}

LogStorageManager::~LogStorageManager()
{
	delete d;
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

size_t LogStorageManager::max_log_size() const
{
	return d->max_log_size();
}

LogStorageManagerPrivate::LogStorageManagerPrivate(LogStorageManager* manager) : QObject(manager), manager(manager), max_log_size_(0)
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
	// TODO: Check if the storages definition has changed actually
	if (storages.size() != 0) {
		LOG4CXX_WARN(logger, "Cannot reconfigure storages while running");
		return;
	}

	// TODO: Check that the dirs are not repeated
	foreach(const QString& dir, dirs) {
		LogStorage* storage = new LogStorage(manager, dir);
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

		LogReadThread* read_thread = new LogReadThread(logger_connection, manager);
		connect(read_thread, SIGNAL(finished()), SLOT(finished_logger_connection()));
		read_threads.append(read_thread);
		read_thread->start();
	}
}

void LogStorageManagerPrivate::finished_logger_connection()
{
	LogReadThread* read_thread(qobject_cast<LogReadThread*>(sender()));

	int item = read_threads.indexOf(read_thread);
	
	delete read_threads.at(item);
	read_threads.remove(item);

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

	storages.at((current_storage++) % nstorages)->schedule_sync();
}

LogStorage::LogStorage(LogStorageManager* manager, const QString& dir) : QObject(manager), storage_dir_(dir), write_thread_(new LogWriteThread(this)), sync_thread_(new LogSyncThread(this)), manager_(manager)
{
	get_next_file();
	sync_thread()->connect(this, SIGNAL(signal_sync()), SLOT(sync()));
}

LogStorage::~LogStorage()
{
	write_thread()->quit();
	write_thread()->wait();
	delete write_thread_;
	sync_thread()->quit();
	sync_thread()->wait();
	delete sync_thread_;

	current_log_.close();
}

void LogStorage::get_next_file()
{
	QMutexLocker locker(&file_guard);
	int next_log_index = -1;

	foreach(const QString& log_filename, storage_dir_.entryList(QStringList() << "asyncthrift.[0-9][0-9][0-9][0-9].log", QDir::Files, QDir::NoSort)) {
		int log_index = log_filename.mid(12, 4).toInt();
		next_log_index = qMax(next_log_index, log_index);
		if (file_to_transaction_map.contains(log_index))
			continue;
		QFile log_file(storage_dir_.absoluteFilePath(log_filename));
		log_file.open(QIODevice::ReadOnly);
		uint64_t transaction, first_good_transaction = 0, last_good_transaction = 0;
		uint64_t timestamp_and_len;
		if (log_file.read((char*)&transaction, sizeof(transaction)) != sizeof(transaction)) {
			LOG4CXX_WARN(logger, qPrintable(QString("Corrupt log file: '%1'").arg(log_file.fileName())));
		} else {
			first_good_transaction = transaction = LOG_ENDIAN(transaction);
			LOG4CXX_INFO(logger, qPrintable(QString("First good transaction for file '%1' is %2").arg(log_file.fileName()).arg(first_good_transaction)));
			while(transaction != 0) {
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
			LOG4CXX_INFO(logger, qPrintable(QString("Last good transaction for file '%1' is %2").arg(log_file.fileName()).arg(last_good_transaction)));
			file_to_transaction_map.insert(log_index, qMakePair(first_good_transaction, last_good_transaction));
			if (last_good_transaction != 0)
				manager()->transaction(last_good_transaction);
		}

		log_file.close();
	}
	next_log_index++;

	if (!storage_dir_.exists("asyncthrift.next.log")) {
		alloc_thread_ = new LogAllocateThread(this);
		alloc_thread_->start();
		alloc_thread_->wait();
	}

	if (current_log_.isOpen())
		current_log_.close();

	current_log_.setFileName(storage_dir_.absoluteFilePath("asyncthrift.next.log"));
	current_log_.rename(storage_dir_.absoluteFilePath(QString("asyncthrift.%1.log").arg(next_log_index, 4, 10, QChar('0'))));
	current_log_.open(QIODevice::ReadWrite);

	alloc_thread_ = new LogAllocateThread(this);
	alloc_thread_->start();
}

void LogStorage::schedule_sync()
{
	emit signal_sync();
}

void LogStorage::sync()
{
	QMutexLocker locker(&file_guard);
	LOG4CXX_DEBUG(logger, "Syncing storage");
	if (current_log_.handle() != -1)
		fdatasync(current_log_.handle());
}

void LogStorage::finished_allocation()
{
	delete alloc_thread_;
	alloc_thread_ = 0;
}

LogAllocateThread::LogAllocateThread(LogStorage* storage) : storage(storage)
{
}

void LogAllocateThread::run()
{
	QTemporaryFile tmp_file(storage->storage_dir()->absoluteFilePath("asyncthrift.next.log"));
	tmp_file.setAutoRemove(false);
	tmp_file.open();

	if (posix_fallocate(tmp_file.handle(), 0, storage->manager()->max_log_size()))
		LOG4CXX_WARN(logger, "Error allocating space for next LogFile");
	tmp_file.rename(storage->storage_dir()->absoluteFilePath("asyncthrift.next.log"));
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
	size_t request_size, rounded_request_size;
	unsigned long buf_transaction;
	void* request;
	char local_buffer[4];
	char padding_buffer[sizeof(uint64_t)];
	uint64_t transaction;
	uint64_t crc;
	uint64_t timestamp_and_len;

	memset(padding_buffer, 0, sizeof(padding_buffer));

	while (true) {
		time_t now = time(NULL);

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

		rounded_request_size = ((request_size + sizeof(uint64_t) - 1) / sizeof(uint64_t)) * sizeof(uint64_t);

		if (storage->current_log()->pos() + sizeof(transaction) + rounded_request_size + sizeof(crc) >= storage->manager()->max_log_size())
			storage->get_next_file();

		transaction = LOG_ENDIAN(storage->manager()->transaction());
//		printf("tx: %lu\n", LOG_ENDIAN(transaction));
		timestamp_and_len = LOG_ENDIAN(((uint32_t) now) | (rounded_request_size << (sizeof(uint32_t) * 8)));
//		printf("ts: %x, size: %x, val: %lx\n", (uint32_t) now, (uint32_t) rounded_request_size, timestamp_and_len);
		crc = LOG_ENDIAN(lzma_crc64(reinterpret_cast<uint8_t*>(request), request_size, 0));
		storage->current_log()->write((const char*)&transaction, sizeof(transaction));
		storage->current_log()->write((const char*)&timestamp_and_len, sizeof(timestamp_and_len));
		if ((size_t)storage->current_log()->write((const char*)request, request_size) != request_size)
			LOG4CXX_ERROR(logger, "Short write on log. This will CORRUPT the log file!");
		if (rounded_request_size != request_size)
			storage->current_log()->write(padding_buffer, rounded_request_size - request_size);
		storage->current_log()->write((const char*)&crc, sizeof(crc));
		storage->current_log()->flush();
		LOG4CXX_DEBUG(logger, "Log transaction");

#if 0
		DeserializableHBaseOperation::MutateRows mutations;
		mutations.deserialize(request);

		if (mutations.type() == HBaseOperation::CLASS_PUT_BATCH) {
			foreach (const DeserializableHBaseOperation::RowValues& row, mutations.rows()) {
				foreach (const DeserializableHBaseOperation::FamilyValues& family, row.second) {
					foreach (const DeserializableHBaseOperation::QualifierValue& qv, family.second) {
						PutRequest* pr = new PutRequest(mutations.table(), row.first, family.first, qv.qualifier(), qv.value(), qv.timestamp());

						pr->set_bufferable(true);
						pr->set_durable(true);

						storage->manager()->hbase_client()->put(*pr);
					}
				}
			}
		}
#endif
		if (request_size > sizeof(local_buffer))
			buffer->commit_read(buf_transaction);
	}
}

void LogWriteThread::quit()
{
	quitNow = true;
}

LogReadThread::LogReadThread(QLocalSocket* socket, LogStorageManager* manager) : socket(socket), manager(manager), stream(socket)
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

	exec();

	socket->close();
}

void LogReadThread::handle_ready_read()
{
	uint64_t transaction;
	
	if ((size_t)socket->bytesAvailable() < sizeof(transaction))
		return;

	disconnect(socket, SIGNAL(readyRead()), this, SLOT(handle_ready_read()));

	stream.readRawData((char*)& transaction, sizeof(transaction));
	transaction = LOG_ENDIAN(transaction);

	LOG4CXX_DEBUG(logger, qPrintable(QString("Pending transaction %1").arg(transaction)));

	// TODO: begin sending pending transactions until current
	// TODO: begin iterating on bytesWritten waiting for bytesToWrite to be below a certain value to not overload the write buffer
}

