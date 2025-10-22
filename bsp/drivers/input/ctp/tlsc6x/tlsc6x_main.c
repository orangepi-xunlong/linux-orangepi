/*
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * * VERSION		DATE			AUTHOR		Note
 *
 */

#include <linux/firmware.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <asm/uaccess.h>
#include <linux/gpio.h>
#include <linux/regulator/consumer.h>
/* #include <soc/sprd/regulator.h> */
#include <linux/input/mt.h>
#include <linux/of_device.h>
#include <linux/of_address.h>
#include <linux/of_gpio.h>

#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/completion.h>
#include <linux/err.h>

#include <linux/proc_fs.h>
#include <linux/file.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <asm/unistd.h>
#include <asm/io.h>
#include <linux/pm_runtime.h>
#include <linux/types.h>

#include <linux/version.h>
#include <linux/fb.h>
#if defined(CONFIG_ADF)
#include <linux/notifier.h>
//#include <video/adf_notifier.h>
#include <linux/fb.h>
#elif defined(CONFIG_HAS_EARLYSUSPEND)
#include <linux/earlysuspend.h>
#endif

#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/ioctl.h>
//#include <linux/wakelock.h>
#include <linux/suspend.h>
#include <linux/irq.h>
#include <linux/string.h>

#include "tlsc6x_main.h"
#ifdef CONFIG_HWINFO
#include <linux/hwinfo.h>
static struct hwinfo *tp_hwinfo;
static char g_ft_tp_firmware[20];
static char *g_tp_devinfo = "CHSC6448_VGA";
static char *g_tp_devinfo1 = "CHSC6448_HD";
unsigned short version_cfg[102];
unsigned int cfg_ver;
int cfg_version = -1;
extern int tlsc6x_get_running_cfg(unsigned short *ptcfg);
#endif

#define	TOUCH_VIRTUAL_KEYS
#define	MULTI_PROTOCOL_TYPE_B	0
#define	TS_MAX_FINGER		2

#define MAX_CHIP_ID   (10)
#define TS_NAME		"tlsc6x_ts"
unsigned char tlsc6x_chip_name[MAX_CHIP_ID][20] = { "null", "tlsc6206a",
	"0x6306", "tlsc6206", "tlsc6324", "tlsc6332", "tlsc6440", "tlsc6432",
	"tlsc6424", "tlsc6448"
};

int g_is_telink_comp;

struct tlsc6x_data *g_tp_drvdata;
static struct i2c_client *this_client;
//static struct wake_lock tlsc6x_wakelock;
#ifdef TLSC_TPD_PROXIMITY
static int tlsc6x_prox_ctrl(int enable);
#endif
static int tlsc6x_do_suspend(void);
static int tlsc6x_do_resume(void);

DEFINE_MUTEX(i2c_rw_access);

#if defined(CONFIG_ADF)
static int tlsc6x_adf_suspend(void);
static int tlsc6x_adf_resume(void);
#elif defined(CONFIG_HAS_EARLYSUSPEND)
static void tlsc6x_ts_suspend(struct early_suspend *handler);
static void tlsc6x_ts_resume(struct early_suspend *handler);
#endif

#ifdef TLSC_ESD_HELPER_EN
static int tpd_esd_flag;
static struct hrtimer tpd_esd_kthread_timer;
static DECLARE_WAIT_QUEUE_HEAD(tpd_esd_waiter);
#endif

#ifdef TLSC_TPD_PROXIMITY
unsigned char tpd_prox_old_state;
static int tpd_prox_active;
static struct class *sprd_tpd_class;
static struct device *sprd_ps_cmd_dev;
#endif

#ifdef TOUCH_VIRTUAL_KEYS

static ssize_t virtual_keys_show(struct kobject *kobj,
				 struct kobj_attribute *attr, char *buf)
{
	struct tlsc6x_data *data = i2c_get_clientdata(this_client);
	struct tlsc6x_platform_data *pdata = data->platform_data;

	return snprintf(buf, PAGE_SIZE,
			"%s:%s:%d:%d:%d:%d:%s:%s:%d:%d:%d:%d:%s:%s:%d:%d:%d:%d\n",
			__stringify(EV_KEY), __stringify(KEY_APPSELECT),
			pdata->virtualkeys[0], pdata->virtualkeys[1],
			pdata->virtualkeys[2], pdata->virtualkeys[3],
			__stringify(EV_KEY), __stringify(KEY_HOMEPAGE),
			pdata->virtualkeys[4], pdata->virtualkeys[5],
			pdata->virtualkeys[6], pdata->virtualkeys[7],
			__stringify(EV_KEY), __stringify(KEY_BACK),
			pdata->virtualkeys[8], pdata->virtualkeys[9],
			pdata->virtualkeys[10], pdata->virtualkeys[11]);
}

static struct kobj_attribute virtual_keys_attr = {
	.attr = {.name = "XXXXXXXXXXXXXXXXXXXXXXXX", .mode = 0444,}, .show =
	    &virtual_keys_show,
};

static struct attribute *properties_attrs[] = { &virtual_keys_attr.attr,
	NULL
};

static struct attribute_group properties_attr_group = {.attrs =
	    properties_attrs,
};

static void tlsc6x_virtual_keys_init(void)
{
	int ret = 0;
	struct kobject *properties_kobj;

	TLSC_FUNC_ENTER();

	//properties_kobj = kobject_create_and_add("board_properties", NULL);
	properties_kobj = kobject_create_and_add("board_properties",
						 &g_tp_drvdata->input_dev->dev.
						 kobj);
	if (properties_kobj) {
		ret =
		    sysfs_create_group(properties_kobj, &properties_attr_group);
	}
	if (!properties_kobj || ret) {
		tlsc_err("failed to create board_properties\n");
	}
}

#endif

/*
 iic access interface
 */
int tlsc6x_i2c_read_sub(struct i2c_client *client, char *writebuf, int writelen,
			char *readbuf, int readlen)
{
	int ret = 0;

	if (client == NULL) {
		tlsc_err("[IIC][%s]i2c_client==NULL!\n", __func__);
		return -EINVAL;
	}

	if (readlen > 0) {
		if (writelen > 0) {
			struct i2c_msg msgs[] = { {.addr = client->addr, .flags =
						   0, .len = writelen, .buf =
						   writebuf,}, {.addr =
								client->addr,
								.flags =
								I2C_M_RD, .len =
								readlen, .buf =
								readbuf,},
			};
			ret = i2c_transfer(client->adapter, msgs, 1);
			if (ret < 0) {
				tlsc_err
				    ("[IIC]: i2c_transfer(1) error, addr= 0x%x!!\n",
				     writebuf[0]);
				tlsc_err
				    ("[IIC]: i2c_transfer(1) error, ret=%d, rlen=%d, wlen=%d!!\n",
				     ret, readlen, writelen);
			} else {
				ret =
				    i2c_transfer(client->adapter, &msgs[1], 1);
				if (ret < 0) {
					tlsc_err
					    ("[IIC]: i2c_transfer(2) error, addr= 0x%x!!\n",
					     writebuf[0]);
					tlsc_err
					    ("[IIC]: i2c_transfer(2) error, ret=%d, rlen=%d, wlen=%d!!\n",
					     ret, readlen, writelen);
				}
			}
		} else {
			struct i2c_msg msgs[] = { {.addr = client->addr, .flags =
						   I2C_M_RD,
						   .len = readlen, .buf =
						   readbuf,},
			};
			ret = i2c_transfer(client->adapter, msgs, 1);
			if (ret < 0) {
				tlsc_err
				    ("[IIC]: i2c_transfer(read) error, ret=%d, rlen=%d, wlen=%d!!",
				     ret, readlen, writelen);
			}
		}
	}

	return ret;
}

/* fail : <0 */
int tlsc6x_i2c_read(struct i2c_client *client, char *writebuf, int writelen,
		    char *readbuf, int readlen)
{
	int ret = 0;

	/* lock in this function so we can do direct mode iic transfer in debug fun */
	mutex_lock(&i2c_rw_access);
	ret = tlsc6x_i2c_read_sub(client, writebuf, writelen, readbuf, readlen);

	mutex_unlock(&i2c_rw_access);

	return ret;
}

