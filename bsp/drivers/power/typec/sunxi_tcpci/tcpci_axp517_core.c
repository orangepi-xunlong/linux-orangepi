// SPDX-License-Identifier: GPL-2.0+
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Copyright 2015-2017 Google, Inc
 *
 * USB Type-C Port Controller Interface.
 */

#include "tcpci_axp517_core.h"

#define	PD_RETRY_COUNT_DEFAULT			3
#define	PD_RETRY_COUNT_3_0_OR_HIGHER		2
#define	AUTO_DISCHARGE_DEFAULT_THRESHOLD_MV	3500
#define	VSINKPD_MIN_IR_DROP_MV			750
#define	VSRC_NEW_MIN_PERCENT			95
#define	VSRC_VALID_MIN_MV			500
#define	VPPS_NEW_MIN_PERCENT			95
#define	VPPS_VALID_MIN_MV			100
#define	VSINKDISCONNECT_PD_MIN_PERCENT		90

struct tcpm_port *axp517_tcpci_get_tcpm_port_overrides(struct tcpci *tcpci)
{
	return tcpci->port;
}

struct axp517_tcpci_chip *tdata_to_axp517(struct axp517_tcpci_data *tdata)
{
	return container_of(tdata, struct axp517_tcpci_chip, data);
}

struct tcpci *axp517_tcpc_to_tcpci(struct tcpc_dev *tcpc)
{
	return container_of(tcpc, struct tcpci, tcpc);
}

int axp517_tcpci_read8(struct tcpci *tcpci, unsigned int reg, u8 *val)
{
	return regmap_raw_read(tcpci->regmap, reg, val, sizeof(u8));
}

int axp517_tcpci_write8(struct tcpci *tcpci, unsigned int reg, u8 val)
{
	return regmap_raw_write(tcpci->regmap, reg, &val, sizeof(u8));
}

int axp517_tcpci_read16(struct tcpci *tcpci, unsigned int reg, u16 *val)
{
	return regmap_raw_read(tcpci->regmap, reg, val, sizeof(u16));
}

int axp517_tcpci_write16(struct tcpci *tcpci, unsigned int reg, u16 val)
{
	return regmap_raw_write(tcpci->regmap, reg, &val, sizeof(u16));
}

static enum typec_cc_status axp517_tcpci_to_typec_cc(unsigned int cc, bool sink)
{
	switch (cc) {
	case 0x1:
		return sink ? TYPEC_CC_RP_DEF : TYPEC_CC_RA;
	case 0x2:
		return sink ? TYPEC_CC_RP_1_5 : TYPEC_CC_RD;
	case 0x3:
		if (sink)
			return TYPEC_CC_RP_3_0;
		fallthrough;
	case 0x0:
	default:
		return TYPEC_CC_OPEN;
	}
}

int axp517_tcpci_get_cc(struct tcpc_dev *tcpc,
			enum typec_cc_status *cc1, enum typec_cc_status *cc2)
{
	struct tcpci *tcpci = axp517_tcpc_to_tcpci(tcpc);
	unsigned int reg, role_control;
	int ret;

	ret = regmap_read(tcpci->regmap, AXP517_TCPC_ROLE_CTRL, &role_control);
	if (ret < 0)
		return ret;

	ret = regmap_read(tcpci->regmap, AXP517_TCPC_CC_STATUS, &reg);
	if (ret < 0)
		return ret;

	*cc1 = axp517_tcpci_to_typec_cc((reg >> AXP517_TCPC_CC_STATUS_CC1_SHIFT) &
				 AXP517_TCPC_CC_STATUS_CC1_MASK,
				 reg & AXP517_TCPC_CC_STATUS_TERM ||
				 tcpc_presenting_rd(role_control, CC1));
	*cc2 = axp517_tcpci_to_typec_cc((reg >> AXP517_TCPC_CC_STATUS_CC2_SHIFT) &
				 AXP517_TCPC_CC_STATUS_CC2_MASK,
				 reg & AXP517_TCPC_CC_STATUS_TERM ||
				 tcpc_presenting_rd(role_control, CC2));

	return 0;
}
EXPORT_SYMBOL_GPL(axp517_tcpci_get_cc);

static int axp517_tcpci_set_cc(struct tcpc_dev *tcpc, enum typec_cc_status cc)
{
	struct tcpci *tcpci = axp517_tcpc_to_tcpci(tcpc);
	bool vconn_pres;
	enum typec_cc_polarity polarity = TYPEC_POLARITY_CC1;
	unsigned int reg;
	int ret;

	if (!tcpci->data->self_powered)
		return 0;
	ret = regmap_read(tcpci->regmap, AXP517_TCPC_POWER_STATUS, &reg);
	if (ret < 0)
		return ret;

	vconn_pres = !!(reg & AXP517_TCPC_POWER_STATUS_VCONN_PRES);
	if (vconn_pres) {
		ret = regmap_read(tcpci->regmap, AXP517_TCPC_CTRL, &reg);
		if (ret < 0)
			return ret;

		if (reg & AXP517_TCPC_CTRL_ORIENTATION)
			polarity = TYPEC_POLARITY_CC2;
	}

	switch (cc) {
	case TYPEC_CC_RA:
		reg = (AXP517_TCPC_ROLE_CTRL_CC_RA << AXP517_TCPC_ROLE_CTRL_CC1_SHIFT) |
			(AXP517_TCPC_ROLE_CTRL_CC_RA << AXP517_TCPC_ROLE_CTRL_CC2_SHIFT);
		break;
	case TYPEC_CC_RD:
		reg = (AXP517_TCPC_ROLE_CTRL_CC_RD << AXP517_TCPC_ROLE_CTRL_CC1_SHIFT) |
			(AXP517_TCPC_ROLE_CTRL_CC_RD << AXP517_TCPC_ROLE_CTRL_CC2_SHIFT);
		break;
	case TYPEC_CC_RP_DEF:
		reg = (AXP517_TCPC_ROLE_CTRL_CC_RP << AXP517_TCPC_ROLE_CTRL_CC1_SHIFT) |
			(AXP517_TCPC_ROLE_CTRL_CC_RP << AXP517_TCPC_ROLE_CTRL_CC2_SHIFT) |
			(AXP517_TCPC_ROLE_CTRL_RP_VAL_3_0 <<
			 AXP517_TCPC_ROLE_CTRL_RP_VAL_SHIFT);
		break;
	case TYPEC_CC_RP_1_5:
		reg = (AXP517_TCPC_ROLE_CTRL_CC_RP << AXP517_TCPC_ROLE_CTRL_CC1_SHIFT) |
			(AXP517_TCPC_ROLE_CTRL_CC_RP << AXP517_TCPC_ROLE_CTRL_CC2_SHIFT) |
			(AXP517_TCPC_ROLE_CTRL_RP_VAL_3_0 <<
			 AXP517_TCPC_ROLE_CTRL_RP_VAL_SHIFT);
		break;
	case TYPEC_CC_RP_3_0:
		reg = (AXP517_TCPC_ROLE_CTRL_CC_RP << AXP517_TCPC_ROLE_CTRL_CC1_SHIFT) |
			(AXP517_TCPC_ROLE_CTRL_CC_RP << AXP517_TCPC_ROLE_CTRL_CC2_SHIFT) |
			(AXP517_TCPC_ROLE_CTRL_RP_VAL_3_0 <<
			 AXP517_TCPC_ROLE_CTRL_RP_VAL_SHIFT);
		break;
	case TYPEC_CC_OPEN:
	default:
		reg = (AXP517_TCPC_ROLE_CTRL_CC_OPEN << AXP517_TCPC_ROLE_CTRL_CC1_SHIFT) |
			(AXP517_TCPC_ROLE_CTRL_CC_OPEN << AXP517_TCPC_ROLE_CTRL_CC2_SHIFT);
		break;
	}

	if (vconn_pres) {
		if (polarity == TYPEC_POLARITY_CC2) {
			reg &= ~(AXP517_TCPC_ROLE_CTRL_CC1_MASK << AXP517_TCPC_ROLE_CTRL_CC1_SHIFT);
			reg |= (AXP517_TCPC_ROLE_CTRL_CC_OPEN << AXP517_TCPC_ROLE_CTRL_CC1_SHIFT);
		} else {
			reg &= ~(AXP517_TCPC_ROLE_CTRL_CC2_MASK << AXP517_TCPC_ROLE_CTRL_CC2_SHIFT);
			reg |= (AXP517_TCPC_ROLE_CTRL_CC_OPEN << AXP517_TCPC_ROLE_CTRL_CC2_SHIFT);
		}
	}

	ret = regmap_write(tcpci->regmap, AXP517_TCPC_ROLE_CTRL, reg);
	if (ret < 0)
		return ret;

	return 0;
}

