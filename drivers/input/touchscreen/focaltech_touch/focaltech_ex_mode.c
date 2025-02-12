/*
 *
 * FocalTech ftxxxx TouchScreen driver.
 *
 * Copyright (c) 2012-2020, Focaltech Ltd. All rights reserved.
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
* File Name: focaltech_ex_mode.c
*
* Author: Focaltech Driver Team
*
* Created: 2016-08-31
*
* Abstract:
*
* Reference:
*
*****************************************************************************/

/*****************************************************************************
* 1.Included header files
*****************************************************************************/
#include "focaltech_core.h"

/*****************************************************************************
* 2.Private constant and macro definitions using #define
*****************************************************************************/

/*****************************************************************************
* 3.Private enumerations, structures and unions using typedef
*****************************************************************************/
enum _ex_mode {
    MODE_GLOVE = 0,
    MODE_COVER,
    MODE_CHARGER,
    MODE_EARPHONE,
    MODE_EDGEPALM
};

/*****************************************************************************
* 4.Static variables
*****************************************************************************/

/*****************************************************************************
* 5.Global variable or extern global variabls/functions
*****************************************************************************/

/*****************************************************************************
* 6.Static function prototypes
*******************************************************************************/
static int fts_ex_mode_set_reg(u8 mode_regaddr, u8 mode_regval)
{
    int i = 0;
    u8 val = 0xFF;

    for (i = 0; i < FTS_MAX_RETRIES_WRITEREG; i++) {
        fts_read_reg(mode_regaddr, &val);
        if (val == mode_regval)
            break;
        fts_write_reg(mode_regaddr, mode_regval);
        fts_msleep(1);
    }

    if (i >= FTS_MAX_RETRIES_WRITEREG) {
        FTS_ERROR("set mode(%x) to %x failed,read val:%x", mode_regaddr, mode_regval, val);
        return -EIO;
    } else if (i > 0) {
        FTS_INFO("set mode(%x) to %x successfully", mode_regaddr, mode_regval);
    }
    return 0;
}

static int fts_ex_mode_switch(enum _ex_mode mode, int value)
{
    int ret = 0;

    switch (mode) {
    case MODE_GLOVE:
        ret = fts_ex_mode_set_reg(FTS_REG_GLOVE_MODE_EN, (value ? 0x01 : 0x00));
        if (ret) FTS_ERROR("Set MODE_GLOVE to %d failed", value);
        break;
    case MODE_COVER:
        ret = fts_ex_mode_set_reg(FTS_REG_COVER_MODE_EN, (value ? 0x01 : 0x00));
        if (ret) FTS_ERROR("Set MODE_COVER to %d failed", value);
        break;
    case MODE_CHARGER:
        ret = fts_ex_mode_set_reg(FTS_REG_CHARGER_MODE_EN, (value ? 0x01 : 0x00));
        if (ret) FTS_ERROR("Set MODE_CHARGER to %d failed", value);
        break;
    case MODE_EARPHONE:
        ret = fts_ex_mode_set_reg(FTS_REG_EARPHONE_MODE_EN, (value ? 0x01 : 0x00));
        if (ret) FTS_ERROR("Set MODE_EARPHONE to %d failed", value);
        break;
    case MODE_EDGEPALM:
        /* FW defines the following values: 0:vertical, 1:horizontal, USB on the right,
         *                                  2:horizontal, USB on the left
         * If host set the value not defined above, you should have a transition.
         */
        ret = fts_ex_mode_set_reg(FTS_REG_EDGEPALM_MODE_EN, (u8)value);
        if (ret) FTS_ERROR("Set MODE_EDGEPALM to %d failed", value);
        break;
    default:
        FTS_ERROR("mode(%d) unsupport", mode);
        ret = -EINVAL;
        break;
    }

    return ret;
}

static ssize_t fts_glove_mode_show(
    struct device *dev, struct device_attribute *attr, char *buf)
{
    int count = 0;
    u8 reg_addr = FTS_REG_GLOVE_MODE_EN;
    u8 reg_val = 0;
    struct fts_ts_data *ts_data = dev_get_drvdata(dev);

    mutex_lock(&ts_data->input_dev->mutex);
    fts_read_reg(reg_addr, &reg_val);
    count = snprintf(buf + count, PAGE_SIZE, "Glove Mode:%s\n",
                     ts_data->glove_mode ? "On" : "Off");
    count += snprintf(buf + count, PAGE_SIZE, "Glove Reg:0x%02x,val:%d\n", reg_addr, reg_val);
    mutex_unlock(&ts_data->input_dev->mutex);

    return count;
}

static ssize_t fts_glove_mode_store(
    struct device *dev,
    struct device_attribute *attr, const char *buf, size_t count)
{
    struct fts_ts_data *ts_data = dev_get_drvdata(dev);

