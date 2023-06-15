/*
 * drivers/input/touchscreen/gslX680.c
 *
 * Copyright (c) 2012 Shanghai Basewin
 *	Guan Yuwei<guanyuwei@basewin.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 *
 * history:
 *	mbgalex@163.com_2013-07-16_14:12
 *	add tp for Q790 OGS project ,tp modules is EC8031-01
 */

#include <linux/module.h>
#include <linux/slab.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/jiffies.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/pm_runtime.h>
#include <linux/firmware.h>

#include <linux/pm_runtime.h>


#include <linux/input/mt.h>

#include <linux/i2c.h>
#include <linux/input.h>

#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/async.h>
#include <linux/hrtimer.h>
#include <linux/init.h>
#include <linux/ioport.h>
#include <asm/irq.h>
#include <linux/io.h>
#include <linux/device.h>

#include <linux/gpio.h>

#include "../../init-input.h"

#include "gslX680.h"

/* open for debug */
/*
#ifdef pr_debug
#undef pr_debug
#define pr_debug pr_info
#endif
*/

struct gslX680_fw_array {
	char name[64];
	unsigned int size;
	const struct fw_data *fw;
} gslx680_fw_grp;

unsigned int *gslX680_config_data;
static int ldo_state;


#ifdef TPD_PROC_DEBUG
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
static struct proc_dir_entry *gsl_config_proc;
#define GSL_CONFIG_PROC_FILE "gsl_config"
#define CONFIG_LEN 31
static char gsl_read[CONFIG_LEN];
static u8 gsl_data_proc[8] = {0};
static u8 gsl_proc_flag;
static unsigned int gsl_config_data_id[256];
#endif

#define GSL_DATA_REG		0x80
#define GSL_STATUS_REG		0xe0
#define GSL_PAGE_REG		0xf0

#define PRESS_MAX		255
#define MAX_FINGERS		5
#define MAX_CONTACTS		10
#define DMA_TRANS_LEN		0x20

#define PHO_CFG2_OFFSET		(0X104)
#define PHO_DAT_OFFSET		(0X10C)
#define PHO_PULL1_OFFSET	(0X11C)
#define GPIOF_CON		0x7f0080a0
#define GPIOF_DAT		0x7f0080a4
#define GPIOF_PUD		0x7f0080a8

#define GSL_NOID_VERSION
#ifdef GSL_MONITOR
static struct delayed_work gsl_monitor_work;
static struct workqueue_struct *gsl_monitor_workqueue;
static char int_1st[4] = {0};
static char int_2nd[4] = {0};
#endif

static struct input_dev *gDevice;

#ifdef HAVE_TOUCH_KEY
static u16 key;
static int key_state_flag;
struct key_data {
	u16 key;
	u16 x_min;
	u16 x_max;
	u16 y_min;
	u16 y_max;
};

const u16 key_array[] = {
	KEY_BACK,
	KEY_HOME,
	KEY_MENU,
	KEY_SEARCH,
};
#define MAX_KEY_NUM		(ARRAY_SIZE(key_array))

struct key_data gsl_key_data[MAX_KEY_NUM] = {
	{KEY_BACK, 816, 836, 115, 125},
	{KEY_HOME, 816, 836, 259, 269},
	{KEY_MENU, 816, 836, 398, 410},
	{KEY_SEARCH, 2048, 2048, 2048, 2048},
};
#endif

struct gsl_ts_data {
	u8 x_index;
	u8 y_index;
	u8 z_index;
	u8 id_index;
	u8 touch_index;
	u8 data_reg;
	u8 status_reg;
	u8 data_size;
	u8 touch_bytes;
	u8 update_data;
	u8 touch_meta_data;
	u8 finger_size;
};

static struct gsl_ts_data devices[] = {
	{
		.x_index = 6,
		.y_index = 4,
		.z_index = 5,
		.id_index = 7,
		.data_reg = GSL_DATA_REG,
		.status_reg = GSL_STATUS_REG,
		.update_data = 0x4,
		.touch_bytes = 4,
		.touch_meta_data = 4,
		.finger_size = 70,
	},
};

struct gsl_ts {
	struct i2c_client *client;
	struct input_dev *input;
	struct work_struct work;
	struct workqueue_struct *wq;
	struct gsl_ts_data *dd;
	u8 *touch_data;
	u8 device_id;
	u8 prev_touches;
	bool is_suspended;
	bool is_runtime_suspend;
	bool try_to_runtime_suspend;
	bool try_to_runtime_resume;
	bool int_pending;
	struct mutex sus_lock;
	int irq;
#ifdef GSL_TIMER
	struct timer_list gsl_timer;
#endif

};

static u32 id_sign[MAX_CONTACTS+1] = {0};
static u8 id_state_flag[MAX_CONTACTS+1] = {0};
static u8 id_state_old_flag[MAX_CONTACTS+1] = {0};
static u16 x_old[MAX_CONTACTS+1] = {0};
static u16 y_old[MAX_CONTACTS+1] = {0};
static u16 x_new;
static u16 y_new;

/* specific tp related macro: need be configured for specific tp */

#define GSLX680_I2C_NAME		"gslX680"

#define CTP_IRQ_NUMBER			(config_info.int_number)
#define CTP_IRQ_MODE			(IRQF_TRIGGER_FALLING)
#define CTP_NAME			GSLX680_I2C_NAME
#define SCREEN_MAX_X			(screen_max_x)
#define SCREEN_MAX_Y			(screen_max_y)

static const char *fwname;


#define GSLX680_I2C_ADDR	0x40

#define GSL_MONITOR_TIMER_ENABLE 		1
#ifdef GSL_TIMER
#undef GSL_MONITOR_TIMER_ENABLE
#define GSL_MONITOR_TIMER_ENABLE 		0
#endif

#if GSL_MONITOR_TIMER_ENABLE
static int gTestMonitor;
struct timer_list monitor_timer;
static void glsX680_monitor_events(struct work_struct *work);
static DECLARE_WORK(glsX680_monitor_work, glsX680_monitor_events);
#endif

static int screen_max_x;
static int screen_max_y;
static int revert_x_flag;
static int revert_y_flag;
static int exchange_x_y_flag;

struct ctp_config_info config_info = {
	.input_type = CTP_TYPE,
	.name = NULL,
	.int_number = 0,
};

static __u32 twi_id;

static const unsigned short normal_i2c[2] = {GSLX680_I2C_ADDR, I2C_CLIENT_END};
static int ctp_get_system_config(void);
static void glsX680_init_events(struct work_struct *work);
static void glsX680_resume_events(struct work_struct *work);
static void glsX680_idle_events(struct work_struct *work);
struct workqueue_struct *gslX680_wq;
struct workqueue_struct *gslX680_resume_wq;
static DECLARE_WORK(glsX680_init_work, glsX680_init_events);
static DECLARE_WORK(glsX680_resume_work, glsX680_resume_events);
static DECLARE_WORK(glsX680_idle_work, glsX680_idle_events);
struct i2c_client *glsX680_i2c;
struct gsl_ts *ts_init;


int ctp_i2c_write_bytes(struct i2c_client *client, uint8_t *data, uint16_t len)
{
	struct i2c_msg msg;
	int ret = -1;

	msg.flags = !I2C_M_RD;
	msg.addr = client->addr;
	msg.len = len;
	msg.buf = data;

	ret = i2c_transfer(client->adapter, &msg, 1);

	return ret;
}

bool ctp_i2c_test(struct i2c_client *client)
{
	int ret, retry;
	uint8_t test_data[1] = {0};

	for (retry = 0; retry < 12; retry++) {
		ret = ctp_i2c_write_bytes(client, test_data, 1);
		if (ret == 1)
			break;
		msleep(50);
	}

	return ret == 1 ? true : false;
}

static int ctp_detect(struct i2c_client *client, struct i2c_board_info *info)
{
	struct i2c_adapter *adapter = client->adapter;
	int ret = -ENODEV;

	if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE_DATA))
		return ret;

	if (twi_id == adapter->nr) {
		pr_info("%s: addr= %x\n", __func__, client->addr);
		if (ctp_i2c_test(client) != 0) {
			ret = 0;
			pr_info("I2C connection success!\n");
			strlcpy(info->type, CTP_NAME, I2C_NAME_SIZE);
		} else
			pr_info("%s:I2C connection might be something wrong\n",
								__func__);
	}
	return ret;
}
/**
 * ctp_print_info - sysconfig print function
 * return value:
 *
 */
static void ctp_print_info(struct ctp_config_info info)
{
	pr_debug("info.ctp_used:%d\n", info.ctp_used);
	pr_debug("info.ctp_name:%s\n", info.name);
	pr_debug("info.twi_id:%d\n", info.twi_id);
	pr_debug("info.screen_max_x:%d\n", info.screen_max_x);
	pr_debug("info.screen_max_y:%d\n", info.screen_max_y);
	pr_debug("info.revert_x_flag:%d\n", info.revert_x_flag);
	pr_debug("info.revert_y_flag:%d\n", info.revert_y_flag);
	pr_debug("info.exchange_x_y_flag:%d\n", info.exchange_x_y_flag);
	pr_debug("info.irq_gpio_number:%d\n", info.irq_gpio.gpio);
	pr_debug("info.wakeup_gpio_number:%d\n", info.wakeup_gpio.gpio);
}

