#ifndef JAVAOBJECT_H
#define JAVAOBJECT_H

#include <exception>
#include <memory>

#include <QThreadStorage>

#include "JniHelper.h"

class JavaEnvironment {
public:
	JavaEnvironment() : env(::getJNIEnv()) { }
	~JavaEnvironment() { }

	JNIEnv* environment() const { return env; }
	
private:
	JNIEnv* env;
};

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
	JavaObject(jobject object = 0);
	virtual ~JavaObject();

	jobject getJObject() const { return object; }
	JNIEnv* getJNIEnv() const
	{
		if (!thread_environment.hasLocalData()) {
			JavaEnvironment* env = new JavaEnvironment;
			thread_environment.setLocalData(env);
			return env->environment();
		} else {
			return thread_environment.localData()->environment();
		}
	}

	void setJObjectLocal(jobject object);
	void setJObject(jobject object);

	jvalue invokeInstanceMethod(const char* name, const char* signature, ...) const;

protected:
	void constructNewObject(const char* signature, ...) const;

private:
	mutable jobject object;
	static QThreadStorage<JavaEnvironment*> thread_environment;
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

	static JByteArray* fromJObject(jobject object);

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
