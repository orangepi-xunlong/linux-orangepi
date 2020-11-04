
/*
 * Copyright (c) 2007-2017 Allwinnertech Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */


#ifndef _NAND_CONTROLLER_H
#define _NAND_CONTROLLER_H

#include "../rawnand_drv_cfg.h"

#define MAX_CHANNEL (NAND_GetMaxChannelCnt())
#define MAX_CHIP_PER_CHANNEL 4
#define MAX_CMD_PER_LIST 16
#define MAX_UDATA_LEN_FOR_ECCBLOCK 32
#define UDATA_LEN_FOR_4BYTESPER1K 4

typedef enum _nand_if_type {
	SDR = 0,
	ONFI_DDR = 0x2,
	ONFI_DDR2 = 0x12,
	TOG_DDR = 0x3,
	TOG_DDR2 = 0x13,
} nand_if_type;

enum _cmd_type {
	CMD_TYPE_NORMAL = 0,
	CMD_TYPE_BATCH,
	CMD_TYPE_MAX_CNT,
};

enum _ecc_layout_type {
	ECC_LAYOUT_INTERLEAVE = 0,
	ECC_LAYOUT_SEQUENCE,
	ECC_LAYOUT_TYPE_MAX_CNT,
};

enum _dma_mode {
	DMA_MODE_GENERAL_DMA = 0,
	DMA_MODE_MBUS_DMA,
	DMA_MODE_MAX_CNT
};

enum _ndfc_version {
	NDFC_VERSION_V1 = 1,
	NDFC_VERSION_V2,
	NDFC_VERSION_MAX_CNT
};

enum _ecc_bch_mode {
	BCH_16 = 0,
	BCH_24,
	BCH_28,
	BCH_32,
	BCH_40,
	BCH_44,
	BCH_48,
	BCH_52,
	BCH_56,
	BCH_60,
	BCH_64,
	BCH_68,
	BCH_72,
	BCH_76,
	BCH_80,
};
#define NDFC_DESC_FIRST_FLAG (0x1 << 3)
#define NDFC_DESC_LAST_FLAG (0x1 << 2)
#define MAX_NFC_DMA_DESC_NUM 32
#define NDFC_DESC_BSIZE(bsize) ((bsize)&0xFFFF) //??? 64KB
struct _ndfc_dma_desc_t {
	__u32 cfg;
	__u32 bcnt;
	__u32 buff;		       /*buffer address*/
	struct _ndfc_dma_desc_t *next; /*pointer to next descriptor*/
};
struct _nctri_cmd {
	u32 cmd_valid; // 0: skip current cmd io; 1: valid cmd io

	u32 cmd;      // cmd value
	u32 cmd_send; // 0: don't send cmd; 1: send cmd. only valid in normal
		      // command type.
	u32 cmd_wait_rb; // 0: ndfc don't wait rb ready; 1: ndfc wait br ready;

	u8 cmd_addr[MAX_CMD_PER_LIST]; // the buffer of address
	u32 cmd_acnt;		       // the number of address

	u32 cmd_trans_data_nand_bus; // 0: don't transfer data on external nand
				     // bus;  1: transfer data;
	u32 cmd_swap_data; // 0: don't swap data with host memory; 1: swap data
			   // with host memory;
	u32 cmd_swap_data_dma; // 0: transfer data using cpu; 1: transfer data
			       // useing dma
	u32 cmd_direction;       // 0: read data;  1: write data
	u32 cmd_mdata_len;       // the byte length of main data
	u32 cmd_data_block_mask; // data block mask of main data
	//    u32   cmd_mdata_addr;      //the address of main data
	u8 *cmd_mdata_addr; // the address of main data
};

struct _nctri_cmd_seq {
	u32 cmd_type;   // 0: normal command; 1: page command;
	u32 ecc_layout; // 0: interleave mode; 1: sequence mode;
	// u32   row_addr_auto_inc; 	 //0:    ; 1: row address auto increase;
	struct _nctri_cmd nctri_cmd[MAX_CMD_PER_LIST];
	u32 re_start_cmd;      // index of first repeated cmd [0,7] [0xff]
	u32 re_end_cmd;	// index of last repeated cmd  [0,7] [0xff]
	u32 re_cmd_times;      // repeat times
	u8 udata_len_mode[32]; // user data lenth for every ecc block,udata_len
			       // is 4 bytes aligned
	struct _ndfc_dma_desc_t *ndfc_dma_desc;     // physic addr
	struct _ndfc_dma_desc_t *ndfc_dma_desc_cpu; // virtual addr
};

