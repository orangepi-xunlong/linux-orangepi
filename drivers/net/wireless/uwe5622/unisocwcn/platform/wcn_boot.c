/*
 * Copyright (C) 2015 Spreadtrum Communications Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/firmware.h>
#include <linux/file.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mfd/syscon.h>
#include <linux/of_gpio.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/regulator/consumer.h>
#include <linux/regmap.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/unistd.h>
#include <linux/version.h>
#include <linux/vmalloc.h>
#include <linux/workqueue.h>
#include <marlin_platform.h>
#include <wcn_bus.h>

#include "../sleep/sdio_int.h"
#include "../sleep/slp_mgr.h"
#include "wcn_misc.h"
#include "wcn_procfs.h"
#include "mdbg_type.h"
#include "wcn_glb_reg.h"
#include "wcn_glb.h"

#define WCN_FW_NAME	"wcnmodem.bin"

#ifndef REG_PMU_APB_XTL_WAIT_CNT0
#define REG_PMU_APB_XTL_WAIT_CNT0 0xe42b00ac
#endif

struct wcn_sync_info_t {
	unsigned int init_status;
	unsigned int mem_pd_bt_start_addr;
	unsigned int mem_pd_bt_end_addr;
	unsigned int mem_pd_wifi_start_addr;
	unsigned int mem_pd_wifi_end_addr;
	unsigned int prj_type;
	unsigned int tsx_dac_data;
	unsigned int sdio_config;
	unsigned int dcache_status;
	unsigned int dcache_start_addr;
	unsigned int dcache_end_addr;
	unsigned int mem_pd_status;
	unsigned int mem_ap_cmd;
	unsigned int rsvd[3];
	unsigned int bind_verify_data[4];
};

struct tsx_data {
	u32 flag; /* cali flag ref */
	u16 dac; /* AFC cali data */
	u16 reserved;
};

struct tsx_cali {
	u32 init_flag;
	struct tsx_data tsxdata;
};

/*
 * sdio config to cp side
 * bit[31:24]: reserved
 * bit[23]: wake_host_data_separation:
 *	0: if BT_WAKEUP_HOST en or WL_WAKEUP_HOST en,
 *	    wifi and bt packets can wake host;
 *	1: if BT_WAKEUP_HOST en, ONLY bt packets can wake host;
 *	    if WL_WAKEUP_HOST en, ONLY wifi packets can wake host
 * bit[22:18]: wake_host_level_duration_10ms: BT_WAKEUP_HOST or WL_WAKEUP_HOST
 *	      level dyration time per 10ms, example: 0:0ms; 3:30ms; 20:200ms
 * bit[17:16]: wl_wake_host_trigger_type:
 *	     00:WL_WAKEUP_HOST  trigger type low
 *	     01:WL_WAKEUP_HOST  trigger type rising
 *	     10:WL_WAKEUP_HOST  trigger type falling
 *	     11:WL_WAKEUP_HOST  trigger type high
 * bit[15]: wl_wake_host_en: 0: disable, 1: enable
 * bit[14:13]: sdio_irq_trigger_type:
 *	      00:pubint gpio irq trigger type low
 *	      01:pubint gpio irq trigger type rising [NOT support]
 *	      10:pubint gpio irq trigger type falling [NOT support]
 *	      11:pubint gpio irq trigger type high
 * bit[12:11]: sdio_irq_type:
 *	      00:dedicated irq, gpio1
 *	      01:inband data1 irq
 *	      10:use BT_WAKEUP_HOST(pubint) pin as gpio irq
 *	      11:use WL_WAKEUP_HOST(esmd3) pin as gpio irq
 * bit[10:9]: bt_wake_host_trigger_type:
 *	     00:BT_WAKEUP_HOST  trigger type low
 *	     01:BT_WAKEUP_HOST  trigger type rising
 *	     10:BT_WAKEUP_HOST  trigger type falling
 *	     11:BT_WAKEUP_HOST  trigger type high
 * bit[8]: bt_wake_host_en: 0: disable, 1: enable
 * bit[7:5]: sdio_blk_size: 000: blocksize 840; 001: blocksize 512
 * bit[4]: sdio_rx_mode: 0: adma; 1: sdma
 * bit[3:1]: vendor_id: 000: default id, unisoc[0x0]
 *		       001: hisilicon default version, pull chipen after resume
 *		       010: hisilicon version, keep power (NOT pull chipen) and
 *			    reset sdio after resume
 * bit[0]: sdio_config_en: 0: disable sdio config
 *		          1: enable sdio config
 */
union wcn_sdiohal_config {
	unsigned int val;
	struct {
		unsigned char sdio_config_en:1;
		unsigned char vendor_id:3;
		unsigned char sdio_rx_mode:1;
		unsigned char sdio_blk_size:3;
		unsigned char bt_wake_host_en:1;
		unsigned char bt_wake_host_trigger_type:2;
		unsigned char sdio_irq_type:2;
		unsigned char sdio_irq_trigger_type:2;
		unsigned char wl_wake_host_en:1;
		unsigned char wl_wake_host_trigger_type:2;
		unsigned char wake_host_level_duration_10ms:5;
		unsigned char wake_host_data_separation:1;
		unsigned int  reserved:8;
	} cfg;
};

struct marlin_device {
	int wakeup_ap;
	int reset;
	int chip_en;
	int int_ap;

	struct mutex power_lock;
	struct completion download_done;
	unsigned long power_state;
	char *write_buffer;
	struct delayed_work power_wq;
	struct work_struct download_wq;
	bool no_power_off;
	int wifi_need_download_ini_flag;
	int first_power_on_flag;
	unsigned char download_finish_flag;
	unsigned char bt_wl_wake_host_en;
	unsigned int bt_wake_host_int_num;
	unsigned int wl_wake_host_int_num;
	int loopcheck_status_change;
	struct wcn_sync_info_t sync_f;
	struct tsx_cali tsxcali;
};

marlin_reset_callback marlin_reset_func;
void *marlin_callback_para;

struct marlin_device *marlin_dev;

unsigned char  flag_reset;
char functionmask[8];

#define IMG_HEAD_MAGIC "WCNM"
#define IMG_HEAD_MAGIC_COMBINE "WCNE"

#define IMG_MARLINAA_TAG "MLAA"
#define IMG_MARLINAB_TAG "MLAB"
#define IMG_MARLINAC_TAG "MLAC"
#define IMG_MARLINAD_TAG "MLAD"

#define IMG_MARLIN3_AA_TAG "30AA"
#define IMG_MARLIN3_AB_TAG "30AB"
#define IMG_MARLIN3_AC_TAG "30AC"
#define IMG_MARLIN3_AD_TAG "30AD"

#define IMG_MARLIN3L_AA_TAG "3LAA"
#define IMG_MARLIN3L_AB_TAG "3LAB"
#define IMG_MARLIN3L_AC_TAG "3LAC"
#define IMG_MARLIN3L_AD_TAG "3LAD"

#define IMG_MARLIN3E_AA_TAG "3EAA"
#define IMG_MARLIN3E_AB_TAG "3EAB"
#define IMG_MARLIN3E_AC_TAG "3EAC"
#define IMG_MARLIN3E_AD_TAG "3EAD"

#define MARLIN_MASK 0x27F
#define AUTO_RUN_MASK 0X100

#define AFC_CALI_FLAG 0x54463031 /* cali flag */
#define AFC_CALI_READ_FINISH 0x12121212
#define WCN_AFC_CALI_PATH "/productinfo/wcn/tsx_bt_data.txt"

#define POWER_WQ_DELAYED_MS 7500

/* #define E2S(x) { case x: return #x; } */

struct head {
	char magic[4];
	unsigned int version;
	unsigned int img_count;
};

struct imageinfo {
	char tag[4];
	unsigned int offset;
	unsigned int size;
};

struct combin_img_info {
	unsigned int addr;			/* image target address */
	unsigned int offset;			/* image combin img offset */
	unsigned int size;			/* image size */
};

unsigned long marlin_get_power_state(void)
{
	return marlin_dev->power_state;
}
EXPORT_SYMBOL_GPL(marlin_get_power_state);

unsigned char marlin_get_bt_wl_wake_host_en(void)
{
	return marlin_dev->bt_wl_wake_host_en;
}
EXPORT_SYMBOL_GPL(marlin_get_bt_wl_wake_host_en);

/* return chipid, for example:
 * 0x2355000x: Marlin3 series
 * 0x2355B00x: Marlin3 lite series
 * 0x5663000x: Marlin3E series
 * 0: read chipid fail or not unisoc module
 */
