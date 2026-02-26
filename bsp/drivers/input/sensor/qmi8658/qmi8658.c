#include <linux/module.h>
#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/mutex.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/hwmon-sysfs.h>

#include <linux/err.h>
#include <linux/hwmon.h>
#include <linux/fs.h>
#include <linux/input.h>
//#include <linux/input-polldev.h>
#include <linux/device.h>
#include "../../init-input.h"
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/device.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/ioctl.h>
#include <linux/platform_device.h>
#include <linux/regulator/consumer.h>
#include <linux/version.h>

#include <linux/hrtimer.h>
#include <linux/ktime.h>
//#include <mach/system.h>
//#include <mach/hardware.h>
//#undef CONFIG_PM

/***********************************************
 *** REGISTER MAP
 ***********************************************/

#define SENSOR_CHIP_ID_QMI8658      0x05

#define QMI8658_DEV_NAME				"qmi8658"
#define QMI8658_DEV_VERSION				"1.0.1"
#define QMI8658_ACC_INPUT_NAME			"qmi8658_acc"
#define QMI8658_GYRO_INPUT_NAME			"qmi8658_gyr"
#define QMI8658_ACC_8G_MIN				(-8 * 1024)
#define QMI8658_ACC_8G_MAX				(8 * 1024)
#define QMI8658_GYRO_DPS_MIN			(-1024)
#define QMI8658_GYRO_DPS_MAX			(1024)

#define QMI8658_MAX_DELAY				2000
#define POLL_INTERVAL_MAX				1000
#define POLL_INTERVAL					50

/* QMI8658 registers */
#define QMI8658_REG_WHO_AM_I        0x00
#define QMI8658_REG_VERSION         0x01
#define QMI8658_REG_CTRL1           0x02
#define QMI8658_REG_CTRL2           0x03
#define QMI8658_REG_CTRL3           0x04
#define QMI8658_REG_CTRL4           0x05
#define QMI8658_REG_CTRL5           0x06
#define QMI8658_REG_CTRL6           0x07
#define QMI8658_REG_CTRL7           0x08
#define QMI8658_REG_CTRL8           0x09
#define QMI8658_REG_CTRL9           0x0a
#define QMI8658_REG_CAL1_L          0x0b
#define QMI8658_REG_CAL1_H          0x0c
#define QMI8658_REG_CAL2_L          0x0d
#define QMI8658_REG_CAL2_H          0x0e
#define QMI8658_REG_CAL3_L          0x0f
#define QMI8658_REG_CAL3_H          0x10
#define QMI8658_REG_CAL4_L          0x11
#define QMI8658_REG_CAL4_H          0x12
#define QMI8658_REG_FIFO_WM         0x13
#define QMI8658_REG_FIFO_CTRL       0x14
#define QMI8658_REG_FIFO_COUNT      0x15
#define QMI8658_REG_FIFO_STATUS     0x16
#define QMI8658_REG_FIFO_DATA       0x17
#define QMI8658_REG_STATUSINT       0x2d
#define QMI8658_REG_STATUS0         0x2e
#define QMI8658_REG_STATUS1         0x2f
#define QMI8658_REG_RESET           0x60
#define QMI8658_REG_ACC_XOUT_LSB    0x35
#define QMI8658_REG_GYR_XOUT_LSB    0x3B

/* QMI8658 Ctrl9 command*/
#define QMI8658_CRTL9_NOP           0x00
#define QMI8658_CRTL9_RST_FIFO      0x04
#define QMI8658_CRTL9_REQ_FIFO      0x05
#define QMI8658_CRTL9_REQ_MOTION    0x0e
#define QMI8658_CRTL9_CLC_SEN       0xa2

/*QMI8658 acc range*/
#define QMI8658_ACC_RANGE_2G        (0x00<<4) /*!< \brief +/- 2g range */
#define QMI8658_ACC_RANGE_4G        (0x01<<4) /*!< \brief +/- 4g range */
#define QMI8658_ACC_RANGE_8G        (0x02<<4) /*!< \brief +/- 8g range */
#define QMI8658_ACC_RANGE_16G       (0x03<<4)/*!< \brief +/- 16g range */
/*QMI8658 gyr range*/
#define QMI8658_GYR_RANGE_16DPS     (0x00<<4) /*!< \brief +-16 degrees per second. */
#define QMI8658_GYR_RANGE_32DPS     (0x01<<4) /*!< \brief +-32 degrees per second. */
#define QMI8658_GYR_RANGE_64DPS     (0x02<<4) /*!< \brief +-64 degrees per second. */
#define QMI8658_GYR_RANGE_128DPS    (0x03<<4) /*!< \brief +-128 degrees per second. */
#define QMI8658_GYR_RANGE_256DPS    (0x04<<4) /*!< \brief +-256 degrees per second. */
#define QMI8658_GYR_RANGE_512DPS    (0x05<<4) /*!< \brief +-512 degrees per second. */
#define QMI8658_GYR_RANGE_1024DPS   (0x06<<4) /*!< \brief +-1024 degrees per second. */
#define QMI8658_GYR_RANGE_2048DPS   (0x07<<4) /*!< \brief +-2048 degrees per second.*/
/*QMI8658 a,g odr*/
#define QMI8658_ODR_30HZ_REG_VALUE      0x08  /*!< 1.5625 Hz (640 ms)*/
#define QMI8658_ODR_59HZ_REG_VALUE      0x07  /*!< 3.125 Hz (320 ms)*/
#define QMI8658_ODR_118HZ_REG_VALUE     0x06  /*!< 6.25 Hz (160 ms)*/
#define QMI8658_ODR_235HZ_REG_VALUE     0x05  /*!< 12.5 Hz (80 ms)*/
#define QMI8658_ODR_470HZ_REG_VALUE     0x04  /*!< 25 Hz (40 ms)*/

