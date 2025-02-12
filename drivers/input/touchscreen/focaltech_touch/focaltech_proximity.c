/*
 *
 * FocalTech TouchScreen driver.
 *
 * Copyright (c) 2012-2020, FocalTech Systems, Ltd., all rights reserved.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

/*****************************************************************************
*
* File Name: focaltech_proximity.c
*
*    Author: Focaltech Driver Team
*
*   Created: 2016-09-19
*
*  Abstract: close proximity function
*
*   Version: v1.0
*
* Revision History:
*        v1.0:
*            First release based on xiaguobin's solution. By luougojin 2016-08-19
*****************************************************************************/

/*****************************************************************************
* Included header files
*****************************************************************************/
#include "focaltech_core.h"
#include "focaltech_common.h"

#if FTS_PSENSOR_EN
/*****************************************************************************
 * Private constant and macro definitions using #define
 *****************************************************************************/
/* proximity register address*/
#define FTS_REG_PSENSOR_ENABLE                  0xB0
#define FTS_REG_PSENSOR_STATUS                  0xB5
#define FTS_REG_PSENSOR_CLEAR_STATUS            0xB6

/* proximity register value read from TP */
#define PROXIMITY_TP_VAL_NEAR                   0xC0
#define PROXIMITY_TP_VAL_FAR                    0xE0
#define PROXIMITY_TP_VAL_ERROR                  0xEE
#define PROXIMITY_TP_VAL_DEFAULT                0xFF

/* host state : far or near */
#define PROXIMITY_HOST_STATE_NEAR               0
#define PROXIMITY_HOST_STATE_FAR                1
#define PROXIMITY_HOST_STATE_DEFAULT            PROXIMITY_HOST_STATE_FAR

/* proximity solutions */
#define PROXIMITY_SOLUTION_SAMPLE               0
#define PROXIMITY_SOLUTION_QCOM                 1
#define PROXIMITY_SOLUTION_MTK                  2
#define PROXIMITY_SOLUTION_SAMPLE_1             3
#define PROXIMITY_SOLUTION                      PROXIMITY_SOLUTION_SAMPLE

/*****************************************************************************
* Private enumerations, structures and unions using typedef
*****************************************************************************/
struct fts_proximity_ops;

/*
 * @tp_val, proximity value read from TP register, the value is:
 *          PROXIMITY_TP_VAL_NEAR/PROXIMITY_TP_VAL_FAR and so on.
 *
 * @tp_val_last, the backup proximity value
 * @host_state, the proximity state of host, tp driver will report the
 *      value to Android, the value is:
 *          PROXIMITY_HOST_STATE_NEAR
 *          PROXIMITY_HOST_STATE_FAR
 */
struct fts_proximity {
    struct fts_ts_data *ts_data;
    struct input_dev *proximity_input_dev;
    struct fts_proximity_ops *ops;
    u8 tp_val;
    u8 tp_val_last;
    int host_state;
};

struct fts_proximity_ops {
    int (*init)(struct fts_proximity *proximity_data);
    int (*exit)(struct fts_proximity *proximity_data);
    int (*report)(struct fts_proximity *proximity_data);
};

/*****************************************************************************
* variables or functions
*****************************************************************************/
static struct fts_proximity fts_proximity_data;


static void fts_proximity_set_reg(int value)
{
    int i = 0;
    u8 enable_value = value ? 0x01 : 0x00;
    u8 regval = 0xFF;

    for (i = 0; i < FTS_MAX_RETRIES_WRITEREG; i++) {
        fts_read_reg(FTS_REG_PSENSOR_ENABLE, &regval);
        if (regval == enable_value)
            break;
        fts_write_reg(FTS_REG_PSENSOR_ENABLE, enable_value);
        fts_msleep(1);
    }

    if (i >= FTS_MAX_RETRIES_WRITEREG)
        FTS_ERROR("set proximity mode to %x failed,reg_val:%x", enable_value, regval);
    else if (i > 0)
        FTS_INFO("set proximity mode to %x successfully", value);
}

