
obj-$(CONFIG_LCD_FB) += lcd_fb.o

lcd_fb-objs := dev_lcd_fb.o disp_lcd.o dev_fb.o
lcd_fb-objs += lcd_fb_intf.o disp_display.o panels/panels.o panels/lcd_source.o
lcd_fb-${CONFIG_LCD_SUPPORT_KLD35512} += panels/kld35512/kld35512.o panels/kld35512/core_api.o
lcd_fb-${CONFIG_LCD_SUPPORT_KLD39501} += panels/kld39501/kld39501.o panels/kld39501/core_api.o
lcd_fb-${CONFIG_LCD_SUPPORT_KLD2844B} += panels/kld2844b.o
