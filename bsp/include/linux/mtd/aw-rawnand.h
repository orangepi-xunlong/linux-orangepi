/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/**
 * SPDX-License-Identifier: GPL-2.0+
 *
 * (C) Copyright 2020 - 2021
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * cuizhikui <cuizhikui@allwinnertech.com>
 *
 */

#ifndef __AW_RAWNAND_H__
#define __AW_RAWNAND_H__

#include <sunxi-log.h>
#include <linux/mtd/mtd.h>
#include <linux/mutex.h>

#define RAWNAND_BLKBURNBOOT0 _IO('v', 125)
#define RAWNAND_BLKBURNBOOT1 _IO('v', 126)

/* aw rawnand warning messages */
#define awrawnand_warn(fmt, ...) sunxi_warn(NULL, "awrawnand(mtd):warning: %s: " fmt, \
					__func__, ##__VA_ARGS__)
/* aw rawnand error messages */
#define awrawnand_err(fmt, ...) sunxi_err(NULL, "awrawnand(mtd):error: %s: " fmt, \
				      __func__, ##__VA_ARGS__)

/* aw rawnand info messages */
#define awrawnand_info(fmt, ...) sunxi_info(NULL, "awrawnand(mtd):info:" fmt, \
					##__VA_ARGS__)

/* aw rawnand debug messages */
#define awrawnand_dbg(fmt, ...) sunxi_debug(NULL, "awrawnand(mtd): dbg:" fmt, \
					##__VA_ARGS__)


/* aw rawnand messages */
#define awrawnand_print(fmt, ...) sunxi_err(NULL, "awrawnand(mtd):" fmt, ##__VA_ARGS__)
//#define DBG
#ifdef DBG
#define AWRAWNAND_TRACE(fmt, ...) sunxi_err(NULL, "awrawnand(mtd):" fmt, ##__VA_ARGS__)
#else
#define AWRAWNAND_TRACE(fmt, ...)
#endif

//#define DBG_NFC
#ifdef DBG_NFC
#define AWRAWNAND_TRACE_NFC(fmt, ...) sunxi_err(NULL, "awrawnand(mtd):" fmt, ##__VA_ARGS__)
#else
#define AWRAWNAND_TRACE_NFC(fmt, ...)
#endif

//#define DBG_CHIP
#ifdef DBG_CHIP
#define awrawnand_chip_trace(fmt, ...) sunxi_err(NULL, fmt,  ##__VA_ARGS__)
#else
#define awrawnand_chip_trace(fmt, ...)
#endif

//#define TRACE_UBI_MTD
#ifdef TRACE_UBI_MTD
#define awrawnand_ubi_trace(fmt, ...) sunxi_err(NULL, "aw-mtd:" fmt, ##__VA_ARGS__)
#else
#define awrawnand_ubi_trace(fmt, ...)
#endif

//#define TRACE_MTD
#ifdef TRACE_MTD
#define awrawnand_mtd_trace(fmt, ...) sunxi_err(NULL, fmt, ##__VA_ARGS__)
#else
#define awrawnand_mtd_trace(fmt, ...)
#endif

/*
 *  rawnand  multiplane.
 *
 * Merge pages in two adjacent blocks with the same page num to super page.
 * Merge adjacent blocs to super block.
 *
 *   phy-block0   phy-block1    = super block 0
 * |------------|------------|
 * | phy-page 0 | phy-page 0 |  = super page 0 on super block 0
 * | phy-page 1 | phy-page 1 |  = super page 1 on super block 0
 * |     ...    |     ...    |
 * |------------|------------|
 *
 */
#define SIMULATE_MULTIPLANE (1)

/* ecc status */
#define ECC_GOOD	(0 << 4)
#define ECC_LIMIT	(1 << 4)
#define ECC_ERR		(2 << 4)
#define ECC_BLANK_PAGE	(3 << 4)

#define MAX_CYCLE (5)
#define RAWNAND_MAX_ID_LEN (8U)


#define UBOOT_START_BLOCK_SMALLNAND 8
#define UBOOT_START_BLOCK_BIGNAND 4
#define AW_RAWNAND_RESERVED_PHY_BLK_FOR_SECURE_STORAGE 8
#define PHY_BLKS_FOR_SECURE_STORAGE AW_RAWNAND_RESERVED_PHY_BLK_FOR_SECURE_STORAGE
#define PSTORE_SIZE_KB	512

/*
 * Standard NAND flash commands
 */
#define RAWNAND_CMD_READ0		0
#define RAWNAND_CMD_READ1		1
#define RAWNAND_CMD_RNDOUT		5
#define RAWNAND_CMD_PAGEPROG		0x10
#define RAWNAND_CMD_READOOB		0x50
#define RAWNAND_CMD_ERASE1		0x60
#define RAWNAND_CMD_STATUS		0x70
#define RAWNAND_CMD_SEQIN		0x80
#define RAWNAND_CMD_JEDEC_SEQIN		0x81
#define RAWNAND_CMD_RNDIN		0x85
#define RAWNAND_CMD_READID		0x90
#define RAWNAND_CMD_ERASE2		0xd0
#define RAWNAND_CMD_PARAM		0xec
#define RAWNAND_CMD_GET_FEATURES	0xee
#define RAWNAND_CMD_SET_FEATURES	0xef
#define RAWNAND_CMD_RESET		0xff

/* Extended commands for large page devices */
#define RAWNAND_CMD_READSTART		0x30
#define RAWNAND_CMD_RNDOUTSTART		0xE0
#define RAWNAND_CMD_CACHEDPROG		0x15
#define RAWNAND_CMD_MULTIPROG		0x11
#define RAWNAND_CMD_MULTIPREADSTART	0x32
#define RAWNAND_CMD_MULTIERASE		0xd1

#define TOGGLE_INTERFACE_CHANGE_ADDR	(0x80)

/* Status bits */
#define RAWNAND_STATUS_FAIL	0x01
#define RAWNAND_STATUS_FAIL_N1	0x02
#define RAWNAND_STATUS_TRUE_READY	0x20
#define RAWNAND_STATUS_READY	0x40
#define RAWNAND_STATUS_WP		0x80

#define DEV_IO_READY	(0x1)
#define DEV_ARRAY_READY	(0x2)
#define DEV_READY	(0x3)

#define FEATURES_PARA_LEN (4)

/*idtab options bitmap*/

/* Device interface*/
#define RAWNAND_ITF_SDR		BIT(0)
#define RAWNAND_ITF_ONFI_DDR	BIT(1)
#define RAWNAND_ITF_ONFI_DDR2	BIT(2)
#define RAWNAND_ITF_TOGGLE_DDR	BIT(3)
#define RAWNAND_ITF_TOGGLE_DDR2	BIT(4)

/* TOGGLE only support*/
#define RAWNAND_TOGGLE_SUPPORT_ONLY	BIT(5)
/* ONFI timing mode, used in both asynchronous and synchronous mode */
#define RAWNAND_ONFI_TIMING_MODE	BIT(6)
/* ONFI features */
#define RAWNAND_ONFI_FEATURE_EXT_PARAM_PAGE	BIT(7)


/* Chip has cache program function */
#define RAWNAND_CACHEPRG	BIT(8)
/* Chip has copy back function */
#define RAWNAND_COPYBACK	BIT(9)

/* Chip allow multi writes */
/* 80h -- 11h ~ 80h -- 10h*/
#define RAWNAND_MULTI_WRITE	BIT(10)


