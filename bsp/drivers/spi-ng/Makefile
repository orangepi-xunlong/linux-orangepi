# SPDX-License-Identifier: GPL-2.0
ccflags-y := -DDYNAMIC_DEBUG_MODULE
ccflags-y += -I $(srctree)/bsp/drivers/spi-ng

obj-$(CONFIG_AW_SPI_NG) += spi-sunxi-ng.o
spi-sunxi-ng-objs += spi-sunxi.o calibrate/spi-sunxi-calibrate.o
spi-sunxi-ng-$(CONFIG_AW_SPI_NG_BIT) += bit/spi-sunxi-bit.o
spi-sunxi-ng-$(CONFIG_AW_SPI_NG_DBI) += dbi/spi-sunxi-dbi.o
spi-sunxi-ng-$(CONFIG_AW_SPI_NG_CAMERA) += camera/spi-sunxi-camera.o

obj-$(CONFIG_AW_SPI_SLAVETEST) += spi-sunxi-slave-test.o
obj-$(CONFIG_AW_SPI_CAMERATEST) += spi-sunxi-camera-test.o
