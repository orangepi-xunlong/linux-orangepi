/*
 * Support code for jtag facilities jtagm - OS independent.
 *
 * Copyright (C) 2024 Synaptics Incorporated. All rights reserved.
 *
 * This software is licensed to you under the terms of the
 * GNU General Public License version 2 (the "GPL") with Broadcom special exception.
 *
 * INFORMATION CONTAINED IN THIS DOCUMENT IS PROVIDED "AS-IS," AND SYNAPTICS
 * EXPRESSLY DISCLAIMS ALL EXPRESS AND IMPLIED WARRANTIES, INCLUDING ANY
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE,
 * AND ANY WARRANTIES OF NON-INFRINGEMENT OF ANY INTELLECTUAL PROPERTY RIGHTS.
 * IN NO EVENT SHALL SYNAPTICS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, PUNITIVE, OR CONSEQUENTIAL DAMAGES ARISING OUT OF OR IN CONNECTION
 * WITH THE USE OF THE INFORMATION CONTAINED IN THIS DOCUMENT, HOWEVER CAUSED
 * AND BASED ON ANY THEORY OF LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, AND EVEN IF SYNAPTICS WAS ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE. IF A TRIBUNAL OF COMPETENT JURISDICTION
 * DOES NOT PERMIT THE DISCLAIMER OF DIRECT DAMAGES OR ANY OTHER DAMAGES,
 * SYNAPTICS' TOTAL CUMULATIVE LIABILITY TO ANY PARTY SHALL NOT
 * EXCEED ONE HUNDRED U.S. DOLLARS
 *
 * Copyright (C) 2024, Broadcom.
 *
 *      Unless you and Broadcom execute a separate written software license
 * agreement governing use of this software, this software is licensed to you
 * under the terms of the GNU General Public License version 2 (the "GPL"),
 * available at http://www.broadcom.com/licenses/GPLv2.php, with the
 * following added to such license:
 *
 *      As a special exception, the copyright holders of this software give you
 * permission to link this software with independent modules, and to copy and
 * distribute the resulting executable under terms of your choice, provided that
 * you also meet, for each linked independent module, the terms and conditions of
 * the license of that module.  An independent module is a module which is not
 * derived from this software.  The special exception does not apply to any
 * modifications of the software.
 *
 *
 * <<Broadcom-WL-IPTag/Dual:>>
 */

#include <typedefs.h>
#include <bcmdefs.h>
#include <osl.h>
#include <bcmutils.h>
#include <siutils.h>
#include <bcmnvram.h>
#include <hndsoc.h>
#include <sbchipc.h>
#include <hndchipc.h>
#include <hndjtagdefs.h>
#include "siutils_priv.h"

/* debug/trace */
#ifdef BCMDBG_ERR
#define CC_ERROR(args)	printf args
#else
#define CC_ERROR(args)
#endif /* BCMDBG_ERR */

#ifdef BCMDBG
#define CC_MSG(args)	printf args
#else
#define CC_MSG(args)
#endif /* BCMDBG */

#define JTAG_RETRIES	10000

/*
 * Initializes jtag master and returns handle for other JTAG-related functions
 * clkd          - 0 or value for JtagMasterClkDiv field of ClkDiv register
 * exttap        - 1 to control external TAP controller, 0 for internal TAP controller
 * prev_jtagctrl - optional output parameter: value of JtagMasterCtrl register before
 *                 call of this function. May be subsequently used by hnd_jtagm_disable()
 * Returns NULL on failure.
 */
volatile void *
hnd_jtagm_init(si_t *sih, uint clkd, bool exttap, uint32 *prev_jtagctrl)
{
	volatile void *regs;
	si_info_t *sii;

	ASSERT(sih != NULL);

	sii = SI_INFO(sih);
	ASSERT((sii != NULL) && (sii->osh != NULL));

	if ((regs = si_setcoreidx(sih, SI_CC_IDX)) != NULL) {
		chipcregs_t *cc = (chipcregs_t *) regs;
		uint32 tmp;

		/*
		 * Determine jtagm availability from
		 * core revision and capabilities.
		 */

		/*
		 * Corerev 10 has jtagm, but the only chip
		 * with it does not have a mips, and
		 * the layout of the jtagcmd register is
		 * different. We'll only accept >= 11.
		 */
		if (sih->ccrev < 11)
			return (NULL);

		if ((sih->cccaps & CC_CAP_JTAGP) == 0)
			return (NULL);

		/* Set clock divider if requested */
		if (clkd != 0) {
			tmp = R_REG(sii->osh, &cc->clkdiv);
			tmp = (tmp & ~CLKD_JTAG) |
				((clkd << CLKD_JTAG_SHIFT) & CLKD_JTAG);
			W_REG(sii->osh, &cc->clkdiv, tmp);
		}

		/* Enable jtagm */
		tmp = R_REG(sii->osh, &cc->jtagctrl);
		if (prev_jtagctrl) {
			*prev_jtagctrl = tmp;
		}
		tmp |= JCTRL_EN | (exttap ? JCTRL_EXT_EN : 0);
		W_REG(sii->osh, &cc->jtagctrl, tmp);
	}
	return (regs);
}