#define WCN_CHIPID_MASK (0xFFFFF000)
unsigned int marlin_get_wcn_chipid(void)
{
	int ret;
	static unsigned long int chip_id;

	if (likely(chip_id))
		return chip_id;

	WCN_DEBUG("%s enter.\n", __func__);

#ifndef CONFIG_CHECK_DRIVER_BY_CHIPID
	ret = sprdwcn_bus_reg_read(CHIPID_REG, &chip_id, 4);
	if (ret < 0) {
		WCN_ERR("marlin read chip ID fail, ret=%d\n", ret);
		return 0;
	}

#else
	/* At first, read Marlin3E chipid register. */
	ret = sprdwcn_bus_reg_read(CHIPID_REG_M3E, &chip_id, 4);
	if (ret < 0) {
		WCN_ERR("read marlin3E chip id fail, ret=%d\n", ret);
		return 0;
	}

	/* If it is not Marlin3E, then read Marlin3 chipid register. */
	if ((chip_id & WCN_CHIPID_MASK) != MARLIN3E_AA_CHIPID) {
		ret = sprdwcn_bus_reg_read(CHIPID_REG_M3_M3L, &chip_id, 4);
		if (ret < 0) {
			WCN_ERR("read marlin3 chip id fail, ret=%d\n", ret);
			return 0;
		}
	}
#endif
	WCN_INFO("%s: chipid: 0x%lx\n", __func__, chip_id);

	return chip_id;
}
EXPORT_SYMBOL_GPL(marlin_get_wcn_chipid);

/* return chip model, for example:
 * 0: WCN_CHIP_INVALID
 * 1: WCN_CHIP_MARLIN3
 * 2: WCN_CHIP_MARLIN3L
 * 3: WCN_CHIP_MARLIN3E
 */
enum wcn_chip_model wcn_get_chip_model(void)
{
	static enum wcn_chip_model chip_model = WCN_CHIP_INVALID;
	unsigned int chip_id;
	static const char *chip_model_str[] = {
		"ERRO",
		"Marlin3",
		"Marlin3Lite",
		"Marlin3E",
	};

	if (likely(chip_model))
		return chip_model;

	/* if read chipid fail or not unisoc module, chip_id is 0. */
	chip_id = marlin_get_wcn_chipid();
	if (chip_id == 0)
		chip_model = WCN_CHIP_INVALID;
	else if ((chip_id & WCN_CHIPID_MASK) == MARLIN3_AA_CHIPID)
		chip_model = WCN_CHIP_MARLIN3;
	else if ((chip_id & WCN_CHIPID_MASK) == MARLIN3L_AA_CHIPID)
		chip_model = WCN_CHIP_MARLIN3L;
	else if ((chip_id & WCN_CHIPID_MASK) == MARLIN3E_AA_CHIPID)
		chip_model = WCN_CHIP_MARLIN3E;
	else
		chip_model = WCN_CHIP_INVALID;
	WCN_DEBUG("%s: chip_model: %s\n", __func__, chip_model_str[chip_model]);

	return chip_model;
}
EXPORT_SYMBOL_GPL(wcn_get_chip_model);

/* return chip type, for example:
 * 0: WCN_CHIP_ID_INVALID
 * 1: WCN_CHIP_ID_AA
 * 2: WCN_CHIP_ID_AB
 * 3: WCN_CHIP_ID_AC
 * 4: WCN_CHIP_ID_AD
 */
enum wcn_chip_id_type wcn_get_chip_type(void)
{
	static enum wcn_chip_id_type chip_type = WCN_CHIP_ID_INVALID;
	unsigned int chip_id;
	static const char *chip_type_str[] = {
		"ERRO",
		"AA",
		"AB",
		"AC",
		"AD",
	};

	if (likely(chip_type))
		return chip_type;

	/* if read chipid fail or not unisoc module, chip_id is 0. */
	chip_id = marlin_get_wcn_chipid();
	if (chip_id == 0)
		chip_type = WCN_CHIP_ID_INVALID;
	else
		chip_type = (chip_id & 0xF) + 1;
	WCN_DEBUG("%s: chip_type: %s\n", __func__, chip_type_str[chip_type]);

	return chip_type;
}
EXPORT_SYMBOL_GPL(wcn_get_chip_type);

#define WCN_CHIP_NAME_UNKNOWN "UNKNOWN"

/* Marlin3_AD_0x23550003 */
const char *wcn_get_chip_name(void)
{
	unsigned int chip_id, pos = 0;
	enum wcn_chip_model chip_model;
	enum wcn_chip_id_type chip_type;
	static char wcn_chip_name[32] = { 0 };
	static const char *chip_model_str[] = {
		"ERRO_",
		"Marlin3_",
		"Marlin3Lite_",
		"Marlin3E_",
	};
	static const char *chip_type_str[] = {
		"ERRO_",
		"AA_",
		"AB_",
		"AC_",
		"AD_",
	};

	if (wcn_chip_name[0])
		return wcn_chip_name;

	/* if read chipid fail or not unisoc module, chip_id is 0. */
	chip_id = marlin_get_wcn_chipid();
	if (chip_id == 0) {
		memcpy((void *)wcn_chip_name, WCN_CHIP_NAME_UNKNOWN,
		       strlen(WCN_CHIP_NAME_UNKNOWN));
		goto out;
	}

	chip_model = wcn_get_chip_model();
	if (chip_model == WCN_CHIP_INVALID) {
		memcpy((void *)wcn_chip_name, WCN_CHIP_NAME_UNKNOWN,
		       strlen(WCN_CHIP_NAME_UNKNOWN));
		goto out;
	}
	sprintf((char *)wcn_chip_name, chip_model_str[chip_model]);
	pos += strlen(chip_model_str[chip_model]);

	chip_type = wcn_get_chip_type();
	if (chip_type == WCN_CHIP_ID_INVALID) {
		memcpy((void *)wcn_chip_name, WCN_CHIP_NAME_UNKNOWN,
		       strlen(WCN_CHIP_NAME_UNKNOWN));
		goto out;
	}
	sprintf((char *)(wcn_chip_name + pos), chip_type_str[chip_type]);
	pos += strlen(chip_type_str[chip_type]);
	sprintf((char *)(wcn_chip_name + pos), "0x%x", chip_id);

out:
	WCN_DEBUG("%s: chip_name: %s\n", __func__, wcn_chip_name);
	return wcn_chip_name;
}
EXPORT_SYMBOL_GPL(wcn_get_chip_name);

/* return number of marlin antennas
 * MARLIN_TWO_ANT; MARLIN_THREE_ANT; ...
 */
int marlin_get_ant_num(void)
{
	return get_board_ant_num();
}
EXPORT_SYMBOL_GPL(marlin_get_ant_num);

/* get the subsys string */
const char *strno(int subsys)
{
	switch (subsys) {
	case MARLIN_BLUETOOTH:
		return "MARLIN_BLUETOOTH";
	case MARLIN_FM:
		return "MARLIN_FM";
	case MARLIN_WIFI:
		return "MARLIN_WIFI";
	case MARLIN_WIFI_FLUSH:
		return "MARLIN_WIFI_FLUSH";
	case MARLIN_SDIO_TX:
		return "MARLIN_SDIO_TX";
	case MARLIN_SDIO_RX:
		return "MARLIN_SDIO_RX";
	case MARLIN_MDBG:
		return "MARLIN_MDBG";
	case WCN_AUTO:
		return "WCN_AUTO";
	case MARLIN_ALL:
		return "MARLIN_ALL";
	default: return "MARLIN_SUBSYS_UNKNOWN";
	}
/* #undef E2S */
}

/* tsx/dac init */
int marlin_tsx_cali_data_read(struct tsx_data *p_tsx_data)
{
	u32 size = 0;
	u32 read_len = 0;
	struct file *file;
	loff_t offset = 0;
	char *pdata;

	file = filp_open(WCN_AFC_CALI_PATH, O_RDONLY, 0);
	if (IS_ERR(file)) {
		WCN_ERR("open file error\n");
		return -1;
	}
	WCN_INFO("open image "WCN_AFC_CALI_PATH" successfully\n");

	/* read file to buffer */
	size = sizeof(struct tsx_data);
	pdata = (char *)p_tsx_data;
	do {
#if KERNEL_VERSION(4, 14, 0) <= LINUX_VERSION_CODE
		read_len = kernel_read(file, (void *)pdata, size, &offset);
#else
		read_len = kernel_read(file, offset, pdata, size);
#endif

		if (read_len > 0) {
			size -= read_len;
			pdata += read_len;
		}
	} while ((read_len > 0) && (size > 0));
	fput(file);
	WCN_INFO("After read, data =%p dac value %02x\n", pdata,
			 p_tsx_data->dac);

	return 0;
}

static u16 marlin_tsx_cali_data_get(void)
{
	int ret;

	return 0;

	WCN_INFO("tsx cali init flag %d\n", marlin_dev->tsxcali.init_flag);

	if (marlin_dev->tsxcali.init_flag == AFC_CALI_READ_FINISH)
		return marlin_dev->tsxcali.tsxdata.dac;

	ret = marlin_tsx_cali_data_read(&marlin_dev->tsxcali.tsxdata);
	marlin_dev->tsxcali.init_flag = AFC_CALI_READ_FINISH;
	if (ret != 0) {
		marlin_dev->tsxcali.tsxdata.dac = 0xffff;
		WCN_INFO("tsx cali read fail! default 0xffff\n");
		return marlin_dev->tsxcali.tsxdata.dac;
	}

	if (marlin_dev->tsxcali.tsxdata.flag != AFC_CALI_FLAG) {
		marlin_dev->tsxcali.tsxdata.dac = 0xffff;
		WCN_INFO("tsx cali flag fail! default 0xffff\n");
		return marlin_dev->tsxcali.tsxdata.dac;
	}
	WCN_INFO("dac flag %d value:0x%x\n",
			    marlin_dev->tsxcali.tsxdata.flag,
			    marlin_dev->tsxcali.tsxdata.dac);

	return marlin_dev->tsxcali.tsxdata.dac;
}

