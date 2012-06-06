#include "HBaseOperations.h"

#include "LogEndian.h"

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

template <typename T>
static QByteArray deserialize_text(uint8_t** buffer)
{
	int size;
	const char* data;
	
	size = (int) LOG_ENDIAN(*reinterpret_cast<T*>(*buffer));
	*buffer += sizeof(T);
	data = reinterpret_cast<const char*>(*buffer);
	*buffer += size;
	return QByteArray::fromRawData(data, size);
}

static inline size_t size_smalltext(const Text& text) { return size_text<uint8_t>(text); }
static inline void serialize_smalltext(uint8_t** buffer, const Text& text) { serialize_text<uint8_t>(buffer, text); }
static inline QByteArray deserialize_smalltext(uint8_t** buffer) { return deserialize_text<uint8_t>(buffer); }
static inline size_t size_bigtext(const Text& text) { return size_text<uint32_t>(text); }
static inline void serialize_bigtext(uint8_t** buffer, const Text& text) { serialize_text<uint32_t>(buffer, text); }
static inline QByteArray deserialize_bigtext(uint8_t** buffer) { return deserialize_text<uint32_t>(buffer); }

template <typename T>
static inline void serialize_integer(uint8_t** buffer, T val)
{
	*reinterpret_cast<T*>(*buffer) = LOG_ENDIAN(val);
	*buffer += sizeof(T);
}

template <typename T>
static inline T deserialize_integer(uint8_t** buffer)
{
	T val;
	val = LOG_ENDIAN(*reinterpret_cast<T*>(*buffer));
	*buffer += sizeof(T);
	return val;
}

size_t SerializableHBaseOperation::MutateRows::size(bool deletes)
{
	size_t size = 0;
	num_rows = 0;

	size += sizeof(uint8_t); // Class
	size += size_smalltext(tableName); // Table
	size += sizeof(uint8_t); // Number of rows

	for (std::vector<BatchMutation>::const_iterator batch_mutation = row_batches.begin(); batch_mutation != row_batches.end(); batch_mutation++) {
		size_t ops_size = 0;
		std::map<std::string, std::vector<const Mutation*> > families;

		ops_size += size_bigtext(batch_mutation->row); // RowKey
		ops_size += sizeof(uint8_t); // Number of families

		for (std::vector<Mutation>::const_iterator mutation = batch_mutation->mutations.begin(); mutation != batch_mutation->mutations.end(); mutation++) {
			if (!((mutation->isDelete && deletes) || (!mutation->isDelete && !deletes)))
				continue;

			size_t pos = mutation->column.find_first_of(':');
			if (pos == std::string::npos || pos == (mutation->column.size() - 1)) {
				// TODO: WARN
				continue;
			}

			std::string family(mutation->column.substr(0, pos));

			ops_size += size_smalltext(mutation->column.substr(pos + 1)); // Qualifier
			ops_size += sizeof(int64_t); // Timestamp
			if (!deletes)
				ops_size += size_bigtext(mutation->value); // Value

			std::map<std::string, std::vector<const Mutation*> >::iterator i = families.find(family);
			if (i != families.end()) {
				i->second.push_back(&*mutation);
			} else {
				std::vector<const Mutation*> mutations;
				mutations.push_back(&*mutation);
				families[family] = mutations;
			}
		}

		for (std::map<std::string, std::vector<const Mutation*> >::const_iterator i = families.begin(); i != families.end(); i++) {
			ops_size += size_smalltext(i->first); // Family
			ops_size += sizeof(uint8_t); // Number of qualifiers
		}

		if (families.size() != 0) {
			size += ops_size;
			num_rows++;
		}
		
		families_per_row.push_back(families);
	}

	if (!num_rows)
		return 0;

	// Align size to 64 bits
	size = (size + (sizeof(uint64_t) - 1)) & ~(sizeof(uint64_t) - 1);
	return size;
}

