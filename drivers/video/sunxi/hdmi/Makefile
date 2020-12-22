obj-$(CONFIG_HDMI_SUNXI) += hdcp.o

hdcp-y					:= aw/hdmi_core.o
hdcp-y					+= aw/hdmi_hal.o
hdcp-y					+= aw/hdmi_interface.o
hdcp-y					+= aw/hdmi_edid.o
hdcp-y					+= drv_hdmi.o
hdcp-y					+= dev_hdmi.o
hdcp-y					+= libhdcp.a

#subdir- += aw/hdcp

quiet_cmd_libhdcp = COPY    $@
      cmd_libhdcp = cp -vfr $(obj)/aw/libhdcp $@

$(obj)/libhdcp.a: $(obj)/aw/libhdcp
	$(call if_changed,libhdcp)

$(obj)/aw/libhdcp: FORCE
	$(if $(wildcard $(obj)/aw/hdcp/Makefile), \
		$(Q)$(MAKE) $(build)=$(obj)/aw/hdcp libhdcp_install, \
		)
