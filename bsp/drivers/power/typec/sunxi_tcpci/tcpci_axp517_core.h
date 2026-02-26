/* SPDX-License-Identifier: GPL-2.0+ */
/* Copyright(c) 2024 - 2025 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Copyright 2015-2017 Google, Inc
 *
 * USB Type-C Port Controller Interface.
 */

#ifndef __AW_AXP517_TCPCI_H
#define __AW_AXP517_TCPCI_H

#include "sunxi-power-supply.h"
#include "sunxi-power-notifier.h"
#include "sunxi-power-typec.h"
#include "axp2101.h"

#define AXP517_TCPC_ROLE_CTRL_SET(drp, rp, cc1, cc2) \
	((drp) << 6 | (rp) << 4 | (cc2) << 2 | (cc1))

/* Resources */
#define AXP517_AXP517_TCPC_MAX_IRQS			(0x10)
#define AXP517_TCPM_DEBOUNCE_MS			500 /* ms */

#define AXP517_TCPC_RECEIVE_BUFFER_LEN				32
#define AXP517_TCPC_RECEIVE_BUFFER_COUNT_OFFSET		0
#define AXP517_TCPC_RECEIVE_BUFFER_FRAME_TYPE_OFFSET		1
#define AXP517_TCPC_RECEIVE_BUFFER_RX_BYTE_BUF_OFFSET		2

#define axp517_tcpci_cc_is_sink(cc) \
	((cc) == TYPEC_CC_RP_DEF || (cc) == TYPEC_CC_RP_1_5 || \
	 (cc) == TYPEC_CC_RP_3_0)

/* As long as cc is pulled up, we can consider it as sink. */
#define axp517_tcpci_port_is_sink(cc1, cc2) \
	(axp517_tcpci_cc_is_sink(cc1) || axp517_tcpci_cc_is_sink(cc2))

#define axp517_tcpci_cc_is_source(cc) ((cc) == TYPEC_CC_RD)
#define axp517_tcpci_cc_is_audio(cc) ((cc) == TYPEC_CC_RA)
#define axp517_tcpci_cc_is_open(cc) ((cc) == TYPEC_CC_OPEN)

#define axp517_tcpci_port_is_source(cc1, cc2) \
	((axp517_tcpci_cc_is_source(cc1) && !axp517_tcpci_cc_is_source(cc2)) || \
	 (axp517_tcpci_cc_is_source(cc2) && !axp517_tcpci_cc_is_source(cc1)))

#define axp517_tcpci_port_is_audio(cc1, cc2) \
	(axp517_tcpci_cc_is_audio(cc1) && axp517_tcpci_cc_is_audio(cc2))

#define axp517_tcpci_port_is_open(cc1, cc2) \
	(axp517_tcpci_cc_is_open(cc1) && axp517_tcpci_cc_is_open(cc2))

#define axp517_tcpci_port_is_close(cc1, cc2) \
	(!axp517_tcpci_cc_is_open(cc1) && !axp517_tcpci_cc_is_open(cc2))

/* regs  */
/* reg AXP517_COMM_STAT1  */
#define AXP517_PWR_OK				BIT(4)

/* reg AXP517_CLK_EN  */
#define AXP517_CC_CLK_EN			BIT(3)

/* reg AXP517_MODULE_EN  */
#define AXP517_BOOST_EN				BIT(4)

/* reg AXP517_TCPC_VENDOR_ID  */
#define AXP517_VENDOR_ID			(0x1F3A)

/* reg AXP517_AWAKE_EN  */
#define AXP517_AWAKE_MODE			BIT(7)
#define AXP517_AWAKE_STATE			GENMASK(6, 5)
#define AXP517_HARD_AWAKE_EN			BIT(1)
#define AXP517_SOFT_AWAKE_EN			BIT(0)

/* reg AXP517_PD_STATE  */
#define AXP517_AWAKE_SEL			BIT(3)

/* reg AXP517_CC_GENERAL_CONTROL  */
#define AXP517_PD_RX_GATE_EN			BIT(7)
#define AXP517_PD_TXRX_RESET			BIT(7)
#define AXP517_GLOBAL_SW_RESET			BIT(5)
#define AXP517_DRP_DUTY_CYCLE_MASK		GENMASK(1, 0)
/* Percent of time that DRP advertises DFP during tDRP */
#define AXP517_DRP_DUTY_CYCLE(n)		((n) & 0x3)

