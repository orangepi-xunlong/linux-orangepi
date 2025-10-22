#
# Makefile for the touchscreen drivers.
#
# Each configuration option enables a list of files.

obj-$(CONFIG_TOUCHSCREEN_GT9XXNEW_TS)          += gt9xxnew_ts.o
gt9xxnew_ts-objs := gt9xx.o
obj-$(CONFIG_TOUCHSCREEN_GT9XXNEW_UPDATE)      += gt9xx_update.o
obj-$(CONFIG_TOUCHSCREEN_GT9XXNEW_TOOL)        += goodix_tool.o
ccflags-y  += -Wno-format -Wno-unused-function
