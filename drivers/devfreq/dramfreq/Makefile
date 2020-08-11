ifneq ($(CONFIG_ARCH_SUN9IW1P1)$(CONFIG_ARCH_SUN8IW5P1)$(CONFIG_ARCH_SUN8IW6P1)$(CONFIG_ARCH_SUN8IW7P1)$(CONFIG_ARCH_SUN8IW8P1)$(CONFIG_ARCH_SUN8IW9P1),)
CFLAGS_REMOVE_sunxi-ddrfreq.o := -pg
obj-y	 += sunxi-ddrfreq.o
ifdef CONFIG_ARCH_SUN9IW1P1
obj-y	 += mdfs_asm.o
endif
ifneq ($(CONFIG_ARCH_SUN9IW1P1)$(CONFIG_ARCH_SUN8IW7P1)$(CONFIG_DEVFREQ_DRAM_FREQ_LOW_POWER_SW),)
CFLAGS_REMOVE_sunxi-mdfs.o := -funwind-tables -pg -g
CFLAGS_sunxi-mdfs.o := -O0
obj-y	 += sunxi-mdfs.o
endif
ifneq ($(CONFIG_ARCH_SUN8IW6P1)$(CONFIG_DEVFREQ_DRAM_FREQ_CFS_SW),)
CFLAGS_REMOVE_sunxi-mdfs.o := -funwind-tables -pg -g
CFLAGS_sunxi-mdfs.o := -O0
obj-y	 += sunxi-mdfs.o
endif
endif
