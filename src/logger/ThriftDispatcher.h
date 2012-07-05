#ifndef THRIFTDISPATCHER_H
#define THRIFTDISPATCHER_H

#include "TLogger.h"

#include <QThread>

class ThriftDispatcherPrivate;
class NBRingByteBuffer;

class ThriftDispatcher : public QThread {
	Q_OBJECT
	Q_DISABLE_COPY(ThriftDispatcher)
	T_LOGGER_DECLARE(ThriftDispatcher);

public:
	ThriftDispatcher(QObject* parent = 0);
	virtual ~ThriftDispatcher();

	void stop();

	void set_port(unsigned int port);
	void set_num_worker_threads(size_t num_worker_threads);
	void set_buffer_size(size_t buffer_size);

	void configure_log_storage(unsigned int max_size, unsigned int period, const QStringList& log_dirs, const QString& socket);

	NBRingByteBuffer* buffer();

protected:
	virtual void run();

private:
	ThriftDispatcherPrivate* d;
};

#endif // THRIFTDISPATCHER_H