/**
 * ctp_wakeup - function
 *
 */
static int ctp_wakeup(int status, int ms)
{
	pr_debug("***CTP*** %s:status:%d,ms = %d\n", __func__, status, ms);

	if (status == 0) {

		if (ms == 0) {
			__gpio_set_value(config_info.wakeup_gpio.gpio, 0);
		} else {
			__gpio_set_value(config_info.wakeup_gpio.gpio, 0);
			msleep(ms);
			__gpio_set_value(config_info.wakeup_gpio.gpio, 1);
		}
	}
	if (status == 1) {
		if (ms == 0) {
			__gpio_set_value(config_info.wakeup_gpio.gpio, 1);
		} else {
			__gpio_set_value(config_info.wakeup_gpio.gpio, 1);
			msleep(ms);
			__gpio_set_value(config_info.wakeup_gpio.gpio, 0);
		}
	}
	usleep_range(5000, 6000);

	return 0;
}

static int gslX680_chip_init(void)
{
	 ctp_wakeup(1, 0);
	 msleep(20);
	 return 0;
}

static int gslX680_shutdown_low(void)
{
	ctp_wakeup(0, 0);
	return 0;
}

static int gslX680_shutdown_high(void)
{
	ctp_wakeup(1, 0);
	return 0;
}

static inline u16 join_bytes(u8 a, u8 b)
{
	u16 ab = 0;

	ab = ab | a;
	ab = ab << 8 | b;
	return ab;
}

#if 0
static u32 gsl_read_interface(struct i2c_client *client, u8 reg, u8 *buf, u32 num)
{
	struct i2c_msg xfer_msg[2];

	xfer_msg[0].addr = client->addr;
	xfer_msg[0].len = 1;
	xfer_msg[0].flags = client->flags & I2C_M_TEN;
	xfer_msg[0].buf = &reg;

	xfer_msg[1].addr = client->addr;
	xfer_msg[1].len = num;
	xfer_msg[1].flags |= I2C_M_RD;
	xfer_msg[1].buf = buf;

	if (reg < 0x80) {
		i2c_transfer(client->adapter, xfer_msg, ARRAY_SIZE(xfer_msg));
		usleep_range(5000, 6000);
	}

	return i2c_transfer(client->adapter, xfer_msg,
		ARRAY_SIZE(xfer_msg)) == ARRAY_SIZE(xfer_msg) ? 0 : -EFAULT;
}
#endif

static u32 gsl_write_interface(struct i2c_client *client, const u8 reg,
							u8 *buf, u32 num)
{
	struct i2c_msg xfer_msg[1];

	buf[0] = reg;

	xfer_msg[0].addr = client->addr;
	xfer_msg[0].len = num + 1;
	xfer_msg[0].flags = client->flags & I2C_M_TEN;
	xfer_msg[0].buf = buf;

	return i2c_transfer(client->adapter, xfer_msg, 1) == 1 ? 0 : -EFAULT;
}

static int gsl_ts_write(struct i2c_client *client, u8 addr,
				u8 *pdata, int datalen)
{
	int ret = 0;
	u8 tmp_buf[128];
	unsigned int bytelen = 0;

	if (datalen > 125) {
		pr_err("%s too big datalen = %d!\n", __func__, datalen);
		return -1;
	}

	tmp_buf[0] = addr;
	bytelen++;

	if (datalen != 0 && pdata != NULL) {
		memcpy(&tmp_buf[bytelen], pdata, datalen);
		bytelen += datalen;
	}

	ret = i2c_master_send(client, tmp_buf, bytelen);

	return ret;
}

static int gsl_ts_read(struct i2c_client *client, u8 addr, u8 *pdata,
							unsigned int datalen)
{
	int ret = 0;

	if (datalen > 126) {
		pr_err("%s too big datalen = %d!\n", __func__, datalen);
		return -1;
	}

	ret = gsl_ts_write(client, addr, NULL, 0);
	if (ret < 0) {
		pr_err("%s set data address fail!\n", __func__);
		return ret;
	}

	return i2c_master_recv(client, pdata, datalen);
}

static ssize_t gslX680_reg_show(
	struct device *dev,
	struct device_attribute *attr,
	char *buf)
{
	 u8 mem_buf[4]  = {0};
	 u8 int_buf[4]  = {0};
	 u8 power_buf[4]  = {0};
	 u8 point_buf = 0;

	 gsl_ts_read(ts_init->client, 0xb0, mem_buf, sizeof(mem_buf));
	 pr_debug("check mem read 0xb0  = %x %x %x %x\n",
			  mem_buf[3], mem_buf[2], mem_buf[1], mem_buf[0]);
	 gsl_ts_read(ts_init->client, 0xb4, int_buf, sizeof(int_buf));
	 pr_debug("int  num  read  0xb4  = %d\n",
			   (int_buf[3]<<24) | (int_buf[2]<<16)
			   | (int_buf[1]<<8) | int_buf[0]);
	 gsl_ts_read(ts_init->client, 0xbc, power_buf, sizeof(power_buf));
	 pr_debug("power check read 0xbc = %4x\n",
			   (power_buf[3]<<24) | (power_buf[2]<<16)
			   | (power_buf[1]<<8) | power_buf[0]);
	 gsl_ts_read(ts_init->client, 0x80, &point_buf, 1);
	 pr_debug("point count read  0x80 = %d\n", point_buf);

	 return sprintf(buf, "[check mem read = 0x%4x ]  [int num read = %d ]  [power check read = 0x%4x ]  [point count read = %d ]\n",
		(mem_buf[3]<<24) | (mem_buf[2]<<16) | (mem_buf[1]<<8) | mem_buf[0],
		(int_buf[3]<<24) | (int_buf[2]<<16) | (int_buf[1]<<8) | int_buf[0],
		(power_buf[3]<<24) | (power_buf[2]<<16) | (power_buf[1]<<8)
		| power_buf[0], point_buf);
}

static DEVICE_ATTR(debug_reg, 0444, gslX680_reg_show, NULL);

static inline void fw2buf(u8 *buf, const u32 *fw)
{
	u32 *u32_buf = (int *)buf;
	*u32_buf = *fw;
}

static int gsl_getfw_from_file(void)
{
	u32 size = 0;
	u32 len;
	int err;
	int idx = 0;
	const u8 *data;
	const struct firmware *fw = NULL;
	char fw_name[128];

	pr_debug("==================getfw from file==================\n");

	snprintf(fw_name, 128, "gsl_firmware/%s.bin", config_info.name);

	err = request_firmware_direct(&fw, fw_name, config_info.dev);
	if (err) {
		pr_err("can not get fw from file %s\n", fw_name);
		return -1;
	}
	len = fw->size;
	data = fw->data;

	// read gslx680_fw_grp.fw size
	memcpy(&size, &data[idx], sizeof(size));
	gslx680_fw_grp.size = size;
	gslx680_fw_grp.fw = kmalloc(size * sizeof (struct fw_data), GFP_KERNEL);
	if (!gslx680_fw_grp.fw) {
		pr_err("gslx680_fw_grp.fw malloc failed\n");
		return -1;
	}
	idx += sizeof(size);
	// read gslx680_fw_grp.fw
	memcpy((void *)gslx680_fw_grp.fw, &data[idx], size * sizeof (struct fw_data));
	idx += (size * sizeof(struct fw_data));
	size = 0;
	// read gslX680_config_data size
	memcpy(&size, &data[idx], sizeof(size));
	idx += sizeof(size);
	gslX680_config_data = kmalloc(size * sizeof(unsigned int), GFP_KERNEL);
	if (!gslX680_config_data) {
		pr_err("gslX680_config_data malloc failed\n");
		return -1;
	}
	memcpy(gslX680_config_data, &data[idx], size * sizeof(unsigned int));
	idx += (size * sizeof(unsigned int));

	// read gslx680_fw_grp.name size
	memcpy(&size, &data[idx], sizeof(size));
	idx += sizeof(size);
	// read gslx680_fw_grp.name
	memcpy(gslx680_fw_grp.name, &data[idx], size);
	idx += size;
	size = 0;
	if (idx != len) {
		err = -1;
		pr_err("data size is not match fw size: data size = %d, fw size = %u\n", idx, len);
	}
	if (strcmp(gslx680_fw_grp.name, config_info.name)) {
		err = -1;
		pr_err("firmware name not match, fw name = %s, config name = %s\n", gslx680_fw_grp.name, config_info.name);
	}

	release_firmware(fw);

	return err;
}


static void gsl_load_fw(struct i2c_client *client)
{
	u8 buf[DMA_TRANS_LEN*4 + 1] = {0};
	u8 send_flag = 1;
	u8 *cur = buf + 1;
	u32 source_line = 0;
	u32 source_len;
	const struct fw_data *ptr_fw;

	pr_debug("=============gsl_load_fw start==============\n");

	ptr_fw = gslx680_fw_grp.fw;
	source_len = gslx680_fw_grp.size;

	for (source_line = 0; source_line < source_len; source_line++) {
		/* init page trans, set the page val */
		if (ptr_fw[source_line].offset == GSL_PAGE_REG) {
			fw2buf(cur, &ptr_fw[source_line].val);
			gsl_write_interface(client, GSL_PAGE_REG, buf, 4);
			send_flag = 1;
		} else {
			if (send_flag % (DMA_TRANS_LEN < 0x20 ? DMA_TRANS_LEN : 0x20) == 1)
				buf[0] = (u8)ptr_fw[source_line].offset;

			fw2buf(cur, &ptr_fw[source_line].val);
			cur += 4;

			if (send_flag % (DMA_TRANS_LEN < 0x20 ? DMA_TRANS_LEN : 0x20) == 0) {
				gsl_write_interface(client, buf[0],
							buf, cur - buf - 1);
				cur = buf + 1;
			}
			send_flag++;
		}
	}

	pr_debug("=============gsl_load_fw end==============\n");
}

