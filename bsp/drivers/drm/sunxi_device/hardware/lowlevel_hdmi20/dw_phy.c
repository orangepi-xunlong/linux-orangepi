/* SPDX-License-Identifier: GPL-2.0-or-later */
/*******************************************************************************
 * Allwinner SoCs hdmi2.0 driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 ******************************************************************************/
#include <linux/delay.h>

#include "dw_dev.h"
#include "dw_mc.h"
#include "dw_phy.h"

#define DWPHY_MAX_TIMES		(500)
#define DWPHY_I2CM_ADDR		(0x69)

#ifdef SUPPORT_PHY_JTAG
#define JTAG_TAP_ADDR_CMD   0
#define JTAG_TAP_WRITE_CMD  1
#define JTAG_TAP_READ_CMD   3
#endif

dw_phy_access_t gPHY_interface = DW_PHY_ACCESS_NULL;

#ifdef SUPPORT_PHY_JTAG
static void _phy_jtag_send_data_pulse(u8 tms, u8 tdi)
{
	dw_write(JTAG_PHY_TAP_TCK, (u8) 0);
	udelay(100);
	dw_write_mask(JTAG_PHY_TAP_IN, JTAG_PHY_TAP_IN_JTAG_TMS_MASK, tms);
	dw_write_mask(JTAG_PHY_TAP_IN, JTAG_PHY_TAP_IN_JTAG_TDI_MASK, tdi);
	udelay(100);
	dw_write(JTAG_PHY_TAP_TCK, (u8) 1);
	udelay(100);
}

static void _phy_jtag_tap_soft_reset(void)
{
	int i;

	for (i = 0; i < 5; i++)
		_phy_jtag_send_data_pulse(1, 0);

	_phy_jtag_send_data_pulse(0, 0);
}

static void _phy_jtag_tap_goto_shift_dr(void)
{
	/* RTI -> Select-DR */
	_phy_jtag_send_data_pulse(1, 0);
	/* Select-DR-Scan -> Capture-DR -> Shift-DR */
	_phy_jtag_send_data_pulse(0, 0);
	_phy_jtag_send_data_pulse(0, 0);
}

static void _phy_jtag_tap_goto_shift_ir(void)
{
	/* RTI->Sel IR_Scan */
	_phy_jtag_send_data_pulse(1, 0);
	_phy_jtag_send_data_pulse(1, 0);
	/* Select-IR-Scan -> Shift_IR */
	_phy_jtag_send_data_pulse(0, 0);
	_phy_jtag_send_data_pulse(0, 0);
}

static void _phy_jtag_tap_goto_run_test_idle(void)
{
	/* Exit1_DR -> Update_DR */
	_phy_jtag_send_data_pulse(1, 0);

	/* Update_DR -> Run_Test_Idle */
	_phy_jtag_send_data_pulse(0, 0);
}

static void _phy_jtag_send_value_shift_ir(u8 jtag_addr)
{
	int i;

	for (i = 0; i < 7; i++) {
		_phy_jtag_send_data_pulse(0, jtag_addr & 0x01);
		jtag_addr = jtag_addr >> 1;
	}
	/* Shift_IR -> Exit_IR w/ last MSB bit */
	_phy_jtag_send_data_pulse(1, jtag_addr & 0x01);
}

static u16 _phy_jtag_send_value_shift_dr(u8 cmd, u16 data_in)
{
	int i;
	u32 aux_in = (cmd << 16) | data_in;
	u16 data_out = 0;
	/* Shift_DR */
	for (i = 0; i < 16; i++) {
		_phy_jtag_send_data_pulse(0, aux_in);
		data_out |= (dw_read(JTAG_PHY_TAP_OUT) & 0x01) << i;
		aux_in = aux_in >> 1;
	}
	/* Shift_DR, TAP command bit */
	_phy_jtag_send_data_pulse(0, aux_in);
	aux_in = aux_in >> 1;

	/* Shift_DR -> Exit_DR w/ MSB TAP command bit */
	i++;
	_phy_jtag_send_data_pulse(1, aux_in);
	data_out |= (dw_read(JTAG_PHY_TAP_OUT) & 0x01) << i;

	return data_out;
}

static void _phy_jtag_reset(void)
{
	dw_write(JTAG_PHY_TAP_IN, 0x10);
	udelay(100);
	dw_write(JTAG_PHY_CONFIG, 0);
	udelay(100);
	dw_write(JTAG_PHY_CONFIG, 1); /* enable interface to JTAG */
	_phy_jtag_send_data_pulse(0, 0);
}

