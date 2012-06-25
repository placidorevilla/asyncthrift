#ifndef HBASEHANDLER_P_H
#define HBASEHANDLER_P_H

#include "TLogger.h"

#include <QObject>

class NBRingByteBuffer;

class HBaseHandlerPrivate : public QObject {
	Q_OBJECT
	Q_DISABLE_COPY(HBaseHandlerPrivate)
	T_LOGGER_DECLARE(HBaseHandlerPrivate);

public:
	HBaseHandlerPrivate();
	virtual ~HBaseHandlerPrivate();

	NBRingByteBuffer* buffer() { return buffer_; }

private:
	NBRingByteBuffer* buffer_;
};

#endif // HBASEHANDLER_P_H
