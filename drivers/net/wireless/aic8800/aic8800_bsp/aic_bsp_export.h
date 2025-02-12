#ifndef __AIC_BSP_EXPORT_H
#define __AIC_BSP_EXPORT_H

#define AICBSP_RESV_MEM_SUPPORT

enum aicbsp_subsys {
	AIC_BLUETOOTH,
	AIC_WIFI,
};

enum aicbsp_pwr_state {
	AIC_PWR_OFF,
	AIC_PWR_ON,
};

enum skb_buff_id {
	AIC_RESV_MEM_TXDATA,
};

enum chip_rev_id {
	CHIP_REV_ID_U01 = 0x1,
	CHIP_REV_ID_U02 = 0x3,
	CHIP_REV_ID_U03 = 0x7,
	CHIP_REV_ID_U04 = 0x7,
};

enum chip_id {
	AIC8800D_CHIP_SUBREV_ID_U04 = 0x20,
};

#define CHIP_ID_H_MASK  0xC0
#define IS_CHIP_ID_H(chip_id)		((chip_id & CHIP_ID_H_MASK) == CHIP_ID_H_MASK)

enum AIC_PRODUCT_ID {
	PRODUCT_ID_AIC8800D = 0,
	PRODUCT_ID_AIC8801,
	PRODUCT_ID_AIC8800DC,
	PRODUCT_ID_AIC8800DW,
	PRODUCT_ID_AIC8800D80,
	PRODUCT_ID_AIC8800D81,
	PRODUCT_ID_AIC8800D80X2,
	PRODUCT_ID_AIC8800D81X2,
};

enum aicbsp_cpmode_type {
	AICBSP_CPMODE_WORK				= 0,
	AICBSP_CPMODE_TEST				= 1,
	AICBSP_CPMODE_BLE_SCAN_WAKEUP	= 2,
	AICBSP_CPMODE_M2D_OTA			= 3,
	AICBSP_CPMODE_DPDCALIB			= 4,
	AICBSP_CPMODE_MAX,
};

struct device_match_entry {
	u16 vid;
	u16 pid;
	u16 chipid;
	char *name;
	u16 rev;
	u16 subrev;
	u16 mcuid;
	u32 feature;
	u16 is_chip_id_h;
};

struct skb_buff_pool {
	uint32_t id;
	uint32_t size;
	const char *name;
	uint8_t used;
	struct sk_buff *skb;
};

struct aicbsp_feature_t {
	int      hwinfo;
	uint32_t sdio_clock;
	uint8_t  sdio_phase;
	bool     fwlog_en;
	struct device_match_entry *chipinfo;
	uint8_t  cpmode;
};

int aicbsp_set_subsys(int, int);
int aicbsp_get_feature(struct aicbsp_feature_t *feature);
struct sk_buff *aicbsp_resv_mem_alloc_skb(unsigned int length, uint32_t id);
void aicbsp_resv_mem_kfree_skb(struct sk_buff *skb, uint32_t id);

#ifdef CONFIG_DPD
#define ROM_FMAC_CALIB_ADDR                0x00130000

typedef struct {
	uint32_t bit_mask[3];
	uint32_t reserved;
	uint32_t dpd_high[96];
	uint32_t dpd_11b[96];
	uint32_t dpd_low[96];
	uint32_t idac_11b[48];
	uint32_t idac_high[48];
	uint32_t idac_low[48];
	uint32_t loft_res[18];
	uint32_t rx_iqim_res[16];
} rf_misc_ram_t;

typedef struct {
	uint32_t bit_mask[4];
	uint32_t dpd_high[96];
	uint32_t loft_res[18];
} rf_misc_ram_lite_t;

#define MEMBER_SIZE(type, member)   sizeof(((type *)0)->member)
#define DPD_RESULT_SIZE_8800DC      sizeof(rf_misc_ram_lite_t)
extern rf_misc_ram_lite_t dpd_res;

#ifndef CONFIG_FORCE_DPD_CALIB
int is_file_exist(char* name);
#define FW_DPDRESULT_NAME_8800DC           "aic_dpdresult_lite_8800dc.bin"
#endif
#endif

#endif
