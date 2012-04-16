#ifndef MAIN_H
#define MAIN_H

#include "TApplication.h"

class ThriftDispatcher;

class MainApp : public TApplication
{
	Q_OBJECT

public:
	MainApp(int argc, char** argv);
	virtual ~MainApp();
	virtual int run();

private:
	ThriftDispatcher* _td;
};

#endif // MAIN_H
