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
