KMODULE_NAME=ssv6051

KBUILD_TOP := $(PWD)

ifeq ($(KERNELRELEASE),)

KVERS_UNAME ?= $(shell uname -r)
KVERS_ARCH ?= $(shell arch)

KBUILD ?= $(shell readlink -f /lib/modules/$(KVERS_UNAME)/build)

ifeq (,$(KBUILD))
$(error kernel build tree not found - set KBUILD to configured kernel)
endif

#KCONFIG := $(KBUILD)/config
#ifeq (,$(wildcard $(KCONFIG)))
#$(error No .config found in $(KBUILD), set KBUILD to configured kernel)
#endif

ifneq (,$(wildcard $(KBUILD)/include/linux/version.h))
ifneq (,$(wildcard $(KBUILD)/include/generated/uapi/linux/version.h))
$(error Multiple copied of version.h found, clean build tree)
endif
endif

# Kernel Makefile doesn't always know the exact kernel version, so we
# get it from the kernel headers instead and pass it to make.
VERSION_H := $(KBUILD)/include/generated/utsrelease.h
ifeq (,$(wildcard $(VERSION_H)))
VERSION_H := $(KBUILD)/include/linux/utsrelease.h
endif
ifeq (,$(wildcard $(VERSION_H)))
VERSION_H := $(KBUILD)/include/linux/version.h
endif
ifeq (,$(wildcard $(VERSION_H)))
$(error Please run 'make modules_prepare' in $(KBUILD))
endif

KVERS := $(shell sed -ne 's/"//g;s/^\#define UTS_RELEASE //p' $(VERSION_H))

ifeq (,$(KVERS))
$(error Cannot find UTS_RELEASE in $(VERSION_H), please report)
endif

INST_DIR = /lib/modules/$(KVERS)/misc

#include $(KCONFIG)

endif

include $(KBUILD_TOP)/platform-config.mak

EXTRA_CFLAGS := -I$(KBUILD_TOP) -I$(KBUILD_TOP)/include #-Wno-error=missing-attributes
DEF_PARSER_H = $(KBUILD_TOP)/include/ssv_conf_parser.h

OBJS := ssvdevice/ssvdevice.c \
	ssvdevice/ssv_cmd.c \
	hci/ssv_hci.c \
	smac/init.c \
	smac/dev.c \
	smac/ssv_rc.c \
	smac/ssv_ht_rc.c \
	smac/ap.c \
	smac/ampdu.c \
	smac/efuse.c \
	smac/ssv_pm.c \
	smac/sar.c \
	hwif/sdio/sdio.c \
	ssv6051-generic-wlan.c

ifeq ($(findstring -DCONFIG_SSV6XXX_DEBUGFS, $(ccflags-y)), -DCONFIG_SSV6XXX_DEBUGFS)
OBJS +=	smac/ssv6xxx_debugfs.c
endif

ifeq ($(findstring -DCONFIG_SSV_VENDOR_EXT_SUPPORT, $(ccflags-y)), -DCONFIG_SSV_VENDOR_EXT_SUPPORT)
OBJS +=	smac/ssv_cfgvendor.c
endif

ifeq ($(findstring -DCONFIG_SSV_SMARTLINK, $(ccflags-y)), -DCONFIG_SSV_SMARTLINK)
OBJS += smac/smartlink.c
endif

$(KMODULE_NAME)-y += $(ASMS:.S=.o)
$(KMODULE_NAME)-y += $(OBJS:.c=.o)

obj-$(CONFIG_SSV6200_CORE) += $(KMODULE_NAME).o

all: modules

modules:
	ARCH=arm $(MAKE) -C $(KBUILD) M=$(KBUILD_TOP)

clean:
	find -type f -iname '*.o' -exec rm {} \;
	find -type f -iname '*.o.cmd' -exec rm {} \;
	rm -f *.o *.ko .*.cmd *.mod.c *.symvers modules.order
	rm -rf .tmp_versions

install: modules
	mkdir -p -m 755 $(DESTDIR)$(INST_DIR)
	install -m 0644 $(KMODULE_NAME).ko $(DESTDIR)$(INST_DIR)
ifndef DESTDIR
	-/sbin/depmod -a $(KVERS)
endif

.PHONY: all modules clean install
