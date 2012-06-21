#ifndef FORWARDERMANAGER_P_H
#define FORWARDERMANAGER_P_H

#include "ForwarderManager.h"

#include "LogEndian.h"
#include "HBaseClient.h"

#include <log4cxx/logger.h>

#include <QThread>
#include <QLocalSocket>
#include <QDataStream>
#include <QFile>
#include <QFutureWatcher>
#include <QCircularBuffer>

class BatchRequests;

class ForwarderManagerPrivate : public QThread {
	Q_OBJECT
	Q_DECLARE_PUBLIC(ForwarderManager)

	enum State { STATE_READ_LEN, STATE_READ_PAYLOAD, STATE_FORWARDING, STATE_STALLED, STATE_DELAYED };

public:
	ForwarderManagerPrivate(const QString& name, const QString& zquorum, unsigned int delay, ForwarderManager* parent = 0);
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
	QString zquorum;
	unsigned int delay;

private:
	QLocalSocket socket;
	QDataStream stream;

	State state;
	quint32 len;

	QDataStream tx_ptr_stream;
	QFile tx_ptr_file;

	QCircularBuffer<uint64_t> flying_txs;
	BatchRequests* delayed_batch;

	ForwarderManager* q_ptr;

	static log4cxx::LoggerPtr logger;
};

class BatchRequests : public QObject
{
	Q_OBJECT
	Q_DISABLE_COPY(BatchRequests)

public:
	BatchRequests(size_t len, QObject* parent = 0) : QObject(parent), buffer_(len) {}
	~BatchRequests() {}

	char* buffer() { return buffer_.data(); }
	uint64_t transaction() const { return LOG_ENDIAN(*(uint64_t*) buffer_.data()); }
	uint32_t timestamp() const { return LOG_ENDIAN(*(((uint64_t*) buffer_.data()) + 1)); }

	bool parse();

signals:
	void finished();

private slots:
	void handle_finished();

private:
	QVector<char> buffer_;
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
