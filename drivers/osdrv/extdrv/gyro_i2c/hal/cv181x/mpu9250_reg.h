#include <linux/types.h>

void mpu9250_readArray(struct i2c_client *client, uint8_t reg, uint8_t *output, int size);
uint8_t mpu9250_read(struct i2c_client *client, uint8_t reg);
int mpu9250_write(struct i2c_client *client, uint8_t reg, uint8_t value);
int mpu9250_write_OR(struct i2c_client *client, uint8_t reg, uint8_t value);
int mpu9250_write_AND(struct i2c_client *client, uint8_t reg, uint8_t value);
