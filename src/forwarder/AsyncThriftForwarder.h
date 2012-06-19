#ifndef ASYNCTHRIFTFORWARDER_H
#define ASYNCTHRIFTFORWARDER_H

#include "TApplication.h"

#include <log4cxx/logger.h>

#include <QDir>

class ForwarderManager;

class AsyncThriftForwarder : public TApplication
{
	Q_OBJECT
	Q_DISABLE_COPY(AsyncThriftForwarder)

public:
	AsyncThriftForwarder(int& argc, char** argv);
	virtual ~AsyncThriftForwarder();

	static AsyncThriftForwarder* instance() { return qobject_cast<AsyncThriftForwarder*>(QCoreApplication::instance()); }

protected:
	virtual bool init();
	virtual int run();

	virtual void signal_received(int signo);

	bool reloadConfig();

private:
	QDir config_dir;
	QList<ForwarderManager*> forwarders;

	static log4cxx::LoggerPtr logger;
};

#endif // ASYNCTHRIFTFORWARDER_H