    mutex_lock(&ts_data->input_dev->mutex);
    if (FTS_SYSFS_ECHO_ON(buf)) {
        FTS_DEBUG("enter glove mode");
        ts_data->glove_mode = ENABLE;
        fts_ex_mode_switch(MODE_GLOVE, ENABLE);
    } else if (FTS_SYSFS_ECHO_OFF(buf)) {
        FTS_DEBUG("exit glove mode");
        ts_data->glove_mode = DISABLE;
        fts_ex_mode_switch(MODE_GLOVE, DISABLE);
    }
    mutex_unlock(&ts_data->input_dev->mutex);
    return count;
}


static ssize_t fts_cover_mode_show(
    struct device *dev, struct device_attribute *attr, char *buf)
{
    int count = 0;
    u8 reg_addr = FTS_REG_COVER_MODE_EN;
    u8 reg_val = 0;
    struct fts_ts_data *ts_data = dev_get_drvdata(dev);

    mutex_lock(&ts_data->input_dev->mutex);
    fts_read_reg(reg_addr, &reg_val);
    count = snprintf(buf + count, PAGE_SIZE, "Cover Mode:%s\n",
                     ts_data->cover_mode ? "On" : "Off");
    count += snprintf(buf + count, PAGE_SIZE, "Cover Reg:0x%02x,val:%d\n", reg_addr, reg_val);
    mutex_unlock(&ts_data->input_dev->mutex);

    return count;
}

static ssize_t fts_cover_mode_store(
    struct device *dev,
    struct device_attribute *attr, const char *buf, size_t count)
{
    struct fts_ts_data *ts_data = dev_get_drvdata(dev);

    mutex_lock(&ts_data->input_dev->mutex);
    if (FTS_SYSFS_ECHO_ON(buf)) {
        FTS_DEBUG("enter cover mode");
        ts_data->cover_mode = ENABLE;
        fts_ex_mode_switch(MODE_COVER, ENABLE);
    } else if (FTS_SYSFS_ECHO_OFF(buf)) {
        FTS_DEBUG("exit cover mode");
        ts_data->cover_mode = DISABLE;
        fts_ex_mode_switch(MODE_COVER, DISABLE);
    }
    mutex_unlock(&ts_data->input_dev->mutex);
    return count;
}

static ssize_t fts_charger_mode_show(
    struct device *dev, struct device_attribute *attr, char *buf)
{
    int count = 0;
    u8 reg_addr = FTS_REG_CHARGER_MODE_EN;
    u8 reg_val = 0;
    struct fts_ts_data *ts_data = dev_get_drvdata(dev);

    mutex_lock(&ts_data->input_dev->mutex);
    fts_read_reg(reg_addr, &reg_val);
    count = snprintf(buf + count, PAGE_SIZE, "Charger Mode:%s\n",
                     ts_data->charger_mode ? "On" : "Off");
    count += snprintf(buf + count, PAGE_SIZE, "Charger Reg:0x%02x,val:%d\n", reg_addr, reg_val);
    mutex_unlock(&ts_data->input_dev->mutex);

    return count;
}

static ssize_t fts_charger_mode_store(
    struct device *dev,
    struct device_attribute *attr, const char *buf, size_t count)
{
    struct fts_ts_data *ts_data = dev_get_drvdata(dev);

    mutex_lock(&ts_data->input_dev->mutex);
    if (FTS_SYSFS_ECHO_ON(buf)) {
        FTS_DEBUG("enter charger mode");
        ts_data->charger_mode = ENABLE;
        fts_ex_mode_switch(MODE_CHARGER, ENABLE);
    } else if (FTS_SYSFS_ECHO_OFF(buf)) {
        FTS_DEBUG("exit charger mode");
        ts_data->charger_mode = DISABLE;
        fts_ex_mode_switch(MODE_CHARGER, DISABLE);
    }
    mutex_unlock(&ts_data->input_dev->mutex);
    return count;
}

/* sysfs node: fts_earphone_mode */
static ssize_t fts_earphone_show(
    struct device *dev, struct device_attribute *attr, char *buf)
{
    int count = 0;
    u8 reg_addr = FTS_REG_EARPHONE_MODE_EN;
    u8 reg_val = 0;
    struct fts_ts_data *ts_data = dev_get_drvdata(dev);

    mutex_lock(&ts_data->input_dev->mutex);
    fts_read_reg(reg_addr, &reg_val);
    count = snprintf(buf + count, PAGE_SIZE, "Earphone Mode:%s\n",
                     ts_data->earphone_mode ? "On" : "Off");
    count += snprintf(buf + count, PAGE_SIZE, "Earphone Reg:0x%02x,val:%d\n", reg_addr, reg_val);
    mutex_unlock(&ts_data->input_dev->mutex);

    return count;
}

static ssize_t fts_earphone_store(
    struct device *dev,
    struct device_attribute *attr, const char *buf, size_t count)
{
    struct fts_ts_data *ts_data = dev_get_drvdata(dev);

