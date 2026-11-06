# SPDX-License-Identifier: GPL-2.0-only
#
# Makefile for the sophgo video device drivers.
#

# export CVIARCH_L := $(shell echo $(CVIARCH) | tr A-Z a-z)
export CVIARCH_L := cv186x

# isp/cif driver
obj-$(CONFIG_SOPH_ISP_V4L2_SUPPORT)     += ispv4l2/
obj-$(CONFIG_SOPH_ISP_V4L2_SUPPORT)     += cif_v4l2/
# sensor driver
obj-y  += sensor/
