#use "-Os" flags.
use-flags := -Os
#Don't use "-O2" flags.
not-use-flags := -O2
KBUILD_CFLAGS := $(filter-out $(not-use-flags), $(KBUILD_CFLAGS))
KBUILD_CFLAGS += $(use-flags)
GCOV_PROFILE := n

subdir- += standby
obj-y	 += pm_tmp.o 

pm_tmp-y :=	mem_mapping.o               \
		mem_divlibc.o               \
		mem_cpu.o                   \
		mem_cpu_asm.o               \
		mem_mmu_pc_asm.o            \
		mem_mmu_pc.o                \
		mem_int.o                   \
		mem_clk.o                   \
		mem_serial.o                   \
		mem_key.o                   \
		mem_ir.o                   \
		mem_usb.o                   \
		mem_tmr.o                   \
		mem_tmstmp.o                \
		mem_twi.o                   \
		mem_gpio.o                  \
		mem_sram.o                  \
		mem_ccu.o                   \
		mem_cci400.o                \
		mem_gtbus.o                 \
		pm_debug.o                  \
		pm_debug_secure.o           \
		mem_timing.o                \
		mem_hwspinlock.o              \
		pm.o                        \
		standby/super/super_power.o \
		standby/super/super_twi.o   \
		standby/super/super_clock.o \
		standby.o		    \
		mem.o			    \
		aw_pm.o			    \
		aw_pwr_dm.o                 

ifneq ($(strip $(CONFIG_ARCH_SUN8IW6) $(CONFIG_ARCH_SUN8IW8)),)
pm_tmp-y +=	extended_standby-sun8iw6.o
else
pm_tmp-y +=	extended_standby.o
endif

$(obj)/mem_mapping.o              : $(obj)/pm_tmp_xx.o
$(obj)/mem_divlibc.o              : $(obj)/pm_tmp_xx.o
$(obj)/mem_cpu.o                  : $(obj)/pm_tmp_xx.o
$(obj)/mem_cpu_asm.o              : $(obj)/pm_tmp_xx.o
$(obj)/mem_mmu_pc_asm.o           : $(obj)/pm_tmp_xx.o
$(obj)/mem_mmu_pc.o               : $(obj)/pm_tmp_xx.o
$(obj)/mem_int.o                  : $(obj)/pm_tmp_xx.o
$(obj)/mem_clk.o                  : $(obj)/pm_tmp_xx.o
$(obj)/mem_tmr.o                  : $(obj)/pm_tmp_xx.o
$(obj)/mem_tmstmp.o               : $(obj)/pm_tmp_xx.o
$(obj)/mem_twi.o                  : $(obj)/pm_tmp_xx.o
$(obj)/mem_gpio.o                 : $(obj)/pm_tmp_xx.o
$(obj)/mem_serial.o               : $(obj)/pm_tmp_xx.o
$(obj)/mem_key.o 	          : $(obj)/pm_tmp_xx.o
$(obj)/mem_ir.o 	          : $(obj)/pm_tmp_xx.o
$(obj)/mem_usb.o 	          : $(obj)/pm_tmp_xx.o
$(obj)/mem_sram.o                 : $(obj)/pm_tmp_xx.o
$(obj)/mem_ccu.o                  : $(obj)/pm_tmp_xx.o
$(obj)/mem_cci400.o               : $(obj)/pm_tmp_xx.o
$(obj)/mem_gtbus.o                : $(obj)/pm_tmp_xx.o
$(obj)/pm_debug.o                 : $(obj)/pm_tmp_xx.o
$(obj)/pm_debug_secure.o          : $(obj)/pm_tmp_xx.o
$(obj)/mem_timing.o               : $(obj)/pm_tmp_xx.o
$(obj)/mem_hwspinlock.o           : $(obj)/pm_tmp_xx.o
$(obj)/pm.o                       : $(obj)/pm_tmp_xx.o
$(obj)/aw_pm.o                    : $(obj)/pm_tmp_xx.o
$(obj)/aw_pwr_dm.o                : $(obj)/pm_tmp_xx.o
$(obj)/extended_standby.o         : $(obj)/pm_tmp_xx.o
$(obj)/extended_standby-sun8iw6.o : $(obj)/pm_tmp_xx.o
$(obj)/standby/super/super_power.o: $(obj)/pm_tmp_xx.o
$(obj)/standby/super/super_twi.o  : $(obj)/pm_tmp_xx.o
$(obj)/standby/super/super_clock.o: $(obj)/pm_tmp_xx.o
$(obj)/mem.o			  : $(obj)/pm_tmp_xx.o	

$(obj)/pm_tmp_xx.o			: $(obj)/standby/super/resume/resume1.code $(obj)/standby/standby.code FORCE
$(obj)/standby.o			: $(obj)/standby/standby.code FORCE
$(obj)/standby/super/resume/resume1.code: $(obj)/standby/standby.code FORCE

$(obj)/standby/standby.code: FORCE
	$(Q)$(MAKE) $(build)=$(obj)/standby
	
