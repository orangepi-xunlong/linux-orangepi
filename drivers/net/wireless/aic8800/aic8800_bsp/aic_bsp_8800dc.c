/**
 ******************************************************************************
 *
 * aic_bsp_8800dc.c
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

#define RAM_LMAC_FW_ADDR                   0x00150000
#define RAM_FMAC_FW_ADDR                   0x00120000
#define ROM_FMAC_PATCH_ADDR                0x00180000
#define RAM_8800DC_U01_ADID_ADDR           0x00101788
#define RAM_8800DC_U02_ADID_ADDR           0x001017d8
#define RAM_8800DC_FW_PATCH_ADDR           0x00184000
#define FW_RESET_START_ADDR                0x40500128
#define FW_RESET_START_VAL                 0x40
#define FW_ADID_FLAG_ADDR                  0x40500150
#define FW_ADID_FLAG_VAL                   0x01
#define AICBSP_FW_PATH_8800DC              "/vendor/etc/firmware/aic8800dc"

static u32 syscfg_tbl_masked_8800dc[][3] = {
	//#ifdef CONFIG_PMIC_SETTING
#if defined(CONFIG_VRF_DCDC_MODE)
	{0x7000216C, (0x3 << 2), (0x1 << 2)}, // pmic_pmu_init
	{0x700021BC, (0x3 << 2), (0x1 << 2)},
	{0x70002118, ((0x7 << 4) | (0x1 << 7)), ((0x2 << 4) | (0x1 << 7))},
	{0x70002104, ((0x3F << 0) | (0x1 << 6)), ((0x2 << 0) | (0x1 << 6))},
	{0x7000210C, ((0x3F << 0) | (0x1 << 6)), ((0x2 << 0) | (0x1 << 6))},
	{0x70002170, (0xF << 0), (0x1 << 0)},
	{0x70002190, (0x3F << 0), (24 << 0)},
	{0x700021CC, ((0x7 << 4) | (0x1 << 7)), ((0x0 << 4) | (0x0 << 7))},
	{0x700010A0, (0x1 << 11), (0x1 << 11)},
	{0x70001034, ((0x1 << 20) | (0x7 << 26)), ((0x0 << 20) | (0x2 << 26))},
	{0x70001038, (0x1 << 8), (0x1 << 8)},
	{0x70001094, (0x3 << 2), (0x0 << 2)},
	{0x700021D0, ((0x1 << 5) | (0x1 << 6)), ((0x1 << 5) | (0x1 << 6))},
	{0x70001000, ((0x1 << 0) | (0x1 << 20) | (0x1 << 22)),
				 ((0x1 << 0) | (0x1 << 20) | (0x0 << 22))},
	{0x70001028, (0xf << 2), (0x1 << 2)},
#else
	{0x7000216C, (0x3 << 2), (0x1 << 2)}, // pmic_pmu_init
	{0x700021BC, (0x3 << 2), (0x1 << 2)},
	{0x70002118, ((0x7 << 4) | (0x1 << 7)), ((0x2 << 4) | (0x1 << 7))},
	{0x70002104, ((0x3F << 0) | (0x1 << 6)), ((0x2 << 0) | (0x1 << 6))},
	{0x7000210C, ((0x3F << 0) | (0x1 << 6)), ((0x2 << 0) | (0x1 << 6))},
	{0x70002170, (0xF << 0), (0x1 << 0)},
	{0x70002190, (0x3F << 0), (24 << 0)},
	{0x700021CC, ((0x7 << 4) | (0x1 << 7)), ((0x0 << 4) | (0x0 << 7))},
	{0x700010A0, (0x1 << 11), (0x1 << 11)},
	{0x70001034, ((0x1 << 20) | (0x7 << 26)), ((0x0 << 20) | (0x2 << 26))},
	{0x70001038, (0x1 << 8), (0x1 << 8)},
	{0x70001094, (0x3 << 2), (0x0 << 2)},
	{0x700021D0, ((0x1 << 5) | (0x1 << 6)), ((0x1 << 5) | (0x1 << 6))},
	{0x70001000, ((0x1 << 0) | (0x1 << 20) | (0x1 << 22)),
				 ((0x0 << 0) | (0x1 << 20) | (0x0 << 22))},
	{0x70001028, (0xf << 2), (0x1 << 2)},
#endif
	//#endif /* CONFIG_PMIC_SETTING */
	{0x00000000, 0x00000000, 0x00000000}, // last one
};

static u32 syscfg_tbl_masked_8800dc_h[][3] = {
	{0x7000216C, ((0x3 << 2) | (0x3 << 4)), ((0x2 << 2) | (0x2 << 4))}, // pmic_pmu_init
	{0x70002138, (0xFF << 0), (0xFF << 0)},
	{0x7000213C, (0xFF << 0), (0xFF << 0)},
	{0x70002144, (0xFF << 0), (0xFF << 0)},
	{0x700021BC, (0x3 << 2), (0x1 << 2)},
	{0x70002118, ((0x7 << 4) | (0x1 << 7)), ((0x2 << 4) | (0x1 << 7))},
	{0x70002104, ((0x3F << 0) | (0x1 << 6)), ((0x2 << 0) | (0x1 << 6))},
	{0x7000210C, ((0x3F << 0) | (0x1 << 6)), ((0x2 << 0) | (0x1 << 6))},
	{0x70002170, (0xF << 0), (0x1 << 0)},
	{0x70002190, (0x3F << 0), (24 << 0)},
	{0x700021CC, ((0x7 << 4) | (0x1 << 7)), ((0x0 << 4) | (0x0 << 7))},
	{0x700010A0, (0x1 << 11), (0x1 << 11)},
	//{0x70001034, ((0x1 << 20) | (0x7 << 26)), ((0x0 << 20) | (0x2 << 26))},
	{0x70001038, (0x1 << 8), (0x1 << 8)},
	{0x70001094, (0x3 << 2), (0x0 << 2)},
	{0x700021D0, ((0x1 << 5) | (0x1 << 6)), ((0x1 << 5) | (0x1 << 6))},
#if defined(CONFIG_VRF_DCDC_MODE)
	{0x70001000, ((0x1 << 0) | (0x1 << 20) | (0x1 << 22)),
				 ((0x1 << 0) | (0x1 << 20) | (0x0 << 22))},
#else
	{0x70001000, ((0x1 << 0) | (0x1 << 20) | (0x1 << 22)),
				 ((0x0 << 0) | (0x1 << 20) | (0x0 << 22))},
#endif
	{0x70001028, (0xf << 2), (0x1 << 2)},
	{0x00000000, 0x00000000, 0x00000000}, // last one
};

static u32 syscfg_tbl_masked_8800dc_u01[][3] = {
	//#ifdef CONFIG_PMIC_SETTING
	{0x70001000, (0x1 << 16), (0x1 << 16)}, // for low temperature
	{0x70001028, (0x1 << 6), (0x1 << 6)},
	{0x70001000, (0x1 << 16), (0x0 << 16)},
	//#endif /* CONFIG_PMIC_SETTING */
};

static u32 syscfg_tbl_8800dc[][2] = {
	{0x40500010, 0x00000004},
	{0x40500010, 0x00000006},//160m clk
};

static u32 syscfg_tbl_8800dc_sdio_u01[][2] = {
	{0x40030000, 0x00036724}, // loop forever after assert_err
	{0x0011E800, 0xE7FE4070},
	{0x40030084, 0x0011E800},
	{0x40030080, 0x00000001},
	{0x4010001C, 0x00000000},
};

static u32 syscfg_tbl_8800dc_sdio_u02[][2] = {
	{0x40030000, 0x00036DA4}, // loop forever after assert_err
	{0x0011E800, 0xE7FE4070},
	{0x40030084, 0x0011E800},
	{0x40030080, 0x00000001},
	{0x4010001C, 0x00000000},
};

#ifdef CONFIG_OOB
u32 oobcfg_tbl_8800dc_sdio_u02[][2] = {
	{0x40504044, 0x2},//oob_enable
	{0x40500060, 0x03020700},
	{0x40500040, 0},
	{0x40100030, 1},
	{0x40241020, 1},
	{0x402400f0, 0x340022},
};
#endif //CONFIG_OOB

static u32 patch_tbl_wifisetting_8800dc_u01[][2] = {
	{0x010c, 0x01001E01}
};

static u32 patch_tbl_wifisetting_8800dc_u02[][2] = {
#if defined(CONFIG_SDIO_PWRCTRL)
	{0x0124, 0x01011E01}
#else
	{0x0124, 0x01001E01}
#endif
};

static u32 adaptivity_patch_tbl_8800dc[][2] = {
	{0x000C, 0x0000320A}, //linkloss_thd
	{0x009C, 0x00000000}, //ac_param_conf
	{0x0128, 0xF6140001}, //tx_adaptivity_en
};

static u32 jump_tbl[][2] = {
#ifndef CONFIG_FOR_IPCOM
	{296, 0x180001},
	{137, 0x180011},
	{303, 0x1810f9},
	{168, 0x18186d},
	{308, 0x181bbd},
	{288, 0x1820c1},
#else
	{308, 0x181001},
	{288, 0x181031},
	{296, 0x18120d},
	{137, 0x18121d},
	{303, 0x182305},
	{168, 0x182a79},
	{258, 0x182ae1},
#endif
};