/* fail : <0 */
int tlsc6x_i2c_write_sub(struct i2c_client *client, char *writebuf,
			 int writelen)
{
	int ret = 0;

	if (client == NULL) {
		tlsc_err("[IIC][%s]i2c_client==NULL!\n", __func__);
		return -EINVAL;
	}

	if (writelen > 0) {
		struct i2c_msg msgs[] = { {.addr = client->addr, .flags =
					   0, .len = writelen, .buf = writebuf,},
		};
		ret = i2c_transfer(client->adapter, msgs, 1);
		if (ret < 0) {
			tlsc_err("[IIC]: i2c_transfer(write) error, ret=%d!!\n",
				 ret);
		}
	}

	return ret;

}

/* fail : <0 */
int tlsc6x_i2c_write(struct i2c_client *client, char *writebuf, int writelen)
{
	int ret = 0;

	mutex_lock(&i2c_rw_access);
	ret = tlsc6x_i2c_write_sub(client, writebuf, writelen);
	mutex_unlock(&i2c_rw_access);

	return ret;

}

/* fail : <0 */
int tlsc6x_write_reg(struct i2c_client *client, u8 regaddr, u8 regvalue)
{
	unsigned char buf[2] = { 0 };

	buf[0] = regaddr;
	buf[1] = regvalue;

	return tlsc6x_i2c_write(client, buf, sizeof(buf));
}

/* fail : <0 */
int tlsc6x_read_reg(struct i2c_client *client, u8 regaddr, u8 *regvalue)
{
	return tlsc6x_i2c_read(client, &regaddr, 1, regvalue, 1);
}

static void tlsc6x_clear_report_data(struct tlsc6x_data *drvdata)
{
	int i;

	for (i = 0; i < TS_MAX_FINGER; i++) {
#if MULTI_PROTOCOL_TYPE_B
		input_mt_slot(drvdata->input_dev, i);
		input_mt_report_slot_state(drvdata->input_dev, MT_TOOL_FINGER,
					   false);
#endif
	}

	input_report_key(drvdata->input_dev, BTN_TOUCH, 0);
#if !MULTI_PROTOCOL_TYPE_B
	input_mt_sync(drvdata->input_dev);
#endif
	input_sync(drvdata->input_dev);
}

void tlsc6x_irq_disable(void)
{
	TLSC_FUNC_ENTER();
	if (!g_tp_drvdata->irq_disabled) {
		disable_irq_nosync(this_client->irq);
		g_tp_drvdata->irq_disabled = true;
	}
}

void tlsc6x_irq_enable(void)
{
	TLSC_FUNC_ENTER();
	if (g_tp_drvdata->irq_disabled) {
		enable_irq(this_client->irq);
		g_tp_drvdata->irq_disabled = false;
	}
}

static int tlsc6x_update_data(void)
{
	struct tlsc6x_data *data = i2c_get_clientdata(this_client);
	struct ts_event *event = &data->event;
	u8 buf[20] = { 0 };
	int ret = -1;
	int i;
	u16 x, y;
	u8 tlsc_pressure, tlsc_size;

	ret = tlsc6x_i2c_read(this_client, buf, 1, buf, 18);
	if (ret < 0) {
		tlsc_err("%s read_data i2c_rxdata failed: %d\n", __func__, ret);
		return ret;
	}

	memset(event, 0, sizeof(struct ts_event));
	event->touch_point = buf[2] & 0x07;

#ifdef TLSC_TPD_PROXIMITY
	if (tpd_prox_active && (event->touch_point == 0)) {
		if (((buf[1] == 0xc0) || (buf[1] == 0xe0))
		    && (tpd_prox_old_state != buf[1])) {
			input_report_abs(g_tp_drvdata->ps_input_dev,
					 ABS_DISTANCE,
					 (buf[1] == 0xc0) ? 0 : 1);
			input_mt_sync(g_tp_drvdata->ps_input_dev);
			input_sync(g_tp_drvdata->ps_input_dev);
		}
		tpd_prox_old_state = buf[1];
	}
#endif

	for (i = 0; i < TS_MAX_FINGER; i++) {
		if ((buf[6 * i + 3] & 0xc0) == 0xc0) {
			continue;
		}
		x = (s16) (buf[6 * i + 3] & 0x0F) << 8 | (s16) buf[6 * i + 4];
		y = (s16) (buf[6 * i + 5] & 0x0F) << 8 | (s16) buf[6 * i + 6];

#ifdef CONFIG_OF
		if (data->platform_data) {
			if (data->platform_data->exchange_x_y_flag)
				swap(x, y);
			if (data->platform_data->revert_x_flag)
				x = data->platform_data->x_res_max - x;
			if (data->platform_data->revert_y_flag)
				y = data->platform_data->y_res_max - y;
		}
#endif

		tlsc_pressure = buf[6 * i + 7];
		if (tlsc_pressure > 127) {
			tlsc_pressure = 127;
		}
		tlsc_size = (buf[6 * i + 8] >> 4) & 0x0F;
		if ((buf[6 * i + 3] & 0x40) == 0x0) {
#if MULTI_PROTOCOL_TYPE_B
			input_mt_slot(data->input_dev, buf[6 * i + 5] >> 4);
			input_mt_report_slot_state(data->input_dev,
						   MT_TOOL_FINGER, true);
#else
			input_report_abs(data->input_dev, ABS_MT_TRACKING_ID,
					 buf[6 * i + 5] >> 4);
#endif
			if (y == 1500) {
				if (x == 40) {
					input_report_key(data->input_dev,
							 KEY_MENU, 1);
				}
				if (x == 80) {
					input_report_key(data->input_dev,
							 KEY_HOMEPAGE, 1);
				}
				if (x == 120) {
					input_report_key(data->input_dev,
							 KEY_BACK, 1);
				}
			} else {
//                      input_report_abs(data->input_dev, ABS_MT_POSITION_X, 319 - y);
//                      input_report_abs(data->input_dev, ABS_MT_POSITION_Y, x);
				input_report_abs(data->input_dev,
						 ABS_MT_POSITION_X, x);
				input_report_abs(data->input_dev,
						 ABS_MT_POSITION_Y, y);

				input_report_abs(data->input_dev,
						 ABS_MT_PRESSURE, 15);
				input_report_abs(data->input_dev,
						 ABS_MT_TOUCH_MAJOR, tlsc_size);
				input_report_key(data->input_dev, BTN_TOUCH, 1);
			}
#if !MULTI_PROTOCOL_TYPE_B
			input_mt_sync(data->input_dev);
#endif
		} else {
#if MULTI_PROTOCOL_TYPE_B
			input_mt_slot(data->input_dev, buf[6 * i + 5] >> 4);
			input_mt_report_slot_state(data->input_dev,
						   MT_TOOL_FINGER, false);
#endif
		}
	}
	if (event->touch_point == 0) {
		if (y == 1500) {
			if (x == 40) {
				input_report_key(data->input_dev, KEY_MENU, 0);
			}
			if (x == 80) {
				input_report_key(data->input_dev, KEY_HOMEPAGE,
						 0);
			}
			if (x == 120) {
				input_report_key(data->input_dev, KEY_BACK, 0);
			}
		}
		tlsc6x_clear_report_data(data);
	}
	input_sync(data->input_dev);

	return 0;

}

static irqreturn_t touch_event_thread_handler(int irq, void *devid)
{
	tlsc6x_update_data();

	return IRQ_HANDLED;
}

void tlsc6x_tpd_reset_force(void)
{
	struct tlsc6x_platform_data *pdata = g_tp_drvdata->platform_data;

	TLSC_FUNC_ENTER();

	gpio_direction_output(pdata->reset_gpio_number, 1);
	usleep_range(10000, 11000);
	gpio_set_value(pdata->reset_gpio_number, 0);
	msleep(20);
	gpio_set_value(pdata->reset_gpio_number, 1);
	msleep(30);
}

static void tlsc6x_tpd_reset(void)
{
	TLSC_FUNC_ENTER();
	if (g_tp_drvdata->needKeepRamCode) {
		return;
	}

	tlsc6x_tpd_reset_force();
}

static unsigned char real_suspend_flag;

#if 1
static int tlsc6x_do_suspend(void)
{
	TLSC_FUNC_ENTER();

#ifdef TLSC_ESD_HELPER_EN
	hrtimer_cancel(&tpd_esd_kthread_timer);
#endif

#ifdef TLSC_TPD_PROXIMITY
	if (tpd_prox_active) {
		real_suspend_flag = 0;
		return 0;
	}
#endif

	tlsc6x_irq_disable();
#ifdef TLSC_GESTRUE
	tlsc6x_enter_gest_mode(0xffffff);
	enable_irq_wake(this_client->irq);
	irq_set_irq_type(this_client->irq,
			 IRQF_TRIGGER_LOW | IRQF_NO_SUSPEND | IRQF_ONESHOT);
	tlsc6x_irq_enable();
#else
	if (tlsc6x_write_reg(this_client, 0xa5, 0x03) < 0) {
		tlsc_err("tlsc6x error::setup suspend fail!\n");
	}
#endif

	real_suspend_flag = 1;
	tlsc6x_clear_report_data(g_tp_drvdata);
	return 0;
}

