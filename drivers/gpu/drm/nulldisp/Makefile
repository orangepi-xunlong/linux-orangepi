# SPDX-License-Identifier: GPL-2.0
ccflags-y += -I$(srctree)/drivers/gpu/drm/img-rogue/ \
 -I$(srctree)/$(src)

obj-${CONFIG_POWERVR_ROGUE_NULLDRMDISP} += drm_nulldisp.o
drm_nulldisp-y := drm_nulldisp_drv.o drm_nulldisp_netlink.o drm_netlink_gem.o drm_nulldisp_gem.o
