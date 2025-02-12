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
* File Name: focaltech_core.c
*
* Author: Focaltech Driver Team
*
* Created: 2016-08-08
*
* Abstract: entrance for focaltech ts driver
*
* Version: V1.0
*
*****************************************************************************/

/*****************************************************************************
* Included header files
*****************************************************************************/
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#include <linux/of_irq.h>
#ifdef CONFIG_SOC_KY_X1
#include <soc/ky/ky_panel.h>
#endif

#if IS_ENABLED(CONFIG_DRM)
#if IS_ENABLED(CONFIG_DRM_PANEL)
#include <drm/drm_panel.h>
#else
#include <linux/msm_drm_notify.h>
#endif //CONFIG_DRM_PANEL

#elif IS_ENABLED(CONFIG_FB)
#include <linux/notifier.h>
#include <linux/fb.h>
#endif //CONFIG_DRM
#include "focaltech_core.h"

/*****************************************************************************
* Private constant and macro definitions using #define
*****************************************************************************/
#define FTS_DRIVER_PEN_NAME                 "fts_ts,pen"

#if FTS_POWER_SOURCE_CUST_EN
#define FTS_VTG_MIN_UV                      2800000
#define FTS_VTG_MAX_UV                      3300000
#define FTS_IOVCC_VTG_MIN_UV                1800000
#define FTS_IOVCC_VTG_MAX_UV                1800000
#endif

#define FTS_WAKELOCK_TIMEOUT                5000

/*****************************************************************************
* Global variable or extern global variabls/functions
*****************************************************************************/
struct fts_ts_data *fts_data;

#if FTS_FOD_EN
static int fts_fod_recovery(struct fts_ts_data *ts_data);
#endif

/*****************************************************************************
* Static function prototypes
*****************************************************************************/
void fts_msleep(unsigned long msecs)
{
    if (msecs > 20) {
        msleep(msecs);
    } else if (msecs > 0) {
        usleep_range(msecs * 1000, (msecs + 2) * 1000);
    }
}

int fts_check_cid(struct fts_ts_data *ts_data, u8 id_h)
{
    int i = 0;
    struct ft_chip_id_t *cid = &ts_data->ic_info.cid;
    u8 cid_h = 0x0;

    if (cid->type == 0)
        return -ENODATA;

    for (i = 0; i < FTS_MAX_CHIP_IDS; i++) {
        cid_h = ((cid->chip_ids[i] >> 8) & 0x00FF);
        if (cid_h && (id_h == cid_h)) {
            return 0;
        }
    }

    return -ENODATA;
}

/*****************************************************************************
*  Name: fts_wait_tp_to_valid
*  Brief: Read chip id until TP FW become valid(Timeout: TIMEOUT_READ_REG),
*         need call when reset/power on/resume...
*  Input:
*  Output:
*  Return: return 0 if tp valid, otherwise return error code
*****************************************************************************/
int fts_wait_tp_to_valid(void)
{
    int ret = 0;
    int i = 0;
    u8 idh = 0xFF;
    struct fts_ts_data *ts_data = fts_data;
    u8 chip_idh = ts_data->ic_info.ids.chip_idh;

    for (i = 0; i < FTS_MAX_RETRIES_READID; i++) {
        ret = fts_read_reg(FTS_REG_CHIP_ID, &idh);
        if ((idh == chip_idh) || (fts_check_cid(ts_data, idh) == 0)) {
            FTS_INFO("TP Ready,Device ID:0x%02x", idh);
            return 0;
        }
        if ((i + 1) < FTS_MAX_RETRIES_READID) fts_msleep((i + 1) * 20);
    }

    FTS_ERROR("TP Not Ready,ReadData:0x%02x,ret:%d", idh, ret);
    return -EIO;
}

/*****************************************************************************
*  Name: fts_tp_state_recovery
*  Brief: Need execute this function when reset
*  Input:
*  Output:
*  Return:
*****************************************************************************/
void fts_tp_state_recovery(struct fts_ts_data *ts_data)
{
    fts_wait_tp_to_valid();
    fts_ex_mode_recovery(ts_data);
#if FTS_PSENSOR_EN
    if (ts_data->proximity_mode) {
        fts_proximity_recovery(ts_data);
        return;
    }
#endif
#if FTS_FOD_EN
    fts_fod_recovery(ts_data);
#endif
    fts_gesture_recovery(ts_data);
}

static int fts_reset_pre(struct fts_ts_data *ts_data, int value)
{
    u16 ic_type = ts_data->ic_info.ids.type;
    if (!value) {
        if ((ic_type == 0x90) || (ic_type == 0x92) || (ic_type == 0x93)) {
            if (!ts_data->power_disabled) {
                FTS_DEBUG("write regb6");
                fts_write_reg(0xB6, 1);
                fts_msleep(20);
            }
        }
    }
    return 0;
}

static int fts_reset_post(struct fts_ts_data *ts_data, int value)
{

    return 0;
}

int fts_set_reset(struct fts_ts_data *ts_data, int value)
{
    int ret = 0;

    FTS_INFO("set reset to %d", !!value);
    fts_reset_pre(ts_data, value);
    ret = gpio_direction_output(ts_data->pdata->reset_gpio, !!value);
    if (ret) {
        FTS_ERROR("[GPIO]set reset gpio to %d failed", !!value);
        return ret;
    }
    fts_reset_post(ts_data, value);
    return 0;
}

int fts_reset_proc(struct fts_ts_data *ts_data, int force, int hdelayms)
{
    if (force || (!ts_data->fw_loading)) {
        fts_set_reset(ts_data, 0);
        fts_msleep(2);
        fts_set_reset(ts_data, 1);
        if (hdelayms) fts_msleep(hdelayms);
    } else {
        FTS_INFO("fw upgrade in process, no reset");
    }

    return 0;
}

void fts_irq_disable(void)
{
    unsigned long irqflags;

    FTS_FUNC_ENTER();
    spin_lock_irqsave(&fts_data->irq_lock, irqflags);

    if (!fts_data->irq_disabled) {
        disable_irq_nosync(fts_data->irq);
        fts_data->irq_disabled = true;
    }

    spin_unlock_irqrestore(&fts_data->irq_lock, irqflags);
    FTS_FUNC_EXIT();
}

void fts_irq_enable(void)
{
    unsigned long irqflags = 0;

    FTS_FUNC_ENTER();
    spin_lock_irqsave(&fts_data->irq_lock, irqflags);

    if (fts_data->irq_disabled) {
        enable_irq(fts_data->irq);
        fts_data->irq_disabled = false;
    }

    spin_unlock_irqrestore(&fts_data->irq_lock, irqflags);
    FTS_FUNC_EXIT();
}

int fts_hid2std(int mode)
{
    int ret = 0;
    u8 buf[3] = {0xEB, 0xAA, 0x09};
    u8 val[3] = { 0 };

    if (fts_data->bus_type != BUS_TYPE_I2C)
        return 0;

    if (mode == 1) {
        /* Don't need delay */
        ret = fts_read(buf, 3, val, 3);
        if (ret < 0) {
            FTS_ERROR("send hid2std cmd failed");
            return ret;
        }
    } else {
        ret = fts_write(buf, 3);
        if (ret < 0) {
            FTS_ERROR("hid2std cmd write fail");
            return ret;
        }

        fts_msleep(10);
        ret = fts_read(NULL, 0, val, 3);
        if (ret < 0) {
            FTS_ERROR("hid2std cmd read fail");
            return ret;
        }
    }

    if ((0xEB == val[0]) && (0xAA == val[1]) && (0x08 == val[2])) {
        FTS_INFO("hidi2c change to stdi2c successful");
    } else {
        FTS_INFO("hidi2c change to stdi2c not support or fail");
    }
    return 0;
}

static int fts_match_cid(struct fts_ts_data *ts_data,
                         u16 type, u8 id_h, u8 id_l, bool force)
{
#ifdef FTS_CHIP_ID_MAPPING
    u32 i = 0;
    u32 j = 0;
    struct ft_chip_id_t chip_id_list[] = FTS_CHIP_ID_MAPPING;
    u32 cid_entries = sizeof(chip_id_list) / sizeof(struct ft_chip_id_t);
    u16 id = (id_h << 8) + id_l;

    memset(&ts_data->ic_info.cid, 0, sizeof(struct ft_chip_id_t));
    for (i = 0; i < cid_entries; i++) {
        if (!force && (type == chip_id_list[i].type)) {
            break;
        } else if (force && (type == chip_id_list[i].type)) {
            FTS_INFO("match cid,type:0x%x", (int)chip_id_list[i].type);
            ts_data->ic_info.cid = chip_id_list[i];
            return 0;
        }
    }

    if (i >= cid_entries) {
        return -ENODATA;
    }

    for (j = 0; j < FTS_MAX_CHIP_IDS; j++) {
        if (id == chip_id_list[i].chip_ids[j]) {
            FTS_DEBUG("cid:%x==%x", id, chip_id_list[i].chip_ids[j]);
            FTS_INFO("match cid,type:0x%x", (int)chip_id_list[i].type);
            ts_data->ic_info.cid = chip_id_list[i];
            return 0;
        }
    }

    return -ENODATA;
#else
    return -EINVAL;
#endif
}


static int fts_get_chip_types(
    struct fts_ts_data *ts_data,
    u8 id_h, u8 id_l, bool fw_valid)
{
    u32 i = 0;
    struct ft_chip_t ctype[] = FTS_CHIP_TYPE_MAPPING;
    u32 ctype_entries = sizeof(ctype) / sizeof(struct ft_chip_t);

    if ((0x0 == id_h) || (0x0 == id_l)) {
        FTS_ERROR("id_h/id_l is 0");
        return -EINVAL;
    }

    FTS_INFO("verify id:0x%02x%02x", id_h, id_l);
    for (i = 0; i < ctype_entries; i++) {
        if (VALID == fw_valid) {
            if (((id_h == ctype[i].chip_idh) && (id_l == ctype[i].chip_idl))
                || (!fts_match_cid(ts_data, ctype[i].type, id_h, id_l, 0)))
                break;
        } else {
            if (((id_h == ctype[i].rom_idh) && (id_l == ctype[i].rom_idl))
                || ((id_h == ctype[i].pb_idh) && (id_l == ctype[i].pb_idl))
                || ((id_h == ctype[i].bl_idh) && (id_l == ctype[i].bl_idl))) {
                break;
            }
        }
    }

    if (i >= ctype_entries) {
        return -ENODATA;
    }

    fts_match_cid(ts_data, ctype[i].type, id_h, id_l, 1);
    ts_data->ic_info.ids = ctype[i];
    return 0;
}

static int fts_read_bootid(struct fts_ts_data *ts_data, u8 *id)
{
    int ret = 0;
    u8 chip_id[2] = { 0 };
    u8 id_cmd[4] = { 0 };
    u32 id_cmd_len = 0;

    id_cmd[0] = FTS_CMD_START1;
    id_cmd[1] = FTS_CMD_START2;
    ret = fts_write(id_cmd, 2);
    if (ret < 0) {
        FTS_ERROR("start cmd write fail");
        return ret;
    }

    fts_msleep(FTS_CMD_START_DELAY);
    id_cmd[0] = FTS_CMD_READ_ID;
    id_cmd[1] = id_cmd[2] = id_cmd[3] = 0x00;
    if (ts_data->ic_info.is_incell)
        id_cmd_len = FTS_CMD_READ_ID_LEN_INCELL;
    else
        id_cmd_len = FTS_CMD_READ_ID_LEN;
    ret = fts_read(id_cmd, id_cmd_len, chip_id, 2);
    if ((ret < 0) || (0x0 == chip_id[0]) || (0x0 == chip_id[1])) {
        FTS_ERROR("read boot id fail,read:0x%02x%02x", chip_id[0], chip_id[1]);
        return -EIO;
    }

    id[0] = chip_id[0];
    id[1] = chip_id[1];
    return 0;
}