static void _phy_jtag_slave_address(u8 jtag_addr)
{
	_phy_jtag_tap_goto_shift_ir();

	/* Shift-IR - write jtag slave address */
	_phy_jtag_send_value_shift_ir(jtag_addr);

	_phy_jtag_tap_goto_run_test_idle();
}

static int _phy_jtag_init(u8 jtag_addr)
{
	dw_write(JTAG_PHY_ADDR, jtag_addr);
	_phy_jtag_reset(void);
	_phy_jtag_tap_soft_reset();
	_phy_jtag_slave_address(jtag_addr);

	return 1;
}

static int _phy_jtag_read(u16 addr, u16 *pvalue)
{
	_phy_jtag_tap_goto_shift_dr();

	/* Shift-DR (shift 16 times) and -> Exit1 -DR */
	_phy_jtag_send_value_shift_dr(JTAG_TAP_ADDR_CMD, addr << 8);

	_phy_jtag_tap_goto_run_test_idle();
	_phy_jtag_tap_goto_shift_dr();

	*pvalue = _phy_jtag_send_value_shift_dr(JTAG_TAP_READ_CMD, 0xFFFF);

	_phy_jtag_tap_goto_run_test_idle();

	return 0;
}

static int _phy_jtag_write(u16 addr,  u16 value)
{
	_phy_jtag_tap_goto_shift_dr();

	/* Shift-DR (shift 16 times) and -> Exit1 -DR */
	_phy_jtag_send_value_shift_dr(JTAG_TAP_ADDR_CMD, addr << 8);

	_phy_jtag_tap_goto_run_test_idle();
	_phy_jtag_tap_goto_shift_dr();

	_phy_jtag_send_value_shift_dr(JTAG_TAP_WRITE_CMD, value);
	_phy_jtag_tap_goto_run_test_idle();

	return 0;
}
#endif

static inline void _dw_phy_set_power_down(u8 bit)
{
	dw_write_mask(PHY_CONF0, PHY_CONF0_SPARES_2_MASK, bit);
}

static inline void _dw_phy_set_tmds_enable(u8 bit)
{
	dw_write_mask(PHY_CONF0, PHY_CONF0_SPARES_1_MASK, bit);
}

static inline void _dw_phy_set_data_enable_polarity(u8 bit)
{
	dw_write_mask(PHY_CONF0, PHY_CONF0_SELDATAENPOL_MASK, bit);
}

static inline void _dw_phy_set_interface_control(u8 bit)
{
	dw_write_mask(PHY_CONF0, PHY_CONF0_SELDIPIF_MASK, bit);
}

static inline void _dw_phy_set_interrupt_mask(u8 mask)
{
	dw_write(PHY_MASK0, mask);
}

static inline void _dw_phy_set_hpdsense(u8 mode)
{
	dw_write_mask(PHY_CONF0, PHY_CONF0_ENHPDRXSENSE_MASK, mode);
}

static inline u8 _dw_phy_get_hpdsense(void)
{
	return (u8)dw_read_mask(PHY_CONF0, PHY_CONF0_ENHPDRXSENSE_MASK);
}

static inline u8 _dw_phy_get_rxsense(void)
{
	return (u8)dw_read_mask(PHY_STAT0, PHY_STAT0_RX_SENSE_ALL_MASK);
}

static inline u8 _dw_phy_get_mpll_lock(void)
{
	return (u8)dw_read_mask(PHY_STAT0, PHY_STAT0_TX_PHY_LOCK_MASK);
}

static inline u8 _dw_phy_get_power_state(void)
{
	return (u8)dw_read_mask(PHY_CONF0, PHY_CONF0_TXPWRON_MASK);
}

static inline void _dw_phy_config_mode(void)
{
	dw_write(JTAG_PHY_CONFIG, JTAG_PHY_CONFIG_I2C_JTAGZ_MASK);
}

static void _dw_phy_i2cm_set_interrupt_mask(int mask)
{
	dw_write_mask(PHY_I2CM_INT, PHY_I2CM_INT_DONE_MASK_MASK, mask);
	dw_write_mask(PHY_I2CM_CTLINT, PHY_I2CM_CTLINT_ARBITRATION_MASK_MASK, mask);
	dw_write_mask(PHY_I2CM_CTLINT, PHY_I2CM_CTLINT_NACK_MASK_MASK, mask);
}

