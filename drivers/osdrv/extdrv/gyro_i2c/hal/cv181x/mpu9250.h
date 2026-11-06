/**
 * @file mpu9250.h
 * @brief Main header of the gyro mpu9250 library.
 */

#ifndef MPU9250_H
#define MPU9250_H

#include <linux/i2c.h>
#include "mpu9250_reg.h"

///modules (for enable / disable / reset functions)
enum modules {
	Acc_X,//accelerometer X axis
	Acc_Y,//accelerometer Y axis
	Acc_Z,//accelerometer Z axis
	Gyro_X,//gyroscope X axis
	Gyro_Y,//gyroscope Y axis
	Gyro_Z,//gyroscope Z axis
	magnetometer,//magnetometer
	accelerometer,//accelerometer
	gyroscope,//gyroscope
	thermometer,//thermometer
	signalPaths,//all signal paths
};

//axis
enum axis {
	X_axis,
	Y_axis,
	Z_axis,
};

//available scales
enum scales {
	scale_2g,//+-2g
	scale_4g,//+-4g
	scale_8g,//+-8g
	scale_16g,//+-16g
	scale_250dps,//+-250 degrees per second
	scale_500dps,//+- 500 degrees per second
	scale_1000dps,//+- 1000 degrees per second
	scale_2000dps,//+- 2000 degrees per second
};

	//bandwidth
enum acc_bandwidth {
	acc_1113Hz,
	acc_460Hz,
	acc_184Hz,
	acc_92Hz,
	acc_41Hz,
	acc_20Hz,
	acc_10Hz,
	acc_5Hz,
};

enum gyro_bandwidth {
	gyro_8800Hz,
	gyro_3600Hz,
	gyro_250Hz,
	gyro_184Hz,
	gyro_92Hz,
	gyro_41Hz,
	gyro_20Hz,
	gyro_10Hz,
	gyro_5Hz,
};

  //registers map
enum registers {
	MPU_address       = 0x68,//main chip

	//main chip
	USER_CTRL         = 0x6A,
	PWR_MGMT_1        = 0x6B,
	PWR_MGMT_2        = 0x6C,
	SIGNAL_PATH_RESET = 0x68,
	INT_PIN_CFG       = 0x37,
	ST1               = 0x02,
	ACCEL_CONFIG      = 0x1C,
	ACCEL_CONFIG_2    = 0x1D,
	MOT_DETECT_CTRL   = 0x69,
	WOM_THR           = 0x1F,
	GYRO_CONFIG       = 0x1B,
	CONFIG            = 0x1A,
	SMPLRT_DIV        = 0x19,
	INT_ENABLE        = 0x38,
	INT_STATUS        = 0x3A,
	WHO_AM_I          = 0x75,

	//gyroscope offset
	XG_OFFSET_H       = 0x13,
	XG_OFFSET_L       = 0x14,
	YG_OFFSET_H       = 0x15,
	YG_OFFSET_L       = 0x16,
	ZG_OFFSET_H       = 0x17,
	ZG_OFFSET_L       = 0x18,

	//accelerometer offset
	XA_OFFSET_H       = 0x77,
	XA_OFFSET_L       = 0x78,
	YA_OFFSET_H       = 0x7A,
	YA_OFFSET_L       = 0x7B,
	ZA_OFFSET_H       = 0x7D,
	ZA_OFFSET_L       = 0x7E,

	//magnetometer
	MAG_ID            = 0x00,
	CNTL              = 0x0A,
	CNTL2             = 0x0B,
	ASAX              = 0x10,
	ASAY              = 0x11,
	ASAZ              = 0x12,

	/// data registers
	MAG_XOUT_L        = 0x03,//magnetometer
	GYRO_XOUT_H       = 0x43,//gyro
	ACCEL_XOUT_H      = 0x3B,//accelerometer
	TEMP_OUT_H        = 0x41,//thermometer

};

struct sAxis {
	int16_t x;
	int16_t y;
	int16_t z;
};

void cvi_gy_reset(struct i2c_client *client);
struct i2c_client *cvi_gy_open(char *node);
void cvi_gy_close(void);

void cvi_gy_adj_gyro_offset(enum acc_bandwidth acc, enum gyro_bandwidth gyro);
void cvi_gy_adj_acc_offset(enum acc_bandwidth acc, enum gyro_bandwidth gyro);

void set_gyro_offset(enum axis selected_axis, int16_t offset);
void set_gyro_bandwidth(enum gyro_bandwidth selected_bandwidth);
void set_gyro_scale(enum scales selected_scale);
uint8_t get_gyro_scale(uint8_t current_state, enum scales selected_scale);

void set_acc_offset(enum axis selected_axis, int16_t offset);
void set_acc_bandwidth(enum acc_bandwidth selected_bandwidth);
void set_acc_scale(enum scales selected_scale);
void set_gyro_scale(enum scales selected_scale);

void read_acc(struct sAxis *a);
void read_gyro(struct sAxis *a);
void read_mag(struct sAxis *a);
int16_t read_temp(void);
#endif //MPU9250_H
