include(../../../../common.pri)

CONFIG += debug_and_release

LIBS += -ldl -lssu
TEMPLATE = app
TARGET = storage-test
QT += dbus xml testlib core5compat
DEFINES += UT_ON
#QMAKE_CXXFLAGS += -ftest-coverage -fprofile-arcs
#QMAKE_LFLAGS += -fprofile-arcs -ftest-coverage

VPATH = ../../../../

DEPENDPATH += . \
              .. \
              ../.. \
              ../../../.. \
              ../../../../protocol

INCLUDEPATH += . \
               .. \
               ../.. \
               ../../../.. \
               ../../../../protocol \
               ../../../../protocol/extensions \
               ../../../../transport \
               ../../../../transport/dummy \
               ../../../../transport/usb \
               ../../../../platform \
               ../../../../platform/deviceinfo\
               ../../../../common \
               ../../../../../include

# Input
HEADERS += fsstorageplugin_test.h \
           ../../storageplugin.h \
           ../fsstorageplugin.h \
           ../fsinotify.h \
           ../thumbnailer.h \
           ../../storagefactory.h \
           ../storageitem.h \
           mts.h \
           protocol/mtpresponder.h \
           protocol/mtpcontainer.h \
           protocol/mtpcontainerwrapper.h \
           protocol/mtprxcontainer.h \
           protocol/mtptxcontainer.h \
           protocol/propertypod.h \
           protocol/objectpropertycache.h \
           protocol/mtpextensionmanager.h \
           protocol/extensions/mtpextension.h \
           transport/mtptransporter.h \
           transport/usb/mtptransporterusb.h \
           transport/usb/threadio.h \
           transport/dummy/mtptransporterdummy.h \
           platform/deviceinfo/xmlhandler.h \
           platform/deviceinfo/deviceinfoprovider.h \
           platform/deviceinfo/mtpdeviceinfo.h

SOURCES += fsstorageplugin_test.cpp \
           ../fsstorageplugin.cpp \
           ../fsinotify.cpp \
           ../storageitem.cpp \
           ../thumbnailer.cpp \
           ../../storagefactory.cpp \
           ../../storageplugin.cpp \
           mts.cpp \
           protocol/mtpresponder.cpp \
           protocol/mtpcontainer.cpp \
           protocol/mtpcontainerwrapper.cpp \
           protocol/mtprxcontainer.cpp \
           protocol/mtptxcontainer.cpp \
           protocol/propertypod.cpp \
           protocol/objectpropertycache.cpp \
           protocol/mtpextensionmanager.cpp \
           transport/usb/mtptransporterusb.cpp \
           transport/usb/threadio.cpp \
           transport/usb/descriptor.c \
           transport/dummy/mtptransporterdummy.cpp \
           platform/deviceinfo/xmlhandler.cpp \
           platform/deviceinfo/deviceinfoprovider.cpp \
           platform/deviceinfo/mtpdeviceinfo.cpp

target.path = /opt/tests/buteo-mtp/
INSTALLS += target

#clean
QMAKE_CLEAN += $(TARGET)
