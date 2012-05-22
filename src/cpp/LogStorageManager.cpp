#include "LogStorageManager.h"
#include "LogStorageManager_p.h"

#include "AsyncThrift.h"
#include "ThriftDispatcher.h"
#include "NBRingByteBuffer.h"

#include <QSettings>
#include <QDir>
#include <QMutexLocker>

static const int RINGBUFFER_READ_TIMEOUT = 500;

LogStorageManager::LogStorageManager(QObject* parent) : QObject(parent), d(new LogStorageManagerPrivate(this))
{
}

LogStorageManager::~LogStorageManager()
{
	delete d;
}

bool LogStorageManager::configure(unsigned int max_log_size, unsigned int sync_period, const QStringList& dirs)
{
	d->create_storages(dirs);
	d->set_max_log_size(max_log_size);
	d->set_sync_period(sync_period);

	return true;
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
	if (storages.size() != 0) {
		// TODO: Warn, cannot reconfigure storages while running
		return;
	}

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

LogStorage::LogStorage(LogStorageManager* manager, const QString& dir) : QObject(manager), write_thread_(new LogWriteThread(this)), sync_thread_(new LogSyncThread(this)), manager(manager)
{
	QDir log_dir(dir);
	unsigned int next_log_index = 0;

	foreach(const QString& log_file, log_dir.entryList(QStringList() << "asyncthrift.[0-9][0-9][0-9][0-9].log", QDir::Files, QDir::NoSort))
		next_log_index = qMax(next_log_index, log_file.mid(12, 4).toUInt());
	next_log_index++;

	current_log.setFileName(log_dir.absoluteFilePath(QString("asyncthrift.%1.log").arg(next_log_index, 4, 10, QChar('0'))));
	current_log.open(QIODevice::WriteOnly);

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

	current_log.close();
}

void LogStorage::schedule_sync()
{
	emit signal_sync();
}

void LogStorage::sync()
{
	QMutexLocker locker(&file_guard);
//	printf("sync! %p\n", (void*)pthread_self());
	fdatasync(current_log.handle());
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

LogWriteThread::LogWriteThread(LogStorage* storage) : quitNow(false), buffer(AsyncThrift::instance()->dispatcher()->buffer()), storage(storage)
{
	start();
	moveToThread(this);
}

LogWriteThread::~LogWriteThread()
{
}

void LogWriteThread::run()
{
	size_t request_size;
	unsigned long transaction;
	void* request;

	while (true) {
		request = buffer->fetch_read(&request_size, &transaction, RINGBUFFER_READ_TIMEOUT);
		if (!request) {
			QCoreApplication::processEvents();
			if (quitNow)
				return;
			continue;
		}
		// TODO: write operation to disk
		buffer->commit_read(transaction);
	}
}

void LogWriteThread::quit()
{
	quitNow = true;
}

