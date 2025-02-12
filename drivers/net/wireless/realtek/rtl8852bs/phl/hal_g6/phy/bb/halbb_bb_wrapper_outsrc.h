/******************************************************************************
 *
 * Copyright(c) 2019 - 2021 Realtek Corporation.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 *****************************************************************************/
#ifndef __HALBB_BB_WRAPPER_OUTSRC_H__
#define __HALBB_BB_WRAPPER_OUTSRC_H__
#include "halbb_ic_hw_info.h"

/*--------------------------------------------------------------------------*/
/*[TX Power Unit(TPU) array size]*/
#define TPU_SIZE_PWR_TAB	16 /*MCS0~MCS11(12) + {dcm_0,1,3,4}4 = 16*/
#define TPU_SIZE_PWR_TAB_BE	18 /*MCS0~MCS13(14) + {dcm_0,1,3,4}4 = 18*/

#define TPU_SIZE_PWR_TAB_CCK	4 /*1M, 2M, 5p5M, 11M*/
#define TPU_SIZE_PWR_TAB_OFDM	8 /*6M, 9M, 12M, 18M, 24M, 36M, 48M, 54M*/
#define TPU_SIZE_PWR_TAB_lGCY	12 /*cck(4) + ofdm(8) = 12*/
#define TPU_SIZE_PWR_TAB_EHT_BE	4 /*MCS14, MCS15, dlru_MCS14, dlru_MCS15*/

#define TPU_SIZE_PWR_TAB_DBW 	5 /*DBW20, 40, 80, 160, 320*/
#define TPU_SIZE_PWR_TAB_DBW_CCK	2 /*DBW20, 40*/

#define TPU_SIZE_MODE		5  /*0~4: HE, VHT, HT, Legacy, CCK, */
#define TPU_SIZE_MODE_BE	8  /*0~7: CCK, Legacy, HT, VHT, HE, EHT, dlru_HE, dlru_EHT*/

#define TPU_SIZE_BW		5 /*0~4: 80_80, 160, 80, 40, 20*/
#define TPU_SIZE_BW_BE		10 /*0~9: 20, 40, 80, 160, 320, dlru_20, dlru_40, dlru_80, dlru_160, dlru_320*/
#define TPU_SIZE_RUA		3 /* {26, 52, 106} */

#define TPU_SIZE_BW20_SC	8 /* 8 * 20M = 160M */
#define TPU_SIZE_BW40_SC	4 /* 4 * 40M = 160M */
#define TPU_SIZE_BW80_SC	2 /* 2 * 80M = 160M */
#define TPU_SIZE_BW20_SC_BE	16 /* 16 * 20M = 320M */
#define TPU_SIZE_BW40_SC_BE	8 /* 8 * 40M = 320M */
#define TPU_SIZE_BW80_SC_BE	4 /* 4 * 80M = 320M */
#define TPU_SIZE_BW160_SC_BE	2 /* 2 * 160M = 320M */ 

#define TPU_SIZE_BF		2 /*{NON_BF, BF}*/
#define TPU_SIZE_0P5_6P5 4 /* BW40_0p5, BW40_2p5, BW40_4p5, BW40_6p5 */
#define TPU_SIZE_CCK_LMT_BW		2 /* 20M, 40M */

/*--------------------------[Structure]-------------------------------------*/
enum rtw_tpu_op_mode {
	TPU_NORMAL_MODE		= 0,
	TPU_DBG_MODE		= 1
};

/*===== [AX] ===============================================================*/
#define AX_IC_TPU

#ifdef AX_IC_TPU

struct rtw_ext_pwr_lmt_info { /* external tx power limit information */
	s8 ext_pwr_lmt_2_4g[RTW_PHL_MAX_RF_PATH];
	s8 ext_pwr_lmt_5g_band1[RTW_PHL_MAX_RF_PATH]; /*CH36 ~ CH48*/
	s8 ext_pwr_lmt_5g_band2[RTW_PHL_MAX_RF_PATH]; /*CH52 ~ CH64*/
	s8 ext_pwr_lmt_5g_band3[RTW_PHL_MAX_RF_PATH]; /*CH100 ~ CH144*/
	s8 ext_pwr_lmt_5g_band4[RTW_PHL_MAX_RF_PATH]; /*CH149 ~ CH165*/
	s8 ext_pwr_lmt_6g[RTW_PHL_MAX_RF_PATH];
};

