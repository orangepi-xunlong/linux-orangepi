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
* File Name: focaltech_hp.c
*
* Author: Focaltech Driver Team
*
* Created: 2023-12-12
*
* Abstract: Host Processing
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
#define FTS_FHP_NAME                        "fhp_ft"
#define FTS_FHPI_DEV_NAME                   "fhp_input"

#define DEFAULT_IOCTL_SPI_BUFSIZE           256
#define MAX_QUEUE_BUFSIZE                   (32768)

#define FHP_CMD_GET_FRAME                   0x01

struct fhp_spi_sync_data {
    char *tx;
    char *rx;
    unsigned int size;
};

struct fhp_input_report_data {
    char *buffer;
    unsigned int size;
};

#define FHP_IOCTL_DEVICE                    0xC5
#define FHP_IOCTL_RESET                     _IOW(FHP_IOCTL_DEVICE, 0x01, long)
#define FHP_IOCTL_SPI_SYNC                  _IOWR(FHP_IOCTL_DEVICE, 0x02, struct fhp_spi_sync_data)
#define FHP_IOCTL_SET_IRQ                   _IOW(FHP_IOCTL_DEVICE, 0x03, long)
#define FHP_IOCTL_SET_RAW_SIZE              _IOW(FHP_IOCTL_DEVICE, 0x04, long)
#define FHP_IOCTL_SET_SPI_SPEED             _IOW(FHP_IOCTL_DEVICE, 0x05, long)
#define FHP_IOCTL_GET_CHIP_INIT_DONE        _IOR(FHP_IOCTL_DEVICE, 0x06, long)
#define FHP_IOCTL_GET_FRAME                 _IOR(FHP_IOCTL_DEVICE, 0x07, void *)
#define FHP_IOCTL_CLEAR_FRAME               _IOW(FHP_IOCTL_DEVICE, 0x08, long)
#define FHP_IOCTL_SET_TIMEOUT               _IOW(FHP_IOCTL_DEVICE, 0x09, long)
#define FHP_IOCTL_REPORT                    _IOW(FHP_IOCTL_DEVICE, 0x0A, struct fhp_input_report_data)

#define FHP_INPUT_IOCTL_DEVICE              0xC6
#define FHP_INPUT_IOCTL_REPORT              _IOW(FHP_INPUT_IOCTL_DEVICE, 0x01, struct fhp_input_report_data)

/*****************************************************************************
* Private enumerations, structures and unions using typedef
*****************************************************************************/
typedef enum {
    FHP_FRAME_WAITQ_DEFAULT,
    FHP_FRAME_WAITQ_WAIT,
    FHP_FRAME_WAITQ_WAIT_POLL,
    FHP_FRAME_WAITQ_WAKEUP,
} FHP_FRAME_WAITQ_FLAG;

struct fhp_frame {
    u64 tv;
    unsigned char value[0];
};

struct fhp_queue {
    int head;
    int tail;
    int count;
    int max_count;
    int elem_size;
    u8 *buffer;
    struct mutex mutexq;
};

struct fhp_core_data {
    struct fts_ts_data *ts_data;
    struct miscdevice fhp_dev;
    struct miscdevice fhpi_dev;
    wait_queue_head_t frame_waitq;
    struct fhp_queue q;
    int fhp_dev_open_cnt;
    int timeout; /*0:no wait, <0 wait infinitely, >0 wait timeout*/
    int frame_waitq_flag;
    int touch_size_bak;
};

/*****************************************************************************
* Global variable or extern global variabls/functions
*****************************************************************************/
static struct fhp_core_data *fts_fhp_data;
static long fhp_input_report(struct fhp_core_data *fhp_data, unsigned long arg);

/*****************************************************************************
* Static function prototypes
*****************************************************************************/
static u64 fhp_get_timestamp(void)
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