static uint32_t ldpc_cfg_ram[] = {
#if 0//def CONFIG_FPGA_VERIFICATION
	0x00363638, 0x1DF8F834, 0x1DF8F834, 0x1DF8F834, 0x1DF8F834, 0x002F2F31, 0x1DF8F82C, 0x1DF8F82C,
	0x1DF8F82C, 0x1DF8F82C, 0x00363639, 0x1AA5F834, 0x1AA5F834, 0x1ADEF834, 0x1ADEF834, 0x003A3A3E,
	0x1578F436, 0x1578F436, 0x1578F436, 0x15B6F436, 0x003B3B40, 0x1DF8F838, 0x1DF8F838, 0x1DF8F838,
	0x1DF8F838, 0x003B3B41, 0x1DC4F838, 0x1DC4F838, 0x1DF8F838, 0x1DF8F838, 0x003B3B40, 0x1781F838,
	0x1781F838, 0x1781F838, 0x17C4F838, 0x003B3B40, 0x0E81F838, 0x0E81F838, 0x0E81F838, 0x0E82F838,
	0x003F3F43, 0x1A92F83D, 0x1A92F83E, 0x1A92F83D, 0x1ADDF83D, 0x00272729, 0x1DF8F824, 0x1DF8F824,
	0x1DF8F843, 0x1DF8F843, 0x00272729, 0x1DF8F824, 0x1DF8F824, 0x1DF8F842, 0x1DF8F842, 0x00262628,
	0x1DF8F823, 0x1DF8F823, 0x1DF8F823, 0x1DF8F823, 0x00252528, 0x1DF8F823, 0x1DF8F823, 0x1DF8F823,
	0x1DF8F823, 0x00262628, 0x1DF8F823, 0x1DF8F823, 0x1DF8F823, 0x1DF8F823, 0x00242427, 0x1DF8F821,
	0x1DF8F821, 0x1DF8F821, 0x1DF8F821, 0x00232326, 0x1DF8F821, 0x1DF8F820, 0x1DF8F820, 0x1DF8F820,
	0x00262628, 0x1DF8F823, 0x1DF8F823, 0x1DF8F823, 0x1DF8F823, 0x00242427, 0x1DF8F821, 0x1DF8F821,
	0x1DF8F821, 0x1DF8F821, 0x001F1F21, 0x1DF8F81D, 0x1DF8F81D, 0x1DF8F81D, 0x1DF8F81D, 0x00262643,
	0x1DF8F822, 0x1DF8F821, 0x1DF8F821, 0x1DF8F821, 0x0018182B, 0x1DF8F816, 0x1DBDF815, 0x1DF8F815,
	0x1DF8F815, 0x0018182A, 0x1195F836, 0x1195F815, 0x1195F815, 0x1196F815, 0x0028282C, 0x1DF8F824,
	0x1DF8F824, 0x1DF8F824, 0x1DF8F824, 0x0027272C, 0x1DF8F824, 0x1DF8F823, 0x1DF8F823, 0x1DF8F823,
	0x0082824A, 0x1ADFF841, 0x1ADDF822, 0x1ADEF822, 0x1ADFF822, 0x003E3E40, 0x09D1F81D, 0x095BF81D,
	0x095BF81D, 0x095BF81D, 0x0029292D, 0x1DF8F825, 0x1DF8F825, 0x1DF8F825, 0x1DF8F825, 0x0028282C,
	0x1DF8F824, 0x1DF8F824, 0x1DF8F824, 0x1DF8F824, 0x0029292D, 0x1DF8F825, 0x1DF8F825, 0x1DF8F825,
	0x1DF8F825, 0x0028282E, 0x1DF8F825, 0x1DF8F824, 0x1DF8F824, 0x1DF8F824, 0x0026262C, 0x1DF8F823,
	0x1DF8F822, 0x1DF8F822, 0x1DF8F822, 0x0028282D, 0x1DF8F825, 0x1DF8F824, 0x1DF8F824, 0x1DF8F824,
	0x00282852, 0x1DF8F827, 0x1DF8F824, 0x1DF8F824, 0x1DF8F824, 0x0029294E, 0x1DF8F823, 0x1DF8F822,
	0x1DF8F822, 0x1DF8F822, 0x00212143, 0x1DF8F821, 0x1DECF81D, 0x1DF4F81D, 0x1DF8F81D, 0x0086864D,
	0x1CF0F844, 0x1CEDF823, 0x1CEFF822, 0x1CF0F822, 0x0047474D, 0x1BE8F823, 0x1BE8F823, 0x1BE9F822,
	0x1BEAF822, 0x0018182F, 0x14B0F83C, 0x14B0F814, 0x14B0F814, 0x14B0F814, 0x00404040, 0x0AE1F81E,
	0x0A61F81D, 0x0A61F81D, 0x0A61F81D, 0x002C2C40, 0x09555526, 0x09555512, 0x09555513, 0x09555512,
	0x00181840, 0x06333329, 0x06333314, 0x06333314, 0x06333314, 0x002B2B2F, 0x1DF8F828, 0x1DF8F828,
	0x1DF8F828, 0x1DF8F828, 0x002B2B32, 0x1DF8F829, 0x1DF8F828, 0x1DF8F828, 0x1DF8F828, 0x002A2A2F,
	0x1DF8F827, 0x1DF8F827, 0x1DF8F827, 0x1DF8F827, 0x002A2A57, 0x1DF8F82B, 0x1DF8F827, 0x1DF8F827,
	0x1DF8F827, 0x00919152, 0x1DF8F84B, 0x1DF8F825, 0x1DF8F825, 0x1DF8F825, 0x004C4C51, 0x1DF8F826,
	0x1DF8F825, 0x1DF8F825, 0x1DF8F825, 0x00444440, 0x0CF8F820, 0x0C6EF81F, 0x0C6EF81F, 0x0C6EF81F,
	0x00424240, 0x0D75753E, 0x0D75751E, 0x0D75751E, 0x0D75751E, 0x00191940, 0x0539392E, 0x05393914,
	0x05393914, 0x05393914, 0x002F2F32, 0x1AA5F82C, 0x1AA5F82C, 0x1ADEF82C, 0x1ADEF82C, 0x002F2F40,
	0x0C6EDE2C, 0x0C6EDE2C, 0x0C6EDE2C, 0x0C6EDE2C, 0x00323240, 0x053BB62E, 0x053BB62E, 0x053BB62E,
	0x053BB62E, 0x00333339, 0x1DC4F82F, 0x1DC4F82F, 0x1DF8F82F, 0x1DF8F82F, 0x00333340, 0x0E81F82F,
	0x0E81F82F, 0x0E81F82F, 0x0E82F82F, 0x00333340, 0x063FC42F, 0x063FC42F, 0x063FC42F, 0x063FC42F,
	0x00404040, 0x063FC42F, 0x063FC42F, 0x063FC42F, 0x063FC42F, 0x00363640, 0x0747DD33, 0x0747DD33,
	0x0747DD33, 0x0747DD33, 0x00404040, 0x0747DD33, 0x0747DD33, 0x0747DD33, 0x0747DD33, 0x00292940,
	0x07484825, 0x07484812, 0x07484812, 0x07484812, 0x00404040, 0x07343428, 0x07343414, 0x07343414,
	0x07343414, 0x00404040, 0x0538382A, 0x05383814, 0x05383814, 0x05383814, 0x00404040, 0x05292914,
	0x05292909, 0x05292909, 0x05292909, 0x000B0B40, 0x02111108, 0x0211110E, 0x02111108, 0x02111108,
	0x00404040, 0x063E3E2E, 0x063E3E15, 0x063E3E14, 0x063E3E14, 0x00404040, 0x062E2E14, 0x062E2E09,
	0x062E2E09, 0x062E2E09, 0x000B0B40, 0x02131308, 0x0213130F, 0x02131308, 0x02131308
#else
	0x00767679, 0x1DF8F870, 0x1DF8F870, 0x1DF8F870, 0x1DF8F870, 0x006E6E72, 0x1DF8F869, 0x1DF8F869,
	0x1DF8F869, 0x1DF8F869, 0x0076767B, 0x1DF8F870, 0x1DF8F870, 0x1DF8F870, 0x1DF8F870, 0x007E7E85,
	0x1DF4F876, 0x1DF4F876, 0x1DF4F876, 0x1DF8F876, 0x0081818A, 0x1DF8F87B, 0x1DF8F87B, 0x1DF8F87B,
	0x1DF8F87B, 0x0081818D, 0x1DF8F87B, 0x1DF8F87B, 0x1DF8F87B, 0x1DF8F87B, 0x0081818A, 0x1DF8F87B,
	0x1DF8F87C, 0x1DF8F87B, 0x1DF8F87B, 0x007E7E40, 0x1DF8F87B, 0x1DF8F87B, 0x1DF8F87B, 0x1DF8F87B,
	0x008B8B92, 0x1DF8F887, 0x1DF8F889, 0x1DF8F887, 0x1DF8F887, 0x00515155, 0x1DF8F84C, 0x1DF8F84C,
	0x1DF8F889, 0x1DF8F889, 0x00515154, 0x1DF8F84C, 0x1DF8F84C, 0x1DF8F888, 0x1DF8F888, 0x004F4F53,
	0x1DF8F84A, 0x1DF8F84A, 0x1DF8F84A, 0x1DF8F84A, 0x004F4F53, 0x1DF8F84A, 0x1DF8F84A, 0x1DF8F84A,
	0x1DF8F84A, 0x004F4F53, 0x1DF8F84A, 0x1DF8F84A, 0x1DF8F84A, 0x1DF8F84A, 0x004E4E53, 0x1DF8F849,
	0x1DF8F848, 0x1DF8F848, 0x1DF8F848, 0x004D4D52, 0x1DF8F847, 0x1DF8F847, 0x1DF8F847, 0x1DF8F847,
	0x004F4F55, 0x1DF8F84B, 0x1DF8F84A, 0x1DF8F84A, 0x1DF8F84A, 0x004E4E53, 0x1DF8F849, 0x1DF8F848,
	0x1DF8F848, 0x1DF8F848, 0x0049494D, 0x1DF8F844, 0x1DF8F844, 0x1DF8F844, 0x1DF8F844, 0x0051518F,
	0x1DF8F849, 0x1DF8F848, 0x1DF8F848, 0x1DF8F848, 0x00424277, 0x1DF8F83F, 0x1DF8F83C, 0x1DF8F83C,
	0x1DF8F83C, 0x00424275, 0x1DF8F89E, 0x1DF8F83C, 0x1DF8F83C, 0x1DF8F83C, 0x0055555C, 0x1DF8F84C,
	0x1DF8F84C, 0x1DF8F84C, 0x1DF8F84C, 0x0053535C, 0x1DF8F84C, 0x1DF8F84B, 0x1DF8F84B, 0x1DF8F84B,
	0x00F8F89E, 0x1DF8F88C, 0x1DF8F84A, 0x1DF8F84A, 0x1DF8F84A, 0x00898940, 0x18F8F846, 0x18CFF845,
	0x18CFF844, 0x18CFF844, 0x0056565F, 0x1DF8F84F, 0x1DF8F84F, 0x1DF8F84F, 0x1DF8F84F, 0x0055555E,
	0x1DF8F84E, 0x1DF8F84E, 0x1DF8F84E, 0x1DF8F84E, 0x0056565F, 0x1DF8F84F, 0x1DF8F84F, 0x1DF8F84F,
	0x1DF8F84F, 0x00555561, 0x1DF8F850, 0x1DF8F84E, 0x1DF8F84E, 0x1DF8F84E, 0x0053535F, 0x1DF8F84D,
	0x1DF8F84C, 0x1DF8F84C, 0x1DF8F84C, 0x0055555F, 0x1DF8F84F, 0x1DF8F84E, 0x1DF8F84E, 0x1DF8F84E,
	0x005555AA, 0x1DF8F854, 0x1DF8F84E, 0x1DF8F84E, 0x1DF8F84E, 0x005959A6, 0x1DF8F84D, 0x1DF8F84C,
	0x1DF8F84C, 0x1DF8F84C, 0x004F4F9B, 0x1DF8F84E, 0x1DF8F846, 0x1DF8F846, 0x1DF8F846, 0x00F8F8A5,
	0x1DF8F894, 0x1DF8F84C, 0x1DF8F84C, 0x1DF8F84C, 0x009898A4, 0x1DF8F84D, 0x1DF8F84C, 0x1DF8F84C,
	0x1DF8F84C, 0x00464686, 0x1DF8F8B3, 0x1DF8F83D, 0x1DF8F83D, 0x1DF8F83D, 0x008E8E40, 0x1AF8F848,
	0x1ADFF848, 0x1ADFF846, 0x1ADFF846, 0x007F7F40, 0x18D2D275, 0x18D2D23A, 0x18D2D23A, 0x18D2D239,
	0x00454540, 0x0F868664, 0x0F86863E, 0x0F86863D, 0x0F86863D, 0x005C5C64, 0x1DF8F856, 0x1DF8F855,
	0x1DF8F855, 0x1DF8F855, 0x005B5B68, 0x1DF8F858, 0x1DF8F855, 0x1DF8F855, 0x1DF8F855, 0x005A5A64,
	0x1DF8F855, 0x1DF8F854, 0x1DF8F854, 0x1DF8F854, 0x005A5AB5, 0x1DF8F85B, 0x1DF8F855, 0x1DF8F854,
	0x1DF8F854, 0x00F8F8B0, 0x1DF8F8A3, 0x1DF8F852, 0x1DF8F852, 0x1DF8F852, 0x00A4A4AE, 0x1DF8F854,
	0x1DF8F852, 0x1DF8F852, 0x1DF8F852, 0x009A9A40, 0x1DF8F84E, 0x1DF8F84D, 0x1DF8F84C, 0x1DF8F84C,
	0x009C9C40, 0x1DF8F895, 0x1DF8F849, 0x1DF8F84A, 0x1DF8F84A, 0x00494940, 0x1197976F, 0x11979742,
	0x11979741, 0x11979741, 0x006E6E74, 0x1DF8F869, 0x1DF8F869, 0x1DF8F869, 0x1DF8F869, 0x006E6E40,
	0x1ADEF869, 0x1ADEF869, 0x1ADEF869, 0x1ADEF869, 0x00757540, 0x0D78F86E, 0x0D78F86E, 0x0D78F86E,
	0x0D79F86E, 0x00787885, 0x1DF8F873, 0x1DF8F873, 0x1DF8F873, 0x1DF8F873, 0x00787840, 0x1DF8F873,
	0x1DF8F873, 0x1DF8F873, 0x1DF8F873, 0x00787840, 0x0E81F873, 0x0E81F873, 0x0E81F873, 0x0E82F873,
	0x00404040, 0x0E82F873, 0x0E82F873, 0x0E82F873, 0x0E82F873, 0x00818140, 0x1092F87E, 0x1092F87E,
	0x1092F87E, 0x1092F87E, 0x00404040, 0x1092F87E, 0x1092F87E, 0x1092F87E, 0x1092F87E, 0x00737340,
	0x14B2B26B, 0x14B2B235, 0x14B2B235, 0x14B2B235, 0x00404040, 0x0E828260, 0x0E82823D, 0x0E82823C,
	0x0E82823C, 0x00404040, 0x0F8B8B66, 0x0F8B8B3F, 0x0F8B8B3D, 0x0F8B8B3D, 0x00404040, 0x0B68683D,
	0x0B68681E, 0x0B68681E, 0x0B68681E, 0x00222240, 0x06434318, 0x06434329, 0x06434318, 0x06434318,
	0x00404040, 0x129D9D72, 0x129D9D43, 0x129D9D41, 0x129D9D41, 0x00404040, 0x0D757542, 0x0D757520,
	0x0D757520, 0x0D757520, 0x00232340, 0x084C4C19, 0x084C4C2C, 0x084C4C19, 0x084C4C19
#endif
};