struct rtw_tpu_pwr_by_rate_info { /*TX Power Unit (TPU)*/
	s8 pwr_by_rate_lgcy[TPU_SIZE_PWR_TAB_lGCY];
	s8 pwr_by_rate[HALBB_MAX_PATH][TPU_SIZE_PWR_TAB];
};

struct rtw_tpu_pwr_imt_info { /*TX Power Unit (TPU)*/
	s8 pwr_lmt_cck_20m[HALBB_MAX_PATH][TPU_SIZE_BF];
	s8 pwr_lmt_cck_40m[HALBB_MAX_PATH][TPU_SIZE_BF];
	s8 pwr_lmt_lgcy_20m[HALBB_MAX_PATH][TPU_SIZE_BF]; /*ofdm*/
	s8 pwr_lmt_20m[HALBB_MAX_PATH][TPU_SIZE_BW20_SC][TPU_SIZE_BF];
	s8 pwr_lmt_40m[HALBB_MAX_PATH][TPU_SIZE_BW40_SC][TPU_SIZE_BF];
	s8 pwr_lmt_80m[HALBB_MAX_PATH][TPU_SIZE_BW80_SC][TPU_SIZE_BF];
	s8 pwr_lmt_160m[HALBB_MAX_PATH][TPU_SIZE_BF];
	s8 pwr_lmt_40m_0p5[HALBB_MAX_PATH][TPU_SIZE_BF];
	s8 pwr_lmt_40m_2p5[HALBB_MAX_PATH][TPU_SIZE_BF];
};

struct rtw_tpu_info { /*TX Power Unit (TPU)*/
	enum rtw_tpu_op_mode op_mode; /*In debug mode, only debug tool control TPU APIs*/
	bool normal_mode_lock_en;
	s8 ofst_int; /*SW: S(8,3) -16 ~ +15.875 (dB)*/
	u8 ofst_fraction; /*[0:3] * 0.125(dBm)*/
	enum bb_path ref_pow_path; /*Select the path with larger pow as the re_ path*/
	u8 path_pow_ofst_decrease; /* Select the path with lower pow and subtract pow_path_ofst_decrease from path_ref*/
	u8 base_cw_0db; /*[63~39~15]: [+24~0~-24 dBm]*/
	u16 tssi_16dBm_cw;
	/*[Ref Pwr]*/
	s16 ref_pow_ofdm; /*-> HW: s(9,2)*/
	s16 ref_pow_cck; /*-> HW: s(9,2)*/
	u16 ref_pow_ofdm_cw; /*BBCR 0x58E0[9:0]*/
	u16 ref_pow_cck_cw; /*BBCR 0x58E0[21:12]*/
	/*[Pwr Ofsset]*/ /*-> HW: s(7,1)*/
	s8 pwr_ofst_mode[TPU_SIZE_MODE]; /*0~4: HE, VHT, HT, Legacy, CCK, */
	s8 pwr_ofst_bw[TPU_SIZE_BW]; /*0~4: 80_80, 160, 80, 40, 20*/
	/*[Pwr By rate]*/ /*-> HW: s(7,1)*/
	struct rtw_tpu_pwr_by_rate_info rtw_tpu_pwr_by_rate_i;
	/*[Pwr Limit]*/ /*-> HW: s(7,1)*/
	struct rtw_tpu_pwr_imt_info rtw_tpu_pwr_imt_i;
	/*[Pwr Limit RUA]*/ /*-> HW: s(7,1)*/
	s8 pwr_lmt_ru[HALBB_MAX_PATH][TPU_SIZE_RUA][TPU_SIZE_BW20_SC];
	u16 pwr_lmt_ru_mem_size;
	bool pwr_lmt_en;
	bool ext_pwr_lmt_en;
	struct rtw_phl_ext_pwr_lmt_info ext_pwr_lmt_i;
	u8 tx_ptrn_shap_idx;
	u8 tx_ptrn_shap_idx_cck;
	u16 pwr_constraint_mb;
};

