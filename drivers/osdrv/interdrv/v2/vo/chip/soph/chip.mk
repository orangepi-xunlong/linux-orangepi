CONFIG_CVI_LOG = 1

$(CVIARCH_L)_vo-objs += chip/$(CVIARCH_L)/disp.o
$(CVIARCH_L)_vo-objs += chip/$(CVIARCH_L)/dsi_phy.o
$(CVIARCH_L)_vo-objs += chip/$(CVIARCH_L)/vo_core.o
$(CVIARCH_L)_vo-objs += chip/$(CVIARCH_L)/vo.o
$(CVIARCH_L)_vo-objs += chip/$(CVIARCH_L)/vo_sdk_layer.o
$(CVIARCH_L)_vo-objs += chip/$(CVIARCH_L)/proc/vo_disp_proc.o
$(CVIARCH_L)_vo-objs += chip/$(CVIARCH_L)/proc/vo_proc.o

ifeq ($(CONFIG_CVI_LOG), 1)
ccflags-y += -DCONFIG_CVI_LOG
endif
