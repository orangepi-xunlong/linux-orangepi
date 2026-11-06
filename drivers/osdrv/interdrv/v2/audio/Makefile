-include $(BUILD_PATH)/.config
include $(PWD)/../Makefile.interdrv.param

#PWD	:= $(shell pwd)
#include $(PWD)/audio.mk

#$(ARCH_L)_audio-objs += ./src/audio_interface.o

#$(ARCH_L)_audio-objs += /common/audio_core.o
#$(ARCH_L)_audio-objs += /src/audio_interface.o

$(ARCH_L)_audio_core-objs :=	common/audio_core.o				\
			src/audio_interface_dev.o			\
			src/pcm.o

audio_driver_header := -I$(PWD)/		\
				-I$(PWD)/include/	\
				-I$(PWD)/common/	\
				-I$(PWD)/src/		\
				-I$(srctree)/drivers/staging/android/	\
				-I$(PWD)/../include/common/uapi/linux/	\
				-I$(PWD)/../include/common/kapi/	\
				-I$(PWD)/../include/chip/$(ARCH_L)/uapi/	\
				-I$(PWD)/../include/common/uapi/	\
				-I$(PWD)/../sys/common/uapi/	\
				-I$(PWD)/../base/	\
				-I$(PWD)/../rtos_cmdqu/ \
				-I$(PWD)/../include/chip/soph/uapi/


ccflags-y += $(PWD) -I$(LDDINCDIR) $(audio_driver_header) -DAUDIO_DRV -Wno-date-time
ifeq ($(CONFIG_SDK_VER), "32bit")
ccflags-y += -DCONFIG_SDK_VER_32BIT
endif

ccflags-y +=-Wall -Wextra -Werror -Wno-unused-parameter -Wno-sign-compare

#PWD	:= $(shell pwd)
obj-m = $(ARCH_L)_audio_core.o
$(ARCH_L)_audio_core-y = $($(ARCH_L)_audio_core-objs)
KBUILD_EXTRA_SYMBOLS = $(PWD)/Module.symvers
KBUILD_EXTRA_SYMBOLS += $(PWD)/../sys/Module.symvers
KBUILD_EXTRA_SYMBOLS += $(PWD)/../base/Module.symvers
KBUILD_EXTRA_SYMBOLS += $(PWD)/../rtos_cmdqu/Module.symvers

all:
	rm -rf *.o *~ core .depend .*.cmd *.ko *.mod.c *.mod
	rm -rf .tmp_versions Module.symvers
	rm -rf modules.order
	$(MAKE) ARCH=$(ARCH) -C $(KERNEL_DIR) M=$(PWD) modules
clean:
	rm -rf *.o *~ core .depend .*.cmd *.ko *.mod.c *.mod
	rm -rf .tmp_versions Module.symvers
	rm -rf modules.order
	@rm -f ./common/*.o  ./common/*.mod.c ./common/*.cmd  ./common/*.o ./common/*.ko
	@rm -rf .tmp_versions
	@find . -name \*.o* -type f -delete
depend .depend dep:
	$(CC) $(CFLAGS)	-M *.c > .depend

ifeq (.depend,$(wildcard .depend))
include	.depend
endif

