#ifndef HBASE_OPERATIONS_H
#define HBASE_OPERATIONS_H

#include "HBaseHandler.h"

#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
#define LOG_ENDIAN(x) (x)
#else
#define LOG_ENDIAN(x) (bswap(x))
#endif

template <typename T> T bswap(T v);
template <> inline uint64_t bswap<uint64_t>(uint64_t v) { return bswap_64(v); }
template <> inline uint32_t bswap<uint32_t>(uint32_t v) { return bswap_32(v); }
template <> inline uint16_t bswap<uint16_t>(uint16_t v) { return bswap_16(v); }
template <> inline uint8_t bswap<uint8_t>(uint8_t v) { return v; }
template <> inline int64_t bswap<int64_t>(int64_t v) { return bswap_64(v); }
template <> inline int32_t bswap<int32_t>(int32_t v) { return bswap_32(v); }
template <> inline int16_t bswap<int16_t>(int16_t v) { return bswap_16(v); }
template <> inline int8_t bswap<int8_t>(int8_t v) { return v; }

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
