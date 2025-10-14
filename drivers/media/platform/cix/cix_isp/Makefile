ccflags-y += -I$(srctree)/drivers/media/platform/cix/cix_isp/platform
ccflags-y += -I$(srctree)/drivers/media/platform/cix/cix_isp/common
ccflags-y += -I$(srctree)/drivers/media/platform/cix/cix_isp/common/isp_hw_if
ccflags-y += -I$(srctree)/drivers/media/platform/cix/cix_isp/sensor
ccflags-y += -I$(srctree)/drivers/media/platform/cix/cix_isp/isp
ccflags-y += -I$(srctree)/drivers/media/platform/cix/cix_isp/platform/logger
ccflags-y += -I$(srctree)/drivers/media/platform/cix/cix_isp/platform/hdmi
ccflags-y += -I$(srctree)/drivers/media/platform/cix/cix_isp/cixvihw
ccflags-y += -I$(srctree)/drivers/misc/

ccflags-y += -DARMCB_CAM_KO
ccflags-y += -DARMCB_CAM_AHB

obj-$(CONFIG_CAMERA_ARMCB) += armcb_isp.o
armcb_isp-objs = sensor/armcb_sensor.o \
					common/armcb_camera_io_drv.o \
					common/system_dma.o \
					common/armcb_v4l_sd.o \
					platform/armcb_platform.o \
					isp/armcb_vb2.o \
					isp/armcb_v4l2_stream.o \
					isp/armcb_isp_driver.o \
					isp/armcb_v4l2_config.o \
					isp/armcb_v4l2_core.o \
					sensor/actuator/armcb_actuator.o \
					common/isp_hw_if/isp_hw_ops.o \
					common/isp_hw_if/isp_hw_utils.o \
					platform/logger/system_logger.o \
					cixvihw/cix_vi_hw.o \
					armcb_isp_entry.o

clean:
	find . -name '*.o' -type f -print -exec rm -rf {} \;
	find . -name '*.mod.c' -type f -print -exec rm -rf {} \;
	find . -name '*.ko' -type f -print -exec rm -rf {} \;
	find . -name '*.symvers' -type f -print -exec rm -rf {} \;
	find . -name '*.order' -type f -print -exec rm -rf {} \;
	find . -name '*.a' -type f -print -exec rm -rf {} \;
	find . -name '*.mod' -type f -print -exec rm -rf {} \;
	find . -name '*.cmd' -type f -print -exec rm -rf {} \;
	find . -name '*.o.d' -type f -print -exec rm -rf {} \;
