/*
 * DHT11/DHT22 bit banging GPIO driver
 *
 * Copyright (c) Harald Geyer <harald@ccbib.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#include <linux/err.h>
#include <linux/interrupt.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/printk.h>
#include <linux/slab.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/sysfs.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/wait.h>
#include <linux/bitops.h>
#include <linux/completion.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/pinctrl/pinconf.h>
#include <linux/pinctrl/pinconf-sunxi.h>
#include <linux/pinctrl/consumer.h>
#include <linux/kthread.h>

#include <mach/platform.h>

#include "../iio.h"

//#define DHT11_DEBUG 1
#define DRIVER_NAME	"dht11"

#define DHT11_DATA_VALID_TIME	2000000000  /* 2s in ns */

#define DHT11_EDGES_PREAMBLE 4
#define DHT11_BITS_PER_READ 40
#define DHT11_EDGES_PER_READ (2*DHT11_BITS_PER_READ + DHT11_EDGES_PREAMBLE)

/* Data transmission timing (nano seconds) */
#define DHT11_START_TRANSMISSION	18  /* ms */
#define DHT11_SENSOR_RESPONSE	80000
#define DHT11_START_BIT		50000
#define DHT11_DATA_BIT_LOW	27000
#define DHT11_DATA_BIT_HIGH	70000

#define DATA_START_FLAG_MIN 	(50000)
#define DATA_START_FLAG_MAX 	(60000)
#define DATA_ONE_MIN 			(65000)
#define DATA_ONE_MAX 			(75000)
#define DATA_ZERO_MIN 			(20000)
#define DATA_ZERO_MAX 			(30000)


struct dht11 {
	struct device			*dev;

	int				gpio;
	int				irq;

	struct completion		completion;

	s64				timestamp;
	int				temperature;
	int				humidity;

	/* num_edges: -1 means "no transmission in progress" */
	int				num_edges;
	struct {s64 ts; int value; }	edges[DHT11_EDGES_PER_READ];
};

static int gpio = -1;
static int dht11_read_flag = 0;
static unsigned char dht11_decode_byte(int *timing)
{
	unsigned char ret = 0;
	int i;

	for (i = 0; i < 8; ++i) {
		ret <<= 1;
		ret +=timing[i];
	}

	return ret;
}

static int dht11_decode(struct dht11 *dht11, int *timing)
{
	unsigned char temp_int, temp_dec, hum_int, hum_dec, checksum;

#ifdef DHT11_DEBUG
	int i = 0;
	for(i=0; i<DHT11_BITS_PER_READ; i++)
		printk(KERN_ERR"%02d %d\n", i, timing[i]);
#endif
	hum_int = dht11_decode_byte(timing);
	hum_dec = dht11_decode_byte(&timing[8]);
	temp_int = dht11_decode_byte(&timing[16]);
	temp_dec = dht11_decode_byte(&timing[24]);
	checksum = dht11_decode_byte(&timing[32]);

#ifdef DHT11_DEBUG
	printk(KERN_ERR"%d + %d + %d + %d = %d\n", hum_int, hum_dec, temp_int, temp_dec, checksum);
#endif
	if (((hum_int + hum_dec + temp_int + temp_dec) & 0xff) != checksum) {
		dev_err(dht11->dev,
			"checksum error %d %d %d %d %d\n",
			hum_int, hum_dec, temp_int, temp_dec, checksum);
		return -EIO;
	}	

	dht11->timestamp = iio_get_time_ns();
	if (hum_int < 20) {  /* DHT22 */
		dht11->temperature = (((temp_int & 0x7f) << 8) + temp_dec) *
					((temp_int & 0x80) ? -100 : 100);
		dht11->humidity = ((hum_int << 8) + hum_dec) * 100;
	} else if (temp_dec == 0 && hum_dec == 0) {  /* DHT11 */
		dht11->temperature = temp_int * 1000;
		dht11->humidity = hum_int * 1000;
	} else {
		dev_err(dht11->dev,
			"Don't know how to decode data: %d %d %d %d\n",
			hum_int, hum_dec, temp_int, temp_dec);
		return -EIO;
	}

	return 0;
}

static int dht11_read_thread(void *__dev)
{
	struct dht11 *dht11 = (struct dht11 *)__dev;
	int value = 0;
	
#ifdef DHT11_DEBUG
	printk(KERN_INFO"%s start\n", __func__);
#endif
	if (gpio_direction_output(dht11->gpio, 0))
		return -1;
	msleep(DHT11_START_TRANSMISSION);
	gpio_direction_input(dht11->gpio);
	dht11->edges[dht11->num_edges].value = gpio_get_value(dht11->gpio);
	dht11->edges[dht11->num_edges].ts = iio_get_time_ns();
	while (dht11->num_edges < DHT11_EDGES_PER_READ && dht11_read_flag == 1) {
		value = gpio_get_value(dht11->gpio);
		if (dht11->edges[dht11->num_edges].value != value) {
			dht11->num_edges++;
			dht11->edges[dht11->num_edges].ts = iio_get_time_ns();
			dht11->edges[dht11->num_edges].value = value;
			if (dht11->num_edges == DHT11_EDGES_PER_READ)
				complete(&dht11->completion);
		}
	}
	return 0;
}

