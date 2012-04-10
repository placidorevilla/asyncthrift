#ifndef JAVAOBJECT_H
#define JAVAOBJECT_H

#include <exception>
#include <memory>

#include "JniHelper.h"

#define DECLARE_JAVA_CLASS_NAME \
	public: \
		virtual const char* getJavaClassName() const { return java_class_name; } \
	private: \
		static const char* java_class_name;

#define DEFINE_JAVA_CLASS_NAME(cpp_class, java_class) \
	const char* cpp_class::java_class_name = java_class;

class JavaObject {
	DECLARE_JAVA_CLASS_NAME

public:
	JavaObject(jobject _object = 0);
	~JavaObject();

	jobject getJObject() const { return object; }
	JNIEnv* getJNIEnv() const { return ::getJNIEnv(); }

	void setJObjectLocal(jobject _object);
	void setJObject(jobject _object);

	jvalue invokeInstanceMethod(const char* name, const char* signature, ...) const;

protected:
	void constructNewObject(const char* signature, ...) const;

private:
	mutable jobject object;
};

class JavaException : public std::exception, public JavaObject {
public:
	JavaException(jobject jexc);
	virtual ~JavaException() throw();

	const char* name() const;
	virtual const char* what() const throw();

private:
	mutable char* exception_name;
	mutable char* exception_message;
};

class JByteArray {
public:
	JByteArray(const char* data, int size = -1);
	~JByteArray();

	jbyteArray getJArray() const { return array; }
	const char* c_str() const;

	static JByteArray* fromJObject(jobject _object);

private:
	JByteArray();

	JNIEnv* env;
	jbyteArray array;
	mutable char* buffer;
};

template<class T> struct JPointer {
	typedef std::shared_ptr<T> Type;
};

typedef JPointer<JByteArray>::Type JByteArrayP;
#define JBAP(x) JByteArrayP(new JByteArray((x)))

#endif // JAVAOBJECT_H
