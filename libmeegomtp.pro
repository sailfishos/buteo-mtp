TEMPLATE = subdirs

SUBDIRS += mts mtpserver test mts/platform/storage/fsstorageplugin/unittests mts/platform/deviceinfo/unittests mts/platform/storage/fsstorageplugin mts/protocol/unittests

# install additional files for the CI Integration tests
citests.path = /usr/share/libmeegomtp-tests/
citests.files = tests.xml

INSTALLS += citests
