#include "rk628_twi_helper.h"

int read_rk628(struct v4l2_subdev *sd, unsigned int addr, unsigned int *value)
{
	__maybe_unused struct sensor_info *info = to_state(sd);
	unsigned char data[8];
	struct i2c_msg msg[2];
	int ret;
	struct i2c_client *client = v4l2_get_subdevdata(sd);

#if defined RK628_REG_ENDIAN
	/* big endian */
	data[0] = (addr & 0xff000000) >> 24;
	data[1] = (addr & 0x00ff0000) >> 16;
	data[2] = (addr & 0x0000ff00) >> 8;
	data[3] = (addr & 0x000000ff);
#else
	/* little endian */
	data[0] = (addr & 0x000000ff);
	data[1] = (addr & 0x0000ff00) >> 8;
	data[2] = (addr & 0x00ff0000) >> 16;
	data[3] = (addr & 0xff000000) >> 24;
#endif
	data[4] = 0xee;
	data[5] = 0xee;
	data[6] = 0xee;
	data[7] = 0xee;

	/*
	 * Send out the register address...
	 */
	msg[0].addr = client->addr;
	msg[0].flags = 0;
	msg[0].len = 4;
	msg[0].buf = &data[0];

	/*
	 * ...then read back the result.
	 */
	msg[1].addr = client->addr;

	msg[1].flags = I2C_M_RD;
	msg[1].len = 4;
	msg[1].buf = &data[4];

	ret = i2c_transfer(client->adapter, msg, 2);
	if (ret >= 0) {
#if RK628_REG_ENDIAN_BIG
		/* big endian */
		*value = data[4] * 0x1000000 + data[5] * 0x10000 + data[6] * 0x100 + data[7];
#else
		/* little endian */
		*value = data[7] * 0x1000000 + data[6] * 0x10000 + data[5] * 0x100 + data[4];
#endif
		ret = 0;
	}
	return ret;
}

int write_rk628(struct v4l2_subdev *sd, unsigned int addr, unsigned int value)
{
	__maybe_unused struct sensor_info *info = to_state(sd);
	struct i2c_msg msg;
	unsigned char data[8];
	int ret;
	struct i2c_client *client = v4l2_get_subdevdata(sd);

#if defined RK628_REG_ENDIAN
	/* big endian */
	data[0] = (addr & 0xff000000) >> 24;
	data[1] = (addr & 0x00ff0000) >> 16;
	data[2] = (addr & 0x0000ff00) >> 8;
	data[3] = (addr & 0x000000ff);
	data[4] = (value & 0xff000000) >> 24;
	data[5] = (value & 0x00ff0000) >> 16;
	data[6] = (value & 0x0000ff00) >> 8;
	data[7] = (value & 0x000000ff);
#else
	/* little endian */
	data[0] = (addr & 0x000000ff);
	data[1] = (addr & 0x0000ff00) >> 8;
	data[2] = (addr & 0x00ff0000) >> 16;
	data[3] = (addr & 0xff000000) >> 24;
	data[4] = (value & 0x000000ff);
	data[5] = (value & 0x0000ff00) >> 8;
	data[6] = (value & 0x00ff0000) >> 16;
	data[7] = (value & 0xff000000) >> 24;
#endif

	msg.addr = client->addr;
	msg.flags = 0;
	msg.len = 8;
	msg.buf = data;

	ret = i2c_transfer(client->adapter, &msg, 1);
	if (ret >= 0)
		ret = 0;
	return ret;
}