/* Terminates JTAG opertions
 * h             - handle, returned by hnd_jtagm_init()
 * prev_jtagctrl - NULL or address of JtagMasterCtrl register value before
 *                 hnd_jtagm_init(). NULL causes cleraring of JtagEnable bit
 *                 in JtagMasterCtrl register, non-NULL causes restoring this
 *                 value to JtagMasterCtrl register
 */
void
hnd_jtagm_disable(si_t *sih, volatile void *h, uint32 *prev_jtagctrl)
{
	si_info_t *sii;
	chipcregs_t *cc = (chipcregs_t *)h;

	ASSERT(sih != NULL);
	sii = SI_INFO(sih);
	ASSERT((sii != NULL) && (sii->osh != NULL));

	W_REG(sii->osh, &cc->jtagctrl,
		prev_jtagctrl ? *prev_jtagctrl : R_REG(sii->osh, &cc->jtagctrl) & ~JCTRL_EN);
}

static uint32
jtm_wait(chipcregs_t *cc, bool readdr, si_info_t *sii)
{
	uint i;

	i = 0;
	while (((R_REG(sii->osh, &cc->jtagcmd) & JCMD_BUSY) == JCMD_BUSY) &&
	       (i < JTAG_RETRIES)) {
		i++;
	}

	if (i >= JTAG_RETRIES)
		return 0xbadbad03;

	if (readdr)
		return R_REG(sii->osh, &cc->jtagdr);
	else
		return 0xffffffff;
}

/* Read/write a jtag register. Assumes both ir and dr <= 64bits. */

uint32
jtag_scan(si_t *sih, volatile void *h, uint irsz, uint32 ir0, uint32 ir1,
          uint drsz, uint32 dr0, uint32 *dr1, bool rti)
{
	chipcregs_t *cc = (chipcregs_t *) h;
	uint32 acc_dr, acc_irdr;
	uint32 tmp;
	si_info_t *sii;

	ASSERT(sih != NULL);
	sii = SI_INFO(sih);
	ASSERT((sii != NULL) && (sii->osh != NULL));

	if ((irsz > 64) || (drsz > 64)) {
		return 0xbadbad00;
	}
	if (rti) {
		if (sih->ccrev < 28)
			return 0xbadbad01;
		acc_irdr = JCMD_ACC_IRDR_I;
		acc_dr = JCMD_ACC_DR_I;
	} else {
		acc_irdr = JCMD_ACC_IRDR;
		acc_dr = JCMD_ACC_DR;
	}
	if (irsz == 0) {
		/* scan in the first (or only) DR word with a dr-only command */
		W_REG(sii->osh, &cc->jtagdr, dr0);
		if (drsz > 32) {
			W_REG(sii->osh, &cc->jtagcmd, JCMD_START | JCMD_ACC_PDR | 31);
			drsz -= 32;
		} else
			W_REG(sii->osh, &cc->jtagcmd, JCMD_START | acc_dr | (drsz - 1));
	} else {
		W_REG(sii->osh, &cc->jtagir, ir0);
		if (irsz > 32) {
			/* Use Partial IR for first IR word */
			W_REG(sii->osh, &cc->jtagcmd, JCMD_START | JCMD_ACC_PIR |
			      (31 << JCMD_IRW_SHIFT));
			jtm_wait(cc, FALSE, sii);
			W_REG(sii->osh, &cc->jtagir, ir1);
			irsz -= 32;
		}
		if (drsz == 0) {
			/* If drsz is 0, do an IR-only scan and that's it */
			W_REG(sii->osh, &cc->jtagcmd, JCMD_START | JCMD_ACC_IR |
			      ((irsz - 1) << JCMD_IRW_SHIFT));
			return jtm_wait(cc, FALSE, sii);
		}
		/* Now scan in the IR word and the first (or only) DR word */
		W_REG(sii->osh, &cc->jtagdr, dr0);
		if (drsz <= 32)
			W_REG(sii->osh, &cc->jtagcmd, JCMD_START | acc_irdr |
			      ((irsz - 1) << JCMD_IRW_SHIFT) | (drsz - 1));
		else
			W_REG(sii->osh, &cc->jtagcmd, JCMD_START | JCMD_ACC_IRPDR |
			      ((irsz - 1) << JCMD_IRW_SHIFT) | 31);
	}
	/* Now scan out the DR and scan in & out the second DR word if needed */
	tmp = jtm_wait(cc, TRUE, sii);
	if (drsz > 32) {
		if (dr1 == NULL)
			return 0xbadbad04;
		W_REG(sii->osh, &cc->jtagdr, *dr1);
		W_REG(sii->osh, &cc->jtagcmd, JCMD_START | acc_dr | (drsz - 33));
		*dr1 = jtm_wait(cc, TRUE, sii);
	}
	return (tmp);
}

