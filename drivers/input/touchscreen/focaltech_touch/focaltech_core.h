/*
 *
 * FocalTech TouchScreen driver.
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
* File Name: focaltech_core.h

* Author: Focaltech Driver Team
*
* Created: 2016-08-08
*
* Abstract:
*
* Reference:
*
*****************************************************************************/

#ifndef __LINUX_FOCALTECH_CORE_H__
#define __LINUX_FOCALTECH_CORE_H__
/*****************************************************************************
* Included header files
*****************************************************************************/
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/i2c.h>
#include <linux/spi/spi.h>
#include <linux/input.h>
#include <linux/input/mt.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/gpio.h>
#include <linux/regulator/consumer.h>
#include <linux/uaccess.h>
#include <linux/firmware.h>
#include <linux/debugfs.h>
#include <linux/mutex.h>
#include <linux/workqueue.h>
#include <linux/wait.h>
#include <linux/jiffies.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/version.h>
#include <linux/types.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/dma-mapping.h>
#include <linux/miscdevice.h>
#include <linux/poll.h>
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 10, 0))
#include <linux/ktime.h>
#include <linux/timekeeping.h>
#endif
#include "focaltech_common.h"

/*****************************************************************************
* Private constant and macro definitions using #define
*****************************************************************************/
#define FTS_DRIVER_NAME                     "fts_ts"

#define FTS_MAX_POINTS_SUPPORT              10 /* constant value, can't be changed */
#define FTS_MAX_KEYS                        4
#define FTS_KEY_DIM                         10
#define FTS_COORDS_ARR_SIZE                 4
#define FTS_ONE_TCH_LEN                     6
#define FTS_ONE_TCH_LEN_V2                  8
#define FTS_TOUCH_DATA_LEN_V2  (FTS_MAX_POINTS_SUPPORT * FTS_ONE_TCH_LEN_V2 + 4)


#define FTS_TOUCH_DATA_LEN  (FTS_MAX_POINTS_SUPPORT * FTS_ONE_TCH_LEN + 2)

#define FTS_GESTURE_POINTS_MAX              6
#define FTS_GESTURE_DATA_LEN               (FTS_GESTURE_POINTS_MAX * 4 + 4)

#define FTS_SIZE_PEN                        15
#define FTS_SIZE_DEFAULT                    15
#define FTS_SIZE_DEFAULT_V2                 21


#define FTS_MAX_ID                          0x0A
#define FTS_TOUCH_OFF_E_XH                  0
#define FTS_TOUCH_OFF_XL                    1
#define FTS_TOUCH_OFF_ID_YH                 2
#define FTS_TOUCH_OFF_YL                    3
#define FTS_TOUCH_OFF_PRE                   4
#define FTS_TOUCH_OFF_AREA                  5
#define FTS_TOUCH_OFF_MINOR                 6

#define FTS_TOUCH_E_NUM                     1
#define FTS_X_MIN_DISPLAY_DEFAULT           0
#define FTS_Y_MIN_DISPLAY_DEFAULT           0
#define FTS_X_MAX_DISPLAY_DEFAULT           (720 - 1)
#define FTS_Y_MAX_DISPLAY_DEFAULT           (1280 - 1)

#define FTS_TOUCH_DOWN                      0
#define FTS_TOUCH_UP                        1
#define FTS_TOUCH_CONTACT                   2
#define EVENT_DOWN(flag)                    ((FTS_TOUCH_DOWN == flag) || (FTS_TOUCH_CONTACT == flag))
#define EVENT_UP(flag)                      (FTS_TOUCH_UP == flag)

#define FTS_MAX_COMPATIBLE_TYPE             8
#define FTS_MAX_COMMMAND_LENGTH             16

#define FTS_MAX_TOUCH_BUF                   4096
#define FTS_MAX_BUS_BUF                     4096

#define FTS_MAX_CUSTOMER_INFO               32
#define FTS_FOD_BUF_LEN                     9

#define FTS_RETVAL_IGNORE_TOUCHES           1


/*****************************************************************************
*  Alternative mode (When something goes wrong, the modules may be able to solve the problem.)
*****************************************************************************/
/*
 * For commnication error in PM(deep sleep) state
 */
#define FTS_PATCH_COMERR_PM                 0
#define FTS_TIMEOUT_COMERR_PM               700

/*
 * For high resolution
 * Set FTS_TOUCH_HIRES_EN to 1 to support high resolution reporting of touch finger.
 * Set FTS_PEN_HIRES_EN to 1 to support high resolution reporting of stylus pen.
 *
 * FTS_XXX_HIRES_X, a multiple relative to the original resolution
 * FTS_HI_RES_X_MAX, const value, can't be modified
 */
#define FTS_TOUCH_HIRES_EN                  0
#define FTS_TOUCH_HIRES_X                   10

#define FTS_PEN_HIRES_EN                    1
#define FTS_PEN_HIRES_X                     10


