#
# Makefile for the kernel mixer device drivers.
#

include ${obj}/g2d_rcq/Makefile

obj-$(CONFIG_SUNXI_G2D) += g2d_sunxi.o
g2d_sunxi-${CONFIG_ARCH_SUN8IW12P1} += g2d_driver.o g2d_bsp_v2.o
g2d_sunxi-${CONFIG_ARCH_SUN8IW17P1} += g2d_driver.o g2d_bsp_v2.o
g2d_sunxi-${CONFIG_ARCH_SUN8IW11P1} += g2d_driver.o g2d_bsp_sun8iw11.o
g2d_sunxi-${CONFIG_ARCH_SUN8IW15P1} += g2d_driver.o g2d_bsp_v2.o
g2d_sunxi-${CONFIG_ARCH_SUN8IW16P1} += g2d_driver.o g2d_bsp_v2.o
g2d_sunxi-${CONFIG_ARCH_SUN50IW9P1} += g2d_driver.o g2d_bsp_v2.o
g2d_sunxi-${CONFIG_ARCH_SUN50IW9}   += g2d_driver.o g2d_bsp_v2.o
g2d_sunxi-${CONFIG_ARCH_SUN50IW10P1} += g2d_driver.o g2d_bsp_v2.o

g2d_sunxi-$(CONFIG_ARCH_SUN8IW19P1) += ${rcq_obj}
g2d_sunxi-$(CONFIG_SUNXI_SYNCFENCE) += syncfence.o
g2d_sunxi-$(CONFIG_ARCH_SUN8IW20) += ${rcq_obj}
g2d_sunxi-$(CONFIG_ARCH_SUN20IW1) += ${rcq_obj}
