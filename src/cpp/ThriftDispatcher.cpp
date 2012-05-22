#include "ThriftDispatcher.h"

#include <arpa/inet.h>

#include "Hbase.h"
#include <protocol/TBinaryProtocol.h>
#include <server/TNonblockingServer.h>
#include <concurrency/ThreadManager.h>
#include <concurrency/PosixThreadFactory.h>

#include "HBaseHandler.h"
#include "NBRingByteBuffer.h"
#include "LogStorageManager.h"

using namespace apache::thrift;
using namespace apache::thrift::protocol;
using namespace apache::thrift::server;
using namespace apache::thrift::concurrency;

using boost::shared_ptr;

using namespace apache::hadoop::hbase::thrift;

log4cxx::LoggerPtr ThriftDispatcher::logger(log4cxx::Logger::getLogger(ThriftDispatcher::staticMetaObject.className()));

class ThriftDispatcherPrivate {
	friend class ThriftDispatcher;

public:
	ThriftDispatcherPrivate() : running_(false), num_worker_threads_(4), port_(9090), _buffer(NBRingByteBuffer(64 * 1024 * 1024)) {}

	void run()
	{
		shared_ptr<HBaseHandler> handler(new HBaseHandler());
		shared_ptr<TProcessor> processor(new HbaseProcessor(handler));
		shared_ptr<TProtocolFactory> protocolFactory(new TBinaryProtocolFactory());
		shared_ptr<ThreadManager> thread_manager = ThreadManager::newSimpleThreadManager(num_worker_threads());

		shared_ptr<PosixThreadFactory> thread_factory(new PosixThreadFactory());
		thread_manager->threadFactory(thread_factory);
		thread_manager->start();

		server_ = shared_ptr<TNonblockingServer>(new TNonblockingServer(processor, protocolFactory, port(), thread_manager));
		running_ = true;
		server_->serve();
		running_ = false;
	}

	void stop()
	{
		server()->stop();
	}

	bool running() const { return running_; }
	size_t num_worker_threads() const { return num_worker_threads_; }
	void set_num_worker_threads(size_t num_worker_threads) { num_worker_threads_ = num_worker_threads; }
	unsigned int port() const { return port_; }
	void set_port(unsigned int port) { port_ = port; }
	shared_ptr<TNonblockingServer> server() const { return server_; }

private:
	bool running_;
	size_t num_worker_threads_;
	unsigned int port_;
	shared_ptr<TNonblockingServer> server_;
	NBRingByteBuffer _buffer;
	LogStorageManager _ls_manager;
};

ThriftDispatcher::ThriftDispatcher(QObject* parent) : QThread(parent), d(new ThriftDispatcherPrivate)
{
}

ThriftDispatcher::~ThriftDispatcher()
{
	delete d;
}

void ThriftDispatcher::run()
{
	d->run();
}

void ThriftDispatcher::stop()
{
	d->stop();
}

void ThriftDispatcher::set_port(unsigned int port)
{
	if (d->running() && port != d->port())
		LOG4CXX_WARN(logger, "Cannot change the binding port while running");

	d->set_port(port);
}

void ThriftDispatcher::set_num_worker_threads(size_t num_worker_threads)
{
	d->set_num_worker_threads(num_worker_threads);

	if (d->running()) {
		shared_ptr<ThreadManager> thread_manager = d->server()->getThreadManager();
		size_t current_workers = thread_manager->workerCount();
		if (num_worker_threads < current_workers)
			thread_manager->removeWorker(current_workers - num_worker_threads);
		else
			thread_manager->addWorker(num_worker_threads - current_workers);
	}
}

void ThriftDispatcher::set_buffer_size(size_t buffer_size)
{
	// TODO: implement
}

void ThriftDispatcher::configure_log_storage(unsigned int max_size, unsigned int period, const QStringList& log_dirs)
{
	d->_ls_manager.configure(max_size, period, log_dirs);
}

NBRingByteBuffer* ThriftDispatcher::buffer()
{
	return &d->_buffer;
}