struct _nand_controller_reg {
	volatile u32 *reg_ctl; /*0x0000 NDFC Control Register*/
	volatile u32 *reg_sta; /*0x0004 NDFC Status Register*/
	volatile u32 *reg_int; /*0x0008 NDFC Interrupt and DMA Enable Register*/
	volatile u32 *reg_timing_ctl; /*0x000C NDFC Timing Control Register*/
	volatile u32 *reg_timing_cfg; /*0x0010 NDFC Timing Configure Register*/
	volatile u32 *reg_addr_low;   /*0x0014 NDFC Address Low Word Register*/
	volatile u32 *reg_addr_high;  /*0x0018 NDFC Address High Word Register*/
	volatile u32
	    *reg_data_block_mask;       /*0x001C NDFC Data Block Mask Register*/
	volatile u32 *reg_ndfc_cnt;     /*0x0020 NDFC Data Block Mask Register*/
	volatile u32 *reg_cmd;		/*0x0024 NDFC Command IO Register*/
	volatile u32 *reg_read_cmd_set; /*0x0028 NDFC Command Set Register 0*/
	volatile u32 *reg_write_cmd_set; /*0x002C NDFC Command Set Register 1*/
	volatile u32 *reg_ecc_ctl;       /*0x0034 NDFC ECC Control Register*/
	volatile u32 *reg_ecc_sta;       /*0x0038 NDFC ECC Status Register*/
	volatile u32
	    *reg_data_pattern_sta; /*0x003C NDFC Data Pattern Status Register*/
	volatile u32 *reg_efr;     /*0x0040 NDFC Enhanced Featur Register*/
	volatile u32 *
	    reg_rdata_sta_ctl; /*0x0044 NDFC Read Data Status Control Register*/
	volatile u32
	    *reg_rdata_sta_0; /*0x0048 NDFC Read Data Status Register 0*/
	volatile u32
	    *reg_rdata_sta_1;       /*0x004C NDFC Read Data Status Register 1*/
	volatile u32 *reg_err_cnt0; /*0x0050 NDFC Error Counter Register 0*/
	volatile u32 *reg_err_cnt1; /*0x0054 NDFC Error Counter Register 1*/
	volatile u32 *reg_err_cnt2; /*0x0058 NDFC Error Counter Register 2*/
	volatile u32 *reg_err_cnt3; /*0x005C NDFC Error Counter Register 3*/
	volatile u32 *reg_err_cnt4; /*0x0060 NDFC Error Counter Register 4*/
	volatile u32 *reg_err_cnt5; /*0x0064 NDFC Error Counter Register 5*/
	volatile u32 *reg_err_cnt6; /*0x0068 NDFC Error Counter Register 6*/
	volatile u32 *reg_err_cnt7; /*0x006C NDFC Error Counter Register 7*/
	volatile u32
	    *reg_user_data_len_base; /*0x0070 NDFC User Data Length Register X*/
	volatile u32 *reg_user_data_base; /*0x0080 NDFC User Data Register X*/
	volatile u32 *reg_flash_sta;      /*0x0100 NDFC Flash Status Register*/
	volatile u32 *reg_cmd_repeat_cnt; /*0x0104 NDFC Command Repeat Counter
					     Register,重发命令最大次数*/
	volatile u32 *reg_cmd_repeat_interval; /*0x0108 NDFC Command Repeat
						  Interval
						  Register,重发命令时间间隔*/
	volatile u32 *reg_efnand_sta; /*0x0110 NDFC EFNAND STATUS Register*/
	volatile u32 *reg_spare_area; /*0x0114 NDFC Spare Aera Register*/
	volatile u32 *reg_pat_id;     /*0x0118 NDFC Pattern ID Register*/
	volatile u32
	    *reg_ddr2_spec_ctl; /*0x011C NDFC DDR2 Specific Control Register*/
	volatile u32
	    *reg_ndma_mode_ctl; /*0x0120 NDFC Normal DMA Mode Control Register*/
	volatile u32 *reg_valid_data_dma_cnt; /*0x012C NDFC Valid Data DMA
						 Counter Register*/
	volatile u32
	    *reg_data_dma_base; /*0x0130 NDFC Data DMA Address X Register */
	volatile u32 *reg_data_dma_size_2x; /*0x0170 NDFC Data DMA Size 2X and
					       Data DMA Size 2X+1 Register*/
	volatile u32 *reg_random_seed_x; /*0x0190 NDFC Random Seed X Register*/
	volatile u32
	    *reg_dma_cnt; /*0x0214 NDFC Normal DMA Byte Counter Register*/
	volatile u32 *reg_emce_ctl; /*0x0218 NDFC EMCE Control Register*/
	volatile u32
	    *reg_emce_iv_fac_cmp_val; /*0x021C NDFC EMCE IV_FAC Compare Value
					 Register*/
	volatile u32
	    *reg_emce_iv_cal_fac_base; /*0x0220 NDFC EMCE IV Calculate Factor
					  Register X*/
	volatile u32 *reg_io_data;     /*0x02A0 NDFC IO Data Register*/
	volatile u32 *reg_ndfc_ver;    /*0x02F0 NDFC Version Number Register*/
	volatile u32 *reg_ldpc_ctl;    /*0x02FC NDFC LDPC Control Register*/
	volatile u32 *reg_enc_ldpc_mode_set; /*0x0300 NDFC Encode LDPC Mode
						Setting Register*/
	volatile u32 *reg_cor_ldpc_mode_set; /*0x0304 NDFC Correct LDPC Mode
						Setting Register*/
	volatile u32 *reg_c0_llr_tbl_addr;   /*0x0308 NDFC C0 LLR Table
						11111-11100 Register*/
	volatile u32 *reg_c1_llr_tbl_addr;   /*0x0328 NDFC C1 LLR Table
						11111-11100 Register*/
	volatile u32 *reg_ram0_base;	 /*0x0400 NDFC Control Register*/
	volatile u32 *reg_ram1_base;	 /*0x0800 NDFC Control Register*/
	volatile u32 *reg_glb_cfg; /*0x0C00 NDFC Global Configure Register*/
	volatile u32
	    *reg_cmd_descr_base_addr; /*0x0C04 NDFC Command Descriptor Base
					 Address Register*/
	volatile u32
	    *reg_cmd_descr_sta; /*0x0C08 NDFC Command Descriptor Status
				   Register*/
	volatile u32 *reg_cmd_descr_intr; /*0x0C0C NDFC Command Descriptor
					     Interrupt Control Register*/
	volatile u32
	    *reg_csic_bist_ctl_reg; /*0x0C10 NDFC BIST Control Register*/
	volatile u32
	    *reg_bist_start_addr; /*0x0C14 NDFC BIST Start Address Register */
	volatile u32
	    *reg_bist_end_addr; /*0x0C18 NDFC BIST End Address Register*/
	volatile u32
	    *reg_bist_data_mask; /*0x0C1C NDFC BIST Data Mask Register*/
};

