#!/bin/bash

SERVICE=/usr/lib/mtp/mtp_service
STORAGE_PATH=$HOME
LINK_BASE=$HOME
XDG_KEYS="DOWNLOAD DOCUMENTS MUSIC PICTURES VIDEOS"
XDG_LINKS="false"

[ -f /etc/default/buteo-mtp ] && . /etc/default/buteo-mtp
[ -f $HOME/.config/buteo-mtp ] && . $HOME/.config/buteo-mtp

if [ $XDG_LINKS = "true" ]; then
    mkdir -p $STORAGE_PATH
    for i in $XDG_KEYS;
    do
        ln -sf `xdg-user-dir $i` $STORAGE_PATH;
    done
fi

exec $SERVICE $SERVICE_PARAMETERS
