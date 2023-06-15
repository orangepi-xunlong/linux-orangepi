/**
 ******************************************************************************
 *
 * @file rwnx_platform.c
 *
 * Copyright (C) RivieraWaves 2012-2019
 *
 ******************************************************************************
 */

#include <linux/module.h>
#include <linux/firmware.h>
#include <linux/delay.h>

#include "rwnx_platform.h"
#include "reg_access.h"
#include "hal_desc.h"
#include "rwnx_main.h"
#include "rwnx_pci.h"
#ifndef CONFIG_RWNX_FHOST
#include "ipc_host.h"
#endif /* !CONFIG_RWNX_FHOST */
#include "rwnx_msg_tx.h"

#ifdef AICWF_SDIO_SUPPORT
#include "aicwf_sdio.h"
#endif

#ifdef AICWF_USB_SUPPORT
#include "aicwf_usb.h"
#endif

struct rwnx_plat *g_rwnx_plat;

#ifdef CONFIG_RWNX_TL4
/**
 * rwnx_plat_tl4_fw_upload() - Load the requested FW into embedded side.
 *
 * @rwnx_plat: pointer to platform structure
 * @fw_addr: Virtual address where the fw must be loaded
 * @filename: Name of the fw.
 *
 * Load a fw, stored as a hex file, into the specified address
 */
static int rwnx_plat_tl4_fw_upload(struct rwnx_plat *rwnx_plat, u8 *fw_addr,
								   char *filename)
{
	struct device *dev = rwnx_platform_get_dev(rwnx_plat);
	const struct firmware *fw;
	int err = 0;
	u32 *dst;
	u8 const *file_data;
	char typ0, typ1;
	u32 addr0, addr1;
	u32 dat0, dat1;
	int remain;

	err = request_firmware(&fw, filename, dev);
	if (err) {
		return err;
	}
	file_data = fw->data;
	remain = fw->size;

	/* Copy the file on the Embedded side */
	dev_dbg(dev, "\n### Now copy %s firmware, @ = %p\n", filename, fw_addr);

	/* Walk through all the lines of the configuration file */
	while (remain >= 16) {
		u32 data, offset;

		if (sscanf(file_data, "%c:%08X %04X", &typ0, &addr0, &dat0) != 3)
			break;
		if ((addr0 & 0x01) != 0) {
			addr0 = addr0 - 1;
			dat0 = 0;
		} else {
			file_data += 16;
			remain -= 16;
		}
		if ((remain < 16) ||
			(sscanf(file_data, "%c:%08X %04X", &typ1, &addr1, &dat1) != 3) ||
			(typ1 != typ0) || (addr1 != (addr0 + 1))) {
			typ1 = typ0;
			addr1 = addr0 + 1;
			dat1 = 0;
		} else {
			file_data += 16;
			remain -= 16;
		}

		if (typ0 == 'C') {
			offset = 0x00200000;
			if ((addr1 % 4) == 3)
				offset += 2*(addr1 - 3);
			else
				offset += 2*(addr1 + 1);

			data = dat1 | (dat0 << 16);
		} else {
			offset = 2*(addr1 - 1);
			data = dat0 | (dat1 << 16);
		}
		dst = (u32 *)(fw_addr + offset);
		*dst = data;
	}

	release_firmware(fw);

	return err;
}
#endif

#if 0
/**
 * rwnx_plat_bin_fw_upload() - Load the requested binary FW into embedded side.
 *
 * @rwnx_plat: pointer to platform structure
 * @fw_addr: Virtual address where the fw must be loaded
 * @filename: Name of the fw.
 *
 * Load a fw, stored as a binary file, into the specified address
 */
static int rwnx_plat_bin_fw_upload(struct rwnx_plat *rwnx_plat, u8 *fw_addr,
							   char *filename)
{
	const struct firmware *fw;
	struct device *dev = rwnx_platform_get_dev(rwnx_plat);
	int err = 0;
	unsigned int i, size;
	u32 *src, *dst;

	err = request_firmware(&fw, filename, dev);
	if (err) {
		return err;
	}

	/* Copy the file on the Embedded side */
	dev_dbg(dev, "\n### Now copy %s firmware, @ = %p\n", filename, fw_addr);

	src = (u32 *)fw->data;
	dst = (u32 *)fw_addr;
	size = (unsigned int)fw->size;

	/* check potential platform bug on multiple stores vs memcpy */
	for (i = 0; i < size; i += 4) {
		*dst++ = *src++;
	}

	release_firmware(fw);

	return err;
}
#endif

