# SPDX-License-Identifier: GPL-2.0-or-later
obj-$(CONFIG_AW_HDMI2_DISP2_SUNXI) += hdmi20.o
ccflags-y := -DDYNAMIC_DEBUG_MODULE

hdmi20-y := aw_hdmi_drv.o
hdmi20-y += aw_hdmi_dev.o
ifeq ($(CONFIG_AW_HDMI2_LOG_BUFFER),y)
hdmi20-y += aw_hdmi_log.o
endif

hdmi20-y += aw_hdmi_core/aw_hdmi_core.o

hdmi20-y += aw_hdmi_core/dw_hdmi/dw_access.o
ifeq ($(CONFIG_AW_HDMI2_CEC_SUNXI),y)
hdmi20-y += aw_hdmi_core/dw_hdmi/dw_cec.o
endif
hdmi20-y += aw_hdmi_core/dw_hdmi/dw_i2cm.o
hdmi20-y += aw_hdmi_core/dw_hdmi/dw_scdc.o
hdmi20-y += aw_hdmi_core/dw_hdmi/dw_audio.o
hdmi20-y += aw_hdmi_core/dw_hdmi/dw_fc.o
hdmi20-y += aw_hdmi_core/dw_hdmi/dw_mc.o
hdmi20-y += aw_hdmi_core/dw_hdmi/dw_video.o
hdmi20-y += aw_hdmi_core/dw_hdmi/dw_phy.o
hdmi20-y += aw_hdmi_core/dw_hdmi/dw_edid.o
ifeq ($(CONFIG_AW_HDMI2_HDCP_SUNXI),y)
hdmi20-y += aw_hdmi_core/dw_hdmi/dw_hdcp.o
endif

ifeq ($(CONFIG_AW_HDMI2_HDCP22_SUNXI),y)
hdmi20-y += aw_hdmi_core/dw_hdmi/dw_hdcp22_tx.o
hdmi20-y += aw_hdmi_core/dw_hdmi/esm_lib/esm_host_lib_init.o
hdmi20-y += aw_hdmi_core/dw_hdmi/esm_lib/esm_host_lib_log.o
hdmi20-y += aw_hdmi_core/dw_hdmi/esm_lib/esm_host_lib_auth.o
hdmi20-y += aw_hdmi_core/dw_hdmi/esm_lib/esm_host_lib_code.o
hdmi20-y += aw_hdmi_core/dw_hdmi/esm_lib/esm_host_lib_exceptions.o
hdmi20-y += aw_hdmi_core/dw_hdmi/esm_lib/esm_host_lib_mailbox.o
hdmi20-y += aw_hdmi_core/dw_hdmi/esm_lib/esm_host_lib_status.o
endif

ifeq ($(CONFIG_AW_AWPHY),y)
hdmi20-y += aw_hdmi_core/dw_hdmi/phy_aw.o
endif
ifeq ($(CONFIG_AW_INNOPHY),y)
hdmi20-y += aw_hdmi_core/dw_hdmi/phy_inno.o
endif
ifeq ($(CONFIG_AW_SYNOPSYSPHY),y)
hdmi20-y += aw_hdmi_core/dw_hdmi/phy_snps.o
endif
ifeq ($(CONFIG_AW_INNOPHY_FPGA),y)
hdmi20-y += aw_hdmi_core/dw_hdmi/phy_inno_fpga.o
endif
