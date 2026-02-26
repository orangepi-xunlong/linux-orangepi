/*
 * ILITEK Touch IC driver
 *
 * Copyright (C) 2011 ILI Technology Corporation.
 *
 * Author: Dicky Chiang <dicky_chiang@ilitek.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */
#ifndef __ILITEK_V3_H
#define __ILITEK_V3_H

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/input/mt.h>
#include <linux/list.h>
#include <linux/platform_device.h>
#include <linux/kobject.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/version.h>
#include <linux/regulator/consumer.h>
#include <linux/power_supply.h>
#include <linux/fs.h>
// #include <linux/earlysuspend.h>
#ifdef CONFIG_COMPAT
#include <linux/compat.h>
#endif
#include <linux/uaccess.h>

#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/proc_fs.h>
#include <linux/string.h>
#include <linux/ctype.h>

#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/wait.h>
#include <linux/time.h>

#include <linux/namei.h>
#include <linux/vmalloc.h>
#include <linux/timer.h>
#include <linux/jiffies.h>
#include <linux/module.h>
#include <linux/dma-mapping.h>

#include <linux/gpio.h>
#include <linux/spi/spi.h>
#include <linux/rtc.h>
#include <linux/syscalls.h>
#include <linux/security.h>
#include <linux/mount.h>
#include <linux/firmware.h>

#ifdef CONFIG_OF
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/of_irq.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#endif

#ifdef CONFIG_FB
#ifdef CONFIG_DRM
#include <drm/drm_panel.h>
#endif
#include <linux/notifier.h>
#include <linux/fb.h>
#endif

#include <linux/notifier.h>
#include <linux/fb.h>

#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif

#ifdef CONFIG_DRM_MSM
#include <linux/msm_drm_notify.h>
#endif

/*#ifdef CONFIG_MTK_SPI
#include "mt_spi.h"
#include "sync_write.h"
#endif*/

#define DRIVER_VERSION "3.0.9.0.230110"

#define ENABLE 1
#define DISABLE 0
#define K (1024)
#define M (K * K)
#define START 1
#define ON 1
#define ILI_WRITE 1
#define ILI_READ 0
#define END 0
#define OFF 0
#define NONE -1

/* Options */
#define TDDI_INTERFACE BUS_I2C /* BUS_I2C(0x18) or BUS_SPI(0x1C) */
#define VDD_VOLTAGE 3300000
#define VCC_VOLTAGE 3300000
#define SPI_CLK 9 /* follow by clk list */
#define SPI_RETRY 5
#define IRQ_GPIO_NUM 66
#define TR_BUF_SIZE (6 * K)	   /* Buffer size of touch report */
#define TR_BUF_LIST_SIZE (256) /* Buffer size of touch report for debug data */
#define SPI_TX_BUF_SIZE (6 * K)
#define SPI_RX_BUF_SIZE (6 * K)
#define WQ_ESD_DELAY 4000
#define WQ_BAT_DELAY 2000
#define AP_INT_TIMEOUT 600	/*600ms*/
#define MP_INT_TIMEOUT 5000 /*5s*/
#define MT_B_TYPE ENABLE
#define TDDI_RST_BIND DISABLE
#define MT_PRESSURE DISABLE
#define ENABLE_WQ_ESD DISABLE
#define ENABLE_WQ_BAT DISABLE
#define ENABLE_GESTURE DISABLE
#define REGULATOR_POWER ENABLE // ENABLE
#define TP_SUSPEND_PRIO ENABLE
#define BOOT_FW_UPDATE ENABLE
#define RESUME_BY_DDI ENABLE
#define MP_INT_LEVEL DISABLE
#define PLL_CLK_WAKEUP_TP_RESUME DISABLE
#define CHARGER_NOTIFIER_CALLBACK DISABLE
#define ENABLE_EDGE_PALM_PARA DISABLE
#define MULTI_REPORT_RATE DISABLE
#define ENGINEER_FLOW ENABLE
#define DMESG_SEQ_FILE ENABLE
#define GENERIC_KERNEL_IMAGE ENABLE /*follow gki */
#define SUSPEND_RESUME_SUPPORT ENABLE

#define BOOT_FW_UPDATE_MODE BOOT_FW_VER_DIFF
#define BOOT_FW_VER_DIFF 0
#define BOOT_FW_VER_UPGRADE 1
#define BOOT_FW_VER_DOWNGRADE 2

/*Compress mode options*/
#define ENABLE_COMPRESS_MODE DISABLE
#define COMPRESS_PACKET_LEN 4093

/*GET_ALL_INFO options*/
#define ENABLE_GET_ALL_INFO DISABLE

/*Pen mode options*/
#define ENABLE_PEN_MODE DISABLE
#define ENABLE_CASCADE DISABLE

#define PROXIMITY_RESET_CMD 0
#define PROXIMITY_NO_RESET_CMD 1

#define PROXIMITY_IGNORE_STATE 0
#define PROXIMITY_NEAR_STATE 1
#define PROXIMITY_FAR_STATE 2

/*if current interface is spi, must to hostdownload */
#if (TDDI_INTERFACE == BUS_SPI)
#define HOST_DOWN_LOAD ENABLE
#else
#define HOST_DOWN_LOAD DISABLE
#endif

/* Plaform compatibility */
#define CONFIG_PLAT_SPRD DISABLE
#define I2C_DMA_TRANSFER DISABLE
#if (ENABLE_CASCADE == ENABLE)
#define SPI_DMA_TRANSFER_SPLIT ENABLE
#else
#define SPI_DMA_TRANSFER_SPLIT DISABLE
#endif
#define SPI_DMA_TRANSFER_SPLIT_OLD DISABLE
#define SPRD_SYSFS_SUSPEND_RESUME DISABLE

/* Path */
#define DEBUG_DATA_FILE_SIZE (10 * K)
#define DEBUG_DATA_FILE_PATH "/sdcard/ILITEK_log.csv"
#define CSV_LCM_ON_PATH "/sdcard/ilitek_mp_lcm_on_log"
#define CSV_LCM_OFF_PATH "/sdcard/ilitek_mp_lcm_off_log"
#define CSV_LCM_DEF_PATH "/data/local/tmp"
#define POWER_STATUS_PATH "/sys/class/power_supply/battery/status"
#define DUMP_FLASH_PATH "/sdcard/flash_dump"
#define DUMP_IRAM_PATH "/sdcard/iram_dump"

/* Debug messages */
#define DEBUG_NONE 0
#define DEBUG_ALL 1
#define DEBUG_OUTPUT DEBUG_NONE

