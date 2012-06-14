#ifndef LOGSTORAGEMANAGER_H
#define LOGSTORAGEMANAGER_H

#include <log4cxx/logger.h>

#include <QObject>

class LogStorageManagerPrivate;
class QStringList;

class LogStorageManager : public QObject {
	Q_OBJECT
	Q_DISABLE_COPY(LogStorageManager)

public:
	explicit LogStorageManager(QObject* parent = 0);
	~LogStorageManager();

	bool configure(unsigned int max_log_size, unsigned int sync_period, const QStringList& dirs);

private:
	LogStorageManagerPrivate* d;

	static log4cxx::LoggerPtr logger;
};

#endif // LOGSTORAGEMANAGER_H
