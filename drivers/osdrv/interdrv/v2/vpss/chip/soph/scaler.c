#ifdef ENV_CVITEST
#include <common.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include "system_common.h"
#include "timer.h"
#elif defined(ENV_EMU)
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include "emu/command.h"
#else
#include <linux/types.h>
#include <linux/delay.h>

#endif	// ENV_CVITEST

#include "vpss_debug.h"
#include "scaler.h"
#include "vpss_common.h"
#include "scaler_reg.h"
#include "reg.h"
#include "cmdq.h"
#include "ion.h"
#include "vo_sys.h"
#include "vi_sys.h"


/****************************************************************************
 * Global parameters
 ****************************************************************************/
static struct sclr_core_cfg g_sc_cfg[SCL_MAX_INST];
static struct sclr_gop_cfg g_gop_cfg[SCL_MAX_INST][SCL_MAX_GOP_INST];
static struct sclr_img_cfg g_img_cfg[SCL_MAX_INST];
static struct sclr_cir_cfg g_cir_cfg[SCL_MAX_INST];
static struct sclr_border_cfg g_bd_cfg[SCL_MAX_INST];
static struct sclr_border_vpp_cfg g_bd_vpp_cfg[SCL_MAX_INST][BORDER_VPP_MAX];
static struct sclr_odma_cfg g_odma_cfg[SCL_MAX_INST];
static struct sclr_fbd_cfg g_fbd_cfg[SCL_MAX_INST];
static uintptr_t reg_base_vi, reg_base_vd0, reg_base_vd1, reg_base_vo;
static uintptr_t top_rst_reg_base, vi_sys_reg_base, vo_sys_reg_base;
static u32 reset_mask[SCL_MAX_INST] = {BIT(16), BIT(17), BIT(18), BIT(19), BIT(10), BIT(11), BIT(14), BIT(15), BIT(4), BIT(5)};
static u32 reset_apb_mask[4] = {BIT(11), BIT(12), BIT(13), BIT(14)}; //only vi apb reset bit pos differ in sw reset
static uintptr_t reg_base = 0;
/****************************************************************************
 * Initial info
 ****************************************************************************/
#define DEFINE_CSC_COEF0(a, b, c) \
		.coef[0][0] = a, .coef[0][1] = b, .coef[0][2] = c,
#define DEFINE_CSC_COEF1(a, b, c) \
		.coef[1][0] = a, .coef[1][1] = b, .coef[1][2] = c,
#define DEFINE_CSC_COEF2(a, b, c) \
		.coef[2][0] = a, .coef[2][1] = b, .coef[2][2] = c,
static struct sclr_csc_matrix csc_mtrx[SCL_CSC_MAX] = {
	// none
	{
		DEFINE_CSC_COEF0(BIT(10),	0,		0)
		DEFINE_CSC_COEF1(0,		BIT(10),	0)
		DEFINE_CSC_COEF2(0,		0,		BIT(10))
		.sub[0] = 0,   .sub[1] = 0,   .sub[2] = 0,
		.add[0] = 0,   .add[1] = 0,   .add[2] = 0
	},
	// yuv2rgb
	// 601 Limited
	//	R = Y				+ 1.402* Pr 		   //
	//	G = Y - 0.344 * Pb	- 0.792* Pr 			//
	//	B = Y + 1.772 * Pb							//
	{
		DEFINE_CSC_COEF0(BIT(10),	0,		1436)
		DEFINE_CSC_COEF1(BIT(10),	BIT(13) | 352,	BIT(13) | 731)
		DEFINE_CSC_COEF2(BIT(10),	1815,		0)
		.sub[0] = 0,   .sub[1] = 128, .sub[2] = 128,
		.add[0] = 0,   .add[1] = 0,   .add[2] = 0
	},
	// 601 Full
	//	R = 1.164 *(Y - 16) 					+ 1.596 *(Cr - 128) 					//
	//	G = 1.164 *(Y - 16) - 0.392 *(Cb - 128) - 0.812 *(Cr - 128) //
	//	B = 1.164 *(Y - 16) + 2.016 *(Cb - 128) 					//
	{
		DEFINE_CSC_COEF0(1192,	0,		1634)
		DEFINE_CSC_COEF1(1192,	BIT(13) | 401,	BIT(13) | 833)
		DEFINE_CSC_COEF2(1192,	2065,		0)
		.sub[0] = 16,  .sub[1] = 128, .sub[2] = 128,
		.add[0] = 0,   .add[1] = 0,   .add[2] = 0
	},
	// 709 Limited
	// R = Y				   + 1.540(Cr – 128)
	// G = Y - 0.183(Cb – 128) – 0.459(Cr – 128)
	// B = Y + 1.816(Cb – 128)
	{
		DEFINE_CSC_COEF0(BIT(10),	0,		1577)
		DEFINE_CSC_COEF1(BIT(10),	BIT(13) | 187,	BIT(13) | 470)
		DEFINE_CSC_COEF2(BIT(10),	1860,		0)
		.sub[0] = 0,   .sub[1] = 128, .sub[2] = 128,
		.add[0] = 0,   .add[1] = 0,   .add[2] = 0
	},
	// 709 Full
	//	R = 1.164 *(Y - 16) 					+ 1.792 *(Cr - 128) 					//
	//	G = 1.164 *(Y - 16) - 0.213 *(Cb - 128) - 0.534 *(Cr - 128) //
	//	B = 1.164 *(Y - 16) + 2.114 *(Cb - 128) 					//
	{
		DEFINE_CSC_COEF0(1192,	0,		1836)
		DEFINE_CSC_COEF1(1192,	BIT(13) | 218,	BIT(13) | 547)
		DEFINE_CSC_COEF2(1192,	2166,		0)
		.sub[0] = 16,  .sub[1] = 128, .sub[2] = 128,
		.add[0] = 0,   .add[1] = 0,   .add[2] = 0
	},
	// rgb2yuv
	// 601 Limited
	//	Y = 0.299 * R + 0.587 * G + 0.114 * B		//
	// Pb =-0.169 * R - 0.331 * G + 0.500 * B		//
	// Pr = 0.500 * R - 0.419 * G - 0.081 * B		//
	{
		DEFINE_CSC_COEF0(306,		601,		117)
		DEFINE_CSC_COEF1(BIT(13)|173,	BIT(13)|339,	512)
		DEFINE_CSC_COEF2(512,		BIT(13)|429,	BIT(13)|83)
		.sub[0] = 0,   .sub[1] = 0,   .sub[2] = 0,
		.add[0] = 0,   .add[1] = 128, .add[2] = 128
	},
	// 601 Full
	//	Y = 16	+ 0.257 * R + 0.504 * g + 0.098 * b //
	// Cb = 128 - 0.148 * R - 0.291 * g + 0.439 * b //
	// Cr = 128 + 0.439 * R - 0.368 * g - 0.071 * b //
	{
		DEFINE_CSC_COEF0(263,		516,		100)
		DEFINE_CSC_COEF1(BIT(13)|152,	BIT(13)|298,	450)
		DEFINE_CSC_COEF2(450,		BIT(13)|377,	BIT(13)|73)
		.sub[0] = 0,   .sub[1] = 0,   .sub[2] = 0,
		.add[0] = 16,  .add[1] = 128, .add[2] = 128
	},
	// 709 Limited
	//	 Y =	   0.2126	0.7152	 0.0722
	//	Cb = 128 - 0.1146  -0.3854	 0.5000
	//	Cr = 128 + 0.5000  -0.4542	-0.0468
	{
		DEFINE_CSC_COEF0(218,		732,		74)
		DEFINE_CSC_COEF1(BIT(13)|117,	BIT(13)|395,	512)
		DEFINE_CSC_COEF2(512,		BIT(13)|465,	BIT(13)|48)
		.sub[0] = 0,   .sub[1] = 0,   .sub[2] = 0,
		.add[0] = 0,   .add[1] = 128, .add[2] = 128
	},
	// 709 Full
	//	Y = 16	+ 0.183 * R + 0.614 * g + 0.062 * b //
	// Cb = 128 - 0.101 * R - 0.339 * g + 0.439 * b //
	// Cr = 128 + 0.439 * R - 0.399 * g - 0.040 * b //
	{
		DEFINE_CSC_COEF0(187,		629,		63)
		DEFINE_CSC_COEF1(BIT(13)|103,	BIT(13)|347,	450)
		DEFINE_CSC_COEF2(450,		BIT(13)|408,	BIT(13)|41)
		.sub[0] = 0,   .sub[1] = 0,   .sub[2] = 0,
		.add[0] = 16,  .add[1] = 128, .add[2] = 128
	},
	{
		DEFINE_CSC_COEF0(BIT(12),	0,		0)
		DEFINE_CSC_COEF1(0,		BIT(12),	0)
		DEFINE_CSC_COEF2(0,		0,		BIT(12))
		.sub[0] = 0,   .sub[1] = 0,   .sub[2] = 0,
		.add[0] = 0,   .add[1] = 0,   .add[2] = 0
	},
};

static int scl_coef_bicubic[128][4] = {
	{-4,	1024,	4,	0},
	{-8,	1024,	8,	0},
	{-12,	1024,	12, 0},
	{-16,	1024,	16, 0},
	{-19,	1023,	20, 0},
	{-23,	1023,	25, -1},
	{-26,	1022,	29, -1},
	{-30,	1022,	33, -1},
	{-34,	1022,	37, -1},
	{-37,	1021,	42, -2},
	{-40,	1020,	46, -2},
	{-44,	1020,	50, -2},
	{-47,	1019,	55, -3},
	{-50,	1018,	59, -3},
	{-53,	1017,	63, -3},
	{-56,	1016,	68, -4},
	{-59,	1015,	72, -4},
	{-62,	1014,	77, -5},
	{-65,	1013,	81, -5},
	{-68,	1012,	86, -6},
	{-71,	1011,	90, -6},
	{-74,	1010,	95, -7},
	{-76,	1008,	100,	-8},
	{-79,	1007,	104,	-8},
	{-81,	1005,	109,	-9},
	{-84,	1004,	113,	-9},
	{-86,	1002,	118,	-10},
	{-89,	1001,	123,	-11},
	{-91,	999,	128,	-12},
	{-94,	998,	132,	-12},
	{-96,	996,	137,	-13},
	{-98,	994,	142,	-14},
	{-100,	992,	147,	-15},
	{-102,	990,	152,	-16},
	{-104,	988,	157,	-17},
	{-106,	986,	161,	-17},
	{-108,	984,	166,	-18},
	{-110,	982,	171,	-19},
	{-112,	980,	176,	-20},
	{-114,	978,	181,	-21},
	{-116,	976,	186,	-22},
	{-117,	973,	191,	-23},
	{-119,	971,	196,	-24},
	{-121,	969,	201,	-25},
	{-122,	966,	206,	-26},
	{-124,	964,	211,	-27},
	{-125,	961,	216,	-28},
	{-127,	959,	221,	-29},
	{-128,	956,	226,	-30},
	{-130,	954,	231,	-31},
	{-131,	951,	237,	-33},
	{-132,	948,	242,	-34},
	{-133,	945,	247,	-35},
	{-134,	942,	252,	-36},
	{-136,	940,	257,	-37},
	{-137,	937,	262,	-38},
	{-138,	934,	267,	-39},
	{-139,	931,	273,	-41},
	{-140,	928,	278,	-42},
	{-141,	925,	283,	-43},
	{-142,	922,	288,	-44},
	{-142,	918,	294,	-46},
	{-143,	915,	299,	-47},
	{-144,	912,	304,	-48},
	{-145,	909,	309,	-49},
	{-145,	905,	315,	-51},
	{-146,	902,	320,	-52},
	{-147,	899,	325,	-53},
	{-147,	895,	330,	-54},
	{-148,	892,	336,	-56},
	{-148,	888,	341,	-57},
	{-149,	885,	346,	-58},
	{-149,	881,	352,	-60},
	{-150,	878,	357,	-61},
	{-150,	874,	362,	-62},
	{-150,	870,	367,	-63},
	{-151,	867,	373,	-65},
	{-151,	863,	378,	-66},
	{-151,	859,	383,	-67},
	{-151,	855,	389,	-69},
	{-151,	851,	394,	-70},
	{-152,	848,	399,	-71},
	{-152,	844,	405,	-73},
	{-152,	840,	410,	-74},
	{-152,	836,	415,	-75},
	{-152,	832,	421,	-77},
	{-152,	828,	426,	-78},
	{-152,	824,	431,	-79},
	{-151,	819,	437,	-81},
	{-151,	815,	442,	-82},
	{-151,	811,	447,	-83},
	{-151,	807,	453,	-85},
	{-151,	803,	458,	-86},
	{-151,	799,	463,	-87},
	{-150,	794,	469,	-89},
	{-150,	790,	474,	-90},
	{-150,	786,	479,	-91},
	{-149,	781,	485,	-93},
	{-149,	777,	490,	-94},
	{-149,	773,	495,	-95},
	{-148,	768,	501,	-97},
	{-148,	764,	506,	-98},
	{-147,	759,	511,	-99},
	{-147,	755,	516,	-100},
	{-146,	750,	522,	-102},
	{-146,	746,	527,	-103},
	{-145,	741,	532,	-104},
	{-144,	736,	537,	-105},
	{-144,	732,	543,	-107},
	{-143,	727,	548,	-108},
	{-142,	722,	553,	-109},
	{-142,	718,	558,	-110},
	{-141,	713,	563,	-111},
	{-140,	708,	569,	-113},
	{-140,	704,	574,	-114},
	{-139,	699,	579,	-115},
	{-138,	694,	584,	-116},
	{-137,	689,	589,	-117},
	{-136,	684,	594,	-118},
	{-135,	679,	600,	-120},
	{-135,	675,	605,	-121},
	{-134,	670,	610,	-122},
	{-133,	665,	615,	-123},
	{-132,	660,	620,	-124},
	{-131,	655,	625,	-125},
	{-130,	650,	630,	-126},
	{-129,	645,	635,	-127},
	{-128,	640,	640,	-128},
};

static int scl_coef_bicubic_opencv[128][4] = {
	{-3, 1024, 3, 0},
	{-6, 1024, 6, 0},
	{-9, 1024, 9, 0},
	{-12, 1023, 12, 0},
	{-14, 1023, 16, 0},
	{-17, 1023, 19, 0},
	{-20, 1022, 22, -1},
	{-23, 1022, 25, -1},
	{-25, 1021, 29, -1},
	{-28, 1021, 32, -1},
	{-30, 1020, 36, -1},
	{-33, 1019, 39, -2},
	{-35, 1018, 43, -2},
	{-38, 1017, 46, -2},
	{-40, 1016, 50, -2},
	{-42, 1015, 54, -3},
	{-44, 1014, 57, -3},
	{-47, 1013, 61, -4},
	{-49, 1012, 65, -4},
	{-51, 1011, 69, -4},
	{-53, 1009, 73, -5},
	{-55, 1008, 77, -5},
	{-57, 1006, 80, -6},
	{-59, 1005, 84, -6},
	{-61, 1003, 88, -7},
	{-63, 1002, 93, -7},
	{-65, 1000, 97, -8},
	{-67, 998, 101, -8},
	{-68, 996, 105, -9},
	{-70, 994, 109, -9},
	{-72, 992, 113, -10},
	{-74, 990, 118, -10},
	{-75, 988, 122, -11},
	{-77, 986, 126, -12},
	{-78, 984, 130, -12},
	{-80, 982, 135, -13},
	{-81, 980, 139, -14},
	{-83, 977, 144, -14},
	{-84, 975, 148, -15},
	{-85, 973, 153, -16},
	{-87, 970, 157, -17},
	{-88, 968, 162, -17},
	{-89, 965, 166, -18},
	{-91, 962, 171, -19},
	{-92, 960, 176, -20},
	{-93, 957, 180, -20},
	{-94, 954, 185, -21},
	{-95, 951, 190, -22},
	{-96, 949, 194, -23},
	{-97, 946, 199, -24},
	{-98, 943, 204, -24},
	{-99, 940, 209, -25},
	{-100, 937, 213, -26},
	{-101, 933, 218, -27},
	{-102, 930, 223, -28},
	{-103, 927, 228, -29},
	{-103, 924, 233, -30},
	{-104, 921, 238, -30},
	{-105, 917, 243, -31},
	{-106, 914, 248, -32},
	{-106, 911, 253, -33},
	{-107, 907, 258, -34},
	{-107, 904, 263, -35},
	{-108, 900, 268, -36},
	{-109, 896, 273, -37},
	{-109, 893, 278, -38},
	{-110, 889, 283, -39},
	{-110, 885, 288, -40},
	{-110, 882, 294, -41},
	{-111, 878, 299, -42},
	{-111, 874, 304, -43},
	{-112, 870, 309, -44},
	{-112, 866, 314, -45},
	{-112, 862, 319, -46},
	{-112, 858, 325, -47},
	{-113, 854, 330, -48},
	{-113, 850, 335, -49},
	{-113, 846, 340, -50},
	{-113, 842, 346, -51},
	{-113, 838, 351, -52},
	{-114, 834, 356, -53},
	{-114, 830, 362, -54},
	{-114, 825, 367, -55},
	{-114, 821, 372, -56},
	{-114, 817, 377, -57},
	{-114, 813, 383, -58},
	{-114, 808, 388, -59},
	{-114, 804, 394, -60},
	{-114, 799, 399, -61},
	{-114, 795, 404, -62},
	{-113, 790, 410, -63},
	{-113, 786, 415, -64},
	{-113, 781, 420, -65},
	{-113, 777, 426, -66},
	{-113, 772, 431, -67},
	{-112, 768, 436, -68},
	{-112, 763, 442, -68},
	{-112, 758, 447, -69},
	{-112, 753, 453, -70},
	{-111, 749, 458, -71},
	{-111, 744, 463, -72},
	{-111, 739, 469, -73},
	{-110, 734, 474, -74},
	{-110, 730, 480, -75},
	{-110, 725, 485, -76},
	{-109, 720, 490, -77},
	{-109, 715, 496, -78},
	{-108, 710, 501, -79},
	{-108, 705, 507, -80},
	{-107, 700, 512, -81},
	{-107, 695, 517, -82},
	{-106, 690, 523, -83},
	{-106, 685, 528, -84},
	{-105, 680, 534, -84},
	{-105, 675, 539, -85},
	{-104, 670, 544, -86},
	{-103, 665, 550, -87},
	{-103, 660, 555, -88},
	{-102, 655, 560, -89},
	{-102, 650, 566, -90},
	{-101, 644, 571, -90},
	{-100, 639, 576, -91},
	{-100, 634, 582, -92},
	{-99, 629, 587, -93},
	{-98, 624, 592, -94},
	{-97, 618, 597, -94},
	{-97, 613, 603, -95},
	{-96, 608, 608, -96},
};

/****************************************************************************
 * Interfaces
 ****************************************************************************/
void sclr_set_base_addr(void *vi_base, void *vd0_base, void *vd1_base, void *vo_base)
{
	reg_base_vi = (uintptr_t)vi_base;
	reg_base_vd0 = (uintptr_t)vd0_base;
	reg_base_vd1 = (uintptr_t)vd1_base;
	reg_base_vo = (uintptr_t)vo_base;
}

void sclr_init_sys_top_addr(void)
{
	top_rst_reg_base = (uintptr_t)ioremap(REG_TOP_RESET_BASE, 0x4);
	vi_sys_reg_base = (uintptr_t)ioremap(REG_VI_STS_BASE, 0x8);
	vo_sys_reg_base = (uintptr_t)ioremap(REG_VO_SYS_BASE, 0x8);
}

void sclr_deinit_sys_top_addr(void)
{
	iounmap((void *)top_rst_reg_base);
	iounmap((void *)vi_sys_reg_base);
	iounmap((void *)vo_sys_reg_base);
}

/**
 * sclr_reg_force_up - trigger reg update by sw.
 *
 */
void sclr_reg_force_up(u8 inst)
{
	_reg_write_mask(reg_base + REG_SCL_TOP_SHD(inst), BIT(0), BIT(0));
}

/**
 * sclr_top_set_cfg - set scl-top's configurations.
 *
 * @param cfg: scl-top's settings.
 */
void sclr_top_set_cfg(u8 inst, bool sc_enable, bool fbd_enable)
{
	union sclr_top_cfg_01 cfg_01;
	u8 qos_en = 0;

	cfg_01.raw = 0;
	cfg_01.b.sc_enable = sc_enable;
	cfg_01.b.fbd_enable = fbd_enable;
	cfg_01.b.sc_debug_en = 1;
	qos_en = BIT(2); //img_in
	if (sc_enable)
		qos_en |= BIT(4);
	if (fbd_enable)
		qos_en |= BIT(7);

	cfg_01.b.qos_en = qos_en;
	_reg_write(reg_base + REG_SCL_TOP_CFG1(inst), cfg_01.raw);
	sclr_top_reg_done(inst);
}

/**
 * sclr_rt_set_cfg - configure sc's channels real-time.
 *
 * @param cfg: sc's channels real-time settings.
 */
void sclr_rt_set_cfg(u8 inst, union sclr_rt_cfg cfg)
{
	_reg_write(reg_base + REG_SCL_TOP_AXI(inst), cfg.raw);
}

union sclr_rt_cfg sclr_rt_get_cfg(u8 inst)
{
	union sclr_rt_cfg cfg;

	cfg.raw = _reg_read(reg_base + REG_SCL_TOP_AXI(inst));
	return cfg;
}

/**
 * sclr_top_reg_done - to mark all sc-reg valid for update.
 *
 */
void sclr_top_reg_done(u8 inst)
{
	_reg_write_mask(reg_base + REG_SCL_TOP_CFG0(inst), BIT(31), BIT(31));
	_reg_write_mask(reg_base + REG_SCL_IMG_CFG(inst), BIT(31), BIT(31));
	_reg_write_mask(reg_base + REG_SCL_CFG(inst), BIT(31), BIT(31)); // clk apb enable

	_reg_write_mask(reg_base + REG_SCL_TOP_CFG0(inst), BIT(0), BIT(0));
}

u8 sclr_top_pg_late_get_bus(u8 inst)
{
	return (_reg_read(reg_base + REG_SCL_TOP_PG(inst)) >> 8) & 0xff;
}

void sclr_top_pg_late_clr(u8 inst)
{
	_reg_write_mask(reg_base + REG_SCL_TOP_PG(inst), 0x0f0000, 0x80000);
}

void sclr_top_get_sb_default(struct sclr_top_sb_cfg *cfg)
{
	memset(cfg, 0, sizeof(*cfg));
}

void sclr_top_set_src_share(u8 inst, bool is_share)
{
	u32 val = is_share ? 0x1 : 0x0;

	_reg_write(reg_base + REG_SCL_TOP_SRC_SHARE(inst), val);
}

/**
 * sclr_update_coef - setup sclr's scaling coef
 *
 * @param inst: (0~3), the instance of scaler which want to be configured.
 */