static u8 _dw_phy_i2cm_wait_irq_state(void)
{
	u8 state = 0;
	int times = DWPHY_MAX_TIMES;

	/* max wait (10us * DWPHY_MAX_TIMES) */
	do {
		udelay(10);
		state = dw_mc_irq_get_state(DW_MC_IRQ_PHYI2C);
	} while ((state == 0) && (times--));
	/* clear read state */
	dw_mc_irq_clear_state(DW_MC_IRQ_PHYI2C, state);

	return state;
}

static int _dw_phy_i2cm_write(u8 addr, u16 data)
{
	u8 status  = 0;

	/* Set address */
	dw_write(PHY_I2CM_ADDRESS, addr);

	/* Set value */
	dw_write(PHY_I2CM_DATAO_1, (u8)((data >> 8) & 0xFF));
	dw_write(PHY_I2CM_DATAO_0, (u8)(data & 0xFF));

	dw_write(PHY_I2CM_OPERATION, PHY_I2CM_OPERATION_WR_MASK);

	status = _dw_phy_i2cm_wait_irq_state();

	if (status & IH_I2CMPHY_STAT0_I2CMPHYERROR_MASK) {
		hdmi_err("dw phy i2cm write has error!!!\n");
		return -1;
	}

	if (status & IH_I2CMPHY_STAT0_I2CMPHYDONE_MASK)
		return 0;

	hdmi_err("dw phy i2cm wait write done state timeout!!!\n");
	return -1;
}

static int _dw_phy_i2cm_read(u8 addr, u16 *value)
{
	u8 status  = 0;

	/* Set address */
	dw_write(PHY_I2CM_ADDRESS, addr);

	dw_write(PHY_I2CM_OPERATION, PHY_I2CM_OPERATION_RD_MASK);

	status = _dw_phy_i2cm_wait_irq_state();

	if (status & IH_I2CMPHY_STAT0_I2CMPHYERROR_MASK) {
		hdmi_inf("dw phy i2m read has error!!!\n");
		return -1;
	}

	if (status & IH_I2CMPHY_STAT0_I2CMPHYDONE_MASK) {
		*value = (u16)((dw_read(PHY_I2CM_DATAI_1) << 8) | dw_read(PHY_I2CM_DATAI_0));
		return 0;
	}

	hdmi_err("dw phy i2cm wait read done state timeout!!!\n");
	return -1;
}

static int _dw_phy_set_slave_addr(void)
{
	switch (gPHY_interface) {
#ifdef SUPPORT_PHY_JTAG
	case DW_PHY_ACCESS_JTAG:
		_phy_jtag_slave_address(0xD4);
		return 0;
#endif
	case DW_PHY_ACCESS_I2C:
		dw_write_mask(PHY_I2CM_SLAVE,
				PHY_I2CM_SLAVE_SLAVEADDR_MASK, DWPHY_I2CM_ADDR);
		return 0;
	default:
		hdmi_err("PHY interface not defined");
	}
	return -1;
}

static int _dw_phy_set_interface(dw_phy_access_t interface)
{
	if (interface == gPHY_interface) {
		if (interface == DW_PHY_ACCESS_I2C) {
			hdmi_trace("dw phy continue config i2cm mode\n");
			_dw_phy_set_slave_addr();
		}
		return 0;
	}

	gPHY_interface = interface;
	dw_phy_config_interface();
	return 0;
}

u8 dw_phy_get_hpd(void)
{
	return (u8)dw_read_mask(PHY_STAT0, PHY_STAT0_HPD_MASK);
}

void dw_phy_config_svsret(void)
{
	dw_write_mask(PHY_CONF0, PHY_CONF0_SVSRET_MASK, 0x0);
	udelay(5);
	dw_write_mask(PHY_CONF0, PHY_CONF0_SVSRET_MASK, 0x1);
}

void dw_phy_set_power(u8 state)
{
	dw_write_mask(PHY_CONF0, PHY_CONF0_PDDQ_MASK, !state);
	dw_write_mask(PHY_CONF0, PHY_CONF0_TXPWRON_MASK, state);
}

int dw_phy_config_interface(void)
{
	switch (gPHY_interface) {
#ifdef SUPPORT_PHY_JTAG
	case DW_PHY_ACCESS_JTAG:
		_phy_jtag_init(0xD4);
		break;
#endif
	case DW_PHY_ACCESS_I2C:
		_dw_phy_config_mode();
		_dw_phy_set_slave_addr();
		break;
	default:
		hdmi_err("dw phy unsupport interface: %d\n", gPHY_interface);
		return -1;
	}
	hdmi_trace("dw phy config interface: %s\n",
		gPHY_interface == DW_PHY_ACCESS_I2C ? "i2c" : "jtag");
	return 0;
}

