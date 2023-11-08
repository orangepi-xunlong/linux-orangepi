/*
 * Copyright (c) 2015 South Silicon Valley Microelectronics Inc.
 * Copyright (c) 2015 iComm Corporation
 *
 * This program is free software: you can redistribute it and/or modify 
 * it under the terms of the GNU General Public License as published by 
 * the Free Software Foundation, either version 3 of the License, or 
 * (at your option) any later version.
 * This program is distributed in the hope that it will be useful, but 
 * WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  
 * See the GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License 
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <linux/irq.h>
#include <linux/module.h>
#include <linux/vmalloc.h>
#include <linux/platform_device.h>
#include <linux/mmc/sdio.h>
#include <linux/mmc/sdio_func.h>
#include <linux/mmc/sdio_ids.h>
#include <linux/mmc/card.h>
#include <linux/mmc/host.h>
#include "sdio_def.h"
#include <linux/pm_runtime.h>
#include <linux/version.h>
#include <linux/firmware.h>
#include <linux/reboot.h>
#include <ssv6200.h>
#include <linux/skbuff.h>

#define LOW_SPEED_SDIO_CLOCK (25000000)
#define HIGH_SPEED_SDIO_CLOCK (37500000)
#define MAX_RX_FRAME_SIZE 0x900
#define SSV_VENDOR_ID 0x3030
#define SSV_CABRIO_DEVID 0x3030
#define ENABLE_FW_SELF_CHECK 1
#define FW_BLOCK_SIZE 0x8000
#define CHECKSUM_BLOCK_SIZE 1024
#define FW_CHECKSUM_INIT (0x12345678)
#define FW_STATUS_REG ADR_TX_SEG
#define FW_STATUS_MASK (0x00FF0000)

#define ret_if_not_ready(value) \
    do { \
    if ((wlan_data.is_enabled == false) || \
        (glue == NULL) || (glue->dev_ready == false)) { \
        pr_warn("ret_if_not_ready() called when not ready"); \
        return value; }\
    } while(0)

static int ssv6xxx_sdio_trigger_pmu(struct device *dev);
static void ssv6xxx_sdio_reset(struct device *child);

static void ssv6xxx_high_sdio_clk(struct sdio_func *func);
static void ssv6xxx_low_sdio_clk(struct sdio_func *func);
extern void *ssv6xxx_ifdebug_info[];
extern int ssv_devicetype;
extern void ssv6xxx_deinit_prepare(void);

static struct ssv6xxx_platform_data wlan_data;

static int ssv6xxx_sdio_status = 0;
u32 sdio_sr_bhvr = SUSPEND_RESUME_0;
EXPORT_SYMBOL(sdio_sr_bhvr);

u32 shutdown_flags = SSV_SYS_REBOOT;

struct ssv6xxx_sdio_glue {
	struct device *dev;
	struct platform_device *core;
	struct sk_buff *dma_skb;
#ifdef CONFIG_PM
	struct sk_buff *cmd_skb;
#endif
	unsigned int ioport_data;
	unsigned int ioport_reg;
	irq_handler_t irq_handler;
	void *irq_dev;
	bool dev_ready;
};

static const struct sdio_device_id ssv6xxx_sdio_devices[] = {
	{SDIO_DEVICE(SSV_VENDOR_ID, SSV_CABRIO_DEVID)},
	{}
};

MODULE_DEVICE_TABLE(sdio, ssv6xxx_sdio_devices);

static bool ssv6xxx_is_ready(struct device *child)
{
	struct ssv6xxx_sdio_glue *glue = dev_get_drvdata(child->parent);

	ret_if_not_ready(false);

	return true;
}

static int ssv6xxx_sdio_cmd52_read(struct device *child, u32 addr, u32 * value)
{
	int ret;
	struct ssv6xxx_sdio_glue *glue = dev_get_drvdata(child->parent);
	struct sdio_func *func;

	ret_if_not_ready(-1);

    func = dev_to_sdio_func(glue->dev);
    sdio_claim_host(func);
    *value = sdio_readb(func, addr, &ret);
    sdio_release_host(func);

	return ret;
}

static int ssv6xxx_sdio_cmd52_write(struct device *child, u32 addr, u32 value)
{
	int ret;
	struct ssv6xxx_sdio_glue *glue = dev_get_drvdata(child->parent);
	struct sdio_func *func;

    ret_if_not_ready(-1);

	func = dev_to_sdio_func(glue->dev);
    sdio_claim_host(func);
    sdio_writeb(func, value, addr, &ret);
    sdio_release_host(func);

	return ret;
}

static int __must_check
ssv6xxx_sdio_read_reg(struct device *child, u32 addr, u32 * buf)
{
	int ret;

	struct ssv6xxx_sdio_glue *glue = dev_get_drvdata(child->parent);
	struct sdio_func *func;
	u32 data;

    ret_if_not_ready(-1);

    func = dev_to_sdio_func(glue->dev);

    sdio_claim_host(func);

    data = addr;

    sdio_writel(func, addr, glue->ioport_reg, &ret);

    if (unlikely(ret)) {
        dev_err(child->parent, "sdio read reg write address failed (%d)\n", ret);
        goto io_err;
    }

    data = sdio_readl(func, glue->ioport_reg, &ret);

    if (unlikely(ret)) {
        *buf = 0xffffffff;
        dev_err(child->parent, "sdio read reg from I/O failed (%d)\n", ret);
        goto io_err;
    }

    *buf = data;

io_err:
    sdio_release_host(func);

	return ret;
}

#ifdef ENABLE_WAKE_IO_ISR_WHEN_HCI_ENQUEUE
static int ssv6xxx_sdio_trigger_tx_rx(struct device *child)
{
	struct ssv6xxx_sdio_glue *glue = dev_get_drvdata(child->parent);
	struct sdio_func *func;
	struct mmc_host *host;

	if (glue == NULL)
		return -1;

	func = dev_to_sdio_func(glue->dev);
	host = func->card->host;
	mmc_signal_sdio_irq(host);

	return 0;

}
#endif

static int __must_check
ssv6xxx_sdio_write_reg(struct device *child, u32 addr, u32 buf)
{
	int ret;

	struct ssv6xxx_sdio_glue *glue = dev_get_drvdata(child->parent);
	struct sdio_func *func;

    u32 data[2];

    ret_if_not_ready(-1);

    func = dev_to_sdio_func(glue->dev);

    sdio_claim_host(func);
    data[0] = addr;
    data[1] = buf;

    ret = sdio_memcpy_toio(func, glue->ioport_reg, data, sizeof(data));
    sdio_release_host(func);

	return ret;
}

static int
ssv6xxx_sdio_write_sram(struct device *child, u32 addr, u8 * data, u32 size)
{
	int ret = 0;
	struct ssv6xxx_sdio_glue *glue;
	struct sdio_func *func = NULL;
	glue = dev_get_drvdata(child->parent);

    ret_if_not_ready(-1);

	func = dev_to_sdio_func(glue->dev);
	sdio_claim_host(func);

    ret |= ssv6xxx_sdio_write_reg(child, 0xc0000860, addr);
    if (unlikely(ret))
        goto out;

    sdio_writeb(func, 0x2, REG_Fn1_STATUS, &ret);
    if (unlikely(ret))
        goto out;

    ret = sdio_memcpy_toio(func, glue->ioport_data, data, size);
    if (unlikely(ret))
        goto out;

    sdio_writeb(func, 0, REG_Fn1_STATUS, &ret);
    if (unlikely(ret))
        goto out;

out:
	sdio_release_host(func);
	return ret;

}

struct file *ssv6xxx_open_firmware(char *user_mainfw)
{
	struct file *fp;
	fp = filp_open(user_mainfw, O_RDONLY, 0);

	if (IS_ERR(fp))
		fp = NULL;

	return fp;
}

int ssv6xxx_read_fw_block(char *buf, int len, struct file *fp)
{

	int read;
	loff_t pos;

	pos = fp->f_pos;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,14,0)
	read = kernel_read(fp, (void *)buf, len, &pos);
#else
	read = kernel_read(fp, pos, buf, len);
#endif

	if (read > 0)
		fp->f_pos += read;

	return read;

}

void ssv6xxx_close_firmware(struct file *fp)
{
	if (fp)
		filp_close(fp, NULL);
}

static int
ssv6xxx_sdio_upload_firmware(struct device *child, const u8 *firmware, u32 firmware_length)
{
	int ret;
    u32 clk_en;
	u32 word_count, i;
    u32 block_size;
	u8 *buffer;
	u32 sram_ptr = 0;
	u32 block_count = 0;
    u32 firmware_ptr = 0;

	u32 checksum = FW_CHECKSUM_INIT;
	u32 fw_checksum, fw_blkcnt;

	struct ssv6xxx_sdio_glue *glue;

	glue = dev_get_drvdata(child->parent);

    if ((wlan_data.is_enabled == false) &&
        (glue == NULL) &&
        (glue->dev_ready == false))
        goto out;

    buffer = (u8 *)kzalloc(FW_BLOCK_SIZE, GFP_KERNEL);
    if (buffer == NULL) {
        dev_err(child, "Failed to allocate buffer for firmware.\n");
        ret = -ENOMEM;
        goto out;
    }

    dev_dbg(child, "preparing registers and clock for firmware upload\n");

    ret = ssv6xxx_sdio_write_reg(child, ADR_BRG_SW_RST, 0x0);
    if (unlikely(ret))
        goto out;

    ret = ssv6xxx_sdio_write_reg(child, ADR_BOOT, 0x01);
    if (unlikely(ret))
        goto out;

    ret = ssv6xxx_sdio_read_reg(child, ADR_PLATFORM_CLOCK_ENABLE, &clk_en);
    if (unlikely(ret))
        goto out;

    ret = ssv6xxx_sdio_write_reg(child, ADR_PLATFORM_CLOCK_ENABLE, clk_en | (1 << 2));
    if (unlikely(ret))
        goto out;

    dev_dbg(child, "begin writing firmware\n");

    while (firmware_length > 0) {

        memset(buffer, 0xA5, FW_BLOCK_SIZE);

        block_size = firmware_length;
        if (block_size > FW_BLOCK_SIZE)
            block_size = FW_BLOCK_SIZE;

        memcpy(buffer, &firmware[firmware_ptr], block_size);

        firmware_ptr += block_size;
        firmware_length -= block_size;

        /*
         * Uploading to chip sram and checksumming happens in chunks of CHECKSUM_BLOCK_SIZE,
         * so we round the block size accordingly and use that valueù
         */
        block_size = DIV_ROUND_UP(block_size, CHECKSUM_BLOCK_SIZE) * CHECKSUM_BLOCK_SIZE;
        ret = ssv6xxx_sdio_write_sram(child, sram_ptr, (u8 *)buffer, block_size);

        if (ret) {
            dev_err(child, "firmware upload failed\n");
            goto out;
        }

        sram_ptr += block_size;

        word_count = block_size / sizeof(u32);
        for (i = 0; i < word_count; i++)
            checksum += ((u32 *)buffer)[i];

    }

    checksum = ((checksum >> 24) +
                (checksum >> 16) +
                (checksum >> 8) +
                checksum) & 0x0FF;
    checksum <<= 16;

    block_count = DIV_ROUND_UP(sram_ptr, CHECKSUM_BLOCK_SIZE);
    ret = ssv6xxx_sdio_write_reg(child, FW_STATUS_REG, (block_count << 16));
    if (unlikely(ret))
        goto out;

    ret = ssv6xxx_sdio_read_reg(child, FW_STATUS_REG, &fw_blkcnt);
    if (unlikely(ret))
        goto out;

    ret = ssv6xxx_sdio_write_reg(child, ADR_BRG_SW_RST, 0x1);
    if (unlikely(ret))
        goto out;

    dev_info(child, "firmware upload complete (wrote %d blocks, verified %d blocks)\n", block_count, fw_blkcnt >> 16);

    msleep(50);

    ret = ssv6xxx_sdio_read_reg(child, FW_STATUS_REG, &fw_checksum);
    fw_checksum = fw_checksum & FW_STATUS_MASK;

    if (fw_checksum == checksum) {
        dev_dbg(child, "firmware check ok, checksum=0x%x\n", checksum);
        ret = ssv6xxx_sdio_write_reg(child, FW_STATUS_REG, (~checksum & FW_STATUS_MASK));
        if (unlikely(ret))
            dev_warn(child, "could not clear checksum condition");
    } else {
        dev_err(child, "firmware checksum mismatch, local=0x%x, sram=0x%x\n", checksum, fw_checksum);
    }

    msleep(50);

    ret = 0;

 out:

	if (buffer)
		kfree(buffer);

	return ret;

}

