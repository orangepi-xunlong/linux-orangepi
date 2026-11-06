/******************************************************************************
 *
 * Copyright(c) 2020 Realtek Corporation.
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
#ifndef _PHL_REGULATION_DEF_H_
#define _PHL_REGULATION_DEF_H_

#define PHL_GET_CHPLAN_UPDATE_INFO_2G(_did_, _info_, \
	_dm_tbl_, _chdef_tbl_) \
do { \
	u8 size = sizeof((_chdef_tbl_)) / sizeof(struct chdef_2ghz); \
	regu_get_chplan_update_result_2g((_did_), size, (_info_),\
		&(_dm_tbl_)[0], &(_chdef_tbl_)[0]); \
} while (0)

#define PHL_GET_CHPLAN_UPDATE_INFO_5G(_group_, _did_, _info_, \
	_dm_tbl_, _chdef_tbl_) \
do { \
	u8 size = sizeof((_chdef_tbl_)) / sizeof(struct chdef_5ghz); \
	regu_get_chplan_update_result_5g((_group_), (_did_), size, (_info_),\
		&(_dm_tbl_)[0], &(_chdef_tbl_)[0]); \
} while (0)

#define PHL_GET_DOMAIN_INDEX(_idx_, _domain_, _dm_tbl_) \
do { \
	u8 i, size; \
	(_idx_) = INVALID_DOMAIN_INDEX; \
	size = sizeof((_dm_tbl_)) / sizeof(struct regulatory_domain_mapping); \
	for (i = 0; i < size; i++) { \
		if ((_domain_) == (_dm_tbl_)[i].domain_code) { \
			(_idx_) = i; \
			break; \
		} \
	} \
} while (0)

#define PHL_GET_DOMAIN_REGU(_regu_, _domain_, _dm_tbl_, _band_) \
do { \
	u8 did = INVALID_DOMAIN_INDEX; \
	(_regu_) = REGULATION_MAX; \
	PHL_GET_DOMAIN_INDEX(did, (_domain_), (_dm_tbl_)); \
	if (did == INVALID_DOMAIN_INDEX) \
		break; \
	switch ((_band_)) { \
	case BAND_ON_24G: \
		(_regu_) = (_dm_tbl_)[did].freq_2g.regulation; \
		break; \
	case BAND_ON_5G: \
		(_regu_) = (_dm_tbl_)[did].freq_5g.regulation; \
		break; \
	case BAND_ON_6G: \
	case BAND_MAX: \
		break; \
	} \
} while (0)

#define PHL_GET_CNTRY_INDEX(_cntry_, _idx_, _cntry_tbl_) \
do { \
	u8 i; \
	u8 size = sizeof((_cntry_tbl_)) / sizeof(struct country_domain_mapping); \
	(_idx_) = INVALID_CNTRY_IDX; \
	for (i = 0; i < size; i++) { \
		if ((_cntry_tbl_)[i].char2[0] == (_cntry_)[0] && \
			(_cntry_tbl_)[i].char2[1] == (_cntry_)[1]) { \
			(_idx_) = i; \
			break; \
		} \
	} \
} while (0)

#define PHL_GET_CNTRY_TBL_SIZE(_size_, _cntry_tbl_) \
	(_size_) = sizeof((_cntry_tbl_)) / sizeof(struct country_domain_mapping);

#define PHL_QRY_GRP_CNTRY_NUM(_num_, _policy_, _id_, _dm_tbl_, _cntry_tbl_) \
do { \
	u8 dm_size = sizeof((_dm_tbl_)) / sizeof(struct regulatory_domain_mapping); \
	u8 cntry_size = sizeof((_cntry_tbl_)) / sizeof(struct country_domain_mapping); \
	(_num_) = 0; \
	(_num_) = regu_calculate_group_cntry_num( \
		(_policy_), (_id_), dm_size, cntry_size, &(_dm_tbl_)[0], &(_cntry_tbl_)[0]); \
} while (0)


#define PHL_FILL_GRP_CNTRY_LIST(_policy_, _list_, _len_, _id_, _dm_tbl_, _cntry_tbl_) \
do { \
	u8 dm_size = sizeof((_dm_tbl_)) / sizeof(struct regulatory_domain_mapping); \
	u8 cntry_size = sizeof((_cntry_tbl_)) / sizeof(struct country_domain_mapping); \
	if (!(_policy_) || !(_list_)) \
		break; \
	regu_fill_specific_group_cntry_list( \
		(_policy_), (_list_), (_len_), (_id_), dm_size, \
		cntry_size, &(_dm_tbl_)[0], &(_cntry_tbl_)[0]); \
} while (0) \

#define PHL_QRY_CNTRY_CHNLPLAN(_cntry_, _chnlplan_, _cntry_tbl_) \
do { \
	u8 size = sizeof((_cntry_tbl_)) / sizeof(struct country_domain_mapping); \
	regu_fill_chnlplan_content((_cntry_), size, (_chnlplan_), &(_cntry_tbl_)[0]); \
} while (0)

/* 6g */
#define PHL_GET_CHDEF_6G(_idx_, _chdef_, _chdef_tbl_) \
do { \
	u8 size = sizeof(_chdef_tbl_) / sizeof(struct chdef_6ghz); \
	regu_get_chdef_6g((_idx_), size, (_chdef_), &(_chdef_tbl_)[0]); \
} while (0)