/************************************************************************
* Name: fts_proximity_enable
* Brief:  enable or disable proximity function, set variable and write it
*         to TP FW.
*
* Input:  proximity_data, global structure variable.
*         @enable, 0 is to disable proximity, 1 to enable.
* Output:
*
* Return: 0 for success
***********************************************************************/
static int fts_proximity_enable(struct fts_proximity *proximity_data, int enable)
{
    int ret = 0;
    if (!proximity_data || !proximity_data->ts_data || !proximity_data->proximity_input_dev) {
        FTS_ERROR("proximity/ts/input is null");
        return -EINVAL;
    }

    FTS_INFO("set proximity mode to %s", !!enable ? "enable" : "disable");
    mutex_lock(&proximity_data->proximity_input_dev->mutex);
    proximity_data->ts_data->proximity_mode = !!enable;
    fts_proximity_set_reg(enable);
    proximity_data->tp_val = PROXIMITY_TP_VAL_DEFAULT;
    proximity_data->tp_val_last = PROXIMITY_TP_VAL_DEFAULT;
    proximity_data->host_state = PROXIMITY_HOST_STATE_DEFAULT;
    mutex_unlock(&proximity_data->proximity_input_dev->mutex);
    return ret;
}




#if (PROXIMITY_SOLUTION == PROXIMITY_SOLUTION_MTK)
#include <hwmsensor.h>
#include <sensors_io.h>
#include <alsps.h>

/*****************************************************************************
* Private constant and macro definitions using #define
*****************************************************************************/
/*
 * FTS_ALSPS_SUPPORT is choose structure hwmsen_object or control_path, data_path
 * FTS_ALSPS_SUPPORT = 1, is control_path, data_path
 * FTS_ALSPS_SUPPORT = 0, hwmsen_object
 */
#define FTS_ALSPS_SUPPORT            1
/*
 * FTS_OPEN_DATA_HAL_SUPPORT is choose structure ps_control_path or batch, flush
 * FTS_ALSPS_SUPPORT = 1, is batch, flush
 * FTS_ALSPS_SUPPORT = 0, NULL
 */
#define FTS_OPEN_DATA_HAL_SUPPORT    1

#if !FTS_ALSPS_SUPPORT
#include <hwmsen_dev.h>
#endif

/*****************************************************************************
* Static variables
*****************************************************************************/

/*****************************************************************************
* Global variable or extern global variabls/functions
*****************************************************************************/

/*****************************************************************************
* Static function prototypes
*****************************************************************************/

#if FTS_ALSPS_SUPPORT
/* if use  this typ of enable , Gsensor should report inputEvent(x, y, z ,stats, div) to HAL */
static int ps_open_report_data(int open)
{
    /* should queue work to report event if  is_report_input_direct=true */
    return 0;
}

/* if use  this type of enable , Psensor only enabled but not report inputEvent to HAL */
static int ps_enable_nodata(int en)
{
    int ret = 0;
    FTS_DEBUG("[PROXIMITY]SENSOR_ENABLE value = %d", en);
    /* Enable proximity */
    ret = fts_proximity_enable(fts_proximity_data, en);
    return ret;
}

static int ps_set_delay(u64 ns)
{
    return 0;
}

#if FTS_OPEN_DATA_HAL_SUPPORT
static int ps_batch(int flag, int64_t sampling_period_ns, int64_t max_batch_report_ns)
{
    return 0;
}

static int ps_flush(void)
{
    return 0;
}
#endif

static int ps_get_data(int *value, int *status)
{
    *value = (int)fts_proximity_data.host_state;
    FTS_DEBUG("proximity status = %d\n", *value);
    *status = SENSOR_STATUS_ACCURACY_MEDIUM;
    return 0;
}

static int ps_local_init(void)
{
    int err = 0;
    struct ps_control_path ps_ctl = { 0 };
    struct ps_data_path ps_data = { 0 };

    ps_ctl.is_use_common_factory = false;
    ps_ctl.open_report_data = ps_open_report_data;
    ps_ctl.enable_nodata = ps_enable_nodata;
    ps_ctl.set_delay = ps_set_delay;
#if FTS_OPEN_DATA_HAL_SUPPORT
    ps_ctl.batch = ps_batch;
    ps_ctl.flush = ps_flush;
#endif
    ps_ctl.is_report_input_direct = false;
    ps_ctl.is_support_batch = false;

    err = ps_register_control_path(&ps_ctl);
    if (err) {
        FTS_ERROR("register fail = %d\n", err);
    }
    ps_data.get_data = ps_get_data;
    ps_data.vender_div = 100;
    err = ps_register_data_path(&ps_data);
    if (err) {
        FTS_ERROR("tregister fail = %d\n", err);
    }

    return err;
}
int ps_local_uninit(void)
{
    return 0;
}

