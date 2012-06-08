TEMPLATE = app
CONFIG -= gdb_dwarf_index
QMAKE_LINK = @: IGNORE THIS LINE

include($$TOP_SRCDIR/asyncthrift.pri)

JAVA_DEPS = \
	java/asynchbase-1.2.0.jar.md5 \
	java/log4j-over-slf4j-1.6.4.jar.md5 \
	java/netty-3.3.1.Final.jar.md5 \
	java/slf4j-api-1.6.4.jar.md5 \
	java/slf4j-simple-1.6.4.jar.md5 \
	java/suasync-1.2.0.jar.md5 \
	java/zookeeper-3.3.4.jar.md5

java_dep.input = JAVA_DEPS
java_dep.output = $$TOP_BUILDDIR/target/$$PKGDATADIR/java/${QMAKE_FILE_IN_BASE}
java_dep.commands = curl -sS `cat ${QMAKE_FILE_IN} | cut -b 33-` -o ${QMAKE_FILE_OUT} && test `md5sum ${QMAKE_FILE_OUT} | awk \'{print \$$1;}\'` = `cat ${QMAKE_FILE_IN} | cut -b -32` || (rm ${QMAKE_FILE_OUT} && false)
silent:java_dep.commands = @echo downloading ${QMAKE_FILE_OUT_BASE} && $$java_dep.commands
java_dep.variable_out = PRE_TARGETDEPS
java_dep.CONFIG += no_link

for(file, JAVA_DEPS) {
	dir = $$dirname(file)
	name = $$replace(dir, "[./\\\\]", "_")
	file = $$TOP_BUILDDIR/target/$$PKGDATADIR/$$replace(file, "(.*)\\.md5$", "\\1")
	eval($${name}.path = $$PREFIX$$PKGDATADIR/java/)
	eval($${name}.files += $$file)
	eval($${name}.CONFIG = no_check_exist)
	INSTALLS *= $$name
}

QMAKE_EXTRA_COMPILERS += java_dep
