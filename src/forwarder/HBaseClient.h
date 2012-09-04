#ifndef HBASE_CLIENT_H
#define HBASE_CLIENT_H

#include "JavaObject.h"

#include <QByteArray>
#include <QFuture>

namespace AsyncHBase {
class HBaseClient;

// TODO: update API to asynchbase 1.3.1

#define DECLARE_HAS_PROPERTY(type, name, prop) \
	class Has##name { \
	public: \
		Has##name(const type& prop) : _##prop(prop) {} \
		type prop() const { return _##prop; } \
	private: \
		type _##prop; \
	};

class KeyValue {
public:
	static const long TIMESTAMP_NOW = LONG_MAX;
};

class HBaseRpc {
public:
	DECLARE_HAS_PROPERTY(QByteArray, Family, family);
	DECLARE_HAS_PROPERTY(QByteArray, Qualifier, qualifier);
	DECLARE_HAS_PROPERTY(QByteArray, Value, value);
	DECLARE_HAS_PROPERTY(long, Timestamp, timestamp);

	class BatchProcessable {
	public:
		virtual QFuture<void> process(HBaseClient* client) = 0;
		virtual ~BatchProcessable() {}
	};

	HBaseRpc(const QByteArray& table, const QByteArray& key) : table_(table), key_(key) {}
	virtual ~HBaseRpc() {}

	QByteArray table() const { return table_; }
	QByteArray key() const { return key_; }

private:
	QByteArray table_;
	QByteArray key_;
};

class BatchableRpc : public HBaseRpc, public HBaseRpc::HasFamily, public HBaseRpc::HasTimestamp {
public:
	BatchableRpc(const QByteArray& table, const QByteArray& key, const QByteArray& family, long timestamp, bool bufferable = true, bool durable = true) :
		HBaseRpc(table, key), HBaseRpc::HasFamily(family), HBaseRpc::HasTimestamp(timestamp), bufferable_(bufferable), durable_(durable)
	{}
	virtual ~BatchableRpc() {}

	void set_bufferable(bool bufferable) { bufferable_ = bufferable; }
	void set_durable(bool durable) { durable_ = durable; }

private:
	bool bufferable_;
	bool durable_;
};

class PutRequest : public JavaObject, public BatchableRpc, public HBaseRpc::HasQualifier, public HBaseRpc::HasValue, public HBaseRpc::BatchProcessable {
	DECLARE_JAVA_CLASS_NAME

public:
	PutRequest(const QByteArray& table, const QByteArray& key, const QByteArray& family, const QByteArray& qualifier, const QByteArray& value, long timestamp = KeyValue::TIMESTAMP_NOW) :
		JavaObject(), BatchableRpc(table, key, family, timestamp), HBaseRpc::HasQualifier(qualifier), HBaseRpc::HasValue(value)
	{}
	virtual QFuture<void> process(HBaseClient* client);
	virtual ~PutRequest() {}

	jobject getJObject() const;
};

class DeleteRequest : public JavaObject, public BatchableRpc, public HBaseRpc::HasQualifier, public HBaseRpc::BatchProcessable {
	DECLARE_JAVA_CLASS_NAME

public:
	DeleteRequest(const QByteArray& table, const QByteArray& key, const QByteArray& family = QByteArray(), const QByteArray& qualifier = QByteArray(), long timestamp = KeyValue::TIMESTAMP_NOW) :
		JavaObject(), BatchableRpc(table, key, family, timestamp), HBaseRpc::HasQualifier(qualifier)
	{}
	virtual QFuture<void> process(HBaseClient* client);
	virtual ~DeleteRequest() {}

	jobject getJObject() const;
};

class HBaseClient : public JavaObject {
	DECLARE_JAVA_CLASS_NAME

public:
	explicit HBaseClient(const QByteArray& quorum_spec, const QByteArray& base_spec = QByteArray());

	long contendedMetaLookupCount() const;
	short getFlushInterval() const;
	QFuture<void> put(const PutRequest& request);
	QFuture<void> del(const DeleteRequest& request);
	long rootLookupCount() const;
	short setFlushInterval(short flush_interval);
	QFuture<void> shutdown();
	long uncontendedMetaLookupCount() const;
};

inline QFuture<void> PutRequest::process(HBaseClient* client)
{
	return client->put(*this);
}

inline QFuture<void> DeleteRequest::process(HBaseClient* client)
{
	return client->del(*this);
}

class HBaseException : public QtConcurrent::Exception {
public:
	explicit HBaseException(const QString& message) : message_(message) {}
	virtual ~HBaseException() throw() {}

	QString message() const { return message_; }
	virtual QString name() const { return QString("HBaseException"); }

	virtual HBaseException* clone() const { return new HBaseException(*this); }
	virtual void raise() const { throw *this; }
private:
	QString message_;
};

#define DECLARE_EXCEPTION(ename, base) \
	class ename : public base { \
	public: \
		ename(const QString& message) : base(message) {} \
		virtual ~ename() throw() {} \
		virtual base* clone() const { return new ename(*this); } \
		virtual void raise() const { throw *this; } \
		virtual QString name() const { return QString(#ename); }; \
	};

DECLARE_EXCEPTION(RecoverableException, HBaseException)
DECLARE_EXCEPTION(NonRecoverableException, HBaseException)

DECLARE_EXCEPTION(ConnectionResetException, RecoverableException)
DECLARE_EXCEPTION(NotServingRegionException, RecoverableException)
DECLARE_EXCEPTION(RegionOfflineException, RecoverableException)
DECLARE_EXCEPTION(UnknownScannerException, RecoverableException)

DECLARE_EXCEPTION(BrokenMetaException, NonRecoverableException)
DECLARE_EXCEPTION(InvalidResponseException, NonRecoverableException)
DECLARE_EXCEPTION(NoSuchColumnFamilyException, NonRecoverableException)
DECLARE_EXCEPTION(PleaseThrottleException, NonRecoverableException)
DECLARE_EXCEPTION(RemoteException, NonRecoverableException)
DECLARE_EXCEPTION(TableNotFoundException, NonRecoverableException)
DECLARE_EXCEPTION(UnknownRowLockException, NonRecoverableException)
DECLARE_EXCEPTION(VersionMismatchException, NonRecoverableException)

} // namespace AsyncHBase

#endif // HBASE_CLIENT_H
