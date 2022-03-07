# SPDX-License-Identifier: GPL-2.0
obj-$(CONFIG_AP6XXX)	+= rkwifi/
obj-$(CONFIG_RTL8188EU)	+= rtl8188eu/
obj-$(CONFIG_RTL8188FU)	+= rtl8188fu/
obj-$(CONFIG_RTL8189ES) += rtl8189es/
obj-$(CONFIG_RTL8189FS) += rtl8189fs/
obj-$(CONFIG_RTL8723BS) += rtl8723bs/
obj-$(CONFIG_RTL8723BU) += rtl8723bu/
obj-$(CONFIG_RTL8723CS)	+= rtl8723cs/
obj-$(CONFIG_RTL8723DS) += rtl8723ds/
obj-$(CONFIG_RTL8822BE)	+= rtl8822be/
obj-$(CONFIG_SSV6051)	+= ssv6xxx/
obj-$(CONFIG_MVL88W8977)	+= mvl88w8977/
obj-$(CONFIG_WL_ROCKCHIP)	+= wifi_sys/rkwifi_sys_iface.o
obj-$(CONFIG_WL_ROCKCHIP)	+= rkwifi/rk_wifi_config.o
obj-$(CONFIG_CYW_BCMDHD)	+= cywdhd/

obj-$(CONFIG_SPRDWL_NG)                += unisocwifi/
UNISOCWCN_DIR := $(shell cd $(srctree)/drivers/misc/unisocwcn/ && /bin/pwd)
UNISOC_BSP_INCLUDE := $(UNISOCWCN_DIR)/include
export UNISOC_BSP_INCLUDE

UNISOC_DBG_PATH_CONFIG := "/dev/unisoc_dbg"
export UNISOC_DBG_PATH_CONFIG