void sclr_update_coef(u8 inst, enum sclr_algorithm coef)
{
	u8 i = 0;

	if (inst >= SCL_MAX_INST) {
		TRACE_VPSS(DBG_ERR, "inst=%d err.\n", inst);
		return;
	}

	if (g_sc_cfg[inst].coef == coef)
		return;

	if (coef == SCL_COEF_BICUBIC) {
		for (i = 0; i < 128; ++i) {
			_reg_write(reg_base + REG_SCL_COEF1(inst),
				(scl_coef_bicubic[i][1] << 16) |
				(scl_coef_bicubic[i][0] & 0x0fff));
			_reg_write(reg_base + REG_SCL_COEF2(inst),
				(scl_coef_bicubic[i][3] << 16) |
				(scl_coef_bicubic[i][2] & 0x0fff));
			_reg_write(reg_base + REG_SCL_COEF0(inst), (0x5 << 8) | i);
		}
	} else if (coef == SCL_COEF_NEAREST) {
		int nearest_coef[4] = {0, 1024, 0, 0};

		for (i = 0; i < 128; ++i) {
			_reg_write(reg_base + REG_SCL_COEF1(inst),
				   (nearest_coef[1] << 16) |
				   (nearest_coef[0] & 0x0fff));
			_reg_write(reg_base + REG_SCL_COEF2(inst),
				   (nearest_coef[3] << 16) |
				   (nearest_coef[2] & 0x0fff));
			_reg_write(reg_base + REG_SCL_COEF0(inst), (0x5 << 8) | i);
		}
	} else if (coef == SCL_COEF_BILINEAR) {
		int bilinear_coef[4] = {0, 1024, 0, 0};

		for (i = 0; i < 128; ++i) {
			bilinear_coef[1] -= 4;
			bilinear_coef[2] += 4;

			_reg_write(reg_base + REG_SCL_COEF1(inst),
				   (bilinear_coef[1] << 16) |
				   (bilinear_coef[0] & 0x0fff));
			_reg_write(reg_base + REG_SCL_COEF2(inst),
				   (bilinear_coef[3] << 16) |
				   (bilinear_coef[2] & 0x0fff));
			_reg_write(reg_base + REG_SCL_COEF0(inst), (0x5 << 8) | i);
		}
	} else if (coef == SCL_COEF_BICUBIC_OPENCV) {
		for (i = 0; i < 128; ++i) {
			_reg_write(reg_base + REG_SCL_COEF1(inst),
				(scl_coef_bicubic_opencv[i][1] << 16) |
				(scl_coef_bicubic_opencv[i][0] & 0x0fff));
			_reg_write(reg_base + REG_SCL_COEF2(inst),
				(scl_coef_bicubic_opencv[i][3] << 16) |
				(scl_coef_bicubic_opencv[i][2] & 0x0fff));
			_reg_write(reg_base + REG_SCL_COEF0(inst), (0x5 << 8) | i);
		}
	}

	g_sc_cfg[inst].coef = coef;
}

/**
 * sclr_set_cfg - set scl's configurations.
 *
 * @param inst: (0~3), the instance of scaler which want to be configured.
 * @param sc_bypass: if bypass scaling-engine.
 * @param gop_bypass: if bypass gop-engine.
 * @param cir_bypass: if bypass circle-engine.
 * @param odma_bypass: if bypass odma-engine.
 */
void sclr_set_cfg(u8 inst, bool sc_bypass, bool gop_bypass,
		  bool cir_bypass, bool odma_bypass)
{
	g_sc_cfg[inst].sc_bypass = sc_bypass;
	g_sc_cfg[inst].gop_bypass = gop_bypass;
	g_sc_cfg[inst].cir_bypass = cir_bypass;
	g_sc_cfg[inst].odma_bypass = odma_bypass;

	_reg_write(reg_base + REG_SCL_CFG(inst), g_sc_cfg[inst].raw);
}

/**
 * sclr_get_cfg - get scl_core's cfg
 *
 * @param inst: (0~3), the instance of scaler which want to be configured.
 * @return: scl_core's cfg
 */
struct sclr_core_cfg *sclr_get_cfg(u8 inst)
{
	if (inst >= SCL_MAX_INST)
		return NULL;
	return &g_sc_cfg[inst];
}

/**
 * sclr_reg_shadow_sel - control the read reg-bank.
 *
 * @param inst: (0~3), the instance of scaler which want to be configured.
 * @param read_shadow: true(shadow); false(working)
 */
void sclr_reg_shadow_sel(u8 inst, bool read_shadow)
{
	_reg_write(reg_base + REG_SCL_SHD(inst), (read_shadow ? 0x0 : 0x2));
}

struct sclr_status sclr_get_status(u8 inst)
{
	struct sclr_status status;

	u32 tmp = _reg_read(reg_base + REG_SCL_STATUS(inst));

	memcpy(&status, &tmp, sizeof(tmp));

	return status;
}

/**
 * sclr_set_input_size - update scl's input-size.
 *
 * @param inst: (0~3), the instance of scaler which want to be configured.
 * @param src_rect: size of input-src
 */
void sclr_set_input_size(u8 inst, struct sclr_size src_rect, bool update)
{
	_reg_write(reg_base + REG_SCL_SRC_SIZE(inst),
		   ((src_rect.h - 1) << 16) | (src_rect.w - 1));

	if (update)
		g_sc_cfg[inst].sc.src = src_rect;
}

/**
 * sclr_set_crop - update scl's crop-size/position.
 *
 * @param inst: (0~3), the instance of scaler which want to be configured.
 * @param crop_rect: to specify pos/size of crop
 */
void sclr_set_crop(u8 inst, struct sclr_rect crop_rect, bool is_update)
{
	_reg_write(reg_base + REG_SCL_CROP_OFFSET(inst),
		   (crop_rect.y << 16) | crop_rect.x);
	_reg_write(reg_base + REG_SCL_CROP_SIZE(inst),
		   ((crop_rect.h - 1) << 16) | (crop_rect.w - 1));
	if(is_update)
		g_sc_cfg[inst].sc.crop = crop_rect;
}

/**
 * sclr_set_output_size - update scl's output-size.
 *
 * @param inst: (0~3), the instance of scaler which want to be configured.
 * @param src_rect: size of output-src
 */
void sclr_set_output_size(u8 inst, struct sclr_size size)
{
	_reg_write(reg_base + REG_SCL_OUT_SIZE(inst),
		   ((size.h - 1) << 16) | (size.w - 1));

	g_sc_cfg[inst].sc.dst = size;

}

void sclr_set_scale_mir(u8 inst, bool enable)
{
	u32 tmp = 0;

	g_sc_cfg[inst].sc.mir_enable = enable;
	tmp = (enable) ? 0x03 : 0x02;
	// enable cb_mode of scaling, since it only works on fac > 4
	_reg_write_mask(reg_base + REG_SCL_SC_CFG(inst), 0x3, tmp);
}

/**
 * sclr_set_scale_phase - Used in tile mode to decide the phase of first data in scaling.
 *
 * @param inst: (0~3), the instance of scaler which want to be configured.
 * @param h_ph: initial horizontal phase
 * @param v_ph: initial vertical phase
 */
void sclr_set_scale_phase(u8 inst, u32 h_ph, u32 v_ph)
{
	_reg_write(reg_base + REG_SCL_SC_H_INI_PH(inst), h_ph);
	_reg_write(reg_base + REG_SCL_SC_V_INI_PH(inst), v_ph);
}

/**
 * sclr_set_scale_mode - control sclr's scaling coef.
 *
 * @param inst: 0~2
 * @param mir_enable: mirror enable or not
 * @param cb_enable: cb mode or not. cb only work if scaling smaller than 1/4.
 *					 In cb mode, use it's own coeff rather than ones update by sclr_update_coef().
 * @param update: update parameter or not
 */
void sclr_set_scale_mode(u8 inst, bool mir_enable, bool cb_enable, bool update)
{
		u32 tmp = 0;

		if (update) {
				g_sc_cfg[inst].sc.mir_enable = mir_enable;
				//g_sc_cfg[inst].sc.cb_enable = cb_enable;
		}

		// mir/cb isn't suggested to enable together
		if (g_sc_cfg[inst].sc.tile_enable) {
				tmp = mir_enable ? 0x03 : 0x02;
		} else {
				tmp = cb_enable ? 0x13 : 0x03;
		}

		// enable cb_mode of scaling, since it only works on fac > 4
		_reg_write(reg_base + REG_SCL_SC_CFG(inst), tmp);
}

/**
 * sclr_set_scale - update scl's scaling settings by current crop/dst values.
 *
 * @param inst: (0~3), the instance of scaler which want to be configured.
 */
void sclr_set_scale(u8 inst)
{
	u64 fix_fac;
	struct sclr_fac_cfg fac;
	bool is_fac_over4 = false;
	fac.h_pos = 0;
	fac.v_pos = 0;

	if (g_sc_cfg[inst].sc.src.w == 0 || g_sc_cfg[inst].sc.src.h == 0)
		return;
	if (g_sc_cfg[inst].sc.crop.w == 0 || g_sc_cfg[inst].sc.crop.h == 0)
		return;
	if (g_sc_cfg[inst].sc.dst.w == 0 || g_sc_cfg[inst].sc.dst.h == 0)
		return;

	if(g_sc_cfg[inst].sc.algorithm == SCL_COEF_BICUBIC){
		// scale_x, 18.13
		fix_fac = (u64)(g_sc_cfg[inst].sc.crop.w) << 13;
		fac.h_fac = fix_fac / (g_sc_cfg[inst].sc.dst.w);
		is_fac_over4 |= (fac.h_fac >= (4 << 13));
		// scale_y, 18.13
		fix_fac = (u64)(g_sc_cfg[inst].sc.crop.h) << 13;
		fac.v_fac = fix_fac / (g_sc_cfg[inst].sc.dst.h);
		is_fac_over4 |= (fac.v_fac >= (4 << 13));
		sclr_set_scale_mode(inst, g_sc_cfg[inst].sc.mir_enable, is_fac_over4, true);
	} else {
		// scale_x, 8.23
		fix_fac = (u64)(g_sc_cfg[inst].sc.crop.w) << 23;
		fac.h_fac = fix_fac / (g_sc_cfg[inst].sc.dst.w);
		// scale_y, 8.23
		fix_fac = (u64)(g_sc_cfg[inst].sc.crop.h) << 23;
		fac.v_fac = fix_fac / (g_sc_cfg[inst].sc.dst.h);
		//23bit open
		sclr_set_scale_mode(inst, true, false, true);
		_reg_write_mask(reg_base + REG_SCL_SC_CFG(inst), 0xc00, 0xc00);
		if(g_sc_cfg[inst].sc.algorithm != SCL_COEF_NEAREST){
			fac.h_pos = (fac.h_fac >= (1 << 23)) ? ((fac.h_fac - (1 << 23)) >> 1) : (((1 << 23) - fac.h_fac) >> 1);
			fac.v_pos = (fac.v_fac >= (1 << 23)) ? ((fac.v_fac - (1 << 23)) >> 1) : (((1 << 23) - fac.v_fac) >> 1);
			if(fac.h_fac < (1 << 23)){
				_reg_write_mask(reg_base + REG_SCL_SC_CFG(inst), BIT(6), BIT(6));
				fac.h_pos |= BIT(31);
				fac.h_pos = (fac.h_pos ^ 0x7fffffff) + 1;
			}
			if(fac.v_fac < (1 << 23)){
				_reg_write_mask(reg_base + REG_SCL_SC_CFG(inst), BIT(7), BIT(7));
				fac.v_pos |= BIT(31);
				fac.v_pos = (fac.v_pos ^ 0x7fffffff) + 1;
			}
			//lut round
			_reg_write_mask(reg_base + REG_SCL_SC_CFG(inst), 0x300, 0x300);
		} else {
			fac.h_pos = 1 << 22;
			fac.v_pos = 1 << 22;
			_reg_write_mask(reg_base + REG_SCL_SC_CFG(inst), BIT(6), BIT(6));
			fac.h_pos |= BIT(31);
			fac.h_pos = (fac.h_pos ^ 0x7fffffff) + 1;
			_reg_write_mask(reg_base + REG_SCL_SC_CFG(inst), BIT(7), BIT(7));
			fac.v_pos |= BIT(31);
			fac.v_pos = (fac.v_pos ^ 0x7fffffff) + 1;
		}
	}
	_reg_write_mask(reg_base + REG_SCL_SC_H_CFG(inst), 0x7fffffff, fac.h_fac);
	_reg_write_mask(reg_base + REG_SCL_SC_V_CFG(inst), 0x7fffffff, fac.v_fac);
	g_sc_cfg[inst].sc.fac = fac;
	sclr_set_scale_phase(inst, fac.h_pos, fac.v_pos);
}

union sclr_intr sclr_get_intr_mask(u8 inst)
{
	union sclr_intr intr_mask;

	intr_mask.raw = _reg_read(reg_base + REG_SCL_TOP_INTR_MASK(inst));
	return intr_mask;
}

/**
 * sclr_set_intr_mask - sclr's interrupt mask. Only enable ones will be
 *			integrated to vip_subsys.  check 'union sclr_intr'
 *			for each bit mask.
 *
 * @param intr_mask: On/Off ctrl of the interrupt.
 */
void sclr_set_intr_mask(u8 inst, union sclr_intr intr_mask)
{
	_reg_write(reg_base + REG_SCL_TOP_INTR_MASK(inst), intr_mask.raw);
}

/**
 * sclr_intr_ctrl - sclr's interrupt on(1)/off(0)
 *					check 'union sclr_intr' for each bit mask
 *
 * @param intr_mask: On/Off ctrl of the interrupt.
 */
void sclr_intr_ctrl(u8 inst, union sclr_intr intr_mask)
{
	_reg_write(reg_base + REG_SCL_TOP_INTR_ENABLE(inst), intr_mask.raw);
}

/**
 * sclr_intr_clr - clear sclr's interrupt
 *				   check 'union sclr_intr' for each bit mask
 *
 * @param intr_mask: On/Off ctrl of the interrupt.
 */
void sclr_intr_clr(u8 inst, union sclr_intr intr_mask)
{
	_reg_write(reg_base + REG_SCL_TOP_INTR_STATUS(inst), intr_mask.raw);
	if(g_fbd_cfg[inst].enable){
		_reg_write_mask(reg_base + REG_SCL_MAP_CONV_STATUS(inst), BIT(0), 0);
	}
}

/**
 * sclr_intr_status - sclr's interrupt status
 *					  check 'union sclr_intr' for each bit mask
 *
 * @return: The interrupt's status
 */
union sclr_intr sclr_intr_status(u8 inst)
{
	union sclr_intr status;

	status.raw = (_reg_read(reg_base + REG_SCL_TOP_INTR_STATUS(inst)) & 0xffffffff);
	return status;
}

/****************************************************************************
 * SCALER IMG
 ****************************************************************************/
/**
 * sclr_img_set_cfg - set img's configurations.
 *
 * @param img_inst: (0~1), the instance of img-in which want to be configured.
 * @param cfg: img's settings.
 */
void sclr_img_set_cfg(u8 img_inst, struct sclr_img_cfg *cfg)
{
	enum sclr_format in_fmt = cfg->fmt;
	u8 y_buf_thre = 0x80;
	u8 c_buf_thre;

	_reg_write(reg_base + REG_SCL_IMG_CFG(img_inst),
		   (cfg->csc_en << 12) | (cfg->burst << 8) |
		   (in_fmt << 4) | cfg->src);

	if ((cfg->fmt == SCL_FMT_YUV420) || (cfg->fmt == SCL_FMT_NV12) || (cfg->fmt == SCL_FMT_NV21))
		c_buf_thre = y_buf_thre / 2;
	else
		c_buf_thre = y_buf_thre;

	_reg_write_mask(reg_base + REG_SCL_IMG_FIFO_THR(img_inst), 0xff00ff, ((c_buf_thre << 16) | y_buf_thre));
	_reg_write(reg_base + REG_SCL_IMG_OUTSTANDING(img_inst), 0xf);
/*
	_reg_write_mask(reg_base + REG_SCL_IMG_FIFO_THR(img_inst), 0xff00ff, ((cfg->fifo_c) << 16 | (cfg->fifo_y)));
	_reg_write_mask(reg_base + REG_SCL_IMG_OUTSTANDING(img_inst), 0xf, cfg->outstanding);
*/
	sclr_img_set_mem(img_inst, &cfg->mem, true);

	if (cfg->src == SCL_INPUT_ISP && !(g_fbd_cfg[img_inst].enable))
		sclr_img_set_trig(img_inst, SCL_IMG_TRIG_SRC_ISP);
	else if (cfg->src == SCL_INPUT_MEM)
		sclr_img_set_trig(img_inst, SCL_IMG_TRIG_SRC_SW);
	else if (cfg->src == SCL_INPUT_SHARE)
		sclr_img_set_trig(img_inst, SCL_IMG_TRIG_SRC_ISP);

	if (cfg->csc == SCL_CSC_NONE) {
		sclr_img_csc_en(img_inst, false);
	} else {
		sclr_img_csc_en(img_inst, true);
		sclr_img_set_csc(img_inst, &csc_mtrx[cfg->csc]);
	}

	sclr_img_dup2fancy_bypass(img_inst, cfg->dup2fancy_enable);

	g_img_cfg[img_inst] = *cfg;
}

/**
 * sclr_img_set_trig - set img's src of job_start.
 *
 * @param inst: (0~1), the instance of img-in which want to be configured.
 * @param trig_src: img's src of job_start.
 */
void sclr_img_set_trig(u8 inst, enum sclr_img_trig_src trig_src)
{
	u32 mask = BIT(13) | BIT(9);
	u32 val = 0;

	switch (trig_src) {
	case SCL_IMG_TRIG_SRC_SW:
		break;
	case SCL_IMG_TRIG_SRC_DISP:
		val = BIT(9);
		break;
	case SCL_IMG_TRIG_SRC_ISP:
		val = BIT(13);
		break;
	default:
		return;
	}
	_reg_write_mask(reg_base + REG_SCL_TOP_IMG_CTRL(inst), mask, val);
}

/**
 * sclr_img_get_cfg - get scl_img's cfg
 *
 * @param inst: 0~7
 * @return: scl_img's cfg
 */
struct sclr_img_cfg *sclr_img_get_cfg(u8 inst)
{
	if (inst >= VPSS_MAX)
		return NULL;
	return &g_img_cfg[inst];
}

/**
 * sclr_fbd_get_cfg - get scl_fbd's cfg
 *
 * @param inst: 0~7
 * @return: scl_3fbd's cfg
 */
struct sclr_fbd_cfg *sclr_fbd_get_cfg(u8 inst)
{
	if (inst >= VPSS_MAX)
		return NULL;
	return &g_fbd_cfg[inst];
}

/**
 * sclr_img_reg_shadow_sel - control scl_img's the read reg-bank.
 *
 * @param read_shadow: true(shadow); false(working)
 */
void sclr_img_reg_shadow_sel(u8 inst, bool read_shadow)
{
	_reg_write(reg_base + REG_SCL_IMG_SHD(inst), (read_shadow ? 0x0 : 0x4));
}

/**
 * sclr_img_reg_shadow_mask - reg won't be update by sw/hw until unmask.
 *
 * @param mask: true(mask); false(unmask)
 * @return: mask status before this modification.
 */
bool sclr_img_reg_shadow_mask(u8 inst, bool mask)
{
	bool is_masked = (_reg_read(reg_base + REG_SCL_IMG_SHD(inst)) & BIT(1));

	if (is_masked != mask)
		_reg_write_mask(reg_base + REG_SCL_IMG_SHD(inst), BIT(1),
				(mask ? 0x0 : BIT(1)));

	return is_masked;
}

void vpss_v_sw_top_reset(u8 inst)
{
	_reg_write_mask(vi_sys_reg_base, reset_mask[inst], reset_mask[inst]);// sw reset
	_reg_write_mask(vi_sys_reg_base + 0x4, reset_apb_mask[inst], reset_apb_mask[inst]);// apb reset
	_reg_write_mask(vi_sys_reg_base, reset_mask[inst], 0);
	_reg_write_mask(vi_sys_reg_base + 0x4, reset_apb_mask[inst], 0);

	return;
}

void vpss_d_sw_top_reset(u8 inst)
{
	_reg_write_mask(vo_sys_reg_base, reset_mask[inst], reset_mask[inst]);// sw reset
	_reg_write_mask(vo_sys_reg_base + 0x4, reset_mask[inst], reset_mask[inst]);// apb reset
	_reg_write_mask(vo_sys_reg_base, reset_mask[inst], 0);
	_reg_write_mask(vo_sys_reg_base + 0x4, reset_mask[inst], 0);

	return;
}

void vpss_t_sw_top_reset(u8 inst)
{
	_reg_write_mask(top_rst_reg_base, reset_mask[inst], 0);// sw + apb reset
	_reg_write_mask(top_rst_reg_base, reset_mask[inst], reset_mask[inst]);

	return;
}

void sclr_vpss_sw_top_reset(u8 inst)
{
	if(inst <= VPSS_V3)
		vpss_v_sw_top_reset(inst);
	else if(inst <= VPSS_T3)
		vpss_t_sw_top_reset(inst);
	else
		vpss_d_sw_top_reset(inst);

	sclr_ctrl_init(inst, false);

	TRACE_VPSS(DBG_DEBUG, "reset(%d)\n", inst);
}

void sclr_img_reset(u8 inst)
{
	_reg_write(reg_base + REG_SCL_IMG_DBG(inst), 0xfffff); // img reset
	if(inst > VPSS_V3)
		_reg_write_mask(reg_base + REG_SCL_MAP_CONV_CTRL(inst), BIT(2), BIT(2)); // fbd reset
}

void sclr_img_start(u8 inst)
{
	_reg_write(reg_base + REG_SCL_IMG_DBG(inst), 0xfffff); // img reset

	if((inst > VPSS_V3) && (_reg_read(reg_base + REG_SCL_MAP_CONV_CTRL(inst)) & BIT(2)))
		_reg_write_mask(reg_base + REG_SCL_MAP_CONV_CTRL(inst), BIT(2), 0); // fbd init

	_reg_write_mask(reg_base + REG_SCL_TOP_IMG_CTRL(inst), BIT(1), BIT(1)); // sc start

	if(g_fbd_cfg[inst].enable){
		_reg_write_mask(reg_base + REG_SCL_MAP_CONV_CTRL(inst), BIT(0), BIT(0)); // fbd start
	}
}

void sclr_slave_ready(u8 inst)
{
	sclr_reg_force_up(inst);
}

/**
 * sclr_img_set_fmt - set scl_img's input data format
 *
 * @param inst: 0~1
 * @param fmt: scl_img's input format
 */
void sclr_img_set_fmt(u8 inst, enum sclr_format fmt)
{
	_reg_write_mask(reg_base + REG_SCL_IMG_CFG(inst), 0x000000f0, fmt << 4);

	g_img_cfg[inst].fmt = fmt;
}

/**
 * sclr_img_set_mem - setup img's mem settings. Only work if img from mem.
 *
 * @param mem: mem settings for img
 */
void sclr_img_set_mem(u8 inst, struct sclr_mem *mem, bool update)
{
	_reg_write(reg_base + REG_SCL_IMG_OFFSET(inst),
		   (mem->start_y << 16) | mem->start_x);
	_reg_write(reg_base + REG_SCL_IMG_SIZE(inst),
		   ((mem->height - 1) << 16) | (mem->width - 1));
	_reg_write(reg_base + REG_SCL_IMG_PITCH_Y(inst), mem->pitch_y);
	_reg_write(reg_base + REG_SCL_IMG_PITCH_C(inst), mem->pitch_c);

	sclr_img_set_addr(inst, mem->addr0, mem->addr1, mem->addr2);

	if (update)
		g_img_cfg[inst].mem = *mem;
}

/**
 * sclr_img_set_addr - setup img's mem address. Only work if img from mem.
 *
 * @param addr0: address of planar0
 * @param addr1: address of planar1
 * @param addr2: address of planar2
 */
void sclr_img_set_addr(u8 inst, u64 addr0, u64 addr1, u64 addr2)
{
	_reg_write(reg_base + REG_SCL_IMG_ADDR0_L(inst), addr0);
	_reg_write(reg_base + REG_SCL_IMG_ADDR0_H(inst), addr0 >> 32);
	_reg_write(reg_base + REG_SCL_IMG_ADDR1_L(inst), addr1);
	_reg_write(reg_base + REG_SCL_IMG_ADDR1_H(inst), addr1 >> 32);
	_reg_write(reg_base + REG_SCL_IMG_ADDR2_L(inst), addr2);
	_reg_write(reg_base + REG_SCL_IMG_ADDR2_H(inst), addr2 >> 32);

	g_img_cfg[inst].mem.addr0 = addr0;
	g_img_cfg[inst].mem.addr1 = addr1;
	g_img_cfg[inst].mem.addr2 = addr2;
}

