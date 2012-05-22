#ifndef MAIN_H
#define MAIN_H

#include "TApplication.h"

#include <log4cxx/logger.h>

#include <QDir>

class ThriftDispatcher;

class AsyncThrift : public TApplication
{
	Q_OBJECT
	Q_DISABLE_COPY(AsyncThrift)

public:
	AsyncThrift(int& argc, char** argv);
	virtual ~AsyncThrift();

	static AsyncThrift* instance() { return qobject_cast<AsyncThrift*>(QCoreApplication::instance()); }
	ThriftDispatcher* dispatcher() const { return dispatcher_; }

protected:
	virtual bool init();
	virtual int run();

	virtual void signal_received(int signo);

	bool reloadConfig();

private:
	ThriftDispatcher* dispatcher_;
	QDir config_dir;

	static log4cxx::LoggerPtr logger;
};

#endif // MAIN_H
