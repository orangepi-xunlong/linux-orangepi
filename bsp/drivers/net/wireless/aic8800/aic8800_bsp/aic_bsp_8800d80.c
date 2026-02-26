/**
 ******************************************************************************
 *
 * aic_bsp_8800d80.c
 *
 * Copyright (C) RivieraWaves 2014-2019
 *
 ******************************************************************************
 */

#include <linux/list.h>
#include <linux/version.h>
#include <linux/firmware.h>
#include "aic_bsp_txrxif.h"
#include "aicsdio.h"
#include "aic_bsp_driver.h"
#include <linux/mmc/sdio_func.h>

#ifdef CONFIG_AIC_INTF_SDIO
#define RAM_FMAC_FW_ADDR                    0x00120000
#else
#define RAM_FMAC_FW_ADDR                    0x00110000
#endif
#define FW_RAM_ADID_BASE_ADDR_8800D80       0x002017E0
#define FW_RAM_PATCH_BASE_ADDR_8800D80      0x0020B2B0
#define FW_RAM_ADID_BASE_ADDR_8800D80_U02   0x00201940
#define FW_RAM_PATCH_BASE_ADDR_8800D80_U02  0x0020b43c

#define AIC_PATCH_MAGIG_NUM                 0x48435450 // "PTCH"
#define AIC_PATCH_MAGIG_NUM_2               0x50544348 // "HCTP"
#define AIC_PATCH_BLOCK_MAX                 4

#ifdef CONFIG_SDIO_F1_FLAG
bool sdio_f1_flag = false;
#endif

typedef struct {
	uint32_t magic_num;
	uint32_t pair_start;
	uint32_t magic_num_2;
	uint32_t pair_count;
	uint32_t block_dst[AIC_PATCH_BLOCK_MAX];
	uint32_t block_src[AIC_PATCH_BLOCK_MAX];
	uint32_t block_size[AIC_PATCH_BLOCK_MAX]; // word count
} aic_patch_t;

#define AIC_PATCH_OFST(mem) ((size_t) &((aic_patch_t *)0)->mem)
#define AIC_PATCH_ADDR(mem) ((u32)(aic_patch_str_base + AIC_PATCH_OFST(mem)))

#if defined(AICWF_SDIO_SUPPORT)
static u32 patch_tbl[][2] = {
#if defined(CONFIG_AMSDU_RX)
	{0x0170, 0x0100000a},
#endif
#if IS_ENABLED(CONFIG_AIC_IRQ_ACTIVE_LOW) || IS_ENABLED(CONFIG_AIC_IRQ_ACTIVE_FALLING)
	{0x00000170, 0x0000010a}, //wlan_hostwake Low level trigger
#endif
};

static u32 aicbsp_syscfg_tbl[][2] = {
};

static u32 adaptivity_patch_tbl_8800d80[][2] = {
	{0x000C, 0x0000320A}, //linkloss_thd
	{0x009C, 0x00000000}, //ac_param_conf
	{0x0168, 0x00010000}, //tx_adaptivity_en
};

#elif defined(AICWF_USB_SUPPORT)

static u32 patch_tbl[][2] = {
};

static u32 aicbsp_syscfg_tbl[][2] = {
	// Common Settings
	{0x40500014, 0x00000101}, // 1)
	{0x40500018, 0x0000010d}, // 2)
	{0x40500004, 0x00000010}, // 3) the order should not be changed
	// CONFIG_PMIC_SETTING
	{0x50000000, 0x03220204}, // 2) pmic interface init
	{0x50019150, 0x00000002},
	{0x50017008, 0x00000000}, // 4) stop wdg

	// U02 bootrom only
	{0x40040000, 0x00001AC8}, // 1) fix panic
	{0x40040084, 0x00011580},
	{0x40040080, 0x00000001},
	{0x40100058, 0x00000000},
};

//adap test
static u32 adaptivity_patch_tbl_8800d80[][2] = {
	{0x000C, 0x0000320A}, //linkloss_thd
	{0x009C, 0x00000000}, //ac_param_conf
	{0x0154, 0x00010000}, //tx_adaptivity_en
};
#endif