#define marlin_firmware_get_combin_info(buffer) \
		(struct combin_img_info *)(buffer + sizeof(struct head))

#define bin_magic_is(data, magic_tag) \
	!strncmp(((struct head *)data)->magic, magic_tag, strlen(magic_tag))

#define marlin_fw_get_img_count(img) (((struct head *)img)->img_count)

static const struct imageinfo *marlin_imageinfo_get_from_data(const char *tag,
		const void *data)
{
	const struct imageinfo *imageinfo;
	int imageinfo_count;
	int i;

	imageinfo = (struct imageinfo *)(data + sizeof(struct head));
	imageinfo_count = marlin_fw_get_img_count(data);

	for (i = 0; i < imageinfo_count; i++)
		if (!strncmp(imageinfo[i].tag, tag, 4))
			return &(imageinfo[i]);
	return NULL;
}

static const struct imageinfo *marlin_judge_images(const unsigned char *buffer)
{
	unsigned long chip_id;
	const struct imageinfo *image_info;

	chip_id = marlin_get_wcn_chipid();
	if (buffer == NULL)
		return NULL;

#ifndef CONFIG_CHECK_DRIVER_BY_CHIPID
	if (wcn_get_chip_model() == WCN_CHIP_MARLIN3) {
		switch (chip_id) {
		case MARLIN_AA_CHIPID:
			return marlin_imageinfo_get_from_data(IMG_MARLINAA_TAG,
							      buffer);
		case MARLIN_AB_CHIPID:
			return marlin_imageinfo_get_from_data(IMG_MARLINAB_TAG,
							      buffer);
		case MARLIN_AC_CHIPID:
		case MARLIN_AD_CHIPID:
			/* bin of m3 AC and AD chip is the same. */
			image_info = marlin_imageinfo_get_from_data(
				IMG_MARLINAC_TAG, buffer);
			if (image_info) {
				WCN_INFO("%s find %s tag\n", __func__,
					 IMG_MARLINAC_TAG);
				return image_info;
			}
			return marlin_imageinfo_get_from_data(IMG_MARLINAD_TAG,
							      buffer);
		default:
			WCN_ERR("Marlin can't find correct imginfo!\n");
		}
	}
	if (wcn_get_chip_model() == WCN_CHIP_MARLIN3L) {
		switch (chip_id) {
		case MARLIN_AA_CHIPID:
			return marlin_imageinfo_get_from_data(IMG_MARLINAA_TAG,
							      buffer);
		case MARLIN_AB_CHIPID:
			return marlin_imageinfo_get_from_data(IMG_MARLINAB_TAG,
							      buffer);
		default:
			WCN_ERR("Marlin can't find correct imginfo!\n");
		}
	}
	if (wcn_get_chip_model() == WCN_CHIP_MARLIN3E) {
		switch (chip_id) {
		case MARLIN_AA_CHIPID:
		case MARLIN_AB_CHIPID:
			/* bin of m3e AA and AB chip is the same. */
			image_info = marlin_imageinfo_get_from_data(
				IMG_MARLINAA_TAG, buffer);
			if (image_info) {
				WCN_INFO("%s find %s tag\n", __func__,
					 IMG_MARLINAA_TAG);
				return image_info;
			}
			return marlin_imageinfo_get_from_data(IMG_MARLINAB_TAG,
							      buffer);
		default:
			WCN_ERR("Marlin can't find correct imginfo!\n");
		}
	}

#else  /*ELOF CONFIG_CHECK_DRIVER_BY_CHIPID*/

	switch (chip_id) {
	case MARLIN3_AA_CHIPID:
		return marlin_imageinfo_get_from_data(
			IMG_MARLIN3_AA_TAG, buffer);
	case MARLIN3_AB_CHIPID:
		return marlin_imageinfo_get_from_data(
			IMG_MARLIN3_AB_TAG, buffer);
	case MARLIN3_AC_CHIPID:
	case MARLIN3_AD_CHIPID:
		/* bin of m3 AC and AD chip is the same. */
		image_info = marlin_imageinfo_get_from_data(
			IMG_MARLIN3_AC_TAG, buffer);
		if (image_info) {
			WCN_INFO("%s find %s tag\n", __func__,
				 IMG_MARLIN3_AC_TAG);
			return image_info;
		}
		return marlin_imageinfo_get_from_data(
			IMG_MARLIN3_AD_TAG, buffer);
	case MARLIN3L_AA_CHIPID:
		return marlin_imageinfo_get_from_data(
			IMG_MARLIN3L_AA_TAG, buffer);
	case MARLIN3L_AB_CHIPID:
		return marlin_imageinfo_get_from_data(
			IMG_MARLIN3L_AB_TAG, buffer);
	case MARLIN3L_AC_CHIPID:
		return marlin_imageinfo_get_from_data(
			IMG_MARLIN3L_AC_TAG, buffer);
	case MARLIN3L_AD_CHIPID:
		return marlin_imageinfo_get_from_data(
			IMG_MARLIN3L_AD_TAG, buffer);
	case MARLIN3E_AA_CHIPID:
	case MARLIN3E_AB_CHIPID:
		/* bin of m3e AA and AB chip is the same. */
		image_info = marlin_imageinfo_get_from_data(
			IMG_MARLIN3E_AA_TAG, buffer);
		if (image_info) {
			WCN_INFO("%s find %s tag\n", __func__,
				 IMG_MARLIN3E_AA_TAG);
			return image_info;
		}
		return marlin_imageinfo_get_from_data(
			IMG_MARLIN3E_AB_TAG, buffer);
	case MARLIN3E_AC_CHIPID:
		return marlin_imageinfo_get_from_data(
			IMG_MARLIN3E_AC_TAG, buffer);
	case MARLIN3E_AD_CHIPID:
		return marlin_imageinfo_get_from_data(
			IMG_MARLIN3E_AD_TAG, buffer);
	default:
		WCN_INFO("%s Cannot find Chip Firmware\n", __func__);
		break;
	}
#endif /*EOF CONFIG_CHECK_DRIVER_BY_CHIPID*/
	return NULL;
}

static int sprdwcn_bus_direct_write_dispack(unsigned int addr, const void *buf,
		size_t buf_size, size_t packet_max_size)
{
	int ret = 0;
	size_t offset = 0;
	void *kbuf = marlin_dev->write_buffer;

	while (offset < buf_size) {
		size_t temp_size = min(packet_max_size, buf_size - offset);

		memcpy(kbuf, buf + offset, temp_size);
		ret = sprdwcn_bus_direct_write(addr + offset, kbuf, temp_size);
		if (ret < 0)
			goto OUT;

		offset += temp_size;
	}
OUT:
	if (ret < 0)
		WCN_ERR(" %s: dt write error:%d\n", __func__, ret);
	return ret;
}

struct marlin_firmware {
	const u8 *data;
	size_t size;
	const void *priv;
};

/* this function __must__ be paired with marlin_firmware_release !*/
/* Suggest use it like Documentation/firmware_class/README:65
 *
 *	if(marlin_request_firmware(&fw) == 0)
 *		handle_this_firmware(fw);
 *	marlin_release_firmware(fw);
 */
static int marlin_request_firmware(struct marlin_firmware **mfirmware_p)
{
	struct marlin_firmware *mfirmware;
	const struct firmware *firmware;
	int ret = 0;

	*mfirmware_p = NULL;
	mfirmware = kmalloc(sizeof(struct marlin_firmware), GFP_KERNEL);
	if (!mfirmware)
		return -ENOMEM;

	WCN_INFO("%s from %s start!\n", __func__, WCN_FW_NAME);
	ret = request_firmware(&firmware, WCN_FW_NAME, NULL);
	if (ret < 0) {
		WCN_ERR("%s not find %s errno:(%d)(ignore!!)\n",
			__func__, WCN_FW_NAME, ret);
		kfree(mfirmware);
		return ret;
	}

	mfirmware->priv = (void *)firmware;
	mfirmware->data = firmware->data;
	mfirmware->size = firmware->size;

	memcpy(functionmask, mfirmware->data, 8);
	if ((functionmask[0] == 0x00) && (functionmask[1] == 0x00)) {
		mfirmware->data += 8;
		mfirmware->size -= 8;
	} else {
		functionmask[7] = 0;
	}

	*mfirmware_p = mfirmware;

	return 0;
}