static int fhp_spi_sync(struct fhp_core_data *fhp_data, u8 *tx_buf, u8 *rx_buf, u32 len)
{
    int ret = 0;
    u8 *tbuf = NULL;
    u8 *rbuf = NULL;
    struct spi_message msg;
    struct spi_transfer xfer = { 0 };

    if (!fhp_data || !fhp_data->ts_data || !fhp_data->ts_data->spi) {
        FTS_ERROR("fhp/ts data/spi is null");
        return -EINVAL;
    }

    if (!fhp_data->ts_data->bus_tx_buf || !fhp_data->ts_data->bus_rx_buf || !tx_buf || !rx_buf || !len) {
        FTS_ERROR("tx_buf/rx_buf is null/len(%d) invalid", len);
        return -EINVAL;
    }

    mutex_lock(&fhp_data->ts_data->bus_lock);
    if (len > FTS_MAX_BUS_BUF) {
        tbuf = kzalloc(len, GFP_KERNEL);
        if (NULL == tbuf) {
            FTS_ERROR("txbuf malloc fail");
            ret = -ENOMEM;
            goto err_spi_sync;
        }

        rbuf = kzalloc(len, GFP_KERNEL);
        if (NULL == rbuf) {
            FTS_ERROR("rxbuf malloc fail");
            ret = -ENOMEM;
            goto err_spi_sync;
        }
    } else {
        tbuf = fhp_data->ts_data->bus_tx_buf;
        rbuf = fhp_data->ts_data->bus_rx_buf;
        memset(tbuf, 0x0, FTS_MAX_BUS_BUF);
        memset(rbuf, 0x0, FTS_MAX_BUS_BUF);
    }

    memcpy(tbuf, tx_buf, len);
    xfer.tx_buf = tbuf;
    xfer.rx_buf = rbuf;
    xfer.len = len;
    spi_message_init(&msg);
    spi_message_add_tail(&xfer, &msg);
    ret = spi_sync(fhp_data->ts_data->spi, &msg);
    memcpy(rx_buf, rbuf, len);
    if (ret) {
        FTS_ERROR("spi_sync fail,addr:%x,status:%x,ret:%d", tbuf[0], rbuf[3], ret);
        goto err_spi_sync;
    }

    ret = 0;
err_spi_sync:
    if (len > FTS_MAX_BUS_BUF) {
        if (tbuf) {
            kfree(tbuf);
            tbuf = NULL;
        }

        if (rbuf) {
            kfree(rbuf);
            rbuf = NULL;
        }
    }

    mutex_unlock(&fhp_data->ts_data->bus_lock);
    return ret;
}

static int fhpq_open(struct fhp_queue *q)
{
    if (!q) {
        FTS_ERROR("q is null");
        return -EINVAL;
    }

    FTS_FUNC_ENTER();
    mutex_lock(&(q->mutexq));
    q->head = q->tail = q->count = 0;
    q->buffer = kzalloc(MAX_QUEUE_BUFSIZE, GFP_KERNEL);
    if (!q->buffer) {
        FTS_ERROR("malloc queue buffer failed");
        mutex_unlock(&(q->mutexq));
        return -ENOMEM;
    }
    mutex_unlock(&(q->mutexq));
    FTS_FUNC_EXIT();
    return 0;
}

static int fhpq_set_element_size(struct fhp_queue *q, int elem_size)
{
    int max_size = FTS_MAX_TOUCH_BUF - sizeof(struct fhp_frame);
    if (!q || !elem_size || (elem_size > max_size)) {
        FTS_ERROR("q is null/elem_size(%d) is invalid", elem_size);
        return -EINVAL;
    }

    FTS_FUNC_ENTER();
    mutex_lock(&(q->mutexq));
    q->elem_size = elem_size;
    q->max_count = MAX_QUEUE_BUFSIZE / elem_size;
    FTS_INFO("set queue elem_size=%d,max_count=%d", q->elem_size, q->max_count);
    mutex_unlock(&(q->mutexq));
    FTS_FUNC_EXIT();
    return 0;
}

