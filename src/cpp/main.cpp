#include "main.h"

#include "ThriftDispatcher.h"

#include <cmdline.hpp>
#include <help.hpp>

#include <log4cxx/basicconfigurator.h>
#include <log4cxx/xml/domconfigurator.h>

#include <QSettings>

#include <signal.h>

const char* LOG4CXX_CONFIG_FILE = "log4cxx.xml";
const char* ASYNCTHRIFT_CONFIG_FILE = "asyncthrift.ini";

log4cxx::LoggerPtr MainApp::logger(log4cxx::Logger::getLogger(MainApp::staticMetaObject.className()));

MainApp::MainApp(int& argc, char** argv) : TApplication(argc, argv), _td(0), _config_dir(QDir("/etc/asyncthrift"))
{
}

MainApp::~MainApp()
{
	delete _td;
}

bool MainApp::init()
{
	log4cxx::BasicConfigurator::configure();
	this->setApplicationName("asyncthrift");

	try {
		QtArgCmdLine cmdline;

		QtArg config('c', "config", "Configuration directory override", false, true);

		QtArgHelp help(&cmdline);
		help.printer()->setProgramDescription("Thrift asynchronous server.");
		help.printer()->setExecutableName(this->applicationName());

		cmdline.addArg(&config);
		cmdline.addArg(help);
		cmdline.parse();

		if (config.isPresent())
			_config_dir = QDir(config.value().toString());

		if (!_config_dir.exists()) {
			LOG4CXX_ERROR(logger, "Invalid configuration directory: " << qPrintable(_config_dir.path()));
			return false;
		}
	} catch (const QtArgHelpHasPrintedEx& ex) {
		return false;
	} catch (const QtArgBaseException& ex) {
		LOG4CXX_ERROR(logger, ex.what());
		return false;
	}

	_td = new ThriftDispatcher;

	if (!reloadConfig())
		return false;

	if (!TApplication::init(QList<int>() << SIGINT << SIGTERM << SIGHUP))
		return false;

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
	else if (signo == SIGHUP)
		reloadConfig();
}

bool MainApp::reloadConfig()
{
	LOG4CXX_DEBUG(logger, "Load configuration...");

	if (!_config_dir.exists(LOG4CXX_CONFIG_FILE)) {
		LOG4CXX_WARN(logger, "Cannot find " << LOG4CXX_CONFIG_FILE);
		return false;
	}

	if (!_config_dir.exists(ASYNCTHRIFT_CONFIG_FILE)) {
		LOG4CXX_WARN(logger, "Cannot find " << ASYNCTHRIFT_CONFIG_FILE);
		return false;
	}

	log4cxx::BasicConfigurator::resetConfiguration();
	log4cxx::xml::DOMConfigurator::configure(qPrintable(_config_dir.absoluteFilePath(LOG4CXX_CONFIG_FILE)));

	QSettings settings(_config_dir.absoluteFilePath(ASYNCTHRIFT_CONFIG_FILE), QSettings::IniFormat);

	_td->setPort(settings.value("Port", 9090).toUInt());
	_td->setWorkerThreads(settings.value("ThriftThreads", 4).toUInt());

	return true;
}

int main(int argc, char **argv)
{
	MainApp app(argc, argv);
	return app.start();
}