#define PHL_GET_DOMAIN_INDEX_6G(_idx_, _domain_, _dm_tbl_) \
do { \
	u8 i, size; \
	(_idx_) = INVALID_DOMAIN_INDEX; \
	size = sizeof((_dm_tbl_)) / sizeof(struct regulatory_domain_mapping_6g); \
	for (i = 0; i < size; i++) { \
		if ((_domain_) == (_dm_tbl_)[i].domain_code) { \
			(_idx_) = i; \
			break; \
		} \
	} \
} while (0)

#define PHL_GET_6G_REGULATORY_INFO(_code_, _regu_, \
	_ch_idx_, _domain_, _dm_tbl_) \
do { \
	u8 idx = INVALID_DOMAIN_INDEX; \
	PHL_GET_DOMAIN_INDEX_6G(idx, (_domain_), (_dm_tbl_)); \
	if (idx != INVALID_DOMAIN_INDEX) { \
		(_code_) = (_dm_tbl_)[idx].domain_code; \
		(_regu_) = (_dm_tbl_)[idx].regulation; \
		(_ch_idx_) = (_dm_tbl_)[idx].ch_idx; \
	} \
} while (0)

#define PHL_GET_CAT6G_BY_COUNTRY(_cat_6g_, _cntry_, _cntry_tbl_) \
do { \
	u8 idx = INVALID_CNTRY_IDX; \
	PHL_GET_CNTRY_INDEX((_cntry_), idx, (_cntry_tbl_)); \
	if (INVALID_CNTRY_IDX == idx) {\
		(_cat_6g_) = 0; \
		break; \
	} \
	(_cat_6g_) = (_cntry_tbl_)[idx].category_6g; \
} while (0)

#define INVALID_DOMAIN_CODE 0xff
#define INVALID_CH_IDX 0xff
#define INVALID_REGU_NUM 0xff
#define INVALID_CNTRY_IDX 0xff
#define INVALID_VER 0x0
#define INVALID_GROUP 0xff

#define RSVD_DOMAIN 0x1a

#define MAX_CH_NUM_2GHZ 14

#define MAX_CH_NUM_BAND1 4 /* 36, 40, 44, 48 */
#define MAX_CH_NUM_BAND2 4 /* 52, 56, 60, 64 */
#define MAX_CH_NUM_BAND3 12 /* 100, 104, 108, 112,
				116, 120, 124, 128,
				132, 136, 140, 144 */