/* reg AXP517_PHY_BMC_TX_CTRL  */
#define AXP517_TX_CARRIER_MODE2_SEL		BIT(3)
#define AXP517_TX_FAST_ROLE_SWAP		BIT(2)
#define AXP517_TX_FAST_ROLE_RX_EN		BIT(1)

/* reg AXP517_VBUS_CC_PERIOD_FREQ  */
#define AXP517_CONFIG_OCP			BIT(4)
#define AXP517_ADC_PERIOD_MASK			GENMASK(3, 2)
#define AXP517_ADC_PERIOD(n)			((n) & 0x3)
#define AXP517_VBUS_DETECT_FREQUENCY_MASK	GENMASK(1, 0)
#define AXP517_VBUS_DETECT_FREQUENCY(n)		((n) & 0x3)

/* reg AXP517_TWI_ADDR_STATIC  */
#define AXP517_I2C_ADDR_STATIC			BIT(0)

/* reg AXP517_CC_CNNT_STA  */
#define AXP517_MASK_VBUS_STAT_BY_TYPEC			(7 << 5)
#define AXP517_VBUS_STAT_BY_TYPEC_EXIST_MAX		(3 << 5)
#define AXP517_TYPEC_STAT_DEBUG					(7 << 5)

/* AXP517_IRQ_PD_ALERTL_STATUS  */
#define AXP517_IRQ_PD_ALERTL_STATUS_EXTND		BIT(14)
#define AXP517_IRQ_PD_ALERTL_STATUS_EXTENDED_STATUS	BIT(13)
#define AXP517_IRQ_PD_ALERTL_STATUS_VBUS_DISCNCT		BIT(11)
#define AXP517_IRQ_PD_ALERTL_STATUS_RX_BUF_OVF		BIT(10)
#define AXP517_IRQ_PD_ALERTL_STATUS_FAULT		BIT(9)
#define AXP517_IRQ_PD_ALERTL_STATUS_V_ALARM_LO		BIT(8)
#define AXP517_IRQ_PD_ALERTL_STATUS_V_ALARM_HI		BIT(7)
#define AXP517_IRQ_PD_ALERTL_STATUS_TX_SUCCESS		BIT(6)
#define AXP517_IRQ_PD_ALERTL_STATUS_TX_DISCARDED		BIT(5)
#define AXP517_IRQ_PD_ALERTL_STATUS_TX_FAILED		BIT(4)
#define AXP517_IRQ_PD_ALERTL_STATUS_RX_HARD_RST		BIT(3)
#define AXP517_IRQ_PD_ALERTL_STATUS_RX_STATUS		BIT(2)
#define AXP517_IRQ_PD_ALERTL_STATUS_POWER_STATUS		BIT(1)
#define AXP517_IRQ_PD_ALERTL_STATUS_CC_STATUS		BIT(0)

/* AXP517_TCPC_EXTENDED_STATUS_MASK  */
#define AXP517_TCPC_EXTENDED_STATUS_MASK_VSAFE0V	BIT(0)

/* AXP517_TCPC_ALERT_EXTENDED_MASK  */
#define AXP517_TCPC_SINK_FAST_ROLE_SWAP	BIT(0)

/* AXP517_TCPC_CTRL  */
#define AXP517_TCPC_CTRL_ORIENTATION	BIT(0)
#define PLUG_ORNT_CC1			0
#define PLUG_ORNT_CC2			1
#define AXP517_TCPC_CTRL_BIST_TM		BIT(1)
#define AXP517_TCPC_CTRL_EN_LK4CONN_ALRT	BIT(6)

/* AXP517_TCPC_EXTENDED_STATUS  */
#define AXP517_TCPC_EXTENDED_STATUS_VSAFE0V	BIT(0)

