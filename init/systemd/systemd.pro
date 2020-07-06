TEMPLATE = subdirs

service.files = *.service
service.path = /usr/lib/systemd/user/

mount.files = *.mount
mount.path = /usr/lib/systemd/system/

INSTALLS += service mount
