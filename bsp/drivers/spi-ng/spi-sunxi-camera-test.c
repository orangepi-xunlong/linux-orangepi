/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * SUNXI SPI Camera test Driver
 *
 */

#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/clk.h>
#include <linux/device.h>
#include <linux/sysfs.h>
#include <linux/spinlock.h>
#include <linux/completion.h>
#include <linux/interrupt.h>
#include <linux/regulator/consumer.h>
#include <linux/spi/spi.h>
#include <linux/spi/sunxi-spi.h>

#include "spi-sunxi-debug.h"

enum sunxi_spi_camera_status {
	SUNXI_SPI_CAMERA_IDLE = 0,
	SUNXI_SPI_CAMERA_PREPARE,
	SUNXI_SPI_CAMERA_RUNNING,
	SUNXI_SPI_CAMERA_ABORT,
	SUNXI_SPI_CAMERA_STOP,
};

enum sunxi_spi_camera_type {
	SUNXI_SPI_CAMERA_SW_VSYNC = 0,
	SUNXI_SPI_CAMERA_HW_VSYNC,
	SUNXI_SPI_CAMERA_HW_FRAMEHEAD,
	SUNXI_SPI_CAMERA_HW_IDLEWAIT,
};

enum sunxi_spi_camera_frame_flag_type {
	SUNXI_SPI_FRAME_FLAG_SOF = 0,
	SUNXI_SPI_FRAME_FLAG_SOL,
	SUNXI_SPI_FRAME_FLAG_EOL,
	SUNXI_SPI_FRAME_FLAG_EOF,
	SUNXI_SPI_FRAME_FLAG_MAX,
};

struct sunxi_spi_camera_framehead {
	u8 *data;
	int size;
};

struct sunxi_spi_camera_info {
	u32 pixel_h;
	u32 pixel_v;
	u32 pixel_bytes;
	enum sunxi_spi_camera_type type;
	u32 avdd_vol;
	u32 iovdd_vol;
	struct clk *mclk;
	struct regulator *avdd;
	struct regulator *iovdd;
	struct gpio_desc *pwdn_gpiod;
	struct gpio_desc *vsync_gpiod;
	int vsync_irq;
	struct sunxi_spi_camera_framehead flag[SUNXI_SPI_FRAME_FLAG_MAX];
	int framehead_len;
	u8 flag_ignore_mask[SUNXI_SPI_FRAME_FLAG_MAX];
	u32 idlewait_us;
};

struct sunxi_spi_camera_buffer {
	spinlock_t lock; /* buffer access lock */
	u8 *buf;
	u32 len;
};

struct sunxi_spi_camera_test {
	struct spi_device *spi;
	struct completion finished;
	struct spi_transfer xfer;
	struct spi_message msg;
	enum sunxi_spi_camera_status status;
	struct sunxi_spi_camera_info info;
	struct bin_attribute img_bin;
	struct bin_attribute raw_bin;
	struct sunxi_spi_camera_buffer img_buf;
	struct sunxi_spi_camera_buffer raw_buf;
};

static void sunxi_spi_camera_clear_buffer(struct sunxi_spi_camera_test *camera, struct sunxi_spi_camera_buffer *buffer)
{
	spin_lock(&buffer->lock);
	memset(buffer->buf, 0, buffer->len);
	spin_unlock(&buffer->lock);
}

static int sunxi_spi_camera_update_buffer(struct sunxi_spi_camera_test *camera, struct sunxi_spi_camera_buffer *buffer, u8 *buf, int len)
{
	struct device *dev = &camera->spi->dev;

	if (len > buffer->len) {
		dev_err(dev, "update size %d over camera buffer size %d\n", len, buffer->len);
		return -EINVAL;
	}

	spin_lock(&buffer->lock);
	if (camera->info.type == SUNXI_SPI_CAMERA_HW_FRAMEHEAD) {
		sunxi_spi_camera_get_framehead_flag(camera->spi, buffer->buf, camera->info.framehead_len);
		memcpy(buffer->buf + camera->info.framehead_len, buf, len);
	} else {
		memcpy(buffer->buf, buf, len);
	}
	spin_unlock(&buffer->lock);

	return 0;
}

