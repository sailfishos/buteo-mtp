include(../../../common.pri)

TEMPLATE = lib
TARGET = fsstorage

CONFIG += plugin debug qtsparql

QT += dbus xml
QT -= gui

PKGCONFIG += blkid mount

DEPENDPATH += . \
              .. \
              ../../.. \
              ../../../protocol \
              ../../../transport \
              ../../../common

INCLUDEPATH += . \
               .. \
               ../../.. \
               ../../../protocol \
               ../../../transport \
               ../../../common \

# Input
HEADERS += fsstorageplugin.h \
           storagetracker.h \
           ../storageplugin.h \
           thumbnailerproxy.h \
           thumbnailer.h \
           fsinotify.h \
           storageitem.h

SOURCES += fsstorageplugin.cpp \
           fsstoragepluginfactory.cpp \
           storagetracker.cpp \
           thumbnailerproxy.cpp \
           thumbnailer.cpp \
           fsinotify.cpp \
           storageitem.cpp

LIBPATH += ../../..
LIBS    += -lmeegomtp -lssu

#install
target.path = /usr/lib/mtp

configuration.path = /etc/fsstorage.d
configuration.files = phone-memory.xml homedir-blacklist.conf sd-card.xml

INSTALLS += target configuration

#clean
QMAKE_CLEAN += $(TARGET)
