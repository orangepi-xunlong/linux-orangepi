#!/bin/bash

if [ $# != '1' ]; then
	echo "< usage >"
	echo "reset.sh hciX"
	exit 0
fi

echo "hciconfig $1 down"
hciconfig $1 down

echo "hciconfig $1 up"
hciconfig $1 up

echo "hciconfig $1 noscan"
hciconfig $1 noscan

hciconfig
sleep 2

hcitool -i $1 cmd 0x03 0x0003