static int marlin_firmware_parse_image(struct marlin_firmware *mfirmware)
{
	if (bin_magic_is(mfirmware->data, IMG_HEAD_MAGIC)) {
		const struct imageinfo *info;

		WCN_INFO("%s imagepack is %s type,need parse it\n",
			 __func__, IMG_HEAD_MAGIC);
		info = marlin_judge_images(mfirmware->data);
		if (!info) {
			WCN_ERR("marlin:%s imginfo is NULL\n", __func__);
			return -1;
		}
		mfirmware->size = info->size;
		mfirmware->data += info->offset;
	} else if (bin_magic_is(mfirmware->data, IMG_HEAD_MAGIC_COMBINE)) {
		/* cal the combin size */
		int img_count;
		const struct combin_img_info *img_info;
		int img_real_size = 0;

		WCN_INFO("%s imagepack is %s type,need parse it\n",
			 __func__, IMG_HEAD_MAGIC_COMBINE);

		img_count = marlin_fw_get_img_count(mfirmware->data);
		img_info = marlin_firmware_get_combin_info(mfirmware->data);

		img_real_size =
		img_info[img_count - 1].size + img_info[img_count - 1].offset;

		mfirmware->size = img_real_size;
	}

	return 0;
}

static int marlin_firmware_write(struct marlin_firmware *mfirmware)
{
	int i = 0;
	int combin_img_count;
	const struct combin_img_info *imginfoe;
	int err;

	if (bin_magic_is(mfirmware->data, IMG_HEAD_MAGIC_COMBINE)) {
		WCN_INFO("marlin %s imagepack is WCNE type,need parse it\n",
			__func__);

		combin_img_count = marlin_fw_get_img_count(mfirmware->data);
		imginfoe = marlin_firmware_get_combin_info(mfirmware->data);
		if (!imginfoe) {
			WCN_ERR("marlin:%s imginfo is NULL\n", __func__);
			return -1;
		}

		for (i = 0; i < combin_img_count; i++) {
			if (imginfoe[i].size + imginfoe[i].offset >
					mfirmware->size) {
				WCN_ERR("%s memory crossover\n", __func__);
				return -1;
			}
			err = sprdwcn_bus_direct_write_dispack(imginfoe[i].addr,
					mfirmware->data + imginfoe[i].offset,
					imginfoe[i].size, PACKET_SIZE);
			if (err) {
				WCN_ERR("%s download error\n", __func__);
				return -1;
			}
		}
	} else {
		err = sprdwcn_bus_direct_write_dispack(CP_START_ADDR,
				mfirmware->data, mfirmware->size, PACKET_SIZE);
		if (err) {
			WCN_ERR("%s download error\n", __func__);
			return -1;
		}
	}
	WCN_INFO("combin_img %d %s finish and successful\n", i, __func__);

	return 0;
}

static void marlin_release_firmware(struct marlin_firmware *mfirmware)
{
	if (mfirmware) {
		release_firmware(mfirmware->priv);
		kfree(mfirmware);
	}
}

/* BT WIFI FM download */
static int btwifi_download_firmware(void)
{
	int ret;
	struct marlin_firmware *mfirmware;

	ret = marlin_request_firmware(&mfirmware);
	if (ret) {
		WCN_ERR("%s request firmware error\n", __func__);
		goto OUT;
	}

	ret = marlin_firmware_parse_image(mfirmware);
	if (ret) {
		WCN_ERR("%s firmware parse AA\\AB error\n", __func__);
		goto OUT;
	}

	ret = marlin_firmware_write(mfirmware);
	if (ret) {
		WCN_ERR("%s firmware write error\n", __func__);
		goto OUT;
	}

OUT:
	marlin_release_firmware(mfirmware);

	return ret;
}

static int marlin_parse_dt(struct device *dev)
{
#ifdef CONFIG_WCN_PARSE_DTS
	struct device_node *np = dev->of_node;
#endif
	int ret = -1;

	if (!marlin_dev)
		return ret;

#ifdef CONFIG_WCN_PARSE_DTS
	marlin_dev->chip_en = of_get_named_gpio(np, "wl-reg-on", 0);
#else
	marlin_dev->chip_en = 0;
#endif
	if (marlin_dev->chip_en > 0) {
		WCN_INFO("%s chip_en gpio=%d\n", __func__,
			 marlin_dev->chip_en);
		if (!gpio_is_valid(marlin_dev->chip_en)) {
			WCN_ERR("chip_en gpio is invalid: %d\n",
				marlin_dev->chip_en);
			return -EINVAL;
		}
		ret = gpio_request(marlin_dev->chip_en, "chip_en");
		if (ret) {
			WCN_ERR("gpio chip_en request err: %d\n",
				marlin_dev->chip_en);
			marlin_dev->chip_en = 0;
		}
	}

#ifdef CONFIG_WCN_PARSE_DTS
	marlin_dev->reset = of_get_named_gpio(np, "bt-reg-on", 0);
#else
	marlin_dev->reset = 0;
#endif
	if (marlin_dev->reset > 0) {
		WCN_INFO("%s reset gpio=%d\n", __func__, marlin_dev->reset);
		if (!gpio_is_valid(marlin_dev->reset)) {
			WCN_ERR("reset gpio is invalid: %d\n",
				marlin_dev->reset);
			return -EINVAL;
		}
		ret = gpio_request(marlin_dev->reset, "reset");
		if (ret) {
			WCN_ERR("gpio reset request err: %d\n",
				marlin_dev->reset);
			marlin_dev->reset = 0;
		}
	}

#ifdef CONFIG_WCN_PARSE_DTS
	marlin_dev->int_ap = of_get_named_gpio(np, "pub-int-gpio", 0);
#else
	marlin_dev->int_ap = 0;
#endif
	if (marlin_dev->int_ap > 0) {
		WCN_INFO("%s int gpio=%d\n", __func__, marlin_dev->int_ap);
		if (!gpio_is_valid(marlin_dev->int_ap)) {
			WCN_ERR("int irq is invalid: %d\n",
				marlin_dev->int_ap);
			return -EINVAL;
		}
		ret = gpio_request(marlin_dev->int_ap, "int_ap");
		if (ret) {
			WCN_ERR("int_ap request err: %d\n",
				marlin_dev->int_ap);
			marlin_dev->int_ap = 0;
		}
	}

#ifdef CONFIG_WCN_PARSE_DTS
	if (of_property_read_bool(np, "keep-power-on")) {
		WCN_INFO("wcn config keep power on\n");
		marlin_dev->no_power_off = true;
	}

	if (of_property_read_bool(np, "bt-wake-host")) {
		int bt_wake_host_gpio;

		WCN_INFO("wcn config bt wake host\n");
		marlin_dev->bt_wl_wake_host_en |= BIT(BT_WAKE_HOST);
		bt_wake_host_gpio =
			of_get_named_gpio(np, "bt-wake-host-gpio", 0);
		WCN_INFO("%s bt-wake-host-gpio=%d\n", __func__,
			 bt_wake_host_gpio);
		if (!gpio_is_valid(bt_wake_host_gpio)) {
			WCN_ERR("int irq is invalid: %d\n",
				bt_wake_host_gpio);
			return -EINVAL;
		}
		ret = gpio_request(bt_wake_host_gpio, "bt-wake-host-gpio");
		if (ret)
			WCN_ERR("bt-wake-host-gpio request err: %d\n",
				bt_wake_host_gpio);
	}

	if (of_property_read_bool(np, "wl-wake-host")) {
		int wl_wake_host_gpio;

		WCN_INFO("wcn config wifi wake host\n");
		marlin_dev->bt_wl_wake_host_en |= BIT(WL_WAKE_HOST);
		wl_wake_host_gpio =
			of_get_named_gpio(np, "wl-wake-host-gpio", 0);
		WCN_INFO("%s wl-wake-host-gpio=%d\n", __func__,
			 wl_wake_host_gpio);
		if (!gpio_is_valid(wl_wake_host_gpio)) {
			WCN_ERR("int irq is invalid: %d\n",
				wl_wake_host_gpio);
			return -EINVAL;
		}
		ret = gpio_request(wl_wake_host_gpio, "wl-wake-host-gpio");
		if (ret)
			WCN_ERR("wl-wake-host-gpio request err: %d\n",
				wl_wake_host_gpio);
	}
#endif

	return 0;
}

static int marlin_gpio_free(void)
{
	if (!marlin_dev)
		return -1;

	if (marlin_dev->reset > 0)
		gpio_free(marlin_dev->reset);
	if (marlin_dev->chip_en > 0)
		gpio_free(marlin_dev->chip_en);
	if (marlin_dev->int_ap > 0)
		gpio_free(marlin_dev->int_ap);

	return 0;
}

