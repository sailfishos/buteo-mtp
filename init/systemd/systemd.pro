TEMPLATE = subdirs

service.files = *.service
service.path = /usr/lib/systemd/user/

mount.files = *.mount
mount.path = /lib/systemd/system/

init.files = buteo-mtp
init.path = /usr/bin/

INSTALLS += service init mount