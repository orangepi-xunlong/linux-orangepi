KMODULE_NAME = ssv6x5x

ifeq ($(KERNEL_OBJ_PATH),)
KERNEL_OBJ_PATH := /lib/modules/`uname -r`/build
endif

ifeq ($(SSV_DRV_PATH),)
SSV_DRV_PATH := $(PWD)
endif

KBUILD_TOP := $(srctree)/$(src)
ifeq ($(MAKELEVEL),0)
KBUILD_TOP := .
endif

ifeq ($(ARCH),)
ARCH := x86
endif

ifeq ($(INSTALL_PATH),)
INSTALL_PATH := kernel/drivers/net/wireless/ssv6200
endif

include $(KBUILD_TOP)/config_common.mak
include $(KBUILD_TOP)/config.mak

MOD_DEF_H = include/ssv_mod_conf.h

# Generate version strings
# GEN_VER := $(shell cd $(KBUILD_TOP); ./ver_info.pl include/ssv_version.h)
# Generate include/ssv_conf_parser.h
# GEN_CONF_PARSER := $(shell cd $(KBUILD_TOP); env ccflags="$(ccflags-y)" ./parser-conf.sh include/ssv_conf_parser.h)
# Generate $(KMODULE_NAME)-wifi.cfg
#BKP_CFG := $(shell cp $(KBUILD_TOP)/$(PLATFORMS)-wifi.cfg $(KBUILD_TOP)/image/$(KMODULE_NAME)-wifi.cfg)

EXTRA_CFLAGS := -I$(KBUILD_TOP) -I$(KBUILD_TOP)/include

DEF_PARSER_H = $(KBUILD_TOP)/include/ssv_conf_parser.h
$(shell env ccflags="$(ccflags-y)" $(KBUILD_TOP)/parser-conf.sh $(DEF_PARSER_H))

#------------------------------
# ssvdevice/
KERN_SRCS := ssvdevice/ssvdevice.c
KERN_SRCS += ssvdevice/ssv_cmd.c

# hci/
KERN_SRCS += hci/ssv_hci.c

# smac/
KERN_SRCS += smac/init.c
KERN_SRCS += smac/dev.c
KERN_SRCS += smac/bdev.c
KERN_SRCS += smac/regd.c
KERN_SRCS += smac/wow.c
KERN_SRCS += smac/hw_scan.c

KERN_SRCS += smac/lib.c
KERN_SRCS += smac/ap.c

ifeq ($(findstring -DCONFIG_SSV6XXX_DEBUGFS, $(ccflags-y)), -DCONFIG_SSV6XXX_DEBUGFS)
KERN_SRCS += smac/ssv6xxx_debugfs.c
endif
KERN_SRCS += smac/efuse.c
KERN_SRCS += smac/ssv_skb.c
ifeq ($(findstring -DCONFIG_SSV_CTL, $(ccflags-y)), -DCONFIG_SSV_CTL)
KERN_SRCS += smac/ssv_wifi_ctl.c
ifeq ($(findstring -DCONFIG_SSV_SMARTLINK, $(ccflags-y)), -DCONFIG_SSV_SMARTLINK)
KERN_SRCS += smac/kssvsmart.c
endif
endif

KERN_SRCS += smac/hal/hal.c
ifeq ($(findstring -DSSV_SUPPORT_SSV6006, $(ccflags-y)), -DSSV_SUPPORT_SSV6006)
KERN_SRCS += smac/hal/ssv6006c/ssv6006_common.c
KERN_SRCS += smac/hal/ssv6006c/ssv6006C_mac.c
KERN_SRCS += smac/hal/ssv6006c/ssv6006_phy.c
KERN_SRCS += smac/hal/ssv6006c/ssv6006_turismoC.c
endif
ifeq ($(findstring -DSSV_SUPPORT_SSV6020, $(ccflags-y)), -DSSV_SUPPORT_SSV6020)
KERN_SRCS += smac/hal/ssv6020/ssv6020_common.c
KERN_SRCS += smac/hal/ssv6020/ssv6020_mac.c
KERN_SRCS += smac/hal/ssv6020/ssv6020_phy.c
KERN_SRCS += smac/hal/ssv6020/ssv6020_turismoE.c
endif

# hwif/hal/
KERN_SRCS += hwif/hal/hwif_hal.c
ifeq ($(findstring -DSSV_SUPPORT_SSV6006, $(ccflags-y)), -DSSV_SUPPORT_SSV6006)
KERN_SRCS += hwif/hal/ssv6006c/ssv6006C_hwif.c
endif
ifeq ($(findstring -DSSV_SUPPORT_SSV6020, $(ccflags-y)), -DSSV_SUPPORT_SSV6020)
KERN_SRCS += hwif/hal/ssv6020/ssv6020_hwif.c
endif

# hwif/sdio/
KERN_SRCS += hwif/sdio/sdio.c


#------------------------------

KERN_SRCS += $(KMODULE_NAME)-generic-wlan.c

$(KMODULE_NAME)-y += $(KERN_SRCS_S:.S=.o)
$(KMODULE_NAME)-y += $(KERN_SRCS:.c=.o)

obj-$(CONFIG_SSV6X5X) += $(KMODULE_NAME).o

#export CONFIG_SSV6X5X=m

.PHONY: all ver modules clean

all: $(MOD_DEF_H) modules

modules:
	$(MAKE) -C $(KERNEL_OBJ_PATH) M=$(SSV_DRV_PATH) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) modules

strip:
	$(CROSS_COMPILE)strip $(KMODULE_NAME).ko --strip-unneeded

install:
	$(MAKE) INSTALL_MOD_DIR=$(INSTALL_PATH) -C $(KERNEL_OBJ_PATH) \
            M=$(SSV_DRV_PATH) modules_install
	depmod -a

clean:
	$(MAKE) -C $(KERNEL_OBJ_PATH) M=$(SSV_DRV_PATH) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) clean
	@rm -rf $(MOD_DEF_H)

$(MOD_DEF_H): config.mak config_common.mak
	#@echo "#ifndef __SSV_MOD_CONF_H__" > $@
	#@echo "#define __SSV_MOD_CONF_H__" >> $@
	#for flag in $(ccflags-y_no_dash); do \
	#   if [ "$$flag" =~ ^D.* ]; then \
	#       echo "#define $$flag" | sed -e s/D//g >> $@; \
	#   fi; \
	#done
	#echo "#endif // __SSV_MOD_CONF_H__" >> $@
	env ccflags="$(ccflags-y)" ./genconf.sh $@