static bool fhpq_full(struct fhp_queue *q)
{
    return q->count == q->max_count;
}

static bool fhpq_empty(struct fhp_queue *q)
{
    return q->count == 0;
}

static void fhpq_clear(struct fhp_queue *q)
{
    mutex_lock(&(q->mutexq));
    q->head = q->tail = q->count = 0;
    mutex_unlock(&(q->mutexq));
}

static int fhpq_enqueue(struct fhp_queue *q, u8 *val, u64 timestamp)
{
    struct fhp_frame *tail_elem = NULL;

    if (!q || !val || !q->buffer || !q->elem_size || !q->max_count) {
        FTS_ERROR("q/val/buffer is null/size is 0");
        return -EINVAL;
    }

    mutex_lock(&(q->mutexq));
    tail_elem = (struct fhp_frame *)&q->buffer[q->tail * q->elem_size];
    tail_elem->tv = timestamp;
    memcpy(tail_elem->value, val, q->elem_size);
    q->tail = (q->tail + 1) % q->max_count;
    if (fhpq_full(q)) {
        q->head = (q->head + 1) % q->max_count;
    } else {
        q->count++;
    }
    mutex_unlock(&(q->mutexq));
    return 0;
}

static int fhpq_dequeue_userspace(struct fhp_queue *q, void __user *buff)
{
    if (!q || !buff || !q->buffer) {
        FTS_ERROR("q/buff/buffer is null");
        return -EINVAL;
    }

    mutex_lock(&(q->mutexq));
    if (fhpq_empty(q)) {
        mutex_unlock(&(q->mutexq));
        return -ENODATA;
    }
    if (copy_to_user(buff, &q->buffer[q->head * q->elem_size], q->elem_size)) {
        FTS_ERROR("copy frame data to user failed");
        mutex_unlock(&(q->mutexq));
        return -EFAULT;
    }
    q->head = (q->head + 1) % q->max_count;
    q->count--;
    mutex_unlock(&(q->mutexq));
    return 0;
}

static int fhpq_release(struct fhp_queue *q)
{
    if (q) {
        mutex_lock(&(q->mutexq));
        q->head = q->tail = q->count = 0;
        q->max_count = q->elem_size = 0;
        if (q->buffer) {
            kfree(q->buffer);
            q->buffer = NULL;
        }
        mutex_unlock(&(q->mutexq));
    }
    return 0;
}


static int fhp_ioctl_reset(struct fhp_core_data *fhp_data, unsigned long high)
{
    if (!fhp_data || !fhp_data->ts_data) {
        FTS_ERROR("fhp/ts data is null");
        return -EINVAL;
    }
    return fts_set_reset(fhp_data->ts_data, (int)high);
}

static int fhp_ioctl_set_irq(struct fhp_core_data *fhp_data, unsigned long enable)
{
    if (!fhp_data || !fhp_data->ts_data) {
        FTS_ERROR("fhp/ts data is null");
        return -EINVAL;
    }

    FTS_INFO("set irq to %s", enable ? "enable" : "disable");
    if (enable)
        fts_irq_enable();
    else
        fts_irq_disable();
    return 0;
}