struct alsps_init_info ps_init_info = {
    .name = "fts_ts",
    .init = ps_local_init,
    .uninit = ps_local_uninit,
};

#else

static int mtk_ps_operate(void *self, uint32_t command, void *buff_in,
                          int size_in, void *buff_out, int size_out, int *actualout)
{
    int err = 0;
    int value;
    struct hwm_sensor_data *sensor_data;
    struct fts_proximity *proximity_data = &fts_proximity_data;

    if (!proximity_data || !proximity_data->ts_data) {
        FTS_ERROR("proximity_data/ts_data" is null);
        return -EINVAL;
    }

    FTS_DEBUG("[PROXIMITY]COMMAND = %d", command);
    switch (command) {
    case SENSOR_DELAY:
        if ((buff_in == NULL) || (size_in < sizeof(int))) {
            FTS_ERROR("[PROXIMITY]Set delay parameter error!");
            err = -EINVAL;
        }
        break;

    case SENSOR_ENABLE:
        if ((buff_in == NULL) || (size_in < sizeof(int))) {
            FTS_ERROR("[PROXIMITY]Enable sensor parameter error!");
            err = -EINVAL;
        } else {
            value = *(int *)buff_in;
            FTS_DEBUG("[PROXIMITY]SENSOR_ENABLE value = %d", value);
            /* Enable proximity */
            err = fts_proximity_enable(proximity_data, value);
        }
        break;

    case SENSOR_GET_DATA:
        if ((buff_out == NULL) || (size_out < sizeof(struct hwm_sensor_data))) {
            FTS_ERROR("[PROXIMITY]get sensor data parameter error!");
            err = -EINVAL;
        } else {
            sensor_data = (struct hwm_sensor_data *)buff_out;
            sensor_data->values[0] = (int)proximity_data->host_state;
            FTS_DEBUG("sensor_data->values[0] = %d", sensor_data->values[0]);
            sensor_data->value_divide = 1;
            sensor_data->status = SENSOR_STATUS_ACCURACY_MEDIUM;
        }
        break;
    default:
        FTS_ERROR("[PROXIMITY]ps has no operate function:%d!", command);
        err = -EPERM;
        break;
    }

    return err;
}
#endif


static int mtk_proximity_report(struct fts_proximity *proximity_data)
{
    int ret = 0;
    int proximity_state = PROXIMITY_HOST_STATE_DEFAULT;
#if !FTS_ALSPS_SUPPORT
    struct hwm_sensor_data sensor_data;
#endif

    if (proximity_data->tp_val == PROXIMITY_TP_VAL_NEAR) {
        /* close. need lcd off */
        proximity_state = PROXIMITY_HOST_STATE_NEAR;
    } else if (proximity_data->tp_val == PROXIMITY_TP_VAL_FAR) {
        /* far away */
        proximity_state = PROXIMITY_HOST_STATE_FAR;
    } else if (proximity_data->tp_val == PROXIMITY_TP_VAL_ERROR) {
        /* error, need report far away */
        proximity_state = PROXIMITY_HOST_STATE_FAR;
    }

    if (proximity_state != proximity_data->host_state) {
        FTS_INFO("report proximity state:%s", proximity_state ? "AWAY" : "NEAR");
        proximity_data->host_state = proximity_state;
#if FTS_ALSPS_SUPPORT
        ret = ps_report_interrupt_data(proximity_state);
#else
        sensor_data.values[0] = proximity_state;
        sensor_data.value_divide = 1;
        sensor_data.status = SENSOR_STATUS_ACCURACY_MEDIUM;
        ret = hwmsen_get_interrupt_data(ID_PROXIMITY, &sensor_data);
        if (ret) {
            FTS_ERROR("[PROXIMITY] Call hwmsen_get_interrupt_data failed, ret=%d", ret);
            return ret;
        }
#endif
        return FTS_RETVAL_IGNORE_TOUCHES;
    }

    return 0;
}