static int axp517_tcpci_apply_rc(struct tcpc_dev *tcpc, enum typec_cc_status cc,
			  enum typec_cc_polarity polarity)
{
	struct tcpci *tcpci = axp517_tcpc_to_tcpci(tcpc);
	unsigned int reg;
	int ret;

	if (!tcpci->data->self_powered)
		return 0;
	ret = regmap_read(tcpci->regmap, AXP517_TCPC_ROLE_CTRL, &reg);
	if (ret < 0)
		return ret;

	/*
	 * APPLY_RC state is when ROLE_CONTROL.CC1 != ROLE_CONTROL.CC2 and vbus autodischarge on
	 * disconnect is disabled. Bail out when ROLE_CONTROL.CC1 != ROLE_CONTROL.CC2.
	 */
	if (((reg & (AXP517_TCPC_ROLE_CTRL_CC2_MASK << AXP517_TCPC_ROLE_CTRL_CC2_SHIFT)) >>
	     AXP517_TCPC_ROLE_CTRL_CC2_SHIFT) !=
	    ((reg & (AXP517_TCPC_ROLE_CTRL_CC1_MASK << AXP517_TCPC_ROLE_CTRL_CC1_SHIFT)) >>
	     AXP517_TCPC_ROLE_CTRL_CC1_SHIFT))
		return 0;

	return regmap_update_bits(tcpci->regmap, AXP517_TCPC_ROLE_CTRL, polarity == TYPEC_POLARITY_CC1 ?
				  AXP517_TCPC_ROLE_CTRL_CC2_MASK << AXP517_TCPC_ROLE_CTRL_CC2_SHIFT :
				  AXP517_TCPC_ROLE_CTRL_CC1_MASK << AXP517_TCPC_ROLE_CTRL_CC1_SHIFT,
				  AXP517_TCPC_ROLE_CTRL_CC_OPEN);
}

static int axp517_tcpci_start_toggling(struct tcpc_dev *tcpc,
				enum typec_port_type port_type,
				enum typec_cc_status cc)
{
	int ret;
	struct tcpci *tcpci = axp517_tcpc_to_tcpci(tcpc);
	unsigned int reg = tcpci->data->self_powered ? AXP517_TCPC_ROLE_CTRL_DRP : 0;

	if (port_type != TYPEC_PORT_DRP)
		return -EOPNOTSUPP;

	/* Handle vendor drp toggling */
	if (tcpci->data->start_drp_toggling) {
		ret = tcpci->data->start_drp_toggling(tcpci, tcpci->data, cc);
		if (ret < 0)
			return ret;
	}

	switch (cc) {
	default:
	case TYPEC_CC_RP_DEF:
		reg |= (AXP517_TCPC_ROLE_CTRL_RP_VAL_3_0 <<
			AXP517_TCPC_ROLE_CTRL_RP_VAL_SHIFT);
		break;
	case TYPEC_CC_RP_1_5:
		reg |= (AXP517_TCPC_ROLE_CTRL_RP_VAL_3_0 <<
			AXP517_TCPC_ROLE_CTRL_RP_VAL_SHIFT);
		break;
	case TYPEC_CC_RP_3_0:
		reg |= (AXP517_TCPC_ROLE_CTRL_RP_VAL_3_0 <<
			AXP517_TCPC_ROLE_CTRL_RP_VAL_SHIFT);
		break;
	}

	if (tcpci->data->self_powered) {
		if (cc == TYPEC_CC_RD)
			reg |= (AXP517_TCPC_ROLE_CTRL_CC_RD << AXP517_TCPC_ROLE_CTRL_CC1_SHIFT) |
				(AXP517_TCPC_ROLE_CTRL_CC_RD << AXP517_TCPC_ROLE_CTRL_CC2_SHIFT);
		else
			reg |= (AXP517_TCPC_ROLE_CTRL_CC_RP << AXP517_TCPC_ROLE_CTRL_CC1_SHIFT) |
				(AXP517_TCPC_ROLE_CTRL_CC_RP << AXP517_TCPC_ROLE_CTRL_CC2_SHIFT);
	} else {
		reg |= (AXP517_TCPC_ROLE_CTRL_CC_RD << AXP517_TCPC_ROLE_CTRL_CC1_SHIFT) |
			   (AXP517_TCPC_ROLE_CTRL_CC_RD << AXP517_TCPC_ROLE_CTRL_CC2_SHIFT);
	}
	ret = regmap_write(tcpci->regmap, AXP517_TCPC_ROLE_CTRL, reg);
	if (ret < 0)
		return ret;
	return regmap_write(tcpci->regmap, AXP517_TCPC_COMMAND,
			    AXP517_TCPC_CMD_LOOK4CONNECTION);
}