static void startup_chip(struct i2c_client *client)
{
	u8 tmp = 0x00;

#ifdef GSL_NOID_VERSION
	gsl_DataInit(gslX680_config_data);
#endif
	gsl_ts_write(client, 0xe0, &tmp, 1);
	usleep_range(10000, 11000);
}

static void reset_chip(struct i2c_client *client)
{
	u8 tmp = 0x88;
	u8 buf[4] = {0x00};

	gslX680_shutdown_low();
	usleep_range(10000, 11000);
	gslX680_shutdown_high();
	usleep_range(10000, 11000);
	gsl_ts_write(client, 0xe0, &tmp, sizeof(tmp));
	usleep_range(10000, 11000);

	tmp = 0x04;
	gsl_ts_write(client, 0xe4, &tmp, sizeof(tmp));
	usleep_range(10000, 11000);
	gsl_ts_write(client, 0xbc, buf, sizeof(buf));
	usleep_range(10000, 11000);
}

static void clr_reg(struct i2c_client *client)
{
	u8 write_buf[4]	= {0};

	write_buf[0] = 0x88;
	gsl_ts_write(client, 0xe0, &write_buf[0], 1);
	msleep(20);
	write_buf[0] = 0x03;
	gsl_ts_write(client, 0x80, &write_buf[0], 1);
	usleep_range(5000, 6000);
	write_buf[0] = 0x04;
	gsl_ts_write(client, 0xe4, &write_buf[0], 1);
	usleep_range(5000, 6000);
	write_buf[0] = 0x00;
	gsl_ts_write(client, 0xe0, &write_buf[0], 1);
	msleep(20);
}

static void init_chip(struct i2c_client *client)
{
	gslX680_shutdown_low();
	msleep(50);
	gslX680_shutdown_high();
	msleep(30);
	clr_reg(client);
	reset_chip(client);
	gsl_load_fw(client);
	startup_chip(client);
	reset_chip(client);
	startup_chip(client);
}

static bool _check_chip_state(struct i2c_client *client)
{
	u8 read_buf[4]  = {0};
	gsl_ts_read(client, 0xb0, read_buf, sizeof(read_buf));

	pr_debug("#########check mem read 0xb0 = %x %x %x %x #########\n",
			read_buf[3], read_buf[2], read_buf[1], read_buf[0]);

	if (read_buf[3] != 0x5a || read_buf[2] != 0x5a || read_buf[1] != 0x5a ||
							read_buf[0] != 0x5a) {
		return false;
	}
	return true;
}

static bool check_chip_state(struct i2c_client *client)
{
	static int retry;
	bool ret;

	if (retry > 3) {
		printk("check_chip_state failed more times, not retry it!!");
		retry = 0;
		return true;
	}
	ret = _check_chip_state(client);
	if (ret) {
		retry = 0;
	} else {
		retry++;
	}
	return ret;
}

static void check_mem_data(struct i2c_client *client)
{
	/*if(gsl_chipType_new == 1) */
	if (ts_init->is_suspended != false ||
			ts_init->is_runtime_suspend != false)
		msleep(30);

	if (!_check_chip_state(client))
		init_chip(client);
}
#ifdef STRETCH_FRAME
static void stretch_frame(u16 *x, u16 *y)
{
	u16 temp_x = *x;
	u16 temp_y = *y;
	u16 temp_0, temp_1, temp_2;

	if (temp_x < X_STRETCH_MAX + X_STRETCH_CUST) {
		temp_0 = temp_1 = temp_2 = 0;
		temp_0 = X_STRETCH_MAX + X_STRETCH_CUST - temp_x;
		temp_0 = temp_0 > X_STRETCH_CUST ? X_STRETCH_CUST : temp_0;
		temp_0 = temp_0*(100 + X_RATIO_CUST)/100;
		if (temp_x < X_STRETCH_MAX) {
			temp_1 = X_STRETCH_MAX - temp_x;
			temp_1 = temp_1 > X_STRETCH_MAX/4 ? X_STRETCH_MAX/4 : temp_1;
			temp_1 = temp_1*(100 + 2*XL_RATIO_1)/100;
		}
		if (temp_x < 3*X_STRETCH_MAX/4) {
			temp_2 = 3*X_STRETCH_MAX/4 - temp_x;
			temp_2 = temp_2*(100 + 2*XL_RATIO_2)/100;
		}
		*x = (temp_0 + temp_1 + temp_2) < (X_STRETCH_MAX +
				X_STRETCH_CUST) ? ((X_STRETCH_MAX +
				X_STRETCH_CUST) - (temp_0 + temp_1 + temp_2)) : 1;
	} else if (temp_x > (CTP_MAX_X - X_STRETCH_MAX - X_STRETCH_CUST)) {
		temp_0 = temp_1 = temp_2 = 0;
		temp_0 = temp_x - (CTP_MAX_X - X_STRETCH_MAX - X_STRETCH_CUST);
		temp_0 = temp_0 > X_STRETCH_CUST ? X_STRETCH_CUST : temp_0;
		temp_0 = temp_0*(100 + X_RATIO_CUST)/100;
		if (temp_x > (CTP_MAX_X - X_STRETCH_MAX)) {
			temp_1 = temp_x - (CTP_MAX_X - X_STRETCH_MAX);
			temp_1 = temp_1 > X_STRETCH_MAX/4 ? X_STRETCH_MAX/4 : temp_1;
			temp_1 = temp_1*(100 + 2*XR_RATIO_1)/100;
		}
		if (temp_x > (CTP_MAX_X - 3*X_STRETCH_MAX/4)) {
			temp_2 = temp_x - (CTP_MAX_X - 3*X_STRETCH_MAX/4);
			temp_2 = temp_2*(100 + 2*XR_RATIO_2)/100;
		}
		*x = (temp_0 + temp_1 + temp_2) < (X_STRETCH_MAX +
				X_STRETCH_CUST) ? ((CTP_MAX_X - X_STRETCH_MAX -
				X_STRETCH_CUST) + (temp_0 + temp_1 +
				temp_2)) : (CTP_MAX_X - 1);
	}

	if (temp_y < Y_STRETCH_MAX + Y_STRETCH_CUST) {
		temp_0 = temp_1 = temp_2 = 0;
		temp_0 = Y_STRETCH_MAX + Y_STRETCH_CUST - temp_y;
		temp_0 = temp_0 > Y_STRETCH_CUST ? Y_STRETCH_CUST : temp_0;
		temp_0 = temp_0*(100 + Y_RATIO_CUST)/100;
		if (temp_y < Y_STRETCH_MAX) {
			temp_1 = Y_STRETCH_MAX - temp_y;
			temp_1 = temp_1 > Y_STRETCH_MAX/4 ? Y_STRETCH_MAX/4 : temp_1;
			temp_1 = temp_1*(100 + 2*YL_RATIO_1)/100;
		}
		if (temp_y < 3*Y_STRETCH_MAX/4) {
			temp_2 = 3*Y_STRETCH_MAX/4 - temp_y;
			temp_2 = temp_2*(100 + 2*YL_RATIO_2)/100;
		}
		*y = (temp_0 + temp_1 + temp_2) < (Y_STRETCH_MAX +
				Y_STRETCH_CUST) ? ((Y_STRETCH_MAX +
				Y_STRETCH_CUST) - (temp_0 +
				temp_1 + temp_2)) : 1;
	} else if (temp_y > (CTP_MAX_Y - Y_STRETCH_MAX - Y_STRETCH_CUST)) {
		temp_0 = temp_1 = temp_2 = 0;
		temp_0 = temp_y - (CTP_MAX_Y - Y_STRETCH_MAX - Y_STRETCH_CUST);
		temp_0 = temp_0 > Y_STRETCH_CUST ? Y_STRETCH_CUST : temp_0;
		temp_0 = temp_0*(100 + Y_RATIO_CUST)/100;
		if (temp_y > (CTP_MAX_Y - Y_STRETCH_MAX)) {
			temp_1 = temp_y - (CTP_MAX_Y - Y_STRETCH_MAX);
			temp_1 = temp_1 > Y_STRETCH_MAX/4 ? Y_STRETCH_MAX/4 : temp_1;
			temp_1 = temp_1*(100 + 2*YR_RATIO_1)/100;
		}
		if (temp_y > (CTP_MAX_Y - 3*Y_STRETCH_MAX/4)) {
			temp_2 = temp_y - (CTP_MAX_Y - 3*Y_STRETCH_MAX/4);
			temp_2 = temp_2*(100 + 2*YR_RATIO_2)/100;
		}
		*y = (temp_0 + temp_1 + temp_2) < (Y_STRETCH_MAX +
				Y_STRETCH_CUST) ? ((CTP_MAX_Y - Y_STRETCH_MAX -
				Y_STRETCH_CUST) + (temp_0 + temp_1 +
				temp_2)) : (CTP_MAX_Y - 1);
	}
}
#endif