struct _nand_controller_reg_bak {
	u32 reg_ctl;	/*0x0000 NDFC Control Register*/
	u32 reg_sta;	/*0x0004 NDFC Status Register*/
	u32 reg_int;	/*0x0008 NDFC Interrupt and DMA Enable Register*/
	u32 reg_timing_ctl; /*0x000C NDFC Timing Control Register*/
	u32 reg_timing_cfg; /*0x0010 NDFC Timing Configure Register*/
	u32 reg_addr_low;   /*0x0014 NDFC Address Low Word Register*/
	u32 reg_addr_high;  /*0x0018 NDFC Address High Word Register*/
	u32 reg_data_block_mask;  /*0x001C NDFC Data Block Mask Register*/
	u32 reg_ndfc_cnt;	 /*0x0020 NDFC Data Block Mask Register*/
	u32 reg_cmd;		  /*0x0024 NDFC Command IO Register*/
	u32 reg_read_cmd_set;     /*0x0028 NDFC Command Set Register 0*/
	u32 reg_write_cmd_set;    /*0x002C NDFC Command Set Register 1*/
	u32 reg_ecc_ctl;	  /*0x0034 NDFC ECC Control Register*/
	u32 reg_ecc_sta;	  /*0x0038 NDFC ECC Status Register*/
	u32 reg_data_pattern_sta; /*0x003C NDFC Data Pattern Status Register*/
	u32 reg_efr;		  /*0x0040 NDFC Enhanced Featur Register*/
	u32 reg_rdata_sta_ctl; /*0x0044 NDFC Read Data Status Control Register*/
	u32 reg_rdata_sta_0;   /*0x0048 NDFC Read Data Status Register 0*/
	u32 reg_rdata_sta_1;   /*0x004C NDFC Read Data Status Register 1*/
	u32 reg_err_cnt0;      /*0x0050 NDFC Error Counter Register 0*/
	u32 reg_err_cnt1;      /*0x0054 NDFC Error Counter Register 0*/
	u32 reg_err_cnt2;      /*0x0058 NDFC Error Counter Register 0*/
	u32 reg_err_cnt3;      /*0x005C NDFC Error Counter Register 0*/
	u32 reg_err_cnt4;      /*0x0060 NDFC Error Counter Register 0*/
	u32 reg_err_cnt5;      /*0x0064 NDFC Error Counter Register 0*/
	u32 reg_err_cnt6;      /*0x0068 NDFC Error Counter Register 0*/
	u32 reg_err_cnt7;      /*0x006C NDFC Error Counter Register 0*/
	u32 reg_user_data_len_base; /*0x0070 NDFC User Data Length Register X*/
	u32 reg_user_data_base;     /*0x0080 NDFC User Data Register X*/
	u32 reg_flash_sta;	  /*0x0100 NDFC Flash Status Register*/
	u32 reg_cmd_repeat_cnt; /*0x0104 NDFC Command Repeat Counter Register*/
	u32 reg_cmd_repeat_interval; /*0x0108 NDFC Command Repeat Interval
					Register*/
	u32 reg_efnand_sta;	  /*0x0110 NDFC EFNAND STATUS Register*/
	u32 reg_spare_area;	  /*0x0114 NDFC Spare Aera Register*/
	u32 reg_pat_id;		     /*0x0118 NDFC Pattern ID Register*/
	u32 reg_ddr2_spec_ctl; /*0x011C NDFC DDR2 Specific Control Register*/
	u32 reg_ndma_mode_ctl; /*0x0120 NDFC Normal DMA Mode Control Register*/
	u32 reg_valid_data_dma_cnt; /*0x012C NDFC Valid Data DMA Counter
				       Register*/
	u32 reg_data_dma_base;      /*0x0130 NDFC Data DMA Address X Register */
	u32 reg_data_dma_size_2x;   /*0x0170 NDFC Data DMA Size 2X and Data DMA
				       Size 2X+1 Register*/
	u32 reg_random_seed_x;      /*0x0190 NDFC Random Seed X Register*/
	u32 reg_dma_cnt;  /*0x0214 NDFC Normal DMA Byte Counter Register*/
	u32 reg_emce_ctl; /*0x0218 NDFC EMCE Control Register*/
	u32 reg_emce_iv_fac_cmp_val;  /*0x021C NDFC EMCE IV_FAC Compare Value
					 Register*/
	u32 reg_emce_iv_cal_fac_base; /*0x0220 NDFC EMCE IV Calculate Factor
					 Register X*/
	u32 reg_io_data;	      /*0x02A0 NDFC IO Data Register*/
	u32 reg_ndfc_ver;	     /*0x02F0 NDFC Version Number Register*/
	u32 reg_ldpc_ctl;	     /*0x02FC NDFC LDPC Control Register*/
	u32 reg_enc_ldpc_mode_set;    /*0x0300 NDFC Encode LDPC Mode Setting
					 Register*/
	u32 reg_cor_ldpc_mode_set;    /*0x0304 NDFC Correct LDPC Mode Setting
					 Register*/
	u32 reg_c0_llr_tbl_addr;      /*0x0308 NDFC C0 LLR Table 11111-11100
					 Register*/
	u32 reg_c1_llr_tbl_addr;      /*0x0328 NDFC C1 LLR Table 11111-11100
					 Register*/
	u32 reg_ram0_base;	    /*0x0400 NDFC Control Register*/
	u32 reg_ram1_base;	    /*0x0800 NDFC Control Register*/
	u32 reg_glb_cfg;	      /*0x0C00 NDFC Global Configure Register*/
	u32 reg_cmd_descr_base_addr;  /*0x0C04 NDFC Command Descriptor Base
					 Address Register*/
	u32 reg_cmd_descr_sta;	/*0x0C08 NDFC Command Descriptor Status
					 Register*/
	u32 reg_cmd_descr_intr;       /*0x0C0C NDFC Command Descriptor Interrupt
					 Control Register*/
	u32 reg_csic_bist_ctl_reg;    /*0x0C10 NDFC BIST Control Register*/
	u32 reg_bist_start_addr; /*0x0C14 NDFC BIST Start Address Register */
	u32 reg_bist_end_addr;   /*0x0C18 NDFC BIST End Address Register*/
	u32 reg_bist_data_mask;  /*0x0C1C NDFC BIST Data Mask Register*/
};

