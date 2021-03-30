#!/bin/bash
#
# This script analyses a dmesg output and shows how much time
# each function takes.
#
# For this script to work, the kernel needs to be compiled with the
# CONFIG_PRINTK_TIME configuration option enabled, and with
# "initcall_debug" passed on the kernel command line.
#
# usage:
# 	dmesg > bootup_printks
#	./scripts/show_time.sh bootup_printks | sort -k3 -n
#
# Authors:
#	WimHuang <huangwei@allwinnertech.com>

if test $# -ne 1; then
	echo "Usage: $0 timefile" >&2
	exit 1
fi
FILENAME=$1

boottime=0
inittime=0
linenum=`wc -l $FILENAME | awk '{print $1}'`

while read line
do
	let linenum-=1

	echo $line | grep "^\[[ \.0-9]*\] calling" > /dev/null 2<&1
	if [ $? == 0 ]; then
		callstamp=`echo $line | awk -F '[][]' '{print $2}'`
		module=`echo $line | awk '{print $4}'`
	else
		continue
	fi

	endcall=`tail -n $linenum $FILENAME | grep "^\[[ \.0-9]*\] initcall $module"`
	if [ $? == 0 ]; then
		endstamp=`echo $endcall | awk -F '[][]' '{print $2}'`
		timestamp=`echo $endstamp - $callstamp | bc`
		duration=`echo $endcall | awk '{print $8}'`
	else
		printf "Error: can't find endcall of $s\n" $module
		continue
	fi

	printf "[%-8f] %-46s %-10d\n" $callstamp $module $duration

	let inittime+=$duration
done < $FILENAME

boottime=`grep "^\[[ \.0-9]*\] Freeing unused kernel memory" $FILENAME	\
		| awk -F '[][]' '{print $2}'`

printf "total boot time: %fs, total initcall time: %dus\n" $boottime $inittime
