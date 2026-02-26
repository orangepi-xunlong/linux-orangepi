# SPDX-License-Identifier: GPL-2.0-or-later
#
# Copyright (c) 2017 Allwinnertech, Inc.
#
#
MIPI_DSI_IP_VERSION=40

mydir = de/lowlevel_v21x

dedir = $(mydir)/de21x

tcondir = $(mydir)/tcon

ifeq ($(MIPI_DSI_IP_VERSION),40)
dsi_obj:=$(tcondir)/de_dsi.o
else
dsi_obj:=$(tcondir)/de_dsi_28.o
endif

obj_low += $(mydir)/disp_al_de.o $(dedir)/de_feat.o $(dedir)/de21x_feat.o\
	$(dedir)/de_top.o $(dedir)/de_rtmx.o \
	$(dedir)/de_ovl.o $(dedir)/de_bld.o \
	$(dedir)/de_csc_table.o $(dedir)/de_ccsc.o \
	$(dedir)/de_csc2.o \
	$(dedir)/de_tfbd.o $(dedir)/de_fbd_atw.o \
	$(dedir)/de_vsu.o $(dedir)/de_scaler_table.o \
	$(dedir)/de_cdc.o $(dedir)/de_cdc_table.o \
	$(dedir)/de_fmt.o \
	$(dedir)/de_enhance.o $(dedir)/de_snr.o $(dedir)/de_dci.o \
	$(dedir)/de_fcm.o $(dedir)/de_sharp.o \
	$(dedir)/de_dither.o $(dedir)/de_deband.o $(dedir)/de_vep_table.o \
	$(dedir)/de_gamma.o $(dedir)/de_dcsc.o $(dedir)/de_crc.o \
	$(dedir)/de_smbl.o $(dedir)/de_snr.o $(dedir)/de_ksc.o\
	$(dedir)/de_wb.o $(dsi_obj) \
	$(mydir)/disp_al_tcon.o $(mydir)/tcon_feat.o \
	$(tcondir)/de_lcd.o

ccflags-y += -DDE_VERSION_V21X
