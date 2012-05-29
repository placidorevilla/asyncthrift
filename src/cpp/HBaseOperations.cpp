#include "HBaseOperations.h"

template <typename T>
static inline size_t size_text(const Text& text)
{
	return sizeof(T) + text.size();
}

template <typename T>
static void serialize_text(uint8_t** buffer, const Text& text)
{
	uint32_t size;

	size = qMin((uint32_t)text.size(), (uint32_t)(1UL << sizeof(T) * 8) - 1);
	*reinterpret_cast<T*>(*buffer) = LOG_ENDIAN((T)size);
	*buffer += sizeof(T);

	memcpy(*buffer, text.data(), size);
	*buffer += size;
}

static inline size_t size_smalltext(const Text& text) { return size_text<uint8_t>(text); }
static inline void serialize_smalltext(uint8_t** buffer, const Text& text) { serialize_text<uint8_t>(buffer, text); }
static inline size_t size_bigtext(const Text& text) { return size_text<uint32_t>(text); }
static inline void serialize_bigtext(uint8_t** buffer, const Text& text) { serialize_text<uint32_t>(buffer, text); }

size_t HBaseOperation::MutateRow::size(bool deletes)
{
	int num_ops = 0;
	size_t size = 0;

	foreach(const Mutation& mutation, mutations) {
		if (!((mutation.isDelete && deletes) || (!mutation.isDelete && !deletes)))
			continue;

		num_ops++;
		size += size_smalltext(mutation.column);
		size += size_bigtext(mutation.value);
	}

	if (!num_ops)
		return 0;

	size += sizeof(uint8_t); // Class
	size += size_smalltext(tableName); // Table
	size += size_bigtext(row); // RowKey
	size += sizeof(int64_t); // Timestamp
	size += sizeof(uint8_t); // Number of mutations

	// Align size to 64 bits
	size = (size + (sizeof(uint64_t) - 1)) & ~(sizeof(uint64_t) - 1);
	return size;
}

void HBaseOperation::MutateRow::serialize(void* buffer_, bool deletes)
{
	uint8_t* buffer = reinterpret_cast<uint8_t*>(buffer_);
	int num_ops = 0;

	*reinterpret_cast<uint8_t*>(buffer) = deletes ? CLASS_DELETE : CLASS_PUT;
	buffer += sizeof(uint8_t);
	serialize_smalltext(&buffer, tableName);
	serialize_bigtext(&buffer, row);
	*reinterpret_cast<int64_t*>(buffer) = LOG_ENDIAN((int64_t)timestamp);
	buffer += sizeof(int64_t);

	foreach(const Mutation& mutation, mutations) {
		if (!((mutation.isDelete && deletes) || (!mutation.isDelete && !deletes)))
			continue;
		num_ops++;
	}

	*reinterpret_cast<uint8_t*>(buffer) = num_ops;
	buffer += sizeof(uint8_t);

	foreach(const Mutation& mutation, mutations) {
		if (!((mutation.isDelete && deletes) || (!mutation.isDelete && !deletes)) || !num_ops--)
			continue;

		serialize_smalltext(&buffer, mutation.column);
		serialize_bigtext(&buffer, mutation.value);
	}
}

