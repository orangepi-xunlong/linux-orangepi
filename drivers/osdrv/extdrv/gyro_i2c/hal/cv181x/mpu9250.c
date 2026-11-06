/**
 * @file mpu9250.cpp
 * @brief This source file contains methods for initialising MPU9250.
 */
#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/iommu.h>

#include "mpu9250.h"

struct i2c_client *g_client;
struct file *g_fp;

struct i2c_client *cvi_gy_open(char *node)
{
	g_fp = filp_open(node, O_RDWR, 0);
	if (IS_ERR(g_fp)) {
		pr_err("open %s error/n", node);
		return NULL;
	}
	g_client = (struct i2c_client *)g_fp->private_data;
	g_client->addr = MPU_address;
	return g_client;
}

void cvi_gy_close(void)
{
	filp_close(g_fp, NULL);
}

void cvi_gy_reset(struct i2c_client *client)
{
	mpu9250_write_OR(client, PWR_MGMT_1, 0x80);
	mdelay(10); // Wait for all registers to reset
	mpu9250_write_OR(client, PWR_MGMT_1, 0x01);
	mpu9250_write_OR(client, PWR_MGMT_2, 0x00);
	mdelay(10);
}

void set_gyro_offset(enum axis selected_axis, int16_t offset)
{
	//read factory gyroscope offset
	int16_t GX_offset = ((int16_t)mpu9250_read(g_client, XG_OFFSET_H) << 8) | mpu9250_read(g_client, XG_OFFSET_L);
	int16_t GY_offset = ((int16_t)mpu9250_read(g_client, YG_OFFSET_H) << 8) | mpu9250_read(g_client, YG_OFFSET_L);
	int16_t GZ_offset = ((int16_t)mpu9250_read(g_client, ZG_OFFSET_H) << 8) | mpu9250_read(g_client, ZG_OFFSET_L);

	switch (selected_axis) {
	case X_axis:
		offset = offset + GX_offset;//add offset to the factory offset
		mpu9250_write(g_client, XG_OFFSET_L, (offset & 0xFF));//write low byte
		mpu9250_write(g_client, XG_OFFSET_H, (offset>>8));//write high byte
		break;

	case Y_axis:
		offset = offset + GY_offset;
		mpu9250_write(g_client, YG_OFFSET_L, (offset & 0xFF));
		mpu9250_write(g_client, YG_OFFSET_H, (offset>>8));
		break;

	case Z_axis:
		offset = offset + GZ_offset;
		mpu9250_write(g_client, ZG_OFFSET_L, (offset & 0xFF));
		mpu9250_write(g_client, ZG_OFFSET_H, (offset>>8));
		break;
	}
}

void get_acc_offset(void)
{
	//read the register values and save them as a 16 bit value
	int16_t AX_offset = (mpu9250_read(g_client, XA_OFFSET_H)<<8) | (mpu9250_read(g_client, XA_OFFSET_L));
	int16_t AY_offset = (mpu9250_read(g_client, YA_OFFSET_H)<<8) | (mpu9250_read(g_client, YA_OFFSET_L));
	int16_t AZ_offset = (mpu9250_read(g_client, ZA_OFFSET_H)<<8) | (mpu9250_read(g_client, ZA_OFFSET_L));
	AX_offset = AX_offset>>1;
	AY_offset = AY_offset>>1;
	AZ_offset = AZ_offset>>1;
	//pr_err("read offset: %d %d %d\n", AX_offset, AY_offset, AZ_offset);
}

void set_acc_offset(enum axis selected_axis, int16_t offset)
{
	//read the register values and save them as a 16 bit value
	int16_t AX_offset = (mpu9250_read(g_client, XA_OFFSET_H)<<8) | (mpu9250_read(g_client, XA_OFFSET_L));
	int16_t AY_offset = (mpu9250_read(g_client, YA_OFFSET_H)<<8) | (mpu9250_read(g_client, YA_OFFSET_L));
	int16_t AZ_offset = (mpu9250_read(g_client, ZA_OFFSET_H)<<8) | (mpu9250_read(g_client, ZA_OFFSET_L));

	AX_offset = AX_offset>>1;
	AY_offset = AY_offset>>1;
	AZ_offset = AZ_offset>>1;
	//pr_err("offset: %d %d %d\n", AX_offset, AY_offset, AZ_offset);
	switch (selected_axis) {
	case X_axis:
		offset = offset + AX_offset;//add offset to the factory offset
		if (abs(offset) < 16384) {
			mpu9250_write(g_client, XA_OFFSET_L, (offset & 0xFF)<<1);//write low byte
			mpu9250_write(g_client, XA_OFFSET_H, (offset>>7));//write high byte
		}
		break;

	case Y_axis:
		offset = offset + AY_offset;
		if (abs(offset) < 16384) {
			mpu9250_write(g_client, YA_OFFSET_L, (offset & 0xFF)<<1);
			mpu9250_write(g_client, YA_OFFSET_H, (offset>>7));
		}
		break;

	case Z_axis:
		offset = offset + AZ_offset;
		if (abs(offset) < 16384) {
			mpu9250_write(g_client, ZA_OFFSET_L, (offset & 0xFF)<<1);
			mpu9250_write(g_client, ZA_OFFSET_H, (offset>>7));
		}
		break;
	}
	get_acc_offset();
}