void sclr_img_csc_en(u8 inst, bool enable)
{
	_reg_write_mask(reg_base + REG_SCL_IMG_CFG(inst), BIT(12),
			enable ? BIT(12) : 0);
}

/**
 * sclr_img_set_csc - configure scl-img's input CSC's coefficient/offset
 *
 * @param inst: (0~1)
 * @param cfg: The settings for CSC
 */
void sclr_img_set_csc(u8 inst, struct sclr_csc_matrix *cfg)
{
	_reg_write(reg_base + REG_SCL_IMG_CSC_COEF0(inst),
		   (cfg->coef[0][1] << 16) | cfg->coef[0][0]);
	_reg_write(reg_base + REG_SCL_IMG_CSC_COEF1(inst),
		   cfg->coef[0][2]);
	_reg_write(reg_base + REG_SCL_IMG_CSC_COEF2(inst),
		   (cfg->coef[1][1] << 16) | cfg->coef[1][0]);
	_reg_write(reg_base + REG_SCL_IMG_CSC_COEF3(inst), cfg->coef[1][2]);
	_reg_write(reg_base + REG_SCL_IMG_CSC_COEF4(inst),
		   (cfg->coef[2][1] << 16) | cfg->coef[2][0]);
	_reg_write(reg_base + REG_SCL_IMG_CSC_COEF5(inst), cfg->coef[2][2]);
	_reg_write(reg_base + REG_SCL_IMG_CSC_SUB(inst),
		   (cfg->sub[2] << 16) | (cfg->sub[1] << 8) | cfg->sub[0]);
	_reg_write(reg_base + REG_SCL_IMG_CSC_ADD(inst),
		   (cfg->add[2] << 16) | (cfg->add[1] << 8) | cfg->add[0]);
}

union sclr_img_dbg_status sclr_img_get_dbg_status(u8 inst, bool clr)
{
	union sclr_img_dbg_status status;

	status.raw = _reg_read(reg_base + REG_SCL_IMG_DBG(inst));

	if (clr) {
		status.b.err_fwr_clr = 1;
		status.b.err_erd_clr = 1;
		status.b.ip_clr = 1;
		status.b.ip_int_clr = 1;
		_reg_write(reg_base + REG_SCL_IMG_DBG(inst), status.raw);
	}

	return status;
}

void sclr_img_checksum_en(u8 inst, bool enable)
{
	_reg_write_mask(reg_base + REG_SCL_IMG_CHECKSUM0(inst), BIT(31),
					enable ? BIT(31) : 0);
}

void sclr_img_get_checksum_status(u8 inst, struct sclr_img_checksum_status *status)
{
	status->checksum_base.raw = _reg_read(reg_base + REG_SCL_IMG_CHECKSUM0(inst));
	status->axi_read_data = _reg_read(reg_base + REG_SCL_IMG_CHECKSUM1(inst));
}

int sclr_img_validate_cb_cfg(struct sclr_img_in_sb_cfg *cfg)
{
	/* 0 : disable, 1 : free-run mode, 2 : frame base mode */
	if (cfg->sb_mode > 2)
		return -1;

	/* 0 : 64 line, 1 : 128 line, 2 : 192 line, 3 : 256 line */
	if (cfg->sb_size > 3)
		return -1;

	return 0;
}

void sclr_img_get_sb_default(struct sclr_img_in_sb_cfg *cfg)
{
	memset(cfg, 0, sizeof(*cfg));
	//cfg->mode = 0;	// disable
	//cfg->size = 0;	// 64 line
	cfg->sb_nb = 3;
	cfg->sb_sw_rptr = 0;
	//cfg->set_str = 0;
	//cfg->sw_clr = 0;
}

/**
 * sclr_img_dup2fancy_bypass - dup2fancy bypass
 *
 */
void sclr_img_dup2fancy_bypass(u8 inst, bool enable)
{
	//default 0: enable dup2fancy   1: disable dup2fancy
	_reg_write_mask(reg_base + REG_SCL_IMG_DUP2FANCY(inst), BIT(0),
					enable ? 0 : BIT(0));
}

/****************************************************************************
 * SCALER CIRCLE
 ****************************************************************************/
/**
 * sclr_cir_set_cfg - configure scl-circle
 *
 * @param inst: (0~3), the instance of scaler which want to be configured.
 * @param cfg: cir's settings.
 */
void sclr_cir_set_cfg(u8 inst, struct sclr_cir_cfg *cfg)
{
	if (cfg->mode == SCL_CIR_DISABLE) {
		_reg_write_mask(reg_base + REG_SCL_CFG(inst), 0x20, 0x20);
	} else {
		_reg_write(reg_base + REG_SCL_CIR_CFG(inst),
			   (cfg->line_width << 8) | cfg->mode);
		_reg_write(reg_base + REG_SCL_CIR_CENTER_X(inst),
			   cfg->center.x);
		_reg_write(reg_base + REG_SCL_CIR_CENTER_Y(inst),
			   cfg->center.y);
		_reg_write(reg_base + REG_SCL_CIR_RADIUS(inst), cfg->radius);
		_reg_write(reg_base + REG_SCL_CIR_SIZE(inst),
			   ((cfg->rect.h - 1) << 16) | (cfg->rect.w - 1));
		_reg_write(reg_base + REG_SCL_CIR_OFFSET(inst),
			   (cfg->rect.y << 16) | cfg->rect.x);
		_reg_write(reg_base + REG_SCL_CIR_COLOR(inst),
			   (cfg->color_b << 16) | (cfg->color_g << 8) |
			   cfg->color_r);

		_reg_write_mask(reg_base + REG_SCL_CFG(inst), 0x20, 0x00);
	}

	g_cir_cfg[inst] = *cfg;
}

/****************************************************************************
 * SCALER BORDER
 ****************************************************************************/
/**
 * sclr_border_set_cfg - configure scaler's odma
 *
 * @param inst: (0~3), the instance of scaler which want to be configured.
 * @param cfg: new border config
 */
void sclr_border_set_cfg(u8 inst, struct sclr_border_cfg *cfg)
{
	if ((g_sc_cfg[inst].sc.dst.w + cfg->start.x) >
		g_odma_cfg[inst].mem.width) {
		TRACE_VPSS(DBG_DEBUG, "[sc%d] sc_width(%d) + offset(%d) > odma_width(%d)\n",
			   inst, g_sc_cfg[inst].sc.dst.w, cfg->start.x, g_odma_cfg[inst].mem.width);
		cfg->start.x = g_odma_cfg[inst].mem.width - g_sc_cfg[inst].sc.dst.w;
	}

	if ((g_sc_cfg[inst].sc.dst.h + cfg->start.y) >
		g_odma_cfg[inst].mem.height) {
		TRACE_VPSS(DBG_DEBUG, "[sc%d] sc_height(%d) + offset(%d) > odma_height(%d)\n",
			   inst, g_sc_cfg[inst].sc.dst.h, cfg->start.y, g_odma_cfg[inst].mem.height);
		cfg->start.y = g_odma_cfg[inst].mem.height - g_sc_cfg[inst].sc.dst.h;
	}

	_reg_write(reg_base + REG_SCL_BORDER_CFG(inst), cfg->cfg.raw);
	_reg_write(reg_base + REG_SCL_BORDER_OFFSET(inst),
		   (cfg->start.y << 16) | cfg->start.x);

	g_bd_cfg[inst] = *cfg;
}

void sclr_border_vpp_set_cfg(u8 inst, u8 border_idx, struct sclr_border_vpp_cfg *cfg, bool update)
{
	if(cfg->inside_start.x > cfg->inside_end.x || cfg->inside_start.y > cfg->inside_end.y){
		TRACE_VPSS(DBG_WARN, "[sc%d] border_idx(%d), inside(%d %d %d %d)\n", inst, border_idx,
				cfg->inside_start.x, cfg->inside_start.y, cfg->inside_end.x, cfg->inside_end.y);
	}
	if(cfg->outside_start.x > cfg->outside_end.x || cfg->outside_start.y > cfg->outside_end.y){
		TRACE_VPSS(DBG_WARN, "[sc%d] idx(%d), outside(%d %d %d %d)\n", inst, border_idx,
				cfg->outside_start.x, cfg->outside_start.y, cfg->outside_end.x, cfg->outside_end.y);
	}
	_reg_write(reg_base + REG_SCL_BORDER_VPP_CFG(inst, border_idx), cfg->cfg.raw);
	_reg_write(reg_base + REG_SCL_BORDER_VPP_INX(inst, border_idx),
		   (cfg->inside_end.x << 16) | cfg->inside_start.x);
	_reg_write(reg_base + REG_SCL_BORDER_VPP_INY(inst, border_idx),
		   (cfg->inside_end.y << 16) | cfg->inside_start.y);
	_reg_write(reg_base + REG_SCL_BORDER_VPP_OUTX(inst, border_idx),
		   (cfg->outside_end.x << 16) | cfg->outside_start.x);
	_reg_write(reg_base + REG_SCL_BORDER_VPP_OUTY(inst, border_idx),
		   (cfg->outside_end.y << 16) | cfg->outside_start.y);
	if(update)
		g_bd_vpp_cfg[inst][border_idx] = *cfg;
}

struct sclr_border_vpp_cfg * sclr_border_vpp_get_cfg(u8 inst, u8 border_idx)
{
	if (inst >= SCL_MAX_INST)
		return NULL;
	return &g_bd_vpp_cfg[inst][border_idx];
}

/**
 * sclr_border_get_cfg - get scl_border's cfg
 *
 * @param inst: (0~3), the instance of scaler which want to be configured.
 * @return: scl_border's cfg
 */
struct sclr_border_cfg *sclr_border_get_cfg(u8 inst)
{
	if (inst >= SCL_MAX_INST)
		return NULL;
	return &g_bd_cfg[inst];
}

/****************************************************************************
 * SCALER ODMA
 ****************************************************************************/
/**
 * sclr_odma_set_cfg - configure scaler's odma
 *
 * @param inst: (0~3), the instance of scaler which want to be configured.
 * @param burst: dma's burst length
 * @param fmt: dma's format
 */
void sclr_odma_set_cfg(u8 inst, struct sclr_odma_cfg *cfg)
{
	enum sclr_format odma_fmt = cfg->fmt;

	_reg_write(reg_base + REG_SCL_ODMA_CFG(inst),
		   (cfg->flip << 16) |	(odma_fmt << 8) | cfg->burst | BIT(1));

	sclr_odma_set_mem(inst, &cfg->mem);
	sclr_ctrl_set_output(inst, &cfg->csc_cfg, cfg->fmt);

	g_odma_cfg[inst] = *cfg;
}

/**
 * sclr_odma_get_cfg - get scl_odma's cfg
 *
 * @param inst: (0~3), the instance of scaler which want to be configured.
 * @return: scl_odma's cfg
 */
struct sclr_odma_cfg *sclr_odma_get_cfg(u8 inst)
{
	if (inst >= SCL_MAX_INST)
		return NULL;
	return &g_odma_cfg[inst];
}

/**
 * sclr_odma_set_fmt - set scl_odma's output data format
 *
 * @param inst: (0~3), the instance of scaler which want to be configured.
 * @param fmt: scl_odma's output format
 */
void sclr_odma_set_fmt(u8 inst, enum sclr_format fmt)
{
	u32 tmp = fmt << 8;

	if (fmt == SCL_FMT_BF16)
		tmp |= BIT(23);
	_reg_write_mask(reg_base + REG_SCL_ODMA_CFG(inst), 0x0080ff00, tmp);

	g_odma_cfg[inst].fmt = fmt;
}

/**
 * sclr_odma_set_mem - setup odma's mem settings.
 *
 * @param mem: mem settings for odma
 */
void sclr_odma_set_mem(u8 inst, struct sclr_mem *mem)
{
	_reg_write(reg_base + REG_SCL_ODMA_OFFSET_X(inst), mem->start_x);
	_reg_write(reg_base + REG_SCL_ODMA_OFFSET_Y(inst), mem->start_y);
	_reg_write(reg_base + REG_SCL_ODMA_WIDTH(inst), mem->width - 1);
	_reg_write(reg_base + REG_SCL_ODMA_HEIGHT(inst), mem->height - 1);
	_reg_write(reg_base + REG_SCL_ODMA_PITCH_Y(inst), mem->pitch_y);
	_reg_write(reg_base + REG_SCL_ODMA_PITCH_C(inst), mem->pitch_c);

	sclr_odma_set_addr(inst, mem->addr0, mem->addr1, mem->addr2);

	g_odma_cfg[inst].mem = *mem;
}

/**
 * sclr_odma_set_addr - setup odma's mem address.
 *
 * @param addr0: address of planar0
 * @param addr1: address of planar1
 * @param addr2: address of planar2
 */
void sclr_odma_set_addr(u8 inst, u64 addr0, u64 addr1, u64 addr2)
{
	_reg_write(reg_base + REG_SCL_ODMA_ADDR0_L(inst), addr0);
	_reg_write(reg_base + REG_SCL_ODMA_ADDR0_H(inst), addr0 >> 32);
	_reg_write(reg_base + REG_SCL_ODMA_ADDR1_L(inst), addr1);
	_reg_write(reg_base + REG_SCL_ODMA_ADDR1_H(inst), addr1 >> 32);
	_reg_write(reg_base + REG_SCL_ODMA_ADDR2_L(inst), addr2);
	_reg_write(reg_base + REG_SCL_ODMA_ADDR2_H(inst), addr2 >> 32);

	g_odma_cfg[inst].mem.addr0 = addr0;
	g_odma_cfg[inst].mem.addr1 = addr1;
	g_odma_cfg[inst].mem.addr2 = addr2;
}

union sclr_odma_dbg_status sclr_odma_get_dbg_status(u8 inst)
{
	union sclr_odma_dbg_status status;

	status.raw = _reg_read(reg_base + REG_SCL_ODMA_DBG(inst));

	return status;
}

/**
 * sclr_set_csc_ctrl - configure scl's output CSC's, including hsv/quant/coefficient/offset
 *
 * @param inst: (0~3), the instance of scaler which want to be configured.
 * @param cfg: The settings for CSC
 */
void sclr_set_csc_ctrl(u8 inst, struct sclr_csc_cfg *cfg)
{
	u32 tmp = 0;

	if(cfg->datatype != SCL_DATATYPE_DISABLE){
		// tmp |= BIT(11) | BIT(1) | BIT(0);
		// sclr_set_csc(inst, &csc_mtrx[SCL_CSC_DATATYPE]);
		tmp |= BIT(1);
	}

	if (cfg->mode == SCL_OUT_HSV) {
		tmp |= BIT(4);
		if(cfg->datatype != SCL_DATATYPE_DISABLE){
			TRACE_VPSS(DBG_WARN, "hsv can not enable in fp32/fp16/bf16/int8\n");
		}
		if (cfg->hsv_round == SCL_HSV_ROUNDING_TOWARD_ZERO)
			tmp |= BIT(5);
		if (!cfg->work_on_border)
			tmp |= BIT(9);
		sclr_set_csc(inst, &csc_mtrx[SCL_CSC_NONE]);
	} else if (cfg->mode == SCL_OUT_QUANT) {
		tmp |= (cfg->quant_round << 2) | BIT(1) | BIT(0);
		if (!cfg->work_on_border)
			tmp |= BIT(8);
		if (cfg->quant_gain_mode == SCL_QUANT_GAIN_MODE_2)
			tmp |= BIT(11);
		if(cfg->datatype == SCL_DATATYPE_BF16)
			if (cfg->quant_border_type == SCL_QUANT_BORDER_TYPE_127)
				tmp |= BIT(10);
		sclr_set_quant(inst, &cfg->quant_form);
	} else if (cfg->mode == SCL_OUT_CONVERT_TO) {
		tmp |= BIT(11) | BIT(1) | BIT(0);
		sclr_set_csc(inst, &csc_mtrx[SCL_CSC_DATATYPE]);
		sclr_set_convert_to(inst, &cfg->convert_to_cfg);
	} else if (cfg->mode == SCL_OUT_CSC){
		tmp |= BIT(0);
		if (!cfg->work_on_border)
			tmp |= BIT(8);
		// sclr_set_csc(inst, &csc_mtrx[cfg->csc_type]);
	} else
		if (!cfg->work_on_border)
			tmp |= BIT(8);

	_reg_write_mask(reg_base + REG_SCL_CSC_EN(inst), 0x00000fff, tmp);

	if (cfg->datatype == SCL_DATATYPE_BF16) {
		_reg_write_mask(reg_base + REG_SCL_ODMA_CFG(inst), 0x0F800000, BIT(23));
	} else if (cfg->datatype == SCL_DATATYPE_FP16) {
		_reg_write_mask(reg_base + REG_SCL_ODMA_CFG(inst), 0x0F800000, BIT(24));
	} else if (cfg->datatype == SCL_DATATYPE_FP32) {
		_reg_write_mask(reg_base + REG_SCL_ODMA_CFG(inst), 0x0F800000, BIT(25));
	} else if (cfg->datatype == SCL_DATATYPE_INT8) {
		_reg_write_mask(reg_base + REG_SCL_ODMA_CFG(inst), 0x0F800000, BIT(26));
	} else if (cfg->datatype == SCL_DATATYPE_U8) {
		_reg_write_mask(reg_base + REG_SCL_ODMA_CFG(inst), 0x0F800000, BIT(27));
	}

	g_odma_cfg[inst].csc_cfg = *cfg;
}

struct sclr_csc_cfg *sclr_get_csc_ctrl(u8 inst)
{
	if (inst < SCL_MAX_INST)
		return &g_odma_cfg[inst].csc_cfg;

	return NULL;
}

/**
 * sclr_set_csc - configure scl's output CSC's coefficient/offset
 *
 * @param inst: (0~3), the instance of scaler which want to be configured.
 * @param cfg: The settings for CSC
 */
void sclr_set_csc(u8 inst, struct sclr_csc_matrix *cfg)
{
	_reg_write(reg_base + REG_SCL_CSC_COEF0(inst),
		   (cfg->coef[0][1] << 16) | cfg->coef[0][0]);
	_reg_write(reg_base + REG_SCL_CSC_COEF1(inst),
		   (cfg->coef[1][0] << 16) | cfg->coef[0][2]);
	_reg_write(reg_base + REG_SCL_CSC_COEF2(inst),
		   (cfg->coef[1][2] << 16) | cfg->coef[1][1]);
	_reg_write(reg_base + REG_SCL_CSC_COEF3(inst),
		   (cfg->coef[2][1] << 16) | cfg->coef[2][0]);
	_reg_write(reg_base + REG_SCL_CSC_COEF4(inst), cfg->coef[2][2]);

	_reg_write(reg_base + REG_SCL_CSC_OFFSET(inst),
		   (cfg->add[2] << 16) | (cfg->add[1] << 8) | cfg->add[0]);
	_reg_write(reg_base + REG_SCL_CSC_FRAC0(inst), 0);
	_reg_write(reg_base + REG_SCL_CSC_FRAC1(inst), 0);
}

/**
 * sclr_set_quant - set quantization by csc module
 *
 * @param inst: (0~3), the instance of scaler which want to be configured.
 * @param cfg: The settings of quantization
 */
void sclr_set_quant(u8 inst, struct sclr_quant_formula *cfg)
{
	_reg_write(reg_base + REG_SCL_CSC_COEF0(inst), cfg->sc_frac[0]);
	_reg_write(reg_base + REG_SCL_CSC_COEF1(inst), 0);
	_reg_write(reg_base + REG_SCL_CSC_COEF2(inst), cfg->sc_frac[1]);
	_reg_write(reg_base + REG_SCL_CSC_COEF3(inst), 0);
	_reg_write(reg_base + REG_SCL_CSC_COEF4(inst), cfg->sc_frac[2]);

	_reg_write(reg_base + REG_SCL_CSC_OFFSET(inst), (cfg->sub[2] << 16) | (cfg->sub[1] << 8) | cfg->sub[0]);
	_reg_write(reg_base + REG_SCL_CSC_FRAC0(inst), (cfg->sub_frac[1] << 16) | cfg->sub_frac[0]);
	_reg_write(reg_base + REG_SCL_CSC_FRAC1(inst), cfg->sub_frac[2]);
}

void sclr_set_convert_to(u8 inst, struct sclr_convertto_formula *cfg)
{
	_reg_write_mask(reg_base + REG_SCL_ODMA_CFG(inst), BIT(28), BIT(28));
	_reg_write(reg_base + REG_SCL_CONVERT_TO_A0(inst), cfg->a_frac[0]);
	_reg_write(reg_base + REG_SCL_CONVERT_TO_A1(inst), cfg->a_frac[1]);
	_reg_write(reg_base + REG_SCL_CONVERT_TO_A2(inst), cfg->a_frac[2]);
	_reg_write(reg_base + REG_SCL_CONVERT_TO_B0(inst), cfg->b_frac[0]);
	_reg_write(reg_base + REG_SCL_CONVERT_TO_B1(inst), cfg->b_frac[1]);
	_reg_write(reg_base + REG_SCL_CONVERT_TO_B2(inst), cfg->b_frac[2]);
	g_odma_cfg[inst].csc_cfg.convert_to_cfg = *cfg;
}

/**
 * sclr_get_csc - get current CSC's coefficient/offset
 *
 * @param inst: (0~3), the instance of scaler which want to be configured.
 * @param cfg: The settings of CSC
 */
void sclr_get_csc(u8 inst, struct sclr_csc_matrix *cfg)
{
	u32 tmp;

	tmp = _reg_read(reg_base + REG_SCL_CSC_COEF0(inst));
	cfg->coef[0][0] = tmp;
	cfg->coef[0][1] = tmp >> 16;
	tmp = _reg_read(reg_base + REG_SCL_CSC_COEF1(inst));
	cfg->coef[0][2] = tmp;
	cfg->coef[1][0] = tmp >> 16;
	tmp = _reg_read(reg_base + REG_SCL_CSC_COEF2(inst));
	cfg->coef[1][1] = tmp;
	cfg->coef[1][2] = tmp >> 16;
	tmp = _reg_read(reg_base + REG_SCL_CSC_COEF3(inst));
	cfg->coef[2][0] = tmp;
	cfg->coef[2][1] = tmp >> 16;
	tmp = _reg_read(reg_base + REG_SCL_CSC_COEF4(inst));
	cfg->coef[2][2] = tmp;

	tmp = _reg_read(reg_base + REG_SCL_CSC_OFFSET(inst));
	cfg->add[0] = tmp;
	cfg->add[1] = tmp >> 8;
	cfg->add[2] = tmp >> 16;
	memset(cfg->sub, 0, sizeof(cfg->sub));
}

void sclr_core_set_cfg(u8 inst, struct sclr_core_cfg *cfg)
{
	sclr_ctrl_set_scale(inst, &cfg->sc);
	sclr_update_coef(inst, cfg->sc.algorithm);
}

void sclr_core_checksum_en(u8 inst, bool enable)
{
	_reg_write_mask(reg_base + REG_SCL_CHECKSUM0(inst), BIT(31),
					enable ? BIT(31) : 0);
}

void sclr_core_get_checksum_status(u8 inst, struct sclr_core_checksum_status *status)
{
	status->checksum_base.raw = _reg_read(reg_base + REG_SCL_CHECKSUM0(inst));
	status->axi_read_gop0_data = _reg_read(reg_base + REG_SCL_CHECKSUM1(inst));
	status->axi_read_gop1_data = _reg_read(reg_base + REG_SCL_CHECKSUM2(inst));
	status->axi_write_data = _reg_read(reg_base + REG_SCL_CHECKSUM3(inst));
}

