TEMPLATE = app
TARGET = asyncthrift-forwarder

include($$TOP_SRCDIR/asyncthrift.pri)

INCLUDEPATH += $$JAVA_HOME/include
INCLUDEPATH += $$JAVA_HOME/include/linux
LIBS += $$JAVA_HOME/jre/lib/$$ARCH/server/libjvm.so

JAVA_SOURCES = java/com/tuenti/async/GenericCallback.java
JAVA_TARGET = $$TOP_BUILDDIR/target/$$PKGDATADIR/java/suasync-generic-callback.jar

java.input = JAVA_SOURCES
java.output = $$JAVA_TARGET
java.commands = $${QMAKE_MKDIR} java && $$JAVAC -cp $$TOP_BUILDDIR/target/$$PKGDATADIR/java/\'*\' -d java ${QMAKE_FILE_IN} && jar cvf ${QMAKE_FILE_OUT} -C java `ls java` || (rm ${QMAKE_FILE_OUT} && false)
silent:java.commands = @echo generating ${QMAKE_FILE_OUT_BASE}.jar && $$java.commands
java.clean_commands = rm -rf java
java.variable_out = PRE_TARGETDEPS
java.CONFIG += no_link combine

java_install.files = $$JAVA_TARGET
java_install.path = $$PREFIX$$PKGDATADIR/java/
java_install.CONFIG += no_check_exist

INSTALLS += java_install

QMAKE_EXTRA_COMPILERS += java

DESTDIR = $$TOP_BUILDDIR/target/$$BINDIR

target.path = $$PREFIX$$BINDIR
INSTALLS += target

QMAKE_CLEAN += ${TARGET}

LIBS += -L$$TOP_BUILDDIR/src/common/ -lcommon

PKGCONFIG += thrift

HEADERS += \
	AsyncThriftForwarder.h \
	ForwarderManager.h \
	ForwarderManager_p.h \
	JniHelper.h \
	JavaObject.h \
	HBaseClient.h

SOURCES += \
	AsyncThriftForwarder.cpp \
	ForwarderManager.cpp \
	JniHelper.c \
	JavaObject.cpp \
	HBaseClient.cpp