#define MAX_CH_NUM_BAND4 8 /* 149, 153, 157, 161, 165, 169, 173, 177 */
#define MAX_CH_NUM_5GHZ (MAX_CH_NUM_BAND1 + MAX_CH_NUM_BAND2 +\
				MAX_CH_NUM_BAND3 + MAX_CH_NUM_BAND4)

#define MAX_CH_NUM_UNII5 24 /* 1 ~ 93 */
#define MAX_CH_NUM_UNII6 6 /* 97 ~ 117 */
#define MAX_CH_NUM_UNII7 18 /* 121 ~ 189 */
#define MAX_CH_NUM_UNII8 11 /* 193 ~ 233 */
#define MAX_CH_NUM_6GHZ (MAX_CH_NUM_UNII5 + MAX_CH_NUM_UNII6 +\
				MAX_CH_NUM_UNII7 + MAX_CH_NUM_UNII8)


#define BAND_2GHZ(_band_) ((_band_ == BAND_ON_24G) ? true : false)
#define BAND_5GHZ(_band_) ((_band_ == BAND_ON_5G) ? true : false)
#define BAND_6GHZ(_band_) ((_band_ == BAND_ON_6G) ? true : false)

#define CH_2GHZ(_ch_) (((_ch_ >= 1) && (_ch_ <= 14)) ? true : false)
#define CH_5GHZ_BAND1(_ch_) (((_ch_ >= 36) && (_ch_ <= 48)) ? true : false)
#define CH_5GHZ_BAND2(_ch_) (((_ch_ >= 52) && (_ch_ <= 64)) ? true : false)
#define CH_5GHZ_BAND3(_ch_) (((_ch_ >= 100) && (_ch_ <= 144)) ? true : false)
#define CH_5GHZ_BAND4(_ch_) (((_ch_ >= 149) && (_ch_ <= 177)) ? true : false)
#define CH_59GHZ(_ch_) (((_ch_ > 165) && (_ch_ <= 177)) ? true : false)

/* UNII-5 support channel list, ch1 ~ ch93, total : 24 */
/* UNII-6 support channel list, ch97 ~ ch117, total : 6 */
/* UNII-7 support channel list, ch121 ~ ch189, total : 18 */
/* UNII-8 support channel list, ch193 ~ ch233, total : 11 */
#define CH_6GHZ_UNII5(_ch_) (((_ch_ >= 1) && (_ch_ <= 93)) ? true : false)
#define CH_6GHZ_UNII6(_ch_) (((_ch_ >= 97) && (_ch_ <= 117)) ? true : false)
#define CH_6GHZ_UNII7(_ch_) (((_ch_ >= 121) && (_ch_ <= 189)) ? true : false)
#define CH_6GHZ_UNII8(_ch_) (((_ch_ >= 193) && (_ch_ <= 233)) ? true : false)

#define CH_IN_SAME_5G_BAND(ch_a, ch_b) \
	((CH_5GHZ_BAND4(ch_a) && CH_5GHZ_BAND4(ch_b)) \
	|| (CH_5GHZ_BAND3(ch_a) && CH_5GHZ_BAND3(ch_b)) \
	|| (CH_5GHZ_BAND2(ch_a) && CH_5GHZ_BAND2(ch_b)) \
	|| (CH_5GHZ_BAND1(ch_a) && CH_5GHZ_BAND1(ch_b)))

#define CH_IN_SAME_6G_BAND(ch_a, ch_b) \
	((CH_6GHZ_UNII8(ch_a) && CH_6GHZ_UNII8(ch_b)) \
	|| (CH_6GHZ_UNII7(ch_a) && CH_6GHZ_UNII7(ch_b)) \
	|| (CH_6GHZ_UNII6(ch_a) && CH_6GHZ_UNII6(ch_b)) \
	|| (CH_6GHZ_UNII5(ch_a) && CH_6GHZ_UNII5(ch_b)))

