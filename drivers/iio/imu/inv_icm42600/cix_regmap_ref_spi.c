// SPDX-License-Identifier: GPL-2.0
/*
 *Copyright 2024 Cix Technology Group Co., Ltd. *
 */

#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/mod_devicetable.h>
#include <linux/spi/spi.h>
#include <linux/property.h>

#include "inv_icm42600.h"
#include "./../../../base/regmap/internal.h"

#define SPI_WRITE_CMD(reg) (reg)
#define SPI_READ_CMD(reg) ((u8)(((reg) & 0xFF) | 0x80))

#define CNDS_SPI_MIN_TRANSFER_SIZE 4

static void inv_icm42600_reverse(u8 *arr, u32 start, u32 end)
{
	u32 temp = 0;

	if (start >= end) {
		return;
	}

	for (; start < end; start++, end--) {
		temp = arr[start];
		arr[start] = arr[end];
		arr[end] = temp;
	}
}

static int inv_icm42600_reverseByGroup(u8 *arr, u32 n, u32 k)
{
	u32 start, end, i;

	if (arr == NULL || n == 0 || k == 0 || n < k)
		return -EINVAL;
	if (n % k != 0)
		return -EINVAL;

	for (i = 0; i < n; i += k) {
		start = i;
		end = i + k - 1;
		if (end >= n) end = n - 1;
		inv_icm42600_reverse(arr, start, end);
	}

	return 0;
}

static inline void *inv_icm42600_regmap_map_get_context(struct regmap *map)
{
	return (map->bus || (!map->bus && map->read)) ? map : map->bus_context;
}

int inv_icm42600_regmap_read(struct regmap *map, u32 reg, u32 *val)
{
	struct regmap *dev_map = inv_icm42600_regmap_map_get_context(map);
	struct device *dev = dev_map->bus_context;
	struct spi_device *spi = to_spi_device(dev);
	struct spi_message m;
	struct spi_transfer t;
	u8 tx_buffer[CNDS_SPI_MIN_TRANSFER_SIZE];
	u8 rx_buffer[CNDS_SPI_MIN_TRANSFER_SIZE];
	s32 ret;

	if (!IS_ALIGNED(reg, map->reg_stride))
		return -EINVAL;

	map->lock(map->lock_arg);

	memset(tx_buffer, 0, sizeof(tx_buffer));
	memset(rx_buffer, 0, sizeof(rx_buffer));

	tx_buffer[CNDS_SPI_MIN_TRANSFER_SIZE - 1] = SPI_READ_CMD(reg);
	t.tx_buf = tx_buffer;
	t.rx_buf = rx_buffer;
	t.len = CNDS_SPI_MIN_TRANSFER_SIZE;

	spi_message_init(&m);
	spi_message_add_tail(&t, &m);
	ret = spi_sync(spi, &m);
	if (ret)
		goto out;

	*val = rx_buffer[CNDS_SPI_MIN_TRANSFER_SIZE - 2];

out:
	map->unlock(map->lock_arg);

	return ret;
}
EXPORT_SYMBOL_GPL(inv_icm42600_regmap_read);

int inv_icm42600_regmap_write(struct regmap *map, u32 reg, u32 val)
{
	struct regmap *dev_map = inv_icm42600_regmap_map_get_context(map);
	struct device *dev = dev_map->bus_context;
	struct spi_device *spi = to_spi_device(dev);
	u8 tx_buffer[CNDS_SPI_MIN_TRANSFER_SIZE];
	s32 ret, temp, i;

	if (!IS_ALIGNED(reg, map->reg_stride))
		return -EINVAL;

	memset(tx_buffer, 0, sizeof(tx_buffer));

	tx_buffer[CNDS_SPI_MIN_TRANSFER_SIZE - 1] = reg;
	tx_buffer[CNDS_SPI_MIN_TRANSFER_SIZE - 2] = val;

	for (i = 0; i < CNDS_SPI_MIN_TRANSFER_SIZE - 2; i++) {
		ret = inv_icm42600_regmap_read(map, reg + 1 + i, &temp);
		if (ret)
			return ret;
		tx_buffer[CNDS_SPI_MIN_TRANSFER_SIZE - 3 - i] = temp;
	}

	map->lock(map->lock_arg);
	ret = spi_write(spi, (const void *)tx_buffer, CNDS_SPI_MIN_TRANSFER_SIZE);
	map->unlock(map->lock_arg);

	return ret;
}
EXPORT_SYMBOL_GPL(inv_icm42600_regmap_write);

inline int inv_icm42600_regmap_update_bits(struct regmap *map, u32 reg,
				     u32 mask, u32 val)
{
	u32 tmp, orig;
	s32 ret;

	ret = inv_icm42600_regmap_read(map, reg, &orig);
	if (ret)
		return ret;

	tmp = orig & ~mask;
	tmp |= val & mask;
	if (tmp != orig) {
		ret = inv_icm42600_regmap_write(map, reg, tmp);
	}

	return ret;
}
EXPORT_SYMBOL_GPL(inv_icm42600_regmap_update_bits);

