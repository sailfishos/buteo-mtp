TEMPLATE = subdirs

service.files = *.service
service.path = /usr/lib/systemd/user/

init.files = buteo-mtp
init.path = /usr/bin/

INSTALLS += service init