#include "HBaseHandler.h"
#include "HBaseHandler_p.h"

#include "HBaseClient.h"

using namespace AsyncHBase;

HBaseHandlerPrivate::HBaseHandlerPrivate() : hbase_client_(new AsyncHBase::HBaseClient("localhost"))
{
}

HBaseHandlerPrivate::~HBaseHandlerPrivate()
{
	delete hbase_client_;
}

void HBaseHandlerPrivate::handleFinished()
{
	PendingRequest* watcher = qobject_cast<PendingRequest*>(sender());
	try {
		printf("FINISHED\n");
		watcher->waitForFinished();
	} catch (HBaseException& e) {
		printf("E: [%s] %s\n", qPrintable(e.name()), qPrintable(e.message()));
	}

	delete watcher;
}

PendingRequest::PendingRequest(AsyncHBase::PutRequest* put_request) : QFutureWatcher<void>(), put_request(put_request)
{
}

PendingRequest::~PendingRequest()
{
	delete put_request;
}

HBaseHandler::HBaseHandler() : d(new HBaseHandlerPrivate)
{
}

HBaseHandler::~HBaseHandler()
{
	delete d;
}

static QByteArray QBA(const std::string& s)
{
	return QByteArray(s.data(), s.size());
}

void HBaseHandler::mutateRow(const Text& tableName, const Text& row, const std::vector<Mutation> & mutations) {
//	printf("handler:%p, thread: %p\n", this, (void*)pthread_self());
	for(std::vector<Mutation>::const_iterator i = mutations.begin(); i != mutations.end(); i++) {
//		printf("mutation\n");
		size_t colon_pos = i->column.find_first_of(':');
		if (colon_pos == std::string::npos)
			throw TException("Invalid column");
		std::string family = i->column.substr(0, colon_pos);
		std::string qualifier = i->column.substr(colon_pos + 1);
		PutRequest* pr = new PutRequest(QBA(tableName), QBA(row), QBA(family), QBA(qualifier), QBA(i->value));

		pr->set_bufferable(true);
		pr->set_durable(true);

		PendingRequest* watcher = new PendingRequest(pr);
		d->connect(watcher, SIGNAL(finished()), d, SLOT(handleFinished()));
		QFuture<void> f = d->hbase_client()->put(*pr);
		watcher->setFuture(f);
	}
//	throw TException("Not implemented");
	return;
}
