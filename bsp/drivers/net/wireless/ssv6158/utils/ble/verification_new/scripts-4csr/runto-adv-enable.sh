#!/bin/bash

if [ $# != '1' ]; then
	echo "< usage >"
	echo "runto-adv.sh hciX"
	exit 0
fi

echo "dev-reset.sh $1"
./dev-reset.sh $1
sleep 2

echo "adv-enable.sh $1 1"
./adv-enable.sh $1 1
