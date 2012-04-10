QMAKE_CC = gcc-4.5
QMAKE_CXX = g++-4.5
QMAKE_CXXFLAGS = -std=gnu++0x -rdynamic

TEMPLATE = app
TARGET = thrift
DEPENDPATH += . ./gen-cpp ./include/thrift
INCLUDEPATH += . ./gen-cpp ./include/thrift

ARCH = amd64
JAVA_HOME = ${JAVA_HOME}
INCLUDEPATH += $$JAVA_HOME/include
INCLUDEPATH += $$JAVA_HOME/include/linux
LIBS += $$JAVA_HOME/jre/lib/$$ARCH/server/libjvm.so

CONFIG += debug warn_on
QT -= gui

LIBS += -L./lib/thrift -lthrift

HEADERS += src/main.h \
	gen-cpp/Hbase_types.h \
	gen-cpp/Hbase_constants.h \
	gen-cpp/Hbase.h \
	src/JniHelper.h \
	src/JavaObject.h \
	src/HBaseClient.h
SOURCES += src/main.cpp \
	gen-cpp/Hbase_constants.cpp \
	gen-cpp/Hbase.cpp \
	gen-cpp/Hbase_types.cpp \
	src/JniHelper.c \
	src/JavaObject.cpp \
	src/HBaseClient.cpp
