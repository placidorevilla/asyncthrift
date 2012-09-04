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
	d->serialize(SerializableHBaseOperation::PutRows(tableName, rowBatches, timestamp));
	d->serialize(SerializableHBaseOperation::DeleteRows(tableName, rowBatches, timestamp));
}

void HBaseHandler::deleteAll(const Text& tableName, const Text& row, const Text& column)
{
	deleteAllTs(tableName, row, column, current_timestamp());
}

void HBaseHandler::deleteAllTs(const Text& tableName, const Text& row, const Text& column, const int64_t timestamp)
{
	std::vector<BatchMutation> rowBatches;
	BatchMutation batchMutation;
	if (!column.empty()) {
		std::vector<Mutation> mutations;
		Mutation mutation;
		mutation.__set_isDelete(true);
		mutation.__set_column(column);
		mutations.push_back(mutation);
		batchMutation.__set_mutations(mutations);
	}
	batchMutation.__set_row(row);
	rowBatches.push_back(batchMutation);

	d->serialize(SerializableHBaseOperation::DeleteRows(tableName, rowBatches, timestamp));
}

void HBaseHandler::deleteAllRow(const Text& tableName, const Text& row)
{
	deleteAllTs(tableName, row, Text(), current_timestamp());
}

void HBaseHandler::deleteAllRowTs(const Text& tableName, const Text& row, const int64_t timestamp)
{
	deleteAllTs(tableName, row, Text(), timestamp);
}

void HBaseHandlerPrivate::serialize(const SerializableHBaseOperation::SerializableInterface& operation)
{
	unsigned long tnx;
	size_t size;

	if ((size = operation.size()) != 0) {
		void* buf = buffer()->alloc_write(size, &tnx);
		if (!buf) {
			TWARN("Invalid state for ring buffer");
			return;
		}

		operation.serialize(buf);
		buffer()->commit_write(tnx);
	}
}

