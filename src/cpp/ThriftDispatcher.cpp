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
	void run(int port = 9090, int nthreads = 4)
	{
		shared_ptr<HBaseHandler> handler(new HBaseHandler());
		shared_ptr<TProcessor> processor(new HbaseProcessor(handler));
		shared_ptr<TProtocolFactory> protocolFactory(new TBinaryProtocolFactory());
		shared_ptr<ThreadManager> thread_manager = ThreadManager::newSimpleThreadManager(nthreads);

		shared_ptr<PosixThreadFactory> thread_factory(new PosixThreadFactory());
		thread_manager->threadFactory(thread_factory);
		thread_manager->start();

		_server = shared_ptr<TNonblockingServer>(new TNonblockingServer(processor, protocolFactory, port, thread_manager));
		_server->serve();
	}

	void stop()
	{
		_server->stop();
	}

private:
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

