#ifndef FORWARDERMANAGER_H
#define FORWARDERMANAGER_H

#include <log4cxx/logger.h>

#include <QObject>

class ForwarderManagerPrivate;

class ForwarderManager : public QObject {
	Q_OBJECT
	Q_DISABLE_COPY(ForwarderManager)

public:
	ForwarderManager(const QString& name, const QString& zquorum, unsigned int delay, QObject* parent = 0);
	~ForwarderManager();

private:
	Q_DECLARE_PRIVATE(ForwarderManager);
	ForwarderManagerPrivate* d_ptr;

	static log4cxx::LoggerPtr logger;
};

#endif // FORWARDERMANAGER_H
