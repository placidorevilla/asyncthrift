#ifndef FORWARDERMANAGER_P_H
#define FORWARDERMANAGER_P_H

#include "ForwarderManager.h"

#include <log4cxx/logger.h>

#include <QThread>
#include <QLocalSocket>
#include <QDataStream>

class ForwarderManagerPrivate : public QThread {
	Q_OBJECT
	Q_DECLARE_PUBLIC(ForwarderManager)

	enum State { STATE_LEN, STATE_PAYLOAD };

public:
	ForwarderManagerPrivate(const QString& name, const QString& zquorum, ForwarderManager* parent = 0);
	~ForwarderManagerPrivate();

	virtual void run();

private slots:
	void handle_connected();
	void handle_error(QLocalSocket::LocalSocketError socket_error);
	void handle_ready_read();
	void reconnect();

public:
	QString name;
	QString zquorum;

private:
	QLocalSocket socket;
	QDataStream stream;

	State state;
	quint32 len;

	ForwarderManager* q_ptr;

	static log4cxx::LoggerPtr logger;
};

#endif // FORWARDERMANAGER_P_H
