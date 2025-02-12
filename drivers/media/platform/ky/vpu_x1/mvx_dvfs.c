/*
 * The confidential and proprietary information contained in this file may
 * only be used by a person authorised under and to the extent permitted
 * by a subsisting licensing agreement from Arm Technology (China) Co., Ltd.
 *
 *            (C) COPYRIGHT 2021-2021 Arm Technology (China) Co., Ltd.
 *                ALL RIGHTS RESERVED
 *
 * This entire notice must be reproduced on all copies of this file
 * and copies of this file may only be made by a person if such person is
 * permitted to do so under the terms of a subsisting license agreement
 * from Arm Technology (China) Co., Ltd.
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

/****************************************************************************
 * Includes
 ****************************************************************************/
#include <linux/list.h>
#include <linux/device.h>
#include <linux/pm_qos.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/stat.h>
#include <linux/sysfs.h>
#include <linux/thermal.h>

#include "mvx_dvfs.h"
#include "mvx_log_group.h"
#include "mvx_session.h"

#define DVFS_INTERNAL_DEBUG
/** Default value for an interval between frequency updates in milliseconds.
 * It could be overwritten by user in debug build when sysfs is enabled.
 */
#define POLL_INTERVAL_MS 100

/* Adjustment step in percents of maximum supported frequency */
#define UP_STEP_PERCENT 25
#define DOWN_STEP_PERCENT 13
#define DVFS_FREQ_MAX  819200000

#if defined(CONFIG_SYSFS) && defined(DVFS_INTERNAL_DEBUG)
#define DVFS_DEBUG_MODE 1
#else
#define DVFS_DEBUG_MODE 0
#endif

#define VPU_DDR_QOS_ENABLE 0
#define VPU_DDR_QOS_MIN  50000   /* 50MB/s */
#define VPU_DDR_QOS_MAX  4096000  /* 4GB/s */
#define VPU_DDR_QOS_PREDEFINED_FPS 30

#define NELEMS(a) (sizeof(a) / sizeof((a)[0]))

extern int session_wait_pending_timeout;
extern int session_watchdog_timeout;

/**
 * Structure used by DVFS module to keep track of session usage and to
 * take decisions about power management.
 *
 * Currently the only parameter taken into consideration is an amount of
 * output buffers enqueued in FW for each session. DVFS tries to keep this
 * parameter equal to 1 for all sessions. If some session has more than one
 * enqueued buffer, it means that a client is waiting for more than one
 * frame and the clock frequency should be increased. If some session has
 * no buffers enqueued, it means that the client is not waiting for
 * anything and the clock frequency could be decreased. Priority is given
 * to frequency increasing (when more than one session is registered).
 */
struct session
{
    mvx_session_id session_id;
    struct list_head list;
    bool is_encoder;
    uint32_t ddr_qos_read;
    uint32_t ddr_qos_write;
    int restrict_buffer_count;
};

/* Avaible VPU frequency. */
enum {
    VPU_VMIN_LEVEL_0 = 0,
    VPU_VMIN_LEVEL_1,
    VPU_VMIN_LEVEL_2,
    VPU_VMIN_LEVEL_3,
    VPU_VMIN_LEVEL_4,
    VPU_VMIN_LEVEL_5,
    VPU_VMIN_LEVEL_6,
};
struct vpu_freq_vmin_info
{
    uint32_t freq;
    uint32_t vmin_level;
};

/* for dvfs and ddr qos adjust. */
struct mvx_dvfs_ctx_t
{
    /* ddr qos params */
    uint32_t ddr_qos_rsum;
    uint32_t ddr_qos_wsum;

    /* Frequency limits */
    struct clk* clock;
    uint32_t max_freq;
    uint32_t min_freq;
    uint32_t up_step_freq;
    uint32_t down_step_freq;
    /**
     * DVFS polling interval - an interval between frequency updates in milliseconds.
     * It is a constant value for non-debug and non-sysfs builds.
     */
    uint32_t poll_interval_ms;

#ifdef CONFIG_THERMAL
    /* thermal restriction */
    unsigned long max_state;
    unsigned long cur_state;
    struct thermal_cooling_device *cdev;
#endif

    bool sched_suspend;
};

/* A list containing all registered sessions */
static LIST_HEAD(sessions);

struct device *mvx_device;

/* Flag used to prevent usage of DVFS module when it was not initialized */
static bool initialized = false;

/* Flag used to indicate that DVFS module is going to shut itself down */
static bool shutdown = false;

/* Semaphore used to prevent concurrent access to DVFS internal structures */
static struct semaphore dvfs_sem;

/* DVFS polling task */
static struct task_struct *dvfs_task = NULL;
static wait_queue_head_t dvfs_wq;

#if (1 == DVFS_DEBUG_MODE)
/**
 * Counters used for debugging/verification purposes.
 */

/* Flag used to enable/disable DVFS in debug builds */
static atomic_t dvfs_enabled = ATOMIC_INIT(1);

#if (1 == VPU_DDR_QOS_ENABLE)
static atomic_t ddr_qos_enabled = ATOMIC_INIT(1);
#endif

/* Amount of times clock frequency was changed by DVFS */
static atomic_long_t changes_cnt = ATOMIC_LONG_INIT(0);

/* Amount of times burst mode was used by DVFS */
static atomic_long_t burst_cnt = ATOMIC_LONG_INIT(0);
#endif

