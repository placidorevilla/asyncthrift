TEMPLATE = app
CONFIG -= gdb_dwarf_index
QMAKE_LINK = @: IGNORE THIS LINE

include($$TOP_SRCDIR/asyncthrift.pri)

JAVA_CLASSPATH = $$TOP_BUILDDIR/target/$$PKGDATADIR/java/\'*\'
JAVA_SOURCES = com/tuenti/async/GenericCallback.java
JAVA_TARGET = $$TOP_BUILDDIR/target/$$PKGDATADIR/java/suasync-generic-callback.jar

defineReplace(javaOutputFile) {
	name = $$system(readlink -m $${OUT_PWD}/$$join(1))
	name = $$replace(name, ".java$", ".class")
	return("$$replace(name, $${IN_PWD}/, '')")
}

java.input = JAVA_SOURCES
java.output_function = javaOutputFile
java.commands = $$JAVAC -cp $$JAVA_CLASSPATH -d . ${QMAKE_FILE_IN} || (rm ${QMAKE_FILE_OUT} && false)
silent:java.commands = @echo compiling ${QMAKE_FILE_OUT_BASE}.class && $$java.commands
java.variable_out = JAR_INPUT
java.CONFIG += no_link

jar.input = JAR_INPUT
jar.output = $$JAVA_TARGET
jar.commands = jar cvf ${QMAKE_FILE_OUT} ${QMAKE_FILE_IN} || (rm ${QMAKE_FILE_OUT} && false)
silent:jar.commands = @echo generating ${QMAKE_FILE_OUT_BASE}.jar && $$jar.commands
jar.variable_out = PRE_TARGETDEPS
jar.CONFIG += no_link combine

java_install.files = $$JAVA_TARGET
java_install.path = $$PREFIX$$PKGDATADIR/java/
java_install.CONFIG += no_check_exist

INSTALLS += java_install

QMAKE_EXTRA_COMPILERS += java jar

