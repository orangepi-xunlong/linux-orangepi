# SPDX-License-Identifier: GPL-2.0
#
# Makefile for the rpmsg drivers.
#
ccflags-y += -I $(srctree)/drivers/rpmsg
obj-$(CONFIG_AW_RPMSG_CTRL)	+= sunxi_rpmsg_ctrl.o
sunxi_rpmsg_ctrl-y		+= rpmsg_client.o
sunxi_rpmsg_ctrl-y		+= rpmsg_master.o
obj-$(CONFIG_AW_RPMSG_HEARTBEAT)	+= rpmsg_heartbeat.o
obj-$(CONFIG_AW_RPMSG_NOTIFY) += rpmsg_notify.o
