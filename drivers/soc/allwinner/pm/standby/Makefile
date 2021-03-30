#makefile for standby.bin
always  := standby.code
targets := standby.elf

STANDBY_XN_FILES := standby.xn

GCOV_PROFILE=no

#use "-Os" flags.
#Don't use "-O2" flags.
KBUILD_CFLAGS := -g -c -nostdlib -march=armv7-a -D__LINUX_ARM_ARCH__=7 -marm -fno-unwind-tables -fno-jump-tables -fno-asynchronous-unwind-tables -mlittle-endian -O2 -mno-unaligned-access

#Include the cur dir.
KBUILD_CPPFLAGS += -I.

LD_FLAGS = -static
LIBS =

INCLUDE   = -I. \
       -I$(KDIR)/include \
       -I$(KDIR)/drivers/soc/allwinner/standby/include \
       -I$(KDIR)/drivers/soc/allwinner/pm

standby-y := common.o                    \
             standby_twi.o               \
             power/axp_power.o           \
             power/axp15_power.o         \
             power/axp22_power.o         \
             power/axp20_power.o         \
             power/standby_power.o       \
             standby_clock.o             \
             standby_debug.o             \
             standby_divlib.o            \
             standby_dram.o            \
             ./../mem_mmu_pc_asm.o       \
             ./../mem_divlibc.o          \
             ./../mem_clk.o              \
             ./../mem_timing.o           \
             ./../mem_serial.o           \
             ./../mem_printk.o           \
             main.o

ifneq ($(strip $(CONFIG_ARCH_SUN8IW11)),)
standby-y += dram/sun8iw11p1/mctl_standby-sun8iw11.o
endif

ifneq ($(strip $(CONFIG_ARCH_SUN8IW10)),)
standby-y += dram/sun8iw10p1/mctl_standby-sun8iw10.o
endif

standby-y := $(addprefix $(obj)/,$(standby-y))

$(obj)/standby.code: $(obj)/standby.elf FORCE
	$(Q)$(CROSS_COMPILE)objcopy -O binary $(obj)/standby.elf $(obj)/standby.code
#$(call if_changed,objcopy)
	rm -rf *.o $(obj)/./*.o


$(obj)/standby.elf: $(obj)/$(STANDBY_XN_FILES) $(standby-y)
	$(Q)$(CROSS_COMPILE)ld -T $(obj)/$(STANDBY_XN_FILES) $(LD_FLAGS) $(LIBS) -EL  $(standby-y) -o $(obj)/standby.elf -Map $(obj)/standby.map
	$(Q)$(CROSS_COMPILE)objdump -D $(obj)/standby.elf > $(obj)/standby.lst

#$(call if_changed,ld)

clean-files += standby.code standby.elf
