plus_sec := $(call as-instr,.arch_extension sec,+sec)
AFLAGS_te_irq.o :=-Wa,-march=armv7-a$(plus_sec)
CFLAGS_te_comms.o :=$(call as-instr,.arch_extension sec,-DREQUIRES_SEC=1)

te_driver-objs += te_device.o
te_driver-objs += te_comms.o
te_driver-objs += te_shmem.o
te_driver-objs += te_rpc.o

obj-$(CONFIG_TRUSTED_LITTLE_KERNEL) += te_driver.o

ifeq ($(CONFIG_TRUSTED_LITTLE_KERNEL_DEBUG),y)
	EXTRA_CFLAGS  += -DDEBUG
endif