/*function cfg default*/
#define QMI8658_ADDR_AI_BE          (0x7C)//0x60
#define QMI8658_SW_RESET            (0xB0)
#define QMI8658_MOTION_HS           (0x80)
#define QMI8658_ACC_RANGE_DEF       (0x20)  //±8g
#define QMI8658_GYRO_RANGE_DEF      (0x70)  //±2048dps
#define QMI8658_CTRL7_ACC_ENABLE    (0x01) //0x21
#define QMI8658_CTRL7_GYRO_ENABLE   (0x02) //0x22

#define QMI8658_STATUS1_CMD_DONE    (0x01)


#define QMI8658_DEBUG
#if defined(QMI8658_DEBUG)
#define QMI8658_TAG				       "qst-"
#define QMI8658_FUN(f)               	printk(QMI8658_TAG"%s\n", __FUNCTION__)
#define QMI8658_ERR(fmt, args...)    	printk(QMI8658_TAG"%s(line:%d)  " fmt, __FUNCTION__, __LINE__, ##args)
#define QMI8658_LOG(fmt, args...)    	printk(QMI8658_TAG"%s(line:%d)  " fmt, __FUNCTION__, __LINE__, ##args)
#else
#define QMI8658_FUN()
#define QMI8658_LOG(fmt, args...)
#define QMI8658_ERR(fmt, args...)
#endif

enum qmi8658_type {
	QMI8658_TYPE_NONE = 0x00,
	QMI8658_TYPE_ACC  = 0x01,
	QMI8658_TYPE_GYRO = 0x02,

	QMI8658_TYPE_MAX = 0xff
};

typedef struct {
	int x;
	int y;
	int z;
} qmi8658_axis;

struct qmi8658_data {
	struct i2c_client		*client;

	struct input_dev 			*acc_input;
	struct input_dev 			*gyro_input;

	struct workqueue_struct 	*qmi_wq;

	struct hrtimer				acc_hr_timer;
	struct work_struct			acc_work;
	ktime_t						acc_ktime;

	struct hrtimer				gyro_hr_timer;
	struct work_struct			gyro_work;
	ktime_t						gyro_ktime;

	struct mutex 				op_mutex;

	atomic_t 					acc_enable;
	atomic_t 					acc_delay;
	atomic_t 					gyro_enable;
	atomic_t 					gyro_delay;

	atomic_t 					need_init;

	qmi8658_axis				acc_out;
	qmi8658_axis				gyro_out;

#if defined(CONFIG_PM)
	volatile int suspend_indator;
#endif
	struct hrtimer hr_timer;
	struct work_struct wq_hrtimer;
	ktime_t ktime;

};

/* Addresses to scan */
static const unsigned short normal_i2c[] = {0x6a, 0x6b, I2C_CLIENT_END};
static __u32 twi_id;


static struct sensor_config_info gsensor_info = {
	.input_type = GSENSOR_TYPE,
	.np_name = QMI8658_DEV_NAME,
};

static struct qmi8658_data *qmi8658;

static int qmi8658_reg_init(struct i2c_client *client);

static int qmi8658_write_reg(unsigned char reg, unsigned char value)
{
	int ret = 0;

	mutex_lock(&qmi8658->op_mutex);
	ret = i2c_smbus_write_byte_data(qmi8658->client, reg, value);
	mutex_unlock(&qmi8658->op_mutex);
	if (ret < 0)
		return ret;
	else
		return 0;
}

