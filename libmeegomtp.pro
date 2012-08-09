TEMPLATE = subdirs
SUBDIRS += mts \
    mtpserver \
    test \
    mts/platform/storage/fsstorageplugin/unittests \
    mts/platform/deviceinfo/unittests \
    mts/platform/storage/fsstorageplugin \
    mts/protocol/unittests \
    service

# install additional files for the CI Integration tests
citests.path = /opt/tests/buteo-mtp/test-definition/
citests.files = tests.xml
INSTALLS += citests
#HEADERS = mts/common/mtp_dbus.h
#SOURCES = mts/common/mtp_dbus.cpp