static bool sunxi_spi_camera_check_frame_flag(u8 *buf, int len, struct sunxi_spi_camera_framehead *flag)
{
	int i;

	for (i = 0; i < flag->size; i++) {
		if (i > len)
			return false;
		if (buf[i] != flag->data[i])
			return false;
	}

	return true;
}

static int sunxi_spi_camera_decode_data(struct sunxi_spi_camera_test *camera, struct sunxi_spi_camera_buffer *raw_buf, struct sunxi_spi_camera_buffer *img_buf)
{
	struct device *dev = &camera->spi->dev;
	u8 *flag_ignore_mask = camera->info.flag_ignore_mask;
	u8 *buf = img_buf->buf;
	struct sunxi_spi_camera_framehead *sof = &camera->info.flag[SUNXI_SPI_FRAME_FLAG_SOF];
	struct sunxi_spi_camera_framehead *sol = &camera->info.flag[SUNXI_SPI_FRAME_FLAG_SOL];
	struct sunxi_spi_camera_framehead *eol = &camera->info.flag[SUNXI_SPI_FRAME_FLAG_EOL];
	struct sunxi_spi_camera_framehead *eof = &camera->info.flag[SUNXI_SPI_FRAME_FLAG_EOF];
	u8 *ptr_sof, *ptr_eof, *ptr_sol, *ptr_eol;
	bool has_sof, has_eof, has_sol, has_eol;
	int row, col;
	int ret;

	dev_info(dev, "buffer decode size %d start\n", img_buf->len);

	ptr_sof = raw_buf->buf;
	ptr_eof = raw_buf->buf + raw_buf->len - eof->size;
	has_sof = flag_ignore_mask[SUNXI_SPI_FRAME_FLAG_SOF] ? true : sunxi_spi_camera_check_frame_flag(ptr_sof, sof->size, sof);
	has_eof = flag_ignore_mask[SUNXI_SPI_FRAME_FLAG_EOF] ? true : sunxi_spi_camera_check_frame_flag(ptr_eof, eof->size, eof);

	col = camera->info.pixel_h * camera->info.pixel_bytes;
	if (has_sof && has_eof) {
		for (row = 0; row < camera->info.pixel_v; row++) {
			ptr_sol = raw_buf->buf + sof->size + (sol->size + col + eol->size) * row;
			ptr_eol = ptr_sol + sol->size + col;
			has_sol = flag_ignore_mask[SUNXI_SPI_FRAME_FLAG_SOL] ? true : sunxi_spi_camera_check_frame_flag(ptr_sol, sol->size, sol);
			has_eol = flag_ignore_mask[SUNXI_SPI_FRAME_FLAG_EOL] ? true : sunxi_spi_camera_check_frame_flag(ptr_eol, eol->size, eol);
			if (has_sol && has_eol) {
				memcpy(buf, ptr_sol + sol->size, col);
				buf += col;
			} else {
				dev_err(dev, "encode buf check sol(%d) eol(%d) failed, drop line %d", has_sol, has_eol, row);
				ret = false;
			}
		}
	} else {
		dev_err(dev, "encode buf check sof(%d) eof(%d) failed, drop frame\n", has_sof, has_eof);
		ret = false;
	}

	if (buf - img_buf->buf == img_buf->len) {
		dev_info(dev, "buffer decode size %d done\n", img_buf->len);
		ret = true;
	}

	return ret;
}