/* Chip allow multi reads */
#define RAWNAND_MULTI_READ	BIT(11)

/* Chip allow multi erase 60h-60h-d0h*/
#define RAWNAND_MULTI_ERASE	BIT(12)

/* Chip allow onfi multi erase 60h-d1h -- 60h- d0h*/
#define RAWNAND_MULTI_ONFI_ERASE	BIT(13)

/* Chip allow multi writes */
/* 80h -- 11h ~ 81h -- 10h*/
#define RAWNAND_JEDEC_MULTI_WRITE	BIT(14)

/* Device needs 2rd row address cycle */
#define RAWNAND_ROW_ADDR_2	BIT(16)

/* Default Toggle DDR1.0 , SDR need to set*/
#define RAWNAND_TOGGLE_DDR_TO_SDR	BIT(29)
/* Open nfc randomizer */
#define RAWNAND_NFC_RANDOM	BIT(30)



/* Macros to identify the above */
#define RAWNAND_HAS_ONLY_TWO_ADDR(chip) ((chip->options & RAWNAND_ROW_ADDR_2))
#define RAWNAND_HAS_ITF_SDR(chip) ((chip->options & RAWNAND_ITF_SDR))
#define RAWNAND_HAS_ITF_ONFI_DDR(chip) ((chip->options & RAWNAND_ITF_ONFI_DDR))
#define RAWNAND_HAS_ITF_ONFI_DDR2(chip) ((chip->options & RAWNAND_ITF_ONFI_DDR2))
#define RAWNAND_HAS_ITF_TOGGLE_DDR(chip) ((chip->options & RAWNAND_ITF_TOGGLE_DDR))
#define RAWNAND_HAS_ITF_TOGGLE_DDR2(chip) ((chip->options & RAWNAND_ITF_TOGGLE_DDR2))
#define RAWNAND_HAS_CACHEPROG(chip) ((chip->options & RAWNAND_CACHEPRG))
#define RAWNAND_HAS_MULTI_WRITE(chip) ((chip)->options & RAWNAND_MULTI_WRITE)
#define RAWNAND_HAS_JEDEC_MULTI_WRITE(chip) ((chip)->options & RAWNAND_JEDEC_MULTI_WRITE)
#define RAWNAND_HAS_MULTI_READ(chip) ((chip->options & RAWNAND_MULTI_READ))
#define RAWNAND_HAS_MULTI_ERASE(chip) ((chip->options & RAWNAND_MULTI_ERASE))
#define RAWNAND_HAS_MULTI_ONFI_ERASE(chip) ((chip->options & RAWNAND_MULTI_ONFI_ERASE))
#define RAWNAND_HAS_ONLY_TOGGLE(chip) ((chip->options & RAWNAND_TOGGLE_SUPPORT_ONLY))
#define RAWNAND_NEED_CHANGE_TO_SDR(chip) ((chip->options & RAWNAND_TOGGLE_DDR_TO_SDR))
#define RAWNAND_NFC_NEED_RANDOM(chip) ((chip->options & RAWNAND_NFC_RANDOM))



/*
 * RAW NAND Flash Manufacturer ID Codes
 */
#define RAWNAND_MFR_TOSHIBA	0x98
#define RAWNAND_MFR_SAMSUNG	0xec
#define RAWNAND_MFR_FUJITSU	0x04
#define RAWNAND_MFR_NATIONAL	0x8f
#define RAWNAND_MFR_RENESAS	0x07
#define RAWNAND_MFR_STMICRO	0x20
#define RAWNAND_MFR_HYNIX	0xad
#define RAWNAND_MFR_MICRON	0x2c
#define RAWNAND_MFR_AMD		0x01
#define RAWNAND_MFR_MACRONIX	0xc2
#define RAWNAND_MFR_EON		0x92
#define RAWNAND_MFR_SANDISK	0x45
#define RAWNAND_MFR_INTEL	0x89
#define RAWNAND_MFR_ATO		0x9b
#define RAWNAND_MFR_SPANSION	0x01
#define RAWNAND_MFR_ESMT	0xc8
#define RAWNAND_MFR_GIGA	0xc8
#define RAWNAND_MFR_MXIC	0xc2
#define RAWNAND_MFR_FORESEE	0xec
#define RAWNAND_MFR_WINBOND	0xef
#define RAWNAND_MFR_FIDELIX	0xad
#define RAWNAND_MFR_UNILC	0xc8
#define RAWNAND_MFR_JSC		0xad
#define RAWNAND_MFR_DOSILICON	0xf8
#define RAWNAND_MFR_FORESEE_1	0xcd
#define RAWNAND_MFR_DOSILICON_1	0xe5

/*
 * RAWNAND Flash Manufacture name
 * */
#define SPANSION_NAME "spansion"
#define ATO_NAME "ato"
#define EON_NAME "eon"
#define ESMT_NAME "esmt"
#define FUJITSU_NAME "fujitsu"
#define HYNIX_NAME "hynix"
#define INTEL_NAME "intel"
#define MACRONIX_NAME "macronix"
#define GIGA_NAME "giga"
#define MXIC_NAME "mxic"
#define MICRON_NAME "micron"
#define NATIONAL_NAME "national"
#define RENESAS_NAME "renesas"
#define SAMSUNG_NAME "samsung"
#define SANDISK_NAME "sandisk"
#define STMICRO_NAME "stmicro"
#define TOSHIBA_NAME "toshiba"
#define WINBOND_NAME "winbond"
#define FORESEE_NAME "foresee"
#define FIDELIX_NAME "fidelix"
#define UNILC_NAME "unilc"
#define JSC_NAME "jsc"
#define DOSILICON_NAME "dosilicon"


#define PE_CYCLES_1K	(1000)
#define PE_CYCLES_2K	(2000)
#define PE_CYCLES_3K	(3000)
#define PE_CYCLES_4K	(4000)
#define PE_CYCLES_5K	(5000)
#define PE_CYCLES_6K	(6000)
#define PE_CYCLES_7K	(7000)
#define PE_CYCLES_8K	(8000)
#define PE_CYCLES_9K	(9000)
#define PE_CYCLES_10K	(10000)
#define PE_CYCLES_20K	(20000)
#define PE_CYCLES_30K	(30000)
#define PE_CYCLES_40K	(40000)
#define PE_CYCLES_50K	(50000)
#define PE_CYCLES_60K	(60000)
#define PE_CYCLES_70K	(70000)
#define PE_CYCLES_80K	(80000)
#define PE_CYCLES_90K	(90000)
#define PE_CYCLES_100K	(100000)
#define PE_CYCLES_200K	(200000)
#define PE_CYCLES_300K	(300000)
#define PE_CYCLES_500K	(500000)
#define PE_CYCLES_600K	(600000)
#define PE_CYCLES_700K	(700000)
#define PE_CYCLES_800K	(800000)
#define PE_CYCLES_900K	(900000)
#define PE_CYCLES_1000K	(1000000)

#define B_TO_KB(x)	((x) >> 10)
#define MOD(x, y)	((x) % (y))

#define MAX_CHIPS (4U)
enum error_management {
/*first spare area location on first page of each block*/
	PST_FIRST_PAGE = 0x1,
/*first spare area location on first page and second page of each block*/
	PST_FIRST_TWO_PAGES = 0x11,
/*first spare area location on last page of each block*/
	PST_LAST_PAGE = 0x2,
/*first spare area location on last two page of each block*/
	PST_LAST_TWO_PAGES = 0x22,
/*first spare area location on first and last page of each block*/
	PST_FIRST_AND_LAST_PAGES = 0x4,
/*first spare area location on first two and last page of each block*/
	PST_FIRST_TWO_AND_LAST_PAGES = 0x8,
};