static int tlsc6x_do_resume(void)
{
	TLSC_FUNC_ENTER();
#ifdef TLSC_ESD_HELPER_EN
	hrtimer_start(&tpd_esd_kthread_timer, ktime_set(3, 0),
		      HRTIMER_MODE_REL);
#endif

#ifdef TLSC_TPD_PROXIMITY
	if (tpd_prox_active && (real_suspend_flag == 0)) {
		return 0;
	}
#endif

	queue_work(g_tp_drvdata->tp_resume_workqueue,
		   &g_tp_drvdata->resume_work);
	return 0;
}

#endif
#if defined(CONFIG_ADF)
static int tlsc6x_adf_suspend(void)
{
	TLSC_FUNC_ENTER();

	return tlsc6x_do_suspend();
}

static int tlsc6x_adf_resume(void)
{
	TLSC_FUNC_ENTER();

	return tlsc6x_do_resume();
}
#elif defined(CONFIG_HAS_EARLYSUSPEND)
static void tlsc6x_ts_suspend(struct early_suspend *handler)
{
	TLSC_FUNC_ENTER();

	tlsc6x_do_suspend();
}

static void tlsc6x_ts_resume(struct early_suspend *handler)
{
	TLSC_FUNC_ENTER();

	tlsc6x_do_resume();
}
#endif

static void tlsc6x_resume_work(struct work_struct *work)
{
	TLSC_FUNC_ENTER();

	tlsc6x_tpd_reset();

	if (g_tp_drvdata->needKeepRamCode) {	/* need wakeup cmd in this mode */
		tlsc6x_write_reg(this_client, 0xa5, 0x00);
	}

	tlsc6x_clear_report_data(g_tp_drvdata);

#ifdef TLSC_GESTRUE
	disable_irq_wake(this_client->irq);
	irq_set_irq_type(this_client->irq,
			 IRQF_TRIGGER_FALLING | IRQF_ONESHOT | IRQF_NO_SUSPEND);
#endif

	tlsc6x_irq_enable();
	real_suspend_flag = 0;
}

#if defined(CONFIG_ADF)
/*
 * touchscreen's suspend and resume state should rely on screen state,
 * as fb_notifier and early_suspend are all disabled on our platform,
 * we can only use adf_event now
 */
static int ts_adf_event_handler(struct notifier_block *nb, unsigned long action,
				void *data)
{

	struct fb_event *event = data;
	int adf_event_data;

//      if (action != ADF_EVENT_BLANK) {
	if (action != FB_EVENT_BLANK) {
		return NOTIFY_DONE;
	}
	adf_event_data = *(int *)event->data;
	tlsc_info("receive adf event with adf_event_data=%d", adf_event_data);

	switch (adf_event_data) {
//      case DRM_MODE_DPMS_ON:
	case FB_BLANK_UNBLANK:
		tlsc6x_adf_resume();
		break;
//      case DRM_MODE_DPMS_OFF:
	case FB_BLANK_POWERDOWN:
		tlsc6x_adf_suspend();
		break;
	default:
		tlsc_info
		    ("receive adf event with error data, adf_event_data=%d",
		     adf_event_data);
		break;
	}

	return NOTIFY_OK;
}
#endif

static int tlsc6x_hw_init(struct tlsc6x_data *drvdata)
{
	struct regulator *reg_vdd;
	struct i2c_client *client = drvdata->client;
	struct tlsc6x_platform_data *pdata = drvdata->platform_data;
//      int ret = 0;

	TLSC_FUNC_ENTER();
	if (gpio_request(pdata->irq_gpio_number, NULL) < 0) {
		goto OUT;
	}
	if (gpio_request(pdata->reset_gpio_number, NULL) < 0) {
		goto OUT;
	}
	gpio_direction_output(pdata->reset_gpio_number, 1);
	gpio_direction_input(pdata->irq_gpio_number);

	drvdata->reg_vdd = NULL;
	if (pdata->vdd_name != NULL) {
		//      reg_vdd = regulator_get(&client->dev, pdata->vdd_name);
		reg_vdd = regulator_get(&client->dev, "vdd");
		if (!WARN(IS_ERR(reg_vdd),
			  "tlsc6x_hw_init regulator: failed to get %s.\n",
			  pdata->vdd_name)) {
			regulator_set_voltage(reg_vdd, 2800000, 2800000);
			if (regulator_enable(reg_vdd)) {
				tlsc_info
				    ("tlsc6x_hw_init:regulator_enable return none zero\n");
			}
			if (regulator_is_enabled(reg_vdd) == 0) {
				tlsc_err
				    ("tlsc6x_hw_init:regulator_enable fail\n");
			}
			drvdata->reg_vdd = reg_vdd;
		}
	}

	msleep(100);
	tlsc6x_tpd_reset();
	return 0;
OUT:	return -EPERM;
}

#ifdef CONFIG_OF
static struct tlsc6x_platform_data *tlsc6x_parse_dt(struct device *dev)
{
	int ret;
	struct tlsc6x_platform_data *pdata;
	struct device_node *np = dev->of_node;

	TLSC_FUNC_ENTER();
	pdata = kzalloc(sizeof(*pdata), GFP_KERNEL);
	if (!pdata) {
		dev_err(dev, "Could not allocate struct tlsc6x_platform_data");
		return NULL;
	}

	pdata->reset_gpio_number = of_get_named_gpio(np, "reset-gpio",
							   0);
	if (pdata->reset_gpio_number < 0) {
		dev_err(dev, "fail to get reset_gpio_number\n");
		goto fail;
	}

	pdata->irq_gpio_number = of_get_named_gpio(np, "irq-gpio",
							 0);
	if (pdata->irq_gpio_number < 0) {
		dev_err(dev, "fail to get irq_gpio_number\n");
		goto fail;
	}

	ret = of_property_read_string(np, "vdd-supply", &pdata->vdd_name);
	if (ret) {
		dev_err(dev, "fail to get vdd_name\n");
		/* goto fail; */
	}
#ifdef TOUCH_VIRTUAL_KEYS
	ret =
	    of_property_read_u32_array(np, "virtualkeys", pdata->virtualkeys,
				       12);
	if (ret) {
		dev_err(dev, "fail to get virtualkeys\n");
		/* goto fail; */
	}
#endif
	ret = of_property_read_u32(np, "TP_MAX_X", &pdata->x_res_max);
	if (ret) {
		dev_err(dev, "fail to get TP_MAX_X\n");
		goto fail;
	}
	ret = of_property_read_u32(np, "TP_MAX_Y", &pdata->y_res_max);
	if (ret) {
		dev_err(dev, "fail to get TP_MAX_Y\n");
		goto fail;
	}
	ret = of_property_read_u32(np, "revert_x_flag", &pdata->revert_x_flag);
	if (ret) {
		dev_err(dev, "fail to get revert_x_flag\n");
		goto fail;
	}
	ret = of_property_read_u32(np, "revert_y_flag", &pdata->revert_y_flag);
	if (ret) {
		dev_err(dev, "fail to get revert_y_flag\n");
		goto fail;
	}
	ret =
	    of_property_read_u32(np, "exchange_x_y_flag",
				 &pdata->exchange_x_y_flag);
	if (ret) {
		dev_err(dev, "fail to get exchange_x_y_flag\n");
		goto fail;
	}

	return pdata;
fail:
	kfree(pdata);
	return NULL;
}
#endif
/* file interface - write*/
/*int tlsc6x_fif_write(char *fname, u8 *pdata, u16 len)
{
	int ret = 0;
	loff_t pos = 0;
	static struct file *pfile;
	mm_segment_t old_fs = KERNEL_DS;

	pfile = filp_open(fname, O_TRUNC | O_CREAT | O_RDWR, 0644);
	if (IS_ERR(pfile)) {
		ret = -EFAULT;
		tlsc_err("tlsc6x tlsc6x_fif_write:open error!\n");
	} else {
		tlsc_info("tlsc6x tlsc6x_fif_write:start write!\n");
		old_fs = get_fs();
		set_fs(KERNEL_DS);
		ret =
		    (int)vfs_write(pfile, (__force const char __user *)pdata,
				   (size_t) len, &pos);
		vfs_fsync(pfile, 0);
		filp_close(pfile, NULL);
		set_fs(old_fs);
	}
	return ret;
}*/

