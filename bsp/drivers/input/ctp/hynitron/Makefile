
# Linux driver folder

#ccflags-y += -I$(srctree)/drivers/input/touchscreen/mediatek/
#ccflags-y += -I$(srctree)/drivers/input/touchscreen/mediatek/hynitron
#ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat/
#ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat/$(MTK_PLATFORM)/include/
#ccflags-y += -I$(srctree)/drivers/misc/mediatek/sensors-1.0/hwmon/include/

obj-$(CONFIG_TOUCHSCREEN_HYNITRON_TS)	+= hynitron.o
hynitron-objs := hynitron_core.o \
		hynitron_i2c.o \
		hynitron_esd_check.o \
		hynitron_update_firmware.o \
		hynitron_gesture.o \
		hynitron_proximity.o
