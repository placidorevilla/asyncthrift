TEMPLATE = app
CONFIG -= gdb_dwarf_index
QMAKE_LINK = @: IGNORE THIS LINE

DESTDIR = $$TOP_BUILDDIR/target/$$PKGSYSCONFDIR
INSTALLDIR = $$PKGSYSCONFDIR

BUILD_SUBSTITUTES = \
	./asyncthrift.ini.in \
	./log4cxx.xml.in

include($$TOP_SRCDIR/asyncthrift.pri)
