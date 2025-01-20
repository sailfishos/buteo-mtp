TEMPLATE = app
TARGET = mtp_test
DEPENDPATH += .
INCLUDEPATH += . ../mts
LIBS += -L../mts -lbuteomtp
DRIVER = nogadgetfs
CONFIG += debug_and_release
QT -= gui
# Input

contains( DRIVER, gadgetfs ) {
    SOURCES += test_gfs.cpp
}

contains( DRIVER, nogadgetfs ) {
    SOURCES += test.cpp
}

#install
target.path += /opt/tests/buteo-mtp/
INSTALLS += target

#clean
QMAKE_CLEAN += $(TARGET)
