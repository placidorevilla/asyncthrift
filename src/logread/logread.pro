TEMPLATE = app
TARGET = asyncthrift-logread

include($$TOP_SRCDIR/asyncthrift.pri)

DESTDIR = $$TOP_BUILDDIR/target/$$BINDIR

target.path = $$PREFIX$$BINDIR
INSTALLS += target

QMAKE_CLEAN += ${TARGET}

LIBS += -L$$TOP_BUILDDIR/src/common/ -lcommon
PRE_TARGETDEPS = $$TOP_BUILDDIR/src/common/libcommon.a

PKGCONFIG += thrift

SOURCES += \
	logread.cpp

