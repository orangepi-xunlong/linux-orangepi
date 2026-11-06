$(CVIARCH_L)_hdmi-y += chip/$(CVIARCH_L)/src/phy/phy.o
$(CVIARCH_L)_hdmi-y += chip/$(CVIARCH_L)/src/phy/phy_i2c.o

$(CVIARCH_L)_hdmi-y += chip/$(CVIARCH_L)/src/util/util.o

$(CVIARCH_L)_hdmi-y += chip/$(CVIARCH_L)/src/identification/identification.o
$(CVIARCH_L)_hdmi-y += chip/$(CVIARCH_L)/src/bsp/access.o
$(CVIARCH_L)_hdmi-y += chip/$(CVIARCH_L)/src/bsp/i2cm.o

$(CVIARCH_L)_hdmi-y += chip/$(CVIARCH_L)/src/core/audio.o
$(CVIARCH_L)_hdmi-y += chip/$(CVIARCH_L)/src/core/fc.o
$(CVIARCH_L)_hdmi-y += chip/$(CVIARCH_L)/src/core/hdmi_core.o
$(CVIARCH_L)_hdmi-y += chip/$(CVIARCH_L)/src/core/irq.o
$(CVIARCH_L)_hdmi-y += chip/$(CVIARCH_L)/src/core/main_controller.o
$(CVIARCH_L)_hdmi-y += chip/$(CVIARCH_L)/src/core/packets.o
$(CVIARCH_L)_hdmi-y += chip/$(CVIARCH_L)/src/core/video.o
$(CVIARCH_L)_hdmi-y += chip/$(CVIARCH_L)/src/core/api.o

$(CVIARCH_L)_hdmi-y += chip/$(CVIARCH_L)/src/edid/edid.o
$(CVIARCH_L)_hdmi-y += chip/$(CVIARCH_L)/src/edid/hdmivsdb.o
$(CVIARCH_L)_hdmi-y += chip/$(CVIARCH_L)/src/edid/desc.o
$(CVIARCH_L)_hdmi-y += chip/$(CVIARCH_L)/src/edid/data_block.o

$(CVIARCH_L)_hdmi-y += chip/$(CVIARCH_L)/src/hdcp/hdcp.o

$(CVIARCH_L)_hdmi-y += chip/$(CVIARCH_L)/src/scdc/scdc.o