/*****************************************************************************
* Name: fts_get_ic_information
* Brief: read chip id to get ic information, after run the function, driver w-
*        ill know which IC is it.
*        If cant get the ic information, maybe not focaltech's touch IC, need
*        unregister the driver
* Input:
* Output:
* Return: return 0 if get correct ic information, otherwise return error code
*****************************************************************************/
static int fts_get_ic_information(struct fts_ts_data *ts_data)
{
    int ret = 0;
    int cnt = 0;
    u8 chip_id[2] = { 0 };

    ts_data->ic_info.is_incell = FTS_CHIP_IDC;
    ts_data->ic_info.hid_supported = FTS_HID_SUPPORTTED;


    do {
        ret = fts_read_reg(FTS_REG_CHIP_ID, &chip_id[0]);
        ret = fts_read_reg(FTS_REG_CHIP_ID2, &chip_id[1]);
        if ((ret < 0) || (0x0 == chip_id[0]) || (0x0 == chip_id[1])) {
            FTS_INFO("chip id read invalid, read:0x%02x%02x",
                     chip_id[0], chip_id[1]);
        } else {
            ret = fts_get_chip_types(ts_data, chip_id[0], chip_id[1], VALID);
            if (!ret)
                break;
            else
                FTS_INFO("TP not ready, read:0x%02x%02x",
                         chip_id[0], chip_id[1]);
        }

        cnt++;
        fts_msleep(INTERVAL_READ_REG);
    } while ((cnt * INTERVAL_READ_REG) < TIMEOUT_READ_REG);

    if ((cnt * INTERVAL_READ_REG) >= TIMEOUT_READ_REG) {
        FTS_INFO("fw is invalid, need read boot id");
        for (cnt = 0; cnt < FTS_MAX_RETRIES_READ_BOOTID; cnt++) {
            if (cnt < 2) {
                if (ts_data->ic_info.hid_supported) fts_hid2std(0);
            } else {
                fts_reset_proc(ts_data, true, 0);
                mdelay(FTS_CMD_START_DELAY + (cnt - 2) * 8);
                if (ts_data->ic_info.hid_supported) {
                    fts_hid2std(1);
                    fts_write_reg(0x55, 0xAA);
                    fts_msleep(FTS_CMD_START_DELAY);
                    fts_hid2std(1);
                }
            }

            ret = fts_read_bootid(ts_data, &chip_id[0]);
            if (ret <  0) {
                FTS_ERROR("read boot id fail");
                continue;
            }

            ret = fts_get_chip_types(ts_data, chip_id[0], chip_id[1], INVALID);
            if (ret < 0) {
                FTS_ERROR("can't get ic informaton");
                continue;
            }
            break;
        }
    }

    FTS_INFO("get ic information, chip id = 0x%02x%02x(cid type=0x%x)",
             ts_data->ic_info.ids.chip_idh, ts_data->ic_info.ids.chip_idl,
             ts_data->ic_info.cid.type);

    return ret;
}

#if FTS_READ_CUSTOMER_INFO
static int fts_read_customer_information(struct fts_ts_data *ts_data)
{
    /* If some customer's code had written to touch chip, please do the followings:
     *  Step 1: read customer information here.
     *  Step 2: save customer information to ts_data->customer_info variable, maximum 32.
     *  Step 3: return ts_data->customer_info to APP via sysfs node.
     *
     * Warning: please check the information is read from FW or not, if from FW, please
     *          take care that FW maybe isn't valid.
     */


    FTS_INFO("customer info:%s", ts_data->customer_info);
    return 0;
}
#endif

#if FTS_FOD_EN
static void fts_fod_set_reg(int value)
{
    int i = 0;
    u8 fod_val = value ? FTS_VAL_FOD_ENABLE : DISABLE;
    u8 regval = 0xFF;

    for (i = 0; i < FTS_MAX_RETRIES_WRITEREG; i++) {
        fts_read_reg(FTS_REG_FOD_MODE_EN, &regval);
        if (regval == fod_val)
            break;
        fts_write_reg(FTS_REG_FOD_MODE_EN, fod_val);
        fts_msleep(1);
    }

    if (i >= FTS_MAX_RETRIES_WRITEREG)
        FTS_ERROR("set fod mode to %x failed,reg_val:%x", fod_val, regval);
    else if (i > 0)
        FTS_INFO("set fod mode to %x successfully", fod_val);
}

void fts_fod_enable(int enable)
{
    struct fts_ts_data *ts_data = fts_data;

    ts_data->fod_fp_down = false;
    if (enable) {
        FTS_INFO("Fod enable");
        ts_data->fod_mode = ENABLE;
        fts_fod_set_reg(FTS_VAL_FOD_ENABLE);
    } else {
        FTS_INFO("Fod disable");
        ts_data->fod_mode = DISABLE;
        fts_fod_set_reg(DISABLE);
    }
}

/*****************************************************************************
* Name: fts_fod_readdata
* Brief: read fod value from TP, check whether having FOD event or not,
*        and report the state to host if need.
*
* Input: ts_data
* Output:
* Return: return negative code if error occurs,return 0 or 1 if success.
*         return 0 if continue report finger touches.
*         return 1(FTS_RETVAL_IGNORE_TOUCHES) if you want to ingore this
*         finger reporting, As default, the following situation will report 1:
*               a.System in suspend state, now not handle gesture.
*****************************************************************************/
static int fts_fod_readdata(struct fts_ts_data *ts_data)
{
    int ret = 0;
    int fod_x = 0;
    int fod_y = 0;
    int fod_pointid = 0;
    int fod_down = 0;
    u8 fod_val[FTS_FOD_BUF_LEN] = { 0 };
    u8 fod_cmd = FTS_REG_FOD_DATA;

    ret = fts_read(&fod_cmd, 1, fod_val, FTS_FOD_BUF_LEN);
    if (ret < 0) {
        FTS_ERROR("read fod data failed,ret=%d", ret);
        return ret;
    }

    if (fod_val[1] == 0x26) {
        fod_pointid = fod_val[0];
        fod_x = (fod_val[4] << 8) + fod_val[5];
        fod_y = (fod_val[6] << 8) + fod_val[7];
        fod_down = (fod_val[8] == 0) ? 1 : 0;
        FTS_DEBUG("FOD data:%x %x %x %x[%x,%x][%x]", fod_val[0], fod_val[1],
                  fod_val[2], fod_val[3], fod_x, fod_x, fod_val[8]);
        if (fod_down) {
            /* FOD down, need do something to tell host */
            ts_data->fod_fp_down = true;
        } else {
            /* FOD up, need do something to tell host */
            ts_data->fod_fp_down = false;
        }

        ret = (ts_data->suspended) ? FTS_RETVAL_IGNORE_TOUCHES : 0;
    } else {
        ret = 0;
    }

    return ret;
}

static int fts_fod_recovery(struct fts_ts_data *ts_data)
{
    if (ts_data->fod_mode) {
        fts_fod_set_reg(FTS_VAL_FOD_ENABLE);
    }
    return 0;
}

/*****************************************************************************
* Name: fts_fod_checkdown
* Brief: check fod down event is triggered, it's used to reset TP or not when
*        resuming.
*
* Input: ts_data
* Output:
* Return: return 1 if having fod down event, or else return 0
*****************************************************************************/
static int fts_fod_checkdown(struct fts_ts_data *ts_data)
{
    return (ts_data->fod_mode && ts_data->fod_fp_down);
}

static int fts_fod_suspend(struct fts_ts_data *ts_data)
{
    ts_data->fod_fp_down = false;
    fts_fod_set_reg(FTS_VAL_FOD_ENABLE);
    return 0;
}

static int fts_fod_resume(struct fts_ts_data *ts_data)
{
    if (!fts_fod_checkdown(ts_data)) fts_fod_set_reg(FTS_VAL_FOD_ENABLE);
    ts_data->fod_fp_down = false;
    return 0;
}
#endif

/*****************************************************************************
*  Reprot related
*****************************************************************************/
static void fts_show_touch_buffer(u8 *data, u32 datalen)
{
    u32 i = 0;
    u32 count = 0;
    char *tmpbuf = NULL;

    tmpbuf = kzalloc(1024, GFP_KERNEL);
    if (!tmpbuf) {
        FTS_ERROR("tmpbuf zalloc fail");
        return;
    }

    for (i = 0; i < datalen; i++) {
        count += snprintf(tmpbuf + count, 1024 - count, "%02X,", data[i]);
        if (count >= 1024)
            break;
    }
    FTS_DEBUG("touch_buf:%s", tmpbuf);

    if (tmpbuf) {
        kfree(tmpbuf);
        tmpbuf = NULL;
    }
}

void fts_release_all_finger(void)
{
    struct fts_ts_data *ts_data = fts_data;
    struct input_dev *input_dev = ts_data->input_dev;
#if FTS_MT_PROTOCOL_B_EN
    u32 finger_count = 0;
    u32 max_touches = ts_data->pdata->max_touch_number;
#endif

    mutex_lock(&ts_data->report_mutex);
#if FTS_MT_PROTOCOL_B_EN
    for (finger_count = 0; finger_count < max_touches; finger_count++) {
        input_mt_slot(input_dev, finger_count);
        input_mt_report_slot_state(input_dev, MT_TOOL_FINGER, false);
    }
#else
    input_mt_sync(input_dev);
#endif
    input_report_key(input_dev, BTN_TOUCH, 0);
    input_sync(input_dev);

#if FTS_PEN_EN
    input_report_key(ts_data->pen_dev, BTN_TOOL_PEN, 0);
    input_report_key(ts_data->pen_dev, BTN_TOUCH, 0);
    input_sync(ts_data->pen_dev);
#endif

    ts_data->touch_points = 0;
    ts_data->key_state = 0;
    mutex_unlock(&ts_data->report_mutex);
}

/*****************************************************************************
* Name: fts_input_report_key
* Brief: process key events,need report key-event if key enable.
*        if point's coordinate is in (x_dim-50,y_dim-50) ~ (x_dim+50,y_dim+50),
*        need report it to key event.
*        x_dim: parse from dts, means key x_coordinate, dimension:+-50
*        y_dim: parse from dts, means key y_coordinate, dimension:+-50
* Input:
* Output:
* Return: return 0 if it's key event, otherwise return error code
*****************************************************************************/
static int fts_input_report_key(struct fts_ts_data *ts_data, struct ts_event *kevent)
{
    int i = 0;
    int x = kevent->x;
    int y = kevent->y;
    int *x_dim = &ts_data->pdata->key_x_coords[0];
    int *y_dim = &ts_data->pdata->key_y_coords[0];

    if (!ts_data->pdata->have_key) {
        return -EINVAL;
    }
    for (i = 0; i < ts_data->pdata->key_number; i++) {
        if ((x >= x_dim[i] - FTS_KEY_DIM) && (x <= x_dim[i] + FTS_KEY_DIM) &&
            (y >= y_dim[i] - FTS_KEY_DIM) && (y <= y_dim[i] + FTS_KEY_DIM)) {
            if (EVENT_DOWN(kevent->flag)
                && !(ts_data->key_state & (1 << i))) {
                input_report_key(ts_data->input_dev, ts_data->pdata->keys[i], 1);
                ts_data->key_state |= (1 << i);
                FTS_DEBUG("Key%d(%d,%d) DOWN!", i, x, y);
            } else if (EVENT_UP(kevent->flag)
                       && (ts_data->key_state & (1 << i))) {
                input_report_key(ts_data->input_dev, ts_data->pdata->keys[i], 0);
                ts_data->key_state &= ~(1 << i);
                FTS_DEBUG("Key%d(%d,%d) Up!", i, x, y);
            }
            return 0;
        }
    }
    return -EINVAL;
}