#if (defined TPD_AUTO_UPGRADE_PATH) || (defined TLSC_APK_DEBUG)
extern int tlsx6x_update_running_cfg(u16 *ptcfg);
extern int tlsx6x_update_burn_cfg(u16 *ptcfg);
extern int tlsc6x_load_ext_binlib(u8 *pcode, u16 len);
extern int tlsc6x_update_f_combboot(u8 *pdata, u16 len);
int auto_upd_busy;
/* 0:success */
/* 1: no file OR open fail */
/* 2: wrong file size OR read error */
/* -1:op-fial */
int tlsc6x_proc_cfg_update(u8 *dir, int behave)
{
#if 0
	int ret = 1;
	u8 *pbt_buf = NULL;
	u32 fileSize;
	mm_segment_t old_fs;
	static struct file *file;

	TLSC_FUNC_ENTER();
	tlsc_info("tlsc6x proc-file:%s\n", dir);

	file = filp_open(dir, O_RDONLY, 0);
	if (IS_ERR(file)) {
		tlsc_err("tlsc6x proc-file:open error!\n");
	} else {
		ret = 2;
		old_fs = get_fs();
		set_fs(KERNEL_DS);
		fileSize = file->f_op->llseek(file, 0, SEEK_END);
		tlsc_info("tlsc6x proc-file, size:%d\n", fileSize);
		pbt_buf = kmalloc(fileSize, GFP_KERNEL);

		file->f_op->llseek(file, 0, SEEK_SET);
		if (fileSize
		    == file->f_op->read(file, (char *)pbt_buf, fileSize,
					&file->f_pos)) {
			tlsc_info("tlsc6x proc-file, read ok1!\n");
			ret = 3;
		}

		if (ret == 3) {
			auto_upd_busy = 1;
			tlsc6x_irq_disable();
			msleep(1000);
			//wake_lock_timeout(&tlsc6x_wakelock,
			//		  msecs_to_jiffies(2000));
			if (behave == 0) {
				if (fileSize == 204) {
					ret = tlsx6x_update_running_cfg((u16 *)
									pbt_buf);
				} else if (fileSize > 0x400) {
					tlsc6x_load_ext_binlib((u8 *) pbt_buf,
							       (u16) fileSize);
				}
			} else if (behave == 1) {
				if (fileSize == 204) {
					ret = tlsx6x_update_burn_cfg((u16 *)
								     pbt_buf);
				} else if (fileSize > 0x400) {
					ret = tlsc6x_update_f_combboot((u8 *)
								       pbt_buf,
								       (u16)
								       fileSize);
				}
				tlsc6x_tpd_reset();
			}
			tlsc6x_irq_enable();
			auto_upd_busy = 0;
		}

		filp_close(file, NULL);
		set_fs(old_fs);

		kfree(pbt_buf);
	}

	return ret;
#else
	return 0;
#endif
}

#endif

#ifdef TLSC_APK_DEBUG
unsigned char proc_out_len;
unsigned char proc_out_buf[256];

unsigned char debug_type;
unsigned char iic_reg[2];
unsigned char sync_flag_addr[3];
unsigned char sync_buf_addr[2];
unsigned char reg_len;

static struct proc_dir_entry *tlsc6x_proc_entry;

static int debug_read(char *writebuf, int writelen, char *readbuf, int readlen)
{
	int ret = 0;

	mutex_lock(&i2c_rw_access);
	tlsc6x_set_dd_mode_sub();

	ret = tlsc6x_i2c_read_sub(this_client, writebuf, writelen, readbuf,
				  readlen);

	tlsc6x_set_nor_mode_sub();
	mutex_unlock(&i2c_rw_access);
	if (ret > 0) {
		ret = readlen;
	}
	return ret;
}

static int debug_write(char *writebuf, int writelen)
{
	int ret = 0;

	mutex_lock(&i2c_rw_access);
	tlsc6x_set_dd_mode_sub();

	ret = tlsc6x_i2c_write_sub(this_client, writebuf, writelen);

	tlsc6x_set_nor_mode_sub();
	mutex_unlock(&i2c_rw_access);
	if (ret > 0) {
		ret = writelen;
	}
	return ret;
}

static int debug_read_sync(char *writebuf, int writelen, char *readbuf,
			   int readlen)
{
	int ret = 0;
	int retryTime;

	mutex_lock(&i2c_rw_access);
	tlsc6x_set_dd_mode_sub();
	sync_flag_addr[2] = 1;
	ret = tlsc6x_i2c_write_sub(this_client, sync_flag_addr, 3);

	retryTime = 100;
	do {
		ret = tlsc6x_i2c_read_sub(this_client, sync_flag_addr, 2,
					  &sync_flag_addr[2], 1);
		if (ret < 0) {
			mutex_unlock(&i2c_rw_access);
			return ret;
		}
		retryTime--;
	} while (retryTime > 0 && sync_flag_addr[2] == 1);
	if (retryTime == 0 && sync_flag_addr[2] == 1) {
		mutex_unlock(&i2c_rw_access);
		return -EFAULT;
	}
	if (ret >= 0) {
		/* read data */
		ret =
		    tlsc6x_i2c_read_sub(this_client, sync_buf_addr, 2, readbuf,
					readlen);
	}

	tlsc6x_set_nor_mode_sub();
	mutex_unlock(&i2c_rw_access);
	if (ret > 0) {
		ret = readlen;
	}
	return ret;
}

static int tlsc6x_rawdata_test_3535allch(u8 *buf, int len)
{
	int ret;
	int retryTime;
	u8 writebuf[4];

	buf[len] = '\0';
	ret = 0;
	tlsc6x_irq_disable();
	g_tp_drvdata->esdHelperFreeze = 1;
	tlsc6x_tpd_reset();
	if (tlsc6x_proc_cfg_update(&buf[2], 0)) {
		ret = -EIO;
	}
	msleep(30);

	mutex_lock(&i2c_rw_access);
	//write addr
	writebuf[0] = 0x9F;
	writebuf[1] = 0x20;
	writebuf[2] = 48;
	writebuf[3] = 0xFF;
	ret = tlsc6x_i2c_write_sub(this_client, writebuf, 4);
	writebuf[0] = 0x9F;
	writebuf[1] = 0x24;
	writebuf[2] = 1;

	ret = tlsc6x_i2c_write_sub(this_client, writebuf, 3);
	retryTime = 100;
	do {
		ret =
		    tlsc6x_i2c_read_sub(this_client, writebuf, 2, &writebuf[2],
					1);
		if (ret < 0) {
			break;
		}
		retryTime--;
		msleep(30);
	} while (retryTime > 0 && writebuf[2] == 1);

	if (ret >= 0) {
		writebuf[0] = 0x9F;
		writebuf[1] = 0x26;
		ret =
		    tlsc6x_i2c_read_sub(this_client, writebuf, 2, proc_out_buf,
					96);
		if (ret >= 0) {
			proc_out_len = 96;
		}
	}

	mutex_unlock(&i2c_rw_access);

	tlsc6x_tpd_reset();

	g_tp_drvdata->esdHelperFreeze = 0;
	tlsc6x_irq_enable();
	return ret;
}