#ifndef CONFIG_ROM_PATCH_EN
#if !defined(CONFIG_NANOPI_M4) && !defined(CONFIG_PLATFORM_ALLWINNER)
/**
 * rwnx_plat_bin_fw_upload_2() - Load the requested binary FW into embedded side.
 *
 * @rwnx_hw: Main driver data
 * @fw_addr: Address where the fw must be loaded
 * @filename: Name of the fw.
 *
 * Load a fw, stored as a binary file, into the specified address
 */
static int rwnx_plat_bin_fw_upload_2(struct rwnx_hw *rwnx_hw, u32 fw_addr,
							   char *filename)
{
	const struct firmware *fw;
	struct device *dev = rwnx_platform_get_dev(rwnx_hw->plat);
	int err = 0;
	unsigned int i, size;
	u32 *src, *dst;

	err = request_firmware(&fw, filename, dev);
	if (err) {
		printk("request_firmware fail: %d\r\n", err);
		return err;
	}

	/* Copy the file on the Embedded side */
	printk("\n### Upload %s firmware, @ = %x\n", filename, fw_addr);

	size = (unsigned int)fw->size;
	src = (u32 *)fw->data;
	dst = (u32 *)kzalloc(size, GFP_KERNEL);
	if (dst == NULL) {
		printk(KERN_CRIT "%s: data allocation failed\n", __func__);
		return -2;
	}
	//printk("src:%p,dst:%p,size:%d\r\n",src,dst,size);
	for (i = 0; i < (size / 4); i++) {
		dst[i] = src[i];
	}

	if (size > 1024) {
		for (i = 0; i < (size - 1024); i += 1024) {
			//printk("wr blk 0: %p -> %x\r\n", dst + i / 4, fw_addr + i);
			err = rwnx_send_dbg_mem_block_write_req(rwnx_hw, fw_addr + i, 1024, dst + i / 4);
			if (err) {
				printk("bin upload fail: %x, err:%d\r\n", fw_addr + i, err);
				break;
			}
		}
	}
	if (!err && (i < size)) {
		//printk("wr blk 1: %p -> %x\r\n", dst + i / 4, fw_addr + i);
		err = rwnx_send_dbg_mem_block_write_req(rwnx_hw, fw_addr + i, size - i, dst + i / 4);
		if (err) {
			printk("bin upload fail: %x, err:%d\r\n", fw_addr + i, err);
		}
	}
	if (dst) {
		kfree(dst);
		dst = NULL;
	}

	release_firmware(fw);

	return err;
}
#endif

typedef struct {
	txpwr_idx_conf_t txpwr_idx;
	txpwr_ofst_conf_t txpwr_ofst;
} nvram_info_t;

nvram_info_t nvram_info = {
	.txpwr_idx = {
		.enable           = 1,
		.dsss             = 9,
		.ofdmlowrate_2g4  = 8,
		.ofdm64qam_2g4    = 8,
		.ofdm256qam_2g4   = 8,
		.ofdm1024qam_2g4  = 8,
		.ofdmlowrate_5g   = 11,
		.ofdm64qam_5g     = 10,
		.ofdm256qam_5g    = 9,
		.ofdm1024qam_5g   = 9
	},
	.txpwr_ofst = {
		.enable       = 1,
		.chan_1_4     = 0,
		.chan_5_9     = 0,
		.chan_10_13   = 0,
		.chan_36_64   = 0,
		.chan_100_120 = 0,
		.chan_122_140 = 0,
		.chan_142_165 = 0,
	},
};

void get_userconfig_txpwr_idx(txpwr_idx_conf_t *txpwr_idx)
{
	memcpy(txpwr_idx, &(nvram_info.txpwr_idx), sizeof(txpwr_idx_conf_t));
}