#if FTS_MT_PROTOCOL_B_EN
static int fts_input_report_b(struct fts_ts_data *ts_data, struct ts_event *events)
{
    int i = 0;
    int touch_down_point_cur = 0;
    int touch_point_pre = ts_data->touch_points;
    u32 max_touch_num = ts_data->pdata->max_touch_number;
    bool touch_event_coordinate = false;
    struct input_dev *input_dev = ts_data->input_dev;

    for (i = 0; i < ts_data->touch_event_num; i++) {
        if (fts_input_report_key(ts_data, &events[i]) == 0) {
            continue;
        }

        touch_event_coordinate = true;
        if (EVENT_DOWN(events[i].flag)) {
            input_mt_slot(input_dev, events[i].id);
            input_mt_report_slot_state(input_dev, MT_TOOL_FINGER, true);
#if FTS_REPORT_PRESSURE_EN
            input_report_abs(input_dev, ABS_MT_PRESSURE, events[i].p);
#endif
            input_report_abs(input_dev, ABS_MT_TOUCH_MAJOR, events[i].area);
            input_report_abs(input_dev, ABS_MT_TOUCH_MINOR, events[i].minor);

            input_report_abs(input_dev, ABS_MT_POSITION_X, events[i].x);
            input_report_abs(input_dev, ABS_MT_POSITION_Y, events[i].y);

            touch_down_point_cur |= (1 << events[i].id);
            touch_point_pre |= (1 << events[i].id);

            if ((ts_data->log_level >= 2) ||
                ((1 == ts_data->log_level) && (FTS_TOUCH_DOWN == events[i].flag))) {
                FTS_DEBUG("[B]P%d(%d, %d)[p:%d,tm:%d] DOWN!",
                          events[i].id, events[i].x, events[i].y,
                          events[i].p, events[i].area);
            }
        } else {
            input_mt_slot(input_dev, events[i].id);
            input_mt_report_slot_state(input_dev, MT_TOOL_FINGER, false);
            touch_point_pre &= ~(1 << events[i].id);
            if (ts_data->log_level >= 1) FTS_DEBUG("[B]P%d UP!", events[i].id);
        }
    }

    if (unlikely(touch_point_pre ^ touch_down_point_cur)) {
        for (i = 0; i < max_touch_num; i++)  {
            if ((1 << i) & (touch_point_pre ^ touch_down_point_cur)) {
                if (ts_data->log_level >= 1) FTS_DEBUG("[B]P%d UP!", i);
                input_mt_slot(input_dev, i);
                input_mt_report_slot_state(input_dev, MT_TOOL_FINGER, false);
            }
        }
    }

    if (touch_down_point_cur)
        input_report_key(input_dev, BTN_TOUCH, 1);
    else if (touch_event_coordinate || ts_data->touch_points) {
        if (ts_data->touch_points && (ts_data->log_level >= 1))
            FTS_DEBUG("[B]Points All Up!");
        input_report_key(input_dev, BTN_TOUCH, 0);
    }

    ts_data->touch_points = touch_down_point_cur;
    input_sync(input_dev);
    return 0;
}
#else
static int fts_input_report_a(struct fts_ts_data *ts_data, struct ts_event *events)
{
    int i = 0;
    int touch_down_point_num_cur = 0;
    bool touch_event_coordinate = false;
    struct input_dev *input_dev = ts_data->input_dev;

    for (i = 0; i < ts_data->touch_event_num; i++) {
        if (fts_input_report_key(ts_data, &events[i]) == 0) {
            continue;
        }

        touch_event_coordinate = true;
        if (EVENT_DOWN(events[i].flag)) {
            input_report_abs(input_dev, ABS_MT_TRACKING_ID, events[i].id);
#if FTS_REPORT_PRESSURE_EN
            input_report_abs(input_dev, ABS_MT_PRESSURE, events[i].p);
#endif
            input_report_abs(input_dev, ABS_MT_TOUCH_MAJOR, events[i].area);
            input_report_abs(input_dev, ABS_MT_TOUCH_MINOR, events[i].minor);

            input_report_abs(input_dev, ABS_MT_POSITION_X, events[i].x);
            input_report_abs(input_dev, ABS_MT_POSITION_Y, events[i].y);
            input_mt_sync(input_dev);

            touch_down_point_num_cur++;
            if ((ts_data->log_level >= 2) ||
                ((1 == ts_data->log_level) && (FTS_TOUCH_DOWN == events[i].flag))) {
                FTS_DEBUG("[A]P%d(%d, %d)[p:%d,tm:%d] DOWN!",
                          events[i].id, events[i].x, events[i].y,
                          events[i].p, events[i].area);
            }
        }
    }

    if (touch_down_point_num_cur)
        input_report_key(input_dev, BTN_TOUCH, 1);
    else if (touch_event_coordinate || ts_data->touch_points) {
        if (ts_data->touch_points && (ts_data->log_level >= 1))
            FTS_DEBUG("[A]Points All Up!");
        input_report_key(input_dev, BTN_TOUCH, 0);
        input_mt_sync(input_dev);
    }

    ts_data->touch_points = touch_down_point_num_cur;
    input_sync(input_dev);
    return 0;
}
#endif

#if FTS_PEN_EN
static int fts_input_pen_report(struct fts_ts_data *ts_data, u8 *pen_buf)
{
    struct input_dev *pen_dev = ts_data->pen_dev;
    struct pen_event *pevt = &ts_data->pevent;

    /*get information of stylus*/
    pevt->inrange = (pen_buf[2] & 0x20) ? 1 : 0;
    pevt->tip = (pen_buf[2] & 0x01) ? 1 : 0;
    pevt->flag = pen_buf[3] >> 6;
#if FTS_PEN_HIRES_EN
    pevt->id = 0;
    pevt->x = ((u32)((pen_buf[3] & 0x0F) << 12) + (pen_buf[4] << 4) + ((pen_buf[5] >> 4) & 0x0F));
    pevt->y = ((u32)((pen_buf[5] & 0x0F) << 12) + (pen_buf[6] << 4) + ((pen_buf[7] >> 4) & 0x0F));
    pevt->x = (pevt->x * FTS_PEN_HIRES_X ) / FTS_HI_RES_X_MAX;
    pevt->y = (pevt->y * FTS_PEN_HIRES_X ) / FTS_HI_RES_X_MAX;
#else
    pevt->id = pen_buf[5] >> 4;
    pevt->x = ((pen_buf[3] & 0x0F) << 8) + pen_buf[4];
    pevt->y = ((pen_buf[5] & 0x0F) << 8) + pen_buf[6];
#endif
    pevt->p = ((pen_buf[7] & 0x0F) << 8) + pen_buf[8];
    pevt->tilt_x = (short)((pen_buf[9] << 8) + pen_buf[10]);
    pevt->tilt_y = (short)((pen_buf[11] << 8) + pen_buf[12]);
    pevt->azimuth = ((pen_buf[13] << 8) + pen_buf[14]);
    pevt->tool_type = BTN_TOOL_PEN;

    input_report_key(pen_dev, BTN_STYLUS, !!(pen_buf[2] & 0x02));
    input_report_key(pen_dev, BTN_STYLUS2, !!(pen_buf[2] & 0x08));

    switch (ts_data->pen_etype) {
    case STYLUS_DEFAULT:
        if (pevt->tip && pevt->p) {
            if ((ts_data->log_level >= 2) || (!pevt->down))
                FTS_DEBUG("[PEN]x:%d,y:%d,p:%d,tip:%d,flag:%d,tilt:%d,%d DOWN",
                          pevt->x, pevt->y, pevt->p, pevt->tip, pevt->flag,
                          pevt->tilt_x, pevt->tilt_y);
            input_report_abs(pen_dev, ABS_X, pevt->x);
            input_report_abs(pen_dev, ABS_Y, pevt->y);
            input_report_abs(pen_dev, ABS_PRESSURE, pevt->p);
            input_report_abs(pen_dev, ABS_TILT_X, pevt->tilt_x);
            input_report_abs(pen_dev, ABS_TILT_Y, pevt->tilt_y);
            input_report_key(pen_dev, BTN_TOUCH, 1);
            input_report_key(pen_dev, BTN_TOOL_PEN, 1);
            pevt->down = 1;
        } else if (!pevt->tip && pevt->down) {
            FTS_DEBUG("[PEN]x:%d,y:%d,p:%d,tip:%d,flag:%d,tilt:%d,%d UP",
                      pevt->x, pevt->y, pevt->p, pevt->tip, pevt->flag,
                      pevt->tilt_x, pevt->tilt_y);
            input_report_abs(pen_dev, ABS_X, pevt->x);
            input_report_abs(pen_dev, ABS_Y, pevt->y);
            input_report_abs(pen_dev, ABS_PRESSURE, pevt->p);
            input_report_key(pen_dev, BTN_TOUCH, 0);
            input_report_key(pen_dev, BTN_TOOL_PEN, 0);
            pevt->down = 0;
        }
        input_sync(pen_dev);
        break;
    case STYLUS_HOVER:
        if (ts_data->log_level >= 1)
            FTS_DEBUG("[PEN][%02X]x:%d,y:%d,p:%d,tip:%d,flag:%d,tilt:%d,%d,%d",
                      pen_buf[2], pevt->x, pevt->y, pevt->p, pevt->tip,
                      pevt->flag, pevt->tilt_x, pevt->tilt_y, pevt->azimuth);
        input_report_abs(pen_dev, ABS_X, pevt->x);
        input_report_abs(pen_dev, ABS_Y, pevt->y);
        input_report_abs(pen_dev, ABS_Z, pevt->azimuth);
        input_report_abs(pen_dev, ABS_PRESSURE, pevt->p);
        input_report_abs(pen_dev, ABS_TILT_X, pevt->tilt_x);
        input_report_abs(pen_dev, ABS_TILT_Y, pevt->tilt_y);
        input_report_key(pen_dev, BTN_TOOL_PEN, EVENT_DOWN(pevt->flag));
        input_report_key(pen_dev, BTN_TOUCH, pevt->tip);
        input_sync(pen_dev);
        break;
    default:
        FTS_ERROR("Unknown stylus event");
        break;
    }

    return 0;
}
#endif

static int fts_input_report_touch(struct fts_ts_data *ts_data, u8 *touch_buf)
{
    int i = 0;
    int event_num = 0;
    int finger_num = 0;
    int pointid = 0;
    int base = 0;
    int max_touch_num = ts_data->pdata->max_touch_number;
    struct ts_event *events = ts_data->events;

    finger_num = touch_buf[FTS_TOUCH_E_NUM] & 0x0F;
    if (finger_num > max_touch_num) {
        FTS_ERROR("invalid point_num(%d)", finger_num);
        return -EIO;
    }

    for (i = 0; i < max_touch_num; i++) {
        base = FTS_ONE_TCH_LEN * i + 2;
        pointid = (touch_buf[FTS_TOUCH_OFF_ID_YH + base]) >> 4;
        if (pointid >= FTS_MAX_ID)
            break;
        else if (pointid >= max_touch_num) {
            FTS_ERROR("ID(%d) beyond max_touch_number", pointid);
            return -EINVAL;
        }

        events[i].id = pointid;
        events[i].flag = touch_buf[FTS_TOUCH_OFF_E_XH + base] >> 6;
#if FTS_TOUCH_HIRES_EN
        events[i].x = ((touch_buf[FTS_TOUCH_OFF_E_XH + base] & 0x0F) << 12) \
                      + ((touch_buf[FTS_TOUCH_OFF_XL + base] & 0xFF) << 4) \
                      + ((touch_buf[FTS_TOUCH_OFF_PRE + base] >> 4) & 0x0F);
        events[i].y = ((touch_buf[FTS_TOUCH_OFF_ID_YH + base] & 0x0F) << 12) \
                      + ((touch_buf[FTS_TOUCH_OFF_YL + base] & 0xFF) << 4) \
                      + (touch_buf[FTS_TOUCH_OFF_PRE + base] & 0x0F);
        events[i].x = (events[i].x * FTS_TOUCH_HIRES_X ) / FTS_HI_RES_X_MAX;
        events[i].y = (events[i].y * FTS_TOUCH_HIRES_X ) / FTS_HI_RES_X_MAX;
        events[i].p = 0x3F;
#if FTS_REPORT_PRESSURE_EN
        FTS_ERROR("high solution project doesn't support pressure property");
#endif
#else
        events[i].x = ((touch_buf[FTS_TOUCH_OFF_E_XH + base] & 0x0F) << 8) \
                      + (touch_buf[FTS_TOUCH_OFF_XL + base] & 0xFF);
        events[i].y = ((touch_buf[FTS_TOUCH_OFF_ID_YH + base] & 0x0F) << 8) \
                      + (touch_buf[FTS_TOUCH_OFF_YL + base] & 0xFF);
        events[i].p =  touch_buf[FTS_TOUCH_OFF_PRE + base];
        if (events[i].p <= 0) events[i].p = 0x3F;
#endif
        events[i].area = touch_buf[FTS_TOUCH_OFF_AREA + base];
        if (events[i].area <= 0) events[i].area = 0x09;
        events[i].minor = events[i].area;

        event_num++;
        if (EVENT_DOWN(events[i].flag) && (finger_num == 0)) {
            FTS_INFO("abnormal touch data from fw");
            return -EIO;
        }
    }

    if (event_num == 0) {
        FTS_INFO("no touch point information(%02x)", touch_buf[2]);
        return -EIO;
    }
    ts_data->touch_event_num = event_num;

    mutex_lock(&ts_data->report_mutex);
#if FTS_MT_PROTOCOL_B_EN
    fts_input_report_b(ts_data, events);
#else
    fts_input_report_a(ts_data, events);
#endif
    mutex_unlock(&ts_data->report_mutex);
    return 0;
}

