#!/bin/bash

if [ $# == '0' ]; then
	echo " "
	echo "hci-reset.sh hciX"
	echo " "
	exit 0
fi

hcitool -i $1 cmd 0x03 0x0003
