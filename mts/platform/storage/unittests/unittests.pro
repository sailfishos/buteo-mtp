CONFIG += link_pkgconfig
TEMPLATE = app
TARGET = storagefactory-test
QT += testlib xml dbus
QT -= gui
PKGCONFIG += Qt5SystemInfo buteosyncfw5

DEFINES += UT_ON

INCLUDEPATH += \
	. \
	.. \
	../../deviceinfo \
	../../.. \
	../../../common \
	../../../protocol \
	../../../protocol/extensions \
	../../../transport \
	../../../transport/dummy \
	../../../transport/usb \

LIBS += -ldl

HEADERS += \
	storagefactory_test.h \
	../storagefactory.h \
	../storageplugin.h \
	../../deviceinfo/deviceinfo.h \
	../../deviceinfo/deviceinfoprovider.h \
	../../deviceinfo/xmlhandler.h \
	../../../device_interface.h \
	../../../protocol/mtpresponder.h \
	../../../protocol/objectpropertycache.h \
	../../../protocol/propertypod.h \
	../../../transport/mtptransporter.h \
	../../../transport/dummy/mtptransporterdummy.h \
	../../../transport/usb/mtptransporterusb.h \
	../../../transport/usb/threadio.h \

SOURCES += \
	storagefactory_test.cpp \
	../storagefactory.cpp \
	../../deviceinfo/deviceinfo.cpp \
	../../deviceinfo/deviceinfoprovider.cpp \
	../../deviceinfo/xmlhandler.cpp \
	../../../device_interface.cpp \
	../../../protocol/mtpcontainer.cpp \
	../../../protocol/mtpcontainerwrapper.cpp \
	../../../protocol/mtpextensionmanager.cpp \
	../../../protocol/mtpresponder.cpp \
	../../../protocol/mtprxcontainer.cpp \
	../../../protocol/mtptxcontainer.cpp \
	../../../protocol/objectpropertycache.cpp \
	../../../protocol/propertypod.cpp \
	../../../transport/dummy/mtptransporterdummy.cpp \
	../../../transport/usb/descriptor.c \
	../../../transport/usb/mtptransporterusb.cpp \
	../../../transport/usb/threadio.cpp \

target.path = /opt/tests/buteo-mtp/

INSTALLS += target