uint32
jtag_read_128(si_t *sih, volatile void *h, uint irsz, uint32 ir0, uint drsz,
	uint32 dr0, uint32 *dr1, uint32 *dr2, uint32 *dr3)
{
	chipcregs_t *cc = (chipcregs_t *) h;
	uint32 tmp;
	si_info_t *sii;

	ASSERT(sih != NULL);
	sii = SI_INFO(sih);
	ASSERT((sii != NULL) && (sii->osh != NULL));
	BCM_REFERENCE(tmp);

	if ((irsz != 128) || (drsz != 128)) {
		return 0xbadbad00;
	}

	/* Write the user reg address bit 31:0 */
	W_REG(sii->osh, &cc->jtagir, ir0);
	/* Write jtag cmd */
	W_REG(sii->osh, &cc->jtagcmd, JCMD_START | JCMD_ACC_PIR |
	      (31 << JCMD_IRW_SHIFT));
	tmp = jtm_wait(cc, FALSE, sii);

	/* write user reg address bit 37:32 */
	W_REG(sii->osh, &cc->jtagir, 0x3f);

	/* Read Word 0 */
	W_REG(sii->osh, &cc->jtagdr, 0x0);
	W_REG(sii->osh, &cc->jtagcmd, 0x8004051f);
	dr0 = jtm_wait(cc, TRUE, sii);

	/* Read Word 1 */
	W_REG(sii->osh, &cc->jtagdr, 0x0);
	W_REG(sii->osh, &cc->jtagcmd, 0x8005251f);
	*dr1 = jtm_wait(cc, TRUE, sii);

	/* Read Word 2 */
	W_REG(sii->osh, &cc->jtagdr, 0x0);
	W_REG(sii->osh, &cc->jtagcmd, 0x8005251f);
	*dr2 = jtm_wait(cc, TRUE, sii);

	/* Read Word 3 */
	W_REG(sii->osh, &cc->jtagdr, 0x0);
	W_REG(sii->osh, &cc->jtagcmd, 0x8001251f);
	*dr3 = jtm_wait(cc, TRUE, sii);
	return (dr0);
}

uint32
jtag_write_128(si_t *sih, volatile void *h, uint irsz, uint32 ir0, uint drsz,
	uint32 dr0, uint32 *dr1, uint32 *dr2, uint32 *dr3)
{
	chipcregs_t *cc = (chipcregs_t *) h;
	uint32 tmp;
	si_info_t *sii;

	ASSERT(sih != NULL);
	sii = SI_INFO(sih);
	ASSERT((sii != NULL) && (sii->osh != NULL));
	BCM_REFERENCE(tmp);

	if ((irsz != 128) || (drsz != 128)) {
		return 0xbadbad00;
	}

	BCM_REFERENCE(cc);

	/* Write the user reg address */
	W_REG(sii->osh, &cc->jtagir, ir0);

	/* Write jtag cmd */
	W_REG(sii->osh, &cc->jtagcmd, 0x80061f00);
	tmp = jtm_wait(cc, FALSE, sii);

	W_REG(sii->osh, &cc->jtagir, 0x3f);

	/* Write Word 0 */
	W_REG(sii->osh, &cc->jtagdr, dr0);
	W_REG(sii->osh, &cc->jtagcmd, 0x8004051f);
	tmp = jtm_wait(cc, FALSE, sii);

	/* Write Word 1 */
	W_REG(sii->osh, &cc->jtagdr, *dr1);
	W_REG(sii->osh, &cc->jtagcmd, 0x8005251f);
	tmp = jtm_wait(cc, FALSE, sii);

	/* Write Word 2 */
	W_REG(sii->osh, &cc->jtagdr, *dr2);
	W_REG(sii->osh, &cc->jtagcmd, 0x8005251f);
	tmp = jtm_wait(cc, FALSE, sii);

	/* Write Word 3 */
	W_REG(sii->osh, &cc->jtagdr, *dr3);
	W_REG(sii->osh, &cc->jtagcmd, 0x8001251f);
	tmp = jtm_wait(cc, FALSE, sii);

	return (tmp);
}

int
jtag_setbit_128(si_t *sih, uint32 jtagureg_addr, uint8 bit_pos, uint8 bit_val)
{
	volatile void *jh;
	int savecidx, ret;
	uint32 dr[4] = {0};
	uint32 prev_jtagctrl = 0;

	/* hnd_jtagm_init does a setcore to chipc */
	savecidx = si_coreidx(sih);

	if ((jh = hnd_jtagm_init(sih, 0, FALSE, &prev_jtagctrl)) != NULL) {
		dr[0] = jtag_read_128(sih, jh, LV_IR_SIZE_128, LV_38_UREG_ROIR(jtagureg_addr),
			LV_DR_SIZE_128, dr[0], &dr[1], &dr[2], &dr[3]);

		if (bit_val) {
			setbit((uint8 *)dr, bit_pos);
		} else {
			clrbit((uint8 *)dr, bit_pos);
		}

		jtag_write_128(sih, jh, LV_IR_SIZE_128, LV_38_UREG_IR(jtagureg_addr),
			LV_DR_SIZE_128, dr[0], &dr[1], &dr[2], &dr[3]);

		hnd_jtagm_disable(sih, jh, &prev_jtagctrl);
		ret = BCME_OK;
	} else {
		ret = BCME_ERROR;
	}

	si_setcoreidx(sih, savecidx);

	return ret;

}
