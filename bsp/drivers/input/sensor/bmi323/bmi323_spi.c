/**
 * @section LICENSE
 * Copyright (c) 2022 Bosch Sensortec GmbH All Rights Reserved
 *
 * This software program is licensed subject to the GNU General
 * Public License (GPL).Version 2,June 1991,
 * available at http://www.fsf.org/copyleft/gpl.html
 *
 * @filename bmi323_spi.c
 * @date	 10/10/2022
 * @version  1.6.0
 *
 * @brief    BMI3 SPI bus Driver
 */

/*********************************************************************/
/* system header files */
/*********************************************************************/
#include <linux/module.h>
#include <linux/spi/spi.h>
#include <linux/delay.h>
#include <linux/version.h>

/*********************************************************************/
/* own header files */
/*********************************************************************/
#include "bmi323_driver.h"
#include "bs_log.h"

/*********************************************************************/
/* Local macro definitions */
/*********************************************************************/
#define BMI323_MAX_BUFFER_SIZE		32

/*********************************************************************/
/* global variables */
/*********************************************************************/
static struct spi_device *bmi323_spi_client;

/*!
 * @brief define spi block wirte function
 *
 * @param[in] reg_addr register address
 * @param[in] data the pointer of data buffer
 * @param[in] len block size need to write
 *
 * @return zero success, non-zero failed
 * @retval zero success
 * @retval non-zero failed
 */
static s8 bmi323_spi_write_block(u8 reg_addr,
					const u8 *data, u8 len)
{
	struct spi_device *client = bmi323_spi_client;
	u8 buffer[BMI323_MAX_BUFFER_SIZE + 1];
	struct spi_transfer xfer = {
		.tx_buf = buffer,
		.len = len + 1,
	};
	struct spi_message msg;

	if (len > BMI323_MAX_BUFFER_SIZE)
		return -EINVAL;

	buffer[0] = reg_addr&0x7F;/* write: MSB = 0 */
	memcpy(&buffer[1], data, len);

	spi_message_init(&msg);
	spi_message_add_tail(&xfer, &msg);
	return spi_sync(client, &msg);
}

/*!
 * @brief define spi block read function
 *
 * @param[in] reg_addr register address
 * @param[out] data the pointer of data buffer
 * @param[in] len block size need to read
 *
 * @return zero success, non-zero failed
 * @retval zero success
 * @retval non-zero failed
 */
static s8 bmi323_spi_read_block(u8 reg_addr,
							u8 *data, uint16_t len)
{
	struct spi_device *client = bmi323_spi_client;
	u8 reg = reg_addr | 0x80;/* read: MSB = 1 */
	struct spi_transfer xfer[2] = {
		[0] = {
			.tx_buf = &reg,
			.len = 1,
		},
		[1] = {
			.rx_buf = data,
			.len = len,
		}
	};
	struct spi_message msg;

	spi_message_init(&msg);
	spi_message_add_tail(&xfer[0], &msg);
	spi_message_add_tail(&xfer[1], &msg);
	return spi_sync(client, &msg);
}

/**
 * bmi323_spi_write_wrapper - The SPI write function pointer used by BMI323 API.
 *
 * @reg_addr : The register address to start writing the data.
 * @data : The pointer to buffer which holds the data to be written.
 * @len : The number of bytes to be written.
 * @intf_ptr  : Void pointer that can enable the linking of descriptors
 *									for interface related call backs.
 *
 * Return : Status of the function.
 * * 0 - OK
 * * negative value - Error.
 */
static s8 bmi323_spi_write_wrapper(u8 reg_addr, const u8 *data,
						u32 len, void *intf_ptr)
{
	s8 err;

	err = bmi323_spi_write_block(reg_addr, data, len);
	return err;
}

/**
 * bmi323_spi_read_wrapper - The SPI read function pointer used by BMI323 API.
 *
 * @reg_addr : The register address to read the data.
 * @data : The pointer to buffer to return data.
 * @len : The number of bytes to be read
 *
 * Return : Status of the function.
 * * 0 - OK
 * * negative value - Error.
 */
