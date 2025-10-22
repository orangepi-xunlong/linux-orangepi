# SPDX-License-Identifier: GPL-2.0

# Common objects
ccu-sunxi-ng-y					+= ccu_common.o
ccu-sunxi-ng-y					+= ccu_reset.o

# trace objects
CFLAGS_ccu-sunxi-trace.o			:= -I$(src)
ccu-sunxi-ng-y					+= ccu-sunxi-trace.o

# Base clock types
ccu-sunxi-ng-y					+= ccu_div.o
ccu-sunxi-ng-y					+= ccu_frac.o
ccu-sunxi-ng-y					+= ccu_gate.o
ccu-sunxi-ng-y					+= ccu_mux.o
ccu-sunxi-ng-y					+= ccu_mult.o
ccu-sunxi-ng-y					+= ccu_phase.o
ccu-sunxi-ng-y					+= ccu_sdm.o

# Multi-factor clocks
ccu-sunxi-ng-y					+= ccu_nk.o
ccu-sunxi-ng-y					+= ccu_nkm.o
ccu-sunxi-ng-y					+= ccu_nkmp.o
ccu-sunxi-ng-y					+= ccu_nm.o
ccu-sunxi-ng-y					+= ccu_mp.o

obj-$(CONFIG_AW_CCU)				+= ccu-sunxi-ng.o
obj-$(CONFIG_AW_RTC_CCU)			+= ccu-sunxi-rtc.o
obj-$(CONFIG_AW_DDR_CCU)			+= ccu-ddr.o

# SoC support
obj-$(CONFIG_AW_SUN8IW11_CCU)		+= ccu-sun8iw11.o
obj-$(CONFIG_AW_SUN8IW18_CCU)		+= ccu-sun8iw18.o
obj-$(CONFIG_AW_SUN8IW18_CCU)		+= ccu-sun8iw18-rtc.o
obj-$(CONFIG_AW_SUN8IW20_CCU)		+= ccu-sun8iw20.o
obj-$(CONFIG_AW_SUN8IW20_CCU)		+= ccu-sun8iw20-rtc.o
obj-$(CONFIG_AW_SUN8IW20_R_CCU)		+= ccu-sun8iw20-r.o
obj-$(CONFIG_AW_SUN8IW20_TEST)		+= ccu-sun8iw20-test.o
obj-$(CONFIG_AW_SUN50IW12_CCU)		+= ccu-sun50iw12.o
obj-$(CONFIG_AW_SUN50IW12_R_CCU)	+= ccu-sun50iw12-r.o
obj-$(CONFIG_AW_SUN50IW10_CCU)		+= ccu-sun50iw10.o
obj-$(CONFIG_AW_SUN50IW10_R_CCU)	+= ccu-sun50iw10-r.o
obj-$(CONFIG_AW_SUN50IW10_DISPLL)	+= ccu-displl.o
obj-$(CONFIG_AW_SUN50IW9_CCU)		+= ccu-sun50iw9.o
obj-$(CONFIG_AW_SUN50IW9_CCU)		+= ccu-sun50iw9-rtc.o
obj-$(CONFIG_AW_SUN50IW9_R_CCU)		+= ccu-sun50iw9-r.o

obj-$(CONFIG_AW_SUN55IW3_CCU)		+= ccu-sun55iw3.o
obj-$(CONFIG_AW_SUN55IW3_CCU)		+= ccu-sun55iw3-rtc.o
obj-$(CONFIG_AW_SUN55IW3_CCU)		+= ccu-sun55iw3-displl.o
obj-$(CONFIG_AW_SUN55IW3_R_CCU)		+= ccu-sun55iw3-r.o
obj-$(CONFIG_AW_SUN55IW3_MCU_CCU)	+= ccu-sun55iw3-mcu.o
obj-$(CONFIG_AW_SUN55IW3_TEST)		+= ccu-sun55iw3-test.o

obj-$(CONFIG_AW_SUN60IW1_CCU)		+= ccu-sun60iw1.o
obj-$(CONFIG_AW_SUN60IW1_CCU)		+= ccu-sun60iw1-rtc.o
obj-$(CONFIG_AW_SUN60IW1_DSP_CCU)	+= ccu-sun60iw1-dsp.o
obj-$(CONFIG_AW_SUN60IW1_R_CCU)		+= ccu-sun60iw1-r.o

obj-$(CONFIG_AW_SUN60IW2_CCU)		+= ccu-sun60iw2.o
obj-$(CONFIG_AW_SUN60IW2_CCU)		+= ccu-sun60iw2-cpupll.o
obj-$(CONFIG_AW_SUN60IW2_CCU)		+= ccu-sun60iw2-rtc.o
obj-$(CONFIG_AW_SUN60IW2_R_CCU)		+= ccu-sun60iw2-r.o

obj-$(CONFIG_AW_SUN55IW5_CCU)		+= ccu-sun55iw5.o
obj-$(CONFIG_AW_SUN55IW5_R_CCU)		+= ccu-sun55iw5-r.o
obj-$(CONFIG_AW_SUN55IW5_CCU)		+= ccu-sun55iw5-rtc.o

obj-$(CONFIG_AW_SUN55IW6_CCU)		+= ccu-sun55iw6.o
obj-$(CONFIG_AW_SUN55IW6_CCU)		+= ccu-sun55iw6-rtc.o
obj-$(CONFIG_AW_SUN55IW6_R_CCU)		+= ccu-sun55iw6-r.o

obj-$(CONFIG_AW_SUN8IW21_CCU)		+= ccu-sun8iw21.o
obj-$(CONFIG_AW_SUN8IW21_CCU)		+= ccu-sun8iw21-rtc.o
obj-$(CONFIG_AW_SUN8IW21_R_CCU)		+= ccu-sun8iw21-r.o

obj-$(CONFIG_AW_SUN20IW5_APP_CCU)	+= ccu-sun20iw5-app.o
obj-$(CONFIG_AW_SUN20IW5_AON_CCU)	+= ccu-sun20iw5-aon.o

obj-$(CONFIG_AW_SUN65IW1_CCU)		+= ccu-sun65iw1.o
obj-$(CONFIG_AW_SUN65IW1_CCU)		+= ccu-sun65iw1-rtc.o
obj-$(CONFIG_AW_SUN65IW1_R_CCU)		+= ccu-sun65iw1-r.o