void get_userconfig_txpwr_ofst(txpwr_ofst_conf_t *txpwr_ofst)
{
	memcpy(txpwr_ofst, &(nvram_info.txpwr_ofst), sizeof(txpwr_ofst_conf_t));
}

#define MATCH_NODE(type, node, cfg_key) {cfg_key, offsetof(type, node)}

struct parse_match_t {
	char keyname[64];
	int  offset;
};

static const char *parse_key_prefix[] = {
	[0x01] = "module0_",
	[0x21] = "module1_",
};

static const struct parse_match_t parse_match_tab[] = {
	MATCH_NODE(nvram_info_t, txpwr_idx.enable,           "enable"),
	MATCH_NODE(nvram_info_t, txpwr_idx.dsss,             "dsss"),
	MATCH_NODE(nvram_info_t, txpwr_idx.ofdmlowrate_2g4,  "ofdmlowrate_2g4"),
	MATCH_NODE(nvram_info_t, txpwr_idx.ofdm64qam_2g4,    "ofdm64qam_2g4"),
	MATCH_NODE(nvram_info_t, txpwr_idx.ofdm256qam_2g4,   "ofdm256qam_2g4"),
	MATCH_NODE(nvram_info_t, txpwr_idx.ofdm1024qam_2g4,  "ofdm1024qam_2g4"),
	MATCH_NODE(nvram_info_t, txpwr_idx.ofdmlowrate_5g,   "ofdmlowrate_5g"),
	MATCH_NODE(nvram_info_t, txpwr_idx.ofdm64qam_5g,     "ofdm64qam_5g"),
	MATCH_NODE(nvram_info_t, txpwr_idx.ofdm256qam_5g,    "ofdm256qam_5g"),
	MATCH_NODE(nvram_info_t, txpwr_idx.ofdm1024qam_5g,   "ofdm1024qam_5g"),

	MATCH_NODE(nvram_info_t, txpwr_ofst.enable,          "ofst_enable"),
	MATCH_NODE(nvram_info_t, txpwr_ofst.chan_1_4,        "ofst_chan_1_4"),
	MATCH_NODE(nvram_info_t, txpwr_ofst.chan_5_9,        "ofst_chan_5_9"),
	MATCH_NODE(nvram_info_t, txpwr_ofst.chan_10_13,      "ofst_chan_10_13"),
	MATCH_NODE(nvram_info_t, txpwr_ofst.chan_36_64,      "ofst_chan_36_64"),
	MATCH_NODE(nvram_info_t, txpwr_ofst.chan_100_120,    "ofst_chan_100_120"),
	MATCH_NODE(nvram_info_t, txpwr_ofst.chan_122_140,    "ofst_chan_122_140"),
	MATCH_NODE(nvram_info_t, txpwr_ofst.chan_142_165,    "ofst_chan_142_165"),
};

static int parse_key_val(const char *str, const char *key, char *val)
{
	const char *p = NULL;
	const char *dst = NULL;
	int keysize = 0;
	int bufsize = 0;

	if (str == NULL || key == NULL || val == NULL)
		return -1;

	keysize = strlen(key);
	bufsize = strlen(str);
	if (bufsize <= keysize)
		return -1;

	p = str;
	while (*p != 0 && *p == ' ')
		p++;

	if (*p == '#')
		return -1;

	if (str + bufsize - p <= keysize)
		return -1;

	if (strncmp(p, key, keysize) != 0)
		return -1;

	p += keysize;

	while (*p != 0 && *p == ' ')
		p++;

	if (*p != '=')
		return -1;

	p++;
	while (*p != 0 && *p == ' ')
		p++;

	if (*p == '"')
		p++;

	dst = p;
	while (*p != 0)
		p++;

	p--;
	while (*p == ' ')
		p--;

	if (*p == '"')
		p--;

	while (*p == '\r' || *p == '\n')
		p--;

	p++;
	strncpy(val, dst, p -dst);
	val[p - dst] = 0;
	return 0;
}

