#ifndef TAPPLICATION_H
#define TAPPLICATION_H

#include <QCoreApplication>
#include <QList>

class QSocketNotifier;

class TApplication : public QCoreApplication {
	Q_OBJECT

public:
	TApplication(int& argc, char** argv);
	virtual ~TApplication();

	int start();

protected:
	virtual bool init(const QList<int>& hsignals = QList<int>());
	virtual int run();
	virtual void signal_received(int signo);

private slots:
	void daemon_signal_handler();

private:
	QSocketNotifier *_daemon_signal_notifier;
};

#endif // TAPPLICATION_H