static const struct vpu_freq_vmin_info vpufclk_freqtable[] =
{
    {307200000, VPU_VMIN_LEVEL_0},
    {409600000, VPU_VMIN_LEVEL_1},
    {491520000, VPU_VMIN_LEVEL_2},
    {600000000, VPU_VMIN_LEVEL_3},
    {614400000, VPU_VMIN_LEVEL_4},
    {750000000, VPU_VMIN_LEVEL_5},
    {819200000, VPU_VMIN_LEVEL_6}
};

#define  FREQ_TABLE_SIZE (sizeof(vpufclk_freqtable)/sizeof(struct vpu_freq_vmin_info))

static struct mvx_dvfs_ctx_t mvx_dvfs_ctx;

static void set_clock_rate(uint32_t clk_rate)
{
    //clk_set_rate(mvx_dvfs_ctx.clock, clk_rate);
}

static uint32_t get_clock_rate(void)
{
    return 0;
    //return clk_get_rate(mvx_dvfs_ctx.clock);
}

static uint32_t get_max_clock_rate(void)
{
    return DVFS_FREQ_MAX;
}

/**
 * Allocate and register a session in DVFS module.
 *
 * This function allocates needed resources for the session and registers
 * it in the module.
 *
 * This function must be called when dvfs_sem semaphore IS locked.
 *
 * @param session_is Session id
 * @return True when registration was successful,
 *         False otherwise.
 */
static bool allocate_session(const mvx_session_id session_id, bool is_encoder)
{
    struct session *session;

    session = devm_kzalloc(mvx_device, sizeof(*session), GFP_KERNEL);
    if (NULL == session)
    {
        MVX_LOG_PRINT(&mvx_log_dev, MVX_LOG_WARNING,
                "DVFS is unable to allocate memory for a new session. session=%p",
                session_id);
        return false;
    }

    session->session_id = session_id;
    session->ddr_qos_read  = session->ddr_qos_write = 0;
    session->is_encoder = is_encoder;

    INIT_LIST_HEAD(&session->list);
    list_add(&session->list, &sessions);

    return true;
}

/**
 * Unregister a session from DVFS module.
 *
 * When session is not NULL, the function releases all previously allocated
 * resources for the session and unregisters it from DVFS.
 *
 * This function must be called when dvfs_sem semaphore IS locked.
 *
 * @param session Session or NULL
 */
static void free_session(struct session *session)
{
    if (NULL == session)
    {
        return;
    }

    list_del(&session->list);
    devm_kfree(mvx_device, session);
}

/**
 * Find a session with provided session_id.
 *
 * This function tries to find previously registered session with provided
 * session_id.
 *
 * This function must be called when dvfs_sem semaphore IS locked.
 *
 * @param session_id Session id
 * @return pointer to session structure when a session was found,
 *         NULL when a session was not found.
 */
static struct session *get_session(const mvx_session_id session_id)
{
    struct list_head *entry;
    struct session *session;
    list_for_each(entry, &sessions)
    {
        session = list_entry(entry, struct session, list);
        if (session->session_id == session_id)
        {
            return session;
        }
    }
    return NULL;
}

/**
 * Warm up VPU.
 *
 * This function increases VPU clock frequency for requested amount
 * of steps when possible.
 *
 * @param steps Requested amount of steps.
 */
static void warm_up(const int steps)
{
    uint32_t old_freq = get_clock_rate();
    uint32_t new_freq;
#if (1 == DVFS_DEBUG_MODE)
    bool do_burst = false;
#endif

    /**
     * If 3 or more steps are requested, we are far behind required
     * performance level.
     */
    if (steps > 2)
    {
        new_freq = mvx_dvfs_ctx.max_freq;
#if (1 == DVFS_DEBUG_MODE)
        do_burst = true;
#endif
    }
    else
    {
        new_freq = min(old_freq + steps * mvx_dvfs_ctx.up_step_freq, mvx_dvfs_ctx.max_freq);
    }

    if (old_freq != new_freq)
    {
        MVX_LOG_PRINT(&mvx_log_dev, MVX_LOG_INFO, "warm_up. buffer count: %d, old_freq: %d, new_freq: %d", steps, old_freq, new_freq);
        set_clock_rate(new_freq);
#if (1 == DVFS_DEBUG_MODE)
        atomic_long_inc(&changes_cnt);
        if (do_burst)
        {
            atomic_long_inc(&burst_cnt);
        }
#endif

    }
}

/**
 * Cool down VPU.
 *
 * This function increases VPU clock frequency if possible.
 */
static void cool_down(void)
{
    uint32_t old_freq = get_clock_rate();
    uint32_t new_freq;
    if (old_freq == mvx_dvfs_ctx.min_freq)
    {
        return;
    }
    new_freq = max(mvx_dvfs_ctx.min_freq, max(mvx_dvfs_ctx.down_step_freq, old_freq - mvx_dvfs_ctx.down_step_freq));
    if (old_freq != new_freq)
    {
        MVX_LOG_PRINT(&mvx_log_dev, MVX_LOG_INFO, "cool_down. old_freq: %d, new_freq: %d", old_freq, new_freq);
        set_clock_rate(new_freq);
#if (1 == DVFS_DEBUG_MODE)
        atomic_long_inc(&changes_cnt);
#endif
    }
}