struct aw_nand_flash_dev {
	char *name;
	union {
		struct {
			uint8_t mfr_id;
			uint8_t dev_id;
		};
		uint8_t id[RAWNAND_MAX_ID_LEN];
	};
	int id_len;
	unsigned int dies_per_chip;
	/*main data size, eg. Page Size:(2K+64)byte ==> pagesize=2K byte,
	 * sparesize=64byte*/
	unsigned int pagesize;
	unsigned int sparesize;
	unsigned int pages_per_blk;
	unsigned int blks_per_die;
	unsigned int access_freq;
	enum error_management badblock_flag_pos;
	unsigned int pe_cycles;
	unsigned int options;
};


struct rawnand_manufacture {
	u8 id;
	char *name;
	struct aw_nand_flash_dev *dev;
	int ndev;
};
#define RAWNAND_MANUFACTURE(_id, _name, _mfr)		\
	{							\
		.id = _id,					\
		.name = _name,					\
		.dev = _mfr,					\
		.ndev = (sizeof(_mfr) / sizeof(_mfr[0])),	\
	}

struct aw_nand_manufactures {
	struct rawnand_manufacture *manuf;
	int nm;
};

#define AW_NAND_MANUFACTURE(_aw_nand, _manufs)			\
		struct aw_nand_manufactures _aw_nand = {		\
			.manuf = _manufs,				\
			.nm = (sizeof(_manufs) / sizeof(_manufs[0])),	\
		}

struct aw_nand_chip;
struct mtd_info;
extern struct aw_nand_chip awnand_chip;
extern struct aw_nand_manufactures aw_nand_manufs;
extern struct aw_nand_sec_sto rawnand_sec_sto;

#if IS_ENABLED(CONFIG_ARCH_SUN8IW11)
extern struct aw_nand_host aw_host;
extern struct udevice awnand_device;

#define BCH_NO	(-1)
#define BCH_16	(0)
#define BCH_24	(1)
#define BCH_28	(2)
#define BCH_32	(3)
#define BCH_40	(4)
#define BCH_48	(5)
#define BCH_56	(6)
#define BCH_60	(7)
#define BCH_64	(8)

struct nfc_reg {
	volatile unsigned int *ctl;			/*0x0000 NDFC Control Register*/
	volatile unsigned int *sta;			/*0x0004 NDFC Status Register*/
	volatile unsigned int *int_ctl;			/*0x0008 NDFC Interrupt and DMA Enable Register*/
	volatile unsigned int *timing_ctl;		/*0x000C NDFC Timing Control Register*/
	volatile unsigned int *timing_cfg;		/*0x0010 NDFC Timing Configure Register*/
	volatile unsigned int *addr_low;		/*0x0014 NDFC Address Low Word Register*/
	volatile unsigned int *addr_high;		/*0x0018 NDFC Address High Word Register*/
	volatile unsigned int *data_sect_num;		/*0x001C NDFC Data Block Mask Register*/
	volatile unsigned int *cnt;			/*0x0020 NDFC Data Block Mask Register*/
	volatile unsigned int *cmd;			/*0x0024 NDFC Command IO Register*/
	volatile unsigned int *read_cmd_set;		/*0x0028 NDFC Command Set Register 0*/
	volatile unsigned int *write_cmd_set;		/*0x002C NDFC Command Set Register 1*/
	volatile unsigned int *io_data;
	volatile unsigned int *ecc_ctl;			/*0x0034 NDFC ECC Control Register*/
	volatile unsigned int *ecc_sta;			/*0x0038 NDFC ECC Status Register*/
//	volatile unsigned int *data_pattern_sta;	/*0x003C NDFC Data Pattern Status Register*/
	volatile unsigned int *efr;			/*0x003C NDFC Enhanced Featur Register*/
	volatile unsigned int *rdata_sta_ctl;		/*0x0044 NDFC Read Data Status Control Register*/
	volatile unsigned int *rdata_sta_0;		/*0x0048 NDFC Read Data Status Register 0*/
	volatile unsigned int *rdata_sta_1;		/*0x004C NDFC Read Data Status Register 1*/
#define MAX_ERR_CNT (4U)
	volatile unsigned int *err_cnt[MAX_ERR_CNT];	/*0x0050 NDFC Error Counter Register 0*/
#define MAX_USER_DATA_LEN (4U)
	volatile unsigned int *user_data_len_base;	/*0x0070 NDFC User Data Length Register X*/
#define MAX_USER_DATA (32U)
	volatile unsigned int *user_data_base;		/*0x0080 NDFC User Data Register X*/
	volatile unsigned int *efnand_sta;		/*0x0110 NDFC EFNAND STATUS Register*/
	volatile unsigned int *spare_area;		/*0x0114 NDFC Spare Aera Register*/
	volatile unsigned int *pat_id;			/*0x0118 NDFC Pattern ID Register*/
//	volatile unsigned int *ddr2_spec_ctl;		/*0x011C NDFC DDR2 Specific Control Register*/
//	volatile unsigned int *ndma_mode_ctl;		/*0x0120 NDFC Normal DMA Mode Control Register*/
	volatile unsigned int *mbus_dma_addr;		/*0x0200 NDFC MBUS DMA Descriptor List Base Address Register in no.1 version*/
	volatile unsigned int *mbus_dma_cnt;		/*0x0204 NDFC MBUS DMA Interrupt Status Register in no.1 version*/
//	volatile unsigned int *mdma_int_mask;		/*0x0208 NDFC MBUS DMA Interrupt Enable Register in no.1 version*/
//	volatile unsigned int *mdma_cur_desc_addr;	/*0x020C NDFC MBUS DMA Current Descriptor Address Register in no.1 version*/
//	volatile unsigned int *mdma_cur_buf_addr;	/*0x0210 NDFC MBUS DMA Current Buffer Address Register in no.1 version*/
	volatile unsigned int *dma_mode;			/*0x0214 NDFC Normal DMA Byte Counter Register*/
//	volatile unsigned int *ver;			/*0x02F0 NDFC Version Number Register*/
	volatile unsigned int *ram0_base;		/*0x0400 NDFC Control Register*/
	volatile unsigned int *ram1_base;		/*0x0800 NDFC Control Register*/
};

#else /* CONFIG_ARCH_SUN8IW11 */

#define BCH_NO	(-1)
#define BCH_16	(0)
#define BCH_24	(1)
#define BCH_28	(2)
#define BCH_32	(3)
#define BCH_40	(4)
#define BCH_44	(5)
#define BCH_48	(6)
#define BCH_52	(7)
#define BCH_56	(8)
#define BCH_60	(9)
#define BCH_64	(10)
#define BCH_68	(11)
#define BCH_72	(12)
#define BCH_76	(13)
#define BCH_80	(14)

