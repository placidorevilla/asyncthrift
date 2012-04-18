#include "main.h"

#include "ThriftDispatcher.h"

#include <signal.h>

log4cxx::LoggerPtr MainApp::logger(log4cxx::Logger::getLogger(MainApp::staticMetaObject.className()));

MainApp::MainApp(int argc, char** argv) : TApplication(argc, argv), _td(0)
{
}

MainApp::~MainApp()
{
	delete _td;
}

bool MainApp::init()
{
	if (!TApplication::init(QList<int>() << SIGINT << SIGTERM << SIGHUP))
		return false;

	_td = new ThriftDispatcher;
	return true;
}

int MainApp::run()
{
	_td->start();
	int res = TApplication::run();
	_td->stop();
	_td->wait();
	return res;
}

void MainApp::signal_received(int signo)
{
	if (signo == SIGTERM || signo == SIGINT)
		quit();
}

int main(int argc, char **argv)
{
	MainApp app(argc, argv);
	return app.start();
}

