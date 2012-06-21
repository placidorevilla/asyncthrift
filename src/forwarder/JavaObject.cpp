#include <stdarg.h>
#include <string.h>

#include "JavaObject.h"

DEFINE_JAVA_CLASS_NAME(JavaObject, "");

JavaObject::JavaObject(jobject object) : object(object)
{
}

JavaObject::~JavaObject()
{
	if (object)
		getJNIEnv()->DeleteGlobalRef(object);
}

void JavaObject::setJObjectLocal(jobject object_)
{
	setJObject(object_);
	getJNIEnv()->DeleteLocalRef(object_);
}

void JavaObject::setJObject(jobject object_)
{
	// TODO: check class_name of jobject
	if (object)
		getJNIEnv()->DeleteGlobalRef(object);
	object = getJNIEnv()->NewGlobalRef(object_);
}

jvalue JavaObject::invokeInstanceMethod(const char* name, const char* signature, ...) const
{
	JNIEnv* env = getJNIEnv();
	jvalue jval;
	jthrowable jexc = 0;
	va_list args;
	va_start(args, signature);
	::invokeMethodV(env, &jval, &jexc, INSTANCE, getJObject(), getJavaClassName(), name, signature, args);
	va_end(args);
	if (jexc)
		throw JavaException(jexc);
	return jval;
}

void JavaObject::constructNewObject(const char* signature, ...) const
{
	JNIEnv* env = getJNIEnv();
	jthrowable jexc = 0;
	va_list args;
	va_start(args, signature);
	jobject new_obj = constructNewObjectOfClassV(env, &jexc, getJavaClassName(), signature, args);
	va_end(args);
	if (jexc)
		throw JavaException(jexc);
	if (object)
		env->DeleteGlobalRef(object);
	object = env->NewGlobalRef(new_obj);
	env->DeleteLocalRef(new_obj);
}

JavaException::JavaException(jobject jexc) : JavaObject(), exception_name(0), exception_message(0)
{
	setJObjectLocal(jexc);
}

JavaException::~JavaException() throw()
{
	free(exception_name);
	free(exception_message);
	getJNIEnv()->ExceptionClear();
}

const char* JavaException::name() const
{
	if (!exception_name) {
		JNIEnv *env = getJNIEnv();

		jclass exccls(env->GetObjectClass(getJObject()));
		jclass clscls(env->FindClass("java/lang/Class"));

		jmethodID getName(env->GetMethodID(clscls, "getName", "()Ljava/lang/String;"));
		jstring name(static_cast<jstring>(env->CallObjectMethod(exccls, getName)));
		const char* utfName(env->GetStringUTFChars(name, 0));
		exception_name = strdup(utfName);
		env->ReleaseStringUTFChars(name, utfName);
	}

	return exception_name;
}

const char* JavaException::what() const throw()
{
	if (!exception_message) {
		JNIEnv *env = getJNIEnv();

		jclass exccls(env->GetObjectClass(getJObject()));

		jmethodID getMessage(env->GetMethodID(exccls, "getMessage", "()Ljava/lang/String;"));
		jstring message(static_cast<jstring>(env->CallObjectMethod(getJObject(), getMessage)));
		const char* utfMessage(env->GetStringUTFChars(message, 0));
		exception_message = strdup(utfMessage);
		env->ReleaseStringUTFChars(message, utfMessage);
	}

	return exception_message;
}

JByteArray::JByteArray(const char* data, int size) : env(::getJNIEnv()), buffer(0)
{
	size = size != -1 ? size : strlen(data);
	jbyteArray a = env->NewByteArray(size);
	array = static_cast<jbyteArray>(env->NewGlobalRef(a));
	env->DeleteLocalRef(a);
	env->SetByteArrayRegion(array, 0, size, (const jbyte*)data);
}

JByteArray::JByteArray() : env(::getJNIEnv()), array(0), buffer(0)
{
}

JByteArray::~JByteArray()
{
	if (array)
		env->DeleteGlobalRef(array);
	if (buffer)
		free(buffer);
}

JByteArray* JByteArray::fromJObject(jobject object)
{
	// TODO: check object type
	JByteArray* a = new JByteArray();
	a->array = static_cast<jbyteArray>(object);
	return a;
}

const char* JByteArray::c_str() const
{
	int len = env->GetArrayLength(array);
	if (buffer)
		free(buffer);
	jbyte* elems = env->GetByteArrayElements(array, NULL);
	if (elems)
		buffer = strndup((char*)elems, len);
	else
		buffer = 0;
	env->ReleaseByteArrayElements(array, elems, JNI_ABORT);
	return buffer;
}