// define the nand flash storage system information
struct _nand_controller_info {
#define MAX_ECC_BLK_CNT 32

	struct _nand_controller_info *next;
	u32 type;       // ndfc type
	u32 channel_id; // 0: channel 0; 1: channel 1;
	u32 chip_cnt;
	u32 chip_connect_info;
	u32 rb_connect_info;
	u32 max_ecc_level;
	u32 channel_sel; // 0:BCH; 1:LDPC

	struct _nctri_cmd_seq nctri_cmd_seq;

	u32 ce[8];
	u32 rb[8];
	u32 dma_type; // 0: general dma; 1: mbus dma;
	u32 dma_addr;

	u32 current_op_type;		 // 1: write operation; 0: others
	u32 write_wait_rb_before_cmd_io; // 1: before send cmd io; 0: after send
					 // cmd io;
	u32 write_wait_rb_mode; // 0: query mode; 1: interrupt mode
	u32 write_wait_dma_mode;
	u32 rb_ready_flag;
	u32 dma_ready_flag;
	u32 dma_channel;
	u32 nctri_flag;
	u32 ddr_timing_ctl[MAX_CHIP_PER_CHANNEL];
	u32 ddr_scan_blk_no[MAX_CHIP_PER_CHANNEL];

	u32 random_cmd2_send_flag; // special nand cmd for some nand in batch
				   // cmd
	u32 random_cmd2;     // special nand cmd for some nand in batch cmd
	u32 random_addr_num; // random col addr num in batch cmd

