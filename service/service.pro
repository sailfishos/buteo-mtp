TEMPLATE = app
TARGET = mtp_service
DEPENDPATH += .
INCLUDEPATH += . ../mts
LIBS += -L../mts -lbuteomtp

QT -= gui
CONFIG += link_pkgconfig
PKGCONFIG += mlite5

SOURCES += service.cpp

#install
target.path += $$[QT_INSTALL_PREFIX]/libexec/
target.files = mtp_service
desktop.path = /etc/xdg/autostart
desktop.files = buteo-mtp.desktop
INSTALLS += target desktop

#clean
QMAKE_CLEAN += $(TARGET)