#ifdef CONFIG_SDIOHAL
static void marlin_send_sdio_config_to_cp_vendor(void)
{
	union wcn_sdiohal_config sdio_cfg = {0};

#if (defined(CONFIG_AW_BOARD))
	/* Vendor config */

	/* bit[0]: sdio_config_en:
	 * 0: disable sdio config
	 * 1: enable sdio config
	 */
	sdio_cfg.cfg.sdio_config_en = 1;

	/* bit[3:1]: vendor_id:
	 * 000: default id, unisoc[0x0]
	 * 001: hisilicon default version, pull chipen after resume
	 * 010: hisilicon version, keep power (NOT pull chipen) and
	 *      reset sdio after resume
	 */
	sdio_cfg.cfg.vendor_id = WCN_VENDOR_DEFAULT;

	/* bit[4]: sdio_rx_mode: 0: adma; 1: sdma */
	if (sprdwcn_bus_get_rx_mode()) {
		sdio_cfg.cfg.sdio_rx_mode = 0;
		WCN_DEBUG("sdio_config rx mode:[adma]\n");
	} else {
		sdio_cfg.cfg.sdio_rx_mode = 1;
		WCN_INFO("sdio_config rx mode:[sdma]\n");
	}

	/* bit[7:5]: sdio_blk_size: 000: blocksize 840; 001: blocksize 512 */
	if (sprdwcn_bus_get_blk_size() == 512) {
		sdio_cfg.cfg.sdio_blk_size = 1;
		WCN_INFO("sdio_config blksize:[512]\n");
	} else
		WCN_DEBUG("sdio_config blksize:[840]\n");

	/*
	 * bit[8]: bt_wake_host_en: 0: disable, 1: enable
	 *
	 * When bit[8] is 1, bit[10:9] region will be parsed:
	 * bit[10:9]: bt_wake_host_trigger_type:
	 * 00:BT_WAKEUP_HOST  trigger type low
	 * 01:BT_WAKEUP_HOST  trigger type rising
	 * 10:BT_WAKEUP_HOST  trigger type falling
	 * 11:BT_WAKEUP_HOST  trigger type high
	 */
	if (marlin_get_bt_wl_wake_host_en() & BIT(BT_WAKE_HOST)) {
		sdio_cfg.cfg.bt_wake_host_en = 1;
		WCN_DEBUG("sdio_config bt_wake_host:[en]\n");
		sdio_cfg.cfg.bt_wake_host_trigger_type = 3;
		WCN_INFO("sdio_config bt_wake_host trigger:[high]\n");
	}

	/* bit[12:11]: sdio_irq_type:
	 * 00:dedicated irq, gpio1
	 * 01:inband data1 irq
	 * 10:use BT_WAKEUP_HOST(pubint) pin as gpio irq
	 * 11:use WL_WAKEUP_HOST(esmd3) pin as gpio irq
	 */
	if (sprdwcn_bus_get_irq_type() != 0) {
		sdio_cfg.cfg.sdio_irq_type = 1;
		WCN_INFO("sdio_config irq:[inband]\n");
	} else {
		WCN_INFO("sdio_config sdio_irq:[gpio1]\n");
	}

	/*
	 * bit[15]: wl_wake_host_en: 0: disable, 1: enable
	 *
	 * When bit[15] is 1, bit[17:16] region will be parsed:
	 * bit[17:16]: wl_wake_host_trigger_type:
	 * 00:WL_WAKEUP_HOST  trigger type low
	 * 01:WL_WAKEUP_HOST  trigger type rising
	 * 10:WL_WAKEUP_HOST  trigger type falling
	 * 11:WL_WAKEUP_HOST  trigger type high
	 */
	if (marlin_get_bt_wl_wake_host_en() & BIT(WL_WAKE_HOST)) {
		sdio_cfg.cfg.wl_wake_host_en = 1;
		WCN_DEBUG("sdio_config wl_wake_host:[en]\n");
		sdio_cfg.cfg.wl_wake_host_trigger_type = 3;
		WCN_INFO("sdio_config wl_wake_host trigger:[high]\n");
	}

	/*
	 * bit[22:18]: wake_host_level_duration_10s: BT_WAKEUP_HOST or
	 * WL_WAKEUP_HOST level dyration time per 10ms,
	 * example: 0:0ms; 3:30ms; 20:200ms
	 */
	sdio_cfg.cfg.wake_host_level_duration_10ms = 2;
	WCN_INFO("sdio_config wake_host_level_duration_time:[%dms]\n",
		 (sdio_cfg.cfg.wake_host_level_duration_10ms * 10));

	WCN_INFO("sdio_config wake_host_data_separation:[bt/wifi reuse]\n");
#else
	/* Default config */
	sdio_cfg.val = 0;
#endif

	marlin_dev->sync_f.sdio_config = sdio_cfg.val;
}

static int marlin_send_sdio_config_to_cp(void)
{
	int sdio_config_off = 0;

	sdio_config_off = (unsigned long)(&(marlin_dev->sync_f.sdio_config)) -
		(unsigned long)(&(marlin_dev->sync_f));
	WCN_DEBUG("sdio_config_offset:0x%x\n", sdio_config_off);

	marlin_send_sdio_config_to_cp_vendor();

	WCN_INFO("%s sdio_config:0x%x (%sable config)\n",
		 __func__, marlin_dev->sync_f.sdio_config,
		 (marlin_dev->sync_f.sdio_config & BIT(0)) ? "en" : "dis");

	return sprdwcn_bus_reg_write(SYNC_ADDR + sdio_config_off,
				     &(marlin_dev->sync_f.sdio_config), 4);
}
#endif
static int marlin_write_cali_data(void)
{
	int ret = 0, init_state = 0, cali_data_offset = 0;
	int i = 0;

	//WCN_INFO("tsx_dac_data:%d\n", marlin_dev->tsxcali.tsxdata.dac);
	cali_data_offset = (unsigned long)(&(marlin_dev->sync_f.tsx_dac_data))-
		(unsigned long)(&(marlin_dev->sync_f));
	WCN_DEBUG("cali_data_offset:0x%x\n", cali_data_offset);

	do {
		i++;
		ret = sprdwcn_bus_reg_read(SYNC_ADDR, &init_state, 4);
		if (ret < 0) {
			WCN_ERR("%s marlin3 read SYNC_ADDR error:%d\n",
				__func__, ret);
			return ret;
		}
		WCN_INFO("%s sync init_state:0x%x\n", __func__, init_state);

		if (init_state != SYNC_CALI_WAITING)
			msleep(20);
		/* wait cp in the state of waiting cali data */
		else {
			/*write cali data to cp*/
#ifdef CONFIG_SDIOHAL
			/*write sdio config to cp*/
			ret = marlin_send_sdio_config_to_cp();
			if (ret < 0) {
				WCN_ERR("write sdio_config error:%d\n", ret);
				return ret;
			}
#endif
			/*tell cp2 can handle cali data*/
			init_state = SYNC_CALI_WRITE_DONE;
			ret = sprdwcn_bus_reg_write(SYNC_ADDR, &init_state, 4);
			if (ret < 0) {
				WCN_ERR("write cali_done flag error:%d\n", ret);
				return ret;
			}

			i = 0;
			WCN_INFO("marlin_write_cali_data finish\n");
			return ret;
		}

		if (i > 10)
			i = 0;
	} while (i);

	return ret;

}

static int spi_read_rf_reg(unsigned int addr, unsigned int *data)
{
	unsigned int reg_data = 0;
	int ret;

	reg_data = ((addr & 0x7fff) << 16) | SPI_BIT31;
	ret = sprdwcn_bus_reg_write(SPI_BASE_ADDR, &reg_data, 4);
	if (ret < 0) {
		WCN_ERR("write SPI RF reg error:%d\n", ret);
		return ret;
	}

	usleep_range(4000, 6000);

	ret = sprdwcn_bus_reg_read(SPI_BASE_ADDR, &reg_data, 4);
	if (ret < 0) {
		WCN_ERR("read SPI RF reg error:%d\n", ret);
		return ret;
	}
	*data = reg_data & 0xffff;

	return 0;
}

static int check_cp_clock_mode(void)
{
	int ret = 0;
	unsigned int temp_val;

	WCN_DEBUG("%s\n", __func__);

	ret = spi_read_rf_reg(AD_DCXO_BONDING_OPT, &temp_val);
	if (ret < 0) {
		WCN_ERR("read AD_DCXO_BONDING_OPT error:%d\n", ret);
		return ret;
	}
	WCN_DEBUG("read AD_DCXO_BONDING_OPT val:0x%x\n", temp_val);
	if ((temp_val & tsx_mode) == tsx_mode)
		WCN_INFO("clock mode: TSX\n");
	else {
		WCN_INFO("clock mode: TCXO, outside clock\n");
	}

	return ret;
}

/* release CPU */
static int marlin_start_run(void)
{
	int ret = 0;
	unsigned int ss_val;

	WCN_DEBUG("marlin_start_run\n");

	marlin_tsx_cali_data_get();
#ifdef CONFIG_WCN_SLP
	sdio_pub_int_btwf_en0();
	/* after chip power on, reset sleep status */
	slp_mgr_reset();
#endif

	ret = sprdwcn_bus_reg_read(CP_RESET_REG, &ss_val, 4);
	if (ret < 0) {
		WCN_ERR("%s read reset reg error:%d\n", __func__, ret);
		return ret;
	}
	WCN_INFO("%s read reset reg val:0x%x\n", __func__, ss_val);
	ss_val  &= (~(RESET_BIT));
	WCN_INFO("after do %s reset reg val:0x%x\n", __func__, ss_val);
	ret = sprdwcn_bus_reg_write(CP_RESET_REG, &ss_val, 4);
	if (ret < 0) {
		WCN_ERR("%s write reset reg error:%d\n", __func__, ret);
		return ret;
	}
	marlin_bootup_time_update();	/* update the time at once. */

	ret = sprdwcn_bus_reg_read(CP_RESET_REG, &ss_val, 4);
	if (ret < 0) {
		WCN_ERR("%s read reset reg error:%d\n", __func__, ret);
		return ret;
	}
	WCN_DEBUG("%s reset reg val:0x%x\n", __func__, ss_val);

	return ret;
}

