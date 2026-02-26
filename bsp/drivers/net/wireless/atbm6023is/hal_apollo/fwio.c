/*
 * Firmware I/O code for mac80211 altobeam APOLLO drivers
 * *
 * Copyright (c) 2016, altobeam
 * Author:
 *
 * Based on apollo code
 * Copyright (c) 2010, ST-Ericsson
 * Author: Dmitry Tarnyagin <dmitry.tarnyagin@stericsson.com>
 *
 * Based on:
 * ST-Ericsson UMAC CW1200 driver which is
 * Copyright (c) 2010, ST-Ericsson
 * Author: Ajitpal Singh <ajitpal.singh@stericsson.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/init.h>
#include <linux/vmalloc.h>
#include <linux/sched.h>
#include <linux/firmware.h>
#include <linux/module.h>
#include <linux/debugfs.h>

#include "apollo.h"
#include "fwio.h"
#include "hwio.h"
#include "sbus.h"
#include "debug.h"
#include "bh.h"
#include "dcxo_dpll.h"

#ifdef ATBM_USE_SAVED_FW
//#pragma message("Suspend Save Firmware")
#endif
#ifdef CONFIG_USE_FW_H
//#pragma message("Use Firmware.h")
#endif
static char *fw = FIRMWARE_DEFAULT_PATH;
#if 0
module_param(fw, charp, 0644);
MODULE_PARM_DESC(fw, "Override platform_data firmware file");
#endif
//#pragma message(FIRMWARE_DEFAULT_PATH)


struct firmware_headr {
	u32 flags; /*0x34353677*/
	u32 version;
	u32 iccm_len;
	u32 dccm_len;
	u32 reserve[3];
	u16 reserve2;
	u16 checksum;
};

struct firmware_altobeam {
	struct firmware_headr hdr;
	u8 *fw_iccm;
	u8 *fw_dccm;
};
static struct firmware_altobeam atbm_fw;

void atbm_release_firmware(void)
{
	if (atbm_fw.fw_dccm)
	{
		vfree(atbm_fw.fw_dccm);
		atbm_fw.fw_dccm = NULL;
	}
	if (atbm_fw.fw_iccm)
	{
		vfree(atbm_fw.fw_iccm);
		atbm_fw.fw_iccm = NULL;
	}
}
int atbm_init_firmware(void)
{
	memset(&atbm_fw,0,sizeof(struct firmware_altobeam));
	return 0;
}

int atbm_set_firmare(struct firmware_altobeam *fw)
{
#ifdef ATBM_USE_SAVED_FW
	if (!fw || (!fw->fw_dccm&&!fw->fw_iccm))
	{
		atbm_printk_err(KERN_ERR "fw is err\n");
		return -1;
	}

	if (atbm_fw.fw_dccm || atbm_fw.fw_iccm)
	{
		atbm_printk_err("atbm_fw has been set\n");
		return -1;
	}
	memcpy(&atbm_fw.hdr,&fw->hdr,sizeof(struct firmware_headr));

	if (atbm_fw.hdr.iccm_len)
	{
		atbm_fw.fw_iccm = vmalloc(atbm_fw.hdr.iccm_len);
		atbm_printk_err("%s:fw_iccm(%p)\n",__func__,atbm_fw.fw_iccm);
		if (!atbm_fw.fw_iccm)
		{
			atbm_printk_err("alloc atbm_fw.fw_iccm err\n");
			goto err;
		}
		memcpy(atbm_fw.fw_iccm,fw->fw_iccm,atbm_fw.hdr.iccm_len);
	}

	if (atbm_fw.hdr.dccm_len)
	{
		atbm_fw.fw_dccm= vmalloc(atbm_fw.hdr.dccm_len);

		atbm_printk_err("%s:fw_dccm(%p)\n",__func__,atbm_fw.fw_dccm);
		if (!atbm_fw.fw_dccm)
		{
			atbm_printk_err("alloc atbm_fw.fw_dccm err\n");
			goto err;
		}
		memcpy(atbm_fw.fw_dccm,fw->fw_dccm,atbm_fw.hdr.dccm_len);
	}
	return 0;
err:
	if (atbm_fw.fw_iccm)
	{
		vfree(atbm_fw.fw_iccm);
		atbm_fw.fw_iccm = NULL;
	}

	if (atbm_fw.fw_dccm)
	{
		vfree(atbm_fw.fw_dccm);
		atbm_fw.fw_dccm = NULL;
	}
#endif //#ifndef USB_BUS
	return -1;
}