static uint32_t agc_cfg_ram[] = {
	0x20000000, 0x0400000E, 0x3000200E, 0x5B000000, 0x0400004B, 0x3000008E, 0x32000000, 0x0400007B,
	0x40000000, 0xF8000026, 0x04000011, 0x4819008E, 0x9C000020, 0x08000191, 0x38008000, 0x0A000000,
	0x08104411, 0x38018000, 0x0C004641, 0x08D00014, 0x30000000, 0x01000000, 0x04000017, 0x30000000,
	0x3C000000, 0x0400001A, 0x38020000, 0x40000001, 0x0800001D, 0x3808008E, 0x14000050, 0x08000020,
	0x4000008E, 0xA400007B, 0x00000101, 0x3000339F, 0x41000700, 0x04104420, 0x90000000, 0x49000000,
	0xF00E842F, 0xEC0E842C, 0xEC0E842C, 0x04000032, 0x30000000, 0x48000101, 0x04000032, 0x30000000,
	0x48000202, 0x04000032, 0x30000000, 0x46000000, 0x04000011, 0x58010006, 0x3D040472, 0xDC204439,
	0x081DD4D2, 0x480A0006, 0xDC2044DC, 0x081DD43C, 0x38050004, 0x0EF1F1C3, 0x342044DC, 0x30000000,
	0x01000000, 0x04000042, 0x30000000, 0x33000000, 0x04104445, 0x38008000, 0x2200109C, 0x08104448,
	0x38008000, 0x23D4509C, 0x08104417, 0x9000A000, 0x32000000, 0x18000063, 0x14000060, 0x1C000051,
	0x10000057, 0x38028000, 0x0C000001, 0x08D04466, 0x3000200F, 0x00000000, 0x00000000, 0x38030000,
	0x0C002601, 0x08D0445A, 0x30000000, 0x3D020230, 0x0400005D, 0x30000000, 0x3E000100, 0x04000066,
	0x38028000, 0x0C001601, 0x34204466, 0x38028000, 0x0C000A01, 0x34204466, 0x38008004, 0xFF000000,
	0x0800007B, 0x3800802F, 0x26000000, 0x0800006C, 0x380404AF, 0x1F191010, 0x0800006F, 0x20000CAF,
	0x04000071, 0x60000CAF, 0x18700079, 0x14000077, 0x10000075, 0x28140CAF, 0x09B00084, 0x280A0CAF,
	0x09B00084, 0x28060CAF, 0x09B00084, 0x28048086, 0x0800007D, 0x38000086, 0x22800000, 0x04000080,
	0x30000000, 0x0EF1F101, 0x36004883, 0x28020000, 0x08000085, 0x3802008E, 0x3D040431, 0x08000088,
	0x3805008E, 0x1F241821, 0x0800008B, 0x3000008E, 0xA0163021, 0x0400008E, 0x3000008E, 0x0EF10012,
	0x34000091, 0x300000CC, 0x50000000, 0x04000094, 0x380095FE, 0x32010000, 0x04000097, 0x50001FFE,
	0x5A010000, 0x6DC9989B, 0xFC19D4B9, 0x30000186, 0x3D840373, 0x0400009E, 0x3000008E, 0x0A000000,
	0x040000A1, 0x3000008E, 0x22C00000, 0x040000A4, 0x9000028E, 0x32010001, 0x8E4000AA, 0xC80000B0,
	0x00000000, 0x00000000, 0x3000008E, 0x32010001, 0x040000CB, 0x3000008E, 0x29000000, 0x94045011,
	0x300019B6, 0x32010000, 0x040000B3, 0x300019B6, 0x3D040431, 0x040000B6, 0x300019B6, 0x22800000,
	0x04000097, 0x30000186, 0x3D840473, 0x040000BC, 0x3000008E, 0x29030000, 0x040000BF, 0x9AEE028E,
	0x32010100, 0x7C0000C5, 0xCC0000B0, 0x080000B0, 0x00000000, 0x3000008E, 0x32010100, 0x040000C8,
	0x3000028E, 0x29000000, 0x94045011, 0x5000038E, 0x29000000, 0x94045011, 0xC0000035, 0x38010006,
	0x3D040472, 0x080000D2, 0x30000004, 0x0EF1F141, 0x340000D5, 0x28040004, 0x080000D7, 0x2808000E,
	0x080000D9, 0x3000018E, 0x0EF10052, 0x340000DC, 0x3000038E, 0x29000000, 0x94045011, 0x38020000,
	0x32000000, 0x080000E2, 0x60000000, 0xD80000E6, 0xD40000E9, 0x040000EC, 0x30000000, 0x0EF1F121,
	0x360048EF, 0x30000000, 0x0C002421, 0x360048EF, 0x30000000, 0x0C000021, 0x360048EF, 0x28020000,
	0x0800007B, 0x50001EFE, 0x5A010000, 0x6DC998F5, 0xFC19D4F8, 0x3000028E, 0x32000040, 0x040000FB,
	0x3AEE028E, 0x32000080, 0x040000FB, 0x30000000, 0x0EF1F101, 0x360048FE, 0x28020000, 0x08000100,
	0x3802008E, 0x3D040431, 0x08000103, 0x3805008E, 0x1F241821, 0x08000106, 0x3000008E, 0xA0163021,
	0x04000109, 0x3000008E, 0x0EF10012, 0x3400010C, 0x300014F6, 0x32010000, 0x04000114, 0x20000000,
	0x04000111, 0x300000EC, 0x50000000, 0x040000F1, 0x300014F6, 0x32030000, 0x04000117, 0x30001086,
	0x3D840473, 0x0400011A, 0x5000108E, 0x22C00000, 0x8E47C0CB, 0xCB30011E, 0x300019B6, 0x32040000,
	0x04000121, 0x300019B6, 0x3D040431, 0x04000124, 0x300019B6, 0x22800000, 0x04000111, 0x00000000,
	0x00000000, 0x00000000, 0x30000186, 0x3D840473, 0x0400012D, 0x5000038E, 0x29000000, 0x94045011,
	0xC0000131, 0x380C800E, 0xFF000000, 0x08000134, 0x30000004, 0x0FF1F103, 0x34000137, 0x28020000,
	0x08000139, 0x3000038E, 0x29000000, 0x94045011, 0x00000000, 0x00000000, 0x00000000, 0x58010006,
	0x3D040472, 0xDC204543, 0x081DD4D2, 0x480A0006, 0xDC2044DC, 0x081DD546, 0x38050004, 0x0EF1F141,
	0x342044DC, 0x2802800E, 0x080000DC, 0x48000035, 0x0400014A, 0x7896638F, 0x4100000F, 0x8C00014F,
	0x080450C4, 0x90104574, 0x88C8620F, 0xC000015A, 0x90104574, 0x08104554, 0x94104557, 0x3000628F,
	0x29000000, 0x9404517A, 0x3000638F, 0x29000000, 0x0410457A, 0x3800E005, 0x3D010131, 0x0810455D,
	0xA832600F, 0x90104574, 0x08000154, 0x94104557, 0xC6104567, 0xC4185563, 0x5802E00F, 0x0FEEEA07,
	0x80000174, 0x3420456B, 0x5802E00F, 0x0EEEEA07, 0x80000174, 0x3420456B, 0x30004000, 0x33000001,
	0x0400016E, 0x38034005, 0x3D030373, 0x08000171, 0x30006007, 0x33000000, 0x04000174, 0x3000608F,
	0x29000000, 0x94045177, 0x4000608F, 0xA010457D, 0x0410457A, 0x3000608F, 0x64000101, 0x04104411,
	0x3000608F, 0x64000101, 0x04104580, 0x3000618F, 0x42000001, 0x04000183, 0x38028000, 0x32000000,
	0x08104586, 0x280A618F, 0x08000188, 0x480A618F, 0xBC00018B, 0x0800018E, 0x3000618F, 0x34000001,
	0x04000005, 0x3000618F, 0x34000000, 0x04000008, 0x3000008F, 0x0EEAED0F, 0x36000194, 0x38038000,
	0x34000000, 0x08000197, 0x38028005, 0x29010002, 0x0800019A, 0x3000028F, 0x2200209C, 0x0400019D,
	0x3000028F, 0x23D4509C, 0x040001A0, 0x2814028F, 0x080001A2, 0x3000028F, 0x43010201, 0x040001A5,
	0x3000128F, 0x32000100, 0x040001A8, 0x5AEE138F, 0x4100000F, 0x7C0001AC, 0x080000F9, 0x592C138F,
	0x29000000, 0x8C0001B0, 0x080000F9, 0x2000138F, 0x94045011, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
};