static __attribute__((unused)) u32 syscfg_tbl_masked[][3] = {
/*
	{0x40506024, 0x000000FF, 0x000000DF}, // for clk gate lp_level
*/
};

static u32 rf_tbl_masked[][3] = {
/*
	{0x40344058, 0x00800000, 0x00000000},// pll trx
*/
};

#ifdef CONFIG_OOB
// for 8800d40/d80     map data1 isr to gpiob1
u32 gpio_cfg_tbl_8800d40d80[][2] = {
	{0x40504084, 0x00000006},
	{0x40500040, 0x00000000},
	{0x40100030, 0x00000001},
	{0x40241020, 0x00000001},
	{0x40240030, 0x00000004},
	{0x40240020, 0x03020700},
};
#endif

#if defined(AICWF_SDIO_SUPPORT)
static const struct aicbsp_firmware fw_u01[] = {
	[AICBSP_CPMODE_WORK] = {
		.desc          = "normal work mode(sdio u01)",
		.bt_adid       = "aic8800d80/fw_adid_8800d80.bin",
		.bt_patch      = "aic8800d80/fw_patch_8800d80.bin",
		.bt_table      = "aic8800d80/fw_patch_table_8800d80.bin",
		.wl_fw         = "aic8800d80/fmacfw_8800d80.bin"
	},

	[AICBSP_CPMODE_TEST] = {
		.desc          = "rf test mode(sdio u01)",
		.bt_adid       = "aic8800d80/fw_adid_8800d80.bin",
		.bt_patch      = "aic8800d80/fw_patch_8800d80.bin",
		.bt_table      = "aic8800d80/fw_patch_table_8800d80.bin",
		.wl_fw         = "aic8800d80/lmacfw_rf_8800d80.bin"
	},
};

static const struct aicbsp_firmware fw_u02[] = {
	[AICBSP_CPMODE_WORK] = {
		.desc          = "normal work mode(sdio u02)",
		.bt_adid       = "aic8800d80/fw_adid_8800d80_u02.bin",
		.bt_patch      = "aic8800d80/fw_patch_8800d80_u02.bin",
		.bt_table      = "aic8800d80/fw_patch_table_8800d80_u02.bin",
		.bt_ext_patch  = "aic8800d80/fw_patch_8800d80_u02_ext",
		.wl_fw         = "aic8800d80/fmacfw_8800d80_u02.bin"
	},

	[AICBSP_CPMODE_TEST] = {
		.desc          = "rf test mode(sdio u02)",
		.bt_adid       = "aic8800d80/fw_adid_8800d80_u02.bin",
		.bt_patch      = "aic8800d80/fw_patch_8800d80_u02.bin",
		.bt_table      = "aic8800d80/fw_patch_table_8800d80_u02.bin",
		.bt_ext_patch  = "aic8800d80/fw_patch_8800d80_u02_ext",
		.wl_fw         = "aic8800d80/lmacfw_rf_8800d80_u02.bin"
	},
};

static const struct aicbsp_firmware fw_h_u02[] = {
	[AICBSP_CPMODE_WORK] = {
		.desc          = "normal work mode(sdio u02)",
		.bt_adid       = "aic8800d80/fw_adid_8800d80_u02.bin",
		.bt_patch      = "aic8800d80/fw_patch_8800d80_u02.bin",
		.bt_table      = "aic8800d80/fw_patch_table_8800d80_u02.bin",
		.bt_ext_patch  = "aic8800d80/fw_patch_8800d80_u02_ext",
		.wl_fw         = "aic8800d80/fmacfw_8800d80_h_u02.bin"
	},

	[AICBSP_CPMODE_TEST] = {
		.desc          = "rf test mode(sdio u02)",
		.bt_adid       = "aic8800d80/fw_adid_8800d80_u02.bin",
		.bt_patch      = "aic8800d80/fw_patch_8800d80_u02.bin",
		.bt_table      = "aic8800d80/fw_patch_table_8800d80_u02.bin",
		.bt_ext_patch  = "aic8800d80/fw_patch_8800d80_u02_ext",
		.wl_fw         = "aic8800d80/lmacfw_rf_8800d80_u02.bin"
	},
};

