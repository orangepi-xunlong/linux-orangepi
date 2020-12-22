#makefile for standby.bin
always  := standby.code resume1.code
targets := standby.elf resume1.elf

GCOV_PROFILE=no

#use "-Os" flags.
#Don't use "-O2" flags.
KBUILD_CFLAGS := -g -c -nostdlib -march=armv7-a -marm -fno-unwind-tables -fno-asynchronous-unwind-tables -mlittle-endian -O2 -mno-unaligned-access

#Include the cur dir.
KBUILD_CPPFLAGS += -I.

LD_FLAGS = -static 
LIBS = 

INCLUDE   = -I. \
       -I$(KDIR)/include \
       -I$(KDIR)/arch/arm/mach-sunxi/include

standby-y := 	common.o         	\
		standby_clock.o  	\
		standby_ir.o     	\
		standby_key.o    	\
		axp209_power.o  	\
		standby_power.o  	\
		standby_twi.o    	\
		standby_usb.o    	\
		standby_delay.o  	\
		./../pm_debug.o       		\
		./../mem_timing.o      		\
		./../mem_mmu_pc.o               \
		./../mem_mmu_pc_asm.o           \
		./../mem_serial.o		\
		./../mem_printk.o  		\
		./../mem_divlib.o		\
		./../mem_divlibc.o		\
		./../mem_int.o			\
                ./../mem_gpio.o                 \
		./../mem_tmr.o			\
		./../mem_tmstmp.o		\
		./../mem_clk.o			\
		standby.o

ifneq ($(strip $(CONFIG_SUNXI_ARISC)),)
standby-y +=	./arisc/standby_arisc.o 	\
		./arisc/arisc_hwmsgbox.o 	\
		./arisc/arisc_hwspinlock.o 	\
		./arisc/arisc_message_manager.o	\
		./../mem_hwspinlock.o		
else
standby-y +=    ./dram/standby.o       
endif

standby-y := $(addprefix $(obj)/,$(standby-y))

$(obj)/standby.code: $(obj)/standby.elf FORCE
	$(Q)$(CROSS_COMPILE)objcopy -O binary $(obj)/standby.elf $(obj)/standby.code
#$(call if_changed,objcopy)
	rm -rf *.o $(obj)/../*.o
	
$(obj)/standby.elf: $(obj)/standby.xn $(standby-y)
	$(Q)$(CROSS_COMPILE)ld -T $(obj)/standby.xn $(LD_FLAGS) $(LIBS) -EL  $(standby-y) -o $(obj)/standby.elf -Map $(obj)/standby.map	
	$(Q)$(CROSS_COMPILE)objdump -D $(obj)/standby.elf > $(obj)/standby.lst
#$(call if_changed,ld)
	
resume1-y :=   super/common.o                \
               super/super_twi.o             \
               super/super_delay.o           \
               super/super_clock.o           \
               super/super_power.o           \
               super/super_cpus.o            \
               super/resume/resume1.o        \
               super/resume/resume_head.o    \
               super/resume/resume1_c_part.o \
               ../pm_debug.o                 \
               ../mem_timing.o               \
               ../mem_mmu_pc.o               \
               ../mem_mmu_pc_asm.o           \
               ../mem_cpu.o                  \
               ../mem_serial.o               \
               ../mem_printk.o               \
               ../mem_divlib.o               \
               ../mem_divlibc.o              \
               ../mem_int.o                  \
               ../mem_gpio.o                 \
               ../mem_tmr.o                  \
               ../mem_tmstmp.o               \
	       ../mem_hwspinlock.o	     \
               ../mem_clk.o

ifneq ($(strip $(CONFIG_ARCH_SUN8IW6P1) $(CONFIG_ARCH_SUN9IW1P1)),)
ifneq ($(strip $(CONFIG_SUNXI_TRUSTZONE)),)
    resume1-y +=    super/resume/monitor.o
endif
else
endif

resume1-y := $(addprefix $(obj)/,$(resume1-y))

$(obj)/resume1.code: $(obj)/resume1.bin
	$(Q)$(obj)/gen_check_code $(obj)/resume1.bin $(obj)/super/resume/resume1.code
	rm -rf *.o $(obj)/../*.o
	
$(obj)/resume1.bin: $(obj)/resume1.elf FORCE
	$(Q)$(CROSS_COMPILE)objcopy -O binary $(obj)/resume1.elf $(obj)/resume1.bin
	

ifneq ($(strip $(CONFIG_ARCH_SUN9I)),)
$(obj)/resume1.elf: $(obj)/sun9i_resume1_scatter.scat $(resume1-y)
	$(Q)$(CROSS_COMPILE)ld -T $(obj)/sun9i_resume1_scatter.scat  -EL  $(resume1-y) -o $(obj)/resume1.elf  -Map $(obj)/resume1.map
	$(Q)$(CROSS_COMPILE)objdump -D $(obj)/resume1.elf > $(obj)/resume1.lst
#FIXME: still donot know why command: $(call if_changed,ld) not work well? the output file size is not proper.
endif

ifneq ($(strip $(CONFIG_ARCH_SUN8I)),)
$(obj)/resume1.elf: $(obj)/sun8i_resume1_scatter.scat $(resume1-y)
	$(Q)$(CROSS_COMPILE)ld -T $(obj)/sun8i_resume1_scatter.scat  -EL  $(resume1-y) -o $(obj)/resume1.elf  -Map $(obj)/resume1.map
	$(Q)$(CROSS_COMPILE)objdump -D $(obj)/resume1.elf > $(obj)/resume1.lst
#FIXME: still donot know why command: $(call if_changed,ld) not work well? the output file size is not proper.
endif

$(resume1-y): $(obj)/standby.code

clean-files += standby.code standby.elf
clean-files += resume1.code resume1.elf
