#ifndef _CMDQ_H_
#define _CMDQ_H_

enum {
	CMDQ_MODE_SDMA,
	CMDQ_MODE_ADMA,
};

struct cmdq_adma {
	unsigned long long addr;
	unsigned int size;
	unsigned int flags_end : 1;
	unsigned int rsv : 2;
	unsigned int flags_link : 1;
	unsigned int rsv2 : 28;
};

enum {
	CMDQ_SET_REG,
	CMDQ_SET_WAIT_TIMER,
	CMDQ_SET_WAIT_FLAG,
};

struct cmdq_set_reg {
	unsigned int data;
	unsigned int addr : 20;
	unsigned int byte_mask : 4;
	unsigned int intr_end : 1;
	unsigned int intr_int : 1;
	unsigned int intr_last : 1;
	unsigned int intr_rsv : 1;
	unsigned int action : 4;  // 0 for this case
};

struct cmdq_set_wait_timer {
	unsigned int counter;
	unsigned int rsv : 24;
	unsigned int intr_end : 1;
	unsigned int intr_int : 1;
	unsigned int intr_last : 1;
	unsigned int intr_rsv : 1;
	unsigned int action : 4;  // 1 for this case
};

struct cmdq_set_wait_flags {
	unsigned int flag_num;   // 0 ~ 15, depending on each module
	unsigned int rsv : 24;
	unsigned int intr_end : 1;
	unsigned int intr_int : 1;
	unsigned int intr_last : 1;
	unsigned int intr_rsv : 1;
	unsigned int action : 4;  // 2 for this case
};

union cmdq_set {
	struct cmdq_set_reg reg;
	struct cmdq_set_wait_timer wait_timer;
	struct cmdq_set_wait_flags wait_flags;
};

/**
 * cmdq_set_package  - package reg_write to cmd_set.
 *
 */
void cmdq_set_package(struct cmdq_set_reg *set, unsigned int addr, unsigned int data);

/**
 * cmdq_set_package  - package reg_write to cmd_set.
 *
 * @param set: the set to modify
 * @param is_timer: 1: wait_timer, 0: wait_flag
 * @param data: the data of wait condition.
 *		counter if timer and flag_num if flag
 * @param intr: the interrupt condition
 */
void cmdq_set_wait(union cmdq_set *set, bool is_timer, unsigned int data, unsigned char intr);

/**
 * cmdq_adma_package  - package adma entries.
 *
 * @param item: adma entry to modify
 * @param addr: address of link/cmd_set
 * @param size: size(bytes) if only this is cmd_set
 * @param is_link: 1: link descriptor, 2: cmd_set
 * @param is_end: true if this is last entry in adma-table.
 */
void cmdq_adma_package(struct cmdq_adma *item, unsigned long long addr, unsigned int size, bool is_link,
		       bool is_end);

/**
 * cmdq_intr_ctrl - cmdQ's interrupt on(1)/off(0)
 *                 bit0: cmdQ intr, bit1: cmdQ end, bit2: cmdQ wait
 *
 * @param intr_mask: On/Off ctrl of the interrupt.
 */
void cmdq_intr_ctrl(uintptr_t base, unsigned char intr_mask);

/**
 * cmdq_intr_ctrl - clear cmdQ's interrupt
 *                 bit0: cmdQ intr, bit1: cmdQ end, bit2: cmdQ wait
 *
 * @param base: base-address of cmdQ
 * @param intr_mask: the mask of the interrupt to clear.
 */
void cmdq_intr_clr(uintptr_t base, unsigned char intr_mask);

/**
 * cmdq_intr_status - cmdQ's interrupt status
 *                 bit0: cmdQ intr, bit1: cmdQ end, bit2: cmdQ wait
 *
 * @param base: base-address of cmdQ
 */
unsigned char cmdq_intr_status(uintptr_t base);

/**
 * cmdq_engine - start cmdQ
 *
 * @param base: base-address of cmdQ
 * @param adma_addr: adma table's address
 * @param apb_base: relative IP's apb_base_address, whose MSB 10bits needed only
 * @param is_hw_restart: wait_flag is waiting for hw(1) or sw(0).
 * @param is_adma: 1 if adma table is used.
 * @param cnt: the number of entry in cmdset. only useful if adma
 */
void cmdq_engine(uintptr_t base, uintptr_t tbl_addr, unsigned short apb_base,
		 bool is_hw_restart, bool is_adma, unsigned short cnt);

/**
 * cmdq_sw_restart - toggle sw_restart if cmdQ wait-flag is sw-toggle.
 *
 * @param base: base-address of cmdQ
 */
void cmdq_sw_restart(uintptr_t base);

bool cmdq_is_sw_restart(uintptr_t base);

#endif  //_CMDQ_H_
