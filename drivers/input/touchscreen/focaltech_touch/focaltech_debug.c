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
* File Name: focaltech_debug.c
*
* Author: Focaltech Driver Team
*
* Created: 2023-12-01
*
* Abstract: Fw Debug
*
* Version: V1.0
*
*****************************************************************************/

/*****************************************************************************
* Included header files
*****************************************************************************/
#include "focaltech_core.h"

/*****************************************************************************
* Private constant and macro definitions using #define
*****************************************************************************/
#define FTS_REG_FW_DEBUG_EN                 0x9E
#define FTS_REG_TPINFO                      0x96
#define FTS_REG_DBGCFG                      0x9D

#define DEFAULT_VAL_REG01                   0xFFFF
#define DEFAULT_MAX_FRAME_NUM               10000
#define MAX_SIZE_TP_INFO                    8
#define MAX_SIZE_DBG_CFG                    16
#define MAX_COUNT_READ_REGFB                3

/*****************************************************************************
* Private enumerations, structures and unions using typedef
*****************************************************************************/
typedef enum {
    FRAME_WAITQ_DEFAULT,
    FRAME_WAITQ_WAIT,
    FRAME_WAITQ_WAKEUP,
} FRAME_WAITQ_FLAG;

struct fwdbg_frame {
    u64 tv;
    unsigned char value[0];
};

struct fwdbg_queue {
    int head;
    int tail;
    int count;
    int max_count;
    int elem_size;
    u8 *buffer;
    struct mutex mutexq;
};

struct fwdbg_config {
    int total_len;
    int dbgoff;
    int dbghdr_len;
    int diff_len;
    int addinfo_len;
    int regfa_len;
    int regfb_len;
    int tx;
    int rx;
};

struct fts_fwdbg {
    struct fts_ts_data *ts_data;
    struct proc_dir_entry *proc_fwdbg;
    struct mutex mutex;
    wait_queue_head_t frame_waitq;
    struct fwdbg_queue q;
    struct fwdbg_config cfg;
    int max_frame_num; //maximum frame number kept in memory
    int frame_size; // is equal to size of one frame
    int touch_size_bak;
    int proc_ppos;
    int frame_waitq_flag;
    int reg01_val;
    char *proc_frame; /* save a frame value comes from queue */
    unsigned char *regfb_val;
    unsigned char *regfa_val;

    bool queue_stop;
    bool frame_logging;
    bool frame_block;
};

/*****************************************************************************
* Global variable or extern global variabls/functions
*****************************************************************************/
static struct fts_fwdbg *fts_fwdbg_data;

/*****************************************************************************
* Static function prototypes
*****************************************************************************/
static u64 fwdbg_get_timestamp(void)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 10, 0))
    ktime_t tv;
    tv = ktime_get_real() / 1000;
    return (u64)tv;
#else
    struct timeval tv;
    do_gettimeofday(&tv);
    return (u64)(((u64)tv.tv_sec * 1000000) + tv.tv_usec);
#endif
}

static int dbgq_open(struct fwdbg_queue *q, int max_framenum, int frame_size)
{
    if (!q || !max_framenum || !frame_size) {
        FTS_ERROR("q is null/max_framenum(%d)/frame_size(%d) is invalid", max_framenum, frame_size);
        return -EINVAL;
    }

    if (q->buffer) {
        vfree(q->buffer);
        q->buffer = NULL;
    }

    if (!q->buffer) {
        q->head = q->tail = q->count = 0;
        q->max_count = max_framenum;
        q->elem_size = frame_size;
        FTS_INFO("queque,max_count=%d,elem_size=%d", q->max_count, q->elem_size);
        q->buffer = vmalloc(q->max_count * q->elem_size);
        if (!q->buffer) {
            FTS_ERROR("malloc queue buffer failed");
            return -ENOMEM;
        }
        memset(q->buffer, 0, q->max_count * q->elem_size);
    }
    return 0;
}

static bool dbgq_full(struct fwdbg_queue *q)
{
    return q->count == q->max_count;
}

static bool dbgq_empty(struct fwdbg_queue *q)
{
    return q->count == 0;
}

