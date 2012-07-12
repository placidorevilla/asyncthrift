TEMPLATE = subdirs
SUBDIRS = conf lib src/common src/logger src/forwarder src/forwarder/java

src-logger.depends = sub-src-common
src-forwarder.depends = sub-src-common
src-forwarder-java.depends = lib

QMAKE_SUBSTITUTES += config.h.in
