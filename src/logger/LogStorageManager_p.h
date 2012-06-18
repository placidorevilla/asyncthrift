#ifndef LOGSTORAGEMANAGER_P_H
#define LOGSTORAGEMANAGER_P_H

#include "TMemFile.h"

#include <log4cxx/logger.h>

#include <QThread>
#include <QFile>
#include <QTimer>
#include <QMutex>
#include <QDir>
#include <QLocalServer>

class LogStorageManagerPrivate;
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

class LogAllocateThread : public QThread {
	Q_OBJECT
	Q_DISABLE_COPY(LogAllocateThread)

public:
	LogAllocateThread(const QString& dir, size_t size);

protected:
	virtual void run();

private:
	QString dir;
	size_t size;

	static log4cxx::LoggerPtr logger;
};

class LogReadThread : public QThread {
	Q_OBJECT
	Q_DISABLE_COPY(LogReadThread)

public:
	LogReadThread(QLocalSocket* socket, LogStorageManagerPrivate* manager);

protected:
	virtual void run();

private slots:
	void handle_ready_read();

private:
	QLocalSocket* socket;
	LogStorageManagerPrivate* manager;
	QDataStream stream;
	uint64_t transaction;

	static log4cxx::LoggerPtr logger;
};

class LogReadContext {
public:
	explicit LogReadContext(LogStorageManagerPrivate* manager) : manager(manager) {}

	bool read_next_transaction(char** buffer, size_t* size);

private:
	LogStorageManagerPrivate* manager;
};

class LogStorage : public QObject {
	Q_OBJECT
	Q_DISABLE_COPY(LogStorage)

	friend class LogSyncThread;
	friend class LogWriteThread;

public:
	LogStorage(LogStorageManagerPrivate* manager, const QString& dir);
	~LogStorage();

	QString path() const { return storage_dir.path(); }

private:
	void map_log_file(int log_index);
	void get_next_file();
	void write(void* buffer, size_t size);

private slots:
	void sync();
	void finished_allocation();

private:
	QDir storage_dir;
	TMemFile current_log;
	QMutex file_guard;
	QMap<int, QPair<uint64_t, uint64_t> > file_to_transaction_map;
	QSet<int> invalid_files_map;
	LogWriteThread* write_thread;
	LogSyncThread* sync_thread;
	LogAllocateThread* alloc_thread;
	LogStorageManagerPrivate* manager;
	int32_t need_sync;

	static log4cxx::LoggerPtr logger;
};

class LogStorageManagerPrivate : public QObject {
	Q_OBJECT
	Q_DISABLE_COPY(LogStorageManagerPrivate)

public:
	LogStorageManagerPrivate(QObject* parent);
	~LogStorageManagerPrivate();

	unsigned int max_log_size() const { return max_log_size_; }
	void set_max_log_size(unsigned int max_log_size) { max_log_size_ = max_log_size; }
	unsigned int sync_period() const { return sync_timer.interval() * storages.size(); }
	void set_sync_period(unsigned int sync_period);

	void create_storages(const QStringList& dirs);
	void listen();

	uint64_t transaction(uint64_t new_transaction = 0);

	LogReadContext* begin_read(uint64_t transaction);
	void end_read(LogReadContext* context);
	bool read_next_transaction(LogReadContext* context, char** buffer, size_t* size);

private slots:
	void sync_timeout();
	void new_logger_connection();
	void finished_logger_connection();

private:
	unsigned int max_log_size_;
	QList<LogStorage*> storages;
	QTimer sync_timer;
	QLocalServer logger_server;
	QVector<LogReadThread*> read_threads;

	static log4cxx::LoggerPtr logger;
};

#endif // LOGSTORAGEMANAGER_P_H