struct nfc_reg {
	volatile unsigned int *ctl;			/*0x0000 NDFC Control Register*/
	volatile unsigned int *sta;			/*0x0004 NDFC Status Register*/
	volatile unsigned int *int_ctl;			/*0x0008 NDFC Interrupt and DMA Enable Register*/
	volatile unsigned int *timing_ctl;		/*0x000C NDFC Timing Control Register*/
	volatile unsigned int *timing_cfg;		/*0x0010 NDFC Timing Configure Register*/
	volatile unsigned int *addr_low;		/*0x0014 NDFC Address Low Word Register*/
	volatile unsigned int *addr_high;		/*0x0018 NDFC Address High Word Register*/
	volatile unsigned int *data_block_mask;		/*0x001C NDFC Data Block Mask Register*/
	volatile unsigned int *cnt;			/*0x0020 NDFC Data Block Mask Register*/
	volatile unsigned int *cmd;			/*0x0024 NDFC Command IO Register*/
	volatile unsigned int *read_cmd_set;		/*0x0028 NDFC Command Set Register 0*/
	volatile unsigned int *write_cmd_set;		/*0x002C NDFC Command Set Register 1*/
	volatile unsigned int *ecc_ctl;			/*0x0034 NDFC ECC Control Register*/
	volatile unsigned int *ecc_sta;			/*0x0038 NDFC ECC Status Register*/
	volatile unsigned int *data_pattern_sta;	/*0x003C NDFC Data Pattern Status Register*/
	volatile unsigned int *efr;			/*0x0040 NDFC Enhanced Featur Register*/
	volatile unsigned int *rdata_sta_ctl;		/*0x0044 NDFC Read Data Status Control Register*/
	volatile unsigned int *rdata_sta_0;		/*0x0048 NDFC Read Data Status Register 0*/
	volatile unsigned int *rdata_sta_1;		/*0x004C NDFC Read Data Status Register 1*/
#define MAX_ERR_CNT (8U)
	volatile unsigned int *err_cnt[MAX_ERR_CNT];	/*0x0050 NDFC Error Counter Register 0*/
#define MAX_USER_DATA_LEN (4U)
	volatile unsigned int *user_data_len_base;	/*0x0070 NDFC User Data Length Register X*/
#define MAX_USER_DATA (32U)
	volatile unsigned int *user_data_base;		/*0x0080 NDFC User Data Register X*/
	volatile unsigned int *efnand_sta;		/*0x0110 NDFC EFNAND STATUS Register*/
	volatile unsigned int *spare_area;		/*0x0114 NDFC Spare Aera Register*/
	volatile unsigned int *pat_id;			/*0x0118 NDFC Pattern ID Register*/
	volatile unsigned int *ddr2_spec_ctl;		/*0x011C NDFC DDR2 Specific Control Register*/
	volatile unsigned int *ndma_mode_ctl;		/*0x0120 NDFC Normal DMA Mode Control Register*/
	volatile unsigned int *mbus_dma_dlba;		/*0x0200 NDFC MBUS DMA Descriptor List Base Address Register in no.1 version*/
	volatile unsigned int *mbus_dma_sta;		/*0x0204 NDFC MBUS DMA Interrupt Status Register in no.1 version*/
	volatile unsigned int *mdma_int_mask;		/*0x0208 NDFC MBUS DMA Interrupt Enable Register in no.1 version*/
	volatile unsigned int *mdma_cur_desc_addr;	/*0x020C NDFC MBUS DMA Current Descriptor Address Register in no.1 version*/
	volatile unsigned int *mdma_cur_buf_addr;	/*0x0210 NDFC MBUS DMA Current Buffer Address Register in no.1 version*/
	volatile unsigned int *dma_cnt;			/*0x0214 NDFC Normal DMA Byte Counter Register*/
	volatile unsigned int *ver;			/*0x02F0 NDFC Version Number Register*/
	volatile unsigned int *ram0_base;		/*0x0400 NDFC Control Register*/
	volatile unsigned int *ram1_base;		/*0x0800 NDFC Control Register*/
};
#endif /* CONFIG_ARCH_SUN8IW11 */

enum op_type {
	FLASH_READ = 0,
	FLASH_WRITE,
	FLASH_CACHE_READ,
	FLASH_CACHE_WRITE,
	FLASH_MULTI_READ,
	FLASH_MULTI_WRITE,
	FLASH_ULTI_CACHE_READ,
	FLASH_MULTI_CACHE_WRITE,
};
enum normal_req_type {
	CMD = 0,
	/*e.g. erase block*/
	CMD_WITH_ADDR,
	/*e.g. set/get feature/read page parameter/read id/read status (addr_cycles is 0)*/
	CMD_WITH_ADDR_DATA,
};
enum ecc_layout {
	INTERLEAVE = 0,
	SEQUENCE,
};

enum data_type {
	MAINSPARE = 0,
	ONLY_SPARE,
};

enum plane_ab {
	PLANE_A = 0,
	PLANE_B,
};

struct aw_nfc_normal_req {
	enum normal_req_type type;
	bool wait_rb;
	union {
		struct {
			uint8_t code;
		} cmd;

		struct {
			uint8_t code;
			uint8_t addr[MAX_CYCLE];
			/*valid address number*/
			uint8_t addr_cycles;
		} cmd_with_addr;

		struct {
			/*read status*/
			uint8_t code;
			int len;
			uint8_t *in;
		} cmd_with_data;

		struct {
			uint8_t code;
			uint8_t addr[MAX_CYCLE];
			/*valid address number*/
			uint8_t addr_cycles;
			uint8_t direct;
			int len;
			uint8_t *in;
			uint8_t *out;
		} cmd_with_addr_data;
	} op;
};

#define NORMAL_REQ_CMD_WITH_ADDR1_DATA_OUT(_req, _cmd, _addr, _data, _len)	\
struct aw_nfc_normal_req _req = {		\
	.type = CMD_WITH_ADDR_DATA,				\
	.op = {					\
		.cmd_with_addr_data = {		\
			.code = _cmd,		\
			.addr[0] = _addr,		\
			.addr_cycles = 1,	\
			.direct = FLASH_WRITE,	\
			.len = _len,		\
			.out = _data,		\
		}				\
	}					\
}

#define NORMAL_REQ_CMD_WITH_ADDR0_DATA_IN(_req, _cmd, _data, _len)	\
struct aw_nfc_normal_req _req = {		\
	.type = CMD_WITH_ADDR_DATA,				\
	.op = {					\
		.cmd_with_addr_data = {		\
			.code = _cmd,		\
			.addr_cycles = 0,	\
			.direct = FLASH_READ,	\
			.len = _len,		\
			.in = _data,		\
		}				\
	}					\
}


#define NORMAL_REQ_CMD_WITH_ADDR1_DATA_IN(_req, _cmd, _addr, _data, _len)	\
struct aw_nfc_normal_req _req = {		\
	.type = CMD_WITH_ADDR_DATA,				\
	.op = {					\
		.cmd_with_addr_data = {		\
			.code = _cmd,		\
			.addr[0] = _addr,	\
			.addr_cycles = 1,	\
			.direct = FLASH_READ,		\
			.len = _len,		\
			.in = _data,		\
		}				\
	}					\
}

#define NORMAL_REQ_CMD(_req, _cmd_code)		\
struct aw_nfc_normal_req _req = {		\
	.type = CMD,				\
	.wait_rb = 0,				\
	.op = {					\
		.cmd = {			\
			.code = _cmd_code,	\
		}				\
	}					\
}

#define NORMAL_REQ_CMD_WAIT_RB(_req, _cmd_code)		\
struct aw_nfc_normal_req _req = {		\
	.type = CMD,				\
	.wait_rb = 1,				\
	.op = {					\
		.cmd = {			\
			.code = _cmd_code,	\
		}				\
	}					\
}