	struct _nand_controller_reg nreg;
	struct _nand_controller_reg_bak nreg_bak;

	struct _nand_chip_info *nci;
	struct _ndfc_dma_desc_t *ndfc_dma_desc;     // physic addr
	struct _ndfc_dma_desc_t *ndfc_dma_desc_cpu; // virtual addr
};

#define NDFC_READ_REG(n) (*((volatile u32 *)(n)))
#define NDFC_WRITE_REG(n, c) (*((volatile u32 *)(n)) = (c))

/*0x0000 : define bit use in NFC_CTL*/
#define NDFC_RB_SEL (0x3 << 3)  // 1 --> 0x3
#define NDFC_CE_SEL (0xf << 24) // 7 --> 0xf
#define NDFC_CE_CTL (1 << 6)
#define NDFC_CE_CTL1 (1 << 7)
#define NDFC_PAGE_SIZE (0xf << 8)

/*0x0004 : define bit use in NFC_ST*/
#define NDFC_RB_B2R (1 << 0)
#define NDFC_CMD_INT_FLAG (1 << 1)
#define NDFC_DMA_INT_FLAG (1 << 2)
#define NDFC_CMD_FIFO_STATUS (1 << 3)
#define NDFC_STA (1 << 4)
#define NDFC_RB_STATE0 (1 << 8) /*define NDFC_NATCH_INT_FLAG	(1 << 5)*/
#define NDFC_RB_STATE1 (1 << 9)
#define NDFC_RB_STATE2 (1 << 10)
#define NDFC_RB_STATE3 (1 << 11)
#define NDFC_RDATA_STA_1 (1 << 12)
#define NDFC_RDATA_STA_0 (1 << 13)