static int fts_input_report_touch_pv2(struct fts_ts_data *ts_data, u8 *touch_buf)
{
    int i = 0;
    int event_num = 0;
    int pointid = 0;
    int base = 0;
    int max_touch_num = ts_data->pdata->max_touch_number;
    struct ts_event *events = ts_data->events;

    event_num = touch_buf[FTS_TOUCH_E_NUM] & 0x0F;
    if (!event_num || (event_num > max_touch_num)) {
        FTS_ERROR("invalid touch event num(%d)", event_num);
        return -EIO;
    }

    ts_data->touch_event_num = event_num;
    for (i = 0; i < event_num; i++) {
        base = FTS_ONE_TCH_LEN_V2 * i + 4;
        pointid = (touch_buf[FTS_TOUCH_OFF_ID_YH + base]) >> 4;
        if (pointid >= max_touch_num) {
            FTS_ERROR("touch point ID(%d) beyond max_touch_number(%d)",
                      pointid, max_touch_num);
            return -EINVAL;
        }

        events[i].id = pointid;
        events[i].flag = touch_buf[FTS_TOUCH_OFF_E_XH + base] >> 6;

        events[i].x = ((touch_buf[FTS_TOUCH_OFF_E_XH + base] & 0x0F) << 12) \
                      + ((touch_buf[FTS_TOUCH_OFF_XL + base] & 0xFF) << 4) \
                      + ((touch_buf[FTS_TOUCH_OFF_PRE + base] >> 4) & 0x0F);

        events[i].y = ((touch_buf[FTS_TOUCH_OFF_ID_YH + base] & 0x0F) << 12) \
                      + ((touch_buf[FTS_TOUCH_OFF_YL + base] & 0xFF) << 4) \
                      + (touch_buf[FTS_TOUCH_OFF_PRE + base] & 0x0F);
#if FTS_TOUCH_HIRES_EN
        events[i].x = (events[i].x * FTS_TOUCH_HIRES_X ) / FTS_HI_RES_X_MAX;
        events[i].y = (events[i].y * FTS_TOUCH_HIRES_X ) / FTS_HI_RES_X_MAX;
#else
        events[i].x = events[i].x  / FTS_HI_RES_X_MAX;
        events[i].y = events[i].y  / FTS_HI_RES_X_MAX;
#endif
        events[i].area = touch_buf[FTS_TOUCH_OFF_AREA + base];
        events[i].minor = touch_buf[FTS_TOUCH_OFF_MINOR + base];
        events[i].p = 0x3F;
#if FTS_REPORT_PRESSURE_EN
        FTS_ERROR("The pressure property isn't supported");
#endif
        if (events[i].area <= 0) events[i].area = 0x09;
        if (events[i].minor <= 0) events[i].minor = 0x09;
    }

    mutex_lock(&ts_data->report_mutex);
#if FTS_MT_PROTOCOL_B_EN
    fts_input_report_b(ts_data, events);
#else
    fts_input_report_a(ts_data, events);
#endif
    mutex_unlock(&ts_data->report_mutex);
    return 0;
}

int fts_input_report_buffer(struct fts_ts_data *ts_data, u8 *report_buf)
{
    int ret = 0;
    int touch_etype = 0;

    if (!ts_data || !report_buf) {
        FTS_ERROR("ts_data/report_buf is null");
        return -EINVAL;
    }

    touch_etype = ((report_buf[FTS_TOUCH_E_NUM] >> 4) & 0x0F);
    switch (touch_etype) {
    case TOUCH_DEFAULT:
        ret = fts_input_report_touch(ts_data, report_buf);
        break;

    case TOUCH_PROTOCOL_v2:
        ret = fts_input_report_touch_pv2(ts_data, report_buf);
        break;

#if FTS_PEN_EN
    case TOUCH_PEN:
        mutex_lock(&ts_data->report_mutex);
        ret = fts_input_pen_report(ts_data, report_buf);
        mutex_unlock(&ts_data->report_mutex);
        break;
#endif

    default:
        FTS_INFO("unknown touch event(%d)", touch_etype);
        break;
    }

    return ret;
}

static int fts_read_touchdata_spi(struct fts_ts_data *ts_data, u8 *buf)
{
    int ret = 0;

    ts_data->touch_addr = 0x01;
    ret = fts_read(&ts_data->touch_addr, 1, buf, ts_data->touch_size);


    if (ret < 0) {
        FTS_ERROR("touch data(%x) abnormal,ret:%d", buf[1], ret);
        return ret;
    }

    return 0;
}

static int fts_read_touchdata_i2c(struct fts_ts_data *ts_data, u8 *buf)
{
    int ret = 0;
    u32 touch_max_size = 0;
    u32 max_touch_num = ts_data->pdata->max_touch_number;
    u8 event = 0xFF;

    ts_data->touch_addr = 0x01;
    ret = fts_read(&ts_data->touch_addr, 1, buf, ts_data->touch_size);
    if (ret < 0) {
        FTS_ERROR("read touchdata fails,ret:%d", ret);
        return ret;
    }

    event = (buf[FTS_TOUCH_E_NUM] >> 4) & 0x0F;
    if (event == TOUCH_DEFAULT) {
        if (buf[ts_data->touch_size - 1] != 0xFF)
            touch_max_size = max_touch_num * FTS_ONE_TCH_LEN + 2;
    } else if (event == TOUCH_PROTOCOL_v2) {
        touch_max_size = (buf[FTS_TOUCH_E_NUM] & 0x0F) * FTS_ONE_TCH_LEN_V2 + 4;
    }
#if FTS_PEN_EN
    else if (event == TOUCH_PEN) {
        touch_max_size = FTS_SIZE_PEN;
        if (touch_max_size > ts_data->touch_size) {
            FTS_INFO("read next touch message of pen,size:%d-%d",
                     touch_max_size, ts_data->touch_size);
        }
    }
#endif

    if (touch_max_size > ts_data->touch_size) {
        ts_data->ta_size = touch_max_size;
        ts_data->touch_addr += ts_data->touch_size;
        ret = fts_read(&ts_data->touch_addr, 1, buf + ts_data->touch_size, \
                       touch_max_size - ts_data->touch_size);
        if (ret < 0) {
            FTS_ERROR("read touchdata2 fails,ret:%d", ret);
            return ret;
        }
    }

    return 0;
}

static int fts_read_parse_touchdata(struct fts_ts_data *ts_data, u8 *touch_buf)
{
    int ret = 0;

    memset(touch_buf, 0xFF, FTS_MAX_TOUCH_BUF);
    ts_data->ta_size = ts_data->touch_size;

    /*read touch data*/
    if (ts_data->bus_type == BUS_TYPE_SPI)
        ret = fts_read_touchdata_spi(ts_data, touch_buf);
    else if (ts_data->bus_type == BUS_TYPE_I2C)
        ret = fts_read_touchdata_i2c(ts_data, touch_buf);
    else FTS_ERROR("unknown bus type:%d", ts_data->bus_type);
    if (ret < 0) {
        FTS_ERROR("unknown BUS type");
        return TOUCH_ERROR;
    }

    if (ts_data->log_level >= 3)
        fts_show_touch_buffer(touch_buf, ts_data->ta_size);

    if (ret)
        return TOUCH_IGNORE;

    if ((touch_buf[1] == 0xFF) && (touch_buf[2] == 0xFF)
        && (touch_buf[3] == 0xFF) && (touch_buf[4] == 0xFF)) {
        FTS_INFO("touch buff is 0xff, FW initialized");
        return TOUCH_FW_INIT;
    }

#if FTS_PSENSOR_EN
    if (ts_data->proximity_mode) {
        if (fts_proximity_readdata(ts_data) == FTS_RETVAL_IGNORE_TOUCHES)
            return TOUCH_IGNORE;
    }
#endif

#if FTS_FOD_EN
    if (ts_data->fod_mode) {
        if (fts_fod_readdata(ts_data) == FTS_RETVAL_IGNORE_TOUCHES)
            return TOUCH_IGNORE;
    }
#endif

    if (ts_data->suspended && ts_data->gesture_support) {
        if (fts_gesture_readdata(ts_data, touch_buf) == FTS_RETVAL_IGNORE_TOUCHES)
            return TOUCH_IGNORE;
    }

    if (ts_data->suspended) {
        FTS_INFO("In suspend state, not report touch points");
        return TOUCH_IGNORE;
    }

    return ((touch_buf[FTS_TOUCH_E_NUM] >> 4) & 0x0F);
}

static int fts_irq_read_report(struct fts_ts_data *ts_data)
{
    int ret = 0;
    int touch_etype = 0;
    u8 *touch_buf = ts_data->touch_buf;

    touch_etype = fts_read_parse_touchdata(ts_data, touch_buf);
    switch (touch_etype) {
    case TOUCH_DEFAULT:
        ret = fts_input_report_touch(ts_data, touch_buf);
        break;

    case TOUCH_PROTOCOL_v2:
        ret = fts_input_report_touch_pv2(ts_data, touch_buf);
        break;

#if FTS_PEN_EN
    case TOUCH_PEN:
        mutex_lock(&ts_data->report_mutex);
        ret = fts_input_pen_report(ts_data, touch_buf);
        mutex_unlock(&ts_data->report_mutex);
        break;
#endif

    case TOUCH_FW_INIT:
        fts_release_all_finger();
        fts_tp_state_recovery(ts_data);
        break;

    case TOUCH_IGNORE:
    case TOUCH_ERROR:
    case TOUCH_FWDBG:
        break;

    default:
        FTS_INFO("unknown touch event(%d)", touch_etype);
        break;
    }

    return ret;
}

static irqreturn_t fts_irq_handler(int irq, void *data)
{
    struct fts_ts_data *ts_data = fts_data;
#if IS_ENABLED(CONFIG_PM) && FTS_PATCH_COMERR_PM
    int ret = 0;

    if ((ts_data->suspended) && (ts_data->pm_suspend)) {
        ret = wait_for_completion_timeout(
                  &ts_data->pm_completion,
                  msecs_to_jiffies(FTS_TIMEOUT_COMERR_PM));
        if (!ret) {
            FTS_ERROR("Bus don't resume from pm(deep),timeout,skip irq");
            return IRQ_HANDLED;
        }
    }
#endif

    if (ts_data->suspended)
        __pm_wakeup_event(ts_data->p_ws, jiffies_to_msecs(FTS_WAKELOCK_TIMEOUT));


    ts_data->intr_jiffies = jiffies;
    fts_prc_queue_work(ts_data);
    fts_irq_read_report(ts_data);
    if (ts_data->touch_analysis_support && ts_data->ta_flag) {
        ts_data->ta_flag = 0;
        if (ts_data->ta_buf && ts_data->ta_size)
            memcpy(ts_data->ta_buf, ts_data->touch_buf, ts_data->ta_size);
        wake_up_interruptible(&ts_data->ts_waitqueue);
    }

    return IRQ_HANDLED;
}

