#ifndef FORWARDERMANAGER_H
#define FORWARDERMANAGER_H

#include "TLogger.h"

#include <QObject>

class ForwarderManagerPrivate;

class ForwarderManager : public QObject {
	Q_OBJECT
	Q_DISABLE_COPY(ForwarderManager)
	T_LOGGER_DECLARE(ForwarderManager);

public:
	ForwarderManager(const QString& name, const QString& zquorum, unsigned int delay, QObject* parent = 0);
	~ForwarderManager();

private:
	Q_DECLARE_PRIVATE(ForwarderManager);
	ForwarderManagerPrivate* d_ptr;
};

#endif // FORWARDERMANAGER_H