/*0x0008 : define bit use in NFC_INT*/
#define NDFC_B2R_INT_ENABLE (1 << 0)
#define NDFC_CMD_INT_ENABLE (1 << 1)
#define NDFC_DMA_INT_ENABLE (1 << 2)
#define NDFC_CMD_REPT_INTR_EN (1 << 5)

/*0x0024 : define bit use in NFC_CMD*/
#define NDFC_CMD_LOW_BYTE (0xff << 0)
#define NDFC_ADR_NUM_IN_PAGECMD (0x3 << 8)
#define NDFC_ADR_NUM (0x7 << 16)
#define NDFC_SEND_ADR (1 << 19)
#define NDFC_ACCESS_DIR (1 << 20)
#define NDFC_DATA_TRANS (1 << 21)
#define NDFC_SEND_CMD1 (1 << 22)
#define NDFC_WAIT_FLAG (1 << 23)
#define NDFC_SEND_CMD2 (1 << 24)
#define NDFC_SEQ (1 << 25)
#define NDFC_DATA_SWAP_METHOD (1 << 26)
#define NDFC_SEND_RAN_CMD2 (1 << 27)
#define NDFC_SEND_CMD3 (1 << 28)
#define NDFC_SEND_CMD4 (1 << 29)
#define NDFC_CMD_TYPE (3 << 30)

/*0x0028 : define bit use in NFC_RCMD_SET*/
#define NDFC_READ_CMD (0xff << 0)
#define NDFC_RANDOM_READ_CMD0 (0xff << 8)
#define NDFC_RANDOM_READ_CMD1 (0xff << 16)

/*0x002C : define bit use in NFC_WCMD_SET*/
#define NDFC_PROGRAM_CMD (0xff << 0)
#define NDFC_RANDOM_WRITE_CMD (0xff << 8)
#define NDFC_READ_CMD0 (0xff << 16)
#define NDFC_READ_CMD1 (0xff << 24)

/*0x0034 : define bit use in NFC_ECC_CTL*/
#define NDFC_ECC_EN (1 << 0)
#define NDFC_ECC_PIPELINE (1 << 3)
#define NDFC_ECC_EXCEPTION (1 << 4)
#define NDFC_ECC_BLOCK_SIZE (1 << 7)
#define NDFC_RANDOM_EN (1 << 5)
#define NDFC_RANDOM_DIRECTION (1 << 6)
#define NDFC_ECC_MODE (0xff << 8)
#define NDFC_RANDOM_SEED        (0x7fff << 16)

/*0x0040 : define bit use in NDFC_EFR*/
#define NDFC_ECC_DEBUG (0x3f << 0)
#define NDFC_WP_CTRL (1 << 8)
#define NDFC_DUMMY_BYTE_CNT (0xff << 16)
#define NDFC_DUMMY_BYTE_EN (1 << 24)

/*0x0044 : define bit use in NDFC_RDATA_STA_CTL*/
#define NDFC_RDATA_STA_EN (0x1 << 24)
#define NDFC_RDATA_STA_TH (0x7FF << 0)

/*0x0070 : define bit use in NDFC_USER_DATA_LEN_X*/
#define NDFC_DATA_LEN_DATA_BLOCK (0x4)

/*0x0100 : define bit use in reg_flash_sta*/
#define CMD_REPT_EN (1 << 24)
#define BITMAP_B (0xff << 16)
#define BITMAP_A (0xff << 8)
#define SYNC_CMD_REPT_READ_STA (0xff << 0)

/*0x0104 : define bit use in reg_cmd_repeat_cnt*/
#define CMD_REPT_CNT (0xffff << 0)

/*0x0108 : define bit use in reg_cmd_repeat_interval*/
#define CMD_REPT_INVL (0xffff << 0)

/*0x02FC : define bit use in reg_ldpc_ctl*/
#define CH1_HB_LLR_VAL (0xf << 28)
#define CH1_SOFT_BIT_DEC_EN (1 << 5)
#define CH1_SRAM_MAIN_OR_SPARE_AREA_IND (1 << 4)
#define CH1_SRAM_IND (0x7 << 1)
#define CH1_SOFT_BIT_IND (1 << 0)