static int dbgq_enqueue(struct fwdbg_queue *q, u8 *val, u64 timestamp)
{
    struct fwdbg_frame *tail_elem = NULL;

    if (!q || !val || !q->buffer) {
        FTS_ERROR("q/val/buffer is null");
        return -EINVAL;
    }

    mutex_lock(&(q->mutexq));
    tail_elem = (struct fwdbg_frame *)&q->buffer[q->tail * q->elem_size];
    tail_elem->tv = timestamp;
    memcpy(tail_elem->value, val, q->elem_size);
    q->tail = (q->tail + 1) % q->max_count;
    if (dbgq_full(q)) {
        q->head = (q->head + 1) % q->max_count;
    } else {
        q->count++;
    }
    mutex_unlock(&(q->mutexq));
    return 0;
}

static int dbgq_dequeue(struct fwdbg_queue *q, u8 *val)
{
    if (!q || !val || !q->buffer) {
        FTS_ERROR("q/val/buffer is null");
        return -EINVAL;
    }

    mutex_lock(&(q->mutexq));
    if (dbgq_empty(q)) {
        mutex_unlock(&(q->mutexq));
        return 1;
    }
    memcpy(val, &q->buffer[q->head * q->elem_size], q->elem_size);
    q->head = (q->head + 1) % q->max_count;
    q->count--;
    mutex_unlock(&(q->mutexq));
    return 0;
}

/*****************************************************************************
*  Name: dbgq_dequeue_to_proc
*  Brief: dequeue frames from queue, and send it to proc read buffer.
*  Input: @q
*         @buff_maxcount, the maximum frame count
*  Output:@buff, address of user space buffer
*  Return:size that send to the buff, or 0 if queue is null, or error code
*****************************************************************************/
static int dbgq_dequeue_to_proc(struct fwdbg_queue *q, char __user *buff, int buff_maxcount)
{
    int valid_count = 0;
    int i = 0;
    char *head_elem = NULL;
    if (!q || !buff || !q->buffer || !buff_maxcount) {
        FTS_ERROR("q/buff/buffer is null/buff_maxcount is 0");
        return -EINVAL;
    }

    mutex_lock(&(q->mutexq));
    if (dbgq_empty(q)) {
        mutex_unlock(&(q->mutexq));
        return 0;
    }
    valid_count = (buff_maxcount > q->count) ? q->count : buff_maxcount;
    for (i = 0; i < valid_count; i++) {
        head_elem = (char *)&q->buffer[q->head * q->elem_size];
        if (copy_to_user(buff + i * q->elem_size, head_elem, q->elem_size)) {
            FTS_ERROR("copy debug frame(%d) to user failed", i);
            mutex_unlock(&(q->mutexq));
            return i * q->elem_size;
        }
        q->head = (q->head + 1) % q->max_count;
        q->count--;
    }
    mutex_unlock(&(q->mutexq));
    return valid_count * q->elem_size;
}

static int dbgq_release(struct fwdbg_queue *q)
{
    q->head = q->tail = q->count = 0;
    q->max_count = q->elem_size = 0;
    if (q && q->buffer) {
        vfree(q->buffer);
        q->buffer = NULL;
    }
    return 0;
}

/*****************************************************************************
*  Name: proc_get_one_frame
*  Brief: Get a frame, and send the frame data to proc read buffer.
*  Input: @dbg
*         @buff_maxsize, the maximum size of buff
*  Output:@buff, address of user space buffer
*  Return:size that send to the buff, or 0 if queue is null, or error code
*****************************************************************************/
static int proc_get_one_frame(struct fts_fwdbg *dbg, char __user *buff, int buff_maxsize)
{
    int ret = 0;
    int frame_remaining_size = 0;
    int valid_size = 0;

    if (!dbg || !buff || !dbg->proc_frame || !buff_maxsize) {
        FTS_ERROR("dbg/buff/proc_frame is null/buff_maxsize is 0");
        return -EINVAL;
    }

    if (dbg->proc_ppos == 0) {
        ret = dbgq_dequeue(&dbg->q, dbg->proc_frame);
        if (ret < 0) {
            FTS_ERROR("get a frame from queue failed");
            return ret;
        } else if (ret == 1) {
            /* queque is null */
            return 0;
        }
    }

    frame_remaining_size = dbg->frame_size - dbg->proc_ppos;
    valid_size = (frame_remaining_size > buff_maxsize) ? buff_maxsize : frame_remaining_size;
    if (copy_to_user(buff, &dbg->proc_frame[dbg->proc_ppos], valid_size)) {
        FTS_ERROR("copy debug frame to user failed");
        return -EFAULT;
    }
    dbg->proc_ppos = (dbg->proc_ppos + valid_size) % dbg->frame_size;

    return valid_size;
}

