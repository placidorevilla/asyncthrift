#include "main.h"

#include "ThriftDispatcher.h"

MainApp::MainApp(int argc, char** argv) : TApplication(argc, argv), _td(new ThriftDispatcher)
{
}

MainApp::~MainApp()
{
	delete _td;
}

int MainApp::run()
{
	_td->start();
	int res = TApplication::run();
	_td->stop();
	_td->wait();
	return res;
}

int main(int argc, char **argv)
{
	MainApp app(argc, argv);
	return app.start();
}