#ifdef CONFIG_AW_BIND_VERIFY
#include <crypto/sha2.h>

static void expand_seed(u8 *seed, u8 *out)
{
	unsigned char hash[64];
	int i;

	sha256(seed, 4, hash);

	for (i = 0; i < 4; i++) {
		memcpy(&out[i * 9], &hash[i * 8], 8);
		out[i * 9 + 8] = seed[i];
	}
}

static int wcn_bind_verify_calculate_verify_data(u8 *in, u8 *out)
{
	u8 seed[4], buf[36], a, b, c;
	int i, n;

	for (i = 0, n = 0; i < 4; i++) {
		a = in[n++];
		b = in[n++];
		c = in[n++];
		seed[i] = (a & b) ^ (a & c) ^ (b & c) ^ in[i + 12];
	}

	expand_seed(seed, buf);

	for (i = 0, n = 0; i < 12; i++) {
		a = buf[n++];
		b = buf[n++];
		c = buf[n++];
		out[i] = (a & b) ^ (a & c) ^ (b & c);
	}

	for (i = 0, n = 0; i < 4; i++) {
		a = out[n++];
		b = out[n++];
		c = out[n++];
		seed[i] = (a & b) ^ (~a & c);
	}

	expand_seed(seed, buf);

	for (i = 0, n = 0; i < 12; i++) {
		a = buf[n++];
		b = buf[n++];
		c = buf[n++];
		out[i] = (a & b) ^ (~a & c);
	}

	for (i = 0, n = 0; i < 4; i++) {
		a = out[n++];
		b = out[n++];
		c = out[n++];
		out[i + 12] = (a & b) ^ (a & c) ^ (b & c) ^ seed[i];
	}

	return 0;
}

static int marlin_bind_verify(void)
{
	unsigned char din[16], dout[16];
	int bind_verify_data_off = 0, init_state, ret = 0;

	/*transform confuse data to verify data*/
	memcpy(din, &marlin_dev->sync_f.bind_verify_data[0], 16);
	WCN_INFO("%s confuse data: 0x%02x%02x%02x%02x%02x%02x%02x%02x"
		 "%02x%02x%02x%02x%02x%02x%02x%02x\n", __func__,
		 din[0], din[1], din[2], din[3],
		 din[4], din[5], din[6], din[7],
		 din[8], din[9], din[10], din[11],
		 din[12], din[13], din[14], din[15]);
	wcn_bind_verify_calculate_verify_data(din, dout);
	WCN_INFO("%s verify data: 0x%02x%02x%02x%02x%02x%02x%02x%02x"
		 "%02x%02x%02x%02x%02x%02x%02x%02x\n", __func__,
		 dout[0], dout[1], dout[2], dout[3],
		 dout[4], dout[5], dout[6], dout[7],
		 dout[8], dout[9], dout[10], dout[11],
		 dout[12], dout[13], dout[14], dout[15]);

	/*send bind verify data to cp2*/
	memcpy(&marlin_dev->sync_f.bind_verify_data[0], dout, 16);
	bind_verify_data_off = (unsigned long)
		(&(marlin_dev->sync_f.bind_verify_data[0])) -
		(unsigned long)(&(marlin_dev->sync_f));
	ret = sprdwcn_bus_direct_write(SYNC_ADDR + bind_verify_data_off,
		&(marlin_dev->sync_f.bind_verify_data[0]), 16);
	if (ret < 0) {
		WCN_ERR("write bind verify data error:%d\n", ret);
		return ret;
	}

	/*tell cp2 can handle bind verify data*/
	init_state = SYNC_VERIFY_WRITE_DONE;
	ret = sprdwcn_bus_reg_write(SYNC_ADDR, &init_state, 4);
	if (ret < 0) {
		WCN_ERR("write bind verify flag error:%d\n", ret);
		return ret;
	}

	return ret;
}
#endif

static int check_cp_ready(void)
{
	int ret = 0;
	int i = 0;

	do {
		i++;
		ret = sprdwcn_bus_direct_read(SYNC_ADDR,
			&(marlin_dev->sync_f), sizeof(struct wcn_sync_info_t));
		if (ret < 0) {
			WCN_ERR("%s marlin3 read SYNC_ADDR error:%d\n",
				__func__, ret);
			return ret;
		}
		WCN_INFO("%s sync val:0x%x, prj_type val:0x%x\n", __func__,
				marlin_dev->sync_f.init_status,
				marlin_dev->sync_f.prj_type);
		if (marlin_dev->sync_f.init_status == SYNC_ALL_FINISHED)
			i = 0;
#ifdef CONFIG_AW_BIND_VERIFY
		else if (marlin_dev->sync_f.init_status ==
			SYNC_VERIFY_WAITING) {
			ret = marlin_bind_verify();
			if (ret != 0) {
				WCN_ERR("%s bind verify error:%d\n",
					__func__, ret);
				return ret;
			}
		}
#endif
		else
			msleep(20);
		if (i > 10)
			return -1;
	} while (i);

	return 0;
}

#if defined CONFIG_UWE5623 || defined CONFIG_CHECK_DRIVER_BY_CHIPID
static int marlin_reset_by_128_bit(void)
{
	unsigned char reg;

	WCN_INFO("%s entry\n", __func__);
	if (sprdwcn_bus_aon_readb(REG_CP_RST_CHIP, &reg)) {
		WCN_ERR("%s line:%d\n", __func__, __LINE__);
		return -1;
	}

	reg |= 1;
	if (sprdwcn_bus_aon_writeb(REG_CP_RST_CHIP, reg)) {
		WCN_ERR("%s line:%d\n", __func__, __LINE__);
		return -1;
	}

	return 0;
}
#endif

static int marlin_reset(int val)
{
#ifndef CONFIG_CHECK_DRIVER_BY_CHIPID
#ifdef CONFIG_UWE5623
	marlin_reset_by_128_bit();
#endif
#else /*CONFIG_CHECK_DRIVER_BY_CHIPID*/
	if (wcn_get_chip_model() == WCN_CHIP_MARLIN3E)
		marlin_reset_by_128_bit();
#endif

	if (marlin_dev->reset > 0) {
		if (gpio_is_valid(marlin_dev->reset)) {
			gpio_direction_output(marlin_dev->reset, 0);
			mdelay(RESET_DELAY);
			gpio_direction_output(marlin_dev->reset, 1);
		}
	}

	return 0;
}

static int chip_reset_release(int val)
{
	static unsigned int reset_count;

	if (marlin_dev->reset <= 0)
		return 0;

	if (!gpio_is_valid(marlin_dev->reset)) {
		WCN_ERR("reset gpio error\n");
		return -1;
	}
	if (val) {
		if (reset_count == 0)
			gpio_direction_output(marlin_dev->reset, 1);
		reset_count++;
	} else {
		gpio_direction_output(marlin_dev->reset, 0);
		reset_count--;
	}

	return 0;
}
void marlin_chip_en(bool enable, bool reset)
{
	static unsigned int chip_en_count;

	/*
	 * Incar board pull chipen gpio at pin control.
	 * Hisi board pull chipen gpio at hi_sdio_detect.ko.
	 */
	if (marlin_dev->chip_en <= 0)
		return;

	if (gpio_is_valid(marlin_dev->chip_en)) {
		if (reset) {
			gpio_direction_output(marlin_dev->chip_en, 0);
			WCN_INFO("marlin gnss chip en reset\n");
			msleep(100);
			gpio_direction_output(marlin_dev->chip_en, 1);
		} else if (enable) {
			if (chip_en_count == 0) {
				gpio_direction_output(marlin_dev->chip_en, 0);
				mdelay(1);
				gpio_direction_output(marlin_dev->chip_en, 1);
				mdelay(1);
				WCN_INFO("marlin chip en pull up\n");
			}
			chip_en_count++;
		} else {
			chip_en_count--;
			if (chip_en_count == 0) {
				gpio_direction_output(marlin_dev->chip_en, 0);
				WCN_INFO("marlin chip en pull down\n");
			}
		}
	}
}
EXPORT_SYMBOL_GPL(marlin_chip_en);

/*
 * MARLIN_AUTO no need loopcheck action
 */