static s8 bmi323_spi_read_wrapper(u8 reg_addr, u8 *data,
						u32 len, void *intf_ptr)
{
	s8 err;

	err = bmi323_spi_read_block(reg_addr, data, len);
	return err;
}

/*!
 * @brief BMI probe function via spi bus
 *
 * @param[in] client the pointer of spi client
 *
 * @return zero success, non-zero failed
 * @retval zero success
 * @retval non-zero failed
 */
static int bmi323_spi_probe(struct spi_device *client)
{
	int status;
	int err = 0;
	u8 dev_id;
	struct bmi3_client_data *client_data = NULL;

	if (bmi323_spi_client == NULL)
		bmi323_spi_client = client;
	else {
		PERR("This driver does not support multiple clients!\n");
		return -EBUSY;
	}
	client->bits_per_word = 8;
	status = spi_setup(client);
	if (status < 0) {
		PERR("spi_setup failed!\n");
		return status;
	}
	client_data = kzalloc(sizeof(struct bmi3_client_data), GFP_KERNEL);
	if (client_data == NULL) {
		PERR("no memory available");
		err = -ENOMEM;
		goto exit_err_clean;
	}

	client_data->device.read_write_len = 4;
	dev_id = BMI3_SPI_INTF;
	client_data->device.intf_ptr = &dev_id;
	client_data->device.intf = BMI3_SPI_INTF;
	client_data->device.read = bmi323_spi_read_wrapper;
	client_data->device.write = bmi323_spi_write_wrapper;
	client_data->IRQ = client->irq;
	return bmi3_probe(client_data, &client->dev);

exit_err_clean:
	if (err)
		bmi323_spi_client = NULL;
	return err;
}

/*!
 * @brief remove bmi spi client
 *
 * @param[in] client the pointer of spi client
 *
 * @return zero
 * @retval zero
 */
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 18, 0))
static void bmi323_spi_remove(struct spi_device *client)
#else
static int bmi323_spi_remove(struct spi_device *client)
#endif
{
	int err = 0;

	err = bmi3_remove(&client->dev);
	if (err)
		printk("bmi323_spi_remove ret %d", err);

	bmi323_spi_client = NULL;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 18, 0))
	return;
#else
	return err;
#endif
}

/*!
 * @brief register spi device id
 */
static const struct spi_device_id bmi323_id[] = {
	{ SENSOR_NAME, 0 },
	{ }
};

MODULE_DEVICE_TABLE(spi, bmi323_id);
/*!
 * @brief register bmi323 device id match
 */
static const struct of_device_id bmi323_of_match[] = {
	{ .compatible = "bmi323", },
	{ }
};

MODULE_DEVICE_TABLE(spi, bmi323_of_match);
/*!
 * @brief register spi driver hooks
 */
static struct spi_driver bmi_spi_driver = {
	.driver = {
		.owner = THIS_MODULE,
		.name  = SENSOR_NAME,
		.of_match_table = bmi323_of_match,
	},
	.id_table = bmi323_id,
	.probe	  = bmi323_spi_probe,
	.remove	  = bmi323_spi_remove,
};

/*!
 * @brief initialize bmi spi module
 *
 * @return zero success, non-zero failed
 * @retval zero success
 * @retval non-zero failed
 */
static int __init bmi_spi_init(void)
{
	return spi_register_driver(&bmi_spi_driver);
}

/*!
 * @brief remove bmi spi module
 *
 * @return no return value
 */
static void __exit bmi_spi_exit(void)
{
	spi_unregister_driver(&bmi_spi_driver);
}


MODULE_AUTHOR("Contact <contact@bosch-sensortec.com>");
MODULE_DESCRIPTION("BMI323 SPI DRIVER");
MODULE_LICENSE("GPL v2");
/*lint -e19*/
module_init(bmi_spi_init);
module_exit(bmi_spi_exit);
/*lint +e19*/