#elif defined(AICWF_USB_SUPPORT)

static const struct aicbsp_firmware fw_u02[] = {
	[AICBSP_CPMODE_WORK] = {
		.desc          = "normal work mode(usb u02)",
		.bt_adid       = "fw_adid.bin",
		.bt_patch      = "fw_patch.bin",
		.bt_table      = "fw_patch_table.bin",
		.wl_fw         = "fmacfw_usb.bin"
	},

	[AICBSP_CPMODE_TEST] = {
		.desc          = "rf test mode(usb u02)",
		.bt_adid       = "fw_adid.bin",
		.bt_patch      = "fw_patch.bin",
		.bt_table      = "fw_patch_table.bin",
		.wl_fw         = "fmacfw_rf_usb.bin"
	},
};

static const struct aicbsp_firmware fw_u03[] = {
	[AICBSP_CPMODE_WORK] = {
		.desc          = "normal work mode(usb u03/u04)",
		.bt_adid       = "fw_adid_u03.bin",
		.bt_patch      = "fw_patch_u03.bin",
		.bt_table      = "fw_patch_table_u03.bin",
		.wl_fw         = "fmacfw_usb.bin"
	},

	[AICBSP_CPMODE_TEST] = {
		.desc          = "rf test mode(usb u03/u04)",
		.bt_adid       = "fw_adid_u03.bin",
		.bt_patch      = "fw_patch_u03.bin",
		.bt_table      = "fw_patch_table_u03.bin",
		.wl_fw         = "fmacfw_rf_usb.bin"
	},
};
#endif

static int aicbt_init(struct priv_dev *aicdev)
{
	int ret = 0;
	struct aicbt_patch_info_t patch_info = {
		.info_len          = 0,
		.adid_addrinf      = 0,
		.addr_adid         = FW_RAM_ADID_BASE_ADDR_8800D80,
		.patch_addrinf     = 0,
		.addr_patch        = FW_RAM_PATCH_BASE_ADDR_8800D80,
		.adid_flag_addr    = 0,
		.adid_flag         = 0,
		.ext_patch_nb_addr = 0,
		.ext_patch_nb      = 0,
	};

	struct aicbt_info_t aicbt_info = {
		.btmode        = AICBT_BTMODE_BT_WIFI_COANT,
		.btport        = AICBT_BTPORT_DEFAULT,
		.uart_baud     = AICBT_UART_BAUD_DEFAULT,
		.uart_flowctrl = AICBT_UART_FC_DEFAULT,
		.lpm_enable    = AICBT_LPM_ENABLE_DEFAULT,
		.txpwr_lvl     = AICBT_TXPWR_LVL_8800D80,
	};

	struct aicbt_patch_table *head = aicbt_patch_table_alloc(aicbsp_firmware_list[aicbsp_info.cpmode].bt_table);
	if (head == NULL) {
		printk("aicbt_patch_table_alloc fail\n");
		return -1;
	}

	aicbsp_driver_btmode_reinit(&aicbt_info);
	aicbsp_driver_lpm_enable_reinit(&aicbt_info);

	if (aicbsp_info.chipinfo->rev == CHIP_REV_ID_U02 || aicbsp_info.chipinfo->rev == CHIP_REV_ID_U03) {
		patch_info.addr_adid = FW_RAM_ADID_BASE_ADDR_8800D80_U02;
		patch_info.addr_patch = FW_RAM_PATCH_BASE_ADDR_8800D80_U02;
	}

	ret = aicbt_patch_info_unpack(head, &patch_info);
	if (ret) {
		pr_warn("%s no patch info found in bt fw\n", __func__);
	}

	if (rwnx_plat_bin_fw_upload_android(aicdev, patch_info.addr_adid, aicbsp_firmware_list[aicbsp_info.cpmode].bt_adid)) {
		return -1;
		goto err;
	}
	if (rwnx_plat_bin_fw_upload_android(aicdev, patch_info.addr_patch, aicbsp_firmware_list[aicbsp_info.cpmode].bt_patch)) {
		return -1;
		goto err;
	}

	if (aicbt_ext_patch_data_load(aicdev, &patch_info)) {
		printk("aicbt_ext_patch_data_load fail\n");
		ret = -1;
		goto err;
	}

	if (aicbt_patch_table_load(aicdev, &aicbt_info, head)) {
		printk("aicbt_patch_table_load fail\n");
		ret = -1;
		goto err;
	}

err:
	aicbt_patch_table_free(&head);
	return ret;
}