static int qmi8658_read_reg(unsigned char reg, unsigned char *buf, unsigned char len)
{
	int ret = 0;

	mutex_lock(&qmi8658->op_mutex);
	if (len == 1)
		*buf = i2c_smbus_read_byte_data(qmi8658->client, reg);
	else
		ret = i2c_smbus_read_i2c_block_data(qmi8658->client, reg, len, buf);

	mutex_unlock(&qmi8658->op_mutex);
	if (ret < 0)
		return ret;
	else
		return 0;
}
/*
static int i2c_write_bytes(struct i2c_client *client, uint8_t *data, uint16_t len)
{
	struct i2c_msg msg;
	int ret=-1;

	msg.flags = !I2C_M_RD;
	msg.addr = client->addr;
	msg.len = len;
	msg.buf = data;

	ret = i2c_transfer(client->adapter, &msg,1);
	return ret;
}

static bool gsensor_i2c_test(struct i2c_client * client)
{
	int ret, retry;
	uint8_t test_data[1] = { 0 };   //only write a data address.

	for (retry = 0; retry < 2; retry++) {
	ret = i2c_write_bytes(client, test_data, 1); //Test i2c.
	if (ret == 1)
		break;
		msleep(5);
	}

	return ret == 1 ? true : false;
}
*/
static int qmi8658_gsensor_detect(struct i2c_client *client, struct i2c_board_info *info)
{
	struct i2c_adapter *adapter = client->adapter;
	//__s32 ret = 0;

	if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE_DATA))
		return -ENODEV;

	if (twi_id == adapter->nr) {
		printk("%s: addr= %x\n", __func__, client->addr);
/*
		ret = gsensor_i2c_test(client);
		if (!ret) {
			pr_info("%s:I2C connection might be something wrong or maybe the other gsensor equipment! \n",__func__);
			return -ENODEV;
		} else
*/
		{
			printk("%s: I2C connection sucess!\n", __func__);
			strlcpy(info->type, QMI8658_DEV_NAME, I2C_NAME_SIZE);
			return 0;
		}
	} else {
		printk("%s: I2C connection error!\n",  __func__);
		return -ENODEV;
	}
}

void qmi8658_send_ctrl9cmd(unsigned char reg_val)
{
	unsigned char status1 = 0, count = 0;

	qmi8658_write_reg(QMI8658_REG_CTRL9, reg_val);
	while (((status1 & QMI8658_STATUS1_CMD_DONE) == 0) && (count++ < 200)) {
		qmi8658_read_reg(QMI8658_REG_STATUS1, &status1, 1);
		mdelay(2);
	}
}

void qmi8658_enable_sensors(enum qmi8658_type type, u8 enableFlags)
{
	unsigned char ctrl7_reg = 0;

	QMI8658_ERR("type=%d enableFlags*******=%d\n", type, enableFlags);

	qmi8658_read_reg(QMI8658_REG_CTRL7, &ctrl7_reg, 1);
	if (type & QMI8658_TYPE_ACC) {
		if (enableFlags) {
			qmi8658_write_reg(QMI8658_REG_CAL1_L, 0x01);
			qmi8658_send_ctrl9cmd(0x12);
			ctrl7_reg = (ctrl7_reg & 0xFE) | 0x01; //accel enable;
		} else {
			qmi8658_write_reg(QMI8658_REG_CAL1_L, 0x00);
			qmi8658_send_ctrl9cmd(0x12);
			ctrl7_reg = (ctrl7_reg & 0xFE) | 0x00; //accel disable;
		}
	}
	if (type & QMI8658_TYPE_GYRO) {
		if (enableFlags) {
			qmi8658_write_reg(QMI8658_REG_CAL1_L, 0x01);
			qmi8658_send_ctrl9cmd(0x12);
			ctrl7_reg = (ctrl7_reg & 0xFD) | 0x02; //gyro enable;
		} else {
			qmi8658_write_reg(QMI8658_REG_CAL1_L, 0x00);
			qmi8658_send_ctrl9cmd(0x12);
		    ctrl7_reg = (ctrl7_reg & 0xFD) | 0x00; //gyro disable;
		}
	}
	qmi8658_write_reg(QMI8658_REG_CTRL7, ctrl7_reg);
	if (enableFlags) {
		mdelay(100);
	}
}

static void qmi8658_set_delay(enum qmi8658_type type, int delay)
{
	QMI8658_FUN();
	if (type == QMI8658_TYPE_ACC) {
		atomic_set(&qmi8658->acc_delay, delay);
		qmi8658->acc_ktime = ktime_set(0, (delay - 1) * NSEC_PER_MSEC);
	} else if (type == QMI8658_TYPE_GYRO) {
		atomic_set(&qmi8658->gyro_delay, delay);
		qmi8658->gyro_ktime = ktime_set(0, (delay - 1) * NSEC_PER_MSEC);
	}
}

static int qmi8658_acc_read_raw(short raw_xyz[3])
{
	unsigned char buf_reg[6];
	int ret = 0;

	ret = qmi8658_read_reg(QMI8658_REG_ACC_XOUT_LSB, buf_reg, 6);

	raw_xyz[0] = (short)((buf_reg[1]<<8) | buf_reg[0]);
	raw_xyz[1] = (short)((buf_reg[3]<<8) | buf_reg[2]);
	raw_xyz[2] = (short)((buf_reg[5]<<8) | buf_reg[4]);
	//QMI8658_LOG("acc_raw [%d %d	%d]\n", raw_xyz[0],raw_xyz[1],raw_xyz[2]);

	return ret;
}


static int qmi8658_read_accel_xyz(qmi8658_axis *acc)
{
	int res = 0;

	short raw_xyz[3];

	res = qmi8658_acc_read_raw(raw_xyz);
	acc->x = raw_xyz[0];
	acc->y = raw_xyz[1];
	acc->z = raw_xyz[2];

	return res;
}


