CONFIG += debug warn_on link_pkgconfig nostrip
#CONFIG += silent
QT -= gui

release {
	OPTIMIZE_FLAGS = -O3 -march=native -fomit-frame-pointer -mfpmath=sse
}

profile {
	PROFILE_CFLAGS = -pg
	PROFILE_LFLAGS = -pg
}

QMAKE_CXXFLAGS += -std=gnu++0x -rdynamic $$OPTIMIZE_FLAGS $$PROFILE_CFLAGS
QMAKE_CFLAGS += $$OPTIMIZE_FLAGS $$PROFILE_CFLAGS
QMAKE_LFLAGS += $$PROFILE_LFLAGS

ARCH = amd64
JAVA_HOME = ${JAVA_HOME}
JAVAC = $$JAVA_HOME/bin/javac

PKGCONFIG += libdaemon liblog4cxx

DEPENDPATH += $$TOP_SRCDIR/include/QtArg $$TOP_SRCDIR/src/common
INCLUDEPATH += $$TOP_SRCDIR/include/QtArg $$TOP_SRCDIR/src/common

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