static int aicwifi_start_from_bootrom(struct priv_dev *aicdev)
{
	int ret = 0;

	/* memory access */
	const u32 fw_addr = RAM_FMAC_FW_ADDR;
	struct dbg_start_app_cfm start_app_cfm;

	/* fw start */
	ret = rwnx_send_dbg_start_app_req(aicdev, fw_addr, HOST_START_APP_AUTO, &start_app_cfm);
	if (ret) {
		return -1;
	}
	aicbsp_info.hwinfo_r = start_app_cfm.bootstatus & 0xFF;

	return 0;
}

static int aicwifi_sys_config(struct priv_dev *aicdev)
{
	int ret, cnt;
	int syscfg_num;

#ifdef CONFIG_OOB
	int gpiocfg_num = sizeof(gpio_cfg_tbl_8800d40d80) / sizeof(u32) / 2;
	for (cnt = 0; cnt < gpiocfg_num; cnt++) {
		ret = rwnx_send_dbg_mem_write_req(aicdev, gpio_cfg_tbl_8800d40d80[cnt][0], gpio_cfg_tbl_8800d40d80[cnt][1]);
		if (ret) {
			printk("%x write fail: %d\n", gpio_cfg_tbl_8800d40d80[cnt][0], ret);
			return ret;
		}
	}
#endif

#ifdef AICWF_SDIO_SUPPORT
	syscfg_num = sizeof(syscfg_tbl_masked) / sizeof(u32) / 3;
	for (cnt = 0; cnt < syscfg_num; cnt++) {
		ret = rwnx_send_dbg_mem_mask_write_req(aicdev,
			syscfg_tbl_masked[cnt][0], syscfg_tbl_masked[cnt][1], syscfg_tbl_masked[cnt][2]);
		if (ret) {
			printk("%x mask write fail: %d\n", syscfg_tbl_masked[cnt][0], ret);
			return ret;
		}
	}
#endif

	syscfg_num = sizeof(rf_tbl_masked) / sizeof(u32) / 3;
	for (cnt = 0; cnt < syscfg_num; cnt++) {
		ret = rwnx_send_dbg_mem_mask_write_req(aicdev,
			rf_tbl_masked[cnt][0], rf_tbl_masked[cnt][1], rf_tbl_masked[cnt][2]);
		if (ret) {
			printk("rf config %x write fail: %d\n", rf_tbl_masked[cnt][0], ret);
			return ret;
		}
	}

	return 0;
}

