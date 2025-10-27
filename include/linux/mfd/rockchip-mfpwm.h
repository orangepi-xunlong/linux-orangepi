/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (c) 2025 Collabora Ltd.
 *
 * Common header file for all the Rockchip Multi-function PWM controller
 * drivers that are spread across subsystems.
 *
 * Authors:
 *     Nicolas Frattaroli <nicolas.frattaroli@collabora.com>
 */

#ifndef __SOC_ROCKCHIP_MFPWM_H__
#define __SOC_ROCKCHIP_MFPWM_H__

#include <linux/bits.h>
#include <linux/clk.h>
#include <linux/hw_bitfield.h>
#include <linux/io.h>
#include <linux/spinlock.h>

struct rockchip_mfpwm;

/**
 * struct rockchip_mfpwm_func - struct representing a single function driver
 *
 * @id: unique id for this function driver instance
 * @base: pointer to start of MMIO registers
 * @parent: a pointer to the parent mfpwm struct
 * @irq: the shared IRQ gotten from the parent mfpwm device
 * @core: a pointer to the clk mux that drives this channel's PWM
 */
struct rockchip_mfpwm_func {
	int id;
	void __iomem *base;
	struct rockchip_mfpwm *parent;
	int irq;
	struct clk *core;
};

/*
 * PWMV4 Register Definitions
 * --------------------------
 *
 * Attributes:
 *  RW  - Read-Write
 *  RO  - Read-Only
 *  WO  - Write-Only
 *  W1T - Write high, Self-clearing
 *  W1C - Write high to clear interrupt
 *
 * Bit ranges to be understood with Verilog-like semantics,
 * e.g. [03:00] is 4 bits: 0, 1, 2 and 3.
 *
 * All registers must be accessed with 32-bit width accesses only
 */

#define PWMV4_REG_VERSION		0x000
/*
 * VERSION Register Description
 * [31:24] RO  | Hardware Major Version
 * [23:16] RO  | Hardware Minor Version
 * [15:15] RO  | Reserved
 * [14:14] RO  | Hardware supports biphasic counters
 * [13:13] RO  | Hardware supports filters
 * [12:12] RO  | Hardware supports waveform generation
 * [11:11] RO  | Hardware supports counter
 * [10:10] RO  | Hardware supports frequency metering
 * [09:09] RO  | Hardware supports power key functionality
 * [08:08] RO  | Hardware supports infrared transmissions
 * [07:04] RO  | Channel index of this instance
 * [03:00] RO  | Number of channels the base instance supports
 */

#define PWMV4_REG_ENABLE		0x004
/*
 * ENABLE Register Description
 * [31:16] WO  | Write Enable Mask for the lower half of the register
 *               Set bit `n` here to 1 if you wish to modify bit `n >> 16` in
 *               the same write operation
 * [15:06] RO  | Reserved
 * [05:05] RW  | PWM Channel Counter Read Enable, 1 = enabled
 */
#define PWMV4_CHN_CNT_RD_EN(v)		FIELD_PREP_WM16(BIT(5), (v))
/*
 * [04:04] W1T | PWM Globally Joined Control Enable
 *               1 = this PWM channel will be enabled by a global pwm enable
 *               bit instead of the PWM Enable bit.
 */
#define PWMV4_GLOBAL_CTRL_EN(v)		FIELD_PREP_WM16(BIT(4), (v))
/*
 * [03:03] RW  | Force Clock Enable
 *               0 = disabled, if the PWM channel is inactive then so is the
 *               clock prescale module
 */
#define PWMV4_FORCE_CLK_EN(v)		FIELD_PREP_WM16(BIT(3), (v))
/*
 * [02:02] W1T | PWM Control Update Enable
 *               1 = enabled, commits modifications of _CTRL, _PERIOD, _DUTY and
 *               _OFFSET registers once 1 is written to it
 */
#define PWMV4_CTRL_UPDATE_EN		FIELD_PREP_WM16_CONST(BIT(2), 1)
/*
 * [01:01] RW  | PWM Enable, 1 = enabled
 *               If in one-shot mode, clears after end of operation
 */
#define PWMV4_EN_MASK			BIT(1)
#define PWMV4_EN(v)			FIELD_PREP_WM16(PWMV4_EN_MASK, \
							((v) ? 1 : 0))