/* AXP517_TCPC_ROLE_CTRL  */
#define AXP517_TCPC_ROLE_CTRL_DRP		BIT(6)
#define AXP517_TCPC_ROLE_CTRL_RP_VAL_SHIFT	4
#define AXP517_TCPC_ROLE_CTRL_RP_VAL_MASK	0x3
#define AXP517_TCPC_ROLE_CTRL_RP_VAL_DEF	0x0
#define AXP517_TCPC_ROLE_CTRL_RP_VAL_1_5	0x1
#define AXP517_TCPC_ROLE_CTRL_RP_VAL_3_0	0x2
#define AXP517_TCPC_ROLE_CTRL_CC2_SHIFT	2
#define AXP517_TCPC_ROLE_CTRL_CC2_MASK		0x3
#define AXP517_TCPC_ROLE_CTRL_CC1_SHIFT	0
#define AXP517_TCPC_ROLE_CTRL_CC1_MASK		0x3
#define AXP517_TCPC_ROLE_CTRL_CC_RA		0x0
#define AXP517_TCPC_ROLE_CTRL_CC_RP		0x1
#define AXP517_TCPC_ROLE_CTRL_CC_RD		0x2
#define AXP517_TCPC_ROLE_CTRL_CC_OPEN		0x3

/* AXP517_TCPC_POWER_CTRL  */
#define AXP517_TCPC_POWER_CTRL_VCONN_ENABLE	BIT(0)
#define AXP517_TCPC_POWER_CTRL_BLEED_DISCHARGE	BIT(3)
#define AXP517_TCPC_POWER_CTRL_AUTO_DISCHARGE	BIT(4)
#define AXP517_TCPC_DIS_VOLT_ALRM		BIT(5)
#define AXP517_TCPC_POWER_CTRL_VBUS_VOLT_MON	BIT(6)
#define AXP517_TCPC_FAST_ROLE_SWAP_EN		BIT(7)

/* AXP517_TCPC_CC_STATUS  */
#define AXP517_TCPC_CC_STATUS_TOGGLING		BIT(5)
#define AXP517_TCPC_CC_STATUS_TERM		BIT(4)
#define AXP517_TCPC_CC_STATUS_TERM_RP		0
#define AXP517_TCPC_CC_STATUS_TERM_RD		1
#define AXP517_TCPC_CC_STATE_SRC_OPEN		0
#define AXP517_TCPC_CC_STATUS_CC2_SHIFT	2
#define AXP517_TCPC_CC_STATUS_CC2_MASK		0x3
#define AXP517_TCPC_CC_STATUS_CC1_SHIFT	0
#define AXP517_TCPC_CC_STATUS_CC1_MASK		0x3

/* AXP517_TCPC_POWER_STATUS  */
#define AXP517_TCPC_POWER_STATUS_DBG_ACC_CON	BIT(7)
#define AXP517_TCPC_POWER_STATUS_UNINIT	BIT(6)
#define AXP517_TCPC_POWER_STATUS_SOURCING_VBUS	BIT(4)
#define AXP517_TCPC_POWER_STATUS_VBUS_DET	BIT(3)
#define AXP517_TCPC_POWER_STATUS_VBUS_PRES	BIT(2)
#define AXP517_TCPC_POWER_STATUS_VCONN_PRES	BIT(1)
#define AXP517_TCPC_POWER_STATUS_SINKING_VBUS	BIT(0)

/* AXP517_TCPC_FAULT_STATUS  */
#define AXP517_TCPC_FAULT_STATUS_ALL_REG_RST_TO_DEFAULT BIT(7)

/* AXP517_TCPC_COMMAND  */
#define AXP517_TCPC_CMD_WAKE_I2C		0x11
#define AXP517_TCPC_CMD_DISABLE_VBUS_DETECT	0x22
#define AXP517_TCPC_CMD_ENABLE_VBUS_DETECT	0x33
#define AXP517_TCPC_CMD_DISABLE_SINK_VBUS	0x44
#define AXP517_TCPC_CMD_SINK_VBUS		0x55
#define AXP517_TCPC_CMD_DISABLE_SRC_VBUS	0x66
#define AXP517_TCPC_CMD_SRC_VBUS_DEFAULT	0x77
#define AXP517_TCPC_CMD_SRC_VBUS_HIGH		0x88
#define AXP517_TCPC_CMD_LOOK4CONNECTION	0x99
#define AXP517_TCPC_CMD_RXONEMORE		0xAA
#define AXP517_TCPC_CMD_I2C_IDLE		0xFF