static int fts_irq_registration(struct fts_ts_data *ts_data)
{
    int ret = 0;
    struct fts_ts_platform_data *pdata = ts_data->pdata;

    ts_data->irq = gpio_to_irq(pdata->irq_gpio);
    pdata->irq_gpio_flags = IRQF_TRIGGER_FALLING | IRQF_ONESHOT;
    FTS_INFO("irq:%d, flag:%x", ts_data->irq, pdata->irq_gpio_flags);
    ret = request_threaded_irq(ts_data->irq, NULL, fts_irq_handler,
                               pdata->irq_gpio_flags,
                               FTS_DRIVER_NAME, ts_data);

    return ret;
}

#if FTS_PEN_EN
static int fts_input_pen_init(struct fts_ts_data *ts_data)
{
    int ret = 0;
    struct input_dev *pen_dev;
    struct fts_ts_platform_data *pdata = ts_data->pdata;
    u32 pen_x_max = pdata->x_max;
    u32 pen_y_max = pdata->y_max;

    FTS_FUNC_ENTER();
    pen_dev = input_allocate_device();
    if (!pen_dev) {
        FTS_ERROR("Failed to allocate memory for input_pen device");
        return -ENOMEM;
    }

#if FTS_PEN_HIRES_EN
    pen_x_max = (pdata->x_max + 1) * FTS_PEN_HIRES_X - 1;
    pen_y_max = (pdata->y_max + 1) * FTS_PEN_HIRES_X - 1;
#endif
    pen_dev->dev.parent = ts_data->dev;
    pen_dev->name = FTS_DRIVER_PEN_NAME;
    pen_dev->evbit[0] |= BIT_MASK(EV_KEY) | BIT_MASK(EV_ABS);
    __set_bit(ABS_X, pen_dev->absbit);
    __set_bit(ABS_Y, pen_dev->absbit);
    __set_bit(BTN_STYLUS, pen_dev->keybit);
    __set_bit(BTN_STYLUS2, pen_dev->keybit);
    __set_bit(BTN_TOUCH, pen_dev->keybit);
    __set_bit(BTN_TOOL_PEN, pen_dev->keybit);
    __set_bit(INPUT_PROP_DIRECT, pen_dev->propbit);
    input_set_abs_params(pen_dev, ABS_X, pdata->x_min, pen_x_max, 0, 0);
    input_set_abs_params(pen_dev, ABS_Y, pdata->y_min, pen_y_max, 0, 0);
    input_set_abs_params(pen_dev, ABS_PRESSURE, 0, 4096, 0, 0);
    input_set_abs_params(pen_dev, ABS_TILT_X, -9000, 9000, 0, 0);
    input_set_abs_params(pen_dev, ABS_TILT_Y, -9000, 9000, 0, 0);
    input_set_abs_params(pen_dev, ABS_Z, 0, 36000, 0, 0);

    ret = input_register_device(pen_dev);
    if (ret) {
        FTS_ERROR("Input device registration failed");
        input_free_device(pen_dev);
        pen_dev = NULL;
        return ret;
    }

    ts_data->pen_dev = pen_dev;
    ts_data->pen_etype = STYLUS_DEFAULT;
    FTS_FUNC_EXIT();
    return 0;
}
#endif

static int fts_input_init(struct fts_ts_data *ts_data)
{
    int ret = 0;
    int key_num = 0;
    struct fts_ts_platform_data *pdata = ts_data->pdata;
    struct input_dev *input_dev;
    u32 touch_x_max = pdata->x_max;
    u32 touch_y_max = pdata->y_max;

    FTS_FUNC_ENTER();
    input_dev = input_allocate_device();
    if (!input_dev) {
        FTS_ERROR("Failed to allocate memory for input device");
        return -ENOMEM;
    }

    /* Init and register Input device */
    input_dev->name = FTS_DRIVER_NAME;
    if (ts_data->bus_type == BUS_TYPE_I2C)
        input_dev->id.bustype = BUS_I2C;
    else
        input_dev->id.bustype = BUS_SPI;
    input_dev->dev.parent = ts_data->dev;

    input_set_drvdata(input_dev, ts_data);

    __set_bit(EV_SYN, input_dev->evbit);
    __set_bit(EV_ABS, input_dev->evbit);
    __set_bit(EV_KEY, input_dev->evbit);
    __set_bit(BTN_TOUCH, input_dev->keybit);
    __set_bit(INPUT_PROP_DIRECT, input_dev->propbit);

    if (pdata->have_key) {
        FTS_INFO("set key capabilities");
        for (key_num = 0; key_num < pdata->key_number; key_num++)
            input_set_capability(input_dev, EV_KEY, pdata->keys[key_num]);
    }

#if FTS_TOUCH_HIRES_EN
    touch_x_max = (pdata->x_max + 1) * FTS_TOUCH_HIRES_X - 1;
    touch_y_max = (pdata->y_max + 1) * FTS_TOUCH_HIRES_X - 1;
#endif

#if FTS_MT_PROTOCOL_B_EN
    input_mt_init_slots(input_dev, pdata->max_touch_number, INPUT_MT_DIRECT);
#else
    input_set_abs_params(input_dev, ABS_MT_TRACKING_ID, 0, 0x0F, 0, 0);
#endif
    input_set_abs_params(input_dev, ABS_MT_POSITION_X, pdata->x_min, touch_x_max, 0, 0);
    input_set_abs_params(input_dev, ABS_MT_POSITION_Y, pdata->y_min, touch_y_max, 0, 0);
    input_set_abs_params(input_dev, ABS_MT_TOUCH_MAJOR, 0, 0xFF, 0, 0);
#if FTS_REPORT_PRESSURE_EN
    input_set_abs_params(input_dev, ABS_MT_PRESSURE, 0, 0xFF, 0, 0);
#endif

    ret = input_register_device(input_dev);
    if (ret) {
        FTS_ERROR("Input device registration failed");
        input_set_drvdata(input_dev, NULL);
        input_free_device(input_dev);
        input_dev = NULL;
        return ret;
    }

#if FTS_PEN_EN
    ret = fts_input_pen_init(ts_data);
    if (ret) {
        FTS_ERROR("Input-pen device registration failed");
        input_set_drvdata(input_dev, NULL);
        input_free_device(input_dev);
        input_dev = NULL;
        return ret;
    }
#endif

    ts_data->input_dev = input_dev;
    FTS_FUNC_EXIT();
    return 0;
}

static int fts_buffer_init(struct fts_ts_data *ts_data)
{
    ts_data->touch_buf = (u8 *)kzalloc(FTS_MAX_TOUCH_BUF, GFP_KERNEL);
    if (!ts_data->touch_buf) {
        FTS_ERROR("failed to alloc memory for touch buf");
        return -ENOMEM;
    }

    if (ts_data->bus_type == BUS_TYPE_SPI)
        ts_data->touch_size = FTS_TOUCH_DATA_LEN_V2;
    else if (ts_data->bus_type == BUS_TYPE_I2C)
        ts_data->touch_size = FTS_SIZE_DEFAULT_V2;
    else FTS_ERROR("unknown bus type:%d", ts_data->bus_type);

    ts_data->touch_analysis_support = 0;
    ts_data->ta_flag = 0;
    ts_data->ta_size = 0;

    return 0;
}

#if FTS_PINCTRL_EN
static int fts_pinctrl_init(struct fts_ts_data *ts)
{
    int ret = 0;

    ts->pinctrl = devm_pinctrl_get(ts->dev);
    if (IS_ERR_OR_NULL(ts->pinctrl)) {
        FTS_ERROR("Failed to get pinctrl, please check dts");
        ret = PTR_ERR(ts->pinctrl);
        goto err_pinctrl_get;
    }

    ts->pins_active = pinctrl_lookup_state(ts->pinctrl, "pmx_ts_active");
    if (IS_ERR_OR_NULL(ts->pins_active)) {
        FTS_ERROR("Pin state[active] not found");
        ret = PTR_ERR(ts->pins_active);
        goto err_pinctrl_lookup;
    }

    ts->pins_suspend = pinctrl_lookup_state(ts->pinctrl, "pmx_ts_suspend");
    if (IS_ERR_OR_NULL(ts->pins_suspend)) {
        FTS_ERROR("Pin state[suspend] not found");
        ret = PTR_ERR(ts->pins_suspend);
        goto err_pinctrl_lookup;
    }

    ts->pins_release = pinctrl_lookup_state(ts->pinctrl, "pmx_ts_release");
    if (IS_ERR_OR_NULL(ts->pins_release)) {
        FTS_ERROR("Pin state[release] not found");
        ret = PTR_ERR(ts->pins_release);
    }

    return 0;
err_pinctrl_lookup:
    if (ts->pinctrl) {
        devm_pinctrl_put(ts->pinctrl);
    }
err_pinctrl_get:
    ts->pinctrl = NULL;
    ts->pins_release = NULL;
    ts->pins_suspend = NULL;
    ts->pins_active = NULL;
    return ret;
}
#endif /* FTS_PINCTRL_EN */

#if FTS_POWER_SOURCE_CUST_EN
/*****************************************************************************
* Power Control
*****************************************************************************/
static int fts_power_source_ctrl(struct fts_ts_data *ts_data, int enable)
{
    int ret = 0;

    if (IS_ERR_OR_NULL(ts_data->vdd)) {
        FTS_ERROR("vdd is invalid");
        return -EINVAL;
    }

    FTS_FUNC_ENTER();
    if (enable) {
        if (ts_data->power_disabled) {
            fts_set_reset(ts_data, 0);
            fts_msleep(2);
            FTS_INFO("set power to on");
            ret = regulator_enable(ts_data->vdd);
            if (ret) {
                FTS_ERROR("enable vdd regulator failed,ret=%d", ret);
            }

            if (!IS_ERR_OR_NULL(ts_data->iovcc)) {
                ret = regulator_enable(ts_data->iovcc);
                if (ret) {
                    FTS_ERROR("enable iovcc regulator failed,ret=%d", ret);
                }
            }
            fts_msleep(2);
            fts_set_reset(ts_data, 1);
            ts_data->power_disabled = false;
        }
    } else {
        if (!ts_data->power_disabled) {
            fts_set_reset(ts_data, 0);
            fts_msleep(2);
            FTS_INFO("set power to off");
            if (!IS_ERR_OR_NULL(ts_data->iovcc)) {
                ret = regulator_disable(ts_data->iovcc);
                if (ret) {
                    FTS_ERROR("disable iovcc regulator failed,ret=%d", ret);
                }
            }
            ret = regulator_disable(ts_data->vdd);
            if (ret) {
                FTS_ERROR("disable vdd regulator failed,ret=%d", ret);
            }
            ts_data->power_disabled = true;
        }
    }

    FTS_FUNC_EXIT();
    return ret;
}