/*0x0300 : define bit use in reg_enc_ldpc_mode_set*/
#define ENCODE (1 << 31)
#define DECODE (0 << 31)
#define LDPC_MODE (0x7 << 28)
#define BOOT0_LDPC_MODE ((0 & 0x7) << 28)
#define FW_EXTEND_ENCODE (1 << 27)
#define BOOT0_FW_EXTEND_ENCODE ((1 & 0x1) << 27)

/*0x0304 : define bit use in reg_cor_ldpc_mode_set*/
#define C0_LDPC_MODE (0x7 << 29)
#define BOOT0_C0_LDPC_MODE ((0 & 0x7) << 29)
#define C0_DECODE_MODE (0x7 << 26)
#define C0_SCALE_MODE (0x3 << 24)
#define C1_LDPC_MODE (0x7 << 21)
#define BOOT0_C1_LDPC_MODE ((0 & 0x7) << 21)
#define C1_DECODE_MODE (0x7 << 18)
#define C1_SCALE_MODE (0x3 << 16)
#define FW_EXTEND_DECODE (1 << 15)
#define BOOT0_FW_EXTEND_DECODE ((1 & 0x1) << 15)

/*0x0C00 : define bit use in reg_glb_cfg*/
#define CMD_DESCR_FINISH_NUM (0xffff << 16)
#define LDPC_OUTPUT_DISORDER_CTL (1 << 8)
#define CMD_DESCR_ECC_ERROR_HANDLE_EN (1 << 7)
#define CMD_DESCR_CMD_REPT_TIMEOUT_HANDLE_EN (1 << 6)
#define CMD_DESCR_FLASH_STA_HANDLE_EN (1 << 5)
#define REG_ACCESS_METHOD (1 << 4)
#define CMD_DESCR_CTL_BIT (1 << 3)
#define NDFC_CHANNEL_SEL (1 << 2)
#define NDFC_RESET (1 << 1)
#define NDFC_EN (1 << 0)

/*0x0C08 : define bit use in reg_cmd_descr_sta*/
#define CMD_DESCR_ECC_ERR_STA (1 << 3)
#define CMD_DESCR_REPT_MODE_TIMEOUT_STA (1 << 2)
#define CMD_DESCR_FLASH_STA_HANDLE_STA (1 << 1)
#define CMD_DESCR_STA (1 << 0)

/*0x0C0C : define bit use in reg_cmd_descr_intr*/
#define CMD_DESCR_INTR_EN (1 << 0)

/*0x0C10  : define bit use in reg_csic_bist_ctl_reg*/
#define BIST_ERR_STA (1 << 15)
#define BIST_ERR_PAT (0x7 << 12)
#define BIST_ERR_CYC (3 << 10)
#define BIST_STOP (1 << 9)
#define BIST_BUSY (1 << 8)
#define BIST_REG_SEL (0x7 << 5)
#define BIST_ADDR_MODE_SEL (0x1 << 4)
#define BIST_WDATA_PAT (0x7 << 1)
#define BIST_EN (1 << 0)

/*0x0C1C  : define bit use in reg_bist_data_mask*/
#define BIST_DATA_MASK (0xffffffff << 0)

/*cmd flag bit*/
#define NDFC_PAGE_MODE 0x1
#define NDFC_NORMAL_MODE 0x0

#define NDFC_DATA_FETCH 0x1
#define NDFC_NO_DATA_FETCH 0x0
#define NDFC_MAIN_DATA_FETCH 0x1
#define NDFC_SPARE_DATA_FETCH 0x0
#define NDFC_WAIT_RB 0x1
#define NDFC_NO_WAIT_RB 0x0
#define NDFC_IGNORE 0x0

#define NDFC_INT_RB 0
#define NDFC_INT_CMD 1
#define NDFC_INT_DMA 2
#define NDFC_INT_BATCh 5