#define NORMAL_REQ_CMD_WITH_ADDR_N1(_req, _cmd_code, _addr)	\
struct aw_nfc_normal_req _req = {		\
	.type = CMD_WITH_ADDR,			\
	.wait_rb = 0,				\
	.op = {					\
		.cmd_with_addr = {		\
			.code = _cmd_code,	\
			.addr[0] = _addr,	\
			.addr_cycles = 1,	\
		}				\
	}					\
}

#define NORMAL_REQ_CMD_WITH_ADDR_N2(_req, _cmd_code, _addr1, _addr2)	\
struct aw_nfc_normal_req _req = {		\
	.type = CMD_WITH_ADDR,			\
	.op = {					\
		.cmd_with_addr = {		\
			.code = _cmd_code,	\
			.addr[0] = _addr1,	\
			.addr[1] = _addr2,	\
			.addr_cycles = 2,	\
		}				\
	}					\
}

#define NORMAL_REQ_CMD_WITH_ADDR_N3(_req, _cmd_code, _addr1, _addr2, _addr3)	\
struct aw_nfc_normal_req _req = {		\
	.type = CMD_WITH_ADDR,			\
	.op = {					\
		.cmd_with_addr = {		\
			.code = _cmd_code,	\
			.addr[0] = _addr1,	\
			.addr[1] = _addr2,	\
			.addr[2] = _addr3,	\
			.addr_cycles = 3,	\
		}				\
	}					\
}

#define NORMAL_REQ_CMD_WITH_ADDR_N4(_req, _cmd_code, _addr1, _addr2, _addr3, _addr4)	\
struct aw_nfc_normal_req _req = {		\
	.type = CMD_WITH_ADDR,			\
	.op = {					\
		.cmd_with_addr = {		\
			.code = _cmd_code,	\
			.addr[0] = _addr1,	\
			.addr[1] = _addr2,	\
			.addr[2] = _addr3,	\
			.addr[3] = _addr4,	\
			.addr_cycles = 4,	\
		}				\
	}					\
}

#define NORMAL_REQ_CMD_WITH_ADDR_N5(_req, _cmd_code, _addr1, _addr2, _addr3, _addr4, _addr5)	\
struct aw_nfc_normal_req _req = {		\
	.type = CMD_WITH_ADDR,			\
	.op = {					\
		.cmd_with_addr = {		\
			.code = _cmd_code,	\
			.addr[0] = _addr1,	\
			.addr[1] = _addr2,	\
			.addr[2] = _addr3,	\
			.addr[3] = _addr4,	\
			.addr[4] = _addr5,	\
			.addr[5] = _addr5,	\
			.addr_cycles = 5,	\
		}				\
	}					\
}


#define NORMAL_REQ_GET_ADDR(__p_req, __p_addr_low, __p_addr_high)	\
	int i = 0;							\
									\
	for (i = 0; i < __p_req->addr_cycles; i++) {			\
		if (i < 4)						\
			(*__p_addr_low) |= req->addr[i] << (i * 8);	\
		else							\
			(*__p_addr_high) |= req->addr[i] << ((i - 4) * 8);	\
	}								\


struct aw_nfc_batch_req {
	enum op_type type;
	enum ecc_layout layout;
	enum plane_ab plane;
	union {
		struct {
			uint8_t first;
			uint8_t snd;
			uint8_t rnd1;
			uint8_t rnd2;
		} val;
		struct {
			/*page read*/
			uint8_t READ0;
			uint8_t READSTART;
			uint8_t RNOUT;
			uint8_t RNOUTSTART;
		} r;

		struct {
			/*page program/write*/
			uint8_t SEQIN;
			uint8_t PAGEPROG;
			uint8_t RNDIN;
		} w;

		struct {
			/*page cache write*/
			uint8_t SEQIN;
			uint8_t CACHEDPROG;
			uint8_t RNDIN;
		} cw;

		struct {
			/*multi-plane page write*/
			uint8_t SEQIN;
			uint8_t MULTIPROG;
			uint8_t RNDIN;
		} mw;

		struct {
			/*page read*/
			uint8_t READ0;
			uint8_t MULTIREADSTART;
			uint8_t RNOUT;
			uint8_t RNOUTSTART;
		} mr;

	} cmd;

	struct {
		uint32_t page;
		uint8_t row_cycles;
	} addr;

	struct {
		enum data_type type;
		/*main_len align with ecc_block(1KB)*/
		/*spare_len == chip->avalid_sparesize*/
		int main_len;
		int spare_len;
		uint8_t *main;
		uint8_t *spare;
	} data;
};

#define BATCH_REQ_READ(_req, _page, _row_cycles, _mdata, _mlen, _sdata, _slen)	\
	struct aw_nfc_batch_req _req = {				\
		.type = FLASH_READ,						\
		.layout = INTERLEAVE,						\
		.cmd.r = {							\
			.READ0 = RAWNAND_CMD_READ0,				\
			.READSTART = RAWNAND_CMD_READSTART,			\
			.RNOUT = RAWNAND_CMD_RNDOUT,				\
			.RNOUTSTART = RAWNAND_CMD_RNDOUTSTART,			\
		},								\
		.addr = {							\
			.page = _page,						\
			.row_cycles = _row_cycles,				\
		},								\
		.data = {							\
			.type = MAINSPARE,					\
			.main_len = _mlen,					\
			.spare_len = _slen,					\
			.main = _mdata,					\
			.spare = _sdata,					\
		},								\
	}

#define BATCH_REQ_MULTI_READ(_req, _page, _row_cycles, _mdata, _mlen, _sdata, _slen)	\
	struct aw_nfc_batch_req _req = {						\
		.type = FLASH_MULTI_READ,						\
		.layout = INTERLEAVE,						\
		.cmd.mr = {							\
			.READ0 = RAWNAND_CMD_READ0,				\
			.MULTIREADSTART = RAWNAND_CMD_MULTIPREADSTART,		\
			.RNOUT = RAWNAND_CMD_RNDOUT,				\
			.RNOUTSTART = RAWNAND_CMD_RNDOUTSTART,			\
		},								\
		.addr = {							\
			.page = _page,						\
			.row_cycles = _row_cycles,				\
		},								\
		.data = {							\
			.type = MAINSPARE,					\
			.main_len = _mlen,					\
			.spare_len = _slen,					\
			.main = _mdata,					\
			.spare = _sdata,					\
		},								\
	}

#define BATCH_REQ_READ_SEQ(_req, _page, _row_cycles, _mdata, _mlen, _sdata, _slen)	\
	struct aw_nfc_batch_req _req = {						\
		.type = FLASH_READ,							\
		.layout = SEQUENCE,						\
		.cmd.r = {							\
			.READ0 = RAWNAND_CMD_READ0,				\
			.READSTART = RAWNAND_CMD_READSTART,			\
			.RNOUT = RAWNAND_CMD_RNDOUT,				\
			.RNOUTSTART = RAWNAND_CMD_RNDOUTSTART,			\
		},								\
		.addr = {							\
			.page = _page,						\
			.row_cycles = _row_cycles,				\
		},								\
		.data = {							\
			.type = MAINSPARE,					\
			.main_len = _mlen,					\
			.spare_len = _slen,					\
			.main = _mdata,					\
			.spare = _sdata,					\
		},								\
	}


