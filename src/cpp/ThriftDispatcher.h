#ifndef THRIFTDISPATCHER_H
#define THRIFTDISPATCHER_H

#include <QThread>

class ThriftDispatcherPrivate;

class ThriftDispatcher : public QThread {
	Q_OBJECT

public:
	ThriftDispatcher(QObject* parent = 0);
	virtual ~ThriftDispatcher();

	void stop();

protected:
	virtual void run();

private:
	ThriftDispatcherPrivate* d;
};

#endif // THRIFTDISPATCHER_H
