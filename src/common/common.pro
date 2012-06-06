TEMPLATE = lib
TARGET = common
CONFIG += staticlib create_prl

include($$TOP_SRCDIR/asyncthrift.pri)

HEADERS += \
	NBRingByteBuffer.h \
	TApplication.h \

SOURCES += \
	TApplication.cpp