static int
ssv6xxx_sdio_load_firmware(struct device *child, u8 *firmware_name, u8 openfile)
{

    int ret;
    const struct firmware *firmware = NULL;
    struct sdio_func *func;
   	struct ssv6xxx_sdio_glue *glue;

    glue = dev_get_drvdata(child->parent);

    ret = request_firmware(&firmware, firmware_name, glue->dev);

    if (ret) {
        dev_err(child, "could not find firmware file %s, err=%d\n", firmware_name, ret);
        goto out;
    }

    ret = ssv6xxx_sdio_upload_firmware(child, firmware->data, firmware->size);

    if (ret) {
        dev_err(child, "could not upload firmware to device, err=%d\n", ret);
        goto out;
    }

    if (glue != NULL) {
		func = dev_to_sdio_func(glue->dev);
		ssv6xxx_high_sdio_clk(func);
	}

out:
    if (firmware != NULL)
        release_firmware(firmware);

    return ret;

}

static int ssv6xxx_sdio_irq_getstatus(struct device *child, int *status)
{
	int ret = (-1);
	struct ssv6xxx_sdio_glue *glue;
	struct sdio_func *func;
	glue = dev_get_drvdata(child->parent);

    ret_if_not_ready(-1);

    func = dev_to_sdio_func(glue->dev);
    sdio_claim_host(func);
    *status = sdio_readb(func, REG_INT_STATUS, &ret);
    sdio_release_host(func);

    return ret;

}

