TEMPLATE = subdirs

# "/usr/lib/libmeegomtp.so" - shared library, mtp responder logic
mts.subdir = mts
mts.target = sub-mts

# "/usr/lib/buteo-plugins-qt5/libmptserver.so" - plugin used by msyncd
mtpserver.subdir = mtpserver
mtpserver.target = sub-mtpserver
mtpserver.depends = sub-mts

# "/usr/lib/mtp/libfsstorage.so" - plugin used by "libmeegomtp.so"
mts_fsstorage_plugin.subdir = mts/platform/storage/fsstorageplugin
mts_fsstorage_plugin.target = sub-mts-fsstorage-plugin
mts_fsstorage_plugin.depends = sub-mts

# "/usr/lib/mtp/mtp_service" - standalone mtp daemon (and env scripts)
service.subdir = service
service.target = sub-service
service.depends = sub-mts

# systemd unit files
systemd.subdir = init/systemd
systemd.target = sub-systemd

# "/opt/tests/buteo-mtp/mtp_test" - unused test application
test.subdir = test
test.target = sub-test
test.depends = sub-mts

# "/opt/tests/buteo-mtp/storagefactory-test" - unit test app
mts_storage_tests.subdir = mts/platform/storage/unittests
mts_storage_tests.target = sub-mts-storage-tests
mts_storage_tests.depends= sub-mts

# "/opt/tests/buteo-mtp/storage-test" - unit test app
mts_fsstorage_tests.subdir = mts/platform/storage/fsstorageplugin/unittests
mts_fsstorage_tests.target = sub-mts-fsstorage-tests
mts_fsstorage_tests.depends = sub-mts

# "/opt/tests/buteo-mtp/deviceinfo-test" - unit test app
mts_deviceinfo_tests.subdir = mts/platform/deviceinfo/unittests
mts_deviceinfo_tests.target = sub-mts-deviceinfo-tests
mts_deviceinfo_tests.depends = sub-mts

# "/opt/tests/buteo-mtp/protocol-test" - unit test app for mtp command handling
mts_protocol_tests.subdir = mts/protocol/unittests
mts_protocol_tests.target = sub-mts-protocol-tests
mts_protocol_tests.depends = sub-mts

SUBDIRS += \
    mts \
    test \
    mtpserver \
    mts_storage_tests \
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
