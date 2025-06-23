// SPDX-License-Identifier: GPL-2.0-only
// Copyright 2024 Cix Technology Group Co., Ltd.
#define pr_fmt(fmt) "DdrException: " fmt

#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/io.h>
#include <linux/interrupt.h>
#include <linux/mailbox_client.h>

#define DDR_STATUS_MASTER 0x574
#define DDR_OUT_OF_RANGE1 0x5e8
#define DDR_OUT_OF_RANGE2 0x5ec
#define DDR_DRAM_ECC1 0x438
#define DDR_DRAM_ECC2 0x43c
#define DDR_DRAM_UECC1 0x428
#define DDR_DRAM_UECC2 0x42c

#define DDR_4_CHANNEL 0xF
#define DDR_2_CHANNEL 0xC

#define EVEN_NUM 0x5555555555555555ULL
#define ODD_NUM 0xaaaaaaaaaaaaaaaaULL
#define DDR_CHANNEL_ADDR_MASK 0xffffffffffffff80ULL

#define DDR_INTR_FUCTION(name) \
	static void ddr_##name##_intr(u32 status, ddr_exception_data *data)
#define DDR_EXCEPTION_INFO(name) static const char *name##_str[]
#define DDR_EXCEPTION(name, intr_status, intr_ack, intr_mask, regs_mask) \
	[name] = { intr_status, intr_ack,                                \
		   intr_mask,	regs_mask,                               \
		   #name,	ddr_##name##_intr,                       \
		   name##_str,	sizeof(name##_str) / sizeof(char *) }

#define DDR_SET_CHANNEL_INVALID(channel) ((channel) = 0xff)
#define DDR_CHANNEL_IS_INVALID(channel) ((channel) == 0xff)

#define MBOX_MSG_OFFSET             (1)
#define MBOX_MSG_LEN                (2)
#define MBOX_SEND_TIMEOUT           (100)
#define FFA_GET_DDR_IRQ_DIS         (0x82000011)

static u32 ddr_ready_msg_send = 0;

typedef enum {
	TIMEOUT = 0,
	ECC,
	LOW_POWER_CONTROL,
	TRAINING = 5,
	USER_INTERFACE,
	MISC_LOGIC,
	BIST_LOGIC,
	DFI = 10,
	DYNAMIC_FREQ_SCAL = 12,
	INITIALIZATION,
	MODE_REGS,
	PARITY,
} ddr_exception_type;

/*Timeout*/
typedef enum {
	TIMEOUT_ZQ_CALLINIT = 7,
	TIMEOUT_ZQ_CALLATCH,
	TIMEOUT_ZQ_CALSTART,
	TIMEOUT_MRR_TEMP = 14,
	TIMEOUT_DQS_WCK,
	TIMEOUT_DFI_UPDATE,
	TIMEOUT_LP_WAKEUP,
	TIMEOUT_LP_WAKEUP2,
	TIMEOUT_AUTO_REFLASH,
	TIMEOUT_MRR_WRITE,
} timeout_bits;

/*ECC*/
typedef enum {
	ECC_CORRECTABLE = 0,
	ECC_MUL_CORRECTABLE,
	ECC_UNCORRECTABLE,
	ECC_MUL_UNCORRECTABLE,
	ECC_WRITEBACK = 6,
	ECC_SCRUB,
	ECC_CORRECTABLE_SCRUB_READ,
	ECC_WRITE_SINGLE_BIT_MRR_M43,
	ECC_WRITE_DOUBLE_BIT_MRR_M43,
	ECC_READ_SINGLE_BIT,
	ECC_READ_DOUBLE_BIT,
	ECC_RMW_READ_DOUBLE_BIT,
} ecc_bits;

/*LOW POWER*/
typedef enum {
	LOWPOWER_OPERA = 0,
	LOWPOWER_TIMEOUT = 3,
} lowpower_bits;

/*TRAINING*/
typedef enum {
	TRAINING_ZQ_CALIBRATION = 11,
	TRAINING_SOFT_WCK,
	TRAINING_WCKI_UPDATE,
	TRAINING_WCKI_OVERFLOW,
	TRAINING_WCKI_OUT,
	TRAINING_WCKO_UPDATE,
	TRAINING_WCKO_OVERFLOW,
	TRAINING_WCKO_OUT,
} training_bits;

/*USERIF*/
typedef enum {
	USERIF_MEM_OUTSIDE = 0,
	USERIF_MEM_MUL_OUTSIDE,
	USERIF_PORT_ERROR,
	USERIF_WRAP_DRAM = 6,
	USERIF_PROGRAM_INVALID,
} userif_bits;

/*MISC*/
typedef enum {
	MISC_MRR_TRAFFIC = 3,
	MISC_CONTROLLER,
	MISC_MRR_CHANGE,
	MISC_TEMP_ALERT,
	MISC_REFRESH_OPERA,
	MISC_WCK_SYNC,
	MISC_JEDEC,
	MISC_SW_REFRESH_OPERA = 11,
} misc_bits;

/*BIST*/
typedef enum {
	BIST_OPERA = 0,
} bist_bits;

/*DFI*/
typedef enum {
	DFI_UPDATE_ERROR = 0,
	DFI_PHY_MASTER_ERROR,
	DFI_BUS_ERROR,
	DFI_STATE_CHANGE,
	DFI_DLL_RESYNC,
	DFI_TIMEOUT,
} dfi_bits;

/*DFS*/
typedef enum {
	DFS_HARD_ENABLE_ERROR = 0,
	DFS_HARD_PHY_NOT_DEASSERT,
	DFS_HARD_INTERFACE,
	DFS_SOFT_ENABLE_ERROR,
	DFS_SOFT_PHY_NOT_DEASSERT,
	DFS_SOFT_INTERFACE,
	DFS_STATE_ERROR,
} dfs_bits;

/*INIT*/
typedef enum {
	INIT_DFI = 0,
	INIT_MC,
	INIT_STATE = 3,
} init_bits;

/*MODE*/
typedef enum {
	MODE_MRR = 0,
	MODE_REQUEST = 2,
	MODE_WRITE,
} mode_bits;

/*PARITY*/
typedef enum {
	PARITY_WRITE = 0,
	PARITY_OVERLAPPING,
} parity_bits;

typedef struct {
	phys_addr_t addr;
	uint8_t channel;
} addr_attr;

typedef struct {
	addr_attr addr[4];
	uint8_t channel;
} addr_trabslate;

typedef struct {
	uint32_t signature;
	uint16_t major_ver;
	uint16_t minor_ver;
	uint8_t ddr_type;
	uint8_t channel_mask;
	uint8_t ranks_per_channel;
	uint8_t reserved0;
	uint32_t total_size;
} ddr_attr;

typedef struct {
	struct list_head list;
	ddr_attr attr;
	uint32_t channel;
	int irq_num;
	phys_addr_t regs_start;
	size_t regs_len;
	const char *name;
	void *vaddr;
} ddr_exception_data;

typedef struct {
	u32 offset;
	u32 mask;
} ddr_regs;

typedef struct ddr_exceptions {
	u32 intr_status;
	u32 intr_ack;
	u32 intr_mask;
	u32 regs_mask;
	char *name;
	void (*fn)(u32 status, ddr_exception_data *data);
	const char **info;
	u32 info_len;
} ddr_exceptions;

static LIST_HEAD(ddr_exception_list);
static DEFINE_SPINLOCK(ddr_lock);

DDR_EXCEPTION_INFO(TIMEOUT) = {
	[0 ... TIMEOUT_ZQ_CALLINIT - 1] = "Reserved",
	[TIMEOUT_ZQ_CALLINIT] =
		"The ZQ cal init, cs, cl, or reset F M timeout has expired.",
	[TIMEOUT_ZQ_CALLATCH] = "The ZQ callatch FM timeout has expired.",
	[TIMEOUT_ZQ_CALSTART] = "The ZQ calstart FM timeout has expired.",
	[TIMEOUT_ZQ_CALSTART + 1 ... TIMEOUT_MRR_TEMP - 1] = "Reserved",
	[TIMEOUT_MRR_TEMP] = "The MRR temperature check FM timeout has expired.",
	[TIMEOUT_DQS_WCK] =
		"A DQS/ WCK oscillator request timeout has been detected.",
	[TIMEOUT_DFI_UPDATE] = "The DFl update FM timeout has expired.",
	[TIMEOUT_LP_WAKEUP... TIMEOUT_LP_WAKEUP2] =
		"The low power interface wakeup timeout has expired.",
	[TIMEOUT_AUTO_REFLASH] =
		"The auto-refresh max deficit timeout has expired.",
	[TIMEOUT_MRR_WRITE] = "The MRR Write link ECC timeout has expired.",
};

DDR_EXCEPTION_INFO(ECC) = {
	[ECC_CORRECTABLE] = "A correctable ECC event has been detected.",
	[ECC_MUL_CORRECTABLE] =
		"Multiple correctable ECC events have been detected.",
	[ECC_UNCORRECTABLE] = "A uncorrectable ECC event has been detected.",
	[ECC_MUL_UNCORRECTABLE] =
		"Multiple uncorrectable ECC events have been detected.",
	[ECC_MUL_UNCORRECTABLE + 1 ... ECC_WRITEBACK - 1] = "Reserved",
	[ECC_WRITEBACK] =
		"One or more ECC writeback commands could not be executed.",
	[ECC_SCRUB] =
		"The scrub operation triggered by setting the ecc_ scrub_ start parameter has completed.",
	[ECC_CORRECTABLE_SCRUB_READ] =
		"An ECC correctable error has been detected in a scrubbing read operation.",
	[ECC_WRITE_SINGLE_BIT_MRR_M43] =
		"A Write Link ECC single-bit error has been detected by the periodic MRR to MR43.",
	[ECC_WRITE_DOUBLE_BIT_MRR_M43] =
		"A Write Link ECC double-bit error has been detected by the periodic MRR to MR43.",
	[ECC_READ_SINGLE_BIT] =
		"A Read Link ECC single-bit error has been detected.",
	[ECC_READ_DOUBLE_BIT] =
		"A Read Link ECC double-bit error has been detected.",
	[ECC_RMW_READ_DOUBLE_BIT] =
		"A RMW Read Link ECC double-bit error has been detected.",
};

DDR_EXCEPTION_INFO(LOW_POWER_CONTROL) = {
	[LOWPOWER_OPERA] = "The low power operation has been completed.",
	[LOWPOWER_OPERA + 1 ... LOWPOWER_TIMEOUT - 1] = "Reserved",
	[LOWPOWER_TIMEOUT] =
		"A Low Power Interface (LPI) timeout error has occurred."
};

DDR_EXCEPTION_INFO(TRAINING) = {
	[0 ... TRAINING_ZQ_CALIBRATION - 1] = "Reserved",
	[TRAINING_ZQ_CALIBRATION] =
		"The ZQ calibration operation has resulted in a status bit being set. \
		Refer to the zq_status_log parameter for more information.",
	[TRAINING_SOFT_WCK] =
		"The software-requested DQS/WCK oscillator measurement has completed.",
	[TRAINING_WCKI_UPDATE] =
		"The DQS/WCKI oscillator has updated the base values.",
	[TRAINING_WCKI_OVERFLOW] =
		"A DQS/WCKI oscillator measurement overflow has been detected.",
	[TRAINING_WCKI_OUT] =
		"A DQS/WCKI oscillator measurement has been detected to be out of variance.",
	[TRAINING_WCKO_UPDATE] =
		"The WCKO oscillator has updated the base values.",
	[TRAINING_WCKO_OVERFLOW] =
		"A WCKO oscillator measurement overflow has been detected.",
	[TRAINING_WCKO_OUT] =
		"A WCKO oscillator measurement has been detected to be out of variance.",
};

DDR_EXCEPTION_INFO(USER_INTERFACE) = {
	[USERIF_MEM_OUTSIDE] =
		"A memory access outside the defined PHYSICAL memory space has occurred.",
	[USERIF_MEM_MUL_OUTSIDE] =
		"Multiple accesses outside the defined PHYSICAL memory space have occurred.",
	[USERIF_PORT_ERROR] = "An error occurred on the port command channel.",
	[USERIF_PORT_ERROR + 1 ... USERIF_WRAP_DRAM - 1] = "Reserved",
	[USERIF_WRAP_DRAM] =
		"A wrap cycle crossing a DRAM page has been detected. \
		This is unsupported and may result in memory data corruption.",
	[USERIF_PROGRAM_INVALID] =
		"The user has programmed an invalid setting associated with \
		core words per burst. Examples: Setting the mem_dp_reduction \
		parameter when burst length is 2 or specifying a 1:2 MC:PHY \
		clock ratio when burst length is 2.",
};

DDR_EXCEPTION_INFO(MISC_LOGIC) = {
	[0 ... MISC_MRR_TRAFFIC - 1] = "Reserved",
	[MISC_MRR_TRAFFIC] =
		"The assertion of the inhibit_dram_cmd parameter has successfully \
		inhibited the command queue and/or MRR traffic.",
	[MISC_CONTROLLER] =
		"The controller has entered the software-requested mode.",
	[MISC_MRR_CHANGE] =
		"The last automatic MRR of MR4 indicated a change in the device \
		temperature or refresh rate (TUF bit set).",
	[MISC_TEMP_ALERT] =
		"A temperature alert condition (low or high temp) has been detected.",
	[MISC_REFRESH_OPERA] =
		"The refresh operation has resulted in a status bit being set.",
	[MISC_WCK_SYNC] =
		"A software-requested WCK Sync ON command or WCK Sync OFF command has completed.",
	[MISC_JEDEC] =
		"JEDEC tAAD (act1 to act2) timing parameter was violated.",
	[MISC_JEDEC + 1 ... MISC_SW_REFRESH_OPERA - 1] = "Reserved",
	[MISC_SW_REFRESH_OPERA] =
		"The sw requested refresh operation has resulted in a status bit being set.",
};

DDR_EXCEPTION_INFO(BIST_LOGIC) = {
	[BIST_OPERA] = "The BIST operation has been completed."
};

DDR_EXCEPTION_INFO(DFI) = {
	[DFI_UPDATE_ERROR] =
		"A DFl update error has occurred. Error information can be found \
		in the update_error_status parameter.",
	[DFI_PHY_MASTER_ERROR] =
		"A DFl PHY Master Interface error has occurred. Error information \
		can be found in the phymstr_error_status parameter.",
	[DFI_BUS_ERROR] = "Error received from the PHY on the DFI bus.",
	[DFI_STATE_CHANGE] =
		"A state change has been detected on the dfi_init_complete signal \
		after initialization.",
	[DFI_DLL_RESYNC] =
		"The user-initiated DLL resynchronization has completed.",
	[DFI_TIMEOUT] =
		"The DFl tinit-complete value has timed out. This value is specified \
		in the tdfi_init_complete param-eter.",
};

DDR_EXCEPTION_INFO(DYNAMIC_FREQ_SCAL) = {
	[DFS_HARD_ENABLE_ERROR] =
		"The DFS request from the hardware interface was ignored because the \
		dfs_enable parameter is cleared to bo, the memory was still initializing, \
		or the DQS oscillator was in progress.",
	[DFS_HARD_PHY_NOT_DEASSERT] =
		"The DFS operation initiated by the hardware interface was terminated \
		because the PHY did not de-assert the dfi_init_complete signal within \
		the time specified in the tdfi_init_start_fN parameter after the controller \
		asserted the dfi_init_start signal during a DFS operation.",
	[DFS_HARD_INTERFACE] =
		"A DFS operation initiated by the hardware interface completed.",
	[DFS_SOFT_ENABLE_ERROR] =
		"The DFS request from software was ignored because the dfs_enable parameter \
		is cleared to 'b0, the memory was still initializing, or the DQS oscillator \
		was in progress.",
	[DFS_SOFT_PHY_NOT_DEASSERT] =
		"The DFS operation initiated by the software interface was terminated because \
		the PHY did not de-assert the dfi_init_complete signal within the time \
		specified in the tdfi_init_start_fN parameter after the controller asserted \
		the dfi_init_start signal during a DFS operation.",
	[DFS_SOFT_INTERFACE] =
		"A DFS operation initiated by software completed.",
	[DFS_STATE_ERROR] =
		"Set when the DFS state machine reaches a state to wait for software and \
		param_wait_for_sw_after_dfs = 1. Clear to zero to move on.",
};

DDR_EXCEPTION_INFO(INITIALIZATION) = {
	[INIT_DFI] = "The memory reset is valid on the DFl bus.",
	[INIT_MC] = "The MC initialization has been completed.",
	[INIT_MC + 1 ... INIT_STATE - 1] = "Reserved",
	[INIT_STATE] =
		"The state machine is in the power on software initialization state during \
		initialization.",
};

DDR_EXCEPTION_INFO(MODE_REGS) = {
	[MODE_MRR] =
		"An MRR error has occurred. Error information can be found in the \
		mrr_error_status parameter.",
	[MODE_MRR + 1 ... MODE_REQUEST - 1] = "Reserved",
	[MODE_REQUEST] =
		"The requested mode register read has completed. The chip and data can be read \
		in the peripheral mrr_data parameter.",
	[MODE_WRITE] =
		"The register interface-initiated mode register write has completed and another \
		mode register write may be issued.",
};

DDR_EXCEPTION_INFO(PARITY) = {
	[PARITY_WRITE] = "A write data parity error has been detected.",
	[PARITY_OVERLAPPING] =
		"An overlapping write data parity error has been detected at the memory controller \
		boundary.",
};

static void translate_2_addr(addr_trabslate *trans)
{
	phys_addr_t addr;

	for (int i = 0; i < 2; i++) {
		addr = trans->addr[i].addr & DDR_CHANNEL_ADDR_MASK;
		trans->addr[i].channel = __sw_hweight64(addr) % 2;
	}
}

static void translate_4_addr(addr_trabslate *trans)
{
	phys_addr_t addr_odd, addr_even;

	for (int i = 0; i < 4; i++) {
		addr_even = (trans->addr[i].addr & DDR_CHANNEL_ADDR_MASK) &
			    ODD_NUM;
		addr_odd = (trans->addr[i].addr & DDR_CHANNEL_ADDR_MASK) &
			   EVEN_NUM;
		trans->addr[i].channel = (__sw_hweight64(addr_even) % 2) |
					 (__sw_hweight64(addr_odd) % 2) << 1;
	}
}

static void translate_addr(phys_addr_t addr, uint32_t type, uint32_t len,
			   ddr_exception_data *data)
{
	phys_addr_t addr_high, addr_low;
	addr_trabslate trans_addr;
	uint32_t index;

	addr_high = addr & DDR_CHANNEL_ADDR_MASK;
	addr_low = addr & ~DDR_CHANNEL_ADDR_MASK;

	switch (data->attr.channel_mask) {
	case DDR_4_CHANNEL:
		trans_addr.addr[0].addr = addr_high << 2 | addr_low;
		trans_addr.addr[1].addr = addr_high << 2 | addr_low | BIT(7);
		trans_addr.addr[2].addr = addr_high << 2 | addr_low | BIT(8);
		trans_addr.addr[3].addr = addr_high << 2 | addr_low | BIT(7) |
					  BIT(8);
		if (DDR_CHANNEL_IS_INVALID(data->channel)) {
			pr_err("err addr: 0x%llx (translate to: 0x%llx or 0x%llx or 0x%llx or 0x%llx), type: 0x%x, len=0x%x\n",
			       addr, trans_addr.addr[0].addr,
			       trans_addr.addr[1].addr, trans_addr.addr[2].addr,
			       trans_addr.addr[3].addr, type, len);
		} else {
			translate_4_addr(&trans_addr);
			for (index = 0; index < 4; index++) {
				if (trans_addr.addr[index].channel ==
				    data->channel) {
					break;
				}
			}
			pr_err("err addr: 0x%llx (translate to: 0x%llx, channel: 0x%x), type: 0x%x, len=0x%x\n",
			       addr, trans_addr.addr[index].addr,
			       trans_addr.addr[index].channel, type, len);
		}
		break;
	case DDR_2_CHANNEL:
		trans_addr.addr[0].addr = addr_high << 1 | addr_low;
		trans_addr.addr[1].addr = addr_high << 1 | addr_low | BIT(7);
		if (DDR_CHANNEL_IS_INVALID(data->channel)) {
			pr_err("err addr: 0x%llx (translate to: 0x%llx or 0x%llx), type: 0x%x, len=0x%x\n",
			       addr, trans_addr.addr[0].addr,
			       trans_addr.addr[1].addr, type, len);
		} else {
			translate_2_addr(&trans_addr);
			for (index = 0; index < 2; index++) {
				if (trans_addr.addr[index].channel ==
				    (data->channel % 2)) {
					break;
				}
			}
			pr_err("err addr: 0x%llx (translate to: 0x%llx, channel: 0x%x), type: 0x%x, len=0x%x\n",
			       addr, trans_addr.addr[index].addr,
			       trans_addr.addr[index].channel, type, len);
		}

		break;
	default:
		pr_err("err_addr: 0x%llx, type: 0x%x, len=0x%x\n", addr, type,
		       len);
		break;
	}
}

DDR_INTR_FUCTION(TIMEOUT)
{
	(void)status;
	(void)data;
}

DDR_INTR_FUCTION(ECC)
{
	u32 reg[2] = { 0 };
	phys_addr_t err_addr = 0;

	if (status & (BIT(ECC_CORRECTABLE) | BIT(ECC_MUL_CORRECTABLE))) {
		reg[0] = readl(data->vaddr + DDR_DRAM_ECC1);
		reg[1] = readl(data->vaddr + DDR_DRAM_ECC2);
	} else if (status &
		   (BIT(ECC_UNCORRECTABLE) | BIT(ECC_MUL_UNCORRECTABLE))) {
		reg[0] = readl(data->vaddr + DDR_DRAM_UECC1);
		reg[1] = readl(data->vaddr + DDR_DRAM_UECC2);
	}
	err_addr = (((u64)reg[1] & 0xff) << 32) | (u64)reg[0];
	if (!err_addr) {
		return;
	}
	translate_addr(err_addr, 0, 0, data);
}

DDR_INTR_FUCTION(LOW_POWER_CONTROL)
{
	(void)status;
	(void)data;
}

DDR_INTR_FUCTION(TRAINING)
{
	(void)status;
	(void)data;
}

DDR_INTR_FUCTION(USER_INTERFACE)
{
	u32 reg[2] = { 0 }, type = 0, len = 0;
	phys_addr_t err_addr = 0;

	if (status & (BIT(USERIF_MEM_OUTSIDE) | BIT(USERIF_MEM_MUL_OUTSIDE))) {
		reg[0] = readl(data->vaddr + DDR_OUT_OF_RANGE1);
		reg[1] = readl(data->vaddr + DDR_OUT_OF_RANGE2);
		err_addr = (((u64)reg[1] & 0xff) << 32) | (u64)reg[0];
		len = (reg[1] >> 8) & 0x1fff;
		type = (reg[1] >> 24) & 0x7f;
	}
	if (!err_addr) {
		return;
	}
	translate_addr(err_addr, type, len, data);
}

DDR_INTR_FUCTION(MISC_LOGIC)
{
	(void)status;
	(void)data;
}

DDR_INTR_FUCTION(BIST_LOGIC)
{
	(void)status;
	(void)data;
}

DDR_INTR_FUCTION(DFI)
{
	(void)status;
	(void)data;
}

DDR_INTR_FUCTION(DYNAMIC_FREQ_SCAL)
{
	(void)status;
	(void)data;
}

DDR_INTR_FUCTION(INITIALIZATION)
{
	(void)status;
	(void)data;
}

DDR_INTR_FUCTION(MODE_REGS)
{
	(void)status;
	(void)data;
}

DDR_INTR_FUCTION(PARITY)
{
	(void)status;
	(void)data;
}

static const ddr_exceptions ddr_exceptions_table[] = {
	DDR_EXCEPTION(TIMEOUT, 0x57c, 0x5a0, 0x5c4, 0xffffffff),
	DDR_EXCEPTION(ECC, 0x580, 0x5a4, 0x5c8, 0xffffffff),
	DDR_EXCEPTION(LOW_POWER_CONTROL, 0x584, 0x5a8, 0x5cc, 0xffff),
	DDR_EXCEPTION(TRAINING, 0x58c, 0x5b0, 0x5d4, 0xffffffff),
	DDR_EXCEPTION(USER_INTERFACE, 0x590, 0x5b4, 0x5d8, 0xffffffff),
	DDR_EXCEPTION(MISC_LOGIC, 0x594, 0x5b8, 0x5dc, 0xffffffff),
	DDR_EXCEPTION(BIST_LOGIC, 0x598, 0x5bc, 0x5e0, 0xff),
	DDR_EXCEPTION(DFI, 0x598, 0x5bc, 0x5e0, 0xff0000),
	DDR_EXCEPTION(DYNAMIC_FREQ_SCAL, 0x59c, 0x5c0, 0x5e4, 0xff),
	DDR_EXCEPTION(INITIALIZATION, 0x59c, 0x5c0, 0x5e4, 0xff00),
	DDR_EXCEPTION(MODE_REGS, 0x59c, 0x5c0, 0x5e4, 0xff0000),
	DDR_EXCEPTION(PARITY, 0x59c, 0x5c0, 0x5e4, 0xff000000),
};

void ddr_exception_show_info(ddr_exception_data *data,
			     const ddr_exceptions *exception, u32 status)
{
	for (int i = 0; i < exception->info_len; i++) {
		if (!(status & (1 << i))) {
			continue;
		}
		pr_err("%s: %s error, offset: 0x%x, mask: 0x%x, IntrStatus: 0x%x\n \
				%s\n",
		       data->name, exception->name, exception->intr_status,
		       exception->regs_mask, status,
		       IS_ERR_OR_NULL(exception->info[i]) ? "None" :
							    exception->info[i]);
	}
}

static void ddr_exception_handle_interrupt(ddr_exception_data *data)
{
	u32 status_master = 0, index = 0, status_group;
	u32 status_regs[PARITY + 1] = { 0 }, ack_regs[PARITY + 1] = { 0 };
	bool intr[PARITY + 1] = { 0 };
	ulong tmp = 0;

	status_master = readl(data->vaddr + DDR_STATUS_MASTER);
	memset(status_regs, 0, sizeof(status_regs));
	memset(intr, 0, sizeof(intr));

	/*Check Interrupt Status*/
	for (int i = 0; i < PARITY + 1; i++) {
		if (!(status_master & (1 << i))) {
			continue;
		}
		intr[i] = true;
		index = (ddr_exceptions_table[i].intr_status -
			 ddr_exceptions_table[0].intr_status) /
			sizeof(u32);
		if (status_regs[index]) {
			continue;
		}
		status_regs[index] = readl(data->vaddr +
					   ddr_exceptions_table[i].intr_status);
	}

	/*Clear Interrupt*/
	memcpy(ack_regs, status_regs, sizeof(ack_regs));
	for (int i = 0; i < PARITY + 1; i++) {
		if (!intr[i]) {
			continue;
		}
		index = (ddr_exceptions_table[i].intr_ack -
			 ddr_exceptions_table[0].intr_ack) /
			sizeof(u32);
		if (ack_regs[index]) {
			writel(ack_regs[index],
			       data->vaddr + ddr_exceptions_table[i].intr_ack);
			ack_regs[index] = 0;
		}
	}

	/*Show Exception Info && Exec Callback*/
	for (int i = 0; i < PARITY + 1; i++) {
		if (!intr[i]) {
			continue;
		}
		index = (ddr_exceptions_table[i].intr_status -
			 ddr_exceptions_table[0].intr_status) /
			sizeof(u32);
		tmp = ddr_exceptions_table[i].regs_mask;
		status_group = status_regs[index] & tmp;
		status_group >>= find_first_bit(
			&tmp, sizeof(ddr_exceptions_table[i].regs_mask) * 8);
		ddr_exception_show_info(data, &ddr_exceptions_table[i],
					status_group);
		if (!IS_ERR_OR_NULL(ddr_exceptions_table[i].fn)) {
			ddr_exceptions_table[i].fn(status_group, data);
		}
	}
}

static irqreturn_t ddr_exception_irq_handler(int irq, void *data)
{
	(void)irq;
	ddr_exception_handle_interrupt(data);
	return IRQ_HANDLED;
}

static int ddr_exception_probe(struct platform_device *pdev)
{
	int err = 0, irq_num = 0;
	struct resource *res = NULL;
	ddr_exception_data *ddr_data = NULL;
	u32 *vaddr = NULL;
	struct mbox_client client;
	struct mbox_chan *channel;
	u32 msg[MBOX_MSG_LEN];

	irq_num = platform_get_irq(pdev, 0);
	res = platform_get_mem_or_io(pdev, 0);
	if (IS_ERR_OR_NULL(res) || irq_num < 0) {
		err = -ENODEV;
		goto out;
	}

	vaddr = ioremap(res->start, resource_size(res));
	if (IS_ERR_OR_NULL(vaddr)) {
		err = -ENOMEM;
		goto out;
	}

	ddr_data = kzalloc(sizeof(*ddr_data), GFP_KERNEL);
	if (IS_ERR_OR_NULL(vaddr)) {
		err = -ENOMEM;
		goto unmap;
	}

	ddr_data->irq_num = irq_num;
	ddr_data->regs_start = res->start;
	ddr_data->regs_len = resource_size(res);
	ddr_data->name = pdev->name;
	ddr_data->vaddr = vaddr;

	res = NULL;
	vaddr = NULL;
	res = platform_get_mem_or_io(pdev, 1);
	if (!IS_ERR_OR_NULL(res)) {
		vaddr = ioremap(res->start, resource_size(res));
		if (!IS_ERR_OR_NULL(vaddr)) {
			memcpy(&ddr_data->attr, vaddr, sizeof(ddr_data->attr));
			iounmap(vaddr);
		}
	}

	if (device_property_read_u32(&pdev->dev, "channel_id",
				     &ddr_data->channel)) {
		pr_warn("%s channel_id not found\n", pdev->name);
		DDR_SET_CHANNEL_INVALID(ddr_data->channel);
	}

	if (ddr_ready_msg_send == 0) {
		client.dev = &pdev->dev;
		client.tx_block = true;
		client.tx_tout = MBOX_SEND_TIMEOUT;
		client.knows_txdone = false;
		channel = mbox_request_channel_byname(&client, "tx4");
		if (IS_ERR(channel)) {
			dev_warn(&pdev->dev, "Failed to request tx4 channel\n");
			goto normal_logic;
		}

		// Send ddr disable msg
		msg[0] = MBOX_MSG_LEN;
		msg[MBOX_MSG_OFFSET] = FFA_GET_DDR_IRQ_DIS;
		err = mbox_send_message(channel, (void *)msg);
		if (err < 0) {
			dev_err(&pdev->dev, "DDR mbox_send_message failed: %d\n", err);
		}

		/* Free the mailbox channel */
		mbox_free_channel(channel);
		ddr_ready_msg_send = 1;
	}

normal_logic:

	err = request_irq(ddr_data->irq_num, ddr_exception_irq_handler,
			  IRQF_SHARED, pdev->name, ddr_data);
	if (err < 0) {
		pr_err("%s request_irq failed\n", pdev->name);
		goto free_data;
	}

	spin_lock(&ddr_lock);
	list_add_tail(&ddr_data->list, &ddr_exception_list);
	spin_unlock(&ddr_lock);
	return 0;

free_data:
	kfree(ddr_data);
unmap:
	iounmap(vaddr);
out:
	return err;
}

static const struct of_device_id ddr_exception_of_match[] = {
	{ .compatible = "cadence,ddr_ctrl" },
	{}
};

static struct platform_driver ddr_exception_driver = {
	.driver		= {
		.name			= "ddr exception",
		.of_match_table		= ddr_exception_of_match,
	},
	.probe		= ddr_exception_probe,
};

int __init ddr_exception_init(void)
{
	pr_info("ddr_exception_init\n");
	platform_driver_register(&ddr_exception_driver);
	return 0;
}

subsys_initcall(ddr_exception_init);