#define ILI_INFO(fmt, arg...)                                         \
	({                                                                \
		pr_info("ILITEK: (%s, %d): " fmt, __func__, __LINE__, ##arg); \
	})

#define ILI_ERR(fmt, arg...)                                         \
	({                                                               \
		pr_err("ILITEK: (%s, %d): " fmt, __func__, __LINE__, ##arg); \
	})

extern bool debug_en;
#define ILI_DBG(fmt, arg...)                                              \
	do                                                                    \
	{                                                                     \
		if (debug_en)                                                     \
			pr_info("ILITEK: (%s, %d): " fmt, __func__, __LINE__, ##arg); \
	} while (0)

#define ERR_ALLOC_MEM(X) ((IS_ERR(X) || X == NULL) ? 1 : 0)
#define DO_SPI_RECOVER -2
#define DO_I2C_RECOVER -3

#define PEN_INPUT_DEVICE "ILITEK_PEN"

#define SINGLE_TOUCH 0
#define MULTI_TOUCH 1
#define SPI_CLK_CASCADE_TABLE_NUM 3

#define NEGITIVE_FLAG BIT(7)
#define TIPSWITCH_HOVER_BIT (BIT(0) | BIT(4))
#define MAX_PRESSURE (BIT(0) | BIT(1) | BIT(2) | BIT(3) | BIT(4) | BIT(5) | BIT(6) | BIT(7) | BIT(8) | BIT(9) | BIT(10) | BIT(11))

/* Path */
#define TXT_FILE_SIZE (1 * K)
#define PEN_ID_NODE_TEST_PATH "/sdcard/Get_Wacom_PenID.txt"

enum CASCADE_MODE
{
	MASTER = 0,
	SLAVE,
	BOTH,
};

enum PROXIMITY_MODE
{
	PROXIMITY_NULL = 0,
	PROXIMITY_SUSPEND_RESUME,
	PROXIMITY_BACKLIGHT
};

enum TP_SPI_CLK_LIST
{
	TP_SPI_CLK_1M = 1000000,
	TP_SPI_CLK_2M = 2000000,
	TP_SPI_CLK_3M = 3000000,
	TP_SPI_CLK_4M = 4000000,
	TP_SPI_CLK_5M = 5000000,
	TP_SPI_CLK_6M = 6000000,
	TP_SPI_CLK_7M = 7000000,
	TP_SPI_CLK_8M = 8000000,
	TP_SPI_CLK_9M = 9000000,
	TP_SPI_CLK_10M = 10000000,
	TP_SPI_CLK_11M = 11000000,
	TP_SPI_CLK_12M = 12000000,
	TP_SPI_CLK_13M = 13000000,
	TP_SPI_CLK_14M = 14000000,
	TP_SPI_CLK_15M = 15000000,
};

enum TP_PLAT_TYPE
{
	TP_PLAT_COMMON
};

enum TP_RST_METHOD
{
	TP_IC_WHOLE_RST_WITH_FLASH = 0,
	TP_IC_WHOLE_RST_WITHOUT_FLASH,
	TP_IC_CODE_RST,
	TP_HW_RST_ONLY,
};

enum TP_FW_UPGRADE_TYPE
{
	UPGRADE_FLASH = 0,
	UPGRADE_IRAM
};

enum TP_FW_UPGRADE_STATUS
{
	FW_STAT_INIT = 0,
	FW_UPDATING = 90,
	FW_UPDATE_PASS = 100,
	FW_UPDATE_FAIL = -1
};

enum TP_FW_OPEN_METHOD
{
	REQUEST_FIRMWARE = 0,
	FILP_OPEN
};

enum TP_SLEEP_STATUS
{
	TP_SUSPEND = 0,
	TP_DEEP_SLEEP = 1,
	TP_RESUME = 2
};

enum TP_PROXIMITY_STATUS
{
	DDI_POWER_OFF = 0,
	DDI_POWER_ON = 1,
	WAKE_UP_GESTURE_RECOVERY = 2,
	WAKE_UP_SWITCH_GESTURE_MODE = 3
};

enum TP_SLEEP_CTRL
{
	SLEEP_IN = 0x0,
	SLEEP_OUT = 0x1,
	DEEP_SLEEP_IN = 0x3
};

enum TP_FW_BLOCK_NUM
{
	AP = 1,
	DATA = 2,
	TUNING = 3,
	GESTURE = 4,
	MP = 5,
	DDI = 6,
	TAG = 7,
	PARA_BACKUP = 8,
	RESERVE_BLOCK3 = 9,
	PEN = 10,
	RESERVE_BLOCK5 = 11,
	RESERVE_BLOCK6 = 12,
	RESERVE_BLOCK7 = 13,
	RESERVE_BLOCK8 = 14,
	RESERVE_BLOCK9 = 15,
	RESERVE_BLOCK10 = 16,
};

enum TP_FW_BLOCK_TAG
{
	BLOCK_TAG_AF = 0xAF,
	BLOCK_TAG_B0 = 0xB0
};

enum TP_WQ_TYPE
{
	WQ_ESD = 0,
	WQ_BAT,
};

enum TP_RECORD_DATA
{
	DISABLE_RECORD = 0,
	ENABLE_RECORD,
	DATA_RECORD
};

enum TP_DATA_FORMAT
{
	DATA_FORMAT_DEMO = 0,
	DATA_FORMAT_DEBUG,
	DATA_FORMAT_DEMO_DEBUG_INFO,
	DATA_FORMAT_GESTURE_SPECIAL_DEMO,
	DATA_FORMAT_GESTURE_INFO,
	DATA_FORMAT_GESTURE_NORMAL,
	DATA_FORMAT_GESTURE_DEMO,
	DATA_FORMAT_GESTURE_DEBUG,
	DATA_FORMAT_DEBUG_LITE_ROI,
	DATA_FORMAT_DEBUG_LITE_WINDOW,
	DATA_FORMAT_DEBUG_LITE_AREA,
	DATA_FORMAT_DEBUG_LITE_PEN,
};

enum NODE_MODE_SWITCH
{
	AP_MODE = 0,
	TEST_MODE,
	DEBUG_MODE,
	DEBUG_LITE_ROI,
	DEBUG_LITE_WINDOW,
	DEBUG_LITE_AREA,
	DEBUG_PEN_ONLY_MODE,
	DEBUG_HAND_ONLY_MODE,
	DEBUG_LITE_PEN,
};

enum TP_MODEL
{
	MODEL_DEF = 0,
	MODEL_CSOT,
	MODEL_AUO,
	MODEL_BOE,
	MODEL_INX,
	MODEL_DJ,
	MODEL_TXD,
	MODEL_TM
};

enum TP_ERR_CODE
{
	EMP_CMD = 100,
	EMP_PROTOCOL,
	EMP_FILE,
	EMP_INI,
	EMP_TIMING_INFO,
	EMP_INVAL,
	EMP_PARSE,
	EMP_NOMEM,
	EMP_GET_CDC,
	EMP_INT,
	EMP_CHECK_BUY,
	EMP_MODE,
	EMP_FW_PROC,
	EMP_FORMUL_NULL,
	EMP_PARA_NULL,
	EFW_CONVERT_FILE,
	EFW_ICE_MODE,
	EFW_CRC,
	EFW_REST,
	EFW_ERASE,
	EFW_PROGRAM,
	EFW_INTERFACE,
};

enum TP_IC_TYPE
{
	ILI_A = 0x0A,
	ILI_B,
	ILI_C,
	ILI_D,
	ILI_E,
	ILI_F,
	ILI_G,
	ILI_H,
	ILI_I,
	ILI_J,
	ILI_K,
	ILI_L,
	ILI_M,
	ILI_N,
	ILI_O,
	ILI_P,
	ILI_Q,
	ILI_R,
	ILI_S,
	ILI_T,
	ILI_U,
	ILI_V,
	ILI_W,
	ILI_X,
	ILI_Y,
	ILI_Z,
};

enum PROXIMITY_REPORT_STATE
{
	PROXIMITY_NONE_STATE_0 = 0,
	PROXIMITY_NEAR_STATE_1 = 1,
	PROXIMITY_NEAR_STATE_2 = 2,
	PROXIMITY_NEAR_STATE_3 = 3,
	PROXIMITY_NONE_STATE_4 = 4,
	PROXIMITY_FAR_STATE_5 = 5,
};

#if MULTI_REPORT_RATE
enum MULTI_REPORT_RATE_TYPE
{
	DDI120_TP120 = 0,
	DDI60_TP120 = 1,
	DDI90_TP180 = 2,
	DDI120_TP240 = 3,
	DDI144_TP144 = 4,
	DDI90_TP120 = 5,
};
#endif

typedef enum cmd_types
{
	CMD_DISABLE = 0x00,
	CMD_ENABLE = 0x01,
	CMD_STATUS = 0x02,
	CMD_ROI_DATA = 0x03,
} cmd_types;

struct gesture_symbol
{
	u8 double_tap : 1;
	u8 alphabet_line_2_top : 1;
	u8 alphabet_line_2_bottom : 1;
	u8 alphabet_line_2_left : 1;
	u8 alphabet_line_2_right : 1;
	u8 alphabet_m : 1;
	u8 alphabet_w : 1;
	u8 alphabet_c : 1;
	u8 alphabet_E : 1;
	u8 alphabet_V : 1;
	u8 alphabet_O : 1;
	u8 alphabet_S : 1;
	u8 alphabet_Z : 1;
	u8 alphabet_V_down : 1;
	u8 alphabet_V_left : 1;
	u8 alphabet_V_right : 1;
	u8 alphabet_two_line_2_bottom : 1;
	u8 alphabet_F : 1;
	u8 alphabet_AT : 1;
	u8 reserve0 : 5;
};

struct report_info_block
{
	u8 nReportByPixel : 1;
	u8 nIsHostDownload : 1;
	u8 nIsSPIICE : 1;
	u8 nIsSPISLAVE : 1;
	u8 nIsI2C : 1;
	u8 nReserved00 : 3;
	u8 nReportResolutionMode : 3;
	u8 nCustomerType : 5;
	u8 nReserved02 : 8;
	u8 nReserved03 : 8;
};

struct file_buffer
{
	char *ptr;
	char fname[128];
	int32_t wlen;
	int32_t flen;
	int32_t max_size;
};

struct cascade_info_block
{
	u8 nDisable : 1;
	u8 nNum : 3;
	u8 nReserved00 : 4;
	u8 nReserved01 : 8;
	u8 nReserved02 : 8;
	u8 nReserved03 : 8;
};

struct pen_info_block
{
	u8 nPxRaw : 8;
	u8 nPyRaw : 8;
	u8 nPxVa : 8;
	u8 nPyVa : 8;
	u8 nReserved00 : 8;
	u8 nReserved01 : 8;
	u8 nReserved02 : 8;
	u8 nReserved03 : 8;
};

typedef enum pen_data_modes
{
	HAND_PEN_DEMO_MODE = 0,
	HAND_PEN_SIGNAL_MODE,
	HAND_PEN_RAW_DATA_MODE,
	PEN_ONLY_SIGNAL_MODE,
	PEN_ONLY_RAW_DATA_MODE,
	HAND_ONLY_SIGNAL_MODE,
	HAND_ONLY_RAW_DATA_MODE,
} pen_data_modes;

typedef enum report_types
{
	P5_X_HAND_PEN_TYPE = 0,	 /* 0x39, FigType 0-2 bits, CustomType 3-5, PenType 6-7 */
	P5_X_ONLY_PEN_TYPE = 1,	 /* 0x3F */
	P5_X_ONLY_HAND_TYPE = 2, /* 0xF9 */

} report_types;

struct ilitek_pen_info
{
	u16 x;
	u16 y;
	u16 pressure;
	s32 tilt_x;
	s32 tilt_y;
	u8 pen_id[8];
	u8 distance_cnt;
	bool TipSwitch;
	bool BarelSwitch;
	bool Eraser;
	bool InRange;
	bool active;
	bool finger_touch;
	report_types report_type;
	pen_data_modes pen_data_mode;
};

#define TDDI_I2C_ADDR 0x41
#define TDDI_I2C_SLAVE_ADDR 0x4f
#define TDDI_DEV_ID "ILITEK_TDDI"

/* define the width and heigth of a screen. */
#define TOUCH_SCREEN_X_MIN 0
#define TOUCH_SCREEN_Y_MIN 0
#define TOUCH_SCREEN_X_MAX 720
#define TOUCH_SCREEN_Y_MAX 1440
#define MAX_TOUCH_NUM 10
#define MAX_PEN_NUM 1
#define PEN_INDEX 10
#define ILITEK_KNUCKLE_ROI_FINGERS 2

/* define the range on space, don't change */
#define TPD_HEIGHT 2048
#define TPD_WIDTH 2048
#define TPD_HIGH_RESOLUTION_HEIGHT 8192
#define TPD_HIGH_RESOLUTION_WIDTH 8192

/* Firmware upgrade */
#define CORE_VER_1410 0x01040100
#define CORE_VER_1420 0x01040200
#define CORE_VER_1430 0x01040300
#define CORE_VER_1460 0x01040600
#define CORE_VER_1470 0x01040700
#define CORE_VER_1600 0x01060000
#define CORE_VER_1700 0x01070000
#define MAX_HEX_FILE_SIZE (256 * K)
#define ILI_FILE_HEADER 256
#define DLM_START_ADDRESS 0x20610
#define DLM_HEX_ADDRESS 0x10000
#define MP_HEX_ADDRESS 0x13000
#define DDI_RSV_BK_ST_ADDR 0x1E000
#define DDI_RSV_BK_END_ADDR 0x1FFFF
#define DDI_RSV_BK_SIZE (1 * K)
#define RSV_BK_ST_ADDR 0x1E000
#define RSV_BK_END_ADDR 0x1E3FF
#define FW_VER_ADDR 0xFFE0
#define FW_BLOCK_INFO_NUM 17
#if SPI_DMA_TRANSFER_SPLIT
#define SPI_UPGRADE_LEN 2048
#else
#define SPI_UPGRADE_LEN 0
#endif
#define SPI_BUF_SIZE MAX_HEX_FILE_SIZE
#define INFO_HEX_ST_ADDR 0x4F
#define INFO_MP_HEX_ADDR 0x1F
#define INFO_PEN_ST_ADDR 0x67
#define INFO_CASCADE_ST_ADDR 0x6B

/* DMA Control Registers */
#define DMA_BASED_ADDR 0x72000
#define DMA48_ADDR (DMA_BASED_ADDR + 0xC0)
#define DMA48_reg_dma_ch0_busy_flag DMA48_ADDR
#define DMA48_reserved_0 0xFFFE
#define DMA48_reg_dma_ch0_trigger_sel (BIT(16) | BIT(17) | BIT(18) | BIT(19))
#define DMA48_reserved_1 (BIT(20) | BIT(21) | BIT(22) | BIT(23))
#define DMA48_reg_dma_ch0_start_set BIT(24)
#define DMA48_reg_dma_ch0_start_clear BIT(25)
#define DMA48_reg_dma_ch0_trigger_src_mask BIT(26)
#define DMA48_reserved_2 BIT(27)

#define DMA49_ADDR (DMA_BASED_ADDR + 0xC4)
#define DMA49_reg_dma_ch0_src1_addr DMA49_ADDR
#define DMA49_reserved_0 BIT(20)

#define DMA50_ADDR (DMA_BASED_ADDR + 0xC8)
#define DMA50_reg_dma_ch0_src1_step_inc DMA50_ADDR
#define DMA50_reserved_0 (DMA50_ADDR + 0x01)
#define DMA50_reg_dma_ch0_src1_format (BIT(24) | BIT(25))
#define DMA50_reserved_1 (BIT(26) | BIT(27) | BIT(28) | BIT(29) | BIT(30))
#define DMA50_reg_dma_ch0_src1_en BIT(31)

#define DMA52_ADDR (DMA_BASED_ADDR + 0xD0)
#define DMA52_reg_dma_ch0_src2_step_inc DMA52_ADDR
#define DMA52_reserved_0 (DMA52_ADDR + 0x01)
#define DMA52_reg_dma_ch0_src2_format (BIT(24) | BIT(25))
#define DMA52_reserved_1 (BIT(26) | BIT(27) | BIT(28) | BIT(29) | BIT(30))
#define DMA52_reg_dma_ch0_src2_en BIT(31)

#define DMA53_ADDR (DMA_BASED_ADDR + 0xD4)
#define DMA53_reg_dma_ch0_dest_addr DMA53_ADDR
#define DMA53_reserved_0 BIT(20)

#define DMA54_ADDR (DMA_BASED_ADDR + 0xD8)
#define DMA54_reg_dma_ch0_dest_step_inc DMA54_ADDR
#define DMA54_reserved_0 (DMA54_ADDR + 0x01)
#define DMA54_reg_dma_ch0_dest_format (BIT(24) | BIT(25))
#define DMA54_reserved_1 (BIT(26) | BIT(27) | BIT(28) | BIT(29) | BIT(30))
#define DMA54_reg_dma_ch0_dest_en BIT(31)

#define DMA55_ADDR (DMA_BASED_ADDR + 0xDC)
#define DMA55_reg_dma_ch0_trafer_counts DMA55_ADDR
#define DMA55_reserved_0 (BIT(17) | BIT(18) | BIT(19) | BIT(20) | BIT(21) | BIT(22) | BIT(23))
#define DMA55_reg_dma_ch0_trafer_mode (BIT(24) | BIT(25) | BIT(26) | BIT(27))
#define DMA55_reserved_1 (BIT(28) | BIT(29) | BIT(30) | BIT(31))

/* INT Function Registers */
#define INTR_BASED_ADDR 0x48000
#define INTR1_ADDR (INTR_BASED_ADDR + 0x4)
#define INTR1_FLASH_INT_FLAG (INTR_BASED_ADDR + 0x7)
#define INTR1_reg_uart_tx_int_flag INTR1_ADDR
#define INTR1_reserved_0 (BIT(1) | BIT(2) | BIT(3) | BIT(4) | BIT(5) | BIT(6) | BIT(7))
#define INTR1_reg_wdt_alarm_int_flag BIT(8)
#define INTR1_reserved_1 (BIT(9) | BIT(10) | BIT(11) | BIT(12) | BIT(13) | BIT(14) | BIT(15))
#define INTR1_reg_dma_ch1_int_flag BIT(16)
#define INTR1_reg_dma_ch0_int_flag BIT(17)
#define INTR1_reg_dma_frame_done_int_flag BIT(18)
#define INTR1_reg_dma_tdi_done_int_flag BIT(19)
#define INTR1_reserved_2 (BIT(20) | BIT(21) | BIT(22) | BIT(23))
#define INTR1_reg_flash_error_flag BIT(24)
#define INTR1_reg_flash_int_flag BIT(25)
#define INTR1_reserved_3 BIT(26)

#define INTR2_ADDR (INTR_BASED_ADDR + 0x8)
#define INTR2_td_int_flag_clear INTR2_ADDR
#define INTR2_td_timeout_int_flag_clear BIT(1)
#define INTR2_td_debug_frame_done_int_flag_clear BIT(2)
#define INTR2_td_frame_start_scan_int_flag_clear BIT(3)
#define INTR2_log_int_flag_clear BIT(4)
#define INTR2_d2t_crc_err_int_flag_clear BIT(8)
#define INTR2_d2t_flash_req_int_flag_clear BIT(9)
#define INTR2_d2t_ddi_int_flag_clear BIT(10)
#define INTR2_wr_done_int_flag_clear BIT(16)
#define INTR2_rd_done_int_flag_clear BIT(17)
#define INTR2_tdi_err_int_flag_clear BIT(18)
#define INTR2_d2t_slpout_rise_flag_clear BIT(24)
#define INTR2_d2t_slpout_fall_flag_clear BIT(25)
#define INTR2_d2t_dstby_flag_clear BIT(26)
#define INTR2_ddi_pwr_rdy_flag_clear BIT(27)

#define INTR32_ADDR (INTR_BASED_ADDR + 0x80)
#define INTR32_reg_ice_sw_int_en INTR32_ADDR
#define INTR32_reg_ice_apb_conflict_int_en BIT(1)
#define INTR32_reg_ice_ilm_conflict_int_en BIT(2)
#define INTR32_reg_ice_dlm_conflict_int_en BIT(3)
#define INTR32_reserved_0 (BIT(4) | BIT(5) | BIT(6) | BIT(7))
#define INTR32_reg_spi_sr_int_en BIT(8)
#define INTR32_reg_spi_sp_int_en BIT(9)
#define INTR32_reg_spi_trx_int_en BIT(10)
#define INTR32_reg_spi_cmd_int_en BIT(11)
#define INTR32_reg_spi_rw_int_en BIT(12)
#define INTR32_reserved_1 (BIT(13) | BIT(14) | BIT(15))
#define INTR32_reg_i2c_start_int_en BIT(16)
#define INTR32_reg_i2c_addr_match_int_en BIT(17)
#define INTR32_reg_i2c_cmd_int_en BIT(18)
#define INTR32_reg_i2c_sr_int_en BIT(19)
#define INTR32_reg_i2c_trx_int_en BIT(20)
#define INTR32_reg_i2c_rx_stop_int_en BIT(21)
#define INTR32_reg_i2c_tx_stop_int_en BIT(22)
#define INTR32_reserved_2 BIT(23)
#define INTR32_reg_t0_int_en BIT(24)
#define INTR32_reg_t1_int_en BIT(25)
#define INTR32_reserved_3 (BIT(26) | BIT(27) | BIT(28) | BIT(29) | BIT(30) | BIT(31))

#define INTR33_ADDR (INTR_BASED_ADDR + 0x84)
#define INTR33_reg_uart_tx_int_en INTR33_ADDR
#define INTR33_reserved_0 (BIT(1) | BIT(2) | BIT(3) | BIT(4) | BIT(5) | BIT(6) | BIT(7))
#define INTR33_reg_wdt_alarm_int_en BIT(8)
#define INTR33_reserved_1 (BIT(9) | BIT(10) | BIT(11) | BIT(12) | BIT(13) | BIT(14) | BIT(15))
#define INTR33_reg_dma_ch1_int_en BIT(16)
#define INTR33_reg_dma_ch0_int_en BIT(17)
#define INTR33_reg_dma_frame_done_int_en BIT(18)
#define INTR33_reg_dma_tdi_done_int_en BIT(19)
#define INTR33_reserved_2 (BIT(20) | BIT(21) | BIT(22) | BIT(23))
#define INTR33_reg_flash_error_en BIT(24)
#define INTR33_reg_flash_int_en BIT(25)
#define INTR33_reserved_3 (BIT(26) | BIT(27) | BIT(28) | BIT(29) | BIT(30) | BIT(31))

/* Flash */
#define FLASH_BASED_ADDR 0x41000
#define FLASH0_ADDR (FLASH_BASED_ADDR + 0x0)
#define FLASH0_reg_flash_csb FLASH0_ADDR
#define FLASH0_reserved_0 (BIT(8) | BIT(9) | BIT(10) | BIT(11) | BIT(12) | BIT(13) | BIT(14) | BIT(15))
#define FLASH0_reg_preclk_sel (BIT(16) | BIT(17) | BIT(18) | BIT(19) | BIT(20) | BIT(21) | BIT(22) | BIT(23))
#define FLASH0_reg_rx_dual BIT(24)
#define FLASH0_reg_tx_dual BIT(25)
#define FLASH0_reserved_26 (BIT(26) | BIT(27) | BIT(28) | BIT(29) | BIT(30) | BIT(31))
#define FLASH0_dual_mode (FLASH0_ADDR + 0x3)
#define FLASH1_ADDR (FLASH_BASED_ADDR + 0x4)
#define FLASH1_reg_flash_key1 FLASH1_ADDR
#define FLASH1_reg_flash_key2 (FLASH1_ADDR + 0x01)
#define FLASH1_reg_flash_key3 (FLASH1_ADDR + 0x02)
#define FLASH1_reserved_0 (FLASH1_ADDR + 0x03)
#define FLASH2_ADDR (FLASH_BASED_ADDR + 0x8)
#define FLASH2_reg_tx_data FLASH2_ADDR
#define FLASH3_ADDR (FLASH_BASED_ADDR + 0xC)
#define FLASH3_reg_rcv_cnt FLASH3_ADDR
#define FLASH4_ADDR (FLASH_BASED_ADDR + 0x10)
#define FLASH4_reg_rcv_data FLASH4_ADDR
#define FLASH4_reg_rcv_dly BIT(8)
#define FLASH4_reg_sutrg_en BIT(9)
#define FLASH4_reserved_1 (BIT(10) | BIT(11) | BIT(12) | BIT(13) | BIT(14) | BIT(15))
#define FLASH4_reg_rcv_data_valid_state BIT(16)
#define FLASH4_reg_flash_rd_finish_state BIT(17)
#define FLASH4_reserved_2 (BIT(18) | BIT(19) | BIT(20) | BIT(21) | BIT(22) | BIT(23))
#define FLASH4_reg_flash_dma_trigger_en (BIT(24) | BIT(25) | BIT(26) | BIT(27) | BIT(28) | BIT(29) | BIT(30) | BIT(31))

/* Dummy Registers */
#define WDT_DUMMY_BASED_ADDR 0x5101C
#define WDT7_DUMMY0 WDT_DUMMY_BASED_ADDR
#define WDT8_DUMMY1 (WDT_DUMMY_BASED_ADDR + 0x04)
#define WDT9_DUMMY2 (WDT_DUMMY_BASED_ADDR + 0x08)

/* Cascade Register */
#define QUAL_MODE_REG 0x62020
#define MS_MISO_SEL_REG 0x62021
#define SINGLE_TO_QUAL_REG 0x6202C
#define READ_BACK_LOCK_REG 0x6202D
#define SLAVE_READ_BACK_WORD_REG 0x62030
#define MSPI_REG 0x62084
#define ENABLE_CASCADE_ADDR 0x71062
#define MSPI_TRIG_BY_DMA_ADDR 0x62070
#define MSPI_TRIGGER_ADDR 0x62085
#define MSPI_CLOCK_ADDR 0x62086
#define MSPI_CS_ADDR 0x62087
#define MSPI_INIT_EN_ADDR 0x480C4
#define DMA_ONE_GROUP_ADDR 0x72318
#define DMA_ONE_CLEAR_ADDR 0x72103
#define SRC_ONE_ADDR 0x72104
#define SRC_ONE_SET_ADDR 0x72108
#define SRC_TWO_EN_ADDR 0x72113
#define DEST_ADDR 0x72114
#define DEST_SET_ADDR 0x72118
#define TRIG_SEL_MSPI_ADDR 0x72102
#define TRANSFER_CNT_ADDR 0x7211C
#define WRITE_BUF_SIZE_ADDR 0x62080
#define READ_BUF_SIZE_ADDR 0x62082
#define WRITE_BUF_ADDR 0x6205C
#define MSPI_DONE_FLAG_ADDR 0x48028
#define CONTROL_SLAVE_EXIT_ICE_ADDR 0x181062
#define WDT_ADDR 0x4004D
#define DMA_CRC_ADDR 0x04101C

/* The example for the gesture virtual keys */
#define GESTURE_DOUBLECLICK 0x58
#define GESTURE_UP 0x60
#define GESTURE_DOWN 0x61
#define GESTURE_LEFT 0x62
#define GESTURE_RIGHT 0x63
#define GESTURE_M 0x64
#define GESTURE_W 0x65
#define GESTURE_C 0x66
#define GESTURE_E 0x67
#define GESTURE_V 0x68
#define GESTURE_O 0x69
#define GESTURE_S 0x6A
#define GESTURE_Z 0x6B
#define KEY_GESTURE_POWER KEY_POWER
#define KEY_GESTURE_UP KEY_UP
#define KEY_GESTURE_DOWN KEY_DOWN
#define KEY_GESTURE_LEFT KEY_LEFT
#define KEY_GESTURE_RIGHT KEY_RIGHT
#define KEY_GESTURE_O KEY_O
#define KEY_GESTURE_E KEY_E
#define KEY_GESTURE_M KEY_M
#define KEY_GESTURE_W KEY_W
#define KEY_GESTURE_S KEY_S
#define KEY_GESTURE_V KEY_V
#define KEY_GESTURE_C KEY_C
#define KEY_GESTURE_Z KEY_Z
#define KEY_GESTURE_F KEY_F
#define GESTURE_V_DOWN 0x6C
#define GESTURE_V_LEFT 0x6D
#define GESTURE_V_RIGHT 0x6E
#define GESTURE_TWOLINE_DOWN 0x6F
#define GESTURE_F 0x70
#define GESTURE_AT 0x71
#define ESD_GESTURE_PWD 0xF38A94EF
#define SPI_ESD_GESTURE_RUN 0xA67C9DFE
#define I2C_ESD_GESTURE_RUN 0xA67C9DFE
#define SPI_ESD_GESTURE_PWD_ADDR 0x25FF8
#define I2C_ESD_GESTURE_PWD_ADDR 0x40054

#define ESD_GESTURE_CORE146_PWD 0xF38A
#define SPI_ESD_GESTURE_CORE146_RUN 0x5B92
#define I2C_ESD_GESTURE_CORE146_RUN 0xA67C
#define SPI_ESD_GESTURE_CORE146_PWD_ADDR 0x4005C
#define I2C_ESD_GESTURE_CORE146_PWD_ADDR 0x4005C

#define DOUBLE_TAP (ON)					/* BIT0 */
#define ALPHABET_LINE_2_TOP (ON)		/* BIT1 */
#define ALPHABET_LINE_2_BOTTOM (ON)		/* BIT2 */
#define ALPHABET_LINE_2_LEFT (ON)		/* BIT3 */
#define ALPHABET_LINE_2_RIGHT (ON)		/* BIT4 */
#define ALPHABET_M (ON)					/* BIT5 */
#define ALPHABET_W (ON)					/* BIT6 */
#define ALPHABET_C (ON)					/* BIT7 */
#define ALPHABET_E (ON)					/* BIT8 */
#define ALPHABET_V (ON)					/* BIT9 */
#define ALPHABET_O (ON)					/* BIT10 */
#define ALPHABET_S (ON)					/* BIT11 */
#define ALPHABET_Z (ON)					/* BIT12 */
#define ALPHABET_V_DOWN (ON)			/* BIT13 */
#define ALPHABET_V_LEFT (ON)			/* BIT14 */
#define ALPHABET_V_RIGHT (ON)			/* BIT15 */
#define ALPHABET_TWO_LINE_2_BOTTOM (ON) /* BIT16 */
#define ALPHABET_F (ON)					/* BIT17 */
#define ALPHABET_AT (ON)				/* BIT18 */

/* FW data format */
#define DATA_FORMAT_DEMO_CMD 0x00
#define DATA_FORMAT_DEBUG_CMD 0x02
#define DATA_FORMAT_DEMO_DEBUG_INFO_CMD 0x04
#define DATA_FORMAT_GESTURE_NORMAL_CMD 0x01
#define DATA_FORMAT_GESTURE_INFO_CMD 0x02
#define DATA_FORMAT_DEBUG_LITE_CMD 0x05
#define DATA_FORMAT_DEBUG_LITE_ROI_CMD 0x01
#define DATA_FORMAT_DEBUG_LITE_WINDOW_CMD 0x02
#define DATA_FORMAT_DEBUG_LITE_AREA_CMD 0x03
#define DATA_FORMAT_DEBUG_LITE_PEN_CMD 0x04

/* Pen data format */
#define DATA_FORMAT_DEBUG_PEN_CMD 0x06
#define DATA_FORMAT_DEBUG_HAND_CMD 0x07

#define P5_X_DEMO_MODE_PACKET_INFO_LEN 3
#define P5_X_DEMO_MODE_PACKET_LEN 43
#define P5_X_DEMO_MODE_PACKET_LEN_HIGH_RESOLUTION 72
#define P5_X_DEMO_LOW_RESOLUTION_FINGER_DATA_LENGTH 44
#define P5_X_DEMO_HIGH_RESOLUTION_FINGER_DATA_LENGTH 54
#define P5_X_DEMO_MODE_AXIS_LEN 50
#define P5_X_DEMO_MODE_STATE_INFO 16
#define P5_X_INFO_HEADER_LENGTH 3
#define P5_X_P_SENSOR_KEY_LENGTH 1
#define P5_X_INFO_CHECKSUM_LENGTH 1
#define P5_X_DEMO_DEBUG_INFO_ID0_LENGTH 14
#define P5_X_DEBUG_MODE_PACKET_LENGTH 1280
#define P5_X_TEST_MODE_PACKET_LENGTH 1180
#define P5_X_GESTURE_NORMAL_LENGTH 8
#define P5_X_GESTURE_INFO_LENGTH 170
#define P5_X_GESTURE_INFO_LENGTH_HIGH_RESOLUTION 221
#if (TDDI_INTERFACE == BUS_SPI)
#define P5_X_DEBUG_LITE_LENGTH 4093
#else
#define P5_X_DEBUG_LITE_LENGTH 300
#endif
#define P5_X_DEBUG_LITE_PEN_LENGTH 136
#define P5_X_CORE_VER_THREE_LENGTH 5
#define P5_X_CORE_VER_FOUR_LENGTH 6
#define P5_X_EDGE_PALM_PARA_LENGTH 31
#define P5_X_DEBUG_LOW_RESOLUTION_FINGER_DATA_LENGTH 35
#define P5_X_DEBUG_HIGH_RESOLUTION_FINGER_DATA_LENGTH 45
#define P5_X_5B_LOW_RESOLUTION_LENGTH 62
#define P5_X_CUSTOMER_LENGTH 50
#define P5_X_DEBUG_MODE_PACKET_INFO_LEN 4

/* Pen data info len */
#define P5_X_ID_TYPE_LEN 5
#define P5_X_PEN_INFO_X_DATA_LEN 6
#define P5_X_PEN_INFO_Y_DATA_LEN 4
#define P5_X_PEN_DATA_LEN 24
#define P5_X_OTHER_DATA_LEN 16
#define P5_X_PEN_ID_LEN 8
#define P5_X_HAND_PEN_TYPE_PACKET_LEN 96
#define P5_X_NEW_DEBUG_FORMAT_X_LEN 50
#define P5_X_NEW_DEBUG_FORMAT_Y_LEN 50
#define TOUCH_TILT_OFFSET 10
#define TOUCH_PRESS_OFFSET 12
#define PEN_ID_OFFSET 14

/* Pen Report mode */
#define ONLY_REPORT_HAND 2
#define ONLY_REPORT_PEN 1
#define REPORT_HAND_PEN 0
#define RELEASE_PEN 0
#define PEN_CONTACT 1

/* Protocol */
#define PROTOCOL_VER_500 0x050000
#define PROTOCOL_VER_510 0x050100
#define PROTOCOL_VER_520 0x050200
#define PROTOCOL_VER_530 0x050300
#define PROTOCOL_VER_540 0x050400
#define PROTOCOL_VER_550 0x050500
#define PROTOCOL_VER_560 0x050600
#define PROTOCOL_VER_570 0x050700
#define P5_X_READ_DATA_CTRL 0xF6
#define P5_X_GET_TP_INFORMATION 0x20
#define P5_X_GET_KEY_INFORMATION 0x27
#define P5_X_GET_TOOL_VERSION 0x28
#define P5_X_GET_PANEL_INFORMATION 0x29
#define P5_X_GET_FW_VERSION 0x21
#define P5_X_GET_PROTOCOL_VERSION 0x22
#define P5_X_GET_CORE_VERSION 0x23
#define P5_X_GET_CORE_VERSION_NEW 0x24
#define P5_X_GET_COMPRESS_INFORMATION 0x2B
#define P5_X_GET_ALL_INFORMATION 0x2F
#define P5_X_GET_REPORT_FORMAT 0x37
#define P5_X_MODE_CONTROL 0xF0
#define P5_X_NEW_CONTROL_FORMAT 0xF2
#define P5_X_SET_CDC_INIT 0xF1
#define P5_X_GET_CDC_DATA 0xF2
#define P5_X_CDC_BUSY_STATE 0xF3
#define P5_X_MP_TEST_MODE_INFO 0xFE
#define P5_X_I2C_UART 0x40
#define CMD_GET_FLASH_DATA 0x41
#define CMD_CTRL_INT_ACTION 0x1B
#define P5_X_FW_UNKNOWN_MODE 0xFF
#define P5_X_FW_AP_MODE 0x00
#define P5_X_FW_TEST_MODE 0x01
#define P5_X_FW_GESTURE_MODE 0x0F
#define P5_X_FW_SIGNAL_DATA_MODE 0x03
#define P5_X_FW_RAW_DATA_MODE 0x08
#define P5_X_DEMO_PACKET_ID 0x5A
#define P5_X_DEMO_AXIS_PACKET_ID 0x5B
#define P5_X_DEBUG_PACKET_ID 0xA7
#define P5_X_DEBUG_AXIS_PACKET_ID 0xA8
#define P5_X_TEST_PACKET_ID 0xF2
#define P5_X_GESTURE_PACKET_ID 0xAA
#define P5_X_GESTURE_FAIL_ID 0xAE
#define P5_X_I2CUART_PACKET_ID 0x7A
#define P5_X_DEBUG_LITE_PACKET_ID 0x9A
#define P5_X_SLAVE_MODE_CMD_ID 0x5F
#define P5_X_INFO_HEADER_PACKET_ID 0xB7
#define P5_X_DEMO_DEBUG_INFO_PACKET_ID 0x5C
#define P5_X_EDGE_PLAM_CTRL_1 0x01
#define P5_X_EDGE_PLAM_CTRL_2 0x12
#define P5_X_EDGE_PALM_TUNING_PARA 0x1E
#define SPI_WRITE 0x82
#define SPI_READ 0x83
#define SPI_ACK 0xA3
#define P5_X_DEMO_PROXIMITY_ID 0xBC
#define P5_X_DEMO_HIGH_RESOLUTION_PACKET_ID 0x5B
#define P5_X_DEBUG_HIGH_RESOLUTION_PACKET_ID 0xA8

/*Pen & Cascade info cmd*/
#define P5_X_GET_PEN_INFO 0x27
#define P5_X_GET_CASCADE_INFO 0x2A

/* Pen Type */
#define P5_X_HAND_PEN_TYPE 0  /* 0x39, FigType 0-2 bits, CustomType 3-5, PenType 6-7 */
#define P5_X_ONLY_PEN_TYPE 1  /* 0x3F */
#define P5_X_ONLY_HAND_TYPE 2 /* 0xF9 */

/* Chips */
#define ILI2882_CHIP 0x2882
#define ILI9881_CHIP 0x9881
#define ILI7807_CHIP 0x7807
#define ILI9881N_AA 0x98811700
#define ILI9881O_AA 0x98811800
#define ILI7807Q_AA 0x78071A00
#define ILI9881N_AA 0x98811700
#define ILI9882_CHIP 0x9882
#define ILI9883_CHIP 0x9883
#define TDDI_PID_ADDR 0x4009C
#define TDDI_OTP_ID_ADDR 0x400A0
#define TDDI_ANA_ID_ADDR 0x400A4
#define TDDI_PC_COUNTER_ADDR 0x44008
#define TDDI_PC_LATCH_ADDR 0x51010
#define TDDI_CHIP_RESET_ADDR 0x40050
#define RAWDATA_NO_BK_SHIFT 8192
#define TDDI_WHOLE_CHIP_RST_WITH_FLASH_KEY 0x00019878
#define TDDI_WHOLE_CHIP_RST_WITHOUT_FLASH_KEY 0xA0019878
#define TDDI_WHOLE_CHIP_RST_WITHOUT_FLASH_PRE_KEY 0xA0009878

/* Edge Palm */
#define ZONE_A_W 6
#define ZONE_B_W 32
#define ZONE_A_ROTATION_W 6
#define ZONE_B_ROTATION_W 32
#define ZONE_C_WIDTH 80
#define ZONE_C_HEIGHT 320
#define ZONE_C_HEIGHT_LITTLE 100

/* DDI */
#define PAGE00_CMD10_SLEEPIN 0x10
#define PAGE00_CMD11_SLEEPOUT 0x11
#define PAGE00_CMD28_DISPLAYOFF 0x28
#define PAGE00_CMD29_DISPLAYON 0x29

/* Report Format Resolution */
#define POSITION_LOW_RESOLUTION 0X00
#define POSITION_HIGH_RESOLUTION 0x01
#define POSITION_CUSTOMER_TYPE_ON 0x00
#define POSITION_CUSTOMER_TYPE_OFF 0x1F
#define POSITION_PEN_TYPE_ON 0x00
#define POSITION_PEN_TYPE_OFF 0x03
#define POSITION_CUSTOMER_TYPE_OFF_3BITS 0x07 /*core ver 1700, CustomerType 3 bits*/

#define SRAM_TEST_GROUP_LEN 16
#define SRAM_TEST_GROUP_STR_LEN 64

struct sram_test_para
{
	/* ini param */
	bool tpu_clock_en;
	bool sram_group_en;
	bool check_failbus;
	u8 group_num;
	u32 MaxAddr;
	u32 MinAddr;
	u32 write_addr;
	u32 read_addr;
	u32 SramPartialAreaTest_en[SRAM_TEST_GROUP_LEN];
	u32 SramResult[SRAM_TEST_GROUP_LEN];
	char strSramPartialArea[SRAM_TEST_GROUP_LEN][SRAM_TEST_GROUP_STR_LEN];
};

struct ilitek_ts_data
{
	struct i2c_client *i2c;
	struct spi_device *spi;
	struct input_dev *input;
	struct input_dev *input_pen;
	struct device *dev;
	struct wakeup_source *ws;

	struct ilitek_hwif_info *hwif;
	struct ilitek_ic_info *chip;
	struct ilitek_protocol_info *protocol;
	struct gesture_coordinate *gcoord;
	struct regulator *vdd;
	struct regulator *vcc;
	struct sram_test_para sram_para;

	struct notifier_block notifier_fb;
// #ifdef CONFIG_FB
// 	struct notifier_block notifier_fb;
// 	// #else
// 	// struct early_suspend early_suspend;

// #endif

#ifdef CONFIG_HAS_EARLYSUSPEND
	struct early_suspend early_suspend;
#endif

#if CHARGER_NOTIFIER_CALLBACK
#if KERNEL_VERSION(4, 1, 0) <= LINUX_VERSION_CODE
	/* add_for_charger_start */
	struct notifier_block notifier_charger;
	struct workqueue_struct *charger_notify_wq;
	struct work_struct update_charger;
	int usb_plug_status;
/*  add_for_charger_end  */
#endif
#endif
#ifdef ROI
	struct ts_kit_device_data *ts_dev_data;
#endif
	struct mutex touch_mutex;
	struct mutex debug_mutex;
	struct mutex debug_read_mutex;
	spinlock_t irq_spin;

	struct completion pm_completion;
	bool pm_suspend;

	/* physical path to the input device in the system hierarchy */
	const char *phys;

	bool boot;
	u32 fw_pc;
	u32 fw_latch;

	u16 max_x;
	u16 max_y;
	u16 min_x;
	u16 min_y;
	u16 panel_wid;
	u16 panel_hei;
	u8 xch_num;
	u8 ych_num;
	u8 stx;
	u8 srx;
	u8 *update_buf;
	u8 *tr_buf;
	u8 *spi_tx;
	u8 *spi_rx;
#ifdef ROI
	u8 knuckle_roi_data[ROI_DATA_READ_LENGTH];
#endif
	struct firmware tp_fw;

	int actual_tp_mode;
	int tp_data_mode;
	int tp_data_format;
	int tp_data_len;

	int irq_num;
	int irq_gpio;
	int irq_tirgger_type;
	int tp_rst;
	int tp_int;
	int wait_int_timeout;

	int finger;
	u8 customertype_off;
	u32 cdc_data_len;
	u8 gesture_data_type;
	bool compress_disable;
	bool compress_handonly_disable;
	bool compress_penonly_disable;

	/* pen report */
	u8 PenType;
	int pen;
	struct pen_info_block pen_info_block;
	struct ilitek_pen_info pen_info;
	int curt_touch[MAX_TOUCH_NUM + MAX_PEN_NUM];
	int prev_touch[MAX_TOUCH_NUM + MAX_PEN_NUM];
	u8 touch_num;

	int last_touch;
	int fw_retry;
	int fw_update_stat;
	int fw_open;
	u8 fw_info[75];
	u8 fw_mp_ver[4];
	u8 edge_palm_para[P5_X_EDGE_PALM_PARA_LENGTH];
	bool wq_ctrl;
	bool wq_esd_ctrl;
	bool wq_bat_ctrl;
	bool esd_func_ctrl;

	/* Cascade */
	struct cascade_info_block cascade_info_block;
	bool mspi_en;
	bool slave_wr_ctrl;
	bool enable_cascade;
	bool mspi_done;
	bool i2c_addr_change;
	int cascade_rst_edge_delay;
	u8 cascade_ctrl_mode;

	u16 addr_value;

	bool report;
	bool gesture;
	bool mp_retry;
	bool knuckle;
	int gesture_mode;
	int gesture_demo_ctrl;
	struct gesture_symbol ges_sym;
	struct report_info_block rib;

	u16 flash_mid;
	u16 flash_devid;
	u8 current_report_rate_mode;
	int program_page;
	int flash_sector;

	/* Sending report data to users for the debug */
	bool dnp;			 /* debug node open */
	int dbf;			 /* debug data frame */
	int odi;			 /* out data index */
	bool all_frame_full; /* all debug data frame is full */
	wait_queue_head_t inq;
	struct debug_buf_list *dbl;
	int raw_count;
	int delta_count;
	int bg_count;

	/* ping pong buffer */
	bool dlnp; /* debug all node open */
	bool ppb;
	struct debug_buf_list *dbf_a;
	struct debug_buf_list *dbf_b;
	struct debug_buf_list *dbf_out;
	struct debug_buf_list *dbf_in;
	int dbf_in_idx;
	int dbf_out_idx;

	int reset;
	int rst_edge_delay;
	int fw_upgrade_mode;
	int mp_ret_len;
	bool wtd_ctrl;
	bool force_fw_update;
	bool irq_after_recovery;
	bool ddi_rest_done;
	bool resume_by_ddi;
	bool tp_suspend;
	bool info_from_hex;
	bool prox_near;
	bool gesture_load_code;
	bool trans_xy;
	bool ss_ctrl;
	bool node_update;
	bool int_pulse;
	bool pll_clk_wakeup;
	bool power_status;
	bool proxmity_face;
	bool eng_flow;
	bool ice_flash_cs_ctrl;
	bool prox_face_gesture;
	u8 prox_face_mode;
	/* module info */
	int tp_module;
	int md_fw_ili_size;
	char *md_name;
	char *md_fw_filp_path;
	char *md_fw_rq_path;
	char *md_ini_path;
	char *md_ini_rq_path;
	char *outputStrArr;
	u8 *md_fw_ili;
	char *mp_result;

	atomic_t irq_stat;
	atomic_t tp_reset;
	atomic_t ice_stat;
	atomic_t fw_stat;
	atomic_t mp_stat;
	atomic_t tp_sleep;
	atomic_t tp_sw_mode;
	atomic_t cmd_int_check;
	atomic_t esd_stat;
	atomic_t ignore_report;
	atomic_t sync_stat;
	atomic_t slave_ice_stat;

	/* Event for driver test */
	struct completion esd_done;

	int (*spi_write_then_read)(struct spi_device *spi, const void *txbuf,
							   unsigned n_tx, void *rxbuf, unsigned n_rx);
	int (*wrapper)(u8 *wdata, u32 wlen, u8 *rdata, u32 rlen, bool spi_irq, bool i2c_irq);
	int (*mp_move_code)(void);
	int (*gesture_move_code)(int mode);
	int (*esd_recover)(void);
	int (*ges_recover)(void);
	void (*demo_debug_info[5])(u8 *, size_t);
	int (*detect_int_stat)(bool status);
};
extern struct ilitek_ts_data *ilits;

struct debug_buf_list
{
	bool mark;
	unsigned char *data;
};

struct ilitek_touch_info
{
	u16 id;
	u16 x;
	u16 y;
	u16 pressure;
};

struct ilitek_axis_info
{
	s8 degree;
	u16 width_major;
	u16 width_minor;
};

struct ilitek_dma_config
{
	u32 src_addr;
	u32 src_fmt;
	u32 dest_addr;
	u32 dest_fmt;
	u32 block_size;
};

struct gesture_coordinate
{
	u16 code;
	u8 clockwise;
	int type;
	struct ilitek_touch_info pos_start;
	struct ilitek_touch_info pos_end;
	struct ilitek_touch_info pos_1st;
	struct ilitek_touch_info pos_2nd;
	struct ilitek_touch_info pos_3rd;
	struct ilitek_touch_info pos_4th;
};

struct ilitek_protocol_info
{
	u32 ver;
	int fw_ver_len;
	int pro_ver_len;
	int tp_info_len;
	int key_info_len;
	int panel_info_len;
	int core_ver_len;
	int func_ctrl_len;
	int window_len;
	int cdc_len;
	int mp_info_len;
};

#define FUNC_CTRL_NUM 17
#define FUNC_CTRL_PROXIMITY_IDX 5
struct ilitek_ic_func_ctrl
{
	const char *name;
	u8 cmd[6];
	int len;
	u8 def_cmd;
	u8 rec_state; /* 0:disable, 1: enable, 2: ignore record */
	u8 rec_cmd;
};
extern struct ilitek_ic_func_ctrl func_ctrl[FUNC_CTRL_NUM];

struct ilitek_ic_info
{
	u8 type;
	u8 ver;
	u16 id;
	u8 slave_ver;
	u16 slave_id;
	u32 slave_pid;
	u32 pid;
	u32 pid_addr;
	u32 product_id;
	u32 pc_counter_addr;
	u32 pc_latch_addr;
	u32 reset_addr;
	u32 otp_addr;
	u32 ana_addr;
	u32 otp_id;
	u32 ana_id;
	u32 fw_ver;
	u32 core_ver;
	u32 fw_mp_ver;
	u32 max_count;
	u32 reset_key;
	u16 wtd_key;
	int no_bk_shift;
	bool dma_reset;
	int (*dma_crc)(u32 start_addr, u32 block_size);
};

struct ilitek_hwif_info
{
	u8 bus_type;
	u8 plat_type;
	const char *name;
	struct module *owner;
	const struct of_device_id *of_match_table;
	const struct dev_pm_ops *pm;
	int (*plat_probe)(void);
	int (*plat_remove)(void);
	void *info;
};

/* Prototypes for tddi firmware/flash functions */
extern void ili_flash_dma_write(u32 start, u32 end, u32 len);
extern void ili_flash_clear_dma(void);
extern void ili_fw_read_flash_info(bool mcu);
extern void ilitek_tddi_flash_protect(bool enable, bool mcu);
extern int ili_fw_read_hw_crc(u32 start, u32 end, u32 *flash_crc);
extern int ili_fw_read_flash(u32 start, u32 end, u8 *data, int len);
extern int ili_fw_dump_iram_data(u32 start, u32 end, bool save, bool mcu);
extern int ili_fw_dump_flash_data(u32 start, u32 end, bool user, bool mcu);
extern int ili_fw_upgrade(int op);

/* Prototypes for tddi mp test */
extern int ili_mp_test_main(char *apk, bool lcm_on);

/* Prototypes for tddi core functions */
extern int ili_touch_esd_gesture_flash(void);
extern int ili_touch_esd_gesture_iram(void);
extern void ili_set_gesture_symbol(void);
extern int ili_move_gesture_code_flash(int mode);
extern int ili_move_gesture_code_iram(int mode);
extern int ili_move_mp_code_flash(void);
extern int ili_move_mp_code_iram(void);
extern void ili_touch_press(u16 x, u16 y, u16 pressure, u16 id);
extern void ili_touch_release(u16 x, u16 y, u16 id);
extern void ili_touch_release_all_point(void);
extern void ili_report_ap_mode(u8 *buf, int len);
extern void ili_pen_release(void);
extern void ili_pen_demo_mode_report_point(u8 *buf, int len);
extern void ili_report_pen_ap_mode(u8 *buf, int len);
extern void ili_report_pen_debug_mode(u8 *buf, int len);
extern void ili_report_debug_mode(u8 *buf, int rlen);
extern void ili_pen_debug_mode_report_point(u8 *buf, int len, u8 offset);
extern void ili_report_debug_lite_mode(u8 *buf, int rlen);
extern void ili_report_gesture_mode(u8 *buf, int rlen);
extern void ili_report_proximity_mode(u8 *buf, int len);
extern void ili_report_i2cuart_mode(u8 *buf, int rlen);
extern void ili_ic_set_ddi_reg_onepage(u8 page, u8 reg, u8 data, bool mcu);
extern void ili_ic_get_ddi_reg_onepage(u8 page, u8 reg, u8 *data, bool mcu);
extern void ili_ic_set_cascade_ddi_reg_onepage(u8 page, u8 reg, u8 data, bool mcu);
extern void ili_ic_get_cascade_ddi_reg_onepage(u8 page, u8 reg, u8 *data, bool mcu);
extern int ili_ic_whole_reset(bool mcu, bool withflash);
extern int ili_ic_code_reset(bool mcu);
extern int ili_ic_func_ctrl(const char *name, int ctrl);
extern int ili_ic_func_ctrl_export(const char *name, int ctrl);
extern void ili_ic_func_ctrl_reset(void);
extern void ili_ic_edge_palm_para_init(void);
extern void ili_ic_send_edge_palm_para(void);
extern int ili_ic_int_trigger_ctrl(bool pulse);
extern void ili_ic_get_pc_counter(int stat);
extern int ili_ic_check_int_level(bool level);
extern int ili_ic_check_int_pulse(bool pulse);
extern int ili_ic_check_busy(int count, int delay, bool irq);
extern int ili_ic_get_project_id(u8 *pdata, int size);
extern int ili_ic_get_panel_info(void);
extern int ili_ic_get_tp_info(void);
extern int ili_ic_get_core_ver(void);
extern int ili_ic_get_protocl_ver(void);
extern int ili_ic_get_fw_ver(void);
extern int ili_ic_get_info(void);
extern int ili_ic_get_all_info(void);
extern void ili_ic_get_compress_info(void);
extern int ili_set_bypass_mode(bool mcu);
extern int ili_cascade_ic_get_info(bool enter_ice, bool exit_ice);
extern void ili_mspi_set_slave_to_qual_mode(void);
extern int ili_mspi_write_slave_register(u32 addr, u32 data, int len);
extern void ili_mspi_read_slave_register(u32 addr, u32 *data, int len);
extern void ili_wdt_reset_status(bool enter_ice, bool exit_ice);
extern int ili_cascade_reset_ctrl(int reset_mode, bool enter_ice);
extern int ili_cascade_rw_tp_reg(u32 mcu, u32 type, u32 addr, u32 wdata, u32 wlen, u32 *rdata);
extern int ili_ic_dummy_check(void);
extern int ili_ic_report_rate_set(u8 mode);
extern int ili_ic_report_rate_get(void);
extern int ili_ic_report_rate_register_set(void);
extern int ili_ice_mode_bit_mask_write(u32 addr, u32 mask, u32 value);
extern int ili_ice_mode_write(u32 addr, u32 data, int len);
extern void ili_cascade_sync_ctrl(bool mode);
extern int ili_ice_mode_ctrl_by_mode(bool enable, bool mcu, int mode);
extern int ili_ice_mode_write_by_mode(u32 addr, u32 data, int len, int mode);
extern int ili_ice_mode_read_by_mode(u32 addr, u32 *data, int len, int mode);
extern int ili_ice_mode_read(u32 addr, u32 *data, int len);
extern int ili_ice_mode_ctrl(bool enable, bool mcu);
extern void ili_ic_init(void);
extern void ili_fw_uart_ctrl(u8 ctrl);
extern int ilitek_tddi_mp_ini_parser(const char *path);
extern int ili_parsing_sram_param(void);
extern int ili_tddi_ic_sram_test(void);
#ifdef ROI
extern int ili_read_knuckle_roi_data(void);
extern int ili_config_get_knuckle_roi_status(void);
extern int ili_config_knuckle_roi_ctrl(cmd_types cmd);
#endif
/* Prototypes for tddi events */
#if RESUME_BY_DDI
extern void ili_resume_by_ddi(void);
#endif
extern int ili_proximity_far(int mode);
extern int ili_proximity_near(int mode);
extern int ili_switch_tp_mode(u8 data);
extern int ili_set_tp_data_len(int format, bool send, u8 *data);
extern int ili_set_pen_data_len(u8 header, u8 ctrl, u8 type);
extern int ili_fw_upgrade_handler(void *data);
extern int ili_wq_esd_i2c_check(void);
extern int ili_wq_esd_spi_check(void);
extern int ili_gesture_recovery(void);
extern void ili_spi_recovery(void);
extern void ili_wq_ctrl(int type, int ctrl);
extern int ili_mp_test_handler(char *apk, bool lcm_on);
extern int ili_report_handler(void);
extern int ili_sleep_handler(int mode);
extern int ili_reset_ctrl(int mode);
extern int ili_tddi_init(void);
extern int ili_dev_init(struct ilitek_hwif_info *hwif);
extern void ili_dev_remove(bool flag);

/* Prototypes for i2c/spi interface */
extern int ili_core_spi_setup(int num);
extern int ili_interface_dev_init(struct ilitek_hwif_info *hwif);
extern void ili_interface_dev_exit(struct ilitek_ts_data *ts, bool flag);
extern int ili_spi_write_then_read_split(struct spi_device *spi,
										 const void *txbuf, unsigned n_tx,
										 void *rxbuf, unsigned n_rx);
extern int ili_spi_write_then_read_direct(struct spi_device *spi,
										  const void *txbuf, unsigned n_tx,
										  void *rxbuf, unsigned n_rx);

/* Prototypes for platform level */
extern void ili_plat_regulator_power_on(bool status);
extern void ili_input_register(void);
extern void ili_input_pen_register(void);
extern void ili_irq_disable(void);
extern void ili_irq_enable(void);
extern void ili_tp_reset(void);
extern void ili_irq_unregister(void);
extern int ili_irq_register(int type);
extern void ilitek_plat_charger_init(void);

/* Prototypes for miscs */
extern void ili_node_init(void);
extern void ili_dump_data(void *data, int type, int len, int row_len, const char *name);
extern u8 ili_calc_packet_checksum(u8 *packet, int len);
extern int ili_katoi(char *str);
extern int ili_str2hex(char *str);

extern int ili_mp_read_write(u8 *wdata, u32 wlen, u8 *rdata, u32 rlen);
/* Prototypes for additional functionalities */
extern void ili_gesture_fail_reason(bool enable);
extern int ili_get_tp_recore_ctrl(int data);
extern int ili_get_tp_recore_data(bool mcu);
extern void ili_demo_debug_info_mode(u8 *buf, size_t rlen);
extern void ili_demo_debug_info_id0(u8 *buf, size_t len);
extern void ili_get_dma1_config(struct ilitek_dma_config *dma);
extern void ili_set_dma1_config(struct ilitek_dma_config *dma);
extern int file_write(struct file_buffer *file, bool new_open);
extern int ili_ic_check_debug_lite_support(int format, bool send, u8 *data);

static inline void ipio_kfree(void **mem)
{
	if (*mem != NULL)
	{
		kfree(*mem);
		*mem = NULL;
	}
}

static inline void ipio_vfree(void **mem)
{
	if (*mem != NULL)
	{
		vfree(*mem);
		*mem = NULL;
	}
}

static inline void *ipio_memcpy(void *dest, const void *src, int n, int dest_size)
{
	if (n > dest_size)
		n = dest_size;

	return memcpy(dest, src, n);
}

static inline int ipio_strcmp(const char *s1, const char *s2)
{
	return (strlen(s1) != strlen(s2)) ? -1 : strncmp(s1, s2, strlen(s1));
}

#endif /* __ILI9881X_H */
