#!/bin/bash

set -e

function generate_rootfs()
{
    if [ -d rootfs ] ; then
    	(cd rootfs; find . | fakeroot cpio -o -Hnewc | gzip > ../"$1")
    else
    	echo "rootfs not exist"
    	exit 1
    fi
}

function extract_rootfs()
{
    if [ -f "$1" ] ; then
    	rm -rf rootfs && mkdir rootfs
    	gzip -dc $1 | (cd rootfs; fakeroot cpio -i)
    else
    	echo "$1 not exist"
    	exit 1
    fi
}

if [ $# -ne 2 ]; then
	echo -e "please input correct parameters"
	echo -e "\t[build.sh -e rootf.cpio.gz] Extract the rootfs template to rootfs folder"
	echo -e "\tthen make some changes in the rootfs folder"
	echo -e "\t[build.sh -c rootf.cpio.gz] Generate the rootfs from the rootfs folder"
	exit 1
fi

if [ "$1" = "-e" ] ; then
	extract_rootfs $2
elif [ "$1" = "-c" ] ; then
	generate_rootfs $2
else
	echo "Argument is invalid!!!"
	exit 1
fi
