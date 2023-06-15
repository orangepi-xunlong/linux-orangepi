obj-$(CONFIG_HDMI2_DISP2_SUNXI) += hdmi20.o

hdmi20-y := hdmi_tx.o
#hdmi20-y += hdmi_test.o

ifeq ($(CONFIG_HDMI2_FREQ_SPREAD_SPECTRUM),y)
hdmi20-y += freq_ss.o
endif

ifeq ($(CONFIG_HDMI2_CEC_SUNXI),y)
hdmi20-y += hdmi_core/core_cec.o
endif

ifeq ($(CONFIG_HDMI2_HDCP_SUNXI),y)
hdmi20-y += hdmi_core/core_hdcp.o
endif

hdmi20-y += hdmi_core/core_edid.o
hdmi20-y += hdmi_core/hdmi_core.o


hdmi20-y += hdmi_core/api/api.o
hdmi20-y += hdmi_core/api/access.o

ifeq ($(CONFIG_HDMI2_CEC_SUNXI),y)
hdmi20-y += hdmi_core/cec/cec.o
endif

hdmi20-y += hdmi_core/api/core/audio.o
hdmi20-y += hdmi_core/api/core/fc_audio.o
hdmi20-y += hdmi_core/api/core/fc_video.o
hdmi20-y += hdmi_core/api/core/irq.o
hdmi20-y += hdmi_core/api/core/main_controller.o
hdmi20-y += hdmi_core/api/core/packets.o
hdmi20-y += hdmi_core/api/core/video.o
hdmi20-y += hdmi_core/api/core/hdr10p.o

hdmi20-y += hdmi_core/api/edid.o

ifeq ($(CONFIG_HDMI2_HDCP_SUNXI),y)
hdmi20-y += hdmi_core/api/hdcp.o
endif

hdmi20-y += hdmi_core/api/identification.o

ifeq ($(CONFIG_AW_PHY),y)
hdmi20-y += hdmi_core/api/aw_phy.o
else
hdmi20-y += hdmi_core/api/phy.o
endif

hdmi20-y += hdmi_core/api/scdc.o
hdmi20-y += hdmi_core/api/general_ops.o

ifeq ($(CONFIG_HDMI2_HDCP22_SUNXI),y)
hdmi20-y += hdmi_core/api/hdcp22_tx.o
hdmi20-y += hdmi_core/api/hdcp22/esm_host_lib_init.o
hdmi20-y += hdmi_core/api/hdcp22/esm_host_lib_log.o
hdmi20-y += hdmi_core/api/hdcp22/esm_host_lib_auth.o
hdmi20-y += hdmi_core/api/hdcp22/esm_host_lib_code.o
hdmi20-y += hdmi_core/api/hdcp22/esm_host_lib_exceptions.o
hdmi20-y += hdmi_core/api/hdcp22/esm_host_lib_mailbox.o
hdmi20-y += hdmi_core/api/hdcp22/esm_host_lib_status.o
endif