static int qmi8658_gyro_read_raw(short raw_xyz[3])
{
	unsigned char buf_reg[6];
	int ret = 0;

	ret = qmi8658_read_reg(QMI8658_REG_GYR_XOUT_LSB, buf_reg, 6);

	raw_xyz[0] = (short)((buf_reg[1]<<8) | buf_reg[0]);
	raw_xyz[1] = (short)((buf_reg[3]<<8) | buf_reg[2]);
	raw_xyz[2] = (short)((buf_reg[5]<<8) | buf_reg[4]);
	//QMI8658_LOG("gyro_raw [%d %d %d]\n", raw_xyz[0],raw_xyz[1],raw_xyz[2]);
    if (raw_xyz[0] == -32768)
		return -1;

	return ret;
}

static int qmi8658_read_gyro_xyz(qmi8658_axis *gyro)
{
	int res = 0;
	short raw_xyz[3];

	res = qmi8658_gyro_read_raw(raw_xyz);
	gyro->x = raw_xyz[0];
	gyro->y = raw_xyz[1];
	gyro->z = raw_xyz[2];

	return res;
}

static enum hrtimer_restart acc_hrtimer_callback(struct hrtimer *timer)
{
	queue_work(qmi8658->qmi_wq, &qmi8658->acc_work);
	hrtimer_forward_now(&qmi8658->acc_hr_timer, qmi8658->acc_ktime);
	return HRTIMER_RESTART;
}

static void acc_work_func(struct work_struct *work)
{
	qmi8658_axis acc = { 0 };
	int is_needinit = 0;
	unsigned char status0 = 0, count = 0;

	is_needinit = atomic_read(&qmi8658->need_init);
	if (is_needinit) {
		atomic_set(&qmi8658->need_init, 0);
		goto reg_init;
	}

	while (!(status0 & 0x01) && (count++ < 20)) {
		qmi8658_read_reg(QMI8658_REG_STATUSINT, &status0, 1);
	}

	qmi8658_read_accel_xyz(&acc);

	qmi8658->acc_out.x = acc.x;
	qmi8658->acc_out.y = acc.y;
	qmi8658->acc_out.z = acc.z;

	input_report_abs(qmi8658->acc_input, ABS_X, acc.x);
	input_report_abs(qmi8658->acc_input, ABS_Y, acc.y);
	input_report_abs(qmi8658->acc_input, ABS_Z, acc.z);
	input_sync(qmi8658->acc_input);
	//QMI8658_LOG(":[%d %d %d ]\n",acc.x,acc.y,acc.z);
	return;

reg_init:
	qmi8658_reg_init(qmi8658->client);
}


static enum hrtimer_restart gyro_hrtimer_callback(struct hrtimer *timer)
{
	queue_work(qmi8658->qmi_wq, &qmi8658->gyro_work);
	hrtimer_forward_now(&qmi8658->gyro_hr_timer, qmi8658->gyro_ktime);
	return HRTIMER_RESTART;
}

static void gyro_work_func(struct work_struct *work)
{
	int ret = 0;
	int is_needinit = 0;
	qmi8658_axis gyro = { 0 };
	unsigned char status0 = 0, count = 0;

	is_needinit = atomic_read(&qmi8658->need_init);
	if (is_needinit) {
		atomic_set(&qmi8658->need_init, 0);
		goto reg_init;
	}

	while ((!(status0 & 0x01) && (count++ < 20))) {
		qmi8658_read_reg(QMI8658_REG_STATUSINT, &status0, 1);
	}

	ret = qmi8658_read_gyro_xyz(&gyro);
	if (ret < 0) {
		QMI8658_LOG("read gyro data error, reinit");
		goto reg_init;
	}

	qmi8658->gyro_out.x = gyro.x;
	qmi8658->gyro_out.y = gyro.y;
	qmi8658->gyro_out.z = gyro.z;

	input_report_abs(qmi8658->gyro_input, ABS_X, gyro.x);
	input_report_abs(qmi8658->gyro_input, ABS_Y, gyro.y);
	input_report_abs(qmi8658->gyro_input, ABS_Z, gyro.z);
	input_sync(qmi8658->gyro_input);
	//QMI8658_LOG(":[%d %d %d ]\n",gyro.x,gyro.y,gyro.z);
	return;

reg_init:
	qmi8658_reg_init(qmi8658->client);
}

