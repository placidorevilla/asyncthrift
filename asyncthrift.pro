TEMPLATE = subdirs
SUBDIRS = conf lib src/common src/logger src/forwarder

src-logger.depends = sub-src-common
src-forwarder.depends = sub-src-common lib

QMAKE_SUBSTITUTES += config.h.in
