

VC_DRV_HEADER := -I$(src)									\
				-I$(src)/common								\
				-I$(src)/common/rcKernel					\
				-I$(src)/common/rcKernel/float_point	\
				-I$(src)/module/include						\
				-I$(src)/include

COMMON_DRV_HEADER := -I$(srctree)/drivers/osdrv/interdrv/v2/include/common/uapi 			\
					-I$(srctree)/drivers/osdrv/interdrv/v2/include/chip/$(CVIARCH_L)/uapi/ \
					-I$(srctree)/drivers/osdrv/interdrv/v2/include/common/kapi/ \
					-I$(srctree)/drivers/osdrv/interdrv/v2/base/

JPU_INC := -I$(src)/cnm_driver/jpeg/jpuapi \
			-I$(src)/cnm_driver/jpeg/jdi \
			-I$(src)/cnm_driver/jpeg/sample \
			-I$(src)/cnm_driver/jpeg/sample/helper \
			-I$(src)/cnm_driver/jpu/bmjpuapi/inc \
			-I$(src)/cnm_driver/jpu \
			-I$(src)/cnm_driver/jpu/jpuapi \
			-I$(src)/cnm_driver/jpu/jdi \
			-I$(src)/cnm_driver/jpu/include


VPU_INC := -I$(src)/cnm_driver/vpu			\
				-I$(src)/cnm_driver/vpu/vdi			\
				-I$(src)/cnm_driver/vpu/vdi/linux/driver \
				-I$(src)/cnm_driver/vpu/sample_v2/helper	\
				-I$(src)/cnm_driver/vpu/sample_v2/helper/misc	\
				-I$(src)/cnm_driver/vpu/sample_v2/component	\
				-I$(src)/cnm_driver/vpu/sample_v2/component_decoder \
				-I$(src)/cnm_driver/vpu/sample_v2/component_encoder \
				-I$(src)/cnm_driver/vpu/vpuapi		\
				-I$(src)/cnm_driver/vpu/vpuapi/coda9\
				-I$(src)/cnm_driver/vpu/vpuapi/wave \
				-I$(src)/cnm_driver/vpu/cvi \
				-I$(src)/cnm_driver/vpu/fw/wave_521 \
				-I$(src)/cnm_driver/vpu/fw/wave_517


MODULE_SOURCES = vc_drv.c 	\
		module/src/venc.c 	\
		module/src/vdec.c	\
		module/src/enc_ctx.c	\
		module/src/venc_api.c \
		module/src/vdec_api.c \
		module/src/jpeg_api.c \
		module/src/venc_help.c \
		module/src/vc_drv_proc.c \
		module/src/venc_rc.c \
		common/rcKernel/float_point/float_point.c \
		common/rcKernel/float_point/soft_float.c \
		common/drv_file.c \
		common/vc_getopt.c \
		common/datastructure.c


VPU_SOURCES = cnm_driver/vpu/vdi/linux/vdi.c \
		cnm_driver/vpu/vdi/linux/vdi_osal.c \
		cnm_driver/vpu/vdi/linux/driver/vpu.c \
		cnm_driver/vpu/vdi/vdi_debug.c \
		cnm_driver/vpu/vdi/vdi_debug.c \
		cnm_driver/vpu/vpuapi/vpuapi.c \
		cnm_driver/vpu/vpuapi/vpuapifunc.c \
		cnm_driver/vpu/vpuapi/product.c \
		cnm_driver/vpu/vpuapi/wave/wave5.c \
		cnm_driver/vpu/vpuapi/wave/wave6.c \
		cnm_driver/vpu/vpuapi/coda9/coda9.c \
		cnm_driver/vpu/sample_v2/helper/main_helper.c \
		cnm_driver/vpu/sample_v2/helper/misc/pbu.c \
		cnm_driver/vpu/sample_v2/helper/misc/debug.c


JPU_SOURCES =	cnm_driver/jpeg/jpuapi/jpuapi.c 	\
				cnm_driver/jpeg/jpuapi/jpuapifunc.c	\
				cnm_driver/jpeg/jdi/mm.c			\
				cnm_driver/jpeg/jdi/linux/jdi.c		\
				cnm_driver/jpeg/jdi/linux/driver/jpu_core_res.c	\
				cnm_driver/jpeg/jdi/linux/driver/jpeg.c	\
				cnm_driver/jpeg/sample/helper/jpulog.c	\
				cnm_driver/jpeg/sample/helper/jpuhelper.c


