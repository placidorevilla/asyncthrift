TEMPLATE = app
TARGET = asyncthrift-forwarder

include($$TOP_SRCDIR/asyncthrift.pri)

INCLUDEPATH += $$JAVA_HOME/include $$JAVA_HOME/include/linux
LIBS += $$JAVA_HOME/jre/lib/$$ARCH/server/libjvm.so

DESTDIR = $$TOP_BUILDDIR/target/$$BINDIR

target.path = $$PREFIX$$BINDIR
INSTALLS += target

QMAKE_CLEAN += ${TARGET}

LIBS += -L$$TOP_BUILDDIR/src/common/ -lcommon

PKGCONFIG += thrift

HEADERS += \
	AsyncThriftForwarder.h \
	ForwarderManager.h \
	ForwarderManager_p.h \
	JniHelper.h \
	JavaObject.h \
	HBaseClient.h

SOURCES += \
	AsyncThriftForwarder.cpp \
	ForwarderManager.cpp \
	JniHelper.c \
	JavaObject.cpp \
	HBaseClient.cpp