/*
 * [00:00] RW  | PWM Clock Enable, 1 = enabled
 *               If in one-shot mode, clears after end of operation
 */
#define PWMV4_CLK_EN_MASK		BIT(0)
#define PWMV4_CLK_EN(v)			FIELD_PREP_WM16(PWMV4_CLK_EN_MASK, \
							((v) ? 1 : 0))
#define PWMV4_EN_BOTH_MASK		(PWMV4_EN_MASK | PWMV4_CLK_EN_MASK)
static inline __pure bool rockchip_pwm_v4_is_enabled(unsigned int val)
{
	return (val & PWMV4_EN_BOTH_MASK);
}

#define PWMV4_REG_CLK_CTRL		0x008
/*
 * CLK_CTRL Register Description
 * [31:16] WO  | Write Enable Mask for the lower half of the register
 *               Set bit `n` here to 1 if you wish to modify bit `n >> 16` in
 *               the same write operation
 * [15:15] RW  | Clock Global Selection
 *               0 = current channel scale clock
 *               1 = global channel scale clock
 */
#define PWMV4_CLK_GLOBAL(v)		FIELD_PREP_WM16(BIT(15), (v))
/*
 * [14:13] RW  | Clock Source Selection
 *               0 = Clock from PLL, frequency can be configured
 *               1 = Clock from crystal oscillator, frequency is fixed
 *               2 = Clock from RC oscillator, frequency is fixed
 *               3 = Reserved
 *               NOTE: The purpose for this clock-mux-outside-CRU construct is
 *                     to let the SoC go into a sleep state with the PWM
 *                     hardware still having a clock signal for IR input, which
 *                     can then wake up the SoC.
 */
#define PWMV4_CLK_SRC_PLL		0x0U
#define PWMV4_CLK_SRC_CRYSTAL		0x1U
#define PWMV4_CLK_SRC_RC		0x2U
#define PWMV4_CLK_SRC_SHIFT		13
#define PWMV4_CLK_SRC_WIDTH		2
/*
 * [12:04] RW  | Scale Factor to apply to pre-scaled clock
 *               1 <= v <= 256, v means clock divided by 2*v
 */
#define PWMV4_CLK_SCALE_F(v)		FIELD_PREP_WM16(GENMASK(12, 4), (v))
/*
 * [03:03] RO  | Reserved
 * [02:00] RW  | Prescale Factor
 *               v here means the input clock is divided by pow(2, v)
 */
#define PWMV4_CLK_PRESCALE_F(v)		FIELD_PREP_WM16(GENMASK(2, 0), (v))

#define PWMV4_REG_CTRL			0x00C
/*
 * CTRL Register Description
 * [31:16] WO  | Write Enable Mask for the lower half of the register
 *               Set bit `n` here to 1 if you wish to modify bit `n >> 16` in
 *               the same write operation
 * [15:09] RO  | Reserved
 * [08:06] RW  | PWM Input Channel Selection
 *               By default, the channel selects its own input, but writing v
 *               here selects PWM input from channel v instead.
 */
#define PWMV4_CTRL_IN_SEL(v)		FIELD_PREP_WM16(GENMASK(8, 6), (v))
/* [05:05] RW  | Aligned Mode, 0 = Valid, 1 = Invalid */
#define PWMV4_CTRL_UNALIGNED(v)		FIELD_PREP_WM16(BIT(5), (v))
/* [04:04] RW  | Output Mode, 0 = Left Aligned, 1 = Centre Aligned */
#define PWMV4_LEFT_ALIGNED		0x0U
#define PWMV4_CENTRE_ALIGNED		0x1U
#define PWMV4_CTRL_OUT_MODE(v)		FIELD_PREP_WM16(BIT(4), (v))
/*
 * [03:03] RW  | Inactive Polarity for when the channel is either disabled or
 *               has completed outputting the entire waveform in one-shot mode.
 *               0 = Negative, 1 = Positive
 */
#define PWMV4_POLARITY_N		0x0U
#define PWMV4_POLARITY_P		0x1U
#define PWMV4_INACTIVE_POL(v)		FIELD_PREP_WM16(BIT(3), (v))
/*
 * [02:02] RW  | Duty Cycle Polarity to use at the start of the waveform.
 *               0 = Negative, 1 = Positive
 */