static int __must_check
ssv6xxx_sdio_read(struct device *child, void *buf, size_t *size)
{

	int ret;
    u32 data_size;

	struct ssv6xxx_sdio_glue *glue = dev_get_drvdata(child->parent);
	struct sdio_func *func;

    ret_if_not_ready(-1);

    func = dev_to_sdio_func(glue->dev);
    sdio_claim_host(func);

    data_size = sdio_readb(func, REG_CARD_PKT_LEN_0, &ret);

    if (unlikely(ret)) {
        dev_err(child->parent, "sdio read high byte len failed, ret=%d\n", ret);
        goto out;
    }

    data_size = data_size | (sdio_readb(func, REG_CARD_PKT_LEN_1, &ret) << 0x8);

    if (unlikely(ret)) {
        dev_err(child->parent, "sdio read low len failed ret[%d]\n", ret);
        goto out;
    }

    ret = sdio_memcpy_fromio(func, buf, glue->ioport_data, sdio_align_size(func, data_size));

    if (unlikely(ret)) {
        dev_err(child->parent, "sdio read failed size ret[%d]\n", ret);
        goto out;
    }
    
    *size = data_size;

out:

    sdio_release_host(func);

	return ret;
}

static int __must_check
ssv6xxx_sdio_write(struct device *child, void *buf, size_t len, u8 queue_num)
{
	int ret;
	struct ssv6xxx_sdio_glue *glue = dev_get_drvdata(child->parent);
	struct sdio_func *func;
	void *ptr;

    ret_if_not_ready(-1);

#ifdef CONFIG_ARM64
    if (((u64) buf) & 3) {
#else
    if (((u32) buf) & 3) {
#endif
        memcpy(glue->dma_skb->data, buf, len);
        ptr = glue->dma_skb->data;
    } else
        ptr = buf;

    func = dev_to_sdio_func(glue->dev);

    sdio_claim_host(func);

    len = sdio_align_size(func, len);
    ret = sdio_memcpy_toio(func, glue->ioport_data, ptr, len);

    if (unlikely(ret))
        dev_err(glue->dev, "sdio write failed, ret=%d\n", ret);

    sdio_release_host(func);

    return ret;

}

static void ssv6xxx_sdio_irq_handler(struct sdio_func *func)
{
	int status;
	struct ssv6xxx_sdio_glue *glue = sdio_get_drvdata(func);
	struct ssv6xxx_platform_data *pwlan_data = &wlan_data;

    ret_if_not_ready();

    if (glue->irq_handler == NULL)
        return;

    atomic_set(&pwlan_data->irq_handling, 1);
    sdio_release_host(func);
    if (glue->irq_handler != NULL)
        status = glue->irq_handler(0, glue->irq_dev);
    sdio_claim_host(func);
    atomic_set(&pwlan_data->irq_handling, 0);

}

static void ssv6xxx_sdio_irq_setmask(struct device *child, int mask)
{
	int err_ret;
	struct ssv6xxx_sdio_glue *glue = dev_get_drvdata(child->parent);
	struct sdio_func *func;

    ret_if_not_ready();

    func = dev_to_sdio_func(glue->dev);
    sdio_claim_host(func);
    sdio_writeb(func, mask, REG_INT_MASK, &err_ret);
    sdio_release_host(func);

}

static void ssv6xxx_sdio_irq_trigger(struct device *child)
{
	int err_ret;
	struct ssv6xxx_sdio_glue *glue = dev_get_drvdata(child->parent);
	struct sdio_func *func;

    ret_if_not_ready();

    func = dev_to_sdio_func(glue->dev);
    sdio_claim_host(func);
    sdio_writeb(func, 0x2, REG_INT_TRIGGER, &err_ret);
    sdio_release_host(func);

}

static int ssv6xxx_sdio_irq_getmask(struct device *child, u32 * mask)
{
	u8 imask = 0;
	int ret = (-1);
	struct ssv6xxx_sdio_glue *glue = dev_get_drvdata(child->parent);
	struct sdio_func *func;

    ret_if_not_ready(-1);

    func = dev_to_sdio_func(glue->dev);
    sdio_claim_host(func);
    imask = sdio_readb(func, REG_INT_MASK, &ret);
    *mask = imask;
    sdio_release_host(func);

	return ret;

}

static void ssv6xxx_sdio_irq_enable(struct device *child)
{
	struct ssv6xxx_sdio_glue *glue = dev_get_drvdata(child->parent);
	struct sdio_func *func;
	int ret;
	struct ssv6xxx_platform_data *pwlan_data = &wlan_data;
	if ((pwlan_data->is_enabled == false)
	    || (glue == NULL) || (glue->dev_ready == false))
		return;

    func = dev_to_sdio_func(glue->dev);
    sdio_claim_host(func);
    ret = sdio_claim_irq(func, ssv6xxx_sdio_irq_handler);
    if (ret)
        dev_err(child->parent, "Failed to claim sdio irq: %d\n",
            ret);
    sdio_release_host(func);

	dev_dbg(child, "ssv6xxx_sdio_irq_enable\n");

}

static void ssv6xxx_sdio_irq_disable(struct device *child, bool iswaitirq)
{
	struct ssv6xxx_sdio_glue *glue = NULL;
	struct sdio_func *func;
	struct ssv6xxx_platform_data *pwlan_data = &wlan_data;
	int ret;

	dev_dbg(child, "ssv6xxx_sdio_irq_disable\n");

	if ((wlan_data.is_enabled == false) || (child->parent == NULL))
		return;

	glue = dev_get_drvdata(child->parent);


	if ((glue == NULL) || (glue->dev_ready == false)
	    || (glue->dev == NULL))
		return;

    func = dev_to_sdio_func(glue->dev);

    if (func == NULL) {
        dev_dbg(child, "sdio func == NULL\n");
        return;
    }

    sdio_claim_host(func);
    while (atomic_read(&pwlan_data->irq_handling)) {
        sdio_release_host(func);
        schedule_timeout(HZ / 10);
        sdio_claim_host(func);
    }
    ret = sdio_release_irq(func);

    if (ret)
        dev_err(child->parent,
            "Failed to release sdio irq: %d\n", ret);

    sdio_release_host(func);

}

static void
ssv6xxx_sdio_irq_request(struct device *child, irq_handler_t irq_handler,
			 void *irq_dev)
{
	struct ssv6xxx_sdio_glue *glue = dev_get_drvdata(child->parent);
	struct sdio_func *func;
	bool isIrqEn = false;

    ret_if_not_ready();

    func = dev_to_sdio_func(glue->dev);
    glue->irq_handler = irq_handler;
    glue->irq_dev = irq_dev;
    if (isIrqEn) {
        ssv6xxx_sdio_irq_enable(child);
    }

}

static void
ssv6xxx_sdio_read_parameter(struct sdio_func *func,
			    struct ssv6xxx_sdio_glue *glue)
{
	int err_ret;
	sdio_claim_host(func);
	glue->ioport_data = 0;
	glue->ioport_data =
	    glue->ioport_data | (sdio_readb(func, REG_DATA_IO_PORT_0, &err_ret)
				<< (8 * 0));
	glue->ioport_data =
	    glue->ioport_data | (sdio_readb(func, REG_DATA_IO_PORT_1, &err_ret)
				<< (8 * 1));
	glue->ioport_data =
	    glue->ioport_data | (sdio_readb(func, REG_DATA_IO_PORT_2, &err_ret)
				<< (8 * 2));
	glue->ioport_reg = 0;
	glue->ioport_reg =
	    glue->ioport_reg | (sdio_readb(func, REG_REG_IO_PORT_0, &err_ret) <<
			       (8 * 0));
	glue->ioport_reg =
	    glue->ioport_reg | (sdio_readb(func, REG_REG_IO_PORT_1, &err_ret) <<
			       (8 * 1));
	glue->ioport_reg =
	    glue->ioport_reg | (sdio_readb(func, REG_REG_IO_PORT_2, &err_ret) <<
			       (8 * 2));
	dev_dbg(&func->dev, "ioport_data=0x%x ioport_reg=0x%x\n",
		glue->ioport_data, glue->ioport_reg);
	err_ret = sdio_set_block_size(func, CONFIG_PLATFORM_SDIO_BLOCK_SIZE);
	if (err_ret != 0) {
		dev_warn(&func->dev, "SDIO setting SDIO_DEF_BLOCK_SIZE fail!!\n");
	}
	sdio_writeb(func, CONFIG_PLATFORM_SDIO_OUTPUT_TIMING,
		    REG_OUTPUT_TIMING_REG, &err_ret);
	sdio_writeb(func, 0x00, REG_Fn1_STATUS, &err_ret);
	sdio_release_host(func);
}

static void ssv6xxx_do_sdio_wakeup(struct sdio_func *func)
{
	int err_ret;
	if (func != NULL) {
		sdio_claim_host(func);
		sdio_writeb(func, 0x01, REG_PMU_WAKEUP, &err_ret);
		mdelay(10);
		sdio_writeb(func, 0x00, REG_PMU_WAKEUP, &err_ret);
		sdio_release_host(func);
	}
}

static void ssv6xxx_sdio_pmu_wakeup(struct device *child)
{
	struct ssv6xxx_sdio_glue *glue = dev_get_drvdata(child->parent);
	struct sdio_func *func;
	if (glue != NULL) {
		func = dev_to_sdio_func(glue->dev);
		ssv6xxx_do_sdio_wakeup(func);
	}
}

static bool ssv6xxx_sdio_support_scatter(struct device *child)
{
	struct ssv6xxx_sdio_glue *glue = dev_get_drvdata(child->parent);
	struct sdio_func *func;

    if (!glue) {
        dev_err(child->parent, "ssv6xxx_sdio_enable_scatter glue == NULL!!!\n");
        return false;
    }

    func = dev_to_sdio_func(glue->dev);

    if (func->card->host->max_segs < MAX_SCATTER_ENTRIES_PER_REQ) {
        dev_err(child->parent,
            "host controller only supports scatter of :%d entries, driver need: %d\n",
            func->card->host->max_segs,
            MAX_SCATTER_ENTRIES_PER_REQ);
        return false;
    }

    return true;

}

static void
ssv6xxx_sdio_setup_scat_data(struct sdio_scatter_req *scat_req,
			     struct mmc_data *data)
{
	struct scatterlist *sg;
	int i;
	data->blksz = SDIO_DEF_BLOCK_SIZE;
	data->blocks = scat_req->len / SDIO_DEF_BLOCK_SIZE;
	pr_debug
	    ("scatter: (%s)  (block len: %d, block count: %d) , (tot:%d,sg:%d)\n",
	     (scat_req->req & SDIO_WRITE) ? "WR" : "RD", data->blksz,
	     data->blocks, scat_req->len, scat_req->scat_entries);
	data->flags =
	    (scat_req->req & SDIO_WRITE) ? MMC_DATA_WRITE : MMC_DATA_READ;
	sg = scat_req->sgentries;
	sg_init_table(sg, scat_req->scat_entries);
	for (i = 0; i < scat_req->scat_entries; i++, sg++) {
		pr_debug("%d: addr:0x%p, len:%d\n",
		       i, scat_req->scat_list[i].buf,
		       scat_req->scat_list[i].len);
		sg_set_buf(sg, scat_req->scat_list[i].buf,
			   scat_req->scat_list[i].len);
	}
	data->sg = scat_req->sgentries;
	data->sg_len = scat_req->scat_entries;
}

static inline void
ssv6xxx_sdio_set_cmd53_arg(u32 * arg, u8 rw, u8 func,
			   u8 mode, u8 opcode, u32 addr, u16 blksz)
{
	*arg = (((rw & 1) << 31) |
		((func & 0x7) << 28) |
		((mode & 1) << 27) |
		((opcode & 1) << 26) | ((addr & 0x1FFFF) << 9) | (blksz &
								  0x1FF));
}

static int
ssv6xxx_sdio_rw_scatter(struct device *child, struct sdio_scatter_req *scat_req)
{
	struct ssv6xxx_sdio_glue *glue = dev_get_drvdata(child->parent);
	struct sdio_func *func;
	struct mmc_request mmc_req;
	struct mmc_command cmd;
	struct mmc_data data;
	u8 opcode, rw;
	int status = 1;

    if (!glue) {
        dev_err(child->parent, "ssv6xxx_sdio_enable_scatter glue == NULL!!!\n");
        return 1;
    }

    func = dev_to_sdio_func(glue->dev);
    memset(&mmc_req, 0, sizeof(struct mmc_request));
    memset(&cmd, 0, sizeof(struct mmc_command));
    memset(&data, 0, sizeof(struct mmc_data));
    ssv6xxx_sdio_setup_scat_data(scat_req, &data);
    opcode = 0;
    rw = (scat_req->req & SDIO_WRITE) ? CMD53_ARG_WRITE :
        CMD53_ARG_READ;
    ssv6xxx_sdio_set_cmd53_arg(&cmd.arg, rw, func->num,
                    CMD53_ARG_BLOCK_BASIS, opcode,
                    glue->ioport_data, data.blocks);
    cmd.opcode = SD_IO_RW_EXTENDED;
    cmd.flags = MMC_RSP_SPI_R5 | MMC_RSP_R5 | MMC_CMD_ADTC;
    mmc_req.cmd = &cmd;
    mmc_req.data = &data;
    mmc_set_data_timeout(&data, func->card);
    mmc_wait_for_req(func->card->host, &mmc_req);

    status = cmd.error ? cmd.error : data.error;

    if (cmd.error)
        return cmd.error;

    if (data.error)
        return data.error;

    return status;

}

static void ssv6xxx_set_sdio_clk(struct sdio_func *func, u32 sdio_hz)
{
	struct mmc_host *host;
	host = func->card->host;
	if (sdio_hz < host->f_min)
		sdio_hz = host->f_min;
	else if (sdio_hz > host->f_max)
		sdio_hz = host->f_max;
	dev_dbg(&func->dev, "%s:set sdio clk %dHz\n", __FUNCTION__, sdio_hz);
	sdio_claim_host(func);
	host->ios.clock = sdio_hz;
	host->ops->set_ios(host, &host->ios);
	mdelay(20);
	sdio_release_host(func);
}

static void ssv6xxx_low_sdio_clk(struct sdio_func *func)
{
	ssv6xxx_set_sdio_clk(func, LOW_SPEED_SDIO_CLOCK);
}

static void ssv6xxx_high_sdio_clk(struct sdio_func *func)
{
#ifndef SDIO_USE_SLOW_CLOCK
	ssv6xxx_set_sdio_clk(func, HIGH_SPEED_SDIO_CLOCK);
#endif
}

static struct ssv6xxx_hwif_ops sdio_ops = {
	.read = ssv6xxx_sdio_read,
	.write = ssv6xxx_sdio_write,
	.readreg = ssv6xxx_sdio_read_reg,
	.writereg = ssv6xxx_sdio_write_reg,
#ifdef ENABLE_WAKE_IO_ISR_WHEN_HCI_ENQUEUE
	.trigger_tx_rx = ssv6xxx_sdio_trigger_tx_rx,
#endif
	.irq_getmask = ssv6xxx_sdio_irq_getmask,
	.irq_setmask = ssv6xxx_sdio_irq_setmask,
	.irq_enable = ssv6xxx_sdio_irq_enable,
	.irq_disable = ssv6xxx_sdio_irq_disable,
	.irq_getstatus = ssv6xxx_sdio_irq_getstatus,
	.irq_request = ssv6xxx_sdio_irq_request,
	.irq_trigger = ssv6xxx_sdio_irq_trigger,
	.pmu_wakeup = ssv6xxx_sdio_pmu_wakeup,
	.load_fw = ssv6xxx_sdio_load_firmware,
	.cmd52_read = ssv6xxx_sdio_cmd52_read,
	.cmd52_write = ssv6xxx_sdio_cmd52_write,
	.support_scatter = ssv6xxx_sdio_support_scatter,
	.rw_scatter = ssv6xxx_sdio_rw_scatter,
	.is_ready = ssv6xxx_is_ready,
	.write_sram = ssv6xxx_sdio_write_sram,
	.interface_reset = ssv6xxx_sdio_reset,
};

static int
ssv6xxx_sdio_power_on(struct ssv6xxx_platform_data *pdata,
		      struct sdio_func *func)
{
	int ret = 0;
	if (pdata->is_enabled == true)
		return 0;

	dev_dbg(&func->dev, "ssv6xxx_sdio_power_on\n");

	sdio_claim_host(func);
	ret = sdio_enable_func(func);
	sdio_release_host(func);

	if (ret) {
		dev_err(&func->dev, "Unable to enable sdio func: %d)\n", ret);
		return ret;
	}

	msleep(10);
	pdata->is_enabled = true;

	return ret;
}

static int
ssv6xxx_sdio_power_off(struct ssv6xxx_platform_data *pdata,
		       struct sdio_func *func)
{
	int ret;
	if (pdata->is_enabled == false)
		return 0;
	dev_dbg(&func->dev, "ssv6xxx_sdio_power_off\n");
	sdio_claim_host(func);
	ret = sdio_disable_func(func);
	sdio_release_host(func);
	if (ret)
		return ret;
	pdata->is_enabled = false;
	return ret;
}

int ssv6xxx_get_dev_status(void)
{
	return ssv6xxx_sdio_status;
}

EXPORT_SYMBOL(ssv6xxx_get_dev_status);

static int
ssv6xxx_sdio_probe(struct sdio_func *func, const struct sdio_device_id *id)
{
	struct ssv6xxx_platform_data *pwlan_data = &wlan_data;
	struct ssv6xxx_sdio_glue *glue;
	int ret;
	const char *chip_family = "ssv6200";

	if (ssv_devicetype != 0) {
		dev_info(&func->dev, "Not using SSV6200 normal SDIO driver.\n");
		return -ENODEV;
	}

	if (func->num != 0x01)
		return -ENODEV;

	glue = kzalloc(sizeof(*glue), GFP_KERNEL);

	if (!glue) {
		dev_err(&func->dev, "can't allocate glue\n");
		return -ENOMEM;
	}

	ssv6xxx_sdio_status = 1;
	ssv6xxx_low_sdio_clk(func);

	glue->dma_skb = __dev_alloc_skb(SDIO_DMA_BUFFER_LEN, GFP_KERNEL);

#ifdef CONFIG_PM
	glue->cmd_skb = __dev_alloc_skb(SDIO_COMMAND_BUFFER_LEN, GFP_KERNEL);
#endif
	memset(pwlan_data, 0, sizeof(struct ssv6xxx_platform_data));
	atomic_set(&pwlan_data->irq_handling, 0);
	glue->dev = &func->dev;
	func->card->quirks |= MMC_QUIRK_LENIENT_FN0;
	func->card->quirks |= MMC_QUIRK_BLKSZ_FOR_BYTE_MODE;
	glue->dev_ready = true;
	pwlan_data->vendor = func->vendor;
	pwlan_data->device = func->device;
	dev_info(glue->dev, "device id: %x:%x\n", pwlan_data->vendor,
		pwlan_data->device);
	pwlan_data->ops = &sdio_ops;
	sdio_set_drvdata(func, glue);
#ifdef CONFIG_PM
	ssv6xxx_do_sdio_wakeup(func);
#endif
	ssv6xxx_sdio_power_on(pwlan_data, func);
	ssv6xxx_sdio_read_parameter(func, glue);
	glue->core = platform_device_alloc(chip_family, -1);

	if (!glue->core) {
		dev_err(glue->dev, "can't allocate platform_device");
		ret = -ENOMEM;
		goto out_free_glue;
	}

	glue->core->dev.parent = &func->dev;

	ret = platform_device_add_data(glue->core, pwlan_data,
				       sizeof(*pwlan_data));

	if (ret) {
		dev_err(glue->dev, "can't add platform data\n");
		goto out_dev_put;
	}

	ret = platform_device_add(glue->core);

	if (ret) {
		dev_err(glue->dev, "can't add platform device\n");
		goto out_dev_put;
	}

	ssv6xxx_sdio_irq_setmask(&glue->core->dev, 0xff);

	ssv6xxx_ifdebug_info[0] = (void *)&glue->core->dev;
	ssv6xxx_ifdebug_info[1] = (void *)glue->core;
	ssv6xxx_ifdebug_info[2] = (void *)&sdio_ops;
	return 0;

 out_dev_put:
	platform_device_put(glue->core);
 out_free_glue:
	kfree(glue);

	return ret;

}

static void ssv6xxx_sdio_remove(struct sdio_func *func)
{
	struct ssv6xxx_sdio_glue *glue = sdio_get_drvdata(func);
	struct ssv6xxx_platform_data *pwlan_data = &wlan_data;

	dev_dbg(&func->dev, "ssv6xxx_sdio_remove enter\n");

	ssv6xxx_sdio_status = 0;

	if (glue) {
		dev_dbg(&func->dev, "ssv6xxx_sdio_remove - ssv6xxx_sdio_irq_disable\n");
		ssv6xxx_sdio_irq_disable(&glue->core->dev, false);
		glue->dev_ready = false;
		ssv6xxx_low_sdio_clk(func);

		if (glue->dma_skb != NULL)
			dev_kfree_skb(glue->dma_skb);

		dev_dbg(&func->dev, "ssv6xxx_sdio_remove - disable mask\n");
		ssv6xxx_sdio_irq_setmask(&glue->core->dev, 0xff);
#ifdef CONFIG_PM
		ssv6xxx_sdio_trigger_pmu(glue->dev);
		if (glue->cmd_skb != NULL)
			dev_kfree_skb(glue->cmd_skb);
#endif
		ssv6xxx_sdio_power_off(pwlan_data, func);
		dev_dbg(&func->dev, "platform_device_del \n");
		platform_device_del(glue->core);
		dev_dbg(&func->dev, "platform_device_put \n");
		platform_device_put(glue->core);
		kfree(glue);
	}

	sdio_set_drvdata(func, NULL);
	dev_dbg(&func->dev, "ssv6xxx_sdio_remove leave\n");

}

static int ssv6xxx_sdio_trigger_pmu(struct device *dev)
{

	int ret = 0;

#ifdef CONFIG_PM
	struct sdio_func *func = dev_to_sdio_func(dev);
	struct ssv6xxx_sdio_glue *glue = sdio_get_drvdata(func);
	struct cfg_host_cmd *host_cmd;
	int writesize;
	void *tempPointer;

	if (ssv6xxx_sdio_write_reg
	    (dev, ADR_RX_FLOW_MNG, M_ENG_MACRX | (M_ENG_TRASH_CAN << 4))) ;
	if (ssv6xxx_sdio_write_reg
	    (dev, ADR_RX_FLOW_DATA, M_ENG_MACRX | (M_ENG_TRASH_CAN << 4))) ;
	if (ssv6xxx_sdio_write_reg
	    (dev, ADR_RX_FLOW_CTRL, M_ENG_MACRX | (M_ENG_TRASH_CAN << 4))) ;

	host_cmd = (struct cfg_host_cmd *)glue->cmd_skb->data;
	host_cmd->c_type = HOST_CMD;
	host_cmd->RSVD0 = 0;
	host_cmd->h_cmd = (u8) SSV6XXX_HOST_CMD_PS;
	host_cmd->len = sizeof(struct cfg_host_cmd);

	host_cmd->dummy = 0;

	{
		tempPointer = glue->cmd_skb->data;
		sdio_claim_host(func);
		writesize = sdio_align_size(func, sizeof(struct cfg_host_cmd));
		do {
			ret =
			    sdio_memcpy_toio(func, glue->ioport_data,
					     tempPointer, writesize);
			if (ret == -EILSEQ || ret == -ETIMEDOUT) {
				ret = -1;
				break;
			} else {
				if (ret)
					dev_err(glue->dev,
						"Unexpected return value ret=[%d]\n",
						ret);
			}
		}
		while (ret == -EILSEQ || ret == -ETIMEDOUT);
		sdio_release_host(func);
		if (ret)
			dev_err(glue->dev, "sdio write failed (%d)\n", ret);
	}

#endif

	return ret;

}

static void ssv6xxx_sdio_reset(struct device *child)
{

#ifdef CONFIG_PM
	struct ssv6xxx_sdio_glue *glue = dev_get_drvdata(child->parent);
	struct sdio_func *func = dev_to_sdio_func(glue->dev);
	dev_dbg(child, "%s\n", __FUNCTION__);
	if (glue == NULL || glue->dev == NULL || func == NULL)
		return;
	ssv6xxx_sdio_trigger_pmu(glue->dev);
	ssv6xxx_do_sdio_wakeup(func);
#endif

	return;

}

#ifdef CONFIG_PM
static int ssv6xxx_sdio_suspend(struct device *dev)
{
	struct sdio_func *func = dev_to_sdio_func(dev);
	mmc_pm_flag_t flags = sdio_get_host_pm_caps(func);
	{
		int ret = 0;
		dev_info(dev, "%s: suspend: PM flags = 0x%x\n",
			 sdio_func_id(func), flags);
		ssv6xxx_low_sdio_clk(func);
		ret = ssv6xxx_sdio_trigger_pmu(dev);
		if (ret)
			dev_warn(dev, "ssv6xxx_sdio_trigger_pmu fail!!\n");
		if (!(flags & MMC_PM_KEEP_POWER)) {
			dev_err(dev,
				"%s: cannot remain alive while host is suspended\n",
				sdio_func_id(func));
		}
		ret = sdio_set_host_pm_flags(func, MMC_PM_KEEP_POWER);
		if (ret)
			return ret;
		mdelay(10);
		return ret;
	}
}

static int ssv6xxx_sdio_resume(struct device *dev)
{
	struct sdio_func *func = dev_to_sdio_func(dev);
	{
		dev_dbg(dev, "ssv6xxx_sdio_resume\n");
		{
			ssv6xxx_do_sdio_wakeup(func);
			mdelay(10);
			ssv6xxx_high_sdio_clk(func);
			mdelay(10);
		}
	}
	return 0;
}

static const struct dev_pm_ops ssv6xxx_sdio_pm_ops = {
	.suspend = ssv6xxx_sdio_suspend,
	.resume = ssv6xxx_sdio_resume,
};
#endif

struct sdio_driver ssv6xxx_sdio_driver = {
	.name = "ssv6051",
	.id_table = ssv6xxx_sdio_devices,
	.probe = ssv6xxx_sdio_probe,
	.remove = ssv6xxx_sdio_remove,
#ifdef CONFIG_PM
	.drv = {
		.pm = &ssv6xxx_sdio_pm_ops,
		},
#endif
};

EXPORT_SYMBOL(ssv6xxx_sdio_driver);

int ssv6xxx_sdio_init(void)
{
	return sdio_register_driver(&ssv6xxx_sdio_driver);
}

void ssv6xxx_sdio_exit(void)
{
	pr_info("ssv6xxx_sdio_exit\n");
	sdio_unregister_driver(&ssv6xxx_sdio_driver);
}

EXPORT_SYMBOL(ssv6xxx_sdio_init);
EXPORT_SYMBOL(ssv6xxx_sdio_exit);