#define BATCH_REQ_READ_ONLY_SPARE(_req, _page, _row_cycles, _sdata, _slen)	\
	struct aw_nfc_batch_req _req = {						\
		.type = FLASH_READ,							\
		.layout = INTERLEAVE,						\
		.cmd.r = {							\
			.READ0 = RAWNAND_CMD_READ0,				\
			.READSTART = RAWNAND_CMD_READSTART,			\
			.RNOUT = RAWNAND_CMD_RNDOUT,				\
			.RNOUTSTART = RAWNAND_CMD_RNDOUTSTART,			\
		},								\
		.addr = {							\
			.page = _page,						\
			.row_cycles = _row_cycles,				\
		},								\
		.data = {							\
			.type = ONLY_SPARE,					\
			.main_len = 0,						\
			.spare_len = _slen,					\
			.main = NULL,					\
			.spare = _sdata,					\
		},								\
	}



#define BATCH_REQ_WRITE(_req, _page, _row_cycles, _mdata, _mlen, _sdata, _slen)	\
	struct aw_nfc_batch_req _req = {						\
		.type = FLASH_WRITE,							\
		.layout = INTERLEAVE,						\
		.cmd.w = {							\
			.SEQIN = RAWNAND_CMD_SEQIN,				\
			.PAGEPROG = RAWNAND_CMD_PAGEPROG,			\
			.RNDIN = RAWNAND_CMD_RNDIN,				\
		},								\
		.addr = {							\
			.page = _page,						\
			.row_cycles = _row_cycles,				\
		},								\
		.data = {							\
			.type = MAINSPARE,					\
			.main_len = _mlen,					\
			.spare_len = _slen,					\
			.main = _mdata,					\
			.spare = _sdata,					\
		},								\
	}

#define BATCH_REQ_JEDEC_WRITE(_req, _page, _row_cycles, _mdata, _mlen, _sdata, _slen)	\
	struct aw_nfc_batch_req _req = {						\
		.type = FLASH_WRITE,							\
		.layout = INTERLEAVE,						\
		.cmd.w = {							\
			.SEQIN = RAWNAND_CMD_JEDEC_SEQIN,			\
			.PAGEPROG = RAWNAND_CMD_PAGEPROG,			\
			.RNDIN = RAWNAND_CMD_RNDIN,				\
		},								\
		.addr = {							\
			.page = _page,						\
			.row_cycles = _row_cycles,				\
		},								\
		.data = {							\
			.type = MAINSPARE,					\
			.main_len = _mlen,					\
			.spare_len = _slen,					\
			.main = _mdata,					\
			.spare = _sdata,					\
		},								\
	}

#define BATCH_REQ_MULTI_WRITE(_req, _page, _row_cycles, _mdata, _mlen, _sdata, _slen, _plane)	\
	struct aw_nfc_batch_req _req = {					\
		.type = FLASH_MULTI_WRITE,					\
		.layout = INTERLEAVE,						\
		.plane = _plane,						\
		.cmd.mw = {							\
			.SEQIN = RAWNAND_CMD_SEQIN,				\
			.MULTIPROG = RAWNAND_CMD_MULTIPROG,			\
			.RNDIN = RAWNAND_CMD_RNDIN,				\
		},								\
		.addr = {							\
			.page = _page,						\
			.row_cycles = _row_cycles,				\
		},								\
		.data = {							\
			.type = MAINSPARE,					\
			.main_len = _mlen,					\
			.spare_len = _slen,					\
			.main = _mdata,					\
			.spare = _sdata,					\
		},								\
	}

#define BATCH_REQ_CACHE_WRITE(_req, _page, _row_cycles, _mdata, _mlen, _sdata, _slen)	\
	struct aw_nfc_batch_req _req = {						\
		.type = FLASH_CACHE_WRITE,						\
		.layout = INTERLEAVE,						\
		.cmd.cw = {							\
			.SEQIN = RAWNAND_CMD_SEQIN,				\
			.CACHEDPROG = RAWNAND_CMD_CACHEDPROG,			\
			.RNDIN = RAWNAND_CMD_RNDIN,				\
		},								\
		.addr = {							\
			.page = _page,						\
			.row_cycles = _row_cycles,				\
		},								\
		.data = {							\
			.type = MAINSPARE,					\
			.main_len = _mlen,					\
			.spare_len = _slen,					\
			.main = _mdata,					\
			.spare = _sdata,					\
		},								\
	}

#define BATCH_REQ_WRITE_SEQ(_req, _page, _row_cycles, _mdata, _mlen, _sdata, _slen)	\
	struct aw_nfc_batch_req _req = {						\
		.type = FLASH_WRITE,							\
		.layout = SEQUENCE,						\
		.cmd.w = {							\
			.SEQIN = RAWNAND_CMD_SEQIN,				\
			.PAGEPROG = RAWNAND_CMD_PAGEPROG,			\
			.RNDIN = RAWNAND_CMD_RNDIN,				\
		},								\
		.addr = {							\
			.page = _page,						\
			.row_cycles = _row_cycles,				\
		},								\
		.data = {							\
			.type = MAINSPARE,					\
			.main_len = _mlen,					\
			.spare_len = _slen,					\
			.main = _mdata,					\
			.spare = _sdata,					\
		},								\
	}


/**
 * enum nand_data_interface_type - NAND interface timing type
 * @NAND_SDR_IFACE:	Single Data Rate interface
 */
enum rawnand_data_interface_type {
	RAWNAND_SDR_IFACE = 0,
	RAWNAND_ONFI_DDR = 0x2,
	RAWNAND_ONFI_DDR2 = 0x12,
	RAWNAND_TOGGLE_DDR = 0x3,
	RAWNAND_TOGGLE_DDR2 = 0x13,
};

enum dma_type {
	GENERIC_DMA = 0,
	MBUS_DMA,
};


#define NFC_DESC_FIRST_FLAG (0x1 << 3)
#define NFC_DESC_LAST_FLAG (0x1 << 2)
#define NFC_DMA_DESC_MAX_NUM (32)
#define NFC_DESC_BSIZE(bsize)	((bsize)&0xFFFF) /*in order to ndfc spec BUFF_SIZE 16bits valid*/
struct aw_nfc_dma_desc {
	unsigned int cfg;
	unsigned int bcnt;
	unsigned int buff;
	struct aw_nfc_dma_desc *next;
};
struct aw_nand_host {
	struct device *dev;
	struct pinctrl *pinctrl;
	void __iomem *base;
	struct nfc_reg nfc_reg;

	struct clk *pclk;	/*pll clock*/
	struct clk *mdclk;	/*nand module clock*/
	struct clk *mcclk;	/*nand ecc engine clock*/
	struct clk *busclk;
	struct clk *mbusclk;
	struct clk *mbus_sw_clk;
#ifndef __UBOOT__
	struct regulator *vcc_nand;
	struct regulator *vcc_io;
#endif
	unsigned int mdclk_val;
	unsigned int mcclk_val;
#if IS_ENABLED(CONFIG_ARCH_SUN8IW11)
	void __iomem *mbus_gate;
	void __iomem *bus_gate;
	void __iomem *bus_reset;
#endif
	bool init;

	unsigned long clk_rate;
	unsigned int timing_ctl;
	unsigned int timing_cfg;
	enum rawnand_data_interface_type nf_type;

	u8 cs[4];
	u8 rb[4];

	/*1: use b2r int when rb signal from busy to ready; 0: don't use b2r int*/
	uint8_t use_rb_int;
	uint8_t rb_ready_flag;

