export CVIARCH_L := soph
export CHIP_ARCH_L := sophon
INTERDRV_PATH := interdrv/v2

#$(info =================CVIARCH_L: $(CVIARCH_L), CHIP_ARCH_L: $(CHIP_ARCH_L))
#ifeq ($(KERNEL_DIR), )
#$(info Please set KERNEL_DIR global variable!!)
#endif
obj-y += interdrv/v2/base/
obj-y += interdrv/v2/sys/
obj-y += interdrv/v2/pwm/
#obj-y += interdrv/v2/mon/
obj-y += interdrv/v2/clock_cooling/
obj-y += interdrv/v2/saradc/
#obj-y += interdrv/v2/keyscan/
#obj-y += interdrv/v2/irrx/
#obj-y += interdrv/v2/wiegand/
#obj-y += extdrv/wiegand-gpio/
obj-y += interdrv/v2/vc_drv/
obj-y += interdrv/v2/rtc/
obj-y += interdrv/v2/vi/
obj-y += interdrv/v2/snsr_i2c/
obj-y += interdrv/v2/cif/
obj-y += interdrv/v2/vpss/
obj-y += interdrv/v2/ldc/
#obj-y += interdrv/v2/vo/
#obj-y += interdrv/v2/vo/mipi_tx/
#obj-y += interdrv/v2/rgn/
obj-y += interdrv/v2/ive/
obj-y += interdrv/v2/2d_engine/
obj-y += interdrv/v2/dpu/
obj-y += interdrv/v2/stitch/
obj-y += interdrv/v2/spacc/
#obj-y += interdrv/v2/hdmi/
#obj-y += interdrv/v2/fb/
#obj-y += extdrv/wireless/


#KO_LIST = base sys pwm mon clock_cooling saradc keyscan irrx wiegand wiegand-gpio vc_drv rtc
#
#KO_LIST += vi snsr_i2c cif vpss ldc vo mipi_tx rgn ive 2d_engine dpu stitch spacc
#FB_DEP = vpss vo
#KO_LIST += hdmi
#
#KO_LIST += fb
#
#KO_LIST += wireless
#OTHERS += cp_ext_wireless