static int fhp_ioctl_spi_sync(struct fhp_core_data *fhp_data, unsigned long arg)
{
    int ret = 0;
    struct fhp_spi_sync_data spi_data;
    u8 txbuf_temp[DEFAULT_IOCTL_SPI_BUFSIZE] = { 0 };
    u8 rxbuf_temp[DEFAULT_IOCTL_SPI_BUFSIZE] = { 0 };
    u8 *txbuf = txbuf_temp;
    u8 *rxbuf = rxbuf_temp;

    if (!fhp_data || !fhp_data->ts_data) {
        FTS_ERROR("fhp/ts data is null");
        return -EINVAL;
    }

    if (copy_from_user(&spi_data, (void *)arg, sizeof(struct fhp_spi_sync_data))) {
        FTS_ERROR("copy spi_sync data from userspace fail");
        return -EFAULT;
    }

    if (!spi_data.tx || !spi_data.rx || !spi_data.size) {
        FTS_ERROR("tx/rx/size(%d) from userspace are invalid", spi_data.size);
        return -EINVAL;
    }

    if (spi_data.size > DEFAULT_IOCTL_SPI_BUFSIZE) {
        txbuf = kmalloc(spi_data.size, GFP_KERNEL);
        rxbuf = kmalloc(spi_data.size, GFP_KERNEL);
        if (!txbuf || !rxbuf) {
            FTS_ERROR("kzalloc memory(size:%d) for spi tx/rx buffer fail", spi_data.size);
            ret = -ENOMEM;
            goto spi_sync_err;
        }
    }
    memset(txbuf, 0, spi_data.size);
    memset(rxbuf, 0, spi_data.size);

    if (copy_from_user(txbuf, spi_data.tx, spi_data.size)) {
        FTS_ERROR("copy spi tx data from userspace fail");
        ret = -EFAULT;
        goto spi_sync_err;
    }

    ret = fhp_spi_sync(fhp_data, txbuf, rxbuf, spi_data.size);
    if (ret) {
        FTS_ERROR("spi sync fail");
        ret = -EIO;
        goto spi_sync_err;
    }

    if (copy_to_user(spi_data.rx, rxbuf, spi_data.size)) {
        FTS_ERROR("copy spi rx data to userspace fail");
        ret = -EFAULT;
        goto spi_sync_err;
    }

    return 0;
spi_sync_err:
    if (spi_data.size > DEFAULT_IOCTL_SPI_BUFSIZE) {
        kfree(txbuf);
        kfree(rxbuf);
    }
    return ret;
}

static int fhp_ioctl_set_frame_size(struct fhp_core_data *fhp_data, unsigned long size)
{
    int ret = 0;
    int chip_frame_size = (int)size;
    int frame_size = 0;
    int max_size = FTS_MAX_TOUCH_BUF - sizeof(struct fhp_frame);

    if (!fhp_data || !fhp_data->ts_data || (chip_frame_size <= 0) || (chip_frame_size > max_size)) {
        FTS_ERROR("fhp/ts data is null/size(%d) is invalid", chip_frame_size);
        return -EINVAL;
    }

    FTS_INFO("set frame size to %d", chip_frame_size);
    frame_size = chip_frame_size + sizeof(struct fhp_frame);
    ret = fhpq_set_element_size(&fhp_data->q, frame_size);
    if (ret) {
        FTS_ERROR("set queue element size failed,ret=%d", ret);
        return ret;
    }
    fhp_data->touch_size_bak = fhp_data->ts_data->touch_size;
    fhp_data->ts_data->touch_size = chip_frame_size;
    return 0;
}

static int fhp_ioctl_set_spi_speed(struct fhp_core_data *fhp_data, unsigned long speed)
{
    int ret = 0;
    if (!fhp_data || !fhp_data->ts_data || !fhp_data->ts_data->spi) {
        FTS_ERROR("fhp/ts data/spi is null");
        return -EINVAL;
    }

    if (fhp_data->ts_data->spi->max_speed_hz == speed) {
        FTS_INFO("spi speed == max_speed_hz");
        return 0;
    }

    FTS_INFO("set spi speed to %ld", speed);
    mutex_lock(&fhp_data->ts_data->bus_lock);
    fhp_data->ts_data->spi->max_speed_hz = speed;
    ret = spi_setup(fhp_data->ts_data->spi);
    if (ret < 0) {
        FTS_ERROR("spi setup fail,ret:%d", ret);
    }
    mutex_unlock(&fhp_data->ts_data->bus_lock);
    return ret;
}