static int qmi8658_input_init(struct qmi8658_data *qmi8658)
{
	struct input_dev *acc_dev, *gyro_dev;
	int err = 0;

	QMI8658_LOG("-----acc input init\n");
	acc_dev = input_allocate_device();
	if (!acc_dev) {
		QMI8658_ERR("acc input can't allocate device!\n");
		return -ENOMEM;
	}

	acc_dev->name = QMI8658_ACC_INPUT_NAME;
	acc_dev->id.bustype = BUS_I2C;
	input_set_capability(acc_dev, EV_ABS, ABS_MISC);
	input_set_abs_params(acc_dev, ABS_X, QMI8658_ACC_8G_MIN, QMI8658_ACC_8G_MAX, 0, 0);
	input_set_abs_params(acc_dev, ABS_Y, QMI8658_ACC_8G_MIN, QMI8658_ACC_8G_MAX, 0, 0);
	input_set_abs_params(acc_dev, ABS_Z, QMI8658_ACC_8G_MIN, QMI8658_ACC_8G_MAX, 0, 0);
	input_set_drvdata(acc_dev, qmi8658);

	err = input_register_device(acc_dev);
	if (err < 0) {
		QMI8658_ERR("acc input can't register device!\n");
		input_free_device(acc_dev);
		return err;
	}
	qmi8658->acc_input = acc_dev;

	QMI8658_LOG("-------gyro input init\n");
	gyro_dev = input_allocate_device();
	if (!gyro_dev) {
		input_unregister_device(qmi8658->acc_input);
		QMI8658_ERR("gyro input can't allocate device!\n");
		return -ENOMEM;
	}
	gyro_dev->name = QMI8658_GYRO_INPUT_NAME;
	gyro_dev->id.bustype = BUS_I2C;
	input_set_capability(gyro_dev, EV_ABS, ABS_MISC);
	input_set_abs_params(gyro_dev, ABS_X, QMI8658_GYRO_DPS_MIN, QMI8658_GYRO_DPS_MAX, 0, 0);
	input_set_abs_params(gyro_dev, ABS_Y, QMI8658_GYRO_DPS_MIN, QMI8658_GYRO_DPS_MAX, 0, 0);
	input_set_abs_params(gyro_dev, ABS_Z, QMI8658_GYRO_DPS_MIN, QMI8658_GYRO_DPS_MAX, 0, 0);
	input_set_drvdata(gyro_dev, (void *)qmi8658);

	err = input_register_device(gyro_dev);
	if (err < 0) {
		QMI8658_ERR("gyro input can't register device!\n");
		input_unregister_device(qmi8658->acc_input);
		input_free_device(gyro_dev);
		return err;
	}
	qmi8658->gyro_input = gyro_dev;

	return 0;
}

static void qmi8658_input_deinit(struct qmi8658_data *qmi8658)
{
	QMI8658_LOG("called\n");
	if (qmi8658->acc_input) {
		input_unregister_device(qmi8658->acc_input);
		qmi8658->acc_input = NULL;
	}

	if (qmi8658->gyro_input) {
		input_unregister_device(qmi8658->gyro_input);
		qmi8658->gyro_input = NULL;
	}
}

static int qmi8658_set_enable(enum qmi8658_type type, int enable)
{
	QMI8658_LOG("type:%d--enable :%d\n", type, enable);

	if (type == QMI8658_TYPE_ACC) {
		atomic_set(&qmi8658->acc_enable, enable);
		if (enable) {
			qmi8658->acc_ktime = ktime_set(0, (atomic_read(&qmi8658->acc_delay) - 1) * NSEC_PER_MSEC);
			hrtimer_start(&qmi8658->acc_hr_timer, qmi8658->acc_ktime, HRTIMER_MODE_REL);
		} else {
			hrtimer_cancel(&qmi8658->acc_hr_timer);
		}
	} else if (type == QMI8658_TYPE_GYRO) {
		atomic_set(&qmi8658->gyro_enable, enable);
		if (enable) {
			qmi8658->gyro_ktime = ktime_set(0, (atomic_read(&qmi8658->gyro_delay) - 1) * NSEC_PER_MSEC);
			hrtimer_start(&qmi8658->gyro_hr_timer, qmi8658->gyro_ktime, HRTIMER_MODE_REL);
		} else {
			hrtimer_cancel(&qmi8658->gyro_hr_timer);
		}
	}

	return 0;
}

static ssize_t show_sensordata_value(struct device *dev, struct device_attribute *attr, char *buf)
{
	qmi8658_read_accel_xyz(&qmi8658->acc_out);
	qmi8658_read_gyro_xyz(&qmi8658->gyro_out);
	return sprintf(buf, "%d %d %d	%d %d %d\n", qmi8658->acc_out.x, qmi8658->acc_out.y, qmi8658->acc_out.z,
					qmi8658->gyro_out.x, qmi8658->gyro_out.y, qmi8658->gyro_out.z);
}

static ssize_t show_dumpallreg_value(struct device *dev, struct device_attribute *attr, char *buf)
{
	int res;
	int i = 0;
	char strbuf[600];
	char tempstrbuf[24];
	unsigned char databuf[2] = {0};
	int length = 0;

	QMI8658_FUN();
	/* Check status register for data availability */
	for (i = 0; i <= 50; i++) {
		databuf[0] = i;
		res = qmi8658_read_reg(databuf[0], &databuf[1], 1);
		if (res < 0)
			QMI8658_LOG("qma8658 dump registers 0x%02x failed !\n", i);

		length = scnprintf(tempstrbuf, sizeof(tempstrbuf), "0x%2x=0x%2x\n", databuf[0], databuf[1]);
		snprintf(strbuf + length * i, sizeof(strbuf) - (length * i), "%s", tempstrbuf);
	}

	return scnprintf(buf, sizeof(strbuf), "%s\n", strbuf);
}