static int axp517_tcpci_set_polarity(struct tcpc_dev *tcpc,
			      enum typec_cc_polarity polarity)
{
	struct tcpci *tcpci = axp517_tcpc_to_tcpci(tcpc);
	unsigned int reg;
	int ret;
	enum typec_cc_status cc1, cc2;

	/* Obtain Rp setting from role control */
	ret = regmap_read(tcpci->regmap, AXP517_TCPC_ROLE_CTRL, &reg);
	if (ret < 0)
		return ret;

	ret = axp517_tcpci_get_cc(tcpc, &cc1, &cc2);
	if (ret < 0)
		return ret;

	/*
	 * When port has drp toggling enabled, ROLE_CONTROL would only have the initial
	 * terminations for the toggling and does not indicate the final cc
	 * terminations when ConnectionResult is 0 i.e. drp toggling stops and
	 * the connection is resolved. Infer port role from AXP517_TCPC_CC_STATUS based on the
	 * terminations seen. The port role is then used to set the cc terminations.
	 */
	if (reg & AXP517_TCPC_ROLE_CTRL_DRP) {
		/* Disable DRP for the OPEN setting to take effect */
		reg = reg & ~AXP517_TCPC_ROLE_CTRL_DRP;

		if (polarity == TYPEC_POLARITY_CC2) {
			reg &= ~(AXP517_TCPC_ROLE_CTRL_CC2_MASK << AXP517_TCPC_ROLE_CTRL_CC2_SHIFT);
			/* Local port is source */
			if (cc2 == TYPEC_CC_RD)
				/* Role control would have the Rp setting when DRP was enabled */
				reg |= AXP517_TCPC_ROLE_CTRL_CC_RP << AXP517_TCPC_ROLE_CTRL_CC2_SHIFT;
			else
				reg |= AXP517_TCPC_ROLE_CTRL_CC_RD << AXP517_TCPC_ROLE_CTRL_CC2_SHIFT;
		} else {
			reg &= ~(AXP517_TCPC_ROLE_CTRL_CC1_MASK << AXP517_TCPC_ROLE_CTRL_CC1_SHIFT);
			/* Local port is source */
			if (cc1 == TYPEC_CC_RD)
				/* Role control would have the Rp setting when DRP was enabled */
				reg |= AXP517_TCPC_ROLE_CTRL_CC_RP << AXP517_TCPC_ROLE_CTRL_CC1_SHIFT;
			else
				reg |= AXP517_TCPC_ROLE_CTRL_CC_RD << AXP517_TCPC_ROLE_CTRL_CC1_SHIFT;
		}
	}

	if (tcpci->data->self_powered) {
		if (polarity == TYPEC_POLARITY_CC2)
			reg |= AXP517_TCPC_ROLE_CTRL_CC_OPEN << AXP517_TCPC_ROLE_CTRL_CC1_SHIFT;
		else
			reg |= AXP517_TCPC_ROLE_CTRL_CC_OPEN << AXP517_TCPC_ROLE_CTRL_CC2_SHIFT;
	}
	ret = regmap_write(tcpci->regmap, AXP517_TCPC_ROLE_CTRL, reg);
	if (ret < 0)
		return ret;

	return regmap_write(tcpci->regmap, AXP517_TCPC_CTRL,
			   (polarity == TYPEC_POLARITY_CC2) ?
			   AXP517_TCPC_CTRL_ORIENTATION : 0);
}

static void axp517_tcpci_set_partner_usb_comm_capable(struct tcpc_dev *tcpc, bool capable)
{
	struct tcpci *tcpci = axp517_tcpc_to_tcpci(tcpc);

	if (tcpci->data->set_partner_usb_comm_capable)
		tcpci->data->set_partner_usb_comm_capable(tcpci, tcpci->data, capable);
}

static int axp517_tcpci_set_vconn(struct tcpc_dev *tcpc, bool enable)
{
	struct tcpci *tcpci = axp517_tcpc_to_tcpci(tcpc);
	int ret;

	/* Handle vendor set vconn */
	if (tcpci->data->set_vconn) {
		ret = tcpci->data->set_vconn(tcpci, tcpci->data, enable);
		if (ret < 0)
			return ret;
	}

	return regmap_update_bits(tcpci->regmap, AXP517_TCPC_POWER_CTRL,
				AXP517_TCPC_POWER_CTRL_VCONN_ENABLE,
				enable ? AXP517_TCPC_POWER_CTRL_VCONN_ENABLE : 0);
}

static int axp517_tcpci_enable_auto_vbus_discharge(struct tcpc_dev *dev, bool enable)
{
	struct tcpci *tcpci = axp517_tcpc_to_tcpci(dev);
	int ret;

	ret = regmap_update_bits(tcpci->regmap, AXP517_TCPC_POWER_CTRL, AXP517_TCPC_POWER_CTRL_AUTO_DISCHARGE,
				 enable ? AXP517_TCPC_POWER_CTRL_AUTO_DISCHARGE : 0);
	return ret;
}

