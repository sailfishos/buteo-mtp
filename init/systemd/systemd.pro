TEMPLATE = subdirs

service.files = *.service
service.path = /usr/lib/systemd/user/

mount.files = *.mount
mount.path = /lib/systemd/system/

INSTALLS += service mount
