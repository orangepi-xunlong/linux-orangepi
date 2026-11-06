#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/of_platform.h>
#include <linux/of_reserved_mem.h>
#include <linux/of_gpio.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/iommu.h>
#include <linux/debugfs.h>
#include <linux/i2c.h>
#include <linux/version.h>

#include "vi_snsr.h"
#include "snsr_i2c.h"
#include "vi_sys.h"

enum {
	SNSR_I2C_DBG_DISABLE = 0,
	SNSR_I2C_DBG_VERIFY,
	SNSR_I2C_DBG_PRINT,
};

static int snsr_i2c_dbg = SNSR_I2C_DBG_DISABLE;
module_param(snsr_i2c_dbg, int, 0644);

static struct i2c_board_info info = {
	I2C_BOARD_INFO("sensor_i2c", (0x7b >> 1)),
};

static int snsr_i2c_write(struct i2c_dev *dev, struct isp_i2c_data *i2c)
{
	struct i2c_client *client;
	struct i2c_adapter *adap;
	struct i2c_msg msg;
	u8 i2c_dev = i2c->i2c_dev;
	u8 tx[4];
	int ret, retry = 5, idx = 0;

	if (!dev)
		return -ENODEV;

	/* check i2c device number. */
	if (i2c_dev >= I2C_MAX_NUM) {
		dev_err(&client->dev, "iic busid [%d] over i2c6, check!!\n", i2c_dev);
		return -EINVAL;
	}

	/* Get i2c client */
	client = dev->ctx[i2c_dev].client;
	adap = client->adapter;

	/* Config reg addr */
	if (i2c->addr_bytes == 1) {
		tx[idx++] = i2c->reg_addr & 0xff;
	} else {
		tx[idx++] = i2c->reg_addr >> 8;
		tx[idx++] = i2c->reg_addr & 0xff;
	}

	/* Config data */
	if (i2c->data_bytes == 1) {
		tx[idx++] = i2c->data & 0xff;
	} else {
		tx[idx++] = i2c->data >> 8;
		tx[idx++] = i2c->data & 0xff;
	}

	/* send the i2c with retry */
	msg.addr = i2c->dev_addr;
	msg.buf = tx;
	msg.len = idx;
	msg.flags = 0;

	while (retry--) {
		ret = i2c_transfer(adap, &msg, 1);
		if (ret == 1) {
			dev_dbg(&client->dev, "0x%x = 0x%x\n",
				i2c->reg_addr, i2c->data);
			break;
		} else if (ret == -EAGAIN) {
			dev_dbg(&client->dev, "retry 0x%x = 0x%x\n",
					i2c->reg_addr, i2c->data);
		} else {
			dev_err(&client->dev, "fail to send 0x%x, 0x%x, %d\n",
						i2c->reg_addr, i2c->data, ret);
		}
	}

	return ret == 1 ? 0 : -EIO;
}

static int snsr_i2c_burst_queue(struct i2c_dev *dev, struct isp_i2c_data *i2c)
{
	struct i2c_client *client;
	struct i2c_adapter *adap;
	u8 i2c_dev = i2c->i2c_dev;
	struct i2c_ctx *ctx;
	struct i2c_msg *msg;
	int idx = 0;
	u8 *tx;

	if (!dev)
		return -ENODEV;

	/* check i2c device number. */
	if (i2c_dev >= I2C_MAX_NUM) {
		dev_err(&client->dev, "iic busid [%d] over i2c6, check!!\n", i2c_dev);
		return -EINVAL;
	}

	/* Get i2c client */
	ctx = &dev->ctx[i2c_dev];
	if (ctx->msg_idx >= I2C_MAX_MSG_NUM)
		return -EINVAL;

	client = ctx->client;
	adap = client->adapter;
	msg = &ctx->msg[ctx->msg_idx];
	tx = &ctx->buf[4*ctx->msg_idx];

	/* Config reg addr */
	if (i2c->addr_bytes == 1) {
		tx[idx++] = i2c->reg_addr & 0xff;
	} else {
		tx[idx++] = i2c->reg_addr >> 8;
		tx[idx++] = i2c->reg_addr & 0xff;
	}

	/* Config data */
	if (i2c->data_bytes == 1) {
		tx[idx++] = i2c->data & 0xff;
	} else {
		tx[idx++] = i2c->data >> 8;
		tx[idx++] = i2c->data & 0xff;
	}

	/* Config msg */
	msg->addr = i2c->dev_addr;
	msg->buf = tx;
	msg->len = idx;
	msg->flags = I2C_M_WRSTOP;

	ctx->addr_bytes = i2c->addr_bytes;
	ctx->data_bytes = i2c->data_bytes;
	ctx->msg_idx++;

	return 0;
}