static int fts_fwdbg_get_cfg(struct fts_fwdbg *dbg)
{
    int ret = 0;
    u8 cmd = 0;
    u8 tp_info[MAX_SIZE_TP_INFO] = { 0 };
    u8 dbg_cfg[MAX_SIZE_DBG_CFG] = { 0 };
    cmd = FTS_REG_TPINFO;
    ret = fts_read(&cmd, 1, tp_info, MAX_SIZE_TP_INFO);
    if (ret < 0) {
        FTS_ERROR("read tp info failed");
        return ret;
    }
    cmd = FTS_REG_DBGCFG;
    ret = fts_read(&cmd, 1, dbg_cfg, MAX_SIZE_DBG_CFG);
    if (ret < 0) {
        FTS_ERROR("read debug config failed");
        return ret;
    }

    dbg->cfg.total_len = (dbg_cfg[2] << 8) + dbg_cfg[3];
    dbg->cfg.dbgoff = dbg_cfg[4];
    dbg->cfg.dbghdr_len = dbg_cfg[5];
    dbg->cfg.diff_len = (dbg_cfg[6] << 8) + dbg_cfg[7];
    dbg->cfg.addinfo_len = (dbg_cfg[8] << 8) + dbg_cfg[9];
    dbg->cfg.regfb_len = (dbg_cfg[10] << 8) + dbg_cfg[11];
    dbg->cfg.regfa_len = (dbg_cfg[12] << 8) + dbg_cfg[13];
    dbg->cfg.tx = tp_info[2];
    dbg->cfg.rx = tp_info[3];
    return 0;
}

static int fts_fwdbg_enable(struct fts_fwdbg *dbg, int value)
{
    int ret = 0;
    if (!dbg || !dbg->ts_data || !value) {
        FTS_ERROR("fwdbg/ts_data is null/value(%d) is invalid", value);
        return -EINVAL;
    }

    ret = fts_fwdbg_get_cfg(dbg);
    if (ret < 0) {
        FTS_ERROR("get cfg from tp failed");
        return ret;
    }

    if ((dbg->cfg.total_len < FTS_TOUCH_DATA_LEN_V2) || (dbg->cfg.total_len > FTS_MAX_TOUCH_BUF)) {
        FTS_ERROR("report buffer length(%d),not in[%d,%d]", dbg->cfg.total_len,
                  FTS_TOUCH_DATA_LEN_V2, FTS_MAX_TOUCH_BUF);
        return -EIO;
    }

    dbg->frame_size = dbg->cfg.total_len + sizeof(struct fwdbg_frame);
    FTS_INFO("FwDebug enable,max_frame_num=%d,frame_size=%d", dbg->max_frame_num, dbg->frame_size);
    if (!dbg->frame_logging) {
        ret = dbgq_open(&dbg->q, dbg->max_frame_num, dbg->frame_size);
        if (ret < 0) {
            FTS_ERROR("dbgq_open failed");
            goto dbgen_err;
        }
    }

    if (dbg->cfg.regfb_len) {
        if (dbg->regfb_val) {
            vfree(dbg->regfb_val);
            dbg->regfb_val = NULL;
        }
        dbg->regfb_val = vmalloc(dbg->cfg.regfb_len);
        if (!dbg->regfb_val) {
            ret = -ENOMEM;
            goto dbgen_err;
        }
    }

    if (dbg->cfg.regfa_len) {
        if (dbg->regfa_val) {
            vfree(dbg->regfa_val);
            dbg->regfa_val = NULL;
        }
        dbg->regfa_val = vmalloc(dbg->cfg.regfa_len);
        if (!dbg->regfa_val) {
            ret = -ENOMEM;
            goto dbgen_err;
        }
    }

    dbg->touch_size_bak = dbg->ts_data->touch_size;
    dbg->ts_data->touch_size = dbg->cfg.total_len;
    ret = fts_write_reg(FTS_REG_FW_DEBUG_EN, value);
    if (ret < 0) {
        FTS_ERROR("write FwDebug to enable failed");
        goto dbgen_err;
    }

    return 0;

dbgen_err:
    if (dbg->regfa_val) {
        vfree(dbg->regfa_val);
        dbg->regfa_val = NULL;
    }
    if (dbg->regfb_val) {
        vfree(dbg->regfb_val);
        dbg->regfb_val = NULL;
    }
    dbgq_release(&dbg->q);
    dbg->ts_data->touch_size = dbg->touch_size_bak;
    fts_write_reg(FTS_REG_FW_DEBUG_EN, 0);
    return ret;
}