static int axp517_tcpci_set_auto_vbus_discharge_threshold(struct tcpc_dev *dev, enum typec_pwr_opmode mode,
						   bool pps_active, u32 requested_vbus_voltage_mv)
{
	struct tcpci *tcpci = axp517_tcpc_to_tcpci(dev);
	unsigned int pwr_ctrl, threshold = 0;
	int ret;

	/*
	 * Indicates that vbus is going to go away due PR_SWAP, hard reset etc.
	 * Do not discharge vbus here.
	 */
	if (requested_vbus_voltage_mv == 0)
		goto write_thresh;

	ret = regmap_read(tcpci->regmap, AXP517_TCPC_POWER_CTRL, &pwr_ctrl);
	if (ret < 0)
		return ret;

	if (pwr_ctrl & AXP517_TCPC_FAST_ROLE_SWAP_EN) {
		/* To prevent disconnect when the source is fast role swap is capable. */
		threshold = AUTO_DISCHARGE_DEFAULT_THRESHOLD_MV;
	} else if (mode == TYPEC_PWR_MODE_PD) {
		if (pps_active)
			threshold = ((VPPS_NEW_MIN_PERCENT * requested_vbus_voltage_mv / 100) -
				     VSINKPD_MIN_IR_DROP_MV - VPPS_VALID_MIN_MV) *
				     VSINKDISCONNECT_PD_MIN_PERCENT / 100;
		else
			threshold = ((VSRC_NEW_MIN_PERCENT * requested_vbus_voltage_mv / 100) -
				     VSINKPD_MIN_IR_DROP_MV - VSRC_VALID_MIN_MV) *
				     VSINKDISCONNECT_PD_MIN_PERCENT / 100;
	} else {
		/* 3.5V for non-pd sink */
		threshold = AUTO_DISCHARGE_DEFAULT_THRESHOLD_MV;
	}

	threshold = threshold / AXP517_TCPC_VBUS_SINK_DISCONNECT_THRESH_LSB_MV;

	if (threshold > AXP517_TCPC_VBUS_SINK_DISCONNECT_THRESH_MAX)
		return -EINVAL;

write_thresh:
	return axp517_tcpci_write16(tcpci, AXP517_TCPC_VBUS_SINK_DISCONNECT_THRESH, threshold);
}

static int axp517_tcpci_enable_frs(struct tcpc_dev *dev, bool enable)
{
	struct tcpci *tcpci = axp517_tcpc_to_tcpci(dev);
	int ret;

	/* To prevent disconnect during FRS, set disconnect threshold to 3.5V */
	ret = axp517_tcpci_write16(tcpci, AXP517_TCPC_VBUS_SINK_DISCONNECT_THRESH, enable ? 0 : 0x8c);
	if (ret < 0)
		return ret;

	ret = regmap_update_bits(tcpci->regmap, AXP517_TCPC_POWER_CTRL, AXP517_TCPC_FAST_ROLE_SWAP_EN, enable ?
				 AXP517_TCPC_FAST_ROLE_SWAP_EN : 0);

	return ret;
}

static void axp517_tcpci_frs_sourcing_vbus(struct tcpc_dev *dev)
{
	struct tcpci *tcpci = axp517_tcpc_to_tcpci(dev);

	if (tcpci->data->frs_sourcing_vbus)
		tcpci->data->frs_sourcing_vbus(tcpci, tcpci->data);
}

static void axp517_tcpci_check_contaminant(struct tcpc_dev *dev)
{
	struct tcpci *tcpci = axp517_tcpc_to_tcpci(dev);

	if (tcpci->data->check_contaminant)
		tcpci->data->check_contaminant(tcpci, tcpci->data);
}

static int axp517_tcpci_set_bist_data(struct tcpc_dev *tcpc, bool enable)
{
	struct tcpci *tcpci = axp517_tcpc_to_tcpci(tcpc);

	return regmap_update_bits(tcpci->regmap, AXP517_TCPC_CTRL, AXP517_TCPC_CTRL_BIST_TM,
				 enable ? AXP517_TCPC_CTRL_BIST_TM : 0);
}

static int axp517_tcpci_set_roles(struct tcpc_dev *tcpc, bool attached,
			   enum typec_role role, enum typec_data_role data)
{
	struct tcpci *tcpci = axp517_tcpc_to_tcpci(tcpc);
	struct axp517_tcpci_chip *chip = tdata_to_axp517(tcpci->data);
	enum typec_cc_status cc1, cc2;
	unsigned int reg;
	int ret;

	reg = PD_REV20 << AXP517_TCPC_MSG_HDR_INFO_REV_SHIFT;
	if (role == TYPEC_SOURCE)
		reg |= AXP517_TCPC_MSG_HDR_INFO_PWR_ROLE;
	if (data == TYPEC_HOST)
		reg |= AXP517_TCPC_MSG_HDR_INFO_DATA_ROLE;
	ret = regmap_write(tcpci->regmap, AXP517_TCPC_MSG_HDR_INFO, reg);
	if (ret < 0)
		return ret;

	/* Support for Audio Accessory Mode. */
	if (axp517_tcpci_get_cc(tcpc, &cc1, &cc2) == 0) {
		if (axp517_tcpci_port_is_audio(cc1, cc2))
			extcon_set_state_sync(chip->edev, EXTCON_JACK_HEADPHONE, true);
		else
			extcon_set_state_sync(chip->edev, EXTCON_JACK_HEADPHONE, false);
	}

	return 0;
}

static int axp517_tcpci_set_pd_rx(struct tcpc_dev *tcpc, bool enable)
{
	struct tcpci *tcpci = axp517_tcpc_to_tcpci(tcpc);
	unsigned int reg = 0;
	int ret;

	if (enable)
		reg = AXP517_TCPC_RX_DETECT_SOP/* | AXP517_TCPC_RX_DETECT_HARD_RESET */;
	/* FIXME: Trigger HardReset interrupt causing state machine exception */
	ret = regmap_write(tcpci->regmap, AXP517_TCPC_RX_DETECT, reg);
	if (ret < 0)
		return ret;

	return 0;
}

static int axp517_tcpci_get_vbus(struct tcpc_dev *tcpc)
{
	struct tcpci *tcpci = axp517_tcpc_to_tcpci(tcpc);
	unsigned int reg;
	int ret;

	ret = regmap_read(tcpci->regmap, AXP517_TCPC_POWER_STATUS, &reg);
	if (ret < 0)
		return ret;

	return !!(reg & AXP517_TCPC_POWER_STATUS_VBUS_PRES);
}

static int axp517_tcpci_get_vbus_float_quirk(struct tcpc_dev *tcpc)
{
	struct tcpci *tcpci = axp517_tcpc_to_tcpci(tcpc);
	struct axp517_tcpci_chip *chip = tdata_to_axp517(tcpci->data);
	enum typec_cc_status cc1, cc2;
	unsigned int reg;
	int ret;

	ret = axp517_tcpci_get_cc(tcpc, &cc1, &cc2);
	if (ret < 0)
		return ret;

	ret = regmap_read(tcpci->regmap, AXP517_TCPC_POWER_STATUS, &reg);
	if (ret < 0)
		return ret;

	return (axp517_tcpci_port_is_sink(cc1, cc2) ? !!(reg & AXP517_TCPC_POWER_STATUS_VBUS_PRES) : 0)  || chip->vbus_on;
}

static bool axp517_tcpci_is_vbus_vsafe0v(struct tcpc_dev *tcpc)
{
	struct tcpci *tcpci = axp517_tcpc_to_tcpci(tcpc);
	unsigned int reg;
	int ret;

	ret = regmap_read(tcpci->regmap, AXP517_TCPC_EXTENDED_STATUS, &reg);
	if (ret < 0)
		return false;

	return !!(reg & AXP517_TCPC_EXTENDED_STATUS_VSAFE0V);
}

