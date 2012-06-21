TEMPLATE = app
TARGET = asyncthrift-logger

include($$TOP_SRCDIR/asyncthrift.pri)

PKGCONFIG += thrift-nb libevent liblzma
LIBS += -levent

DESTDIR = $$TOP_BUILDDIR/target/$$BINDIR

target.path = $$PREFIX$$BINDIR
INSTALLS += target

QMAKE_CLEAN += ${TARGET}

LIBS += -L$$TOP_BUILDDIR/src/common/ -lcommon
PRE_TARGETDEPS = $$TOP_BUILDDIR/src/common/libcommon.a

HEADERS += \
	AsyncThriftLogger.h \
	HBaseHandler.h \
	HBaseHandler_p.h \
	ThriftDispatcher.h \
	LogStorageManager.h \
	LogStorageManager_p.h

SOURCES += \
	AsyncThriftLogger.cpp \
	HBaseHandler.cpp \
	ThriftDispatcher.cpp \
	LogStorageManager.cpp