#define FTS_HI_RES_X_MAX                    16


/* If need read customer info when probing, max:FTS_MAX_CUSTOMER_INFO */
#define FTS_READ_CUSTOMER_INFO              0

/*****************************************************************************
* Private enumerations, structures and unions using typedef
*****************************************************************************/
struct ftxxxx_proc {
    struct proc_dir_entry *proc_entry;
    u8 opmode;
    u8 cmd_len;
    u8 cmd[FTS_MAX_COMMMAND_LENGTH];
};

struct fts_ts_platform_data {
    u32 irq_gpio;
    u32 irq_gpio_flags;
    u32 reset_gpio;
    u32 reset_gpio_flags;
    bool have_key;
    u32 key_number;
    u32 keys[FTS_MAX_KEYS];
    u32 key_y_coords[FTS_MAX_KEYS];
    u32 key_x_coords[FTS_MAX_KEYS];
    u32 x_max;
    u32 y_max;
    u32 x_min;
    u32 y_min;
    u32 max_touch_number;
};

struct ts_event {
    int x;      /*x coordinate */
    int y;      /*y coordinate */
    int p;      /* pressure */
    int flag;   /* touch event flag: 0 -- down; 1-- up; 2 -- contact */
    int id;     /*touch ID */
    int area;
    int minor;
};

struct pen_event {
    int down;
    int inrange;
    int tip;
    int x;      /*x coordinate */
    int y;      /*y coordinate */
    int p;      /* pressure */
    int flag;   /* touch event flag: 0 -- down; 1-- up; 2 -- contact */
    int id;     /*touch ID */
    int tilt_x;
    int tilt_y;
    int azimuth;
    int tool_type;
};

struct fts_ts_data {
    struct i2c_client *client;
    struct spi_device *spi;
    u32 spi_speed;
    struct device *dev;
    struct input_dev *input_dev;
    struct input_dev *pen_dev;
    struct fts_ts_platform_data *pdata;
    struct ts_ic_info ic_info;
    struct workqueue_struct *ts_workqueue;
    struct work_struct resume_work;
    struct delayed_work esdcheck_work;
    struct delayed_work prc_work;
    struct delayed_work fwdbg_work;
    wait_queue_head_t ts_waitqueue;
    struct ftxxxx_proc proc;
    struct ftxxxx_proc proc_ta;
    spinlock_t irq_lock;
    struct mutex report_mutex;
    struct mutex bus_lock;
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 4, 0))
    struct wakeup_source ws;
#endif
    struct wakeup_source *p_ws;
    unsigned long intr_jiffies;
    int irq;
    int log_level;
    int fw_is_running;      /* confirm fw is running when using spi:default 0 */
    int dummy_byte;
#if IS_ENABLED(CONFIG_PM) && FTS_PATCH_COMERR_PM
    struct completion pm_completion;
    bool pm_suspend;
#endif
    bool suspended;
    bool need_work_in_suspend;
    bool fw_loading;
    bool irq_disabled;
    bool power_disabled;
    bool glove_mode;
    bool cover_mode;
    bool charger_mode;
    bool earphone_mode;
    bool edgepalm_mode;
    bool touch_analysis_support;
    bool prc_support;
    bool prc_mode;
    bool esd_support;
    bool fod_mode;
    bool proximity_mode;
    bool fhp_mode;

    bool fwdbg_support;
    bool gesture_support;   /* gesture enable or disable, default: disable */
    u8 gesture_bmode;       /*gesture buffer mode*/

    int fod_fp_down;
    int edgepalm_value;
    int fwdbg_value;

    u8 pen_etype;
    struct pen_event pevent;
    struct ts_event events[FTS_MAX_POINTS_SUPPORT];    /* multi-touch */
    u8 touch_addr;
    u32 touch_size;
    u8 *touch_buf;
    int touch_event_num;
    int touch_points;
    int key_state;
    int ta_flag;
    u32 ta_size;
    u8 *ta_buf;

    u8 *bus_tx_buf;
    u8 *bus_rx_buf;
    int bus_type;
    int bus_ver;
    char customer_info[FTS_MAX_CUSTOMER_INFO];
    struct regulator *vdd;
    struct regulator *iovcc;
#if FTS_PINCTRL_EN
    struct pinctrl *pinctrl;
    struct pinctrl_state *pins_active;
    struct pinctrl_state *pins_suspend;
    struct pinctrl_state *pins_release;
#endif
    struct notifier_block fb_notif;
};

enum _FTS_BUS_TYPE {
    BUS_TYPE_NONE,
    BUS_TYPE_I2C,
    BUS_TYPE_SPI,
};

enum _FTS_BUS_VER {
    BUS_VER_DEFAULT = 1,
    BUS_VER_V2,
};