static void snsr_i2c_verify(struct i2c_client *client, struct i2c_ctx *ctx, uint32_t size)
{
	struct i2c_adapter *adap = client->adapter;
	struct i2c_msg msg;
	int i, step, ret;
	u8 tx[4];

	msg.addr = ctx->msg[0].addr;
	msg.buf = tx;

	/*
	 * In burst mode we have no idea when the transfer completes,
	 * therefore we wait for 1ms before verifying the results in debug mode.
	 */
	usleep_range(1000, 2000);
	step = ctx->addr_bytes + ctx->data_bytes;
	for (i = 0; i < size; i++) {
		if (step == 2) {
			/* 1 byte address, 1 byte data*/
			tx[0] = ctx->msg[i].buf[0];
			msg.buf = tx;
			msg.len = 1;
			msg.flags = 0;
			ret = i2c_transfer(adap, &msg, 1);
			if (ret != 1) {
				dev_dbg(&client->dev, "%s, i2c xfer ng\n", __func__);
				break;
			}
			tx[0] = 0;
			msg.flags = I2C_M_RD;
			ret = i2c_transfer(adap, &msg, 1);
			if (ret != 1) {
				dev_dbg(&client->dev, "%s, i2c xfer ng\n", __func__);
				break;
			}
			if (ctx->msg[i].buf[1] != tx[0]) {
				dev_dbg(&client->dev, "%s, addr 0x%02x, w: 0x%02x, r: 0x%02x\n",
					__func__, ctx->msg[i].buf[0], ctx->msg[i].buf[1], tx[0]);
			}
		} else if (step == 3) {
			/* 2 byte address, 1 byte data*/
			tx[0] = ctx->msg[i].buf[0];
			tx[1] = ctx->msg[i].buf[1];
			msg.buf = tx;
			msg.len = 2;
			msg.flags = 0;
			ret = i2c_transfer(adap, &msg, 1);
			if (ret != 1) {
				dev_dbg(&client->dev, "%s, i2c xfer ng\n", __func__);
				break;
			}
			tx[0] = 0;
			msg.len = 1;
			msg.flags = I2C_M_RD;
			ret = i2c_transfer(adap, &msg, 1);
			if (ret != 1) {
				dev_dbg(&client->dev, "%s, i2c xfer ng\n", __func__);
				break;
			}
			if (ctx->msg[i].buf[2] != tx[0]) {
				dev_dbg(&client->dev, "%s, addr 0x%02x,0x%02x w: 0x%02x, r: 0x%02x\n",
					__func__, ctx->msg[i].buf[0], ctx->msg[i].buf[1],
					ctx->msg[i].buf[2], tx[0]);
			}
		} else {
			/* 2 byte address, 2 byte data*/
			tx[0] = ctx->msg[i].buf[0];
			tx[1] = ctx->msg[i].buf[1];
			msg.buf = tx;
			msg.len = 2;
			msg.flags = 0;
			ret = i2c_transfer(adap, &msg, 1);
			if (ret != 1) {
				dev_dbg(&client->dev, "%s, i2c xfer ng\n", __func__);
				break;
			}
			tx[0] = 0;
			tx[1] = 0;
			msg.len = 2;
			msg.flags = I2C_M_RD;
			ret = i2c_transfer(adap, &msg, 1);
			if (ret != 1) {
				dev_dbg(&client->dev, "%s, i2c xfer ng\n", __func__);
				break;
			}
			if ((ctx->msg[i].buf[2] != tx[0]) || (ctx->msg[i].buf[3] != tx[1])) {
				dev_dbg(&client->dev, "%s, addr 0x%02x 0x%02x, w: 0x%02x 0x%02x, r: 0x%02x 0x%02x\n",
					__func__, ctx->msg[i].buf[0], ctx->msg[i].buf[1],
					ctx->msg[i].buf[2], ctx->msg[i].buf[3],
					tx[0], tx[1]);
			}
		}
	}
}

