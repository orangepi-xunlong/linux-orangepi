trilin-dpsub-y := trilin_dptx_cix.o trilin_dptx.o trilin_host_tmr.o dptx_infoframe.o trilin_dptx_audio.o trilin_drm.o trilin_drm_mst.o
trilin-dpsub-y += trilin_edp_phy.o trilin_usbdp_phy.o
trilin-dpsub-$(CONFIG_TRILIN_DP_HDCP_VALIDATION) += hdcp/hdcp2_hw_tx.o
trilin-dpsub-$(CONFIG_TRILIN_DP_HDCP_VALIDATION) += hdcp/hdcp2_key_drvn_tx.o
trilin-dpsub-$(CONFIG_TRILIN_DP_HDCP_VALIDATION) += hdcp/hdcp2_tx_state.o
trilin-dpsub-y += hdcp/hdcp2_tx_tmr.o
trilin-dpsub-y += hdcp/cix_hdcp_ioctl.o
trilin-dpsub-y += hdcp/cix_hdcp.o
trilin-dpsub-y += cix_edp_panel.o

obj-$(CONFIG_DRM_TRILIN_DPSUB) += trilin-dpsub.o