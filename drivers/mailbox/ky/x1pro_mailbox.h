#ifndef __KY_MAILBOX_H__
#define __KY_MAILBOX_H__

#include <linux/kernel.h>
#include <linux/mailbox_controller.h>
#include <linux/spinlock.h>

#define KY_NUM_CHANNELS	4

#define IRQ_NEW_MSG_MASK	0x1
#define IRQ_NOT_FULL_MASK	0x2


/* mailbox register description */
/* mailbox sysconfig */
typedef union mbox_sysconfig {
	unsigned int val;
	struct {
		unsigned int resetn:1;
		unsigned int reserved:31;
	} bits;
} mbox_sysconfig_t;


typedef union mbox_msg {
	unsigned int val;
	struct {
		unsigned int msg:32;
	} bits;
} mbox_msg_t;

typedef union mbox_fifo_status {
	unsigned int val;
	struct {
		unsigned int is_full:1;
		unsigned int reserved:31;
	} bits;
} mbox_fifo_status_t;


typedef union mbox_msg_status {
	unsigned int val;
	struct {
		unsigned int num_msg:4;
		unsigned int reserved:28;
	} bits;
} mbox_msg_status_t;

typedef union mbox_irq_status {
	unsigned int val;
	struct {
		unsigned int new_msg0_status:1;
		unsigned int not_msg0_full:1;
		unsigned int new_msg1_status:1;
		unsigned int not_msg1_full:1;
		unsigned int new_msg2_status:1;
		unsigned int not_msg2_full:1;
		unsigned int new_msg3_status:1;
		unsigned int not_msg3_full:1;
		unsigned int reserved:24;
	} bits;
} mbox_irq_status_t;

typedef union mbox_irq_status_clr {
	unsigned int val;
	struct {
		unsigned int new_msg0_clr:1;
		unsigned int not_msg0_full_clr:1;
		unsigned int new_msg1_clr:1;
		unsigned int not_msg1_full_clr:1;
		unsigned int new_msg2_clr:1;
		unsigned int not_msg2_full_clr:1;
		unsigned int new_msg3_clr:1;
		unsigned int not_msg3_full_clr:1;
		unsigned int reserved:24;
	} bits;
} mbox_irq_status_clr_t;

typedef union mbox_irq_enable_set {
	unsigned int val;
	struct {
		unsigned int new_msg0_irq_en:1;
		unsigned int not_msg0_full_irq_en:1;
		unsigned int new_msg1_irq_en:1;
		unsigned int not_msg1_full_irq_en:1;
		unsigned int new_msg2_irq_en:1;
		unsigned int not_msg2_full_irq_en:1;
		unsigned int new_msg3_irq_en:1;
		unsigned int not_msg3_full_irq_en:1;
		unsigned int reserved:24;
	} bits;
} mbox_irq_enable_set_t;

typedef union mbox_irq_enable_clr {
	unsigned int val;
	struct {
		unsigned int new_msg0_irq_clr:1;
		unsigned int not_msg0_full_irq_clr:1;
		unsigned int new_msg1_irq_clr:1;
		unsigned int not_msg1_full_irq_clr:1;
		unsigned int new_msg2_irq_clr:1;
		unsigned int not_msg2_full_irq_clr:1;
		unsigned int new_msg3_irq_clr:1;
		unsigned int not_msg3_full_irq_clr:1;
		unsigned int reserved:24;
	} bits;
} mbox_irq_enable_clr_t;

typedef struct mbox_irq {
	mbox_irq_status_t irq_status;
	mbox_irq_status_clr_t irq_status_clr;
	mbox_irq_enable_set_t irq_en_set;
	mbox_irq_enable_clr_t irq_en_clr;
} mbox_irq_t;

typedef struct mbox_reg_desc {
	unsigned int mbox_version; /* 0x00 */
	unsigned int reserved0[3]; /* 0x4 0x8 0xc */
	mbox_sysconfig_t mbox_sysconfig; /* 0x10 */
	unsigned int reserved1[11]; /* 0x14, 0x18, 0x1c, 0x20, 0x24, 0x28, 0x2c, 0x30, 0x34, 0x38, 0x3c */
	mbox_msg_t mbox_msg[4]; /* 0x40, 0x44, 0x48, 0x4c */
	unsigned int reserved2[12];
	mbox_fifo_status_t fifo_status[4]; /* 0x80, 0x84, 0x88, 0x8c */
	unsigned int reserved3[12];
	mbox_msg_status_t msg_status[4]; /* 0xc0 */
	unsigned int reserved4[12];
	mbox_irq_t mbox_irq[2]; /* 0x100 */
} mbox_reg_desc_t;

struct ky_mailbox {
	struct mbox_controller controller;
	struct clk *clk;
	struct reset_control *reset;
	mbox_reg_desc_t *regs;
	spinlock_t lock;
};

#endif /* __KY_MAILBOX_H__ */
