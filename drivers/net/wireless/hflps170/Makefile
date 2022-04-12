BL_VERS_NUM=5.5.0.0

CONFIG_BL_FULLMAC ?= m

KERNELVERSION := 	$(shell uname -r)

subdir-ccflags-$(CONFIG_DEBUG_FS) += -DCONFIG_BL_DEBUGFS
subdir-ccflags-y += -I$(src)/fullmac

# FW VARS
subdir-ccflags-y += -DNX_VIRT_DEV_MAX=4
subdir-ccflags-y += -DNX_REMOTE_STA_MAX=10
subdir-ccflags-y += -DNX_CHAN_CTXT_CNT=3

obj-$(CONFIG_BL_FULLMAC) += fullmac/

#KERNELDIR = ~/Workspace/linux-4.10
KERNELDIR ?= /lib/modules/$(KERNELVERSION)/build

# Enable A-MSDU support (need FW support)
## Select this if FW is compiled with AMSDU support
CONFIG_BL_SPLIT_TX_BUF ?= n
## Select this TO send AMSDU
CONFIG_BL_AMSDUS_TX ?= n

# Enable HW queue for Broadcast/Multicast traffic (need FW support)
CONFIG_BL_BCMC ?= y

# extra DEBUG config
CONFIG_BL_SW_PROFILING ?= n
CONFIG_BL_DBG ?= n
CONFIG_AUTO_DNLD ?= y

obj-m := bl_fdrv.o
bl_fdrv-y := bl_cfgfile.o           \
               fullmac/bl_main.o    \
               bl_mod_params.o      \
               bl_platform.o        \
               bl_sdio.o            \
               bl_msg_tx.o          \
               bl_msg_rx.o          \
               bl_utils.o           \
               bl_cmds.o            \
               bl_irqs.o            \
               ipc_host.o           \
               bl_txq.o             \
               bl_strs.o            \
               fullmac/bl_tx.o      \
               fullmac/bl_rx.o      \
               bl_v7.o              \
               bl_bootrom.o
bl_fdrv-$(CONFIG_DEBUG_FS)         += bl_debugfs.o

ccflags-y := -DCONFIG_BL_FULLMAC
ccflags-y += -I$(src)
ccflags-$(CONFIG_BL_SPLIT_TX_BUF) += -DCONFIG_BL_SPLIT_TX_BUF
ifeq ($(CONFIG_BL_SPLIT_TX_BUF), y)
ccflags-$(CONFIG_BL_AMSDUS_TX) += -DCONFIG_BL_AMSDUS_TX
endif
ccflags-$(CONFIG_BL_DBG) += -DCONFIG_BL_DBG
ccflags-$(CONFIG_BL_MOD_LEV_DBG) += -DCONFIG_BL_MOD_LEV_DBG

ccflags-y += -DCONFIG_USER_MAX=1

ifeq ($(CONFIG_BL_BCMC), y)
ccflags-y += -DNX_TXQ_CNT=5
else
ccflags-y += -DNX_TXQ_CNT=4
endif


quiet_cmd_genvers = GENVERSION $@
      cmd_genvers = ($(if $(KBUILD_EXTMOD),,$(srctree)/)$(src)/mkvers.sh bl_version_gen.h $@)

$(obj)/fullmac/bl_main.o: $(obj)/bl_version_gen.h

$(obj)/bl_version_gen.h: FORCE
	chmod 755 $(src)/mkvers.sh
	chmod 755 $(src)/mklink.sh
	$(call cmd,genvers)

clean-files := bl_version_gen.h


all: modules

modules clean:
	@$(PWD)/mklink.sh fullmac
	$(MAKE) -C $(KERNELDIR) O=$(KBUILDDIR) M=$(PWD) $@
	@$(PWD)/mklink.sh clean

