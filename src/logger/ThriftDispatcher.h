#ifndef THRIFTDISPATCHER_H
#define THRIFTDISPATCHER_H

#include <log4cxx/logger.h>

#include <QThread>

class ThriftDispatcherPrivate;
class NBRingByteBuffer;

class ThriftDispatcher : public QThread {
	Q_OBJECT

public:
	ThriftDispatcher(QObject* parent = 0);
	virtual ~ThriftDispatcher();

	void stop();

	void set_port(unsigned int port);
	void set_num_worker_threads(size_t num_worker_threads);
	void set_buffer_size(size_t buffer_size);

	void configure_log_storage(unsigned int max_size, unsigned int period, const QStringList& log_dirs);

	NBRingByteBuffer* buffer();

protected:
	virtual void run();

private:
	ThriftDispatcherPrivate* d;
	static log4cxx::LoggerPtr logger;
};

#endif // THRIFTDISPATCHER_H
