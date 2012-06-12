#ifndef LOGSTORAGEMANAGER_H
#define LOGSTORAGEMANAGER_H

#include <log4cxx/logger.h>

#include <QObject>

class LogStorageManagerPrivate;
class QSettings;
class QStringList;

class LogStorageManager : public QObject {
	Q_OBJECT
	Q_DISABLE_COPY(LogStorageManager)

public:
	explicit LogStorageManager(QObject* parent = 0);
	~LogStorageManager();

	bool configure(unsigned int max_log_size, unsigned int sync_period, const QStringList& dirs);
	uint64_t transaction(uint64_t new_transaction = 0);

	size_t max_log_size() const;

private:
	LogStorageManagerPrivate* d;

	static log4cxx::LoggerPtr logger;
};

inline uint64_t LogStorageManager::transaction(uint64_t new_transaction)
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

#endif // LOGSTORAGEMANAGER_H