static uint32_t txgain_map[96] =  {
#ifdef CONFIG_FPGA_VERIFICATION
	0x20c0c971, 0x20c0c980, 0x20c0c992, 0x20c0c9a6, 0x20c0c9bf, 0x20c0caa5, 0x20c0cabd, 0x20c0cba0,
	0x20c0cbb6, 0x20c0cbea, 0x20c0ccc5, 0x20c0cdac, 0x20c0cdd0, 0x20c0ceb2, 0x20c0ceff, 0x20c0cfff,
	0x20c0c922, 0x20c0c922, 0x20c0c922, 0x20c0c922, 0x20c0c922, 0x20c0c922, 0x20c0c922, 0x20c0c927,
	0x20c0c92c, 0x20c0c931, 0x20c0c937, 0x20c0c93f, 0x20c0c946, 0x20c0c94f, 0x20c0c959, 0x20c0c964,
	0x20c0cbee, 0x20c0cce0, 0x20c0ccff, 0x20c0cde2, 0x20c0cdfe, 0x20c0cede, 0x20c0cefc, 0x20c0cfd9,
	0x20c0cff8, 0x20c0cfff, 0x20c0cfff, 0x20c0cfff, 0x20c0cfff, 0x20c0cfff, 0x20c0cfff, 0x20c0cfff,
	0x20c0c97c, 0x20c0c97c, 0x20c0c97c, 0x20c0c97c, 0x20c0c97c, 0x20c0c97c, 0x20c0c97c, 0x20c0c98c,
	0x20c0ca79, 0x20c0ca89, 0x20c0cb74, 0x20c0cb84, 0x20c0cb94, 0x20c0cba8, 0x20c0cbbb, 0x20c0cbd2,
	0x20c0cbee, 0x20c0cce0, 0x20c0ccff, 0x20c0cde2, 0x20c0cdfe, 0x20c0cede, 0x20c0cefc, 0x20c0cfd9,
	0x20c0cff8, 0x20c0cfff, 0x20c0cfff, 0x20c0cfff, 0x20c0cfff, 0x20c0cfff, 0x20c0cfff, 0x20c0cfff,
	0x20c0c97c, 0x20c0c97c, 0x20c0c97c, 0x20c0c97c, 0x20c0c97c, 0x20c0c97c, 0x20c0c97c, 0x20c0c98c,
	0x20c0ca79, 0x20c0ca89, 0x20c0cb74, 0x20c0cb84, 0x20c0cb94, 0x20c0cba8, 0x20c0cbbb, 0x20c0cbd2,
#else
//11b
	0x00ffd780, 0x00ffd872, 0x00ffd880, 0x00ffd972, 0x00ffd980, 0x00ffda75, 0x00ffda86, 0x00ffdb77,
	0x00ffdb86, 0x00ffdc78, 0x00ffdc89, 0x00ffdd79, 0x00ffdd89, 0x00ffde83, 0x00ffdf79, 0x00ffdf8b,
	0x00ffd072, 0x00ffd072, 0x00ffd080, 0x00ffd172, 0x00ffd180, 0x00ffd272, 0x00ffd280, 0x00ffd36d,
	0x00ffd379, 0x00ffd46d, 0x00ffd479, 0x00ffd572, 0x00ffd580, 0x00ffd672, 0x00ffd680, 0x00ffd772,
//high
	0x00ffc87d, 0x00ffc88b, 0x00ffc979, 0x00ffc989, 0x00ffcc4b, 0x00ffcc54, 0x00ffcc5e, 0x00ffcc69,
	0x00ffcc78, 0x00ffcc85, 0x00ffcd70, 0x00ffcd80, 0x00ffce70, 0x00ffce80, 0x00ffcf7d, 0x00ffcf90,
	0x00ffc080, 0x00ffc090, 0x00ffc180, 0x00ffc190, 0x00ffc27b, 0x00ffc28b, 0x00ffc37b, 0x00ffc390,
	0x00ffc485, 0x00ffc495, 0x00ffc579, 0x00ffc589, 0x00ffc679, 0x00ffc689, 0x00ffc780, 0x00ffc790,
//low
	0x00ffc87d, 0x00ffc88b, 0x00ffc979, 0x00ffc989, 0x00ffcc4b, 0x00ffcc54, 0x00ffcc5e, 0x00ffcc69,
	0x00ffcc78, 0x00ffcc85, 0x00ffcd70, 0x00ffcd80, 0x00ffce70, 0x00ffcf68, 0x00ffcf75, 0x00ffcf83,
	0x00ffc080, 0x00ffc090, 0x00ffc180, 0x00ffc190, 0x00ffc27b, 0x00ffc28b, 0x00ffc37b, 0x00ffc390,
	0x00ffc485, 0x00ffc495, 0x00ffc579, 0x00ffc589, 0x00ffc679, 0x00ffc689, 0x00ffc780, 0x00ffc790,
#endif
};

const uint32_t txgain_map_h[96] =
{
	//11b
	0xffd888, //11
	0xffd979, //12
	0xffd988, //13
	0xffda79, //14
	0xffda88, //15
	0xffdb79, //16
	0xffdb88, //17
	0xffdc72, //18
	0xffdc80, //19
	0xffdd80, //20
	0xffde66, //21
	0xffde72, //22
	0xffde80, //23
	0xffdf79, //24
	0xffdf88, //25
	0xffdf98, //26
	0xffd079, //-5
	0xffd088, //-4
	0xffd179, //-3
	0xffd188, //-2
	0xffd288, //-1
	0xffd36c, //0
	0xffd379, //1
	0xffd388, //2
	0xffd479, //3
	0xffd488, //4
	0xffd579, //5
	0xffd588, //6
	0xffd679, //7
	0xffd688, //8
	0xffd779, //9
	0xffd879, //10
	//high
	0xffc879, //8
	0xffc96b, //9
	0xffc979, //10
	0xffcc45, //11
	0xffcc4d, //12
	0xffcc56, //13
	0xffcc60, //14
	0xffcc6b, //15
	0xffcc79, //16
	0xffcd72, //17
	0xffce60, //18
	0xffce72, //19
	0xffcf72, //20
	0xffcf80, //21
	0xffcf90, //22
	0xffcf90, //23
	0xffc079, //-8
	0xffc16b, //-7
	0xffc179, //-6
	0xffc26b, //-5
	0xffc279, //-4
	0xffc36b, //-3
	0xffc379, //-2
	0xffc46b, //-1
	0xffc479, //0
	0xffc56b, //1
	0xffc579, //2
	0xffc66b, //3
	0xffc679, //4
	0xffc76b, //5
	0xffc779, //6
	0xffc86b, //7
	//low
	0xffc879, //8
	0xffc96b, //9
	0xffc979, //10
	0xffcc45, //11
	0xffcc4d, //12
	0xffcc56, //13
	0xffcc60, //14
	0xffcc6b, //15
	0xffcc79, //16
	0xffcd72, //17
	0xffcf5b, //18
	0xffcf66, //19
	0xffcf72, //20
	0xffcf80, //21
	0xffcf90, //22
	0xffcf90, //23
	0xffc079, //-8
	0xffc16b, //-7
	0xffc179, //-6
	0xffc26b, //-5
	0xffc279, //-4
	0xffc36b, //-3
	0xffc379, //-2
	0xffc46b, //-1
	0xffc479, //0
	0xffc56b, //1
	0xffc579, //2
	0xffc66b, //3
	0xffc679, //4
	0xffc76b, //5
	0xffc779, //6
	0xffc86b, //7
};

static u32 patch_tbl_func[][2] = {
#ifndef CONFIG_FOR_IPCOM
	{0x00110054, 0x0018186D}, // same as jump_tbl idx 168
	{0x0011005C, 0x0018186D}, // same as jump_tbl idx 168
#else
	{0x00110054, 0x00182A79}, // same as jump_tbl idx 168
	{0x0011005C, 0x00182A79}, // same as jump_tbl idx 168
	{0x001118D4, 0x00000011},
#endif
};

static u32 patch_tbl_rf_func[][2] = {
	{0x00110bf0, 0x00180001},
};

#if defined(AICWF_SDIO_SUPPORT)
static const struct aicbsp_firmware fw_u01[] = {
	[AICBSP_CPMODE_WORK] = {
		.desc          = "normal work mode(sdio u01)",
		.bt_adid       = "aic/sdio/aic8800dc/fw_adid_8800dc.bin",
		.bt_patch      = "aic/sdio/aic8800dc/fw_patch_8800dc.bin",
		.bt_table      = "aic/sdio/aic8800dc/fw_patch_table_8800dc.bin",
		.wl_fw         = "aic/sdio/aic8800dc/fmacfw_patch_8800dc.bin",
		.wl_table      = "aic/sdio/aic8800dc/fmacfw_patch_tbl_8800dc.bin",
		.wl_calib      = NULL,
	},

	[AICBSP_CPMODE_TEST] = {
		.desc          = "rf test mode(sdio u01)",
		.bt_adid       = "aic/sdio/aic8800dc/fw_adid_8800dc.bin",
		.bt_patch      = "aic/sdio/aic8800dc/fw_patch_8800dc.bin",
		.bt_table      = "aic/sdio/aic8800dc/fw_patch_table_8800dc.bin",
		.wl_fw         = "aic/sdio/aic8800dc/fmacfw_rf_8800dc.bin",
		.wl_table      = NULL,
		.wl_calib      = NULL,
	},
};

static const struct aicbsp_firmware fw_u02[] = {
	[AICBSP_CPMODE_WORK] = {
		.desc          = "normal work mode(sdio u02)",
		.bt_adid       = "aic/sdio/aic8800dc/fw_adid_8800dc_u02.bin",
		.bt_patch      = "aic/sdio/aic8800dc/fw_patch_8800dc_u02.bin",
		.bt_table      = "aic/sdio/aic8800dc/fw_patch_table_8800dc_u02.bin",
		.wl_fw         = "aic/sdio/aic8800dc/fmacfw_patch_8800dc_u02.bin",
		.wl_table      = "aic/sdio/aic8800dc/fmacfw_patch_tbl_8800dc_u02.bin",
		.wl_calib      = "aic/sdio/aic8800dc/fmacfw_calib_8800dc_u02.bin",
	},

	[AICBSP_CPMODE_TEST] = {
		.desc          = "rf test mode(sdio u02)",
		.bt_adid       = "aic/sdio/aic8800dc/fw_adid_8800dc_u02.bin",
		.bt_patch      = "aic/sdio/aic8800dc/fw_patch_8800dc_u02.bin",
		.bt_table      = "aic/sdio/aic8800dc/fw_patch_table_8800dc_u02.bin",
		.wl_fw         = "aic/sdio/aic8800dc/lmacfw_rf_8800dc.bin",
		.wl_table      = NULL,
		.wl_calib      = "aic/sdio/aic8800dc/fmacfw_calib_8800dc_u02.bin",
	},
};

static const struct aicbsp_firmware fw_h_u02[] = {
	[AICBSP_CPMODE_WORK] = {
		.desc          = "normal work mode(sdio h u02)",
		.bt_adid       = "aic/sdio/aic8800dc/fw_adid_8800dc_u02h.bin",
		.bt_patch      = "aic/sdio/aic8800dc/fw_patch_8800dc_u02h.bin",
		.bt_table      = "aic/sdio/aic8800dc/fw_patch_table_8800dc_u02h.bin",
		.wl_fw         = "aic/sdio/aic8800dc/fmacfw_patch_8800dc_h_u02.bin",
		.wl_table      = "aic/sdio/aic8800dc/fmacfw_patch_tbl_8800dc_h_u02.bin",
		.wl_calib      = "aic/sdio/aic8800dc/fmacfw_calib_8800dc_h_u02.bin",
	},

	[AICBSP_CPMODE_TEST] = {
		.desc          = "rf test mode(sdio h u02)",
		.bt_adid       = "aic/sdio/aic8800dc/fw_adid_8800dc_u02h.bin",
		.bt_patch      = "aic/sdio/aic8800dc/fw_patch_8800dc_u02h.bin",
		.bt_table      = "aic/sdio/aic8800dc/fw_patch_table_8800dc_u02h.bin",
		.wl_fw         = "aic/sdio/aic8800dc/lmacfw_rf_8800dc.bin",
		.wl_table      = NULL,
		.wl_calib      = "aic/sdio/aic8800dc/fmacfw_calib_8800dc_h_u02.bin",
	},
};

