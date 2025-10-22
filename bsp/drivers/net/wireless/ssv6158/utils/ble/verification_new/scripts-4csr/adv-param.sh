#!/bin/bash

if [ $# == '0' ]; then
	echo " "
	echo "NOTE: plz directly modify this script for different settings!"
	echo " "
	echo "< usage >"
	echo "adv-param.sh hciX"
	echo " "
	exit 0
fi

#------------------------------------------------
# Advertising_Interval
#------------------------------------------------
# NOTE: take care endianess
#
# Range: 0x0020 to 0x4000, Time = N * 0.625 ms
# Default: 00 08 (=0x0800)
Adv_Itv_Min="00 08"
Adv_Itv_Max="00 08"
#------------------------------------------------
# Advertising_Type
#------------------------------------------------
# Default: ADV_IND
ADV_IND="00"
ADV_DIRECT_IND="01"
ADV_SCAN_IND="02"
ADV_NONCONN_IND="03"

Adv_Type=$ADV_NONCONN_IND
#------------------------------------------------
# Own_Address_Type & Direct_Address_Type
#------------------------------------------------
# Default: PUBLIC_DEVICE_ADDRESS
PUBLIC_DEVICE_ADDRESS="00"
RANDOM_DEVICE_ADDRESS="01"

Own_Addr_Type=$PUBLIC_DEVICE_ADDRESS
Direct_Addr_Type=$PUBLIC_DEVICE_ADDRESS
#------------------------------------------------
# Direct_Address
#------------------------------------------------
# Default: N/A
Direct_Addr="00 00 00 00 00 00"
#------------------------------------------------
# Advertising_Channel_Map
#------------------------------------------------
# Default: CH_ALL
CH_37="01"
CH_38="02"
CH_39="04"
CH_ALL="07"

Adv_Ch_Map=$CH_37
#------------------------------------------------
# Advertising_Filter_Policy
#------------------------------------------------
# Default: SCAN_ANY_CONN_ANY
SCAN_ANY_CONN_ANY="00"
SCAN_WHITELIST_CONN_ANY="01"
SCAN_ANY_CONN_WHITELIST="02"
SCAN_WHITELIST_CONN_WHITELIST="03"

Adv_Filter_Policy=$SCAN_ANY_CONN_ANY

# -------------------- exec ---------------------
echo "Adv_Itv_Min       = " $Adv_Itv_Min
echo "Adv_Itv_Max       = " $Adv_Itv_Max
echo "Adv_Type          = " $Adv_Type
echo "Own_Addr_Type     = " $Own_Addr_Type
echo "Direct_Addr_Type  = " $Direct_Addr_Type
echo "Direct_Addr       = " $Direct_Addr
echo "Adv_Ch_Map        = " $Adv_Ch_Map
echo "Adv_Filter_Policy = " $Adv_Filter_Policy
echo " "

hcitool -i $1 cmd 0x08 0x0006 $Adv_Itv_Min $Adv_Itv_Max $Adv_Type $Own_Addr_Type $Direct_Addr_Type $Direct_Addr $Adv_Ch_Map $Adv_Filter_Policy