/* AXP517_TCPC_MSG_HDR_INFO  */
#define AXP517_TCPC_MSG_HDR_INFO_DATA_ROLE	BIT(3)
#define AXP517_TCPC_MSG_HDR_INFO_PWR_ROLE	BIT(0)
#define AXP517_TCPC_MSG_HDR_INFO_REV_SHIFT	1
#define AXP517_TCPC_MSG_HDR_INFO_REV_MASK	0x3

/* AXP517_TCPC_RX_DETECT  */
#define AXP517_TCPC_RX_DETECT_HARD_RESET	BIT(5)
#define AXP517_TCPC_RX_DETECT_SOP		BIT(0)
#define AXP517_TCPC_RX_DETECT_SOP1		BIT(1)
#define AXP517_TCPC_RX_DETECT_SOP2		BIT(2)
#define AXP517_TCPC_RX_DETECT_DBG1		BIT(3)
#define AXP517_TCPC_RX_DETECT_DBG2		BIT(4)

/* AXP517_TCPC_RX_BYTE_CNT  */
#define AXP517_TCPC_RX_BUF_FRAME_TYPE		0x31
#define AXP517_TCPC_RX_BUF_FRAME_TYPE_SOP	0
#define AXP517_TCPC_RX_BUF_FRAME_TYPE_SOP1	1
#define AXP517_TCPC_RX_HDR			0x32
#define AXP517_TCPC_RX_DATA			0x34 /* through 0x4f */

/* AXP517_TCPC_TRANSMIT  */
#define AXP517_TCPC_TRANSMIT_RETRY_SHIFT	4
#define AXP517_TCPC_TRANSMIT_RETRY_MASK	0x3
#define AXP517_TCPC_TRANSMIT_TYPE_SHIFT	0
#define AXP517_TCPC_TRANSMIT_TYPE_MASK		0x7

/* AXP517_TCPC_TX_BYTE_CNT  */
#define AXP517_TCPC_TX_HDR			0x52
#define AXP517_TCPC_TX_DATA			0x54 /* through 0x6f */

/* AXP517_TCPC_VBUS_VOLTAGE  */
#define AXP517_TCPC_VBUS_VOLTAGE_MASK			0x3ff
#define AXP517_TCPC_VBUS_VOLTAGE_LSB_MV		25

/* AXP517_TCPC_VBUS_SINK_DISCONNECT_THRESH  */
#define AXP517_TCPC_VBUS_SINK_DISCONNECT_THRESH_LSB_MV	25
#define AXP517_TCPC_VBUS_SINK_DISCONNECT_THRESH_MAX	0x3ff

/* I2C_SMBUS_BLOCK_MAX + 1 when RX/TX_BUF_BYTE_x is only accessible I2C_SMBUS_BLOCK_MAX */
#define AXP517_TCPC_TRANSMIT_BUFFER_MAX_LEN		31
#define AXP517_TCPC_RECEIVED_BUFFER_MAX_LEN		32