void set_acc_bandwidth(enum acc_bandwidth selected_bandwidth)
{
	switch (selected_bandwidth) {
	case acc_1113Hz:
		mpu9250_write_OR(g_client, ACCEL_CONFIG_2, (1<<3));//set accel_fchoice_b to 1
		break;

	case acc_460Hz:
		//set accel_fchoice_b to 0 and  A_DLPF_CFG to 0(000)
		mpu9250_write_AND(g_client, ACCEL_CONFIG_2, ~((1<<3)|(1<<2)|(1<<1)|(1<<0)));
		break;

	case acc_184Hz:
		mpu9250_write_AND(g_client, ACCEL_CONFIG_2, ~(1<<3));
		//set A_DLPF_CFG to 1(001)
		mpu9250_write_AND(g_client, ACCEL_CONFIG_2, ~((1<<1)|(1<<2)));
		mpu9250_write_OR(g_client, ACCEL_CONFIG_2, (1<<0));
		break;

	case acc_92Hz:
		mpu9250_write_AND(g_client, ACCEL_CONFIG_2, ~(1<<3));
		//set A_DLPF_CFG to 2(010)
		mpu9250_write_AND(g_client, ACCEL_CONFIG_2, ~((1<<0)|(1<<2)));
		mpu9250_write_OR(g_client, ACCEL_CONFIG_2, (1<<1));
		break;

	case acc_41Hz:
		mpu9250_write_AND(g_client, ACCEL_CONFIG_2, ~(1<<3));
		//set A_DLPF_CFG to 3(011)
		mpu9250_write_AND(g_client, ACCEL_CONFIG_2, ~(1<<2));
		mpu9250_write_OR(g_client, ACCEL_CONFIG_2, (1<<0)|(1<<1));
		break;

	case acc_20Hz:
		mpu9250_write_AND(g_client, ACCEL_CONFIG_2, ~(1<<3));
		//set A_DLPF_CFG to 4(100)
		mpu9250_write_AND(g_client, ACCEL_CONFIG_2, ~((1<<0)|(1<<1)));
		mpu9250_write_OR(g_client, ACCEL_CONFIG_2, (1<<2));
		break;

	case acc_10Hz:
		mpu9250_write_AND(g_client, ACCEL_CONFIG_2, ~(1<<3));
		//set A_DLPF_CFG to 5(101)
		mpu9250_write_AND(g_client, ACCEL_CONFIG_2, ~(1<<1));
		mpu9250_write_OR(g_client, ACCEL_CONFIG_2, (1<<0)|(1<<2));
		break;

	case acc_5Hz:
		mpu9250_write_AND(g_client, ACCEL_CONFIG_2, ~(1<<3));
		//set A_DLPF_CFG to 6(110)
		mpu9250_write_AND(g_client, ACCEL_CONFIG_2, ~(1<<0));
		mpu9250_write_OR(g_client, ACCEL_CONFIG_2, (1<<1)|(1<<2));
		break;
	}
}

