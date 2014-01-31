TEMPLATE = subdirs

mts.subdir = mts
mts.target = sub-mts

test.subdir = test
test.target = sub-test
test.depends = sub-mts

mtpserver.subdir = mtpserver
mtpserver.target = sub-mtpserver
mtpserver.depends = sub-mts

mts_fsstorage_plugin.subdir = mts/platform/storage/fsstorageplugin
mts_fsstorage_plugin.target = sub-mts-fsstorage-plugin
mts_fsstorage_plugin.depends = sub-mts

mts_fsstorage_tests.subdir = mts/platform/storage/fsstorageplugin/unittests
mts_fsstorage_tests.target = sub-mts-fsstorage-tests

mts_deviceinfo_tests.subdir = mts/platform/deviceinfo/unittests
mts_deviceinfo_tests.target = sub-mts-deviceinfo-tests

mts_protocol_tests.subdir = mts/protocol/unittests
mts_protocol_tests.target = sub-mts-protocol-tests

service.subdir = service
service.target = sub-service
service.depends = sub-mts

systemd.subdir = init/systemd
systemd.target = sub-systemd

SUBDIRS += \
    mts \
    test \
    mtpserver \
    mts_fsstorage_plugin \
    mts_fsstorage_tests \
    mts_deviceinfo_tests \
    mts_protocol_tests \
    service \
    systemd

# install additional files for the CI Integration tests
citests.path = /opt/tests/buteo-mtp/test-definition/
citests.files = tests.xml
INSTALLS += citests