#define NEW_PATCH_BUFFER_MAP    1
static int aicwifi_patch_config(struct priv_dev *aicdev)
{
	const u32 rd_patch_addr = RAM_FMAC_FW_ADDR + 0x0198;
	u32 aic_patch_addr;
	u32 config_base, aic_patch_str_base;
	#if (NEW_PATCH_BUFFER_MAP)
	u32 patch_buff_addr, patch_buff_base, rd_version_addr, rd_version_val;
	#endif
#if defined(AICWF_USB_SUPPORT)
	uint32_t start_addr = 0x001D7000;
#else
	uint32_t start_addr = 0x0016F800;
#endif
	u32 patch_addr = start_addr;
	u32 patch_cnt = sizeof(patch_tbl)/sizeof(u32)/2;
	struct dbg_mem_read_cfm rd_patch_addr_cfm;
	int ret = 0;
	u16 cnt = 0;
	//adap test
	int adap_patch_cnt = 0;

	if (aicbsp_info.adap_test == 1) {
		adap_patch_cnt = sizeof(adaptivity_patch_tbl_8800d80)/sizeof(u32)/2;
	}

	aic_patch_addr = rd_patch_addr + 8;

	ret = rwnx_send_dbg_mem_read_req(aicdev, rd_patch_addr, &rd_patch_addr_cfm);
	if (ret) {
		printk("patch rd fail\n");
		return ret;
	}

	config_base = rd_patch_addr_cfm.memdata;

	ret = rwnx_send_dbg_mem_read_req(aicdev, aic_patch_addr, &rd_patch_addr_cfm);
	if (ret) {
		printk("patch str rd fail\n");
		return ret;
	}

	aic_patch_str_base = rd_patch_addr_cfm.memdata;

	#if (NEW_PATCH_BUFFER_MAP)
	rd_version_addr = RAM_FMAC_FW_ADDR + 0x01C;
	ret = rwnx_send_dbg_mem_read_req(aicdev, rd_version_addr, &rd_patch_addr_cfm);
	if (ret) {
		printk("version val[0x%x] rd fail: %d\n", rd_version_addr, ret);
		return ret;
	}
	rd_version_val = rd_patch_addr_cfm.memdata;
	printk("rd_version_val=%08X\n", rd_version_val);
	aicdev->fw_version_uint = rd_version_val;
	if (rd_version_val > 0x06090100) {
		patch_buff_addr = rd_patch_addr + 12;
		ret = rwnx_send_dbg_mem_read_req(aicdev, patch_buff_addr, &rd_patch_addr_cfm);
		if (ret) {
			printk("patch buf rd fail\n");
			return ret;
		}
		patch_buff_base = rd_patch_addr_cfm.memdata;
		patch_addr = start_addr = patch_buff_base;
	}
	#endif

	ret = rwnx_send_dbg_mem_write_req(aicdev, AIC_PATCH_ADDR(magic_num), AIC_PATCH_MAGIG_NUM);
	if (ret) {
		printk("0x%x write fail\n", AIC_PATCH_ADDR(magic_num));
		return ret;
	}

	ret = rwnx_send_dbg_mem_write_req(aicdev, AIC_PATCH_ADDR(magic_num_2), AIC_PATCH_MAGIG_NUM_2);
	if (ret) {
		printk("0x%x write fail\n", AIC_PATCH_ADDR(magic_num_2));
		return ret;
	}

	ret = rwnx_send_dbg_mem_write_req(aicdev, AIC_PATCH_ADDR(pair_start), patch_addr);
	if (ret) {
		printk("0x%x write fail\n", AIC_PATCH_ADDR(pair_start));
		return ret;
	}

	ret = rwnx_send_dbg_mem_write_req(aicdev, AIC_PATCH_ADDR(pair_count), patch_cnt + adap_patch_cnt);
	if (ret) {
		printk("0x%x write fail\n", AIC_PATCH_ADDR(pair_count));
		return ret;
	}

	for (cnt = 0; cnt < patch_cnt; cnt++) {
		ret = rwnx_send_dbg_mem_write_req(aicdev, start_addr+8*cnt, patch_tbl[cnt][0]+config_base);
		if (ret) {
			printk("%x write fail\n", start_addr+8*cnt);
			return ret;
		}

		ret = rwnx_send_dbg_mem_write_req(aicdev, start_addr+8*cnt+4, patch_tbl[cnt][1]);
		if (ret) {
			printk("%x write fail\n", start_addr+8*cnt+4);
			return ret;
		}
	}

	if (aicbsp_info.adap_test == 1) {
		int tmp_cnt = patch_cnt + adap_patch_cnt;

		for (cnt = patch_cnt; cnt < tmp_cnt; cnt++) {
			int tbl_idx = cnt - patch_cnt;

			ret = rwnx_send_dbg_mem_write_req(aicdev, start_addr+8*cnt, adaptivity_patch_tbl_8800d80[tbl_idx][0]+config_base);
			if (ret) {
				printk("%x write fail\n", start_addr+8*cnt);
				return ret;
			}
			ret = rwnx_send_dbg_mem_write_req(aicdev, start_addr+8*cnt+4, adaptivity_patch_tbl_8800d80[tbl_idx][1]);
			if (ret) {
				printk("%x write fail\n", start_addr+8*cnt+4);
				return ret;
			}
		}
	}

	ret = rwnx_send_dbg_mem_write_req(aicdev, AIC_PATCH_ADDR(block_size[0]), 0);
	if (ret) {
		printk("block_size[0x%x] write fail: %d\n", AIC_PATCH_ADDR(block_size[0]), ret);
		return ret;
	}
	ret = rwnx_send_dbg_mem_write_req(aicdev, AIC_PATCH_ADDR(block_size[1]), 0);
	if (ret) {
		printk("block_size[0x%x] write fail: %d\n", AIC_PATCH_ADDR(block_size[1]), ret);
		return ret;
	}
	ret = rwnx_send_dbg_mem_write_req(aicdev, AIC_PATCH_ADDR(block_size[2]), 0);
	if (ret) {
		printk("block_size[0x%x] write fail: %d\n", AIC_PATCH_ADDR(block_size[2]), ret);
		return ret;
	}
	ret = rwnx_send_dbg_mem_write_req(aicdev, AIC_PATCH_ADDR(block_size[3]), 0);
	if (ret) {
		printk("block_size[0x%x] write fail: %d\n", AIC_PATCH_ADDR(block_size[3]), ret);
		return ret;
	}

	return 0;
}