#define PWMV4_DUTY_POL_SHIFT		2
#define PWMV4_DUTY_POL_MASK		BIT(PWMV4_DUTY_POL_SHIFT)
#define PWMV4_DUTY_POL(v)		FIELD_PREP_WM16(PWMV4_DUTY_POL_MASK, \
							(v))
/*
 * [01:00] RW  | PWM Mode
 *               0 = One-shot mode, PWM generates waveform RPT times
 *               1 = Continuous mode
 *               2 = Capture mode, PWM measures cycles of input waveform
 *               3 = Reserved
 */
#define PWMV4_MODE_ONESHOT		0x0U
#define PWMV4_MODE_CONT			0x1U
#define PWMV4_MODE_CAPTURE		0x2U
#define PWMV4_MODE_MASK			GENMASK(1, 0)
#define PWMV4_MODE(v)			FIELD_PREP_WM16(PWMV4_MODE_MASK, (v))
#define PWMV4_CTRL_COM_FLAGS	(PWMV4_INACTIVE_POL(PWMV4_POLARITY_N) | \
				 PWMV4_DUTY_POL(PWMV4_POLARITY_P) | \
				 PWMV4_CTRL_OUT_MODE(PWMV4_LEFT_ALIGNED) | \
				 PWMV4_CTRL_UNALIGNED(true))
#define PWMV4_CTRL_CONT_FLAGS	(PWMV4_MODE(PWMV4_MODE_CONT) | \
				 PWMV4_CTRL_COM_FLAGS)
#define PWMV4_CTRL_CAP_FLAGS	(PWMV4_MODE(PWMV4_MODE_CAPTURE) | \
				 PWMV4_CTRL_COM_FLAGS)

#define PWMV4_REG_PERIOD		0x010
/*
 * PERIOD Register Description
 * [31:00] RW  | Period of the output waveform
 *               Constraints: should be even if CTRL_OUT_MODE is CENTRE_ALIGNED
 */

#define PWMV4_REG_DUTY			0x014
/*
 * DUTY Register Description
 * [31:00] RW  | Duty cycle of the output waveform
 *               Constraints: should be even if CTRL_OUT_MODE is CENTRE_ALIGNED
 */

#define PWMV4_REG_OFFSET		0x018
/*
 * OFFSET Register Description
 * [31:00] RW  | Offset of the output waveform, based on the PWM clock
 *               Constraints: 0 <= v <= (PERIOD - DUTY)
 */

#define PWMV4_REG_RPT			0x01C
/*
 * RPT Register Description
 * [31:16] RW  | Second dimensional of the effective number of waveform
 *               repetitions. Increases by one every first dimensional times.
 *               Value `n` means `n + 1` repetitions. The final number of
 *               repetitions of the waveform in one-shot mode is:
 *               `(first_dimensional + 1) * (second_dimensional + 1)`
 * [15:00] RW  | First dimensional of the effective number of waveform
 *               repetitions. Value `n` means `n + 1` repetitions.
 */

#define PWMV4_REG_FILTER_CTRL		0x020
/*
 * FILTER_CTRL Register Description
 * [31:16] WO  | Write Enable Mask for the lower half of the register
 *               Set bit `n` here to 1 if you wish to modify bit `n >> 16` in
 *               the same write operation
 * [15:10] RO  | Reserved
 * [09:04] RW  | Filter window number
 * [03:01] RO  | Reserved
 * [00:00] RW  | Filter Enable, 0 = disabled, 1 = enabled
 */

#define PWMV4_REG_CNT			0x024
/*
 * CNT Register Description
 * [31:00] RO  | Current value of the PWM Channel 0 counter in pwm clock cycles,
 *               0 <= v <= 2^32-1
 */

#define PWMV4_REG_ENABLE_DELAY		0x028
/*
 * ENABLE_DELAY Register Description
 * [31:16] RO  | Reserved
 * [15:00] RW  | PWM enable delay, in an unknown unit but probably cycles
 */

#define PWMV4_REG_HPC			0x02C
/*
 * HPC Register Description
 * [31:00] RW  | Number of effective high polarity cycles of the input waveform
 *               in capture mode. Based on the PWM clock. 0 <= v <= 2^32-1
 */

#define PWMV4_REG_LPC			0x030
/*
 * LPC Register Description
 * [31:00] RW  | Number of effective low polarity cycles of the input waveform
 *               in capture mode. Based on the PWM clock. 0 <= v <= 2^32-1
 */

