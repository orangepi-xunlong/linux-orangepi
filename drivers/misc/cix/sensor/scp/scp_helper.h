/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2020 MediaTek Inc.
 */

#ifndef __SCP_HELPER_H__
#define __SCP_HELPER_H__

#include <linux/arm-smccc.h>
#include <linux/notifier.h>
#include <linux/interrupt.h>
#include <linux/io.h>

#include "scp.h"

#define ROUNDUP(a, b)		        (((a) + ((b)-1)) & ~((b)-1))

/* scp config reg. definition*/
#define SCP_REGION_INFO_OFFSET	0x4
#define SCP_RTOS_START		(0x800)

#define SCP_DRAM_MAPSIZE	(0x100000)

/* scp dvfs return status flag */
#define SET_PLL_FAIL		(1)
#define SET_PMIC_VOLT_FAIL	(2)

#define mt_reg_sync_writel(v, a) \
	do {    \
		__raw_writel((v), (void __force __iomem *)((a)));   \
		mb();  /*make sure register access in order */ \
	} while (0)

/* This structre need to sync with SCP-side */
struct SCP_IRQ_AST_INFO {
	unsigned int scp_irq_ast_time;
	unsigned int scp_irq_ast_pc_s;
	unsigned int scp_irq_ast_pc_e;
	unsigned int scp_irq_ast_lr_s;
	unsigned int scp_irq_ast_lr_e;
	unsigned int scp_irq_ast_irqd;
};

/* reset ID */
#define SCP_ALL_ENABLE	0x00
#define SCP_ALL_REBOOT	0x01

/* scp semaphore definition*/
enum SEMAPHORE_FLAG {
	SEMAPHORE_CLK_CFG_5 = 0,
	SEMAPHORE_PTP,
	SEMAPHORE_I2C0,
	SEMAPHORE_I2C1,
	SEMAPHORE_TOUCH,
	SEMAPHORE_APDMA,
	SEMAPHORE_SENSOR,
	SEMAPHORE_SCP_A_AWAKE,
	SEMAPHORE_SCP_B_AWAKE,
	NR_FLAG = 9,
};

/* scp semaphore 3way definition */
enum SEMAPHORE_3WAY_FLAG {
	SEMA_SCP_3WAY_UART = 0,
	SEMA_SCP_3WAY_C2C_A = 1,
	SEMA_SCP_3WAY_C2C_B = 2,
	SEMA_SCP_3WAY_DVFS = 3,
	SEMA_SCP_3WAY_AUDIO = 4,
	SEMA_SCP_3WAY_AUDIOREG = 5,
	SEMA_SCP_3WAY_NUM = 6,
};

/* scp semaphore status */
enum  SEMAPHORE_STATUS {
	SEMAPHORE_NOT_INIT = -1,
	SEMAPHORE_FAIL = 0,
	SEMAPHORE_SUCCESS = 1,
};

/* scp reset status */
enum SCP_RESET_STATUS {
	RESET_STATUS_STOP = 0,
	RESET_STATUS_START = 1,
	/* this state mean scp already kick reboot, if wdt trigger before
	 * recovery finish mean recovery fail, should retry again
	 */
	RESET_STATUS_START_KICK = 2,
	RESET_STATUS_START_WDT = 3,
};

/* scp reset status */
enum SCP_RESET_TYPE {
	RESET_TYPE_WDT = 0,
	RESET_TYPE_AWAKE = 1,
	RESET_TYPE_CMD = 2,
	RESET_TYPE_TIMEOUT = 3,
};

struct scp_bus_tracker_status {
	u32 dbg_con;
	u32 dbg_r[32];
	u32 dbg_w[32];
};

struct scp_regs {
	void __iomem *scpsys;
	void __iomem *sram;
	void __iomem *cfg;
	void __iomem *clkctrl;
	void __iomem *l1cctrl;
	void __iomem *cfg_core0;
	void __iomem *cfg_core1;
	void __iomem *cfg_sec;
	void __iomem *bus_tracker;
	int irq0;
	int irq1;
	unsigned int total_tcmsize;
	unsigned int cfgregsize;
	unsigned int scp_tcmsize;
	unsigned int core_nums;
	unsigned int twohart;
	unsigned int secure_dump;
	struct scp_bus_tracker_status tracker_status;
};

/* scp work struct definition*/
struct scp_work_struct {
	struct work_struct work;
	unsigned int flags;
	unsigned int id;
};

struct scp_reserve_mblock {
	enum scp_reserve_mem_id_t num;
	u64 start_phys;
	u64 start_virt;
	u64 size;
};

extern void memcpy_to_scp(void __iomem *trg,
		const void *src, int size);
extern void memcpy_from_scp(void *trg, const void __iomem *src,
		int size);
extern int reset_scp(int reset);

extern phys_addr_t scp_mem_base_phys;
extern phys_addr_t scp_mem_base_virt;
extern phys_addr_t scp_mem_size;
extern atomic_t scp_reset_status;

enum MTK_TINYSYS_SCP_KERNEL_OP {
	MTK_TINYSYS_SCP_KERNEL_OP_DUMP_START = 0,
	MTK_TINYSYS_SCP_KERNEL_OP_DUMP_POLLING,
	MTK_TINYSYS_SCP_KERNEL_OP_RESET_SET,
	MTK_TINYSYS_SCP_KERNEL_OP_RESET_RELEASE,
	MTK_TINYSYS_SCP_KERNEL_OP_RESTORE_L2TCM,
	MTK_TINYSYS_SCP_KERNEL_OP_RESTORE_DRAM,
	MTK_TINYSYS_SCP_KERNEL_OP_WDT_SET,
	MTK_TINYSYS_SCP_KERNEL_OP_HALT_SET,
	MTK_TINYSYS_SCP_KERNEL_OP_WDT_CLEAR,
	MTK_TINYSYS_SCP_KERNEL_OP_DUMP_TBUF,
	MTK_TINYSYS_SCP_KERNEL_OP_DUMP_L2TCM,
	MTK_TINYSYS_SCP_KERNEL_OP_DUMP_REG,
	MTK_TINYSYS_SCP_KERNEL_OP_NUM,
};

#endif
