include(../../../common.pri)

TEMPLATE = lib
TARGET = fsstorage

CONFIG += plugin debug

QT += dbus xml
QT -= gui

PKGCONFIG += blkid mount
PKGCONFIG += nemodbus

system($$[QT_INSTALL_BINS]/qdbusxml2cpp -c ThumbnailerProxy -p thumbnailerproxy.h:thumbnailerproxy.cpp -i thumbnailpathlist.h ./org.nemomobile.Thumbnailer.xml)

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
           ../storageplugin.h \
           thumbnailerproxy.h \
           thumbnailer.h \
           fsinotify.h \
           storageitem.h

SOURCES += fsstorageplugin.cpp \
           fsstoragepluginfactory.cpp \
           thumbnailerproxy.cpp \
           thumbnailer.cpp \
           fsinotify.cpp \
           storageitem.cpp

LIBPATH += ../../..
LIBS    += -lmeegomtp -lssu

#install
target.path = $$[QT_INSTALL_LIBS]/mtp

configuration.path = /etc/fsstorage.d
configuration.files = phone-memory.xml homedir-blacklist.conf sd-card.xml

INSTALLS += target configuration

#clean
QMAKE_CLEAN += $(TARGET)