/****************************************************************************
 * SCALER GOP
 ****************************************************************************/
/**
 * sclr_gop_set_cfg - configure gop
 *
 * @param inst: (0~3), the instance of gop which want to be configured.
 *		0~3 is on scl, 4 is on disp.
 * @param layer: (0~1) 0 is layer 0(gop0). 1 is layer 1(gop1).
 * @param cfg: gop's settings
 */
void sclr_gop_set_cfg(u8 inst, u8 layer, struct sclr_gop_cfg *cfg, bool update)
{
	if (inst < SCL_MAX_INST) {
		if (layer == 0) {
			_reg_write(reg_base + REG_SCL_GOP0_CFG(inst), cfg->gop_ctrl.raw);
			_reg_write(reg_base + REG_SCL_GOP0_FONTCOLOR(inst),
					   (cfg->font_fg_color << 16) | cfg->font_bg_color);
			if (cfg->gop_ctrl.b.colorkey_en)
			_reg_write(reg_base + REG_SCL_GOP0_COLORKEY(inst),
					   cfg->colorkey);
			_reg_write(reg_base + REG_SCL_GOP0_FONTBOX_CTRL(inst), cfg->fb_ctrl.raw);

			_reg_write(reg_base + REG_SCL_GOP0_DEC_CTRL(inst), cfg->odec_cfg.odec_ctrl.raw);
		} else if (layer == 1) {
			_reg_write(reg_base + REG_SCL_GOP1_CFG(inst), cfg->gop_ctrl.raw);
			_reg_write(reg_base + REG_SCL_GOP1_FONTCOLOR(inst),
					   (cfg->font_fg_color << 16) | cfg->font_bg_color);
			if (cfg->gop_ctrl.b.colorkey_en)
				_reg_write(reg_base + REG_SCL_GOP1_COLORKEY(inst), cfg->colorkey);
			_reg_write(reg_base + REG_SCL_GOP1_FONTBOX_CTRL(inst), cfg->fb_ctrl.raw);

			_reg_write(reg_base + REG_SCL_GOP1_DEC_CTRL(inst), cfg->odec_cfg.odec_ctrl.raw);

		} else {
			TRACE_VPSS(DBG_WARN, "[sc%d] only 0 or 1 layer, no such layer(%d).\n",
				inst, layer);
			return;
		}
		if (update)
			g_gop_cfg[inst][layer] = *cfg;
	}
}

/**
 * sclr_gop_get_cfg - get gop's configurations.
 *
 * @param inst: (0~3), the instance of gop which want to be configured.
 *		0~3 is on scl, 4 is on disp.
 * @param layer: (0~1) 0 is layer 0(gop0). 1 is layer 1(gop1).
 */
struct sclr_gop_cfg *sclr_gop_get_cfg(u8 inst, u8 layer)
{
	if (inst < SCL_MAX_INST && layer < SCL_MAX_GOP_INST)
		return &g_gop_cfg[inst][layer];

	return NULL;
}

/**
 * sclr_gop_setup 256LUT - setup gop's Look-up table
 *
 * @param inst: (0~3), the instance of gop which want to be configured.
 *		0~3 is on scl, 4 is on disp.
 * @param layer: (0~1) 0 is layer 0(gop0). 1 is layer 1(gop1).
 * @param length: update 256LUT-table for index 0 ~ length.
 *		There should be smaller than 256 instances.
 * @param data: values of 256LUT-table. There should be 256 instances.
 */
int sclr_gop_setup_256LUT(u8 inst, u8 layer, u16 length, u16 *data)
{
	u16 i = 0;

	if (length > 256)
		return -1;

	if (inst < SCL_MAX_INST) {
		if (layer == 0) {
			for (i = 0; i < length; ++i) {
				_reg_write(reg_base + REG_SCL_GOP0_256LUT0(inst),
						   (i << 16) | *(data + i));
				_reg_write(reg_base + REG_SCL_GOP0_256LUT1(inst), ~BIT(16));
				_reg_write(reg_base + REG_SCL_GOP0_256LUT1(inst), BIT(16));
			}
		} else if (layer == 1) {
			for (i = 0; i < length; ++i) {
				_reg_write(reg_base + REG_SCL_GOP1_256LUT0(inst),
						   (i << 16) | *(data + i));
				_reg_write(reg_base + REG_SCL_GOP1_256LUT1(inst), ~BIT(16));
				_reg_write(reg_base + REG_SCL_GOP1_256LUT1(inst), BIT(16));
			}
		} else {
			TRACE_VPSS(DBG_WARN, "[sc%d] only 0 or 1 layer, no such layer(%d).\n",
				inst, layer);
		}
	}

	return 0;
}

/**
 * sclr_gop_update_256LUT - update gop's Look-up table by index.
 *
 * @param inst: (0~3), the instance of gop which want to be configured.
 *		0~3 is on scl, 4 is on disp.
 * @param layer: (0~1) 0 is layer 0(gop0). 1 is layer 1(gop1).
 * @param index: start address of 256LUT-table. There should be 256 instances.
 * @param data: value of 256LUT-table.
 */
int sclr_gop_update_256LUT(u8 inst, u8 layer, u8 index, u16 data)
{
	//if (index > 255)
	//	return -1;

	if (inst < SCL_MAX_INST) {
		if (layer == 0) {
			_reg_write(reg_base + REG_SCL_GOP0_256LUT0(inst),
					   (index << 16) | data);
			_reg_write(reg_base + REG_SCL_GOP0_256LUT1(inst), ~BIT(16));
			_reg_write(reg_base + REG_SCL_GOP0_256LUT1(inst), BIT(16));
		} else if (layer == 1) {
			_reg_write(reg_base + REG_SCL_GOP1_256LUT0(inst),
					   (index << 16) | data);
			_reg_write(reg_base + REG_SCL_GOP1_256LUT1(inst), ~BIT(16));
			_reg_write(reg_base + REG_SCL_GOP1_256LUT1(inst), BIT(16));
		} else {
			TRACE_VPSS(DBG_WARN, "[sc%d] only 0 or 1 layer, no such layer(%d).\n",
				inst, layer);
		}
	}

	return 0;
}

/**
 * sclr_gop_setup 16LUT - setup gop's Look-up table
 *
 * @param inst: (0~3), the instance of gop which want to be configured.
 *		0~3 is on scl, 4 is on disp.
 * @param layer: (0~1) 0 is layer 0(gop0). 1 is layer 1(gop1).
 * @param length: update 16LUT-table for index 0 ~ length.
 *		There should be smaller than 16 instances.
 * @param data: values of 16LUT-table. There should be 16 instances.
 */
int sclr_gop_setup_16LUT(u8 inst, u8 layer, u8 length, u16 *data)
{
	u16 i = 0;
	//if (length > 15)
	//	return -1;

	if (inst < SCL_MAX_INST) {
		if (layer == 0) {
			for (i = 0; i < length; i += 2 ) {
				_reg_write(reg_base + REG_SCL_GOP0_16LUT(inst, i),
						   ((*(data + i + 1) << 16) | (*(data + i))));
			}
		} else if (layer == 1) {
			for (i = 0; i <= length; i += 2) {
				_reg_write(reg_base + REG_SCL_GOP1_16LUT(inst, i),
						   ((*(data + i + 1) << 16) | (*(data + i))));
			}
		} else {
			TRACE_VPSS(DBG_WARN, "[sc%d] only 0 or 1 layer, no such layer(%d).\n",
				inst, layer);
		}
	}

	return 0;
}

/**
 * sclr_gop_update_16LUT - update gop's Look-up table by index.
 *
 * @param inst: (0~3), the instance of gop which want to be configured.
 *		0~3 is on scl, 4 is on disp.
 * @param layer: (0~1) 0 is layer 0(gop0). 1 is layer 1(gop1).
 * @param index: start address of 16LUT-table. There should be 16 instances.
 * @param data: value of 16LUT-table.
 */
int sclr_gop_update_16LUT(u8 inst, u8 layer, u8 index, u16 data)
{
	u16 tmp;
	if (index > 15)
		return -1;

	if (inst < SCL_MAX_INST) {
		if (layer == 0) {
			if (index % 2 == 0) {
				tmp = _reg_read(reg_base + REG_SCL_GOP0_16LUT(inst, index + 1));
				_reg_write(reg_base + REG_SCL_GOP0_16LUT(inst, index),
						   ((tmp << 16) | data));
			} else {
				tmp = _reg_read(reg_base + REG_SCL_GOP0_16LUT(inst, index - 1));
				_reg_write(reg_base + REG_SCL_GOP0_16LUT(inst, index - 1),
						   ((data << 16) | tmp));
			}
		} else if (layer == 1) {
			if (index % 2 == 0) {
				tmp = _reg_read(reg_base + REG_SCL_GOP1_16LUT(inst, index + 1));
				_reg_write(reg_base + REG_SCL_GOP1_16LUT(inst, index),
						   ((tmp << 16) | data));
			} else {
				tmp = _reg_read(reg_base + REG_SCL_GOP1_16LUT(inst, index - 1));
				_reg_write(reg_base + REG_SCL_GOP1_16LUT(inst, index - 1),
						   ((data << 16) | tmp));
			}
		} else {
			TRACE_VPSS(DBG_WARN, "[sc%d] only 0 or 1 layer, no such layer(%d).\n",
				inst, layer);
		}
	}

	return 0;
}

/**
 * sclr_gop_ow_set_cfg - set gop's osd-window configurations.
 *
 * @param inst: (0~3), the instance of gop which want to be configured.
 *		0~3 is on scl, 4 is on disp.
 * @param layer: (0~1) 0 is layer 0(gop0). 1 is layer 1(gop1).
 * @param ow_inst: (0~7), the instance of ow which want to be configured.
 * @param cfg: ow's settings.
 */
void sclr_gop_ow_set_cfg(u8 inst, u8 layer, u8 ow_inst, struct sclr_gop_ow_cfg *ow_cfg, bool update)
{
	//OW Format
	//4'b0000: ARGB8888
	//4'b0100: ARGB4444
	//4'b0101: ARGB1555
	//4'b1000: 256LUT-ARGB4444
	//4'b1010: 16-LUT-ARGB4444
	//4'b1100: Font-base"
	static const u8 reg_map_fmt[SCL_GOP_FMT_MAX] = {0, 0x4, 0x5, 0x8, 0xa, 0xc};

	if (ow_inst >= SCL_MAX_GOP_OW_INST)
		return;

	if (inst < SCL_MAX_INST) {
		if (layer == 0) {
			_reg_write(reg_base + REG_SCL_GOP0_FMT(inst, ow_inst),
					   reg_map_fmt[ow_cfg->fmt]);
			_reg_write(reg_base + REG_SCL_GOP0_H_RANGE(inst, ow_inst),
					   (ow_cfg->end.x << 16) | ow_cfg->start.x);
			_reg_write(reg_base + REG_SCL_GOP0_V_RANGE(inst, ow_inst),
					   (ow_cfg->end.y << 16) | ow_cfg->start.y);
			_reg_write(reg_base + REG_SCL_GOP0_ADDR_L(inst, ow_inst),
					   ow_cfg->addr);
			_reg_write(reg_base + REG_SCL_GOP0_ADDR_H(inst, ow_inst),
					   ow_cfg->addr >> 32);
			_reg_write(reg_base + REG_SCL_GOP0_CROP_PITCH(inst, ow_inst),
					   (ow_cfg->crop_pixels << 16) | ow_cfg->pitch);
			_reg_write(reg_base + REG_SCL_GOP0_SIZE(inst, ow_inst),
					   (ow_cfg->mem_size.h << 16) | ow_cfg->mem_size.w);
		} else if (layer == 1) {
			_reg_write(reg_base + REG_SCL_GOP1_FMT(inst, ow_inst),
					   reg_map_fmt[ow_cfg->fmt]);
			_reg_write(reg_base + REG_SCL_GOP1_H_RANGE(inst, ow_inst),
					   (ow_cfg->end.x << 16) | ow_cfg->start.x);
			_reg_write(reg_base + REG_SCL_GOP1_V_RANGE(inst, ow_inst),
					   (ow_cfg->end.y << 16) | ow_cfg->start.y);
			_reg_write(reg_base + REG_SCL_GOP1_ADDR_L(inst, ow_inst),
					   ow_cfg->addr);
			_reg_write(reg_base + REG_SCL_GOP1_ADDR_H(inst, ow_inst),
					   ow_cfg->addr >> 32);
			_reg_write(reg_base + REG_SCL_GOP1_CROP_PITCH(inst, ow_inst),
					   (ow_cfg->crop_pixels << 16) | ow_cfg->pitch);
			_reg_write(reg_base + REG_SCL_GOP1_SIZE(inst, ow_inst),
					   (ow_cfg->mem_size.h << 16) | ow_cfg->mem_size.w);
		} else {
			TRACE_VPSS(DBG_WARN, "[sc%d] only 0 or 1 layer, no such layer(%d).\n",
				inst, layer);
			return;
		}
		if (update)
			g_gop_cfg[inst][layer].ow_cfg[ow_inst] = *ow_cfg;

	}
}

/**
 * sclr_gop_fb_set_cfg - setup fontbox
 *
 * @param inst: (0~3), the instance of gop which want to be configured.
 *		0~3 is on scl, 4 is on disp.
 * @param layer: (0~1) 0 is layer 0(gop0). 1 is layer 1(gop1).
 * @param fb_inst: (0~1), the instance of ow which want to be configured.
 * @param cfg: fontbox configuration
 */
void sclr_gop_fb_set_cfg(u8 inst, u8 layer, u8 fb_inst, struct sclr_gop_fb_cfg *fb_cfg)
{
	if (fb_inst >= SCL_MAX_GOP_FB_INST)
		return;

	if (fb_cfg->fb_ctrl.b.sample_rate > 3)
		fb_cfg->fb_ctrl.b.sample_rate = 3;
	if (fb_cfg->fb_ctrl.b.pix_thr > 31)
		fb_cfg->fb_ctrl.b.pix_thr = 31;

	if (inst < SCL_MAX_INST) {
		if (layer == 0) {
			_reg_write(reg_base + REG_SCL_GOP0_FONTBOX_CFG(inst, fb_inst), fb_cfg->fb_ctrl.raw);
			_reg_write(reg_base + REG_SCL_GOP0_FONTBOX_INIT(inst, fb_inst), fb_cfg->init_st);
		} else if (layer == 1) {
			_reg_write(reg_base + REG_SCL_GOP1_FONTBOX_CFG(inst, fb_inst), fb_cfg->fb_ctrl.raw);
			_reg_write(reg_base + REG_SCL_GOP1_FONTBOX_INIT(inst, fb_inst), fb_cfg->init_st);
		} else {
			TRACE_VPSS(DBG_WARN, "[sc%d] only 0 or 1 layer, no such layer(%d).\n",
				inst, layer);
		}
	}

	return;
}

/**
 * sclr_gop_fb_get_record - get fontbox's record
 *
 * @param inst: (0~3), the instance of gop which want to be configured.
 *		0~3 is on scl, 4 is on disp.
 * @param layer: (0~1) 0 is layer 0(gop0). 1 is layer 1(gop1).
 * @param fb_inst: (0~1), the instance of ow which want to be configured.
 * @return: fontbox's record
 */
u32 sclr_gop_fb_get_record(u8 inst, u8 layer, u8 fb_inst)
{
	if (inst < SCL_MAX_INST) {
		if (layer == 0)
			return _reg_read(reg_base + REG_SCL_GOP0_FONTBOX_REC(inst, fb_inst));
		else if (layer == 1)
			return _reg_read(reg_base + REG_SCL_GOP1_FONTBOX_REC(inst, fb_inst));
		else {
			TRACE_VPSS(DBG_WARN, "[sc%d] only 0 or 1 layer, no such layer(%d).\n",
				inst, layer);
			return -1;
		}
	} else
		return -1;
}

/**
 * sclr_cover_set_cfg - setup cover
 *
 * @param inst: (0~3), the instance of gop which want to be configured.
 *		0~3 is on scl, 4 is on disp.
 * @param cover_cfg: cover_cfg configuration
 */
void sclr_cover_set_cfg(u8 inst, u8 cover_w_inst, struct sclr_cover_cfg *cover_cfg, bool update)
{
	if (inst < SCL_MAX_INST) {
		_reg_write(reg_base + REG_SCL_COVER_CFG(inst, cover_w_inst), cover_cfg->start.raw);
		_reg_write(reg_base + REG_SCL_COVER_SIZE(inst, cover_w_inst),
				   (cover_cfg->img_size.h << 16) | cover_cfg->img_size.w);
		_reg_write(reg_base + REG_SCL_COVER_COLOR(inst, cover_w_inst), cover_cfg->color.raw);
	}
	if (update)
		g_sc_cfg[inst].cover_cfg[cover_w_inst] = *cover_cfg;

}

/****************************************************************************
 * SCALER PRICAY MASK
 ****************************************************************************/
/**
 * sclr_pri_set_cfg - configure privacy mask
 *
 * @param inst: (0~3), the instance of gop which want to be configured.
 *		0~3 is on scl, 4 is on disp.
 * @param cfg: privacy mask's settings
 */
void sclr_pri_set_cfg(u8 inst, struct sclr_privacy_cfg *cfg)
{
	u16 grid_line_per_frame, grid_num_per_line;
	u16 w, h;
	u16 pitch;

	if (inst < SCL_MAX_INST) {
		_reg_write(reg_base + REG_SCL_PRI_CFG(inst), cfg->cfg.raw);

		if (cfg->cfg.b.fit_picture) {
			w = g_sc_cfg[inst].sc.dst.w;
			h = g_sc_cfg[inst].sc.dst.h;
		} else {
			_reg_write(reg_base + REG_SCL_PRI_START(inst), (cfg->start.y << 16) | cfg->start.x);
			_reg_write(reg_base + REG_SCL_PRI_END(inst), (cfg->end.y << 16) | cfg->end.x);
			w = cfg->end.x - cfg->start.x + 1;
			h = cfg->end.y - cfg->start.y + 1;
		}

		_reg_write(reg_base + REG_SCL_PRI_ALPHA(inst),
			   (cfg->map_cfg.alpha_factor << 8) | cfg->map_cfg.no_mask_idx);
		_reg_write(reg_base + REG_SCL_PRI_MAP_ADDR_L(inst), cfg->map_cfg.base);
		_reg_write(reg_base + REG_SCL_PRI_MAP_ADDR_H(inst), cfg->map_cfg.base >> 32);

		if (cfg->cfg.b.mode == 0) {
			u8 grid_shift = cfg->cfg.b.grid_size ? 4 : 3;

			grid_num_per_line = UPPER(w, grid_shift);
			grid_line_per_frame = UPPER(h, grid_shift);
			_reg_write(reg_base + REG_SCL_PRI_GRID_CFG(inst),
				   (grid_num_per_line << 16) | (grid_line_per_frame - 1));
			pitch = ALIGN(grid_num_per_line, 16);
		} else {
			_reg_write(reg_base + REG_SCL_PRI_GRID_CFG(inst),
				   (w << 16) | (h - 1));
			pitch = ALIGN(w, 16);
		}

		_reg_write(reg_base + REG_SCL_PRI_MAP_AXI_CFG(inst), (cfg->map_cfg.axi_burst << 16) | pitch);
	}
}

/****************************************************************************
 * SCALER CTRL
 ****************************************************************************/
/**
 * sclr_ctrl_init - setup all sc instances.
 *
 */
void sclr_ctrl_init(u8 inst, bool is_resume)
{
	union sclr_intr intr_mask;
	union sclr_rt_cfg rt_cfg;
	u8 i;

	if (inst >= SCL_MAX_INST) {
		TRACE_VPSS(DBG_ERR, "[sc%d] no enough sclr-instance\n", inst);
		return;
	}

	if (!is_resume) {
		// init variables
		memset(&g_sc_cfg[inst], 0, sizeof(g_sc_cfg[0]));
		memset(&g_bd_cfg[inst], 0, sizeof(g_bd_cfg[0]));
		memset(&g_cir_cfg[inst], 0, sizeof(g_cir_cfg[0]));
		memset(&g_img_cfg[inst], 0, sizeof(g_img_cfg[0]));
		memset(&g_odma_cfg[inst], 0, sizeof(g_odma_cfg[0]));
		memset(&g_gop_cfg[inst], 0, sizeof(g_gop_cfg[0]));

		for (i = 0; i < SCL_MAX_GOP_INST; ++i) {
			g_gop_cfg[inst][i].fb_ctrl.b.hi_thr = 0x30;
			g_gop_cfg[inst][i].fb_ctrl.b.lo_thr = 0x20;
			g_gop_cfg[inst][i].fb_ctrl.b.detect_fnum = 1;
			g_gop_cfg[inst][i].fb_ctrl.b.hi_thr = 0x30;
			g_gop_cfg[inst][i].fb_ctrl.b.lo_thr = 0x20;
			g_gop_cfg[inst][i].fb_ctrl.b.detect_fnum = 1;
		}
		g_odma_cfg[inst].flip = SCL_FLIP_NO;
		g_odma_cfg[inst].burst = false;
		g_img_cfg[inst].burst = 7;
		g_img_cfg[inst].src = SCL_INPUT_MEM;
		g_sc_cfg[inst].coef = SCL_COEF_MAX;
		g_sc_cfg[inst].sc.mir_enable = true;
	} else {
		g_sc_cfg[inst].coef = SCL_COEF_MAX;
	}

	sclr_img_reg_shadow_sel(inst, false);

	rt_cfg.b.sc_d_rt = 0;
	rt_cfg.b.sc_v_rt = 0;
	rt_cfg.b.sc_rot_rt = 0;
	rt_cfg.b.img_d_rt = 0;
	rt_cfg.b.img_v_rt = 0;
	rt_cfg.b.img_d_sel = 1;

	intr_mask.raw = 0;
	intr_mask.b.img_in_frame_end = true;
	intr_mask.b.scl_frame_end = true;
	//intr_mask.b.prog_too_late = true;
	intr_mask.b.cmdq = true;
	//intr_mask.b.scl_line_target_hit = true;

	sclr_rt_set_cfg(inst, rt_cfg);
	sclr_set_intr_mask(inst, intr_mask);
	sclr_reg_shadow_sel(inst, false);
	sclr_update_coef(inst, SCL_COEF_BILINEAR);
	sclr_set_cfg(inst, false, false, false, false);
	sclr_reg_force_up(inst);

	sclr_top_reg_done(inst);
	sclr_top_pg_late_clr(inst);

	intr_mask.raw = 0xffffffff;
	sclr_intr_ctrl(inst, intr_mask);
}

/**
 * sclr_ctrl_set_scale - setup scaling
 *
 * @param inst: (0~3), the instance of sc
 * @param cfg: scaling settings, include in/crop/out size.
 */
void sclr_ctrl_set_scale(u8 inst, struct sclr_scale_cfg *cfg)
{
	if (inst >= SCL_MAX_INST) {
		TRACE_VPSS(DBG_ERR, "[sc%d] no enough sclr-instance\n", inst);
		return;
	}
	//if (memcmp(cfg, &g_sc_cfg[inst].sc, sizeof(*cfg)) == 0)
	//	return;

	sclr_set_input_size(inst, cfg->src, true);

	// if crop invalid, use src-size
	if (cfg->crop.w + cfg->crop.x > cfg->src.w) {
		cfg->crop.x = 0;
		cfg->crop.w = cfg->src.w;
	}
	if (cfg->crop.h + cfg->crop.y > cfg->src.h) {
		cfg->crop.y = 0;
		cfg->crop.h = cfg->src.h;
	}
	sclr_set_crop(inst, cfg->crop, true);
	sclr_set_output_size(inst, cfg->dst);

	//if (cfg->tile_enable)
	//	_tile_cal_size(cfg);
	//g_sc_cfg[inst].sc.mir_enable = cfg->mir_enable;
	//g_sc_cfg[inst].sc.tile_enable = cfg->tile_enable;
	//g_sc_cfg[inst].sc.tile = cfg->tile;
	g_sc_cfg[inst].sc = *cfg;
	sclr_set_scale(inst);
}