#elif defined(AICWF_USB_SUPPORT)
static const struct aicbsp_firmware fw_u01[] = {
	[AICBSP_CPMODE_WORK] = {
		.desc          = "normal work mode(usb u01)",
		.bt_adid       = "aic/usb/aic8800dc/fw_adid_8800dc.bin",
		.bt_patch      = "aic/usb/aic8800dc/fw_patch_8800dc.bin",
		.bt_table      = "aic/usb/aic8800dc/fw_patch_table_8800dc.bin",
		.wl_fw         = "aic/usb/aic8800dc/fmacfw_patch_8800dc.bin",
		.wl_table      = "aic/usb/aic8800dc/fmacfw_patch_tbl_8800dc.bin",
		.wl_calib      = NULL,
	},

	[AICBSP_CPMODE_TEST] = {
		.desc          = "rf test mode(usb u01)",
		.bt_adid       = "aic/usb/aic8800dc/fw_adid_8800dc.bin",
		.bt_patch      = "aic/usb/aic8800dc/fw_patch_8800dc.bin",
		.bt_table      = "aic/usb/aic8800dc/fw_patch_table_8800dc.bin",
		.wl_fw         = "aic/usb/aic8800dc/fmacfw_rf_8800dc.bin",
		.wl_table      = NULL,
		.wl_calib      = NULL,
	},
};

static const struct aicbsp_firmware fw_u02[] = {
	[AICBSP_CPMODE_WORK] = {
		.desc          = "normal work mode(usb u02)",
		.bt_adid       = "aic/usb/aic8800dc/fw_adid_8800dc_u02.bin",
		.bt_patch      = "aic/usb/aic8800dc/fw_patch_8800dc_u02.bin",
		.bt_table      = "aic/usb/aic8800dc/fw_patch_table_8800dc_u02.bin",
		.wl_fw         = "aic/usb/aic8800dc/fmacfw_patch_8800dc_u02.bin",
		.wl_table      = "aic/usb/aic8800dc/fmacfw_patch_tbl_8800dc_u02.bin",
		.wl_calib      = "aic/usb/aic8800dc/fmacfw_calib_8800dc_u02.bin",
	},

	[AICBSP_CPMODE_TEST] = {
		.desc          = "rf test mode(usb u02)",
		.bt_adid       = "aic/usb/aic8800dc/fw_adid_8800dc_u02.bin",
		.bt_patch      = "aic/usb/aic8800dc/fw_patch_8800dc_u02.bin",
		.bt_table      = "aic/usb/aic8800dc/fw_patch_table_8800dc_u02.bin",
		.wl_fw         = "aic/usb/aic8800dc/lmacfw_rf_8800dc.bin",
		.wl_table      = NULL,
		.wl_calib      = "aic/usb/aic8800dc/fmacfw_calib_8800dc_u02.bin",
	},
};

static const struct aicbsp_firmware fw_h_u02[] = {
	[AICBSP_CPMODE_WORK] = {
		.desc          = "normal work mode(usb h u02)",
		.bt_adid       = "usb/aic8800dc/fw_adid_8800dc_u02h.bin",
		.bt_patch      = "usb/aic8800dc/fw_patch_8800dc_u02h.bin",
		.bt_table      = "usb/aic8800dc/fw_patch_table_8800dc_u02h.bin",
		.wl_fw         = "usb/aic8800dc/fmacfw_patch_8800dc_h_u02.bin",
		.wl_table      = "usb/aic8800dc/fmacfw_patch_tbl_8800dc_h_u02.bin",
		.wl_calib      = "usb/aic8800dc/fmacfw_calib_8800dc_h_u02.bin",
	},

	[AICBSP_CPMODE_TEST] = {
		.desc          = "rf test mode(usb h u02)",
		.bt_adid       = "usb/aic8800dc/fw_adid_8800dc_u02h.bin",
		.bt_patch      = "usb/aic8800dc/fw_patch_8800dc_u02h.bin",
		.bt_table      = "usb/aic8800dc/fw_patch_table_8800dc_u02h.bin",
		.wl_fw         = "usb/aic8800dc/lmacfw_rf_8800dc.bin",
		.wl_table      = NULL,
		.wl_calib      = "usb/aic8800dc/fmacfw_calib_8800dc_h_u02.bin",
	},
};

#endif

static int aic8800dc_wifi_patch_table_load(struct priv_dev *aicdev, const char *filename)
{
	unsigned int i = 0;
	int size;
	u32 *dst = NULL;
	int err = 0;

	const struct firmware *fw = NULL;
	int ret = request_firmware(&fw, filename, NULL);
	u8 *describle;
	u32 fmacfw_patch_tbl_8800dc_u02_describe_size = 124;
	u32 fmacfw_patch_tbl_8800dc_u02_describe_base;//read from patch_tbl

	printk("rwnx_request_firmware, name: %s\n", filename);
	if (ret < 0) {
		printk("Load %s fail\n", filename);
		return ret;
	}

	size = fw->size;
	dst = (u32 *)fw->data;

	if (size <= 0) {
		printk("wrong size of firmware file\n");
		release_firmware(fw);
		return -1;
	}

	fmacfw_patch_tbl_8800dc_u02_describe_base = dst[0];
	printk("FMACFW_PATCH_TBL_8800DC_U02_DESCRIBE_BASE = %x \n", fmacfw_patch_tbl_8800dc_u02_describe_base);

	if (!err && (i < size)) {
		err = rwnx_send_dbg_mem_block_write_req(aicdev, fmacfw_patch_tbl_8800dc_u02_describe_base, fmacfw_patch_tbl_8800dc_u02_describe_size + 4, dst);
		if (err)
			printk("write describe information fail\n");

		describle = kzalloc(fmacfw_patch_tbl_8800dc_u02_describe_size, GFP_KERNEL);
		memcpy(describle, &dst[1], fmacfw_patch_tbl_8800dc_u02_describe_size);
		printk("8800DC PATCH VERSION: %s", describle);
		kfree(describle);
		describle = NULL;
	}

	if (!err && (i < size)) {// <1KB data
		for (i = 128 / 4; i < size / 4; i += 2)
			err = rwnx_send_dbg_mem_write_req(aicdev, dst[i], dst[i+1]);

		if (err)
			printk("bin upload fail: %x, err:%d\n", dst[i], err);
	}

	release_firmware(fw);
	return err;
}

static int aic8800dc_wifi_patch_config(struct priv_dev *aicdev)
{
	int ret = 0;
	int cnt = 0;
	if (aicbsp_info.cpmode == AICBSP_CPMODE_WORK) {
		const u32 cfg_base = 0x10164;

		u32 wifisetting_cfg_addr;
		u32 ldpc_cfg_addr;
		u32 agc_cfg_addr;
		u32 txgain_cfg_addr;
		u32 jump_tbl_addr;

		u32 patch_tbl_wifisetting_num;
		u32 ldpc_cfg_size = sizeof(ldpc_cfg_ram);
		u32 agc_cfg_size = sizeof(agc_cfg_ram);
		u32 txgain_cfg_size, *txgain_cfg_array;
		u32 jump_tbl_size = sizeof(jump_tbl)/2;
		u32 patch_tbl_func_num = sizeof(patch_tbl_func)/sizeof(u32)/2;

		struct dbg_mem_read_cfm cfm;
		int i;

		int adap_patch_num;

		array2_tbl_t jump_tbl_base = NULL;
		array2_tbl_t patch_tbl_func_base = NULL;
		array2_tbl_t patch_tbl_wifisetting_8800dc_base = NULL;

		if (aicbsp_info.chipinfo->subrev == 0) {
			jump_tbl_base = jump_tbl;
			jump_tbl_size = sizeof(jump_tbl)/2;
			patch_tbl_func_base = patch_tbl_func;
			patch_tbl_func_num = sizeof(patch_tbl_func)/sizeof(u32)/2;
			patch_tbl_wifisetting_num = sizeof(patch_tbl_wifisetting_8800dc_u01)/sizeof(u32)/2;
			patch_tbl_wifisetting_8800dc_base = patch_tbl_wifisetting_8800dc_u01;
		} else if ((aicbsp_info.chipinfo->subrev == 1) || (aicbsp_info.chipinfo->subrev == 2)) {
			patch_tbl_wifisetting_num = sizeof(patch_tbl_wifisetting_8800dc_u02)/sizeof(u32)/2;
			patch_tbl_wifisetting_8800dc_base = patch_tbl_wifisetting_8800dc_u02;
		} else {
			printk("unsupported id: %d", aicbsp_info.chipinfo->subrev);
			ret = -1;
			goto out;
		}

		ret = rwnx_send_dbg_mem_read_req(aicdev, cfg_base, &cfm);
		if (ret) {
			pr_err("setting base[0x%x] rd fail: %d\n", cfg_base, ret);
			goto out;
		}
		wifisetting_cfg_addr = cfm.memdata;

		if (aicbsp_info.chipinfo->subrev == 0) {
			ret = rwnx_send_dbg_mem_read_req(aicdev, cfg_base + 4, &cfm);
			if (ret) {
				pr_err("setting base[0x%x] rd fail: %d\n", cfg_base + 4, ret);
				goto out;
			}
			jump_tbl_addr = cfm.memdata;
		}

		ret = rwnx_send_dbg_mem_read_req(aicdev, cfg_base + 8, &cfm);
		if (ret) {
			pr_err("setting base[0x%x] rd fail: %d\n", cfg_base + 8, ret);
			goto out;
		}
		ldpc_cfg_addr = cfm.memdata;

		ret = rwnx_send_dbg_mem_read_req(aicdev, cfg_base + 0xc, &cfm);
		if (ret) {
			pr_err("setting base[0x%x] rd fail: %d\n", cfg_base + 0xc, ret);
			goto out;
		}
		agc_cfg_addr = cfm.memdata;

		if (rwnx_send_dbg_mem_read_req(aicdev, cfg_base + 0x10, &cfm)) {
			pr_err("setting base[0x%x] rd fail: %d\n", cfg_base + 0x10, ret);
			goto out;
		}
		txgain_cfg_addr = cfm.memdata;

		printk("wifisetting_cfg_addr=%x, ldpc_cfg_addr=%x, agc_cfg_addr=%x, txgain_cfg_addr=%x\n", wifisetting_cfg_addr, ldpc_cfg_addr, agc_cfg_addr, txgain_cfg_addr);

		for (cnt = 0; cnt < patch_tbl_wifisetting_num; cnt++) {
			ret = rwnx_send_dbg_mem_write_req(aicdev, wifisetting_cfg_addr + patch_tbl_wifisetting_8800dc_base[cnt][0], patch_tbl_wifisetting_8800dc_base[cnt][1]);
			if (ret) {
				pr_err("wifisetting %x write fail\n", patch_tbl_wifisetting_8800dc_base[cnt][0]);
				goto out;
			}
		}
		if (aicbsp_info.adap_test == 1) {
			printk("%s for adaptivity test \r\n", __func__);
			adap_patch_num = sizeof(adaptivity_patch_tbl_8800dc)/sizeof(u32)/2;
			for (cnt = 0; cnt < adap_patch_num; cnt++) {
				ret = rwnx_send_dbg_mem_write_req(aicdev, wifisetting_cfg_addr + adaptivity_patch_tbl_8800dc[cnt][0], adaptivity_patch_tbl_8800dc[cnt][1]);
				if (ret) {
					printk("%x write fail\n", wifisetting_cfg_addr + adaptivity_patch_tbl_8800dc[cnt][0]);
				}
			}
		}

		if (ldpc_cfg_size > 512) {// > 0.5KB data
			for (i = 0; i < (ldpc_cfg_size - 512); i += 512) {//each time write 0.5KB
				ret = rwnx_send_dbg_mem_block_write_req(aicdev, ldpc_cfg_addr + i, 512, ldpc_cfg_ram + i / 4);
				if (ret) {
					pr_err("ldpc upload fail: %x, err:%d\r\n", ldpc_cfg_addr + i, ret);
					goto out;
				}
			}
		}

		if (!ret && (i < ldpc_cfg_size)) {// < 0.5KB data
			ret = rwnx_send_dbg_mem_block_write_req(aicdev, ldpc_cfg_addr + i, ldpc_cfg_size - i, ldpc_cfg_ram + i / 4);
			if (ret) {
				pr_err("ldpc upload fail: %x, err:%d\r\n", ldpc_cfg_addr + i, ret);
				goto out;
			}
		}

		if (agc_cfg_size > 512) {// > 0.5KB data
			for (i = 0; i < (agc_cfg_size - 512); i += 512) {//each time write 0.5KB
				ret = rwnx_send_dbg_mem_block_write_req(aicdev, agc_cfg_addr + i, 512, agc_cfg_ram + i / 4);
				if (ret) {
					pr_err("agc upload fail: %x, err:%d\r\n", agc_cfg_addr + i, ret);
					goto out;
				}
			}
		}

		if (!ret && (i < agc_cfg_size)) {// < 0.5KB data
			ret = rwnx_send_dbg_mem_block_write_req(aicdev, agc_cfg_addr + i, agc_cfg_size - i, agc_cfg_ram + i / 4);
			if (ret) {
				pr_err("agc upload fail: %x, err:%d\r\n", agc_cfg_addr + i, ret);
				goto out;
			}
		}

#if !defined(CONFIG_FPGA_VERIFICATION)
		if (aicbsp_info.chipinfo->is_chip_id_h) {
			txgain_cfg_size = sizeof(txgain_map_h);
			txgain_cfg_array = (u32 *)txgain_map_h;
		} else {
			txgain_cfg_size = sizeof(txgain_map);
			txgain_cfg_array = (u32 *)txgain_map;
		}
		ret = rwnx_send_dbg_mem_block_write_req(aicdev, txgain_cfg_addr, txgain_cfg_size, txgain_cfg_array);
		if (ret) {
			pr_err("txgain upload fail: %x, err:%d\r\n", txgain_cfg_addr, ret);
			goto out;
		}

		if (aicbsp_info.chipinfo->subrev == 0) {
			for (cnt = 0; cnt < jump_tbl_size/4; cnt += 1) {
				//printk("%x = %x\n", jump_tbl[cnt][0]*4+jump_tbl_addr, jump_tbl[cnt][1]);
				ret = rwnx_send_dbg_mem_write_req(aicdev, jump_tbl_base[cnt][0]*4+jump_tbl_addr, jump_tbl_base[cnt][1]);
				if (ret) {
					pr_err("%x write fail\n", jump_tbl_addr+8*cnt);
					goto out;
				}
			}
			for (cnt = 0; cnt < patch_tbl_func_num; cnt++) {
				ret = rwnx_send_dbg_mem_write_req(aicdev, patch_tbl_func_base[cnt][0], patch_tbl_func_base[cnt][1]);
				if (ret) {
					pr_err("patch_tbl_func %x write fail\n", patch_tbl_func_base[cnt][0]);
					goto out;
				}
			}
		} else {
			ret = aic8800dc_wifi_patch_table_load(aicdev, aicbsp_firmware_list[aicbsp_info.cpmode].wl_table);
			if (ret) {
				printk("patch_tbl upload fail: err:%d\r\n", ret);
				goto out;
			}
		}
#endif
	} else {
		if (aicbsp_info.chipinfo->subrev == 0) {
			u32 patch_tbl_rf_func_num = sizeof(patch_tbl_rf_func)/sizeof(u32)/2;
			for (cnt = 0; cnt < patch_tbl_rf_func_num; cnt++) {
				ret = rwnx_send_dbg_mem_write_req(aicdev, patch_tbl_rf_func[cnt][0], patch_tbl_rf_func[cnt][1]);
				if (ret) {
					pr_err("patch_tbl_rf_func %x write fail\n", patch_tbl_rf_func[cnt][0]);
					goto out;
				}
			}
		}
	}

out:
	return ret;
}