static ssize_t tlsc6x_proc_write(struct file *filp, const char __user *buff,
				 size_t len, loff_t *data)
{
	int ret;
	int buflen = len;
	unsigned char local_buf[256];

	if (buflen > 255) {
		return -EFAULT;
	}

	if (copy_from_user(local_buf, buff, buflen)) {
		tlsc_err("%s:copy from user error\n", __func__);
		return -EFAULT;
	}
	ret = 0;
	debug_type = local_buf[0];
/* format:cmd+para+data0+data1+data2... */
	switch (local_buf[0]) {
	case 0:		/* cfg version */
		proc_out_len = 4;
		proc_out_buf[0] = g_tlsc6x_cfg_ver;
		proc_out_buf[1] = g_tlsc6x_cfg_ver >> 8;
		proc_out_buf[2] = g_tlsc6x_cfg_ver >> 16;
		proc_out_buf[3] = g_tlsc6x_cfg_ver >> 24;
		break;
	case 1:
		local_buf[buflen] = '\0';
		if (tlsc6x_proc_cfg_update(&local_buf[2], 0) < 0) {
			len = -EIO;
		}
		break;
	case 2:
		local_buf[buflen] = '\0';
		if (tlsc6x_proc_cfg_update(&local_buf[2], 1) < 0) {
			len = -EIO;
		}
		break;
	case 3:
		ret = debug_write(&local_buf[1], len - 1);
		break;
	case 4:		/* read */
		reg_len = local_buf[1];
		iic_reg[0] = local_buf[2];
		iic_reg[1] = local_buf[3];
		break;
	case 5:		/* read with sync */
		ret = debug_write(&local_buf[1], 4);	/* write size */
		if (ret >= 0) {
			ret = debug_write(&local_buf[5], 4);	/* write addr */
		}
		sync_flag_addr[0] = local_buf[9];
		sync_flag_addr[1] = local_buf[10];
		sync_buf_addr[0] = local_buf[11];
		sync_buf_addr[1] = local_buf[12];
		break;
	case 8:		// Force reset ic
		tlsc6x_tpd_reset_force();
		break;
	case 9:		// raw test
		ret = tlsc6x_rawdata_test_3535allch(local_buf, buflen);
		break;
	case 14:		/* e, esd control */
		g_tp_drvdata->esdHelperFreeze = (int)local_buf[1];
		break;

	default:
		break;
	}
	if (ret < 0) {
		len = ret;
	}

	return len;
}

static ssize_t tlsc6x_proc_read(struct file *filp, char __user *page,
				size_t len, loff_t *pos)
{
	int ret = 0;

	if (*pos != 0) {
		return 0;
	}

	switch (debug_type) {
	case 0:		/* version information */
		proc_out_len = 4;
		proc_out_buf[0] = g_tlsc6x_cfg_ver;
		proc_out_buf[1] = g_tlsc6x_cfg_ver >> 8;
		proc_out_buf[2] = g_tlsc6x_cfg_ver >> 16;
		proc_out_buf[3] = g_tlsc6x_cfg_ver >> 24;
		if (copy_to_user(page, proc_out_buf, proc_out_len)) {
			ret = -EFAULT;
		} else {
			ret = proc_out_len;
		}
		break;
	case 1:
		break;
	case 2:
		break;
	case 3:
		break;
	case 4:
		len = debug_read(iic_reg, reg_len, proc_out_buf, len);
		if (len > 0) {
			if (copy_to_user(page, proc_out_buf, len)) {
				ret = -EFAULT;
			} else {
				ret = len;
			}
		} else {
			ret = len;
		}
		break;
	case 5:
		len = debug_read_sync(iic_reg, reg_len, proc_out_buf, len);
		if (len > 0) {
			if (copy_to_user(page, proc_out_buf, len)) {
				ret = -EFAULT;
			} else {
				ret = len;
			}
		} else {
			ret = len;
		}
		break;
	case 9:
		if (proc_out_buf > 0) {
			if (copy_to_user(page, proc_out_buf, len)) {
				ret = -EFAULT;
			} else {
				ret = proc_out_len;
			}
		}
		break;
	default:
		break;
	}

	if (ret > 0) {
		*pos += ret;
	}

	return ret;
}

static const struct proc_ops tlsc6x_proc_ops = {.proc_read = tlsc6x_proc_read,
	.proc_write = tlsc6x_proc_write,
};

void tlsc6x_release_apk_debug_channel(void)
{
	if (tlsc6x_proc_entry) {
		remove_proc_entry("tlsc6x-debug", NULL);
	}
}

int tlsc6x_create_apk_debug_channel(struct i2c_client *client)
{
	tlsc6x_proc_entry =
	    proc_create("tlsc6x-debug", 0x0644, NULL, &tlsc6x_proc_ops);

	if (tlsc6x_proc_entry == NULL) {
		dev_err(&client->dev, "Couldn't create proc entry!\n");
		return -ENOMEM;
	}
	dev_info(&client->dev, "Create proc entry success!\n");

	return 0;
}
#endif

extern unsigned int g_mccode;
static ssize_t show_tlsc_version(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	u8 reg[2];
	u8 readBuf[4];
	char *ptr = buf;
	u8 vender_id;

	mutex_lock(&i2c_rw_access);
	tlsc6x_set_dd_mode_sub();

	reg[0] = 0x80;
	reg[1] = 0x04;
	tlsc6x_i2c_read_sub(this_client, reg, 2, readBuf, 2);
	ptr += sprintf(ptr, "The boot version is %04X.\n",
		       (readBuf[0] + (readBuf[1] << 8)));

	if (g_mccode == 0) {
		reg[0] = 0xD6;
		reg[1] = 0xE0;
	} else {
		reg[0] = 0x9E;
		reg[1] = 0x00;
	}
	tlsc6x_i2c_read_sub(this_client, reg, 2, readBuf, 4);
	ptr += sprintf(ptr, "The config version is %d.\n", readBuf[3] >> 2);

	vender_id = readBuf[1] >> 1;
	ptr +=
	    sprintf(ptr, "The vender id is %d, the vender name is ", vender_id);

	switch (vender_id) {
	case 1:
		ptr += sprintf(ptr, "xufang");
		break;
	case 2:
		ptr += sprintf(ptr, "xuri");
		break;
	case 3:
		ptr += sprintf(ptr, "yuye");
		break;
	case 4:
		ptr += sprintf(ptr, "tianyi");
		break;
	case 5:
		ptr += sprintf(ptr, "minglang");
		break;
	case 6:
		ptr += sprintf(ptr, "duoxinda");
		break;
	case 7:
		ptr += sprintf(ptr, "zhenhua");
		break;
	case 8:
		ptr += sprintf(ptr, "jitegao");
		break;
	case 9:
		ptr += sprintf(ptr, "guangjishengtai");
		break;
	case 10:
		ptr += sprintf(ptr, "shengguang");
		break;
	case 11:
		ptr += sprintf(ptr, "xintiantong");
		break;
	case 12:
		ptr += sprintf(ptr, "xinyou");
		break;
	case 13:
		ptr += sprintf(ptr, "yanqi");
		break;
	case 14:
		ptr += sprintf(ptr, "zhongcheng");
		break;
	case 15:
		ptr += sprintf(ptr, "xinmaoxin");
		break;
	case 16:
		ptr += sprintf(ptr, "zhenzhixin");
		break;
	case 17:
		ptr += sprintf(ptr, "helitai");
		break;
	case 18:
		ptr += sprintf(ptr, "huaxin");
		break;
	case 19:
		ptr += sprintf(ptr, "lihaojie");
		break;
	case 20:
		ptr += sprintf(ptr, "jiandong");
		break;
	case 21:
		ptr += sprintf(ptr, "xinpengda");
		break;
	case 22:
		ptr += sprintf(ptr, "jiake");
		break;
	case 23:
		ptr += sprintf(ptr, "yijian");
		break;
	case 24:
		ptr += sprintf(ptr, "yixing");
		break;
	case 25:
		ptr += sprintf(ptr, "zhongguangdian");
		break;
	case 26:
		ptr += sprintf(ptr, "hongzhan");
		break;
	case 27:
		ptr += sprintf(ptr, "huaxingda");
		break;
	case 28:
		ptr += sprintf(ptr, "dongjianhuanyu");
		break;
	case 29:
		ptr += sprintf(ptr, "dawosi");
		break;
	case 30:
		ptr += sprintf(ptr, "dacheng");
		break;
	case 31:
		ptr += sprintf(ptr, "mingwangda");
		break;
	case 32:
		ptr += sprintf(ptr, "huangze");
		break;
	case 33:
		ptr += sprintf(ptr, "jinxinxiang");
		break;
	case 34:
		ptr += sprintf(ptr, "gaoge");
		break;
	case 35:
		ptr += sprintf(ptr, "zhihui");
		break;
	case 36:
		ptr += sprintf(ptr, "miaochu");
		break;
	case 37:
		ptr += sprintf(ptr, "qicai");
		break;
	case 38:
		ptr += sprintf(ptr, "zhenghai");
		break;
	case 39:
		ptr += sprintf(ptr, "hongfazhan");
		break;
	case 40:
		ptr += sprintf(ptr, "lianchuang");
		break;
	case 41:
		ptr += sprintf(ptr, "saihua");
		break;
	case 42:
		ptr += sprintf(ptr, "keleli");
		break;

	case 43:
		ptr += sprintf(ptr, "weiyi");
		break;
	case 44:
		ptr += sprintf(ptr, "futuo");
		break;
	default:
		ptr += sprintf(ptr, "unknown");
		break;
	}
	ptr += sprintf(ptr, ".\n");

	tlsc6x_set_nor_mode_sub();
	mutex_unlock(&i2c_rw_access);

	return (ptr - buf);
}

