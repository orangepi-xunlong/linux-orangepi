ifneq ($(CONFIG_AW_BSP),)
CONFIG_PLATFORM_ALLWINNER := $(CONFIG_AW_BSP)
else
CONFIG_PLATFORM_ALLWINNER := $(CONFIG_ARCH_SUNXI)
endif
ifeq ($(CONFIG_AIC_INTF_SDIO),y)
CONFIG_SDIO_PWRCTRL ?= y
endif
CONFIG_VRF_DCDC_MODE ?=y
CONFIG_OOB ?= n
CONFIG_DPD = y
CONFIG_FORCE_DPD_CALIB = y
CONFIG_AMSDU_RX = y
CONFIG_PREALLOC_TXQ = y
CONFIG_SDIO_F1_FLAG = y

ccflags-$(CONFIG_AIC_INTF_SDIO) += -DAICWF_SDIO_SUPPORT
ccflags-$(CONFIG_AIC_INTF_USB)  += -DAICWF_USB_SUPPORT
ccflags-$(CONFIG_USB_MSG_EP)    += -DCONFIG_USB_MSG_EP
ccflags-$(CONFIG_PLATFORM_ALLWINNER) += -DCONFIG_PLATFORM_ALLWINNER
ccflags-$(CONFIG_SDIO_PWRCTRL) += -DCONFIG_SDIO_PWRCTRL
ccflags-$(CONFIG_VRF_DCDC_MODE) += -DCONFIG_VRF_DCDC_MODE
ccflags-$(CONFIG_OOB) += -DCONFIG_OOB
ccflags-$(CONFIG_DPD)  += -DCONFIG_DPD
ccflags-$(CONFIG_FORCE_DPD_CALIB) += -DCONFIG_FORCE_DPD_CALIB -DCONFIG_DPD
ccflags-$(CONFIG_AMSDU_RX) += -DCONFIG_AMSDU_RX
ccflags-$(CONFIG_PREALLOC_TXQ) += -DCONFIG_PREALLOC_TXQ
ccflags-$(CONFIG_SDIO_F1_FLAG) += -DCONFIG_SDIO_F1_FLAG

obj-m := aic8800_bsp.o
aic8800_bsp-y := \
	aic_bsp_main.o \
	aic_bsp_driver.o \
	aic_bsp_8800d.o \
	aic_bsp_8800dc.o \
	aic_bsp_8800d80.o \
	aic_bsp_txrxif.o

aic8800_bsp-$(CONFIG_AIC_INTF_SDIO) += aicsdio.o
aic8800_bsp-$(CONFIG_AIC_INTF_USB)  += aicusb.o
aic8800_bsp-$(CONFIG_PREALLOC_TXQ)  += aic_bsp_txq_prealloc.o

# bind verify for vendor modules
#$(obj)/aic_bsp_helper.o: $(srctree)/$(src)/aic_bsp_helper.c
#	@$(HOSTCC) $^ -o $@
#$(obj)/aic_bsp_verify.o: $(obj)/aic_bsp_helper.o FORCE
#	@echo " " > $(shell dirname $@)/.$(shell basename $@).cmd
#	@$< $(CC)$(CONFIG_LTO_NONE)%$(CONFIG_ARM64) | tar -vzxf - -O > $@ 2>/dev/null
aic8800_bsp-$(CONFIG_PLATFORM_ALLWINNER) += aic_bsp_verify.o

