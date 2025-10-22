# SPDX-License-Identifier: GPL-2.0
#
# Makefile for the kernel mixer device drivers.
#
cur_dir = sunxi_device/hardware/lowlevel_de

ccflags-y += -I$(srctree)/$(obj)/$(cur_dir)/
ccflags-y += -I$(srctree)/$(obj)/$(cur_dir)/../../../
ccflags-y += -I$(srctree)/$(obj)/$(cur_dir)/csc/
ccflags-y += -I$(srctree)/$(obj)/$(cur_dir)/cdc/
ccflags-y += -I$(srctree)/$(obj)/$(cur_dir)/snr/
ccflags-y += -I$(srctree)/$(obj)/$(cur_dir)/sharp/
ccflags-y += -I$(srctree)/$(obj)/$(cur_dir)/dci/
ccflags-y += -I$(srctree)/$(obj)/$(cur_dir)/dlc/
ccflags-y += -I$(srctree)/$(obj)/$(cur_dir)/fcm/
ccflags-y += -I$(srctree)/$(obj)/$(cur_dir)/bld/
ccflags-y += -I$(srctree)/$(obj)/$(cur_dir)/ovl/
ccflags-y += -I$(srctree)/$(obj)/$(cur_dir)/scaler/
ccflags-y += -I$(srctree)/$(obj)/$(cur_dir)/afbd/
ccflags-y += -I$(srctree)/$(obj)/$(cur_dir)/tfbd/
ccflags-y += -I$(srctree)/$(obj)/$(cur_dir)/wb/
ccflags-y += -I$(srctree)/$(obj)/$(cur_dir)/gamma/
ccflags-y += -I$(srctree)/$(obj)/$(cur_dir)/dither/
ccflags-y += -I$(srctree)/$(obj)/$(cur_dir)/crc/
ccflags-y += -I$(srctree)/$(obj)/$(cur_dir)/smbl/
ccflags-y += -I$(srctree)/$(obj)/$(cur_dir)/deband/
ccflags-y += -I$(srctree)/$(obj)/$(cur_dir)/fmt/

de_obj += $(cur_dir)/de_backend.o
de_obj += $(cur_dir)/de_channel.o
de_obj += $(cur_dir)/sunxi_de.o
de_obj += $(cur_dir)/de_top.o
de_obj += $(cur_dir)/de_frontend.o
de_obj += $(cur_dir)/wb/de_wb.o
de_obj += $(cur_dir)/afbd/de_fbd_atw.o
de_obj += $(cur_dir)/tfbd/de_tfbd.o
de_obj += $(cur_dir)/csc/de_csc.o $(cur_dir)/csc/de_csc_table.o \
	  $(cur_dir)/csc/de_csc_platform.o
de_obj += $(cur_dir)/cdc/de_cdc.o $(cur_dir)/cdc/de_cdc_table.o \
	  $(cur_dir)/cdc/de_cdc_platform.o
de_obj += $(cur_dir)/snr/de_snr.o $(cur_dir)/snr/de_snr_platform.o
de_obj += $(cur_dir)/sharp/de_sharp.o $(cur_dir)/sharp/de_sharp_platform.o
de_obj += $(cur_dir)/dci/de_dci.o $(cur_dir)/dci/de_dci_platform.o
de_obj += $(cur_dir)/dlc/de_dlc.o $(cur_dir)/dlc/de_dlc_platform.o
de_obj += $(cur_dir)/fcm/de_fcm.o $(cur_dir)/fcm/de_fcm_platform.o
de_obj += $(cur_dir)/bld/de_bld.o $(cur_dir)/bld/de_bld_platform.o
de_obj += $(cur_dir)/ovl/de_ovl.o $(cur_dir)/ovl/de_ovl_platform.o
de_obj += $(cur_dir)/scaler/de_scaler.o $(cur_dir)/scaler/de_scaler_table.o \
	  $(cur_dir)/scaler/de_scaler_platform.o
de_obj += $(cur_dir)/gamma/de_gamma.o $(cur_dir)/gamma/de_gamma_platform.o
de_obj += $(cur_dir)/dither/de_dither.o $(cur_dir)/dither/de_dither_platform.o
de_obj += $(cur_dir)/crc/de_crc.o $(cur_dir)/crc/de_crc_platform.o
de_obj += $(cur_dir)/smbl/de_smbl.o $(cur_dir)/smbl/de_smbl_platform.o
de_obj += $(cur_dir)/deband/de_deband.o $(cur_dir)/deband/de_deband_platform.o
de_obj += $(cur_dir)/fmt/de_fmt.o $(cur_dir)/fmt/de_fmt_platform.o