#define SUPPORT_11A BIT(0)
#define SUPPORT_11B BIT(1)
#define SUPPORT_11G BIT(2)
#define SUPPORT_11N BIT(3)
#define SUPPORT_11AC BIT(4)
#define SUPPORT_11AX BIT(5)

#define INVALID_DOMAIN_INDEX 0xff
#define MAX_CNTRY_NUM 0xff

enum regulation_rsn {
	REGU_RSN_DEFAULT = 0x0,
	REGU_RSN_SMBIOS,
	REGU_RSN_EFUSE,
	REGU_RSN_11D,
	REGU_RSN_REGISTRY,
	REGU_RSN_LOCATION,
	REGU_RSN_MANUAL,
	REGU_RSN_CUSTOM,
	REGU_RSN_MAX
};

enum rtw_regulation_capability {
	CAPABILITY_2GHZ = BIT(0),
	CAPABILITY_5GHZ = BIT(1),
	CAPABILITY_DFS = BIT(2),
	CAPABILITY_6GHZ = BIT(3),
	CAPABILITY_59GHZ = BIT(4),
};

enum rtw_regulation_query {
	REGULQ_CHPLAN_2GHZ = BIT(0),
	REGULQ_CHPLAN_5GHZ_BAND1 = BIT(1),
	REGULQ_CHPLAN_5GHZ_BAND2 = BIT(2),
	REGULQ_CHPLAN_5GHZ_BAND3 = BIT(3),
	REGULQ_CHPLAN_5GHZ_BAND4 = BIT(4),
	REGULQ_CHPLAN_6GHZ_UNII5 = BIT(5),
	REGULQ_CHPLAN_6GHZ_UNII6 = BIT(6),
	REGULQ_CHPLAN_6GHZ_UNII7 = BIT(7),
	REGULQ_CHPLAN_6GHZ_UNII8 = BIT(8),
	REGULQ_CHPLAN_6GHZ_PSC = BIT(9),
	REGULQ_CHPLAN_6GHZ_ALL = (REGULQ_CHPLAN_6GHZ_UNII5 |
		REGULQ_CHPLAN_6GHZ_UNII6 | REGULQ_CHPLAN_6GHZ_UNII7 |
		REGULQ_CHPLAN_6GHZ_UNII8),
	REGULQ_CHPLAN_5GHZ_ALL = (REGULQ_CHPLAN_5GHZ_BAND1 |
		REGULQ_CHPLAN_5GHZ_BAND2 | REGULQ_CHPLAN_5GHZ_BAND3 |
		REGULQ_CHPLAN_5GHZ_BAND4),
	REGULQ_CHPLAN_2GHZ_5GHZ = (REGULQ_CHPLAN_2GHZ |
		REGULQ_CHPLAN_5GHZ_BAND1 | REGULQ_CHPLAN_5GHZ_BAND2 |
		REGULQ_CHPLAN_5GHZ_BAND3 | REGULQ_CHPLAN_5GHZ_BAND4),
	REGULQ_CHPLAN_FULL = (REGULQ_CHPLAN_2GHZ | REGULQ_CHPLAN_5GHZ_ALL |
					REGULQ_CHPLAN_6GHZ_ALL),
	REGULQ_CHPLAN_DEF = (REGULQ_CHPLAN_2GHZ | REGULQ_CHPLAN_5GHZ_ALL |
					REGULQ_CHPLAN_6GHZ_PSC),
};

enum country_property {
	CNTRY_EU = BIT(0), /* European Union */
};

enum ch_property {
	CH_PASSIVE = BIT(0), /* regulatory passive channel */
	CH_DFS = BIT(1), /* 5 ghz DFS channel */
	CH_PSC = BIT(2) /* 6 ghz preferred scanning channel */
};

enum rtw_regulation_group_id {
	DEFAULT_SUPPORT_6G = 0x0,
	CURRENT_SUPPORT_6G,
	EU_GROUP,
	FCC_GROUP,
	OTHERS_MODULE_RF_APPROVAL
};