static int mtk_proximity_init(struct fts_proximity *proximity_data)
{
#if !FTS_ALSPS_SUPPORT
    int err = 0;
    struct hwmsen_object obj_ps;
#endif

    FTS_FUNC_ENTER();
#if FTS_ALSPS_SUPPORT
    alsps_driver_add(&ps_init_info);
#else
    obj_ps.polling = 0; /* interrupt mode */
    obj_ps.sensor_operate = mtk_ps_operate;
    err = hwmsen_attach(ID_PROXIMITY, &obj_ps);
    if (err)
        FTS_ERROR("[PROXIMITY]fts proximity attach fail = %d!", err);
    else
        FTS_INFO("[PROXIMITY]fts proximity attach ok = %d\n", err);
#endif

    FTS_FUNC_EXIT();
    return 0;
}

struct fts_proximity_ops mtk_proximity_ops = {
    .init = mtk_proximity_init,
    .report = mtk_proximity_report,
};
#endif

#if (PROXIMITY_SOLUTION == PROXIMITY_SOLUTION_QCOM)
#include <linux/sensors.h>

struct qcom_proximity {
    struct fts_proximity *proximity_data;
    struct sensors_classdev ps_cdev;
};

static struct qcom_proximity qcom_proximity_data;

static struct sensors_classdev __maybe_unused qcom_proximity_cdev = {
    .name = "fts-proximity",
    .vendor = "FocalTech",
    .version = 1,
    .handle = SENSORS_PROXIMITY_HANDLE,
    .type = SENSOR_TYPE_PROXIMITY,
    .max_range = "5.0",
    .resolution = "5.0",
    .sensor_power = "0.1",
    .min_delay = 0,
    .fifo_reserved_event_count = 0,
    .fifo_max_event_count = 0,
    .enabled = 0,
    .delay_msec = 200,
    .sensors_enable = NULL,
    .sensors_poll_delay = NULL,
};

static int qcom_proximity_enable(struct sensors_classdev *sensors_cdev, unsigned int enable)
{
    struct qcom_proximity *qps = container_of(sensors_cdev, struct qcom_proximity, ps_cdev);
    if (qps && qps->proximity_data) {
        fts_proximity_enable(qps->proximity_data, enable);
    }
    return enable;
}

