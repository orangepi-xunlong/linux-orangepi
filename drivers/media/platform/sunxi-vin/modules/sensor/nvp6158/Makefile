#ifeq ($(KERNELRELEASE),)
#export MMZ_SRC_BASE=$(PWD)
#endif

obj-y := nvp6158.o
nvp6158-objs := nvp6158c.o nvp6158_drv.o video_auto_detect.o video_eq.o  coax_protocol.o video.o motion.o audio.o

ifeq ($(HIARCH),hi3520D)
EXTRA_CFLAGS += -DHI_GPIO_I2C
EXTRA_CFLAGS += -I$(PWD)/../gpio_i2c
EXTRA_CFLAGS += -DSUPPORT_HI3520D
endif
ifeq ($(HIARCH),hi3521a)
EXTRA_CFLAGS += -DHI_CHIP_HI3521A
#EXTRA_CFLAGS += -DI2C_INTERNAL
EXTRA_CFLAGS += -DHI_I2C
EXTRA_CFLAGS += -I$(REL_INC)
EXTRA_CFLAGS += -I$(PWD)/../hi_i2c
EXTRA_CFLAGS += $(DRV_CFLAGS)
endif
ifeq ($(HIARCH),hi3531)
EXTRA_CFLAGS += -DHI_GPIO_I2C
EXTRA_CFLAGS += -I$(PWD)/../gpio_i2c_8b
endif
ifeq ($(HIARCH),hi3531a)
EXTRA_CFLAGS += -DHI_CHIP_HI3531A
EXTRA_CFLAGS += -DI2C_INTERNAL
EXTRA_CFLAGS += -I$(REL_INC)
EXTRA_CFLAGS += $(DRV_CFLAGS)
endif

# add Show/hide NVP6158(C) function
#EXTRA_CFLAGS += -D__DEC_HIDE_SHOW_FUNCTION
EXTRA_CFLAGS += -Wframe-larger-than=4096
default:
	@make -C $(LINUX_ROOT) M=$(PWD) modules
ifeq ($(HIARCH),hi3520D)
	cp *.ko /nfs/hi3520d/app/ko/extdrv/
endif
ifeq ($(HIARCH),hi3521a)
	cp *.ko /nfs/hi3521a/app/ko/extdrv/
endif
ifeq ($(HIARCH),hi3531)
	cp *.ko /nfs/root/raptor3/
endif
ifeq ($(HIARCH),hi3531a)
	cp *.ko /nfs/root/raptor3/
endif
clean:
	@make -C $(LINUX_ROOT) M=$(PWD) clean


