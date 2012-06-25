TEMPLATE = lib
TARGET = common
CONFIG += staticlib create_prl

include($$TOP_SRCDIR/asyncthrift.pri)

DEPENDPATH += gen-cpp
INCLUDEPATH += gen-cpp

PKGCONFIG += thrift

HEADERS += \
	gen-cpp/Hbase_types.h \
	gen-cpp/Hbase_constants.h \
	gen-cpp/Hbase.h \
	NBRingByteBuffer.h \
	TApplication.h \
	TMemFile.h \
	LogEndian.h \
	TLogger.h \
	HBaseOperations.h

SOURCES += \
	gen-cpp/Hbase_constants.cpp \
	gen-cpp/Hbase.cpp \
	gen-cpp/Hbase_types.cpp \
	TApplication.cpp \
	TMemFile.cpp \
	HBaseOperations.cpp
