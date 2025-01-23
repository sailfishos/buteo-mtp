include(common.pri)

QT += xml dbus
QT -= gui

CONFIG += link_pkgconfig debug

TEMPLATE = lib
TARGET = buteomtp
DEPENDPATH += . \
              common \
              protocol \
              protocol/extensions \
              platform \
              transport \
              transport/usb \
              transport/dummy \
              platform/storage \
              platform/deviceinfo

INCLUDEPATH += . \
               protocol \
               protocol/extensions \
               common \
               platform \
               platform/storage \
               platform/deviceinfo \
               transport \
               transport/dummy \
               transport/usb \
               ../include

HEADERS += mts.h \
           common/trace.h \
           common/mtptypes.h \
           protocol/mtpresponder.h \
           protocol/propertypod.h \
           protocol/objectpropertycache.h \
           protocol/mtpextensionmanager.h \
           protocol/mtpcontainer.h \
           protocol/mtpcontainerwrapper.h \
           protocol/mtprxcontainer.h \
           protocol/mtptxcontainer.h \
           protocol/extensions/mtpextension.h \
           platform/deviceinfo/mtpdeviceinfo.h \
           platform/deviceinfo/deviceinfoprovider.h \
           platform/deviceinfo/xmlhandler.h \
           transport/mtptransporter.h \
           transport/usb/mtptransporterusb.h \
           transport/usb/threadio.h \
           transport/dummy/mtptransporterdummy.h \
           platform/storage/storagefactory.h \
           platform/storage/storageplugin.h

SOURCES += mts.cpp \
           protocol/mtpresponder.cpp \
           protocol/propertypod.cpp \
           protocol/objectpropertycache.cpp \
           protocol/mtpextensionmanager.cpp \
           protocol/mtpcontainer.cpp \
           protocol/mtpcontainerwrapper.cpp \
           protocol/mtprxcontainer.cpp \
           protocol/mtptxcontainer.cpp \
           transport/usb/mtptransporterusb.cpp \
           transport/dummy/mtptransporterdummy.cpp \
           platform/deviceinfo/mtpdeviceinfo.cpp \
           platform/deviceinfo/deviceinfoprovider.cpp \
           platform/deviceinfo/xmlhandler.cpp \
           platform/storage/storagefactory.cpp \
           platform/storage/storageplugin.cpp \
           transport/usb/descriptor.c \
           transport/usb/threadio.cpp

target.path = $$[QT_INSTALL_LIBS]/
INSTALLS += target

data.path = /usr/share/mtp
data.files = deviceInfo.xml device.ico

INSTALLS += data
