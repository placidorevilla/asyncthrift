#include "ForwarderManager.h"
#include "ForwarderManager_p.h"

#include "HBaseOperations.h"

#include "config.h"

#include <QDir>
#include <QTimer>
#include <QThread>
#include <QTimerEvent>

using namespace AsyncHBase;

T_QLOGGER_DEFINE_ROOT(ForwarderManager);
T_QLOGGER_DEFINE_OTHER_ROOT(ForwarderManagerPrivate, ForwarderManager);
T_QLOGGER_DEFINE_ROOT(BatchRequests);
T_QLOGGER_DEFINE_ROOT(PendingRequest);

static uint64_t TX_DONE_MASK = 1ULL << 63;
static uint64_t TX_FLYING_MASK = ~(1ULL << 63);

static int RECONNECT_TIME = 1000;
static int MAX_FLYING_TX = 4096;
static quint64 MAX_SOCKET_BUFFER_SIZE = 4 * 1024 * 1024;

ForwarderManagerPrivate::ForwarderManagerPrivate(const QString& name, const QString& zquorum, unsigned int delay, unsigned int flush_interval, const QString& socket_name, ForwarderManager* parent) : QThread(), name(name), delay(delay), socket_name(socket_name), stream(&socket), state(STATE_READ_LEN), len(0), flying_txs(MAX_FLYING_TX), hbase_client(new HBaseClient(zquorum.toAscii(), "/hbase")), next_worker(0), q_ptr(parent)
{
	QDir state_dir(PKGSTATEDIR);

	hbase_client->setFlushInterval(flush_interval);
	socket.moveToThread(this);
	socket.setReadBufferSize(MAX_SOCKET_BUFFER_SIZE);
	// TODO: this path should be config
	tx_ptr_file.setFileName(state_dir.absoluteFilePath(QString("forwarder_%1.ptr").arg(name)));
	tx_ptr_file.open(QIODevice::ReadWrite);
	tx_ptr_stream.setDevice(&tx_ptr_file);
	tx_ptr_stream.setIntegerBase(10);

	for (int i = 0; i < QThread::idealThreadCount(); i++) {
		QThread* thread = new QThread(this);
		worker_threads.append(thread);
		thread->start();
	}

	moveToThread(this);
	start();
}

ForwarderManagerPrivate::~ForwarderManagerPrivate()
{
}

void ForwarderManagerPrivate::run()
{
	QTimer::singleShot(0, this, SLOT(reconnect()));
	connect(&socket, SIGNAL(connected()), SLOT(handle_connected()));
	connect(&socket, SIGNAL(error(QLocalSocket::LocalSocketError)), SLOT(handle_error(QLocalSocket::LocalSocketError)));
	connect(&socket, SIGNAL(readyRead()), SLOT(handle_ready_read()));
	exec();
}

void ForwarderManagerPrivate::reconnect()
{
	socket.connectToServer(socket_name);
}

void ForwarderManagerPrivate::handle_connected()
{
	quint64 transaction;
	
	tx_ptr_stream.seek(0);
	tx_ptr_stream >> transaction;
	tx_ptr_stream.seek(0);
	if (tx_ptr_stream.status() != QTextStream::Ok) {
		transaction = 1;
		tx_ptr_stream.resetStatus();
		tx_ptr_stream << transaction;
		tx_ptr_stream.flush();
		tx_ptr_file.resize(tx_ptr_stream.pos());
		tx_ptr_stream.seek(0);
	}
	stream.resetStatus();
	stream.writeRawData((char *)&transaction, sizeof(transaction));
}

void ForwarderManagerPrivate::handle_error(QLocalSocket::LocalSocketError socket_error)
{
	Q_UNUSED(socket_error);
	socket.close();
	QTimer::singleShot(RECONNECT_TIME, this, SLOT(reconnect()));
}

