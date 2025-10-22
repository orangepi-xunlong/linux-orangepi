/**
 * @section LICENSE
 * Copyright (c) 2022 Bosch Sensortec GmbH All Rights Reserved
 *
 * This software program is licensed subject to the GNU General
 * Public License (GPL).Version 2,June 1991,
 * available at http://www.fsf.org/copyleft/gpl.html
 *
 * @filename bmi323_i2c.c
 * @date	 10/10/2022
 * @version  1.6.0
 *
 * @brief    BMI3 I2C bus Driver
 */

/*********************************************************************/
/* system header files */
/*********************************************************************/
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/types.h>
#include <linux/input.h>

/*********************************************************************/
/* own header files */
/*********************************************************************/
#include "bmi323_driver.h"
#include "bs_log.h"

/*********************************************************************/
/* global variables */
/*********************************************************************/
static struct i2c_client *bmi3_i2c_client;

/**
 * bmi3_i2c_read - The I2C read function.
 *
 * @client : Instance of the I2C client
 * @reg_addr : The register address from where the data is read.
 * @data : The pointer to buffer to return data.
 * @len : The number of bytes to be read
 *
 * Return : Status of the function.
 * * 0 - OK
 * * negative value - Error.
 */
static s8 bmi3_i2c_read(struct i2c_client *client,
			  u8 reg_addr, u8 *data, u16 len)
{
	s32 retry;

	struct i2c_msg msg[] = {
		{
		.addr = client->addr,
		.flags = 0,
		.len = 1,
		.buf = &reg_addr,
		},

		{
		.addr = client->addr,
		.flags = I2C_M_RD,
		.len = len,
		.buf = data,
		},
	};
	for (retry = 0; retry < BMI3_MAX_RETRY_I2C_XFER; retry++) {
		if (i2c_transfer(client->adapter, msg, ARRAY_SIZE(msg)) > 0)
			break;
		usleep_range(BMI3_I2C_WRITE_DELAY_TIME * 1000,
				BMI3_I2C_WRITE_DELAY_TIME * 1000);
	}

	if (retry >= BMI3_MAX_RETRY_I2C_XFER) {
		PERR("I2C xfer error");
		return -EIO;
	}

	return 0;
}

/**
 * bmi3_i2c_write - The I2C write function.
 *
 * @client : Instance of the I2C client
 * @reg_addr : The register address to start writing the data.
 * @data : The pointer to buffer holding data to be written.
 * @len : The number of bytes to write.
 *
 * Return : Status of the function.
 * * 0 - OK
 * * negative value - Error.
 */
static s8 bmi3_i2c_write(struct i2c_client *client,
			   u8 reg_addr, const u8 *data, u16 len)
{
	s32 retry;

	struct i2c_msg msg = {
		.addr = client->addr,
		.flags = 0,
		.len = len + 1,
		.buf = NULL,
	};
	msg.buf = kmalloc(len + 1, GFP_KERNEL);
	if (!msg.buf) {
		PERR("Allocate memory failed\n");
		return -ENOMEM;
	}
	msg.buf[0] = reg_addr;
	memcpy(&msg.buf[1], data, len);
	for (retry = 0; retry < BMI3_MAX_RETRY_I2C_XFER; retry++) {
		if (i2c_transfer(client->adapter, &msg, 1) > 0)
			break;
		usleep_range(BMI3_I2C_WRITE_DELAY_TIME * 1000,
				BMI3_I2C_WRITE_DELAY_TIME * 1000);
	}
	kfree(msg.buf);
	if (retry >= BMI3_MAX_RETRY_I2C_XFER) {
		PERR("I2C xfer error");
		return -EIO;
	}

	return 0;
}

/**
 * bmi3_i2c_read_wrapper - The I2C read function pointer used by BMI3 API.
 *
 * @dev_addr : I2c Device address
 * @reg_addr : The register address to read the data.
 * @data : The pointer to buffer to return data.
 * @len : The number of bytes to be read
 *
 * Return : Status of the function.
 * * 0 - OK
 * * negative value - Error.
 */
static s8 bmi3_i2c_read_wrapper(u8 reg_addr, u8 *data,
				u32 len, void *intf_ptr)
{
	s8 err;

	err = bmi3_i2c_read(bmi3_i2c_client, reg_addr, data, len);
	return err;
}

