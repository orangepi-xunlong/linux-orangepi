#
# Makefile for sunxi specific clk
#

obj-y += clk-sunxi.o clk-factors.o clk-periph.o
obj-${CONFIG_COMMON_CLK_DEBUG} += clk-debugfs.o
obj-${CONFIG_ARCH_SUN50IW3} += clk-sun50iw3.o
obj-${CONFIG_ARCH_SUN50IW1} += clk-sun50iw1.o
obj-${CONFIG_ARCH_SUN50IW6} += clk-sun50iw6.o
obj-${CONFIG_ARCH_SUN50IW8} += clk-sun50iw8.o
obj-${CONFIG_ARCH_SUN50IW9} += clk-sun50iw9.o
ifeq ($(CONFIG_ARCH_SUN50IW9),y)
obj-${CONFIG_ARCH_SUN50IW9} += clk-cpu.o
endif
obj-${CONFIG_ARCH_SUN50IW10} += clk-sun50iw10.o
obj-${CONFIG_ARCH_SUN50IW5T} += clk-sun50iw5t.o
obj-${CONFIG_ARCH_SUN8IW6} += clk-sun8iw6.o
ifeq ($(CONFIG_SUNXI_ARISC), y)
obj-${CONFIG_ARCH_SUN8IW6} += clk-ac100.o
endif
obj-${CONFIG_ARCH_SUN8IW7} += clk-sun8iw7.o
obj-${CONFIG_ARCH_SUN8IW11} += clk-sun8iw11.o
obj-${CONFIG_ARCH_SUN8IW12} += clk-sun8iw12.o
obj-${CONFIG_ARCH_SUN8IW15} += clk-sun8iw15.o
obj-${CONFIG_ARCH_SUN8IW17} += clk-sun8iw17.o
obj-${CONFIG_ARCH_SUN8IW18} += clk-sun8iw18.o