MODULE_OBJS=$(patsubst %.c,%.o,$(MODULE_SOURCES))
VPU_OBJS=$(patsubst %.c,%.o,$(VPU_SOURCES))
JPU_OBJS=$(patsubst %.c,%.o,$(JPU_SOURCES))

VC_DRV_HEADER += $(COMMON_DRV_HEADER)
VC_DRV_HEADER += $(JPU_INC)
VC_DRV_HEADER += $(VPU_INC)

KBUILD_EXTRA_SYMBOLS += $(src)/../base/Module.symvers
KBUILD_EXTRA_SYMBOLS += $(src)/../sys/Module.symvers

ccflags-y += $(VC_DRV_HEADER) -DUSE_KERNEL_MODE -DKERNEL_MODE -DENABLE_DEC -DMJPEG_INTERFACE_API -DCNM_WAVE_SERIES

#ccflags-y += -DRV_TEST -mno-outline-atomics

ccflags-y += -DRV_TEST

ifneq ($(KERNELRELEASE),)
# call from kernel build system


$(CVIARCH_L)_vc_drv-objs :=	$(MODULE_OBJS)
$(CVIARCH_L)_vc_drv-objs +=	$(VPU_OBJS)
$(CVIARCH_L)_vc_drv-objs +=	$(JPU_OBJS)
obj-m	:= $(CVIARCH_L)_vc_drv.o

else

# ifeq ($(KERNELDIR), )
#   $(error "kernel header path is empty, please input it!")
# endif

src	  := $(shell pwd)

all:
	$(MAKE)	ARCH=$(ARCH) -C $(KERNEL_DIR) M=$(src) modules

endif



clean:
	rm -rf *.o *~ core .depend *.mod *.order *.symvers .*.cmd *.ko
	rm -rf *.mod.c .tmp_versions *.o *.o.d module/src/*.o*
	rm -rf base/.*.o.cmd*
	rm -rf common/rcKernel/float_point/*.o*
	rm -rf common/rcKernel/float_point/*.o*
	rm -rf common/rcKernel/float_point/.*.o.cmd
	rm -rf common/rcKernel/*.o*
	rm -rf common/*.o*
	rm -rf common/.*.o.cmd*
	rm -rf module/src/.*.o.cmd
	rm -rf common/rcKernel/.*.o.cmd
	rm -rf cnm_driver/vpu/cvi/.*.o.cmd*
	rm -rf cnm_driver/vpu/vdi/.*.o.cmd*
	rm -rf cnm_driver/vpu/vdi/linux/driver/*.ko
	rm -rf cnm_driver/vpu/vdi/linux/driver/*.o*
	rm -rf cnm_driver/vpu/vdi/linux/driver/.*.o*
	rm -rf cnm_driver/vpu/vdi/linux/driver/.*.cmd*
	rm -rf cnm_driver/vpu/vdi/linux/.*.o.cmd*
	rm -rf cnm_driver/vpu/vpuapi/coda9/.*.o.cmd*
	rm -rf cnm_driver/vpu/vpuapi/wave/.*.o.cmd*
	rm -rf cnm_driver/vpu/vpuapi/.*.o.cmd*
	rm -rf cnm_driver/vpu/encoder/src/.*.o.cmd*
	rm -rf cnm_driver/vpu/sample_v2/.*.o.cmd*
	rm -rf cnm_driver/vpu/sample_v2/helper/misc/.*.o.cmd*
	rm -rf cnm_driver/vpu/sample_v2/helper/.*.o.cmd*
	rm -rf cnm_driver/jpeg/jpuapi/.*.o.cmd
	rm -rf cnm_driver/jpeg/jdi/.*.o.cmd
	rm -rf cnm_driver/jpeg/jdi/linux/.*.o.cmd
	rm -rf cnm_driver/jpeg/jdi/linux/driver/.*.cmd*
	rm -rf cnm_driver/jpeg/jdi/linux/driver/*.ko
	rm -rf cnm_driver/jpeg/jdi/linux/driver/*.o*
	rm -rf cnm_driver/jpeg/jdi/linux/driver/.*.o*
	rm -rf cnm_driver/jpeg/sample/.*.o.cmd
	rm -rf cnm_driver/jpeg/sample/helper/.*.o.cmd
	rm -rf .*.o.d
	rm -rf $(MODULE_OBJS) $(VPU_OBJS) $(JPU_OBJS)

depend .depend dep:
	$(CC) $(CFLAGS)	-M *.c > .depend


ifeq (.depend,$(wildcard .depend))
include	.depend
endif
