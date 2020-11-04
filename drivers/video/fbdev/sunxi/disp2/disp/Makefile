include $(obj)/de/Makefile

# define_trace.h needs to know how to find our header
CFLAGS_disp_trace.o := -I$(src)

obj-$(CONFIG_DISP2_SUNXI) += disp.o

disp-objs :=  dev_disp.o dev_fb.o dev_disp_debugfs.o
disp-objs += disp_sys_intf.o
disp-objs += disp_trace.o
disp-$(CONFIG_DISP2_SUNXI_COMPOSER) += dev_composer.o
disp-objs += de/disp_display.o de/disp_features.o de/disp_device.o \
	     de/disp_lcd.o de/disp_manager.o de/disp_private.o \
	     de/disp_smart_backlight.o de/disp_enhance.o de/disp_capture.o \
	     de/disp_hdmi.o de/disp_tv.o de/disp_vdevice.o \
	     de/disp_vga.o \
	     de/disp_vdpo.o de/disp_edp.o de/disp_rtwb.o

disp-$(CONFIG_EINK_PANEL_USED) += de/disp_eink_manager.o \
		de/eink_buffer_manager.o de/eink_pipeline_manager.o \
		de/disp_format_convert.o lcd/default_eink.o

disp-objs += lcd/panels.o lcd/lcd_source.o lcd/default_panel.o \
		lcd/lt070me05000.o lcd/wtq05027d01.o lcd/t27p06.o \
		lcd/dx0960be40a1.o lcd/tft720x1280.o lcd/S6D7AA0X01.o \
		lcd/cpu_gg1p4062utsw.o lcd/ls029b3sx02.o \
		lcd/he0801a068.o lcd/inet_dsi_panel.o lcd/lq101r1sx03.o  \
		lcd/WilliamLcd.o lcd/fx070.o lcd/FX070_DHM18BOEL2_1024X600.o \
		lcd/bp101wx1-206.o
disp-objs += $(obj_low)
