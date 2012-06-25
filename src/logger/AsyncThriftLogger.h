#ifndef ASYNCTHRIFTLOGGER_H
#define ASYNCTHRIFTLOGGER_H

#include "TApplication.h"
#include "TLogger.h"

#include <QDir>

class ThriftDispatcher;

class AsyncThriftLogger : public TApplication
{
	Q_OBJECT
	Q_DISABLE_COPY(AsyncThriftLogger)
	T_LOGGER_DECLARE(AsyncThriftLogger);

public:
	AsyncThriftLogger(int& argc, char** argv);
	virtual ~AsyncThriftLogger();

	static AsyncThriftLogger* instance() { return qobject_cast<AsyncThriftLogger*>(QCoreApplication::instance()); }
	ThriftDispatcher* dispatcher() const { return dispatcher_; }

protected:
	virtual bool init();
	virtual int run();

	virtual void signal_received(int signo);

	bool reloadConfig();

private:
	ThriftDispatcher* dispatcher_;
	QDir config_dir;
};

#endif // ASYNCTHRIFTLOGGER_H
