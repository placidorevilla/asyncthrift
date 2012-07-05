#ifndef ASYNCTHRIFTFORWARDER_H
#define ASYNCTHRIFTFORWARDER_H

#include "TApplication.h"

#include "TLogger.h"

#include <QDir>

class ForwarderManager;

class AsyncThriftForwarder : public TApplication
{
	Q_OBJECT
	Q_DISABLE_COPY(AsyncThriftForwarder)
	T_LOGGER_DECLARE(AsyncThriftForwarder);

public:
	AsyncThriftForwarder(int& argc, char** argv);
	virtual ~AsyncThriftForwarder();

	static AsyncThriftForwarder* instance() { return qobject_cast<AsyncThriftForwarder*>(QCoreApplication::instance()); }

protected:
	virtual bool init();
	virtual int run();

	virtual void signal_received(int signo);

private:
	bool reloadConfig();
	void finish();

private:
	QDir config_dir;
	QList<ForwarderManager*> forwarders;
};

#endif // ASYNCTHRIFTFORWARDER_H