void rwnx_plat_userconfig_parsing(struct rwnx_hw *rwnx_hw, char *buffer, int size)
{
	char conf[100], keyname[64];
	char *line;
	char *data;
	int  i = 0, err, len = 0;
	long val;

	if (size <= 0) {
		pr_err("Config buffer size %d error\n", size);
		return;
	}

	if (rwnx_hw->vendor_info > (sizeof(parse_key_prefix) / sizeof(parse_key_prefix[0]) - 1)) {
		pr_err("Unsuppor vendor info config\n");
		return;
	}

	data = vmalloc(size + 1);
	if (!data) {
		pr_err("vmalloc fail\n");
		return;
	}

	memcpy(data, buffer, size);
	buffer = data;

	while (1) {
		line = buffer;
		if (*line == 0)
			break;

		while (*buffer != '\r' && *buffer != '\n' && *buffer != 0 && len++ < size)
			buffer++;

		while ((*buffer == '\r' || *buffer == '\n') && len++ < size)
			*buffer++ = 0;

		if (len >= size)
			*buffer = 0;

		// store value to data struct
		for (i = 0; i < sizeof(parse_match_tab) / sizeof(parse_match_tab[0]); i++) {
			sprintf(&keyname[0], "%s%s", parse_key_prefix[rwnx_hw->vendor_info], parse_match_tab[i].keyname);
			if (parse_key_val(line, keyname, conf) == 0) {
				err = kstrtol(conf, 0, &val);
				*(unsigned long *)((unsigned long)&nvram_info + parse_match_tab[i].offset) = val;
				printk("%s, %s = %ld\n",  __func__, parse_match_tab[i].keyname, val);
				break;
			}
		}
	}
	vfree(data);
}

#define FW_USERCONFIG_NAME       "aic_userconfig.txt"

int rwnx_plat_userconfig_upload_android(struct rwnx_hw *rwnx_hw, char *filename)
{
	int size;
	char *dst = NULL;

	const struct firmware *fw = NULL;
	int ret = request_firmware(&fw, filename, NULL);

	printk("userconfig file path:%s \r\n", filename);

	if (ret < 0) {
		printk("Load %s fail\n", filename);
		return ret;
	}

	size = fw->size;
	dst = (char *)fw->data;

	if (size <= 0) {
		printk("wrong size of firmware file\n");
		release_firmware(fw);
		return -1;
	}

	rwnx_plat_userconfig_parsing(rwnx_hw, (char *)dst, size);

	release_firmware(fw);

	return 0;
}

/**
 * rwnx_plat_fmac_load() - Load FW code
 *
 * @rwnx_hw: Main driver data
 */
static int rwnx_plat_fmac_load(struct rwnx_hw *rwnx_hw)
{
	int ret = 0;

	RWNX_DBG(RWNX_FN_ENTRY_STR);
	ret = rwnx_plat_userconfig_upload_android(rwnx_hw, FW_USERCONFIG_NAME);
	return ret;
}
#endif /* !CONFIG_ROM_PATCH_EN */

/**
 * rwnx_platform_reset() - Reset the platform
 *
 * @rwnx_plat: platform data
 */
static int rwnx_platform_reset(struct rwnx_plat *rwnx_plat)
{
	u32 regval;

#if defined(AICWF_USB_SUPPORT) || defined(AICWF_SDIO_SUPPORT)
	return 0;
#endif

	/* the doc states that SOFT implies FPGA_B_RESET
	 * adding FPGA_B_RESET is clearer */
	RWNX_REG_WRITE(SOFT_RESET | FPGA_B_RESET, rwnx_plat,
				   RWNX_ADDR_SYSTEM, SYSCTRL_MISC_CNTL_ADDR);
	msleep(100);

	regval = RWNX_REG_READ(rwnx_plat, RWNX_ADDR_SYSTEM, SYSCTRL_MISC_CNTL_ADDR);

	if (regval & SOFT_RESET) {
		dev_err(rwnx_platform_get_dev(rwnx_plat), "reset: failed\n");
		return -EIO;
	}

	RWNX_REG_WRITE(regval & ~FPGA_B_RESET, rwnx_plat,
				   RWNX_ADDR_SYSTEM, SYSCTRL_MISC_CNTL_ADDR);
	msleep(100);
	return 0;
}