    mutex_lock(&ts_data->input_dev->mutex);
    if (FTS_SYSFS_ECHO_ON(buf)) {
        FTS_DEBUG("enter earphone mode");
        ts_data->earphone_mode = ENABLE;
        fts_ex_mode_switch(MODE_EARPHONE, ENABLE);
    } else if (FTS_SYSFS_ECHO_OFF(buf)) {
        FTS_DEBUG("exit earphone mode");
        ts_data->earphone_mode = DISABLE;
        fts_ex_mode_switch(MODE_EARPHONE, DISABLE);
    }
    mutex_unlock(&ts_data->input_dev->mutex);
    return count;
}

/* sysfs node: fts_edgepalm_mode */
static ssize_t fts_edgepalm_show(
    struct device *dev, struct device_attribute *attr, char *buf)
{
    int count = 0;
    u8 reg_addr = FTS_REG_EDGEPALM_MODE_EN;
    u8 reg_val = 0;
    struct fts_ts_data *ts_data = dev_get_drvdata(dev);

    mutex_lock(&ts_data->input_dev->mutex);
    fts_read_reg(reg_addr, &reg_val);
    count = snprintf(buf + count, PAGE_SIZE, "Edgepalm Mode:%s,value:%d\n",
                     ts_data->edgepalm_mode ? "On" : "Off", ts_data->edgepalm_value);
    count += snprintf(buf + count, PAGE_SIZE, "Edgepalm Reg:0x%02x,val:%d\n", reg_addr, reg_val);
    mutex_unlock(&ts_data->input_dev->mutex);

    return count;
}

static ssize_t fts_edgepalm_store(
    struct device *dev,
    struct device_attribute *attr, const char *buf, size_t count)
{
    int value = 0;
    int n = 0;
    struct fts_ts_data *ts_data = dev_get_drvdata(dev);

    mutex_lock(&ts_data->input_dev->mutex);
    n = sscanf(buf, "%d", &value);
    if (n == 1) {
        ts_data->edgepalm_value = value;
        ts_data->edgepalm_mode = !!value;
        fts_ex_mode_switch(MODE_EDGEPALM, value);
    }
    mutex_unlock(&ts_data->input_dev->mutex);
    return count;
}


/* read and write charger mode
 * read example: cat fts_glove_mode        ---read  glove mode
 * write example:echo 1 > fts_glove_mode   ---write glove mode to 01
 */
static DEVICE_ATTR(fts_glove_mode, S_IRUGO | S_IWUSR, fts_glove_mode_show, fts_glove_mode_store);
static DEVICE_ATTR(fts_cover_mode, S_IRUGO | S_IWUSR, fts_cover_mode_show, fts_cover_mode_store);
static DEVICE_ATTR(fts_charger_mode, S_IRUGO | S_IWUSR, fts_charger_mode_show, fts_charger_mode_store);
static DEVICE_ATTR(fts_earphone_mode, S_IRUGO | S_IWUSR, fts_earphone_show, fts_earphone_store);
static DEVICE_ATTR(fts_edgepalm_mode, S_IRUGO | S_IWUSR, fts_edgepalm_show, fts_edgepalm_store);

static struct attribute *fts_touch_mode_attrs[] = {
    &dev_attr_fts_glove_mode.attr,
    &dev_attr_fts_cover_mode.attr,
    &dev_attr_fts_charger_mode.attr,
    &dev_attr_fts_earphone_mode.attr,
    &dev_attr_fts_edgepalm_mode.attr,
    NULL,
};

static struct attribute_group fts_touch_mode_group = {
    .attrs = fts_touch_mode_attrs,
};

int fts_ex_mode_recovery(struct fts_ts_data *ts_data)
{
    if (ts_data->glove_mode) {
        fts_ex_mode_switch(MODE_GLOVE, ENABLE);
    }

    if (ts_data->cover_mode) {
        fts_ex_mode_switch(MODE_COVER, ENABLE);
    }

    if (ts_data->charger_mode) {
        fts_ex_mode_switch(MODE_CHARGER, ENABLE);
    }

    if (ts_data->earphone_mode) {
        fts_ex_mode_switch(MODE_EARPHONE, ENABLE);
    }

    if (ts_data->edgepalm_mode) {
        fts_ex_mode_switch(MODE_EDGEPALM, ts_data->edgepalm_value);
    }

    return 0;
}

int fts_ex_mode_init(struct fts_ts_data *ts_data)
{
    int ret = 0;

    ts_data->glove_mode = DISABLE;
    ts_data->cover_mode = DISABLE;
    ts_data->charger_mode = DISABLE;
    ts_data->earphone_mode = DISABLE;
    ts_data->edgepalm_mode = DISABLE;

    ret = sysfs_create_group(&ts_data->dev->kobj, &fts_touch_mode_group);
    if (ret < 0) {
        FTS_ERROR("create sysfs(ex_mode) fail");
        sysfs_remove_group(&ts_data->dev->kobj, &fts_touch_mode_group);
        return ret;
    } else {
        FTS_DEBUG("create sysfs(ex_mode) successfully");
    }

    return 0;
}

int fts_ex_mode_exit(struct fts_ts_data *ts_data)
{
    sysfs_remove_group(&ts_data->dev->kobj, &fts_touch_mode_group);
    return 0;
}