/**
 * sclr_ctrl_set_input - setup input
 *
 * @param img_inst: (0~1), the instance of img_in
 * @param input: input mux
 * @param fmt: color format
 * @param csc: csc which used to convert from yuv to rgb.
 * @return: 0 if success
 */
int sclr_ctrl_set_input(u8 inst, enum sclr_input input,
			enum sclr_format fmt, enum sclr_csc csc)
{
	int ret = 0;

	// always transfer to rgb for sc
	if ((inst >= VPSS_MAX) || (csc >= SCL_CSC_601_LIMIT_RGB2YUV))
		return -EINVAL;

	g_img_cfg[inst].src = input;

	g_img_cfg[inst].fmt = fmt;
	sclr_img_set_cfg(inst, &g_img_cfg[inst]);

	g_img_cfg[inst].csc = csc;
	if (csc == SCL_CSC_NONE) {
		sclr_img_csc_en(inst, false);
	} else {
		sclr_img_csc_en(inst, true);
		sclr_img_set_csc(inst, &csc_mtrx[csc]);
	}

	return ret;
}

/**
 * sclr_ctrl_set_output - setup output of sc
 *
 * @param inst: (0~3), the instance of sc
 * @param cfg: csc config, including hvs/quant settings if any
 * @param fmt: color format
 * @return: 0 if success
 */
int sclr_ctrl_set_output(u8 inst, struct sclr_csc_cfg *cfg,
			 enum sclr_format fmt)
{
	if (inst >= SCL_MAX_INST)
		return -EINVAL;

	if (cfg->mode == SCL_OUT_CSC) {
		// Use yuv for csc
		// sc's data is always rgb
		if ((cfg->csc_type < SCL_CSC_601_LIMIT_RGB2YUV) &&
			 (cfg->csc_type != SCL_CSC_NONE)) {
			TRACE_VPSS(DBG_ERR, "csc_type Invalid parameter\n");
			return -EINVAL;
		}
		//yuv444p ---> SCL_FMT_RGB_PLANAR, yuv444 ---> SCL_FMT_RGB_PACKED
		if (!IS_YUV_FMT(fmt) && (fmt != SCL_FMT_RGB_PLANAR) && ((fmt != SCL_FMT_RGB_PACKED))) {
			TRACE_VPSS(DBG_ERR, "format Invalid parameter\n");
			return -EINVAL;
		}
	} else {
		// Use rgb for quant/hsv
		if (IS_YUV_FMT(fmt)) {
			TRACE_VPSS(DBG_ERR,"quant/hsv not support yuv format\n");
			return -EINVAL;
		}
	}

	sclr_set_csc_ctrl(inst, cfg);

	return 0;
}

struct sclr_csc_matrix *sclr_get_csc_mtrx(enum sclr_csc csc)
{
	if (csc < SCL_CSC_MAX)
		return &csc_mtrx[csc];

	return &csc_mtrx[SCL_CSC_601_LIMIT_RGB2YUV];
}

int sclr_set_fbd(u8 inst, struct sclr_fbd_cfg *cfg, bool is_update)
{
	if (inst >= SCL_MAX_INST || inst < VPSS_T0)
		return -EINVAL;

	_reg_write(reg_base + REG_SCL_MAP_CONV_OFF_BASE_Y(inst), cfg->offset_base_y & 0xfffffff0);
	_reg_write(reg_base + REG_SCL_MAP_CONV_OFF_BASE_C(inst), cfg->offset_base_c & 0xfffffff0);
	_reg_write(reg_base + REG_SCL_MAP_CONV_COMP_BASE_Y(inst), cfg->comp_base_y & 0xfffffff0);
	_reg_write(reg_base + REG_SCL_MAP_CONV_COMP_BASE_C(inst), cfg->comp_base_c & 0xfffffff0);
	_reg_write(reg_base + REG_SCL_TOP_FBD_HIGNDDR(inst), cfg->offset_base_y >> 32);
	_reg_write(reg_base + REG_SCL_MAP_CONV_CROP_POS(inst), (cfg->crop.x << 16) | (cfg->crop.y));
	_reg_write(reg_base + REG_SCL_MAP_CONV_CROP_SIZE(inst), (cfg->crop.w << 16) | (cfg->crop.h));
	_reg_write(reg_base + REG_SCL_MAP_CONV_COMP_STRIDE(inst), (cfg->stride_y << 16) | (cfg->stride_c));
	_reg_write(reg_base + REG_SCL_MAP_CONV_OFF_STRIDE(inst), (cfg->height_y << 16) | (cfg->height_c));
	_reg_write(reg_base + REG_SCL_MAP_CONV_ENDIAN(inst), (cfg->endian << 4) | (cfg->endian));
	_reg_write(reg_base + REG_SCL_MAP_CONV_BIT_DEPTH(inst), (cfg->mono_en << 12) | (cfg->otbg_64x64_en << 8) | (cfg->depth_y << 2) | (cfg->depth_c));
	_reg_write_mask(reg_base + REG_SCL_MAP_CONV_OUT_CTRL(inst), 0x770000, (cfg->out_mode_y << 20) | (cfg->out_mode_c) << 16);
	if(is_update)
		g_fbd_cfg[inst] = *cfg;
	return 0;
}

static void _map_ctrl_to_img(struct sclr_ctrl_cfg *cfg,
				 struct sclr_img_cfg *img_cfg)
{
	img_cfg->burst = g_img_cfg[cfg->img_inst].burst;
	img_cfg->fmt = cfg->src_fmt;
	img_cfg->csc = cfg->src_csc;
	img_cfg->csc_en = (cfg->src_csc == SCL_CSC_NONE) ? false : true;
	img_cfg->src = cfg->input;
	img_cfg->mem.addr0 = cfg->src_addr0;
	img_cfg->mem.addr1 = cfg->src_addr1;
	img_cfg->mem.addr2 = cfg->src_addr2;
	img_cfg->mem.start_x = cfg->src_crop.x;
	img_cfg->mem.start_y = cfg->src_crop.y;
	img_cfg->mem.width = cfg->src_crop.w;
	img_cfg->mem.height = cfg->src_crop.h;
	img_cfg->mem.pitch_y = cfg->pitch_y;
	img_cfg->mem.pitch_c = cfg->pitch_c;
	//img_cfg->mem.pitch_y = IS_PACKED_FMT(cfg->src_fmt)
	//			? VIP_ALIGN(3 * cfg->src.w)
	//			: VIP_ALIGN(cfg->src.w);
	//img_cfg->mem.pitch_c = IS_YUV_FMT(cfg->src_fmt)
	//			? VIP_ALIGN(cfg->src.w >> 1)
	//			: VIP_ALIGN(cfg->src.w);
}

static void _map_ctrl_to_sc(struct sclr_ctrl_cfg *cfg,
				struct sclr_core_cfg *sc_cfg)
{
	sc_cfg->sc_bypass = false;
	sc_cfg->gop_bypass = false;
	sc_cfg->cir_bypass = true;
	sc_cfg->odma_bypass = false;
	sc_cfg->sc.src.w = cfg->src_crop.w;
	sc_cfg->sc.src.h = cfg->src_crop.h;
	sc_cfg->sc.crop.x = 0;
	sc_cfg->sc.crop.y = 0;
	sc_cfg->sc.crop.w = cfg->src_crop.w;
	sc_cfg->sc.crop.h = cfg->src_crop.h;
	sc_cfg->sc.dst = cfg->dst.frame;
}

static void _map_ctrl_to_odma(struct sclr_ctrl_cfg *cfg,
				  struct sclr_odma_cfg *odma_cfg, u8 inst)
{
	odma_cfg->flip = cfg->dst.flip_mode;
	odma_cfg->burst = g_odma_cfg[inst].burst;
	odma_cfg->fmt = cfg->dst.fmt;
	odma_cfg->csc_cfg.csc_type = cfg->dst.csc;
	odma_cfg->mem.addr0 = cfg->dst.addr0;
	odma_cfg->mem.addr1 = cfg->dst.addr1;
	odma_cfg->mem.addr2 = cfg->dst.addr2;
	odma_cfg->mem.start_x = cfg->dst.offset.x;
	odma_cfg->mem.start_y = cfg->dst.offset.y;
	odma_cfg->mem.width = cfg->dst.window.w;
	odma_cfg->mem.height = cfg->dst.window.h;
	odma_cfg->mem.pitch_y = cfg->dst.pitch_y;
	odma_cfg->mem.pitch_c = cfg->dst.pitch_c;
}

int _map_img_to_cmdq(struct sclr_ctrl_cfg *ctrl_cfg, union cmdq_set *cmd_start,
			 u16 cmd_idx)
{
	struct sclr_img_cfg img_cfg;
	struct sclr_csc_matrix *mtrx;
	u8 img_inst = ctrl_cfg->img_inst;

	if (img_inst >= VPSS_MAX)
		return cmd_idx;

	_map_ctrl_to_img(ctrl_cfg, &img_cfg);
	mtrx = &csc_mtrx[img_cfg.csc];

	cmdq_set_package(&cmd_start[cmd_idx++].reg, REG_SCL_IMG_CFG(img_inst),
			(img_cfg.csc_en << 12) | (img_cfg.burst << 8) | (img_cfg.fmt << 4) |
			 img_cfg.src);
	cmdq_set_package(&cmd_start[cmd_idx++].reg,
			 REG_SCL_IMG_OFFSET(img_inst),
			 (img_cfg.mem.start_y << 16) | img_cfg.mem.start_x);
	cmdq_set_package(&cmd_start[cmd_idx++].reg, REG_SCL_IMG_SIZE(img_inst),
			 ((img_cfg.mem.height - 1) << 16) |
			 (img_cfg.mem.width - 1));
	cmdq_set_package(&cmd_start[cmd_idx++].reg,
			 REG_SCL_IMG_PITCH_Y(img_inst), img_cfg.mem.pitch_y);
	cmdq_set_package(&cmd_start[cmd_idx++].reg,
			 REG_SCL_IMG_PITCH_C(img_inst), img_cfg.mem.pitch_c);
	cmdq_set_package(&cmd_start[cmd_idx++].reg,
			 REG_SCL_IMG_ADDR0_L(img_inst), img_cfg.mem.addr0);
	cmdq_set_package(&cmd_start[cmd_idx++].reg,
			 REG_SCL_IMG_ADDR0_H(img_inst),
			 img_cfg.mem.addr0 >> 32);
	cmdq_set_package(&cmd_start[cmd_idx++].reg,
			 REG_SCL_IMG_ADDR1_L(img_inst), img_cfg.mem.addr1);
	cmdq_set_package(&cmd_start[cmd_idx++].reg,
			 REG_SCL_IMG_ADDR1_H(img_inst),
			 img_cfg.mem.addr1 >> 32);
	cmdq_set_package(&cmd_start[cmd_idx++].reg,
			 REG_SCL_IMG_ADDR2_L(img_inst), img_cfg.mem.addr2);
	cmdq_set_package(&cmd_start[cmd_idx++].reg,
			 REG_SCL_IMG_ADDR2_H(img_inst),
			 img_cfg.mem.addr2 >> 32);

	if (img_cfg.csc_en) {
		cmdq_set_package(&cmd_start[cmd_idx++].reg,
				 REG_SCL_IMG_CSC_COEF0(img_inst),
				 (mtrx->coef[0][1] << 16) | mtrx->coef[0][0]);
		cmdq_set_package(&cmd_start[cmd_idx++].reg,
				 REG_SCL_IMG_CSC_COEF1(img_inst), mtrx->coef[0][2]);
		cmdq_set_package(&cmd_start[cmd_idx++].reg,
				 REG_SCL_IMG_CSC_COEF2(img_inst),
				 (mtrx->coef[1][1] << 16) | mtrx->coef[1][0]);
		cmdq_set_package(&cmd_start[cmd_idx++].reg,
				 REG_SCL_IMG_CSC_COEF3(img_inst), mtrx->coef[1][2]);
		cmdq_set_package(&cmd_start[cmd_idx++].reg,
				 REG_SCL_IMG_CSC_COEF4(img_inst),
				 (mtrx->coef[2][1] << 16) | mtrx->coef[2][0]);
		cmdq_set_package(&cmd_start[cmd_idx++].reg,
				 REG_SCL_IMG_CSC_COEF5(img_inst), mtrx->coef[2][2]);
		cmdq_set_package(&cmd_start[cmd_idx++].reg,
				 REG_SCL_IMG_CSC_SUB(img_inst),
				 (mtrx->sub[2] << 16) | (mtrx->sub[1] << 8)
				  | mtrx->sub[0]);
		cmdq_set_package(&cmd_start[cmd_idx++].reg,
				 REG_SCL_IMG_CSC_ADD(img_inst),
				 (mtrx->add[2] << 16) | (mtrx->add[1] << 8)
				  | mtrx->add[0]);
	}

	return cmd_idx;
}

int _map_sc_to_cmdq(struct sclr_ctrl_cfg *ctrl_cfg,
			union cmdq_set *cmd_start, u16 cmd_idx)
{
	u32 tmp = 0;
	u8 inst = ctrl_cfg->dst.inst;
	struct sclr_core_cfg sc_cfg;
	struct sclr_odma_cfg odma_cfg;

	if (inst >= SCL_MAX_INST)
		return cmd_idx;

	_map_ctrl_to_sc(ctrl_cfg, &sc_cfg);
	_map_ctrl_to_odma(ctrl_cfg, &odma_cfg, inst);

	// sc core
	tmp = 0;
	if (sc_cfg.sc_bypass)
		tmp |= 0x02;
	if (sc_cfg.gop_bypass)
		tmp |= 0x04;
	if (sc_cfg.cir_bypass)
		tmp |= 0x20;
	if (sc_cfg.odma_bypass)
		tmp |= 0x40;

	cmdq_set_package(&cmd_start[cmd_idx++].reg, REG_SCL_CFG(inst), tmp);
	cmdq_set_package(&cmd_start[cmd_idx++].reg, REG_SCL_SRC_SIZE(inst),
			 ((sc_cfg.sc.src.h - 1) << 16) | (sc_cfg.sc.src.w - 1));
	cmdq_set_package(&cmd_start[cmd_idx++].reg, REG_SCL_CROP_OFFSET(inst),
			 (sc_cfg.sc.crop.y << 16) | sc_cfg.sc.crop.x);
	cmdq_set_package(&cmd_start[cmd_idx++].reg, REG_SCL_CROP_SIZE(inst),
			 ((sc_cfg.sc.crop.h - 1) << 16)
			  | (sc_cfg.sc.crop.w - 1));
	cmdq_set_package(&cmd_start[cmd_idx++].reg, REG_SCL_OUT_SIZE(inst),
			 ((sc_cfg.sc.dst.h - 1) << 16) | (sc_cfg.sc.dst.w - 1));
	tmp = ((sc_cfg.sc.crop.w) << 13) / (sc_cfg.sc.dst.w);
	cmdq_set_package(&cmd_start[cmd_idx++].reg, REG_SCL_SC_H_CFG(inst),
			 tmp);
	tmp = ((sc_cfg.sc.crop.h) << 13) / (sc_cfg.sc.dst.h);
	cmdq_set_package(&cmd_start[cmd_idx++].reg, REG_SCL_SC_V_CFG(inst),
			 tmp);

	// sc odma
	cmdq_set_package(&cmd_start[cmd_idx++].reg, REG_SCL_ODMA_CFG(inst),
			 (odma_cfg.flip << 16) |  (odma_cfg.fmt << 8) |
			 odma_cfg.burst);
	cmdq_set_package(&cmd_start[cmd_idx++].reg, REG_SCL_ODMA_OFFSET_X(inst),
			 odma_cfg.mem.start_x);
	cmdq_set_package(&cmd_start[cmd_idx++].reg, REG_SCL_ODMA_OFFSET_Y(inst),
			 odma_cfg.mem.start_y);
	cmdq_set_package(&cmd_start[cmd_idx++].reg, REG_SCL_ODMA_WIDTH(inst),
			 odma_cfg.mem.width - 1);
	cmdq_set_package(&cmd_start[cmd_idx++].reg, REG_SCL_ODMA_HEIGHT(inst),
			 odma_cfg.mem.height - 1);
	cmdq_set_package(&cmd_start[cmd_idx++].reg, REG_SCL_ODMA_PITCH_Y(inst),
			 odma_cfg.mem.pitch_y);
	cmdq_set_package(&cmd_start[cmd_idx++].reg, REG_SCL_ODMA_PITCH_C(inst),
			 odma_cfg.mem.pitch_c);
	cmdq_set_package(&cmd_start[cmd_idx++].reg, REG_SCL_ODMA_ADDR0_L(inst),
			 odma_cfg.mem.addr0);
	cmdq_set_package(&cmd_start[cmd_idx++].reg, REG_SCL_ODMA_ADDR0_H(inst),
			 odma_cfg.mem.addr0 >> 32);
	cmdq_set_package(&cmd_start[cmd_idx++].reg, REG_SCL_ODMA_ADDR1_L(inst),
			 odma_cfg.mem.addr1);
	cmdq_set_package(&cmd_start[cmd_idx++].reg, REG_SCL_ODMA_ADDR1_H(inst),
			 odma_cfg.mem.addr1 >> 32);
	cmdq_set_package(&cmd_start[cmd_idx++].reg, REG_SCL_ODMA_ADDR2_L(inst),
			 odma_cfg.mem.addr2);
	cmdq_set_package(&cmd_start[cmd_idx++].reg, REG_SCL_ODMA_ADDR2_H(inst),
			 odma_cfg.mem.addr2 >> 32);

	// sc csc
	if (odma_cfg.csc_cfg.csc_type == SCL_CSC_NONE) {
		cmdq_set_package(&cmd_start[cmd_idx++].reg,
				 REG_SCL_CSC_EN(inst), 0);
	} else {
		struct sclr_csc_matrix *cfg = &csc_mtrx[odma_cfg.csc_cfg.csc_type];

		cmdq_set_package(&cmd_start[cmd_idx++].reg,
				 REG_SCL_CSC_EN(inst), 1);

		cmdq_set_package(&cmd_start[cmd_idx++].reg,
				 REG_SCL_CSC_COEF0(inst),
				 (cfg->coef[0][1] << 16) | cfg->coef[0][0]);
		cmdq_set_package(&cmd_start[cmd_idx++].reg,
				 REG_SCL_CSC_COEF1(inst),
				 (cfg->coef[1][0] << 16) | cfg->coef[0][2]);
		cmdq_set_package(&cmd_start[cmd_idx++].reg,
				 REG_SCL_CSC_COEF2(inst),
				 (cfg->coef[1][2] << 16) | cfg->coef[1][1]);
		cmdq_set_package(&cmd_start[cmd_idx++].reg,
				 REG_SCL_CSC_COEF3(inst),
				 (cfg->coef[2][1] << 16) | cfg->coef[2][0]);
		cmdq_set_package(&cmd_start[cmd_idx++].reg,
				 REG_SCL_CSC_COEF4(inst), cfg->coef[2][2]);

		cmdq_set_package(&cmd_start[cmd_idx++].reg,
				 REG_SCL_CSC_OFFSET(inst),
				 (cfg->add[2] << 16) | (cfg->add[1] << 8) |
				 cfg->add[0]);
	}

	return cmd_idx;
}

int _map_border_to_cmdq(struct sclr_ctrl_cfg *ctrl_cfg,
			union cmdq_set *cmd_start, u16 cmd_idx)
{
	u8 i = 0;

	cmdq_set_package(&cmd_start[cmd_idx++].reg,
			REG_SCL_BORDER_CFG(ctrl_cfg->img_inst), ctrl_cfg->border_cfg.cfg.raw);
	if (ctrl_cfg->border_cfg.cfg.b.enable) {
		cmdq_set_package(&cmd_start[cmd_idx++].reg,
			REG_SCL_BORDER_OFFSET(ctrl_cfg->img_inst),
			(ctrl_cfg->border_cfg.start.y << 16) | ctrl_cfg->border_cfg.start.x);
	}

	//vpp border
	for (i = 0; i < BORDER_VPP_MAX; i++) {
		cmdq_set_package(&cmd_start[cmd_idx++].reg,
				REG_SCL_BORDER_VPP_CFG(ctrl_cfg->img_inst, i), ctrl_cfg->border_vpp_cfg[i].cfg.raw);

		if (ctrl_cfg->border_vpp_cfg[i].cfg.b.enable) {
			cmdq_set_package(&cmd_start[cmd_idx++].reg,
				REG_SCL_BORDER_VPP_INX(ctrl_cfg->img_inst, i),
				(ctrl_cfg->border_vpp_cfg[i].inside_end.x << 16) | ctrl_cfg->border_vpp_cfg[i].inside_start.x);
			cmdq_set_package(&cmd_start[cmd_idx++].reg,
				REG_SCL_BORDER_VPP_INY(ctrl_cfg->img_inst, i),
				(ctrl_cfg->border_vpp_cfg[i].inside_end.y << 16) | ctrl_cfg->border_vpp_cfg[i].inside_start.y);
			cmdq_set_package(&cmd_start[cmd_idx++].reg,
				REG_SCL_BORDER_VPP_OUTX(ctrl_cfg->img_inst, i),
				(ctrl_cfg->border_vpp_cfg[i].outside_end.x << 16) | ctrl_cfg->border_vpp_cfg[i].outside_start.x);
			cmdq_set_package(&cmd_start[cmd_idx++].reg,
				REG_SCL_BORDER_VPP_OUTY(ctrl_cfg->img_inst, i),
				(ctrl_cfg->border_vpp_cfg[i].outside_end.y << 16) | ctrl_cfg->border_vpp_cfg[i].outside_start.y);
		}
	}
	return cmd_idx;
}

/**
 * sclr_engine_cmdq - trigger sc  by cmdq
 *
 * @param cfgs: settings for cmdq
 * @param cnt: the number of settings
 * @param cmdq_addr: memory-address to put cmdq
 */
