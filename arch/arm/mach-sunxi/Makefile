obj-$(CONFIG_ARCH_SUNXI)	+= sunxi.o

ifeq ($(CONFIG_ARCH_SUN8IW6),y)
#obj-y		+= sun8i-cci.o
#obj-y		+= sun8i-setup.o
#obj-y		+= irq-gic-sun8iw6.o
#obj-y		+= irq-gic-common-sun8iw6.o
#obj-y		+= sun8iw6-mcpm.o
ifdef CONFIG_SUNXI_GIC_ACCESS_SS
obj-y			        += sun8iw6-gic.o
endif
else
ifneq ($(CONFIG_ARM_PSCI), y)
ifeq ($(CONFIG_ARCH_SUN8IW17),y)
obj-y		+= sun8iw17-mcpm.o
obj-y		+= sun8i-cci.o
obj-y           += sun8iw17-setup.o
else
obj-$(CONFIG_SMP)		+= headsmp.o platsmp.o
obj-$(CONFIG_HOTPLUG_CPU)	+= hotplug.o

endif

endif
endif

