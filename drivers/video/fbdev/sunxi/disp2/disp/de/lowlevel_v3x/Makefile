#
# Copyright (c) 2015 Allwinnertech, Inc.
#
#

mydir = de/lowlevel_v3x

ifeq ($(MIPI_DSI_IP_VERSION),40)
dsi_obj:=$(mydir)/de_dsi.o
else
dsi_obj:=$(mydir)/de_dsi_28.o
endif

obj_low = $(mydir)/de_fcc.o $(mydir)/de_gsu.o $(mydir)/de_hal.o $(mydir)/disp_al.o \
		$(mydir)/de_peak.o $(mydir)/de_rtmx.o $(mydir)/de_scaler_table.o\
		$(mydir)/de_vep_table.o $(mydir)/de_vsu.o $(mydir)/de_feat.o \
		$(mydir)/de_lcd.o $(mydir)/de_lti.o $(mydir)/de_fce.o \
		$(mydir)/de_bls.o $(mydir)/de_dns.o $(mydir)/de_enhance.o \
		$(mydir)/de_smbl.o $(mydir)/de_ccsc.o $(mydir)/de_peak2d.o \
		$(mydir)/de_wb.o $(mydir)/de_clock.o $(dsi_obj) \
		$(mydir)/de_atw.o $(mydir)/de_fmt.o $(mydir)/de_cdc_table.o