void ForwarderManagerPrivate::handle_ready_read()
{
	BatchRequests* batch = 0;
	struct timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts);
	// This is the current time in tenths of a second
	uint32_t now = (uint32_t) (ts.tv_sec * 10 + ts.tv_nsec / 100000000);

	while (true) {
		if (state == STATE_READ_LEN) {
			if ((size_t)socket.bytesAvailable() < sizeof(len))
				return;

			stream >> len;
			state = STATE_READ_PAYLOAD;
		}

		if (state == STATE_READ_PAYLOAD) {
			if ((size_t)socket.bytesAvailable() < len)
				return;

			if (flying_txs.isFull()) {
				disconnect(&socket, SIGNAL(readyRead()), this, SLOT(handle_ready_read()));
				state = STATE_STALLED;
				return;
			}

			batch = new BatchRequests(len, hbase_client);
			stream.readRawData(batch->buffer(), len);
//			TDEBUG("Received tx: %ld, len: %ld", batch->transaction(), len)
			flying_txs.push_back(batch->transaction() & TX_FLYING_MASK);
			if ((now - batch->timestamp()) < (delay * 10)) {
				TDEBUG("Delaying %d seconds...", (delay * 10 - now + batch->timestamp()) / 10);
				delayed_batch = batch;
				QTimer::singleShot((delay * 10 - now + batch->timestamp()) * 100, this, SLOT(handle_end_delay()));
				disconnect(&socket, SIGNAL(readyRead()), this, SLOT(handle_ready_read()));
				state = STATE_DELAYED;
				return;
			} else {
				state = STATE_FORWARDING;
			}
		}

		if (state == STATE_STALLED) {
			if (flying_txs.isFull())
				return;

			state = STATE_READ_PAYLOAD;
		}

		if (state == STATE_DELAYED) {
			batch = delayed_batch;
			state = STATE_FORWARDING;
		}

		if (state == STATE_FORWARDING) {
			QThread* thread = worker_threads.at(next_worker);
			next_worker = (next_worker + 1) % worker_threads.size();
			batch->moveToThread(thread);
			connect(batch, SIGNAL(finished(BatchRequests*)), SLOT(handle_finished(BatchRequests*)));
			QMetaObject::invokeMethod(batch, "send", Qt::QueuedConnection);

			state = STATE_READ_LEN;
		}
	}
}

void ForwarderManagerPrivate::handle_finished(BatchRequests* batch)
{
	uint64_t transaction = batch->transaction() & TX_FLYING_MASK;

	int tx_idx = flying_txs.indexOf(transaction);
	flying_txs[tx_idx] = transaction | TX_DONE_MASK;
	if (flying_txs.at(tx_idx) == flying_txs.front()) {
		auto i = flying_txs.begin();
		while (i != flying_txs.end() && (*i & TX_DONE_MASK))
			i++;
		if (i == flying_txs.end()) {
			transaction = (*(i - 1) & TX_FLYING_MASK) + 1;
		} else {
			transaction = *i & TX_FLYING_MASK;
		}
		flying_txs.erase(flying_txs.begin(), i);
		tx_ptr_stream << (quint64) transaction;
		tx_ptr_stream.flush();
		tx_ptr_file.resize(tx_ptr_stream.pos());
		tx_ptr_stream.seek(0);
		stream.writeRawData((char *)&transaction, sizeof(transaction));
		TDEBUG("Sync transaction %lu", transaction);

		if (state == STATE_STALLED) {
			connect(&socket, SIGNAL(readyRead()), SLOT(handle_ready_read()));
			QMetaObject::invokeMethod(this, "handle_ready_read", Qt::QueuedConnection);
		}
	}

	batch->deleteLater();
}

void ForwarderManagerPrivate::handle_end_delay()
{
	connect(&socket, SIGNAL(readyRead()), SLOT(handle_ready_read()));
	handle_ready_read();
}

void ForwarderManagerPrivate::finish()
{
	disconnect(&socket);
	socket.disconnectFromServer();
	QFutureWatcher<void>* watcher = new QFutureWatcher<void>;
	connect(watcher, SIGNAL(finished()), this, SLOT(handle_finished_client()));
	disconnect(&socket, SIGNAL(readyRead()), this, SLOT(handle_ready_read()));
	watcher->setFuture(hbase_client->shutdown());
}

void ForwarderManagerPrivate::handle_finished_client()
{
	delete sender();
	quit();
}

ForwarderManager::ForwarderManager(const QString& name, const QString& zquorum, unsigned int delay, unsigned int flush_interval, const QString& socket, QObject* parent) : QObject(parent), d_ptr(new ForwarderManagerPrivate(name, zquorum, delay, flush_interval, socket))
{
}