static int fts_fwdbg_disable(struct fts_fwdbg *dbg)
{
    int ret = 0;
    if (!dbg || !dbg->ts_data) {
        FTS_ERROR("fwdbg/ts_data is null");
        return -EINVAL;
    }

    if (!dbg->frame_logging) {
        dbgq_release(&dbg->q);
    }

    if (dbg->regfa_val) {
        vfree(dbg->regfa_val);
        dbg->regfa_val = NULL;
    }
    if (dbg->regfb_val) {
        vfree(dbg->regfb_val);
        dbg->regfb_val = NULL;
    }

    dbg->ts_data->touch_size = dbg->touch_size_bak;
    ret = fts_write_reg(FTS_REG_FW_DEBUG_EN, 0);
    if (ret < 0) {
        FTS_ERROR("write FwDebug to disable failed");
    }
    return ret;
}

static void fts_logging_frame(struct fwdbg_config *cfg, u8 *frame_buf, u64 timestamp)
{
    int i = 0;
    int n = 0;
    int index = 0;
    char logbuf[512] = { 0 };
    if (!cfg | !frame_buf)
        return ;

    FTS_DEBUG("logging a frame,timestamp=%lld", timestamp);
    for (i = 0; i < cfg->dbgoff; i++) {
        n += snprintf(logbuf + n, 512 - n, "%02x,", frame_buf[i]);
        if (n >= 512) break;
    }
    FTS_DEBUG("%s", logbuf);

    /**/
    n = 0;
    index = cfg->dbgoff;
    for (i = 0; i < cfg->dbghdr_len; i++) {
        n += snprintf(logbuf + n, 512 - n, "%02x,", frame_buf[index + i]);
        if (n >= 512) break;
    }
    FTS_DEBUG("%s", logbuf);


    index = cfg->dbgoff + cfg->dbghdr_len;
    FTS_DEBUG("%d", frame_buf[index]);
    n = 0;
    index = cfg->dbgoff + cfg->dbghdr_len + 1;
    for (i = 0; i < cfg->diff_len; i += 2) {
        n += snprintf(logbuf + n, 512 - n, "%d,", (short)((frame_buf[index + i] << 8) + frame_buf[index + i + 1]));
        if (n >= 512) break;
        else if (((i + 1) % cfg->rx) == 0) {
            FTS_DEBUG("%s", logbuf);
            n = 0;
        }
    }

    n = 0;
    index = cfg->dbgoff + cfg->dbghdr_len + cfg->diff_len;
    for (i = 0; i < cfg->addinfo_len; i += 2) {
        n += snprintf(logbuf + n, 512 - n, "%d,", (short)((frame_buf[index + i] << 8) + frame_buf[index + i + 1]));
        if (n >= 512) break;
        else if (((i + 1) % cfg->rx) == 0) {
            FTS_DEBUG("%s", logbuf);
            n = 0;
        }
    }
}

static void fts_logging_regfb(struct fts_fwdbg *dbg, u64 timestamp)
{
    int ret = 0;
    int i = 0;
    int n = 0;
    u8 cmd = 0xFB;
    char logbuf[512] = { 0 };

    if (!dbg || ! dbg->regfb_val || !dbg->cfg.regfb_len || !dbg->cfg.rx) {
        FTS_ERROR("dbg/regfb_val/regfb_len(%d)/rx(%d) is invalid", dbg->cfg.regfb_len, dbg->cfg.rx);
        return ;
    }

    ret = fts_read(&cmd, 1, dbg->regfb_val, dbg->cfg.regfb_len);
    if (ret < 0) {
        FTS_ERROR("read regfb failed,ret=%d", ret);
        return ;
    }

    FTS_DEBUG("logging regfb,timestamp=%lld", timestamp);
    for (i = 0; i < dbg->cfg.regfb_len; i += 2) {
        n += snprintf(logbuf + n, 512 - n, "%d,", (short)((dbg->regfb_val[i] << 8) + dbg->regfb_val[i + 1]));
        if (n >= 512) break;
        else if (((i + 1) % dbg->cfg.rx) == 0) {
            FTS_DEBUG("%s", logbuf);
            n = 0;
        }
    }
}

