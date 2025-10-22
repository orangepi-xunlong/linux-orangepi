# SPDX-License-Identifier: GPL-2.0
#
# Makefile for the Linux Bluetooth HCI device drivers.
#

obj-$(CONFIG_BCM_BT_LPM) += bcm_btlpm.o
obj-$(CONFIG_RTL_BT_LPM) += rtl_btlpm.o
obj-$(CONFIG_XR_BT_LPM)  += xradio_btlpm.o
obj-$(CONFIG_BT_RTKBTUSB) += rtk_btusb.o
obj-$(CONFIG_BT_RTKH5) += hci_rtk_h5.o
ccflags-$(CONFIG_BT_RTKH5) += -I $(srctree)/drivers/bluetooth
