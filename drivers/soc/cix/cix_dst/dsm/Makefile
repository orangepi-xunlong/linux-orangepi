# SPDX-License-Identifier: GPL-2.0

# Makefile for the DSM drivers.
#
# Each configuration option enables a list of files.

obj-y		+= dsm_core.o dsm_plat.o
obj-$(CONFIG_PLAT_DSM_TEST)		  	+= dsm_test.o
obj-$(CONFIG_DSM_EARLYBOOT_CLIENT)	+= dsm_earlyboot_client/