static int get_restrict_buffer_count(mvx_session_id session_id)
{
    struct mvx_session *session = (struct mvx_session *)session_id;
    int buffers_cnt;
    struct session* dvfs_session = get_session(session_id);
    if (dvfs_session == NULL)
    {
        return -1;
    }

    /* Don't have to lock the session since we just want to get the number of buffers */
    if (!dvfs_session->is_encoder)
    {
        buffers_cnt = session->port[MVX_DIR_OUTPUT].buffer_count;
        buffers_cnt -= session->port[MVX_DIR_OUTPUT].buffer_on_hold_count;

        /* There is no enough inputs, no need to boost vpu. */
        if (session->port[MVX_DIR_INPUT].buffer_count <= 1)
        {
            buffers_cnt = session->port[MVX_DIR_INPUT].buffer_count;
        }
        //MVX_LOG_PRINT(&mvx_log_dev, MVX_LOG_INFO, "get_restrict_buffer_count. out buffer_count: %d, buffer_on_hold_count: %d, in buffer_count: %d", session->port[MVX_DIR_OUTPUT].buffer_count, session->port[MVX_DIR_OUTPUT].buffer_on_hold_count, session->port[MVX_DIR_INPUT].buffer_count);
    }
    else
    {
        buffers_cnt = session->port[MVX_DIR_INPUT].buffer_count;
        if (false != session->eos_queued && buffers_cnt < 2)
        {
            buffers_cnt = 2;
        }
        //MVX_LOG_PRINT(&mvx_log_dev, MVX_LOG_INFO, "get_restrict_buffer_count. buffer_count: %d, eos_queued: %d", session->port[MVX_DIR_INPUT].buffer_count, session->eos_queued);
    }

    if (session->keep_freq_high) {
        buffers_cnt += 2;
    }

    return buffers_cnt;
}

/**
 * Update sessions list and VPU clock frequency.
 *
 * This function queries the state of all registered sessions and adjusts
 * VPU clock frequency to meet their needs when dvfs_control is enabled.
 * When SYSFS is enabled, the function also stores the status of all sessions
 * so it could be retrieved by the user.
 *
 * This function must be called when dvfs_sem semaphore IS NOT locked.
 */
static void update_sessions(void)
{
    struct list_head *entry;
    struct list_head *safe;
    struct session *session;
    int  restrict_buffer_count;
    unsigned int buf_max = 0;
    unsigned int buf_min = UINT_MAX;
    int sem_failed;

    sem_failed = down_interruptible(&dvfs_sem);
    if (sem_failed)
    {
        return;
    }

    if (mvx_dvfs_ctx.sched_suspend == true) {
        up(&dvfs_sem);
        return;
    }

    list_for_each_safe(entry, safe, &sessions)
    {
        session = list_entry(entry, struct session, list);

        /**
         * To avoid potential dead lock we release dvfs_sem before a call to
         * get_session_status() callback. After a return from the callback
         * we have to take dvfs_sem again and to verify that current session
         * was not unregistered by the scheduler while we were sleeping.
         */
        restrict_buffer_count = get_restrict_buffer_count(session->session_id);
        session->restrict_buffer_count = restrict_buffer_count;

        if (shutdown)
        {
            up(&dvfs_sem);
            return;
        }

        if (restrict_buffer_count < 0)
        {
            //MVX_LOG_PRINT(&mvx_log_dev, MVX_LOG_WARNING,
            //        "DVFS failed to retrieve status for the session. Session was removed? session=%p, restrict_buffer_count=%d",
            //        session->session_id, restrict_buffer_count);
            continue;
        }

        if (restrict_buffer_count > buf_max)
        {
            buf_max = restrict_buffer_count;
        }
        if (restrict_buffer_count < buf_min)
        {
            buf_min = restrict_buffer_count;
        }
    }

#if (1 == DVFS_DEBUG_MODE)
    if (0 == atomic_read(&dvfs_enabled))
    {
        up(&dvfs_sem);
        return;
    }
#endif

    if (buf_max > 1)
    {
        warm_up(buf_max);
    }
    else if (buf_min < 1)
    {
        cool_down();
    }
    up(&dvfs_sem);
}

/**
 * DVFS polling thread.
 *
 * This function is executed in a separate kernel thread. It updates clock
 * frequency every poll_interval_ms milliseconds.
 */
static int dvfs_thread(void *v)
{
    MVX_LOG_PRINT(&mvx_log_dev, MVX_LOG_INFO, "DVFS polling thread started");
    while (!kthread_should_stop())
    {
        wait_event_interruptible(dvfs_wq, list_empty(&sessions) == 0 || shutdown);
        update_sessions();
        msleep_interruptible(mvx_dvfs_ctx.poll_interval_ms);
    }

    MVX_LOG_PRINT(&mvx_log_dev, MVX_LOG_INFO, "DVFS polling thread finished");
    return 0;
}

/**
 *  Return percent percents from a value val.
 */
static uint32_t ratio(const uint32_t val, const uint32_t percent)
{
    return (uint32_t)(((uint64_t)val * percent) / 100);
}

#if (1 == DVFS_DEBUG_MODE)
/**
 * Print DVFS statistics to sysfs attribute.
 *
 * Used for debugging/verification purposes.
 */