static void snsr_i2c_print(struct i2c_client *client, struct i2c_ctx *ctx, uint32_t size)
{
	int i, step;

	step = ctx->addr_bytes + ctx->data_bytes;

	for (i = 0; i < size; i++) {
		if (step == 2)
			dev_dbg(&client->dev, "a: 0x%02x, d: 0x%02x",
				ctx->msg[i].buf[0], ctx->msg[i].buf[1]);
		else if (step == 3)
			dev_dbg(&client->dev, "a: 0x%02x, 0x%02x, d: 0x%02x",
				ctx->msg[i].buf[0], ctx->msg[i].buf[1], ctx->msg[i].buf[2]);
		else
			dev_dbg(&client->dev, "a: 0x%02x, 0x%02x, d: 0x%02x, 0x%02x",
				ctx->msg[i].buf[0], ctx->msg[i].buf[1],
				ctx->msg[i].buf[2], ctx->msg[i].buf[3]);
	}
}

static int snsr_i2c_burst_fire(struct i2c_dev *dev, uint32_t i2c_dev)
{
	struct i2c_client *client;
	struct i2c_adapter *adap;
	struct i2c_ctx *ctx;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
	struct timespec64 ts;
#else
	struct timeval tv;
#endif
	uint64_t t1, t2;
	int ret, retry = 5;

	if (!dev)
		return -ENODEV;

	/* check i2c device number. */
	if (i2c_dev >= I2C_MAX_NUM) {
		dev_err(&client->dev, "iic busid [%d] over i2c6, check!!\n", i2c_dev);
		return -EINVAL;
	}

	/* Get i2c client */
	ctx = &dev->ctx[i2c_dev];
	if (!ctx->msg_idx)
		return 0;
	client = ctx->client;
	adap = client->adapter;

	while (retry--) {
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
		ktime_get_real_ts64(&ts);
		t1 = ts.tv_sec * 1000000 + ts.tv_nsec / 1000;
#else
		do_gettimeofday(&tv);
		t1 = tv.tv_sec * 1000000 + tv.tv_usec;
#endif
		ret = i2c_transfer(adap, ctx->msg, ctx->msg_idx);
		if (ret == ctx->msg_idx) {
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
			ktime_get_real_ts64(&ts);
			t2 = ts.tv_sec * 1000000 + ts.tv_nsec / 1000;
#else
			do_gettimeofday(&tv);
			t2 = tv.tv_sec * 1000000 + tv.tv_usec;
#endif

			dev_dbg(&client->dev, "burst [%d] success %lld us\n", ret, (t2 - t1));
			if (snsr_i2c_dbg == SNSR_I2C_DBG_VERIFY)
				snsr_i2c_verify(client, ctx, ret);
			else if (snsr_i2c_dbg == SNSR_I2C_DBG_PRINT)
				snsr_i2c_print(client, ctx, ret);
			break;
		} else if (ret == -EAGAIN) {
			dev_dbg(&client->dev, "retry\n");
		} else {
			dev_err(&client->dev, "fail to send burst\n");
		}
	}

	ctx->msg_idx = 0;

	return ret == 1 ? 0 : -EIO;
}

static long snsr_i2c_ioctl(void *hdlr, unsigned int cmd, void *arg)
{
	struct i2c_dev *dev = (struct i2c_dev *)hdlr;
	uint32_t *argp = (uint32_t *)arg;
	uint32_t i2c_dev;

	switch (cmd) {
	case SNS_I2C_WRITE:
		return snsr_i2c_write(dev, (struct isp_i2c_data *)arg);
	case SNS_I2C_BURST_QUEUE:
		return snsr_i2c_burst_queue(dev, (struct isp_i2c_data *)arg);
	case SNS_I2C_BURST_FIRE:
		i2c_dev = *argp;
		return snsr_i2c_burst_fire(dev, i2c_dev);
	default:
		return -ENOIOCTLCMD;
	}

	return 0;
}

static int snsr_i2c_register_cb(struct platform_device *pdev,
				struct i2c_dev *dev)
{
	/* register cmm callbacks */
	return vi_sys_register_cmm_cb(0, dev, snsr_i2c_ioctl);
}

