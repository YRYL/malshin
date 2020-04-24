#!/bin/bash

FS_LINK=http://dl-cdn.alpinelinux.org/alpine/v3.11/releases/x86_64/alpine-minirootfs-3.11.3-x86_64.tar.gz

gcc malshin.c -o malshin

sudo chown root:root malshin
sudo chmod u+s malshin

if [ -d new_root ] && [ $# -gt 0 ] && [ $1 == "--clean_fs" ]; then
    echo "removing new_root folder..."
    rm -rf new_root
    exit
fi

if [[ ! -d new_root  ||  ($# -gt 0  &&  $1 == "--get_fs") ]]; then
    mkdir -p new_root
    cd new_root
    rm -rf ./*

    wget -O new_root.tar.gz $FS_LINK

    unp new_root.tar.gz
    rm new_root.tar.gz

    cd ..
fi