#ifdef FILTER_POINT
static void filter_point(u16 x, u16 y, u8 id)
{
	u16 x_err = 0;
	u16 y_err = 0;
	u16 filter_step_x = 0, filter_step_y = 0;

	id_sign[id] = id_sign[id] + 1;
	if (id_sign[id] == 1) {
		x_old[id] = x;
		y_old[id] = y;
	}

	x_err = x > x_old[id] ? (x - x_old[id]) : (x_old[id] - x);
	y_err = y > y_old[id] ? (y - y_old[id]) : (y_old[id] - y);

	if ((x_err > FILTER_MAX && y_err > FILTER_MAX/3) ||
				(x_err > FILTER_MAX/3 && y_err > FILTER_MAX)) {
		filter_step_x = x_err;
		filter_step_y = y_err;
	} else {
		if (x_err > FILTER_MAX)
			filter_step_x = x_err;
		if (y_err > FILTER_MAX)
			filter_step_y = y_err;
	}

	if (x_err <= 2*FILTER_MAX && y_err <= 2*FILTER_MAX) {
		filter_step_x >>= 2;
		filter_step_y >>= 2;
	} else if (x_err <= 3*FILTER_MAX && y_err <= 3*FILTER_MAX) {
		filter_step_x >>= 1;
		filter_step_y >>= 1;
	} else if (x_err <= 4*FILTER_MAX && y_err <= 4*FILTER_MAX) {
		filter_step_x = filter_step_x*3/4;
		filter_step_y = filter_step_y*3/4;
	}

	x_new = x > x_old[id] ? (x_old[id] + filter_step_x) : (x_old[id] -
								filter_step_x);
	y_new = y > y_old[id] ? (y_old[id] + filter_step_y) : (y_old[id] -
								filter_step_y);

	x_old[id] = x_new;
	y_old[id] = y_new;
}
#else

static void record_point(u16 x, u16 y, u8 id)
{
	u16 x_err = 0;
	u16 y_err = 0;

	id_sign[id] = id_sign[id]+1;

	if (id_sign[id] == 1) {
		x_old[id] = x;
		y_old[id] = y;
	}

	x = (x_old[id] + x)/2;
	y = (y_old[id] + y)/2;

	if (x > x_old[id])
		x_err = x - x_old[id];
	else
		x_err = x_old[id]-x;

	if (y > y_old[id])
		y_err = y - y_old[id];
	else
		y_err = y_old[id]-y;

	if ((x_err > 3 && y_err > 1) || (x_err > 1 && y_err > 3)) {
		x_new = x;     x_old[id] = x;
		y_new = y;     y_old[id] = y;
	} else {
		if (x_err > 3) {
			x_new = x;
			x_old[id] = x;
		} else
			x_new = x_old[id];

		if (y_err > 3) {
			y_new = y;
			y_old[id] = y;
		} else
			y_new = y_old[id];
	}

	if (id_sign[id] == 1) {
		x_new = x_old[id];
		y_new = y_old[id];
	}

}
#endif
#ifdef TPD_PROC_DEBUG
static int char_to_int(char ch)
{
	if (ch >= '0' && ch <= '9')
		return (ch-'0');
	else
		return (ch-'a'+10);
}

/* static int gsl_config_read_proc(char *page, char **start, off_t off,
 * int count, int *eof, void *data)
 */
static int gsl_config_read_proc(struct seq_file *m, void *v)
{
	/* char *ptr = page; */
	char temp_data[5] = {0};
	unsigned int tmp = 0;
	unsigned int *ptr_fw;

	if ('v' == gsl_read[0] && 's' == gsl_read[1]) {
#ifdef GSL_NOID_VERSION
		tmp = gsl_version_id();
#else
		tmp = 0x20121215;
#endif
		/* ptr += sprintf(ptr,"version:%x\n",tmp); */
		seq_printf(m, "version:%x\n", tmp);
	} else if ('r' == gsl_read[0] && 'e' == gsl_read[1]) {
		if ('i' == gsl_read[3]) {
#ifdef GSL_NOID_VERSION
			tmp = (gsl_data_proc[5]<<8) | gsl_data_proc[4];
			/* ptr +=sprintf(ptr,"gsl_config_data_id[%u] = ",tmp); */
			seq_printf(m, "gsl_config_data_id[%u] = ", tmp);
			if (tmp >= 0 && tmp < 512)
				/*gslX680_config_data*/
				/*ptr +=sprintf(ptr,"%d\n",gsl_config_data_id[tmp]); */
				seq_printf(m, "%d\n", gsl_config_data_id[tmp]);
#endif
		} else {
			gsl_ts_write(glsX680_i2c, 0xf0, &gsl_data_proc[4], 4);
			gsl_read_interface(glsX680_i2c, gsl_data_proc[0],
								temp_data, 4);
			/*ptr +=sprintf(ptr, "offset : {0x%02x, 0x", gsl_data_proc[0]);*/
			/*ptr +=sprintf(ptr, "%02x", temp_data[3]);*/
			/*ptr +=sprintf(ptr, "%02x", temp_data[2]);*/
			/*ptr +=sprintf(ptr, "%02x", temp_data[1]);*/
			/*ptr +=sprintf(ptr, "%02x};\n", temp_data[0]);*/
			seq_printf(m, "offset : {0x%02x,0x", gsl_data_proc[0]);
			seq_printf(m, "%02x", temp_data[3]);
			seq_printf(m, "%02x", temp_data[2]);
			seq_printf(m, "%02x", temp_data[1]);
			seq_printf(m, "%02x};\n", temp_data[0]);
		}
	}
	/*eof = 1;*/
	/*return (ptr - page);*/
	return 0;
}
static int gsl_config_write_proc(struct file *file, const char *buffer,
						unsigned long count, void *data)
{
	u8 buf[8] = {0};
	int tmp = 0;
	int tmp1 = 0;


	if (count > CONFIG_LEN) {
		pr_err("size not match [%d:%ld]\n", CONFIG_LEN, count);
		return -EFAULT;
	}

	if (copy_from_user(gsl_read, buffer,
				(count < CONFIG_LEN?count:CONFIG_LEN))) {
		pr_err("copy from user fail\n");
		return -EFAULT;
	}
	pr_debug("[tp-gsl][%s][%s]\n", __func__, gsl_read);

	buf[3] = char_to_int(gsl_read[14])<<4 | char_to_int(gsl_read[15]);
	buf[2] = char_to_int(gsl_read[16])<<4 | char_to_int(gsl_read[17]);
	buf[1] = char_to_int(gsl_read[18])<<4 | char_to_int(gsl_read[19]);
	buf[0] = char_to_int(gsl_read[20])<<4 | char_to_int(gsl_read[21]);

	buf[7] = char_to_int(gsl_read[5])<<4 | char_to_int(gsl_read[6]);
	buf[6] = char_to_int(gsl_read[7])<<4 | char_to_int(gsl_read[8]);
	buf[5] = char_to_int(gsl_read[9])<<4 | char_to_int(gsl_read[10]);
	buf[4] = char_to_int(gsl_read[11])<<4 | char_to_int(gsl_read[12]);
	if ('v' == gsl_read[0] && 's' == gsl_read[1])
		pr_debug("gsl version\n");

	else if ('s' == gsl_read[0] && 't' == gsl_read[1]) {
		gsl_proc_flag = 1;
		reset_chip(glsX680_i2c);
	} else if ('e' == gsl_read[0] && 'n' == gsl_read[1]) {
		msleep(20);
		reset_chip(glsX680_i2c);
		startup_chip(glsX680_i2c);

#ifdef GSL_NOID_VERSION
		gsl_DataInit(gslX680_config_data);
#endif
		gsl_proc_flag = 0;
	} else if ('r' == gsl_read[0] && 'e' == gsl_read[1])
		memcpy(gsl_data_proc, buf, 8);
	else if ('w' == gsl_read[0] && 'r' == gsl_read[1])
		gsl_ts_write(glsX680_i2c, buf[4], buf, 4);

#ifdef GSL_NOID_VERSION
	else if ('i' == gsl_read[0] && 'd' == gsl_read[1]) {
		tmp1 = (buf[7]<<24) | (buf[6]<<16) | (buf[5]<<8) | buf[4];
		tmp = (buf[3]<<24) | (buf[2]<<16) | (buf[1]<<8) | buf[0];
		if (tmp1 >= 0 && tmp1 < 512)
			gslX680_config_data[tmp1] = tmp;
	}
#endif
	return count;
}
static int gsl_server_list_open(struct inode *inode, struct file *file)
{
	return single_open(file, gsl_config_read_proc, NULL);
}
static const struct file_operations gsl_seq_fops = {
	.open = gsl_server_list_open,
	.read = seq_read,
	.release = single_release,
	.write = gsl_config_write_proc,
	.owner = THIS_MODULE,
};
#endif
#ifdef HAVE_TOUCH_KEY
static void report_key(struct gsl_ts *ts, u16 x, u16 y)
{
	u16 i = 0;

	for (i = 0; i < MAX_KEY_NUM; i++) {
		if ((gsl_key_data[i].x_min < x) &&
				(x < gsl_key_data[i].x_max) &&
				(gsl_key_data[i].y_min < y) &&
				(y < gsl_key_data[i].y_max)) {
			key = gsl_key_data[i].key;
			pr_debug("key=%d\n", key);
			input_report_key(ts->input, key, 1);
			input_sync(ts->input);
			key_state_flag = 1;
			break;
		}
	}
}
#endif