static void fts_logging_regfa(struct fts_fwdbg *dbg, u64 timestamp)
{
    int ret = 0;
    int i = 0;
    int n = 0;
    u8 cmd = 0xFA;
    int line_count = 0;
    char logbuf[512] = { 0 };

    if (!dbg || ! dbg->regfa_val || !dbg->cfg.regfa_len || !dbg->cfg.rx) {
        FTS_ERROR("dbg/regfa_val/regfa_len(%d)/rx(%d) is invalid", dbg->cfg.regfa_len, dbg->cfg.rx);
        return ;
    }

    ret = fts_read(&cmd, 1, dbg->regfa_val, dbg->cfg.regfa_len);
    if (ret < 0) {
        FTS_ERROR("read regfa failed,ret=%d", ret);
        return ;
    }

    FTS_DEBUG("logging regfa,timestamp=%lld", timestamp);
    line_count = dbg->cfg.rx * 2;
    for (i = 0; i < dbg->cfg.regfa_len; i++) {
        n += snprintf(logbuf + n, 512 - n, "%02X,", dbg->regfa_val[i]);
        if (n >= 512) break;
        else if (((i + 1) % line_count) == 0) {
            FTS_DEBUG("%s", logbuf);
            n = 0;
        }
    }
}


int fts_fwdbg_readdata(struct fts_ts_data *ts_data, u8 *buf)
{
    struct fts_fwdbg *dbg = fts_fwdbg_data;
    u64 timestamp = 0;
    if (!ts_data || !buf || !dbg) {
        FTS_ERROR("ts_data/buf/dbg is null");
        return -EINVAL;
    }

    if (!ts_data->fwdbg_support)
        return 0;

    timestamp = fwdbg_get_timestamp();
    if (dbg->frame_logging) {
        fts_logging_frame(&dbg->cfg, buf, timestamp);
    } else if (!dbg->queue_stop) {
        dbgq_enqueue(&dbg->q, buf, timestamp);
        if (dbg->frame_waitq_flag == FRAME_WAITQ_WAIT) {
            dbg->frame_waitq_flag = FRAME_WAITQ_WAKEUP;
            wake_up_interruptible(&dbg->frame_waitq);
        }
    }

    if (dbg->reg01_val == DEFAULT_VAL_REG01)
        dbg->reg01_val = buf[0];
    else if (buf[0] != dbg->reg01_val) {
        if (!dbg->frame_logging) fts_logging_frame(&dbg->cfg, buf, timestamp);
        if (dbg->cfg.regfb_len) fts_logging_regfb(dbg, timestamp);
        dbg->reg01_val = buf[0];
    }
    return 0;
}

/* proc node:fts_fwdbg */
static ssize_t fts_fwdbg_read(struct file *filp, char __user *buff, size_t count, loff_t *ppos)
{
    int read_byte_num = (int)count;
    int cnt = 0;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 17, 0))
    struct fts_fwdbg *dbg = pde_data(file_inode(filp));
#else
    struct fts_fwdbg *dbg = PDE_DATA(file_inode(filp));
#endif
    if (!dbg || !dbg->ts_data || !dbg->ts_data->fwdbg_support) {
        FTS_ERROR("dbg/ts_data is null/fwdbg isn't support");
        return -EINVAL;
    }

    if (dbg->frame_logging) {
        FTS_ERROR("frame logging is null,not return frame data");
        return -EINVAL;
    }

    if ((dbg->frame_block) && (read_byte_num < dbg->frame_size)) {
        FTS_ERROR("in block mode, proc count(%d) < frame size(%d)", read_byte_num, dbg->frame_size);
        return -EINVAL;
    }

    if (dbgq_empty(&dbg->q) && dbg->frame_block) {
        dbg->queue_stop = false;
        dbg->frame_waitq_flag = FRAME_WAITQ_WAIT;
        wait_event_interruptible(dbg->frame_waitq, dbg->frame_waitq_flag == FRAME_WAITQ_WAKEUP);
    }

    if (read_byte_num >= dbg->frame_size)
        cnt = dbgq_dequeue_to_proc(&dbg->q, buff, read_byte_num / dbg->frame_size);
    else
        cnt = proc_get_one_frame(dbg, buff, read_byte_num);

    FTS_DEBUG("cnt=%d", cnt);
    return cnt;
}

static int fts_fwdbg_open(struct inode *inode, struct file *file)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 17, 0))
    struct fts_fwdbg *dbg = pde_data(inode);