static void power_state_notify_or_not(int subsys, int poweron)
{
#ifndef CONFIG_WCN_LOOPCHECK
	return;
#endif

	if ((test_bit(MARLIN_BLUETOOTH, &marlin_dev->power_state) +
	      test_bit(MARLIN_FM, &marlin_dev->power_state) +
	      test_bit(MARLIN_WIFI, &marlin_dev->power_state) +
	      test_bit(MARLIN_MDBG, &marlin_dev->power_state)) == 1) {
#ifdef CONFIG_WCN_LOOPCHECK
		WCN_DEBUG("only one module open, need to notify loopcheck\n");
		start_loopcheck();
#endif
		marlin_dev->loopcheck_status_change = 1;
		wakeup_loopcheck_int();
	}

	if (((marlin_dev->power_state) & MARLIN_MASK) == 0) {
#ifdef CONFIG_WCN_LOOPCHECK
		WCN_DEBUG("marlin close, need to notify loopcheck\n");
		stop_loopcheck();
#endif
		marlin_dev->loopcheck_status_change = 1;
		wakeup_loopcheck_int();

	}
}

static void pre_btwifi_download_sdio(struct work_struct *work)
{
	wcn_get_chip_name();

	if (btwifi_download_firmware() == 0 &&
		marlin_start_run() == 0) {
		check_cp_clock_mode();
		marlin_write_cali_data();
		/* check_cp_ready must be in front of mem_pd_save_bin,
		 * save bin task is scheduled after verify.
		 */
		if (check_cp_ready() != 0) {
			sprdwcn_bus_set_carddump_status(true);
			return;
		}
		sprdwcn_bus_runtime_get();

#ifndef CONFIG_WCND
		get_cp2_version();
		switch_cp2_log(false);
#endif
		complete(&marlin_dev->download_done);
	}
}

/*
 * RST_N (LOW)
 * VDDIO -> DVDD12/11 ->CHIP_EN ->DVDD_CORE(inner)
 * ->(>=550uS) RST_N (HIGH)
 * ->(>=100uS) ADVV12
 * ->(>=10uS)  AVDD33
 */
int chip_power_on(int subsys)
{
	WCN_DEBUG("%s\n", __func__);

	marlin_chip_en(true, false);
	msleep(20);
	chip_reset_release(1);
	sprdwcn_bus_driver_register();

	loopcheck_first_boot_set();
	if (marlin_dev->int_ap > 0)
		sdio_pub_int_poweron(true);

	return 0;
}

int chip_power_off(int subsys)
{
	WCN_INFO("%s\n", __func__);

	sprdwcn_bus_driver_unregister();
	marlin_chip_en(false, false);
	chip_reset_release(0);
	marlin_dev->wifi_need_download_ini_flag = 0;
	marlin_dev->power_state = 0;
	sprdwcn_bus_remove_card();
	loopcheck_first_boot_clear();
	if (marlin_dev->int_ap > 0)
		sdio_pub_int_poweron(false);

	return 0;
}

int open_power_ctl(void)
{
	marlin_dev->no_power_off = 0;
	clear_bit(WCN_AUTO, &marlin_dev->power_state);

	return 0;
}
EXPORT_SYMBOL_GPL(open_power_ctl);

void marlin_schedule_download_wq(void)
{
	schedule_work(&marlin_dev->download_wq);
}

static int marlin_set_power(int subsys, int val)
{
	unsigned long timeleft;

	WCN_DEBUG("mutex_lock\n");
// 	mutex_lock(&marlin_dev->power_lock);

	WCN_INFO("marlin power state:%lx, subsys: [%s] power %d\n",
			marlin_dev->power_state, strno(subsys), val);
	init_completion(&marlin_dev->download_done);

	/*  power on */
	if (val) {
		/* 1. when the first open:
		 * `- first download gnss, and then download btwifi
		 */
		marlin_dev->first_power_on_flag++;
		if (marlin_dev->first_power_on_flag == 1) {
			WCN_INFO("the first power on start\n");
			if (chip_power_on(subsys) < 0) {
				WCN_ERR("chip power on fail\n");
				goto out;
			}
			set_bit(subsys, &marlin_dev->power_state);
			WCN_INFO("then marlin start to download\n");
			schedule_work(&marlin_dev->download_wq);
			timeleft = wait_for_completion_timeout(
				&marlin_dev->download_done,
				msecs_to_jiffies(POWERUP_WAIT_MS));
			if (!timeleft) {
				WCN_ERR("marlin download timeout\n");
				goto out;
			}
			marlin_dev->download_finish_flag = 1;
			WCN_INFO("then marlin download finished and run ok\n");
			marlin_dev->first_power_on_flag = 2;
			WCN_DEBUG("mutex_unlock\n");
			mutex_unlock(&marlin_dev->power_lock);
			power_state_notify_or_not(subsys, val);
			return 0;
		}
		/* 2. the second time, WCN_AUTO coming */
		else if (subsys == WCN_AUTO) {
			if (marlin_dev->no_power_off) {
				WCN_INFO("have power on, no action\n");
				set_bit(subsys, &marlin_dev->power_state);
			}

			else {

				WCN_INFO("!1st,not to bkup gnss cal, no act\n");
			}
		}

		/* 3. when GNSS open,
		 *	  |- GNSS and MARLIN have power on and ready
		 */
		else if (((marlin_dev->power_state) & AUTO_RUN_MASK) != 0) {
			WCN_INFO("GNSS and marlin have ready\n");
			if (((marlin_dev->power_state) & MARLIN_MASK) == 0)
				loopcheck_first_boot_set();
			set_bit(subsys, &marlin_dev->power_state);

			goto check_power_state_notify;
		}
		/* 4. when GNSS close, marlin open.
		 *	  ->  subsys=gps,GNSS download
		 */
		else if (((marlin_dev->power_state) & MARLIN_MASK) != 0) {
			if (subsys == WCN_AUTO) {
				WCN_INFO("BTWF ready, GPS start to download\n");
				set_bit(subsys, &marlin_dev->power_state);
				WCN_INFO("GNSS download finished and ok\n");

			} else {
				WCN_INFO("marlin have open, GNSS is closed\n");
				set_bit(subsys, &marlin_dev->power_state);

				goto check_power_state_notify;
			}
		}
		/* 5. when GNSS close, marlin close.no module to power on */
		else {
			WCN_INFO("no module to power on, start to power on\n");
			if (chip_power_on(subsys) < 0) {
				WCN_ERR("chip power on fail\n");
				goto out;
			}
			set_bit(subsys, &marlin_dev->power_state);

			/* 5.1 first download marlin, and then download gnss */
			if (subsys == WCN_AUTO) {
				WCN_INFO("marlin start to download\n");
				schedule_work(&marlin_dev->download_wq);
				timeleft = wait_for_completion_timeout(
					&marlin_dev->download_done,
					msecs_to_jiffies(POWERUP_WAIT_MS));
				if (!timeleft) {
					WCN_ERR("marlin download timeout\n");
					goto out;
				}
				marlin_dev->download_finish_flag = 1;

				WCN_INFO("marlin dl finished and run ok\n");

			}
			/* 5.2 only download marlin, and then
			 * close gnss power domain
			 */
			else {
				WCN_INFO("only marlin start to download\n");
				schedule_work(&marlin_dev->download_wq);
				if (wait_for_completion_timeout(
					&marlin_dev->download_done,
					msecs_to_jiffies(POWERUP_WAIT_MS))
					<= 0) {

					WCN_ERR("marlin download timeout\n");
					goto out;
				}
				marlin_dev->download_finish_flag = 1;
				WCN_INFO("BTWF download finished and run ok\n");
			}
		}
		/* power on together's Action */
		power_state_notify_or_not(subsys, val);

		WCN_INFO("wcn chip power on and run finish: [%s]\n",
				  strno(subsys));
	/* power off */
	} else {
		if (marlin_dev->power_state == 0)
			goto check_power_state_notify;

		if (flag_reset)
			marlin_dev->power_state = 0;

		if (marlin_dev->no_power_off) {
			if (!flag_reset) {
				if (subsys != WCN_AUTO) {
					/* in order to not download again */
					set_bit(WCN_AUTO,
						&marlin_dev->power_state);
					clear_bit(subsys,
						&marlin_dev->power_state);
				}

				MDBG_LOG("marlin reset flag_reset:%d\n",
					flag_reset);

				goto check_power_state_notify;
			}
		}

		if (!marlin_dev->download_finish_flag)
			goto check_power_state_notify;

		clear_bit(subsys, &marlin_dev->power_state);
		if (marlin_dev->power_state != 0) {
			WCN_INFO("can not power off, other module is on\n");

			goto check_power_state_notify;
		}

		power_state_notify_or_not(subsys, val);

		WCN_INFO("wcn chip start power off!\n");
		sprdwcn_bus_runtime_put();
		chip_power_off(subsys);
		WCN_INFO("marlin power off!\n");
		marlin_dev->download_finish_flag = 0;
		if (flag_reset)
			flag_reset = FALSE;
	} /* power off end */

	/* power on off together's Action */
	WCN_DEBUG("mutex_unlock\n");
	mutex_unlock(&marlin_dev->power_lock);

	return 0;

out:
	sprdwcn_bus_runtime_put();
	sprdwcn_bus_driver_unregister();
	chip_reset_release(0);
	marlin_dev->power_state = 0;
	WCN_DEBUG("mutex_unlock\n");
	mutex_unlock(&marlin_dev->power_lock);

	return -1;

check_power_state_notify:
	power_state_notify_or_not(subsys, val);
	WCN_DEBUG("mutex_unlock\n");
	mutex_unlock(&marlin_dev->power_lock);

	return 0;
}