static int fhp_ioctl_get_chip_init_done(struct fhp_core_data *fhp_data, unsigned long arg)
{
    int ret = 0;
    u8 status = 0;
    if (!fhp_data || !fhp_data->ts_data) {
        FTS_ERROR("fhp/ts data is null");
        return -EINVAL;
    }

    /* 1:init done, 0:not done, others:reserved */
    status = fhp_data->ts_data->fw_loading ? 0 : 1;
    FTS_INFO("chip init flag:%d", status);
    if (copy_to_user((void *)arg, (u8 *)&status, sizeof(u8))) {
        FTS_ERROR("copy chip_init_done flag data to userspace fail");
        ret = -EFAULT;
        return ret;
    }

    return 0;
}

static int fhp_ioctl_get_frame(struct fhp_core_data *fhp_data, unsigned long arg)
{
    int ret = 0;
    if (!fhp_data || !fhp_data->ts_data || !fhp_data->ts_data->touch_buf) {
        FTS_ERROR("fhp/ts data is null");
        return -EINVAL;
    }

    if (fhpq_empty(&fhp_data->q) && (fhp_data->frame_waitq_flag != FHP_FRAME_WAITQ_WAIT_POLL)) {
        fhp_data->frame_waitq_flag = FHP_FRAME_WAITQ_WAIT;
        if (fhp_data->timeout == 0) {
            FTS_INFO("No frame data, and not wait");
            return -ENODATA;
        } else if (fhp_data->timeout < 0) {
            wait_event_interruptible(fhp_data->frame_waitq,
                                     fhp_data->frame_waitq_flag == FHP_FRAME_WAITQ_WAKEUP);
        } else if (fhp_data->timeout > 0) {
            ret = wait_event_interruptible_timeout(fhp_data->frame_waitq,
                                                   fhp_data->frame_waitq_flag == FHP_FRAME_WAITQ_WAKEUP,
                                                   msecs_to_jiffies(fhp_data->timeout));
            if (ret == 0) return -ETIMEDOUT;
        }
    }

    ret = fhpq_dequeue_userspace(&fhp_data->q, (void *)arg);
    if (ret < 0) {
        FTS_ERROR("dequeue frame failed");
        return ret;
    }

    return 0;
}

static int fhp_ioctl_clear_frame(struct fhp_core_data *fhp_data, unsigned long arg)
{
    if (fhp_data) {
        FTS_INFO("clear queue frame");
        fhpq_clear(&fhp_data->q);
    }
    return 0;
}

static int fhp_ioctl_set_timeout(struct fhp_core_data *fhp_data, unsigned long arg)
{
    int timeout = (int)arg;
    if (!fhp_data) {
        FTS_ERROR("fhp data is null");
        return -EINVAL;
    }

    if (fhp_data->timeout != timeout) {
        FTS_INFO("set timeout:%d->%d", fhp_data->timeout, timeout);
        fhp_data->timeout = timeout;
    }
    return 0;
}

static int fhp_open(struct inode *inode, struct file *file)
{
    int ret = 0;
    struct fhp_core_data *fhp_data = container_of(file->private_data, struct fhp_core_data, fhp_dev);
    FTS_FUNC_ENTER();
    if (!fhp_data && !fhp_data->ts_data) {
        FTS_ERROR("fhp_data/ts_data is null");
        return -ENODATA;
    }

    if (fhp_data->fhp_dev_open_cnt >= 1) {
        FTS_ERROR("open fail(cnt:%d)", fhp_data->fhp_dev_open_cnt);
        return -EBUSY;
    }

    fts_irq_disable();
    ret = fhpq_open(&fhp_data->q);
    if (ret) {
        FTS_ERROR("queue open failed");
        return ret;
    }

    fhp_data->frame_waitq_flag = FHP_FRAME_WAITQ_DEFAULT;
    fhp_data->timeout = 0;
    fhp_data->fhp_dev_open_cnt++;
    fhp_data->ts_data->fhp_mode = ENABLE;
    FTS_INFO("open successfully(cnt:%d)", fhp_data->fhp_dev_open_cnt);
    FTS_FUNC_EXIT();
    return 0;
}

