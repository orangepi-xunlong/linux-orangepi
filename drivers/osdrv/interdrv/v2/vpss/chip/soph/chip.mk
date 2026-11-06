CONFIG_LOG = 1
CONFIG_REG_DUMP = 0

$(CVIARCH_L)_vpss-objs += chip/$(CVIARCH_L)/vpss_platform.o
$(CVIARCH_L)_vpss-objs += chip/$(CVIARCH_L)/vpss_core.o
$(CVIARCH_L)_vpss-objs += chip/$(CVIARCH_L)/vpss_hal.o
$(CVIARCH_L)_vpss-objs += chip/$(CVIARCH_L)/cmdq.o
$(CVIARCH_L)_vpss-objs += chip/$(CVIARCH_L)/scaler.o

ifeq ($(CONFIG_LOG), 1)
ccflags-y += -DCONFIG_LOG
endif

ifeq ($(CONFIG_REG_DUMP), 1)
ccflags-y += -DCONFIG_REG_DUMP
endif