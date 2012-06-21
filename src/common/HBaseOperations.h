#ifndef HBASE_OPERATIONS_H
#define HBASE_OPERATIONS_H

#include "Hbase.h"

#include <QByteArray>
#include <QPair>
#include <QVector>

using namespace apache::hadoop::hbase::thrift;

class HBaseOperation {
public:
	typedef enum { CLASS_PUT_BATCH, CLASS_DELETE_BATCH } Type;
};

class SerializableHBaseOperation : public HBaseOperation {
public:
	class SerializableInterface {
	public:
		size_t size();
		void serialize(void* buffer);
	};

	class MutateRows : public SerializableInterface {
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

class DeserializableHBaseOperation : public HBaseOperation {
public:
	class DeserializableInterface {
	public:
		void deserialize(void* buffer);
	};

	class QualifierValue {
	public:
		QualifierValue() : timestamp_(0)
		{}
		QualifierValue(QByteArray qualifier, QByteArray value, int64_t timestamp) : qualifier_(qualifier), value_(value), timestamp_(timestamp)
		{}

		QByteArray qualifier() const { return qualifier_; }
		QByteArray value() const { return value_; }
		int64_t timestamp() const { return timestamp_; }

	private:
		QByteArray qualifier_;
		QByteArray value_;
		int64_t timestamp_;
	};

	typedef QPair<QByteArray, QVector<QualifierValue> > FamilyValues;
	typedef QPair<QByteArray, QVector<FamilyValues> > RowValues;

	class MutateRows : public DeserializableInterface {
	public:
		MutateRows() : DeserializableInterface()
		{}

		void deserialize(void* buffer);

		Type type() const { return type_; }
		QByteArray table() const { return table_; }
		QVector<RowValues> rows() const { return rows_; }

	private:
		Type type_;
		QByteArray table_;
		QVector<RowValues> rows_;
	};
};

#endif // HBASE_OPERATIONS_H
