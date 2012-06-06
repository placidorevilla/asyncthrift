#ifndef HBASEHANDLER_P_H
#define HBASEHANDLER_P_H

#include <log4cxx/logger.h>

#include <QObject>

class NBRingByteBuffer;

class HBaseHandlerPrivate : public QObject {
	Q_OBJECT
	Q_DISABLE_COPY(HBaseHandlerPrivate)

public:
	HBaseHandlerPrivate();
	virtual ~HBaseHandlerPrivate();

	NBRingByteBuffer* buffer() { return buffer_; }

private:
	NBRingByteBuffer* buffer_;

	static log4cxx::LoggerPtr logger;
};

#endif // HBASEHANDLER_P_H