static int axp517_tcpci_set_vbus(struct tcpc_dev *tcpc, bool source, bool sink)
{
	struct tcpci *tcpci = axp517_tcpc_to_tcpci(tcpc);
	int ret;

	if (tcpci->data->set_vbus) {
		ret = tcpci->data->set_vbus(tcpci, tcpci->data, source, sink);
		/* Bypass when ret > 0 */
		if (ret != 0)
			return ret < 0 ? ret : 0;
	}

	/* Disable both source and sink first before enabling anything */

	if (!source) {
		ret = regmap_write(tcpci->regmap, AXP517_TCPC_COMMAND,
				   AXP517_TCPC_CMD_DISABLE_SRC_VBUS);
		if (ret < 0)
			return ret;
	}

	if (!sink) {
		ret = regmap_write(tcpci->regmap, AXP517_TCPC_COMMAND,
				   AXP517_TCPC_CMD_DISABLE_SINK_VBUS);
		if (ret < 0)
			return ret;
	}

	if (source) {
		ret = regmap_write(tcpci->regmap, AXP517_TCPC_COMMAND,
				   AXP517_TCPC_CMD_SRC_VBUS_DEFAULT);
		if (ret < 0)
			return ret;
	}

	if (sink) {
		ret = regmap_write(tcpci->regmap, AXP517_TCPC_COMMAND,
				   AXP517_TCPC_CMD_SINK_VBUS);
		if (ret < 0)
			return ret;
	}

	return 0;
}

static int axp517_tcpci_set_vbus_regulator(struct tcpci *tcpci, struct axp517_tcpci_data *tdata,
			    bool on, bool charge)
{
	struct axp517_tcpci_chip *chip = tdata_to_axp517(tdata);
	struct regmap *regmap = chip->data.regmap;
	int ret = 0;

	if (chip->vbus_on == on) {
		dev_info(chip->dev, " vbus is already %s", on ? "On" : "Off");
		goto done;
	}

	dev_info(chip->dev, " set vbus %s", on ? "On" : "Off");

	if (on)
		ret = regulator_enable(chip->vbus);
	else
		ret = regulator_disable(chip->vbus);
	if (ret < 0) {
		dev_err(chip->dev, " cannot %s vbus regulator, ret=%d",
			on ? "enable" : "disable", ret);
		goto done;
	}

	chip->vbus_on = on;
	/**
	 * FIXME:
	 * 1. Trigger Power interrupt When Use External BOOST.
	 * 2. Boost module enable for PD communication.
	 */
	if (tcpci->port && chip->vbus_float_quirk) {
		tcpm_vbus_change(tcpci->port);
		if (on)
			regmap_update_bits(regmap, AXP517_MODULE_EN, AXP517_BOOST_EN, AXP517_BOOST_EN);
		else
			regmap_update_bits(regmap, AXP517_MODULE_EN, AXP517_BOOST_EN, 0);
	}

done:
	return ret;
}

static int axp517_tcpci_pd_transmit(struct tcpc_dev *tcpc, enum tcpm_transmit_type type,
			     const struct pd_message *msg, unsigned int negotiated_rev)
{
	struct tcpci *tcpci = axp517_tcpc_to_tcpci(tcpc);
	u16 header = msg ? le16_to_cpu(msg->header) : 0;
	unsigned int reg, cnt;
	int ret;

	cnt = msg ? pd_header_cnt(header) * 4 : 0;
	/**
	 * TCPCI spec forbids direct access of AXP517_TCPC_TX_DATA.
	 * But, since some of the chipsets offer this capability,
	 * it's fair to support both.
	 */
	if (tcpci->data->TX_BUF_BYTE_x_hidden) {
		u8 buf[AXP517_TCPC_TRANSMIT_BUFFER_MAX_LEN] = {0,};
		struct reg_sequence regs[AXP517_TCPC_TRANSMIT_BUFFER_MAX_LEN] = { 0, };
		int i;
		u8 pos = 0;

		/* Payload + header + AXP517_TCPC_TX_BYTE_CNT */
		buf[pos++] = cnt + 2;

		if (msg)
			memcpy(&buf[pos], &msg->header, sizeof(msg->header));

		pos += sizeof(header);

		if (cnt > 0)
			memcpy(&buf[pos], msg->payload, cnt);

		pos += cnt;
		if (tcpci->data->RX_TX_FIFO_supported) {
			/* regmap_noinc_write not support due to use_single_write is true */
			for (i = 0; i < pos; i++) {
				regs[i].reg = AXP517_TCPC_TX_BYTE_CNT;
				regs[i].def = buf[i];
			}
			ret = regmap_multi_reg_write(tcpci->regmap, regs, pos);
			if (ret < 0)
				return ret;
		} else {
			ret = regmap_raw_write(tcpci->regmap, AXP517_TCPC_TX_BYTE_CNT, buf, pos);
			if (ret < 0)
				return ret;
		}
	} else {
		ret = regmap_write(tcpci->regmap, AXP517_TCPC_TX_BYTE_CNT, cnt + 2);
		if (ret < 0)
			return ret;

		ret = axp517_tcpci_write16(tcpci, AXP517_TCPC_TX_HDR, header);
		if (ret < 0)
			return ret;

		if (cnt > 0) {
			ret = regmap_raw_write(tcpci->regmap, AXP517_TCPC_TX_DATA, &msg->payload, cnt);
			if (ret < 0)
				return ret;
		}
	}

	/* nRetryCount is 3 in PD2.0 spec where 2 in PD3.0 spec */
	reg = ((negotiated_rev > PD_REV20 ? PD_RETRY_COUNT_3_0_OR_HIGHER : PD_RETRY_COUNT_DEFAULT)
	       << AXP517_TCPC_TRANSMIT_RETRY_SHIFT) | (type << AXP517_TCPC_TRANSMIT_TYPE_SHIFT);
	ret = regmap_write(tcpci->regmap, AXP517_TCPC_TRANSMIT, reg);
	if (ret < 0)
		return ret;

	return 0;
}