#define PWMV4_REG_BIPHASIC_CNT_CTRL0	0x040
/*
 * BIPHASIC_CNT_CTRL0 Register Description
 * [31:16] WO  | Write Enable Mask for the lower half of the register
 *               Set bit `n` here to 1 if you wish to modify bit `n >> 16` in
 *               the same write operation
 * [15:10] RO  | Reserved
 * [09:09] RW  | Biphasic Counter Phase Edge Selection for mode 0,
 *               0 = rising edge (posedge), 1 = falling edge (negedge)
 * [08:08] RW  | Biphasic Counter Clock force enable, 1 = force enable
 * [07:07] W1T | Synchronous Enable
 * [06:06] W1T | Mode Switch
 *               0 = Normal Mode, 1 = Switch timer clock and measured clock
 *               Constraints: "Biphasic Counter Mode" must be 0 if this is 1
 * [05:03] RW  | Biphasic Counter Mode
 *               0x0 = Mode 0, 0x1 = Mode 1, 0x2 = Mode 2, 0x3 = Mode 3,
 *               0x4 = Mode 4, 0x5 = Reserved
 * [02:02] RW  | Biphasic Counter Clock Selection
 *               0 = clock is from PLL and frequency can be configured
 *               1 = clock is from crystal oscillator and frequency is fixed
 * [01:01] RW  | Biphasic Counter Continuous Mode
 * [00:00] W1T | Biphasic Counter Enable
 */

#define PWMV4_REG_BIPHASIC_CNT_CTRL1	0x044
/*
 * BIPHASIC_CNT_CTRL1 Register Description
 * [31:16] WO  | Write Enable Mask for the lower half of the register
 *               Set bit `n` here to 1 if you wish to modify bit `n >> 16` in
 *               the same write operation
 * [15:11] RO  | Reserved
 * [10:04] RW  | Biphasic Counter Filter Window Number
 * [03:01] RO  | Reserved
 * [00:00] RW  | Biphasic Counter Filter Enable
 */

#define PWMV4_REG_BIPHASIC_CNT_TIMER	0x048
/*
 * BIPHASIC_CNT_TIMER Register Description
 * [31:00] RW  | Biphasic Counter Timer Value, in number of biphasic counter
 *               timer clock cycles
 */

#define PWMV4_REG_BIPHASIC_CNT_RES	0x04C
/*
 * BIPHASIC_CNT_RES Register Description
 * [31:00] RO  | Biphasic Counter Result Value
 *               Constraints: Can only be read after INTSTS[9] is asserted
 */

#define PWMV4_REG_BIPHASIC_CNT_RES_S	0x050
/*
 * BIPHASIC_CNT_RES_S Register Description
 * [31:00] RO  | Biphasic Counter Result Value with synchronised processing
 *               Can be read in real-time if BIPHASIC_CNT_CTRL0[7] was set to 1
 */

#define PWMV4_REG_INTSTS		0x070
/*
 * INTSTS Register Description
 * [31:10] RO  | Reserved
 * [09:09] W1C | Biphasic Counter Interrupt Status, 1 = interrupt asserted
 * [08:08] W1C | Waveform Middle Interrupt Status, 1 = interrupt asserted
 * [07:07] W1C | Waveform Max Interrupt Status, 1 = interrupt asserted
 * [06:06] W1C | IR Transmission End Interrupt Status, 1 = interrupt asserted
 * [05:05] W1C | Power Key Match Interrupt Status, 1 = interrupt asserted
 * [04:04] W1C | Frequency Meter Interrupt Status, 1 = interrupt asserted
 * [03:03] W1C | Reload Interrupt Status, 1 = interrupt asserted
 * [02:02] W1C | Oneshot End Interrupt Status, 1 = interrupt asserted
 * [01:01] W1C | HPC Capture Interrupt Status, 1 = interrupt asserted
 * [00:00] W1C | LPC Capture Interrupt Status, 1 = interrupt asserted
 */
#define PWMV4_INT_LPC			BIT(0)
#define PWMV4_INT_HPC			BIT(1)
#define PWMV4_INT_LPC_W(v)		FIELD_PREP_WM16(PWMV4_INT_LPC, \
							((v) ? 1 : 0))