#endif

/*===== [BE] ===============================================================*/
#define BE_IC_TPU

#ifdef BE_IC_TPU
struct bb_tpu_pwr_by_rate_info_be { /*TX Power Unit (TPU)*/
	s8 pwr_by_rate_cck[TPU_SIZE_PWR_TAB_DBW_CCK][TPU_SIZE_PWR_TAB_CCK]; /* {DBW20, DBW40} x {1M, 2M, 5p5M, 11M}*/
	s8 pwr_by_rate_ofdm[TPU_SIZE_PWR_TAB_DBW][TPU_SIZE_PWR_TAB_OFDM]; /* {DBW20~DBW320} x {6M, 9M, 12M, 18M, 24M, 36M, 48M, 54M}*/
	s8 pwr_by_rate_mcs[TPU_SIZE_PWR_TAB_DBW][HALBB_MAX_PATH][TPU_SIZE_PWR_TAB_BE]; /* {DBW20~DBW320} x {{path_i}_{i is enable}} x {MCS_idx:0~13, dcm_0,1,3,4} */
	s8 pwr_by_rate_eht[TPU_SIZE_PWR_TAB_DBW][TPU_SIZE_PWR_TAB_EHT_BE]; /* {DBW20~DBW320} x {MCS14, MCS15, dlru_MCS14, dlru_MCS15} */
	s8 pwr_by_rate_dlru[TPU_SIZE_PWR_TAB_DBW][HALBB_MAX_PATH][TPU_SIZE_PWR_TAB_BE]; /* {DBW20~DBW320} x {{path_i}_{i is enable}} x {MCS_idx:0~13, dcm_0,1,3,4} */
};

struct bb_tpu_pwr_lmt_info_be { /*TX Power Unit (TPU)*/
	s8 pwr_lmt_cck[HALBB_MAX_PATH][TPU_SIZE_CCK_LMT_BW][TPU_SIZE_BF]; /* {{path_i}_{i is enable}} x {BW20, BW40} x {non-BF/BF} */
	s8 pwr_lmt_lgcy_non_dup[HALBB_MAX_PATH][TPU_SIZE_BF]; /* {{path_i}_{i is enable}} x {non-BF/BF} */
	s8 pwr_lmt_20m[HALBB_MAX_PATH][TPU_SIZE_BW20_SC_BE][TPU_SIZE_BF]; /* {{path_i}_{i is enable}} x {1,2,...,16} x {non-BF/BF} */
	s8 pwr_lmt_40m[HALBB_MAX_PATH][TPU_SIZE_BW40_SC_BE][TPU_SIZE_BF]; /* {{path_i}_{i is enable}} x {1,2,...,8} x {non-BF/BF} */
	s8 pwr_lmt_80m[HALBB_MAX_PATH][TPU_SIZE_BW80_SC_BE][TPU_SIZE_BF]; /* {{path_i}_{i is enable}} x {1,2,3,4} x {non-BF/BF} */
	s8 pwr_lmt_160m[HALBB_MAX_PATH][TPU_SIZE_BW160_SC_BE][TPU_SIZE_BF]; /* {{path_i}_{i is enable}} x {1,2} x {non-BF/BF} */
	s8 pwr_lmt_320m[HALBB_MAX_PATH][TPU_SIZE_BF]; /* {{path_i}_{i is enable}} x {non-BF/BF} */
	s8 pwr_lmt_40m_0p5[HALBB_MAX_PATH][TPU_SIZE_BF]; /* {{path_i}_{i is enable}} x {non-BF/BF} */
	s8 pwr_lmt_40m_2p5[HALBB_MAX_PATH][TPU_SIZE_BF]; /* {{path_i}_{i is enable}} x {non-BF/BF} */
	s8 pwr_lmt_40m_4p5[HALBB_MAX_PATH][TPU_SIZE_BF]; /* {{path_i}_{i is enable}} x {non-BF/BF} */
	s8 pwr_lmt_40m_6p5[HALBB_MAX_PATH][TPU_SIZE_BF]; /* {{path_i}_{i is enable}} x {non-BF/BF} */
};

