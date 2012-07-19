TEMPLATE = lib
TARGET = common
CONFIG += staticlib create_prl

include($$TOP_SRCDIR/asyncthrift.pri)

PKGCONFIG += thrift

HEADERS += \
	NBRingByteBuffer.h \
	TApplication.h \
	TMemFile.h \
	LogEndian.h \
	TLogger.h \
	HBaseOperations.h

SOURCES += \
	TApplication.cpp \
	TMemFile.cpp \
	HBaseOperations.cpp

THRIFTS = $$TOP_SRCDIR/resources/Hbase.thrift
include(thrift.pri)
