#include "HBaseOperations.h"

static size_t size_text(const Text& text, bool long_text)
{
	return text.size() + (long_text ? sizeof(quint32) : sizeof(quint8));
}

static void serialize_text(quint8** buffer, const Text& text, bool long_text)
{
	quint32 size;

	// TODO: endianness
	if (long_text) {
		size = *reinterpret_cast<quint32*>(*buffer) = qMin((quint32)text.size(), (quint32)(1UL << 32) - 1);
		*buffer += sizeof(quint32);
	} else {
		size = *reinterpret_cast<quint8*>(*buffer) = qMin((quint32)text.size(), (1U << 8) - 1);
		*buffer += sizeof(quint8);
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

	size += sizeof(quint8); // Class
	size += size_text(tableName, false); // Table
	size += size_text(row, true); // RowKey
	size += sizeof(timestamp); // Timestamp

	size = (size + (sizeof(quint64) - 1)) & ~(sizeof(quint64) - 1);
	return size;
}

void HBaseOperation::MutateRow::serialize(void* buffer_, bool deletes)
{
	quint8* buffer = reinterpret_cast<quint8*>(buffer_);
	int num_ops = 0;

	*reinterpret_cast<quint8*>(buffer) = deletes ? CLASS_DELETE : CLASS_PUT;
	buffer += sizeof(quint8);
	serialize_text(&buffer, tableName, false);
	serialize_text(&buffer, row, true);
	*reinterpret_cast<int64_t*>(buffer) = timestamp;
	buffer += sizeof(timestamp);

	foreach(const Mutation& mutation, mutations) {
		if (!((mutation.isDelete && deletes) || (!mutation.isDelete && !deletes)))
			continue;
		num_ops++;
	}

	*reinterpret_cast<quint8*>(buffer) = num_ops;
	buffer += sizeof(quint8);

	foreach(const Mutation& mutation, mutations) {
		if (!((mutation.isDelete && deletes) || (!mutation.isDelete && !deletes)) || !num_ops--)
			continue;

		serialize_text(&buffer, mutation.column, false);
		serialize_text(&buffer, mutation.value, true);
	}
}

