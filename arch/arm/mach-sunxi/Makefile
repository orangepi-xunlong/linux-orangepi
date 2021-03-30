obj-$(CONFIG_ARCH_SUN8I) += sun8i.o
obj-$(CONFIG_ARCH_SUN50I) += sun50i.o

ifeq ($(CONFIG_SMP), y)
ifneq ($(CONFIG_ARM_PSCI), y)
obj-y += headsmp.o platsmp.o
obj-$(CONFIG_HOTPLUG_CPU) += hotplug.o
endif
endif