static void sunxi_spi_camera_test_complete(void *arg)
{
	struct sunxi_spi_camera_test *camera = (struct sunxi_spi_camera_test *)arg;
	struct device *dev = &camera->spi->dev;
	int ret;

	ret = camera->msg.status;
	if (ret) {
		switch (camera->status) {
		case SUNXI_SPI_CAMERA_ABORT:
			dev_warn(dev, "camera transfer abort\n");
			break;
		case SUNXI_SPI_CAMERA_STOP:
			dev_warn(dev, "camera transfer stop\n");
			break;
		default:
			break;
		}
		goto terminate;
	}

	dev_info(dev, "camera capture done\n");

	switch (camera->info.type) {
	case SUNXI_SPI_CAMERA_SW_VSYNC:
	case SUNXI_SPI_CAMERA_HW_VSYNC:
		sunxi_spi_camera_clear_buffer(camera, &camera->img_buf);
		sunxi_spi_camera_update_buffer(camera, &camera->img_buf, camera->xfer.rx_buf, camera->img_buf.len);
		break;
	case SUNXI_SPI_CAMERA_HW_FRAMEHEAD:
	case SUNXI_SPI_CAMERA_HW_IDLEWAIT:
		sunxi_spi_camera_clear_buffer(camera, &camera->raw_buf);
		sunxi_spi_camera_clear_buffer(camera, &camera->img_buf);
		sunxi_spi_camera_update_buffer(camera, &camera->raw_buf, camera->xfer.rx_buf, camera->raw_buf.len);
		if (!sunxi_spi_camera_decode_data(camera, &camera->raw_buf, &camera->img_buf)) {
			dev_err(dev, "decode image from raw data failed\n");
			goto terminate;
		}
		break;
	}

terminate:
	camera->status = SUNXI_SPI_CAMERA_IDLE;
	devm_kfree(dev, camera->xfer.rx_buf);
	if (camera->info.vsync_irq)
		disable_irq(camera->info.vsync_irq);
	spi_transfer_del(&camera->xfer);
	memset(&camera->xfer, 0, sizeof(camera->xfer));
	complete(&camera->finished);
}

static int sunxi_spi_camera_test_submit(struct sunxi_spi_camera_test *camera)
{
	struct device *dev = &camera->spi->dev;
	int ret;

	reinit_completion(&camera->finished);

	switch (camera->info.type) {
	case SUNXI_SPI_CAMERA_SW_VSYNC:
	case SUNXI_SPI_CAMERA_HW_VSYNC:
		camera->xfer.len = camera->img_buf.len;
		break;
	case SUNXI_SPI_CAMERA_HW_FRAMEHEAD:
		camera->xfer.len = camera->raw_buf.len - camera->info.framehead_len;
		break;
	case SUNXI_SPI_CAMERA_HW_IDLEWAIT:
		camera->xfer.len = camera->raw_buf.len;
		break;
	}
	camera->xfer.rx_buf = devm_kzalloc(dev, camera->xfer.len, GFP_KERNEL | GFP_DMA);
	if (camera->spi->mode & SPI_RX_QUAD)
		camera->xfer.rx_nbits = 4;
	else if (camera->spi->mode & SPI_RX_DUAL)
		camera->xfer.rx_nbits = 2;
	else
		camera->xfer.rx_nbits = 1;

	spi_message_init_with_transfers(&camera->msg, &camera->xfer, 1);
	camera->msg.complete = sunxi_spi_camera_test_complete;
	camera->msg.context = camera;

	ret = spi_async(camera->spi, &camera->msg);
	if (ret) {
		dev_err(dev, "spi_async failed %d\n", ret);
	}

	return ret;
}

static ssize_t sunxi_spi_camera_raw_data_read(struct file *filp, struct kobject *kobj,
		struct bin_attribute *attr, char *buf, loff_t off, size_t count)
{
	struct sunxi_spi_camera_test *camera = dev_get_drvdata(kobj_to_dev(kobj));
	struct device *dev = &camera->spi->dev;

	if (off > camera->raw_buf.len || (off + count) > camera->raw_buf.len) {
		dev_err(dev, "read size over range\n");
		return -EINVAL;
	}

