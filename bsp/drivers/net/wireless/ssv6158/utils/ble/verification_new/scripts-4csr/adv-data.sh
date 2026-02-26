#!/bin/bash

if [ $# != '1' ]; then
	echo "< usage >"
	echo "adv-data.sh hciX"
	exit 0
fi

hcitool -i $1 cmd 0x08 0x0008 1E 02 01 1A 1A FF 4C 00 02 15 92 77 83 0A B2 EB 49 0F A1 DD 7F E3 8C 49 2E DE 00 00 00 00 C5 00
