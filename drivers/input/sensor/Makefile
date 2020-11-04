#
# Makefile for the sensor:wq drivers.
#

# Each configuration option enables a list of files.

obj-$(CONFIG_SENSORS_BMA250)    += bma250.o
obj-$(CONFIG_SENSORS_SC7A30)   += sc7a30.o
obj-$(CONFIG_SENSORS_MMA7660)   += mma7660.o
obj-$(CONFIG_SENSORS_MXC622X)   += mxc622x.o
obj-$(CONFIG_SENSORS_MMA8452)   += mma8452.o
obj-$(CONFIG_SENSORS_MMA865X)   += mma865x.o
obj-$(CONFIG_SENSORS_GPADC)   += sunxi_gpadc.o
obj-$(CONFIG_SENSORS_TPADC)   += sunxi_tpadc.o
obj-$(CONFIG_SENSORS_GPADC_TEST)   += sunxi_gpadc_test.o
obj-$(CONFIG_SENSORS_MIR3DA)    += mir3da.o
mir3da-objs:= mir3da_cust.o mir3da_core.o
obj-y   += bma253/
obj-$(CONFIG_SENSORS_MC32X0)    += mc32x0.o
obj-$(CONFIG_SENSORS_SC7660)    += sc7660.o
