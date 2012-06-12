TEMPLATE = app
TARGET = asyncthrift-logger

include($$TOP_SRCDIR/asyncthrift.pri)

DEPENDPATH += gen-cpp
INCLUDEPATH += gen-cpp

PKGCONFIG += thrift thrift-nb libevent liblzma
LIBS += -levent

DESTDIR = $$TOP_BUILDDIR/target/$$BINDIR

target.path = $$PREFIX$$BINDIR
INSTALLS += target

QMAKE_CLEAN += ${TARGET}

LIBS += -L$$TOP_BUILDDIR/src/common/ -lcommon

HEADERS += \
	gen-cpp/Hbase_types.h \
	gen-cpp/Hbase_constants.h \
	gen-cpp/Hbase.h \
	AsyncThriftLogger.h \
	HBaseHandler.h \
	HBaseHandler_p.h \
	ThriftDispatcher.h \
	LogStorageManager.h \
	LogStorageManager_p.h \
	HBaseOperations.h

SOURCES += \
	gen-cpp/Hbase_constants.cpp \
	gen-cpp/Hbase.cpp \
	gen-cpp/Hbase_types.cpp \
	AsyncThriftLogger.cpp \
	HBaseHandler.cpp \
	ThriftDispatcher.cpp \
	LogStorageManager.cpp \
	HBaseOperations.cpp

