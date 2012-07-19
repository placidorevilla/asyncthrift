CONFIG += link_pkgconfig nostrip silent
QT -= gui
QT += network

release {
	OPTIMIZE_FLAGS = -O3 -march=native -fomit-frame-pointer -mfpmath=sse
}

profile {
	PROFILE_CFLAGS = -pg
	PROFILE_LFLAGS = -pg
}

QMAKE_CXXFLAGS += -I$$TOP_BUILDDIR -I$$TOP_SRCDIR -std=gnu++0x -rdynamic $$OPTIMIZE_FLAGS $$PROFILE_CFLAGS
QMAKE_CFLAGS += -I$$TOP_BUILDDIR -I$$TOP_SRCDIR $$OPTIMIZE_FLAGS $$PROFILE_CFLAGS
QMAKE_LFLAGS += $$PROFILE_LFLAGS
# To avoid some warnings on thrift generated code
QMAKE_CXXFLAGS_WARN_ON += -Wno-return-type

ARCH = $$(ARCH)
isEmpty(ARCH):ARCH = $$system(dpkg --print-architecture)
JAVA_HOME = $$(JAVA_HOME)
isEmpty(JAVA_HOME):JAVA_HOME = $$system(update-java-alternatives -l java-6-sun | cut -d\' \' -f3)
JAVAC = $$JAVA_HOME/bin/javac

PKGCONFIG += libdaemon liblog4cxx

COMMON_DEPENDENCIES = \
	$$TOP_BUILDDIR \
	$$TOP_BUILDDIR/src/common/gen-cpp \
	$$TOP_SRCDIR/include/QtArg \
	$$TOP_SRCDIR/include/QCircularBuffer \
	$$TOP_SRCDIR/src/common

DEPENDPATH += $$COMMON_DEPENDENCIES
INCLUDEPATH += $$COMMON_DEPENDENCIES