static int axp517_tcpci_init(struct tcpc_dev *tcpc)
{
	struct tcpci *tcpci = axp517_tcpc_to_tcpci(tcpc);
	unsigned long timeout = jiffies + msecs_to_jiffies(2000); /* XXX */
	unsigned int reg;
	int ret;

	while (time_before_eq(jiffies, timeout)) {
		ret = regmap_read(tcpci->regmap, AXP517_TCPC_POWER_STATUS, &reg);
		if (ret < 0)
			return ret;
		if (!(reg & AXP517_TCPC_POWER_STATUS_UNINIT))
			break;
		usleep_range(10000, 20000);
	}
	if (time_after(jiffies, timeout))
		return -ETIMEDOUT;

	ret = axp517_tcpci_write16(tcpci, AXP517_TCPC_FAULT_STATUS, AXP517_TCPC_FAULT_STATUS_ALL_REG_RST_TO_DEFAULT);
	if (ret < 0)
		return ret;

	/* Handle vendor init */
	if (tcpci->data->init) {
		ret = tcpci->data->init(tcpci, tcpci->data);
		if (ret < 0)
			return ret;
	}

	/* Clear all events */
	ret = axp517_tcpci_write16(tcpci, AXP517_IRQ_PD_ALERTL_STATUS, 0xffff);
	if (ret < 0)
		return ret;

	if (tcpci->controls_vbus)
		reg = AXP517_TCPC_POWER_STATUS_VBUS_PRES;
	else
		reg = 0;
	ret = regmap_write(tcpci->regmap, AXP517_TCPC_POWER_STATUS_MASK, reg);
	if (ret < 0)
		return ret;

	/* Enable Vbus detection */
	ret = regmap_write(tcpci->regmap, AXP517_TCPC_COMMAND,
			   AXP517_TCPC_CMD_ENABLE_VBUS_DETECT);
	if (ret < 0)
		return ret;

	reg = AXP517_IRQ_PD_ALERTL_STATUS_TX_SUCCESS | AXP517_IRQ_PD_ALERTL_STATUS_TX_FAILED |
		AXP517_IRQ_PD_ALERTL_STATUS_TX_DISCARDED | AXP517_IRQ_PD_ALERTL_STATUS_RX_STATUS |
		AXP517_IRQ_PD_ALERTL_STATUS_RX_HARD_RST | AXP517_IRQ_PD_ALERTL_STATUS_CC_STATUS;
	if (tcpci->controls_vbus)
		reg |= AXP517_IRQ_PD_ALERTL_STATUS_POWER_STATUS;
	/* Enable VSAFE0V status interrupt when detecting VSAFE0V is supported */
	if (tcpci->data->vbus_vsafe0v) {
		reg |= AXP517_IRQ_PD_ALERTL_STATUS_EXTENDED_STATUS;
		ret = regmap_write(tcpci->regmap, AXP517_TCPC_EXTENDED_STATUS_MASK,
				   AXP517_TCPC_EXTENDED_STATUS_VSAFE0V);
		if (ret < 0)
			return ret;
	}

	tcpci->alert_mask = reg;

	return axp517_tcpci_write16(tcpci, AXP517_TCPC_ALERT_MASK, reg);
}

static int axp517_tcpci_get_current_limit(struct tcpc_dev *tcpc)
{
	struct tcpci *tcpci = axp517_tcpc_to_tcpci(tcpc);
	struct axp517_tcpci_chip *chip = tdata_to_axp517(tcpci->data);

	dev_info(chip->dev, " get current limit %u mA", chip->current_limit);

	return chip->current_limit ? chip->current_limit : 2500;
}

static int axp517_tcpci_set_current_limit(struct tcpc_dev *tcpc, u32 max_ma, u32 mv)
{
	struct tcpci *tcpci = axp517_tcpc_to_tcpci(tcpc);
	struct axp517_tcpci_chip *chip = tdata_to_axp517(tcpci->data);
	int ret = 0;

	dev_info(chip->dev, " Setting voltage/current limit %u mV %u mA", mv, max_ma);

	return ret;
}

irqreturn_t axp517_tcpci_irq_overrides(struct tcpci *tcpci)
{
	u16 status;
	int ret;
	unsigned int raw;
	u8 reg_status;

	axp517_tcpci_read16(tcpci, AXP517_IRQ_PD_ALERTL_STATUS, &status);

	/*
	 * Clear alert status for everything except RX_STATUS, which shouldn't
	 * be cleared until we have successfully retrieved message.
	 */
	if (status & ~AXP517_IRQ_PD_ALERTL_STATUS_RX_STATUS)
		axp517_tcpci_write16(tcpci, AXP517_IRQ_PD_ALERTL_STATUS,
			      status & ~AXP517_IRQ_PD_ALERTL_STATUS_RX_STATUS);

	if (status & AXP517_IRQ_PD_ALERTL_STATUS_FAULT) {
		ret = axp517_tcpci_read8(tcpci, AXP517_TCPC_FAULT_STATUS, &reg_status);
		if (ret < 0)
			return ret;

		ret = axp517_tcpci_write8(tcpci, AXP517_TCPC_FAULT_STATUS, reg_status);
		if (ret < 0)
			return ret;
	}

	if (status & AXP517_IRQ_PD_ALERTL_STATUS_CC_STATUS) {
		tcpm_cc_change(tcpci->port);
		if (tcpci->data->vbus_floated)
			tcpm_vbus_change(tcpci->port);
	}

	if (status & AXP517_IRQ_PD_ALERTL_STATUS_POWER_STATUS) {
		regmap_read(tcpci->regmap, AXP517_TCPC_POWER_STATUS_MASK, &raw);
		/*
		 * If power status mask has been reset, then the TCPC
		 * has reset.
		 */
		if (raw == 0xff)
			tcpm_tcpc_reset(tcpci->port);
		else
			tcpm_vbus_change(tcpci->port);
	}

	if (status & AXP517_IRQ_PD_ALERTL_STATUS_RX_STATUS) {
		if (tcpci->data->RX_BUF_BYTE_x_hidden) {
			if (tcpci->data->process_rx)
				tcpci->data->process_rx(tcpci, status);
		} else {
			struct pd_message msg;
			unsigned int cnt, payload_cnt;
			u16 header;

			regmap_read(tcpci->regmap, AXP517_TCPC_RX_BYTE_CNT, &cnt);
			/*
			* 'cnt' corresponds to READABLE_BYTE_COUNT in section 4.4.14
			* of the TCPCI spec [Rev 2.0 Ver 1.0 October 2017] and is
			* defined in table 4-36 as one greater than the number of
			* bytes received. And that number includes the header. So:
			*/
			if (cnt > 3)
				payload_cnt = cnt - (1 + sizeof(msg.header));
			else
				payload_cnt = 0;

			axp517_tcpci_read16(tcpci, AXP517_TCPC_RX_HDR, &header);
			msg.header = cpu_to_le16(header);

			if (WARN_ON(payload_cnt > sizeof(msg.payload)))
				payload_cnt = sizeof(msg.payload);

			if (payload_cnt > 0)
				regmap_raw_read(tcpci->regmap, AXP517_TCPC_RX_DATA,
						&msg.payload, payload_cnt);

			/* Read complete, clear RX status alert bit */
			axp517_tcpci_write16(tcpci, AXP517_IRQ_PD_ALERTL_STATUS, AXP517_IRQ_PD_ALERTL_STATUS_RX_STATUS);

			tcpm_pd_receive(tcpci->port, &msg);
		}
	}

	if (tcpci->data->vbus_vsafe0v && (status & AXP517_IRQ_PD_ALERTL_STATUS_EXTENDED_STATUS)) {
		ret = regmap_read(tcpci->regmap, AXP517_TCPC_EXTENDED_STATUS, &raw);
		if (!ret && (raw & AXP517_TCPC_EXTENDED_STATUS_VSAFE0V))
			tcpm_vbus_change(tcpci->port);
	}

	if (status & AXP517_IRQ_PD_ALERTL_STATUS_RX_HARD_RST)
		tcpm_pd_hard_reset(tcpci->port);

	if (status & AXP517_IRQ_PD_ALERTL_STATUS_TX_SUCCESS)
		tcpm_pd_transmit_complete(tcpci->port, TCPC_TX_SUCCESS);
	else if (status & AXP517_IRQ_PD_ALERTL_STATUS_TX_DISCARDED)
		tcpm_pd_transmit_complete(tcpci->port, TCPC_TX_DISCARDED);
	else if (status & AXP517_IRQ_PD_ALERTL_STATUS_TX_FAILED)
		tcpm_pd_transmit_complete(tcpci->port, TCPC_TX_FAILED);

	return IRQ_RETVAL(status & tcpci->alert_mask);
}
EXPORT_SYMBOL_GPL(axp517_tcpci_irq_overrides);

