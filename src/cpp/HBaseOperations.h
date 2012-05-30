#ifndef HBASE_OPERATIONS_H
#define HBASE_OPERATIONS_H

#include "HBaseHandler.h"

class HBaseOperationInterface {
public:
	size_t size();
	void serialize(void* buffer);
};

class HBaseOperation {
public:
	enum { CLASS_PUT_BATCH, CLASS_DELETE_BATCH };

	class MutateRows : public HBaseOperationInterface {
	public:
		MutateRows(const Text& tableName, const std::vector<BatchMutation> & row_batches, const int64_t timestamp) :
			tableName(tableName), row_batches(row_batches), timestamp(timestamp), num_rows(0)
		{}

		size_t size(bool deletes);
		void serialize(void* buffer, bool deletes);

		const Text& tableName;
		const std::vector<BatchMutation> & row_batches;
		const int64_t timestamp;

	private:
		uint8_t num_rows;
		std::vector<std::map<std::string, std::vector<const Mutation*> > > families_per_row;
	};

	class PutRows : public MutateRows {
	public:
		PutRows(const Text& tableName, const std::vector<BatchMutation> & row_batches, const int64_t timestamp) :
			MutateRows(tableName, row_batches, timestamp)
		{}

		size_t size() { return MutateRows::size(false); }
		void serialize(void* buffer) { MutateRows::serialize(buffer, false); }
	};

	class DeleteRows : public MutateRows {
	public:
		DeleteRows(const Text& tableName, const std::vector<BatchMutation> & row_batches, const int64_t timestamp) :
			MutateRows(tableName, row_batches, timestamp)
		{}

		size_t size() { return MutateRows::size(true); }
		void serialize(void* buffer) { MutateRows::serialize(buffer, true); }
	};
};

#endif // HBASE_OPERATIONS_H
