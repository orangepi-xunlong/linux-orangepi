# generic pinmux support

ccflags-$(CONFIG_DEBUG_PINCTRL)	+= -DDEBUG

obj-$(CONFIG_PINCTRL)		+= core.o
obj-$(CONFIG_PINMUX)		+= pinmux.o
obj-$(CONFIG_PINCONF)		+= pinconf.o
obj-$(CONFIG_GENERIC_PINCONF)	+= pinconf-generic.o
obj-$(CONFIG_PINCTRL_PXA3xx)	+= pinctrl-pxa3xx.o
obj-$(CONFIG_PINCTRL_MMP2)	+= pinctrl-mmp2.o
obj-$(CONFIG_PINCTRL_PXA168)	+= pinctrl-pxa168.o
obj-$(CONFIG_PINCTRL_PXA910)	+= pinctrl-pxa910.o
obj-$(CONFIG_PINCTRL_SIRF)	+= pinctrl-sirf.o
obj-$(CONFIG_PINCTRL_SUNXI)	+= pinctrl-sunxi.o
obj-$(CONFIG_ARCH_SUN8IW1)	+= pinctrl-sun8iw1.o
obj-$(CONFIG_ARCH_SUN8IW3)	+= pinctrl-sun8iw3.o
obj-$(CONFIG_ARCH_SUN8IW5)	+= pinctrl-sun8iw5.o
obj-$(CONFIG_ARCH_SUN8IW6)	+= pinctrl-sun8iw6.o
obj-$(CONFIG_ARCH_SUN8IW7)	+= pinctrl-sun8iw7.o
obj-$(CONFIG_ARCH_SUN8IW8)	+= pinctrl-sun8iw8.o
obj-$(CONFIG_ARCH_SUN8IW9)	+= pinctrl-sun8iw9.o
obj-$(CONFIG_ARCH_SUN9IW1)	+= pinctrl-sun9iw1.o
obj-$(CONFIG_SUNXI_PINCTRL_TEST)     += sunxi_pinctrl_test.o
obj-$(CONFIG_PINCTRL_AXP)       += pinctrl-axp.o
obj-$(CONFIG_PINCTRL_AXP_TEST)  += pinctrl-axp-test.o
obj-$(CONFIG_PINCTRL_TEGRA)	+= pinctrl-tegra.o
obj-$(CONFIG_PINCTRL_TEGRA20)	+= pinctrl-tegra20.o
obj-$(CONFIG_PINCTRL_TEGRA30)	+= pinctrl-tegra30.o
obj-$(CONFIG_PINCTRL_U300)	+= pinctrl-u300.o
obj-$(CONFIG_PINCTRL_COH901)	+= pinctrl-coh901.o
ifeq ($(CONFIG_PINCTRL_SUNXI_DEBUG),y)
	EXTRA_CFLAGS  += -DDEBUG
endif