static ssize_t sysfs_print_stats(struct device *dev,
        struct device_attribute *attr,
        char *buf)
{
    ssize_t num = 0;
    struct list_head *entry;
    struct session *session;
    uint32_t freq = get_clock_rate();

    num += scnprintf(buf + num, PAGE_SIZE - num,
            "freq: %4u, max_freq: %4u, up_step_freq: %3u, down_step_freq: %3u",
            freq, mvx_dvfs_ctx.max_freq, mvx_dvfs_ctx.up_step_freq, mvx_dvfs_ctx.down_step_freq);
#if (1 == DVFS_DEBUG_MODE)
    num += scnprintf(buf + num, PAGE_SIZE - num,
            ", enabled: %1u, poll_interval_ms: %3u, changes_cnt: %10lu, burst_cnt: %10lu",
            atomic_read(&dvfs_enabled), mvx_dvfs_ctx.poll_interval_ms,
            atomic_long_read(&changes_cnt), atomic_long_read(&burst_cnt));
#endif
    num += scnprintf(buf + num, PAGE_SIZE - num, "\n");
    list_for_each(entry, &sessions)
    {
        session = list_entry(entry, struct session, list);
        num += scnprintf(buf + num, PAGE_SIZE - num,
                "%p: out_buf: %02u\n",
                session->session_id, session->restrict_buffer_count);
    }

    return num;
}

/**
 * Print DVFS enabling status to sysfs attribute.
 *
 * Used for debugging/verification purposes.
 */
static ssize_t sysfs_print_enabled(struct device *dev,
        struct device_attribute *attr,
        char *buf)
{
    ssize_t num = 0;
    num += scnprintf(buf, PAGE_SIZE, "%u\n", atomic_read(&dvfs_enabled) ? 1 : 0);
    return num;
}

/**
 * Set DVFS enabling status from sysfs attribute.
 *
 * Used for debugging/verification purposes.
 */
ssize_t sysfs_set_enabled(struct device *dev, struct device_attribute *attr,
        const char *buf, size_t count)
{
    int failed;
    int enabled;
    failed = kstrtouint(buf, 10, &enabled);
    if (!failed)
    {
        atomic_set(&dvfs_enabled, enabled);
    }
    return (failed) ? failed : count;
}

/**
 * Print current clock frequency to sysfs attribute.
 *
 * Used for debugging/verification purposes.
 */
static ssize_t sysfs_print_freq(struct device *dev,
        struct device_attribute *attr,
        char *buf)
{
    ssize_t num = 0;
    uint32_t freq = get_clock_rate();
    num += scnprintf(buf, PAGE_SIZE, "%u\n", freq);
    return num;
}

/**
 * Set current clock frequency from sysfs attribute.
 *
 * Used for debugging/verification purposes.
 */
ssize_t sysfs_set_freq(struct device *dev, struct device_attribute *attr,
        const char *buf, size_t count)
{
    int failed;
    unsigned int freq;
    failed = kstrtouint(buf, 10, &freq);
    if (!failed)
    {
        set_clock_rate((uint32_t)freq);
    }
    return (failed) ? failed : count;
}

/**
 * Print min clock frequency to sysfs attribute.
 *
 * Used for debugging/verification purposes.
 */
static ssize_t sysfs_print_min_freq(struct device *dev,
        struct device_attribute *attr,
        char *buf)
{
    ssize_t num = 0;
    uint32_t freq = mvx_dvfs_ctx.min_freq;
    num += scnprintf(buf, PAGE_SIZE, "%u\n", freq);
    return num;
}

uint32_t clip_min_max_rate(uint32_t freq, bool is_min_freq)
{
    int i = 0;
    bool clip = false;
    uint32_t clip_freq;
    if (is_min_freq) {
        for (i = 0; i < FREQ_TABLE_SIZE; i++)
        {
            if (freq <= vpufclk_freqtable[i].freq)
            {
                clip = true;
                clip_freq = vpufclk_freqtable[i].freq;
                break;
            }
        }
        if (!clip) clip_freq = vpufclk_freqtable[FREQ_TABLE_SIZE-1].freq;
    } else {
        for (i = FREQ_TABLE_SIZE-1; i >= 0; i--)
        {
            if (freq >= vpufclk_freqtable[i].freq)
            {
                clip = true;
                clip_freq = vpufclk_freqtable[i].freq;
                break;
            }
        }
        if (!clip) clip_freq = vpufclk_freqtable[0].freq;
    }

    return clip_freq;
}

/**
 * Set min clock frequency from sysfs attribute.
 *
 * Used for debugging/verification purposes.
 */
ssize_t sysfs_set_min_freq(struct device *dev, struct device_attribute *attr,
        const char *buf, size_t count)
{
    int failed;
    unsigned int freq;
    failed = kstrtouint(buf, 10, &freq);
    freq = clip_min_max_rate(freq, true);
    if (!failed)
    {
        mvx_dvfs_ctx.min_freq = (uint32_t)freq;
    }
    return (failed) ? failed : count;
}

/**
 * Print max clock frequency to sysfs attribute.
 *
 * Used for debugging/verification purposes.
 */
static ssize_t sysfs_print_max_frep(struct device *dev,
        struct device_attribute *attr,
        char *buf)
{
    ssize_t num = 0;
    uint32_t freq = mvx_dvfs_ctx.max_freq;
    num += scnprintf(buf, PAGE_SIZE, "%u\n", freq);
    return num;
}

/**
 * Set max clock frequency from sysfs attribute.
 *
 * Used for debugging/verification purposes.
 */
ssize_t sysfs_set_max_freq(struct device *dev, struct device_attribute *attr,
        const char *buf, size_t count)
{
    int failed;
    unsigned int freq;
    failed = kstrtouint(buf, 10, &freq);
    freq = clip_min_max_rate(freq, false);
    if (!failed)
    {
        mvx_dvfs_ctx.max_freq = (uint32_t)freq;
        mvx_dvfs_ctx.up_step_freq = ratio(mvx_dvfs_ctx.max_freq, UP_STEP_PERCENT);
        mvx_dvfs_ctx.down_step_freq = ratio(mvx_dvfs_ctx.max_freq, DOWN_STEP_PERCENT);
    }
    return (failed) ? failed : count;
}