/*****************************************************************************
* Name: fts_power_source_init
* Brief: Init regulator power:vdd/vcc_io(if have), generally, no vcc_io
*        vdd---->vdd-supply in dts, kernel will auto add "-supply" to parse
*        Must be call after fts_gpio_configure() execute,because this function
*        will operate reset-gpio which request gpio in fts_gpio_configure()
* Input:
* Output:
* Return: return 0 if init power successfully, otherwise return error code
*****************************************************************************/
static int fts_power_source_init(struct fts_ts_data *ts_data)
{
    int ret = 0;

    FTS_FUNC_ENTER();
    ts_data->vdd = regulator_get(ts_data->dev, "vdd");
    if (IS_ERR_OR_NULL(ts_data->vdd)) {
        ret = PTR_ERR(ts_data->vdd);
        FTS_ERROR("get vdd regulator failed,ret=%d", ret);
        return ret;
    }

    if (regulator_count_voltages(ts_data->vdd) > 0) {
        ret = regulator_set_voltage(ts_data->vdd, FTS_VTG_MIN_UV,
                                    FTS_VTG_MAX_UV);
        if (ret) {
            FTS_ERROR("vdd regulator set_vtg failed ret=%d", ret);
            regulator_put(ts_data->vdd);
            return ret;
        }
    }

    ts_data->iovcc = regulator_get(ts_data->dev, "iovcc");
    if (!IS_ERR_OR_NULL(ts_data->iovcc)) {
        if (regulator_count_voltages(ts_data->iovcc) > 0) {
            ret = regulator_set_voltage(ts_data->iovcc,
                                        FTS_IOVCC_VTG_MIN_UV,
                                        FTS_IOVCC_VTG_MAX_UV);
            if (ret) {
                FTS_ERROR("iovcc regulator set_vtg failed,ret=%d", ret);
                regulator_put(ts_data->iovcc);
            }
        }
    }

    ret = fts_power_source_ctrl(ts_data, ENABLE);
    if (ret) {
        FTS_ERROR("fail to enable power(regulator)");
    }

    FTS_FUNC_EXIT();
    return ret;
}

static int fts_power_source_exit(struct fts_ts_data *ts_data)
{
    fts_power_source_ctrl(ts_data, DISABLE);

    if (!IS_ERR_OR_NULL(ts_data->vdd)) {
        if (regulator_count_voltages(ts_data->vdd) > 0)
            regulator_set_voltage(ts_data->vdd, 0, FTS_VTG_MAX_UV);
        regulator_put(ts_data->vdd);
    }

    if (!IS_ERR_OR_NULL(ts_data->iovcc)) {
        if (regulator_count_voltages(ts_data->iovcc) > 0)
            regulator_set_voltage(ts_data->iovcc, 0, FTS_IOVCC_VTG_MAX_UV);
        regulator_put(ts_data->iovcc);
    }

    return 0;
}
#endif /* FTS_POWER_SOURCE_CUST_EN */

static int fts_power_init(struct fts_ts_data *ts_data)
{
    int ret = 0;

    ts_data->power_disabled = true;
#if FTS_POWER_SOURCE_CUST_EN
    ret = fts_power_source_init(ts_data);
    if (ret) {
        FTS_ERROR("fail to get power(regulator)");
        return ret;
    }
#else
#if (!FTS_CHIP_IDC)
    ret = fts_set_reset(ts_data, 0);
    if (ret) return ret;
#endif
    fts_msleep(2);
    ret = fts_set_reset(ts_data, 1);
    if (ret) return ret;
#endif /* FTS_POWER_SOURCE_CUST_EN */

    /* Init BUS pins(SPI/I2C) after powring on if enabling FTS_PINCTRL_EN*/
#if FTS_PINCTRL_EN
    fts_pinctrl_init(ts_data);
    if (ts_data->pinctrl && ts_data->pins_active) {
        ret = pinctrl_select_state(ts_data->pinctrl, ts_data->pins_active);
        if (ret < 0) {
            FTS_ERROR("Set bus pins to active state failed,ret=%d", ret);
        }
    }
#endif

    fts_msleep(200);
    return 0;
}

static int fts_power_suspend(struct fts_ts_data *ts_data)
{
    int ret = 0;

    FTS_FUNC_ENTER();
    FTS_INFO("make TP enter into sleep mode");
    ret = fts_write_reg(FTS_REG_POWER_MODE, FTS_REG_POWER_MODE_SLEEP);
    if (ret < 0) {
        FTS_ERROR("set TP to sleep mode failed, ret=%d", ret);
    }

#if FTS_POWER_SUSPEND_OFF_EN
#if FTS_PINCTRL_EN
    if (ts_data->pinctrl && ts_data->pins_suspend) {
        if (pinctrl_select_state(ts_data->pinctrl, ts_data->pins_suspend)) {
            FTS_ERROR("Set bus pins to suspend state failed");
        }
    }
#endif /* FTS_PINCTRL_EN */

#if FTS_POWER_SOURCE_CUST_EN
    if (fts_power_source_ctrl(ts_data, DISABLE)) {
        FTS_ERROR("set power to off failed");
    }
#else
    FTS_ERROR("FTS_POWER_SOURCE_CUST_EN=0,FTS_POWER_SUSPEND_OFF_EN=1");
#endif /* FTS_POWER_SOURCE_CUST_EN */
#endif /* FTS_POWER_SUSPEND_OFF_EN */

    FTS_FUNC_EXIT();
    return 0;
}

static int fts_power_resume(struct fts_ts_data *ts_data)
{
    FTS_FUNC_ENTER();
#if FTS_POWER_SUSPEND_OFF_EN
#if FTS_POWER_SOURCE_CUST_EN
    if (fts_power_source_ctrl(ts_data, ENABLE)) {
        FTS_ERROR("set power to on failed");
    }
    fts_msleep(FTS_DELAY_RESUME_RESET);
#else
    FTS_ERROR("FTS_POWER_SOURCE_CUST_EN=0,FTS_POWER_SUSPEND_OFF_EN=1");
#endif /* FTS_POWER_SOURCE_CUST_EN */

#if FTS_PINCTRL_EN
    if (ts_data->pinctrl && ts_data->pins_active) {
        if (pinctrl_select_state(ts_data->pinctrl, ts_data->pins_active)) {
            FTS_ERROR("Set bus pins to active state failed");
        }
    }
#endif /* FTS_PINCTRL_EN */

#else /* else FTS_POWER_SUSPEND_OFF_EN */
    if (!ts_data->ic_info.is_incell) {
        fts_reset_proc(ts_data, false, FTS_DELAY_RESUME_RESET);
    }
#endif /* FTS_POWER_SUSPEND_OFF_EN */

    FTS_FUNC_EXIT();
    return 0;
}

static int fts_gpio_configure(struct fts_ts_data *ts_data)
{
    int ret = 0;

    FTS_FUNC_ENTER();
    /* request irq gpio */
    if (gpio_is_valid(ts_data->pdata->irq_gpio)) {
        ret = gpio_request(ts_data->pdata->irq_gpio, "fts_irq_gpio");
        if (ret) {
            FTS_ERROR("[GPIO]irq gpio request failed");
            goto err_irq_gpio_req;
        }

        ret = gpio_direction_input(ts_data->pdata->irq_gpio);
        if (ret) {
            FTS_ERROR("[GPIO]set_direction for irq gpio failed");
            goto err_irq_gpio_dir;
        }
    }

    /* request reset gpio */
    if (gpio_is_valid(ts_data->pdata->reset_gpio)) {
        ret = gpio_request(ts_data->pdata->reset_gpio, "fts_reset_gpio");
        if (ret) {
            FTS_ERROR("[GPIO]reset gpio request failed");
            goto err_irq_gpio_dir;
        }
    }

    FTS_FUNC_EXIT();
    return 0;

err_irq_gpio_dir:
    if (gpio_is_valid(ts_data->pdata->irq_gpio))
        gpio_free(ts_data->pdata->irq_gpio);
err_irq_gpio_req:
    FTS_FUNC_EXIT();
    return ret;
}

static int fts_bus_init(struct fts_ts_data *ts_data)
{
    FTS_FUNC_ENTER();
    ts_data->bus_tx_buf = kzalloc(FTS_MAX_BUS_BUF, GFP_KERNEL);
    if (NULL == ts_data->bus_tx_buf) {
        FTS_ERROR("failed to allocate memory for bus_tx_buf");
        return -ENOMEM;
    }

    ts_data->bus_rx_buf = kzalloc(FTS_MAX_BUS_BUF, GFP_KERNEL);
    if (NULL == ts_data->bus_rx_buf) {
        FTS_ERROR("failed to allocate memory for bus_rx_buf");
        return -ENOMEM;
    }

    FTS_FUNC_EXIT();
    return 0;
}

static int fts_get_dt_coords(struct device *dev, char *name,
                             struct fts_ts_platform_data *pdata)
{
    int ret = 0;
    u32 coords[FTS_COORDS_ARR_SIZE] = { 0 };
    struct property *prop;
    struct device_node *np = dev->of_node;
    int coords_size;

    prop = of_find_property(np, name, NULL);
    if (!prop)
        return -EINVAL;
    if (!prop->value)
        return -ENODATA;

    coords_size = prop->length / sizeof(u32);
    if (coords_size != FTS_COORDS_ARR_SIZE) {
        FTS_ERROR("invalid:%s, size:%d", name, coords_size);
        return -EINVAL;
    }

    ret = of_property_read_u32_array(np, name, coords, coords_size);
    if (ret < 0) {
        FTS_ERROR("Unable to read %s, please check dts", name);
        pdata->x_min = FTS_X_MIN_DISPLAY_DEFAULT;
        pdata->y_min = FTS_Y_MIN_DISPLAY_DEFAULT;
        pdata->x_max = FTS_X_MAX_DISPLAY_DEFAULT;
        pdata->y_max = FTS_Y_MAX_DISPLAY_DEFAULT;
        return -ENODATA;
    } else {
        pdata->x_min = coords[0];
        pdata->y_min = coords[1];
        pdata->x_max = coords[2];
        pdata->y_max = coords[3];
    }

    FTS_INFO("display x(%d %d) y(%d %d)", pdata->x_min, pdata->x_max,
             pdata->y_min, pdata->y_max);
    return 0;
}

static int fts_parse_dt(struct device *dev, struct fts_ts_platform_data *pdata)
{
    int ret = 0;
    struct device_node *np = dev->of_node;
    u32 temp_val = 0;

    FTS_FUNC_ENTER();
    if (!np || !pdata) {
        FTS_ERROR("np/pdata is null");
        return -EINVAL;
    }

    ret = fts_get_dt_coords(dev, "focaltech,display-coords", pdata);
    if (ret < 0)
        FTS_ERROR("Unable to get display-coords");

    /* key */
    pdata->have_key = of_property_read_bool(np, "focaltech,have-key");
    if (pdata->have_key) {
        ret = of_property_read_u32(np, "focaltech,key-number", &pdata->key_number);
        if (ret < 0)
            FTS_ERROR("Key number undefined!");

        ret = of_property_read_u32_array(np, "focaltech,keys",
                                         pdata->keys, pdata->key_number);
        if (ret < 0)
            FTS_ERROR("Keys undefined!");
        else if (pdata->key_number > FTS_MAX_KEYS)
            pdata->key_number = FTS_MAX_KEYS;

        ret = of_property_read_u32_array(np, "focaltech,key-x-coords",
                                         pdata->key_x_coords,
                                         pdata->key_number);
        if (ret < 0)
            FTS_ERROR("Key Y Coords undefined!");

        ret = of_property_read_u32_array(np, "focaltech,key-y-coords",
                                         pdata->key_y_coords,
                                         pdata->key_number);
        if (ret < 0)
            FTS_ERROR("Key X Coords undefined!");

        FTS_INFO("VK Number:%d, key:(%d,%d,%d), "
                 "coords:(%d,%d),(%d,%d),(%d,%d)",
                 pdata->key_number,
                 pdata->keys[0], pdata->keys[1], pdata->keys[2],
                 pdata->key_x_coords[0], pdata->key_y_coords[0],
                 pdata->key_x_coords[1], pdata->key_y_coords[1],
                 pdata->key_x_coords[2], pdata->key_y_coords[2]);
    }

    /* reset, irq gpio info */
    pdata->reset_gpio = of_get_named_gpio(np, "focaltech,reset-gpio", 0);
    if (pdata->reset_gpio < 0)
        FTS_ERROR("Unable to get reset_gpio");

    pdata->irq_gpio = of_get_named_gpio(np, "focaltech,irq-gpio", 0);
    if (pdata->irq_gpio < 0)
        FTS_ERROR("Unable to get irq_gpio");

    ret = of_property_read_u32(np, "focaltech,max-touch-number", &temp_val);
    if (ret < 0) {
        FTS_ERROR("Unable to get max-touch-number, please check dts");
        pdata->max_touch_number = FTS_MAX_POINTS_SUPPORT;
    } else {
        if (temp_val < 2)
            pdata->max_touch_number = 2; /* max_touch_number must >= 2 */
        else if (temp_val > FTS_MAX_POINTS_SUPPORT)
            pdata->max_touch_number = FTS_MAX_POINTS_SUPPORT;
        else
            pdata->max_touch_number = temp_val;
    }

    FTS_INFO("max touch number:%d, irq gpio:%d, reset gpio:%d",
             pdata->max_touch_number, pdata->irq_gpio, pdata->reset_gpio);

    FTS_FUNC_EXIT();
    return 0;
}