static ssize_t store_tlsc_version(struct device *dev,
				  struct device_attribute *attr,
				  const char *buf, size_t count)
{

	return -EPERM;
}

static ssize_t show_tlsc_info(struct device *dev, struct device_attribute *attr,
			      char *buf)
{
	char *ptr = buf;

	ptr += sprintf(ptr, "Max finger number is %0d.\n", TS_MAX_FINGER);
	ptr += sprintf(ptr, "Int irq is %d.\n", this_client->irq);
	ptr +=
	    sprintf(ptr, "I2c address is 0x%02X(0x%02X).\n", this_client->addr,
		    (this_client->addr) << 1);

	return (ptr - buf);
}

static ssize_t store_tlsc_info(struct device *dev,
			       struct device_attribute *attr, const char *buf,
			       size_t count)
{

	return -EPERM;
}

static ssize_t show_tlsc_ps(struct device *dev, struct device_attribute *attr,
			    char *buf)
{
	char *ptr = buf;
#ifdef TLSC_TPD_PROXIMITY

	ptr += sprintf(ptr, "%d\n", tpd_prox_active);
#else
	ptr += sprintf(ptr, "No proximity function.\n");
#endif

	return (ptr - buf);
}

static ssize_t store_tlsc_ps(struct device *dev, struct device_attribute *attr,
			     const char *buf, size_t count)
{
#ifdef TLSC_TPD_PROXIMITY
	if (buf[0] == '0') {
		tlsc6x_prox_ctrl(0);
//ps off

	} else if (buf[0] == '1') {
//ps on
		tlsc6x_prox_ctrl(1);
	}
#endif
	return count;
}

static ssize_t show_tlsc_gesture(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	char *ptr = buf;
#ifdef TLSC_GESTRUE

	ptr += sprintf(ptr, "%d\n", gesture_enable);
#else
	ptr += sprintf(ptr, "No gesture function.\n");
#endif

	return (ptr - buf);
}

static ssize_t store_tlsc_gesture(struct device *dev,
				  struct device_attribute *attr,
				  const char *buf, size_t count)
{
#ifdef TLSC_GESTRUE
	if (buf[0] == '0') {
		//gesture off
		gesture_enable = 0;

	} else if (buf[0] == '1') {
		//gesture on
		gesture_enable = 1;
	}
#endif
	return count;
}

//static int debugResult=0;
u8 readFlashbuf[204];

static ssize_t show_tlsc_debug_flash(struct device *dev,
				     struct device_attribute *attr, char *buf)
{
	char *ptr = buf;
	int i;

	for (i = 0; i < 204; i++) {
		ptr += sprintf(ptr, "%d,", readFlashbuf[i]);
	}

	ptr += sprintf(ptr, "\n");

	return (ptr - buf);
}

extern int tlsc6x_download_ramcode(u8 *pcode, u16 len);
extern int tlsc6x_write_burn_space(u8 *psrc, u16 adr, u16 len);
extern int tlsc6x_read_burn_space(u8 *pdes, u16 adr, u16 len);
extern int tlsc6x_set_nor_mode(void);
extern unsigned char fw_fcode_burn[2024];
int writeFlash(u8 *buf, u16 addr, int len)
{
	//int ret=0;
	auto_upd_busy = 1;
	tlsc6x_irq_disable();
	if (tlsc6x_download_ramcode(fw_fcode_burn, sizeof(fw_fcode_burn))) {
		tlsc_err("Tlsc6x:write flash error:ram-code error!\n");
		return -EPERM;
	}

	if (tlsc6x_write_burn_space((unsigned char *)buf, addr, len)) {
		tlsc_err("Tlsc6x:write flash  fail!\n");
		return -EPERM;
	}
	tlsc6x_set_nor_mode();

	tlsc6x_tpd_reset();

	auto_upd_busy = 0;

	tlsc6x_irq_enable();
	return 0;
}

int readFlash(u16 addr, int len)
{
	//int ret=0;
	auto_upd_busy = 1;
	tlsc6x_irq_disable();
	if (tlsc6x_download_ramcode(fw_fcode_burn, sizeof(fw_fcode_burn))) {
		tlsc_err("Tlsc6x:write flash error:ram-code error!\n");
		return -EPERM;
	}

	if (tlsc6x_read_burn_space((unsigned char *)readFlashbuf, addr, len)) {
		tlsc_err("Tlsc6x:write flash  fail!\n");
		return -EPERM;
	}
	tlsc6x_set_nor_mode();

	tlsc6x_tpd_reset();

	auto_upd_busy = 0;

	tlsc6x_irq_enable();
	return 0;
}

static ssize_t store_tlsc_debug_flash(struct device *dev,
				      struct device_attribute *attr,
				      const char *buf, size_t count)
{
	u8 wBuf[16] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

	if (buf[0] == '0') {
//gesture off

	} else if (buf[0] == '1') {
//gesture on
		tlsc_info("not support!\n");
	} else if (buf[0] == '2') {
//gesture on
		writeFlash(wBuf, 0, 16);
	} else if (buf[0] == '3') {
//gesture on
		writeFlash(wBuf, 0x8000, 16);
	} else if (buf[0] == '4') {
//gesture on
		readFlash(0xF000, 204);
	} else if (buf[0] == '5') {
//gesture on
		readFlash(0, 204);
	} else if (buf[0] == '6') {
//gesture on
		readFlash(0x8000, 204);
	}

	return count;
}

static DEVICE_ATTR(tlsc_version, 0664, show_tlsc_version, store_tlsc_version);
static DEVICE_ATTR(tlsc_tp_info, 0664, show_tlsc_info, store_tlsc_info);
static DEVICE_ATTR(tlsc_ps_ctl, 0664, show_tlsc_ps, store_tlsc_ps);
static DEVICE_ATTR(tlsc_gs_ctl, 0664, show_tlsc_gesture, store_tlsc_gesture);
static DEVICE_ATTR(tlsc_flash_ctl, 0664, show_tlsc_debug_flash,
		   store_tlsc_debug_flash);
static struct attribute *tlsc_attrs[] = { &dev_attr_tlsc_version.attr,
	&dev_attr_tlsc_tp_info.attr, &dev_attr_tlsc_ps_ctl.attr,
	&dev_attr_tlsc_gs_ctl.attr, &dev_attr_tlsc_flash_ctl.attr,
	NULL,			// Can not delete this line!!! The phone will reset.
};

static struct attribute_group tlsc_attr_group = {.attrs = tlsc_attrs, };

#ifdef TLSC_ESD_HELPER_EN

unsigned char g_tlsc6x_esdtar;

static int tlsc6x_power_on(struct regulator *p_reg)
{
	int err = 0;
	int retry = 0;

	TLSC_FUNC_ENTER();
	if (p_reg == NULL) {
		return -EPERM;
	}

	while (retry++ < 30) {
		err = regulator_enable(p_reg);
		if (regulator_is_enabled(p_reg)) {
			break;
		}
	}

	return err;
}

static int tlsc6x_power_off(struct regulator *p_reg)
{
	int err = 0;
	int retry = 0;

	TLSC_FUNC_ENTER();
	if (p_reg == NULL) {
		return -EPERM;
	}

	while (retry++ < 30) {
		err = regulator_disable(p_reg);
		if (regulator_is_enabled(p_reg) == 0) {
			break;
		}
	}

	return err;
}

static int esd_check_work(void)
{
	int ret = -1;
	u8 test_val = 0;

	TLSC_FUNC_ENTER();

	if (g_tp_drvdata->esdHelperFreeze) {
		return 1;
	}

	ret = tlsc6x_read_reg(this_client, 0xa3, &test_val);

	if (ret < 0) {		//maybe confused by some noise,so retry is make sense.
		msleep(10);
		tlsc6x_read_reg(this_client, 0xa3, &test_val);
		ret = tlsc6x_read_reg(this_client, 0xa3, &test_val);
	}

	if ((ret >= 0) && (g_tlsc6x_esdtar != 0)) {
		if (g_tlsc6x_esdtar != test_val) {
			ret = -1;
		}
	}

	if (ret < 0) {
		/* re-power-on */
		tlsc6x_power_off(g_tp_drvdata->reg_vdd);
		msleep(20);
		tlsc6x_power_on(g_tp_drvdata->reg_vdd);

		tlsc6x_tpd_reset();
		if (tlsc6x_read_reg(this_client, 0xa3, &g_tlsc6x_esdtar) < 0) {
			g_tlsc6x_esdtar = 0x00;
		}
	}

	return ret;
}

