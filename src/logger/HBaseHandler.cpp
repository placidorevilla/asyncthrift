#include "HBaseHandler.h"
#include "HBaseHandler_p.h"

#include "AsyncThriftLogger.h"
#include "ThriftDispatcher.h"
#include "NBRingByteBuffer.h"
#include "HBaseOperations.h"

T_QLOGGER_DEFINE_ROOT(HBaseHandler);
T_QLOGGER_DEFINE_OTHER_ROOT(HBaseHandlerPrivate, HBaseHandler);

static int64_t current_timestamp()
{
	struct timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts);
	return ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

HBaseHandlerPrivate::HBaseHandlerPrivate() : buffer_(AsyncThriftLogger::instance()->dispatcher()->buffer())
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

void HBaseHandler::mutateRow(const Text& tableName, const Text& row, const std::vector<Mutation> & mutations)
{
	mutateRowTs(tableName, row, mutations, current_timestamp());
}

void HBaseHandler::mutateRowTs(const Text& tableName, const Text& row, const std::vector<Mutation> & mutations, const int64_t timestamp)
{
	std::vector<BatchMutation> rowBatches;
	BatchMutation row_batch;
	row_batch.row = row;
	row_batch.mutations = mutations;
	rowBatches.push_back(row_batch);

	mutateRowsTs(tableName, rowBatches, timestamp);
}

void HBaseHandler::mutateRows(const Text& tableName, const std::vector<BatchMutation> & rowBatches)
{
	mutateRowsTs(tableName, rowBatches, current_timestamp());
}

void HBaseHandler::mutateRowsTs(const Text& tableName, const std::vector<BatchMutation> & rowBatches, const int64_t timestamp)
{
	unsigned long tnx;
	size_t size;

	SerializableHBaseOperation::PutRows put(tableName, rowBatches, timestamp);
	SerializableHBaseOperation::DeleteRows del(tableName, rowBatches, timestamp);

	if ((size = put.size()) != 0) {
		void* buffer = d->buffer()->alloc_write(size, &tnx);
		if (!buffer) {
			TWARN("Invalid state for ring buffer");
			return;
		}

		put.serialize(buffer);
		d->buffer()->commit_write(tnx);
	}

	if ((size = del.size()) != 0) {
		void* buffer = d->buffer()->alloc_write(size, &tnx);
		if (!buffer) {
			TWARN("Invalid state for ring buffer");
			return;
		}

		del.serialize(buffer);
		d->buffer()->commit_write(tnx);
	}

	return;
}