void SerializableHBaseOperation::MutateRows::serialize(void* buffer_, bool deletes)
{
	uint8_t* buffer = reinterpret_cast<uint8_t*>(buffer_);

	serialize_integer(&buffer, (uint8_t)(deletes ? CLASS_DELETE_BATCH : CLASS_PUT_BATCH));
//	printf("%s in table %s (%u rows)\n", deletes ? "DELETE" : "PUT", tableName.c_str(), num_rows);
	serialize_smalltext(&buffer, tableName);
	serialize_integer(&buffer, num_rows);

	std::vector<BatchMutation>::const_iterator batch_mutation = row_batches.begin();
	std::vector<std::map<std::string, std::vector<const Mutation*> > >::const_iterator families = families_per_row.begin();
	for (; batch_mutation != row_batches.end(); batch_mutation++, families++) {
		if (families->size() == 0)
			continue;
		
//		printf("\trow '%s' (%u families)\n", batch_mutation->row.c_str(), (uint8_t)families->size());
		serialize_bigtext(&buffer, batch_mutation->row);
		serialize_integer(&buffer, (uint8_t)families->size());

		for (std::map<std::string, std::vector<const Mutation*> >::const_iterator family = families->begin(); family != families->end(); family++) {
//			printf("\t\tfamily '%s' (%u qualifiers)\n", family->first.c_str(), (uint8_t)family->second.size());
			serialize_smalltext(&buffer, family->first);
			serialize_integer(&buffer, (uint8_t)family->second.size());

			for (std::vector<const Mutation*>::const_iterator qualifier = family->second.begin(); qualifier != family->second.end(); qualifier++) {
				size_t pos = (*qualifier)->column.find_first_of(':');
//				printf("\t\t\tqualifier '%s' value '%s' timestamp %lld\n", (*qualifier)->column.substr(pos + 1).c_str(), (*qualifier)->value.c_str(), (int64_t)timestamp);
				serialize_smalltext(&buffer, (*qualifier)->column.substr(pos + 1));
				serialize_integer(&buffer, (int64_t)timestamp);
				if (!deletes)
					serialize_bigtext(&buffer, (*qualifier)->value);
			}
		}
	}
}

void DeserializableHBaseOperation::MutateRows::deserialize(void* buffer_)
{
	uint8_t* buffer = reinterpret_cast<uint8_t*>(buffer_);

	type_ = (typeof(type_)) deserialize_integer<uint8_t>(&buffer);
	table_ = deserialize_smalltext(&buffer);
//	printf("%s in table '%.*s'\n", type_ == CLASS_DELETE_BATCH ? "DELETE" : "PUT", table_.size(), table_.constData());
	
	for (uint8_t num_rows = deserialize_integer<uint8_t>(&buffer); num_rows; num_rows--) {
		QByteArray key(deserialize_bigtext(&buffer));
		QVector<FamilyValues> families;
//		printf("\trow '%.*s'\n", key.size(), key.constData());
		for (uint8_t num_families = deserialize_integer<uint8_t>(&buffer); num_families; num_families--) {
			QByteArray family(deserialize_smalltext(&buffer));
			QVector<QualifierValue> qualifiers;
//			printf("\t\tfamily '%.*s'\n", family.size(), family.constData());
			for (uint8_t num_qualifiers = deserialize_integer<uint8_t>(&buffer); num_qualifiers; num_qualifiers--) {
				QByteArray qualifier(deserialize_smalltext(&buffer));
				int64_t timestamp(deserialize_integer<int64_t>(&buffer));
				QByteArray value;
				if (type_ == CLASS_PUT_BATCH)
					value = deserialize_bigtext(&buffer);
//				printf("\t\t\tqualifier '%.*s' value '%.*s' timestamp %lld\n", qualifier.size(), qualifier.constData(), value.size(), value.constData(), timestamp);
				qualifiers.append(QualifierValue(qualifier, value, timestamp));
			}
			families.append(qMakePair(family, qualifiers));
		}
		rows_.append(qMakePair(key, families));
	}
}

