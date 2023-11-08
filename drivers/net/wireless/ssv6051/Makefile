# SPDX-License-Identifier: ISC

include $(src)/platform-config.mak

ccflags-y += \
	-I $(srctree)/$(src) \
        -I $(srctree)/$(src)/include

obj-$(CONFIG_SSV6051) += ssv6051.o
ssv6051-objs += \
	ssv6051-generic-wlan.o \
	ssvdevice/ssvdevice.o \
	ssvdevice/ssv_cmd.o \
	hci/ssv_hci.o \
	smac/init.o \
	smac/dev.o \
	smac/ssv_rc.o \
	smac/ssv_ht_rc.o \
	smac/ap.o \
	smac/ampdu.o \
	smac/efuse.o \
	smac/ssv_pm.o \
	smac/sar.o \
	smac/ssv_cfgvendor.o \
	hwif/sdio/sdio.o

