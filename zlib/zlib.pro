TEMPLATE = lib
CONFIG += lib
QT -= core gui

TARGET = zlib
DESTDIR = $$OUT_PWD/../basket

HEADERS += \
        crc32.h \
        deflate.h \
        gzguts.h \
        inffast.h \
        inffixed.h \
        inflate.h \
        inftrees.h \
        trees.h \
        zutil.h \
        zconf.h

SOURCES += \
        adler32.c \
        compress.c \
        crc32.c \
        deflate.c \
        gzclose.c \
        gzlib.c \
        gzread.c \
        gzwrite.c \
        inflate.c \
        infback.c \
        inftrees.c \
        inffast.c \
        trees.c \
        uncompr.c \
        zutil.c \
