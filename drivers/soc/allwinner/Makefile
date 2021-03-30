ifneq ($(strip $(CONFIG_ARCH_SUN50I) $(CONFIG_ARCH_SUN8IW10) $(CONFIG_ARCH_SUN8IW11)),)
obj-$(CONFIG_PM) += pm/
endif

obj-$(CONFIG_SUNXI_SRAM) += sunxi_sram.o

obj-$(CONFIG_ARCH_SUNXI) += sunxi-sid.o

obj-$(CONFIG_SUNXI_BOOTUP_EXTEND) += sunxi_bootup_extend.o
#ccflags-y += -DDEBUG
