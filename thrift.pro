QMAKE_CC = gcc-4.5
QMAKE_CXX = g++-4.5
QMAKE_CXXFLAGS = -std=gnu++0x -rdynamic

TEMPLATE = app
TARGET = thrift
DEPENDPATH += src/cpp src/gen-cpp
INCLUDEPATH += src/cpp src/gen-cpp

ARCH = amd64
JAVA_HOME = ${JAVA_HOME}
INCLUDEPATH += $$JAVA_HOME/include
INCLUDEPATH += $$JAVA_HOME/include/linux
LIBS += $$JAVA_HOME/jre/lib/$$ARCH/server/libjvm.so

CONFIG += debug warn_on link_pkgconfig
QT -= gui

PKGCONFIG += thrift thrift-nb

DESTDIR = build
MOC_DIR = build
OBJECTS_DIR = build

HEADERS += src/cpp/main.h \
	src/gen-cpp/Hbase_types.h \
	src/gen-cpp/Hbase_constants.h \
	src/gen-cpp/Hbase.h \
	src/cpp/JniHelper.h \
	src/cpp/JavaObject.h \
	src/cpp/HBaseClient.h
SOURCES += src/cpp/main.cpp \
	src/gen-cpp/Hbase_constants.cpp \
	src/gen-cpp/Hbase.cpp \
	src/gen-cpp/Hbase_types.cpp \
	src/cpp/JniHelper.c \
	src/cpp/JavaObject.cpp \
	src/cpp/HBaseClient.cpp
