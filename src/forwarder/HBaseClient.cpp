#include "HBaseClient.h"

#include <pthread.h>

namespace AsyncHBase {

static pthread_once_t initialized = PTHREAD_ONCE_INIT;
static void initialize();

class Deferred : public QFutureInterface<void>, public JavaObject {
	DECLARE_JAVA_CLASS_NAME

public:
	Deferred(jobject& object);
	~Deferred();

	jobject call(jobject arg);
	void connect();

	static QFuture<void> newFuture(jobject& object);
private:
	jobject cb;
};

DEFINE_JAVA_CLASS_NAME(HBaseClient, "org/hbase/async/HBaseClient");

HBaseClient::HBaseClient(const QByteArray& quorum_spec, const QByteArray& base_spec) : JavaObject()
{
	pthread_once(&initialized, initialize);

	jstring quorum = getJNIEnv()->NewStringUTF(quorum_spec.data());
	if (!base_spec.isNull()) {
		jstring base = getJNIEnv()->NewStringUTF(base_spec.data());
		constructNewObject("(Ljava/lang/String;Ljava/lang/String;)V", quorum, base);
	} else {
		constructNewObject("(Ljava/lang/String;)V", quorum);
	}
}

long HBaseClient::contendedMetaLookupCount() const
{
	return invokeInstanceMethod("contendedMetaLookupCount", "()J").j;
}

short HBaseClient::getFlushInterval() const
{
	return invokeInstanceMethod("getFlushInterval", "()S").s;
}

QFuture<void> HBaseClient::put(const PutRequest& request)
{
	jobject o = invokeInstanceMethod("put", "(Lorg/hbase/async/PutRequest;)Lcom/stumbleupon/async/Deferred;", request.getJObject()).l;
	return Deferred::newFuture(o);
}

QFuture<void> HBaseClient::del(const DeleteRequest& request)
{
	jobject o = invokeInstanceMethod("delete", "(Lorg/hbase/async/DeleteRequest;)Lcom/stumbleupon/async/Deferred;", request.getJObject()).l;
	return Deferred::newFuture(o);
}

long HBaseClient::rootLookupCount() const
{
	return invokeInstanceMethod("rootLookupCount", "()J").j;
}

short HBaseClient::setFlushInterval(short flush_interval)
{
	return invokeInstanceMethod("setFlushInterval", "(S)S", flush_interval).s;
}

QFuture<void> HBaseClient::shutdown()
{
	jobject o = invokeInstanceMethod("shutdown", "()Lcom/stumbleupon/async/Deferred;").l;
	return Deferred::newFuture(o);
}

long HBaseClient::uncontendedMetaLookupCount() const
{
	return invokeInstanceMethod("uncontendedMetaLookupCount", "()J").j;
}

static const char* generic_callback_class_name = "com/tuenti/async/GenericCallback";

JNIEXPORT jobject JNICALL Java_com_tuenti_async_GenericCallback_call(JNIEnv* env, jobject self, jobject arg)
{
	jclass cls = env->FindClass(generic_callback_class_name);
	jfieldID fid = env->GetFieldID(cls, "native_object", "J");
	Deferred* native_object = reinterpret_cast<Deferred*>(env->GetLongField(self, fid));
	jobject res = native_object->call(arg);
	delete native_object;
	return res;
}

static JNINativeMethod methods[] = {
	{(char *)"call", (char *)"(Ljava/lang/Object;)Ljava/lang/Object;", (void *)&Java_com_tuenti_async_GenericCallback_call}
};

static void initialize()
{
	JNIEnv* env = ::getJNIEnv();
	jclass cls = env->FindClass(generic_callback_class_name);
	env->RegisterNatives(cls, methods, sizeof(methods)/sizeof(methods[0]));
	env->DeleteLocalRef(cls);
}

DEFINE_JAVA_CLASS_NAME(Deferred, "com/stumbleupon/async/Deferred");

Deferred::Deferred(jobject& object) : QFutureInterface<void>(QFutureInterfaceBase::Running), JavaObject()
{
	setJObjectLocal(object);
}

QFuture<void> Deferred::newFuture(jobject& object)
{
	Deferred* native = new Deferred(object);
	QFuture<void> f = native->future();
	native->connect();
	return f;
}

void Deferred::connect()
{
	JNIEnv* env = getJNIEnv();

	jobject _cb = constructNewObjectOfClass(env, NULL, generic_callback_class_name, "()V");
	cb = env->NewGlobalRef(_cb);
	env->DeleteLocalRef(_cb);
	jclass cls = env->FindClass(generic_callback_class_name);
	jfieldID fid = env->GetFieldID(cls, "native_object", "J");
	env->SetLongField(cb, fid, (long) this);
	env->DeleteLocalRef(cls);
	env->DeleteLocalRef(invokeInstanceMethod("addBoth", "(Lcom/stumbleupon/async/Callback;)Lcom/stumbleupon/async/Deferred;", cb).l);
}

Deferred::~Deferred()
{
	getJNIEnv()->DeleteGlobalRef(cb);
}

jobject Deferred::call(jobject arg)
{
	JNIEnv* env = getJNIEnv();

	if (arg && env->IsInstanceOf(arg, globalClassReference("java/lang/Exception", env))) {
		jclass exccls(env->GetObjectClass(arg));
		jclass clscls(env->FindClass("java/lang/Class"));

		jmethodID getName(env->GetMethodID(clscls, "getName", "()Ljava/lang/String;"));
		jstring name(static_cast<jstring>(env->CallObjectMethod(exccls, getName)));
		const char* utfName(env->GetStringUTFChars(name, 0));

		jmethodID getMessage(env->GetMethodID(exccls, "getMessage", "()Ljava/lang/String;"));
		jstring message(static_cast<jstring>(env->CallObjectMethod(arg, getMessage)));
		const char* utfMessage(env->GetStringUTFChars(message, 0));

		env->DeleteLocalRef(exccls);
		env->DeleteLocalRef(clscls);

		if (strcmp(utfName, "org.hbase.async.ConnectionResetException") == 0) {
			this->reportException(ConnectionResetException(utfMessage));
		} else if (strcmp(utfName, "org.hbase.async.NotServingRegionException") == 0) {
			this->reportException(NotServingRegionException(utfMessage));
		} else if (strcmp(utfName, "org.hbase.async.RegionOfflineException") == 0) {
			this->reportException(RegionOfflineException(utfMessage));
		} else if (strcmp(utfName, "org.hbase.async.UnknownScannerException") == 0) {
			this->reportException(UnknownScannerException(utfMessage));
		} else if (strcmp(utfName, "org.hbase.async.BrokenMetaException") == 0) {
			this->reportException(BrokenMetaException(utfMessage));
		} else if (strcmp(utfName, "org.hbase.async.InvalidResponseException") == 0) {
			this->reportException(InvalidResponseException(utfMessage));
		} else if (strcmp(utfName, "org.hbase.async.NoSuchColumnFamilyException") == 0) {
			this->reportException(NoSuchColumnFamilyException(utfMessage));
		} else if (strcmp(utfName, "org.hbase.async.PleaseThrottleException") == 0) {
			this->reportException(PleaseThrottleException(utfMessage));
		} else if (strcmp(utfName, "org.hbase.async.RemoteException") == 0) {
			this->reportException(RemoteException(utfMessage));
		} else if (strcmp(utfName, "org.hbase.async.TableNotFoundException") == 0) {
			this->reportException(TableNotFoundException(utfMessage));
		} else if (strcmp(utfName, "org.hbase.async.UnknownRowLockException") == 0) {
			this->reportException(UnknownRowLockException(utfMessage));
		} else if (strcmp(utfName, "org.hbase.async.VersionMismatchException") == 0) {
			this->reportException(VersionMismatchException(utfMessage));
		} else if (strcmp(utfName, "org.hbase.async.RecoverableException") == 0) {
			this->reportException(RecoverableException(utfMessage));
		} else if (strcmp(utfName, "org.hbase.async.NonRecoverableException") == 0) {
			this->reportException(NonRecoverableException(utfMessage));
		} else {
			this->reportException(HBaseException(utfMessage));
		}

		env->ReleaseStringUTFChars(name, utfName);
		env->ReleaseStringUTFChars(message, utfMessage);
	}

	this->reportFinished();

	return arg;
}

static jbyteArray jbyteArray_from_QByteArray(const QByteArray& array, JNIEnv* env)
{
	jbyteArray a = env->NewByteArray(array.size());
	env->SetByteArrayRegion(a, 0, array.size(), (const jbyte*) array.data());
	return a;
}

DEFINE_JAVA_CLASS_NAME(PutRequest, "org/hbase/async/PutRequest");

jobject PutRequest::getJObject() const
{
	JNIEnv* env = getJNIEnv();

	jbyteArray table_array = jbyteArray_from_QByteArray(table(), env);
	jbyteArray key_array = jbyteArray_from_QByteArray(key(), env);
	jbyteArray family_array = jbyteArray_from_QByteArray(family(), env);
	jbyteArray qualifier_array = jbyteArray_from_QByteArray(qualifier(), env);
	jbyteArray value_array = jbyteArray_from_QByteArray(value(), env);

	constructNewObject("([B[B[B[B[BJ)V", table_array, key_array, family_array, qualifier_array, value_array, timestamp());

	env->DeleteLocalRef(table_array);
	env->DeleteLocalRef(key_array);
	env->DeleteLocalRef(family_array);
	env->DeleteLocalRef(qualifier_array);
	env->DeleteLocalRef(value_array);
	
	return JavaObject::getJObject();
}

DEFINE_JAVA_CLASS_NAME(DeleteRequest, "org/hbase/async/DeleteRequest");

jobject DeleteRequest::getJObject() const
{
	JNIEnv* env = getJNIEnv();

	jbyteArray table_array = jbyteArray_from_QByteArray(table(), env);
	jbyteArray key_array = jbyteArray_from_QByteArray(key(), env);

	if (!family().isEmpty()) {
		jbyteArray family_array = jbyteArray_from_QByteArray(family(), env);

		if (!qualifier().isEmpty()) {
			jbyteArray qualifier_array = jbyteArray_from_QByteArray(qualifier(), env);

			constructNewObject("([B[B[B[BJ)V", table_array, key_array, family_array, qualifier_array, timestamp());
			env->DeleteLocalRef(qualifier_array);
		} else {
			constructNewObject("([B[B[BJ)V", table_array, key_array, family_array, timestamp());
		}
		env->DeleteLocalRef(family_array);
	} else {
		constructNewObject("([B[BJ)V", table_array, key_array, timestamp());
	}

	env->DeleteLocalRef(table_array);
	env->DeleteLocalRef(key_array);
	
	return JavaObject::getJObject();
}

} // namespace AsyncHBase
