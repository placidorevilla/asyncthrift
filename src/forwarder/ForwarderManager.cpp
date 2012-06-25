#include "ForwarderManager.h"
#include "ForwarderManager_p.h"

#include "HBaseOperations.h"

#include "config.h"

#include <QDir>
#include <QTimer>

using namespace AsyncHBase;

T_QLOGGER_DEFINE_ROOT(ForwarderManager);
T_QLOGGER_DEFINE_OTHER_ROOT(ForwarderManagerPrivate, ForwarderManager);
T_QLOGGER_DEFINE_ROOT(BatchRequests);

static uint64_t TX_DONE_MASK = 1ULL << 63;
static uint64_t TX_FLYING_MASK = ~(1ULL << 63);

// TODO: this should be config
static const char* LOGGER_SOCKET_NAME = "logger";
static int RECONNECT_TIME = 1000;
static int MAX_FLYING_TX = 4096;
static quint64 MAX_SOCKET_BUFFER_SIZE = 4 * 1024 * 1024;

ForwarderManagerPrivate::ForwarderManagerPrivate(const QString& name, const QString& zquorum, unsigned int delay, ForwarderManager* parent) : QThread(), name(name), zquorum(zquorum), delay(delay), stream(&socket), state(STATE_READ_LEN), len(0), flying_txs(MAX_FLYING_TX), hbase_client(new HBaseClient(zquorum.toAscii(), "/hbase")), q_ptr(parent)
{
	QDir state_dir(PKGSTATEDIR);

	// TODO: configure the HBaseClient object
	moveToThread(this);
	socket.moveToThread(this);
	socket.setReadBufferSize(MAX_SOCKET_BUFFER_SIZE);
	tx_ptr_file.setFileName(state_dir.absoluteFilePath(QString("forwarder_%1.ptr").arg(name)));
	tx_ptr_file.open(QIODevice::ReadWrite);
	tx_ptr_stream.setDevice(&tx_ptr_file);
	tx_ptr_stream.setVersion(QDataStream::Qt_4_6);
	tx_ptr_stream.setByteOrder(QDataStream::LittleEndian);
	start();
}

ForwarderManagerPrivate::~ForwarderManagerPrivate()
{
}

void ForwarderManagerPrivate::run()
{
	qsrand(time(NULL));
	QTimer::singleShot(0, this, SLOT(reconnect()));
	connect(&socket, SIGNAL(connected()), SLOT(handle_connected()));
	connect(&socket, SIGNAL(error(QLocalSocket::LocalSocketError)), SLOT(handle_error(QLocalSocket::LocalSocketError)));
	connect(&socket, SIGNAL(readyRead()), SLOT(handle_ready_read()));
	exec();
}

void ForwarderManagerPrivate::reconnect()
{
	QDir server_dir(PKGSTATEDIR);
	socket.connectToServer(server_dir.absoluteFilePath(LOGGER_SOCKET_NAME));
}

void ForwarderManagerPrivate::handle_connected()
{
	quint64 transaction;
	
	tx_ptr_stream.device()->seek(0);
	tx_ptr_stream >> transaction;
	tx_ptr_stream.device()->seek(0);
	if (tx_ptr_stream.status() != QDataStream::Ok) {
		transaction = 1;
		tx_ptr_stream.resetStatus();
		tx_ptr_stream << transaction;
		tx_ptr_file.flush();
		tx_ptr_stream.device()->seek(0);
	}
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
	uint32_t now = time(NULL);

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

			batch = new BatchRequests(len);
			stream.readRawData(batch->buffer(), len);
			connect(batch, SIGNAL(finished()), SLOT(handle_finished()));
			flying_txs.push_back(batch->transaction() & TX_FLYING_MASK);
			if (now - batch->timestamp() / 10 < delay) {
				delayed_batch = batch;
				QTimer::singleShot((now - batch->timestamp() / 10) * 1000, this, SLOT(handle_end_delay()));
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
			if (!batch->send(hbase_client))
				delete batch;

			state = STATE_READ_LEN;
		}
	}
}

void ForwarderManagerPrivate::handle_finished()
{
	BatchRequests* batch = qobject_cast<BatchRequests*>(sender());

	uint64_t transaction = batch->transaction() & TX_FLYING_MASK;
//	TDEBUG("finished transaction %llu", transaction);

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
		tx_ptr_file.flush();
		tx_ptr_stream.device()->seek(0);
		stream.writeRawData((char *)&transaction, sizeof(transaction));
		TDEBUG("Sync transaction %llu", transaction);

		if (state == STATE_STALLED) {
			connect(&socket, SIGNAL(readyRead()), SLOT(handle_ready_read()));
			handle_ready_read();
		}
	}

	delete batch;
}

void ForwarderManagerPrivate::handle_end_delay()
{
	connect(&socket, SIGNAL(readyRead()), SLOT(handle_ready_read()));
	handle_ready_read();
}

ForwarderManager::ForwarderManager(const QString& name, const QString& zquorum, unsigned int delay, QObject* parent) : QObject(parent), d_ptr(new ForwarderManagerPrivate(name, zquorum, delay))
{
}

ForwarderManager::~ForwarderManager()
{
}

bool BatchRequests::send(HBaseClient* client)
{
	DeserializableHBaseOperation::MutateRows mutate;

	// TODO: check type of request
	mutate.deserialize(buffer() + sizeof(uint64_t) * 2); // Skip transaction and timestamp
	foreach (const DeserializableHBaseOperation::RowValues& row, mutate.rows()) {
		foreach (const DeserializableHBaseOperation::FamilyValues& family, row.second) {
			foreach (const DeserializableHBaseOperation::QualifierValue& qv, family.second) {
				PutRequest* pr = new PutRequest(mutate.table(), row.first, family.first, qv.qualifier(), qv.value(), qv.timestamp());
				pr->set_bufferable(true);
				pr->set_durable(true);
				PendingRequest* pending = new PendingRequest(pr);
				connect(pending, SIGNAL(finished()), this, SLOT(handle_finished()));

				try {
//					TDEBUG("send %p", pending);
					pending->setFuture(client->put(*pr));
					pending_requests.insert(pending);
				} catch (JavaException& e) {
					TWARN("EXCEPTION: [%s] %s", e.name(), e.what());
					delete pr;
					delete pending;
				}
			}
		}
	}
	return true;
}

void BatchRequests::handle_finished()
{
	PendingRequest* pending = qobject_cast<PendingRequest*>(sender());

	try {
//		TDEBUG("finished %p", pending);
		pending->waitForFinished();
	// TODO: check the different kinds of exceptions
	} catch (HBaseException& e) {
		TWARN("EXCEPTION: [%s] %s", qPrintable(e.name()), qPrintable(e.message()));
	}

	delete pending->request();
	delete pending;

	pending_requests.remove(pending);

	if (pending_requests.isEmpty())
		emit finished();
}