	spin_lock(&camera->raw_buf.lock);
	memcpy(buf, &camera->raw_buf.buf[off], count);
	spin_unlock(&camera->raw_buf.lock);

	return count;
}

static ssize_t sunxi_spi_camera_img_data_read(struct file *filp, struct kobject *kobj,
		struct bin_attribute *attr, char *buf, loff_t off, size_t count)
{
	struct sunxi_spi_camera_test *camera = dev_get_drvdata(kobj_to_dev(kobj));
	struct device *dev = &camera->spi->dev;

	if (off > camera->img_buf.len || (off + count) > camera->img_buf.len) {
		dev_err(dev, "read size over range\n");
		return -EINVAL;
	}

	spin_lock(&camera->img_buf.lock);
	memcpy(buf, &camera->img_buf.buf[off], count);
	spin_unlock(&camera->img_buf.lock);

	return count;
}

static ssize_t sunxi_camera_abort_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct spi_device *spi = to_spi_device(dev);
	struct sunxi_spi_camera_test *camera = spi_get_drvdata(spi);

	camera->status = SUNXI_SPI_CAMERA_ABORT;
	spi_slave_abort(spi);

	return count;
}

static ssize_t sunxi_camera_capture_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct spi_device *spi = to_spi_device(dev);
	struct sunxi_spi_camera_test *camera = spi_get_drvdata(spi);

	if (camera->status != SUNXI_SPI_CAMERA_IDLE) {
		dev_warn(dev, "camera capture isn't idle\n");
		return count;
	}

	if (camera->info.vsync_irq > 0) {
		dev_info(dev, "camera capture prepare\n");
		camera->status = SUNXI_SPI_CAMERA_PREPARE;
		enable_irq(camera->info.vsync_irq);
	} else {
		dev_info(dev, "camera capture start\n");
		camera->status = SUNXI_SPI_CAMERA_RUNNING;
		sunxi_spi_camera_test_submit(camera);
	}

	return count;
}

static ssize_t sunxi_camera_pwdn_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct spi_device *spi = to_spi_device(dev);
	struct sunxi_spi_camera_test *camera = spi_get_drvdata(spi);
	static char *pwdn_state[] = { "on", "off" };

	if (IS_ERR(camera->info.pwdn_gpiod)) {
		dev_err(dev, "camera pwdn gpio not exist\n");
		return -EINVAL;
	}

	if (strncmp(buf, pwdn_state[0], strlen(pwdn_state[0])) == 0) {
		gpiod_set_value(camera->info.pwdn_gpiod, true);
		dev_info(dev, "spi camera power on\n");
	} else if (strncmp(buf, pwdn_state[1], strlen(pwdn_state[1])) == 0) {
		gpiod_set_value(camera->info.pwdn_gpiod, false);
		dev_info(dev, "spi camera power off\n");
	} else {
		dev_err(dev, "pwdn argument %s error\n", buf);
		return -EINVAL;
	}

	return count;
}

static struct device_attribute sunxi_spi_camera_debug_attr[] = {
	__ATTR(abort, 0200, NULL, sunxi_camera_abort_store),
	__ATTR(capture, 0200, NULL, sunxi_camera_capture_store),
	__ATTR(pwdn, 0200, NULL, sunxi_camera_pwdn_store),
};

static void sunxi_spi_camera_create_sysfs(struct sunxi_spi_camera_test *camera)
{
	int i;
	for (i = 0; i < ARRAY_SIZE(sunxi_spi_camera_debug_attr); i++)
		device_create_file(&camera->spi->dev, &sunxi_spi_camera_debug_attr[i]);
}

static void sunxi_spi_camera_remove_sysfs(struct sunxi_spi_camera_test *camera)
{
	int i;
	for (i = 0; i < ARRAY_SIZE(sunxi_spi_camera_debug_attr); i++)
		device_remove_file(&camera->spi->dev, &sunxi_spi_camera_debug_attr[i]);
}