#else
    struct fts_fwdbg *dbg = PDE_DATA(inode);
#endif
    if (!dbg || !dbg->ts_data || !dbg->ts_data->fwdbg_support) {
        FTS_ERROR("dbg/ts_data is null");
        return -EINVAL;
    }

    FTS_DEBUG("frame,block=%d,logging=%d,size=%d,macount=%d,queuecount=%d", dbg->frame_block,
              dbg->frame_logging, dbg->frame_size, dbg->max_frame_num, dbg->q.count);
    if ((!dbg->frame_logging) && (dbg->q.elem_size != dbg->frame_size)) {
        FTS_ERROR("elem_size(%d) != frame_size(%d)", dbg->q.elem_size, dbg->frame_size);
        return -EINVAL;
    }

    dbg->proc_ppos = 0;
    dbg->queue_stop = false;
    if (!dbg->frame_block) {
        dbg->queue_stop = true;
        /* get fa/fb info */
        if (dbg->cfg.regfb_len) fts_logging_regfb(dbg, fwdbg_get_timestamp());
        if (dbg->cfg.regfa_len) fts_logging_regfa(dbg, fwdbg_get_timestamp());

        if (dbg->proc_frame) {
            vfree(dbg->proc_frame);
            dbg->proc_frame = NULL;
        }
        dbg->proc_frame = vmalloc(dbg->frame_size);
    }

    return 0;
}

static int fts_fwdbg_release(struct inode *inode, struct file *file)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 17, 0))
    struct fts_fwdbg *dbg = pde_data(inode);
#else
    struct fts_fwdbg *dbg = PDE_DATA(inode);
#endif
    if (!dbg) {
        FTS_ERROR("dbg is null");
        return -EINVAL;
    }

    if (dbg->proc_frame) {
        vfree(dbg->proc_frame);
        dbg->proc_frame = NULL;
    }
    dbg->proc_ppos = 0;
    dbg->queue_stop = false;
    return 0;
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 6, 0))
static const struct proc_ops fts_fwdbg_fops = {
    .proc_open = fts_fwdbg_open,
    .proc_read = fts_fwdbg_read,
    .proc_release = fts_fwdbg_release,
};
#else
static const struct file_operations fts_fwdbg_fops = {
    .open = fts_fwdbg_open,
    .read = fts_fwdbg_read,
    .release = fts_fwdbg_release,
};
#endif


/* sysfs node:fts_fwdbg_mode */
static ssize_t fts_fwdbg_mode_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    int count = 0;
    u8 val = 0;
    struct fts_ts_data *ts_data = dev_get_drvdata(dev);
    struct fts_fwdbg *dbg = fts_fwdbg_data;
    if (!dbg || !ts_data) {
        FTS_ERROR("dbg/ts_data is null");
        return count;
    }
    mutex_lock(&dbg->mutex);
    fts_read_reg(FTS_REG_FW_DEBUG_EN, &val);
    count = snprintf(buf, PAGE_SIZE, "FwDebug support:%d,value:0x%x\n", ts_data->fwdbg_support, ts_data->fwdbg_value);
    count += snprintf(buf + count, PAGE_SIZE, "Reg(0x9E)=0x%x\n", val);
    mutex_unlock(&dbg->mutex);
    return count;
}

static ssize_t fts_fwdbg_mode_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    int n = 0;
    int value = 0;
    struct fts_ts_data *ts_data = dev_get_drvdata(dev);
    struct fts_fwdbg *dbg = fts_fwdbg_data;
    if (!dbg || !ts_data) {
        FTS_ERROR("dbg/ts_data is null");
        return count;
    }

    mutex_lock(&dbg->mutex);
    n = sscanf(buf, "%d", &value);
    if ((n == 1) && (!!value ^ ts_data->fwdbg_support)) {
        if (value) {
            if (0 == fts_fwdbg_enable(dbg, value)) {
                ts_data->fwdbg_value = (u8)value;
                ts_data->fwdbg_support = ENABLE;
            }
        } else {
            ts_data->fwdbg_support = DISABLE;
            fts_fwdbg_disable(dbg);
        }
    } else FTS_INFO("n(%d)!=/value(%d)==fwdbg_support(%d)", n, !!value, ts_data->fwdbg_support);
    mutex_unlock(&dbg->mutex);
    return count;
}

