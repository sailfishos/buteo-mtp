include(../../common.pri)

CONFIG += debug_and_release

LIBS += -ldl -lssu
TEMPLATE = app
TARGET = protocol-test
QT += dbus xml testlib core5compat
QT -= gui
DEFINES += UT_ON
#QMAKE_CXXFLAGS += -ftest-coverage -fprofile-arcs
#QMAKE_LFLAGS += -fprofile-arcs -ftest-coverage

DEPENDPATH += . \
              .. \
              ../.. \
              ../../platform \
              ../../platform/storage \
              ../../platform/deviceinfo \
              ../../transport \
              ../../transport/dummy \
              ../../transport/usb \
              ../../common

INCLUDEPATH += . \
               .. \
               ../extensions \
               ../.. \
               ../../platform \
               ../../platform/storage \
               ../../platform/deviceinfo \
               ../../transport \
               ../../transport/dummy \
               ../../transport/usb \
               ../../common  \
               ../../../include

# Input
HEADERS += mtpresponder_test.h \
           ../mtpresponder.h \
           ../mtpcontainer.h \
           ../mtpcontainerwrapper.h \
           ../mtprxcontainer.h \
           ../mtptxcontainer.h \
           ../propertypod.h \
           ../objectpropertycache.h \
           ../mtpextensionmanager.h \
           ../extensions/mtpextension.h \
           ../extensions/mtpextension.h \
           ../../platform/storage/storagefactory.h \
           ../../platform/storage/storageplugin.h \
           ../../platform/deviceinfo/xmlhandler.h \
           ../../platform/deviceinfo/deviceinfoprovider.h \
           ../../platform/deviceinfo/mtpdeviceinfo.h \
           ../../transport/mtptransporter.h \
           ../../transport/usb/mtptransporterusb.h \
           ../../transport/usb/threadio.h \
           ../../transport/dummy/mtptransporterdummy.h \
           ../../mts.h

SOURCES += mtpresponder_test.cpp \
           ../mtpresponder.cpp \
           ../mtpcontainer.cpp \
           ../mtpcontainerwrapper.cpp \
           ../mtprxcontainer.cpp \
           ../mtptxcontainer.cpp \
           ../propertypod.cpp \
           ../objectpropertycache.cpp \
           ../mtpextensionmanager.cpp \
           ../../platform/storage/storagefactory.cpp \
           ../../platform/deviceinfo/xmlhandler.cpp \
           ../../platform/deviceinfo/deviceinfoprovider.cpp \
           ../../platform/deviceinfo/mtpdeviceinfo.cpp \
           ../../transport/usb/mtptransporterusb.cpp \
           ../../transport/usb/descriptor.c \
           ../../transport/usb/threadio.cpp \
           ../../transport/dummy/mtptransporterdummy.cpp \
           ../../mts.cpp

target.path = /opt/tests/buteo-mtp/
INSTALLS += target

#clean
QMAKE_CLEAN += $(TARGET)