/**
 * Set polling interval from sysfs attribute.
 *
 * Used for debugging/verification purposes.
 */
ssize_t sysfs_set_poll_interval_ms(struct device *dev,
        struct device_attribute *attr,
        const char *buf, size_t count)
{
    int failed;
    failed = kstrtouint(buf, 10, &mvx_dvfs_ctx.poll_interval_ms);
    return (failed) ? failed : count;
}

/**
 * Set up_step value from sysfs attribute.
 *
 * Used for debugging/verification purposes.
 */
ssize_t sysfs_set_up_step_percent(struct device *dev, struct device_attribute *attr,
        const char *buf, size_t count)
{
    int failed;
    unsigned int up_step_percent;
    failed = kstrtouint(buf, 10, &up_step_percent);
    if (!failed)
    {
        mvx_dvfs_ctx.up_step_freq = ratio(mvx_dvfs_ctx.max_freq, up_step_percent);
    }
    return (failed) ? failed : count;
}

/**
 * Set down_step value from sysfs attribute.
 *
 * Used for debugging/verification purposes.
 */
ssize_t sysfs_set_down_step_percent(struct device *dev, struct device_attribute *attr,
        const char *buf, size_t count)
{
    int failed;
    unsigned int down_step_percent;
    failed = kstrtouint(buf, 10, &down_step_percent);
    if (!failed)
    {
        mvx_dvfs_ctx.down_step_freq = ratio(mvx_dvfs_ctx.max_freq, down_step_percent);
    }
    return (failed) ? failed : count;
}
/**
 * Print available clock frequency to sysfs attribute.
 *
 * Used for debugging/verification purposes.
 */
static ssize_t sysfs_print_available_freq(struct device *dev,
        struct device_attribute *attr,
        char *buf)
{
    ssize_t num = 0;
    int32_t i;
    for(i=0; i<FREQ_TABLE_SIZE; i++)
    {
        num += scnprintf(buf + num, PAGE_SIZE - num, "%u	", vpufclk_freqtable[i].freq);
    }
    num += scnprintf(buf + num, PAGE_SIZE - num, "\n");
    return num;
}

#if (1 == VPU_DDR_QOS_ENABLE)
static ssize_t sysfs_print_ddr_qos_enable(struct device *dev,
        struct device_attribute *attr,
        char *buf)
{
    ssize_t num = 0;
    num += snprintf(buf, PAGE_SIZE, "%u\n", atomic_read(&ddr_qos_enabled) ? 1 : 0);
    return num;
}

static ssize_t sysfs_set_ddr_qos_enable(struct device *dev,
        struct device_attribute *attr,
        const char *buf,
        size_t count)
{
    int failed;
    int enable;
    failed = kstrtouint(buf, 10, &enable);
    if (!failed)
    {
        atomic_set(&ddr_qos_enabled, enable);
    }
    return (failed) ? failed : count;
}
#endif

/**
 * Print watchdog timeout value to sysfs attribute.
 *
 * Used for debugging/verification purposes.
 */
static ssize_t sysfs_print_watchdog_timeout(struct device *dev,
        struct device_attribute *attr,
        char *buf)
{
    ssize_t num = 0;
    uint32_t watchdog_timeout = session_watchdog_timeout;
    num += scnprintf(buf, PAGE_SIZE, "%u\n", watchdog_timeout);
    return num;
}

/**
 * Set watchdog timeout value from sysfs attribute.
 *
 * Used for debugging/verification purposes.
 */
ssize_t sysfs_set_watchdog_timeout(struct device *dev, struct device_attribute *attr,
        const char *buf, size_t count)
{
    int failed;
    unsigned int watchdog_timeout;
    failed = kstrtouint(buf, 10, &watchdog_timeout);
    if (!failed)
    {
        session_watchdog_timeout = watchdog_timeout;
    }
    return (failed) ? failed : count;
}

/**
 * Print wait pending timeout value to sysfs attribute.
 *
 * Used for debugging/verification purposes.
 */
static ssize_t sysfs_print_wait_pending_timeout(struct device *dev,
        struct device_attribute *attr,
        char *buf)
{
    ssize_t num = 0;
    uint32_t wait_pending_timeout = session_wait_pending_timeout;
    num += scnprintf(buf, PAGE_SIZE, "%u\n", wait_pending_timeout);
    return num;
}

/**
 * Set wait pending timeout value from sysfs attribute.
 *
 * Used for debugging/verification purposes.
 */
ssize_t sysfs_set_wait_pending_timeout(struct device *dev, struct device_attribute *attr,
        const char *buf, size_t count)
{
    int failed;
    unsigned int wait_pending_timeout;
    failed = kstrtouint(buf, 10, &wait_pending_timeout);
    if (!failed)
    {
        session_wait_pending_timeout = wait_pending_timeout;
    }
    return (failed) ? failed : count;
}