static int fhp_close(struct inode *inode, struct file *file)
{
    struct fhp_core_data *fhp_data = container_of(file->private_data, struct fhp_core_data, fhp_dev);

    FTS_FUNC_ENTER();
    if (!fhp_data && !fhp_data->ts_data) {
        FTS_ERROR("fhp_data/ts_data is null");
        return -ENODATA;
    }
    fhpq_release(&fhp_data->q);
    fhp_data->fhp_dev_open_cnt--;
    fhp_data->ts_data->fhp_mode = DISABLE;
    fhp_data->ts_data->touch_size = fhp_data->touch_size_bak;
    fts_irq_enable();
    FTS_FUNC_EXIT();
    return 0;
}

static long fhp_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    int ret = 0;
    struct fhp_core_data *fhp_data = container_of(filp->private_data, struct fhp_core_data, fhp_dev);

    if (!fhp_data) {
        FTS_ERROR("fhp_data is null");
        return -ENODATA;
    }

    //FTS_DEBUG("ioctl cmd:%x,arg:%lx", cmd, arg);
    switch (cmd) {
    case FHP_IOCTL_GET_FRAME:
        ret = fhp_ioctl_get_frame(fhp_data, arg);
        break;
    case FHP_IOCTL_SPI_SYNC:
        ret = fhp_ioctl_spi_sync(fhp_data, arg);
        break;
    case FHP_IOCTL_RESET:
        ret = fhp_ioctl_reset(fhp_data, arg);
        break;
    case FHP_IOCTL_SET_IRQ:
        ret = fhp_ioctl_set_irq(fhp_data, arg);
        break;
    case FHP_IOCTL_SET_RAW_SIZE:
        fhp_ioctl_set_frame_size(fhp_data, arg);
        break;
    case FHP_IOCTL_SET_SPI_SPEED:
        ret = fhp_ioctl_set_spi_speed(fhp_data, arg);
        break;
    case FHP_IOCTL_GET_CHIP_INIT_DONE:
        ret = fhp_ioctl_get_chip_init_done(fhp_data, arg);
        break;
    case FHP_IOCTL_CLEAR_FRAME:
        ret = fhp_ioctl_clear_frame(fhp_data, arg);
        break;
    case FHP_IOCTL_SET_TIMEOUT:
        ret = fhp_ioctl_set_timeout(fhp_data, arg);
        break;
    case FHP_IOCTL_REPORT:
        ret = fhp_input_report(fhp_data, arg);
        break;
    default:
        FTS_ERROR("unkown ioctl cmd(0x%x)", (int)cmd);
        return -EINVAL;
    }

    return ret;
}

static unsigned int fhp_poll(struct file *filp, struct poll_table_struct *wait)
{
    unsigned int mask = 0;
    struct fhp_core_data *fhp_data = container_of(filp->private_data, struct fhp_core_data, fhp_dev);

    if (!fhp_data) {
        FTS_ERROR("fhp_data is null");
        return -ENODATA;
    }

    poll_wait(filp, &fhp_data->frame_waitq, wait);
    if (fhp_data->frame_waitq_flag == FHP_FRAME_WAITQ_WAKEUP)
        mask |= POLLIN | POLLRDNORM;

    fhp_data->frame_waitq_flag = FHP_FRAME_WAITQ_WAIT_POLL;
    return mask;
}

static struct file_operations fhp_fops = {
    .open = fhp_open,
    .release = fhp_close,
    .unlocked_ioctl = fhp_ioctl,
    .poll = fhp_poll,
};

