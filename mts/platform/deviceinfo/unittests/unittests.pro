include(../../../common.pri)

QT += dbus xml testlib
QT -= gui
CONFIG += warn_off debug_and_release

PKGCONFIG += buteosyncfw5 Qt5SystemInfo

LIBS += -lssu

TEMPLATE = app
TARGET = deviceinfo-test
DEFINES += UT_ON
#QMAKE_CXXFLAGS += -ftest-coverage -fprofile-arcs
#QMAKE_LFLAGS += -fprofile-arcs -ftest-coverage
DEPENDPATH += .
INCLUDEPATH += . \
.. \
../../ \
../../../ \
../../../protocol/ \
../../../common/


# Input
HEADERS += deviceinfoprovider_test.h \
        ../deviceinfoprovider.h \
        ../deviceinfo.h \
        ../xmlhandler.h

SOURCES += deviceinfoprovider_test.cpp \
        ../deviceinfoprovider.cpp \
        ../deviceinfo.cpp \
        ../xmlhandler.cpp

target.path = /opt/tests/buteo-mtp/
INSTALLS += target

data.path = /opt/tests/buteo-mtp/data/
data.files = ../../../deviceInfo.xml

INSTALLS += data

#clean
QMAKE_CLEAN += $(TARGET)
