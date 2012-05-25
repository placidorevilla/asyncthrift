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
	enum { CLASS_PUT, CLASS_DELETE };

	class MutateRow : public HBaseOperationInterface {
	public:
		MutateRow(const Text& tableName, const Text& row, const std::vector<Mutation> & mutations, const int64_t timestamp) :
			tableName(tableName), row(row), mutations(mutations), timestamp(timestamp)
		{}

		size_t size(bool deletes);
		void serialize(void* buffer, bool deletes);

		const Text& tableName;
		const Text& row;
		const std::vector<Mutation> & mutations;
		const int64_t timestamp;
	};

	class PutRow : public MutateRow {
	public:
		PutRow(const Text& tableName, const Text& row, const std::vector<Mutation> & mutations, const int64_t timestamp) :
			MutateRow(tableName, row, mutations, timestamp)
		{}

		size_t size() { return MutateRow::size(false); }
		void serialize(void* buffer) { MutateRow::serialize(buffer, false); }
	};

	class DeleteRow : public MutateRow {
	public:
		DeleteRow(const Text& tableName, const Text& row, const std::vector<Mutation> & mutations, const int64_t timestamp) :
			MutateRow(tableName, row, mutations, timestamp)
		{}

		size_t size() { return MutateRow::size(true); }
		void serialize(void* buffer) { MutateRow::serialize(buffer, true); }
	};
};

#endif // HBASE_OPERATIONS_H