struct bb_tpu_pwr_lmt_ru_info_be { /*TX Power Unit (TPU)*/
	s8 pwr_lmt_ru_be[HALBB_MAX_PATH][TPU_SIZE_RUA][TPU_SIZE_BW20_SC_BE]; /* {{path_i}_{i is enable}} x {26, 52, 106} x {0~15: 16 * 20M = 320M } */
	s8 pwr_lmt_ru52_26_be[HALBB_MAX_PATH][TPU_SIZE_BW20_SC_BE]; /* {{path_i}_{i is enable}} x {0~15: 16 * 20M = 320M } */
	s8 pwr_lmt_ru106_26_be[HALBB_MAX_PATH][TPU_SIZE_BW20_SC_BE]; /* {{path_i}_{i is enable}} x {0~15: 16 * 20M = 320M } */
};

struct bb_tpu_be_info { /*TX Power Unit (TPU)*/
	/*[Pwr By rate for BE]*/ /*-> HW: s(7,1)*/
	struct bb_tpu_pwr_by_rate_info_be rtw_tpu_pwr_by_rate_be_i;
	/*[Pwr Limit for BE]*/ /*-> HW: s(7,1)*/
	struct bb_tpu_pwr_lmt_info_be rtw_tpu_pwr_lmt_be_i;
	/*[Pwr Limit By RU for BE]*/ /*-> HW: s(7,1)*/
	struct bb_tpu_pwr_lmt_ru_info_be rtw_tpu_pwr_lmt_ru_be_i;
	s8 pwr_cck_dup_patha_l_pathb_h_2tx; /*-> HW: s(7.1)*/
	s8 pwr_cck_dup_patha_h_pathb_l_2tx; /*-> HW: s(7.1)*/
	/*[Ref Pwr]*/
	s16 ref_pow_ofdm; /*-> HW: s(9,2)*/
	s16 ref_pow_cck; /*-> HW: s(9,2)*/
	u16 ref_pow_ofdm_cw; /*BBCR 0x58E0[9:0]*/
	u16 ref_pow_cck_cw; /*BBCR 0x58E0[21:12]*/
	/*[Pwr Ofsset]*/ /*-> HW: s(7,1)*/
	s8 pwr_ofst_mode[TPU_SIZE_MODE_BE]; /*0~7: CCK, Legacy, HT, VHT, HE, EHT, DLRU_HE, DLRU_EHT*/
	s8 pwr_ofst_bw[TPU_SIZE_BW_BE]; /*0~9: 20, 40, 80, 160, 320, dlru_20, dlru_40, dlru_80, dlru_160, dlru_320*/
	s8 ofst_int; /*SW: S(8,3) -16 ~ +15.875 (dB)*/
	u8 ofst_fraction; /*[0:3] * 0.125(dBm)*/
	u8 base_cw_0db; /*[63~39~15]: [+24~0~-24 dBm]*/
	u16 tssi_16dBm_cw;
	/*[Pwr Cusoft]*/
	s16 pwr_cusofst_bylim; /*-> HW: s(9,2)*/
	s16 pwr_cusofst_bylimbf; /*-> HW: s(9,2)*/
	s16 pwr_cusofst_byrate; /*-> HW: s(9,2)*/
	s16 pwr_cusofst_byrulim; /*-> HW: s(9,2)*/
	u8 pwr_cusofst_sw; /*-> HW: u(4,0)*/
	/*[Pwr Limit Enable]*/
	bool pwr_lmt_en;
	
	/*[Misc]*/
	enum rtw_tpu_op_mode op_mode; /*In debug mode, only debug tool control TPU APIs*/
	bool normal_mode_lock_en;
	u16 pwr_lmt_ru_mem_size;
	bool ext_pwr_lmt_en;
	struct rtw_ext_pwr_lmt_info ext_pwr_lmt_i;
	u8 tx_ptrn_shap_idx;
	u8 tx_ptrn_shap_idx_cck;
	u16 pwr_constraint_mb;
};
#endif
/*==========================================================================*/

union bb_tpu_all_info {
	struct rtw_tpu_info bb_tpu_i;
	struct bb_tpu_be_info bb_tpu_be_i;
};

#endif
