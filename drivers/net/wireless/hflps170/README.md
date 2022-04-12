===============================================================================
			U S E R  M A N U A L

  Copyright (C) BouffaloLab 2017-2020

1) FOR DRIVER BUILD

	Goto source code directory 
	make [clean]	
	The driver bl_fdrv.ko can be found in fullmac directory.
	The driver code supports Linux kernel from 3.10 to 5.5.19.

2) FOR DRIVER INSTALL

	a) Copy firmware image to /lib/firmware/, and rename to wholeimg_if.bin, copy bl_settings.ini to /lib/firmware/.
	b) Install WLAN driver
	   insmod bl_fdrv.ko
	c) uninstall driver
	   ifconfig wlan0 down
	   rmmod bl_fdrv



