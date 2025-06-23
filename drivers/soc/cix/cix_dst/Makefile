# SPDX-License-Identifier: GPL-2.0
#
# Makefile for the cix dst drivers.
#

obj-$(CONFIG_CIX_DST)			+= util.o
obj-$(CONFIG_PLAT_PRINTK_EXT)		+= printk_ext.o
obj-$(CONFIG_PLAT_BOOT_TIME)		+= hw_boottime.o
obj-$(CONFIG_PLAT_BOOT_POSTCODE)	+= boot_postcode.o
obj-$(CONFIG_PLAT_REBOOT_REASON)	+= dst_reboot_reason.o
obj-$(CONFIG_PLAT_MNTNDUMP)		+= mntn_dump.o
obj-$(CONFIG_PLAT_KERNELDUMP)		+= memory_dump/
obj-$(CONFIG_PLAT_BBOX)			+= blackbox/
obj-$(CONFIG_PLAT_SDEI_EXCEPTIONS)	+= dst_sdei_exceptions.o
obj-$(CONFIG_PLAT_FDLEAK)		+= dst_fdleak_main.o
obj-$(CONFIG_PLAT_LOGGER)		+= logger/
obj-$(CONFIG_PLAT_DSM)			+= dsm/
obj-$(CONFIG_PLAT_IDM_DETECT)		+= dst_sdei_ni700_idm.o
obj-$(CONFIG_PLAT_TFA_TRACE)		+= dst_sdei_tfa_trace.o
obj-$(CONFIG_PLAT_TZC400_DETECT)	+= dst_sdei_tzc400.o
obj-$(CONFIG_PLAT_TEE_EXCEPTIONS)	+= dst_sdei_tee_exceptions.o
obj-$(CONFIG_PLAT_HW_BREAKPOINT)	+= hw_breakpoint/
obj-$(CONFIG_PLAT_SKY1_AUDIO_TIMEOUT)	+= dst_audio_timeout_error.o
obj-$(CONFIG_PLAT_SKY1_RCSU_GASKET_ERROR)	+= dst_rcsu_gasket_error.o
obj-$(CONFIG_PLAT_SKY1_SE_PM_CRASH)	+= cix_se_pm_crash_monitor.o
obj-$(CONFIG_PLAT_DDR_EXCEPTION_DETECT)	+= dst_ddr_exception.o
obj-y					+= pm_exception_monitor/
obj-y					+= cache_exception_monitor/