static ssize_t qmi8658_enable_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	QMI8658_FUN();

	return sprintf(buf, "acc:%d gyro:%d\n", atomic_read(&qmi8658->acc_enable), atomic_read(&qmi8658->gyro_enable));
}

static ssize_t qmi8658_enable_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	int type, enable = 0;

	QMI8658_FUN();

	if (2 == sscanf(buf, "%d %d", &type, &enable)) {
		if ((type == QMI8658_TYPE_ACC) || (type == QMI8658_TYPE_GYRO)) {
			if (enable) {
				qmi8658_set_enable(type, 1);
			} else {
				qmi8658_set_enable(type, 0);
			}
		}
	} else {
		QMI8658_ERR("invalid format = '%s'\n", buf);
	}

	return count;
}

static ssize_t qmi8658_acc_enable_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	int enable = 0;

	QMI8658_FUN();

	if (1 == sscanf(buf, "%d", &enable)) {
		if (enable) {
			qmi8658_set_enable(QMI8658_TYPE_ACC, 1);
		} else {
			qmi8658_set_enable(QMI8658_TYPE_ACC, 0);
		}
	} else {
		QMI8658_ERR("invalid format = '%s'\n", buf);
	}

	return count;
}

static ssize_t qmi8658_gyro_enable_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	int enable = 0;

	QMI8658_FUN();

	if (1 == sscanf(buf, "%d", &enable)) {
		if (enable) {
			qmi8658_set_enable(QMI8658_TYPE_GYRO, 1);
		} else {
			qmi8658_set_enable(QMI8658_TYPE_GYRO, 0);
		}
	} else {
		QMI8658_ERR("invalid format = '%s'\n", buf);
	}

	return count;
}

/*----------------------------------------------------------------------------*/
static ssize_t qmi8658_delay_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	QMI8658_FUN();
	return sprintf(buf, "acc:%d gyro:%d\n", atomic_read(&qmi8658->acc_delay), atomic_read(&qmi8658->gyro_delay));
}

static ssize_t qmi8658_delay_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	int type, delay;

	QMI8658_FUN();
	if (2 == sscanf(buf, "%d %d", &type, &delay)) {
		if (delay > QMI8658_MAX_DELAY) {
			delay = QMI8658_MAX_DELAY;
		} else if (delay <= 1) {
			delay = 1;
		}

		if ((type == QMI8658_TYPE_ACC) || (type == QMI8658_TYPE_GYRO)) {
			qmi8658_set_delay(type, delay);
		}
	} else {
		QMI8658_ERR("invalid format = '%s'\n", buf);
	}

	return count;
}

static ssize_t qmi8658_acc_delay_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	int delay;

	QMI8658_FUN();
	if (1 == sscanf(buf, "%d", &delay)) {
		if (delay > QMI8658_MAX_DELAY) {
			delay = QMI8658_MAX_DELAY;
		} else if (delay <= 1) {
			delay = 1;
		}

		qmi8658_set_delay(QMI8658_TYPE_ACC, delay);
	} else {
		QMI8658_ERR("invalid format = '%s'\n", buf);
	}

	return count;
}

static ssize_t qmi8658_gyro_delay_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	int delay;

	QMI8658_FUN();
	if (1 == sscanf(buf, "%d", &delay)) {
		if (delay > QMI8658_MAX_DELAY) {
			delay = QMI8658_MAX_DELAY;
		} else if (delay <= 1) {
			delay = 1;
		}

		qmi8658_set_delay(QMI8658_TYPE_GYRO, delay);
	} else {
		QMI8658_ERR("invalid format = '%s'\n", buf);
	}

	return count;
}

static DEVICE_ATTR(sensordata,	0644, show_sensordata_value, NULL);
static DEVICE_ATTR(dumpallreg,	0644, show_dumpallreg_value, NULL);
static DEVICE_ATTR(enable,		0644, qmi8658_enable_show, qmi8658_enable_store);
static DEVICE_ATTR(acc_enable,	0644, NULL, qmi8658_acc_enable_store);
static DEVICE_ATTR(gyro_enable,	0644, NULL, qmi8658_gyro_enable_store);
static DEVICE_ATTR(delay,		0644, qmi8658_delay_show, qmi8658_delay_store);
static DEVICE_ATTR(acc_delay,	0644, NULL, qmi8658_acc_delay_store);
static DEVICE_ATTR(gyro_delay,	0644, NULL, qmi8658_gyro_delay_store);

static struct attribute *qmi8658_attributes[] = {
	&dev_attr_sensordata.attr,
	&dev_attr_dumpallreg.attr,
	&dev_attr_enable.attr,
	&dev_attr_acc_enable.attr,
	&dev_attr_gyro_enable.attr,
	&dev_attr_delay.attr,
	&dev_attr_acc_delay.attr,
	&dev_attr_gyro_delay.attr,
	NULL
};

static struct attribute_group qmi8658_attribute_group = {
	.attrs = qmi8658_attributes
};