void sclr_engine_cmdq(u8 inst, struct sclr_ctrl_cfg *cfgs, u8 cnt,
							u64 cmdq_phy_addr, void *cmdq_vir_addr,
							u32 cmdq_buf_size)
{
	u8 i = 0;
	u16 cmd_idx = 0;
	union cmdq_set *cmd_start = (union cmdq_set *)cmdq_vir_addr;
	//u32 tmp = 0, inst_enable = 0;
	u32 flag_num = 1;	// 0: sc_str_flag, sc_stp_flag
	union sclr_top_cfg_01 cfg_01;
	u32 buflen;

	base_ion_cache_invalidate(cmdq_phy_addr, cmdq_vir_addr, cmdq_buf_size);
	memset(cmdq_vir_addr, 0, cmdq_buf_size);
	_reg_write(reg_base + REG_SCL_TOP_CMDQ_START(inst), 0x18000);
	_reg_write(reg_base + REG_SCL_TOP_CMDQ_STOP(inst), 0x18000);
	_reg_write_mask(reg_base + REG_SCL_IMG_DBG(inst), 0x00040000, BIT(18));

	for (i = 0; i < cnt; ++i) {
		struct sclr_ctrl_cfg *cfg = (cfgs + i);
		//u8 img_inst = cfg->img_inst;

		cmd_idx = _map_img_to_cmdq(cfg, cmd_start, cmd_idx);

		cmd_idx = _map_sc_to_cmdq(cfg, cmd_start, cmd_idx);

		cmd_idx = _map_border_to_cmdq(cfg, cmd_start, cmd_idx);
		cmdq_set_package(&cmd_start[cmd_idx++].reg,
				 REG_SCL_TOP_CMDQ_STOP(inst), 0x100ff);

		cfg_01.raw = 0;
		cfg_01.b.sc_enable = 1;
		cfg_01.b.fbd_enable = 0;
		cfg_01.b.qos_en = BIT(2) | BIT(4);

		//tmp = _reg_read(reg_base + REG_SCL_TOP_CFG1(inst));
		//tmp &= ~0x1f;
		//tmp |= inst_enable;
		cmdq_set_package(&cmd_start[cmd_idx++].reg, REG_SCL_TOP_CFG1(inst),
				 cfg_01.raw);

		// start sclr
		cmdq_set_package(&cmd_start[cmd_idx++].reg,
				 REG_SCL_TOP_IMG_CTRL(inst), BIT(1));

		// wait end-inter(1)
		cmdq_set_wait(&cmd_start[cmd_idx++], /*is_timer*/false, flag_num, 0);
	}
	// clear intr
	cmdq_set_package(&cmd_start[cmd_idx++].reg, REG_SCL_TOP_INTR_STATUS(inst),
			 0x1000ffff);

	cmd_start[cmd_idx-1].reg.intr_end = 1;
	cmd_start[cmd_idx-1].reg.intr_last = 1;

	buflen = sizeof(union cmdq_set) * cmd_idx;
	if (buflen > cmdq_buf_size)
		TRACE_VPSS(DBG_ERR, "sc(%d) cmdq buf size is too small.\n", inst);

	//TRACE_VPSS(DBG_DEBUG, "use buf len(%d) cmd_idx(%d).\n", buflen, cmd_idx);
	base_ion_cache_flush(cmdq_phy_addr, cmdq_vir_addr, buflen);

	cmdq_intr_ctrl(REG_SCL_CMDQ_BASE(inst), 0x02);
	cmdq_engine(REG_SCL_CMDQ_BASE(inst), (uintptr_t)cmdq_phy_addr,
			REG_SCL_TOP_BASE(inst) >> 22, true, false, cmd_idx);
}

void sclr_clr_cmdq(u8 inst)
{
	u8 cmdq_intr_stat = cmdq_intr_status(REG_SCL_CMDQ_BASE(inst));

	cmdq_intr_clr(REG_SCL_CMDQ_BASE(inst), cmdq_intr_stat);
}

/**
 * sclr_tile_cal_size - calculate parameters for left/right tile
 *
 * @param inst: (0~7), the instance of sc
 */
u8 sclr_tile_cal_size(u8 inst, u16 out_l_end)
{
#ifndef TILE_ON_IMG
	struct sclr_scale_cfg *cfg = &g_sc_cfg[inst].sc;
	struct sclr_size crop_size = { .w = cfg->crop.w, .h = cfg->crop.h };
	struct sclr_size out_size = cfg->dst;
	u8 fix = (cfg->algorithm == SCL_BICUBIC) ? 13 : 23;
	u32 out_l_width = (out_size.w >> 1) & ~0x01; // make sure op on even pixels.
	u32 h_sc_fac = cfg->fac.h_fac;
	u32 h_pos = cfg->fac.h_pos;
	if(cfg->algorithm != SCL_COEF_BICUBIC && cfg->algorithm != SCL_COEF_NEAREST && (in_size.w < out_size.w))
		h_pos = (((1 << 23) - h_sc_fac) >> 1);
	else if (cfg->algorithm == SCL_COEF_NEAREST)
		h_pos = 1 << 22;
	u64 L_last_phase, R_first_phase;
	u16 L_last_pixel, R_first_pixel;
	u8 mode = SCL_TILE_BOTH;

	L_last_phase = (u64)out_l_width * h_sc_fac;
	L_last_pixel = (L_last_phase >> fix) + ((cfg->mir_enable) ? 0 : 1);
	cfg->tile.src_l_width = L_last_pixel + 1
				+ ((L_last_phase) ? 1 : 0);
	cfg->tile.out_l_width = out_l_width;

	// right tile no mirror
	R_first_phase = L_last_phase + h_sc_fac;
	R_first_pixel = (R_first_phase >> fix) + ((cfg->mir_enable) ? 0 : 1);
	cfg->tile.r_ini_phase = R_first_phase - ((R_first_pixel - 2) << fix) + h_pos * ((in_size.w < out_size.w) ? -1 : 1);
	cfg->tile.src_r_offset = R_first_pixel - 2;
	cfg->tile.src_r_width = crop_size.w - cfg->tile.src_r_offset;
#else
	u16 src_l_last_pixel_max = out_l_end;
	struct sclr_scale_cfg *cfg = &g_sc_cfg[inst].sc;
	struct sclr_size crop_size = { .w = cfg->crop.w, .h = cfg->crop.h };
	struct sclr_size out_size = cfg->dst;
	u8 fix = (cfg->algorithm == SCL_COEF_BICUBIC) ? 13 : 23;
	u32 out_l_width = 0;// = (out_size.w >> 1) & ~0x01; // make sure op on even pixels.
	u32 h_sc_fac = cfg->fac.h_fac;
	u32 h_pos = cfg->fac.h_pos;
	u64 L_last_phase = 0, R_first_phase = 0;
	u16 L_last_pixel = 0, R_first_pixel = 0;
	u8 mode = SCL_TILE_BOTH;
	if(cfg->algorithm != SCL_COEF_BICUBIC && cfg->algorithm != SCL_COEF_NEAREST && (crop_size.w < out_size.w))
		h_pos = (((1 << 23) - h_sc_fac) >> 1);
	else if (cfg->algorithm == SCL_COEF_NEAREST)
		h_pos = 1 << 22;

	TRACE_VPSS(DBG_DEBUG, "%s: on sc(%d)\n", __func__, inst);
	TRACE_VPSS(DBG_DEBUG, "width: src(%d), crop(%d), dst(%d)\n",
		g_sc_cfg[inst].sc.src.w, crop_size.w, out_size.w);

	if (cfg->crop.x >= src_l_last_pixel_max) {
		// do nothing on left tile if crop out-of-range.
		cfg->tile.src_l_width = 0;
		cfg->tile.out_l_width = 0;
		cfg->tile.r_ini_phase = 0;
		cfg->tile.src_r_offset = 0;
		cfg->tile.src_r_width = crop_size.w;
		mode = SCL_TILE_RIGHT;
	} else {
		// find available out_l_width
		if (cfg->crop.x + cfg->crop.w - 1 <= src_l_last_pixel_max) {
			out_l_width = out_size.w; // make sure op on even pixels.
			L_last_phase = (u64)out_l_width * h_sc_fac;
			L_last_pixel = (L_last_phase >> fix) + ((cfg->mir_enable) ? 0 : 1);
			cfg->tile.src_l_width = L_last_pixel + 1 + ((L_last_phase) ? 1 : 0);
			cfg->tile.out_l_width = out_l_width;

			// do nothing on right tile if crop out-of-range.
			cfg->tile.r_ini_phase = 0;
			cfg->tile.src_r_offset = 0;
			cfg->tile.src_r_width = 0;
			mode = SCL_TILE_LEFT;
		} else {
			u64 src_l = MAX((u64)src_l_last_pixel_max - cfg->crop.x - ((cfg->mir_enable) ? 0 : 1), MAX((2*h_sc_fac)>>fix, 2));
			if(cfg->crop.w - src_l < 2)
				src_l = cfg->crop.w - 2;
			if(((cfg->crop.w - src_l) << fix) / h_sc_fac < 2)
				src_l = cfg->crop.w - ((2*h_sc_fac)>>fix);
			out_l_width = (((src_l) << fix) / h_sc_fac) & ~0x01;
			L_last_phase = (u64)out_l_width * h_sc_fac;
			L_last_pixel = (L_last_phase >> fix) + ((cfg->mir_enable) ? 0 : 1);
			cfg->tile.src_l_width = (L_last_pixel + 2 + ((L_last_phase) ? 1 : 0)) & ~0x1;
			cfg->tile.out_l_width = out_l_width;

			// right tile no mirror
			R_first_phase = L_last_phase;
			R_first_pixel = (R_first_phase >> fix) + ((cfg->mir_enable) ? 0 : 1);
			cfg->tile.r_ini_phase = R_first_phase - (((R_first_pixel - 2) & ~0x1) << fix) + h_pos * ((crop_size.w < out_size.w || cfg->algorithm == SCL_COEF_NEAREST) ? -1 : 1);
			cfg->tile.src_r_offset = (R_first_pixel - 2) & ~0x1;
			cfg->tile.src_r_width = crop_size.w - cfg->tile.src_r_offset;
			if (!out_l_width)
				mode = SCL_TILE_RIGHT;
			else
				mode = SCL_TILE_BOTH;
		}
	}
#endif
	cfg->tile.in_mem.w = g_img_cfg[inst].mem.width;
	cfg->tile.in_mem.h = g_img_cfg[inst].mem.height;
	cfg->tile.src = crop_size;
	cfg->tile.out = out_size;
	cfg->tile.border_enable = g_bd_cfg[inst].cfg.b.enable;
	if (g_bd_cfg[inst].cfg.b.enable) {
		// if border, then only on left tile enabled to fill bgcolor.
		cfg->tile.dma_l_x = g_bd_cfg[inst].start.x & ~0x01;
		cfg->tile.dma_l_y = g_bd_cfg[inst].start.y;
		cfg->tile.dma_r_x = cfg->tile.dma_l_x + out_l_width;
		cfg->tile.dma_r_y = g_bd_cfg[inst].start.y;
		cfg->tile.dma_l_width = g_odma_cfg[inst].mem.width;
		if ((g_odma_cfg[inst].flip == SCL_FLIP_HFLIP) || (g_odma_cfg[inst].flip == SCL_FLIP_HVFLIP))
			cfg->tile.dma_r_x
				= g_odma_cfg[inst].frame_size.w - out_size.w - (g_bd_cfg[inst].start.x & ~0x01);
		if ((g_odma_cfg[inst].flip == SCL_FLIP_VFLIP) || (g_odma_cfg[inst].flip == SCL_FLIP_HVFLIP))
			cfg->tile.dma_r_y = g_odma_cfg[inst].frame_size.h - out_size.h - g_bd_cfg[inst].start.y;
	} else {
		cfg->tile.dma_l_x = g_odma_cfg[inst].mem.start_x;
		cfg->tile.dma_l_y = g_odma_cfg[inst].mem.start_y;
		cfg->tile.dma_r_x = g_odma_cfg[inst].mem.start_x + out_l_width;
		cfg->tile.dma_r_y = g_odma_cfg[inst].mem.start_y;
		cfg->tile.dma_l_width = out_l_width;
		if ((g_odma_cfg[inst].flip == SCL_FLIP_HFLIP) || (g_odma_cfg[inst].flip == SCL_FLIP_HVFLIP)) {
			cfg->tile.dma_l_x = g_odma_cfg[inst].frame_size.w - out_l_width - g_odma_cfg[inst].mem.start_x;
			cfg->tile.dma_r_x = g_odma_cfg[inst].frame_size.w - out_size.w - g_odma_cfg[inst].mem.start_x;
		}
		if ((g_odma_cfg[inst].flip == SCL_FLIP_VFLIP) || (g_odma_cfg[inst].flip == SCL_FLIP_HVFLIP)) {
			cfg->tile.dma_l_y = g_odma_cfg[inst].frame_size.h - out_size.h - g_odma_cfg[inst].mem.start_y;
			cfg->tile.dma_r_y = cfg->tile.dma_l_y;
		}
	}

	TRACE_VPSS(DBG_DEBUG, "h_sc_fac:%d\n", h_sc_fac);
	TRACE_VPSS(DBG_DEBUG, "L last phase(%lld) last pixel(%d)\n", L_last_phase, L_last_pixel);
	TRACE_VPSS(DBG_DEBUG, "R first phase(%lld) first pixel(%d)\n", R_first_phase, R_first_pixel);
	TRACE_VPSS(DBG_DEBUG, "tile cfg: Left src width(%d) out(%d)\n",
		cfg->tile.src_l_width, cfg->tile.out_l_width);
	TRACE_VPSS(DBG_DEBUG, "tile cfg: Right src offset(%d) width(%d) phase(%d)\n"
		, cfg->tile.src_r_offset, cfg->tile.src_r_width, cfg->tile.r_ini_phase);
	TRACE_VPSS(DBG_DEBUG, "tile cfg: odma left x(%d) y(%d) width(%d)\n"
		, cfg->tile.dma_l_x, cfg->tile.dma_l_y, cfg->tile.dma_l_width);
	TRACE_VPSS(DBG_DEBUG, "tile cfg: odma right x(%d) y(%d)\n"
		, cfg->tile.dma_r_x, cfg->tile.dma_r_y);
	return mode;
}

/**
 * sclr_v_tile_cal_size - calculate parameters for top/down tile
 *
 * @param inst: (0~7), the instance of sc
 */
u8 sclr_v_tile_cal_size(u8 inst, u16 out_l_end)
{
#ifndef TILE_ON_IMG
	struct sclr_scale_cfg *cfg = &g_sc_cfg[inst].sc;
	struct sclr_size crop_size = { .w = cfg->crop.w, .h = cfg->crop.h };
	struct sclr_size out_size = cfg->dst;
	u8 fix = (cfg->algorithm == SCL_BICUBIC) ? 13 : 23;
	u32 out_l_width = (out_size.h >> 1) & ~0x01; // make sure op on even pixels.
	u32 v_sc_fac = cfg->fac.v_fac;
	u32 v_pos = cfg->fac.v_pos;
	if(cfg->algorithm != SCL_COEF_BICUBIC && cfg->algorithm != SCL_COEF_NEAREST && (crop_size.h < out_size.h))
		v_pos = (((1 << 23) - v_sc_fac) >> 1);
	else if (cfg->algorithm == SCL_COEF_NEAREST)
		v_pos = 1 << 22;
	u64 L_last_phase, R_first_phase;
	u16 L_last_pixel, R_first_pixel;
	u8 mode = SCL_TILE_BOTH;

	L_last_phase = (u64)out_l_width * v_sc_fac;
	L_last_pixel = (L_last_phase >> fix);
	cfg->v_tile.src_l_width = L_last_pixel + 1
				+ ((L_last_phase) ? 1 : 0);
	cfg->v_tile.out_l_width = out_l_width;

	// right tile no mirror
	R_first_phase = L_last_phase;
	R_first_pixel = (R_first_phase >> fix);
	cfg->v_tile.r_ini_phase = R_first_phase - ((R_first_pixel - 2) << fix) + v_pos * ((crop_size.h < out_size.h) ? -1 : 1);
	cfg->v_tile.src_r_offset = R_first_pixel - 2;
	cfg->v_tile.src_r_width = crop_size.h - cfg->v_tile.src_r_offset;
#else
	u16 src_l_last_pixel_max = out_l_end;
	struct sclr_scale_cfg *cfg = &g_sc_cfg[inst].sc;
	struct sclr_size crop_size = { .w = cfg->crop.w, .h = cfg->crop.h };
	struct sclr_size out_size = cfg->dst;
	u8 fix = (cfg->algorithm == SCL_COEF_BICUBIC) ? 13 : 23;
	u32 out_l_width = 0;// = (out_size.w >> 1) & ~0x01; // make sure op on even pixels.
	u32 v_sc_fac = cfg->fac.v_fac;
	u32 v_pos = cfg->fac.v_pos;
	u64 L_last_phase = 0, R_first_phase = 0;
	u16 L_last_pixel = 0, R_first_pixel = 0;
	u8 mode = SCL_TILE_BOTH;
	if(cfg->algorithm != SCL_COEF_BICUBIC && cfg->algorithm != SCL_COEF_NEAREST && (crop_size.h < out_size.h))
		v_pos = (((1 << 23) - v_sc_fac) >> 1);
	else if (cfg->algorithm == SCL_COEF_NEAREST)
		v_pos = 1 << 22;
	TRACE_VPSS(DBG_DEBUG, "%s: on sc(%d)\n", __func__, inst);
	TRACE_VPSS(DBG_DEBUG, "height: src(%d), crop(%d), dst(%d)\n",
		g_sc_cfg[inst].sc.src.h, crop_size.h, out_size.h);

	if (cfg->crop.y >= src_l_last_pixel_max) {
		// do nothing on left tile if crop out-of-range.
		cfg->v_tile.src_l_width = 0;
		cfg->v_tile.out_l_width = 0;
		cfg->v_tile.r_ini_phase = 0;
		cfg->v_tile.src_r_offset = 0;
		cfg->v_tile.src_r_width = crop_size.h;
		mode = SCL_TILE_RIGHT;
	} else {
		// find available out_l_width
		if (cfg->crop.y + cfg->crop.h - 1 <= src_l_last_pixel_max) {
			out_l_width = out_size.h; // make sure op on even pixels.
			L_last_phase = (u64)out_l_width * v_sc_fac;
			L_last_pixel = (L_last_phase >> fix);
			cfg->v_tile.src_l_width = L_last_pixel + 1 + ((L_last_phase) ? 1 : 0);
			cfg->v_tile.out_l_width = out_l_width;

			// do nothing on right tile if crop out-of-range.
			cfg->v_tile.r_ini_phase = 0;
			cfg->v_tile.src_r_offset = 0;
			cfg->v_tile.src_r_width = 0;
			mode = SCL_TILE_LEFT;
		} else {
			u64 src_l = MAX((u64)src_l_last_pixel_max - cfg->crop.y, MAX((2*v_sc_fac)>>fix, 2));
			if(cfg->crop.h - src_l < 2)
				src_l = cfg->crop.h - 2;
			if(((cfg->crop.h - src_l) << fix) / v_sc_fac < 2)
				src_l = cfg->crop.h - ((2*v_sc_fac)>>fix);
			out_l_width = ((src_l << fix) / v_sc_fac) & ~0x01;
			L_last_phase = (u64)out_l_width * v_sc_fac;
			L_last_pixel = (L_last_phase >> fix);
			cfg->v_tile.src_l_width = (L_last_pixel + 2 + ((L_last_phase) ? 1 : 0)) & ~0x1;
			cfg->v_tile.out_l_width = out_l_width;

			// right tile no mirror
			R_first_phase = L_last_phase;
			R_first_pixel = (R_first_phase >> fix);
			cfg->v_tile.r_ini_phase = R_first_phase - (((R_first_pixel - 2) & ~0x1) << fix) + v_pos * ((crop_size.h < out_size.h || cfg->algorithm == SCL_COEF_NEAREST) ? -1 : 1);
			cfg->v_tile.src_r_offset = (R_first_pixel - 2) & ~0x1;
			cfg->v_tile.src_r_width = crop_size.h - cfg->v_tile.src_r_offset;
			mode = SCL_TILE_BOTH;
		}
	}
#endif
	cfg->v_tile.in_mem.w = g_img_cfg[inst].mem.width;
	cfg->v_tile.in_mem.h = g_img_cfg[inst].mem.height;
	cfg->v_tile.src = crop_size;
	cfg->v_tile.out = out_size;
	cfg->v_tile.border_enable = g_bd_cfg[inst].cfg.b.enable;
	if (g_bd_cfg[inst].cfg.b.enable) {
		// if border, then only on top tile enabled to fill bgcolor.
		cfg->v_tile.dma_l_x = g_bd_cfg[inst].start.x;
		cfg->v_tile.dma_l_y = g_bd_cfg[inst].start.y & ~0x01;
		cfg->v_tile.dma_r_x = g_bd_cfg[inst].start.x;
		cfg->v_tile.dma_r_y = cfg->v_tile.dma_l_y + out_l_width;
		cfg->v_tile.dma_l_width = g_odma_cfg[inst].mem.height;
		if ((g_odma_cfg[inst].flip == SCL_FLIP_HFLIP) || (g_odma_cfg[inst].flip == SCL_FLIP_HVFLIP))
			cfg->v_tile.dma_r_x = g_odma_cfg[inst].frame_size.w - out_size.w - (g_bd_cfg[inst].start.x);
		if ((g_odma_cfg[inst].flip == SCL_FLIP_VFLIP) || (g_odma_cfg[inst].flip == SCL_FLIP_HVFLIP))
			cfg->v_tile.dma_r_y = g_odma_cfg[inst].frame_size.h - out_size.h - (g_bd_cfg[inst].start.y & ~0x01);
	} else {
		cfg->v_tile.dma_l_x = g_odma_cfg[inst].mem.start_x;
		cfg->v_tile.dma_l_y = g_odma_cfg[inst].mem.start_y;
		cfg->v_tile.dma_r_x = g_odma_cfg[inst].mem.start_x;
		cfg->v_tile.dma_r_y = g_odma_cfg[inst].mem.start_y + out_l_width;
		cfg->v_tile.dma_l_width = out_l_width;
		if ((g_odma_cfg[inst].flip == SCL_FLIP_HFLIP) || (g_odma_cfg[inst].flip == SCL_FLIP_HVFLIP)) {
			cfg->v_tile.dma_l_x = g_odma_cfg[inst].frame_size.w - out_size.w - g_odma_cfg[inst].mem.start_x;
			cfg->v_tile.dma_r_x = cfg->v_tile.dma_l_x;
		}
		if ((g_odma_cfg[inst].flip == SCL_FLIP_VFLIP) || (g_odma_cfg[inst].flip == SCL_FLIP_HVFLIP)) {
			cfg->v_tile.dma_l_y = g_odma_cfg[inst].frame_size.h - out_l_width - g_odma_cfg[inst].mem.start_y;
			cfg->v_tile.dma_r_y = g_odma_cfg[inst].frame_size.h - out_size.h - g_odma_cfg[inst].mem.start_y;
		}
	}

	TRACE_VPSS(DBG_DEBUG, "v_sc_fac:%d\n", v_sc_fac);
	TRACE_VPSS(DBG_DEBUG, "L last phase(%lld) last pixel(%d)\n", L_last_phase, L_last_pixel);
	TRACE_VPSS(DBG_DEBUG, "R first phase(%lld) first pixel(%d)\n", R_first_phase, R_first_pixel);
	TRACE_VPSS(DBG_DEBUG, "v_tile cfg: Left src length(%d) out(%d)\n",
		cfg->v_tile.src_l_width, cfg->v_tile.out_l_width);
	TRACE_VPSS(DBG_DEBUG, "v_tile cfg: Right src offset(%d) length(%d) phase(%d)\n"
		, cfg->v_tile.src_r_offset, cfg->v_tile.src_r_width, cfg->v_tile.r_ini_phase);
	TRACE_VPSS(DBG_DEBUG, "v_tile cfg: odma left x(%d) y(%d) length(%d)\n"
		, cfg->v_tile.dma_l_x, cfg->v_tile.dma_l_y, cfg->v_tile.dma_l_width);
	TRACE_VPSS(DBG_DEBUG, "v_tile cfg: odma right x(%d) y(%d)\n"
		, cfg->v_tile.dma_r_x, cfg->v_tile.dma_r_y);
	return mode;
}

/**
 * _gop_tile_shift - update tile for gop
 *
 * @param gop_ow_cfg: gop windows cfg which will be referenced and updated
 * @param left_width: width of gop on left tile
 * @param is_right_tile: true for right tile; false for left tile
 */
