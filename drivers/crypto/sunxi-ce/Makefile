obj-$(CONFIG_CRYPTO_DEV_SUN4I_SS) += sun4i-ss.o
sun4i-ss-y += sun4i/sun4i-ss-core.o sun4i/sun4i-ss-hash.o sun4i/sun4i-ss-cipher.o

obj-$(CONFIG_CRYPTO_DEV_SUNXI) += sunxi-ce.o
obj-$(CONFIG_CRYPTO_DEV_SUNXI_IOCTL) += sunxi-ce-ioctl.o

sunxi-ce-ioctl-$(CONFIG_CRYPTO_DEV_SUNXI_IOCTL) += sunxi_ce_cdev.o
sunxi-ce-$(CONFIG_CRYPTO_DEV_SUNXI) += sunxi_ce.o sunxi_ce_proc_comm.o

#ifdef CONFIG_ARCH_SUN8IW6
#	SUNXI_CE_VER = v2
#endif
ifdef CONFIG_ARCH_SUN20IW1
	SUNXI_CE_VER = v3
endif
ifdef CONFIG_ARCH_SUN8IW11
	SUNXI_CE_VER = v3
endif
ifdef CONFIG_ARCH_SUN8IW12
	SUNXI_CE_VER = v3
endif
ifdef CONFIG_ARCH_SUN8IW15
	SUNXI_CE_VER = v3
endif
ifdef CONFIG_ARCH_SUN8IW17
	SUNXI_CE_VER = v3
endif
ifdef CONFIG_ARCH_SUN8IW7
	SUNXI_CE_VER = v3
endif
ifdef CONFIG_ARCH_SUN8IW18
	SUNXI_CE_VER = v3
endif
ifdef CONFIG_ARCH_SUN50I
	SUNXI_CE_VER = v3
endif
ifdef CONFIG_ARCH_SUN8IW16
	SUNXI_CE_VER = v3
endif
ifdef CONFIG_ARCH_SUN8IW19
	SUNXI_CE_VER = v3
endif
ifdef CONFIG_ARCH_SUN8IW20
	SUNXI_CE_VER = v3
endif
ifdef CONFIG_ARCH_SUN50IW8
	SUNXI_CE_VER = v4
endif
ifdef CONFIG_ARCH_SUN50IW10
	SUNXI_CE_VER = v4
endif
ifdef CONFIG_ARCH_SUN50IW12
	SUNXI_CE_VER = v5
endif

sunxi-ce-$(CONFIG_CRYPTO_DEV_SUNXI) +=  $(SUNXI_CE_VER)/sunxi_ce_reg.o $(SUNXI_CE_VER)/sunxi_ce_proc.o
sunxi-ce-ioctl-$(CONFIG_CRYPTO_DEV_SUNXI_IOCTL) += $(SUNXI_CE_VER)/sunxi_ce_reg.o $(SUNXI_CE_VER)/sunxi_ce_cdev_comm.o

ccflags-y += -I$(srctree)/drivers/crypto/sunxi-ce/$(SUNXI_CE_VER)
#ccflags-y += -DDEBUG