static int dht11_read_raw(struct iio_dev *iio_dev,
			const struct iio_chan_spec *chan,
			int *val, int *val2, long m)
{
	struct dht11 *dht11 = iio_priv(iio_dev);
	int ret;
	int i, j;
	int timing[DHT11_BITS_PER_READ], time;
	struct task_struct *th;

	if (dht11->timestamp + DHT11_DATA_VALID_TIME < iio_get_time_ns()) {
		dht11->num_edges = 0;
		dht11->completion.done = 0;
		th = kthread_create(dht11_read_thread, dht11, "dht11-read");
		if (IS_ERR(th)) {
			ret = -1;
			goto err;
		}
		dht11_read_flag = 1;
		wake_up_process(th);
		ret = wait_for_completion_killable_timeout(&dht11->completion, HZ);
		dht11_read_flag = 0;
		if(th)
			kthread_stop(th);
		if (ret == 0) {
			dev_err(&iio_dev->dev, "dht11 read raw timeout\n");
			ret = -1;
			//goto err;
		}	
		for(i=0,j=0; i<dht11->num_edges; i++) {
			if (i != 0) {
#ifdef DHT11_DEBUG
				printk(KERN_DEBUG"%02d ts=%lld value=%d pass=%lld\n", i, dht11->edges[i].ts, dht11->edges[i].value, dht11->edges[i].ts - dht11->edges[i-1].ts);
#endif
				time = dht11->edges[i].ts - dht11->edges[i-1].ts;
			}
			if (i > 3) {
				if ((i%2) == 0) {	// one bit start always about 50us
					if (time<DATA_START_FLAG_MIN || time>DATA_START_FLAG_MAX) {
						printk(KERN_ERR"bit start without 50us, time=%d\n", time);
						ret = -EINVAL;
						goto err;
					}	
				} else {
					time = dht11->edges[i].ts - dht11->edges[i-1].ts;
					if (time > DATA_ZERO_MIN && time < DATA_ZERO_MAX)
						timing[j] = 0;
					else if (time > DATA_ONE_MIN && time < DATA_ONE_MAX)
						timing[j]= 1;
					else {
						printk(KERN_ERR"bit value is not 1 or 0, time=%d\n", time);
						ret = -EINVAL;
						goto err;
					}
					j++;
				}
			}
		}

		ret = dht11_decode(dht11, timing);
		if (ret)
			goto err;
	}

	ret = IIO_VAL_INT;
	if (chan->type == IIO_TEMP)
		*val = dht11->temperature;
	else if (chan->type == IIO_HUMIDITYRELATIVE)
		*val = dht11->humidity;
	else
		ret = -EINVAL;
err:
	dht11->num_edges = -1;
	return ret;
}

static const struct iio_info dht11_iio_info = {
	.driver_module		= THIS_MODULE,
	.read_raw		= dht11_read_raw,
};

static const struct iio_chan_spec dht11_chan_spec[] = {
	{ 
		.type = IIO_TEMP,
	  	.processed_val = IIO_PROCESSED,
	},
	{ 
		.type = IIO_HUMIDITYRELATIVE,
	  	.processed_val = IIO_PROCESSED,
	}
};

static const struct of_device_id dht11_dt_ids[] = {
	{ .compatible = "dht11", },
	{ }
};
MODULE_DEVICE_TABLE(of, dht11_dt_ids);

static int dht11_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct dht11 *dht11;
	struct iio_dev *iio;
	int ret = -1;

	iio = iio_allocate_device(sizeof(*dht11));
	if (!iio) {
		dev_err(dev, "Failed to allocate IIO device\n");
		return -ENOMEM;
	}

	dht11 = iio_priv(iio);
	dht11->dev = dev;
	dht11->gpio = gpio;

	ret = gpio_request(dht11->gpio, pdev->name);
	if (ret)
		return ret;

	dht11->timestamp = iio_get_time_ns() - DHT11_DATA_VALID_TIME - 1;
	dht11->num_edges = -1;

	platform_set_drvdata(pdev, iio);

	init_completion(&dht11->completion);
	iio->name = pdev->name;
	iio->dev.parent = &pdev->dev;
	iio->info = &dht11_iio_info;
	iio->modes = INDIO_DIRECT_MODE;
	iio->channels = dht11_chan_spec;
	iio->num_channels = ARRAY_SIZE(dht11_chan_spec);

	return iio_device_register(iio);
}

static int dht11_remove(struct platform_device *pdev)
{
	struct iio_dev *iio = platform_get_drvdata(pdev);
	struct dht11 *dht11 = iio_priv(iio);

	iio_device_unregister(iio);
	gpio_free(dht11->gpio);
	iio_free_device(iio);
	return 0;
}

static struct platform_driver dht11_driver = {
	.driver = {
		.name	= DRIVER_NAME,
		.owner	= THIS_MODULE,
		.of_match_table = dht11_dt_ids,
	},
	.probe  = dht11_probe,
	.remove = dht11_remove,
};

void matrix_dht11_device_release(struct device *dev)
{
}

static struct platform_device dht11_device = {
	.name			= "dht11",
	.id				= -1,
	.num_resources	= 0,
	.dev = {        
        	.release 			= matrix_dht11_device_release,
        }
};

static int __init dht11_init(void)
{
	int ret = -1;
	
	printk("plat: add device matrix-dht11, gpio=%d\n", gpio);
	if ((ret = platform_device_register(&dht11_device))) {
		return ret;
	}

	if ((ret = platform_driver_register(&dht11_driver))) {
		platform_device_unregister(&dht11_device);
	}
	return ret;
}

static void __exit dht11_exit(void)
{
	platform_driver_unregister(&dht11_driver);
	platform_device_unregister(&dht11_device);
	gpio = -1;
}

module_init(dht11_init);
module_exit(dht11_exit);
MODULE_AUTHOR("Harald Geyer <harald@ccbib.org>");
MODULE_DESCRIPTION("DHT11 humidity/temperature sensor driver");
MODULE_LICENSE("GPL v2");
module_param(gpio, int, 0644);
