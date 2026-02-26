#
# Makefile for the touchscreen drivers.
#

# Each configuration option enables a list of files.

ccflags-y += -Wall

ccflags-y += -I$(srctree)/drivers/input/ctp/ilitek_tddi/
ccflags-y += -I$(srctree)/drivers/input/ctp/ilitek_tddi/firmware/

ifdef CONFIG_TOUCHSCREEN_ILITEK_TDDI_I2C_TRANSFER
interface=ilitek_v3_i2c
fwupdate=ilitek_v3_flash
else ifdef CONFIG_TOUCHSCREEN_ILITEK_TDDI_SPI_TRANSFER
interface=ilitek_v3_spi
fwupdate=ilitek_v3_hostdl
endif

obj-$(CONFIG_TOUCHSCREEN_ILITEK_TDDI) += ilitek.o

ilitek-objs := ilitek_v3.o \
	ilitek_v3_init.o \
	ilitek_v3_ic.o \
	ilitek_v3_touch.o \
	ilitek_v3_mp.o \
	$(interface).o \
	$(fwupdate).o \
	ilitek_v3_node.o
