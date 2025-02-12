/**
 ******************************************************************************
 *
 * aic_bsp_8800d.c
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

#ifdef CONFIG_AIC_INTF_SDIO
#define RAM_FMAC_FW_ADDR            0x00120000
#define RAM_FMAC_FW_PATCH_ADDR      0x00190000
#else
#define RAM_FMAC_FW_ADDR            0x00110000
#endif
#define FW_RAM_ADID_BASE_ADDR       0x00161928
#define FW_RAM_ADID_BASE_ADDR_U03   0x00161928
#define FW_RAM_PATCH_BASE_ADDR      0x00100000

#if defined(AICWF_SDIO_SUPPORT)
static u32 patch_tbl[][2] = {
};

static u32 aicbsp_syscfg_tbl[][2] = {
	{0x40500014, 0x00000101}, // 1)
	{0x40500018, 0x00000109}, // 2)
	{0x40500004, 0x00000010}, // 3) the order should not be changed

	// def CONFIG_PMIC_SETTING
	// U02 bootrom only
	{0x40040000, 0x00001AC8}, // 1) fix panic
	{0x40040084, 0x00011580},
	{0x40040080, 0x00000001},
	{0x40100058, 0x00000000},

	{0x50000000, 0x03220204}, // 2) pmic interface init
	{0x50019150, 0x00000002}, // 3) for 26m xtal, set div1
	{0x50017008, 0x00000000}, // 4) stop wdg
};

static u32 adaptivity_patch_tbl[][2] = {
	{0x0004, 0x0000320A}, //linkloss_thd
	{0x0094, 0x00000000}, //ac_param_conf
	{0x00F8, 0x00010138}, //tx_adaptivity_en
};
#elif defined(AICWF_USB_SUPPORT)

static u32 patch_tbl[][2] = {
	{0x0044, 0x00000002}, //hosttype
	{0x0048, 0x00000060},
#ifdef CONFIG_USB_MSG_EP
	{0x004c, 0x00040050},
#else
	{0x004c, 0x00000052},
#endif
	{0x0050, 0x00000000}, //ipc base
	{0x0054, 0x00190000}, //buf base
	{0x0058, 0x00190140}, //desc base
	{0x005c, 0x00000ee0}, //desc size
	{0x0060, 0x00191020}, //pkt base
	{0x0064, 0x0002efe0}, //pkt size
	{0x0068, 0x00000008}, //tx BK A-MPDU 8
	{0x006c, 0x00000040}, //tx BE A-MPDU 40
	{0x0070, 0x00000040}, //tx VI A-MPDU 40
	{0x0074, 0x00000020}, //tx VO A-MPDU 20
	{0x0078, 0x00000000}, //bcn queue
	{0x007c, 0x00000020}, //rx A-MPDU 20
	{0x0080, 0x001d0000},
	{0x0084, 0x0000fc00}, //63kB
	{0x0088, 0x001dfc00},
	{0x00a8, 0x8d080103}, //dm 8D08
	{0x00d0, 0x00010103}, //aon sram
#ifdef CONFIG_USB_MSG_EP
	{0x00d4, 0x0404087c},
#else
	{0x00d4, 0x0000087c},
#endif
	{0x00d8, 0x001c0000}, //bt base
	{0x00dc, 0x00008000}, //bt size
	{0x00e0, 0x04020a08},
	{0x00e4, 0x00000001},
#ifdef CONFIG_USB_MSG_EP
	{0x00fc, 0x00000302}, //rx msg fc pkt cnt
#endif
};

static u32 aicbsp_syscfg_tbl_u02[][2] = {
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

static u32 aicbsp_syscfg_tbl_u03[][2] = {
	// Common Settings
	{0x40500014, 0x00000101}, // 1)
	{0x40500018, 0x0000010d}, // 2)
	{0x40500004, 0x00000010}, // 3) the order should not be changed
	//CONFIG_PMIC_SETTING
	{0x50000000, 0x03220204}, // 2) pmic interface init
	{0x50019150, 0x00000002},
	{0x50017008, 0x00000000}, // 4) stop wdg
};

static u32 aicbsp_syscfg_tbl_u04[][2] = {
	// Common Settings
	{0x40500014, 0x00000101}, // 1)
	{0x40500018, 0x0000010d}, // 2)
	{0x40500004, 0x00000010}, // 3) the order should not be changed
	//CONFIG_PMIC_SETTING
	{0x50000000, 0x03220204}, // 2) pmic interface init
	{0x50019150, 0x00000002},
	{0x50017008, 0x00000000}, // 4) stop wdg

	// U04 bootrom only
	{0x40040000, 0x0000042C}, // protect usb replenish rxq / flush rxq, skip flush rxq before start_app
	{0x40040004, 0x0000DD44},
	{0x40040008, 0x00000448},
	{0x4004000C, 0x0000044C},
	{0x0019B800, 0xB9F0F19B},
	{0x0019B804, 0x0019B81D},
	{0x0019B808, 0xBF00FA79},
	{0x0019B80C, 0xF007BF00},
	{0x0019B810, 0x4605B672}, // code
	{0x0019B814, 0x21E0F04F},
	{0x0019B818, 0xBE0BF664},
	{0x0019B81C, 0xF665B510},
	{0x0019B820, 0x4804FC9D},
	{0x0019B824, 0xFA9EF66C},
	{0x0019B828, 0xFCA8F665},
	{0x0019B82C, 0x4010E8BD},
	{0x0019B830, 0xBAC6F66C},
	{0x0019B834, 0x0019A0C4},
	{0x40040084, 0x0019B800}, // out base
	{0x40040080, 0x0000000F},
	{0x40100058, 0x00000000},
};

static u32 adaptivity_patch_tbl[][2] = {
	{0x0004, 0x0000320A}, //linkloss_thd
	{0x0094, 0x00000000}, //ac_param_conf
	{0x00F8, 0x00010138}, //tx_adaptivity_en
};
#endif

static __attribute__((unused)) u32 syscfg_tbl_masked[][3] = {
	{0x40506024, 0x000000FF, 0x000000DF}, // for clk gate lp_level
};

static u32 rf_tbl_masked[][3] = {
	{0x40344058, 0x00800000, 0x00000000},// pll trx
};

#if defined(AICWF_SDIO_SUPPORT)
static const struct aicbsp_firmware fw_u02[] = {
	[AICBSP_CPMODE_WORK] = {
		.desc          = "normal work mode(sdio u02)",
		.bt_adid       = "aic/sdio/fw_adid.bin",
		.bt_patch      = "aic/sdio/fw_patch.bin",
		.bt_table      = "aic/sdio/fw_patch_table.bin",
		.wl_fw         = "aic/sdio/fmacfw.bin",
		.wl_table      = "aic/sdio/fmacfw_patch.bin"
	},

	[AICBSP_CPMODE_TEST] = {
		.desc          = "rf test mode(sdio u02)",
		.bt_adid       = "aic/sdio/fw_adid.bin",
		.bt_patch      = "aic/sdio/fw_patch.bin",
		.bt_table      = "aic/sdio/fw_patch_table.bin",
		.wl_fw         = "aic/sdio/fmacfw_rf.bin"
	},
};

static const struct aicbsp_firmware fw_u03[] = {
	[AICBSP_CPMODE_WORK] = {
		.desc          = "normal work mode(sdio u03/u04)",
		.bt_adid       = "aic/sdio/fw_adid_u03.bin",
		.bt_patch      = "aic/sdio/fw_patch_u03.bin",
		.bt_table      = "aic/sdio/fw_patch_table_u03.bin",
		.wl_fw         = "aic/sdio/fmacfw.bin",
		.wl_table      = "aic/sdio/fmacfw_patch.bin"
	},

	[AICBSP_CPMODE_TEST] = {
		.desc          = "rf test mode(sdio u03/u04)",
		.bt_adid       = "aic/sdio/fw_adid_u03.bin",
		.bt_patch      = "aic/sdio/fw_patch_u03.bin",
		.bt_table      = "aic/sdio/fw_patch_table_u03.bin",
		.wl_fw         = "aic/sdio/fmacfw_rf.bin"
	},
};

#elif defined(AICWF_USB_SUPPORT)

static const struct aicbsp_firmware fw_u02[] = {
	[AICBSP_CPMODE_WORK] = {
		.desc          = "normal work mode(usb u02)",
		.bt_adid       = "aic/usb/fw_adid.bin",
		.bt_patch      = "aic/usb/fw_patch.bin",
		.bt_table      = "aic/usb/fw_patch_table.bin",
		.wl_fw         = "aic/usb/fmacfw.bin"
	},

	[AICBSP_CPMODE_TEST] = {
		.desc          = "rf test mode(usb u02)",
		.bt_adid       = "aic/usb/fw_adid.bin",
		.bt_patch      = "aic/usb/fw_patch.bin",
		.bt_table      = "aic/usb/fw_patch_table.bin",
		.wl_fw         = "aic/usb/fmacfw_rf.bin"
	},
};

static const struct aicbsp_firmware fw_u03[] = {
	[AICBSP_CPMODE_WORK] = {
		.desc          = "normal work mode(usb u03/u04)",
		.bt_adid       = "aic/usb/fw_adid_u03.bin",
		.bt_patch      = "aic/usb/fw_patch_u03.bin",
		.bt_table      = "aic/usb/fw_patch_table_u03.bin",
		.wl_fw         = "aic/usb/fmacfw.bin"
	},

	[AICBSP_CPMODE_TEST] = {
		.desc          = "rf test mode(usb u03/u04)",
		.bt_adid       = "aic/usb/fw_adid_u03.bin",
		.bt_patch      = "aic/usb/fw_patch_u03.bin",
		.bt_table      = "aic/usb/fw_patch_table_u03.bin",
		.wl_fw         = "aic/usb/fmacfw_rf.bin"
	},
};
#endif

static int aicbt_init(struct priv_dev *aicdev)
{
	int ret = 0;
	struct aicbt_patch_info_t patch_info = {
		.info_len          = 0,
		.adid_addrinf      = 0,
		.addr_adid         = FW_RAM_ADID_BASE_ADDR,
		.patch_addrinf     = 0,
		.addr_patch        = FW_RAM_PATCH_BASE_ADDR,
		.adid_flag_addr    = 0,
		.adid_flag         = 0,
	};

	struct aicbt_info_t aicbt_info = {
		.btmode        = AICBT_BTMODE_DEFAULT,
		.btport        = AICBT_BTPORT_DEFAULT,
		.uart_baud     = AICBT_UART_BAUD_DEFAULT,
		.uart_flowctrl = AICBT_UART_FC_DEFAULT,
		.lpm_enable    = AICBT_LPM_ENABLE_DEFAULT,
		.txpwr_lvl     = AICBT_TXPWR_LVL_DEFAULT,
	};

	struct aicbt_patch_table *head = aicbt_patch_table_alloc(aicbsp_firmware_list[aicbsp_info.cpmode].bt_table);
	if (head == NULL) {
		printk("aicbt_patch_table_alloc fail\n");
		return -1;
	}

	aicbsp_driver_btmode_reinit(&aicbt_info);

	ret = aicbt_patch_info_unpack(head, &patch_info);
	if (ret) {
		pr_warn("%s no patch info found in bt fw\n", __func__);
	}

	ret = rwnx_plat_bin_fw_upload_android(aicdev, patch_info.addr_adid, aicbsp_firmware_list[aicbsp_info.cpmode].bt_adid);
	if (ret)
		goto err;

	ret = rwnx_plat_bin_fw_upload_android(aicdev, patch_info.addr_patch, aicbsp_firmware_list[aicbsp_info.cpmode].bt_patch);
	if (ret)
		goto err;

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
			rf_tbl_masked[0][0], rf_tbl_masked[0][1], rf_tbl_masked[0][2]);
		if (ret) {
			printk("rf config %x write fail: %d\n", rf_tbl_masked[0][0], ret);
			return ret;
		}
	}

	return 0;
}

static int aicwifi_patch_config(struct priv_dev *aicdev)
{
	const u32 rd_patch_addr = RAM_FMAC_FW_ADDR + 0x0180;
	u32 config_base;
	uint32_t start_addr = 0x1e6000;
	u32 patch_addr = start_addr;
	u32 patch_num = sizeof(patch_tbl)/4;
	struct dbg_mem_read_cfm rd_patch_addr_cfm;
	int ret = 0;
	u16 cnt = 0;
#if defined(AICWF_USB_SUPPORT)
	u32 patch_addr_reg = 0x1e5318;
	u32 patch_num_reg = 0x1e531c;
#else
	u32 patch_addr_reg = 0x1e5318;
	u32 patch_num_reg = 0x1e531c;
#endif
	int tmp_cnt = 0;
	int adap_patch_num = 0;

#if defined(AICWF_SDIO_SUPPORT)
	if (aicbsp_info.cpmode == AICBSP_CPMODE_TEST) {
		patch_addr_reg = 0x1e5304;
		patch_num_reg = 0x1e5308;
	}
#endif

	ret = rwnx_send_dbg_mem_read_req(aicdev, rd_patch_addr, &rd_patch_addr_cfm);
	if (ret) {
		printk("patch rd fail\n");
		return ret;
	}

	config_base = rd_patch_addr_cfm.memdata;

	ret = rwnx_send_dbg_mem_write_req(aicdev, patch_addr_reg, patch_addr);
	if (ret) {
		printk("0x%x write fail\n", patch_addr_reg);
		return ret;
	}
	if (aicbsp_info.adap_test == 1) {
		printk("%s for adaptivity test \r\n", __func__);
		adap_patch_num = sizeof(adaptivity_patch_tbl)/4;
		ret = rwnx_send_dbg_mem_write_req(aicdev, patch_num_reg, patch_num + adap_patch_num);
	} else {
		ret = rwnx_send_dbg_mem_write_req(aicdev, patch_num_reg, patch_num);
	}
	if (ret) {
		printk("0x%x write fail\n", patch_num_reg);
		return ret;
	}

	for (cnt = 0; cnt < patch_num/2; cnt += 1) {
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

	tmp_cnt = cnt;
	if (aicbsp_info.adap_test == 1) {
		for (cnt = 0; cnt < adap_patch_num/2; cnt += 1) {
			ret = rwnx_send_dbg_mem_write_req(aicdev, start_addr+8*(cnt+tmp_cnt), adaptivity_patch_tbl[cnt][0]+config_base);
			if (ret) {
				printk("%x write fail\n", start_addr+8*cnt);
			}
			ret = rwnx_send_dbg_mem_write_req(aicdev, start_addr+8*(cnt+tmp_cnt)+4, adaptivity_patch_tbl[cnt][1]);
			if (ret) {
				printk("%x write fail\n", start_addr+8*cnt+4);
			}
		}
	}

	return 0;
}

static int aicwifi_init(struct priv_dev *aicdev)
{
	if (rwnx_plat_bin_fw_upload_android(aicdev, RAM_FMAC_FW_ADDR, aicbsp_firmware_list[aicbsp_info.cpmode].wl_fw)) {
		printk("download wifi fw fail\n");
		return -1;
	}

#if defined(AICWF_SDIO_SUPPORT)
	if (aicbsp_info.cpmode == AICBSP_CPMODE_WORK) {
		if (rwnx_plat_bin_fw_upload_android(aicdev, RAM_FMAC_FW_PATCH_ADDR, aicbsp_firmware_list[aicbsp_info.cpmode].wl_table)) {
			printk("download wifi fw patch fail\n");
			return -1;
		}
	}
#endif

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
#if defined(AICWF_SDIO_SUPPORT)
	syscfg_num = sizeof(aicbsp_syscfg_tbl) / sizeof(aicbsp_syscfg_tbl[0]);
	cfg_tbl = aicbsp_syscfg_tbl;
#elif defined(AICWF_USB_SUPPORT)
	if (aicbsp_info.chipinfo->rev == CHIP_REV_ID_U02) {
		syscfg_num = sizeof(aicbsp_syscfg_tbl_u02) / sizeof(aicbsp_syscfg_tbl_u02[0]);
		cfg_tbl = aicbsp_syscfg_tbl_u02;
	} else if (aicbsp_info.chipinfo->rev == CHIP_REV_ID_U03) {
		if (aicbsp_info.chipinfo->subrev == AIC8800D_CHIP_SUBREV_ID_U04) {
			syscfg_num = sizeof(aicbsp_syscfg_tbl_u04) / sizeof(aicbsp_syscfg_tbl_u04[0]);
			cfg_tbl = aicbsp_syscfg_tbl_u04;
		} else {
			syscfg_num = sizeof(aicbsp_syscfg_tbl_u03) / sizeof(aicbsp_syscfg_tbl_u03[0]);
			cfg_tbl = aicbsp_syscfg_tbl_u03;
		}
	}
#endif
	for (cnt = 0; cnt < syscfg_num; cnt++) {
		ret = rwnx_send_dbg_mem_write_req(aicdev, cfg_tbl[cnt][0], cfg_tbl[cnt][1]);
		if (ret) {
			bsp_err("%x write fail: %d\n", cfg_tbl[cnt][0], ret);
			return ret;
		}
	}
	return 0;
}

int aicbsp_8800d_fw_init(struct priv_dev *aicdev)
{
	const u32 mem_addr = 0x40500000;
	struct dbg_mem_read_cfm rd_mem_addr_cfm;

	uint8_t binding_status;
	uint8_t dout[16];

	need_binding_verify = false;
	aicbsp_firmware_list = fw_u02;

	if (rwnx_send_dbg_mem_read_req(aicdev, mem_addr, &rd_mem_addr_cfm))
		return -1;

	aicbsp_info.chipinfo->rev = (u8)(rd_mem_addr_cfm.memdata >> 16);

	if (rwnx_send_dbg_mem_read_req(aicdev, 0x00000004, &rd_mem_addr_cfm))
		return -1;

	aicbsp_info.chipinfo->subrev = (u8)(rd_mem_addr_cfm.memdata >> 4);

	if (aicbsp_info.chipinfo->rev != CHIP_REV_ID_U02 &&
		aicbsp_info.chipinfo->rev != CHIP_REV_ID_U03 &&
		aicbsp_info.chipinfo->rev != CHIP_REV_ID_U04) {
		pr_err("aicbsp: %s, unsupport chip rev id: 0x%x\n", __func__, aicbsp_info.chipinfo->rev);
		return -1;
	}

	printk("aicbsp: %s, rev id: 0x%x, subrev id: 0x%x\n", __func__, aicbsp_info.chipinfo->rev, aicbsp_info.chipinfo->subrev);

	if (aicbsp_info.chipinfo->rev != CHIP_REV_ID_U02)
		aicbsp_firmware_list = fw_u03;

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

