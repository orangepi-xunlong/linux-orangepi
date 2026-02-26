# SPDX-License-Identifier: GPL-2.0-only
ccflags-y += -I $(srctree)/drivers/net/ethernet/stmicro/stmmac/
ccflags-y += -DDYNAMIC_DEBUG_MODULE

obj-$(CONFIG_AW_DWMAC_SUNXI) += sunxi-stmmac.o
sunxi-stmmac-objs += dwmac-sunxi.o dwmac-sunxi-sysfs.o
obj-$(CONFIG_AW_STMMAC_UIO) += sunxi-uio.o