static irqreturn_t sunxi_camera_vsync_handler(int irq, void *dev_id)
{
	struct sunxi_spi_camera_test *camera = (struct sunxi_spi_camera_test *)dev_id;
	struct device *dev = &camera->spi->dev;

	if (camera->status == SUNXI_SPI_CAMERA_PREPARE) {
		dev_info(dev, "camera capture start\n");
		camera->status = SUNXI_SPI_CAMERA_RUNNING;
		sunxi_spi_camera_test_submit(camera);
	}

	return IRQ_HANDLED;
}

static int sunxi_spi_camera_flag_property_get(struct sunxi_spi_camera_test *camera, struct sunxi_spi_camera_framehead *flag, char *name)
{
	struct device *dev = &camera->spi->dev;
	struct device_node *np = dev->of_node;
	char tmp[128];
	int ret;

	flag->size = of_property_count_u8_elems(np, name);
	if (flag->size > 0) {
		flag->data = devm_kzalloc(dev, flag->size, GFP_KERNEL);
		ret = of_property_read_u8_array(np, name, flag->data, flag->size);
		if (ret < 0) {
			dev_err(dev, "failed to get %s size %d\n", name, flag->size);
			return -EINVAL;
		}
		hex_dump_to_buffer(flag->data, flag->size, flag->size, 1, tmp, sizeof(tmp), false);
		dev_info(dev, "get flag %s: %s\n", name, tmp);
	} else {
		flag->size = 0;
	}

	return 0;
}

static int sunxi_spi_camera_hw_init(struct sunxi_spi_camera_test *camera)
{
	struct device *dev = &camera->spi->dev;
	int ret = 0;

	if (camera->info.avdd_vol > 0) {
		regulator_set_voltage(camera->info.avdd, camera->info.avdd_vol, camera->info.avdd_vol);
		ret = regulator_enable(camera->info.avdd);
		if (ret) {
			dev_err(dev, "regulator avdd enable %d uV failed %d\n", camera->info.avdd_vol, ret);
			goto err_avdd;
		}
		dev_info(dev, "regulator avdd enable %d uV\n", camera->info.avdd_vol);
	}

	if (camera->info.iovdd_vol > 0) {
		regulator_set_voltage(camera->info.iovdd, camera->info.iovdd_vol, camera->info.iovdd_vol);
		ret = regulator_enable(camera->info.iovdd);
		if (ret) {
			dev_err(dev, "regulator iovdd enable %d uV failed %d\n", camera->info.iovdd_vol, ret);
			goto err_iovdd;
		}
		dev_info(dev, "regulator iovdd enable %d uV\n", camera->info.iovdd_vol);
	}

	if (camera->info.mclk) {
		ret = clk_prepare_enable(camera->info.mclk);
		if (ret) {
			dev_err(dev, "failed to enable mclk %d\n", ret);
			goto err_mclk;
		}
		dev_info(dev, "enable mclk freq %ld\n", clk_get_rate(camera->info.mclk));
	}

	switch (camera->info.type) {
	case SUNXI_SPI_CAMERA_HW_VSYNC:
		sunxi_spi_camera_set_mode(camera->spi, SUNXI_SPI_CAMERA_VSYNC);
		ret = sunxi_spi_camera_set_vsync(camera->spi, camera->img_buf.len);
		break;
	case SUNXI_SPI_CAMERA_HW_FRAMEHEAD:
		sunxi_spi_camera_set_mode(camera->spi, SUNXI_SPI_CAMERA_FRAMEHEAD);
		ret = sunxi_spi_camera_set_framehead_flag(camera->spi, camera->info.flag[SUNXI_SPI_FRAME_FLAG_SOF].data, camera->info.flag[SUNXI_SPI_FRAME_FLAG_SOF].size);
		break;
	case SUNXI_SPI_CAMERA_HW_IDLEWAIT:
		sunxi_spi_camera_set_mode(camera->spi, SUNXI_SPI_CAMERA_IDLEWAIT);
		ret = sunxi_spi_camera_set_idlewait_us(camera->spi, camera->info.idlewait_us);
		break;
	default:
		break;
	}

	return ret;
err_mclk:
	if (camera->info.iovdd_vol > 0)
		regulator_disable(camera->info.iovdd);
err_iovdd:
	if (camera->info.avdd_vol > 0)
		regulator_disable(camera->info.avdd);
err_avdd:
	return ret;
}

