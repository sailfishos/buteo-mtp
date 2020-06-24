TEMPLATE = lib
TARGET = mtp-server
DEPENDPATH += .
INCLUDEPATH += . \
/usr/include/sync \
../mts/ \
../mts/common

CONFIG += plugin debug_and_release link_pkgconfig
PKGCONFIG += buteosyncfw5
QT -= gui
LIBS += -L../mts/ -lmeegomtp

# Input
HEADERS += mtpserver.h
SOURCES += mtpserver.cpp

#clean
QMAKE_CLEAN += $(TARGET) $(TARGET0) $(TARGET1) $(TARGET2)

#install
target.path = $$[QT_INSTALL_LIBS]/buteo-plugins-qt5
server.path = /etc/buteo/profiles/server
server.files = mtp.xml

INSTALLS += target \
            server