void marlin_power_off(enum marlin_sub_sys subsys)
{
	WCN_INFO("%s all\n", __func__);

	marlin_dev->no_power_off = false;
	set_bit(subsys, &marlin_dev->power_state);
	marlin_set_power(subsys, false);
}

int marlin_get_power(enum marlin_sub_sys subsys)
{
	if (subsys == MARLIN_ALL)
		return marlin_dev->power_state != 0;
	else
		return test_bit(subsys, &marlin_dev->power_state);
}
EXPORT_SYMBOL_GPL(marlin_get_power);

bool marlin_get_download_status(void)
{
	return marlin_dev->download_finish_flag;
}
EXPORT_SYMBOL_GPL(marlin_get_download_status);

int wcn_get_module_status_changed(void)
{
	return marlin_dev->loopcheck_status_change;
}
EXPORT_SYMBOL_GPL(wcn_get_module_status_changed);

void wcn_set_module_status_changed(bool status)
{
	marlin_dev->loopcheck_status_change = status;
}
EXPORT_SYMBOL_GPL(wcn_set_module_status_changed);

int marlin_get_module_status(void)
{
	if (test_bit(MARLIN_BLUETOOTH, &marlin_dev->power_state) ||
		test_bit(MARLIN_FM, &marlin_dev->power_state) ||
		test_bit(MARLIN_WIFI, &marlin_dev->power_state) ||
		test_bit(MARLIN_MDBG, &marlin_dev->power_state) ||
		test_bit(WCN_AUTO, &marlin_dev->power_state))
		return 1;
	else
		return 0;
}
EXPORT_SYMBOL_GPL(marlin_get_module_status);

int is_first_power_on(enum marlin_sub_sys subsys)
{
	if (marlin_dev->wifi_need_download_ini_flag == 1)
		return 1;	/* the first */
	else
		return 0;	/* not the first */
}
EXPORT_SYMBOL_GPL(is_first_power_on);

int cali_ini_need_download(enum marlin_sub_sys subsys)
{
	if (marlin_dev->wifi_need_download_ini_flag == 1) {
		WCN_INFO("cali_ini_need_download return 1\n");
		return 1;	/* the first */
	}
	return 0;	/* not the first */
}
EXPORT_SYMBOL_GPL(cali_ini_need_download);

/* Temporary modification for UWE5623:
 * cmd52 read/write timeout -110 issue.
 */
void marlin_read_test_after_reset(void)
{
	int ret;
	unsigned int reg_addr = AON_APB_TEST_READ_REG, reg_val;

	ret = sprdwcn_bus_reg_read(reg_addr, &reg_val, 4);
	if (ret < 0)
		WCN_ERR("%s read 0x%x error:%d\n", __func__, reg_addr, ret);
	else
		WCN_INFO("%s read 0x%x = 0x%x\n", __func__, reg_addr, reg_val);
}

int marlin_reset_reg(void)
{
	marlin_reset(true);
	mdelay(1);

	/* Temporary modification for UWE5623:
	 * cmd52 read/write timeout -110 issue.
	 */
	marlin_read_test_after_reset();

	return 0;
}
EXPORT_SYMBOL_GPL(marlin_reset_reg);

int start_marlin(u32 subsys)
{
	WCN_INFO("%s [%s]\n", __func__, strno(subsys));
	if (sprdwcn_bus_get_carddump_status() != 0) {
		WCN_ERR("%s SDIO card dump\n", __func__);
		return -1;
	}

	if (get_loopcheck_status()) {
		WCN_ERR("%s loopcheck status is fail\n", __func__);
		return -1;
	}

	if (subsys == MARLIN_WIFI) {
		/* not need write cali */
		if (marlin_dev->wifi_need_download_ini_flag == 0)
			/* need write cali */
			marlin_dev->wifi_need_download_ini_flag = 1;
		else
			/* not need write cali */
			marlin_dev->wifi_need_download_ini_flag = 2;
	}
	marlin_set_power(subsys, true);

	return 0;
}
EXPORT_SYMBOL_GPL(start_marlin);

int stop_marlin(u32 subsys)
{
	if (sprdwcn_bus_get_carddump_status() != 0) {
		WCN_ERR("%s SDIO card dump\n", __func__);
		return -1;
	}

	if (get_loopcheck_status()) {
		WCN_ERR("%s loopcheck status is fail\n", __func__);
		return -1;
	}

	return marlin_set_power(subsys, false);
}
EXPORT_SYMBOL_GPL(stop_marlin);

static void marlin_power_wq(struct work_struct *work)
{
	WCN_INFO("%s start\n", __func__);

	/* WCN_AUTO is for auto backup gnss cali data */
	marlin_set_power(WCN_AUTO, true);
}

int marlin_probe(struct device *dev)
{
	marlin_dev = devm_kzalloc(dev,
			sizeof(struct marlin_device), GFP_KERNEL);
	if (!marlin_dev)
		return -ENOMEM;
	marlin_dev->write_buffer = devm_kzalloc(dev,
			PACKET_SIZE, GFP_KERNEL);
	if (marlin_dev->write_buffer == NULL) {
		devm_kfree(dev, marlin_dev);
		WCN_ERR("marlin_probe write buffer low memory\n");
		return -ENOMEM;
	}
	mutex_init(&(marlin_dev->power_lock));
	marlin_dev->power_state = 0;
	if (marlin_parse_dt(dev) < 0)
		WCN_INFO("marlin2 parse_dt some para not config\n");
	if (marlin_dev->reset > 0) {
		if (gpio_is_valid(marlin_dev->reset))
			gpio_direction_output(marlin_dev->reset, 0);
	}
#ifdef CONFIG_WCN_SLP
	slp_mgr_init();
#endif
	/* register ops */
	wcn_bus_init();
	/* sdiom_init or pcie_init */
	sprdwcn_bus_preinit();
	if (marlin_dev->int_ap > 0)
		sdio_pub_int_init(marlin_dev->int_ap);
	proc_fs_init();
	mdbg_atcmd_owner_init();
#ifndef CONFIG_WCND
	loopcheck_init();
#endif

	flag_reset = 0;

	INIT_WORK(&marlin_dev->download_wq, pre_btwifi_download_sdio);
	INIT_DELAYED_WORK(&marlin_dev->power_wq, marlin_power_wq);

	WCN_INFO("marlin_probe ok!\n");

	return 0;
}
EXPORT_SYMBOL_GPL(marlin_probe);

int marlin_remove(struct device *dev)
{
	cancel_work_sync(&marlin_dev->download_wq);
	cancel_delayed_work_sync(&marlin_dev->power_wq);
#ifndef CONFIG_WCND
	loopcheck_deinit();
#endif
	mdbg_atcmd_owner_deinit();
	proc_fs_exit();
	if (marlin_dev->int_ap > 0)
		sdio_pub_int_deinit();

	sprdwcn_bus_deinit();
	if ((marlin_dev->power_state != 0) && (!marlin_dev->no_power_off)) {
		WCN_INFO("marlin some subsys power is on, warning!\n");
		marlin_chip_en(false, false);
	}
	wcn_bus_deinit();
#ifdef CONFIG_WCN_SLP
	slp_mgr_deinit();
#endif
	marlin_gpio_free();
	mutex_destroy(&marlin_dev->power_lock);

	WCN_INFO("marlin_remove ok!\n");

	return 0;
}
EXPORT_SYMBOL_GPL(marlin_remove);

void marlin_shutdown(void)
{
	/* When the following three conditions are met at the same time,
	 * wcn chip will be powered off:
	 * 1. chip has been powered on (power_state is not 0);
	 * 2. config power up and down (not keep power on);
	 * 3. bt/wifi wake host is disabled.
	 */
	if ((marlin_dev->power_state != 0) && (!marlin_dev->no_power_off) &&
		(!marlin_get_bt_wl_wake_host_en())) {
		WCN_INFO("marlin some subsys power is on, warning!\n");
		marlin_chip_en(false, false);
	}
	WCN_INFO("marlin_shutdown end\n");
}
EXPORT_SYMBOL_GPL(marlin_shutdown);

int marlin_reset_register_notify(void *callback_func, void *para)
{
	marlin_reset_func = (marlin_reset_callback)callback_func;
	marlin_callback_para = para;

	return 0;
}
EXPORT_SYMBOL_GPL(marlin_reset_register_notify);

int marlin_reset_unregister_notify(void)
{
	marlin_reset_func = NULL;
	marlin_callback_para = NULL;

	return 0;
}
EXPORT_SYMBOL_GPL(marlin_reset_unregister_notify);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("Spreadtrum  WCN Marlin Driver");
MODULE_AUTHOR("Yufeng Yang <yufeng.yang@spreadtrum.com>");