	uint8_t use_dma;
	enum dma_type dma_type;
	dma_addr_t dma_addr;
	dma_addr_t desc_addr; /*descripte dma addr*/
	/*1: use dma int when dma is completed; 0: don't use dma int*/
	uint8_t use_dma_int;
	uint8_t dma_ready_flag;

	uint8_t bitflips;

#define MAX_SPARE_SIZE	(64)
	uint8_t *spare_default;

	struct aw_nfc_dma_desc *nfc_dma_desc;	/*physic addr*/
	struct aw_nfc_dma_desc *nfc_dma_desc_cpu; /*virtual addr*/

	int (*normal_op)(struct aw_nand_chip *chip, struct aw_nfc_normal_req *req);
	int (*batch_op)(struct aw_nand_chip *chip, struct aw_nfc_batch_req *req);
	bool (*rb_ready)(struct aw_nand_chip *chip, struct aw_nand_host *host);

	void *priv;

};


#define NAND_DATA_ITF_TYPE_TOGGLE_DDR1_2(chip)	\
	((chip->data_interface.type == RAWNAND_TOGGLE_DDR) || \
	 (chip->data_interface.type == RAWNAND_TOGGLE_DDR2))

/**
 * struct nand_data_interface - NAND interface timing
 * @type:	type of the timing
 * @timings:	The timing, type according to @type
 */
struct rawnand_data_interface {
	enum rawnand_data_interface_type type;

	int (*set_feature)(struct aw_nand_chip *chip,
			int feature_addr, uint8_t *feature_para);
	int (*get_feature)(struct aw_nand_chip *chip,
			int feature_addr, uint8_t *feature_para);
};

struct ce_info {
	int ce_no;
	int relate_rb_no;
};

struct select_chip {
	int chip_no;
	struct ce_info ceinfo[MAX_CHIPS];
};

/*aw_nand_chip_cache design in simu chip layer*/
struct aw_nand_chip_cache {
#define INVALID_CACHE (-1)
	uint32_t simu_pageno;
	uint32_t simu_oobno;
	int simu_page_len;
	/*valid data size in simup pagebuf start*/
	int valid_col;
	/*valid data size in simu pagebuf len from valid col*/
	int valid_len;
	uint8_t *simu_pagebuf;
	/*valid data size in simu oob start*/
	int valid_oob_col;
	/*valid data size in simu oob len from valid oob col*/
	int valid_oob_len;
	int simu_oob_len;
	uint8_t *simu_oobbuf;
	uint8_t bitflips;
	int sub_page_len;
	int chipno;
};


struct aw_nand_chip {
	struct mutex lock;
	/**************************
	 *  mtd layer
	 *------------------------
	 *  simu chip
	 *------------------------
	 *     chip
	 *| --blkn----|--blkn+1--|
	 *|  (planeA) |  (planeB)|
	 * ************************/
	struct mtd_info mtd;
#define SLC_NAND	(0)
#define MLC_NAND	(1)
	int type;

	uint8_t id[RAWNAND_MAX_ID_LEN];
	unsigned int dies;
#define MAX_DIES (2U)
	uint64_t diesize[MAX_DIES];
	int chips;
	uint64_t chipsize;
	uint64_t simu_chipsize;
	int chip_shift;
	int simu_chip_shift;
	int chip_pages;
	/*simulation is for multi, see line@48 rawnand  multiplane layout.*/
	int simu_chip_pages;
	int chip_pages_mask;
	int simu_chip_pages_mask;

	/*main data size*/
	int pagesize;
	int simu_pagesize;
	/*main data size shift*/
	unsigned int pagesize_shift;
	unsigned int simu_pagesize_shift;
	int pagesize_mask;
	int simu_pagesize_mask;
	/*main data size + spare data size*/
	int real_pagesize;
	unsigned int erasesize;
	unsigned int simu_erasesize;
	unsigned int erase_shift;
	unsigned int simu_erase_shift;
	unsigned int erasesize_mask;
	unsigned int simu_erasesize_mask;
	unsigned int pages_per_blk_shift;
	unsigned int simu_pages_per_blk_shift;
	unsigned int pages_per_blk_mask;
	unsigned int simu_pages_per_blk_mask;
	int avalid_sparesize;
	int ecc_mode;
	int random;
	int row_cycles;
	enum error_management badblock_mark_pos;
	unsigned int pe_cycles;

	unsigned int options;
	int clk_rate;

	int operate_boot0;
	int boot0_ecc_mode;
	int uboot_end;

	struct select_chip selected_chip;
	struct ce_info ceinfo[MAX_CHIPS];

	struct aw_nand_chip_cache simu_chip_buffer;

	struct rawnand_data_interface data_interface;
#define BBT_B_INVALID	(2)
#define BBT_B_BAD	(1)
#define BBT_B_GOOD	(0)
	uint8_t *bbt;
	/*mark whether the corresponding bbt bit is updated*/
	uint8_t *bbtd;

	uint8_t bitflips;

	void (*select_chip)(struct mtd_info *mtd, int chip);
	bool  (*dev_ready_wait)(struct mtd_info *mtd);
	int  (*dev_status)(struct mtd_info *mtd);


	int (*block_bad)(struct mtd_info *mtd, int block);
	int (*simu_block_bad)(struct mtd_info *mtd, int block);
	int (*block_markbad)(struct mtd_info *mtd, int block);
	int (*simu_block_markbad)(struct mtd_info *mtd, int block);
	/*scan device to update bbt*/
	int (*scan_bbt)(struct mtd_info *mtd);

	int (*erase)(struct mtd_info *mtd, int page);
	int (*multi_erase)(struct mtd_info *mtd, int page);

	int (*write_page)(struct mtd_info *mtd, struct aw_nand_chip *chip,
		uint8_t *mdata, int mlen, uint8_t *sdata, int slen, int page);
	int (*multi_write_page)(struct mtd_info *mtd, struct aw_nand_chip *chip,
		uint8_t *mdata, int mlen, uint8_t *sdata, int slen, int page);
	int (*cache_write_page)(struct mtd_info *mtd, struct aw_nand_chip *chip,
		uint8_t *mdata, int mlen, uint8_t *sdata, int slen, int page);
	int (*read_page)(struct mtd_info *mtd, struct aw_nand_chip *chip,
		uint8_t *mdata, int mlen, uint8_t *sdata, int slen, int page);
	int (*multi_read_page)(struct mtd_info *mtd, struct aw_nand_chip *chip,
		uint8_t *mdata, int mlen, uint8_t *sdata, int slen, int page);
	int (*read_page_spare)(struct mtd_info *mtd, struct aw_nand_chip *chip,
		uint8_t *sdata, int slen, int page);

	int (*write_boot0_page)(struct mtd_info *mtd, struct aw_nand_chip *chip,
		uint8_t *mdata, int mlen, uint8_t *sdata, int slen, int page);
	int (*read_boot0_page)(struct mtd_info *mtd, struct aw_nand_chip *chip,
		uint8_t *mdata, int mlen, uint8_t *sdata, int slen, int page);

	int (*setup_read_retry)(struct mtd_info *mtd, struct aw_nand_chip *chip);
	int (*setup_data_interface)(struct mtd_info *mtd, struct aw_nand_chip *chip,
			int chipnr, const struct rawnand_data_interface *conf);


	struct aw_nand_flash_dev *dev;

	void *priv;
	struct list_head node;
};


struct aw_nand_sec_sto {
	struct aw_nand_chip *chip;
	/*initialized before read/write*/
	unsigned int startblk;
	unsigned int endblk;