/**
 * rwmx_platform_save_config() - Save hardware config before reload
 *
 * @rwnx_plat: Pointer to platform data
 *
 * Return configuration registers values.
 */
static void *rwnx_term_save_config(struct rwnx_plat *rwnx_plat)
{
	const u32 *reg_list;
	u32 *reg_value, *res;
	int i, size = 0;

	if (rwnx_plat->get_config_reg) {
		size = rwnx_plat->get_config_reg(rwnx_plat, &reg_list);
	}

	if (size <= 0)
		return NULL;

	res = kmalloc(sizeof(u32) * size, GFP_KERNEL);
	if (!res)
		return NULL;

	reg_value = res;
	for (i = 0; i < size; i++) {
		*reg_value++ = RWNX_REG_READ(rwnx_plat, RWNX_ADDR_SYSTEM,
									 *reg_list++);
	}

	return res;
}

#if 0
/**
 * rwmx_platform_restore_config() - Restore hardware config after reload
 *
 * @rwnx_plat: Pointer to platform data
 * @reg_value: Pointer of value to restore
 * (obtained with rwmx_platform_save_config())
 *
 * Restore configuration registers value.
 */
static void rwnx_term_restore_config(struct rwnx_plat *rwnx_plat,
									 u32 *reg_value)
{
	const u32 *reg_list;
	int i, size = 0;

	if (!reg_value || !rwnx_plat->get_config_reg)
		return;

	size = rwnx_plat->get_config_reg(rwnx_plat, &reg_list);

	for (i = 0; i < size; i++) {
		RWNX_REG_WRITE(*reg_value++, rwnx_plat, RWNX_ADDR_SYSTEM,
					   *reg_list++);
	}
}
#endif

