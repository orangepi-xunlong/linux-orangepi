#
# Makefile for Invensense inv-mpu-iio device.
#

#obj-$(CONFIG_INV_MPU_IIO) += inv-mpu-iio.o

#inv-mpu-iio-objs += inv_mpu_ring.o
#inv-mpu-iio-objs += inv_mpu_trigger.o
#inv-mpu-iio-objs += inv_mpu_misc.o
#inv-mpu-iio-objs += inv_mpu3050_iio.o
#inv-mpu-iio-objs += dmpDefaultMPU6050.o

#CFLAGS_inv_mpu_core.o      += -Idrivers/staging/iio
#CFLAGS_inv_mpu_ring.o      += -Idrivers/staging/iio
#CFLAGS_inv_mpu_trigger.o   += -Idrivers/staging/iio
#CFLAGS_inv_mpu_misc.o      += -Idrivers/staging/iio
#CFLAGS_inv_mpu3050_iio.o   += -Idrivers/staging/iio
#CFLAGS_dmpDefaultMPU6050.o += -Idrivers/staging/iio

# the Bosch BMA250 driver is added to the inv-mpu device driver because it
# must be connected to an MPU3050 device on the secondary slave bus.

#obj-m += mpu6500.o
obj-$(CONFIG_INV_MPU6500_IIO) += mpu6500.o
mpu6500-objs := inv_mpu_core.o inv_mpu_ring.o inv_mpu_trigger.o inv_mpu_misc.o inv_mpu3050_iio.o dmpDefaultMPU6050.o inv_slave_bma250.o