//=============================================================================
// define the mask for the nand flash operation status
#define NAND_OPERATE_FAIL (1 << 0)  // nand flash program/erase failed mask
#define NAND_CACHE_READY (1 << 5)   // nand flash cache program true ready mask
#define NAND_STATUS_READY (1 << 6)  // nand flash ready/busy status mask
#define NAND_WRITE_PROTECT (1 << 7) // nand flash write protected mask

//=============================================================================
#define CMD_RESET 0xFF
#define CMD_SYNC_RESET 0xFC
#define CMD_READ_STA 0x70
#define CMD_READ_ID 0x90
#define CMD_SET_FEATURE 0xEF
#define CMD_GET_FEATURE 0xEE
#define CMD_READ_PAGE_CMD1 0x00
#define CMD_READ_PAGE_CMD2 0x30
#define CMD_CHANGE_READ_ADDR_CMD1 0x05
#define CMD_CHANGE_READ_ADDR_CMD2 0xE0
#define CMD_WRITE_PAGE_CMD1 0x80
#define CMD_WRITE_PAGE_CMD2 0x10
#define CMD_CHANGE_WRITE_ADDR_CMD 0x85
#define CMD_ERASE_CMD1 0x60
#define CMD_ERASE_CMD2 0xD0

//==============================================================================
//  define some constant variable for the nand flash driver used
//==============================================================================

// define the mask for the nand flash optional operation
//#define NAND_CACHE_READ          (1<<0)              //nand flash support
//cache read operation
//#define NAND_CACHE_PROGRAM       (1<<1)              //nand flash support page
//cache program operation
//#define NAND_MULTI_READ          (1<<2)              //nand flash support
//multi-plane page read operation
//#define NAND_MULTI_PROGRAM       (1<<3)              //nand flash support
//multi-plane page program operation
//#define NAND_PAGE_COPYBACK       (1<<4)              //nand flash support page
//copy-back command mode operation
//#define NAND_INT_INTERLEAVE      (1<<5)              //nand flash support
//internal inter-leave operation, it based multi-bank
//#define NAND_EXT_INTERLEAVE      (1<<6)              //nand flash support
//external inter-leave operation, it based multi-chip
//#define NAND_RANDOM		         (1<<7)			     //nand
//flash support RANDOMIZER
//#define NAND_READ_RETRY	         (1<<8)			     //nand
//falsh support READ RETRY
//#define NAND_READ_UNIQUE_ID	     (1<<9)			     //nand
//falsh support READ UNIQUE_ID
//#define NAND_PAGE_ADR_NO_SKIP	 (1<<10)			    //nand falsh
//page adr no skip is requiered
//#define NAND_DIE_SKIP            (1<<11)             //nand flash die adr skip
////#define NAND_LOG_BLOCK_MANAGE   (1<<12)             //nand flash log block
///type management
//#define NAND_FORCE_WRITE_SYNC    (1<<13)             //nand flash need check
//status after write or erase
//#define NAND_LOG_BLOCK_LSB_TYPE (0xff<<16)          //nand flash log block lsb
//page type
//#define NAND_LSB_PAGE_TYPE		 (0xf<<16)			//nand flash
//lsb page type....

//#define NAND_MAX_BLK_ERASE_CNT	 (1<<20)			//nand flash
//support the maximum block erase cnt
//#define NAND_READ_RECLAIM		 (1<<21)			//nand flash
//support to read reclaim Operation
//
//#define NAND_TIMING_MODE    	 (1<<24)            //nand flash support to
//change timing mode according to ONFI 3.0
//#define NAND_DDR2_CFG			 (1<<25)            //nand flash
//support to set ddr2 specific feature according to ONFI 3.0 or Toggle DDR2
//#define NAND_IO_DRIVER_STRENGTH  (1<<26)            //nand flash support to
//set io driver strength according to ONFI 2.x/3.0 or Toggle DDR1/DDR2
//#define NAND_VENDOR_SPECIFIC_CFG (1<<27)            //nand flash support to
//set vendor's specific configuration
//#define NAND_ONFI_SYNC_RESET_OP	 (1<<28)            //nand flash support
//onfi's sync reset
//#define NAND_TOGGLE_ONLY 		 (1<<29)            //nand flash support
//toggle interface only, and do not support switch between legacy and toggle

#endif //_NAND_CONTROLLER_H
