The driver is used for:
---------

    Allwinner platform

How to:
---------
1. make a directory in kernel's tree, such as drivers/input/gsensor/mir3da

2. copy following files to the new folder:
        mir3da_cust.h
        mir3da_cust.c
        ../mir3da_core.h
        ../mir3da_core.c

3. to make build take effect, you should modify the sysconfig.fex in configs/android/wing-xxx
				[gsensor_para]
				gsensor_used		= 1
				gsensor_twi_id	= 1//please according to the hardware configure to adjust the twi_id

4. Confirm chip's I2C slave address in sysconfig.fex
       	Our I2C slave address is 0x26<-> SD0=GND;0x27<-> SD0=High
       	if you want to know more detail please see the datasheet

5. choose system platform
				If your platform is A13,please set #define ALLWINNER_A13(about line 46) to 1,else set to 0

6, calibrate
				add MiramemsGsensorTool.APK to system APK.
				Reference readme.txt in Settings to modify display_settings.xml,display_settings.java,strings.xml
				Reference readme.txt in bootable to modify recovery.c or recovery.cpp
Note:
    1. all source in hal folder is only for youre reference, and please modify it base on customer used implmention

