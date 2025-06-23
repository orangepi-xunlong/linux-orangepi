# SPDX-License-Identifier: GPL-2.0

ccflags-y := \
    -I $(srctree)/$(src)/include \
    -I $(srctree)/$(src)

linlon-dp-y := \
    linlondp_drv.o \
    linlondp_dev.o \
    linlondp_format_caps.o \
    linlondp_color_mgmt.o \
    linlondp_pipeline.o \
    linlondp_pipeline_state.o \
    linlondp_framebuffer.o \
    linlondp_kms.o \
    linlondp_crtc.o \
    linlondp_plane.o \
    linlondp_wb_connector.o \
    linlondp_private_obj.o \
    linlondp_event.o \
    linlondp_coeffs.o

linlon-dp-y += \
    hw/dp_dev.o \
    hw/dp_axi.o \
    hw/dp_component.o

obj-$(CONFIG_DRM_LINLONDP) += linlon-dp.o
