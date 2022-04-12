/**
 ******************************************************************************
 *
 * @file bl_v7.c - Support for v7 platform
 *
 * Copyright (C) BouffaloLab 2017-2018
 *
 ******************************************************************************
 */

#include "bl_v7.h"
#include "bl_defs.h"
#include "bl_irqs.h"
#include "bl_sdio.h"


void bl_device_deinit(struct bl_plat *bl_plat)
{

	sdio_claim_host(bl_plat->func);
	sdio_release_irq(bl_plat->func);
	sdio_disable_func(bl_plat->func);
	sdio_release_host(bl_plat->func);

	kfree(bl_plat->mp_regs);
    kfree(bl_plat);
}

int bl_device_init(struct sdio_func *func, struct bl_plat **bl_plat)
{
	struct bl_device *device;
	u8 sdio_ireg;
    int ret = 0;

    *bl_plat = kzalloc(sizeof(struct bl_plat) + sizeof(struct bl_device), GFP_KERNEL);
    if (!*bl_plat)
        return -ENOMEM;

	device = (struct bl_device *)(*bl_plat)->priv;

	device->firmware = SD606_DEFAULT_FW_NAME;
	device->reg = &bl_reg_sd606;
	device->max_ports = 16;
	device->mp_agg_pkt_limit = 8;
	device->supports_sdio_new_mode = false;
	device->has_control_mask = true;
	device->supports_fw_dump = false;
	device->mp_tx_agg_buf_size = BL_MP_AGGR_BUF_SIZE_16K;
	device->mp_rx_agg_buf_size = BL_MP_AGGR_BUF_SIZE_16K;
	device->auto_tdls = false;

	func->card->quirks |= MMC_QUIRK_BLKSZ_FOR_BYTE_MODE;

	sdio_claim_host(func);
	
	/*1. enable func*/
	ret = sdio_enable_func(func);

	/*
	 * 2. Read the host_int_status_reg for ACK the first interrupt got
	 * from the bootloader. If we don't do this we get a interrupt
	 * as soon as we register the irq.
	 */
	sdio_readb(func, device->reg->host_int_status_reg, &ret);

	/* 3. Get SDIO ioport */
	(*bl_plat)->io_port = (sdio_readb(func, device->reg->io_port_0_reg, &ret) & 0xff);
	(*bl_plat)->io_port |= ((sdio_readb(func, device->reg->io_port_1_reg, &ret) & 0xff) << 8);
	(*bl_plat)->io_port |= ((sdio_readb(func, device->reg->io_port_2_reg, &ret) & 0xff) << 16);
	BL_DBG("info: SDIO FUNC1 IO port: %#x\n", (*bl_plat)->io_port);

	/* Set Host interrupt reset to read to clear */
	sdio_ireg = sdio_readb(func, device->reg->host_int_rsr_reg, &ret);
	if(!ret)
		sdio_writeb(func, sdio_ireg | device->reg->sdio_int_mask, device->reg->host_int_rsr_reg, &ret);

	/* Dnld/Upld ready set to auto reset */
	sdio_ireg = sdio_readb(func, device->reg->card_misc_cfg_reg, &ret);
	if(!ret)
		sdio_writeb(func, sdio_ireg | AUTO_RE_ENABLE_INT, device->reg->card_misc_cfg_reg, &ret);
	

	/* 4. set block size*/
	sdio_set_block_size(func, BL_SDIO_BLOCK_SIZE);

    /* 4.1 Download Wi-Fi firmware*/
	(*bl_plat)->func = func;
	ret = bl_sdio_init(*bl_plat);
	if (ret) {
		printk("bl_sdio_init failed: ret=%d\n", ret);
		sdio_release_host(func);
		return ret;
	}

	/* 5. request irq */
    ret = sdio_claim_irq(func, bl_irq_hdlr);
	if (ret) {
		printk("sdio_claim_irq failed: ret=%d\n", ret);
		sdio_release_host(func);
		return ret;
	}
	
	/* Simply write the mask to the register */
	sdio_writeb(func, UP_LD_HOST_INT_MASK | DN_LD_HOST_INT_MASK, device->reg->host_int_mask_reg, &ret);
	if (ret) {
		printk("enable host interrupt failed\n");
		sdio_release_irq(func);
		sdio_release_host(func);
		return ret;
	}

	sdio_ireg = sdio_readb(func, device->reg->host_int_mask_reg, &ret);
	BL_DBG("irq_enable=0x%x\n", sdio_ireg);

	sdio_release_host(func);
	
	/* 6. Initialize SDIO variables in card */
	(*bl_plat)->mp_rd_bitmap = 0;
	(*bl_plat)->mp_wr_bitmap = 0;
	(*bl_plat)->curr_rd_port = device->reg->start_rd_port;
	(*bl_plat)->curr_wr_port = device->reg->start_wr_port;

	(*bl_plat)->mp_regs = kzalloc(device->reg->max_mp_regs, GFP_KERNEL);
	if (!(*bl_plat)->mp_regs) {
		printk("kzalloc mp_regs failed!\n");
		return -ENOMEM;
	}
	
	if(ret)	{
		printk("bl_device_init failed!\n");
		sdio_claim_host(func);
		sdio_release_irq(func);
		ret = sdio_disable_func(func);
		sdio_release_host(func);
		kfree((*bl_plat)->mp_regs);
		kfree(*bl_plat);
	}
	return ret;
}