/**
 * bmi3_i2c_write_wrapper - The I2C write function pointer used by BMI3 API.
 *
 * @dev_addr : I2c Device address
 * @reg_addr : The register address to start writing the data.
 * @data : The pointer to buffer which holds the data to be written.
 * @len : The number of bytes to be written.
 *
 * Return : Status of the function.
 * * 0 - OK
 * * negative value - Error.
 */
static s8 bmi3_i2c_write_wrapper(u8 reg_addr, const u8 *data,
				u32 len, void *intf_ptr)
{
	s8 err;

	err = bmi3_i2c_write(bmi3_i2c_client, reg_addr, data, len);
	return err;
}

/**
 * bmi3_i2c_probe - The I2C probe function called by I2C bus driver.
 *
 * @client : The I2C client instance
 * @id : The I2C device ID instance
 *
 * Return : Status of the function.
 * * 0 - OK
 * * negative value - Error.
 */
static int bmi3_i2c_probe(struct i2c_client *client,
			    const struct i2c_device_id *id)
{
	int err = 0;
	u8 dev_id;
	struct bmi3_client_data *client_data = NULL;

	PDEBUG("entrance");
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		dev_err(&client->dev, "i2c_check_functionality error!");
		PERR("I2C adapter is not supported");
		err = -EIO;
		goto exit_err_clean;
	}

	if (bmi3_i2c_client == NULL) {
		bmi3_i2c_client = client;
	} else {
		PERR("this driver does not support multiple clients");
		err = -EBUSY;
		goto exit_err_clean;
	}

	client_data = kzalloc(sizeof(struct bmi3_client_data),
						GFP_KERNEL);
	if (client_data == NULL) {
		PERR("no memory available");
		err = -ENOMEM;
		goto exit_err_clean;
	}
	/* h/w init */
	client_data->device.read_write_len = 4;
	dev_id = BMI3_I2C_INTF;
	client_data->device.intf_ptr = &dev_id;
	client_data->device.intf = BMI3_I2C_INTF;
	client_data->device.read = bmi3_i2c_read_wrapper;
	client_data->device.write = bmi3_i2c_write_wrapper;
	client_data->IRQ = client->irq;
	return bmi3_probe(client_data, &client->dev);

exit_err_clean:
	if (err)
		bmi3_i2c_client = NULL;
	return err;
}

/**
 *  bmi3_i2c_remove - Callback called when device is unbinded.
 *  @client : Instance of I2C client device.
 *
 *  Return : Status of the suspend function.
 *  * 0 - OK.
 *  * Negative value - Error.
 */
static int bmi3_i2c_remove(struct i2c_client *client)
{
	int err = 0;

	err = bmi3_remove(&client->dev);
	bmi3_i2c_client = NULL;
	return err;
}

static const struct i2c_device_id bmi3_id[] = {
	{ SENSOR_NAME, 0 },
	{ }
};

MODULE_DEVICE_TABLE(i2c, bmi3_id);
static const struct of_device_id bmi3_of_match[] = {
	{ .compatible = "bmi323", },
	{ }
};
MODULE_DEVICE_TABLE(i2c, bmi3_of_match);

static struct i2c_driver bmi3_driver = {
	.driver = {
		.owner = THIS_MODULE,
		.name = SENSOR_NAME,
		.of_match_table = bmi3_of_match,
	},
	.class = I2C_CLASS_HWMON,
	.id_table = bmi3_id,
	.probe = bmi3_i2c_probe,
	.remove = bmi3_i2c_remove,
};

/**
 *  bmi3_i2c_init - I2C driver init function.
 *
 *  Return : Status of the suspend function.
 *  * 0 - OK.
 *  * Negative value - Error.
 */
static int __init bmi3_i2c_init(void)
{
	return i2c_add_driver(&bmi3_driver);
}

/**
 *  bmi3_i2c_exit - I2C driver exit function.
 */
static void __exit bmi3_i2c_exit(void)
{
	i2c_del_driver(&bmi3_driver);
}

MODULE_AUTHOR("contact@bosch-sensortec.com>");
MODULE_DESCRIPTION("BMI3 SENSOR DRIVER");
MODULE_LICENSE("GPL v2");
/*lint -e19*/
module_init(bmi3_i2c_init);
module_exit(bmi3_i2c_exit);
/*lint +e19*/
