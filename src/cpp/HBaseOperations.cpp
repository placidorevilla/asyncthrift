#include "HBaseOperations.h"

static size_t size_text(const Text& text, bool long_text)
{
	return text.size() + (long_text ? sizeof(uint32_t) : sizeof(uint8_t));
}

static void serialize_text(uint8_t** buffer, const Text& text, bool long_text)
{
	uint32_t size;

	// TODO: endianness
	if (long_text) {
		size = *reinterpret_cast<uint32_t*>(*buffer) = qMin((uint32_t)text.size(), (uint32_t)(1UL << 32) - 1);
		*buffer += sizeof(uint32_t);
	} else {
		size = *reinterpret_cast<uint8_t*>(*buffer) = qMin((uint32_t)text.size(), (1U << 8) - 1);
		*buffer += sizeof(uint8_t);
	}

	memcpy(*buffer, text.data(), size);
	*buffer += size;
}

size_t HBaseOperation::MutateRow::size(bool deletes)
{
	int num_ops = 0;
	size_t size = 0;

	foreach(const Mutation& mutation, mutations) {
		if (!((mutation.isDelete && deletes) || (!mutation.isDelete && !deletes)))
			continue;

		num_ops++;
		size += size_text(mutation.column, false);
		size += size_text(mutation.value, true);
	}

	if (!num_ops)
		return 0;

	size += sizeof(uint8_t); // Class
	size += size_text(tableName, false); // Table
	size += size_text(row, true); // RowKey
	size += sizeof(timestamp); // Timestamp
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
	serialize_text(&buffer, tableName, false);
	serialize_text(&buffer, row, true);
	*reinterpret_cast<int64_t*>(buffer) = timestamp;
	buffer += sizeof(timestamp);

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

		serialize_text(&buffer, mutation.column, false);
		serialize_text(&buffer, mutation.value, true);
	}
}

