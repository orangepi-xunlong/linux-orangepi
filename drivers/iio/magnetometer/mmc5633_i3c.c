// SPDX-License-Identifier: GPL-2.0
/*
 * MMC5633 - MEMSIC 3-axis Magnetic Sensor
 *
 *Copyright 2024 Cix Technology Group Co., Ltd. *
 * IIO driver for MMC5633 (7-bit I2C slave address 0x30).
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/i3c/device.h>
#include <linux/i3c/master.h>
#include <linux/slab.h>
#include <linux/of.h>
#include <linux/regmap.h>
#include <linux/delay.h>

#define MMC5633_REG_TEMPERATURE 0x09
#define MMC5633_REG_STATUS      0x18
#define MMC5633_REG_CTRL0       0x1B
#define MMC5633_ST_X            0x27
#define MMC5633_REG_ID          0x39
#define MMC5633_REG_INTERNAL_CONTROL2       0x1D

#define MMC5633_PRODUCT_ID      0x10

static const struct i3c_device_id mmc5633_i3c_ids[] = {
	I3C_DEVICE(0x251, 0x0, (void *)0),
	{ /* sentinel */ },
};
MODULE_DEVICE_TABLE(i3c, mmc5633_i3c_ids);

void mmc5633_ibi_handler(struct i3c_device *i3cdev, const struct i3c_ibi_payload *payload)
{
        printk("IBI received from device %s\n", dev_name(&i3cdev->dev));
}

static int mmc5633_ibi_config(struct i3c_device *i3cdev)
{
        struct i3c_ibi_setup ibi_setup = {
                .num_slots = 1,
                .max_payload_len = 32,
                .handler = mmc5633_ibi_handler,
        };
        int ret;

        ret = i3c_device_request_ibi(i3cdev, &ibi_setup);
        if (ret)
        {
                dev_err(&i3cdev->dev, "Failed to request IBI for %s\n", dev_name(&i3cdev->dev));
                return ret;
        }
        ret = i3c_device_enable_ibi(i3cdev);
        if (ret)
        {
                dev_err(&i3cdev->dev, "Failed to enable IBI for %s\n", dev_name(&i3cdev->dev));
                return ret;
        }
        dev_info(&i3cdev->dev, "IBI setup successfully for %s\n", dev_name(&i3cdev->dev));
        return ret;
}


static int mmc5633_i3c_probe(struct i3c_device *i3cdev)
{
	int data = 0,err = 0,temperature = 0;
	struct regmap_config mmc5633_i3c_regmap_config = {
		.reg_bits = 8,
		.val_bits = 8,
	};
	const struct i3c_device_id *id = i3c_device_match_id(i3cdev,
							    mmc5633_i3c_ids);
	struct regmap *regmap;
	regmap = devm_regmap_init_i3c(i3cdev, &mmc5633_i3c_regmap_config);
	if (IS_ERR(regmap)) {
		dev_err(&i3cdev->dev, "Failed to register i3c regmap %d\n",
			(int)PTR_ERR(regmap));
		return PTR_ERR(regmap);
	}
	err = regmap_read(regmap, MMC5633_REG_ID, &data);
	dev_info(&i3cdev->dev, "product ID = 0x%x\n", data);
	err = regmap_write(regmap, MMC5633_REG_CTRL0, 0x2);
	udelay(10);
	err = regmap_read(regmap, MMC5633_REG_STATUS, &data);
	err = regmap_read(regmap, MMC5633_REG_TEMPERATURE, &data);
	temperature = (unsigned int)(8 * data / 10) - 75;
        dev_info(&i3cdev->dev, "tempaurate=%d\n", temperature);

	/*config ibi*/
	mmc5633_ibi_config(i3cdev);
        err = regmap_write(regmap, MMC5633_REG_INTERNAL_CONTROL2, 0x40);

	return err;
}

static struct i3c_driver mmc5633_driver = {
	.driver = {
		.name = "mmc5633_i3c",
	},
	.probe = mmc5633_i3c_probe,
	.id_table = mmc5633_i3c_ids,
};
module_i3c_driver(mmc5633_driver);

MODULE_AUTHOR("hongliang.yang@cixtech.com");
MODULE_DESCRIPTION("STMicroelectronics mmc5633 i3c driver");
MODULE_LICENSE("GPL v2");
