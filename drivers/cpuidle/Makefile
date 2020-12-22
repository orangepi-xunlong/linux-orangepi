#
# Makefile for cpuidle.
#

obj-y += cpuidle.o driver.o governor.o sysfs.o governors/
obj-$(CONFIG_ARCH_NEEDS_CPU_IDLE_COUPLED) += coupled.o
obj-$(CONFIG_ARM_SUNXI_CPUIDLE)          += cpuidle-sunxi.o
obj-$(CONFIG_ARM_SUN8IW6_CPUIDLE)          += cpuidle-sun8iw6.o