/* Sysfs attributes used to debug/verify DVFS module */
static struct device_attribute sysfs_files[] =
{
    __ATTR(dvfs_stats, S_IRUGO, sysfs_print_stats, NULL),
    __ATTR(dvfs_enable, (S_IRUGO | S_IWUSR), sysfs_print_enabled, sysfs_set_enabled),
    __ATTR(dvfs_freq,	(S_IRUGO | S_IWUSR), sysfs_print_freq, sysfs_set_freq),
    __ATTR(dvfs_poll_interval_ms,   S_IWUSR, NULL, sysfs_set_poll_interval_ms),
    __ATTR(dvfs_up_step_percent,    S_IWUSR, NULL, sysfs_set_up_step_percent),
    __ATTR(dvfs_down_step_percent,  S_IWUSR, NULL, sysfs_set_down_step_percent),
    __ATTR(dvfs_available_freqency,	  S_IRUGO, sysfs_print_available_freq, NULL),
    __ATTR(dvfs_min_freq, (S_IRUGO | S_IWUSR), sysfs_print_min_freq, sysfs_set_min_freq),
    __ATTR(dvfs_max_freq, (S_IRUGO | S_IWUSR), sysfs_print_max_frep, sysfs_set_max_freq),
#if (1 == VPU_DDR_QOS_ENABLE)
    __ATTR(ddr_qos_enable, (S_IRUGO | S_IWUSR), sysfs_print_ddr_qos_enable, sysfs_set_ddr_qos_enable),
#endif
    __ATTR(watchdog_timeout, (S_IRUGO | S_IWUSR), sysfs_print_watchdog_timeout, sysfs_set_watchdog_timeout),
    __ATTR(wait_pending_timeout, (S_IRUGO | S_IWUSR), sysfs_print_wait_pending_timeout, sysfs_set_wait_pending_timeout),
};

/**
 * Register all DVFS attributes in sysfs subsystem
 */
static void sysfs_register_devices(struct device *dev)
{
    int err;
    int i = NELEMS(sysfs_files);

    while (i--)
    {
        err = device_create_file(dev, &sysfs_files[i]);
        if (err < 0)
        {
            MVX_LOG_PRINT(&mvx_log_dev, MVX_LOG_ERROR,
                    "DVFS is unable to create sysfs file. name=%s",
                    sysfs_files[i].attr.name);
        }
    }
}

/**
 * Remove DVFS attributes from sysfs subsystem
 */
static void sysfs_unregister_devices(struct device *dev)
{
    int i = NELEMS(sysfs_files);

    while (i--)
    {
        device_remove_file(dev, &sysfs_files[i]);
    }
}
#endif /* DVFS_DEBUG_MODE */

#ifdef CONFIG_THERMAL
static int vpu_get_max_state(struct thermal_cooling_device *cdev,
        unsigned long *state)
{
    struct mvx_dvfs_ctx_t *ctx = cdev->devdata;

    *state = ctx->max_state;
    return 0;
}

static int vpu_get_cur_state(struct thermal_cooling_device *cdev,
        unsigned long *state)
{
    struct mvx_dvfs_ctx_t *ctx = cdev->devdata;

    *state = ctx->cur_state;
    return 0;
}

static int vpu_set_cur_state(struct thermal_cooling_device *cdev,
        unsigned long state)
{
    struct mvx_dvfs_ctx_t *ctx = cdev->devdata;

    if (state > ctx->max_state)
        return -EINVAL;

    if (ctx->cur_state == state)
        return 0;

    ctx->max_freq = vpufclk_freqtable[FREQ_TABLE_SIZE - state - 1].freq;
    ctx->cur_state = state;
    ctx->up_step_freq = ratio(mvx_dvfs_ctx.max_freq, UP_STEP_PERCENT);
    ctx->down_step_freq = ratio(mvx_dvfs_ctx.max_freq, DOWN_STEP_PERCENT);
    return 0;
}

__maybe_unused static struct thermal_cooling_device_ops vpu_cooling_ops = {
    .get_max_state		= vpu_get_max_state,
    .get_cur_state		= vpu_get_cur_state,
    .set_cur_state		= vpu_set_cur_state,
};
#endif

/**
 * Initialize the DVFS module.
 *
 * Must be called before any other function in this module.
 *
 * @param dev Device
 */
void mvx_dvfs_init(struct device *dev)
{
    if (!initialized)
    {
        int i;
        int min_vmin_level;
        sema_init(&dvfs_sem, 1);

        mvx_dvfs_ctx.max_freq = get_max_clock_rate();
        mvx_dvfs_ctx.up_step_freq = ratio(mvx_dvfs_ctx.max_freq, UP_STEP_PERCENT);
        mvx_dvfs_ctx.down_step_freq = ratio(mvx_dvfs_ctx.max_freq, DOWN_STEP_PERCENT);
        mvx_dvfs_ctx.min_freq = vpufclk_freqtable[0].freq;
        min_vmin_level = vpufclk_freqtable[0].vmin_level;
        mvx_dvfs_ctx.sched_suspend = false;

        /*Use the max clk freq with min vmin as bottom freq of dvfs */
        for (i=1; i<FREQ_TABLE_SIZE; i++)
        {
            if (min_vmin_level < vpufclk_freqtable[i].vmin_level)
            {
                break;
            }
            mvx_dvfs_ctx.min_freq = vpufclk_freqtable[i].freq;
        }

        //mvx_dvfs_ctx.clock = devm_clk_get(dev, NULL);
        mvx_dvfs_ctx.poll_interval_ms = POLL_INTERVAL_MS;

        init_waitqueue_head(&dvfs_wq);
        dvfs_task = kthread_run(dvfs_thread, NULL, "dvfs");

#if (1 == DVFS_DEBUG_MODE)
        if (NULL != dev && IS_ENABLED(CONFIG_DEBUG_FS))
        {
            sysfs_register_devices(dev);
        }
#endif

        initialized = true;
        shutdown = false;
        mvx_device = dev;

    }
    else
    {
        MVX_LOG_PRINT(&mvx_log_dev, MVX_LOG_WARNING, "Attempt to initialize DVFS twice");
    }
}

