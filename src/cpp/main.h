#ifndef MAIN_H
#define MAIN_H

#include "TApplication.h"

#include <log4cxx/logger.h>

class ThriftDispatcher;

class MainApp : public TApplication
{
	Q_OBJECT

public:
	MainApp(int argc, char** argv);
	virtual ~MainApp();

	virtual bool init();
	virtual int run();

	virtual void signal_received(int signo);
private:
	ThriftDispatcher* _td;
	static log4cxx::LoggerPtr logger;
};

#endif // MAIN_H