static int _init_resource(struct platform_device *pdev)
{
	struct i2c_adapter *i2c_adap;
	struct i2c_dev *dev = dev_get_drvdata(&pdev->dev);
	int i = 0;

	if (!dev)
		return -ENODEV;

	for (i = 0; i < I2C_MAX_NUM; i++) {
		struct i2c_ctx *ctx;

		i2c_adap = i2c_get_adapter(i);
		if (!i2c_adap)
			continue;
		dev_info(&pdev->dev, "i2c:-------hook %d\n", i2c_adap->i2c_idx);
		if (i2c_adap->i2c_idx < I2C_MAX_NUM) {
			ctx = &dev->ctx[i2c_adap->i2c_idx];
			if (!ctx->client) {
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
				ctx->client = i2c_new_client_device(i2c_adap, &info);
#else
				ctx->client = i2c_new_device(i2c_adap, &info);
#endif
				ctx->buf = kmalloc(I2C_BUF_SIZE, GFP_KERNEL);
				if (!ctx->buf)
					return -ENOMEM;
			} else
				dev_err(&pdev->dev, "duplicate i2c_adpa idx %d\n", i2c_adap->i2c_idx);

		}
		i2c_put_adapter(i2c_adap);
	}

	return 0;
}

static int snsr_i2c_probe(struct platform_device *pdev)
{
	int rc = 0;
	struct i2c_dev *dev;

	/* allocate main snsr state structure */
	dev = devm_kzalloc(&pdev->dev, sizeof(*dev), GFP_KERNEL);
	if (!dev)
		return -ENOMEM;

	/* initialize locks */
	spin_lock_init(&dev->lock);
	mutex_init(&dev->mutex);

	platform_set_drvdata(pdev, dev);

	rc = snsr_i2c_register_cb(pdev, dev);
	if (rc < 0) {
		dev_err(&pdev->dev, "Failed to register cmm for snsr, %d\n", rc);
		return rc;
	}

	rc = _init_resource(pdev);
	if (rc < 0) {
		dev_err(&pdev->dev, "Failed to init res for snsr, %d\n", rc);
		return rc;
	}

	return 0;
}

static int snsr_i2c_remove(struct platform_device *pdev)
{
	struct i2c_dev *dev;
	int i = 0;

	if (!pdev) {
		dev_err(&pdev->dev, "invalid param");
		return -EINVAL;
	}

	dev = dev_get_drvdata(&pdev->dev);
	if (!dev) {
		dev_err(&pdev->dev, "Can not get snsr drvdata");
		return 0;
	}

	for (i = 0; i < I2C_MAX_NUM; i++) {
		struct i2c_ctx *ctx;

		ctx = &dev->ctx[i];
		if (ctx->client)
			i2c_unregister_device(ctx->client);
		kfree(ctx->buf);
	}

	dev_set_drvdata(&pdev->dev, NULL);

	return 0;
}

static void snsr_i2c_pdev_release(struct device *dev)
{
}

static struct platform_device snsr_i2c_pdev = {
	.name		= "snsr_i2c",
	.dev.release	= snsr_i2c_pdev_release,
	.id		= PLATFORM_DEVID_NONE,
};

static struct platform_driver snsr_i2c_pdrv = {
	.probe      = snsr_i2c_probe,
	.remove     = snsr_i2c_remove,
	.driver     = {
		.name		= "snsr_i2c",
		.owner		= THIS_MODULE,
	},
};

static int __init snsr_i2c_init(void)
{
	int rc;

	rc = platform_device_register(&snsr_i2c_pdev);
	if (rc)
		return rc;

	rc = platform_driver_register(&snsr_i2c_pdrv);
	if (rc)
		platform_device_unregister(&snsr_i2c_pdev);

	return rc;
}

static void __exit snsr_i2c_exit(void)
{
	platform_driver_unregister(&snsr_i2c_pdrv);
	platform_device_unregister(&snsr_i2c_pdev);
}

MODULE_DESCRIPTION("Cvitek Sensor Driver");
MODULE_AUTHOR("Max Liao");
MODULE_LICENSE("GPL");
module_init(snsr_i2c_init);
module_exit(snsr_i2c_exit);
