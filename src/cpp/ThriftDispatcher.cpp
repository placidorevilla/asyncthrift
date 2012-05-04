#include "ThriftDispatcher.h"

#include <arpa/inet.h>

#include "Hbase.h"
#include <protocol/TBinaryProtocol.h>
#include <server/TNonblockingServer.h>
#include <concurrency/ThreadManager.h>
#include <concurrency/PosixThreadFactory.h>

#include "HBaseHandler.h"

using namespace apache::thrift;
using namespace apache::thrift::protocol;
using namespace apache::thrift::server;
using namespace apache::thrift::concurrency;

using boost::shared_ptr;

using namespace apache::hadoop::hbase::thrift;

class ThriftDispatcherPrivate {
	friend class ThriftDispatcher;

public:
	ThriftDispatcherPrivate() : _running(false), _workerThreads(4), _port(9090) {}

	void run()
	{
		shared_ptr<HBaseHandler> handler(new HBaseHandler());
		shared_ptr<TProcessor> processor(new HbaseProcessor(handler));
		shared_ptr<TProtocolFactory> protocolFactory(new TBinaryProtocolFactory());
		shared_ptr<ThreadManager> thread_manager = ThreadManager::newSimpleThreadManager(_workerThreads);

		shared_ptr<PosixThreadFactory> thread_factory(new PosixThreadFactory());
		thread_manager->threadFactory(thread_factory);
		thread_manager->start();

		_server = shared_ptr<TNonblockingServer>(new TNonblockingServer(processor, protocolFactory, _port, thread_manager));
		_running = true;
		_server->serve();
		_running = false;
	}

	void stop()
	{
		_server->stop();
	}

private:
	bool _running;
	size_t _workerThreads;
	unsigned int _port;
	shared_ptr<TNonblockingServer> _server;
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

void ThriftDispatcher::setPort(unsigned int port)
{
	if (d->_running && port != d->_port) {
		// TODO: warning, cannot change port while running
	}

	d->_port = port;
}

void ThriftDispatcher::setWorkerThreads(size_t nWorkers)
{
	d->_workerThreads = nWorkers;

	if (d->_running) {
		shared_ptr<ThreadManager> thread_manager = d->_server->getThreadManager();
		size_t current_workers = thread_manager->workerCount();
		if (nWorkers < current_workers)
			thread_manager->removeWorker(current_workers - nWorkers);
		else
			thread_manager->addWorker(nWorkers - current_workers);
	}
}