#define PWMV4_INT_HPC_W(v)		FIELD_PREP_WM16(PWMV4_INT_HPC, \
							((v) ? 1 : 0))

#define PWMV4_REG_INT_EN		0x074
/*
 * INT_EN Register Description
 * [31:16] WO  | Write Enable Mask for the lower half of the register
 *               Set bit `n` here to 1 if you wish to modify bit `n >> 16` in
 *               the same write operation
 * [15:10] RO  | Reserved
 * [09:09] RW  | Biphasic Counter Interrupt Enable, 1 = enabled
 * [08:08] W1C | Waveform Middle Interrupt Enable, 1 = enabled
 * [07:07] W1C | Waveform Max Interrupt Enable, 1 = enabled
 * [06:06] W1C | IR Transmission End Interrupt Enable, 1 = enabled
 * [05:05] W1C | Power Key Match Interrupt Enable, 1 = enabled
 * [04:04] W1C | Frequency Meter Interrupt Enable, 1 = enabled
 * [03:03] W1C | Reload Interrupt Enable, 1 = enabled
 * [02:02] W1C | Oneshot End Interrupt Enable, 1 = enabled
 * [01:01] W1C | HPC Capture Interrupt Enable, 1 = enabled
 * [00:00] W1C | LPC Capture Interrupt Enable, 1 = enabled
 */

#define PWMV4_REG_INT_MASK		0x078
/*
 * INT_MASK Register Description
 * [31:16] WO  | Write Enable Mask for the lower half of the register
 *               Set bit `n` here to 1 if you wish to modify bit `n >> 16` in
 *               the same write operation
 * [15:10] RO  | Reserved
 * [09:09] RW  | Biphasic Counter Interrupt Masked, 1 = masked
 * [08:08] W1C | Waveform Middle Interrupt Masked, 1 = masked
 * [07:07] W1C | Waveform Max Interrupt Masked, 1 = masked
 * [06:06] W1C | IR Transmission End Interrupt Masked, 1 = masked
 * [05:05] W1C | Power Key Match Interrupt Masked, 1 = masked
 * [04:04] W1C | Frequency Meter Interrupt Masked, 1 = masked
 * [03:03] W1C | Reload Interrupt Masked, 1 = masked
 * [02:02] W1C | Oneshot End Interrupt Masked, 1 = masked
 * [01:01] W1C | HPC Capture Interrupt Masked, 1 = masked
 * [00:00] W1C | LPC Capture Interrupt Masked, 1 = masked
 */

static inline u32 mfpwm_reg_read(void __iomem *base, u32 reg)
{
	return readl(base + reg);
}

static inline void mfpwm_reg_write(void __iomem *base, u32 reg, u32 val)
{
	writel(val, base + reg);
}

/**
 * mfpwm_acquire - try becoming the active mfpwm function device
 * @pwmf: pointer to the calling driver instance's &struct rockchip_mfpwm_func
 *
 * mfpwm device "function" drivers must call this function before doing anything
 * that either modifies or relies on the parent device's state, such as clocks,
 * enabling/disabling outputs, modifying shared regs etc.
 *
 * The return statues should always be checked.
 *
 * All mfpwm_acquire() calls must be balanced with corresponding mfpwm_release()
 * calls once the device is no longer making changes that affect other devices,
 * or stops producing user-visible effects that depend on the current device
 * state being kept as-is. (e.g. after the PWM output signal is stopped)
 *
 * The same device function may mfpwm_acquire() multiple times while it already
 * is active, i.e. it is re-entrant, though it needs to balance this with the
 * same number of mfpwm_release() calls.
 *
 * Context: This function does not sleep.
 *
 * Return:
 * * %0                 - success
 * * %-EBUSY            - a different device function is active
 * * %-EOVERFLOW        - the acquire counter is at its maximum
 */
extern int __must_check mfpwm_acquire(struct rockchip_mfpwm_func *pwmf);

/**
 * mfpwm_release - drop usage of active mfpwm device function by 1
 * @pwmf: pointer to the calling driver instance's &struct rockchip_mfpwm_func
 *
 * This is the balancing call to mfpwm_acquire(). If no users of the device
 * function remain, set the mfpwm device to have no active device function,
 * allowing other device functions to claim it.
 */
extern void mfpwm_release(const struct rockchip_mfpwm_func *pwmf);

#endif /* __SOC_ROCKCHIP_MFPWM_H__ */