static int esd_checker_handler(void *unused)
{
	ktime_t ktime;

	if (tlsc6x_read_reg(this_client, 0xa3, &g_tlsc6x_esdtar) < 0) {
		g_tlsc6x_esdtar = 0x00;
	}

	do {
		wait_event_interruptible(tpd_esd_waiter, tpd_esd_flag != 0);
		tpd_esd_flag = 0;

		ktime = ktime_set(4, 0);
		hrtimer_start(&tpd_esd_kthread_timer, ktime, HRTIMER_MODE_REL);

		if (g_tp_drvdata->esdHelperFreeze) {
			continue;
		}
#if (defined TPD_AUTO_UPGRADE_PATH) || (defined TLSC_APK_DEBUG)
		if (auto_upd_busy) {
			continue;
		}
#endif
		esd_check_work();

	} while (!kthread_should_stop());

	return 0;
}

enum hrtimer_restart tpd_esd_kthread_hrtimer_func(struct hrtimer *timer)
{
	tpd_esd_flag = 1;
	wake_up_interruptible(&tpd_esd_waiter);

	return HRTIMER_NORESTART;
}
#endif

#ifdef TLSC_TPD_PROXIMITY

static int tlsc6x_prox_ctrl(int enable)
{
	TLSC_FUNC_ENTER();
	if (enable == 1) {
		tpd_prox_active = 1;
		tlsc6x_write_reg(this_client, 0xb0, 0x01);
	} else if (enable == 0) {
		tpd_prox_active = 0;
		tlsc6x_write_reg(this_client, 0xb0, 0x00);
	}
	tpd_prox_old_state = 0xe0;	/* default is far away */

	return 1;
}

static ssize_t tlsc6x_prox_enable_show(struct device *dev,
				       struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "ps enable %d\n", tpd_prox_active);
}

static ssize_t tlsc6x_prox_enable_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count)
{
	unsigned int enable;
	int ret = 0;

	TLSC_FUNC_ENTER();
	ret = kstrtouint(buf, 0, &enable);
	if (ret)
		return -EINVAL;
	enable = (enable > 0) ? 1 : 0;

	tlsc6x_prox_ctrl(enable);

	return count;

}

static DEVICE_ATTR(proximity, 0664, tlsc6x_prox_enable_show,
		   tlsc6x_prox_enable_store);

/* default cmd interface(refer to sensor HAL):"/sys/class/sprd-tpd/device/proximity" */
static void tlsc6x_prox_cmd_path_init(void)
{
	sprd_tpd_class = class_create(THIS_MODULE, "sprd-tpd");
	if (IS_ERR(sprd_tpd_class)) {
		dev_err(&this_client->dev,
			"tlsc6x error::create sprd-tpd fail\n");
	} else {
		sprd_ps_cmd_dev =
		    device_create(sprd_tpd_class, NULL, 0, NULL, "device");
		if (IS_ERR(sprd_ps_cmd_dev)) {
			dev_err(&this_client->dev,
				"tlsc6x error::create ges&ps cmd-io fail\n");
		} else {
			/* sys/class/sprd-tpd/device/proximity */
			if (device_create_file
			    (sprd_ps_cmd_dev, &dev_attr_proximity) < 0) {
				dev_err(&this_client->dev,
					" tlsc6x error::create device file fail(%s)!\n",
					dev_attr_proximity.attr.name);
			}
		}
	}

}
#endif
static int tlsc6x_request_irq_work(void)
{
	int ret = 0;

	this_client->irq =
	    gpio_to_irq(g_tp_drvdata->platform_data->irq_gpio_number);
	tlsc_info("The irq node num is %d", this_client->irq);

	ret = request_threaded_irq(this_client->irq,
				   NULL, touch_event_thread_handler,
				   IRQF_TRIGGER_FALLING | IRQF_ONESHOT |
				   IRQF_NO_SUSPEND, "tlsc6x_tpd_irq", NULL);
	if (ret < 0) {
		tlsc_err("Request irq thread error!");
		return ret;
	}

	return ret;
}

ssize_t GTprocTpInfoRead(struct file *pFile, char __user *pBuffer,
			 size_t nCount, loff_t *pPos)
{
	u8 nLength;

	nLength = sprintf(pBuffer, "TLSC%s\n", TS_NAME);
	*pPos += nLength;

	return nLength;
}

ssize_t GTprocTpInfoWrite(struct file *pFile, const char __user *pBuffer,
			  size_t nCount, loff_t *pPos)
{
	return nCount;
}

static const struct proc_ops GTProcTpInfo = {.proc_read = GTprocTpInfoRead,
	.proc_write = GTprocTpInfoWrite,
};

static struct proc_dir_entry *GTProcTpInfoEntry;

int i2c_correspond(void)
{
	u8 test_val = 0;
	int ret = tlsc6x_read_reg(this_client, 0xa3, &test_val);

	if (ret < 0)
		return 1;
	else
		return 0;
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 6, 0))
static int tlsc6x_probe(struct i2c_client *client)
#else
static int tlsc6x_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
#endif
{
	int ret = -1;
	int err = 0;
	int reset_count;
	struct input_dev *input_dev;
	struct tlsc6x_platform_data *pdata = NULL;

	TLSC_FUNC_ENTER();

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		err = -ENODEV;
		goto exit_alloc_platform_data_failed;
	}
#ifdef CONFIG_OF		/* NOTE:THIS IS MUST!!! */
	if (client->dev.of_node) {
		pdata = tlsc6x_parse_dt(&client->dev);
		if (pdata) {
			client->dev.platform_data = pdata;
		}
	}
#endif

	if (pdata == NULL) {
		err = -ENOMEM;
		tlsc_err("%s: no platform data!!!\n", __func__);
		goto exit_alloc_platform_data_failed;
	}

	g_tp_drvdata = kzalloc(sizeof(*g_tp_drvdata), GFP_KERNEL);	/* auto clear */
	if (!g_tp_drvdata) {
		err = -ENOMEM;
		goto exit_alloc_data_failed;
	}

	this_client = client;
	g_tp_drvdata->client = client;
	g_tp_drvdata->platform_data = pdata;

	err = tlsc6x_hw_init(g_tp_drvdata);
	if (err < 0) {
		goto exit_gpio_request_failed;
	}

	i2c_set_clientdata(client, g_tp_drvdata);

	reset_count = 0;
	g_is_telink_comp = 0;
	while (++reset_count <= 3) {
		tlsc6x_tpd_reset();
		g_is_telink_comp = tlsc6x_tp_dect(client);
		if (g_is_telink_comp) {
			break;
		}

	}

	g_tp_drvdata->needKeepRamCode = g_needKeepRamCode;

	if (g_is_telink_comp) {
		tlsc6x_tpd_reset();
	} else {
		tlsc_err("tlsc6x:%s, no tlsc6x!\n", __func__);
		err = -ENODEV;
		goto exit_chip_check_failed;
	}

	input_dev = input_allocate_device();
	if (!input_dev) {
		err = -ENOMEM;
		dev_err(&client->dev,
			"tlsc6x error::failed to allocate input device\n");
		goto exit_input_dev_alloc_failed;
	}
	g_tp_drvdata->input_dev = input_dev;

	__set_bit(ABS_MT_TOUCH_MAJOR, input_dev->absbit);
	__set_bit(ABS_MT_POSITION_X, input_dev->absbit);
	__set_bit(ABS_MT_POSITION_Y, input_dev->absbit);
	__set_bit(ABS_MT_WIDTH_MAJOR, input_dev->absbit);
	__set_bit(KEY_MENU, input_dev->keybit);
	__set_bit(KEY_BACK, input_dev->keybit);
	__set_bit(KEY_HOMEPAGE, input_dev->keybit);
	__set_bit(BTN_TOUCH, input_dev->keybit);
	__set_bit(INPUT_PROP_DIRECT, input_dev->propbit);

#if MULTI_PROTOCOL_TYPE_B
	input_mt_init_slots(input_dev, TS_MAX_FINGER, 0);