/**
 * Deinitialize the DVFS module.
 *
 * All remaining sessions will be unregistered.
 *
 * @param dev Device
 */
void mvx_dvfs_deinit(struct device *dev)
{
    int sem_failed;
    struct list_head *entry;
    struct list_head *safe;
    struct session *session;

    if (!initialized)
    {
        MVX_LOG_PRINT(&mvx_log_dev, MVX_LOG_WARNING,
                "Attempt to deinitialize DVFS when it was not initialized");
        return;
    }

#ifdef CONFIG_THERMAL
    if (mvx_dvfs_ctx.cdev) {
        thermal_cooling_device_unregister(mvx_dvfs_ctx.cdev);
        mvx_dvfs_ctx.cdev = NULL;
    }
#endif

    sem_failed = down_interruptible(&dvfs_sem);
    shutdown = true;
    if (!sem_failed)
    {
        up(&dvfs_sem);
    }

    wake_up_interruptible(&dvfs_wq);
    if (!IS_ERR_OR_NULL(dvfs_task))
    {
        kthread_stop(dvfs_task);
    }

    sem_failed = down_interruptible(&dvfs_sem);
    list_for_each_safe(entry, safe, &sessions)
    {
        session = list_entry(entry, struct session, list);
        free_session(session);
    }

#if (1 == DVFS_DEBUG_MODE)
    if (NULL != dev && IS_ENABLED(CONFIG_DEBUG_FS))
    {
        sysfs_unregister_devices(dev);
    }
#endif

    //devm_clk_put(mvx_device, mvx_dvfs_ctx.clock);
    initialized = false;
    mvx_device = NULL;
    if (!sem_failed)
    {
        up(&dvfs_sem);
    }
}

/**
 * Register session in the DFVS module.
 *
 * @param session_id Session id
 * @return True when registration was successful,
 *         False, otherwise
 */
bool mvx_dvfs_register_session(const mvx_session_id session_id, bool is_encoder)
{
    bool success = false;
    int sem_failed;

    if (!initialized)
    {
        MVX_LOG_PRINT(&mvx_log_dev, MVX_LOG_WARNING,
                "DVFS module was not initialized");
        return false;
    }

    sem_failed = down_interruptible(&dvfs_sem);
    if (sem_failed)
    {
        MVX_LOG_PRINT(&mvx_log_dev, MVX_LOG_WARNING,
                "DVFS semaphore was not obtained, sem_failed=%d", sem_failed);
        return false;
    }

    if (shutdown)
    {
        up(&dvfs_sem);
        return false;
    }

    if (get_session(session_id) != NULL)
    {
        MVX_LOG_PRINT(&mvx_log_dev, MVX_LOG_INFO,
                "this session is already registered. session=%p",
                session_id);
        up(&dvfs_sem);
        return true;
    }

    MVX_LOG_PRINT(&mvx_log_dev, MVX_LOG_INFO,
            "mvx_dvfs_register_session. session=%p", session_id);

    success = allocate_session(session_id, is_encoder);
    up(&dvfs_sem);

    if (success)
    {
        bool adjust = true;
#if (1 == DVFS_DEBUG_MODE)
        /* Has DVFS been disabled through the sysfs interface? */
        adjust = atomic_read(&dvfs_enabled);
#endif
        if (adjust) {
            set_clock_rate(mvx_dvfs_ctx.max_freq);
        }
    }
    wake_up_interruptible(&dvfs_wq);

    return success;
}

/**
 * Unregister session from the DFVS module.
 *
 * Usage of corresponding session is not permitted after this call.
 * @param session_id Session id
 */
void mvx_dvfs_unregister_session(const mvx_session_id session_id)
{
    struct session *session;
    int sem_failed;

    if (!initialized)
    {
        MVX_LOG_PRINT(&mvx_log_dev, MVX_LOG_WARNING,
                "DVFS module was not initialized");
        return;
    }

    sem_failed = down_interruptible(&dvfs_sem);
    if (sem_failed)
    {
        MVX_LOG_PRINT(&mvx_log_dev, MVX_LOG_WARNING,
                "DVFS semaphore was not obtained, %d",
                sem_failed);
        return;
    }

    session = get_session(session_id);
    if (NULL != session)
    {
#if (1 == VPU_DDR_QOS_ENABLE)
        if ((session->ddr_qos_read + session->ddr_qos_write) &&
                mvx_dvfs_ctx.ddr_qos_rsum >= session->ddr_qos_read &&
                mvx_dvfs_ctx.ddr_qos_wsum >= session->ddr_qos_write)
        {
            mvx_dvfs_ctx.ddr_qos_rsum -= session->ddr_qos_read;
            mvx_dvfs_ctx.ddr_qos_wsum -= session->ddr_qos_write;
            MVX_LOG_PRINT(&mvx_log_dev, MVX_LOG_INFO,
                    "DVFS remove session[%p] ddr qos: [%d, %d]/[%d, %d]", session_id,  session->ddr_qos_read, session->ddr_qos_write, mvx_dvfs_ctx.ddr_qos_rsum, mvx_dvfs_ctx.ddr_qos_wsum);
            session->ddr_qos_read = session->ddr_qos_write = 0;
        }
#endif

        MVX_LOG_PRINT(&mvx_log_dev, MVX_LOG_INFO, "mvx_dvfs_unregister_session. session=%p", session_id);
        free_session(session);

    } else {
        MVX_LOG_PRINT(&mvx_log_dev, MVX_LOG_INFO, "session[%p] is already removed.", session_id);
    }

    up(&dvfs_sem);
}

