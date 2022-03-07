ccflags-y += -I$(srctree)/drivers/net/wireless/uwe5622/sprdwcn/include

ifneq ($(UNISOC_BSP_INCLUDE),)
ccflags-y += -I$(UNISOC_BSP_INCLUDE)
endif

sprdbt_tty-objs := \
	tty.o \
	rfkill.o \
	woble.o\
	alignment/sitm.o

obj-$(CONFIG_TTY_OVERY_SDIO) += sprdbt_tty.o

