#ifndef HBASEHANDLER_P_H
#define HBASEHANDLER_P_H

#include "HBaseClient.h"

#include <QObject>
#include <QFutureWatcher>

class HBaseHandlerPrivate : public QObject {
	Q_OBJECT
	friend class HBaseHandler;

public:
	HBaseHandlerPrivate();
	virtual ~HBaseHandlerPrivate();

public slots:
	void handleFinished();

private:
	AsyncHBase::HBaseClient* hbase_client;
};

class PendingRequest : public QFutureWatcher<void>
{
	Q_OBJECT

public:
	PendingRequest(AsyncHBase::PutRequest* pr) : QFutureWatcher<void>(), _pr(pr) {}
	~PendingRequest();

private:
	AsyncHBase::PutRequest* _pr;
};

#endif // HBASEHANDLER_P_H
