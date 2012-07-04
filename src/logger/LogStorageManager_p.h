#ifndef LOGSTORAGEMANAGER_P_H
#define LOGSTORAGEMANAGER_P_H

#include "TMemFile.h"

#include "TLogger.h"

#include <QThread>
#include <QFile>
#include <QTimer>
#include <QMutex>
#include <QDir>
#include <QLocalServer>

class LogStorageManagerPrivate;
class LogStorage;
class NBRingByteBuffer;
class LogReadContext;
class StorageReadContext;

class LogWriteThread : public QThread {
	Q_OBJECT
	Q_DISABLE_COPY(LogWriteThread)
	T_LOGGER_DECLARE(LogWriteThread);

public:
	explicit LogWriteThread(LogStorage* storage);

public slots:
	void quit();

protected:
	virtual void run();

private:
	bool quitNow;
	NBRingByteBuffer* buffer;
	LogStorage* storage;
};

class LogSyncThread : public QThread {
	Q_OBJECT
	Q_DISABLE_COPY(LogSyncThread)
	T_LOGGER_DECLARE(LogSyncThread);

public:
	explicit LogSyncThread(LogStorage* storage);

public slots:
	void sync();

private:
	LogStorage* storage;
};

class LogAllocateThread : public QThread {
	Q_OBJECT
	Q_DISABLE_COPY(LogAllocateThread)
	T_LOGGER_DECLARE(LogAllocateThread);

public:
	LogAllocateThread(const QString& dir, size_t size) : dir(dir), size(size)
	{}

protected:
	virtual void run();

private:
	QString dir;
	size_t size;
};

class LogReadThread : public QThread {
	Q_OBJECT
	Q_DISABLE_COPY(LogReadThread)
	T_LOGGER_DECLARE(LogReadThread);

public:
	LogReadThread(QLocalSocket* socket, LogStorageManagerPrivate* manager);

protected:
	virtual void run();

private slots:
	void write_transactions();
	void handle_ready_read();
	void handle_bytes_written(qint64 bytes);

private:
	QLocalSocket* socket;
	LogStorageManagerPrivate* manager;
	QDataStream stream;
	uint64_t transaction;
	LogReadContext *read_context;
};

class LogReadaheadThread : public QThread {
	Q_OBJECT
	Q_DISABLE_COPY(LogReadaheadThread)
	T_LOGGER_DECLARE(LogReadaheadThread);

public:
	explicit LogReadaheadThread(int fd, QObject* parent = 0) : QThread(), fd(fd)
	{
		if (parent) {
			moveToThread(parent->thread());
			setParent(parent);
		}
		start();
	}

	~LogReadaheadThread()
	{
		deleteLater();
	}

protected:
	virtual void run();

private:
	int fd;
};

class LogReadContext {
public:
	LogReadContext(LogStorageManagerPrivate* manager, const QList<StorageReadContext*>& storage_contexts);
	~LogReadContext();

	bool read_next_transaction(char** buffer, size_t* size);

private:
	LogStorageManagerPrivate* manager;
	QList<StorageReadContext*> storage_contexts;
};

class StorageReadContext {
	T_LOGGER_DECLARE(StorageReadContext);

public:
	StorageReadContext(LogStorage* storage, int index, TMemFile* file);
	~StorageReadContext();

	bool peek_next_transaction(char** buffer, size_t* size);
	bool advance(uint64_t transaction = 0);

private:
	LogStorage* storage;
	int index;
	TMemFile* file;
};

class LogStorage : public QObject {
	Q_OBJECT
	Q_DISABLE_COPY(LogStorage)
	T_LOGGER_DECLARE(LogStorage);

	friend class LogSyncThread;
	friend class LogWriteThread;

public:
	LogStorage(LogStorageManagerPrivate* manager, const QString& dir);
	~LogStorage();

	QString path() const { return storage_dir.path(); }

	StorageReadContext* begin_read(uint64_t transaction);
	TMemFile* advance_next_file(int* index);

private:
	void map_log_file(int log_index, bool only_start = false);
	void get_next_file();
	void write(void* buffer, size_t size);
	TMemFile* open_log_file(int index);

private slots:
	void sync();
	void finished_allocation();

private:
	QDir storage_dir;
	TMemFile current_log;
	int current_index;
	uint64_t first_transaction;
	uint64_t last_transaction;
	QMutex file_guard;
	QMap<int, QPair<uint64_t, uint64_t> > file_to_transaction_map;
	QSet<int> invalid_files_map;
	LogWriteThread* write_thread;
	LogSyncThread* sync_thread;
	LogAllocateThread* alloc_thread;
	LogStorageManagerPrivate* manager;
	int32_t need_sync;
};

class LogStorageManagerPrivate : public QObject {
	Q_OBJECT
	Q_DISABLE_COPY(LogStorageManagerPrivate)
	T_LOGGER_DECLARE(LogStorageManagerPrivate);

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
};

#endif // LOGSTORAGEMANAGER_P_H