static int qcom_proximity_report(struct fts_proximity *proximity_data)
{
    int proximity_state = PROXIMITY_HOST_STATE_DEFAULT;

    if (!proximity_data || !proximity_data->proximity_input_dev) {
        FTS_ERROR("proximity/input is null");
        return -EINVAL;
    }

    if (proximity_data->tp_val == PROXIMITY_TP_VAL_NEAR) {
        /* close. need lcd off */
        proximity_state = PROXIMITY_HOST_STATE_NEAR;
    } else if (proximity_data->tp_val == PROXIMITY_TP_VAL_FAR) {
        /* far away */
        proximity_state = PROXIMITY_HOST_STATE_FAR;
    } else if (proximity_data->tp_val == PROXIMITY_TP_VAL_ERROR) {
        /* error, need report far away */
        proximity_state = PROXIMITY_HOST_STATE_FAR;
    }

    if (proximity_state != proximity_data->host_state) {
        FTS_INFO("report proximity state:%s", proximity_state ? "AWAY" : "NEAR");
        proximity_data->host_state = proximity_state;
        input_report_abs(proximity_data->proximity_input_dev, ABS_DISTANCE,
                         (proximity_state == PROXIMITY_HOST_STATE_NEAR) ? 0 : 1;
                         input_sync(proximity_data->proximity_input_dev);
                         return FTS_RETVAL_IGNORE_TOUCHES;
    }

    return 0;
}

static int qcom_proximity_init(struct fts_proximity *proximity_data)
{
    int ret = 0;
    struct qcom_proximity *qps = &qcom_proximity_data;
    FTS_FUNC_ENTER();
    if (proximity_data && proximity_data->ts_data && proximity_data->ts_data->dev) {
        memset(qps, 0, sizeof(struct qcom_proximity));
        qps->proximity_data = proximity_data;
        qps->ps_cdev = qcom_proximity_cdev;
        qps->ps_cdev.sensors_enable = qcom_proximity_enable;
        ret = sensors_classdev_register(proximity_data->ts_data->dev, &qps->ps_cdev);
        if (ret) FTS_ERROR("sensors_classdev_register failed,ret=%d", ret);
    } else {
        FTS_ERROR("proximity/ts/device is null");
        ret = -EINVAL;
    }
    FTS_FUNC_EXIT();
    return ret;
}

static int qcom_proximity_exit(struct fts_proximity *proximity_data)
{
    FTS_FUNC_ENTER();
    sensors_classdev_unregister(qcom_proximity_data.ps_cdev);
    FTS_FUNC_EXIT();
    return 0;
}

struct fts_proximity_ops qcom_proximity_ops = {
    .init = qcom_proximity_init,
    .exit = qcom_proximity_exit,
    .report = qcom_proximity_report,
};
#endif // #if (PROXIMITY_SOLUTION == PROXIMITY_SOLUTION_QCOM)


#if (PROXIMITY_SOLUTION == PROXIMITY_SOLUTION_SAMPLE_1)
static int sample_1_proximity_report(struct fts_proximity *proximity_data)
{
    u8 clear_status_value = 0xFF;
    int proximity_state = PROXIMITY_HOST_STATE_DEFAULT;

    if (!proximity_data || !proximity_data->proximity_input_dev) {
        FTS_ERROR("proximity/input is null");
        return -EINVAL;
    }

    if ((proximity_data->tp_val > 0x00) && (proximity_data->tp_val < 0x04)) {
        /* close. need lcd off */
        proximity_state = PROXIMITY_HOST_STATE_NEAR;
        clear_status_value = 1;
    } else if (proximity_data->tp_val == 0x00) {
        /* far away */
        proximity_state = PROXIMITY_HOST_STATE_FAR;
        clear_status_value = 0;
    } else {
        /* error, need report far away */
        proximity_state = PROXIMITY_HOST_STATE_FAR;
        clear_status_value = 0xEE;
    }

    if (proximity_data->tp_val != proximity_data->tp_val_last) {
        FTS_DEBUG("tp proximity status:%x->%x", proximity_data->tp_val_last, proximity_data->tp_val);
        tpd_notifier_call_chain(proximity_data->host_state, NULL);
        fts_write_reg(FTS_REG_PSENSOR_CLEAR_STATUS, clear_status_value);
    }

    if (proximity_state != proximity_data->host_state) {
        FTS_INFO("report proximity state:%s", proximity_state ? "AWAY" : "NEAR");
        proximity_data->host_state = proximity_state;
        return FTS_RETVAL_IGNORE_TOUCHES;
    }

    return 0;
}

static struct fts_proximity_ops sample_1_proximity_ops = {
    .report = sample_1_proximity_report,
};
#endif // #if (PROXIMITY_SOLUTION == PROXIMITY_SOLUTION_SAMPLE_1)


#if (PROXIMITY_SOLUTION == PROXIMITY_SOLUTION_SAMPLE)
static int sample_proximity_report(struct fts_proximity *proximity_data)
{
    int proximity_state = PROXIMITY_HOST_STATE_DEFAULT;
    if (!proximity_data || !proximity_data->proximity_input_dev) {
        FTS_ERROR("proximity/input is null");
        return -EINVAL;
    }

    if (proximity_data->tp_val == PROXIMITY_TP_VAL_NEAR) {
        /* close. need lcd off */
        proximity_state = PROXIMITY_HOST_STATE_NEAR;
    } else if (proximity_data->tp_val == PROXIMITY_TP_VAL_FAR) {
        /* far away */
        proximity_state = PROXIMITY_HOST_STATE_FAR;
    } else if (proximity_data->tp_val == PROXIMITY_TP_VAL_ERROR) {
        /* error, need report far away */
        proximity_state = PROXIMITY_HOST_STATE_FAR;
    }

    if (proximity_data->ts_data->log_level >= 3) {
        FTS_DEBUG("tp proximity status, now:%x,before:%x", proximity_data->tp_val, proximity_data->tp_val_last);
        FTS_DEBUG("proximity state, now:%x,before:%d", proximity_state, proximity_data->host_state);
    }
    if (proximity_state != proximity_data->host_state) {
        FTS_INFO("report proximity state:%s", proximity_state ? "AWAY" : "NEAR");
        proximity_data->host_state = proximity_state;
        /* TODO: Report proximity state to host */


        return FTS_RETVAL_IGNORE_TOUCHES;
    }

    return 0;
}

static int sample_proximity_init(struct fts_proximity *proximity_data)
{
    FTS_FUNC_ENTER();
    FTS_FUNC_EXIT();
    return 0;
}

static int sample_proximity_exit(struct fts_proximity *proximity_data)
{
    FTS_FUNC_ENTER();
    FTS_FUNC_EXIT();
    return 0;
}

static struct fts_proximity_ops sample_proximity_ops = {
    .init = sample_proximity_init,
    .exit = sample_proximity_exit,
    .report = sample_proximity_report,
};
#endif // #if (PROXIMITY_SOLUTION == PROXIMITY_SOLUTION_SAMPLE)


static ssize_t fts_proximity_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    int count = 0;
    u8 val = 0;
    struct fts_ts_data *ts_data = dev_get_drvdata(dev);
    struct fts_proximity *proximity_data = &fts_proximity_data;

    if (proximity_data->proximity_input_dev) {
        mutex_lock(&proximity_data->proximity_input_dev->mutex);
        fts_read_reg(FTS_REG_PSENSOR_ENABLE, &val);
        count = snprintf(buf, PAGE_SIZE, "Proximity Mode:%s\n", ts_data->proximity_mode ? "On" : "Off");
        count += snprintf(buf + count, PAGE_SIZE, "Reg(0xB0)=%d\n", val);
        mutex_unlock(&proximity_data->proximity_input_dev->mutex);
    }

    return count;
}