static void _gop_tile_shift(struct sclr_gop_ow_cfg *gop_ow_cfg, u16 left_width,
	u16 total_width, bool is_right_tile)
{
	s16 byte_offset;
	s8 pixel_offset = 0;
	u8 bpp = (gop_ow_cfg->fmt == SCL_GOP_FMT_ARGB8888) ? 4 :
		 (gop_ow_cfg->fmt == SCL_GOP_FMT_256LUT) ? 1 : 2;

	// workaround for dma-address 32 alignment
	byte_offset = (bpp * (left_width - gop_ow_cfg->start.x)) & (GOP_ALIGNMENT - 1);
	if (byte_offset) {
		if (byte_offset <= (GOP_ALIGNMENT - byte_offset)) {
			pixel_offset = (byte_offset / bpp);
			// use another side if out-of-window
			if ((gop_ow_cfg->end.x + pixel_offset) >= total_width)
				pixel_offset = -(GOP_ALIGNMENT - byte_offset) / bpp;
		} else {
			pixel_offset = -(GOP_ALIGNMENT - byte_offset) / bpp;
			// use another side if out-of-window
			if ((gop_ow_cfg->start.x + pixel_offset) < 0)
				pixel_offset = (byte_offset / bpp);
		}

		gop_ow_cfg->start.x += pixel_offset;
		gop_ow_cfg->end.x += pixel_offset;
		TRACE_VPSS(DBG_DEBUG, "gop tile wa: byte_offset(%d) pixoffset(%d)\n", byte_offset, pixel_offset);
	}

	if (is_right_tile) {
		gop_ow_cfg->addr += bpp * (left_width - gop_ow_cfg->start.x);
		gop_ow_cfg->start.x = 0;
		gop_ow_cfg->end.x -= left_width;
		gop_ow_cfg->img_size.w = gop_ow_cfg->end.x;
		gop_ow_cfg->mem_size.w = ALIGN(gop_ow_cfg->img_size.w * bpp, GOP_ALIGNMENT);
	} else {
		gop_ow_cfg->img_size.w = left_width - gop_ow_cfg->start.x;
		gop_ow_cfg->end.x = left_width;
		gop_ow_cfg->mem_size.w = ALIGN(gop_ow_cfg->img_size.w * bpp, GOP_ALIGNMENT);
	}
	TRACE_VPSS(DBG_DEBUG, "gop_tile(%s): addr(%#llx), startx(%d), endx(%d) img_width(%d) mem_width(%d)\n",
		is_right_tile ? "right" : "left",
		gop_ow_cfg->addr, gop_ow_cfg->start.x, gop_ow_cfg->end.x,
		gop_ow_cfg->img_size.w, gop_ow_cfg->mem_size.w);
}

static void _gop_v_tile_shift(struct sclr_gop_ow_cfg *gop_ow_cfg, u16 left_height,
	u16 total_height, u16 left_width, bool is_down_tile)
{
	// s16 byte_offset;
	// s8 pixel_offset = 0;
	// u8 bpp = (gop_ow_cfg->fmt == SCL_GOP_FMT_ARGB8888) ? 4 :
	// 	 (gop_ow_cfg->fmt == SCL_GOP_FMT_256LUT) ? 1 : 2;

	if (is_down_tile) {
		gop_ow_cfg->addr += (left_height - gop_ow_cfg->start.y) * (gop_ow_cfg->pitch);
		gop_ow_cfg->end.y -= left_height;
		gop_ow_cfg->img_size.h = gop_ow_cfg->end.y;
	} else {
		gop_ow_cfg->img_size.h = left_height - gop_ow_cfg->start.y;
		gop_ow_cfg->end.y = left_height;
	}
	TRACE_VPSS(DBG_DEBUG, "gop_tile(%s): addr(%#llx), starty(%d), endy(%d) img_h(%d) mem_h(%d)\n",
		is_down_tile ? "down" : "top",
		gop_ow_cfg->addr, gop_ow_cfg->start.y, gop_ow_cfg->end.y,
		gop_ow_cfg->img_size.h, gop_ow_cfg->mem_size.h);
}

/**
 * sclr_left_tile - update parameters for left tile
 *
 * @param inst: (0~3), the instance of sc
 * @param src_l_w: width of left tile from img
 * @return: true if success; false if no need or something wrong
 */
bool sclr_left_tile(u8 inst, u16 src_l_w)
{
	struct sclr_scale_cfg *sc;
	struct sclr_odma_cfg *odma_cfg;
	struct sclr_rect crop;
	struct sclr_size src, dst;
	struct sclr_gop_cfg gop_cfg;
	struct sclr_gop_ow_cfg gop_ow_cfg;
	int i = 0, j = 0;

	if (inst >= SCL_MAX_INST) {
		TRACE_VPSS(DBG_ERR, "no enough sclr-instance ");
		TRACE_VPSS(DBG_ERR, "for the requirement(%d)\n", inst);
		return false;
	}

	sc = &(sclr_get_cfg(inst)->sc);
	odma_cfg = sclr_odma_get_cfg(inst);

	sc->tile_enable = true;

	// skip if crop in the right tile.
	if (sc->tile.src_l_width == 0) {
		sclr_top_set_cfg(inst, false, false);
		return false;
	}
	sclr_top_set_cfg(inst, true, g_fbd_cfg[inst].enable);

	src.w = src_l_w;
	src.h = sc->src.h;
	crop.x = sc->crop.x;
	crop.y = sc->crop.y;
	crop.w = sc->tile.src_l_width;
	crop.h = sc->tile.src.h;
	dst.w = sc->tile.out_l_width;
	dst.h = sc->tile.out.h;

	sclr_set_input_size(inst, src, true);
	sclr_set_crop(inst, crop, true);
	sclr_set_output_size(inst, dst);

	if (sc->tile.border_enable) {
		odma_cfg->mem.start_x = 0;
		odma_cfg->mem.start_y = 0;
		odma_cfg->mem.width = sc->tile.dma_l_width;
		sclr_odma_set_mem(inst, &odma_cfg->mem);

		g_bd_cfg[inst].cfg.b.enable = true;
		g_bd_cfg[inst].start.x = sc->tile.dma_l_x;
		g_bd_cfg[inst].start.y = sc->tile.dma_l_y;
		sclr_border_set_cfg(inst, &g_bd_cfg[inst]);
		TRACE_VPSS(DBG_DEBUG, "sc%d border start: x=%d y=%d\n", inst, g_bd_cfg[inst].start.x,
			g_bd_cfg[inst].start.y);
	} else {
		odma_cfg->mem.start_x = sc->tile.dma_l_x;
		odma_cfg->mem.start_y = sc->tile.dma_l_y;
		odma_cfg->mem.width = sc->tile.dma_l_width;
		sclr_odma_set_mem(inst, &odma_cfg->mem);
	}
	TRACE_VPSS(DBG_DEBUG, "sc%d input size: w=%d h=%d\n", inst, src.w, src.h);
	TRACE_VPSS(DBG_DEBUG, "sc%d crop size: x=%d y=%d w=%d h=%d\n", inst, crop.x, crop.y, crop.w, crop.h);
	TRACE_VPSS(DBG_DEBUG, "sc%d output size: w=%d h=%d\n", inst, dst.w, dst.h);
	TRACE_VPSS(DBG_DEBUG, "sc%d odma mem: x=%d y=%d w=%d h=%d pitch_y=%d pitch_c=%d\n", inst,
		odma_cfg->mem.start_x, odma_cfg->mem.start_y, odma_cfg->mem.width,
		odma_cfg->mem.height, odma_cfg->mem.pitch_y, odma_cfg->mem.pitch_c);

	for (j = 0; j < SCL_MAX_GOP_INST; ++j) {
		gop_cfg = *sclr_gop_get_cfg(inst, j);
		for (i = 0; i < SCL_MAX_GOP_OW_INST; ++i) {
			if (gop_cfg.gop_ctrl.raw & BIT(i)) {
				gop_ow_cfg = gop_cfg.ow_cfg[i];
				TRACE_VPSS(DBG_DEBUG, "gop(%d) startx(%d) endx(%d), tile_l_width(%d)\n",
					i, gop_ow_cfg.start.x, gop_ow_cfg.end.x, sc->tile.out_l_width);

				// gop-window on right tile: pass
				if (gop_ow_cfg.start.x >= sc->tile.out_l_width) {
					gop_cfg.gop_ctrl.raw &= ~BIT(i);
					continue;
				}
				// gop-window on left tile: ok to go
				if (gop_ow_cfg.end.x <= sc->tile.out_l_width) {
					sclr_gop_ow_set_cfg(inst, j, i, &gop_ow_cfg, false);
					continue;
				}

				// gop-window on both tile: workaround
				_gop_tile_shift(&gop_ow_cfg, sc->tile.out_l_width, sc->tile.out.w, false);
				sclr_gop_ow_set_cfg(inst, j, i, &gop_ow_cfg, false);
			}
		}
		TRACE_VPSS(DBG_DEBUG, "gop_cfg:%#x\n", gop_cfg.gop_ctrl.raw);
		sclr_gop_set_cfg(inst, j, &gop_cfg, false);
	}

	sclr_reg_force_up(inst);
	return true;
}

bool sclr_top_tile(u8 inst, u16 src_l_h, u8 is_right)
{
	struct sclr_scale_cfg *sc;
	struct sclr_odma_cfg *odma_cfg;
	struct sclr_rect crop;
	struct sclr_size src, dst;
	struct sclr_gop_cfg gop_cfg;
	struct sclr_gop_ow_cfg gop_ow_cfg;
	int i = 0, j = 0;

	if (inst >= SCL_MAX_INST) {
		TRACE_VPSS(DBG_ERR, "no enough sclr-instance ");
		TRACE_VPSS(DBG_ERR, "for the requirement(%d)\n", inst);
		return false;
	}

	sc = &(sclr_get_cfg(inst)->sc);
	odma_cfg = sclr_odma_get_cfg(inst);

	sc->v_tile_enable = true;

	// skip if crop in the right tile.
	if (sc->v_tile.src_l_width == 0) {
		sclr_top_set_cfg(inst, false, false);
		return false;
	}
	sclr_top_set_cfg(inst, true, g_fbd_cfg[inst].enable);

	src.w = sc->src.w;
	src.h = src_l_h;
	crop.x = sc->crop.x;
	crop.y = sc->crop.y;
	crop.w = sc->crop.w;
	crop.h = sc->v_tile.src_l_width;
	dst.w = sc->dst.w;
	dst.h = sc->v_tile.out_l_width;

	sclr_set_input_size(inst, src, false);
	sclr_set_crop(inst, crop, false);
	sclr_set_output_size(inst, dst);

	if (sc->v_tile.border_enable && (!is_right)) {
		odma_cfg->mem.start_y = 0;
		odma_cfg->mem.height = sc->v_tile.dma_l_width;
		sclr_odma_set_mem(inst, &odma_cfg->mem);

		g_bd_cfg[inst].cfg.b.enable = true;
		g_bd_cfg[inst].start.y = sc->v_tile.dma_l_y;
		sclr_border_set_cfg(inst, &g_bd_cfg[inst]);
		TRACE_VPSS(DBG_DEBUG, "sc%d border start: x=%d y=%d\n", inst, g_bd_cfg[inst].start.x,
			g_bd_cfg[inst].start.y);
	} else {
		odma_cfg->mem.start_y = sc->v_tile.dma_l_y;
		odma_cfg->mem.height = dst.h;
		sclr_odma_set_mem(inst, &odma_cfg->mem);
	}
	TRACE_VPSS(DBG_DEBUG, "sc%d input size: w=%d h=%d\n", inst, src.w, src.h);
	TRACE_VPSS(DBG_DEBUG, "sc%d crop size: x=%d y=%d w=%d h=%d\n", inst, crop.x, crop.y, crop.w, crop.h);
	TRACE_VPSS(DBG_DEBUG, "sc%d output size: w=%d h=%d\n", inst, dst.w, dst.h);
	TRACE_VPSS(DBG_DEBUG, "sc%d odma mem: x=%d y=%d w=%d h=%d pitch_y=%d pitch_c=%d\n", inst,
		odma_cfg->mem.start_x, odma_cfg->mem.start_y, odma_cfg->mem.width,
		odma_cfg->mem.height, odma_cfg->mem.pitch_y, odma_cfg->mem.pitch_c);

	for (j = 0; j < SCL_MAX_GOP_INST; ++j) {
		gop_cfg = *sclr_gop_get_cfg(inst, j);
		for (i = 0; i < SCL_MAX_GOP_OW_INST; ++i) {
			if (gop_cfg.gop_ctrl.raw & BIT(i)) {
				gop_ow_cfg = gop_cfg.ow_cfg[i];
				TRACE_VPSS(DBG_DEBUG, "gop(%d) starty(%d) endy(%d), tile_l_width(%d)\n",
					i, gop_ow_cfg.start.y, gop_ow_cfg.end.y, sc->v_tile.out_l_width);

				// gop-window on right tile: pass
				if (gop_ow_cfg.start.y >= sc->v_tile.out_l_width) {
					gop_cfg.gop_ctrl.raw &= ~BIT(i);
					continue;
				}
				// gop-window on left tile: ok to go
				if (gop_ow_cfg.end.y <= sc->v_tile.out_l_width) {
					sclr_gop_ow_set_cfg(inst, j, i, &gop_ow_cfg, false);
					continue;
				}

				// gop-window on both tile: workaround
				_gop_v_tile_shift(&gop_ow_cfg, sc->v_tile.out_l_width, sc->v_tile.out.w, sc->tile.out_l_width, false);
				sclr_gop_ow_set_cfg(inst, j, i, &gop_ow_cfg, false);
			}
		}
		TRACE_VPSS(DBG_DEBUG, "gop_cfg:%#x\n", gop_cfg.gop_ctrl.raw);
		sclr_gop_set_cfg(inst, j, &gop_cfg, false);
	}

	sclr_reg_force_up(inst);
	return true;
}

/**
 * sclr_right_tile - update parameters for right tile
 *
 * @param inst: (0~3), the instance of sc
 * @param src_offset: offset of the right tile relative to original image
 * @return: true if success; false if no need or something wrong
 */
bool sclr_right_tile(u8 inst, u16 src_offset)
{
	struct sclr_scale_cfg *sc;
	struct sclr_odma_cfg *odma_cfg;
	struct sclr_rect crop;
	struct sclr_size src, dst;
	struct sclr_gop_cfg gop_cfg;
	struct sclr_gop_ow_cfg gop_ow_cfg;
	struct sclr_border_vpp_cfg border_vpp_cfg;
	struct sclr_cover_cfg cover_cfg;
	int i = 0, j = 0;
	u8 is_need_fill = 0;

	if (inst >= SCL_MAX_INST) {
		TRACE_VPSS(DBG_DEBUG, "no enough sclr-instance ");
		TRACE_VPSS(DBG_DEBUG, "for the requirement(%d)\n", inst);
		return false;
	}
	sc = &(sclr_get_cfg(inst)->sc);
	odma_cfg = sclr_odma_get_cfg(inst);

	// skip if crop in the right tile.
	if (sc->tile.src_r_width == 0) {
		sclr_top_set_cfg(inst, false, false);
		sc->tile_enable = false;
		return false;
	}
	sclr_top_set_cfg(inst, true, g_fbd_cfg[inst].enable);

	src.w = sc->tile.in_mem.w - src_offset;
	src.h = sc->src.h;
	crop.x = sc->crop.x + sc->tile.src_r_offset - src_offset;
	crop.y = sc->crop.y;
	crop.w = sc->tile.src_r_width;
	crop.h = sc->tile.src.h;
	dst.w = sc->tile.out.w - sc->tile.out_l_width;
	dst.h = sc->tile.out.h;

	sclr_set_input_size(inst, src, true);
	sclr_set_crop(inst, crop, true);
	sclr_set_output_size(inst, dst);
	sclr_set_scale_phase(inst, sc->tile.r_ini_phase, sc->fac.v_pos);

	odma_cfg->mem.start_x = sc->tile.dma_r_x;
	odma_cfg->mem.start_y = sc->tile.dma_r_y;
	odma_cfg->mem.width = dst.w;
	odma_cfg->mem.height = dst.h;
	sclr_odma_set_mem(inst, &odma_cfg->mem);

	// right tile don't do border.
	g_bd_cfg[inst].cfg.b.enable = false;
	sclr_border_set_cfg(inst, &g_bd_cfg[inst]);

	TRACE_VPSS(DBG_DEBUG, "sc%d input size: w=%d h=%d\n", inst, src.w, src.h);
	TRACE_VPSS(DBG_DEBUG, "sc%d crop size: x=%d y=%d w=%d h=%d\n", inst, crop.x, crop.y, crop.w, crop.h);
	TRACE_VPSS(DBG_DEBUG, "sc%d output size: w=%d h=%d\n", inst, dst.w, dst.h);
	TRACE_VPSS(DBG_DEBUG, "sc%d odma mem: x=%d y=%d w=%d h=%d pitch_y=%d pitch_c=%d\n", inst,
		odma_cfg->mem.start_x, odma_cfg->mem.start_y, odma_cfg->mem.width,
		odma_cfg->mem.height, odma_cfg->mem.pitch_y, odma_cfg->mem.pitch_c);

	for (j = 0; j < COVER_MAX; ++j){
		cover_cfg = g_sc_cfg[inst].cover_cfg[j];
		if(cover_cfg.start.b.enable){
			if((cover_cfg.img_size.w + cover_cfg.start.b.x) < sc->tile.out_l_width)
				cover_cfg.img_size.w = 0;
			else if((cover_cfg.img_size.w + cover_cfg.start.b.x) > sc->tile.out_l_width && cover_cfg.start.b.x < sc->tile.out_l_width)
				cover_cfg.img_size.w = cover_cfg.img_size.w + cover_cfg.start.b.x - sc->tile.out_l_width;
			if(cover_cfg.start.b.x < sc->tile.out_l_width)
				cover_cfg.start.b.x = 0;
			else
				cover_cfg.start.b.x -= sc->tile.out_l_width;
			if(cover_cfg.img_size.w == 0)
				cover_cfg.start.b.enable = false;
			TRACE_VPSS(DBG_DEBUG, "right_tile: cover(%d) start.x(%d) img_size.w(%d), v_tile_l_width(%d)\n",
				j, cover_cfg.start.b.x, cover_cfg.img_size.w, sc->tile.out_l_width);
		}
		sclr_cover_set_cfg(inst, j, &cover_cfg, true);
	}
	for (j = 0; j < SCL_MAX_GOP_INST; ++j) {
		gop_cfg = *sclr_gop_get_cfg(inst, j);
		for (i = 0; i < SCL_MAX_GOP_OW_INST; ++i) {
			if (gop_cfg.gop_ctrl.raw & BIT(i)) {
				gop_ow_cfg = gop_cfg.ow_cfg[i];
				TRACE_VPSS(DBG_DEBUG, "gop(%d) startx(%d) endx(%d), tile_l_width(%d)\n",
					i, gop_ow_cfg.start.x, gop_ow_cfg.end.x, sc->tile.out_l_width);

				// gop-window on left tile: pass
				if (gop_ow_cfg.end.x <= sc->tile.out_l_width) {
					gop_cfg.gop_ctrl.raw &= ~BIT(i);
					continue;
				}
				// gop-window on right tile: ok to go
				if (gop_ow_cfg.start.x >= sc->tile.out_l_width) {
					gop_ow_cfg.start.x -= sc->tile.out_l_width;
					gop_ow_cfg.end.x -= sc->tile.out_l_width;
					sclr_gop_ow_set_cfg(inst, j, i, &gop_ow_cfg, false);
					continue;
				}

				// gop-window on both tile: workaround
				_gop_tile_shift(&gop_ow_cfg, sc->tile.out_l_width, sc->tile.out.w, true);
				sclr_gop_ow_set_cfg(inst, j, i, &gop_ow_cfg, false);
			}
		}
		TRACE_VPSS(DBG_DEBUG, "gop_cfg:%#x\n", gop_cfg.gop_ctrl.raw);
		sclr_gop_set_cfg(inst, j, &gop_cfg, false);
	}
	for(j = 0; j < BORDER_VPP_MAX; ++j){
		border_vpp_cfg = *sclr_border_vpp_get_cfg(inst, j);
		is_need_fill = 0;
		if(border_vpp_cfg.cfg.b.enable){
			if(border_vpp_cfg.inside_end.x < sc->tile.out_l_width && border_vpp_cfg.outside_end.x > sc->tile.out_l_width)
				is_need_fill = 1;
			if(border_vpp_cfg.inside_start.x < sc->tile.out_l_width)
				border_vpp_cfg.inside_start.x = 0;
			else
				border_vpp_cfg.inside_start.x -= sc->tile.out_l_width;
			if(border_vpp_cfg.inside_end.x < sc->tile.out_l_width)
				border_vpp_cfg.inside_end.x = 0;
			else
				border_vpp_cfg.inside_end.x -= sc->tile.out_l_width;
			if(border_vpp_cfg.outside_start.x < sc->tile.out_l_width)
				border_vpp_cfg.outside_start.x = 0;
			else
				border_vpp_cfg.outside_start.x -= sc->tile.out_l_width;
			if(border_vpp_cfg.outside_end.x < sc->tile.out_l_width)
				border_vpp_cfg.outside_end.x = 0;
			else
				border_vpp_cfg.outside_end.x -= sc->tile.out_l_width;
			if(border_vpp_cfg.inside_start.x == border_vpp_cfg.inside_end.x && border_vpp_cfg.outside_start.x == border_vpp_cfg.outside_end.x)
				border_vpp_cfg.cfg.b.enable = false;
			if(is_need_fill){
				border_vpp_cfg.inside_start.y = border_vpp_cfg.outside_end.y + 1;
				border_vpp_cfg.inside_end.x = border_vpp_cfg.outside_end.x;
				border_vpp_cfg.inside_end.y = border_vpp_cfg.inside_start.y;
				border_vpp_cfg.outside_end.y += 1;
			}
			TRACE_VPSS(DBG_DEBUG, "right_tile: border_vpp(%d) inside_start_x(%d) inside_end_x(%d) outside_start_x(%d) outside_end_x(%d), tile_l_width(%d)\n",
				j, border_vpp_cfg.inside_start.x, border_vpp_cfg.inside_end.x, border_vpp_cfg.outside_start.x, border_vpp_cfg.outside_end.x, sc->tile.out_l_width);
		}
		sclr_border_vpp_set_cfg(inst, j, &border_vpp_cfg, true);
	}
	sclr_reg_force_up(inst);

	sc->tile_enable = false;
	return true;
}

