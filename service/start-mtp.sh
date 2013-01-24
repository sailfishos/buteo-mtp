#!/bin/bash

SERVICE=/usr/lib/mtp/mtp_service
STORAGE_PATH=$HOME/.config/mtpstorage
LINK="Music Videos Pictures Downloads"
LINK_BASE=$HOME

if [ ! -d $STORAGE_PATH ]; then
    mkdir -p $STORAGE_PATH
    for i in $LINK;
    do
        ln -s $LINK_BASE/$i $STORAGE_PATH;
    done
fi

$SERVICE
