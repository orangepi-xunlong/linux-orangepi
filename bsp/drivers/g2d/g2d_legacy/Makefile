# SPDX-License-Identifier: GPL-2.0

my_dir = g2d_legacy
legacy_obj = ${my_dir}/g2d_driver.o

#ccflags-y += -I $(srctree)/drivers/hwspinlock
ccflags-y += -I $(srctree)/bsp/drivers/hwspinlock
ccflags-y += -I $(srctree)/bsp/drivers/g2d
CFLAGS_g2d_driver.o := -I$(src)/$(my_dir)
ifneq ($(findstring ${CONFIG_ARCH_SUN8IW11P1}${CONFIG_ARCH_SUN8IW11},y y), )
legacy_obj += ${my_dir}/g2d_bsp_sun8iw11.o
else
legacy_obj += ${my_dir}/g2d_bsp_v2.o
ccflags-y += -DG2D_V2X_SUPPORT
endif