enum _FTS_TOUCH_ETYPE {
    TOUCH_DEFAULT = 0x00,
    TOUCH_PROTOCOL_v2 = 0x02,
    TOUCH_PEN = 0x0B,
    TOUCH_FW_INIT = 0x81,
    TOUCH_IGNORE = 0xFE,
    TOUCH_FWDBG = 0x0E,
    TOUCH_ERROR = 0xFF,
};

enum _FTS_STYLUS_ETYPE {
    STYLUS_DEFAULT,
    STYLUS_HOVER,
};

enum _FTS_GESTURE_BMODE {
    GESTURE_BM_REG,
    GESTURE_BM_TOUCH,
};

enum _FTS_FW_MODE {
    FW_MODE_NORMAL = 0xAA,
    FW_MODE_FACTORY = 0x55,
    FW_MODE_GESTURE = 0x66,
};


/*****************************************************************************
* Global variable or extern global variabls/functions
*****************************************************************************/
extern struct fts_ts_data *fts_data;


/* communication interface */
int fts_read(u8 *cmd, u32 cmdlen, u8 *data, u32 datalen);
int fts_read_reg(u8 addr, u8 *value);
int fts_write(u8 *writebuf, u32 writelen);
int fts_write_reg(u8 addr, u8 value);
int fts_bus_configure(struct fts_ts_data *ts_data, u8 *buf, u32 size);
int fts_bus_transfer_direct(u8 *writebuf, u32 writelen, u8 *readbuf, u32 readlen);
int fts_bus_set_speed(struct fts_ts_data *ts_data, u32 speed);
int fts_hid2std(int mode);
int fts_ts_probe_entry(struct fts_ts_data *ts_data);
int fts_ts_remove_entry(struct fts_ts_data *ts_data);

/* Gesture functions */
int fts_gesture_init(struct fts_ts_data *ts_data);
int fts_gesture_exit(struct fts_ts_data *ts_data);
void fts_gesture_recovery(struct fts_ts_data *ts_data);
int fts_gesture_readdata(struct fts_ts_data *ts_data, u8 *data);
int fts_gesture_suspend(struct fts_ts_data *ts_data);
int fts_gesture_resume(struct fts_ts_data *ts_data);

#if FTS_FOD_EN
void fts_fod_enable(int enable);
#endif

/* Apk and functions */
int fts_create_apk_debug_channel(struct fts_ts_data *);
void fts_release_apk_debug_channel(struct fts_ts_data *);

/* ADB functions */
int fts_create_sysfs(struct fts_ts_data *ts_data);
int fts_remove_sysfs(struct fts_ts_data *ts_data);

/* ESD */
int fts_esdcheck_init(struct fts_ts_data *ts_data);
int fts_esdcheck_exit(struct fts_ts_data *ts_data);
void fts_esdcheck_switch(struct fts_ts_data *ts_data, bool enable);
void fts_esdcheck_proc_busy(struct fts_ts_data *ts_data, bool proc_debug);
void fts_esdcheck_suspend(struct fts_ts_data *ts_data);
void fts_esdcheck_resume(struct fts_ts_data *ts_data);
bool fts_esdcheck_is_running(struct fts_ts_data *ts_data);


/* Host test */

/* Point Report Check*/
int fts_point_report_check_init(struct fts_ts_data *ts_data);
int fts_point_report_check_exit(struct fts_ts_data *ts_data);
void fts_prc_queue_work(struct fts_ts_data *ts_data);

/* FW upgrade */
int fts_fwupg_init(struct fts_ts_data *ts_data);
int fts_fwupg_exit(struct fts_ts_data *ts_data);
int fts_upgrade_bin(char *fw_name, bool force);
int fts_enter_test_environment(bool test_state);
int fts_enter_normal_fw(void);

/* Other */
void fts_msleep(unsigned long msecs);
int fts_set_reset(struct fts_ts_data *ts_data, int value);
int fts_reset_proc(struct fts_ts_data *ts_data, int force, int hdelayms);
int fts_check_cid(struct fts_ts_data *ts_data, u8 id_h);
int fts_wait_tp_to_valid(void);
void fts_release_all_finger(void);
void fts_tp_state_recovery(struct fts_ts_data *ts_data);
int fts_ex_mode_init(struct fts_ts_data *ts_data);
int fts_ex_mode_exit(struct fts_ts_data *ts_data);
int fts_ex_mode_recovery(struct fts_ts_data *ts_data);
int fts_input_report_buffer(struct fts_ts_data *ts_data, u8 *touch_buf);

void fts_irq_disable(void);
void fts_irq_enable(void);

#if FTS_PSENSOR_EN
int fts_proximity_init(struct fts_ts_data *ts_data);
int fts_proximity_exit(struct fts_ts_data *ts_data);
int fts_proximity_readdata(struct fts_ts_data *ts_data);
int fts_proximity_suspend(struct fts_ts_data *ts_data);
int fts_proximity_resume(struct fts_ts_data *ts_data);
int fts_proximity_recovery(struct fts_ts_data *ts_data);
#endif


#endif /* __LINUX_FOCALTECH_CORE_H__ */