struct rtw_regulation_channel {
	enum band_type band;
	u8 channel;
	u8 property;
};

struct rtw_regulation_chplan {
	u32 cnt;
	struct rtw_regulation_channel ch[MAX_CH_NUM_2GHZ +
					MAX_CH_NUM_5GHZ +
					MAX_CH_NUM_6GHZ];
};

struct rtw_ch {
	enum band_type band;
	u8 ch;
};

struct rtw_chlist {
	u32 cnt;
	struct rtw_ch ch[MAX_CH_NUM_2GHZ +
			MAX_CH_NUM_5GHZ +
			MAX_CH_NUM_6GHZ];
};

struct rtw_regulatory_domain {
	u8 domain; /* original 2/5 ghz domain */
	u8 domain_6g; /* 6 ghz domain */
};

enum cntry_property_6g_bp {
	CP_6G_BAND_BLOCKED = BIT(0), /* country is blocked, cannot use 6 ghz band */
	CP_6G_BAND_SOFTAP_BLOCKED = BIT(1), /* country cannot use softap on 6 ghz band */
};

enum cntry_property_5g_bp {
	CP_5G_BAND1_SOFTAP_BLOCKED = BIT(0), /* country cannot use softap on 5 ghz band 1 */
	CP_5G_BAND2_SOFTAP_BLOCKED = BIT(1), /* country cannot use softap on 5 ghz band 2 */
	CP_5G_BAND3_SOFTAP_BLOCKED = BIT(2), /* country cannot use softap on 5 ghz band 3 */
	CP_5G_BAND4_SOFTAP_BLOCKED = BIT(3), /* country cannot use softap on 5 ghz band 4 */
};

enum cp_bp_mode { /* country property - band policy mode */
	BLOCK_MODE = 0, /* selected countries are blocked, otherwise follow RTK */
	ALLOW_MODE = 1, /* selected countries are allowed (to follow RTK), otherwise blocked */
};

enum rtw_power_limit_6g_info {
	PWR_LMT_6G_LPI = 0,
	PWR_LMT_6G_STD,
	PWR_LMT_6G_VLP,
	PWR_LMT_6G_MAX,
};

struct rtw_regu_policy_info {
	u8 valid_6g_bp;
	u8 cp_6g_bp; /* country property - 6g band policy */
	u8 valid_5g_bp;
	u8 cp_5g_bp; /* country property - 5g band policy */
};

struct rtw_regulation_info {
	u8 domain_code;
	u8 domain_reason;
	u8 domain_code_6g;
	u8 domain_reason_6g;
	char country[2];
	u8 support_mode;
	u8 regulation_2g;
	u8 regulation_5g;
	u8 regulation_6g;
	enum rtw_power_limit_6g_info category_6g;
	u8 tpo;
	u8 chplan_ver;
	u8 country_ver;
	u16 capability;

	/* Regulation table source:
	 * 0 for common tbl
	 * 1 for IC module tbl
	 * >= 2 for customized tbl
	 */
	u8 tbl_idx;
};

struct rtw_regulation_interface {
	_os_lock lock;
	/* the content of the struct is for halrf */
	struct rtw_regulation_info regu_info;
};

struct rtw_regulation_country_chplan {
	u8 domain_code;
	u8 domain_code_6g;
	u8 support_mode;
	/*
	* bit0: accept 11a
	* bit1: accept 11b
	* bit2: accept 11g
	* bit3: accept 11n
	* bit4: accept 11ac
	* bit5: accept 11ax
	*/
	u8 tpo; /* tx power overwrite */
	bool valid;
};

struct chdef_2ghz {
    /* ch_def index */
    u8 idx;

    /* support channel list
     * support_ch[0]: bit(0~7) stands for ch(1~8)
     * support_ch[1]: bit(0~5) stands for ch (9~14)
     */
    u8 support_ch[2];

