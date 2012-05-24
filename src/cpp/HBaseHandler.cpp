#include "HBaseHandler.h"
#include "HBaseHandler_p.h"

#include "AsyncThrift.h"
#include "ThriftDispatcher.h"
#include "NBRingByteBuffer.h"

log4cxx::LoggerPtr HBaseHandler::logger(log4cxx::Logger::getLogger(HBaseHandler::staticMetaObject.className()));
log4cxx::LoggerPtr HBaseHandlerPrivate::logger(log4cxx::Logger::getLogger(HBaseHandler::staticMetaObject.className()));

HBaseHandlerPrivate::HBaseHandlerPrivate() : buffer_(AsyncThrift::instance()->dispatcher()->buffer())
{
}

HBaseHandlerPrivate::~HBaseHandlerPrivate()
{
}

HBaseHandler::HBaseHandler() : d(new HBaseHandlerPrivate)
{
}

HBaseHandler::~HBaseHandler()
{
	delete d;
}

void HBaseHandler::mutateRow(const Text& tableName, const Text& row, const std::vector<Mutation> & mutations) {
	unsigned long tnx;

	void* buffer = d->buffer()->alloc_write(sizeof(unsigned long), &tnx);
	if (!buffer) {
		LOG4CXX_WARN(logger, "Invalid state for ring buffer");
		return;
	}
	*((unsigned long*)buffer) = 0xDEADBEEF;
	d->buffer()->commit_write(tnx);
	return;
}