/* sysfs node:fts_fwdbg_maxcount */
static ssize_t fts_fwdbg_maxcount_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    int count = 0;
    struct fts_fwdbg *dbg = fts_fwdbg_data;
    if (!dbg) {
        FTS_ERROR("dbg is null");
        return count;
    }
    mutex_lock(&dbg->mutex);
    count = snprintf(buf, PAGE_SIZE, "FwDebug,maximum frame count:%d\n", dbg->max_frame_num);
    mutex_unlock(&dbg->mutex);
    return count;
}

static ssize_t fts_fwdbg_maxcount_store(
    struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    int n = 0;
    int value = 0;
    struct fts_fwdbg *dbg = fts_fwdbg_data;
    if (!dbg || !dbg->ts_data) {
        FTS_ERROR("dbg is null");
        return count;
    }

    mutex_lock(&dbg->mutex);
    n = sscanf(buf, "%d", &value);
    if ((n == 1) && (value > 0) && !dbg->ts_data->fwdbg_support) {
        FTS_INFO("maximum frame count: %d->%d", dbg->max_frame_num, value);
        dbg->max_frame_num = value;
    } else FTS_INFO("n(%d)!=1/value(%d)=0/fwdbg_support(%d)!=0", n, value, dbg->ts_data->fwdbg_support);
    mutex_unlock(&dbg->mutex);
    return count;
}

/* sysfs node:fts_fwdbg_logging */
static ssize_t fts_fwdbg_logging_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    int count = 0;
    struct fts_fwdbg *dbg = fts_fwdbg_data;
    if (!dbg) {
        FTS_ERROR("dbg is null");
        return count;
    }
    mutex_lock(&dbg->mutex);
    count = snprintf(buf, PAGE_SIZE, "FwDebug,frame logging:%d\n", dbg->frame_logging);
    mutex_unlock(&dbg->mutex);
    return count;
}

static ssize_t fts_fwdbg_logging_store(
    struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    int n = 0;
    int value = 0;
    struct fts_fwdbg *dbg = fts_fwdbg_data;
    if (!dbg) {
        FTS_ERROR("dbg is null");
        return count;
    }

    mutex_lock(&dbg->mutex);
    n = sscanf(buf, "%d", &value);
    if ((n == 1) && !dbg->ts_data->fwdbg_support) {
        FTS_INFO("frame logging: %d->%d", dbg->frame_logging, !!value);
        dbg->frame_logging = !!value;
    } else FTS_INFO("n(%d)!=1/fwdbg_support(%d)!=0", n, dbg->ts_data->fwdbg_support);
    mutex_unlock(&dbg->mutex);
    return count;
}

/* sysfs node:fts_fwdbg_block */
static ssize_t fts_fwdbg_block_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    int count = 0;
    struct fts_fwdbg *dbg = fts_fwdbg_data;
    if (!dbg) {
        FTS_ERROR("dbg is null");
        return count;
    }
    mutex_lock(&dbg->mutex);
    count = snprintf(buf, PAGE_SIZE, "frame block:%d\n", dbg->frame_block);
    mutex_unlock(&dbg->mutex);
    return count;
}

static ssize_t fts_fwdbg_block_store(
    struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    int n = 0;
    int value = 0;
    struct fts_fwdbg *dbg = fts_fwdbg_data;
    if (!dbg) {
        FTS_ERROR("dbg is null");
        return count;
    }

    mutex_lock(&dbg->mutex);
    n = sscanf(buf, "%d", &value);
    if (n == 1) {
        FTS_INFO("frame block: %d->%d", dbg->frame_block, !!value);
        dbg->frame_block = !!value;
    }
    mutex_unlock(&dbg->mutex);
    return count;
}

static DEVICE_ATTR(fts_fwdbg_mode, S_IRUGO | S_IWUSR, fts_fwdbg_mode_show, fts_fwdbg_mode_store);
static DEVICE_ATTR(fts_fwdbg_maxcount, S_IRUGO | S_IWUSR, fts_fwdbg_maxcount_show, fts_fwdbg_maxcount_store);
static DEVICE_ATTR(fts_fwdbg_logging, S_IRUGO | S_IWUSR, fts_fwdbg_logging_show, fts_fwdbg_logging_store);
static DEVICE_ATTR(fts_fwdbg_block, S_IRUGO | S_IWUSR, fts_fwdbg_block_show, fts_fwdbg_block_store);
static struct attribute *fts_fwdbg_attrs[] = {
    &dev_attr_fts_fwdbg_mode.attr,
    &dev_attr_fts_fwdbg_maxcount.attr,
    &dev_attr_fts_fwdbg_logging.attr,
    &dev_attr_fts_fwdbg_block.attr,
    NULL,
};
static struct attribute_group fts_fwdbg_group = {.attrs = fts_fwdbg_attrs,};