    /* passive ch list
     * passive[0]: bit(0~7) stands for ch(1~8)
     * passive[1]: bit(0~5) stands for ch (9~14)
     */
    u8 passive[2];
};

struct chdef_5ghz {
    /* ch_def index */
    u8 idx;

    /*
     * band1 support channel list, passive and dfs
     * bit0 stands for ch36
     * bit1 stands for ch40
     * bit2 stands for ch44
     * bit3 stands for ch48
     */
    u8 support_ch_b1;
    u8 passive_b1;
    u8 dfs_b1;

    /*
     * band2 support channel list, passive and dfs
     * bit0 stands for ch52
     * bit1 stands for ch56
     * bit2 stands for ch60
     * bit3 stands for ch64
     */
    u8 support_ch_b2;
    u8 passive_b2;
    u8 dfs_b2;

    /*
     * band3 support channel list, passive and dfs
     * byte[0]:
     *    bit0 stands for ch100
     *     bit1 stands for ch104
     *     bit2 stands for ch108
     *     bit3 stands for ch112
     *     bit4 stands for ch116
     *     bit5 stands for ch120
     *     bit6 stands for ch124
     *     bit7 stands for ch128
     * byte[1]:
     *    bit0 stands for ch132
     *     bit1 stands for ch136
     *     bit2 stands for ch140
     *     bit3 stands for ch144
     */
    u8 support_ch_b3[2];
    u8 passive_b3[2];
    u8 dfs_b3[2];

    /*
     * band4 support channel list, passive and dfs
     * bit0 stands for ch149
     * bit1 stands for ch153
     * bit2 stands for ch157
     * bit3 stands for ch161
     * bit4 stands for ch165
     * bit5 stands for ch169
     * bit6 stands for ch173
     * bit7 stands for ch177
     */
    u8 support_ch_b4;
    u8 passive_b4;
    u8 dfs_b4;
};

struct freq_plan {
    u8 regulation;
    u8 ch_idx;
};

struct regulatory_domain_mapping {
    u8 domain_code;
    struct freq_plan freq_2g;
    struct freq_plan freq_5g;
};

#define COUNTRY_CODE_LEN 2
struct country_domain_mapping {
    u8 domain_code;
    u8 domain_code_6g;
    char char2[COUNTRY_CODE_LEN];
    u8 tpo; /* tx power overwrite */

    /*
     * bit0: accept 11bgn
     * bit1: accept 11a
     * bit2: accept 11ac
     * bit3: accept 11ax
     */
    u8 support;
    u8 country_property;

    /*
     * bit0: support LPI
     * bit1: support SP
     * bit2: support VLP
     */
    u8 category_6g;
    u8 others_module_rf_approval;
};

/*
 * 6 GHz channel group from UNII-5 to UNII-8
 * channel index diff is 4 : minimum working bandwidth : 20 MHz
 * => next channel index = current index + 4
 */

struct chdef_6ghz {
    /* ch_def index */
    u8 idx;

    /*
     * UNII-5 support channel list, ch1 ~ ch93, total : 24
     * bit0 stands for ch1
     * bit1 stands for ch5
     * bit2 stands for ch9
     * ...
     * bit23 stands for ch93
     */
    u8 support_ch_u5[3];
    u8 passive_u5[3];

    /*
     * UNII-6 support channel list, ch97 ~ ch117, total : 6
     * bit0 stands for ch97
     * bit1 stands for ch101
     * bit2 stands for ch105
     * bit3 stands for ch109
     * bit4 stands for ch113
     * bit5 stands for ch117
     */
    u8 support_ch_u6;
    u8 passive_u6;

    /*
     * UNII-7 support channel list, ch121 ~ ch189, total : 18
     * bit0 stands for ch121
     * bit1 stands for ch125
     * bit2 stands for ch129
     * ...
     * bit17 stands for ch189
     */
    u8 support_ch_u7[3];
    u8 passive_u7[3];

