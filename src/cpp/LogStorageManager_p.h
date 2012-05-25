#ifndef LOGSTORAGEMANAGER_P_H
#define LOGSTORAGEMANAGER_P_H

#include <log4cxx/logger.h>

#include <QThread>
#include <QFile>
#include <QTimer>
#include <QMutex>

class LogStorageManager;
class LogStorage;
class NBRingByteBuffer;

class LogWriteThread : public QThread {
	Q_OBJECT
	Q_DISABLE_COPY(LogWriteThread)

public:
	explicit LogWriteThread(LogStorage* storage);
	~LogWriteThread();

public slots:
	void quit();

protected:
	virtual void run();

private:
	bool quitNow;
	NBRingByteBuffer* buffer;
	LogStorage* storage;

	static log4cxx::LoggerPtr logger;
};

class LogSyncThread : public QThread {
	Q_OBJECT
	Q_DISABLE_COPY(LogSyncThread)

public:
	explicit LogSyncThread(LogStorage* storage);
	~LogSyncThread();

public slots:
	void sync();

private:
	LogStorage* storage;

	static log4cxx::LoggerPtr logger;
};

class LogStorage : public QObject {
	Q_OBJECT
	Q_DISABLE_COPY(LogStorage)

public:
	LogStorage(LogStorageManager* manager, const QString& dir);
	~LogStorage();

	LogWriteThread* write_thread() const { return write_thread_; }
	LogSyncThread* sync_thread() const { return sync_thread_; }

	void schedule_sync();
	void sync();

	int handle() const { return current_log.handle(); }
	LogStorageManager* manager() { return manager_; }

signals:
	void signal_sync();

private:
	QFile current_log;
	LogWriteThread* write_thread_;
	LogSyncThread* sync_thread_;
	LogStorageManager* manager_;
	QMutex file_guard;

	static log4cxx::LoggerPtr logger;
};

class LogStorageManagerPrivate : public QObject {
	Q_OBJECT
	Q_DISABLE_COPY(LogStorageManagerPrivate)

public:
	LogStorageManagerPrivate(LogStorageManager* manager);
	~LogStorageManagerPrivate();

	unsigned int max_log_size() const { return max_log_size_; }
	void set_max_log_size(unsigned int max_log_size) { max_log_size_ = max_log_size; }
	unsigned int sync_period() const { return sync_timer.interval() * storages.size(); }
	void set_sync_period(unsigned int sync_period);

	void create_storages(const QStringList& dirs);

private slots:
	void sync_timeout();

private:
	LogStorageManager* manager;
	unsigned int max_log_size_;
	QList<LogStorage*> storages;
	QTimer sync_timer;

	static log4cxx::LoggerPtr logger;
};

#endif // LOGSTORAGEMANAGER_P_H
