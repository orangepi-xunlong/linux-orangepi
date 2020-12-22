ifneq ($(strip $(CONFIG_ARCH_SUN9IW1)),)
obj-$(CONFIG_FB_SUNXI) += disp.o

disp-objs :=  dev_disp.o dev_fb.o dev_disp_attrnode.o dev_capture.o dev_composer.o\
							OSAL/OSAL_Cache.o OSAL/OSAL_Clock.o OSAL/OSAL_Dma.o OSAL/OSAL_Int.o OSAL/OSAL_IrqLock.o  OSAL/OSAL_Pwm.o OSAL/OSAL_Power.o \
							OSAL/OSAL_Lib_C.o OSAL/OSAL_Pin.o OSAL/OSAL_Semi.o OSAL/OSAL_Thread.o OSAL/OSAL_Time.o OSAL/OSAL_Parser.o\
							de/lowlevel_sun9iw1/de_be.o de/lowlevel_sun9iw1/de_fe.o de/lowlevel_sun9iw1/de_hwc.o de/lowlevel_sun9iw1/de_clock.o\
							de/lowlevel_sun9iw1/de_layer.o de/lowlevel_sun9iw1/de_lcd.o \
							de/lowlevel_sun9iw1/iep_drc_ebios.o de/lowlevel_sun9iw1/iep_drc.o de/lowlevel_sun9iw1/iep_cmu.o\
							de/lowlevel_sun9iw1/iep_deu.o de/lowlevel_sun9iw1/iep_deu_ebios.o\
							de/disp_private.o de/disp_features.o de/lowlevel_sun9iw1/disp_al.o de/disp_cursor.o de/disp_capture.o de/disp_hdmi.o\
							de/disp_lcd.o de/disp_layer.o de/disp_manager.o de/disp_smart_color.o de/disp_smart_backlight.o de/disp_display.o\
							de/lowlevel_sun9iw1/libdsi de/lowlevel_sun9iw1/libedp\

disp-objs			+= libdsi.a

quiet_cmd_libdsi = COPY    $@
      cmd_libdsi = cp -vfr $(obj)/de/lowlevel_sun9iw1/libdsi $@

$(obj)/libdsi.a: $(obj)/de/lowlevel_sun9iw1/libdsi
	$(call if_changed,libdsi)

$(obj)/de/lowlevel_sun9iw1/libdsi: FORCE
	$(if $(wildcard $(obj)/de/lowlevel_sun9iw1/display/dsi/Makefile), \
		$(Q)$(MAKE) $(build)=$(obj)/de/lowlevel_sun9iw1/display/dsi libdsi_install, \
		)

disp-objs			+= libedp.a

quiet_cmd_libedp = COPY    $@
      cmd_libedp = cp -vfr $(obj)/de/lowlevel_sun9iw1/libedp $@

$(obj)/libedp.a: $(obj)/de/lowlevel_sun9iw1/libedp
	$(call if_changed,libedp)

$(obj)/de/lowlevel_sun9iw1/libedp: FORCE
	$(if $(wildcard $(obj)/de/lowlevel_sun9iw1/display/edp/Makefile), \
		$(Q)$(MAKE) $(build)=$(obj)/de/lowlevel_sun9iw1/display/edp libedp_install, \
		)

endif

ifneq ($(strip $(CONFIG_ARCH_SUN8IW5)),)

obj-$(CONFIG_FB_SUNXI) += disp.o

disp-objs :=  dev_disp.o dev_fb.o dev_disp_attrnode.o dev_capture.o dev_composer.o\
							OSAL/OSAL_Cache.o OSAL/OSAL_Clock.o OSAL/OSAL_Dma.o OSAL/OSAL_Int.o OSAL/OSAL_IrqLock.o  OSAL/OSAL_Pwm.o OSAL/OSAL_Power.o \
							OSAL/OSAL_Lib_C.o OSAL/OSAL_Pin.o OSAL/OSAL_Semi.o OSAL/OSAL_Thread.o OSAL/OSAL_Time.o OSAL/OSAL_Parser.o\
							de/lowlevel_sun8iw5/de_be.o de/lowlevel_sun8iw5/de_fe.o de/lowlevel_sun8iw5/iep_drc_ebios.o de/lowlevel_sun8iw5/iep_wb_ebios.o\
							de/lowlevel_sun8iw5/iep_drc.o de/lowlevel_sun8iw5/de_layer.o de/lowlevel_sun8iw5/de_dsi.o de/lowlevel_sun8iw5/de_lcd.o \
							de/disp_private.o de/disp_features.o de/lowlevel_sun8iw5/disp_al.o de/lowlevel_sun8iw5/sat.o de/lowlevel_sun8iw5/de_clock.o\
							de/disp_display.o de/disp_private.o de/disp_features.o de/disp_lcd.o de/disp_layer.o de/disp_manager.o\
							de/disp_smart_backlight.o de/disp_capture.o de/disp_smart_color.o\

endif