static void report_data(struct gsl_ts *ts, u16 x, u16 y, u8 pressure, u8 id)
{
	pr_debug("source data:ID:%d, X:%d, Y:%d, W:%d\n", id, x, y, pressure);
	if (exchange_x_y_flag == 1)
		swap(x, y);

	if (revert_x_flag == 1)
		x = SCREEN_MAX_X - x;

	if (revert_y_flag == 1)
		y = SCREEN_MAX_Y - y;

	pr_debug("report data:ID:%d, X:%d, Y:%d, W:%d\n", id, x, y, pressure);

	if (x > SCREEN_MAX_X || y > SCREEN_MAX_Y) {
	#ifdef HAVE_TOUCH_KEY

		report_key(ts, x, y);

	#endif
		return;
	}

#ifdef REPORT_DATA_ANDROID_4_0
	input_mt_slot(ts->input, id);
	input_report_abs(ts->input, ABS_MT_TRACKING_ID, id);
	input_report_abs(ts->input, ABS_MT_TOUCH_MAJOR, pressure);
	input_report_abs(ts->input, ABS_MT_POSITION_X, x);
	input_report_abs(ts->input, ABS_MT_POSITION_Y, y);
	input_report_abs(ts->input, ABS_MT_WIDTH_MAJOR, 1);
#else
	input_report_abs(ts->input, ABS_MT_TRACKING_ID, id);
	input_report_abs(ts->input, ABS_MT_TOUCH_MAJOR, pressure);
	input_report_abs(ts->input, ABS_MT_POSITION_X, x);
	input_report_abs(ts->input, ABS_MT_POSITION_Y, y);
	input_report_abs(ts->input, ABS_MT_WIDTH_MAJOR, 1);
	input_mt_sync(ts->input);
#endif
}


static void process_gslX680_data(struct gsl_ts *ts)
{
	u8 id, touches;
	u16 x, y;
	int i = 0;
#ifdef GSL_NOID_VERSION
	struct gsl_touch_info cinfo;
#endif
	touches = ts->touch_data[ts->dd->touch_index];
#ifdef GSL_NOID_VERSION
	cinfo.finger_num = touches;
	pr_debug("tp-gsl  finger_num = %d\n", cinfo.finger_num);
	for (i = 0; i < (touches < MAX_CONTACTS ? touches : MAX_CONTACTS); i++) {
		cinfo.x[i] = join_bytes((ts->touch_data[ts->dd->x_index + 4 * i + 1] & 0xf),
				ts->touch_data[ts->dd->x_index + 4 * i]);
		cinfo.y[i] = join_bytes(ts->touch_data[ts->dd->y_index + 4 * i + 1],
				ts->touch_data[ts->dd->y_index + 4 * i]);
	}
	cinfo.finger_num = ts->touch_data[0] | (ts->touch_data[1]<<8) |
			(ts->touch_data[2]<<16) | (ts->touch_data[3]<<24);
	gsl_alg_id_main(&cinfo);
	pr_debug("tp-gsl  finger_num = %d\n", cinfo.finger_num);
#if 0
	tmp1 = gsl_mask_tiaoping();
	if (tmp1 > 0 && tmp1 < 0xffffffff) {
		buf[0] = 0xa;
		buf[1] = 0;
		buf[2] = 0;
		buf[3] = 0;
		gsl_ts_write(ts->client, 0xf0, buf, 4);
		buf[0] = (u8)(tmp1 & 0xff);
		buf[1] = (u8)((tmp1>>8) & 0xff);
		buf[2] = (u8)((tmp1>>16) & 0xff);
		buf[3] = (u8)((tmp1>>24) & 0xff);
		pr_debug("tmp1=%08x,buf[0]=%02x,buf[1]=%02x,buf[2]=%02x,buf[3]=%02x\n",
			tmp1, buf[0], buf[1], buf[2], buf[3]);
		gsl_write_interface(ts->client, 0x8, buf, 4);
	}
#endif
	touches = cinfo.finger_num;
	input_report_key(ts->input, BTN_TOUCH, (touches > 0 ? 1 : 0));
#endif
	for (i = 1; i <= MAX_CONTACTS; i++) {
		if (touches == 0)
			id_sign[i] = 0;
		id_state_flag[i] = 0;
	}
	for (i = 0; i < (touches > MAX_FINGERS ? MAX_FINGERS : touches); i++) {
	#ifdef GSL_NOID_VERSION
		id = cinfo.id[i];
		x =  cinfo.x[i];
		y =  cinfo.y[i];
	#else
		x = join_bytes((ts->touch_data[ts->dd->x_index+4*i+1] & 0xf),
				ts->touch_data[ts->dd->x_index + 4 * i]);
		y = join_bytes(ts->touch_data[ts->dd->y_index + 4 * i + 1],
				ts->touch_data[ts->dd->y_index + 4 * i]);
		id = ts->touch_data[ts->dd->id_index + 4 * i] >> 4;
	#endif
		if (1 <= id && id <= MAX_CONTACTS) {
		#ifdef STRETCH_FRAME
			stretch_frame(&x, &y);

		#endif
		#ifdef FILTER_POINT
			filter_point(x, y, id);
		#else
			record_point(x, y, id);
		#endif
			report_data(ts, x_new, y_new, 10, id);
			id_state_flag[id] = 1;
		}
	}


	for (i = 1; i <= MAX_CONTACTS ; i++) {
		if ((touches == 0) || ((id_state_old_flag[i] != 0) &&
						(id_state_flag[i] == 0))) {
		#ifdef REPORT_DATA_ANDROID_4_0
			input_mt_slot(ts->input, i);
			input_report_abs(ts->input, ABS_MT_TRACKING_ID, -1);
			input_mt_report_slot_state(ts->input, MT_TOOL_FINGER,
									false);
		#endif
			id_sign[i] = 0;
		}
		id_state_old_flag[i] = id_state_flag[i];
	}
#ifndef REPORT_DATA_ANDROID_4_0
	if (touches == 0) {
	#ifdef HAVE_TOUCH_KEY
		if (key_state_flag) {
			input_report_key(ts->input, key, 0);
			input_sync(ts->input);
			key_state_flag = 0;
		}
	#endif
	}
#endif
	input_sync(ts->input);
	ts->prev_touches = touches;
}


static void gsl_ts_xy_worker(struct work_struct *work)
{
	int rc;
	u8 read_buf[4] = {0};
	struct gsl_ts *ts = container_of(work, struct gsl_ts, work);
#ifndef GSL_TIMER
	int ret;

	input_set_int_enable(&(config_info.input_type), 0);
#endif
	pr_debug("---gsl_ts_xy_worker---\n");
#ifdef TPD_PROC_DEBUG
	if (gsl_proc_flag == 1)
		goto schedule;
#endif
	/* read data from DATA_REG */
	rc = gsl_ts_read(ts->client, 0x80, ts->touch_data, ts->dd->data_size);
	pr_debug("---touches: %d ---\n", ts->touch_data[0]);
	if (rc < 0) {
		dev_err(&ts->client->dev, "read failed\n");
		goto schedule;
	}

	if (ts->touch_data[ts->dd->touch_index] == 0xff)
		goto schedule;

	rc = gsl_ts_read(ts->client, 0xbc, read_buf, sizeof(read_buf));
	if (rc < 0) {
		dev_err(&ts->client->dev, "read 0xbc failed\n");
		goto schedule;
	}
	pr_debug("reg %x : %x %x %x %x\n", 0xbc, read_buf[3],
					read_buf[2], read_buf[1], read_buf[0]);
	if (read_buf[3] == 0 && read_buf[2] == 0 && read_buf[1] == 0 &&
							read_buf[0] == 0){
		process_gslX680_data(ts);
	} else {
		if (!_check_chip_state(glsX680_i2c)) {
			printk("gslX680 gsl_ts_xy_worker fail and check state fialed!!!\n");
		}
	}

schedule:
#ifndef GSL_TIMER
	ret = input_set_int_enable(&(config_info.input_type), 1);
	if (ret < 0)
		printk("%s irq enable failed\n", __func__);
#endif
}


#ifdef GSL_MONITOR
static void gsl_monitor_worker(struct work_struct *work)
{
	char read_buf[4]  = {0};

	pr_debug("-----------gsl_monitor_worker----------\n");

	gsl_ts_read(glsX680_i2c, 0xb4, read_buf, 4);
	int_2nd[3] = int_1st[3];
	int_2nd[2] = int_1st[2];
	int_2nd[1] = int_1st[1];
	int_2nd[0] = int_1st[0];
	int_1st[3] = read_buf[3];
	int_1st[2] = read_buf[2];
	int_1st[1] = read_buf[1];
	int_1st[0] = read_buf[0];

	if (int_1st[3] == int_2nd[3] && int_1st[2] == int_2nd[2] &&
			int_1st[1] == int_2nd[1] && int_1st[0] == int_2nd[0]) {
		pr_debug("===int_1st:%x %x %x %x , int_2nd:%x %x %x %x===\n",
				int_1st[3], int_1st[2], int_1st[1], int_1st[0],
				int_2nd[3], int_2nd[2], int_2nd[1], int_2nd[0]);
		init_chip(glsX680_i2c);
	}
	queue_delayed_work(gsl_monitor_workqueue, &gsl_monitor_work, 300);
}
#endif


