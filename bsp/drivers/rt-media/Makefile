EXTRA_CFLAGS += -I$(PWD)/../vin/vin-video \
				-I$(PWD)/memory \
				-I$(PWD)/ven_adapter

obj-$(CONFIG_VIDEO_RT_MEDIA) += rt-media.o

rt-media-objs := rt_media.o
rt-media-objs += ven_adapter/vencoder.o
rt-media-objs += component/rt_message.o
rt-media-objs += component/rt_component.o
rt-media-objs += component/rt_venc_component.o
rt-media-objs += component/rt_vi_component.o
#rt-media-objs += memory/ion_mem/ion_mem.o
#rt-media-objs += ../cedar-ve/codec/lib/libvencoder
#rt-media-objs += ../cedar-ve/codec/lib/libbase
#rt-media-objs += ../cedar-ve/codec/lib/libve
#rt-media-objs += ../cedar-ve/codec/lib/libgcc
ifeq (${CONFIG_VIDEO_RT_MEDIA},m)
rt-media-objs += ./libmath
endif