static int fhp_miscdev_init(struct fhp_core_data *fhp_data)
{
    int ret = 0;

    fhp_data->fhp_dev.minor = MISC_DYNAMIC_MINOR;
    fhp_data->fhp_dev.name = FTS_FHP_NAME;
    fhp_data->fhp_dev.fops = &fhp_fops;
    ret = misc_register(&fhp_data->fhp_dev);
    if (ret) {
        FTS_ERROR("misc_register(fhp_dev) fail");
        return ret;
    }

    FTS_INFO("misc_register(fhp_dev) success");
    return 0;
}

static long fhp_input_report(struct fhp_core_data *fhp_data, unsigned long arg)
{
    int ret = 0;
    u8 report_buf[FTS_TOUCH_DATA_LEN_V2] = { 0 };
    struct fhp_input_report_data report;

    if (!fhp_data || !fhp_data->ts_data) {
        FTS_ERROR("fhp_data/ts_data is null");
        return -EINVAL;
    }

    if (copy_from_user(&report, (void *)arg, sizeof(struct fhp_input_report_data))) {
        FTS_ERROR("copy input touch report data from userspace fail");
        return -EFAULT;
    }

    if ((report.size <= 0) || (report.size > FTS_TOUCH_DATA_LEN_V2)) {
        FTS_ERROR("report size(%d) is invalid", report.size);
        return -EINVAL;
    }

    memset(report_buf, 0xFF, FTS_TOUCH_DATA_LEN_V2);
    if (copy_from_user(report_buf, report.buffer , report.size)) {
        FTS_ERROR("copy input touch report buffer from userspace fail");
        return -EFAULT;
    }

    if ((report_buf[1] == 0xFF) && (report_buf[2] == 0xFF) && (report_buf[3] == 0xFF) && (report_buf[4] == 0xFF)) {
        fts_release_all_finger();
        fts_tp_state_recovery(fhp_data->ts_data);
        return 0;
    }

    ret = fts_input_report_buffer(fhp_data->ts_data, report_buf);
    if (ret < 0) {
        FTS_ERROR("report buffer failed");
        return ret;
    }
    return 0;
}

static int fhp_input_open(struct inode *inode, struct file *file)
{
    return 0;
}

static int fhp_input_close(struct inode *inode, struct file *file)
{
    return 0;
}

static long fhp_input_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    int ret = 0;
    struct fhp_core_data *fhp_data = container_of(filp->private_data, struct fhp_core_data, fhpi_dev);

    if (!fhp_data) {
        FTS_ERROR("fhp_data is null");
        return -ENODATA;
    }

    switch (cmd) {
    case FHP_INPUT_IOCTL_REPORT:
        ret = fhp_input_report(fhp_data, arg);
        break;
    default:
        FTS_ERROR("unkown ioctl cmd(0x%x)", (int)cmd);
        return -EINVAL;
    }

    return ret;
}
/*
* On certain platforms, it is necessary to initialize the open function in order to initialize the private_data of the current device.
* Otherwise, there may be a situation where using container_of to retrieve the device fails.
*/
static struct file_operations fhp_input_fops = {
    .open = fhp_input_open,
    .release = fhp_input_close,
    .unlocked_ioctl = fhp_input_ioctl,
};

static int fhp_input_miscdev_init(struct fhp_core_data *fhp_data)
{
    int ret = 0;

    fhp_data->fhpi_dev.minor = MISC_DYNAMIC_MINOR;
    fhp_data->fhpi_dev.name = FTS_FHPI_DEV_NAME;
    fhp_data->fhpi_dev.fops = &fhp_input_fops;
    ret = misc_register(&fhp_data->fhpi_dev);
    if (ret) {
        FTS_ERROR("misc_register(input misc device) fail");
        return ret;
    }

    FTS_INFO("misc_register(input misc device) success");
    return 0;
}