static void sunxi_spi_camera_hw_exit(struct sunxi_spi_camera_test *camera)
{
	if (camera->info.mclk)
		clk_disable_unprepare(camera->info.mclk);
	if (camera->info.iovdd_vol > 0)
		regulator_disable(camera->info.iovdd);
	if (camera->info.avdd_vol > 0)
		regulator_disable(camera->info.avdd);
}

static int sunxi_spi_camera_test_probe(struct spi_device *spi)
{
	struct sunxi_spi_camera_test *camera;
	struct device *dev = &spi->dev;
	char *gpioname;
	char *irqname;
	int ret;

	camera = devm_kzalloc(dev, sizeof(*camera), GFP_KERNEL);
	if (!camera)
		return -ENOMEM;

	camera->spi = spi;
	init_completion(&camera->finished);
	spin_lock_init(&camera->img_buf.lock);
	spin_lock_init(&camera->raw_buf.lock);

	of_property_read_u32(dev->of_node, "pixel-horizontal", &camera->info.pixel_h);
	of_property_read_u32(dev->of_node, "pixel-vertical", &camera->info.pixel_v);
	of_property_read_u32(dev->of_node, "pixel-bytes", &camera->info.pixel_bytes);
	if (camera->info.pixel_h && camera->info.pixel_v && camera->info.pixel_bytes) {
		dev_info(dev, "get pixel h_%d v_%d bytes_%d\n", camera->info.pixel_h, camera->info.pixel_v, camera->info.pixel_bytes);
	} else {
		dev_err(dev, "failed to get pixel property h_%d v_%d bytes_%d\n", camera->info.pixel_h, camera->info.pixel_v, camera->info.pixel_bytes);
		return -EINVAL;
	}

	camera->info.mclk = devm_clk_get_optional(dev, "mclk");
	if (IS_ERR(camera->info.mclk)) {
		ret = PTR_ERR(camera->info.mclk);
		dev_err(dev, "failed to get mclk %d\n", ret);
		return -EINVAL;
	}

	camera->info.avdd = devm_regulator_get_optional(dev, "avdd");
	of_property_read_u32(dev->of_node, "avdd-microvolt", &camera->info.avdd_vol);
	camera->info.iovdd = devm_regulator_get_optional(dev, "iovdd");
	of_property_read_u32(dev->of_node, "iovdd-microvolt", &camera->info.iovdd_vol);

	camera->info.pwdn_gpiod = devm_gpiod_get(dev, "pwdn", GPIOD_ASIS);
	if (!IS_ERR(camera->info.pwdn_gpiod)) {
		dev_info(dev, "camera get pwdn gpio %d\n", desc_to_gpio(camera->info.pwdn_gpiod));
		gpioname = devm_kasprintf(dev, GFP_KERNEL, "%s pwdn", dev_name(dev));
		gpiod_set_consumer_name(camera->info.pwdn_gpiod, gpioname);
		gpiod_direction_output(camera->info.pwdn_gpiod, false);
	}

	of_property_read_u32(dev->of_node, "camera-type", &camera->info.type);
	dev_info(dev, "camera get type %d\n", camera->info.type);
	switch (camera->info.type) {
	case SUNXI_SPI_CAMERA_SW_VSYNC:
		camera->info.vsync_gpiod = devm_gpiod_get(dev, "vsync", GPIOD_ASIS);
		if (!IS_ERR(camera->info.vsync_gpiod)) {
			dev_info(dev, "camera get vsync gpio %d\n", desc_to_gpio(camera->info.vsync_gpiod));
			camera->info.vsync_irq = gpiod_to_irq(camera->info.vsync_gpiod);
			irqname = devm_kasprintf(dev, GFP_KERNEL, "%s vsync", dev_name(dev));
			ret = devm_request_irq(dev, camera->info.vsync_irq, sunxi_camera_vsync_handler, gpiod_is_active_low(camera->info.vsync_gpiod) ? IRQ_TYPE_EDGE_FALLING : IRQ_TYPE_EDGE_RISING, irqname, camera);
			if (ret) {
				dev_err(dev, "failed request vsync irq_%d %d\n", camera->info.vsync_irq, ret);
				return -EINVAL;
			}
			disable_irq(camera->info.vsync_irq);
		} else {
			dev_err(dev, "need software vsync gpio\n");
			return -EINVAL;
		}
		break;
	case SUNXI_SPI_CAMERA_HW_VSYNC:
		break;
	case SUNXI_SPI_CAMERA_HW_IDLEWAIT:
		of_property_read_u32(dev->of_node, "idlewait-us", &camera->info.idlewait_us);
		dev_info(dev, "get idlewait-us %d\n", camera->info.idlewait_us);
		fallthrough;
	case SUNXI_SPI_CAMERA_HW_FRAMEHEAD:
		sunxi_spi_camera_flag_property_get(camera, &camera->info.flag[SUNXI_SPI_FRAME_FLAG_SOF], "start-of-frame");
		sunxi_spi_camera_flag_property_get(camera, &camera->info.flag[SUNXI_SPI_FRAME_FLAG_SOL], "start-of-line");
		sunxi_spi_camera_flag_property_get(camera, &camera->info.flag[SUNXI_SPI_FRAME_FLAG_EOL], "end-of-line");
		sunxi_spi_camera_flag_property_get(camera, &camera->info.flag[SUNXI_SPI_FRAME_FLAG_EOF], "end-of-frame");
		camera->info.framehead_len = camera->info.flag[SUNXI_SPI_FRAME_FLAG_SOF].size;
		ret = of_property_read_u8_array(dev->of_node, "flag-ignore-mask", camera->info.flag_ignore_mask, SUNXI_SPI_FRAME_FLAG_MAX);
		if (!ret) {
			char tmp[128] = { 0 };
			hex_dump_to_buffer(camera->info.flag_ignore_mask, SUNXI_SPI_FRAME_FLAG_MAX, SUNXI_SPI_FRAME_FLAG_MAX, 1, tmp, sizeof(tmp), false);
			dev_info(dev, "get flag-ignore-mask <%s>\n", tmp);
		}
		camera->raw_buf.len = camera->info.flag[SUNXI_SPI_FRAME_FLAG_SOF].size + camera->info.flag[SUNXI_SPI_FRAME_FLAG_EOF].size +
					(camera->info.flag[SUNXI_SPI_FRAME_FLAG_SOL].size + (camera->info.pixel_h * camera->info.pixel_bytes) + camera->info.flag[SUNXI_SPI_FRAME_FLAG_EOL].size) * camera->info.pixel_v;
		camera->raw_buf.buf = devm_kzalloc(dev, camera->raw_buf.len, GFP_KERNEL);
		if (!camera->raw_buf.buf) {
			dev_err(dev, "alloc raw buffer %d failed\n", camera->raw_buf.len);
			return -ENOMEM;
		}
		dev_info(dev, "raw buffer len %d\n", camera->raw_buf.len);
		sysfs_bin_attr_init(&camera->raw_bin);
		camera->raw_bin.attr.name = "sunxi-camera-test-raw";
		camera->raw_bin.attr.mode = S_IRUSR;
		camera->raw_bin.read = sunxi_spi_camera_raw_data_read;
		camera->raw_bin.size = camera->raw_buf.len;
		ret = sysfs_create_bin_file(&spi->dev.kobj, &camera->raw_bin);
		if (ret)
			return ret;
		break;
	default:
		dev_err(dev, "unsupport camera type %d\n", camera->info.type);
		return -EINVAL;
	}

	camera->img_buf.len = camera->info.pixel_h * camera->info.pixel_v * camera->info.pixel_bytes;
	camera->img_buf.buf = devm_kzalloc(dev, camera->img_buf.len, GFP_KERNEL);
	if (!camera->img_buf.buf) {
		dev_err(dev, "alloc img buffer %d failed\n", camera->img_buf.len);
		return -ENOMEM;
	}
	dev_info(dev, "img buffer len %d\n", camera->img_buf.len);
	sysfs_bin_attr_init(&camera->img_bin);
	camera->img_bin.attr.name = "sunxi-camera-test-img";
	camera->img_bin.attr.mode = S_IRUSR;
	camera->img_bin.read = sunxi_spi_camera_img_data_read;
	camera->img_bin.size = camera->img_buf.len;
	ret = sysfs_create_bin_file(&spi->dev.kobj, &camera->img_bin);
	if (ret)
		goto err;

	spi_set_drvdata(spi, camera);

	sunxi_spi_camera_hw_init(camera);

	sunxi_spi_camera_create_sysfs(camera);

	camera->status = SUNXI_SPI_CAMERA_IDLE;

	return 0;
err:
	if (camera->info.type == SUNXI_SPI_CAMERA_HW_IDLEWAIT ||
		camera->info.type == SUNXI_SPI_CAMERA_HW_FRAMEHEAD)
		sysfs_remove_bin_file(&spi->dev.kobj, &camera->raw_bin);
	return ret;
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 18, 0))
static void sunxi_spi_camera_test_remove(struct spi_device *spi)
#else
static int sunxi_spi_camera_test_remove(struct spi_device *spi)
#endif
{
	struct sunxi_spi_camera_test *camera = spi_get_drvdata(spi);

	if (camera->status == SUNXI_SPI_CAMERA_RUNNING) {
		camera->status = SUNXI_SPI_CAMERA_STOP;
		spi_slave_abort(spi);
		wait_for_completion(&camera->finished);
	}

	if (!IS_ERR(camera->info.pwdn_gpiod))
		gpiod_set_value(camera->info.pwdn_gpiod, false);
	sunxi_spi_camera_remove_sysfs(camera);
	sunxi_spi_camera_hw_exit(camera);
	sysfs_remove_bin_file(&spi->dev.kobj, &camera->img_bin);
	if (camera->info.type & (SUNXI_SPI_CAMERA_HW_IDLEWAIT | SUNXI_SPI_CAMERA_HW_FRAMEHEAD))
		sysfs_remove_bin_file(&spi->dev.kobj, &camera->raw_bin);

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 18, 0))
#else
	return 0;
#endif
}

static const struct spi_device_id sunxi_spicamera_ids[] = {
	{ .name = "spicamera" },
	{},
};
MODULE_DEVICE_TABLE(spi, sunxi_spicamera_ids);

static const struct of_device_id sunxi_spicamera_dt_ids[] = {
	{ .compatible = "sunxi,spicamera" },
	{},
};
MODULE_DEVICE_TABLE(of, sunxi_spicamera_dt_ids);

static struct spi_driver sunxi_spi_camera_test_driver = {
	.driver = {
		.name	= "sunxi-spi-camera",
		.owner	= THIS_MODULE,
		.of_match_table = sunxi_spicamera_dt_ids,
	},
	.probe  = sunxi_spi_camera_test_probe,
	.remove = sunxi_spi_camera_test_remove,
	.id_table   = sunxi_spicamera_ids,
};
module_spi_driver(sunxi_spi_camera_test_driver);

MODULE_AUTHOR("jingyanliang <jingyanliang@allwinnertech.com>");
MODULE_DESCRIPTION("SUNXI SPI camera test driver");
MODULE_VERSION("1.0.2");
MODULE_LICENSE("GPL");
MODULE_ALIAS("spi:sunxi-camera-test");
