#include "AsyncThriftLogger.h"

#include "ThriftDispatcher.h"

#include <cmdline.hpp>
#include <help.hpp>

#include <QSettings>

#include <signal.h>

#include "config.h"

const char* LOG4CXX_CONFIG_FILE = "log4cxx.xml";
const char* ASYNCTHRIFT_CONFIG_FILE = "asyncthrift.ini";

T_QLOGGER_DEFINE_ROOT(AsyncThriftLogger);

AsyncThriftLogger::AsyncThriftLogger(int& argc, char** argv) : TApplication(argc, argv), dispatcher_(0), config_dir(QDir(PKGSYSCONFDIR))
{
}

AsyncThriftLogger::~AsyncThriftLogger()
{
	delete dispatcher_;
}

bool AsyncThriftLogger::init()
{
	TLogger::configure();
	this->setApplicationName("asyncthrift-logger");

	try {
		QtArgCmdLine cmdline;

		QtArg config('c', "config", "Configuration directory override", false, true);
		QtArg daemonize('d', "daemonize", "Daemonize on start", false, false);
		QtArg user('u', "user", "Run as the given user", false, true);

		QtArgHelp help(&cmdline);
		help.printer()->setProgramDescription("Thrift asynchronous server logger.");
		help.printer()->setExecutableName(this->applicationName());

		cmdline.addArg(&config);
		cmdline.addArg(&daemonize);
		cmdline.addArg(&user);
		cmdline.addArg(help);
		cmdline.parse();

		if (config.isPresent())
			config_dir = QDir(config.value().toString());

		if (!config_dir.exists()) {
			TERROR("Invalid configuration directory: '%s'", qPrintable(config_dir.path()));
			return false;
		}

		config_dir.makeAbsolute();

		if (!TApplication::init(QList<int>() << SIGINT << SIGTERM << SIGHUP, daemonize.isPresent(), user.value().toString(), "asyncthrift/logger"))
			return false;
	} catch (const QtArgHelpHasPrintedEx& ex) {
		return false;
	} catch (const QtArgBaseException& ex) {
		TERROR("%s", ex.what());
		return false;
	}

	dispatcher_ = new ThriftDispatcher;

	if (!reloadConfig())
		return false;

	return true;
}

int AsyncThriftLogger::run()
{
	dispatcher()->start();
	int res = TApplication::run();
	dispatcher()->stop();
	dispatcher()->wait();
	return res;
}

void AsyncThriftLogger::signal_received(int signo)
{
	if (signo == SIGTERM || signo == SIGINT)
		quit();
	else if (signo == SIGHUP)
		reloadConfig();
}

bool AsyncThriftLogger::reloadConfig()
{
	TDEBUG("Load configuration...");

	if (!config_dir.exists(LOG4CXX_CONFIG_FILE)) {
		TWARN("Cannot find '%s'", LOG4CXX_CONFIG_FILE);
		return false;
	}

	if (!config_dir.exists(ASYNCTHRIFT_CONFIG_FILE)) {
		TWARN("Cannot find '%s'", ASYNCTHRIFT_CONFIG_FILE);
		return false;
	}

	TLogger::configure(qPrintable(config_dir.absoluteFilePath(LOG4CXX_CONFIG_FILE)));

	QSettings settings(config_dir.absoluteFilePath(ASYNCTHRIFT_CONFIG_FILE), QSettings::IniFormat);

	dispatcher()->set_port(settings.value("Port", 9090).toUInt());
	dispatcher()->set_num_worker_threads(settings.value("ThriftThreads", 4).toUInt());
	dispatcher()->set_buffer_size(settings.value("BufferSize", 64).toUInt());

	settings.beginGroup("LogStorage");
	QList<QString> log_dirs;

	int nentries = settings.beginReadArray("Paths");
	for (int i = 0; i < nentries; i++) {
		settings.setArrayIndex(i);
		log_dirs.append(settings.value("Dir").toString());
	}
	settings.endArray();

	dispatcher()->configure_log_storage(settings.value("MaxLogSize", 64).toUInt(), settings.value("SyncPeriod", 1000).toUInt(), log_dirs);

	return true;
}

const char* TLoggerRoot()
{
	return "logger";
}

int main(int argc, char **argv)
{
	AsyncThriftLogger app(argc, argv);
	return app.start();
}

