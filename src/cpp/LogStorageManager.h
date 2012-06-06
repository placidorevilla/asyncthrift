#ifndef LOGSTORAGEMANAGER_H
#define LOGSTORAGEMANAGER_H

#include <log4cxx/logger.h>

#include <QObject>

class LogStorageManagerPrivate;
class QSettings;
class QStringList;

namespace AsyncHBase {
	class HBaseClient;
}

class LogStorageManager : public QObject {
	Q_OBJECT
	Q_DISABLE_COPY(LogStorageManager)

public:
	explicit LogStorageManager(QObject* parent = 0);
	~LogStorageManager();

	bool configure(unsigned int max_log_size, unsigned int sync_period, const QStringList& dirs);
	uint64_t transaction() { return __sync_fetch_and_add(&cur_transaction, 1); }

	size_t max_log_size() const;
	AsyncHBase::HBaseClient* hbase_client();

private:
	LogStorageManagerPrivate* d;
	uint64_t cur_transaction;

	static log4cxx::LoggerPtr logger;
};

#endif // LOGSTORAGEMANAGER_H
