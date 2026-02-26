/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2025 - 2028 Allwinner Technology Co.,Ltd. All rights reserved. */
#ifndef _SUNXI_CC_LOGIC_H
#define _SUNXI_CC_LOGIC_H

#include "sunxi-power-supply.h"
#include "sunxi-power-regulator.h"
#include "axp2101.h"

#include <linux/usb/role.h>

#define TYPEC_STATE(val)			((val) & 0x0F)
#define POWER_STATE(val)		(((val) & 0x30) >> 4)
#define FLAG_STATE(val)			(((val) & 0x40) >> 6)

#define STATE_MACHINE_LOOP_TIMES	(15) /* frequency */
#define STATE_MACHINE_DELAY_TIME	(10) /* ms */
#define TCPM_DEBOUNCE_MS		(500) /* ms */

/* The State of CC Logic in HW mode */
enum sunxi_cc_logic_state {
	INVALID_STATE,		/* 0000: DISABLE  */
	SNK_UNATTACHED,		/* 0001: UNATTACH_SNK */
	SNK_ATTACH_WAIT,	/* 0010: ATTACHWAIT_SNK */
	SNK_ATTACHED,		/* 0011: ATTACH_SNK */
	SRC_UNATTACHED,		/* 0100: UNATTACH_SRC */
	SRC_ATTACH_WAIT,	/* 0101: ATTACHWAIT */
	SRC_ATTACHED,		/* 0110: ATTACH_SRC */
	AUDIO_ACC_ATTACHED,	/* 0111: AUDIO_ACSY */
	RESERVED0,		/* 1000: Reserved */
	SRC_TRY,		/* 1001: TRY_SRC */
	SNK_TRYWAIT,		/* 1010: TRYWAIT_SNK */
	SNK_TRY,		/* 1011: TRY_SNK */
	SRC_TRYWAIT,		/* 1100: TRYWAIT_SRC */
	RESERVED1,		/* 1101: Reserved */
	ERROR_RECOVERY,		/* 1110: ERROR_RECOVERY */
	RESERVED2,		/* 1111: Reserved */
};

static const char * const sunxi_cc_logic_states[] = {
	"INVALID_STATE",
	"SNK_UNATTACHED",
	"SNK_ATTACH_WAIT",
	"SNK_ATTACHED",
	"SRC_UNATTACHED",
	"SRC_ATTACH_WAIT",
	"SRC_ATTACHED",
	"AUDIO_ACC_ATTACHED",
	"RESERVED0",
	"SRC_TRY",
	"SNK_TRYWAIT",
	"SNK_TRY",
	"SRC_TRYWAIT",
	"RESERVED1",
	"ERROR_RECOVERY",
	"RESERVED2",
};

enum power_state {
	POWER_IDLE,
	POWER_DEF,
	POWER_1_5,
	POWER_3_0,
};

enum flag_state {
	ACITVE_CC2,
	ACITVE_CC1,
};

struct sunxi_pmic_interrupts {
	char *name;
	irq_handler_t isr;
	int irq;
};

struct sunxi_pmic_cc_gpio_para {
	int gpio;
	int irq_num;
};

enum sunxi_pmic_cc_logic_virq_index {
	SUNXI_PMIC_CC_LOGIC_VIRQ_TYPEC_IN,
	SUNXI_PMIC_CC_LOGIC_VIRQ_TYPEC_OUT,
	SUNXI_PMIC_CC_LOGIC_VIRQ_PWR_CHNG,
	SUNXI_PMIC_CC_LOGIC_VIRQ_MAX_VIRQ,
};

struct sunxi_pmic_cc_logic;

struct sunxi_pmic_cc_logic_data {
	unsigned int cc_status_reg;
	unsigned int cc_flag_reg;
	unsigned int cc_mode_ctrl_reg;
	const char *irq_names[SUNXI_PMIC_CC_LOGIC_VIRQ_MAX_VIRQ];
	int (*init_regs)(struct sunxi_pmic_cc_logic *port, bool enable);
	int (*vbus_offline_process)(struct sunxi_pmic_cc_logic *port);
	int (*reset_cc)(struct sunxi_pmic_cc_logic *port);
};

struct sunxi_pmic_cc_logic {
	struct device *dev;
	struct regmap *regmap;
	struct regulator *vbus;
	struct usb_role_switch *role_sw;

	struct sunxi_pmic_cc_logic_data cc_logic_data;
	enum sunxi_cc_logic_state prev_sunxi_cc_logic_state;
	enum sunxi_cc_logic_state sunxi_cc_logic_state;

	enum power_state power_state;
	enum flag_state flag_state;

	struct power_supply *usb_power_psy;
	struct power_supply *cc_logic_psy;

	struct extcon_dev *edev;
	struct notifier_block sunxi_pmic_cc_logic_nb;

	struct sunxi_pmic_cc_gpio_para      usbid_drv;
	struct delayed_work       vbus_online_mon;

	bool vbus_on;
	bool vbus_vsafe0v;
	bool registered;

	unsigned int hrtimer_interval;
	unsigned int state_machine_times;
	struct mutex lock; /* state machine lock */
	struct mutex vbus_lock;

	struct kthread_worker *wq;
	struct kthread_work event_work;
	struct hrtimer state_machine_timer;
	struct kthread_work state_machine;

	unsigned long debounce_jiffies;
	struct delayed_work wq_detcable;
	bool state_machine_running;
	enum usb_role current_usb_role;
	atomic_t set_vbus_online;
	unsigned int vsafe0v_delay;
	unsigned int cc_default_cur;

	atomic_t current_limit_ma;
	struct sunxi_power_debug_data *debug;
};

#endif	/* _SUNXI_CC_LOGIC_H */