static ssize_t fts_proximity_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    struct fts_ts_data *ts_data = dev_get_drvdata(dev);
    struct fts_proximity *proximity_data = &fts_proximity_data;

    if (FTS_SYSFS_ECHO_ON(buf)) {
        if (ts_data->suspended) {
            FTS_INFO("In suspend,not allowed to enable proximity mode");
        } else {
            FTS_DEBUG("enable proximity");
            fts_proximity_enable(proximity_data, ENABLE);
        }
    } else if (FTS_SYSFS_ECHO_OFF(buf)) {
        FTS_DEBUG("disable proximity");
        fts_proximity_enable(proximity_data, DISABLE);
    }

    return count;
}

/* sysfs node of proximity_mode, maybe not used */
static DEVICE_ATTR(fts_proximity_mode, S_IRUGO | S_IWUSR, fts_proximity_show, fts_proximity_store);
static struct attribute *fts_proximity_mode_attrs[] = { &dev_attr_fts_proximity_mode.attr, NULL, };
static struct attribute_group fts_proximity_group = {.attrs = fts_proximity_mode_attrs,};


static fts_proximity_input_init(struct fts_proximity *proximity_data)
{
    int ret = 0;
    struct input_dev *proximity_input_dev;

    FTS_FUNC_ENTER();
    if (!proximity_data || !proximity_data->ts_data || !proximity_data->ts_data->dev) {
        FTS_ERROR("proximity_data/ts_data/device is null");
        return -EINVAL;
    }

    proximity_input_dev = input_allocate_device();
    if (!proximity_input_dev) {
        FTS_ERROR("Failed to allocate memory for input_proximity device");
        return -ENOMEM;
    }

    proximity_input_dev->dev.parent = proximity_data->ts_data->dev;
    proximity_input_dev->name = "proximity";
    __set_bit(EV_ABS, proximity_input_dev->evbit);
    input_set_abs_params(proximity_input_dev, ABS_DISTANCE, 0, 1, 0, 0);
    ret = input_register_device(proximity_input_dev);
    if (ret) {
        FTS_ERROR("proximity input device registration failed");
        input_free_device(proximity_input_dev);
        proximity_input_dev = NULL;
        return ret;
    }

    proximity_data->proximity_input_dev = proximity_input_dev;
    FTS_FUNC_EXIT();
    return 0;
}

int fts_proximity_recovery(struct fts_ts_data *ts_data)
{
    if (ts_data->proximity_mode) {
        fts_proximity_set_reg(ENABLE);
    }
    return 0;
}