#define tcpc_presenting_rd(reg, cc) \
	(!(AXP517_TCPC_ROLE_CTRL_DRP & (reg)) && \
	 (((reg) & (AXP517_TCPC_ROLE_CTRL_## cc ##_MASK << AXP517_TCPC_ROLE_CTRL_## cc ##_SHIFT)) == \
	  (AXP517_TCPC_ROLE_CTRL_CC_RD << AXP517_TCPC_ROLE_CTRL_## cc ##_SHIFT)))

struct tcpci {
	struct device *dev;

	struct tcpm_port *port;

	struct regmap *regmap;
	unsigned int alert_mask;

	bool controls_vbus;

	struct tcpc_dev tcpc;
	struct axp517_tcpci_data *data;
};

struct tcpm_port;

/*
 * @RX_BUF_BYTE_x_hidden:
 *		optional; Set when RX_BUF_BYTE_x can only be accessed through I2C_SMBUS_BLOCK_MAX.
 * @TX_BUF_BYTE_x_hidden:
 *		optional; Set when TX_BUF_BYTE_x can only be accessed through I2C_SMBUS_BLOCK_MAX.
 * @RX_TX_FIFO_supported:
 *		optional; Set when RX/TX_FIFO can only be accessed through regmap_noinc_read
 *		or regmap_noinc_write.
 * @frs_sourcing_vbus:
 *		Optional; Callback to perform chip specific operations when FRS
 *		is sourcing vbus.
 * @auto_discharge_disconnect:
 *		Optional; Enables TCPC to autonously discharge vbus on disconnect.
 * @vbus_vsafe0v:
 *		optional; Set when TCPC can detect whether vbus is at VSAFE0V.
 * @vbus_floated:
 *		optional; Set when HW can support Multi-Port Charging and use External Vbus.
 * @set_partner_usb_comm_capable:
 *		Optional; The USB Communications Capable bit indicates if port
 *		partner is capable of communication over the USB data lines
 *		(e.g. D+/- or SS Tx/Rx). Called to notify the status of the bit.
 * @check_contaminant:
 *		Optional; The callback is invoked when chiplevel drivers indicated
 *		that the USB port needs to be checked for contaminant presence.
 *		Chip level drivers are expected to check for contaminant and call
 *		tcpm_clean_port when the port is clean to put the port back into
 *		toggling state.
 * @process_rx:
 *		Optional; The TCPC can process vendor specific reception of PD messages.
 */
struct axp517_tcpci_data {
	struct regmap *regmap;
	unsigned char RX_BUF_BYTE_x_hidden:1;
	unsigned char TX_BUF_BYTE_x_hidden:1;
	unsigned char RX_TX_FIFO_supported:1;
	unsigned char auto_discharge_disconnect:1;
	unsigned char vbus_vsafe0v:1;
	unsigned char vbus_floated:1;
	unsigned char self_powered:1;

	int (*init)(struct tcpci *tcpci, struct axp517_tcpci_data *data);
	int (*set_vconn)(struct tcpci *tcpci, struct axp517_tcpci_data *data,
			 bool enable);
	int (*start_drp_toggling)(struct tcpci *tcpci, struct axp517_tcpci_data *data,
				  enum typec_cc_status cc);
	int (*set_vbus)(struct tcpci *tcpci, struct axp517_tcpci_data *data, bool source, bool sink);
	void (*frs_sourcing_vbus)(struct tcpci *tcpci, struct axp517_tcpci_data *data);
	void (*set_partner_usb_comm_capable)(struct tcpci *tcpci, struct axp517_tcpci_data *data,
					     bool capable);
	void (*check_contaminant)(struct tcpci *tcpci, struct axp517_tcpci_data *data);
	void (*process_rx)(struct tcpci *tcpci, u16 status);
};

struct irq_params {
	int virq;
	char *irq_name;
};

struct axp517_tcpc_resources {
	unsigned int nr_irqs;
	struct irq_params irq_params[AXP517_AXP517_TCPC_MAX_IRQS];
};

struct axp517_tcpci_chip {
	struct axp517_tcpci_data data;
	struct tcpci *tcpci;
	struct device *dev;
	struct regulator *vbus;
	struct power_supply *usb_psy;
	struct usb_role_switch *role_sw;
	unsigned long debounce_jiffies;
	struct delayed_work wq_detcable;
	struct extcon_dev *charger_edev;
	struct notifier_block charger_nb;
	struct extcon_dev *edev;

	bool vbus_on;
	bool port_reset_quirk;
	bool vbus_float_quirk;
	bool battery_exist;
	u32 current_limit;

	u16 vendor_id;
	struct delayed_work  vbus_check_mon;
	struct delayed_work  power_save_mon;
	struct delayed_work  resume_mon;
};

struct tcpci *axp517_tcpci_register_port_overrides(struct device *dev, struct axp517_tcpci_data *data);
void axp517_tcpci_unregister_port_overrides(struct tcpci *tcpci);
irqreturn_t axp517_tcpci_irq_overrides(struct tcpci *tcpci);
struct tcpm_port *axp517_tcpci_get_tcpm_port_overrides(struct tcpci *tcpci);
int axp517_tcpci_get_cc(struct tcpc_dev *tcpc, enum typec_cc_status *cc1, enum typec_cc_status *cc2);


#endif /* __AW_AXP517_TCPCI_H */
