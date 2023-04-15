#!/bin/bash
#
# Shell script to install Bluetooth firmware and attach BT part of
# RTL8822BS for MGV2000 1/8
# based on the work of Staars lwfinger
# MGV2000 use UART_A for BT serial , use GPIOX_17 (0x60 = 96) for BT_EN pin

echo "(Re)start Bluetooth device ..."
sleep 1
echo
echo "Power off BT device now ..."

have_bt_en=`lsmod | grep 8822BS_BT_EN`
if [ -z "$have_bt_en" ];
then
 echo "[ not find 8822BS_BT_EN ]"
else
 echo "find 8822BS_BT_EN , do rmmod "
 rmmod 8822BS_BT_EN
fi

hciattach_pid=`ps -aux | grep hciattach | grep -v grep | awk '{print $2}'`
if [ -z "$hciattach_pid" ];
then
 echo "[ not find hciattach pid ]"
else
 echo "find result: $hciattach_pid "
 kill  $hciattach_pid
fi

echo "starting BT device now ..."
sleep 1
insmod ./8822BS_BT_EN.ko
echo
#meson platform use /dev/ttyAML1
#sunxi platform use /dev/ttyS1
TTY="/dev/ttyAML1"
systemctl stop serial-getty@$TTY


echo "Using device $TTY for Bluetooth"
modprobe rfkill
echo "Power cycle 8822BS BT-section"
#rfkill block bluetooth
sleep 2
rfkill unblock blueooth
echo "Start attaching"
rfkill unblock all
./8822b_hciattach -n -s 115200 $TTY rtk_h5 2>&1 &