static void fts_fwdbg_work_func(struct work_struct *work)
{
    struct fts_fwdbg *dbg = fts_fwdbg_data;
    struct fts_ts_data *ts_data = container_of(work, struct fts_ts_data, fwdbg_work.work);
    if (ts_data && ts_data->fwdbg_support && dbg) {
        if (dbg->cfg.regfb_len) {
            fts_logging_regfb(dbg, fwdbg_get_timestamp());
            fts_logging_regfb(dbg, fwdbg_get_timestamp());
            fts_logging_regfb(dbg, fwdbg_get_timestamp());
        }

        if (dbg->cfg.regfa_len) fts_logging_regfa(dbg, fwdbg_get_timestamp());

    }
}

void fts_fwdbg_handle_reset(struct fts_ts_data *ts_data)
{
    struct fts_fwdbg *dbg = fts_fwdbg_data;
    if (ts_data && ts_data->ts_workqueue && ts_data->fwdbg_support && dbg) {
        dbg->reg01_val = DEFAULT_VAL_REG01;
        if (dbg->cfg.regfb_len || dbg->cfg.regfa_len)
            queue_delayed_work(ts_data->ts_workqueue, &ts_data->fwdbg_work, msecs_to_jiffies(200));
    }
}

int fts_fwdbg_init(struct fts_ts_data *ts_data)
{
    int ret = 0;
    struct fts_fwdbg *dbg = NULL;

    FTS_FUNC_ENTER();
    dbg = kzalloc(sizeof(struct fts_fwdbg), GFP_KERNEL);
    if (!dbg) {
        FTS_ERROR("allocate memory for fwdbg failed");
        return -ENOMEM;
    }
    fts_fwdbg_data = dbg;
    dbg->ts_data = ts_data;
    dbg->max_frame_num = DEFAULT_MAX_FRAME_NUM;
    dbg->frame_block = false;
    dbg->frame_logging = false;
    dbg->frame_waitq_flag = FRAME_WAITQ_DEFAULT;
    dbg->reg01_val = DEFAULT_VAL_REG01;
    mutex_init(&dbg->mutex);
    mutex_init(&dbg->q.mutexq);
    init_waitqueue_head(&dbg->frame_waitq);

    dbg->proc_fwdbg = proc_create_data("fts_fwdbg", 0777, NULL, &fts_fwdbg_fops, dbg);
    if (NULL == dbg->proc_fwdbg) {
        FTS_ERROR("create proc_fwdbg entry failed");
    }

    ret = sysfs_create_group(&ts_data->dev->kobj, &fts_fwdbg_group);
    if (ret) {
        FTS_ERROR("create fwdebug sysfs node failed");
        sysfs_remove_group(&ts_data->dev->kobj, &fts_fwdbg_group);
    }

    if (ts_data->ts_workqueue) INIT_DELAYED_WORK(&ts_data->fwdbg_work, fts_fwdbg_work_func);
    FTS_FUNC_EXIT();
    return 0;
}

int fts_fwdbg_exit(struct fts_ts_data *ts_data)
{
    struct fts_fwdbg *dbg = fts_fwdbg_data;
    FTS_FUNC_ENTER();
    if (dbg) {
        if (dbg->proc_fwdbg) proc_remove(dbg->proc_fwdbg);
        if (dbg->regfa_val) {
            vfree(dbg->regfa_val);
            dbg->regfa_val = NULL;
        }

        if (dbg->regfb_val) {
            vfree(dbg->regfb_val);
            dbg->regfb_val = NULL;
        }

        if (dbg->q.buffer) {
            vfree(dbg->q.buffer);
            dbg->q.buffer = NULL;
        }

        if (dbg->proc_frame) {
            vfree(dbg->proc_frame);
            dbg->proc_frame = NULL;
        }

        kfree_safe(dbg);
    }

    if (ts_data) {
        sysfs_remove_group(&ts_data->dev->kobj, &fts_fwdbg_group);
        cancel_delayed_work_sync(&ts_data->fwdbg_work);
    }
    FTS_FUNC_EXIT();
    return 0;
}