static int fts_ts_suspend(struct device *dev)
{
    struct fts_ts_data *ts_data = fts_data;

    FTS_FUNC_ENTER();
    if (ts_data->suspended) {
        FTS_INFO("Already in suspend state");
        return 0;
    }

    if (ts_data->fw_loading) {
        FTS_INFO("fw upgrade in process, can't suspend");
        return 0;
    }

    ts_data->need_work_in_suspend = false;
    fts_esdcheck_suspend(ts_data);
#if FTS_PSENSOR_EN
    if (ts_data->proximity_mode) {
        fts_proximity_suspend(ts_data);
        ts_data->need_work_in_suspend = true;
        fts_release_all_finger();
        ts_data->suspended = true;
        return 0;
    }
#endif

#if FTS_FOD_EN
    if (ts_data->fod_mode) {
        fts_fod_suspend(ts_data);
        ts_data->need_work_in_suspend = true;
    }
#endif

    if (ts_data->gesture_support) {
        fts_gesture_suspend(ts_data);
        ts_data->need_work_in_suspend = true;
    }

    if (ts_data->need_work_in_suspend) {
        if (enable_irq_wake(ts_data->irq)) {
            FTS_ERROR("enable_irq_wake(irq:%d) fail", ts_data->irq);
        }
    } else {
        fts_irq_disable();
        fts_power_suspend(ts_data);
    }

    fts_release_all_finger();
    ts_data->suspended = true;
    FTS_FUNC_EXIT();
    return 0;
}

static int fts_ts_resume(struct device *dev)
{
    struct fts_ts_data *ts_data = fts_data;

    FTS_FUNC_ENTER();
    if (!ts_data->suspended) {
        FTS_DEBUG("Already in awake state");
        return 0;
    }

    if (ts_data->fw_loading) {
        FTS_INFO("fw upgrade in process, don't resume");
        return 0;
    }

    ts_data->suspended = false;
    fts_release_all_finger();
#if FTS_PSENSOR_EN
    if (ts_data->proximity_mode) {
        fts_wait_tp_to_valid();
        fts_proximity_resume(ts_data);
        fts_esdcheck_resume(ts_data);
        if (ts_data->gesture_support) fts_gesture_resume(ts_data);
        return 0;
    }
#endif

    if (ts_data->need_work_in_suspend) {
#if FTS_FOD_EN
        if ((!ts_data->ic_info.is_incell) && (!fts_fod_checkdown(ts_data))) {
            fts_reset_proc(ts_data, false, FTS_DELAY_RESUME_RESET);
        }
#else
        if (!ts_data->ic_info.is_incell) {
            fts_reset_proc(ts_data, false, FTS_DELAY_RESUME_RESET);
        }
#endif
    } else {
        fts_power_resume(ts_data);
    }

    fts_enter_normal_fw();
    if (ts_data->gesture_support) {
        fts_gesture_resume(ts_data);
    }
#if FTS_FOD_EN
    if (ts_data->fod_mode) {
        fts_fod_resume(ts_data);
    }
#endif
    fts_ex_mode_recovery(ts_data);
    fts_esdcheck_resume(ts_data);

    if (ts_data->need_work_in_suspend) {
        if (disable_irq_wake(ts_data->irq)) {
            FTS_ERROR("disable_irq_wake(irq:%d) fail", ts_data->irq);
        }
    } else {
        fts_irq_enable();
    }

    FTS_FUNC_EXIT();
    return 0;
}

static void fts_resume_work(struct work_struct *work)
{
    struct fts_ts_data *ts_data = container_of(work, struct fts_ts_data, resume_work);
    fts_ts_resume(ts_data->dev);
}


#if IS_ENABLED(CONFIG_DRM)
#if IS_ENABLED(CONFIG_DRM_PANEL)
static struct drm_panel *active_panel;

static int drm_check_dt(struct fts_ts_data *ts_data)
{
    int i = 0;
    int count = 0;
    struct device_node *node = NULL;
    struct drm_panel *panel = NULL;
    struct device_node *np = NULL;

    if (ts_data && ts_data->dev && ts_data->dev->of_node) {
        np = ts_data->dev->of_node;
        count = of_count_phandle_with_args(np, "panel", NULL);
        if (count <= 0) {
            FTS_ERROR("find drm_panel count(%d) fail", count);
            return -ENODEV;
        }

        for (i = 0; i < count; i++) {
            node = of_parse_phandle(np, "panel", i);
            panel = of_drm_find_panel(node);
            of_node_put(node);
            if (!IS_ERR(panel)) {
                FTS_INFO("find drm_panel successfully");
                active_panel = panel;
                return 0;
            }
        }
    }

    FTS_ERROR("no find drm_panel");
    return -ENODEV;
}
#endif //CONFIG_DRM_PANEL
#endif //CONFIG_DRM

#ifdef CONFIG_SOC_KY_X1
static int ky_fts_hw_init(struct fts_ts_data *ts_data)
{
    int ret = 0;
    ret = fts_bus_init(ts_data);
    if (ret) {
        FTS_ERROR("bus initialize fail");
        goto err_bus_init;
    }

    ret = fts_buffer_init(ts_data);
    if (ret) {
        FTS_ERROR("buffer init fail");
        goto err_bus_init;
    }

    ret = fts_gpio_configure(ts_data);
    if (ret) {
        FTS_ERROR("configure the gpios fail");
        goto err_gpio_config;
    }

    ret = fts_power_init(ts_data);
    if (ret) {
        FTS_ERROR("fail to init power");
        goto err_power_init;
    }

    ret = fts_get_ic_information(ts_data);
    if (ret) {
        FTS_ERROR("not focal IC, unregister driver");
        goto err_power_init;
    }

    ret = fts_input_init(ts_data);
    if (ret) {
        FTS_ERROR("input initialize fail");
        goto err_power_init;
    }

#if FTS_READ_CUSTOMER_INFO
    ret = fts_read_customer_information(ts_data);
    if (ret) {
        FTS_ERROR("read customer information fail");
    }
#endif

    ret = fts_create_apk_debug_channel(ts_data);
    if (ret) {
        FTS_ERROR("create apk debug node fail");
    }

    ret = fts_create_sysfs(ts_data);
    if (ret) {
        FTS_ERROR("create sysfs node fail");
    }

    ret = fts_point_report_check_init(ts_data);
    if (ret) {
        FTS_ERROR("init point report check fail");
    }

    ret = fts_ex_mode_init(ts_data);
    if (ret) {
        FTS_ERROR("init glove/cover/charger fail");
    }

    ret = fts_gesture_init(ts_data);
    if (ret) {
        FTS_ERROR("init gesture fail");
    }

#if FTS_PSENSOR_EN
    ret = fts_proximity_init(ts_data);
    if (ret) {
        FTS_ERROR("init proximity fail");
    }
#endif

    ret = fts_esdcheck_init(ts_data);
    if (ret) {
        FTS_ERROR("init esd check fail");
    }

    ret = fts_irq_registration(ts_data);
    if (ret) {
        FTS_ERROR("request irq failed");
        goto err_irq_req;
    }

    ret = fts_fwupg_init(ts_data);
    if (ret) {
        FTS_ERROR("init fw upgrade fail");
    }

#if IS_ENABLED(CONFIG_PM) && FTS_PATCH_COMERR_PM
    init_completion(&ts_data->pm_completion);
    ts_data->pm_suspend = false;
#endif
    FTS_FUNC_EXIT();
    return 0;

err_irq_req:
    fts_esdcheck_exit(ts_data);
#if FTS_PSENSOR_EN
    fts_proximity_exit(ts_data);
#endif
    fts_gesture_exit(ts_data);
    fts_ex_mode_exit(ts_data);
    fts_point_report_check_exit(ts_data);
    fts_remove_sysfs(ts_data);
    fts_release_apk_debug_channel(ts_data);
    input_unregister_device(ts_data->input_dev);
#if FTS_PEN_EN
    input_unregister_device(ts_data->pen_dev);
#endif
err_power_init:
#if FTS_PINCTRL_EN
    if (ts_data->pinctrl) {
        if (ts_data->pins_release) {
            pinctrl_select_state(ts_data->pinctrl, ts_data->pins_release);
        }
        devm_pinctrl_put(ts_data->pinctrl);
        ts_data->pinctrl = NULL;
    }
#endif
#if FTS_POWER_SOURCE_CUST_EN
    fts_power_source_exit(ts_data);
#endif
    if (gpio_is_valid(ts_data->pdata->reset_gpio))
        gpio_free(ts_data->pdata->reset_gpio);
    if (gpio_is_valid(ts_data->pdata->irq_gpio))
        gpio_free(ts_data->pdata->irq_gpio);
err_gpio_config:
    kfree_safe(ts_data->touch_buf);
err_bus_init:
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 4, 0))
    wakeup_source_trash(&ts_data->ws);
    ts_data->p_ws = NULL;
#else
    wakeup_source_unregister(ts_data->p_ws);
#endif
    cancel_work_sync(&ts_data->resume_work);
    if (ts_data->ts_workqueue) destroy_workqueue(ts_data->ts_workqueue);
    kfree_safe(ts_data->bus_tx_buf);
    kfree_safe(ts_data->bus_rx_buf);
    kfree_safe(ts_data->pdata);

    FTS_FUNC_EXIT();
    return ret;
}
#endif
static int callback_times = 0;
static int fb_notifier_callback(struct notifier_block *self, unsigned long event, void *v)
{
    struct fts_ts_data *ts_data = container_of(self, struct fts_ts_data, fb_notif);
    FTS_FUNC_ENTER();
    if (ts_data && v) {
#if IS_ENABLED(CONFIG_DRM)

#if IS_ENABLED(CONFIG_DRM_PANEL)
#ifdef CONFIG_SOC_KY_X1
	const unsigned long event_enum[2] = {DRM_PANEL_EARLY_EVENT_BLANK, DRM_PANEL_EVENT_BLANK};
#else //CONFIG_SOC_KY_X1
        int blank_value = *((int *)(((struct drm_panel_notifier *)v)->data));
        const unsigned long event_enum[2] = {DRM_PANEL_EARLY_EVENT_BLANK, DRM_PANEL_EVENT_BLANK};
        const int blank_enum[2] = {DRM_PANEL_BLANK_POWERDOWN, DRM_PANEL_BLANK_UNBLANK};
#endif //CONFIG_SOC_KY_X1
#else //CONFIG_DRM_PANEL
        int blank_value = *((int *)(((struct msm_drm_notifier *)v)->data));
        const unsigned long event_enum[2] = {MSM_DRM_EARLY_EVENT_BLANK, MSM_DRM_EVENT_BLANK};
        const int blank_enum[2] = {MSM_DRM_BLANK_POWERDOWN, MSM_DRM_BLANK_UNBLANK};
#endif //CONFIG_DRM_PANEL

#elif IS_ENABLED(CONFIG_FB)
        const unsigned long event_enum[2] = {FB_EARLY_EVENT_BLANK, FB_EVENT_BLANK};
        const int blank_enum[2] = {FB_BLANK_POWERDOWN, FB_BLANK_UNBLANK};
        int blank_value = *((int *)(((struct fb_event *)v)->data));
#endif //CONFIG_DRM
#ifdef CONFIG_SOC_KY_X1
	FTS_INFO("notifier,event:%lu", event);
	if(callback_times == 0)
	    ky_fts_hw_init(ts_data);
	else {
            if (event_enum[1] == event) {
                queue_work(fts_data->ts_workqueue, &fts_data->resume_work);
            } else if (event_enum[0] == event) {
                cancel_work_sync(&fts_data->resume_work);
                fts_ts_suspend(ts_data->dev);
            } else {
                FTS_DEBUG("notifier,event:%lu, not care", event);
            }
	}
#else
        FTS_INFO("notifier,event:%lu,blank:%d", event, blank_value);
        if ((blank_enum[1] == blank_value) && (event_enum[1] == event)) {
            queue_work(fts_data->ts_workqueue, &fts_data->resume_work);
        } else if ((blank_enum[0] == blank_value) && (event_enum[0] == event)) {
            cancel_work_sync(&fts_data->resume_work);
            fts_ts_suspend(ts_data->dev);
        } else {
            FTS_DEBUG("notifier,event:%lu,blank:%d, not care", event, blank_value);
        }
#endif
    } else {
        FTS_ERROR("ts_data/v is null");
        return -EINVAL;
    }
    FTS_FUNC_EXIT();
    callback_times++;
    return 0;
}