#define FW_IS_READY	((atbm_fw.fw_dccm != NULL) || (atbm_fw.fw_iccm != NULL))
int atbm_get_fw(struct firmware_altobeam *fw)
{
	if (!FW_IS_READY)
	{
		return -1;
	}

	memcpy(&fw->hdr,&atbm_fw.hdr,sizeof(struct firmware_headr));
	fw->fw_iccm = atbm_fw.fw_iccm;
	fw->fw_dccm = atbm_fw.fw_dccm;
	return 0;
}


int atbm_get_hw_type(u32 config_reg_val, int *major_revision)
{
#if 0
	int hw_type = -1;
	u32 config_value = config_reg_val;
	//u32 silicon_type = (config_reg_val >> 24) & 0x3;
	u32 silicon_vers = (config_reg_val >> 31) & 0x1;

	/* Check if we have CW1200 or STLC9000 */

	hw_type = HIF_1601_CHIP;
#endif
	return HIF_1601_CHIP;
}

static int atbm_load_firmware_generic(struct atbm_common *priv, u8 *data,u32 size,u32 addr)
{

	int ret=0;
	u32 put = 0;
	u8 *buf = NULL;


	buf = atbm_kmalloc(DOWNLOAD_BLOCK_SIZE*2, GFP_KERNEL | GFP_DMA);
	if (!buf) {
		atbm_dbg(ATBM_APOLLO_DBG_ERROR,
			"%s: can't allocate bootloader buffer.\n", __func__);
		ret = -ENOMEM;
		goto error;
	}

#ifndef HW_DOWN_FW
	if (priv->sbus_ops->bootloader_debug_config)
		priv->sbus_ops->bootloader_debug_config(priv->sbus_priv,0);
#endif //#ifndef HW_DOWN_FW

	/*  downloading loop */
	atbm_printk_init( "%s: addr %x: len %x\n",__func__,addr,size);
	for (put = 0; put < size ;put += DOWNLOAD_BLOCK_SIZE) {
		u32 tx_size;


		/* calculate the block size */
		tx_size  = min((size - put),(u32)DOWNLOAD_BLOCK_SIZE);

		memcpy(buf, &data[put], tx_size);

		/* send the block to sram */
		ret = atbm_fw_write(priv,put+addr,buf, tx_size);
		if (ret < 0) {
			atbm_dbg(ATBM_APOLLO_DBG_ERROR,
				"%s: can't write block at line %d.\n",
				__func__, __LINE__);
			goto error;
		}
	} /* End of bootloader download loop */

error:
	atbm_kfree(buf);
	return ret;


}

void  atbm_HwGetChipType(struct atbm_common *priv)
{
	u32 chipver = 0;

	atbm_direct_read_reg_32(priv,0x0acc017c,&chipver);
    chipver&=0xff;



	switch(chipver)
	{
		case 0x14:
			priv->chip_version = APOLLO_F;
			break;
		case 0x24:
		case 0x25:
			//strHwChipFw = ("AthenaB.bin");
			priv->chip_version = ATHENA_B;
			break;
		case 0x45:
		case 0x46:
		case 0x47:
			priv->chip_version = ARES_A;
			break;
		case 0x49:
			priv->chip_version = ARES_B;
			break;
		case 0x64:
		case 0x65:
			priv->chip_version = HERA;
			break;
		default:
			//g_wifi_chip_type = ATHENA_B;
			atbm_printk_always("%s, <ERROR> cannot read chip id\n",__func__ );

		break;
	}

	atbm_printk_always("%s, chipver=0x%x, g_wifi_chip_type[%d]\n",__func__, chipver,priv->chip_version );
}


