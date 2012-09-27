#include <TMemFile.h>
#include <HBaseOperations.h>
#include <LogEndian.h>

QByteArray decode_user(unsigned char* buffer)
{
	uint32_t u = buffer[0] + (buffer[1] << 8) + (buffer[2] << 16) + (buffer[3] << 24);
	return QByteArray::number(u);
}

QByteArray decode_conversation(unsigned char* buffer)
{
	QByteArray result;
	if (buffer[0] == 0) {
		result.append("P u1: ");
		result.append(decode_user(buffer + 1));
		result.append(" u2: ");
		result.append(decode_user(buffer + 5));
	} else if (buffer[0] == 1) {
		result.append("G c: ");
		result.append(decode_user(buffer + 1));
		// TODO: timestamp
	}
	return result;
}

QByteArray decode_key(QByteArray table, QByteArray key)
{
	if (table == "bin_user_conversations") {
		return "{u: " + decode_user((unsigned char*)key.data()) + " " + decode_conversation(((unsigned char*)key.data() + 4)) + "}";
	} else if (table == "bin_conversation_messages") {
		return "{" + decode_conversation((unsigned char*)key.data()) + /*decode_timestamp((unsigned char*)key.data() + 9) + */ " a: " + decode_user((unsigned char*)key.data() + 17) + "}";
	} else if (table == "bin_user_status") {
		return "{u: " + decode_user((unsigned char*)key.data()) + "}";
	}
	return key.toHex();
}

QByteArray escape(QByteArray buffer)
{
	QByteArray result;
	for (int i = 0; i < buffer.size(); i++) {
		if (buffer.at(i) >= 32/* || buffer.at(i) <= 127*/) {
			result.append(buffer.at(i));
		} else {
			result.append("\\x");
			result.append(QByteArray(1, buffer.at(i)).toHex());
		}
	}
	return result;
}

void dump(TMemFile* file)
{
	uchar* buffer = file->buffer();
	while ((buffer - file->buffer()) < file->size() && *((uint64_t*) buffer)) {
		DeserializableHBaseOperation::MutateRows mutate;

		uint64_t transaction = LOG_ENDIAN(*((uint64_t*) buffer));
		uint32_t timestamp = LOG_ENDIAN(*(((uint64_t*) buffer) + 1)) & 0xFFFFFFFF;
		size_t len = (LOG_ENDIAN(*(((uint64_t*) buffer) + 1)) >> 32) & 0xFFFFFFFF;
		uint64_t crc = LOG_ENDIAN(*((uint64_t*) (buffer + len)));

		mutate.deserialize(buffer + sizeof(uint64_t) * 2); // Skip transaction and timestamp

		printf("%10ld: ts: %8d (len: %ld, crc: %016lx) %s %.*s: [", transaction, timestamp, len, crc, mutate.type() == HBaseOperation::CLASS_DELETE_BATCH ? "DEL" : "PUT", mutate.table().size(), mutate.table().data());

		if (mutate.type() == HBaseOperation::CLASS_DELETE_BATCH || mutate.type() == HBaseOperation::CLASS_PUT_BATCH) {
			bool first_row = true;
			foreach (const DeserializableHBaseOperation::RowValues& row, mutate.rows()) {
				printf("%s%s", !first_row ? ", " : "", decode_key(mutate.table(), row.first).data());
				first_row = false;
				if (row.second.size() != 0)
					printf(" [");
				bool first_family = true;
				foreach (const DeserializableHBaseOperation::FamilyValues& family, row.second) {
					printf("%s%.*s", !first_family ? ", " : "", family.first.size(), family.first.data());
					first_family = false;
					if (family.second.size() != 0)
						printf(" [");
					bool first_qv = true;
					foreach (const DeserializableHBaseOperation::QualifierValue& qv, family.second) {
						printf("%s%.*s", !first_qv ? ", " : "", qv.qualifier().size(), qv.qualifier().data());
						first_qv = false;
						if (mutate.type() == HBaseOperation::CLASS_PUT_BATCH) {
							printf(": '%s'", escape(qv.value()).data());
						}
						printf(" (%ld)", qv.timestamp());
					}
					if (family.second.size() != 0)
						printf("]");
				}
				if (row.second.size() != 0)
					printf("]");
			}
		} else {
			printf("INVALID");
		}

		printf("]\n");
		buffer += len + 3 * sizeof(uint64_t);
	}
}

int main(int /*argc*/, char** argv)
{
	while (*++argv) {
		TMemFile* file = new TMemFile(*argv);
		if (file->open(QIODevice::ReadOnly))
			dump(file);
		delete file;
	}
	return 0;
}