static int aic8800dc_bt_patch_config(struct priv_dev *aicdev)
{
	int ret = 0;
	struct aicbt_patch_info_t patch_info = {
		.info_len          = 0,
		.adid_addrinf      = 0,
		.addr_adid         = RAM_8800DC_U01_ADID_ADDR,
		.patch_addrinf     = 0,
		.addr_patch        = RAM_8800DC_FW_PATCH_ADDR,
		.reset_addr        = 0,
		.reset_val         = 0,
		.adid_flag_addr    = 0,
		.adid_flag         = 0,
	};

	struct aicbt_info_t aicbt_info = {
		.btmode        = AICBT_BTMODE_BT_WIFI_COMBO,
		.btport        = AICBT_BTPORT_DEFAULT,
		.uart_baud     = AICBT_UART_BAUD_DEFAULT,
		.uart_flowctrl = AICBT_UART_FC_DEFAULT,
		.lpm_enable    = AICBT_LPM_ENABLE_DEFAULT,
		.txpwr_lvl     = AICBT_TXPWR_LVL_8800DC,
	};

	struct aicbt_patch_table *head = aicbt_patch_table_alloc(aicbsp_firmware_list[aicbsp_info.cpmode].bt_table);
	if (head == NULL) {
		printk("aicbt_patch_table_alloc fail\n");
		return -1;
	}

	aicbsp_driver_btmode_reinit(&aicbt_info);

	if (aicbsp_info.chipinfo->rev != CHIP_REV_ID_U01)
		patch_info.addr_adid = RAM_8800DC_U02_ADID_ADDR;

	ret = aicbt_patch_info_unpack(head, &patch_info);
	if (ret) {
		pr_warn("%s no patch info found in bt fw\n", __func__);
	}

	if (patch_info.reset_addr == 0) {
		patch_info.reset_addr        = FW_RESET_START_ADDR;
		patch_info.reset_val         = FW_RESET_START_VAL;
		patch_info.adid_flag_addr    = FW_ADID_FLAG_ADDR;
		patch_info.adid_flag         = FW_ADID_FLAG_VAL;
		ret = rwnx_send_dbg_mem_write_req(aicdev, patch_info.reset_addr, patch_info.reset_val);
		if (ret)
			goto err;

		ret = rwnx_send_dbg_mem_write_req(aicdev, patch_info.adid_flag_addr, patch_info.adid_flag);
		if (ret)
			goto err;
	}

	ret = rwnx_plat_bin_fw_upload_android(aicdev, patch_info.addr_adid, aicbsp_firmware_list[aicbsp_info.cpmode].bt_adid);
	if (ret)
		goto err;

	ret = rwnx_plat_bin_fw_upload_android(aicdev, patch_info.addr_patch, aicbsp_firmware_list[aicbsp_info.cpmode].bt_patch);
	if (ret)
		goto err;

	ret = aicbt_patch_table_load(aicdev, &aicbt_info, head);
	if (ret)
		printk("aicbt_patch_table_load fail\n");

err:
	aicbt_patch_table_free(&head);
	return ret;
}

#ifdef CONFIG_DPD
rf_misc_ram_lite_t dpd_res = {{0},};
EXPORT_SYMBOL(dpd_res);

static int aicwf_misc_ram_valid_check_8800dc(struct priv_dev *aicdev, int *valid_out)
{
	int ret = 0;
	uint32_t cfg_base = 0x10164;
	struct dbg_mem_read_cfm cfm;
	uint32_t misc_ram_addr;
	uint32_t ram_base_addr, ram_word_cnt;
	uint32_t bit_mask[4];
	int i;

	if (valid_out) {
		*valid_out = 0;
	}

	if (aicbsp_info.cpmode == AICBSP_CPMODE_TEST) {
		uint32_t vect1 = 0;
		uint32_t vect2 = 0;
		cfg_base = RAM_LMAC_FW_ADDR + 0x0004;
		ret = rwnx_send_dbg_mem_read_req(aicdev, cfg_base, &cfm);
		if (ret) {
			printk("cfg_base:%x vcet1 rd fail: %d\n", cfg_base, ret);
			return ret;
		}
		vect1 = cfm.memdata;
		if ((vect1 & 0xFFFF0000) != (RAM_LMAC_FW_ADDR & 0xFFFF0000)) {
			printk("vect1 invalid: %x\n", vect1);
			return ret;
		}
		cfg_base = RAM_LMAC_FW_ADDR + 0x0008;
		ret = rwnx_send_dbg_mem_read_req(aicdev, cfg_base, &cfm);
		if (ret) {
			printk("cfg_base:%x vcet2 rd fail: %d\n", cfg_base, ret);
			return ret;
		}
		vect2 = cfm.memdata;
		if ((vect2 & 0xFFFF0000) != (RAM_LMAC_FW_ADDR & 0xFFFF0000)) {
			printk("vect2 invalid: %x\n", vect2);
			return ret;
		}

		cfg_base = RAM_LMAC_FW_ADDR + 0x0164;
	}
	// init misc ram
	ret = rwnx_send_dbg_mem_read_req(aicdev, cfg_base + 0x14, &cfm);
	if (ret) {
		printk("rf misc ram[0x%x] rd fail: %d\n", cfg_base + 0x14, ret);
		return ret;
	}
	misc_ram_addr = cfm.memdata;
	printk("misc_ram_addr=%x\n", misc_ram_addr);
	// bit_mask
	ram_base_addr = misc_ram_addr + offsetof(rf_misc_ram_t, bit_mask);
	ram_word_cnt = (MEMBER_SIZE(rf_misc_ram_t, bit_mask) + MEMBER_SIZE(rf_misc_ram_t, reserved)) / 4;
	for (i = 0; i < ram_word_cnt; i++) {
		ret = rwnx_send_dbg_mem_read_req(aicdev, ram_base_addr + i * 4, &cfm);
		if (ret) {
			printk("bit_mask[0x%x] rd fail: %d\n",  ram_base_addr + i * 4, ret);
			return ret;
		}
		bit_mask[i] = cfm.memdata;
	}
	printk("bit_mask:%x,%x,%x,%x\n",bit_mask[0],bit_mask[1],bit_mask[2],bit_mask[3]);
	if ((bit_mask[0] == 0) && ((bit_mask[1] & 0xFFF00000) == 0x80000000) &&
		(bit_mask[2] == 0) && ((bit_mask[3] & 0xFFFFFF00) == 0x00000000)) {
		if (valid_out) {
			*valid_out = 1;
		}
	}
	return ret;
}

static int aicwf_plat_calib_load_8800dc(struct priv_dev *aicdev)
{
	int ret = 0;

	ret = rwnx_plat_bin_fw_upload_android(aicdev, ROM_FMAC_CALIB_ADDR, aicbsp_firmware_list[aicbsp_info.cpmode].wl_calib);
	if (ret) {
		printk("load rftest bin fail: %d\n", ret);
		return ret;
	}

	return ret;
}