ForwarderManager::~ForwarderManager()
{
	delete d_ptr;
}

void ForwarderManager::finish()
{
	QMetaObject::invokeMethod(d_ptr, "finish", Qt::QueuedConnection);
	d_ptr->wait();
}

bool BatchRequests::process(AsyncHBase::HBaseRpc::BatchProcessable* request)
{
	PendingRequest* pending = new PendingRequest(client, request);
	connect(pending, SIGNAL(finished()), this, SLOT(handle_finished()));

	try {
		pending->process();
		pending_requests.insert(pending);
	} catch (JavaException& e) {
		TWARN("EXCEPTION: [%s] %s", e.name(), e.what());
		delete pending;
		delete request;
		return false;
	}

	return true;
}

bool BatchRequests::send()
{
	DeserializableHBaseOperation::MutateRows mutate;

	mutate.deserialize(buffer() + sizeof(uint64_t) * 2); // Skip transaction and timestamp
	foreach (const DeserializableHBaseOperation::RowValues& row, mutate.rows()) {
		if (mutate.type() == HBaseOperation::CLASS_DELETE_BATCH && row.second.size() == 0) {
			// TODO: there is no timestamp for the whole request
			DeleteRequest* request = new DeleteRequest(mutate.table(), row.first);
			process(request);
		}
		foreach (const DeserializableHBaseOperation::FamilyValues& family, row.second) {
			if (mutate.type() == HBaseOperation::CLASS_DELETE_BATCH && family.second.size() == 0) {
				// TODO: there is no timestamp for the whole family
				DeleteRequest* request = new DeleteRequest(mutate.table(), row.first, family.first);
				process(request);
			}
			foreach (const DeserializableHBaseOperation::QualifierValue& qv, family.second) {
				AsyncHBase::HBaseRpc::BatchProcessable* request;
				if (mutate.type() == HBaseOperation::CLASS_DELETE_BATCH)
					request = new DeleteRequest(mutate.table(), row.first, family.first, qv.qualifier(), qv.timestamp());
				else
					request = new PutRequest(mutate.table(), row.first, family.first, qv.qualifier(), qv.value(), qv.timestamp());
				process(request);
			}
		}
	}

	return true;
}

void BatchRequests::handle_finished()
{
	bool failed = true;
	PendingRequest* pending = qobject_cast<PendingRequest*>(sender());

	try {
		pending->waitForFinished();
		failed = false;
	// TODO: determine what to do in each of these cases, for now, just ignore it and keep on going
	} catch (RecoverableException& e) {
		TWARN("RECOVERABLE EXCEPTION: [%s] %s", qPrintable(e.name()), qPrintable(e.message()));
		if (pending->retry())
			return;
	} catch (PleaseThrottleException& e) {
		TWARN("PLEASETHROTTLE EXCEPTION: [%s] %s", qPrintable(e.name()), qPrintable(e.message()));
		// TODO: Notify server to throttle requests
		if (pending->retry(1000, 10))
			return;
	} catch (NonRecoverableException& e) {
		TWARN("NONRECOVERABLE EXCEPTION: [%s] %s", qPrintable(e.name()), qPrintable(e.message()));
	} catch (HBaseException& e) {
		TWARN("UNKNOWN EXCEPTION: [%s] %s", qPrintable(e.name()), qPrintable(e.message()));
	}

	if (failed)
		failed_ = true;

	delete pending->request();
	delete pending;

	pending_requests.remove(pending);

	if (pending_requests.isEmpty())
		emit finished(this);
}

void PendingRequest::process()
{
	setFuture(request_->process(client_));
}

bool PendingRequest::retry(int delay, int limit)
{
	if (tries > limit) {
		TWARN("No more retries left, failing");
		return false;
	}

	this->delay += delay;

	if (this->delay == 0) {
		TDEBUG("Retrying RPC");
		process();
	} else {
		TDEBUG("Retrying RPC in %d ms", this->delay);
		startTimer(this->delay);
	}

	tries++;

	return true;
}

void PendingRequest::timerEvent(QTimerEvent *event)
{
	killTimer(event->timerId());
	process();
}

