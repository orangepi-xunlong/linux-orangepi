#
# Makefile for MMC/SD card drivers
#

obj-$(CONFIG_MMC_BLOCK)		+= mmc_block.o
mmc_block-objs			:= block.o queue.o
mmc_block-$(CONFIG_FATFS_FS_SUPPORT) += mmc_fatfs.o

obj-$(CONFIG_MMC_TEST)		+= mmc_test.o

obj-$(CONFIG_SDIO_UART)		+= sdio_uart.o