static int qmi8658_reg_init(struct i2c_client *client)
{
	int res = 0;
	unsigned char reg_value = 0 ;

    reg_value = i2c_smbus_read_byte_data(client, QMI8658_REG_WHO_AM_I);
	if (reg_value != SENSOR_CHIP_ID_QMI8658) {
		printk("%s:check id err,reg_value:%d\n", __func__, reg_value);
		return -1;
	} else {
		printk("%s:check id success -->qmi8658\n", __func__);
	}
    //qmi8658 reset
	res = i2c_smbus_write_byte_data(client, QMI8658_REG_RESET, QMI8658_SW_RESET);
	msleep(100);
	res += i2c_smbus_write_byte_data(client, QMI8658_REG_CTRL1, QMI8658_ADDR_AI_BE);
	msleep(100);
	//config acc range = ±8g,odr=479hz
	res += i2c_smbus_write_byte_data(client, QMI8658_REG_CTRL2, (QMI8658_ACC_RANGE_DEF | QMI8658_ODR_470HZ_REG_VALUE));
	//config gyro scale = 1024dps,odr=479hz
	res += i2c_smbus_write_byte_data(client, QMI8658_REG_CTRL3, (QMI8658_GYR_RANGE_1024DPS | QMI8658_ODR_470HZ_REG_VALUE));

	qmi8658_enable_sensors(QMI8658_TYPE_ACC, 1);
	qmi8658_enable_sensors(QMI8658_TYPE_GYRO, 1);

	return res;
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 6, 0))
static int qmi8658_i2c_probe(struct i2c_client *client)
#else
static int qmi8658_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
#endif
{
	int err = 0;
	struct qmi8658_data *data;

	QMI8658_LOG("start\n");

	if (gsensor_info.dev == NULL)
		gsensor_info.dev = &client->dev;

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C|I2C_FUNC_SMBUS_BYTE | I2C_FUNC_SMBUS_BYTE_DATA)) {
		QMI8658_ERR("%s: check_functionality failed.", __func__);
		err = -ENODEV;
		goto exit;
	}

	data = kzalloc(sizeof(struct qmi8658_data), GFP_KERNEL);
	if (!data) {
		QMI8658_ERR("%s: can't allocate memory for Qmi8658_Data!\n", __func__);
		err = -ENOMEM;
		goto exit;
	}
	qmi8658 = data;
	data->client = client;
	mutex_init(&data->op_mutex);
	i2c_set_clientdata(client, qmi8658);

	err = qmi8658_reg_init(client);
	if (err < 0) {
		QMI8658_ERR("%s: g_qmi8658 read id fail!\n", __func__);
		goto exit1;
	}

	atomic_set(&qmi8658->acc_delay, (unsigned int) POLL_INTERVAL_MAX);
	atomic_set(&qmi8658->gyro_delay, (unsigned int) POLL_INTERVAL_MAX);

	qmi8658->qmi_wq = create_singlethread_workqueue("qmi8658_wq");
	if (!qmi8658->qmi_wq) {
		QMI8658_ERR("%s: g_qmi8658 create work queue fail!\n", __func__);
		goto exit1;
	}
	flush_workqueue(qmi8658->qmi_wq);

	hrtimer_init(&qmi8658->acc_hr_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	qmi8658->acc_hr_timer.function = acc_hrtimer_callback;
	INIT_WORK(&qmi8658->acc_work, acc_work_func);

	hrtimer_init(&qmi8658->gyro_hr_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	qmi8658->gyro_hr_timer.function = gyro_hrtimer_callback;
	INIT_WORK(&qmi8658->gyro_work, gyro_work_func);

	err = qmi8658_input_init(qmi8658);
	if (err < 0) {
		QMI8658_ERR("input init fail!\n");
		goto exit1;
	}

	err = sysfs_create_group(&qmi8658->acc_input->dev.kobj, &qmi8658_attribute_group);
	err += sysfs_create_group(&qmi8658->gyro_input->dev.kobj, &qmi8658_attribute_group);
	if (err < 0) {
		QMI8658_ERR("%s: create group fail!\n", __func__);
		goto exit2;
	}
	kobject_uevent(&qmi8658->acc_input->dev.kobj, KOBJ_CHANGE);
	kobject_uevent(&qmi8658->gyro_input->dev.kobj, KOBJ_CHANGE);
	return 0;

exit2:
	qmi8658_input_deinit(qmi8658);
exit1:
	if (qmi8658) {
		kfree(qmi8658);
		qmi8658 = NULL;
	}
exit:
	return err;
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0))
static void qmi8658_remove(struct i2c_client *client)
#else
static int qmi8658_remove(struct i2c_client *client)
#endif
{
	QMI8658_FUN();
	if (qmi8658) {
		sysfs_remove_group(&qmi8658->acc_input->dev.kobj, &qmi8658_attribute_group);
		sysfs_remove_group(&qmi8658->gyro_input->dev.kobj, &qmi8658_attribute_group);

		qmi8658_set_enable(QMI8658_TYPE_ACC, 0);
		qmi8658_set_enable(QMI8658_TYPE_GYRO, 0);

		cancel_work_sync(&qmi8658->acc_work);
		cancel_work_sync(&qmi8658->gyro_work);

		destroy_workqueue(qmi8658->qmi_wq);

		qmi8658_input_deinit(qmi8658);
		kfree(qmi8658);
		qmi8658 = NULL;
	}
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0))
	return;