#ifndef CONFIG_RWNX_FHOST
#if 0
static int rwnx_check_fw_compatibility(struct rwnx_hw *rwnx_hw)
{
	struct ipc_shared_env_tag *shared = rwnx_hw->ipc_env->shared;
	#ifdef CONFIG_RWNX_FULLMAC
	struct wiphy *wiphy = rwnx_hw->wiphy;
	#endif //CONFIG_RWNX_FULLMAC
	#ifdef CONFIG_RWNX_OLD_IPC
	int ipc_shared_version = 10;
	#else //CONFIG_RWNX_OLD_IPC
	int ipc_shared_version = 11;
	#endif //CONFIG_RWNX_OLD_IPC
	int res = 0;

	if (shared->comp_info.ipc_shared_version != ipc_shared_version) {
		wiphy_err(wiphy, "Different versions of IPC shared version between driver and FW (%d != %d)\n ",
				  ipc_shared_version, shared->comp_info.ipc_shared_version);
		res = -1;
	}

	if (shared->comp_info.radarbuf_cnt != IPC_RADARBUF_CNT) {
		wiphy_err(wiphy, "Different number of host buffers available for Radar events handling "\
				  "between driver and FW (%d != %d)\n", IPC_RADARBUF_CNT,
				  shared->comp_info.radarbuf_cnt);
		res = -1;
	}

	if (shared->comp_info.unsuprxvecbuf_cnt != IPC_UNSUPRXVECBUF_CNT) {
		wiphy_err(wiphy, "Different number of host buffers available for unsupported Rx vectors "\
				  "handling between driver and FW (%d != %d)\n", IPC_UNSUPRXVECBUF_CNT,
				  shared->comp_info.unsuprxvecbuf_cnt);
		res = -1;
	}

	#ifdef CONFIG_RWNX_FULLMAC
	if (shared->comp_info.rxdesc_cnt != IPC_RXDESC_CNT) {
		wiphy_err(wiphy, "Different number of shared descriptors available for Data RX handling "\
				  "between driver and FW (%d != %d)\n", IPC_RXDESC_CNT,
				  shared->comp_info.rxdesc_cnt);
		res = -1;
	}
	#endif /* CONFIG_RWNX_FULLMAC */

	if (shared->comp_info.rxbuf_cnt != IPC_RXBUF_CNT) {
		wiphy_err(wiphy, "Different number of host buffers available for Data Rx handling "\
				  "between driver and FW (%d != %d)\n", IPC_RXBUF_CNT,
				  shared->comp_info.rxbuf_cnt);
		res = -1;
	}

	if (shared->comp_info.msge2a_buf_cnt != IPC_MSGE2A_BUF_CNT) {
		wiphy_err(wiphy, "Different number of host buffers available for Emb->App MSGs "\
				  "sending between driver and FW (%d != %d)\n", IPC_MSGE2A_BUF_CNT,
				  shared->comp_info.msge2a_buf_cnt);
		res = -1;
	}

	if (shared->comp_info.dbgbuf_cnt != IPC_DBGBUF_CNT) {
		wiphy_err(wiphy, "Different number of host buffers available for debug messages "\
				  "sending between driver and FW (%d != %d)\n", IPC_DBGBUF_CNT,
				  shared->comp_info.dbgbuf_cnt);
		res = -1;
	}

	if (shared->comp_info.bk_txq != NX_TXDESC_CNT0) {
		wiphy_err(wiphy, "Driver and FW have different sizes of BK TX queue (%d != %d)\n",
				  NX_TXDESC_CNT0, shared->comp_info.bk_txq);
		res = -1;
	}

	if (shared->comp_info.be_txq != NX_TXDESC_CNT1) {
		wiphy_err(wiphy, "Driver and FW have different sizes of BE TX queue (%d != %d)\n",
				  NX_TXDESC_CNT1, shared->comp_info.be_txq);
		res = -1;
	}

	if (shared->comp_info.vi_txq != NX_TXDESC_CNT2) {
		wiphy_err(wiphy, "Driver and FW have different sizes of VI TX queue (%d != %d)\n",
				  NX_TXDESC_CNT2, shared->comp_info.vi_txq);
		res = -1;
	}

	if (shared->comp_info.vo_txq != NX_TXDESC_CNT3) {
		wiphy_err(wiphy, "Driver and FW have different sizes of VO TX queue (%d != %d)\n",
				  NX_TXDESC_CNT3, shared->comp_info.vo_txq);
		res = -1;
	}

	#if NX_TXQ_CNT == 5
	if (shared->comp_info.bcn_txq != NX_TXDESC_CNT4) {
		wiphy_err(wiphy, "Driver and FW have different sizes of BCN TX queue (%d != %d)\n",
				NX_TXDESC_CNT4, shared->comp_info.bcn_txq);
		res = -1;
	}
	#else
	if (shared->comp_info.bcn_txq > 0) {
		wiphy_err(wiphy, "BCMC enabled in firmware but disabled in driver\n");
		res = -1;
	}
	#endif /* NX_TXQ_CNT == 5 */

	if (shared->comp_info.ipc_shared_size != sizeof(ipc_shared_env)) {
		wiphy_err(wiphy, "Different sizes of IPC shared between driver and FW (%zd != %d)\n",
				  sizeof(ipc_shared_env), shared->comp_info.ipc_shared_size);
		res = -1;
	}

	if (shared->comp_info.msg_api != MSG_API_VER) {
		wiphy_warn(wiphy, "WARNING: Different supported message API versions between "\
				   "driver and FW (%d != %d)\n", MSG_API_VER, shared->comp_info.msg_api);
	}

	return res;
}
#endif
#endif /* !CONFIG_RWNX_FHOST */

/**
 * rwnx_platform_on() - Start the platform
 *
 * @rwnx_hw: Main driver data
 * @config: Config to restore (NULL if nothing to restore)
 *
 * It starts the platform :
 * - load fw and ucodes
 * - initialize IPC
 * - boot the fw
 * - enable link communication/IRQ
 *
 * Called by 802.11 part
 */
int rwnx_platform_on(struct rwnx_hw *rwnx_hw, void *config)
{
	int ret;
	struct rwnx_plat *rwnx_plat = rwnx_hw->plat;
	(void)ret;

	RWNX_DBG(RWNX_FN_ENTRY_STR);

	if (rwnx_plat->enabled)
		return 0;

	#ifndef CONFIG_ROM_PATCH_EN
	#ifdef CONFIG_DOWNLOAD_FW
	ret = rwnx_plat_fmac_load(rwnx_hw);
	if (ret)
		return ret;
	#endif /* !CONFIG_ROM_PATCH_EN */
	#endif

	rwnx_plat->enabled = true;

	return 0;
}

