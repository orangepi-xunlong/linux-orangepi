#!/bin/bash

dev="hci0"
val="1"

if [ $# != '2' ]; then
	echo "< usage >"
	echo "adv-enable.sh hciX [0|1]"
	exit 0
fi

dev=$1
val=$2

hcitool -i $dev cmd 0x08 0x000A $val
