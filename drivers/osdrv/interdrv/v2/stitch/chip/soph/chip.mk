CONFIG_CVI_LOG = 1
CONFIG_REG_DUMP = 0

$(CVIARCH_L)_stitch-objs += chip/$(CVIARCH_L)/stitch_core.o
#$(CVIARCH_L)_stitch-objs += chip/$(CVIARCH_L)/stitch_hal.o
$(CVIARCH_L)_stitch-objs += chip/$(CVIARCH_L)/stitch_reg_cfg.o

ifeq ($(CONFIG_CVI_LOG), 1)
ccflags-y += -DCONFIG_CVI_LOG
endif

ifeq ($(CONFIG_REG_DUMP), 1)
ccflags-y += -DCONFIG_REG_DUMP
endif
