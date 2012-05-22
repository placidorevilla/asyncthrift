#ifndef HBASEHANDLER_P_H
#define HBASEHANDLER_P_H

#include "HBaseClient.h"

#include <QObject>
#include <QFutureWatcher>

class HBaseHandlerPrivate : public QObject {
	Q_OBJECT
	Q_DISABLE_COPY(HBaseHandlerPrivate)

//	friend class HBaseHandler;

public:
	HBaseHandlerPrivate();
	virtual ~HBaseHandlerPrivate();

	AsyncHBase::HBaseClient* hbase_client() const { return hbase_client_; }

public slots:
	void handleFinished();

private:
	AsyncHBase::HBaseClient* hbase_client_;
};

class PendingRequest : public QFutureWatcher<void>
{
	Q_OBJECT
	Q_DISABLE_COPY(PendingRequest)

public:
	explicit PendingRequest(AsyncHBase::PutRequest* put_request);
	~PendingRequest();

private:
	AsyncHBase::PutRequest* put_request;
};

#endif // HBASEHANDLER_P_H