char * atbm_HwGetChipFw(struct atbm_common *priv)
{
	char * strHwChipFw = NULL;
	int i=0;
	int support_chip_cnt =0;

	atbm_printk_always("PROJ_TYPE_SUPPORT [%x]\n",(u32)PROJ_TYPE_SUPPORT );
	//check if just support 1 chip type
	for(i=0;i<32;i++) {
		if (PROJ_TYPE_SUPPORT&BIT(i)) {
			support_chip_cnt++;
			//atbm_printk_always("PROJ_TYPE_SUPPORT bit[%d]\n" ,i);
			if (ARES_B == i) {
				atbm_printk_always("PROJ_TYPE_SUPPORT ARES_B\n" );
			}
			else if(ATHENA_B == i) {
				atbm_printk_always("PROJ_TYPE_SUPPORT ATHENA_B\n" );
			}
			else if(HERA == i) {
				atbm_printk_always("PROJ_TYPE_SUPPORT HERA\n" );
			}
		}
	}

	if (support_chip_cnt<=1) {
		if (fw)
		{
			atbm_printk_always("fw [%s]\n", fw );
		 	return fw;
		}
	}
	else {

		switch(priv->chip_version)
		{
			case APOLLO_F:
				strHwChipFw = ("atbm_ApolloF.bin");
				break;
			case ARES_A:
				strHwChipFw = ("atbm_AresA.bin");
				break;
			case ATHENA_B:
				strHwChipFw = ("atbm_AthenaB.bin");
				break;
			case ARES_B:
				strHwChipFw = ("atbm_AresB.bin");
				break;
			case HERA:
				strHwChipFw = ("atbm_HareA.bin");
				break;
			default:
				strHwChipFw = FIRMWARE_DEFAULT_PATH;
			break;
		}

		atbm_printk_always("%s, chipver=0x%x, use fw [%s]\n",__func__, priv->chip_version,strHwChipFw );
	}
	return strHwChipFw;
}