/**
 * rwnx_platform_off() - Stop the platform
 *
 * @rwnx_hw: Main driver data
 * @config: Updated with pointer to config, to be able to restore it with
 * rwnx_platform_on(). It's up to the caller to free the config. Set to NULL
 * if configuration is not needed.
 *
 * Called by 802.11 part
 */
void rwnx_platform_off(struct rwnx_hw *rwnx_hw, void **config)
{
#if defined(AICWF_USB_SUPPORT) || defined(AICWF_SDIO_SUPPORT)
	tasklet_kill(&rwnx_hw->task);
	rwnx_hw->plat->enabled = false;
	return ;
#endif

	if (!rwnx_hw->plat->enabled) {
		if (config)
			*config = NULL;
		return;
	}

	if (config)
		*config = rwnx_term_save_config(rwnx_hw->plat);

	rwnx_hw->plat->disable(rwnx_hw);

	tasklet_kill(&rwnx_hw->task);
	rwnx_platform_reset(rwnx_hw->plat);

	rwnx_hw->plat->enabled = false;
}

/**
 * rwnx_platform_init() - Initialize the platform
 *
 * @rwnx_plat: platform data (already updated by platform driver)
 * @platform_data: Pointer to store the main driver data pointer (aka rwnx_hw)
 *                That will be set as driver data for the platform driver
 * Return: 0 on success, < 0 otherwise
 *
 * Called by the platform driver after it has been probed
 */
int rwnx_platform_init(struct rwnx_plat *rwnx_plat, void **platform_data)
{
	RWNX_DBG(RWNX_FN_ENTRY_STR);

	rwnx_plat->enabled = false;
	g_rwnx_plat = rwnx_plat;

#if defined CONFIG_RWNX_FULLMAC
	return rwnx_cfg80211_init(rwnx_plat, platform_data);
#elif defined CONFIG_RWNX_FHOST
	return rwnx_fhost_init(rwnx_plat, platform_data);
#endif
}

/**
 * rwnx_platform_deinit() - Deinitialize the platform
 *
 * @rwnx_hw: main driver data
 *
 * Called by the platform driver after it is removed
 */
void rwnx_platform_deinit(struct rwnx_hw *rwnx_hw)
{
	RWNX_DBG(RWNX_FN_ENTRY_STR);

#if defined CONFIG_RWNX_FULLMAC
	rwnx_cfg80211_deinit(rwnx_hw);
#elif defined CONFIG_RWNX_FHOST
	rwnx_fhost_deinit(rwnx_hw);
#endif
}

/**
 * rwnx_platform_register_drv() - Register all possible platform drivers
 */
int rwnx_platform_register_drv(void)
{
	return rwnx_pci_register_drv();
}


/**
 * rwnx_platform_unregister_drv() - Unegister all platform drivers
 */
void rwnx_platform_unregister_drv(void)
{
	return rwnx_pci_unregister_drv();
}

struct device *rwnx_platform_get_dev(struct rwnx_plat *rwnx_plat)
{
#ifdef AICWF_SDIO_SUPPORT
	return rwnx_plat->sdiodev->dev;
#endif
#ifdef AICWF_USB_SUPPORT
	return rwnx_plat->usbdev->dev;
#endif
	return &(rwnx_plat->pci_dev->dev);
}


#ifndef CONFIG_RWNX_SDM
MODULE_FIRMWARE(RWNX_AGC_FW_NAME);
MODULE_FIRMWARE(RWNX_FCU_FW_NAME);
MODULE_FIRMWARE(RWNX_LDPC_RAM_NAME);
#endif
MODULE_FIRMWARE(RWNX_MAC_FW_NAME);
#ifndef CONFIG_RWNX_TL4
MODULE_FIRMWARE(RWNX_MAC_FW_NAME2);
#endif


