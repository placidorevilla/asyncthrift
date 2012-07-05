#ifndef FORWARDERMANAGER_P_H
#define FORWARDERMANAGER_P_H

#include "ForwarderManager.h"

#include "LogEndian.h"
#include "HBaseClient.h"

#include "TLogger.h"

#include <QThread>
#include <QLocalSocket>
#include <QDataStream>
#include <QFile>
#include <QFutureWatcher>
#include <QSet>
#include <QCircularBuffer>

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

private slots:
	void handle_connected();
	void handle_error(QLocalSocket::LocalSocketError socket_error);
	void handle_ready_read();
	void reconnect();

	void handle_finished();

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

	QDataStream tx_ptr_stream;
	QFile tx_ptr_file;

	QCircularBuffer<uint64_t> flying_txs;
	BatchRequests* delayed_batch;

	AsyncHBase::HBaseClient* hbase_client;

	ForwarderManager* q_ptr;
};

class BatchRequests : public QObject
{
	Q_OBJECT
	Q_DISABLE_COPY(BatchRequests)
	T_LOGGER_DECLARE(BatchRequests);

public:
	BatchRequests(size_t len, QObject* parent = 0) : QObject(parent), buffer_(len) {}
	~BatchRequests() {}

	char* buffer() { return buffer_.data(); }
	uint64_t transaction() const { return LOG_ENDIAN(*(uint64_t*) buffer_.data()); }
	uint32_t timestamp() const { return LOG_ENDIAN(*(((uint64_t*) buffer_.data()) + 1)); }

	bool send(AsyncHBase::HBaseClient* client);

signals:
	void finished();

private slots:
	void handle_finished();

private:
	QVector<char> buffer_;
	QSet<PendingRequest*> pending_requests;
};

class PendingRequest : public QFutureWatcher<void>
{
	Q_OBJECT
	Q_DISABLE_COPY(PendingRequest)

public:
	explicit PendingRequest(AsyncHBase::HBaseRpc* request = 0) : request_(request) {}
	~PendingRequest() {}

	AsyncHBase::HBaseRpc* request() { return request_; }
	void set_request(AsyncHBase::HBaseRpc* request) { request_ = request; }

private:
	AsyncHBase::HBaseRpc* request_;
};

#endif // FORWARDERMANAGER_P_H
