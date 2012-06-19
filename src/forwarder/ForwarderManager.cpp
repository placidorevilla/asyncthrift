#include "ForwarderManager.h"
#include "ForwarderManager_p.h"

#include "config.h"

#include <QDir>
#include <QTimer>

log4cxx::LoggerPtr ForwarderManager::logger(log4cxx::Logger::getLogger(ForwarderManager::staticMetaObject.className()));
log4cxx::LoggerPtr ForwarderManagerPrivate::logger(log4cxx::Logger::getLogger(ForwarderManager::staticMetaObject.className()));

static const char* LOGGER_SOCKET_NAME = "logger";
static int RECONNECT_TIME = 1000;

ForwarderManagerPrivate::ForwarderManagerPrivate(const QString& name, const QString& zquorum, ForwarderManager* parent) : QThread(), name(name), zquorum(zquorum), stream(&socket), state(STATE_LEN), len(0), q_ptr(parent)
{
	moveToThread(this);
	socket.moveToThread(this);
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
	QDir server_dir(PKGSTATEDIR);
	socket.connectToServer(server_dir.absoluteFilePath(LOGGER_SOCKET_NAME));
}

void ForwarderManagerPrivate::handle_connected()
{
	// TODO: find the correct transaction to begin
	uint64_t transaction = 17;
	
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
	while (true) {
		if (state == STATE_LEN) {
			if ((size_t)socket.bytesAvailable() < sizeof(len))
				return;
			stream >> len;
			state = STATE_PAYLOAD;
		}

		if (state == STATE_PAYLOAD) {
			if ((size_t)socket.bytesAvailable() < len)
				return;

			stream.skipRawData(len);
			state = STATE_LEN;
		}
	}
}

ForwarderManager::ForwarderManager(const QString& name, const QString& zquorum, QObject* parent) : QObject(parent), d_ptr(new ForwarderManagerPrivate(name, zquorum))
{
}

ForwarderManager::~ForwarderManager()
{
}

