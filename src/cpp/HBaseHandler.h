#ifndef HBASEHANDLER_H
#define HBASEHANDLER_H

#include "Hbase.h"

#include <Thrift.h>

#include <log4cxx/logger.h>

#include <QObject>

using namespace apache::thrift;
using namespace apache::hadoop::hbase::thrift;

class HBaseHandlerPrivate;

class HBaseHandler : public QObject, virtual public HbaseIf {
	Q_OBJECT
	Q_DISABLE_COPY(HBaseHandler)

public:
	HBaseHandler();
	virtual ~HBaseHandler();
	void enableTable(const Bytes& /* tableName */) {
		throw TException("Not implemented");
		return;
	}
	void disableTable(const Bytes& /* tableName */) {
		throw TException("Not implemented");
		return;
	}
	bool isTableEnabled(const Bytes& /* tableName */) {
		throw TException("Not implemented");
		bool _return = false;
		return _return;
	}
	void compact(const Bytes& /* tableNameOrRegionName */) {
		throw TException("Not implemented");
		return;
	}
	void majorCompact(const Bytes& /* tableNameOrRegionName */) {
		throw TException("Not implemented");
		return;
	}
	void getTableNames(std::vector<Text> & /* _return */) {
		throw TException("Not implemented");
		return;
	}
	void getColumnDescriptors(std::map<Text, ColumnDescriptor> & /* _return */, const Text& /* tableName */) {
		throw TException("Not implemented");
		return;
	}
	void getTableRegions(std::vector<TRegionInfo> & /* _return */, const Text& /* tableName */) {
		throw TException("Not implemented");
		return;
	}
	void createTable(const Text& /* tableName */, const std::vector<ColumnDescriptor> & /* columnFamilies */) {
		throw TException("Not implemented");
		return;
	}
	void deleteTable(const Text& /* tableName */) {
		throw TException("Not implemented");
		return;
	}
	void get(std::vector<TCell> & /* _return */, const Text& /* tableName */, const Text& /* row */, const Text& /* column */) {
		throw TException("Not implemented");
		return;
	}
	void getVer(std::vector<TCell> & /* _return */, const Text& /* tableName */, const Text& /* row */, const Text& /* column */, const int32_t /* numVersions */) {
		throw TException("Not implemented");
		return;
	}
	void getVerTs(std::vector<TCell> & /* _return */, const Text& /* tableName */, const Text& /* row */, const Text& /* column */, const int64_t /* timestamp */, const int32_t /* numVersions */) {
		throw TException("Not implemented");
		return;
	}
	void getRow(std::vector<TRowResult> & /* _return */, const Text& /* tableName */, const Text& /* row */) {
		throw TException("Not implemented");
		return;
	}
	void getRowWithColumns(std::vector<TRowResult> & /* _return */, const Text& /* tableName */, const Text& /* row */, const std::vector<Text> & /* columns */) {
		throw TException("Not implemented");
		return;
	}
	void getRowTs(std::vector<TRowResult> & /* _return */, const Text& /* tableName */, const Text& /* row */, const int64_t /* timestamp */) {
		throw TException("Not implemented");
		return;
	}
	void getRowWithColumnsTs(std::vector<TRowResult> & /* _return */, const Text& /* tableName */, const Text& /* row */, const std::vector<Text> & /* columns */, const int64_t /* timestamp */) {
		throw TException("Not implemented");
		return;
	}
	void getRows(std::vector<TRowResult> & /* _return */, const Text& /* tableName */, const std::vector<Text> & /* rows */) {
		throw TException("Not implemented");
		return;
	}
	void getRowsWithColumns(std::vector<TRowResult> & /* _return */, const Text& /* tableName */, const std::vector<Text> & /* rows */, const std::vector<Text> & /* columns */) {
		throw TException("Not implemented");
		return;
	}
	void getRowsTs(std::vector<TRowResult> & /* _return */, const Text& /* tableName */, const std::vector<Text> & /* rows */, const int64_t /* timestamp */) {
		throw TException("Not implemented");
		return;
	}
	void getRowsWithColumnsTs(std::vector<TRowResult> & /* _return */, const Text& /* tableName */, const std::vector<Text> & /* rows */, const std::vector<Text> & /* columns */, const int64_t /* timestamp */) {
		throw TException("Not implemented");
		return;
	}
	void mutateRow(const Text& tableName, const Text& row, const std::vector<Mutation> & mutations);

	void mutateRowTs(const Text& /* tableName */, const Text& /* row */, const std::vector<Mutation> & /* mutations */, const int64_t /* timestamp */) {
		throw TException("Not implemented");
		return;
	}
	void mutateRows(const Text& /* tableName */, const std::vector<BatchMutation> & /* rowBatches */) {
		throw TException("Not implemented");
		return;
	}
	void mutateRowsTs(const Text& /* tableName */, const std::vector<BatchMutation> & /* rowBatches */, const int64_t /* timestamp */) {
		throw TException("Not implemented");
		return;
	}
	int64_t atomicIncrement(const Text& /* tableName */, const Text& /* row */, const Text& /* column */, const int64_t /* value */) {
		throw TException("Not implemented");
		int64_t _return = 0;
		return _return;
	}
	void deleteAll(const Text& /* tableName */, const Text& /* row */, const Text& /* column */) {
		throw TException("Not implemented");
		return;
	}
	void deleteAllTs(const Text& /* tableName */, const Text& /* row */, const Text& /* column */, const int64_t /* timestamp */) {
		throw TException("Not implemented");
		return;
	}
	void deleteAllRow(const Text& /* tableName */, const Text& /* row */) {
		throw TException("Not implemented");
		return;
	}
	void deleteAllRowTs(const Text& /* tableName */, const Text& /* row */, const int64_t /* timestamp */) {
		throw TException("Not implemented");
		return;
	}
	ScannerID scannerOpen(const Text& /* tableName */, const Text& /* startRow */, const std::vector<Text> & /* columns */) {
		throw TException("Not implemented");
		ScannerID _return = 0;
		return _return;
	}
	ScannerID scannerOpenWithStop(const Text& /* tableName */, const Text& /* startRow */, const Text& /* stopRow */, const std::vector<Text> & /* columns */) {
		throw TException("Not implemented");
		ScannerID _return = 0;
		return _return;
	}
	ScannerID scannerOpenWithPrefix(const Text& /* tableName */, const Text& /* startAndPrefix */, const std::vector<Text> & /* columns */) {
		throw TException("Not implemented");
		ScannerID _return = 0;
		return _return;
	}
	ScannerID scannerOpenTs(const Text& /* tableName */, const Text& /* startRow */, const std::vector<Text> & /* columns */, const int64_t /* timestamp */) {
		throw TException("Not implemented");
		ScannerID _return = 0;
		return _return;
	}
	ScannerID scannerOpenWithStopTs(const Text& /* tableName */, const Text& /* startRow */, const Text& /* stopRow */, const std::vector<Text> & /* columns */, const int64_t /* timestamp */) {
		throw TException("Not implemented");
		ScannerID _return = 0;
		return _return;
	}
	void scannerGet(std::vector<TRowResult> & /* _return */, const ScannerID /* id */) {
		throw TException("Not implemented");
		return;
	}
	void scannerGetList(std::vector<TRowResult> & /* _return */, const ScannerID /* id */, const int32_t /* nbRows */) {
		throw TException("Not implemented");
		return;
	}
	void scannerClose(const ScannerID /* id */) {
		throw TException("Not implemented");
		return;
	}
private:
	HBaseHandlerPrivate* d;

	static log4cxx::LoggerPtr logger;
};

#endif // HBASEHANDLER_H