	unsigned int blk[2];
	int init_end;
};

static inline struct aw_nand_host *awnand_chip_to_host(struct aw_nand_chip *chip)
{
	return (struct aw_nand_host *)chip->priv;
}

static inline struct aw_nand_chip *awnand_host_to_chip(struct aw_nand_host *host)
{
	return (struct aw_nand_chip *)host->priv;
}

static inline struct mtd_info *awnand_chip_to_mtd(struct aw_nand_chip *chip)
{
	return (struct mtd_info *)&chip->mtd;
}

static inline struct aw_nand_chip *awnand_mtd_to_chip(struct mtd_info *mtd)
{
	return container_of(mtd, struct aw_nand_chip, mtd);
	//return (struct aw_nand_chip *)container_of(mtd, struct aw_nand_chip, mtd);
}

static inline struct aw_nand_host *awnand_nfc_to_host(struct nfc_reg *nfc)
{
	return container_of(nfc, struct aw_nand_host, nfc_reg);
}

static inline struct aw_nand_chip *get_rawnand(void)
{
	return &awnand_chip;
}

static inline struct aw_nand_sec_sto *get_rawnand_sec_sto(void)
{
	return &rawnand_sec_sto;
}

/**
 * check_offs_len - check mtd->_erase requset legality
 * */
static inline int check_offs_len(struct mtd_info *mtd, loff_t ofs, uint64_t len)
{
	struct aw_nand_chip *chip = awnand_mtd_to_chip(mtd);
	int ret = 0;

	/*check align to block and exceed the mtd size*/
	if ((ofs & chip->erasesize_mask) ||
		(len & chip->erasesize_mask) ||
		((ofs + len) > mtd->size)) {
		awrawnand_err("unaligned address@%llu len@%llu\n", ofs, len);
		ret = -EINVAL;
	}

	return ret;
}

/**
 * check_ofs_mtd_oob_ops - use to check _read/write_oob requeset legality
 * @rw: 0: read , 1: write
 * */
static inline int check_ofs_mtd_oob_ops(struct mtd_info *mtd, loff_t ofs, struct mtd_oob_ops *ops,
		int rw)
{

	struct aw_nand_chip *chip = awnand_mtd_to_chip(mtd);

	/*check boundary*/
	if (ops->datbuf && (ofs + ops->len) > mtd->size) {
		awrawnand_err("attemp to %s to@%llu len@%zu beyond end of device@%llu\n",
				rw ? "write" : "read", ofs, ops->len, mtd->size);
		return -EINVAL;
	}

	/*chec spare boundary*/
	if (ops->oobbuf && ((ops->ooboffs + ops->ooblen) > mtd->oobsize)) {
		awrawnand_err("attemp to %s ooblen@%zu beyond end of avalid_sparesize@%d\n",
				rw ? "write" : "read", ops->ooblen, chip->avalid_sparesize);
		return -EINVAL;
	}

	/*check align*/
	if ((ofs & chip->pagesize_mask) || (ops->len & chip->pagesize_mask)) {
		awrawnand_print("attemp to %s to@%llu len@%zu  non pagesize@%d aligned data\n",
				rw ? "write" : "read", ofs, ops->len, chip->pagesize);
		return 0;
	}

	return 0;
}

/**
 * check_from_len - check mtd->_read request legality
 * */
static inline int check_from_len(struct mtd_info *mtd, loff_t from, size_t len)
{
	struct aw_nand_chip *chip = awnand_mtd_to_chip(mtd);
	/*check boundary*/
	if ((from + len) > mtd->size) {
		awrawnand_err("attemp to read from@0x%llx len@0x%zx  beyond end of device@0x%llx\n",
				from, len, mtd->size);
		return -EINVAL;
	}

	if (from & chip->pagesize_mask) {
		awrawnand_err("attemp to read from@%llu not align to pagesize@%u\n",
				from, chip->pagesize);
		return -EINVAL;
	}

	return 0;
}

/**
 * check_to_len - check mtd->_write request legality
 * */
static inline int check_to_len(struct mtd_info *mtd, loff_t to, size_t len)
{
	struct aw_nand_chip *chip = awnand_mtd_to_chip(mtd);
	/*check boundary*/
	if ((to + len) > mtd->size) {
		awrawnand_err("attemp to write to@0x%llx len@0x%zx  beyond end of device@0x%llx\n",
				to, len, mtd->size);
		return -EINVAL;
	}

	if ((to & chip->pagesize_mask) || (len & chip->pagesize_mask)) {
		awrawnand_err("attemp to write to@%llu not align to phy-pagesize@%u\n",
				to, chip->pagesize);
		return -EINVAL;
	}

	return 0;
}

extern bool support_rawnand(void);
extern int aw_rawnand_secure_storage_read(struct aw_nand_sec_sto *sec_sto,
		int item, char *buf, unsigned int len);

extern int aw_rawnand_secure_storage_write(struct aw_nand_sec_sto *sec_sto,
		int item, char *buf, unsigned int len);

extern void rawnand_uboot_blknum(unsigned int *start, unsigned int *end);

extern int rawnand_mtd_download_uboot(unsigned int len, void *buf);
extern int rawnand_mtd_download_boot0(unsigned int len, void *buf);
extern int rawnand_mtd_secure_storage_write(int item, char *buf, unsigned int len);
extern int rawnand_mtd_secure_storage_read(int item, char *buf, unsigned int len);


extern int aw_rawnand_chip_block_bad(struct mtd_info *mtd, int block);
extern int aw_rawnand_chip_simu_block_bad(struct mtd_info *mtd, int block);
extern int aw_rawnand_chip_block_markbad(struct mtd_info *mtd, int block);
extern int aw_rawnand_chip_simu_block_markbad(struct mtd_info *mtd, int block);
extern int aw_rawnand_chip_scan_bbt(struct mtd_info *mtd);


extern int aw_host_init(struct device *dev);
extern void aw_host_exit(struct aw_nand_host *host);
extern int aw_host_init_tail(struct aw_nand_host *host);
extern int rawnand_mtd_init(void);
extern void rawnand_mtd_exit(void);
extern int rawnand_mtd_flush(void);
extern int rawnand_mtd_attach_mtd(void);
extern unsigned rawnand_mtd_size(void);
extern int rawnand_mtd_erase(int flag);
extern int rawnand_mtd_force_erase(void);

extern int rawnand_mtd_read(unsigned int start, unsigned int sects, void *buf);
extern int rawnand_mtd_write(unsigned int start, unsigned int sects, void *buf);
extern int rawnand_mtd_write_end(void);
extern int rawnand_mtd_get_flash_info(void *data, unsigned int len);
extern int rawnand_mtd_update_ubi_env(void);
extern int rawnand_mtd_set_last_vol_sects(unsigned int sects);

extern int rawslcnand_write_boot0_page(struct mtd_info *mtd, struct aw_nand_chip *chip,
		uint8_t *mdata, int mlen, uint8_t *sdata, int slen, int page);
extern int rawslcnand_read_boot0_page(struct mtd_info *mtd, struct aw_nand_chip *chip,
		uint8_t *mdata, int mlen, uint8_t *sdata, int slen, int page);
extern void rawnand_uboot_blknum(unsigned int *start, unsigned int *end);

extern int aw_rawnand_mtd_download_boot0(unsigned int len, void *buf);
extern int aw_rawnand_mtd_download_uboot(unsigned int len, void *buf);

#endif /*AW_RAWNAND_H*/