void set_gyro_bandwidth(enum gyro_bandwidth selected_bandwidth)
{
	switch (selected_bandwidth) {
	case gyro_8800Hz:
		mpu9250_write_OR(g_client, GYRO_CONFIG, (1<<0));//set Fchoice_b <0> to 1
		break;

	case gyro_3600Hz:
		mpu9250_write_AND(g_client, GYRO_CONFIG, ~(1<<0));
		mpu9250_write_OR(g_client, GYRO_CONFIG, (1<<1));
		break;

	case gyro_250Hz:
		mpu9250_write_AND(g_client, GYRO_CONFIG, ~((1<<0)|(1<<1)));
		//write 0(000) to DLPF_CFG
		mpu9250_write_AND(g_client, CONFIG, ~((1<<0)|(1<<1)|(1<<2)));
		break;

	case gyro_184Hz:
		mpu9250_write_AND(g_client, GYRO_CONFIG, ~((1<<0)|(1<<1)));
		//write 1(001) to DLPF_CFG
		mpu9250_write_AND(g_client, CONFIG, ~((1<<1)|(1<<2)));
		mpu9250_write_OR(g_client, CONFIG, (1<<0));
		break;

	case gyro_92Hz:
		mpu9250_write_AND(g_client, GYRO_CONFIG, ~((1<<0)|(1<<1)));
		//write 2(010) to DLPF_CFG
		mpu9250_write_AND(g_client, CONFIG, ~((1<<2)|(1<<0)));
		mpu9250_write_OR(g_client, CONFIG, (1<<1));
		break;

	case gyro_41Hz:
		mpu9250_write_AND(g_client, GYRO_CONFIG, ~((1<<0)|(1<<1)));
		//write 3(011) to DLPF_CFG
		mpu9250_write_AND(g_client, CONFIG, ~(1<<2));
		mpu9250_write_OR(g_client, CONFIG, (1<<0)|(1<<1));
		break;

	case gyro_20Hz:
		mpu9250_write_AND(g_client, GYRO_CONFIG, ~((1<<0)|(1<<1)));
		//write 4(100) to DLPF_CFG
		mpu9250_write_AND(g_client, CONFIG, ~((1<<1)|(1<<0)));
		mpu9250_write_OR(g_client, CONFIG, (1<<2));
		break;

	case gyro_10Hz:
		mpu9250_write_AND(g_client, GYRO_CONFIG, ~((1<<0)|(1<<1)));
		//write 5(101) to DLPF_CFG
		mpu9250_write_AND(g_client, CONFIG, ~(1<<1));
		mpu9250_write_OR(g_client, CONFIG, (1<<2)|(1<<0));
		break;

	case gyro_5Hz:
		mpu9250_write_AND(g_client, GYRO_CONFIG, ~((1<<0)|(1<<1)));
		//write 6(110) to DLPF_CFG
		mpu9250_write_AND(g_client, CONFIG, ~(1<<0));
		mpu9250_write_OR(g_client, CONFIG, (1<<1)|(1<<2));
		break;
	}
}

uint8_t get_gyro_scale(uint8_t current_state, enum scales selected_scale)
{
	if (selected_scale == scale_2g || selected_scale == scale_250dps) {
		current_state &= ~((1<<3)|(1<<4));
	}

	if (selected_scale == scale_4g || selected_scale == scale_500dps) {
		current_state &= ~(1<<4);
		current_state |= (1<<3);
	}

	if (selected_scale == scale_8g || selected_scale == scale_1000dps) {
		current_state &= ~(1<<3);
		current_state |= (1<<4);
	}

	if (selected_scale == scale_16g || selected_scale == scale_2000dps) {
		current_state |= (1<<4)|(1<<3);
	}

	return current_state;
}

void set_acc_scale(enum scales selected_scale)
{
	uint8_t val = mpu9250_read(g_client, ACCEL_CONFIG);

	val = get_gyro_scale(val, selected_scale);
	mpu9250_write(g_client, ACCEL_CONFIG, val);
}

void set_gyro_scale(enum scales selected_scale)
{
	uint8_t val = mpu9250_read(g_client, GYRO_CONFIG);

	val = get_gyro_scale(val, selected_scale);
	mpu9250_write(g_client, GYRO_CONFIG, val);
}

void read_acc(struct sAxis *a)
{
	//read data
	uint8_t rawData[6];

	mpu9250_readArray(g_client, ACCEL_XOUT_H, rawData, 6);

	//store data in raw data variables
	a->x = ((int16_t)rawData[0] << 8) | rawData[1];
	a->y = ((int16_t)rawData[2] << 8) | rawData[3];
	a->z = ((int16_t)rawData[4] << 8) | rawData[5];

	get_acc_offset();
}

void read_gyro(struct sAxis *a)
{
	uint8_t rawData[6];

	mpu9250_readArray(g_client, GYRO_XOUT_H, rawData, 6);

	a->x = ((int16_t)rawData[0] << 8) | rawData[1];
	a->y = ((int16_t)rawData[2] << 8) | rawData[3];
	a->z = ((int16_t)rawData[4] << 8) | rawData[5];
}