static int aicwf_dpd_calib_8800dc(struct priv_dev *aicdev, rf_misc_ram_lite_t *dpd_res)
{
	int ret = 0;
	uint32_t fw_addr, boot_type;
	int valid_flag;
	const uint32_t cfg_base = 0x10164;
	struct dbg_mem_read_cfm cfm;
	uint32_t misc_ram_addr;
	uint32_t ram_base_addr, ram_word_cnt;
	int i;

	printk("%s\n", __func__);

	ret = aicwf_misc_ram_valid_check_8800dc(aicdev, &valid_flag);
	if (ret) {
		printk("misc ram check fail: %d\n", ret);
		return ret;
	}
	if (valid_flag) {
		printk("misc ram valid, skip calib process\n");
		return ret;
	}
	ret = aicwf_plat_calib_load_8800dc(aicdev);
	if (ret) {
		printk("load calib bin fail: %d\n", ret);
		return ret;
	}
	/* fw start */
	fw_addr = 0x00130009;
	boot_type = HOST_START_APP_FNCALL;
	printk("Start app: %08x, %d\n", fw_addr, boot_type);
	ret = rwnx_send_dbg_start_app_req(aicdev, fw_addr, boot_type, NULL);
	if (ret) {
		printk("start app fail: %d\n", ret);
		return ret;
	}

	// read dpd res
	ret = rwnx_send_dbg_mem_read_req(aicdev, cfg_base + 0x14, &cfm);
	if (ret) {
		printk("rf misc ram[0x%x] rd fail: %d\n", cfg_base + 0x14, ret);
		return ret;
	}
	misc_ram_addr = cfm.memdata;
	// bit_mask
	ram_base_addr = misc_ram_addr + offsetof(rf_misc_ram_t, bit_mask);
	ram_word_cnt = (MEMBER_SIZE(rf_misc_ram_t, bit_mask) + MEMBER_SIZE(rf_misc_ram_t, reserved)) / 4;
	for (i = 0; i < ram_word_cnt; i++) {
		ret = rwnx_send_dbg_mem_read_req(aicdev, ram_base_addr + i * 4, &cfm);
		if (ret) {
			printk("bit_mask[0x%x] rd fail: %d\n",  ram_base_addr + i * 4, ret);
			return ret;
		}
		dpd_res->bit_mask[i] = cfm.memdata;
	}
	// dpd_high
	ram_base_addr = misc_ram_addr + offsetof(rf_misc_ram_t, dpd_high);
	ram_word_cnt = MEMBER_SIZE(rf_misc_ram_t, dpd_high) / 4;
	for (i = 0; i < ram_word_cnt; i++) {
		ret = rwnx_send_dbg_mem_read_req(aicdev, ram_base_addr + i * 4, &cfm);
		if (ret) {
			printk("bit_mask[0x%x] rd fail: %d\n",  ram_base_addr + i * 4, ret);
			return ret;
		}
		dpd_res->dpd_high[i] = cfm.memdata;
	}
	// loft_res
	ram_base_addr = misc_ram_addr + offsetof(rf_misc_ram_t, loft_res);
	ram_word_cnt = MEMBER_SIZE(rf_misc_ram_t, loft_res) / 4;
	for (i = 0; i < ram_word_cnt; i++) {
		ret = rwnx_send_dbg_mem_read_req(aicdev, ram_base_addr + i * 4, &cfm);
		if (ret) {
			printk("bit_mask[0x%x] rd fail: %d\n",  ram_base_addr + i * 4, ret);
			return ret;
		}
		dpd_res->loft_res[i] = cfm.memdata;
	}

	return ret;
}

#ifndef CONFIG_FORCE_DPD_CALIB
int is_file_exist(char* name)
{
	const struct firwmware *fw = NULL;
	int ret = request_firmware(&fw, name, NULL);
	if (ret < 0) {
		printk("File %s load fail or not exist", name);
		return 0;
	} else {
		return 1;
	}
}
EXPORT_SYMBOL(is_file_exist);

static int aicwf_dpd_result_apply_8800dc(struct priv_dev *aicdev, rf_misc_ram_lite_t *dpd_res)
{
	int ret = 0;
	uint32_t cfg_base = 0x10164;
	struct dbg_mem_read_cfm cfm;
	uint32_t misc_ram_addr;
	uint32_t ram_base_addr, ram_byte_cnt;

	printk("bit_mask[1]=%x\n", dpd_res->bit_mask[1]);
	if (dpd_res->bit_mask[1] == 0) {
		printk("void dpd_res, bypass it.\n");
		return 0;
	}
	if (aicbsp_info.cpmode == AICBSP_CPMODE_TEST) {
		cfg_base = RAM_LMAC_FW_ADDR + 0x0164;
	}
	ret = rwnx_send_dbg_mem_read_req(aicdev, cfg_base + 0x14, &cfm);
	if (ret) {
		printk("rf misc ram[0x%x] rd fail: %d\n", cfg_base + 0x14, ret);
		return ret;
	}
	misc_ram_addr = cfm.memdata;
	printk("misc_ram_addr: %x\n", misc_ram_addr);
	/* Copy dpd_res on the Embedded side */
	// bit_mask
	printk("bit_mask[0]=%x\n", dpd_res->bit_mask[0]);
	ram_base_addr = misc_ram_addr + offsetof(rf_misc_ram_t, bit_mask);
	ram_byte_cnt = MEMBER_SIZE(rf_misc_ram_t, bit_mask) + MEMBER_SIZE(rf_misc_ram_t, reserved);
	ret = rwnx_send_dbg_mem_block_write_req(aicdev, ram_base_addr, ram_byte_cnt, (u32 *)&dpd_res->bit_mask[0]);
	if (ret) {
		printk("bit_mask wr fail: %x, ret:%d\r\n", ram_base_addr, ret);
		return ret;
	}
	// dpd_high
	printk("dpd_high[0]=%x\n", dpd_res->dpd_high[0]);
	ram_base_addr = misc_ram_addr + offsetof(rf_misc_ram_t, dpd_high);
	ram_byte_cnt = MEMBER_SIZE(rf_misc_ram_t, dpd_high);
	ret = rwnx_send_dbg_mem_block_write_req(aicdev, ram_base_addr, ram_byte_cnt, (u32 *)&dpd_res->dpd_high[0]);
	if (ret) {
		printk("dpd_high wr fail: %x, ret:%d\r\n", ram_base_addr, ret);
		return ret;
	}
	// loft_res
	printk("loft_res[0]=%x\n", dpd_res->loft_res[0]);
	ram_base_addr = misc_ram_addr + offsetof(rf_misc_ram_t, loft_res);
	ram_byte_cnt = MEMBER_SIZE(rf_misc_ram_t, loft_res);
	ret = rwnx_send_dbg_mem_block_write_req(aicdev, ram_base_addr, ram_byte_cnt, (u32 *)&dpd_res->loft_res[0]);
	if (ret) {
		printk("loft_res wr fail: %x, ret:%d\r\n", ram_base_addr, ret);
		return ret;
	}
	return ret;
}

static int aicwf_dpd_result_load_8800dc(struct priv_dev *aicdev, rf_misc_ram_lite_t *dpd_res)
{
	int ret = 0;
	int size;
	u32 *dst=NULL;
	char *filename = FW_DPDRESULT_NAME_8800DC;
	struct device *dev = aicdev->dev;
	printk("%s: dpd_res file path:%s \r\n", __func__, filename);
	/* load file */
	size = request_firmware(&dst, filename, dev);
	if (size <= 0) {
		printk("wrong size of dpd_res file\n");
		if (dst) {
#ifndef CONFIG_FIRMWARE_ARRAY
			vfree(dst);
#endif
			dst = NULL;
		}
		return -1;
	}
	printk("### Load file done: %s, size=%d, dst[0]=%x\n", filename, size, dst[0]);
	memcpy((u8 *)dpd_res, (u8 *)dst, sizeof(rf_misc_ram_lite_t));
	if (dst) {
#ifndef CONFIG_FIRMWARE_ARRAY
		vfree(dst);
#endif
		dst = NULL;
	}
	return ret;
}

#if 0
static int aicwf_dpd_result_write_8800dc(void *buf, int buf_len)
{
	int sum = 0, len = 0;
	char *path = NULL;
	struct file *fp = NULL;
	loff_t pos = 0;
	mm_segment_t fs;

	printk("%s\n", __func__);

	path = __getname();
	if (!path) {
		printk("get path fail\n");
		return -1;
	}

	len = snprintf(path, FW_PATH_MAX_LEN, "%s/%s", AICBSP_FW_PATH_8800DC, FW_DPDRESULT_NAME_8800DC);
	printk("%s\n", path);

	fp = filp_open(path, O_RDWR | O_CREAT, 0644);
	if (IS_ERR(fp)) {
		printk"fp open fial\n");
		__putname(path);
		fp = NULL;
		return -1;
	}

	fs = get_fs();
	set_fs(KERNEL_DS);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 14, 0)
	sum = kernel_write(fp, buf, buf_len, &pos);
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(3, 9, 0)
	sum = kernel_write(fp, (char *)buf, buf_len, pos);
#else
	sum = vfs_write(fp, (char *)buf, buf_len, &pos);
#endif

	set_fs(fs);
	__putname(path);
	filp_close(fp, NULL);
	fp = NULL;

	return 0;
}
#endif
#endif /* !CONFIG_FORCE_DPD_CALIB */
#endif

static int aicbsp_misc_ram_init_8800dc(struct priv_dev *aicdev)
{
	int ret = 0;
	uint32_t cfg_base = 0x10164;
	struct dbg_mem_read_cfm cfm;
	uint32_t misc_ram_addr;
	uint32_t misc_ram_size = 12;
	int i;

	if (aicbsp_info.cpmode == AICBSP_CPMODE_TEST) {
		cfg_base = RAM_LMAC_FW_ADDR + 0x0164;
	}
	// init misc ram
	ret = rwnx_send_dbg_mem_read_req(aicdev, cfg_base + 0x14, &cfm);
	if (ret) {
		printk("rf misc ram[0x%x] rd fail: %d\n", cfg_base + 0x14, ret);
		return ret;
	}
	misc_ram_addr = cfm.memdata;
	printk("misc_ram_addr=%x\n", misc_ram_addr);
	for (i = 0; i < (misc_ram_size / 4); i++) {
		ret = rwnx_send_dbg_mem_write_req(aicdev, misc_ram_addr + i * 4, 0);
		if (ret) {
			printk("rf misc ram[0x%x] wr fail: %d\n",  misc_ram_addr + i * 4, ret);
			return ret;
		}
	}
	return ret;
}

