CONFIG += debug warn_on link_pkgconfig nostrip link_prl
#CONFIG += silent
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

ARCH = amd64
JAVA_HOME = $$(JAVA_HOME)
isEmpty(JAVA_HOME) {
	JAVA_HOME = $$system(readlink -m `which javac`)
	JAVA_HOME = $$dirname(JAVA_HOME)
	JAVA_HOME = $$dirname(JAVA_HOME)
}
JAVAC = $$JAVA_HOME/bin/javac

PKGCONFIG += libdaemon liblog4cxx

DEPENDPATH += $$TOP_BUILDDIR $$TOP_SRCDIR/include/QtArg $$TOP_SRCDIR/include/QCircularBuffer $$TOP_SRCDIR/src/common $$TOP_SRCDIR/src/common/gen-cpp
INCLUDEPATH += $$TOP_BUILDDIR $$TOP_SRCDIR/include/QtArg $$TOP_SRCDIR/include/QCircularBuffer $$TOP_SRCDIR/src/common $$TOP_SRCDIR/src/common/gen-cpp

ALL_BUILD_SUBSTITUTES = $$BUILD_SUBSTITUTES $$BUILD_SUBSTITUTES_NOINSTALL
QMAKE_SUBSTITUTES += $$ALL_BUILD_SUBSTITUTES

for(file, BUILD_SUBSTITUTES) {
	dir = $$dirname(file)
	name = $$replace(dir, "[./\\\\]", "_")
	file = $$replace(file, "(.*)\\.in$", "\\1")
	eval($${name}.path = $$INSTALLDIR/$$dir)
	eval($${name}.files += $$file)
	INSTALLS *= $$name
}

