#
#makefile for build atbm_wifi.ko
#
###############################################################################
#
# when release to customer ,the CUSTOMER_SUPPORT_USED must set to y!!!!!!!!!!!!!
#
###############################################################################
CUSTOMER_SUPPORT_USED=y
###############################################################################
#PLATFORM_MTK_9255 22
###############################################################################

export platform ?= PLATFORM_MTK_9255
ATBM_WIFI__EXT_CCFLAGS = -DATBM_WIFI_PLATFORM=22
#Android
#Linux
sys ?= linux
#arch:arm or arm64 or mips(NVT98517)
arch ?= arm
#ATBM_WIFI__EXT_CCFLAGS = -DATBM_WIFI_PLATFORM=$(platform)

ifeq ($(CUSTOMER_SUPPORT_USED),y)
    MAKEFILE_SUB ?= Makefile.build.customer
else
    MAKEFILE_SUB ?= Makefile.build
endif

ifeq ($(platform),)
    platform ?= PLATFORM_DEFAULT
endif

ifeq ($(KERNELRELEASE),)
    export DRIVER_PATH ?= $(shell pwd)

########################################PLATFORM_DEFAULT###########################
    ifeq ($(platform),PLATFORM_DEFAULT)
        KERDIR:=/home/builder/code/android/kernel/fusion/4.9
        CROSS_COMPILE:=/home/code/android/prebuilts/mtk_toolchain/linaro-4.9.3-2014.11-arm-linux-gnueabihf/bin/arm-linux-gnueabihf-
        arch = arm
        ATBM_WIFI__EXT_CCFLAGS = -DATBM_WIFI_PLATFORM=0
    endif
###################################### END ########################################

    ifeq ($(platform),PLATFORM_MTK_9255)
        ifeq ($(sys),Linux)
            KERDIR:=/wifi_prj/staff/zhouzhanchao/mstar/linux3.18_i3/
            CROSS_COMPILE:=/wifi_prj/staff/zhouzhanchao/mstar/arm-linux-gnueabihf-4.8.3-201404/bin/arm-linux-gnueabihf-
        else
            KERDIR:=/usr/lchome/zhengzhiqiang/9255_new/git_9255/kernel/fusion/4.9
            CROSS_COMPILE:=/usr/lchome/zhengzhiqiang/9255_new/git_9255/prebuilts/gcc/linux-x86/arm/arm-linux-androideabi-4.9/bin/arm-linux-androideabi-
        endif
        arch = arm
        ATBM_WIFI__EXT_CCFLAGS = -DATBM_WIFI_PLATFORM=22
    endif

    export arch ATBM_WIFI__EXT_CCFLAGS

all:install

install:
	@echo "make PLATFORM_CROSS=$(platform)"
	$(MAKE) all -f $(MAKEFILE_SUB) ARCH=$(arch)  CROSS_COMPILE=$(CROSS_COMPILE) KDIR=$(KERDIR) SYS=$(sys) PLAT=$(platform) -j8
clean:
	$(MAKE) -f $(MAKEFILE_SUB) KDIR=$(KERDIR) ARCH=$(arch) clean
strip:
	$(MAKE) -f $(MAKEFILE_SUB) KDIR=$(KERDIR) ARCH=$(arch) SYS=$(sys) PLAT=$(platform) strip
get_ver:
	$(MAKE) -f $(MAKEFILE_SUB) KDIR=$(KERDIR) ARCH=$(arch) SYS=$(sys) PLAT=$(platform) get_ver
buid_config:
	#@rm -rf ./.config
	$(MAKE) -C atbm_kconf clean
	$(MAKE) -C atbm_kconf mconf -f Makefile
menuconfig:buid_config
	@./atbm_kconf/mconf ./atbm_kconf/Kconfig
debug:
	$(MAKE) all -f $(MAKEFILE_SUB) ARCH=$(arch)  CROSS_COMPILE=$(CROSS_COMPILE) KDIR=$(KERDIR) SYS=$(sys) PLAT=$(platform) DEBUG_ZEZER=y -j8

else
    ifeq ($(platform),PLATFORM_DEFAULT)
        ATBM_WIFI__EXT_CCFLAGS = -DATBM_WIFI_PLATFORM=0
    endif
    ifeq ($(platform),PLATFORM_MTK_9255)
        ATBM_WIFI__EXT_CCFLAGS = -DATBM_WIFI_PLATFORM=22
    endif

export ATBM_WIFI__EXT_CCFLAGS
include $(srctree)/$(src)/Makefile.build.kernel
endif