inline int inv_icm42600_regmap_update_bits_check(struct regmap *map, u32 reg,
					   u32 mask, u32 val, bool *change)
{
	u32 tmp, orig;
	s32 ret;

	if (change)
		*change = false;

	ret = inv_icm42600_regmap_read(map, reg, &orig);
	if (ret)
		return ret;

	tmp = orig & ~mask;
	tmp |= val & mask;
	if (tmp != orig) {
		ret = inv_icm42600_regmap_write(map, reg, tmp);
		if (ret == 0 && change)
			*change = true;
	}

	return ret;
}
EXPORT_SYMBOL_GPL(inv_icm42600_regmap_update_bits_check);

int inv_icm42600_regmap_noinc_read(struct regmap *map, u32 reg,
		      void *val, size_t val_len)
{
	size_t read_len = val_len;
	s32 ret;

	while (read_len) {
		ret = inv_icm42600_regmap_read(map, reg, val);
		if (ret)
			break;
		val = ((u8 *)val) + 1;
		read_len -= 1;
	}

	return ret;
}
EXPORT_SYMBOL_GPL(inv_icm42600_regmap_noinc_read);

int inv_icm42600_regmap_bulk_write(struct regmap *map, u32 reg, const void *val,
		     size_t val_count)
{
	struct regmap *dev_map = inv_icm42600_regmap_map_get_context(map);
	struct device *dev = dev_map->bus_context;
	struct spi_device *spi = to_spi_device(dev);
	u8 *tx_buffer = NULL;
	const u8 *b = val;
	s32 ret = -1, reg_avl;
	u32 len, i, reg_inc;

	if (!IS_ALIGNED(reg, map->reg_stride))
		return -EINVAL;

	map->lock(map->lock_arg);

	len = (val_count / CNDS_SPI_MIN_TRANSFER_SIZE + 1) * CNDS_SPI_MIN_TRANSFER_SIZE;
	tx_buffer = kzalloc(len, GFP_KERNEL);
	if (!tx_buffer) {
		map->unlock(map->lock_arg);
		return -ENOMEM;
	}

	tx_buffer[0] = reg;
	for (i = 0; i < val_count; i++) {
		tx_buffer[i + 1] = b[i];
	}

	reg_inc = 0;
	for (i = val_count; i < len; i++) {
		ret = inv_icm42600_regmap_read(map, reg + val_count + reg_inc, &reg_avl);
		if (ret)
			goto out;
		tx_buffer[i + 1] = reg_avl;
		reg_inc++;
	}

	ret = inv_icm42600_reverseByGroup(tx_buffer, len, CNDS_SPI_MIN_TRANSFER_SIZE);
	if (ret)
		goto out;

	ret = spi_write(spi, (const void *)tx_buffer, len);

out:
	kfree(tx_buffer);
	map->unlock(map->lock_arg);

	return ret;
}
EXPORT_SYMBOL_GPL(inv_icm42600_regmap_bulk_write);

int inv_icm42600_regmap_bulk_read(struct regmap *map, u32 reg, void *val,
		     size_t val_count)
{
	struct regmap *dev_map = inv_icm42600_regmap_map_get_context(map);
	struct device *dev = dev_map->bus_context;
	struct spi_device *spi = to_spi_device(dev);
	struct spi_message m;
	struct spi_transfer t;
	u8 *tx_buffer = NULL;
	u8 *rx_buffer = NULL;
	u8 *b = val;
	u32 i, len;
	s32 ret;

	if (!IS_ALIGNED(reg, map->reg_stride))
		return -EINVAL;

	map->lock(map->lock_arg);

	len = (val_count / CNDS_SPI_MIN_TRANSFER_SIZE + 1) * CNDS_SPI_MIN_TRANSFER_SIZE;
	rx_buffer = kzalloc(len, GFP_KERNEL);
	if (!rx_buffer) {
		map->unlock(map->lock_arg);
		return -ENOMEM;
	}

	tx_buffer = kzalloc(len, GFP_KERNEL);
	if (!tx_buffer) {
		map->unlock(map->lock_arg);
		kfree(rx_buffer);
		return -ENOMEM;
	}

	tx_buffer[CNDS_SPI_MIN_TRANSFER_SIZE - 1] = SPI_READ_CMD(reg);
	t.tx_buf = tx_buffer;
	t.rx_buf = rx_buffer;
	t.len = len;

	spi_message_init(&m);
	spi_message_add_tail(&t, &m);
	ret = spi_sync(spi, &m);
	if (ret)
		goto out;

	ret = inv_icm42600_reverseByGroup(rx_buffer, len, CNDS_SPI_MIN_TRANSFER_SIZE);
	if (ret)
		goto out;

	for (i = 0; i < val_count; i++)
	{
		b[i] = rx_buffer[i + 1];
	}

out:
	kfree(rx_buffer);
	kfree(tx_buffer);
	map->unlock(map->lock_arg);

	return ret;
}
EXPORT_SYMBOL_GPL(inv_icm42600_regmap_bulk_read);

MODULE_AUTHOR("Cix Tech");
MODULE_DESCRIPTION("Redefine invenSense ICM-426xx SPI write/read interface to adapt the cadence spi control driver");
MODULE_LICENSE("GPL v2");