static int axp517_tcpci_parse_config(struct tcpci *tcpci)
{
	tcpci->controls_vbus = true; /* XXX */

	tcpci->tcpc.fwnode = device_get_named_child_node(tcpci->dev,
							 "connector");
	if (!tcpci->tcpc.fwnode) {
		dev_err(tcpci->dev, "Can't find connector node.\n");
		return -EINVAL;
	}

	return 0;
}


static int axp517_tcpci_data_init(struct tcpci *tcpci, struct axp517_tcpci_data *tdata)
{
	int ret = 0;
	struct axp517_tcpci_chip *chip = tdata_to_axp517(tdata);
	struct regmap *regmap = chip->data.regmap;

	/* UFP Both RD setting : DRP = 0, RpVal = 0 (Default), Rd, Rd */
	ret = regmap_write(regmap, AXP517_TCPC_ROLE_CTRL,
			  AXP517_TCPC_ROLE_CTRL_SET(0, 0, AXP517_TCPC_ROLE_CTRL_CC_RD, AXP517_TCPC_ROLE_CTRL_CC_RD));
	/* tTCPCfilter : (26.7 * val) us */

	/* tDRP : (51.2 + 6.4 * val) ms */

	/* dcSRC.DRP : 33% */

	/* Vconn OC */

	/* CK_300K from 320K, SHIPPING off, AUTOIDLE enable, TIMEOUT = 6.4ms */

	/* software low-power mode */
	ret |= regmap_update_bits(regmap, AXP517_PD_STATE, AXP517_AWAKE_SEL, BIT(3));

	if (ret < 0)
		dev_err(chip->dev, " fail to init registers(%d)\n", ret);

	/* Enable I2C ADDR Static */
	ret = regmap_update_bits(regmap, AXP517_TWI_ADDR_STATIC, AXP517_I2C_ADDR_STATIC, BIT(0));
	if (ret < 0)
		dev_err(chip->dev, " fail to enable i2c addr static registers(%d)\n", ret);

	return ret;
}

static void process_rx(struct tcpci *tcpci, u16 status)
{
	struct axp517_tcpci_chip *chip = tdata_to_axp517(tcpci->data);
	struct pd_message msg;
	u8 count, frame_type, rx_buf[AXP517_TCPC_RECEIVE_BUFFER_LEN];
	int ret, payload_index;
	u8 *rx_buf_ptr;
	enum tcpm_transmit_type rx_type;

	/*
	 * READABLE_BYTE_COUNT: Indicates the number of bytes in the RX_BUF_BYTE_x registers
	 * plus one (for the RX_BUF_FRAME_TYPE) Table 4-36.
	 * Read the count and frame type.
	 */
	ret = regmap_noinc_read(chip->data.regmap, AXP517_TCPC_RX_BYTE_CNT, rx_buf, 2);
	if (ret < 0) {
		dev_err(chip->dev, "AXP517_TCPC_RX_BYTE_CNT read failed ret:%d\n", ret);
		return;
	}

	count = rx_buf[AXP517_TCPC_RECEIVE_BUFFER_COUNT_OFFSET];
	frame_type = rx_buf[AXP517_TCPC_RECEIVE_BUFFER_FRAME_TYPE_OFFSET];

	switch (frame_type) {
	case AXP517_TCPC_RX_BUF_FRAME_TYPE_SOP1:
		rx_type = TCPC_TX_SOP_PRIME;
		break;
	case AXP517_TCPC_RX_BUF_FRAME_TYPE_SOP:
		rx_type = TCPC_TX_SOP;
		break;
	default:
		rx_type = TCPC_TX_SOP;
		break;
	}

	if (count == 0 || (frame_type != AXP517_TCPC_RX_BUF_FRAME_TYPE_SOP &&
	    frame_type != AXP517_TCPC_RX_BUF_FRAME_TYPE_SOP1)) {
		axp517_tcpci_write16(chip->tcpci, AXP517_IRQ_PD_ALERTL_STATUS, AXP517_IRQ_PD_ALERTL_STATUS_RX_STATUS);
		dev_err(chip->dev, "%s\n", count ==  0 ? "error: count is 0" :
			"error frame_type is not SOP/SOP'");
		return;
	}

	if ((count > (sizeof(struct pd_message) + 1)) || (count + 1 > AXP517_TCPC_RECEIVE_BUFFER_LEN)) {
		dev_err(chip->dev, "Invalid AXP517_TCPC_RX_BYTE_CNT %d\n", count);
		return;
	}

	/*
	 * Read count + 1 as RX_BUF_BYTE_x is hidden and can only be read through
	 * AXP517_TCPC_RX_BYTE_CNT
	 */
	count += 1;
	ret = regmap_noinc_read(chip->data.regmap, AXP517_TCPC_RX_BYTE_CNT, rx_buf, count);
	if (ret < 0) {
		dev_err(chip->dev, "Error: AXP517_TCPC_RX_BYTE_CNT read failed: %d\n", ret);
		return;
	}

	rx_buf_ptr = rx_buf + AXP517_TCPC_RECEIVE_BUFFER_RX_BYTE_BUF_OFFSET;
	msg.header = cpu_to_le16(*(u16 *)rx_buf_ptr);
	rx_buf_ptr = rx_buf_ptr + sizeof(msg.header);
	for (payload_index = 0; payload_index < pd_header_cnt_le(msg.header); payload_index++,
	     rx_buf_ptr += sizeof(msg.payload[0]))
		msg.payload[payload_index] = cpu_to_le32(*(u32 *)rx_buf_ptr);

	/* Read complete, clear RX status alert bit */
	axp517_tcpci_write16(chip->tcpci, AXP517_IRQ_PD_ALERTL_STATUS, AXP517_IRQ_PD_ALERTL_STATUS_RX_STATUS);

	tcpm_pd_receive(tcpci->port, &msg);
}