void read_mag(struct sAxis *a)
{
	uint8_t rawData[6];

	mpu9250_readArray(g_client, GYRO_XOUT_H, rawData, 6);

	a->x = ((int16_t)rawData[0] << 8) | rawData[1];
	a->y = ((int16_t)rawData[2] << 8) | rawData[3];
	a->z = ((int16_t)rawData[4] << 8) | rawData[5];
}

int16_t read_temp(void)
{
	uint8_t rawData[2];

	mpu9250_readArray(g_client, TEMP_OUT_H, rawData, 2);

	return ((int16_t)rawData[0] << 8) | rawData[1];
}

void cvi_gy_adj_gyro_offset(enum acc_bandwidth acc_bw, enum gyro_bandwidth gyro_bw)
{
	//gyroscope
	bool update_gX = true;
	bool update_gY = true;
	bool update_gZ = true;

	int16_t gX_offset = 0;
	int16_t gY_offset = 0;
	int16_t gZ_offset = 0;

	struct sAxis gyro_axis;

	int count = 0;
	const short maximum_error = 5;//set maximum deviation to 5 LSB

	set_acc_bandwidth(acc_bw);
	set_gyro_bandwidth(gyro_bw);
	read_gyro(&gyro_axis);

	while (count < 20) {
		count++;
		read_gyro(&gyro_axis);

		if (gyro_axis.x != 0 && update_gX == true) {
			gX_offset = -1*(gyro_axis.x/4);
		}

		if (gyro_axis.y != 0 && update_gY == true) {
			gY_offset = -1*(gyro_axis.y/4);
		}

		if (gyro_axis.z != 0 && update_gZ == true) {
			gZ_offset = -1*(gyro_axis.z/4);
		}
		//pr_err("GX: %d (Offset: %d), GY: %d (Offset: %d), GZ: %d (Offset: %d)\n",
		// gyro_axis.x, gX_offset, gyro_axis.y, gY_offset, gyro_axis.z, gZ_offset);

		if (update_gX == true) {
			set_gyro_offset(X_axis, gX_offset);
		}

		if (update_gY == true) {
			set_gyro_offset(Y_axis, gY_offset);
		}

		if (update_gZ == true) {
			set_gyro_offset(Z_axis, gZ_offset);
		}

		if (abs(gyro_axis.x) <= maximum_error) {
			update_gX = false;
		}

		if (abs(gyro_axis.y) <= maximum_error) {
			update_gY = false;
		}

		if (abs(gyro_axis.z) <= maximum_error) {
			update_gZ = false;
		}

		if (update_gX == false && update_gY == false && update_gZ == false) {
			break;
		}
		mdelay(10);
	}
}

void cvi_gy_adj_acc_offset(enum acc_bandwidth acc_bw, enum gyro_bandwidth gyro_bw)
{
	//accelerometer
	bool update_aX = true;
	bool update_aY = true;
	bool update_aZ = true;

	int16_t aX_offset = 0;
	int16_t aY_offset = 0;
	int16_t aZ_offset = 0;

	struct sAxis acc_axis;

	int count = 0;
	const short maximum_error = 10;//set maximum deviation to 5 LSB

	set_acc_bandwidth(acc_bw);
	set_gyro_bandwidth(gyro_bw);

	while (count < 10) {
		count++;
		read_acc(&acc_axis);

		if (acc_axis.x != 0 && update_aX == true) {
			aX_offset = -1*(acc_axis.x/10);
		}

		if (acc_axis.y != 0 && update_aY == true) {
			aY_offset = -1*(acc_axis.y/10);
		}

		if (acc_axis.z != 0 && update_aZ == true) {
			aZ_offset = -1*(acc_axis.z/10);
		}

		//pr_err("AX: %d (Offset: %d), AY: %d (Offset: %d), AZ: %d (Offset: %d)\n",
		// acc_axis.x, aX_offset, acc_axis.y, aY_offset, acc_axis.z, aZ_offset);

		if (update_aX == true) {
			set_acc_offset(X_axis, aX_offset);
		}

		if (update_aY == true) {
			set_acc_offset(Y_axis, aY_offset);
		}

		if (update_aZ == true) {
			set_acc_offset(Z_axis, aZ_offset);
		}

		if (abs(acc_axis.x) <= maximum_error) {
			update_aX = false;
		}

		if (abs(acc_axis.y) <= maximum_error) {
			update_aY = false;
		}

		if (abs(acc_axis.z) <= maximum_error) {
			update_aZ = false;
		}

		if (update_aX == false && update_aY == false && update_aZ == false) {
			break;
		}
		mdelay(10);
	}
}