static int aicbsp_patch_load_8800dc(struct priv_dev *aicdev)
{
	int ret = 0;
	u32 ld_addr;

	if (aicbsp_info.cpmode == AICBSP_CPMODE_TEST) {
		ld_addr = RAM_LMAC_FW_ADDR;
	} else {
		ld_addr = ROM_FMAC_PATCH_ADDR;
	}

	printk("%s %d, aicbsp_info.cpmode=%d\n", __func__, __LINE__, aicbsp_info.cpmode);
	if (aicbsp_info.chipinfo->subrev == 0) {
		ret = rwnx_plat_bin_fw_upload_android(aicdev, ld_addr, aicbsp_firmware_list[aicbsp_info.cpmode].wl_fw);
		if (ret)
			return ret;
	} else if ((aicbsp_info.chipinfo->subrev == 1) || (aicbsp_info.chipinfo->subrev == 2)) {
		if (aicbsp_info.cpmode == AICBSP_CPMODE_WORK) {
			ret = rwnx_plat_bin_fw_upload_android(aicdev, ld_addr, aicbsp_firmware_list[aicbsp_info.cpmode].wl_fw);
			if (ret)
				return ret;
#ifdef CONFIG_DPD
#ifdef CONFIG_FORCE_DPD_CALIB
			if (1) {
				printk("dpd calib & write\n");
				ret = aicwf_dpd_calib_8800dc(aicdev, &dpd_res);
				if (ret) {
					printk("dpd calib fail: %d\n", ret);
					return ret;
				}
			}
#else
			if (is_file_exist(FW_DPDRESULT_NAME_8800DC) == 1) {
				printk("dpd bin load\n");
				ret = aicwf_dpd_result_load_8800dc(aicdev, &dpd_res);
				if (ret) {
					printk("load dpd bin fail: %d\n", ret);
					return ret;
				}
				ret = aicwf_dpd_result_apply_8800dc(aicdev, &dpd_res);
				if (ret) {
					printk("apply dpd bin fail: %d\n", ret);
					return ret;
				}
			}
#endif
			else
#endif
			{
				ret = aicbsp_misc_ram_init_8800dc(aicdev);
				if (ret) {
					printk("misc ram init fail: %d\n", ret);
					return ret;
				}
			}
		} else if (aicbsp_info.cpmode == AICBSP_CPMODE_TEST) {
#ifdef CONFIG_DPD
#ifdef CONFIG_FORCE_DPD_CALIB
			if (1) {
				printk("patch load\n");
				ret = rwnx_plat_bin_fw_upload_android(aicdev, ROM_FMAC_PATCH_ADDR, aicbsp_firmware_list[AICBSP_CPMODE_WORK].wl_fw); // 180000
				if (ret) {
					printk("load patch bin fail: %d\n", ret);
					return ret;
				}
				printk("dpd calib & write\n");
				ret = aicwf_dpd_calib_8800dc(aicdev, &dpd_res);
				if (ret) {
					printk("dpd calib fail: %d\n", ret);
					return ret;
				}
			}
#endif
#endif
			ret = rwnx_plat_bin_fw_upload_android(aicdev, ld_addr, aicbsp_firmware_list[aicbsp_info.cpmode].wl_fw); //150000
			if (ret) {
			    printk("load rftest bin fail: %d\n", ret);
			    return ret;
			}
		} else if (aicbsp_info.cpmode == AICBSP_CPMODE_DPDCALIB) {
#if (defined(CONFIG_DPD) && !defined(CONFIG_FORCE_DPD_CALIB))
			if (is_file_exist(FW_DPDRESULT_NAME_8800DC) == 0) {
				printk("patch load\n");
				ret = rwnx_plat_bin_fw_upload_android(aicdev, ROM_FMAC_PATCH_ADDR, aicbsp_firmware_list[AICBSP_CPMODE_WORK].wl_fw);
				if (ret) {
					printk("load patch bin fail: %d\n", ret);
					return ret;
				}
				printk("dpd calib & write\n");
				ret = aicwf_dpd_calib_8800dc(aicdev, &dpd_res);
				if (ret) {
					printk("dpd calib fail: %d\n", ret);
					return ret;
				}
#if 0
				ret = aicwf_dpd_result_write_8800dc((void *)&dpd_res, DPD_RESULT_SIZE_8800DC);
				if (ret) {
					printk("file write fail: %d\n", ret);
					return ret;
				}
#endif
			}
#endif
			return 1; // exit calib mode
		}
	}

    return ret;
}

int aicbsp_8800dc_fw_init(struct priv_dev *aicdev)
{
	u32 mem_addr = 0x40500000;
	struct dbg_mem_read_cfm rd_mem_addr_cfm;

	int syscfg_num, cnt, ret;
	array3_tbl_t p_syscfg_msk_tbl;
	u32 boot_type = HOST_START_APP_DUMMY;
	u32 rd_addr, fw_addr;
#ifdef CONFIG_OOB
	int oobcfg_num;
#endif

	if (aicbsp_info.cpmode == AICBSP_CPMODE_TEST) {
		rd_addr = RAM_LMAC_FW_ADDR;
		fw_addr = RAM_LMAC_FW_ADDR;
	} else {
		rd_addr = RAM_FMAC_FW_ADDR;
		fw_addr = RAM_FMAC_FW_ADDR;
	}

	if (rwnx_send_dbg_mem_read_req(aicdev, mem_addr, &rd_mem_addr_cfm))
		return -1;
	aicbsp_info.chipinfo->rev = (u8)((rd_mem_addr_cfm.memdata >> 16) & 0x3F);
	aicbsp_info.chipinfo->is_chip_id_h = (u8)IS_CHIP_ID_H(rd_mem_addr_cfm.memdata >> 16);

	aicbsp_info.chipinfo->mcuid = 0;
	if (((rd_mem_addr_cfm.memdata >> 25) & 0x01UL) == 0x00UL)
		aicbsp_info.chipinfo->mcuid = 1;

	mem_addr = 0x00000020;
	if (rwnx_send_dbg_mem_read_req(aicdev, mem_addr, &rd_mem_addr_cfm))
		return -1;
	aicbsp_info.chipinfo->subrev = (u8)rd_mem_addr_cfm.memdata;

	printk("%s(%d), rev id: 0x%x, subrev id: 0x%x, mcu id: 0x%x, is_chip_id_h: %d\n", __func__, __LINE__,
			aicbsp_info.chipinfo->rev, aicbsp_info.chipinfo->subrev, aicbsp_info.chipinfo->mcuid, aicbsp_info.chipinfo->is_chip_id_h);

	if (aicbsp_info.chipinfo->subrev != 0 && aicbsp_info.chipinfo->subrev != 1 && aicbsp_info.chipinfo->subrev != 2) {
		printk("%s(%d), unsupported subrev: %d\n", __func__, __LINE__, aicbsp_info.chipinfo->subrev);
		return -1;
	}

	if (aicbsp_info.chipinfo->is_chip_id_h) {
		aicbsp_firmware_list = fw_h_u02;
	} else {
		if (aicbsp_info.chipinfo->rev == CHIP_REV_ID_U01)
			aicbsp_firmware_list = fw_u01;
		else
			aicbsp_firmware_list = fw_u02;
	}

	mem_addr = 0x40500010;
	if (rwnx_send_dbg_mem_read_req(aicdev, mem_addr, &rd_mem_addr_cfm))
		return -1;

	if (aicbsp_info.chipinfo->is_chip_id_h) {
		p_syscfg_msk_tbl = syscfg_tbl_masked_8800dc_h;
		syscfg_num = sizeof(syscfg_tbl_masked_8800dc_h) / sizeof(u32) / 3;
	} else {
		p_syscfg_msk_tbl = syscfg_tbl_masked_8800dc;
		syscfg_num = sizeof(syscfg_tbl_masked_8800dc) / sizeof(u32) / 3;
	}

	for (cnt = 0; cnt < syscfg_num; cnt++) {
		if (p_syscfg_msk_tbl[cnt][0] == 0x00000000)
			break;

		if (p_syscfg_msk_tbl[cnt][0] == 0x70001000) {
			if (aicbsp_info.chipinfo->mcuid == 0) {
				p_syscfg_msk_tbl[cnt][1] |= ((0x1 << 8) | (0x1 << 15)); // mask
				p_syscfg_msk_tbl[cnt][2] |= ((0x1 << 8) | (0x1 << 15));
			}
		}

		ret = rwnx_send_dbg_mem_mask_write_req(aicdev,
			p_syscfg_msk_tbl[cnt][0], p_syscfg_msk_tbl[cnt][1], p_syscfg_msk_tbl[cnt][2]);
		if (ret) {
			printk("%x mask write fail: %d\n", p_syscfg_msk_tbl[cnt][0], ret);
			return ret;
		}
	}

	aic8800dc_bt_patch_config(aicdev);

	syscfg_num = sizeof(syscfg_tbl_8800dc) / sizeof(u32) / 2;

	for (cnt = 0; cnt < syscfg_num; cnt++) {
		ret = rwnx_send_dbg_mem_write_req(aicdev, syscfg_tbl_8800dc[cnt][0], syscfg_tbl_8800dc[cnt][1]);
		if (ret) {
			bsp_err("%x write fail: %d\n", syscfg_tbl_8800dc[cnt][0], ret);
			return ret;
		}
	}

	if (aicbsp_info.chipinfo->mcuid == 0) {
		if (aicbsp_info.chipinfo->subrev == 0) {
			syscfg_num = sizeof(syscfg_tbl_8800dc_sdio_u01) / sizeof(u32) / 2;

			for (cnt = 0; cnt < syscfg_num; cnt++) {
				ret = rwnx_send_dbg_mem_write_req(aicdev, syscfg_tbl_8800dc_sdio_u01[cnt][0], syscfg_tbl_8800dc_sdio_u01[cnt][1]);
				if (ret) {
					printk("%x write fail: %d\n", syscfg_tbl_8800dc_sdio_u01[cnt][0], ret);
					return ret;
				}
			}
		} else if ((aicbsp_info.chipinfo->subrev == 1) || (aicbsp_info.chipinfo->subrev == 2)) {
			syscfg_num = sizeof(syscfg_tbl_8800dc_sdio_u02) / sizeof(u32) / 2;

			for (cnt = 0; cnt < syscfg_num; cnt++) {
				ret = rwnx_send_dbg_mem_write_req(aicdev, syscfg_tbl_8800dc_sdio_u02[cnt][0], syscfg_tbl_8800dc_sdio_u02[cnt][1]);
				if (ret) {
					printk("%x write fail: %d\n", syscfg_tbl_8800dc_sdio_u02[cnt][0], ret);
					return ret;
				}
			}
		}
	}

#ifdef CONFIG_OOB
	if ((aicbsp_info.chipinfo->subrev == 1) || (aicbsp_info.chipinfo->subrev == 2)) {
		oobcfg_num = sizeof(oobcfg_tbl_8800dc_sdio_u02) / sizeof(u32) / 2;
		for (cnt = 0; cnt < oobcfg_num; cnt++) {
			ret = rwnx_send_dbg_mem_write_req(aicdev, oobcfg_tbl_8800dc_sdio_u02[cnt][0], oobcfg_tbl_8800dc_sdio_u02[cnt][1]);
			if (ret) {
				printk("%x write fail: %d\n", oobcfg_tbl_8800dc_sdio_u02[cnt][0], ret);
				return ret;
			}
		}
	}
#endif

	if (aicbsp_info.chipinfo->subrev == 0) {
		syscfg_num = sizeof(syscfg_tbl_masked_8800dc_u01) / sizeof(u32) / 3;
		for (cnt = 0; cnt < syscfg_num; cnt++) {
			ret = rwnx_send_dbg_mem_mask_write_req(aicdev,
				syscfg_tbl_masked_8800dc_u01[cnt][0], syscfg_tbl_masked_8800dc_u01[cnt][1], syscfg_tbl_masked_8800dc_u01[cnt][2]);
			if (ret) {
				printk("%x mask write fail: %d\n", syscfg_tbl_masked_8800dc_u01[cnt][0], ret);
				return ret;
			}
		}
	}

	ret = aicbsp_patch_load_8800dc(aicdev);
	if (ret)
		return ret;

	ret = aic8800dc_wifi_patch_config(aicdev);
	if (ret)
		return ret;

	printk("Read FW mem: %08x\n", rd_addr);
	if (rwnx_send_dbg_mem_read_req(aicdev, rd_addr, &rd_mem_addr_cfm))
		return -1;

	printk("cfm: [%08x] = %08x\n", rd_mem_addr_cfm.memaddr, rd_mem_addr_cfm.memdata);

	boot_type = HOST_START_APP_DUMMY;
	if (aicbsp_info.cpmode == AICBSP_CPMODE_TEST)
		boot_type = HOST_START_APP_AUTO;

	/* fw start */
	printk("Start app: %08x, %d\n", fw_addr, boot_type);
	if (rwnx_send_dbg_start_app_req(aicdev, fw_addr, boot_type, NULL))
		return -1;

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