static int aicwifi_init(struct priv_dev *aicdev)
{
	if (rwnx_plat_bin_fw_upload_android(aicdev, RAM_FMAC_FW_ADDR, aicbsp_firmware_list[aicbsp_info.cpmode].wl_fw)) {
		printk("download wifi fw fail\n");
		return -1;
	}

	if (aicwifi_patch_config(aicdev)) {
		printk("aicwifi_patch_config fail\n");
		return -1;
	}

	if (aicwifi_sys_config(aicdev)) {
		printk("aicwifi_sys_config fail\n");
		return -1;
	}

	if (aicwifi_start_from_bootrom(aicdev)) {
		printk("wifi start fail\n");
		return -1;
	}

	return 0;
}

static int aicbsp_system_config(struct priv_dev *aicdev)
{
	int syscfg_num = 0;
	int ret, cnt;
	u32 (*cfg_tbl)[2];

	syscfg_num = sizeof(aicbsp_syscfg_tbl) / sizeof(aicbsp_syscfg_tbl[0]);
	cfg_tbl = aicbsp_syscfg_tbl;

	for (cnt = 0; cnt < syscfg_num; cnt++) {
		ret = rwnx_send_dbg_mem_write_req(aicdev, cfg_tbl[cnt][0], cfg_tbl[cnt][1]);
		if (ret) {
			bsp_err("%x write fail: %d\n", cfg_tbl[cnt][0], ret);
			return ret;
		}
	}
	return 0;
}