static int fhp_get_frame(u8 *touch_buf, u32 touch_size)
{
    struct fhp_core_data *fhp_data = fts_fhp_data;
    if (!touch_buf || !touch_size) {
        FTS_ERROR("touch_buf is null/touch_size is 0");
        return -EINVAL;
    }

    if (fhp_data->q.elem_size != (sizeof(struct fhp_frame) + touch_size)) {
        FTS_ERROR("touch size(%u,%d,%lu) is invalid", touch_size, fhp_data->q.elem_size, sizeof(struct fhp_frame));
        return -EINVAL;
    }

    fhpq_enqueue(&fhp_data->q, touch_buf, fhp_get_timestamp());

    if ((fhp_data->frame_waitq_flag == FHP_FRAME_WAITQ_WAIT)
        || (fhp_data->frame_waitq_flag == FHP_FRAME_WAITQ_WAIT_POLL)) {
        fhp_data->frame_waitq_flag = FHP_FRAME_WAITQ_WAKEUP;
        wake_up_interruptible(&fhp_data->frame_waitq);
    }

    return 0;
}

int fts_fhp_irq_handler(struct fts_ts_data *ts_data)
{
    int ret = 0;
    u8 cmd = FHP_CMD_GET_FRAME;

    if (!ts_data || !ts_data->touch_buf) {
        FTS_ERROR("ts_data/touch_buf is null");
        return -EINVAL;
    }

    if (!ts_data->touch_size || (ts_data->touch_size > FTS_MAX_TOUCH_BUF)) {
        FTS_ERROR("touch_size(%d) is invalid", ts_data->touch_size);
        return -EINVAL;
    }

    if (ts_data->suspended && ts_data->gesture_support) {
        fts_gesture_readdata(ts_data, NULL);
        return 0;
    }

    ret = fts_read(&cmd, 1, ts_data->touch_buf, ts_data->touch_size);
    if (ret < 0) {
        FTS_ERROR("read frame data failed");
        return ret;
    }

    ret = fhp_get_frame(ts_data->touch_buf, ts_data->touch_size);
    if (ret < 0) {
        FTS_ERROR("get frame failed");
        return ret;
    }

    return 0;
}

int fts_fhp_init(struct fts_ts_data *ts_data)
{
    int ret = 0;
    struct fhp_core_data *fhp_data = NULL;

    FTS_FUNC_ENTER();
    fhp_data = kzalloc(sizeof(struct fhp_core_data), GFP_KERNEL);
    if (!fhp_data) {
        FTS_ERROR("allocate memory for fhp_data failed");
        return -ENOMEM;
    }

    fts_fhp_data = fhp_data;
    fhp_data->ts_data = ts_data;
    fhp_data->fhp_dev_open_cnt = 0;
    fhp_data->frame_waitq_flag = FHP_FRAME_WAITQ_DEFAULT;
    init_waitqueue_head(&fhp_data->frame_waitq);
    mutex_init(&fhp_data->q.mutexq);

    /*register /dev/fhp_ft*/
    ret = fhp_miscdev_init(fhp_data);
    if (ret) {
        FTS_ERROR("fhp_dev miscdev register fail");
        goto err_miscdev_init;
    }

    ret = fhp_input_miscdev_init(fhp_data);
    if (ret) {
        FTS_ERROR("input misc device register fail");
        goto err_input_miscdev_init;
    }

    FTS_FUNC_EXIT();
    return 0;

err_input_miscdev_init:
    misc_deregister(&fhp_data->fhp_dev);
err_miscdev_init:
    kfree(fhp_data);
    fhp_data = NULL;
    return ret;
}

void fts_fhp_exit(struct fts_ts_data *ts_data)
{
    struct fhp_core_data *fhp_data = fts_fhp_data;
    FTS_FUNC_ENTER();
    if (fhp_data) {
        misc_deregister(&fhp_data->fhpi_dev);
        misc_deregister(&fhp_data->fhp_dev);
        kfree(fhp_data);
        fhp_data = NULL;
    }
    FTS_FUNC_EXIT();
}