/*****************************************************************************
* Name: fts_proximity_readdata
* Brief: read proximity value from TP, check whether tp is near or far away,
*        and report the state to host if need.
*
* Input: ts_data
* Output:
* Return: return negative code if error occurs,return 0 or 1 if success.
*         return 0 if continue report finger touches.
*         return 1(FTS_RETVAL_IGNORE_TOUCHES) if you want to ingore this
*         finger reporting, As default, the following situation will report 1:
*               a.proximity state changed
*               b.System in suspend state
*****************************************************************************/
int fts_proximity_readdata(struct fts_ts_data *ts_data)
{
    int ret = 0;
    struct fts_proximity *proximity_data = &fts_proximity_data;
    u8 psensor_status = 0xFF;
    u8 psensor_enable = 0xFF;

    ret = fts_read_reg(FTS_REG_PSENSOR_ENABLE, &psensor_enable);
    if (psensor_enable != ENABLE) {
        FTS_DEBUG("proximity not enable in FW, don't process proximity");
        return 0;
    }

    ret = fts_read_reg(FTS_REG_PSENSOR_STATUS, &psensor_status);
    if (ret < 0) {
        FTS_ERROR("read proximity value failed,ret=%d", ret);
        proximity_data->tp_val = PROXIMITY_TP_VAL_ERROR;
    } else {
        if (ts_data->log_level >= 3)
            FTS_INFO("read proximity status:0x%x", psensor_status);
        else if (proximity_data->tp_val != psensor_status)
            FTS_INFO("read proximity status:0x%x[%x]", psensor_status, proximity_data->tp_val);
        proximity_data->tp_val = psensor_status;
    }

    if (proximity_data->ops->report) {
        ret = proximity_data->ops->report(proximity_data);
    }

    proximity_data->tp_val_last = proximity_data->tp_val;
    if (ts_data->suspended) ret = FTS_RETVAL_IGNORE_TOUCHES;
    return ret;
}

int fts_proximity_suspend(struct fts_ts_data *ts_data)
{
    if (enable_irq_wake(ts_data->irq)) {
        FTS_ERROR("enable_irq_wake(irq:%d) fail", ts_data->irq);
    }
    FTS_INFO("proximity mode in suspend.");
    return 0;
}

int fts_proximity_resume(struct fts_ts_data *ts_data)
{
    if (disable_irq_wake(ts_data->irq)) {
        FTS_ERROR("disable_irq_wake(irq:%d) fail", ts_data->irq);
    }
    fts_proximity_recovery(ts_data);
    return 0;
}

int fts_proximity_init(struct fts_ts_data *ts_data)
{
    int ret = 0;
    struct fts_proximity *proximity_data = &fts_proximity_data;

    FTS_FUNC_ENTER();
    memset((u8 *)proximity_data, 0, sizeof(struct fts_proximity));
    proximity_data->ts_data = ts_data;
    proximity_data->tp_val = PROXIMITY_TP_VAL_DEFAULT;
    proximity_data->tp_val_last = PROXIMITY_TP_VAL_DEFAULT;
    proximity_data->host_state = PROXIMITY_HOST_STATE_DEFAULT;

    /* TODO: initialize following platform implementation  */
#if (PROXIMITY_SOLUTION == PROXIMITY_SOLUTION_SAMPLE)
    proximity_data->ops = &sample_proximity_ops;
#elif (PROXIMITY_SOLUTION == PROXIMITY_SOLUTION_QCOM)
    proximity_data->ops = &qcom_proximity_ops;
#elif (PROXIMITY_SOLUTION == PROXIMITY_SOLUTION_MTK)
    proximity_data->ops = &mtk_proximity_ops;
#elif (PROXIMITY_SOLUTION == PROXIMITY_SOLUTION_SAMPLE_1)
    proximity_data->ops = &sample_1_proximity_ops;
#endif

    ret = fts_proximity_input_init(proximity_data);
    if (ret) {
        FTS_ERROR("proximity input init failed");
        return ret;
    }

    ret = sysfs_create_group(&ts_data->dev->kobj, &fts_proximity_group);
    if (ret) {
        FTS_ERROR("proximity sys node create failed");
        sysfs_remove_group(&ts_data->dev->kobj, &fts_proximity_group);
    }

    if (proximity_data->ops && proximity_data->ops->init) {
        ret = proximity_data->ops->init(proximity_data);
        if (ret) FTS_ERROR("proximity init failed,ret=%d", ret);
    }
    FTS_FUNC_EXIT();
    return ret;
}

int fts_proximity_exit(struct fts_ts_data *ts_data)
{
    int ret = 0;
    struct fts_proximity *proximity_data = &fts_proximity_data;
    FTS_FUNC_ENTER();
    sysfs_remove_group(&ts_data->dev->kobj, &fts_proximity_group);
    input_unregister_device(proximity_data->proximity_input_dev);
    if (proximity_data->ops && proximity_data->ops->exit) {
        ret = proximity_data->ops->exit(proximity_data);
        if (ret) FTS_ERROR("proximity exit failed,ret=%d", ret);
    }
    FTS_FUNC_EXIT();
    return ret;
}
#endif  /* FTS_PSENSOR_EN */