    /*
     * UNII-8 support channel list, ch193 ~ ch237, total : 12
     * bit0 stands for ch193
     * bit1 stands for ch197
     * bit2 stands for ch201
     * ...
     * bit10 stands for ch233
     */
    u8 support_ch_u8[2];
    u8 passive_u8[2];
};

struct regulatory_domain_mapping_6g {
    u8 domain_code;
    u8 regulation;
    u8 ch_idx;
};

struct rtw_regu_policy {
	u8 valid_6g_bp; /* 1: valid, 0: invalid */
	u8 cp_6g_bp[MAX_CNTRY_NUM]; /* the country property for 6g band policy */
	u8 valid_5g_bp; /* 1: valid, 0: invalid */
	u8 cp_5g_bp[MAX_CNTRY_NUM]; /* the country property for 5g band policy */
};

enum TP_OVERWRITE {
    TPO_CHILE = 0,
    TPO_UK = 1,
    TPO_QATAR = 2,
    TPO_UKRAINE = 3,
    TPO_CN = 4,
    TPO_NA = 5
};

enum REGULATION {
    REGULATION_WW        =  0,
    REGULATION_ETSI      =  1,
    REGULATION_FCC       =  2,
    REGULATION_MKK       =  3,
    REGULATION_KCC       =  4,
    REGULATION_NCC       =  5,
    REGULATION_ACMA      =  6,
    REGULATION_NA        =  7,
    REGULATION_IC        =  8,
    REGULATION_CHILE     =  9,
    REGULATION_MEX       = 10,
    REGULATION_MAX       = 11,
};

enum rtw_regulation_table_index {
	CMN_TBL = 0x0,
	IC_TBL,
};

struct rtw_chplan_update_info_2g {
	u8 ch_idx;
	u8 regulation;
	u16 ch;
	u16 passive;
	bool valid;
};

struct rtw_chplan_update_info_5g {
	u8 ch_idx;
	u8 regulation;
	u16 ch;
	u16 passive;
	u16 dfs;
	u8 max_num;
	u8 ch_start;
	bool valid;
};

/*
 * NOTE:
 * 	This api prototype will be removed after hal related API/header is added
 * 	for halrf.
 */
bool rtw_phl_query_regulation_info(void *phl, struct rtw_regulation_info *info);

u8 regu_calculate_group_cntry_num(
	struct rtw_regu_policy *policy, u8 group_id,
	u8 rdmap_size, u8 cdmap_size,
	const struct regulatory_domain_mapping *rdmap_tbl,
	const struct country_domain_mapping *cdmap_tbl);

void regu_fill_specific_group_cntry_list(
	struct rtw_regu_policy *policy, char* list, u32 group_size,
	u8 group_id, u8 rdmap_size, u8 cdmap_size,
	const struct regulatory_domain_mapping *rdmap_tbl,
	const struct country_domain_mapping *cdmap_tbl);

void
regu_fill_chnlplan_content(char *country, u8 size,
	struct rtw_regulation_country_chplan *chnlplan,
	const struct country_domain_mapping *cdmap_tbl);

void
regu_get_chplan_update_result_2g(u8 did, u8 size,
	struct rtw_chplan_update_info_2g *info,
	const struct regulatory_domain_mapping *rdmap_tbl,
	const struct chdef_2ghz *chdef_tbl);

void
regu_get_chplan_update_result_5g(
	u8 group, u8 did, u8 size,

	struct rtw_chplan_update_info_5g *info,
	const struct regulatory_domain_mapping *rdmap_tbl,
	const struct chdef_5ghz *chdef_tbl);

void
regu_get_chdef_6g(
	u8 did, u8 size, struct chdef_6ghz *chdef,
	const struct chdef_6ghz *chdef_tbl);

#endif /* _PHL_REGULATION_DEF_H_ */