#else
	return 0;
#endif
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0))
static void qmi8658_i2c_remove(struct i2c_client *client)
#else
static int qmi8658_i2c_remove(struct i2c_client *client)
#endif
{
	return qmi8658_remove(client);
}


static void qmi8658_shutdown(struct i2c_client *client)
{
	QMI8658_FUN();

	if (qmi8658) {
		qmi8658_set_enable(QMI8658_TYPE_ACC, 0);
		qmi8658_set_enable(QMI8658_TYPE_GYRO, 0);
	}
}

static int qmi8658_suspend(struct device *dev)
{
	QMI8658_FUN();

	if (atomic_read(&qmi8658->acc_enable)) {
		hrtimer_cancel(&qmi8658->gyro_hr_timer);
	}

	if (atomic_read(&qmi8658->gyro_enable)) {
		hrtimer_cancel(&qmi8658->gyro_hr_timer);
	}

	input_set_power_enable(&(gsensor_info.input_type), 0);

	return 0;
}

static int qmi8658_resume(struct device *dev)
{
	QMI8658_FUN();
	input_set_power_enable(&(gsensor_info.input_type), 1);
	atomic_set(&qmi8658->need_init, 1);

	if (atomic_read(&qmi8658->acc_enable)) {
		hrtimer_start(&qmi8658->acc_hr_timer, qmi8658->acc_ktime, HRTIMER_MODE_REL);
		//schedule_delayed_work(&qmi8658->acc_work, msecs_to_jiffies(atomic_read(&qmi8658->acc_delay) - 1));
	}

	if (atomic_read(&qmi8658->gyro_enable)) {
		hrtimer_start(&qmi8658->gyro_hr_timer, qmi8658->gyro_ktime, HRTIMER_MODE_REL);
		//schedule_delayed_work(&qmi8658->gyro_work, msecs_to_jiffies(atomic_read(&qmi8658->gyro_delay) - 1));
	}

	return 0;
}


static const struct i2c_device_id qmi8658_id[] = {
	{QMI8658_DEV_NAME, 0},
	{ }
};

MODULE_DEVICE_TABLE(i2c, qmi8658_id);

#ifdef CONFIG_OF
static const struct of_device_id qmi8658_of_match[] = {
       { .compatible = "allwinner,qmi8658", },
       { }
};
#endif
MODULE_DEVICE_TABLE(of, qmi8658_of_match);

#if IS_ENABLED(CONFIG_PM)
static UNIVERSAL_DEV_PM_OPS(qmi8658_pm_ops, qmi8658_suspend,
		qmi8658_resume, NULL);
#endif

static struct i2c_driver qmi8658_driver = {
	.class = I2C_CLASS_HWMON,
	.driver = {
		.name = QMI8658_DEV_NAME,
		.owner = THIS_MODULE,
#ifdef CONFIG_OF
		.of_match_table = qmi8658_of_match,
#endif
#if IS_ENABLED(CONFIG_PM)
		.pm = &qmi8658_pm_ops,
#endif
	},
	.probe    = qmi8658_i2c_probe,
	.remove   = qmi8658_i2c_remove,
	.id_table = qmi8658_id,
	.shutdown	= qmi8658_shutdown,
	.address_list = normal_i2c,
};

static int startup(void)
{
	int ret = -1;

	printk("function=%s=========LINE=%d. \n", __func__, __LINE__);

	if (input_sensor_startup(&(gsensor_info.input_type))) {
		printk("%s: err.\n", __func__);
		return -1;
	} else
		ret = input_sensor_init(&(gsensor_info.input_type));

	if (0 != ret) {
	    printk("%s:gsensor.init_platform_resource err. \n", __func__);
	}

	twi_id = gsensor_info.twi_id;
	input_set_power_enable(&(gsensor_info.input_type), 1);
	return 0;
}

static int __init qmi8658_i2c_init(void)
{
	int ret = -1;

	QMI8658_LOG("%s A+G driver: init\n", QMI8658_DEV_NAME);
	if (startup() != 0)
		return -1;

	if (!gsensor_info.isI2CClient)
		qmi8658_driver.detect = qmi8658_gsensor_detect;

	ret = i2c_add_driver(&qmi8658_driver);
	if (ret < 0) {
		printk("add qmi8658 i2c driver failed\n");
		return -ENODEV;
	}

	return 0;
}

static void __exit qmi8658_i2c_exit(void)
{
	QMI8658_LOG("%s A+G driver exit\n", QMI8658_DEV_NAME);

	i2c_del_driver(&qmi8658_driver);
}

module_init(qmi8658_i2c_init);			//late_initcall(qmi8658_i2c_init);
module_exit(qmi8658_i2c_exit);

MODULE_DESCRIPTION("qmi8658 IMU driver");
MODULE_AUTHOR("qing_lv@qstcorp.com");
MODULE_LICENSE("GPL");
MODULE_VERSION(QMI8658_DEV_VERSION);

