obj-$(CONFIG_CSI_VIN) += modules/sensor/
obj-$(CONFIG_CSI_VIN) += modules/actuator/
obj-$(CONFIG_CSI_VIN) += vin_io.o
vin_io-y					:= vin-cci/csi_cci_reg.o
vin_io-y					+= vin-cci/bsp_cci.o
vin_io-y					+= vin-cci/cci_helper.o
vin_io-y					+= vin-cci/sunxi_cci.o
vin_io-y					+= utility/vin_supply.o
vin_io-y					+= utility/vin_os.o

obj-$(CONFIG_CSI_VIN) += vin_v4l2.o
vin_v4l2-y					:= vin-csi/sunxi_csi.o
vin_v4l2-y					+= vin-csi/csi_reg.o
vin_v4l2-y					+= vin-csi/bsp_csi.o
ifneq ($(strip $(CONFIG_ARCH_SUN50IW3)),)
vin_v4l2-y					+= vin-csi/parser_reg.o
endif

vin_v4l2-y					+= vin-mipi/sunxi_mipi.o
vin_v4l2-y					+= vin-mipi/bsp_mipi_csi_null.o
vin_v4l2-y					+= vin-isp/sunxi_isp.o
vin_v4l2-y					+= vin-isp/bsp_isp.o \
							vin-isp/sun8iw6p1/sun8iw6p1_isp_reg_cfg.o \
							vin-isp/sun8iw12p1/sun8iw12p1_isp_reg_cfg.o \
							vin-isp/isp_platform_drv.o

vin_v4l2-y					+= vin-vipp/sunxi_scaler.o
vin_v4l2-y					+= modules/flash/flash.o
vin_v4l2-y					+= utility/bsp_common.o
vin_v4l2-y					+= utility/config.o
vin_v4l2-y					+= utility/sensor_info.o
vin_v4l2-y					+= utility/cfg_op.o
vin_v4l2-y					+= vin-stat/vin_ispstat.o
vin_v4l2-y					+= vin-stat/vin_h3a.o
vin_v4l2-y					+= vin-video/vin_video.o
vin_v4l2-y					+= vin-video/vin_core.o
ifneq ($(strip $(CONFIG_ARCH_SUN50IW3)),)
vin_v4l2-y					+= vin-video/dma_reg.o
vin_v4l2-y					+= top_reg.o
endif
vin_v4l2-y					+= vin.o




