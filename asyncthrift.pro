TEMPLATE = subdirs
SUBDIRS = conf lib src/common src/logger src/forwarder src/forwarder/java src/logread

src-logger.depends = sub-src-common
src-forwarder.depends = sub-src-common
src-forwarder-java.depends = lib
src-logread.depends = sub-src-common

QMAKE_SUBSTITUTES += config.h.in