irqreturn_t gsl_ts_irq(int irq, void *dev_id)
{
	struct gsl_ts *ts = (struct gsl_ts *)dev_id;

	pr_debug("==========GSLX680 Interrupt============\n");
	queue_work(ts->wq, &ts->work);
#ifdef GSL_TIMER
	mod_timer(&ts->gsl_timer, jiffies + msecs_to_jiffies(30));
#endif
	return IRQ_HANDLED;
}

#if GSL_MONITOR_TIMER_ENABLE
static void glsX680_monitor_events(struct work_struct *work)
{
	if (!_check_chip_state(glsX680_i2c) || gTestMonitor == 1) {
		printk("gsl_monitor_timer_handle check failed, reinit chip!!\n");
		input_set_int_enable(&(config_info.input_type), 0);
		cancel_work_sync(&ts_init->work);
		queue_work(gslX680_resume_wq, &glsX680_resume_work);
		gTestMonitor = 0;
	} else {
		pr_debug("gsl_monitor_timer_handle check success!!!\n");
		del_timer(&monitor_timer);
		monitor_timer.expires = jiffies + msecs_to_jiffies(3000);
		add_timer(&monitor_timer);
	}
}
static void gsl_monitor_timer_handle(struct timer_list *timer)
{
	queue_work(ts_init->wq, &glsX680_monitor_work);
}
#endif

#ifdef GSL_TIMER
static void gsl_timer_handle(unsigned long data)
{
	struct gsl_ts *ts = (struct gsl_ts *)data;

#ifdef GSL_DEBUG
	pr_debug("----------------gsl_timer_handle-----------------\n");
#endif
	ret = input_set_int_enable(&(config_info.input_type), 1);
	if (ret < 0)
		pr_debug("%s irq disable failed\n", __func__);
	check_mem_data(ts->client);
	ts->gsl_timer.expires = jiffies + 3 * HZ;
	add_timer(&ts->gsl_timer);
}
#endif

static int gsl_ts_init_ts(struct i2c_client *client, struct gsl_ts *ts)
{
	struct input_dev *input_device;
	int  rc = 0;
#ifdef HAVE_TOUCH_KEY
	int i = 0;
#endif
	pr_debug("[GSLX680] Enter %s\n", __func__);
	ts->dd = &devices[ts->device_id];

	if (ts->device_id == 0) {
		ts->dd->data_size = MAX_FINGERS * ts->dd->touch_bytes +
							ts->dd->touch_meta_data;
		ts->dd->touch_index = 0;
	}

	ts->touch_data = kzalloc(ts->dd->data_size, GFP_KERNEL);

	ts->prev_touches = 0;

	input_device = input_allocate_device();
	if (!input_device) {
		rc = -ENOMEM;
		goto error_alloc_dev;
	}

	ts->input = input_device;
	input_device->name = GSLX680_I2C_NAME;
	input_device->id.bustype = BUS_I2C;
	input_device->dev.parent = &client->dev;
	input_set_drvdata(input_device, ts);

#ifdef REPORT_DATA_ANDROID_4_0
	__set_bit(EV_ABS, input_device->evbit);
	__set_bit(EV_KEY, input_device->evbit);
	__set_bit(EV_REP, input_device->evbit);
	__set_bit(INPUT_PROP_DIRECT, input_device->propbit);
	input_mt_init_slots(input_device, (MAX_CONTACTS+1));
#else
	input_set_abs_params(input_device, ABS_MT_TRACKING_ID, 0,
							(MAX_CONTACTS+1), 0, 0);
	set_bit(EV_ABS, input_device->evbit);
	set_bit(EV_KEY, input_device->evbit);
	__set_bit(INPUT_PROP_DIRECT, input_device->propbit);
	input_device->keybit[BIT_WORD(BTN_TOUCH)] = BIT_MASK(BTN_TOUCH);
#endif
#ifdef HAVE_TOUCH_KEY
	for (i = 0; i < MAX_KEY_NUM; i++)
		set_bit(key_array[i] & KEY_MAX, input_device->keybit);

#endif

	set_bit(ABS_MT_POSITION_X, input_device->absbit);
	set_bit(ABS_MT_POSITION_Y, input_device->absbit);
	set_bit(ABS_MT_TOUCH_MAJOR, input_device->absbit);
	set_bit(ABS_MT_WIDTH_MAJOR, input_device->absbit);

	input_set_abs_params(input_device, ABS_MT_POSITION_X, 0,
							SCREEN_MAX_X, 0, 0);
	input_set_abs_params(input_device, ABS_MT_POSITION_Y, 0,
							SCREEN_MAX_Y, 0, 0);
	input_set_abs_params(input_device, ABS_MT_TOUCH_MAJOR, 0,
							PRESS_MAX, 0, 0);
	input_set_abs_params(input_device, ABS_MT_WIDTH_MAJOR, 0, 200, 0, 0);

	ts->wq = create_singlethread_workqueue("kworkqueue_ts");
	if (!ts->wq) {
		dev_err(&client->dev, "Could not create workqueue\n");
		goto error_wq_create;
	}
	flush_workqueue(ts->wq);

	INIT_WORK(&ts->work, gsl_ts_xy_worker);

	rc = input_register_device(input_device);
	if (rc)
		goto error_unreg_device;

	return 0;

error_unreg_device:
	destroy_workqueue(ts->wq);
error_wq_create:
	input_free_device(input_device);
error_alloc_dev:
	kfree(ts->touch_data);
	return rc;
}

static void glsX680_resume_events(struct work_struct *work)
{
#ifndef GSL_TIMER
	int ret;
#endif
#if GSL_MONITOR_TIMER_ENABLE
	del_timer(&monitor_timer);
#endif
	while (true) {
		gslX680_shutdown_low();
		if (ldo_state) {
			input_set_power_enable(&(config_info.input_type), 0);
			ldo_state--;
			msleep(200);
		} else {
			msleep(10);
		}
		input_set_power_enable(&(config_info.input_type), 1);
		ldo_state++;
		gslX680_shutdown_high();
		usleep_range(10000, 11000);
		reset_chip(glsX680_i2c);
		startup_chip(glsX680_i2c);
		check_mem_data(glsX680_i2c);
#ifndef GSL_TIMER
		if (check_chip_state(glsX680_i2c)) {
			ret = input_set_int_enable(&(config_info.input_type), 1);
			if (ret < 0)
				pr_debug("%s irq disable failed\n", __func__);
			printk("gslX680_resume_events success!!!\n");
#if GSL_MONITOR_TIMER_ENABLE
			del_timer(&monitor_timer);
			monitor_timer.expires = jiffies + msecs_to_jiffies(3000);
			add_timer(&monitor_timer);
#endif
			return;
		} else {
			printk("glsX680_resume_events failed, retry!!!!\n");
		}
#endif
	}
}

static unsigned long idle_data;
static unsigned long idle_next_data;
#if IS_ENABLED(CONFIG_PM)
static int gsl_ts_suspend(struct device *dev)
{
#ifndef GSL_TIMER
	int ret;
#endif
	struct gsl_ts *ts = dev_get_drvdata(dev);

	pr_debug("%s,start\n", __func__);
#if GSL_MONITOR_TIMER_ENABLE
	del_timer(&monitor_timer);
	cancel_work_sync(&glsX680_monitor_work);
#endif
	cancel_work_sync(&glsX680_resume_work);
	cancel_work_sync(&glsX680_idle_work);
	flush_workqueue(gslX680_resume_wq);
#if GSL_MONITOR_TIMER_ENABLE
	del_timer(&monitor_timer);
	cancel_work_sync(&glsX680_monitor_work);
#endif
	/*if already do runtime suspend,and try to do suspend,then return*/
	if (pm_runtime_suspended(dev)) {
		pr_debug("do suspend\n");
		ts->is_suspended = true;
		return 0;
	}

#ifdef GSL_TIMER
	pr_debug("gsl_ts_suspend () : delete gsl_timer\n");
	del_timer(&ts->gsl_timer);
#endif

#ifndef GSL_TIMER
	ret = input_set_int_enable(&(config_info.input_type), 0);
	if (ret < 0)
		printk("%s irq disable failed\n", __func__);
#endif
	flush_workqueue(gslX680_resume_wq);
	cancel_work_sync(&ts->work);
	flush_workqueue(ts->wq);
	gslX680_shutdown_low();

	if (ts->try_to_runtime_suspend) {
		pr_debug("do runtime_suspend\n");
		ts->is_runtime_suspend = true;
	} else {
		pr_debug("do suspend\n");
		ts->is_suspended = true;
	}
	input_set_power_enable(&(config_info.input_type), 0);
	ldo_state--;
	pr_info("gslX680 suspend finished");
	return 0;
}

