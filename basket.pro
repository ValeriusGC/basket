TEMPLATE=subdirs 

CONFIG += ordered

BUILD_DIR += $$OUT_PWD/basket

WIN32:SUBDIRS += zlib
WIN32:SUBDIRS += quazip
SUBDIRS += basket
