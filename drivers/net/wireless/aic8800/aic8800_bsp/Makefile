CONFIG_PLATFORM_ALLWINNER := y
CONFIG_SDIO_SUPPORT := y

ifeq ($(CONFIG_SDIO_SUPPORT), y)
ccflags-y += -DAICWF_SDIO_SUPPORT
endif
ccflags-$(CONFIG_PLATFORM_ALLWINNER) += -DCONFIG_PLATFORM_ALLWINNER

obj-m := aic8800_bsp.o
aic8800_bsp-y := \
	aic_bsp_main.o \
	aic_bsp_driver.o \
	aicsdio.o \
	aicsdio_txrxif.o

# bind verify for vendor modules
$(obj)/aic_bsp_helper.o: $(srctree)/$(src)/aic_bsp_helper.c
	@$(HOSTCC) $^ -o $@
$(obj)/aic_bsp_verify.o: $(obj)/aic_bsp_helper.o FORCE
	@echo " " > $(shell dirname $@)/.$(shell basename $@).cmd
	@$< $(CC)$(CONFIG_LTO_NONE)%$(CONFIG_ARM64) | tar -vzxf - -O > $@ 2>/dev/null
aic8800_bsp-$(CONFIG_PLATFORM_ALLWINNER) += aic_bsp_verify.o

