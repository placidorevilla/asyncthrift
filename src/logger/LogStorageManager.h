#ifndef LOGSTORAGEMANAGER_H
#define LOGSTORAGEMANAGER_H

#include "TLogger.h"

#include <QObject>

class LogStorageManagerPrivate;
class QStringList;

class LogStorageManager : public QObject {
	Q_OBJECT
	Q_DISABLE_COPY(LogStorageManager)
	T_LOGGER_DECLARE(LogStorageManager);

public:
	explicit LogStorageManager(QObject* parent = 0);
	~LogStorageManager();

	bool configure(unsigned int max_log_size, unsigned int sync_period, const QStringList& dirs);

private:
	LogStorageManagerPrivate* d;
};

#endif // LOGSTORAGEMANAGER_H
