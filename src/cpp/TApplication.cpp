#include "TApplication.h"

#include <libdaemon/daemon.h>

#include <QSocketNotifier>

#include <signal.h>

TApplication::TApplication(int& argc, char** argv) : QCoreApplication(argc, argv), _daemon_signal_notifier(0)
{
}

TApplication::~TApplication()
{
	if (_daemon_signal_notifier) {
		daemon_signal_done();
		delete _daemon_signal_notifier;
	}
}

int TApplication::start()
{
	if (init())
		return run();
	return -1;
}

bool TApplication::init(const QList<int>& hsignals)
{
	if (hsignals.size() == 0)
		daemon_signal_init(SIGINT, SIGTERM, 0);
	foreach (int signal, hsignals)
		daemon_signal_install(signal);
	_daemon_signal_notifier = new QSocketNotifier(daemon_signal_fd(), QSocketNotifier::Read);
	connect(_daemon_signal_notifier, SIGNAL(activated(int)), SLOT(daemon_signal_handler()));
	return true;
}

int TApplication::run()
{
	return exec();
}

void TApplication::signal_received(int signo)
{
	Q_UNUSED(signo);
	exit(1);
}

void TApplication::daemon_signal_handler()
{
	int signo;

	signo = daemon_signal_next();
	signal_received(signo);
}

