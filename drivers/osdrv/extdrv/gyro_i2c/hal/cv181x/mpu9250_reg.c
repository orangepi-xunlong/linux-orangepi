#include <linux/i2c.h>
#include <mpu9250_reg.h>

/**
 * @brief Read the array of bytes from the device.
 * @note Array has to be first requested using requestBytes method.
 * @param output Pointer to the array where the received bytes will be written.
 * @param size How many bytes we want to read.
 */
void mpu9250_readArray(struct i2c_client *client, uint8_t reg, uint8_t *output, int size)
{
	int i = 0;

	for (i = 0; i < size; i++) {
		output[i] = mpu9250_read(client, reg + i);
	}
}

/**
 * @brief Read a single byte from the register
 * @param client struct i2c_client.
 * @param reg Address of the memory
 * @return Address value
 */
uint8_t mpu9250_read(struct i2c_client *client, uint8_t reg)
{
	return (i2c_smbus_read_byte_data(client, reg) & 0xFF);
}

/**
 * @brief Write a single byte to the register.
 * @param client struct i2c_client.
 * @param reg Address of the memory.
 * @param value Value that we want to put in the register.
 * @return SUCCESS or FAILED.
 */
int mpu9250_write(struct i2c_client *client, uint8_t reg, uint8_t value)
{
	return (i2c_smbus_write_byte_data(client, reg, value) & 0xFF);
}

/**
 * @brief Change state of the register using OR operation
 * @param client struct i2c_client.
 * @param reg register address
 * @param value data
 * @return SUCCESS or FAILED.
 */
int mpu9250_write_OR(struct i2c_client *client, uint8_t reg, uint8_t value)
{
	uint8_t ret;
	uint8_t c;

	c = mpu9250_read(client, reg) | value;
	ret = mpu9250_write(client, reg, c);

	return ret;
}

/**
 * @brief Change state of the register using AND operation
 * @param client struct i2c_client.
 * @param reg register address
 * @param value data
 * @return SUCCESS or FAILED.
 */
int mpu9250_write_AND(struct i2c_client *client, uint8_t reg, uint8_t value)
{
	int ret;
	uint8_t c;

	c = mpu9250_read(client, reg) & value;
	ret = mpu9250_write(client, reg, c);

	return ret;
}