#else
	input_set_abs_params(input_dev, ABS_MT_TRACKING_ID, 0, 255, 0, 0);
#endif

#ifndef CONFIG_OF
	if ((g_tlsc6x_cfg_ver & 0xffffff) == 0x0e3610) {
		pdata->x_res_max = 480;
		pdata->y_res_max = 854;
	} else if ((g_tlsc6x_cfg_ver & 0xffffff) == 0x0e3614) {
		pdata->x_res_max = 720;
		pdata->y_res_max = 1280;
	} else {
		pdata->x_res_max = 720;
		pdata->y_res_max = 1280;
	}
#endif

	input_set_abs_params(input_dev, ABS_MT_POSITION_X, 0, pdata->x_res_max,
			     0, 0);
	input_set_abs_params(input_dev, ABS_MT_POSITION_Y, 0, pdata->y_res_max,
			     0, 0);
	input_set_abs_params(input_dev, ABS_MT_TOUCH_MAJOR, 0, 15, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_WIDTH_MAJOR, 0, 15, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_PRESSURE, 0, 127, 0, 0);
	input_set_abs_params(input_dev, ABS_DISTANCE, 0, 1, 0, 0);	/* give this capability aways */
	set_bit(EV_ABS, input_dev->evbit);
	set_bit(EV_KEY, input_dev->evbit);

	input_dev->name = "tlsc6x_touch";
	err = input_register_device(input_dev);
	if (err) {
		dev_err(&client->dev,
			"tlsc6x error::failed to register input device: %s\n",
			dev_name(&client->dev));
		goto exit_input_register_device_failed;
	}
#ifdef TLSC_TPD_PROXIMITY
	tlsc6x_prox_cmd_path_init();
	g_tp_drvdata->ps_input_dev = input_allocate_device();
	if (!g_tp_drvdata->ps_input_dev) {
		err = -ENOMEM;
		dev_err(&client->dev,
			"tlsc6x error::failed to allocate ps-input device\n");
		goto exit_input_register_device_failed;
	}
	g_tp_drvdata->ps_input_dev->name = "proximity_tp";
	set_bit(EV_ABS, g_tp_drvdata->ps_input_dev->evbit);
	input_set_capability(g_tp_drvdata->ps_input_dev, EV_ABS, ABS_DISTANCE);
	input_set_abs_params(g_tp_drvdata->ps_input_dev, ABS_DISTANCE, 0, 1, 0,
			     0);
	err = input_register_device(g_tp_drvdata->ps_input_dev);
	if (err) {
		dev_err(&client->dev,
			"tlsc6x error::failed to register ps-input device: %s\n",
			dev_name(&client->dev));
		goto exit_input_register_device_failed;
	}
#endif

#ifdef TOUCH_VIRTUAL_KEYS
	tlsc6x_virtual_keys_init();
#endif

	INIT_WORK(&g_tp_drvdata->resume_work, tlsc6x_resume_work);
	g_tp_drvdata->tp_resume_workqueue =
	    create_singlethread_workqueue("tlsc6x_resume_work");
	if (!g_tp_drvdata->tp_resume_workqueue) {
		err = -ESRCH;
		goto exit_input_register_device_failed;
	}

	//wake_lock_init(&tlsc6x_wakelock, WAKE_LOCK_SUSPEND, "tlsc6x_wakelock");

	err = tlsc6x_request_irq_work();
	if (err < 0) {
		dev_err(&client->dev, "tlsc6x error::request irq failed %d\n",
			err);
		goto exit_irq_request_failed;
	}
#if defined(CONFIG_ADF)
	g_tp_drvdata->fb_notif.notifier_call = ts_adf_event_handler;
	ret = fb_register_client(&g_tp_drvdata->fb_notif);
	if (ret) {
		dev_err(&client->dev,
			"tlsc6x error::unable to register fb_notifier: %d",
			ret);
	}
#elif defined(CONFIG_HAS_EARLYSUSPEND)
	g_tp_drvdata->early_suspend.level =
	    EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
	g_tp_drvdata->early_suspend.suspend = tlsc6x_ts_suspend;
	g_tp_drvdata->early_suspend.resume = tlsc6x_ts_resume;
	register_early_suspend(&g_tp_drvdata->early_suspend);
#endif

#ifdef TLSC_APK_DEBUG
	tlsc6x_create_apk_debug_channel(client);
#endif

	err = sysfs_create_group(&client->dev.kobj, &tlsc_attr_group);
	if (err < 0) {
		tlsc_err("Can not create sysfs group!");
	}
	//tlsc6x_tptest_init(client);

#ifdef TLSC_ESD_HELPER_EN
	{			/* esd issue: i2c monitor thread */
		ktime_t ktime = ktime_set(30, 0);

		hrtimer_init(&tpd_esd_kthread_timer, CLOCK_MONOTONIC,
			     HRTIMER_MODE_REL);
		tpd_esd_kthread_timer.function = tpd_esd_kthread_hrtimer_func;
		hrtimer_start(&tpd_esd_kthread_timer, ktime, HRTIMER_MODE_REL);
		kthread_run(esd_checker_handler, 0, "tlsc6x_esd_helper");
	}
#endif
	GTProcTpInfoEntry = proc_create("tp_info", 0444, NULL, &GTProcTpInfo);
#if 0
	if (tp_status_register(i2c_correspond)) {
		printk("tp_status_register failed\n");
	} else
		printk("tp_status_register success\n");
#endif

	return 0;

exit_irq_request_failed:input_unregister_device(input_dev);
exit_input_register_device_failed:input_free_device(input_dev);
exit_input_dev_alloc_failed: exit_chip_check_failed:gpio_free
	    (pdata->
	     irq_gpio_number);
	gpio_free(pdata->reset_gpio_number);
exit_gpio_request_failed:kfree(g_tp_drvdata);
exit_alloc_data_failed:if (pdata != NULL) {
		kfree(pdata);
	}
	g_tp_drvdata = NULL;
	i2c_set_clientdata(client, g_tp_drvdata);
exit_alloc_platform_data_failed:return err;
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0))
static void tlsc6x_remove(struct i2c_client *client)
#else
static int tlsc6x_remove(struct i2c_client *client)
#endif
{
	struct tlsc6x_data *drvdata = i2c_get_clientdata(client);

	TLSC_FUNC_ENTER();

#ifdef TLSC_APK_DEBUG
	tlsc6x_release_apk_debug_channel();
#endif

#ifdef TLSC_ESD_HELPER_EN
	hrtimer_cancel(&tpd_esd_kthread_timer);
#endif

	if (drvdata == NULL) {
		goto ret;
	}
#if defined(CONFIG_ADF)
	fb_unregister_client(&g_tp_drvdata->fb_notif);
#elif defined(CONFIG_HAS_EARLYSUSPEND)
	unregister_early_suspend(&drvdata->early_suspend);
#endif

	free_irq(client->irq, drvdata);
	input_unregister_device(drvdata->input_dev);
	input_free_device(drvdata->input_dev);

#ifdef CONFIG_HAS_EARLYSUSPEND
	cancel_work_sync(&drvdata->resume_work);
	destroy_workqueue(drvdata->tp_resume_workqueue);
#endif
	kfree(drvdata);
	drvdata = NULL;
	i2c_set_clientdata(client, drvdata);

ret:
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0))
	return;
#else
	return 0;
#endif
}

static const struct i2c_device_id tlsc6x_id[] = { {TS_NAME, 0}, {} };

MODULE_DEVICE_TABLE(i2c, tlsc6x_id);

static const struct of_device_id tlsc6x_of_match[] = { {.compatible =
							"tlsc6x,tlsc6x_ts",}, {}
};

MODULE_DEVICE_TABLE(of, tlsc6x_of_match);
static struct i2c_driver tlsc6x_driver = {
	.probe = tlsc6x_probe,
	.remove = tlsc6x_remove,
	.id_table = tlsc6x_id,
	.driver = {
		.name = TS_NAME,
		.owner = THIS_MODULE,
		.of_match_table = tlsc6x_of_match,
	},
};

static int __init tlsc6x_init(void)
{
	tlsc_info("%s: ++\n", __func__);
	return i2c_add_driver(&tlsc6x_driver);
}

static void __exit tlsc6x_exit(void)
{
	i2c_del_driver(&tlsc6x_driver);
}

module_init(tlsc6x_init);
module_exit(tlsc6x_exit);

MODULE_DESCRIPTION("Chipsemi touchscreen driver");
MODULE_LICENSE("GPL");
