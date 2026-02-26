#
# Makefile for Bosch sensor driver.
#

EXTRA_CFLAGS += -Wno-implicit-fallthrough

# make command to build driver as a dynamically loadable kernel object module
SRCS   = bmi323_spi.c bs_log.c bmi3.c bmi323.c bmi323_driver.c
OBJS   = $(SRCS:.c=.o)
obj-$(CONFIG_SENSORS_BMI323) += imu_bmi323_spi.o

imu_bmi323_spi-objs := bmi3.o bmi323.o bmi323_spi.o bs_log.o bmi323_driver.o

# make command to build driver as a static build module
#obj-y 	+= bmi3.o bmi323.o bmi323_spi.o bs_log.o bmi323_driver.o
