#include "AsyncThriftForwarder.h"

#include "ForwarderManager.h"

#include <cmdline.hpp>
#include <help.hpp>

#include <log4cxx/basicconfigurator.h>
#include <log4cxx/xml/domconfigurator.h>

#include <QSettings>

#include <signal.h>

const char* LOG4CXX_CONFIG_FILE = "log4cxx.xml";
const char* ASYNCTHRIFT_CONFIG_FILE = "asyncthrift.ini";

// TODO: daemonization and stuff

log4cxx::LoggerPtr AsyncThriftForwarder::logger(log4cxx::Logger::getLogger(AsyncThriftForwarder::staticMetaObject.className()));

AsyncThriftForwarder::AsyncThriftForwarder(int& argc, char** argv) : TApplication(argc, argv), config_dir(QDir("/etc/asyncthrift"))
{
}

AsyncThriftForwarder::~AsyncThriftForwarder()
{
}

bool AsyncThriftForwarder::init()
{
	log4cxx::BasicConfigurator::configure();
	this->setApplicationName("asyncthrift");

	try {
		QtArgCmdLine cmdline;

		QtArg config('c', "config", "Configuration directory override", false, true);

		QtArgHelp help(&cmdline);
		help.printer()->setProgramDescription("Thrift asynchronous server forwarder.");
		help.printer()->setExecutableName(this->applicationName());

		cmdline.addArg(&config);
		cmdline.addArg(help);
		cmdline.parse();

		if (config.isPresent())
			config_dir = QDir(config.value().toString());

		if (!config_dir.exists()) {
			LOG4CXX_ERROR(logger, "Invalid configuration directory: " << qPrintable(config_dir.path()));
			return false;
		}
	} catch (const QtArgHelpHasPrintedEx& ex) {
		return false;
	} catch (const QtArgBaseException& ex) {
		LOG4CXX_ERROR(logger, ex.what());
		return false;
	}

	if (!reloadConfig())
		return false;

	if (!TApplication::init(QList<int>() << SIGINT << SIGTERM << SIGHUP))
		return false;

	return true;
}

int AsyncThriftForwarder::run()
{
	int res = TApplication::run();
	return res;
}

void AsyncThriftForwarder::signal_received(int signo)
{
	if (signo == SIGTERM || signo == SIGINT)
		quit();
	else if (signo == SIGHUP)
		reloadConfig();
}

bool AsyncThriftForwarder::reloadConfig()
{
	LOG4CXX_DEBUG(logger, "Load configuration...");

	if (!config_dir.exists(LOG4CXX_CONFIG_FILE)) {
		LOG4CXX_WARN(logger, "Cannot find " << LOG4CXX_CONFIG_FILE);
		return false;
	}

	if (!config_dir.exists(ASYNCTHRIFT_CONFIG_FILE)) {
		LOG4CXX_WARN(logger, "Cannot find " << ASYNCTHRIFT_CONFIG_FILE);
		return false;
	}

	log4cxx::BasicConfigurator::resetConfiguration();
	log4cxx::xml::DOMConfigurator::configure(qPrintable(config_dir.absoluteFilePath(LOG4CXX_CONFIG_FILE)));

	QSettings settings(config_dir.absoluteFilePath(ASYNCTHRIFT_CONFIG_FILE), QSettings::IniFormat);

	// TODO: Check reconfiguration
	settings.beginGroup("LogForwarder");
	int nentries = settings.beginReadArray("Forwarders");
	for (int i = 0; i < nentries; i++) {
		settings.setArrayIndex(i);
		forwarders.append(new ForwarderManager(settings.value("Name").toString(), settings.value("ZQuorum").toString(), settings.value("Delay").toUInt()));
	}
	settings.endArray();

	return true;
}

int main(int argc, char **argv)
{
	AsyncThriftForwarder app(argc, argv);
	return app.start();
}

