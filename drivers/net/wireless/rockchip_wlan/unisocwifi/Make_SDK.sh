#!/bin/bash
#copy code files of bsp\wifi\bt\ini to code
#sed -i 's/\r//' Make_SDK.sh, if \r error
#Make_SDK.sh A100 or Make_SDK.sh MV300
function do_make_clean()
{
	#clean ko
	PLAT_NAME=$1
	WORK_DIR=$2
	echo "do_make_clean"
	if [ ${PLAT_NAME} = "A100" ]; then
		./build.sh distclean
	else
		echo "not A100"
	fi	

}
function do_make_sdk()
{
	echo "do_make_sdk start"
	#path
	PLAT_NAME=$1
	WORK_DIR=$2
	PATH_BSP=$3
	PATH_WIFI=$4
	PATH_BT1=$5
	PATH_INI=$6
	if [ ${PLAT_NAME} = "A100" ]; then
		echo "BT2"
		PATH_BT2=$7
	elif [ ${PLAT_NAME} = "MV300" ]; then
		echo "AML path"
		PATH_BT_AMLOGIC_PLATFORM_PATCH=$7
	fi
	echo "pwd is $WORK_DIR"

	#SDK directory name is code
	SDK_NAME="CODE"
	SDK_PATH=${WORK_DIR}/$SDK_NAME
	echo "sdk_path is $SDK_PATH"
	#delete files in code directory
	rm -rf ${SDK_PATH}
	rm -rf ${WORK_DIR}/$SDK_NAME.tar
	echo "do_make_sdk rmdir finish"	
	mkdir ${SDK_PATH}
	echo "do_make_sdk mkdir finish"	

	#copy files
	cp -r $PATH_BSP/ $SDK_PATH/BSP
	echo "do_make_sdk copy bsp finish"	
	cp -r $PATH_WIFI/ $SDK_PATH/WIFI
	echo "do_make_sdk copy wifi finish"	
	
	cp -r $PATH_INI/ $SDK_PATH/INI
	echo "do_make_sdk copy ini finish"	
	if [ ${PLAT_NAME} = "A100" ]; then
		mkdir $SDK_NAME/BT
		mkdir $SDK_NAME/BT/driver
		cp -r $PATH_BT1/ $SDK_PATH/BT/driver
		echo "do_make_sdk copy bt finish"
		echo "copy BT2"
		cp -r $PATH_BT2/ $SDK_PATH/BT
	else
		cp -r $PATH_BT1/ $SDK_PATH/BT
		cp -r $PATH_BT_AMLOGIC_PLATFORM_PATCH/ $SDK_PATH/BT_AMLOGIC_PLATFORM_PATCH
		echo "do_make_sdk copy bt finish"
		#clean unuseless files
		rm -rf $SDK_PATH/BT/driver/.tmp_versions
		rm -rf $SDK_PATH/BT/driver/*.o
		rm -rf $SDK_PATH/BT/driver/.*.cmd
		rm -rf $SDK_PATH/BT/driver/*.mod.c
		rm -rf $SDK_PATH/BT/driver/uwe5621_bt_sdio.*
		rm -rf $SDK_PATH/BT/driver/Module.symvers
		rm -rf $SDK_PATH/BT/driver/modules.order
		echo "do_make_sdk BT/driver clean finish"
		
		rm -rf $SDK_PATH/BT/driver-usb/.tmp_versions
		rm -rf $SDK_PATH/BT/driver-usb/*.o
		rm -rf $SDK_PATH/BT/driver-usb/.*.cmd
		rm -rf $SDK_PATH/BT/driver-usb/*.mod.c
		rm -rf $SDK_PATH/BT/driver-usb/uwe5621_bt_sdio.*
		rm -rf $SDK_PATH/BT/driver-usb/Module.symvers
		rm -rf $SDK_PATH/BT/driver-usb/modules.order
		echo "do_make_sdk BT/driver-usb clean finish"
	fi
	
	#tar sdk
	echo "tar sdk start"
	tar cf $SDK_NAME.tar $SDK_NAME
	rm -rf ${SDK_PATH}	
	echo "do_make_sdk stop"
}

if [ $1 = "A100" ]; then
	echo "Make SDK Platform is $1"
	PLATFORM=$1
	PLATFORM_DIR=`pwd`
	echo "PLATFORM_DIR is $PLATFORM_DIR"
	SRC_BSP=${PLATFORM_DIR}/kernel/linux-4.9/drivers/net/wireless/uwe5622/unisocwcn
	SRC_WIFI=${PLATFORM_DIR}/kernel/linux-4.9/drivers/net/wireless/uwe5622/unisocwifi
	SRC_BT1=${PLATFORM_DIR}/kernel/linux-4.9/drivers/net/wireless/uwe5622/tty-sdio
	SRC_BT2=${PLATFORM_DIR}/../android/hardware/sprd/libbt
	SRC_INI=${PLATFORM_DIR}/../android/hardware/sprd/wlan/firmware/uwe5622/
	do_make_clean $PLATFORM $PLATFORM_DIR
	do_make_sdk $PLATFORM $PLATFORM_DIR $SRC_BSP $SRC_WIFI $SRC_BT1 $SRC_INI $SRC_BT2 
elif [ $1 = "MV300" ]; then
	echo "Make SDK Platform is $1"
	PLATFORM=$1
	PLATFORM_DIR=`pwd`
	echo "PLATFORM_DIR is $PLATFORM_DIR"
	SRC_BSP=${PLATFORM_DIR}/device/hisilicon/bigfish/sdk/source/component/wifi/drv/sdio_uwe5621/unisocwcn
	SRC_WIFI=${PLATFORM_DIR}/device/hisilicon/bigfish/sdk/source/component/wifi/drv/sdio_uwe5621/unisocwifi
	SRC_BT1=${PLATFORM_DIR}/device/hisilicon/bigfish/bluetooth/uwe5621/
	SRC_INI=${PLATFORM_DIR}/device/hisilicon/bigfish/bluetooth/uwe5621/driver/fw/uwe5621
	SRC_BT_AMLOGIC_PLATFORM_PATCH=${PLATFORM_DIR}/device/hisilicon/bigfish/bluetooth/amlogic_platform_patch
	do_make_sdk $PLATFORM $PLATFORM_DIR $SRC_BSP $SRC_WIFI $SRC_BT1 $SRC_INI $SRC_BT_AMLOGIC_PLATFORM_PATCH
else
	echo "Invalid platform"
fi
