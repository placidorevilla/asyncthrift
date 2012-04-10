#include <stdio.h>

#include "main.h"
#include "HBaseClient.h"

using namespace AsyncHBase;

MainApp::MainApp(int argc, char** argv) : QCoreApplication(argc, argv)
{
	this->hbase_client = new HBaseClient("localhost", "/hbase");

	printf("getFlushInterval: %d\n", hbase_client->getFlushInterval());
	printf("setFlushInterval: %d\n", hbase_client->setFlushInterval(500));
	printf("getFlushInterval: %d\n", hbase_client->getFlushInterval());
	printf("contendedMetaLookupCount: %ld\n", hbase_client->contendedMetaLookupCount());
	printf("uncontendedMetaLookupCount: %ld\n", hbase_client->uncontendedMetaLookupCount());
	printf("rootLookupCount: %ld\n", hbase_client->rootLookupCount());
}

int MainApp::run() {
	int timer = startTimer(0);
	exec();
	killTimer(timer);
	QFutureWatcher<void>* watcher = new QFutureWatcher<void>();
	connect(watcher, SIGNAL(finished()), this, SLOT(handleExit()));
	QFuture<void> f = hbase_client->shutdown();
	watcher->setFuture(f);
	exec();
	return 0;
}

void MainApp::timerEvent(QTimerEvent *event)
{
	Q_UNUSED(event);

	static int c = 10000;
	if (!--c)
		quit();

	QString key = QString::number(qrand());
	PutRequest* pr = new PutRequest("a", qPrintable(key), "c", "d", "e");
//	printf("key: %s\n", pr->key().data());

	pr->setBufferable(true);
	pr->setDurable(true);

	PendingRequest* watcher = new PendingRequest(pr);
	connect(watcher, SIGNAL(finished()), this, SLOT(handleFinished()));
	QFuture<void> f = hbase_client->put(*pr);
	watcher->setFuture(f);
//	hbase_client->shutdown().waitForFinished();
}

void MainApp::handleFinished()
{
	PendingRequest* watcher = qobject_cast<PendingRequest*>(sender());
	try {
//		printf("FINISHED\n");
		watcher->waitForFinished();
	} catch (HBaseException& e) {
		printf("E: [%s] %s\n", qPrintable(e.name()), qPrintable(e.message()));
	}

	delete watcher;
}

void MainApp::handleExit()
{
	quit();
}

PendingRequest::~PendingRequest()
{
	delete _pr;
}

int main(int argc, char **argv)
{
	MainApp app(argc, argv);
	return app.run();
}