static int axp517_tcpci_check_battery(struct axp517_tcpci_chip *chip)
{
	struct device_node *np = NULL;
	static int battery_check;

	if (!battery_check) {
		battery_check = 1;
		np = of_parse_phandle(chip->dev->of_node, "det_battery_supply", 0);
		if (np) {
			if (of_device_is_available(np))
				battery_check = 2;
		}
	}

	return battery_check - 1;
}

static void axp517_tcpci_init_tcpci_data(struct axp517_tcpci_chip *chip)
{
	chip->data.init = axp517_tcpci_data_init;
	chip->data.process_rx = process_rx;
	chip->data.RX_BUF_BYTE_x_hidden = true;
	chip->data.TX_BUF_BYTE_x_hidden = true;
	chip->data.RX_TX_FIFO_supported = true;
	chip->data.vbus_floated = chip->vbus_float_quirk;
	chip->battery_exist = axp517_tcpci_check_battery(chip) ? true : false;
	chip->data.self_powered = chip->battery_exist;
}

struct tcpci *axp517_tcpci_register_port_overrides(struct device *dev, struct axp517_tcpci_data *data)
{
	struct axp517_tcpci_chip *chip = tdata_to_axp517(data);
	struct tcpci *tcpci;
	int err;

	axp517_tcpci_init_tcpci_data(chip);

	tcpci = devm_kzalloc(dev, sizeof(*tcpci), GFP_KERNEL);
	if (!tcpci)
		return ERR_PTR(-ENOMEM);

	tcpci->dev = dev;
	tcpci->data = data;
	tcpci->regmap = data->regmap;

	tcpci->tcpc.init = axp517_tcpci_init;

	if (tcpci->data->vbus_floated)
		tcpci->tcpc.get_vbus = axp517_tcpci_get_vbus_float_quirk;
	else
		tcpci->tcpc.get_vbus = axp517_tcpci_get_vbus;

	if (chip->vbus)
		tcpci->data->set_vbus = axp517_tcpci_set_vbus_regulator;

	tcpci->tcpc.set_vbus = axp517_tcpci_set_vbus;
	tcpci->tcpc.set_cc = axp517_tcpci_set_cc;
	tcpci->tcpc.apply_rc = axp517_tcpci_apply_rc;
	tcpci->tcpc.get_cc = axp517_tcpci_get_cc;
	tcpci->tcpc.set_polarity = axp517_tcpci_set_polarity;
	tcpci->tcpc.set_vconn = axp517_tcpci_set_vconn;
	tcpci->tcpc.start_toggling = axp517_tcpci_start_toggling;

	tcpci->tcpc.set_pd_rx = axp517_tcpci_set_pd_rx;
	tcpci->tcpc.set_roles = axp517_tcpci_set_roles;
	tcpci->tcpc.pd_transmit = axp517_tcpci_pd_transmit;
	tcpci->tcpc.set_bist_data = axp517_tcpci_set_bist_data;
	tcpci->tcpc.enable_frs = axp517_tcpci_enable_frs;
	tcpci->tcpc.frs_sourcing_vbus = axp517_tcpci_frs_sourcing_vbus;
	tcpci->tcpc.set_partner_usb_comm_capable = axp517_tcpci_set_partner_usb_comm_capable;

	if (tcpci->data->check_contaminant)
		tcpci->tcpc.check_contaminant = axp517_tcpci_check_contaminant;

	if (tcpci->data->auto_discharge_disconnect) {
		tcpci->tcpc.enable_auto_vbus_discharge = axp517_tcpci_enable_auto_vbus_discharge;
		tcpci->tcpc.set_auto_vbus_discharge_threshold =
			axp517_tcpci_set_auto_vbus_discharge_threshold;
		regmap_update_bits(tcpci->regmap, AXP517_TCPC_POWER_CTRL, AXP517_TCPC_POWER_CTRL_BLEED_DISCHARGE,
				   AXP517_TCPC_POWER_CTRL_BLEED_DISCHARGE);
	}

	if (tcpci->data->vbus_vsafe0v)
		tcpci->tcpc.is_vbus_vsafe0v = axp517_tcpci_is_vbus_vsafe0v;

	tcpci->tcpc.set_current_limit = axp517_tcpci_set_current_limit;
	tcpci->tcpc.get_current_limit = axp517_tcpci_get_current_limit;

	err = axp517_tcpci_parse_config(tcpci);
	if (err < 0)
		return ERR_PTR(err);

	tcpci->port = tcpm_register_port(tcpci->dev, &tcpci->tcpc);
	if (IS_ERR(tcpci->port)) {
		fwnode_handle_put(tcpci->tcpc.fwnode);
		return ERR_CAST(tcpci->port);
	}

	return tcpci;
}
EXPORT_SYMBOL_GPL(axp517_tcpci_register_port_overrides);

void axp517_tcpci_unregister_port_overrides(struct tcpci *tcpci)
{
	tcpm_unregister_port(tcpci->port);
	fwnode_handle_put(tcpci->tcpc.fwnode);
}
EXPORT_SYMBOL_GPL(axp517_tcpci_unregister_port_overrides);

MODULE_DESCRIPTION("USB Type-C Port Controller Interface driver");
MODULE_LICENSE("GPL");
