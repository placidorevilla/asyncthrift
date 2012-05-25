QMAKE_CC = gcc-4.5
QMAKE_CXX = g++-4.5
QMAKE_CXXFLAGS = -std=gnu++0x -rdynamic
#QMAKE_CXXFLAGS += -O3 -march=native -fomit-frame-pointer -mfpmath=sse
#QMAKE_CFLAGS += -pg
#QMAKE_CXXFLAGS += -pg
#QMAKE_LFLAGS = -pg

TEMPLATE = app
TARGET = asyncthrift
VERSION = -1.0.0
DEPENDPATH += src/cpp src/cpp/QtArg src/gen-cpp
INCLUDEPATH += src/cpp src/cpp/QtArg src/gen-cpp

ARCH = amd64
JAVA_HOME = ${JAVA_HOME}
JAVAC = $$JAVA_HOME/bin/javac
INCLUDEPATH += $$JAVA_HOME/include
INCLUDEPATH += $$JAVA_HOME/include/linux
LIBS += $$JAVA_HOME/jre/lib/$$ARCH/server/libjvm.so

CONFIG += debug warn_on link_pkgconfig nostrip
CONFIG += silent
QT -= gui

PKGCONFIG += thrift thrift-nb libevent libdaemon liblog4cxx zlib

BUILD_DIR = build

DESTDIR = $$BUILD_DIR/bin
OBJECTS_DIR = $$BUILD_DIR/.obj
MOC_DIR = $$BUILD_DIR/.moc

JAVA_SOURCES = src/java/com/stumbleupon/async/GenericCallback.java
JAVA_TARGET = $$BUILD_DIR/lib/$$TARGET$$VERSION/java/suasync-generic-callback.jar
JAVA_DEPS = \
	lib/java/asynchbase-1.2.0.jar.md5 \
	lib/java/log4j-over-slf4j-1.6.4.jar.md5 \
	lib/java/netty-3.3.1.Final.jar.md5 \
	lib/java/slf4j-api-1.6.4.jar.md5 \
	lib/java/slf4j-simple-1.6.4.jar.md5 \
	lib/java/suasync-1.2.0.jar.md5 \
	lib/java/zookeeper-3.3.4.jar.md5

java.input = JAVA_SOURCES
java.output = $$JAVA_TARGET
java.commands = $${QMAKE_MKDIR} $$OBJECTS_DIR/java && $$JAVAC -cp `dirname ${QMAKE_FILE_OUT}`/\'*\' -d $$OBJECTS_DIR/java ${QMAKE_FILE_IN} && jar cvf ${QMAKE_FILE_OUT} -C $$OBJECTS_DIR/java `ls $$OBJECTS_DIR/java` || (rm ${QMAKE_FILE_OUT} && false)
silent:java.commands = @echo generating ${QMAKE_FILE_OUT_BASE}.jar && $$java.commands
java.clean_commands = rm -rf $$OBJECTS_DIR/java
java.variable_out = java_install.files PRE_TARGETDEPS
java.CONFIG += no_link combine

java_install.path = /lib/$$TARGET$$VERSION/java/
java_install.CONFIG += no_check_exist

java_dep.input = JAVA_DEPS
java_dep.output = $$BUILD_DIR/lib/$$TARGET$$VERSION/java/${QMAKE_FILE_IN_BASE}
java_dep.commands = curl -sS `cat ${QMAKE_FILE_IN} | cut -b 33-` -o ${QMAKE_FILE_OUT} && test `md5sum ${QMAKE_FILE_OUT} | awk \'{print \$$1;}\'` = `cat ${QMAKE_FILE_IN} | cut -b -32` || (rm ${QMAKE_FILE_OUT} && false)
silent:java_dep.commands = @echo downloading ${QMAKE_FILE_OUT_BASE} && $$java_dep.commands
java_dep.variable_out = java_dep_install.files java.depends
java_dep.CONFIG += no_link

java_dep_install.path = /lib/$$TARGET$$VERSION/java/
java_dep_install.CONFIG += no_check_exist

QMAKE_EXTRA_COMPILERS += java java_dep

target.path = /bin/
INSTALLS += target java_install java_dep_install

QMAKE_CLEAN += ${TARGET}

QTARG_HEADERS = \
	src/cpp/QtArg/argconstraint.hpp \
	src/cpp/QtArg/helpiface.hpp \
	src/cpp/QtArg/help.hpp \
	src/cpp/QtArg/cmdline.hpp \
	src/cpp/QtArg/arg.hpp \
	src/cpp/QtArg/cmdlineiface.hpp \
	src/cpp/QtArg/cmdlinecontext.hpp \
	src/cpp/QtArg/xorarg.hpp \
	src/cpp/QtArg/visitor.hpp \
	src/cpp/QtArg/exceptions.hpp \
	src/cpp/QtArg/multiarg.hpp

HEADERS += src/cpp/AsyncThrift.h \
	src/gen-cpp/Hbase_types.h \
	src/gen-cpp/Hbase_constants.h \
	src/gen-cpp/Hbase.h \
	src/cpp/TApplication.h \
	src/cpp/JniHelper.h \
	src/cpp/JavaObject.h \
	src/cpp/HBaseClient.h \
	src/cpp/HBaseHandler.h \
	src/cpp/HBaseHandler_p.h \
	src/cpp/ThriftDispatcher.h \
	src/cpp/LogStorageManager.h \
	src/cpp/LogStorageManager_p.h \
	src/cpp/HBaseOperations.h \
	$$QTARG_HEADERS

SOURCES += src/cpp/AsyncThrift.cpp \
	src/gen-cpp/Hbase_constants.cpp \
	src/gen-cpp/Hbase.cpp \
	src/gen-cpp/Hbase_types.cpp \
	src/cpp/TApplication.cpp \
	src/cpp/JniHelper.c \
	src/cpp/JavaObject.cpp \
	src/cpp/HBaseClient.cpp \
	src/cpp/HBaseHandler.cpp \
	src/cpp/ThriftDispatcher.cpp \
	src/cpp/LogStorageManager.cpp \
	src/cpp/HBaseOperations.cpp