//#define TEST_DCXO_CONFIG move to makefile
#ifndef CONFIG_USE_FW_H
#define USED_FW_FILE
#endif
#ifdef USED_FW_FILE
/*check if fw headr ok*/
static int atbm_fw_checksum(struct firmware_headr * hdr)
{
	return 1;
}
#else
#ifdef USB_BUS
#include "firmware_usb_aresb.h"
#include "firmware_usb_athenab.h"
#endif
#ifdef SDIO_BUS
#include "firmware_sdio.h"
#endif
#ifdef SPI_BUS
#include "firmware_spi.h"
#endif
#endif
#ifdef CONFIG_PM_SLEEP
//#pragma message("CONFIG_PM_SLEEP")
int atbm_cache_fw_before_suspend(struct device	 *pdev)
{
#if defined (USED_FW_FILE) && defined(ATBM_USE_SAVED_FW)
	int ret = 0;
	const char *fw_path= fw;
	const struct firmware *firmware = NULL;
	struct firmware_altobeam fw_altobeam;

	memset(&fw_altobeam,0,sizeof(struct firmware_altobeam));
	if (fw_path == NULL) {
		goto error2;
	}
	if (FW_IS_READY) {
		atbm_printk_err("atbm_fw ready\n");
		goto error2;
	}

	ret = request_firmware(&firmware, fw_path, pdev);
	if (ret) {
		atbm_printk_err("request_firmware err\n");
		goto error2;
	}
	if (*(int *)firmware->data == ALTOBEAM_WIFI_HDR_FLAG) {
		memcpy(&fw_altobeam.hdr,firmware->data,sizeof(struct firmware_headr));
		if (atbm_fw_checksum(&fw_altobeam.hdr)==0) {
			ret = -1;
			 atbm_dbg(ATBM_APOLLO_DBG_ERROR,"%s: atbm_fw_checksum fail 11\n", __func__);
			 goto error1;
		}
		fw_altobeam.fw_iccm = (u8 *)firmware->data + sizeof(struct firmware_headr);
		fw_altobeam.fw_dccm = fw_altobeam.fw_iccm + fw_altobeam.hdr.iccm_len;
		atbm_dbg(ATBM_APOLLO_DBG_ERROR,"%s: have header,lmac version(%d) iccm_len(%d) dccm_len(%d)\n", __func__,
			fw_altobeam.hdr.version,fw_altobeam.hdr.iccm_len,fw_altobeam.hdr.dccm_len);
	}
	else {
		fw_altobeam.hdr.version =  0x001;
		if (firmware->size > DOWNLOAD_ITCM_SIZE) {
			fw_altobeam.hdr.iccm_len =  DOWNLOAD_ITCM_SIZE;
			fw_altobeam.hdr.dccm_len =  firmware->size - fw_altobeam.hdr.iccm_len;
			if (fw_altobeam.hdr.dccm_len > DOWNLOAD_DTCM_SIZE) {
				ret = -1;
			 	atbm_dbg(ATBM_APOLLO_DBG_ERROR,"%s: atbm_fw_checksum fail 22\n", __func__);
			 	goto error1;
			}
			fw_altobeam.fw_iccm = (u8 *)firmware->data;
			fw_altobeam.fw_dccm = fw_altobeam.fw_iccm+fw_altobeam.hdr.iccm_len;
		}
		else {
			fw_altobeam.hdr.iccm_len = firmware->size;
			fw_altobeam.hdr.dccm_len = 0;
			fw_altobeam.fw_iccm = (u8 *)firmware->data;

		}

	}
	atbm_release_firmware();

	memcpy(&atbm_fw.hdr,&fw_altobeam.hdr,sizeof(struct firmware_headr));
	if (atbm_fw.hdr.iccm_len)
	{
		atbm_fw.fw_iccm = vmalloc(atbm_fw.hdr.iccm_len);

		if (!atbm_fw.fw_iccm)
		{
			atbm_printk_err( "alloc atbm_fw.fw_iccm err\n");
			goto error1;
		}
		memcpy(atbm_fw.fw_iccm,fw_altobeam.fw_iccm,atbm_fw.hdr.iccm_len);
	}

	if (atbm_fw.hdr.dccm_len)
	{
		atbm_fw.fw_dccm= vmalloc(atbm_fw.hdr.dccm_len);

		if (!atbm_fw.fw_dccm)
		{
			atbm_printk_err("alloc atbm_fw.fw_dccm err\n");
			goto error1;
		}
		memcpy(atbm_fw.fw_dccm,fw_altobeam.fw_dccm,atbm_fw.hdr.dccm_len);
	}
	atbm_printk_always("%s:cached fw\n",__func__);
	release_firmware(firmware);
	return 0;
error1:

	atbm_printk_err("%s:error1\n",__func__);
	release_firmware(firmware);
	if (atbm_fw.fw_iccm)
	{
		vfree(atbm_fw.fw_iccm);
		atbm_fw.fw_iccm = NULL;
	}

	if (atbm_fw.fw_dccm)
	{
		vfree(atbm_fw.fw_dccm);
		atbm_fw.fw_dccm = NULL;
	}
error2:
	atbm_printk_err("%s:error2\n",__func__);
	return ret;
#else
	return 0;
#endif//
}
#endif
static int atbm_start_load_firmware(struct atbm_common *priv)
{

	int ret;
#ifdef USED_FW_FILE
	const char *fw_path= atbm_HwGetChipFw(priv);
#endif//
	const struct firmware *firmware = NULL;
	struct firmware_altobeam fw_altobeam;
	u32 DOWNLOAD_ITCM_ADDR_U32 = 0;
loadfw:
	//u32 testreg_uart;
#ifdef START_DCXO_CONFIG
	atbm_ahb_write_32(priv,0x18e00014,0x200);
	atbm_ahb_read_32(priv,0x18e00014,&val32_1);
	//atbm_ahb_read_32(priv,0x16400000,&testreg_uart);
	atbm_printk_always("0x18e000e4-->%08x %08x\n",val32_1);
#endif//TEST_DCXO_CONFIG
	if (!FW_IS_READY)
	{
#ifdef USED_FW_FILE
	    atbm_dbg(ATBM_APOLLO_DBG_MSG,"%s:FW FILE = %s\n",__func__,fw_path);
		ret = request_firmware(&firmware, fw_path, priv->pdev);
		if (ret) {
			atbm_dbg(ATBM_APOLLO_DBG_ERROR,
				"%s: can't load firmware file %s.\n",
				__func__, fw_path);
			goto error;
		}
		BUG_ON(!firmware->data);
		if (*(int *)firmware->data == ALTOBEAM_WIFI_HDR_FLAG) {
			memcpy(&fw_altobeam.hdr,firmware->data,sizeof(struct firmware_headr));
			if (atbm_fw_checksum(&fw_altobeam.hdr)==0) {
				ret = -1;
				 atbm_dbg(ATBM_APOLLO_DBG_ERROR,"%s: atbm_fw_checksum fail 11\n", __func__);
				 goto error;
			}
			fw_altobeam.fw_iccm = (u8 *)firmware->data + sizeof(struct firmware_headr);
			fw_altobeam.fw_dccm = fw_altobeam.fw_iccm + fw_altobeam.hdr.iccm_len;
			atbm_dbg(ATBM_APOLLO_DBG_ERROR,"%s: have header,lmac version(%d) iccm_len(%d) dccm_len(%d),fwsize(%zu),hdrsize(%zu)\n", __func__,
				fw_altobeam.hdr.version,fw_altobeam.hdr.iccm_len,fw_altobeam.hdr.dccm_len,firmware->size,sizeof(struct firmware_headr));

			//frame_hexdump("fw_iccm ",fw_altobeam.fw_iccm,64);
			//frame_hexdump("fw_dccm ",fw_altobeam.fw_dccm,64);
		}
		else {
			fw_altobeam.hdr.version =  0x001;
			if (firmware->size > DOWNLOAD_ITCM_SIZE) {
				fw_altobeam.hdr.iccm_len =  DOWNLOAD_ITCM_SIZE;
				fw_altobeam.hdr.dccm_len =  firmware->size - fw_altobeam.hdr.iccm_len;
				if (fw_altobeam.hdr.dccm_len > DOWNLOAD_DTCM_SIZE) {
					ret = -1;
				 	atbm_dbg(ATBM_APOLLO_DBG_ERROR,"%s: atbm_fw_checksum fail 22\n", __func__);
				 	goto error;
				}
				fw_altobeam.fw_iccm = (u8 *)firmware->data;
				fw_altobeam.fw_dccm = fw_altobeam.fw_iccm+fw_altobeam.hdr.iccm_len;
			}
			else {
				fw_altobeam.hdr.iccm_len = firmware->size;
				fw_altobeam.hdr.dccm_len = 0;
				fw_altobeam.fw_iccm = (u8 *)firmware->data;

			}

		}
#else //USED_FW_FILE
		{
		atbm_dbg(ATBM_APOLLO_DBG_ERROR,"used firmware.h=\n");
		switch(priv->chip_version)
		{
			case ATHENA_B:
				fw_altobeam.hdr.iccm_len = sizeof(fw_code);
				fw_altobeam.hdr.dccm_len = sizeof(fw_data);
				fw_altobeam.fw_iccm = &fw_code[0];
				fw_altobeam.fw_dccm = &fw_data[0];
				break;
			case ARES_B:
				fw_altobeam.hdr.iccm_len = sizeof(fw_code_aresb);
				fw_altobeam.hdr.dccm_len = sizeof(fw_data_aresb);
				fw_altobeam.fw_iccm = &fw_code_aresb[0];
				fw_altobeam.fw_dccm = &fw_data_aresb[0];
				break;
			default:
				fw_altobeam.hdr.iccm_len = sizeof(fw_code);
				fw_altobeam.hdr.dccm_len = sizeof(fw_data);
				fw_altobeam.fw_iccm = &fw_code[0];
				fw_altobeam.fw_dccm = &fw_data[0];
			break;
		}
		}
#endif //USED_FW_FILE
		atbm_set_firmare(&fw_altobeam);
	}
	else
	{
		if ((ret = atbm_get_fw(&fw_altobeam))<0)
		{
			goto error;
		}
	}
	atbm_dbg(ATBM_APOLLO_DBG_ERROR,"START DOWNLOAD ICCM=========\n");

	if ((priv->chip_version>=ARES_A) || (priv->chip_version==ATHENA_LITE_ECO)) {
		DOWNLOAD_ITCM_ADDR_U32 = 		(0x00010000);
	}
	else {
		DOWNLOAD_ITCM_ADDR_U32 = 		(0x00000000);
	}
	ret = atbm_load_firmware_generic(priv,fw_altobeam.fw_iccm,fw_altobeam.hdr.iccm_len,DOWNLOAD_ITCM_ADDR_U32);
	if (ret<0)
		goto error;
	#ifdef USB_BUS
	fw_altobeam.hdr.dccm_len = 0x8000;
	#else
	if (fw_altobeam.hdr.dccm_len > 0x9000)
	fw_altobeam.hdr.dccm_len = 0x9000;
	#endif
	atbm_dbg(ATBM_APOLLO_DBG_ERROR,"START DOWNLOAD DCCM=========\n");
	ret = atbm_load_firmware_generic(priv,fw_altobeam.fw_dccm,fw_altobeam.hdr.dccm_len,DOWNLOAD_DTCM_ADDR);
	if (ret<0)
		goto error;

	atbm_dbg(ATBM_APOLLO_DBG_MSG, "FIRMWARE DOWNLOAD SUCCESS\n");

error:
	if (ret<0) {
		if (atbm_reset_lmc_cpu(priv) == 0)
			goto loadfw;
	}
	if (firmware)
		release_firmware(firmware);
	return ret;


}


int atbm_load_firmware(struct atbm_common *hw_priv)
{
	int ret;

	atbm_printk_init("atbm_before_load_firmware++\n");
	ret = atbm_before_load_firmware(hw_priv);
	if (ret <0)
		goto out;
	atbm_printk_init("atbm_start_load_firmware++\n");
	ret = atbm_start_load_firmware(hw_priv);
	if (ret <0)
		goto out;
	atbm_printk_init("atbm_after_load_firmware++\n");
	ret = atbm_after_load_firmware(hw_priv);
	if (ret <0) {
		goto out;
	}
	ret =0;
out:
	return ret;

}