int dw_phy_standby(void)
{
	u8 phy_mask = 0;

	phy_mask |= PHY_MASK0_TX_PHY_LOCK_MASK;
	phy_mask |= PHY_MASK0_RX_SENSE_0_MASK;
	phy_mask |= PHY_MASK0_RX_SENSE_1_MASK;
	phy_mask |= PHY_MASK0_RX_SENSE_2_MASK;
	phy_mask |= PHY_MASK0_RX_SENSE_3_MASK;
	_dw_phy_set_interrupt_mask(phy_mask);
	_dw_phy_set_tmds_enable(DW_HDMI_DISABLE);
	_dw_phy_set_power_down(DW_HDMI_DISABLE);
	dw_phy_set_power(DW_HDMI_DISABLE);

	hdmi_trace("dw phy standby done\n");
	return 0;
}

u8 dw_phy_wait_lock(void)
{
	int i = 0;

	for (i = 0; i < DWPHY_MAX_TIMES; i++) {
		udelay(5);
		if (_dw_phy_get_mpll_lock())
			return DW_HDMI_ENABLE;
	}
	return DW_HDMI_DISABLE;
}

int dw_phy_write(u8 addr, u16 data)
{
	switch (gPHY_interface) {
#ifdef SUPPORT_PHY_JTAG
	case DW_PHY_ACCESS_JTAG:
		return _phy_jtag_write(addr, data);
#endif
	case DW_PHY_ACCESS_I2C:
		return _dw_phy_i2cm_write(addr, data);
	default:
		hdmi_err("PHY interface not defined");
	}
	return -1;
}

int dw_phy_read(u8 addr, u16 *value)
{
	switch (gPHY_interface) {
#ifdef SUPPORT_PHY_JTAG
	case DW_PHY_ACCESS_JTAG:
		return _phy_jtag_read(addr, value);
#endif
	case DW_PHY_ACCESS_I2C:
		return _dw_phy_i2cm_read(addr, value);
	default:
		hdmi_err("PHY interface not defined");
	}
	return -1;
}

int dw_phy_initialize(void)
{
	u8 phy_mask = 0;

	if (_dw_phy_set_interface(DW_PHY_ACCESS_I2C) < 0) {
		hdmi_err("set phy interface i2c failed!\n");
		return -1;
	}

	dw_phy_set_power(DW_HDMI_ENABLE);

	phy_mask |= PHY_MASK0_TX_PHY_LOCK_MASK;
	phy_mask |= PHY_STAT0_HPD_MASK;
	phy_mask |= PHY_MASK0_RX_SENSE_0_MASK;
	phy_mask |= PHY_MASK0_RX_SENSE_1_MASK;
	phy_mask |= PHY_MASK0_RX_SENSE_2_MASK;
	phy_mask |= PHY_MASK0_RX_SENSE_3_MASK;
	_dw_phy_set_interrupt_mask(phy_mask);

	_dw_phy_set_data_enable_polarity(DW_HDMI_ENABLE);

	_dw_phy_set_interface_control(DW_HDMI_DISABLE);

	_dw_phy_set_tmds_enable(DW_HDMI_DISABLE);

	_dw_phy_set_power_down(DW_HDMI_DISABLE);

	_dw_phy_i2cm_set_interrupt_mask(DW_HDMI_DISABLE);

	dw_mc_irq_clear_state(DW_MC_IRQ_PHYI2C, 0x0);

	return 0;
}

int dw_phy_init(void)
{
	struct dw_hdmi_dev_s *hdmi = dw_get_hdmi();

	gPHY_interface = DW_PHY_ACCESS_I2C;

	/* enable hpd exsense */
	_dw_phy_set_hpdsense(DW_HDMI_ENABLE);

	if (hdmi->phy_ext->phy_init)
		hdmi->phy_ext->phy_init();

	return 0;
}

ssize_t dw_phy_dump(char *buf)
{
	int n = 0;

	n += sprintf(buf + n, "\n[dw phy]\n");
	n += sprintf(buf + n, "|  name |  mpll  | power | rxsense | hpd | hpd sense |\n");
	n += sprintf(buf + n, "|-------+--------+--------+--------+-----+-----------|\n");
	n += sprintf(buf + n, "| state | %-6s |  %-3s  |  0x%-4x | %-3s |   %-7s |\n",
		_dw_phy_get_mpll_lock() ? "lock" : "unlock",
		_dw_phy_get_power_state() ? "on" : "off",
		_dw_phy_get_rxsense(),
		dw_phy_get_hpd() ? "in" : "out",
		_dw_phy_get_hpdsense() ? "enable" : "disable");
	return n;
}
