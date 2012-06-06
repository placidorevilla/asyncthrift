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

#include <lzma.h>

#include <fcntl.h>

log4cxx::LoggerPtr LogStorageManager::logger(log4cxx::Logger::getLogger(LogStorageManager::staticMetaObject.className()));
log4cxx::LoggerPtr LogStorageManagerPrivate::logger(log4cxx::Logger::getLogger(LogStorageManager::staticMetaObject.className()));
log4cxx::LoggerPtr LogStorage::logger(log4cxx::Logger::getLogger(LogStorage::staticMetaObject.className()));
log4cxx::LoggerPtr LogWriteThread::logger(log4cxx::Logger::getLogger(LogWriteThread::staticMetaObject.className()));
log4cxx::LoggerPtr LogSyncThread::logger(log4cxx::Logger::getLogger(LogSyncThread::staticMetaObject.className()));
log4cxx::LoggerPtr LogAllocateThread::logger(log4cxx::Logger::getLogger(LogAllocateThread::staticMetaObject.className()));

static const int RINGBUFFER_READ_TIMEOUT = 500;

LogStorageManager::LogStorageManager(QObject* parent) : QObject(parent), d(new LogStorageManagerPrivate(this)), cur_transaction(0)
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
	unsigned int next_log_index = 0;

	foreach(const QString& log_file, storage_dir_.entryList(QStringList() << "asyncthrift.[0-9][0-9][0-9][0-9].log", QDir::Files, QDir::NoSort))
		next_log_index = qMax(next_log_index, log_file.mid(12, 4).toUInt());
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
	char local_buffer[4096];
	char padding_buffer[sizeof(uint64_t)];
	uint64_t transaction;
	uint64_t crc;

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

		rounded_request_size = ((request_size + sizeof(uint64_t) - 1) / sizeof(uint64_t)) * sizeof(uint64_t);

		if (storage->current_log()->pos() + sizeof(transaction) + rounded_request_size + sizeof(crc) >= storage->manager()->max_log_size())
			storage->get_next_file();

		// TODO: test that we write the whole buffer
		transaction = LOG_ENDIAN((storage->manager()->transaction() * sizeof(uint64_t)) | (rounded_request_size / sizeof(uint64_t)));
		crc = LOG_ENDIAN(lzma_crc64(reinterpret_cast<uint8_t*>(request), request_size, 0));
		storage->current_log()->write((const char*)&transaction, sizeof(transaction));
		storage->current_log()->write((const char*)request, request_size);
		if (rounded_request_size != request_size)
			storage->current_log()->write(padding_buffer, rounded_request_size - request_size);
		storage->current_log()->write((const char*)&crc, sizeof(crc));
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