static int gsl_ts_resume(struct device *dev)
{
	struct gsl_ts *ts = dev_get_drvdata(dev);

	if (ts->is_runtime_suspend && ts->is_suspended) {
		pr_debug("do resume\n");
		ts->is_suspended = false;
		return 0;
	}
	pr_debug("I'am in gsl_ts_resume() start\n");
	cancel_work_sync(&ts->work);
#if GSL_MONITOR_TIMER_ENABLE
	cancel_work_sync(&glsX680_monitor_work);
#endif
	flush_workqueue(ts->wq);
	queue_work(gslX680_resume_wq, &glsX680_resume_work);

	if (ts->try_to_runtime_suspend && ts->is_runtime_suspend &&
						!ts->is_suspended) {
		pr_debug("do runtime_resume\n");
		ts->try_to_runtime_suspend  = false;
		ts->is_runtime_suspend = false;
	} else if (ts->is_suspended) {
		pr_debug("do resume\n");
		ts->is_suspended = false;
	}

#ifdef GSL_TIMER
	pr_debug("gsl_ts_resume () : add gsl_timer\n");
	init_timer(&ts->gsl_timer);
	ts->gsl_timer.expires = jiffies + 3 * HZ;
	ts->gsl_timer.function = &gsl_timer_handle;
	ts->gsl_timer.data = (unsigned long)ts;
	add_timer(&ts->gsl_timer);
#endif
	pr_info("gslX680 resume finished");
	return 0;
}
#endif


static void glsX680_init_events(struct work_struct *work)
{
	int ret = 0;

	gslX680_chip_init();
	init_chip(glsX680_i2c);
	check_mem_data(glsX680_i2c);

#ifndef GSL_TIMER
	config_info.dev = &(ts_init->input->dev);
	ret = input_request_int(&(config_info.input_type), gsl_ts_irq,
				CTP_IRQ_MODE, ts_init);
	if (ret)
		printk("glsX680_init_events: request irq failed\n");
#else
	pr_debug("add gsl_timer\n");
	init_timer(&ts_init->gsl_timer);
	ts_init->gsl_timer.expires = jiffies + msecs_to_jiffies(500);
	ts_init->gsl_timer.function = &gsl_ts_irq;
	ts_init->gsl_timer.data = (unsigned long)ts_init;
	add_timer(&ts_init->gsl_timer);
#endif
#if GSL_MONITOR_TIMER_ENABLE
	monitor_timer.expires = jiffies + msecs_to_jiffies(3000);;
	add_timer(&monitor_timer);
#endif
}

static unsigned long data_save;

static ssize_t gsl_enable_show(struct device *dev,
	       struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", (int)data_save);
}

static ssize_t runtime_suspend_show(struct class *cls,
	       struct class_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", (int)data_save);
}
static ssize_t gsl_enable_store_l(const char *buf)
{
	int error;
	//struct input_dev *input = to_input_dev(dev);
	struct i2c_client *client = input_get_drvdata(gDevice);

	error = kstrtoul(buf, 10, &data_save);
	if (error)
		return error;
	if (data_save == 0 && !ts_init->is_runtime_suspend) {
		pr_info("gslX680 go to runtime_suspend\n");
		ts_init->try_to_runtime_suspend = true;
		pm_runtime_put(&client->dev);
	} else if (data_save == 1 && ts_init->is_runtime_suspend) {
		pr_info("gslX680 go to runtime_resume\n");
		pm_runtime_get_sync(&client->dev);
	}
	return 0;
}

static ssize_t gsl_enable_store(struct device *dev,
	       struct device_attribute *attr,
	       const char *buf, size_t count)
{
	int error = gsl_enable_store_l(buf);

	if (error)
		return error;
	return count;
}

static ssize_t runtime_suspend_store(struct class *cls,
	       struct class_attribute *attr,
	       const char *buf, size_t count)
{
	int error = gsl_enable_store_l(buf);

	if (error)
		return error;
	return count;
}

static ssize_t gsl_idle_enable_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", (int)idle_data);
}

static ssize_t tp_idle_show(struct class *cls,
			struct class_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", (int)idle_data);
}

static ssize_t tp_state_show(struct class *cls,
			struct class_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", _check_chip_state(glsX680_i2c));
}

static void glsX680_idle_events(struct work_struct *work)
{
	if (idle_next_data == 1 && idle_data == 0) {
		input_set_int_enable(&(config_info.input_type), 0);
		idle_data = 1;
#if GSL_MONITOR_TIMER_ENABLE
		del_timer(&monitor_timer);
		cancel_work_sync(&glsX680_monitor_work);
#endif
		cancel_work_sync(&glsX680_resume_work);
		msleep(10);
		pr_info("gslX680 go to idle\n");
		gslX680_shutdown_low();
	} else if (idle_next_data == 0 && idle_data == 1) {
#if GSL_MONITOR_TIMER_ENABLE
		del_timer(&monitor_timer);
		cancel_work_sync(&glsX680_monitor_work);
#endif
		pr_info("gslX680 go to active\n");
		reset_chip(glsX680_i2c);
		startup_chip(glsX680_i2c);
		msleep(10);
		input_set_int_enable_force(&(config_info.input_type), 1);
		idle_data = 0;
#if GSL_MONITOR_TIMER_ENABLE
		monitor_timer.expires = jiffies + msecs_to_jiffies(3000);
		del_timer(&monitor_timer);
		add_timer(&monitor_timer);
#endif
	}
}

static ssize_t gsl_idle_enable_store_l(const char *buf)
{
	int error;

	error = kstrtoul(buf, 10, &idle_next_data);
	if (error)
		return error;
	if (idle_next_data != idle_data) {
		queue_work(gslX680_resume_wq, &glsX680_idle_work);
	}
	return 0;
}

static ssize_t gsl_idle_enable_store(struct device *dev,
	       struct device_attribute *attr,
	       const char *buf, size_t count)
{
	int error = gsl_idle_enable_store_l(buf);

	if (error)
		return error;
	return count;
}

static ssize_t tp_idle_store(struct class *cls,
	       struct class_attribute *attr,
	       const char *buf, size_t count)
{
	int error = gsl_idle_enable_store_l(buf);

	if (error)
		return error;
	return count;
}

static ssize_t screen_width_show(struct class *cls,
			struct class_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", (int)screen_max_x);
}

static ssize_t screen_width_store(struct class *cls,
	       struct class_attribute *attr,
	       const char *buf, size_t count)
{
	int error;
	int val;
	error = kstrtoint(buf, 10, &val);
	if (error)
		return error;
	screen_max_x = val;
	return count;
}

static ssize_t screen_height_show(struct class *cls,
			struct class_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", (int)screen_max_y);
}

static ssize_t screen_height_store(struct class *cls,
	       struct class_attribute *attr,
	       const char *buf, size_t count)
{
	int error;
	int val;
	error = kstrtoint(buf, 10, &val);
	if (error)
		return error;
	screen_max_y = val;
	return count;
}

static ssize_t screen_revert_x_show(struct class *cls,
			struct class_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", (int)revert_x_flag);
}

static ssize_t screen_revert_x_store(struct class *cls,
	       struct class_attribute *attr,
	       const char *buf, size_t count)
{
	int error;
	int val;
	error = kstrtoint(buf, 10, &val);
	if (error || (val != 0 && val != 1))
		return error;
	revert_x_flag = val;
	return count;
}

static ssize_t screen_revert_y_show(struct class *cls,
			struct class_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", (int)revert_y_flag);
}

static ssize_t screen_revert_y_store(struct class *cls,
	       struct class_attribute *attr,
	       const char *buf, size_t count)
{
	int error;
	int val;
	error = kstrtoint(buf, 10, &val);
	if (error || (val != 0 && val != 1))
		return error;
	revert_y_flag = val;
	return count;
}

static ssize_t screen_exchange_xy_show(struct class *cls,
			struct class_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", (int)exchange_x_y_flag);
}

static ssize_t screen_exchange_xy_store(struct class *cls,
	       struct class_attribute *attr,
	       const char *buf, size_t count)
{
	int error;
	int val;
	error = kstrtoint(buf, 10, &val);
	if (error || (val != 0 && val != 1))
		return error;
	exchange_x_y_flag = val;
	return count;
}

#if GSL_MONITOR_TIMER_ENABLE
static ssize_t tp_monitor_store(struct class *cls,
	       struct class_attribute *attr,
	       const char *buf, size_t count)
{
	gTestMonitor = 1;
	queue_work(gslX680_resume_wq, &glsX680_monitor_work);

	return count;
}
#endif
static DEVICE_ATTR(runtime_suspend, S_IRUGO | S_IWUSR | S_IWGRP,
		gsl_enable_show, gsl_enable_store);

static DEVICE_ATTR(tp_idle, S_IRUGO | S_IWUSR | S_IWGRP,
		gsl_idle_enable_show, gsl_idle_enable_store);

static struct attribute *gsl_attributes[] = {
	&dev_attr_runtime_suspend.attr,
	&dev_attr_tp_idle.attr,
	NULL
};


static CLASS_ATTR_RW(runtime_suspend);
static CLASS_ATTR_RW(tp_idle);
static CLASS_ATTR_RO(tp_state);
#if GSL_MONITOR_TIMER_ENABLE
static CLASS_ATTR_WO(tp_monitor);
#endif
static CLASS_ATTR_RW(screen_width);
static CLASS_ATTR_RW(screen_height);
static CLASS_ATTR_RW(screen_revert_x);
static CLASS_ATTR_RW(screen_revert_y);
static CLASS_ATTR_RW(screen_exchange_xy);