static int fts_notifier_callback_init(struct fts_ts_data *ts_data)
{
    int ret = 0;
    FTS_FUNC_ENTER();
#if IS_ENABLED(CONFIG_DRM)
    ts_data->fb_notif.notifier_call = fb_notifier_callback;
#if IS_ENABLED(CONFIG_DRM_PANEL)
    ret = drm_check_dt(ts_data);
    if (ret) FTS_ERROR("parse drm-panel fail");
    FTS_INFO("init notifier with drm_panel_notifier_register");
#ifdef CONFIG_DRM_KY
    ky_drm_register_client(&ts_data->fb_notif);
#endif
#else
    FTS_INFO("init notifier with msm_drm_register_client");
    ret = msm_drm_register_client(&ts_data->fb_notif);
    if (ret) FTS_ERROR("[DRM]msm_drm_register_client fail: %d", ret);
#endif //CONFIG_DRM_PANEL

#elif IS_ENABLED(CONFIG_FB)
    FTS_INFO("init notifier with fb_register_client");
    ts_data->fb_notif.notifier_call = fb_notifier_callback;
    ret = fb_register_client(&ts_data->fb_notif);
    if (ret) {
        FTS_ERROR("[FB]Unable to register fb_notifier: %d", ret);
    }

#endif //CONFIG_DRM
    FTS_FUNC_EXIT();
    return ret;
}

static int fts_notifier_callback_exit(struct fts_ts_data *ts_data)
{
    FTS_FUNC_ENTER();
#if IS_ENABLED(CONFIG_DRM)
#if IS_ENABLED(CONFIG_DRM_PANEL)
#ifdef CONFIG_DRM_KY
    ky_drm_unregister_client(&ts_data->fb_notif);
#endif
#else
    if (msm_drm_unregister_client(&ts_data->fb_notif))
        FTS_ERROR("[DRM]Error occurred while unregistering fb_notifier.");
#endif

#elif IS_ENABLED(CONFIG_FB)
    if (fb_unregister_client(&ts_data->fb_notif))
        FTS_ERROR("[FB]Error occurred while unregistering fb_notifier.");
#endif //CONFIG_DRM
    FTS_FUNC_EXIT();
    return 0;
}


int fts_ts_probe_entry(struct fts_ts_data *ts_data)
{
    int ret = 0;

    FTS_FUNC_ENTER();
    FTS_INFO("%s", FTS_DRIVER_VERSION);
    fts_data = ts_data;
    ts_data->pdata = kzalloc(sizeof(struct fts_ts_platform_data), GFP_KERNEL);
    if (!ts_data->pdata) {
        FTS_ERROR("allocate memory for platform_data fail");
        return -ENOMEM;
    }

    ret = fts_parse_dt(ts_data->dev, ts_data->pdata);
    if (ret) {
        FTS_ERROR("device-tree parse fail");
    }

    ts_data->ts_workqueue = create_singlethread_workqueue("fts_wq");
    if (!ts_data->ts_workqueue) {
        FTS_ERROR("create fts workqueue fail");
    } else {
        INIT_WORK(&ts_data->resume_work, fts_resume_work);
    }
    spin_lock_init(&ts_data->irq_lock);
    mutex_init(&ts_data->report_mutex);
    mutex_init(&ts_data->bus_lock);
    init_waitqueue_head(&ts_data->ts_waitqueue);
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 4, 0))
    wakeup_source_init(&ts_data->ws, "fts_ws");
    ts_data->p_ws = &ts_data->ws;
#else
    ts_data->p_ws = wakeup_source_register(ts_data->dev, "fts_ws");
#endif
#ifndef CONFIG_SOC_KY_X1
    ret = fts_bus_init(ts_data);
    if (ret) {
        FTS_ERROR("bus initialize fail");
        goto err_bus_init;
    }

    ret = fts_buffer_init(ts_data);
    if (ret) {
        FTS_ERROR("buffer init fail");
        goto err_bus_init;
    }

    ret = fts_gpio_configure(ts_data);
    if (ret) {
        FTS_ERROR("configure the gpios fail");
        goto err_gpio_config;
    }

    ret = fts_power_init(ts_data);
    if (ret) {
        FTS_ERROR("fail to init power");
        goto err_power_init;
    }

    ret = fts_get_ic_information(ts_data);
    if (ret) {
        FTS_ERROR("not focal IC, unregister driver");
        goto err_power_init;
    }

    ret = fts_input_init(ts_data);
    if (ret) {
        FTS_ERROR("input initialize fail");
        goto err_power_init;
    }

#if FTS_READ_CUSTOMER_INFO
    ret = fts_read_customer_information(ts_data);
    if (ret) {
        FTS_ERROR("read customer information fail");
    }
#endif

    ret = fts_create_apk_debug_channel(ts_data);
    if (ret) {
        FTS_ERROR("create apk debug node fail");
    }

    ret = fts_create_sysfs(ts_data);
    if (ret) {
        FTS_ERROR("create sysfs node fail");
    }

    ret = fts_point_report_check_init(ts_data);
    if (ret) {
        FTS_ERROR("init point report check fail");
    }

    ret = fts_ex_mode_init(ts_data);
    if (ret) {
        FTS_ERROR("init glove/cover/charger fail");
    }

    ret = fts_gesture_init(ts_data);
    if (ret) {
        FTS_ERROR("init gesture fail");
    }

#if FTS_PSENSOR_EN
    ret = fts_proximity_init(ts_data);
    if (ret) {
        FTS_ERROR("init proximity fail");
    }
#endif

    ret = fts_esdcheck_init(ts_data);
    if (ret) {
        FTS_ERROR("init esd check fail");
    }

    ret = fts_irq_registration(ts_data);
    if (ret) {
        FTS_ERROR("request irq failed");
        goto err_irq_req;
    }

    ret = fts_fwupg_init(ts_data);
    if (ret) {
        FTS_ERROR("init fw upgrade fail");
    }

#if IS_ENABLED(CONFIG_PM) && FTS_PATCH_COMERR_PM
    init_completion(&ts_data->pm_completion);
    ts_data->pm_suspend = false;
#endif
#endif
    ret = fts_notifier_callback_init(ts_data);
    if (ret) {
        FTS_ERROR("init notifier callback fail");
    }

    FTS_FUNC_EXIT();
    return ret;
#ifndef CONFIG_SOC_KY_X1
err_irq_req:
    fts_esdcheck_exit(ts_data);
#if FTS_PSENSOR_EN
    fts_proximity_exit(ts_data);
#endif
    fts_gesture_exit(ts_data);
    fts_ex_mode_exit(ts_data);
    fts_point_report_check_exit(ts_data);
    fts_remove_sysfs(ts_data);
    fts_release_apk_debug_channel(ts_data);
    input_unregister_device(ts_data->input_dev);
#if FTS_PEN_EN
    input_unregister_device(ts_data->pen_dev);
#endif
err_power_init:
#if FTS_PINCTRL_EN
    if (ts_data->pinctrl) {
        if (ts_data->pins_release) {
            pinctrl_select_state(ts_data->pinctrl, ts_data->pins_release);
        }
        devm_pinctrl_put(ts_data->pinctrl);
        ts_data->pinctrl = NULL;
    }
#endif
#if FTS_POWER_SOURCE_CUST_EN
    fts_power_source_exit(ts_data);
#endif
    if (gpio_is_valid(ts_data->pdata->reset_gpio))
        gpio_free(ts_data->pdata->reset_gpio);
    if (gpio_is_valid(ts_data->pdata->irq_gpio))
        gpio_free(ts_data->pdata->irq_gpio);
err_gpio_config:
    kfree_safe(ts_data->touch_buf);
err_bus_init:
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 4, 0))
    wakeup_source_trash(&ts_data->ws);
    ts_data->p_ws = NULL;
#else
    wakeup_source_unregister(ts_data->p_ws);
#endif
    cancel_work_sync(&ts_data->resume_work);
    if (ts_data->ts_workqueue) destroy_workqueue(ts_data->ts_workqueue);
    kfree_safe(ts_data->bus_tx_buf);
    kfree_safe(ts_data->bus_rx_buf);
    kfree_safe(ts_data->pdata);

    FTS_FUNC_EXIT();
    return ret;
#endif
}

int fts_ts_remove_entry(struct fts_ts_data *ts_data)
{
    FTS_FUNC_ENTER();
    cancel_work_sync(&ts_data->resume_work);
    fts_notifier_callback_exit(ts_data);
    free_irq(ts_data->irq, ts_data);
    fts_fwupg_exit(ts_data);
    fts_esdcheck_exit(ts_data);
#if FTS_PSENSOR_EN
    fts_proximity_exit(ts_data);
#endif
    fts_gesture_exit(ts_data);
    fts_ex_mode_exit(ts_data);
    fts_point_report_check_exit(ts_data);
    fts_remove_sysfs(ts_data);
    fts_release_apk_debug_channel(ts_data);
    input_unregister_device(ts_data->input_dev);
#if FTS_PEN_EN
    input_unregister_device(ts_data->pen_dev);
#endif
    if (ts_data->ts_workqueue) destroy_workqueue(ts_data->ts_workqueue);
    if (gpio_is_valid(ts_data->pdata->reset_gpio))
        gpio_free(ts_data->pdata->reset_gpio);
    if (gpio_is_valid(ts_data->pdata->irq_gpio))
        gpio_free(ts_data->pdata->irq_gpio);

#if FTS_PINCTRL_EN
    if (ts_data->pinctrl) {
        if (ts_data->pins_release) {
            pinctrl_select_state(ts_data->pinctrl, ts_data->pins_release);
        }
        devm_pinctrl_put(ts_data->pinctrl);
        ts_data->pinctrl = NULL;
    }
#endif

#if FTS_POWER_SOURCE_CUST_EN
    fts_power_source_exit(ts_data);
#endif
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 4, 0))
    wakeup_source_trash(&ts_data->ws);
    ts_data->p_ws = NULL;
#else
    wakeup_source_unregister(ts_data->p_ws);
#endif
    kfree_safe(ts_data->touch_buf);
    kfree_safe(ts_data->bus_tx_buf);
    kfree_safe(ts_data->bus_rx_buf);
    kfree_safe(ts_data->pdata);

    FTS_FUNC_EXIT();
    return 0;
}
