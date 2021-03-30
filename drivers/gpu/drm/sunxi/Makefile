
low_level_dir = drivers/video/sunxi/disp2/disp
hdmi_dir = drivers/video/sunxi/disp2/hdmi
sub_dir = lowlevel_v2x

include $(low_level_dir)/de/Makefile
include $(low_level_dir)/de/$(sub_dir)/Makefile

ccflags-y := -Iinclude/drm -Idrivers/gpu/drm/sunxi -Idrivers/video/sunxi/disp2/disp -Idrivers/video/sunxi/disp2/disp/de/$(sub_dir)
ccflags-y += -I$(low_level_dir)/$(sub_dir)
ccflags-y += -I$(hdmi_dir)

ifeq ($(CONFIG_ARM64),y)
$(shell cp $(obj)/../../../../$(hdmi_dir)/libhdmi_sun50iw1 $(obj)/subdev/libhdmi_sunxi.a)
endif

ifeq ($(CONFIG_ARM),y)
$(shell cp $(obj)/../../../../$(hdmi_dir)/libhdmi_sun8iw11 $(obj)/subdev/libhdmi_sunxi.a)
endif

sunxidrm-y := sunxi_drm_core.o sunxi_drm_crtc.o  sunxi_drm_connector.o sunxi_drm_dmabuf.o \
			  sunxi_drm_encoder.o sunxi_drm_fbdev.o sunxi_drm_gem.o sunxi_drm_panel.o \
			  sunxi_drm_drv.o sunxi_drm_fb.o sunxi_drm_plane.o

sunxidrm-y += drm_de/drm_al.o
sunxidrm-y += subdev/sunxi_lcd.o subdev/sunxi_common.o subdev/sunxi_hdmi.o subdev/sun50iw1_transform.o subdev/sunxi_rotate.o
sunxidrm-y += subdev/libhdmi_sunxi.a

filter_obj = $(filter-out $(mydir)/disp_al.o,$(obj_low))

drm_low_obj = $(addprefix ../../../../$(low_level_dir)/,$(filter_obj))

sunxidrm-y += $(drm_low_obj)

obj-$(CONFIG_DRM_SUNXI)        += sunxidrm.o