void mvx_dvfs_estimate_ddr_bandwidth(struct estimate_ddr_input* input, struct estimate_ddr_output* output)
{
    /* predefined DDR throughput requirement for 1080p@30fps */
    const int defined_width = 1920;
    const int defined_height = 1080;
    const uint32_t defined_bandwidth_tbl[2][2][2]={
        {
            /*decoder*/
            {45000, 105000}, /*non-afbc[r, w]*/
            {45000, 20000}   /*afbc*/
        },
        {    /*encoder*/
            {162000, 54000}, /*non-afbc[r, w]*/
            {162000, 54000} /*afbc*/
        }
    };
    uint64_t estimated_read;
    uint64_t estimated_write;

    if (input->width == 0 || input->height == 0) {
        input->width = defined_width;
        input->height = defined_height;
    }
    if (input->fps <= 0) {
        input->fps = VPU_DDR_QOS_PREDEFINED_FPS;
    }

    MVX_LOG_PRINT(&mvx_log_dev, MVX_LOG_INFO, "estimate_ddr_bandwidth. isEnc=%d, isAFBC=%d, size=(%d, %d), framerate=%d.", input->isEnc, input->isAFBC, input->width, input->height, input->fps);

    estimated_read = ((uint64_t)defined_bandwidth_tbl[input->isEnc][input->isAFBC][0] * ( input->width * input->height) * input->fps /(defined_width * defined_height*VPU_DDR_QOS_PREDEFINED_FPS));
    estimated_write = ((uint64_t)defined_bandwidth_tbl[input->isEnc][input->isAFBC][1] * ( input->width * input->height) * input->fps /(defined_width * defined_height*VPU_DDR_QOS_PREDEFINED_FPS));
    if ((estimated_read + estimated_write) < VPU_DDR_QOS_MIN)
    {
        estimated_read = VPU_DDR_QOS_MIN/2;
        estimated_write = VPU_DDR_QOS_MIN/2;
    }

    if ((estimated_read + estimated_write) > VPU_DDR_QOS_MAX)
    {
        estimated_read = VPU_DDR_QOS_MAX/2;
        estimated_write = VPU_DDR_QOS_MAX/2;
    }

    output->estimated_read = estimated_read;
    output->estimated_write = estimated_write;
}

void mvx_dvfs_session_update_ddr_qos(const mvx_session_id session_id, uint32_t read_value, uint32_t write_value)
{
#if (1 == VPU_DDR_QOS_ENABLE)
    struct session *session;
    int sem_failed;

#if (1 == DVFS_DEBUG_MODE)
    if (0 == atomic_read(&ddr_qos_enabled))
    {
        return;
    }
#endif

    sem_failed = down_interruptible(&dvfs_sem);
    if (sem_failed)
    {
        MVX_LOG_PRINT(&mvx_log_dev, MVX_LOG_ERROR,
                "DVFS semaphore was not obtained, %d",
                sem_failed);
        return;
    }

    session = get_session(session_id);
    if (NULL != session && (session->ddr_qos_read != read_value || session->ddr_qos_write != write_value)
            && mvx_dvfs_ctx.ddr_qos_rsum >= session->ddr_qos_read
            && mvx_dvfs_ctx.ddr_qos_wsum >= session->ddr_qos_write)
    {
        mvx_dvfs_ctx.ddr_qos_rsum -= session->ddr_qos_read;
        mvx_dvfs_ctx.ddr_qos_rsum += read_value;
        session->ddr_qos_read = read_value;
        mvx_dvfs_ctx.ddr_qos_wsum -= session->ddr_qos_write;
        mvx_dvfs_ctx.ddr_qos_wsum += write_value;
        session->ddr_qos_write = write_value;
        MVX_LOG_PRINT(&mvx_log_dev, MVX_LOG_INFO,
                "DVFS update session[%p] ddr qos: [%d, %d]/[%d, %d]", session_id,  read_value, write_value, mvx_dvfs_ctx.ddr_qos_rsum, mvx_dvfs_ctx.ddr_qos_wsum);
    }

    up(&dvfs_sem);
#endif
}

void mvx_dvfs_suspend_session(void)
{
    int sem_failed;

    sem_failed = down_interruptible(&dvfs_sem);
    if (sem_failed)
    {
        MVX_LOG_PRINT(&mvx_log_dev, MVX_LOG_WARNING,
                "DVFS semaphore was not obtained, sem_failed=%d", sem_failed);
    }

    mvx_dvfs_ctx.sched_suspend = true;

    if (!sem_failed)
    {
        up(&dvfs_sem);
    }
}

void mvx_dvfs_resume_session(void)
{
    int sem_failed;

    sem_failed = down_interruptible(&dvfs_sem);
    if (sem_failed)
    {
        MVX_LOG_PRINT(&mvx_log_dev, MVX_LOG_WARNING,
                "DVFS semaphore was not obtained, sem_failed=%d", sem_failed);
    }

    mvx_dvfs_ctx.sched_suspend = false;

    if (!sem_failed)
    {
        up(&dvfs_sem);
    }
}