int aicbsp_8800d80_fw_init(struct priv_dev *aicdev)
{
	const u32 mem_addr = 0x40500000;
	struct dbg_mem_read_cfm rd_mem_addr_cfm;

	uint8_t binding_status;
	uint8_t dout[16];
#ifdef AICWF_SDIO_SUPPORT
	int ret = 0;
#endif
	need_binding_verify = false;

#ifdef AICWF_SDIO_SUPPORT
	if (rwnx_send_dbg_mem_write_req(aicdev, 0x40500058, 0x40))
		return -1;
#ifdef CONFIG_SDIO_F1_FLAG
	printk("aicbsp:sdio_f1_flag %d\n",sdio_f1_flag);
	if (sdio_f1_flag) {
#endif
		sdio_claim_host(aicdev->func[0]);
		sdio_f0_writeb(aicdev->func[0], 0x0, 0xF1, &ret);
		if (ret) {
			bsp_err("set iopad delay1 fail %d\n", ret);
			sdio_release_host(aicdev->func[0]);
			return ret;
		}
		msleep(1);
		sdio_release_host(aicdev->func[0]);
#ifdef CONFIG_SDIO_F1_FLAG
	}
#endif
#endif

	if (rwnx_send_dbg_mem_read_req(aicdev, mem_addr, &rd_mem_addr_cfm))
		return -1;

	aicbsp_info.chipinfo->rev = (u8)((rd_mem_addr_cfm.memdata >> 16) & 0x3F);
	aicbsp_info.chipinfo->is_chip_id_h = (u8)IS_CHIP_ID_H(rd_mem_addr_cfm.memdata >> 16);

	if (rwnx_send_dbg_mem_read_req(aicdev, 0x00000004, &rd_mem_addr_cfm))
		return -1;

	aicbsp_info.chipinfo->subrev = (u8)(rd_mem_addr_cfm.memdata >> 4);

	if (aicbsp_info.chipinfo->rev != CHIP_REV_ID_U01 &&
		aicbsp_info.chipinfo->rev != CHIP_REV_ID_U02 &&
		aicbsp_info.chipinfo->rev != CHIP_REV_ID_U03) {
		pr_err("aicbsp: %s, unsupport chip rev id: 0x%x\n", __func__, aicbsp_info.chipinfo->rev);
		return -1;
	}

	printk("aicbsp: %s, rev id: 0x%x, subrev id: 0x%x, is_chip_id_h: %d\n", __func__, aicbsp_info.chipinfo->rev, aicbsp_info.chipinfo->subrev, aicbsp_info.chipinfo->is_chip_id_h);

#if defined(AICWF_SDIO_SUPPORT)
	if (aicbsp_info.chipinfo->is_chip_id_h) {
		aicbsp_firmware_list = fw_h_u02;
	} else {
		if (aicbsp_info.chipinfo->rev == CHIP_REV_ID_U01)
			aicbsp_firmware_list = fw_u01;
		else if (aicbsp_info.chipinfo->rev == CHIP_REV_ID_U02 || aicbsp_info.chipinfo->rev == CHIP_REV_ID_U03)
			aicbsp_firmware_list = fw_u02;
	}
#elif defined(AICWF_USB_SUPPORT)
	if (aicbsp_info.chipinfo->rev == CHIP_REV_ID_U02)
		 aicbsp_firmware_list = fw_u02;
	else if (aicbsp_info.chipinfo->rev == CHIP_REV_ID_U03)
		aicbsp_firmware_list = fw_u03;
#endif

	if (aicbsp_system_config(aicdev))
		return -1;

	if (aicbt_init(aicdev))
		return -1;

	if (aicwifi_init(aicdev))
		return -1;

	if (need_binding_verify) {
		printk("aicbsp: crypto data %02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X\n",
						binding_enc_data[0],  binding_enc_data[1],  binding_enc_data[2],  binding_enc_data[3],
						binding_enc_data[4],  binding_enc_data[5],  binding_enc_data[6],  binding_enc_data[7],
						binding_enc_data[8],  binding_enc_data[9],  binding_enc_data[10], binding_enc_data[11],
						binding_enc_data[12], binding_enc_data[13], binding_enc_data[14], binding_enc_data[15]);

		/* calculate verify data from crypto data */
		if (wcn_bind_verify_calculate_verify_data(binding_enc_data, dout)) {
			pr_err("aicbsp: %s, binding encrypt data incorrect\n", __func__);
			return -1;
		}

		printk("aicbsp: verify data %02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X\n",
						dout[0],  dout[1],  dout[2],  dout[3],
						dout[4],  dout[5],  dout[6],  dout[7],
						dout[8],  dout[9],  dout[10], dout[11],
						dout[12], dout[13], dout[14], dout[15]);

		if (rwnx_send_dbg_binding_req(aicdev, dout, &binding_status)) {
			pr_err("aicbsp: %s, send binding request failn", __func__);
			return -1;
		}

		if (binding_status) {
			pr_err("aicbsp: %s, binding verify fail\n", __func__);
			return -1;
		}
	}

#ifdef AICWF_SDIO_SUPPORT
#if defined(CONFIG_SDIO_PWRCTRL)
	if (aicwf_sdio_writeb(aicdev->func[0], aicdev->sdio_reg.wakeup_reg, 4)) {
		bsp_err("reg:%d write failed!\n", aicdev->sdio_reg.wakeup_reg);
		return -1;
	}
#endif
#endif

	return 0;
}