static struct attribute_group gsl_attr_group = {
	.attrs = gsl_attributes,
};

static struct attribute *ctp_class_attrs[] = {
	&class_attr_runtime_suspend.attr,
	&class_attr_tp_idle.attr,
	&class_attr_tp_state.attr,
#if GSL_MONITOR_TIMER_ENABLE
	&class_attr_tp_monitor.attr,
#endif
	&class_attr_screen_width.attr,
	&class_attr_screen_height.attr,
	&class_attr_screen_revert_x.attr,
	&class_attr_screen_revert_y.attr,
	&class_attr_screen_exchange_xy.attr,
	NULL
};

ATTRIBUTE_GROUPS(ctp_class);

static struct class ctp_class = {
	.name = "ctp",
	.owner = THIS_MODULE,
	.class_groups = ctp_class_groups,
};

static int startup(void)
{
	int ret = 0;
	pr_debug("*******************************************\n");
	if (input_sensor_startup(&(config_info.input_type))) {
		pr_err("%s: ctp_startup err.\n", __func__);
		return 0;
	} else {
		ret = input_sensor_init(&(config_info.input_type));
		if (ret != 0)
			pr_debug("%s:ctp_ops.init err.\n", __func__);
	}
	if (config_info.ctp_used == 0) {
		pr_debug("*** ctp_used set to 0 !\n");
		pr_debug("if use ctp,please put the sys_config.fex ctp_used set to 1.\n");
		return 0;
	}
	if (!ctp_get_system_config()) {
		pr_err("%s:read config fail!\n", __func__);
		return 0;
	}

	input_set_power_enable(&(config_info.input_type), 1);
	ldo_state++;
	msleep(20);
	ctp_wakeup(1, 0);
	return 1;
}

static int  gsl_ts_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	struct gsl_ts *ts;
	int rc = 0;
	int ret = 0;

	pr_debug("GSLX680 Enter %s\n", __func__);
	if (config_info.dev == NULL)
		config_info.dev = &client->dev;
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		dev_err(&client->dev, "I2C functionality not supported\n");
		return -ENODEV;
	}

	pr_debug("%s:fwname:%s\n", __func__, fwname);
	ret = gsl_getfw_from_file();
	if (ret != 0) {
		return -1;
	}

	ts = kzalloc(sizeof(*ts), GFP_KERNEL);

	gslX680_resume_wq = create_singlethread_workqueue("gslX680_resume");
	if (gslX680_resume_wq == NULL) {
		pr_err("create gslX680_resume_wq fail!\n");
		return -ENOMEM;
	}

	glsX680_i2c = client;
	ts->client = client;
	i2c_set_clientdata(client, ts);
	ts->device_id = id->driver_data;

	ts->is_suspended = false;
	ts->is_runtime_suspend	= false;
	ts->int_pending = false;
	mutex_init(&ts->sus_lock);

	rc = gsl_ts_init_ts(client, ts);
	if (rc < 0) {
		dev_err(&client->dev, "GSLX680 init failed\n");
		goto error_mutex_destroy;
	}
	ts_init = ts;
	gslX680_wq = create_singlethread_workqueue("gslX680_init");
	if (gslX680_wq == NULL) {
		pr_err("create gslX680_wq fail!\n");
		return -ENOMEM;
	}

#if GSL_MONITOR_TIMER_ENABLE
	timer_setup(&monitor_timer, NULL, 0);
	monitor_timer.expires = jiffies + msecs_to_jiffies(3000);;
	monitor_timer.function = &gsl_monitor_timer_handle;
#endif

	queue_work(gslX680_wq, &glsX680_init_work);

	device_create_file(&ts->input->dev, &dev_attr_debug_reg);
	device_enable_async_suspend(&client->dev);

	input_set_drvdata(ts->input, client);
	gDevice = ts->input;

	ret = sysfs_create_group(&ts->input->dev.kobj, &gsl_attr_group);
	if (ret < 0) {
		dev_err(&client->dev, "gsl: sysfs_create_group err\n");
		goto error_mutex_destroy;
	}

	pm_runtime_set_active(&client->dev);
	pm_runtime_get(&client->dev);
	pm_runtime_enable(&client->dev);

#ifdef TPD_PROC_DEBUG
#if 0
	gsl_config_proc = create_proc_entry(GSL_CONFIG_PROC_FILE, 0664, NULL);
	if (gsl_config_proc == NULL) {
		pr_debug("create_proc_entry %s failed\n", GSL_CONFIG_PROC_FILE);
	} else {
		gsl_config_proc->read_proc = gsl_config_read_proc;
		gsl_config_proc->write_proc = gsl_config_write_proc;
	}
#else
	proc_create(GSL_CONFIG_PROC_FILE, 0664, NULL, &gsl_seq_fops);
#endif
	gsl_proc_flag = 0;
#endif

#ifdef GSL_MONITOR
	pr_debug("gsl_ts_probe () : queue gsl_monitor_workqueue\n");

	INIT_DELAYED_WORK(&gsl_monitor_work, gsl_monitor_worker);
	gsl_monitor_workqueue = create_singlethread_workqueue("gsl_monitor_workqueue");
	queue_delayed_work(gsl_monitor_workqueue, &gsl_monitor_work, 1000);
#endif
	class_register(&ctp_class);
	return 0;

error_mutex_destroy:
	mutex_destroy(&ts->sus_lock);
	input_free_device(ts->input);
	kfree(ts);
	return rc;
}

static int  gsl_ts_remove(struct i2c_client *client)
{
	struct gsl_ts *ts = i2c_get_clientdata(client);

	pr_debug("==gsl_ts_remove=\n");

	class_unregister(&ctp_class);
	pm_runtime_disable(&client->dev);
	pm_runtime_set_suspended(&client->dev);
	sysfs_remove_group(&ts->input->dev.kobj, &gsl_attr_group);

	device_remove_file(&ts->input->dev, &dev_attr_debug_reg);
#ifdef GSL_MONITOR
	cancel_delayed_work_sync(&gsl_monitor_work);
	destroy_workqueue(gsl_monitor_workqueue);
#endif

	device_init_wakeup(&client->dev, 0);
	cancel_work_sync(&ts->work);
	cancel_work_sync(&glsX680_init_work);
	cancel_work_sync(&glsX680_resume_work);
	cancel_work_sync(&glsX680_idle_work);

#ifndef GSL_TIMER
	input_free_int(&(config_info.input_type), ts);
#else
	del_timer(&ts->gsl_timer);
#endif
#if GSL_MONITOR_TIMER_ENABLE
	del_timer(&monitor_timer);
#endif
	destroy_workqueue(ts->wq);
	destroy_workqueue(gslX680_wq);
	destroy_workqueue(gslX680_resume_wq);
	input_unregister_device(ts->input);
	mutex_destroy(&ts->sus_lock);
	kfree(ts->touch_data);
	kfree(ts);

	return 0;
}
static const struct of_device_id gsl_of_match[] = {
	{.compatible = "allwinner,gslX680"},
	{},
};
static const struct i2c_device_id gsl_ts_id[] = {
	{GSLX680_I2C_NAME, 0},
	{}
};
MODULE_DEVICE_TABLE(i2c, gsl_ts_id);

static UNIVERSAL_DEV_PM_OPS(gsl_pm_ops, gsl_ts_suspend,
		gsl_ts_resume, NULL);

#define GSL_PM_OPS (&gsl_pm_ops)

static struct i2c_driver gsl_ts_driver = {
	.class = I2C_CLASS_HWMON,
	.driver = {
		.of_match_table = gsl_of_match,
		.name = GSLX680_I2C_NAME,
		.owner = THIS_MODULE,
		.pm = GSL_PM_OPS,
	},
	.probe		= gsl_ts_probe,
	.remove		= gsl_ts_remove,
	.id_table		= gsl_ts_id,
	.address_list	= normal_i2c,
};
static int ctp_get_system_config(void)
{
	ctp_print_info(config_info);
	fwname = config_info.name;

	twi_id = config_info.twi_id;
	screen_max_x = config_info.screen_max_x;
	screen_max_y = config_info.screen_max_y;
	revert_x_flag = config_info.revert_x_flag;
	revert_y_flag = config_info.revert_y_flag;
	exchange_x_y_flag = config_info.exchange_x_y_flag;
	if ((screen_max_x == 0) || (screen_max_y == 0)) {
		pr_err("%s:read config error!\n", __func__);
		return 0;
	}

	return 1;
}
static int __init gsl_ts_init(void)
{
	int ret = -1;

	if (startup() != 1)
		return -1;
	if (!config_info.isI2CClient)
		gsl_ts_driver.detect = ctp_detect;
	ret = i2c_add_driver(&gsl_ts_driver);
	if (ret < 0) {
		printk("add gslX680 i2c driver failed\n");
	}
	return ret;
}

static void __exit gsl_ts_exit(void)
{
	pr_debug("==gsl_ts_exit==\n");
	i2c_del_driver(&gsl_ts_driver);
	input_sensor_free(&(config_info.input_type));
}

module_init(gsl_ts_init);
module_exit(gsl_ts_exit);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("GSLX680 touchscreen controller driver");
MODULE_AUTHOR("allwinner");
MODULE_VERSION("1.0.5");
MODULE_ALIAS("platform:gsl_ts");
