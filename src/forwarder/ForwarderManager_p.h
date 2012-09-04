#ifndef FORWARDERMANAGER_P_H
#define FORWARDERMANAGER_P_H

#include "ForwarderManager.h"

#include "LogEndian.h"
#include "HBaseClient.h"

#include "TLogger.h"

#include <QThread>
#include <QLocalSocket>
#include <QDataStream>
#include <QTextStream>
#include <QFile>
#include <QFutureWatcher>
#include <QSet>
#include <QCircularBuffer>

class QTimerEvent;

class BatchRequests;
class PendingRequest;

class ForwarderManagerPrivate : public QThread {
	Q_OBJECT
	Q_DECLARE_PUBLIC(ForwarderManager)
	T_LOGGER_DECLARE(ForwarderManagerPrivate);

	enum State { STATE_READ_LEN, STATE_READ_PAYLOAD, STATE_FORWARDING, STATE_STALLED, STATE_DELAYED };

public:
	ForwarderManagerPrivate(const QString& name, const QString& zquorum, unsigned int delay, unsigned int flush_interval, const QString& socket_name, ForwarderManager* parent = 0);
	~ForwarderManagerPrivate();

	virtual void run();
	Q_INVOKABLE void finish();

private slots:
	void handle_finished_client();
	void handle_connected();
	void handle_error(QLocalSocket::LocalSocketError socket_error);
	void handle_ready_read();
	void reconnect();

	void handle_finished(BatchRequests* batch);

	void handle_end_delay();

public:
	QString name;
	unsigned int delay;
	QString socket_name;

private:
	QLocalSocket socket;
	QDataStream stream;

	State state;
	quint32 len;

	QTextStream tx_ptr_stream;
	QFile tx_ptr_file;

	QCircularBuffer<uint64_t> flying_txs;
	BatchRequests* delayed_batch;

	AsyncHBase::HBaseClient* hbase_client;

	int next_worker;
	QList<QThread*> worker_threads;

	ForwarderManager* q_ptr;
};

class BatchRequests : public QObject
{
	Q_OBJECT
	Q_DISABLE_COPY(BatchRequests)
	T_LOGGER_DECLARE(BatchRequests);

public:
	BatchRequests(size_t len, AsyncHBase::HBaseClient* client, QObject* parent = 0) : QObject(parent), buffer_(len), client(client), failed_(false) {}
	virtual ~BatchRequests() {}

	bool failed() const { return failed_; }

	char* buffer() { return buffer_.data(); }
	uint64_t transaction() const { return LOG_ENDIAN(*(uint64_t*) buffer_.data()); }
	uint32_t timestamp() const { return LOG_ENDIAN(*(((uint64_t*) buffer_.data()) + 1)); }

public slots:
	bool send();

signals:
	void finished(BatchRequests* batch);

private slots:
	void handle_finished();

private:
	bool process(AsyncHBase::HBaseRpc::BatchProcessable* request);

	QVector<char> buffer_;
	QSet<PendingRequest*> pending_requests;
	AsyncHBase::HBaseClient* client;
	bool failed_;
};

class PendingRequest : public QFutureWatcher<void>
{
	Q_OBJECT
	Q_DISABLE_COPY(PendingRequest)
	T_LOGGER_DECLARE(PendingRequest);

public:
	explicit PendingRequest(AsyncHBase::HBaseClient* client, AsyncHBase::HBaseRpc::BatchProcessable* request = 0) : client_(client), request_(request), delay(0), tries(1) {}
	~PendingRequest() {}

	AsyncHBase::HBaseRpc::BatchProcessable* request() { return request_; }
	void set_request(AsyncHBase::HBaseRpc::BatchProcessable* request) { request_ = request; }

	void process();
	bool retry(int delay = 0, int limit = 10);

protected:
	void timerEvent(QTimerEvent *event);

private:
	AsyncHBase::HBaseClient* client_;
	AsyncHBase::HBaseRpc::BatchProcessable* request_;
	int delay;
	int tries;
};

#endif // FORWARDERMANAGER_P_H