bool sclr_down_tile(u8 inst, u16 src_offset, u8 is_right)
{
	struct sclr_scale_cfg *sc;
	struct sclr_odma_cfg *odma_cfg;
	struct sclr_rect crop;
	struct sclr_size src, dst;
	struct sclr_gop_cfg gop_cfg;
	struct sclr_gop_ow_cfg gop_ow_cfg;
	struct sclr_border_vpp_cfg border_vpp_cfg;
	struct sclr_cover_cfg cover_cfg;
	int i = 0, j = 0;
	u8 is_need_fill = 0;

	if (inst >= SCL_MAX_INST) {
		TRACE_VPSS(DBG_DEBUG, "no enough sclr-instance ");
		TRACE_VPSS(DBG_DEBUG, "for the requirement(%d)\n", inst);
		return false;
	}
	sc = &(sclr_get_cfg(inst)->sc);
	odma_cfg = sclr_odma_get_cfg(inst);

	// skip if crop in the right tile.
	if (sc->v_tile.src_r_width == 0) {
		sclr_top_set_cfg(inst, false, false);
		sc->v_tile_enable = false;
		return false;
	}
	sclr_top_set_cfg(inst, true, g_fbd_cfg[inst].enable);

	src.w = sc->src.w;
	src.h = sc->v_tile.in_mem.h - src_offset;
	crop.x = sc->crop.x;
	crop.y = sc->crop.y + sc->v_tile.src_r_offset - src_offset;
	crop.w = sc->crop.w;
	crop.h = sc->v_tile.src_r_width;
	dst.w = sc->dst.w;
	dst.h = sc->v_tile.out.h - sc->v_tile.out_l_width;

	sclr_set_input_size(inst, src, false);
	sclr_set_crop(inst, crop, false);
	sclr_set_output_size(inst, dst);
	if(is_right)
		sclr_set_scale_phase(inst, sc->tile.r_ini_phase, sc->v_tile.r_ini_phase);
	else
		sclr_set_scale_phase(inst, sc->fac.h_pos, sc->v_tile.r_ini_phase);

	odma_cfg->mem.start_y = sc->v_tile.dma_r_y;
	odma_cfg->mem.width = dst.w;
	odma_cfg->mem.height = dst.h;
	sclr_odma_set_mem(inst, &odma_cfg->mem);

	// down tile don't do border.
	g_bd_cfg[inst].cfg.b.enable = false;
	sclr_border_set_cfg(inst, &g_bd_cfg[inst]);

	TRACE_VPSS(DBG_DEBUG, "sc%d input size: w=%d h=%d\n", inst, src.w, src.h);
	TRACE_VPSS(DBG_DEBUG, "sc%d crop size: x=%d y=%d w=%d h=%d\n", inst, crop.x, crop.y, crop.w, crop.h);
	TRACE_VPSS(DBG_DEBUG, "sc%d output size: w=%d h=%d\n", inst, dst.w, dst.h);
	TRACE_VPSS(DBG_DEBUG, "sc%d odma mem: x=%d y=%d w=%d h=%d pitch_y=%d pitch_c=%d\n", inst,
		odma_cfg->mem.start_x, odma_cfg->mem.start_y, odma_cfg->mem.width,
		odma_cfg->mem.height, odma_cfg->mem.pitch_y, odma_cfg->mem.pitch_c);

	for(j = 0; j < COVER_MAX; ++j){
		cover_cfg = g_sc_cfg[inst].cover_cfg[j];
		if(cover_cfg.start.b.enable){
			if((cover_cfg.img_size.h + cover_cfg.start.b.y) < sc->v_tile.out_l_width)
				cover_cfg.img_size.h = 0;
			else if((cover_cfg.img_size.h + cover_cfg.start.b.y) > sc->v_tile.out_l_width && cover_cfg.start.b.y < sc->v_tile.out_l_width)
				cover_cfg.img_size.h = cover_cfg.img_size.h + cover_cfg.start.b.y - sc->v_tile.out_l_width;
			if(cover_cfg.start.b.y < sc->v_tile.out_l_width)
				cover_cfg.start.b.y = 0;
			else
				cover_cfg.start.b.y -= sc->v_tile.out_l_width;
			if(cover_cfg.img_size.h == 0)
				cover_cfg.start.b.enable = false;
			TRACE_VPSS(DBG_DEBUG, "down_tile: cover(%d) start.y(%d) img_size.h(%d), v_tile_l_width(%d)\n",
				j, cover_cfg.start.b.y, cover_cfg.img_size.h, sc->v_tile.out_l_width);
		}
		sclr_cover_set_cfg(inst, j, &cover_cfg, false);
	}
	for (j = 0; j < SCL_MAX_GOP_INST; ++j) {
		gop_cfg = *sclr_gop_get_cfg(inst, j);
		for (i = 0; i < SCL_MAX_GOP_OW_INST; ++i) {
			if (gop_cfg.gop_ctrl.raw & BIT(i)) {
				gop_ow_cfg = gop_cfg.ow_cfg[i];
				TRACE_VPSS(DBG_DEBUG, "gop(%d) starty(%d) endy(%d), v_tile_l_width(%d)\n",
					i, gop_ow_cfg.start.y, gop_ow_cfg.end.y, sc->v_tile.out_l_width);

				// gop-window on left tile: pass
				if (gop_ow_cfg.end.y <= sc->v_tile.out_l_width) {
					gop_cfg.gop_ctrl.raw &= ~BIT(i);
					continue;
				}
				// gop-window on right tile: ok to go
				if (gop_ow_cfg.start.y >= sc->v_tile.out_l_width) {
					gop_ow_cfg.start.y -= sc->v_tile.out_l_width;
					gop_ow_cfg.end.y -= sc->v_tile.out_l_width;
					sclr_gop_ow_set_cfg(inst, j, i, &gop_ow_cfg, false);
					continue;
				}

				// gop-window on both tile: workaround
				_gop_v_tile_shift(&gop_ow_cfg, sc->v_tile.out_l_width, sc->v_tile.out.h, sc->tile.out_l_width, true);
				sclr_gop_ow_set_cfg(inst, j, i, &gop_ow_cfg, false);
			}
		}
		TRACE_VPSS(DBG_DEBUG, "gop_cfg:%#x\n", gop_cfg.gop_ctrl.raw);
		sclr_gop_set_cfg(inst, j, &gop_cfg, false);
	}
	for(j = 0; j < BORDER_VPP_MAX; ++j){
		border_vpp_cfg = *sclr_border_vpp_get_cfg(inst, j);
		is_need_fill = 0;
		if(border_vpp_cfg.cfg.b.enable){
			if(border_vpp_cfg.inside_end.y < sc->v_tile.out_l_width && border_vpp_cfg.outside_end.y > sc->v_tile.out_l_width)
				is_need_fill = 1;
			if(border_vpp_cfg.inside_start.y < sc->v_tile.out_l_width)
				border_vpp_cfg.inside_start.y = 0;
			else
				border_vpp_cfg.inside_start.y -= sc->v_tile.out_l_width;
			if(border_vpp_cfg.inside_end.y < sc->v_tile.out_l_width)
				border_vpp_cfg.inside_end.y = 0;
			else
				border_vpp_cfg.inside_end.y -= sc->v_tile.out_l_width;
			if(border_vpp_cfg.outside_start.y < sc->v_tile.out_l_width)
				border_vpp_cfg.outside_start.y = 0;
			else
				border_vpp_cfg.outside_start.y -= sc->v_tile.out_l_width;
			if(border_vpp_cfg.outside_end.y < sc->v_tile.out_l_width)
				border_vpp_cfg.outside_end.y = 0;
			else
				border_vpp_cfg.outside_end.y -= sc->v_tile.out_l_width;
			if(border_vpp_cfg.inside_start.y == border_vpp_cfg.inside_end.y && border_vpp_cfg.outside_start.y == border_vpp_cfg.outside_end.y)
				border_vpp_cfg.cfg.b.enable = false;
			if(is_need_fill){
				border_vpp_cfg.inside_start.x = border_vpp_cfg.outside_end.x + 1;
				border_vpp_cfg.inside_end.y = border_vpp_cfg.outside_end.y;
				border_vpp_cfg.inside_end.x = border_vpp_cfg.inside_start.x;
				border_vpp_cfg.outside_end.x += 1;
			}
			TRACE_VPSS(DBG_DEBUG, "down_tile: border_vpp(%d) inside_start_y(%d) inside_end_y(%d) outside_start_y(%d) outside_end_y(%d), tile_l_width(%d)\n",
				j, border_vpp_cfg.inside_start.y, border_vpp_cfg.inside_end.y, border_vpp_cfg.outside_start.y, border_vpp_cfg.outside_end.y, sc->v_tile.out_l_width);
		}
		sclr_border_vpp_set_cfg(inst, j, &border_vpp_cfg, false);
	}
	sclr_reg_force_up(inst);
	return true;
}


void sclr_dump_top_register(u8 inst)
{
	u32 val;
	uintptr_t top_base = reg_base + REG_SCL_TOP_BASE(inst);

	if (inst >= SCL_MAX_INST)
		TRACE_VPSS(DBG_DEBUG, "inst(%d) out of range\n", inst);

	TRACE_VPSS(DBG_DEBUG, "vpss(%d) top base address=0x%lx\n", inst, top_base);

	val = _reg_read(reg_base + REG_SCL_TOP_CFG1(inst));
	TRACE_VPSS(DBG_DEBUG, "\t sc_en=%d, fbd_en=%d, dma_qos_en=0x%x\n",
		(val >> 1) & 0x1, (val >> 5) & 0x1, (val >> 16) & 0xff);

	val = _reg_read(reg_base + REG_SCL_TOP_INTR_STATUS(inst));
	TRACE_VPSS(DBG_DEBUG, "\t interrupter status=0x%x, img_in satrt=%d, img_in end=%d, sc end=%d\n",
		val, (val >> 4) & 0x1, (val >> 5) & 0x1, (val >> 7) & 0x1);

	val = _reg_read(reg_base + REG_SCL_TOP_IMG_CTRL(inst));
	TRACE_VPSS(DBG_DEBUG, "\t trig_by_isp=%d trig_by_disp=%d\n",
		(val >> 13) & 0x1, (val >> 9) & 0x1);

	val = _reg_read(reg_base + REG_SCL_TOP_SRC_SHARE(inst));
	TRACE_VPSS(DBG_DEBUG, "\t share=%d\n", val & 0x1);

}

void sclr_dump_img_in_register(int inst)
{
	u32 val, val2;
	union sclr_img_dbg_status status;
	uintptr_t img_base = reg_base + REG_SCL_IMG_BASE(inst);

	if (inst >= SCL_MAX_INST)
		TRACE_VPSS(DBG_DEBUG, "inst(%d) out of range\n", inst);

	TRACE_VPSS(DBG_DEBUG, "img(%d) base addr 0x%lx\n", inst, img_base);

	val = _reg_read(reg_base + REG_SCL_IMG_CFG(inst));
	TRACE_VPSS(DBG_DEBUG, "\t source=%d, fmt=%d, csc_en=%d\n",
		val & 0x3, (val >> 4) & 0xf, (val >> 12) & 0x1);

	val = _reg_read(reg_base + REG_SCL_IMG_OFFSET(inst));
	val2 = _reg_read(reg_base + REG_SCL_IMG_SIZE(inst));
	TRACE_VPSS(DBG_DEBUG, "\t crop(%d %d %d %d)\n",
		val & 0xffff, (val >> 16) & 0xffff, val2 & 0xffff, (val2 >> 16) & 0xffff);

	val = _reg_read(reg_base + REG_SCL_IMG_PITCH_Y(inst));
	val2 = _reg_read(reg_base + REG_SCL_IMG_PITCH_C(inst));
	TRACE_VPSS(DBG_DEBUG, "\t pitch_y=%d pitch_c=%d\n",
		val & 0xfffffff, val2 & 0xfffffff);

	val = _reg_read(reg_base + REG_SCL_IMG_ADDR0_L(inst));
	val2 = _reg_read(reg_base + REG_SCL_IMG_ADDR0_H(inst));
	TRACE_VPSS(DBG_DEBUG, "\t Y: addr_h=0x%x addr_l=0x%x\n",
		val2 & 0xff, val);

	val = _reg_read(reg_base + REG_SCL_IMG_ADDR1_L(inst));
	val2 = _reg_read(reg_base + REG_SCL_IMG_ADDR1_H(inst));
	TRACE_VPSS(DBG_DEBUG, "\t U: addr_h=0x%x addr_l=0x%x\n",
		val2 & 0xff, val);

	val = _reg_read(reg_base + REG_SCL_IMG_ADDR2_L(inst));
	val2 = _reg_read(reg_base + REG_SCL_IMG_ADDR2_H(inst));
	TRACE_VPSS(DBG_DEBUG, "\t V: addr_h=0x%x addr_l=0x%x\n",
		val2 & 0xff, val);

	status = sclr_img_get_dbg_status(inst, true);

	TRACE_VPSS(DBG_DEBUG, "\t err_fwr_yuv(%d%d%d err_erd_yuv(%d%d%d) "
									"lb_full_yuv(%d%d%d) lb_empty_yuv(%d%d%d)\n"
		  , status.b.err_fwr_y, status.b.err_fwr_u,  status.b.err_fwr_v, status.b.err_erd_y
		  , status.b.err_erd_u, status.b.err_erd_v, status.b.lb_full_y, status.b.lb_full_u
		  , status.b.lb_full_v, status.b.lb_empty_y, status.b.lb_empty_u, status.b.lb_empty_v);
	TRACE_VPSS(DBG_DEBUG, "\t ip idle(%d) ip int(%d)\n", status.b.ip_idle, status.b.ip_int);

}

void sclr_dump_core_register(int inst)
{
	u32 val, val2;
	struct sclr_status status;
	uintptr_t core_base = reg_base + REG_SCL_CORE_BASE(inst);

	if (inst >= SCL_MAX_INST)
		TRACE_VPSS(DBG_DEBUG, "sc(%d) out of range\n", inst);

	TRACE_VPSS(DBG_DEBUG, "sc(%d) core base address=0x%lx\n", inst, core_base);

	val = _reg_read(reg_base + REG_SCL_CFG(inst));
	TRACE_VPSS(DBG_DEBUG, "\t bypass: sc_core(%d) vgop(%d) cir(%d) dma(%d)\n",
		(val >> 1) & 0x1, (val >> 2) & 0x1, (val >> 5) & 0x1, (val >> 6) & 0x1);

	val = _reg_read(reg_base + REG_SCL_SRC_SIZE(inst));
	TRACE_VPSS(DBG_DEBUG, "\t src_w=%d src_h=%d\n",
		val & 0x3fff, (val >> 16) & 0x3fff);

	val = _reg_read(reg_base + REG_SCL_CROP_OFFSET(inst));
	val2 = _reg_read(reg_base + REG_SCL_CROP_SIZE(inst));
	TRACE_VPSS(DBG_DEBUG, "\t crop(%d %d %d %d)\n",
		val & 0x3fff, (val >> 16) & 0x3fff, val2 & 0x3fff, (val2 >> 16) & 0x3fff);

	val = _reg_read(reg_base + REG_SCL_OUT_SIZE(inst));
	TRACE_VPSS(DBG_DEBUG, "\t out_w=%d out_h=%d\n",
		val & 0x3fff, (val >> 16) & 0x3fff);

	status = sclr_get_status(inst);

	TRACE_VPSS(DBG_DEBUG, "\t crop(%d) hscale(%d) vscale(%d) gop(%d) dma(%d)\n",
		status.crop_idle, status.hscale_idle, status.vscale_idle, status.gop_idle, status.wdma_idle);
}

void sclr_dump_odma_register(u8 inst)
{
	u32 val, val2;
	uintptr_t odma_base = reg_base + REG_SCL_ODMA_BASE(inst);

	if (inst >= SCL_MAX_INST)
		TRACE_VPSS(DBG_DEBUG, "sc(%d) out of range\n", inst);

	TRACE_VPSS(DBG_DEBUG, "sc(%d) odma base address=0x%lx\n", inst, odma_base);

	_reg_write(reg_base + REG_SCL_ODMA_LATCH_LINE_CNT(inst), 0x1);
	val = _reg_read(reg_base + REG_SCL_ODMA_LATCH_LINE_CNT(inst));
	TRACE_VPSS(DBG_DEBUG, "\t latch line count=%d\n", val >> 8);

	val = _reg_read(reg_base + REG_SCL_ODMA_CFG(inst));
	TRACE_VPSS(DBG_DEBUG, "\t fmt=%d hflip=%d vflip=%d\n",
		(val >> 8) & 0xf, (val >> 16) & 0x1, (val >> 17) & 0x1);
	TRACE_VPSS(DBG_DEBUG, "\t bf16_en=%d fp16_en=%d fp32_en=%d int8_en=%d uint8_en=%d convert_en=%d\n",
		(val >> 23) & 0x1, (val >> 24) & 0x1, (val >> 25) & 0x1,
		(val >> 26) & 0x1, (val >> 27) & 0x1, (val >> 28) & 0x1);

	val = _reg_read(reg_base + REG_SCL_ODMA_ADDR0_L(inst));
	val2 = _reg_read(reg_base + REG_SCL_ODMA_ADDR0_H(inst));
	TRACE_VPSS(DBG_DEBUG, "\t Y: addr_h=0x%x addr_l=0x%x\n",
		val2 & 0xff, val);

	val = _reg_read(reg_base + REG_SCL_ODMA_ADDR1_L(inst));
	val2 = _reg_read(reg_base + REG_SCL_ODMA_ADDR1_H(inst));
	TRACE_VPSS(DBG_DEBUG, "\t U: addr_h=0x%x addr_l=0x%x\n",
		val2 & 0xff, val);

	val = _reg_read(reg_base + REG_SCL_ODMA_ADDR2_L(inst));
	val2 = _reg_read(reg_base + REG_SCL_ODMA_ADDR2_H(inst));
	TRACE_VPSS(DBG_DEBUG, "\t V: addr_h=0x%x addr_l=0x%x\n",
		val2 & 0xff, val);

	val = _reg_read(reg_base + REG_SCL_ODMA_PITCH_Y(inst));
	val2 = _reg_read(reg_base + REG_SCL_ODMA_PITCH_C(inst));
	TRACE_VPSS(DBG_DEBUG, "\t pitch_y=%d pitch_c=%d\n",
		val & 0xfffffff, val2 & 0xfffffff);

	TRACE_VPSS(DBG_DEBUG, "\t odma x=%d y=%d w=%d h=%d\n",
		_reg_read(reg_base + REG_SCL_ODMA_OFFSET_X(inst)),
		_reg_read(reg_base + REG_SCL_ODMA_OFFSET_Y(inst)),
		_reg_read(reg_base + REG_SCL_ODMA_WIDTH(inst)),
		_reg_read(reg_base + REG_SCL_ODMA_HEIGHT(inst)));
}

void sclr_dump_register(u8 inst) {
	int i;

	TRACE_VPSS(DBG_ERR, "---dump vpss(%d) register---\n", inst);
	TRACE_VPSS(DBG_ERR, "---sc top register---\n");
	for(i = 0; i <= 0xf4; i += 8){
		TRACE_VPSS(DBG_ERR, "addr:0x%x 0x%x  addr:0x%x 0x%x\n",
			(unsigned int)(i), (unsigned int)_reg_read(reg_base + REG_SCL_TOP_BASE(inst) + i),
			(unsigned int)(i + 0x4), (unsigned int)_reg_read(reg_base + REG_SCL_TOP_BASE(inst) + i + 0x4));
	}
	TRACE_VPSS(DBG_ERR, "addr:0x%x 0x%x  addr:0x%x 0x%x\n",
		(unsigned int)(0x104), (unsigned int)_reg_read(reg_base + REG_SCL_TOP_BASE(inst) + 0x104),
		(unsigned int)(0x108), (unsigned int)_reg_read(reg_base + REG_SCL_TOP_BASE(inst) + 0x108));
	for(i = 0x220; i <= 0x22c; i += 8){
		TRACE_VPSS(DBG_ERR, "addr:0x%x 0x%x  addr:0x%x 0x%x\n",
			(unsigned int)(i), (unsigned int)_reg_read(reg_base + REG_SCL_TOP_BASE(inst) + i),
			(unsigned int)(i + 0x4), (unsigned int)_reg_read(reg_base + REG_SCL_TOP_BASE(inst) + i + 0x4));
	}

	TRACE_VPSS(DBG_ERR, "---sc img register---\n");
	for(i = 0; i <= 0x9c; i += 8){
		TRACE_VPSS(DBG_ERR, "addr:0x%x 0x%x  addr:0x%x 0x%x\n",
			(unsigned int)(i), (unsigned int)_reg_read(reg_base + REG_SCL_IMG_BASE(inst) + i),
			(unsigned int)(i + 0x4), (unsigned int)_reg_read(reg_base + REG_SCL_IMG_BASE(inst) + i + 0x4));
	}

	TRACE_VPSS(DBG_ERR, "---sc fbd register---\n");
	for(i = 0; i <= 0x34; i += 8){
		TRACE_VPSS(DBG_ERR, "addr:0x%x 0x%x  addr:0x%x 0x%x\n",
			(unsigned int)(i), (unsigned int)_reg_read(reg_base + REG_SCL_FBD_BASE(inst) + i),
			(unsigned int)(i + 0x4), (unsigned int)_reg_read(reg_base + REG_SCL_FBD_BASE(inst) + i + 0x4));
	}

	TRACE_VPSS(DBG_ERR, "---sc core register---\n");
	for(i = 0; i <= 0x28; i += 8){
		TRACE_VPSS(DBG_ERR, "addr:0x%x 0x%x  addr:0x%x 0x%x\n",
			(unsigned int)(i), (unsigned int)_reg_read(reg_base + REG_SCL_CORE_BASE(inst) + i),
			(unsigned int)(i + 0x4), (unsigned int)_reg_read(reg_base + REG_SCL_CORE_BASE(inst) + i + 0x4));
	}
	for(i = 0x40; i <= 0x4c; i += 8){
		TRACE_VPSS(DBG_ERR, "addr:0x%x 0x%x  addr:0x%x 0x%x\n",
			(unsigned int)(i), (unsigned int)_reg_read(reg_base + REG_SCL_CORE_BASE(inst) + i),
			(unsigned int)(i + 0x4), (unsigned int)_reg_read(reg_base + REG_SCL_CORE_BASE(inst) + i + 0x4));
	}
	for(i = 0x80; i <= 0x9c; i += 8){
		TRACE_VPSS(DBG_ERR, "addr:0x%x 0x%x  addr:0x%x 0x%x\n",
			(unsigned int)(i), (unsigned int)_reg_read(reg_base + REG_SCL_CORE_BASE(inst) + i),
			(unsigned int)(i + 0x4), (unsigned int)_reg_read(reg_base + REG_SCL_CORE_BASE(inst) + i + 0x4));
	}
	for(i = 0x118; i <= 0x14c; i += 8){
		TRACE_VPSS(DBG_ERR, "addr:0x%x 0x%x  addr:0x%x 0x%x\n",
			(unsigned int)(i), (unsigned int)_reg_read(reg_base + REG_SCL_CORE_BASE(inst) + i),
			(unsigned int)(i + 0x4), (unsigned int)_reg_read(reg_base + REG_SCL_CORE_BASE(inst) + i + 0x4));
	}
	for(i = 0x200; i <= 0x218; i += 8){
		TRACE_VPSS(DBG_ERR, "addr:0x%x 0x%x  addr:0x%x 0x%x\n",
			(unsigned int)(i), (unsigned int)_reg_read(reg_base + REG_SCL_CORE_BASE(inst) + i),
			(unsigned int)(i + 0x4), (unsigned int)_reg_read(reg_base + REG_SCL_CORE_BASE(inst) + i + 0x4));
	}
	for(i = 0x280; i <= 0x2ac; i += 8){
		TRACE_VPSS(DBG_ERR, "addr:0x%x 0x%x  addr:0x%x 0x%x\n",
			(unsigned int)(i), (unsigned int)_reg_read(reg_base + REG_SCL_CORE_BASE(inst) + i),
			(unsigned int)(i + 0x4), (unsigned int)_reg_read(reg_base + REG_SCL_CORE_BASE(inst) + i + 0x4));
	}
	for(i = 0x300; i <= 0x370; i += 8){
		TRACE_VPSS(DBG_ERR, "addr:0x%x 0x%x  addr:0x%x 0x%x\n",
			(unsigned int)(i), (unsigned int)_reg_read(reg_base + REG_SCL_CORE_BASE(inst) + i),
			(unsigned int)(i + 0x4), (unsigned int)_reg_read(reg_base + REG_SCL_CORE_BASE(inst) + i + 0x4));
	}

	TRACE_VPSS(DBG_ERR, "---sc odma register---\n");
	for(i = 0; i <= 0x5c; i += 8){
		TRACE_VPSS(DBG_ERR, "addr:0x%x 0x%x  addr:0x%x 0x%x\n",
			(unsigned int)(i), (unsigned int)_reg_read(reg_base + REG_SCL_ODMA_BASE(inst) + i),
			(unsigned int)(i + 0x4), (unsigned int)_reg_read(reg_base + REG_SCL_ODMA_BASE(inst) + i + 0x4));
	}
	for(i = 0x100; i <= 0x138; i += 8){
		TRACE_VPSS(DBG_ERR, "addr:0x%x 0x%x  addr:0x%x 0x%x\n",
			(unsigned int)(i), (unsigned int)_reg_read(reg_base + REG_SCL_ODMA_BASE(inst) + i),
			(unsigned int)(i + 0x4), (unsigned int)_reg_read(reg_base + REG_SCL_ODMA_BASE(inst) + i + 0x4));
	}
}
