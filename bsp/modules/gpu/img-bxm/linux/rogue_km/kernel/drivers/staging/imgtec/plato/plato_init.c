/*
 * @File        plato_init.c
 * @Codingstyle LinuxKernel
 * @Copyright   Copyright (c) Imagination Technologies Ltd. All Rights Reserved
 * @License     Dual MIT/GPLv2
 *
 * The contents of this file are subject to the MIT license as set out below.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * Alternatively, the contents of this file may be used under the terms of
 * the GNU General Public License Version 2 ("GPL") in which case the provisions
 * of GPL are applicable instead of those above.
 *
 * If you wish to allow use of your version of this file only under the terms of
 * GPL, and not to allow others to use your version of this file under the terms
 * of the MIT license, indicate your decision by deleting the provisions above
 * and replace them with the notice and other provisions required by GPL as set
 * out in the file called "GPL-COPYING" included in this distribution. If you do
 * not delete the provisions above, a recipient may use your version of this file
 * under the terms of either the MIT license or GPL.
 *
 * This License is also included in this distribution in the file called
 * "MIT-COPYING".
 *
 * EXCEPT AS OTHERWISE STATED IN A NEGOTIATED AGREEMENT: (A) THE SOFTWARE IS
 * PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
 * BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
 * PURPOSE AND NONINFRINGEMENT; AND (B) IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * Low level device initialization
 */
#if defined(CONFIG_MTRR)
#include <asm/mtrr.h>
#endif

#include "plato_drv.h"
#include "plato_aon_regs.h"
#include "plato_top_regs.h"
#include "plato_ddr_ctrl_regs.h"
#include "plato_ddr_publ_regs.h"

#define PLATO_DDR_KINGSTON 1

#define PLATO_HAS_NON_MAPPABLE(dev) (dev->rogue_mem.size < SYS_DEV_MEM_REGION_SIZE)

static int poll_pr(struct device *dev, void *base, u32 reg,
				u32 val, u32 msk, u32 cnt, u32 intrvl)
{
	u32 polnum;

	for (polnum = 0; polnum < cnt; polnum++) {
		if ((plato_read_reg32(base, reg) & msk) == val)
			break;
		plato_sleep_ms(intrvl);
	}
	if (polnum == cnt) {
		dev_info(dev,
			"Poll failed for register: 0x%08X. Expected 0x%08X Received 0x%08X",
			(unsigned int)reg, val,
			plato_read_reg32(base, reg) & msk);
		return -ETIME;
	}

	return 0;
}

#define poll(dev, base, reg, val, msk) poll_pr(dev, base, reg, val, msk, 10, 10)

static int plato_dram_init(struct plato_device *plato,
	void *publ_regs,
	void *ctrl_regs,
	void *aon_regs,
	u32 bldr_data[PLATO_DDR_PUBL_DATX_LANE_COUNT][PLATO_DDR_PUBL_DXBDLR_REGS_PER_LANE],
	u32 reset_flags)
{
	struct device *dev = &plato->pdev->dev;
	/*
	 * Phase 1: Program the DWC_ddr_umctl2 registers
	 */

	/* Single rank only for Kingston DDRs*/
	plato_write_reg32(ctrl_regs, PLATO_DDR_CTRL_MSTR, 0x41040001);

	plato_sleep_us(100);

	/*refresh timings*/
#if defined(PLATO_DDR_KINGSTON)
	plato_write_reg32(ctrl_regs, PLATO_DDR_CTRL_RFSHTMG, 0x0081008B);
#else
	plato_write_reg32(ctrl_regs, PLATO_DDR_CTRL_RFSHTMG, 0x006100BB);
#endif

	/* Train DDR sequence */
	plato_write_reg32(ctrl_regs, PLATO_DDR_CTRL_INIT0, 0x00020100);
	plato_write_reg32(ctrl_regs, PLATO_DDR_CTRL_INIT1, 0x00010000);

#if defined(PLATO_DDR_KINGSTON)
	/* write recovery */
	plato_write_reg32(ctrl_regs, PLATO_DDR_CTRL_INIT3, 0x01700000);
	plato_write_reg32(ctrl_regs, PLATO_DDR_CTRL_INIT4, 0x00280000);
	plato_write_reg32(ctrl_regs, PLATO_DDR_CTRL_INIT5, 0x0012000c);
	plato_write_reg32(ctrl_regs, PLATO_DDR_CTRL_DRAMTMG0, 0x0f0e2112);
	plato_write_reg32(ctrl_regs, PLATO_DDR_CTRL_DRAMTMG1, 0x00040618);
	plato_write_reg32(ctrl_regs, PLATO_DDR_CTRL_DRAMTMG2, 0x0506040B);

	plato_write_reg32(ctrl_regs, PLATO_DDR_CTRL_DRAMTMG3, 0x00002008);

	plato_write_reg32(ctrl_regs, PLATO_DDR_CTRL_DRAMTMG4, 0x06020307);
	plato_write_reg32(ctrl_regs, PLATO_DDR_CTRL_DRAMTMG5, 0x090e0403);

	plato_write_reg32(ctrl_regs, PLATO_DDR_CTRL_DRAMTMG6, 0x0d0e000e);
	plato_write_reg32(ctrl_regs, PLATO_DDR_CTRL_DRAMTMG7, 0x00000c0e);

	plato_write_reg32(ctrl_regs, PLATO_DDR_CTRL_DRAMTMG8, 0x01010a05);

	/*impedance registers */
	plato_write_reg32(ctrl_regs, PLATO_DDR_CTRL_ZQCTL0, 0x30ab002b);
	plato_write_reg32(ctrl_regs, PLATO_DDR_CTRL_ZQCTL1, 0x00000070);
	plato_write_reg32(ctrl_regs, PLATO_DDR_CTRL_ZQCTL2, 0x00000000);

	/*refresh control*/
	plato_write_reg32(ctrl_regs, PLATO_DDR_CTRL_RFSHCTL0, 0x00e01020);
	plato_write_reg32(ctrl_regs, PLATO_DDR_CTRL_RFSHCTL1, 0x0078007e);
	plato_write_reg32(ctrl_regs, PLATO_DDR_CTRL_RFSHCTL2, 0x0057000e);
	plato_write_reg32(ctrl_regs, PLATO_DDR_CTRL_RFSHCTL3, 0x00000000);
	/*DFI Timings*/
	plato_write_reg32(ctrl_regs, PLATO_DDR_CTRL_DFITMG0, 0x02878208);

	plato_write_reg32(ctrl_regs, PLATO_DDR_CTRL_DFITMG1, 0x00020202);

	plato_write_reg32(ctrl_regs, PLATO_DDR_CTRL_DFIUPD0, 0x00400003);
	plato_write_reg32(ctrl_regs, PLATO_DDR_CTRL_DFIUPD1, 0x00f000ff);
	plato_write_reg32(ctrl_regs, PLATO_DDR_CTRL_DFIUPD2, 0x80100010);
	plato_write_reg32(ctrl_regs, PLATO_DDR_CTRL_DFIUPD3, 0x088105c3);

	plato_write_reg32(ctrl_regs, PLATO_DDR_CTRL_DFIMISC, 0x00000000);
#else
	plato_write_reg32(ctrl_regs, PLATO_DDR_CTRL_INIT3, 0x0d700000);
	plato_write_reg32(ctrl_regs, PLATO_DDR_CTRL_INIT4, 0x00180000);
	plato_write_reg32(ctrl_regs, PLATO_DDR_CTRL_INIT5, 0x00090009);
	plato_write_reg32(ctrl_regs, PLATO_DDR_CTRL_DRAMTMG0, 0x0c101a0e);
	plato_write_reg32(ctrl_regs, PLATO_DDR_CTRL_DRAMTMG1, 0x000a0313);
	plato_write_reg32(ctrl_regs, PLATO_DDR_CTRL_DRAMTMG2, 0x04050509);

	plato_write_reg32(ctrl_regs, PLATO_DDR_CTRL_DRAMTMG3, 0x00002008);

	plato_write_reg32(ctrl_regs, PLATO_DDR_CTRL_DRAMTMG4, 0x06020306);
	plato_write_reg32(ctrl_regs, PLATO_DDR_CTRL_DRAMTMG5, 0x070c0202);

	plato_write_reg32(ctrl_regs, PLATO_DDR_CTRL_DRAMTMG6, 0x0d0e000e);
	plato_write_reg32(ctrl_regs, PLATO_DDR_CTRL_DRAMTMG7, 0x00000c0e);

	plato_write_reg32(ctrl_regs, PLATO_DDR_CTRL_DRAMTMG8, 0x01010a07);
	/*impedance registers */
	plato_write_reg32(ctrl_regs, PLATO_DDR_CTRL_ZQCTL0, 0x10800020);
	plato_write_reg32(ctrl_regs, PLATO_DDR_CTRL_ZQCTL1, 0x00000070);
	plato_write_reg32(ctrl_regs, PLATO_DDR_CTRL_ZQCTL2, 0x00000000);

	/*refresh control*/
	plato_write_reg32(ctrl_regs, PLATO_DDR_CTRL_RFSHCTL0, 0x00e06010);
	plato_write_reg32(ctrl_regs, PLATO_DDR_CTRL_RFSHCTL1, 0x00600031);
	plato_write_reg32(ctrl_regs, PLATO_DDR_CTRL_RFSHCTL2, 0x0004002a);
	plato_write_reg32(ctrl_regs, PLATO_DDR_CTRL_RFSHCTL3, 0x00000000);

	/*DFI Timings*/
	plato_write_reg32(ctrl_regs, PLATO_DDR_CTRL_DFITMG0, 0x02878206);

	plato_write_reg32(ctrl_regs, PLATO_DDR_CTRL_DFITMG1, 0x00020202);

	plato_write_reg32(ctrl_regs, PLATO_DDR_CTRL_DFILPCFG0, 0x07111031);
	plato_write_reg32(ctrl_regs, PLATO_DDR_CTRL_DFILPCFG1, 0x00000050);

	plato_write_reg32(ctrl_regs, PLATO_DDR_CTRL_DFIUPD0, 0x00400003);
	plato_write_reg32(ctrl_regs, PLATO_DDR_CTRL_DFIUPD1, 0x006a006f);
	plato_write_reg32(ctrl_regs, PLATO_DDR_CTRL_DFIUPD2, 0x0d0b02b6);
	plato_write_reg32(ctrl_regs, PLATO_DDR_CTRL_DFIUPD3, 0x00100010);

	plato_write_reg32(ctrl_regs, PLATO_DDR_CTRL_DFIMISC, 0x00000001);
#endif

	/* Single rank only on Kingston */
	plato_write_reg32(ctrl_regs, PLATO_DDR_CTRL_ADDRMAP0, 0x00001F1F);
	plato_write_reg32(ctrl_regs, PLATO_DDR_CTRL_ADDRMAP1, 0x00070707);
	plato_write_reg32(ctrl_regs, PLATO_DDR_CTRL_ADDRMAP2, 0x00000000);
	plato_write_reg32(ctrl_regs, PLATO_DDR_CTRL_ADDRMAP3, 0x0F000000);
	plato_write_reg32(ctrl_regs, PLATO_DDR_CTRL_ADDRMAP4, 0x00000F0F);
	plato_write_reg32(ctrl_regs, PLATO_DDR_CTRL_ADDRMAP5, 0x06060606);
	plato_write_reg32(ctrl_regs, PLATO_DDR_CTRL_ADDRMAP6, 0x06060606);
	plato_write_reg32(ctrl_regs, PLATO_DDR_CTRL_ADDRMAP7, 0x00000F0F);
	plato_write_reg32(ctrl_regs, PLATO_DDR_CTRL_ADDRMAP8, 0x00000707);

#if defined(PLATO_DDR_KINGSTON)
	plato_write_reg32(ctrl_regs, PLATO_DDR_CTRL_ODTCFG, 0x06000604);
	plato_write_reg32(ctrl_regs, PLATO_DDR_CTRL_ODTMAP, 0x99c5b050);
#else
	plato_write_reg32(ctrl_regs, PLATO_DDR_CTRL_ODTCFG, 0x0d0f0740);
	plato_write_reg32(ctrl_regs, PLATO_DDR_CTRL_ODTMAP, 0x99c5b050);
#endif

	plato_write_reg32(ctrl_regs, PLATO_DDR_CTRL_PERFHPR1, 0x9f008f23);
	plato_write_reg32(ctrl_regs, PLATO_DDR_CTRL_SCHED, 0x00003f00);
	plato_write_reg32(ctrl_regs, PLATO_DDR_CTRL_PERFLPR1, 0x18000064);
	plato_write_reg32(ctrl_regs, PLATO_DDR_CTRL_PERFWR1, 0x18000096);

	/* Setup the virtual channels */
	plato_write_reg32(ctrl_regs, 0x00000410, 0x0000000F);
	plato_write_reg32(ctrl_regs, 0x00000414, 0x00000000);
	plato_write_reg32(ctrl_regs, 0x00000418, 0x0000000F);
	plato_write_reg32(ctrl_regs, 0x0000041C, 0x00000001);
	plato_write_reg32(ctrl_regs, 0x00000420, 0x0000000F);
	plato_write_reg32(ctrl_regs, 0x00000424, 0x00000002);
	plato_write_reg32(ctrl_regs, 0x00000428, 0x0000000F);
	plato_write_reg32(ctrl_regs, 0x0000042C, 0x00000003);
	plato_write_reg32(ctrl_regs, 0x00000430, 0x0000000F);
	plato_write_reg32(ctrl_regs, 0x00000434, 0x00000004);
	plato_write_reg32(ctrl_regs, 0x00000438, 0x0000000F);
	plato_write_reg32(ctrl_regs, 0x0000043C, 0x00000005);
	plato_write_reg32(ctrl_regs, 0x00000440, 0x0000000F);
	plato_write_reg32(ctrl_regs, 0x00000444, 0x00000006);
	plato_write_reg32(ctrl_regs, 0x00000448, 0x0000000F);
	plato_write_reg32(ctrl_regs, 0x0000044C, 0x00000007);
	plato_write_reg32(ctrl_regs, 0x00000450, 0x0000000F);
	plato_write_reg32(ctrl_regs, 0x00000454, 0x00000008);
	plato_write_reg32(ctrl_regs, 0x00000458, 0x0000000F);
	plato_write_reg32(ctrl_regs, 0x0000045C, 0x00000009);
	plato_write_reg32(ctrl_regs, 0x00000460, 0x0000000F);
	plato_write_reg32(ctrl_regs, 0x00000464, 0x0000000A);
	plato_write_reg32(ctrl_regs, 0x00000468, 0x0000000F);
	plato_write_reg32(ctrl_regs, 0x0000046C, 0x0000000B);
	plato_write_reg32(ctrl_regs, 0x00000470, 0x0000000F);
	plato_write_reg32(ctrl_regs, 0x00000474, 0x0000000C);
	plato_write_reg32(ctrl_regs, 0x00000478, 0x0000000F);
	plato_write_reg32(ctrl_regs, 0x0000047C, 0x0000000D);
	plato_write_reg32(ctrl_regs, 0x00000480, 0x0000000F);
	plato_write_reg32(ctrl_regs, 0x00000484, 0x0000000E);

	poll(dev, ctrl_regs, 0x484, 0x0000000E, 0x0000000F);

	plato_sleep_us(1000);

	PLATO_CHECKPOINT(plato);

	/*
	 * Phase 2: Deassert soft reset signal core_ddrc_rstn
	 */

	/* Now getting DRAM controller out of reset */
	plato_write_reg32(aon_regs, PLATO_AON_CR_RESET_CTRL,
			reset_flags |
			plato_read_reg32(aon_regs, PLATO_AON_CR_RESET_CTRL));

	plato_sleep_us(1000);

	/* ECC disable */
	plato_write_reg32(ctrl_regs, PLATO_DDR_CTRL_DBG1, 0x00000000);
	/* power related */
	plato_write_reg32(ctrl_regs, PLATO_DDR_CTRL_PWRCTL, 0x00000000);
	/* Enabling AXI input port (Port control) */
	plato_write_reg32(ctrl_regs, PLATO_DDR_CTRL_PCTRL, 0x00000001);

	PLATO_CHECKPOINT(plato);

	/*
	 * Phase 7: Set DFIMISC.dfi_init_complete_en to 1
	 */

	/*
	 * Phase 3: Start PHY initialization by accessing relevant PUB registers
	 */
	plato_write_reg32(publ_regs, PLATO_DDR_PUBL_DCR_OFFSET, 0x0000040B);

#if defined(PLATO_DDR_KINGSTON)
	/* IF DDR_RISE_RISE */
	plato_write_reg32(publ_regs, PLATO_DDR_PUBL_DSGCR_OFFSET, 0x0064641F);

	/* MR Registers */
	plato_write_reg32(publ_regs, PLATO_DDR_PUBL_MR0_OFFSET, 0x170);
	plato_write_reg32(publ_regs, PLATO_DDR_PUBL_MR1_OFFSET, 0x00000400);
	plato_write_reg32(publ_regs, PLATO_DDR_PUBL_MR2_OFFSET, 0x00000228);
	plato_write_reg32(publ_regs, PLATO_DDR_PUBL_MR3_OFFSET, 0x00000000);

	plato_write_reg32(publ_regs, PLATO_DDR_PUBL_DTPR0_OFFSET, 0x06220308);
	plato_write_reg32(publ_regs, PLATO_DDR_PUBL_DTPR1_OFFSET, 0x281b1004);
	plato_write_reg32(publ_regs, PLATO_DDR_PUBL_DTPR2_OFFSET, 0x00060120);
	plato_write_reg32(publ_regs, PLATO_DDR_PUBL_DTPR3_OFFSET, 0x02000101);
	plato_write_reg32(publ_regs, PLATO_DDR_PUBL_DTPR4_OFFSET, 0x01200807);
	plato_write_reg32(publ_regs, PLATO_DDR_PUBL_DTPR5_OFFSET, 0x00300c08);

	plato_write_reg32(publ_regs, PLATO_DDR_PUBL_PGCR1_OFFSET, 0x020046A0);
	plato_write_reg32(publ_regs, PLATO_DDR_PUBL_PGCR2_OFFSET, 0x00F09088);
#else
	plato_write_reg32(publ_regs, PLATO_DDR_PUBL_PTR0_OFFSET, 0x10000010);
	plato_write_reg32(publ_regs, PLATO_DDR_PUBL_PTR1_OFFSET, 0x271012c0);

	/* IF DDR_RISE_RISE */
	plato_write_reg32(publ_regs, PLATO_DDR_PUBL_DSGCR_OFFSET, 0x0064641F);

	plato_write_reg32(publ_regs, PLATO_DDR_PUBL_MR0_OFFSET, 0xd70);
	plato_write_reg32(publ_regs, PLATO_DDR_PUBL_MR1_OFFSET, 0x00000000);
	plato_write_reg32(publ_regs, PLATO_DDR_PUBL_MR2_OFFSET, 0x00000018);
	plato_write_reg32(publ_regs, PLATO_DDR_PUBL_MR3_OFFSET, 0x00000000);

	plato_write_reg32(publ_regs, PLATO_DDR_PUBL_DTPR0_OFFSET, 0x061c0B06);
	plato_write_reg32(publ_regs, PLATO_DDR_PUBL_DTPR1_OFFSET, 0x28200400);
	plato_write_reg32(publ_regs, PLATO_DDR_PUBL_DTPR2_OFFSET, 0x00040005);
	plato_write_reg32(publ_regs, PLATO_DDR_PUBL_DTPR3_OFFSET, 0x02000101);
	plato_write_reg32(publ_regs, PLATO_DDR_PUBL_DTPR4_OFFSET, 0x01180805);
	plato_write_reg32(publ_regs, PLATO_DDR_PUBL_DTPR5_OFFSET, 0x00250B06);
	/* IF TRAIN_DDR */
	plato_write_reg32(publ_regs, PLATO_DDR_PUBL_PGCR2_OFFSET, 0x00F09088);
#endif

	/* DISABLE VT COMPENSATION */
	plato_write_reg32(publ_regs, PLATO_DDR_PUBL_DXCCR_OFFSET, 0x20C01884);

	/* VREF CHANGE */
#if 0
	plato_write_reg32(publ_regs, 0x0710, 0x0E00083C);
	plato_write_reg32(publ_regs, 0x0810, 0x0E00083C);
	plato_write_reg32(publ_regs, 0x0910, 0x0E00083C);
	plato_write_reg32(publ_regs, 0x0A10, 0x0E00083C);
	plato_write_reg32(publ_regs, 0x0B10, 0x0E00083C);
	plato_write_reg32(publ_regs, 0x0C10, 0x0E00083C);
	plato_write_reg32(publ_regs, 0x0D10, 0x0E00083C);
	plato_write_reg32(publ_regs, 0x0E10, 0x0E00083C);
#endif

	PLATO_CHECKPOINT(plato);

	/*
	 * Phase 4: Trigger PHY initialization: Impedance, PLL, and DDL; assert PHY reset
	 */
	plato_write_reg32(publ_regs, PLATO_DDR_PUBL_PIR_OFFSET, 0x00000073);

	/*
	 * Phase 5: Monitor PHY initialization status by polling the PUB register PGSR0
	 * (not done on emu)
	 */
	poll(dev, publ_regs, PLATO_DDR_PUBL_PGSR0_OFFSET, 0xF, 0xF);

	plato_write_reg32(ctrl_regs, PLATO_DDR_CTRL_SWCTL, 0x00000000);
	plato_write_reg32(ctrl_regs, PLATO_DDR_CTRL_DFIMISC, 0x00000001);
	plato_write_reg32(ctrl_regs, PLATO_DDR_CTRL_SWCTL, 0x00000001);

	PLATO_CHECKPOINT(plato);

	/*
	 * Phase 6: Indicate to the PUB that the controller will perform SDRAM
	 * initialization by setting PIR.INIT and PIR.CTLDINIT, and
	 * poll PGSR0.IDONE
	 */
	plato_write_reg32(publ_regs, PLATO_DDR_PUBL_PIR_OFFSET, 0x00040001);
	poll(dev, publ_regs, PLATO_DDR_PUBL_PGSR0_OFFSET, 0x11, 0x11);

	/*
	 * Phase 8: Wait for DWC_ddr_umctl2 to move to "normal" operating
	 * mode by monitoring STAT.operating_mode signal
	 */
	poll_pr(dev, ctrl_regs, PLATO_DDR_CTRL_STAT, 0x01, 0x01, 10, 100);

	plato_sleep_us(100);

	/* IF TRAIN_DDR */
	plato_write_reg32(publ_regs, PLATO_DDR_PUBL_DTCR0_OFFSET, 0x8000B1C7);
	/*single rank only for Kingston */
	plato_write_reg32(publ_regs, PLATO_DDR_PUBL_DTCR1_OFFSET, 0x00010237);

#if defined(PLATO_DDR_BDLR_TRAINING)
	/* Bit delay line register training */
	{
		u8 lane = 0;
		u8 reg = 0;

		for (lane = 0; lane < PLATO_DDR_PUBL_DATX_LANE_COUNT; lane++) {
			for (reg = 0; reg < PLATO_DDR_PUBL_DXBDLR_REGS_PER_LANE; reg++) {
				plato_write_reg32(publ_regs,
					PLATO_DDR_PUBL_DXnBDLR_OFFSET(lane, reg),
					bldr_data[lane][reg]);
			}
		}
	}

	/* poll on general status register 2 for each lane */
	{
		u8 lane;

		for (lane = 0; lane < PLATO_DDR_PUBL_DATX_LANE_COUNT; lane++)
			poll(dev, publ_regs,
					PLATO_DDR_PUBL_DXnGSR_OFFSET(lane, 2),
					0, 0x001FFFFF);
	}
#endif

	plato_write_reg32(publ_regs, PLATO_DDR_PUBL_PIR_OFFSET, 0x0000ff72);
	plato_write_reg32(publ_regs, PLATO_DDR_PUBL_PIR_OFFSET, 0x0000ff73);
	poll(dev, publ_regs, PLATO_DDR_PUBL_PGSR0_OFFSET, 0x80000fff, 0xfff80fff);
	poll(dev, ctrl_regs, PLATO_DDR_CTRL_STAT, 0x1, 0x1);

	/* Setting the Anti Glitch OFF (?), Disabling On Die pullup/pulldowns */
#if defined(PLATO_DDR_KINGSTON)
	plato_write_reg32(publ_regs, PLATO_DDR_PUBL_DXCCR_OFFSET, 0x02401884);
#else
	plato_write_reg32(publ_regs, PLATO_DDR_PUBL_DXCCR_OFFSET, 0x02400004);
#endif

	if (plato_read_reg32(publ_regs, PLATO_DDR_PUBL_PGSR0_OFFSET) != 0x80000fff) {
		dev_err(dev, "-%s: DDR Training failed", __func__);
		return PLATO_INIT_FAILURE;
	}

	plato_sleep_us(100);

#if defined(PLATO_DRM_DEBUG)
	{
		u8 lane = 0;
		u8 reg = 0;

		for (lane = 0; lane < PLATO_DDR_PUBL_DATX_LANE_COUNT; lane++) {
			for (reg = 0; reg < PLATO_DDR_PUBL_DXGSR_REGS_PER_LANE; reg++) {
				plato_dev_info(dev, "DX%dGSR%d: 0x%08x", lane, reg,
					plato_read_reg32(publ_regs,
						PLATO_DDR_PUBL_DXnGSR_OFFSET(lane, reg)));
			}
		}
	}
#endif

	return PLATO_INIT_SUCCESS;
}

#define PLATO_PDP_REGISTER_CORE_ID_OFFSET (0x04e0)
#define VALID_PDP_CORE_ID (0x7010003)

/*
 * Helpers for getting integer and fractional values which are needed when
 * programming PLATO_AON_CR_{GPU|DDR}_PLL_CTRL_{0|1} registers for DDR/GPU PLLs.
 *
 * PLL_CLOCK_SPEED = (PLL_REF_CLOCK_SPEED * REFDIV) *
 *                   (PLL_INT + (PLL_FRAC / 2^24)) / POSTDIV1 / POSTDIV2
 *
 * NOTE: It's assumed that REFDIV, POSTDIV1 and POSTDIV2 are '1' in all cases.
 */
static u32 get_plato_pll_int(u32 pll_clock)
{
	return pll_clock / PLATO_PLL_REF_CLOCK_SPEED;
}

static u32 get_plato_pll_frac(u32 pll_clock)
{
	/* Shift to get 24 bits for fractional part after div */
	u64 shift = (u64)pll_clock << 24;

	/* Div and return only fractional part of the result */
	return (shift / PLATO_PLL_REF_CLOCK_SPEED) & ((1 << 24) - 1);
}

/*
 * Helper for getting value of integer divider for GPU clock.
 */
static u32 get_plato_gpuv_div0(u32 pll_clock, u32 core_clock)
{
	u32 div, ret;

	div = pll_clock / core_clock;

	/* Bias the result by (-1) with saturation, then clip it */
	ret = (div - (div > 0)) &
		(PLATO_CR_GPUV_DIV_0_MASK >> PLATO_CR_GPUV_DIV_0_SHIFT);

	/* Check for lost result after clipping, saturate if so */
	return (div > 1) && (ret != (div - (div > 0))) ?
		(PLATO_CR_GPUV_DIV_0_MASK >> PLATO_CR_GPUV_DIV_0_SHIFT) : ret;
}

/*
 * Helpers for getting values of integer dividers for PDP clock.
 *
 * NOTE: Use only if PLL clock speed > ~350 MHz.
 */
static u32 get_plato_pdpv0_div0(u32 pll_clock)
{
	u32 div, ret;

	div = pll_clock / PLATO_MIN_PDP_CLOCK_SPEED;

	/* Bias the result by (-1) with saturation, then clip it */
	ret = (div - (div > 0)) &
		(PLATO_CR_PDPV0_DIV_0_MASK >> PLATO_CR_PDPV0_DIV_0_SHIFT);

	/* Check for lost result after clipping, saturate if so */
	return (div > 1) && (ret != (div - (div > 0))) ?
		(PLATO_CR_PDPV0_DIV_0_MASK >> PLATO_CR_PDPV0_DIV_0_SHIFT) : ret;
}

static u32 get_plato_pdpv1_div0(u32 pll_clock)
{
	u32 div, ret;

	div = (pll_clock / (get_plato_pdpv0_div0(pll_clock) + 1)) /
		PLATO_MIN_PDP_CLOCK_SPEED;

	/* Bias the result by (-1) with saturation, then clip it */
	ret = (div - (div > 0)) &
		(PLATO_CR_PDPV1_DIV_0_MASK >> PLATO_CR_PDPV1_DIV_0_SHIFT);

	/* Check for lost result after clipping, saturate if so */
	return (div > 1) && (ret != (div - (div > 0))) ?
		(PLATO_CR_PDPV1_DIV_0_MASK >> PLATO_CR_PDPV1_DIV_0_SHIFT) : ret;
}

#if defined(ENABLE_PLATO_HDMI)
/*
 * Helpers for getting values of integer dividers for HDMICEC clocks.
 *
 * NOTE: They strive to get clock speed of HDMI-SFR as close to
 *       27 MHz as possible.
 */
static u32 get_plato_hdmicecv0_div0(u32 pll_clock)
{
	u32 hdmicecv0_div0, hdmicecv0_div0_limit;
	u32 hdmicecv1_div0, hdmicecv1_div0_limit;
	u32 hdmisfr_clock_speed;

	hdmicecv0_div0_limit = PLATO_CR_HDMICECV0_DIV_0_MASK >>
	    PLATO_CR_HDMICECV0_DIV_0_SHIFT;
	hdmicecv1_div0_limit = PLATO_CR_HDMICECV1_DIV_0_MASK >>
	    PLATO_CR_HDMICECV1_DIV_0_SHIFT;

	hdmicecv0_div0 = 0;
	while (hdmicecv0_div0 < hdmicecv0_div0_limit) {
		hdmicecv1_div0 = 0;
		while (hdmicecv1_div0 < hdmicecv1_div0_limit) {
			hdmisfr_clock_speed = pll_clock /
				(hdmicecv0_div0 + 1) / (hdmicecv1_div0 + 1);

			if (hdmisfr_clock_speed <= PLATO_TARGET_HDMI_SFR_CLOCK_SPEED) {
				/* Done, value of the divider found */
				return hdmicecv0_div0;
			}

			hdmicecv1_div0++;
		}

		hdmicecv0_div0++;
	}

	// Here the function returns the highest possible value of the divider
	return hdmicecv0_div0;
}

static u32 get_plato_hdmicecv1_div0(u32 pll_clock)
{
	u64 div, ret;

	/* Calculate the divider using 32.32 fixed point math */
	div = (u64)pll_clock << 32;
	div /= get_plato_hdmicecv0_div0(pll_clock) + 1;
	div /= PLATO_TARGET_HDMI_SFR_CLOCK_SPEED;

	/* Round up if the fractional part is present */
	div = (div >> 32) + ((div & 0xFFFFFFFF) > 0);

	/* Bias the result by (-1) with saturation, then clip it */
	ret = (div - (div > 0)) &
		(PLATO_CR_HDMICECV1_DIV_0_MASK >>
		 PLATO_CR_HDMICECV1_DIV_0_SHIFT);

	/* Check for lost result after clipping, saturate if so */
	return (div > 1) && (ret != (div - (div > 0))) ?
		(PLATO_CR_HDMICECV1_DIV_0_MASK >>
		 PLATO_CR_HDMICECV1_DIV_0_SHIFT) : ret;
}

static u32 get_plato_hdmicecv2_div0(u32 pll_clock)
{
	u64 div, ret;

	/* Calculate the divider using 32.32 fixed point math */
	div = (u64)pll_clock << 32;
	div /= get_plato_hdmicecv0_div0(pll_clock) + 1;
	div /= get_plato_hdmicecv1_div0(pll_clock) + 1;
	div /= PLATO_TARGET_HDMI_CEC_CLOCK_SPEED;

	/* Round up if the fractional part is present */
	div = (div >> 32) + ((div & 0xFFFFFFFF) > 0);

	/* Bias the result by (-1) with saturation, then clip it */
	ret = (div - (div > 0)) &
		(PLATO_CR_HDMICECV2_DIV_0_MASK >>
		 PLATO_CR_HDMICECV2_DIV_0_SHIFT);

	/* Check for lost result after clipping, saturate if so */
	return (div > 1) && (ret != (div - (div > 0))) ?
		(PLATO_CR_HDMICECV2_DIV_0_MASK >>
		 PLATO_CR_HDMICECV2_DIV_0_SHIFT) : ret;
}
#endif

#if defined(PLATO_DUAL_CHANNEL_DDR)

static int plato_dual_channel_init(struct plato_device *plato)
{
	struct device *dev = &plato->pdev->dev;
	int err = 0;
	void *dbg_perip_regs	 = plato->sys_io.registers + SYS_PLATO_REG_PERIP_OFFSET;
	void *ddra_ctrl_regs	 = plato->sys_io.registers + SYS_PLATO_REG_DDR_A_CTRL_OFFSET;
	void *ddra_publ_regs	 = plato->sys_io.registers + SYS_PLATO_REG_DDR_A_PUBL_OFFSET;
	void *ddrb_ctrl_regs	 = plato->sys_io.registers + SYS_PLATO_REG_DDR_B_CTRL_OFFSET;
	void *ddrb_publ_regs	 = plato->sys_io.registers + SYS_PLATO_REG_DDR_B_PUBL_OFFSET;
	void *noc_regs			 = plato->sys_io.registers + SYS_PLATO_REG_NOC_OFFSET;
	void *aon_regs			 = plato->aon_regs.registers;
	u32 bdlr_setup_ddra[PLATO_DDR_PUBL_DATX_LANE_COUNT][PLATO_DDR_PUBL_DXBDLR_REGS_PER_LANE] = {
		 {0x0F0F0F0F, 0x0F0F0F0F, 0x000F0F0F},
		 {0x0F0F0F0F, 0x0F0F0F0F, 0x000F0F0F},
		 {0x14141414, 0x14141414, 0x00141414},
		 {0x0F0F0F0F, 0x0F0F0F0F, 0x000F0F0F},
		 {0x14141414, 0x14141414, 0x00141414},
		 {0x14141414, 0x14141414, 0x00141414},
		 {0x0F0F0F0F, 0x0F0F0F0F, 0x000F0F0F},
		 {0x0F0F0F0F, 0x0F0F0F0F, 0x000F0F0F} };
	u32 bdlr_setup_ddrb[PLATO_DDR_PUBL_DATX_LANE_COUNT][PLATO_DDR_PUBL_DXBDLR_REGS_PER_LANE] = {
		 {0x0F0F0F0F, 0x0F0F0F0F, 0x000F0F0F},
		 {0x14141414, 0x14141414, 0x00141414},
		 {0x14141414, 0x14141414, 0x00141414},
		 {0x0F0F0F0F, 0x0F0F0F0F, 0x000F0F0F},
		 {0x0F0F0F0F, 0x0F0F0F0F, 0x000F0F0F},
		 {0x14141414, 0x14141414, 0x00141414},
		 {0x14141414, 0x14141414, 0x00141414},
		 {0x0F0F0F0F, 0x0F0F0F0F, 0x000F0F0F} };
	u32 mem_clock_speed, mem_pll_clock_speed;
	u32 mem_clock_pll_control0, mem_clock_pll_control1;
	u32 mem_clock_control;
	u32 core_clock_speed, core_pll_clock_speed;
	u32 core_clock_pll_control0, core_clock_pll_control1;
	u32 core_clock_control;
#if defined(ENABLE_PLATO_HDMI)
	u32 hdmicec_clock_control;
#endif
	u32 pdp_clock_control, hdmi_clock_control;

	/* Plato Soft reset */
	plato_write_reg32(aon_regs, PLATO_AON_CR_RESET_CTRL, 0x40);
	plato_sleep_ms(100);

	// Temporary fix for non-32GB PCI BAR Plato boards that seem
	// to not signal getting out reset
	if (!PLATO_HAS_NON_MAPPABLE(plato)) {
		err = poll(dev, aon_regs, PLATO_AON_CR_RESET_CTRL, 0x30, 0xF0);
		if (err) {
			dev_err(dev, "%s: Plato failed to come out of reset!", __func__);
			return PLATO_INIT_FAILURE;
		}
	}

	// On non-32GB PCI BAR Plato boards, bring display subsystem
	// out of reset with PLL bypassed.
	if (PLATO_HAS_NON_MAPPABLE(plato)) {
		plato_write_reg32(aon_regs, PLATO_AON_CR_PLL_BYPASS, 0x0);
		plato_write_reg32(aon_regs, PLATO_AON_CR_RESET_CTRL, PLATO_CR_DISPLAY_RESET_MASK);
		plato_write_reg32(aon_regs, PLATO_AON_CR_PLL_BYPASS, 0x1);
	}

	/* Setting dual memory interleaved mode */
	plato_write_reg32(noc_regs, 0x00000050, 0x01);
	poll_pr(dev, noc_regs, 0x00000058, 0x01, 0x1, 1, 10);

	plato_write_reg32(aon_regs, PLATO_AON_CR_NOC_CLK_CTRL, 0x1);
	poll(dev, aon_regs, PLATO_AON_CR_NOC_CLK_CTRL, 0x1, 0x1);

	/* Setup DDR PLL's */
	mem_clock_speed = plato_mem_clock_speed(dev);
	mem_pll_clock_speed = mem_clock_speed;

	mem_clock_pll_control0 =
		(get_plato_pll_int(mem_pll_clock_speed) <<
		 PLATO_CR_DDR_PLL_FBDIV_SHIFT);
	mem_clock_pll_control0 &= PLATO_CR_DDR_PLL_FBDIV_MASK;
	mem_clock_pll_control0 |= (1 << PLATO_CR_DDR_PLL_REFDIV_SHIFT);
	mem_clock_pll_control0 |= (1 << PLATO_CR_DDR_PLL_POSTDIV1_SHIFT);
	mem_clock_pll_control0 |= (1 << PLATO_CR_DDR_PLL_POSTDIV2_SHIFT);

	mem_clock_pll_control1 =
		(get_plato_pll_frac(mem_pll_clock_speed) <<
		 PLATO_CR_DDR_PLL_FRAC_SHIFT);
	mem_clock_pll_control1 &= PLATO_CR_DDR_PLL_FRAC_MASK;

	plato_write_reg32(aon_regs, PLATO_AON_CR_DDR_PLL_CTRL_0,
			  mem_clock_pll_control0);
	poll(dev, aon_regs, PLATO_AON_CR_DDR_PLL_CTRL_0,
	     mem_clock_pll_control0, mem_clock_pll_control0);

	plato_write_reg32(aon_regs, PLATO_AON_CR_DDR_PLL_CTRL_1,
			  mem_clock_pll_control1);
	poll(dev, aon_regs, PLATO_AON_CR_DDR_PLL_CTRL_1,
	     mem_clock_pll_control1, mem_clock_pll_control1);

	dev_info(dev,
		 "%s: DDR clock: %u", __func__, mem_clock_speed);

	/* Setup GPU PLL's */
	core_clock_speed = plato_core_clock_speed(dev);
	core_pll_clock_speed = plato_pll_clock_speed(dev,
						     core_clock_speed);

	core_clock_pll_control0 =
		(get_plato_pll_int(core_pll_clock_speed) <<
		 PLATO_CR_GPU_PLL_FBDIV_SHIFT);
	core_clock_pll_control0 &= PLATO_CR_GPU_PLL_FBDIV_MASK;
	core_clock_pll_control0 |= (1 << PLATO_CR_GPU_PLL_REFDIV_SHIFT);
	core_clock_pll_control0 |= (1 << PLATO_CR_GPU_PLL_POSTDIV1_SHIFT);
	core_clock_pll_control0 |= (1 << PLATO_CR_GPU_PLL_POSTDIV2_SHIFT);

	core_clock_pll_control1 =
		(get_plato_pll_frac(core_pll_clock_speed) <<
		 PLATO_CR_GPU_PLL_FRAC_SHIFT);
	core_clock_pll_control1 &= PLATO_CR_GPU_PLL_FRAC_MASK;

	plato_write_reg32(aon_regs, PLATO_AON_CR_GPU_PLL_CTRL_0,
			  core_clock_pll_control0);
	poll(dev, aon_regs, PLATO_AON_CR_GPU_PLL_CTRL_0,
	     core_clock_pll_control0, core_clock_pll_control0);

	plato_write_reg32(aon_regs, PLATO_AON_CR_GPU_PLL_CTRL_1,
			  core_clock_pll_control1);
	poll(dev, aon_regs, PLATO_AON_CR_GPU_PLL_CTRL_1,
	     core_clock_pll_control1, core_clock_pll_control1);

	dev_info(dev,
		 "%s: GPU clock: %u", __func__, core_clock_speed);

#if defined(ENABLE_PLATO_HDMI)
	/* Setup HDMI CEC clock outputs */
	hdmicec_clock_control = 0;
	hdmicec_clock_control |=
		(get_plato_hdmicecv0_div0(core_pll_clock_speed) <<
		 PLATO_CR_HDMICECV0_DIV_0_SHIFT);
	hdmicec_clock_control |=
		(get_plato_hdmicecv1_div0(core_pll_clock_speed) <<
		 PLATO_CR_HDMICECV1_DIV_0_SHIFT);
	hdmicec_clock_control |=
		(get_plato_hdmicecv2_div0(core_pll_clock_speed) <<
		 PLATO_CR_HDMICECV2_DIV_0_SHIFT);

	plato_write_reg32(dbg_perip_regs, PLATO_TOP_CR_HDMI_CEC_CLK_CTRL,
			  hdmicec_clock_control);
	poll(dev, dbg_perip_regs, PLATO_TOP_CR_HDMI_CEC_CLK_CTRL,
	     hdmicec_clock_control, hdmicec_clock_control);

	plato_write_reg32(dbg_perip_regs, PLATO_TOP_CR_I2C_CLK_CTRL, 0x1);
	poll(dev, dbg_perip_regs, PLATO_TOP_CR_I2C_CLK_CTRL, 0x1, 0x1);
#endif

	PLATO_CHECKPOINT(plato);

	/* Waiting for DDR and GPU PLL's to lock */
	poll_pr(dev, aon_regs, PLATO_AON_CR_PLL_STATUS, 0x3, 0x3, -1, 10);

	plato_write_reg32(aon_regs, PLATO_AON_CR_PLL_BYPASS, 0x01);
	poll(dev, aon_regs, PLATO_AON_CR_PLL_BYPASS, 0x01, 0x01);
	/* PLL Lock is done */

	plato_sleep_us(1000);

	/* Enabling gated clock output for DDR A/B */
	mem_clock_control = (1 << PLATO_CR_DDRAG_GATE_EN_SHIFT) |
			    (1 << PLATO_CR_DDRBG_GATE_EN_SHIFT);

	plato_write_reg32(dbg_perip_regs,
			  PLATO_TOP_CR_DDR_CLK_CTRL, mem_clock_control);
	poll(dev, dbg_perip_regs,
	     PLATO_TOP_CR_DDR_CLK_CTRL, mem_clock_control, mem_clock_control);

	/* Enabling gated clock output for GPU and dividing the clock */
	core_clock_control = (1 << PLATO_CR_GPUG_GATE_EN_SHIFT);
	core_clock_control |=
		(get_plato_gpuv_div0(core_pll_clock_speed, core_clock_speed)
		 << PLATO_CR_GPUV_DIV_0_SHIFT);

	plato_write_reg32(dbg_perip_regs,
			  PLATO_TOP_CR_GPU_CLK_CTRL, core_clock_control);
	poll(dev, dbg_perip_regs,
	     PLATO_TOP_CR_GPU_CLK_CTRL, core_clock_control, core_clock_control);

	PLATO_CHECKPOINT(plato);

	plato_sleep_us(100);

	/* Enabling PDP gated clock output >= 165 MHz for <= 1080p */
	pdp_clock_control = (1 << PLATO_CR_PDPG_GATE_EN_SHIFT);
	pdp_clock_control |=
		(get_plato_pdpv0_div0(core_pll_clock_speed)
		 << PLATO_CR_PDPV0_DIV_0_SHIFT);
	pdp_clock_control |=
		(get_plato_pdpv1_div0(core_pll_clock_speed)
		 << PLATO_CR_PDPV1_DIV_0_SHIFT);

	plato_write_reg32(dbg_perip_regs, PLATO_TOP_CR_PDP_CLK_CTRL,
			  pdp_clock_control);
	poll(dev, dbg_perip_regs, PLATO_TOP_CR_PDP_CLK_CTRL,
	     pdp_clock_control, pdp_clock_control);

	plato_sleep_us(100);

	/*
	 * Enabling HDMI gated clock output,
	 * PDP needs HDMI clocks on for framegrabber.
	 *
	 * NOTE: The dividers will be reconfigured in video.c,
	 *       for now they are set to their highest values.
	 */
	hdmi_clock_control = (1 << PLATO_CR_HDMIG_GATE_EN_SHIFT);
	hdmi_clock_control |= (PLATO_CR_HDMIV0_DIV_0_MASK);
	hdmi_clock_control |= (PLATO_CR_HDMIV1_DIV_0_MASK);
	plato_write_reg32(dbg_perip_regs, PLATO_TOP_CR_HDMI_CLK_CTRL,
			  hdmi_clock_control);
	poll(dev, dbg_perip_regs, PLATO_TOP_CR_HDMI_CLK_CTRL,
	     hdmi_clock_control, hdmi_clock_control);

	plato_sleep_us(100);

	plato_dev_info(dev, "%s: Enabled PDP and HDMI clocks", __func__);
	PLATO_CHECKPOINT(plato);

	/* Now putting DRAM controller out of reset */
	plato_write_reg32(aon_regs, PLATO_AON_CR_RESET_CTRL,
					PLATO_CR_DDR_A_DATA_RESET_N_MASK |
					PLATO_CR_DDR_A_CTRL_RESET_N_MASK |
					PLATO_CR_DDR_B_DATA_RESET_N_MASK |
					PLATO_CR_DDR_B_CTRL_RESET_N_MASK);
	plato_sleep_us(100);

	/* Now putting DRAM controller into reset */
	plato_write_reg32(aon_regs, PLATO_AON_CR_RESET_CTRL,
					PLATO_CR_DDR_A_CTRL_RESET_N_MASK |
					PLATO_CR_DDR_B_CTRL_RESET_N_MASK);

	/* Always configure DDR A */
	err = plato_dram_init(plato, ddra_publ_regs, ddra_ctrl_regs, aon_regs,
					bdlr_setup_ddra,
					PLATO_CR_DDR_A_DATA_RESET_N_MASK |
					PLATO_CR_DDR_A_CTRL_RESET_N_MASK);
	if (err != 0) {
		dev_err(dev, "DDR Bank A setup failed. Init cannot proceed.");
		return err;
	}

	plato_dev_info(dev, "%s: Finished DDR A Setup", __func__);

	/* Configure DDR B */
	err = plato_dram_init(plato, ddrb_publ_regs, ddrb_ctrl_regs, aon_regs,
					bdlr_setup_ddrb,
					PLATO_CR_DDR_B_DATA_RESET_N_MASK |
					PLATO_CR_DDR_B_CTRL_RESET_N_MASK);
	if (err != 0) {
		dev_err(dev, "DDR Bank B setup failed. Init cannot proceed.");
		return err;
	}

	plato_dev_info(dev, "%s: Finished DDR B Setup", __func__);

	/* Getting GPU And DDR A/B out of reset */
	plato_write_reg32(aon_regs, PLATO_AON_CR_RESET_CTRL, 0x00000F12);
	err = poll_pr(dev, aon_regs,
				PLATO_AON_CR_RESET_CTRL,
				0x00000F12, 0x00000F12, -1, 100);
	if (err)
		return err;

	/* setting CR_ISO_CTRL:CR_GPU_CLK_E */
	plato_write_reg32(aon_regs, PLATO_AON_CR_ISO_CTRL, 0x000001F);

	return err;

}
#else

static int plato_single_channel_init(struct plato_device *plato)
{
	struct device *dev = &plato->pdev->dev;
	int err = 0;
	void *dbg_perip_regs	 = plato->sys_io.registers + SYS_PLATO_REG_PERIP_OFFSET;
	void *ddra_ctrl_regs	 = plato->sys_io.registers + SYS_PLATO_REG_DDR_A_CTRL_OFFSET;
	void *ddra_publ_regs	 = plato->sys_io.registers + SYS_PLATO_REG_DDR_A_PUBL_OFFSET;
	void *aon_regs			 = plato->aon_regs.registers;
	u32 mem_clock_speed, mem_pll_clock_speed;
	u32 mem_clock_pll_control0, mem_clock_pll_control1;
	u32 mem_clock_control;
	u32 core_clock_speed, core_pll_clock_speed;
	u32 core_clock_pll_control0, core_clock_pll_control1;
	u32 core_clock_control;
#if defined(ENABLE_PLATO_HDMI)
	u32 hdmicec_clock_control;
#endif
	u32 hdmi_clock_control, pdp_clock_control;

	/* Plato Soft reset */
	plato_write_reg32(aon_regs, PLATO_AON_CR_RESET_CTRL, 0x40);
	plato_sleep_ms(1000);
	err = poll(dev, aon_regs, PLATO_AON_CR_RESET_CTRL, 0x030, 0xF0);
	if (err)
		return err;

	PLATO_CHECKPOINT(plato);

	plato_write_reg32(aon_regs, PLATO_AON_CR_NOC_CLK_CTRL, 0x1);
	poll(dev, aon_regs, PLATO_AON_CR_NOC_CLK_CTRL, 0x1, 0x1);

	/* Setup DDR PLL's */
	mem_clock_speed = plato_mem_clock_speed(dev);
	mem_pll_clock_speed = mem_clock_speed;

	mem_clock_pll_control0 =
		(get_plato_pll_int(mem_pll_clock_speed) <<
		 PLATO_CR_DDR_PLL_FBDIV_SHIFT);
	mem_clock_pll_control0 &= PLATO_CR_DDR_PLL_FBDIV_MASK;
	mem_clock_pll_control0 |= (1 << PLATO_CR_DDR_PLL_REFDIV_SHIFT);
	mem_clock_pll_control0 |= (1 << PLATO_CR_DDR_PLL_POSTDIV1_SHIFT);
	mem_clock_pll_control0 |= (1 << PLATO_CR_DDR_PLL_POSTDIV2_SHIFT);

	mem_clock_pll_control1 =
		(get_plato_pll_frac(mem_pll_clock_speed) <<
		 PLATO_CR_DDR_PLL_FRAC_SHIFT);
	mem_clock_pll_control1 &= PLATO_CR_DDR_PLL_FRAC_MASK;

	plato_write_reg32(aon_regs, PLATO_AON_CR_DDR_PLL_CTRL_0,
			  mem_clock_pll_control0);
	poll(dev, aon_regs, PLATO_AON_CR_DDR_PLL_CTRL_0,
	     mem_clock_pll_control0, mem_clock_pll_control0);

	plato_write_reg32(aon_regs, PLATO_AON_CR_DDR_PLL_CTRL_1,
			  mem_clock_pll_control1);
	poll(dev, aon_regs, PLATO_AON_CR_DDR_PLL_CTRL_1,
	     mem_clock_pll_control1, mem_clock_pll_control1);

	dev_info(dev,
		 "%s: DDR clock: %u", __func__, mem_clock_speed);

	/* Setup GPU PLL's */
	core_clock_speed = plato_core_clock_speed(dev);
	core_pll_clock_speed = plato_pll_clock_speed(dev,
						core_clock_speed);

	core_clock_pll_control0 =
		(get_plato_pll_int(core_pll_clock_speed) <<
		 PLATO_CR_GPU_PLL_FBDIV_SHIFT);
	core_clock_pll_control0 &= PLATO_CR_GPU_PLL_FBDIV_MASK;
	core_clock_pll_control0 |= (1 << PLATO_CR_GPU_PLL_REFDIV_SHIFT);
	core_clock_pll_control0 |= (1 << PLATO_CR_GPU_PLL_POSTDIV1_SHIFT);
	core_clock_pll_control0 |= (1 << PLATO_CR_GPU_PLL_POSTDIV2_SHIFT);

	core_clock_pll_control1 =
		(get_plato_pll_frac(core_pll_clock_speed) <<
		 PLATO_CR_GPU_PLL_FRAC_SHIFT);
	core_clock_pll_control1 &= PLATO_CR_GPU_PLL_FRAC_MASK;

	plato_write_reg32(aon_regs, PLATO_AON_CR_GPU_PLL_CTRL_0,
			  core_clock_pll_control0);
	poll(dev, aon_regs, PLATO_AON_CR_GPU_PLL_CTRL_0,
	     core_clock_pll_control0, core_clock_pll_control0);

	plato_write_reg32(aon_regs, PLATO_AON_CR_GPU_PLL_CTRL_1,
			  core_clock_pll_control1);
	poll(dev, aon_regs, PLATO_AON_CR_GPU_PLL_CTRL_1,
	     core_clock_pll_control1, core_clock_pll_control1);

	dev_info(dev,
		 "%s: GPU clock: %u", __func__, core_clock_speed);

#if defined(ENABLE_PLATO_HDMI)
	/* Setup HDMI CEC clock outputs */
	hdmicec_clock_control = 0;
	hdmicec_clock_control |=
		(get_plato_hdmicecv0_div0(core_pll_clock_speed) <<
		 PLATO_CR_HDMICECV0_DIV_0_SHIFT);
	hdmicec_clock_control |=
		(get_plato_hdmicecv1_div0(core_pll_clock_speed) <<
		 PLATO_CR_HDMICECV1_DIV_0_SHIFT);
	hdmicec_clock_control |=
		(get_plato_hdmicecv2_div0(core_pll_clock_speed) <<
		 PLATO_CR_HDMICECV2_DIV_0_SHIFT);

	plato_write_reg32(dbg_perip_regs, PLATO_TOP_CR_HDMI_CEC_CLK_CTRL,
			  hdmicec_clock_control);
	poll(dev, dbg_perip_regs, PLATO_TOP_CR_HDMI_CEC_CLK_CTRL,
	     hdmicec_clock_control, hdmicec_clock_control);

	plato_write_reg32(dbg_perip_regs, PLATO_TOP_CR_I2C_CLK_CTRL, 0x1);
	poll(dev, dbg_perip_regs, PLATO_TOP_CR_I2C_CLK_CTRL, 0x1, 0x1);
#endif

	PLATO_CHECKPOINT(plato);

	/* Waiting for DDR and GPU PLL's to lock */
	poll_pr(dev, aon_regs, PLATO_AON_CR_PLL_STATUS, 0x3, 0x3, -1, 10);

	plato_write_reg32(aon_regs, PLATO_AON_CR_PLL_BYPASS, 0x01);
	poll(dev, aon_regs, PLATO_AON_CR_PLL_BYPASS, 0x01, 0x01);
	/* PLL Lock is done */

	plato_sleep_us(100);

	/* Enabling gated clock output for DDR A/B */
	mem_clock_control = (1 << PLATO_CR_DDRAG_GATE_EN_SHIFT) |
			    (1 << PLATO_CR_DDRBG_GATE_EN_SHIFT);

	plato_write_reg32(dbg_perip_regs,
			  PLATO_TOP_CR_DDR_CLK_CTRL, mem_clock_control);
	poll(dev, dbg_perip_regs,
	     PLATO_TOP_CR_DDR_CLK_CTRL, mem_clock_control, mem_clock_control);

	/* Enabling gated clock output for GPU and dividing the clock */
	core_clock_control = (1 << PLATO_CR_GPUG_GATE_EN_SHIFT);
	core_clock_control |=
		(get_plato_gpuv_div0(core_pll_clock_speed, core_clock_speed)
		 << PLATO_CR_GPUV_DIV_0_SHIFT);

	plato_write_reg32(dbg_perip_regs,
			  PLATO_TOP_CR_GPU_CLK_CTRL, core_clock_control);
	poll(dev, dbg_perip_regs,
	     PLATO_TOP_CR_GPU_CLK_CTRL, core_clock_control, core_clock_control);

	PLATO_CHECKPOINT(plato);

	plato_sleep_us(1000);

	/* Enabling PDP gated clock output >= 165 MHz for <= 1080p */
	pdp_clock_control = (1 << PLATO_CR_PDPG_GATE_EN_SHIFT);
	pdp_clock_control |=
		(get_plato_pdpv0_div0(core_pll_clock_speed)
		 << PLATO_CR_PDPV0_DIV_0_SHIFT);
	pdp_clock_control |=
		(get_plato_pdpv1_div0(core_pll_clock_speed)
		 << PLATO_CR_PDPV1_DIV_0_SHIFT);

	plato_write_reg32(dbg_perip_regs, PLATO_TOP_CR_PDP_CLK_CTRL,
			  pdp_clock_control);
	poll(dev, dbg_perip_regs, PLATO_TOP_CR_PDP_CLK_CTRL,
	     pdp_clock_control, pdp_clock_control);

	plato_sleep_us(100);

	/*
	 * Enabling HDMI gated clock output. PDP needs HDMI clocks on for framegrabber.
	 *
	 * NOTE: The dividers will be reconfigured in video.c,
	 *       for now they are set to their highest values.
	 */
	hdmi_clock_control = (1 << PLATO_CR_HDMIG_GATE_EN_SHIFT);
	hdmi_clock_control |= (PLATO_CR_HDMIV0_DIV_0_MASK);
	hdmi_clock_control |= (PLATO_CR_HDMIV1_DIV_0_MASK);
	plato_write_reg32(dbg_perip_regs, PLATO_TOP_CR_HDMI_CLK_CTRL,
			  hdmi_clock_control);
	poll(dev, dbg_perip_regs, PLATO_TOP_CR_HDMI_CLK_CTRL,
	     hdmi_clock_control, hdmi_clock_control);

	plato_sleep_us(100);

	plato_dev_info(dev, "%s: Enabled PDP and HDMI clocks", __func__);

	PLATO_CHECKPOINT(plato);

	/* Now putting DRAM A controller out of reset */
	plato_write_reg32(aon_regs, PLATO_AON_CR_RESET_CTRL, 0x0300);
	plato_sleep_us(100);

	/* Now putting DRAM A controller into reset */
	plato_write_reg32(aon_regs, PLATO_AON_CR_RESET_CTRL, 0x0200);

	/* Configure DRAM A control and publ regs */
	err = plato_dram_init(ddra_publ_regs, ddra_ctrl_regs, aon_regs,
		PLATO_CR_DDR_A_DATA_RESET_N_MASK | PLATO_CR_DDR_A_CTRL_RESET_N_MASK);
	if (err != 0) {
		dev_err(dev, "DDR Bank setup failed. Init cannot proceed.");
		return err;
	}
	plato_dev_info(dev, "- %s: Finished DDR A Setup", __func__);

	/* Getting GPU and DDR A out of reset */
	plato_write_reg32(aon_regs, PLATO_AON_CR_RESET_CTRL, 0x00000312);
	err = poll_pr(dev, aon_regs,
				PLATO_AON_CR_RESET_CTRL,
				0x00000312, 0x00000312, -1, 100);
	if (err)
		return err;

	/* setting CR_ISO_CTRL:CR_GPU_CLK_E */
	plato_write_reg32(aon_regs, PLATO_AON_CR_ISO_CTRL, 0x000001F);

	return err;
}

#endif /* PLATO_DUAL_CHANNEL_DDR */

static int plato_memory_test(struct plato_device *plato)
{
	struct device *dev = &plato->pdev->dev;
	u64 i, j = 0;
	u32 tmp = 0;
	u32 chunk = sizeof(u32) * 10;
	u64 mem_base = plato->rogue_heap_mappable.base;
	u64 mem_size = plato->rogue_heap_mappable.size +
		       PLATO_PDP_MEM_SIZE; /* test PDP memory heap too. */

	plato_dev_info(dev, "Starting Local memory test from 0x%llx to 0x%llx (in CPU space)",
			mem_base, mem_base + mem_size);

	while (j < mem_size - chunk) {
		u64 p_addr = mem_base + j;
		u32 *v_addr = (u32 *)ioremap(p_addr, chunk);

		for (i = 0; i < chunk/sizeof(u32); i++) {
			*(v_addr + i) = 0xdeadbeef;
			tmp = *(v_addr + i);
			if (tmp != 0xdeadbeef) {
				dev_err(dev,
					"Local memory read-write test failed at address=0x%llx: written 0x%x, read 0x%x",
					mem_base + ((i * sizeof(u32)) + j), (u32) 0xdeadbeef, tmp);

				iounmap(v_addr);
				return PLATO_INIT_FAILURE;
			}
		}

		iounmap(v_addr);

		j += (1024 * 1024 * 500);
	}

	dev_info(dev, "Local memory read-write test passed!");

	return PLATO_INIT_SUCCESS;
}



int plato_memory_init(struct plato_device *plato)
{
	struct device *dev = &plato->pdev->dev;

	/* Setup card memory */
	plato->rogue_mem.size = pci_resource_len(plato->pdev, SYS_DEV_MEM_PCI_BASENUM);
	if (request_pci_io_addr(plato->pdev, SYS_DEV_MEM_PCI_BASENUM, 0, plato->rogue_mem.size)
		!= PLATO_INIT_SUCCESS) {
		dev_err(dev, "Failed to request PCI memory region");
		return PLATO_INIT_FAILURE;
	}

	plato->rogue_mem.base = pci_resource_start(plato->pdev, SYS_DEV_MEM_PCI_BASENUM);

	if (arch_io_reserve_memtype_wc(plato->rogue_mem.base,
					plato->rogue_mem.size))
		return PLATO_INIT_FAILURE;
	plato->mtrr = arch_phys_wc_add(plato->rogue_mem.base,
					plato->rogue_mem.size);
	if (plato->mtrr < 0) {
		arch_io_free_memtype_wc(plato->rogue_mem.base,
					plato->rogue_mem.size);
		return PLATO_INIT_FAILURE;
	}

	plato->rogue_heap_mappable.base = plato->rogue_mem.base;
	plato->rogue_heap_mappable.size = plato->rogue_mem.size;

	if (!PLATO_HAS_NON_MAPPABLE(plato)) {
		plato->has_nonmappable = false;
		/*
		 * With a BAR size that's greater than the actual memory size,
		 * move base CPU address of the heap region to use last
		 * aliased part of odd region and first aliased part of even region.
		 *
		 * This allows to use full available memory in one contiguous heap region.
		 */
		dev_info(dev, "System does NOT have non-mappable memory (32GB BAR)");
		plato->rogue_heap_mappable.base +=
							PLATO_DRAM_SPLIT_ADDR -
							(SYS_DEV_MEM_REGION_SIZE >> 1) -
							PLATO_DDR_DEV_PHYSICAL_BASE;
		plato->rogue_heap_mappable.size = SYS_DEV_MEM_REGION_SIZE;

		plato->dev_mem_base = PLATO_DDR_ALIASED_DEV_PHYSICAL_BASE;
	} else {
		u64 preceding_region_base = 0;
		u64 preceding_region_size = 0;
		u64 following_region_base = 0;
		u64 following_region_size = 0;

		plato->has_nonmappable = true;

		dev_info(dev, "System has non-mappable memory (<8GB BAR)");

		plato->dev_mem_base = PLATO_DDR_DEV_PHYSICAL_BASE;

		/*
		 * With a BAR size less than the actual memory size (8GB),
		 * we need to dynamically calculate the device base address
		 * that the PCI memory window is pointing to. The address depends on
		 * what address in host memory space the memory BAR was assigned by the BIOS.
		 *
		 * Bits x to 34 in host CPU base address decide on where
		 * within the DRAM region the BAR points to, where x is the shift calculated
		 * below based on the mapped memory size (BAR).
		 */
		{
			u32 shift = __ffs64(plato->rogue_heap_mappable.size);
			u64 mask = (1ULL << (35-shift)) - 1ULL;

			plato->dev_mem_base += plato->rogue_heap_mappable.base & (mask << shift);
		}

		/*
		 * Our aliased address can point anywhere in the 32GB - 64GB range,
		 * we now need to determine the closest aliased address to the split point (48GB).
		 * This is done by first finding the offset from the previous segment (mod below)
		 * and adding it to either the DRAM split point or the start of the aliased
		 * region that's closest to the split point.
		 */
		if (plato->dev_mem_base >= PLATO_DRAM_SPLIT_ADDR)
			plato->dev_mem_base = PLATO_DRAM_SPLIT_ADDR +
				(plato->dev_mem_base % PLATO_DDR_ALIASED_DEV_SEGMENT_SIZE);
		else
			plato->dev_mem_base = PLATO_DDR_ALIASED_DEV_PHYSICAL_BASE +
				(plato->dev_mem_base % PLATO_DDR_ALIASED_DEV_SEGMENT_SIZE);

		// Setup non-mappable region if BAR size is less than
		// actual memory size (8GB)
		plato->rogue_heap_nonmappable.base = PLATO_DDR_DEV_PHYSICAL_BASE;

		/*
		 * If mapped region is not at the base of memory,
		 * then it is preceded by a non-mappable region
		 */
		 preceding_region_base = PLATO_DDR_ALIASED_DEV_PHYSICAL_BASE;
		 preceding_region_size = plato->dev_mem_base - preceding_region_base;

		/*
		 * If mapped region is not at the end of memory,
		 * then it is followed by a non-mappable region
		 */
		following_region_base = plato->dev_mem_base + plato->rogue_heap_mappable.size;
		following_region_size = PLATO_DDR_ALIASED_DEV_PHYSICAL_END -
			(plato->dev_mem_base + plato->rogue_heap_mappable.size);

		/* Use only bigger region for now */
		if (following_region_size > preceding_region_size) {
			plato->rogue_heap_nonmappable.base = following_region_base;
			plato->rogue_heap_nonmappable.size = following_region_size;
		} else {
			plato->rogue_heap_nonmappable.base = preceding_region_base;
			plato->rogue_heap_nonmappable.size = preceding_region_size;
		}
	}

#if defined(SUPPORT_PLATO_DISPLAY)
	if (plato->rogue_heap_mappable.size < PLATO_PDP_MEM_SIZE) {
		dev_err(dev, "Not enough memory for the PDP (0x%llx < 0x%llx)",
			(u64)plato->rogue_heap_mappable.size, (u64)PLATO_PDP_MEM_SIZE);
		plato_memory_deinit(plato);
		return PLATO_INIT_FAILURE;
	}

	plato->rogue_heap_mappable.size -= PLATO_PDP_MEM_SIZE;
	/* Setup ranges for the device heaps */
	plato->pdp_heap.size = PLATO_PDP_MEM_SIZE;
	plato->pdp_heap.base = plato->rogue_heap_mappable.base + plato->rogue_heap_mappable.size;
#endif

	plato_dev_info(dev, "Initialized rogue heap with base 0x%llx and size 0x%llx",
		(u64)plato->rogue_heap_mappable.base, (u64)plato->rogue_heap_mappable.size);

	if (plato_memory_test(plato) != PLATO_INIT_SUCCESS) {
		plato_memory_deinit(plato);
		return PLATO_INIT_FAILURE;
	}

	return PLATO_INIT_SUCCESS;
}

void plato_memory_deinit(struct plato_device *plato)
{
	PLATO_DRM_CHECKPOINT;

	if (plato->mtrr >= 0) {
		arch_phys_wc_del(plato->mtrr);

		arch_io_free_memtype_wc(plato->rogue_mem.base,
					plato->rogue_mem.size);
	}

	release_pci_io_addr(plato->pdev, SYS_DEV_MEM_PCI_BASENUM,
		plato->rogue_mem.base, plato->rogue_mem.size);
}

#if defined(EMULATOR)
static int plato_emu_init(struct plato_device *plato)
{
	struct device *dev = &plato->pdev->dev;
	void *perip_regs		= plato->sys_io.registers + SYS_PLATO_REG_PERIP_OFFSET;
	void *ddra_ctrl_regs	= plato->sys_io.registers + SYS_PLATO_REG_DDR_A_CTRL_OFFSET;
	void *ddra_publ_regs	= plato->sys_io.registers + SYS_PLATO_REG_DDR_A_PUBL_OFFSET;
	void *ddrb_ctrl_regs	= plato->sys_io.registers + SYS_PLATO_REG_DDR_B_CTRL_OFFSET;
	void *ddrb_publ_regs	= plato->sys_io.registers + SYS_PLATO_REG_DDR_B_PUBL_OFFSET;
	void *noc_regs			= plato->sys_io.registers + SYS_PLATO_REG_NOC_OFFSET;
	void *aon_regs			= plato->aon_regs.registers;

#if defined(ENABLE_PLATO_HDMI)
	plato_write_reg32(perip_regs, PLATO_TOP_CR_HDMI_CEC_CLK_CTRL, 0x3370A03);
	poll(dev, perip_regs, PLATO_TOP_CR_HDMI_CEC_CLK_CTRL, 0x3370A03, 0x3370A03);
	plato_write_reg32(perip_regs, PLATO_TOP_CR_I2C_CLK_CTRL, 0x1);
	poll(dev, perip_regs, PLATO_TOP_CR_I2C_CLK_CTRL, 0x1, 0x1);
#endif

	plato_write_reg32(aon_regs, PLATO_AON_CR_NOC_CLK_CTRL, 0x1);
	poll(dev, aon_regs, PLATO_AON_CR_NOC_CLK_CTRL, 0x1, 0x1);

	plato_write_reg32(aon_regs, PLATO_AON_CR_DDR_PLL_CTRL_0, 0x01101037);
	poll(dev, aon_regs, PLATO_AON_CR_DDR_PLL_CTRL_0, 0x01101037, 0x01101037);
	plato_write_reg32(aon_regs, PLATO_AON_CR_DDR_PLL_CTRL_1, 0x00780000);
	poll(dev, aon_regs, PLATO_AON_CR_DDR_PLL_CTRL_1, 0x00780000, 0x00780000);

	/* Waiting for DDR PLL getting locked */
	poll_pr(dev, aon_regs, PLATO_AON_CR_PLL_STATUS, 0x2, 0x2, -1, 10);

	plato_write_reg32(aon_regs, PLATO_AON_CR_PLL_BYPASS, 0x01);
	poll(dev, aon_regs, PLATO_AON_CR_PLL_BYPASS, 0x01, 0x01);

	plato_sleep_us(100);

#if defined(PLATO_DUAL_CHANNEL_DDR)
	plato_write_reg32(perip_regs, PLATO_TOP_CR_DDR_CLK_CTRL, 0x011);
	poll(dev, perip_regs, PLATO_TOP_CR_DDR_CLK_CTRL, 0x011, 0x011);
#else
	plato_write_reg32(perip_regs, PLATO_TOP_CR_DDR_CLK_CTRL, 0x01);
	poll(dev, perip_regs, PLATO_TOP_CR_DDR_CLK_CTRL, 0x01, 0x01);
#endif
	/* PLL Lock is done */

	plato_sleep_us(1000);

	/* Enabling PDP gated clock output - 198 MHz */

	plato_write_reg32(perip_regs, PLATO_TOP_CR_PDP_CLK_CTRL, 0x00001210);
	poll(dev, perip_regs, PLATO_TOP_CR_PDP_CLK_CTRL, 0x00001210, 0x00001210);

	plato_sleep_us(100);

	/* PDP needs HDMI clocks on for framegrabber, start them here */
	/* Enabling HDMI gated clock output - 148.5 MHz */
	plato_write_reg32(perip_regs, PLATO_TOP_CR_HDMI_CLK_CTRL, 0x00001310);
	poll(dev, perip_regs, PLATO_TOP_CR_HDMI_CLK_CTRL, 0x00001310, 0x00001310);

	plato_sleep_us(100);
	plato_dev_info(dev, "%s: Enabled PDP and HDMI clocks", __func__);

	/* GPU PLL configuration */
	plato_write_reg32(aon_regs, PLATO_AON_CR_GPU_PLL_CTRL_0, 0x0110103D);
	poll(dev, aon_regs, PLATO_AON_CR_GPU_PLL_CTRL_0, 0x0110103D, 0x0110103D);

	plato_write_reg32(aon_regs, PLATO_AON_CR_GPU_PLL_CTRL_1, 0x00E00000);
	poll(dev, aon_regs, PLATO_AON_CR_GPU_PLL_CTRL_1, 0x00E00000, 0x00E00000);
	/* Waiting for GPU PLL getting locked */

	poll_pr(dev, aon_regs, PLATO_AON_CR_PLL_STATUS, 0x3, 0x3, -1, 10);
	/* GPU and DDR PLL Locked */

	plato_write_reg32(aon_regs, PLATO_AON_CR_PLL_BYPASS, 0x01);
	poll(dev, aon_regs, PLATO_AON_CR_PLL_BYPASS, 0x1, 0x1);

	plato_sleep_us(100);

	/* Enabling gated clock output */
	plato_write_reg32(perip_regs, PLATO_TOP_CR_GPU_CLK_CTRL, 0x011);
	poll(dev, perip_regs, PLATO_TOP_CR_GPU_CLK_CTRL, 0x011, 0x011);

	plato_sleep_us(1000);

#if defined(PLATO_DUAL_CHANNEL_DDR)
	/* Setting dual memory interleaved mode */
	plato_write_reg32(noc_regs, 0x00000050, 0x01);
	poll_pr(dev, noc_regs, 0x00000058, 0x01, 0x1, 1, 10);

	/* Now putting DRAM controller out of reset */
	plato_write_reg32(aon_regs, PLATO_AON_CR_RESET_CTRL, 0x0F30);
	if (poll(dev, aon_regs, PLATO_AON_CR_RESET_CTRL, 0x0F30, 0x0F30))
		return PLATO_INIT_FAILURE;

	plato_sleep_us(10);

	plato_write_reg32(aon_regs, PLATO_AON_CR_RESET_CTRL, 0x0A30);
	poll(dev, aon_regs, PLATO_AON_CR_RESET_CTRL, 0x0A30, 0xA30);

#else
	plato_write_reg32(aon_regs, PLATO_AON_CR_RESET_CTRL, 0x0330);
	if (poll(dev, aon_regs, PLATO_AON_CR_RESET_CTRL, 0x0330, 0x0330))
		return PLATO_INIT_FAILURE;

	plato_sleep_us(10);

	/* Now putting DRAM controller into reset */
	plato_write_reg32(aon_regs, PLATO_AON_CR_RESET_CTRL, 0x0230);
	poll(dev, aon_regs, PLATO_AON_CR_RESET_CTRL, 0x0230, 0x0230);

#endif

	/*
	 * Phase 1: Program the DWC_ddr_umctl2 registers
	 */
	plato_write_reg32(ddra_ctrl_regs, PLATO_DDR_CTRL_MSTR, 0x8F040001);
	poll(dev, ddra_ctrl_regs, PLATO_DDR_CTRL_MSTR, 0x8F040001, 0x8F040001);

#if defined(PLATO_DUAL_CHANNEL_DDR)
	plato_write_reg32(ddrb_ctrl_regs, PLATO_DDR_CTRL_MSTR, 0x8F040001);
	poll(dev, ddrb_ctrl_regs, PLATO_DDR_CTRL_MSTR, 0x8F040001, 0x8F040001);
#endif

	plato_sleep_us(100);

	plato_write_reg32(ddra_ctrl_regs, PLATO_DDR_CTRL_RFSHTMG, 0x007f0056);
	plato_write_reg32(ddrb_ctrl_regs, PLATO_DDR_CTRL_RFSHTMG, 0x007f0056);

	plato_write_reg32(ddra_ctrl_regs, PLATO_DDR_CTRL_INIT3, 0x01140000);
	plato_write_reg32(ddrb_ctrl_regs, PLATO_DDR_CTRL_INIT3, 0x01140000);

	plato_write_reg32(ddra_ctrl_regs, PLATO_DDR_CTRL_INIT1, 0x00010000);
	plato_write_reg32(ddrb_ctrl_regs, PLATO_DDR_CTRL_INIT1, 0x00010000);

	plato_write_reg32(ddra_ctrl_regs, PLATO_DDR_CTRL_INIT0, 0x00020001);
	plato_write_reg32(ddrb_ctrl_regs, PLATO_DDR_CTRL_INIT0, 0x00020001);

	plato_write_reg32(ddra_ctrl_regs, PLATO_DDR_CTRL_INIT4, 0x00280000);
	plato_write_reg32(ddrb_ctrl_regs, PLATO_DDR_CTRL_INIT4, 0x00280000);

	plato_write_reg32(ddra_ctrl_regs, PLATO_DDR_CTRL_INIT5, 0x000C000C);
	plato_write_reg32(ddrb_ctrl_regs, PLATO_DDR_CTRL_INIT5, 0x000C000C);

	plato_write_reg32(ddra_ctrl_regs, PLATO_DDR_CTRL_DRAMTMG0, 0x0f132312);
	plato_write_reg32(ddrb_ctrl_regs, PLATO_DDR_CTRL_DRAMTMG0, 0x0f132312);

	plato_write_reg32(ddra_ctrl_regs, PLATO_DDR_CTRL_DRAMTMG1, 0x00080419);
	plato_write_reg32(ddrb_ctrl_regs, PLATO_DDR_CTRL_DRAMTMG1, 0x00080419);

	plato_write_reg32(ddra_ctrl_regs, PLATO_DDR_CTRL_DRAMTMG2, 0x0507050b);
	plato_write_reg32(ddrb_ctrl_regs, PLATO_DDR_CTRL_DRAMTMG2, 0x0507050b);

	plato_write_reg32(ddra_ctrl_regs, PLATO_DDR_CTRL_DRAMTMG3, 0x00002008);
	plato_write_reg32(ddrb_ctrl_regs, PLATO_DDR_CTRL_DRAMTMG3, 0x00002008);

	plato_write_reg32(ddra_ctrl_regs, PLATO_DDR_CTRL_DRAMTMG4, 0x07020407);
	plato_write_reg32(ddrb_ctrl_regs, PLATO_DDR_CTRL_DRAMTMG4, 0x07020407);

	plato_write_reg32(ddra_ctrl_regs, PLATO_DDR_CTRL_DRAMTMG5, 0x090e0403);
	plato_write_reg32(ddrb_ctrl_regs, PLATO_DDR_CTRL_DRAMTMG5, 0x090e0403);

	plato_write_reg32(ddra_ctrl_regs, PLATO_DDR_CTRL_DRAMTMG6, 0x020e000e);
	plato_write_reg32(ddrb_ctrl_regs, PLATO_DDR_CTRL_DRAMTMG6, 0x020e000e);

	plato_write_reg32(ddra_ctrl_regs, PLATO_DDR_CTRL_DRAMTMG7, 0x00000c0e);
	plato_write_reg32(ddrb_ctrl_regs, PLATO_DDR_CTRL_DRAMTMG7, 0x00000c0e);

	plato_write_reg32(ddra_ctrl_regs, PLATO_DDR_CTRL_DRAMTMG8, 0x01010a05);
	plato_write_reg32(ddrb_ctrl_regs, PLATO_DDR_CTRL_DRAMTMG8, 0x01010a05);

	plato_write_reg32(ddra_ctrl_regs, PLATO_DDR_CTRL_ZQCTL0, 0x30ab002b);
	plato_write_reg32(ddrb_ctrl_regs, PLATO_DDR_CTRL_ZQCTL0, 0x30ab002b);

	plato_write_reg32(ddra_ctrl_regs, PLATO_DDR_CTRL_ZQCTL1, 0x00000070);
	plato_write_reg32(ddrb_ctrl_regs, PLATO_DDR_CTRL_ZQCTL1, 0x00000070);

	plato_write_reg32(ddra_ctrl_regs, PLATO_DDR_CTRL_ZQCTL2, 0x00000000);
	plato_write_reg32(ddrb_ctrl_regs, PLATO_DDR_CTRL_ZQCTL2, 0x00000000);

	plato_write_reg32(ddra_ctrl_regs, PLATO_DDR_CTRL_RFSHCTL0, 0x00e01020);
	plato_write_reg32(ddrb_ctrl_regs, PLATO_DDR_CTRL_RFSHCTL0, 0x00e01020);

	plato_write_reg32(ddra_ctrl_regs, PLATO_DDR_CTRL_RFSHCTL1, 0x0078007e);
	plato_write_reg32(ddrb_ctrl_regs, PLATO_DDR_CTRL_RFSHCTL1, 0x0078007e);

	plato_write_reg32(ddra_ctrl_regs, PLATO_DDR_CTRL_RFSHCTL2, 0x0057000e);
	plato_write_reg32(ddrb_ctrl_regs, PLATO_DDR_CTRL_RFSHCTL2, 0x0057000e);

	plato_write_reg32(ddra_ctrl_regs, PLATO_DDR_CTRL_RFSHCTL3, 0x00000000);
	plato_write_reg32(ddrb_ctrl_regs, PLATO_DDR_CTRL_RFSHCTL3, 0x00000000);

	plato_write_reg32(ddra_ctrl_regs, PLATO_DDR_CTRL_DFITMG0, 0x028A8208);
	plato_write_reg32(ddrb_ctrl_regs, PLATO_DDR_CTRL_DFITMG0, 0x028A8208);

	plato_write_reg32(ddra_ctrl_regs, PLATO_DDR_CTRL_DFITMG1, 0x00020202);
	plato_write_reg32(ddrb_ctrl_regs, PLATO_DDR_CTRL_DFITMG1, 0x00020202);

	plato_write_reg32(ddra_ctrl_regs, PLATO_DDR_CTRL_DFIUPD0, 0x00400003);
	plato_write_reg32(ddrb_ctrl_regs, PLATO_DDR_CTRL_DFIUPD0, 0x00400003);

	plato_write_reg32(ddra_ctrl_regs, PLATO_DDR_CTRL_DFIUPD1, 0x00F000FF);
	plato_write_reg32(ddrb_ctrl_regs, PLATO_DDR_CTRL_DFIUPD1, 0x00F000FF);

	plato_write_reg32(ddra_ctrl_regs, PLATO_DDR_CTRL_DFIUPD2, 0x80100010);
	plato_write_reg32(ddrb_ctrl_regs, PLATO_DDR_CTRL_DFIUPD2, 0x80100010);

	plato_write_reg32(ddra_ctrl_regs, PLATO_DDR_CTRL_DFIUPD3, 0x00100010);
	plato_write_reg32(ddrb_ctrl_regs, PLATO_DDR_CTRL_DFIUPD3, 0x00100010);

	plato_write_reg32(ddra_ctrl_regs, PLATO_DDR_CTRL_DFIMISC, 0x00000000);
	plato_write_reg32(ddrb_ctrl_regs, PLATO_DDR_CTRL_DFIMISC, 0x00000000);

	plato_write_reg32(ddra_ctrl_regs, PLATO_DDR_CTRL_ADDRMAP0, 0x00001414);
	plato_write_reg32(ddra_ctrl_regs, PLATO_DDR_CTRL_ADDRMAP1, 0x00070707);
	plato_write_reg32(ddra_ctrl_regs, PLATO_DDR_CTRL_ADDRMAP2, 0x00000000);
	plato_write_reg32(ddra_ctrl_regs, PLATO_DDR_CTRL_ADDRMAP3, 0x0F000000);
	plato_write_reg32(ddra_ctrl_regs, PLATO_DDR_CTRL_ADDRMAP4, 0x00000F0F);
	plato_write_reg32(ddra_ctrl_regs, PLATO_DDR_CTRL_ADDRMAP5, 0x06060606);
	plato_write_reg32(ddra_ctrl_regs, PLATO_DDR_CTRL_ADDRMAP6, 0x0F0F0606);

	plato_write_reg32(ddrb_ctrl_regs, PLATO_DDR_CTRL_ADDRMAP0, 0x00001414);
	plato_write_reg32(ddrb_ctrl_regs, PLATO_DDR_CTRL_ADDRMAP1, 0x00070707);
	plato_write_reg32(ddrb_ctrl_regs, PLATO_DDR_CTRL_ADDRMAP2, 0x00000000);
	plato_write_reg32(ddrb_ctrl_regs, PLATO_DDR_CTRL_ADDRMAP3, 0x0F000000);
	plato_write_reg32(ddrb_ctrl_regs, PLATO_DDR_CTRL_ADDRMAP4, 0x00000F0F);
	plato_write_reg32(ddrb_ctrl_regs, PLATO_DDR_CTRL_ADDRMAP5, 0x06060606);
	plato_write_reg32(ddrb_ctrl_regs, PLATO_DDR_CTRL_ADDRMAP6, 0x0F0F0606);

	plato_write_reg32(ddra_ctrl_regs, PLATO_DDR_CTRL_ODTCFG, 0x04000400);
	plato_write_reg32(ddrb_ctrl_regs, PLATO_DDR_CTRL_ODTCFG, 0x04000400);

	plato_write_reg32(ddra_ctrl_regs, PLATO_DDR_CTRL_SCHED, 0x00003F00);
	plato_write_reg32(ddrb_ctrl_regs, PLATO_DDR_CTRL_SCHED, 0x00003F00);

	plato_write_reg32(ddra_ctrl_regs, PLATO_DDR_CTRL_PERFHPR1, 0x0F0000FF);
	plato_write_reg32(ddrb_ctrl_regs, PLATO_DDR_CTRL_PERFHPR1, 0x0F0000FF);

	plato_write_reg32(ddra_ctrl_regs, PLATO_DDR_CTRL_PERFLPR1, 0x0F0000FF);
	plato_write_reg32(ddrb_ctrl_regs, PLATO_DDR_CTRL_PERFLPR1, 0x0F0000FF);

	plato_write_reg32(ddra_ctrl_regs, PLATO_DDR_CTRL_PERFWR1, 0x0F0000FF);
	plato_write_reg32(ddrb_ctrl_regs, PLATO_DDR_CTRL_PERFWR1, 0x0F0000FF);

	/* Setup the virtual channels */
	plato_write_reg32(ddra_ctrl_regs, 0x00000410, 0x0000000F);
	plato_write_reg32(ddra_ctrl_regs, 0x00000414, 0x00000000);
	plato_write_reg32(ddra_ctrl_regs, 0x00000418, 0x0000000F);
	plato_write_reg32(ddra_ctrl_regs, 0x0000041C, 0x00000001);
	plato_write_reg32(ddra_ctrl_regs, 0x00000420, 0x0000000F);
	plato_write_reg32(ddra_ctrl_regs, 0x00000424, 0x00000002);
	plato_write_reg32(ddra_ctrl_regs, 0x00000428, 0x0000000F);
	plato_write_reg32(ddra_ctrl_regs, 0x0000042C, 0x00000003);
	plato_write_reg32(ddra_ctrl_regs, 0x00000430, 0x0000000F);
	plato_write_reg32(ddra_ctrl_regs, 0x00000434, 0x00000004);
	plato_write_reg32(ddra_ctrl_regs, 0x00000438, 0x0000000F);
	plato_write_reg32(ddra_ctrl_regs, 0x0000043C, 0x00000005);
	plato_write_reg32(ddra_ctrl_regs, 0x00000440, 0x0000000F);
	plato_write_reg32(ddra_ctrl_regs, 0x00000444, 0x00000006);
	plato_write_reg32(ddra_ctrl_regs, 0x00000448, 0x0000000F);
	plato_write_reg32(ddra_ctrl_regs, 0x0000044C, 0x00000007);
	plato_write_reg32(ddra_ctrl_regs, 0x00000450, 0x0000000F);
	plato_write_reg32(ddra_ctrl_regs, 0x00000454, 0x00000008);
	plato_write_reg32(ddra_ctrl_regs, 0x00000458, 0x0000000F);
	plato_write_reg32(ddra_ctrl_regs, 0x0000045C, 0x00000009);
	plato_write_reg32(ddra_ctrl_regs, 0x00000460, 0x0000000F);
	plato_write_reg32(ddra_ctrl_regs, 0x00000464, 0x0000000A);
	plato_write_reg32(ddra_ctrl_regs, 0x00000468, 0x0000000F);
	plato_write_reg32(ddra_ctrl_regs, 0x0000046C, 0x0000000B);
	plato_write_reg32(ddra_ctrl_regs, 0x00000470, 0x0000000F);
	plato_write_reg32(ddra_ctrl_regs, 0x00000474, 0x0000000C);
	plato_write_reg32(ddra_ctrl_regs, 0x00000478, 0x0000000F);
	plato_write_reg32(ddra_ctrl_regs, 0x0000047C, 0x0000000D);
	plato_write_reg32(ddra_ctrl_regs, 0x00000480, 0x0000000F);
	plato_write_reg32(ddra_ctrl_regs, 0x00000484, 0x0000000E);

	plato_write_reg32(ddrb_ctrl_regs, 0x00000410, 0x0000000F);
	plato_write_reg32(ddrb_ctrl_regs, 0x00000414, 0x00000000);
	plato_write_reg32(ddrb_ctrl_regs, 0x00000418, 0x0000000F);
	plato_write_reg32(ddrb_ctrl_regs, 0x0000041C, 0x00000001);
	plato_write_reg32(ddrb_ctrl_regs, 0x00000420, 0x0000000F);
	plato_write_reg32(ddrb_ctrl_regs, 0x00000424, 0x00000002);
	plato_write_reg32(ddrb_ctrl_regs, 0x00000428, 0x0000000F);
	plato_write_reg32(ddrb_ctrl_regs, 0x0000042C, 0x00000003);
	plato_write_reg32(ddrb_ctrl_regs, 0x00000430, 0x0000000F);
	plato_write_reg32(ddrb_ctrl_regs, 0x00000434, 0x00000004);
	plato_write_reg32(ddrb_ctrl_regs, 0x00000438, 0x0000000F);
	plato_write_reg32(ddrb_ctrl_regs, 0x0000043C, 0x00000005);
	plato_write_reg32(ddrb_ctrl_regs, 0x00000440, 0x0000000F);
	plato_write_reg32(ddrb_ctrl_regs, 0x00000444, 0x00000006);
	plato_write_reg32(ddrb_ctrl_regs, 0x00000448, 0x0000000F);
	plato_write_reg32(ddrb_ctrl_regs, 0x0000044C, 0x00000007);
	plato_write_reg32(ddrb_ctrl_regs, 0x00000450, 0x0000000F);
	plato_write_reg32(ddrb_ctrl_regs, 0x00000454, 0x00000008);
	plato_write_reg32(ddrb_ctrl_regs, 0x00000458, 0x0000000F);
	plato_write_reg32(ddrb_ctrl_regs, 0x0000045C, 0x00000009);
	plato_write_reg32(ddrb_ctrl_regs, 0x00000460, 0x0000000F);
	plato_write_reg32(ddrb_ctrl_regs, 0x00000464, 0x0000000A);
	plato_write_reg32(ddrb_ctrl_regs, 0x00000468, 0x0000000F);
	plato_write_reg32(ddrb_ctrl_regs, 0x0000046C, 0x0000000B);
	plato_write_reg32(ddrb_ctrl_regs, 0x00000470, 0x0000000F);
	plato_write_reg32(ddrb_ctrl_regs, 0x00000474, 0x0000000C);
	plato_write_reg32(ddrb_ctrl_regs, 0x00000478, 0x0000000F);
	plato_write_reg32(ddrb_ctrl_regs, 0x0000047C, 0x0000000D);
	plato_write_reg32(ddrb_ctrl_regs, 0x00000480, 0x0000000F);
	plato_write_reg32(ddrb_ctrl_regs, 0x00000484, 0x0000000E);

	/* DRAM controller registers configuration done */

	plato_sleep_us(100);

	/*
	 * Phase 2: Deassert soft reset signal core_ddrc_rstn
	 */

	/* Now getting DRAM controller out of reset */
#if defined(PLATO_DUAL_CHANNEL_DDR)
	plato_write_reg32(aon_regs, PLATO_AON_CR_RESET_CTRL, 0x00000F30);
#else
	plato_write_reg32(aon_regs, PLATO_AON_CR_RESET_CTRL, 0x00000330);
#endif

	plato_sleep_us(100);

	/* ECC disable */
	plato_write_reg32(ddra_ctrl_regs, PLATO_DDR_CTRL_DBG1, 0x00000000);
	/* power related */
	plato_write_reg32(ddra_ctrl_regs, PLATO_DDR_CTRL_PWRCTL, 0x00000000);
	/* Enabling AXI input port (Port control) */
	plato_write_reg32(ddra_ctrl_regs, PLATO_DDR_CTRL_PCTRL, 0x00000001);

	plato_write_reg32(ddrb_ctrl_regs, PLATO_DDR_CTRL_DBG1, 0x00000000);
	plato_write_reg32(ddrb_ctrl_regs, PLATO_DDR_CTRL_PWRCTL, 0x00000000);
	plato_write_reg32(ddrb_ctrl_regs, PLATO_DDR_CTRL_PCTRL, 0x00000001);

	/*
	 * Phase 7: Set DFIMISC.dfi_init_complete_en to 1
	 * (skipped on emu?)
	 */

	/*
	 * Phase 3: Start PHY initialization by accessing relevant PUB registers
	 */
	plato_write_reg32(ddra_publ_regs, PLATO_DDR_PUBL_DCR_OFFSET, 0x0000040B);
	plato_write_reg32(ddrb_publ_regs, PLATO_DDR_PUBL_DCR_OFFSET, 0x0000040B);

	plato_write_reg32(ddra_publ_regs, PLATO_DDR_PUBL_PTR0_OFFSET, 0x10000010);
	plato_write_reg32(ddrb_publ_regs, PLATO_DDR_PUBL_PTR0_OFFSET, 0x10000010);

	plato_write_reg32(ddra_publ_regs, PLATO_DDR_PUBL_PTR1_OFFSET, 0x00800080);
	plato_write_reg32(ddrb_publ_regs, PLATO_DDR_PUBL_PTR1_OFFSET, 0x00800080);

	plato_write_reg32(ddra_publ_regs, PLATO_DDR_PUBL_PTR2_OFFSET, 0x00080421);
	plato_write_reg32(ddrb_publ_regs, PLATO_DDR_PUBL_PTR2_OFFSET, 0x00080421);

	plato_write_reg32(ddra_publ_regs, PLATO_DDR_PUBL_DSGCR_OFFSET, 0x0020641F);
	plato_write_reg32(ddrb_publ_regs, PLATO_DDR_PUBL_DSGCR_OFFSET, 0x0020641F);

	plato_write_reg32(ddra_publ_regs, PLATO_DDR_PUBL_MR0_OFFSET, 0x00000114);
	plato_write_reg32(ddra_publ_regs, PLATO_DDR_PUBL_MR1_OFFSET, 0x00000000);
	plato_write_reg32(ddra_publ_regs, PLATO_DDR_PUBL_MR2_OFFSET, 0x00000028);
	plato_write_reg32(ddra_publ_regs, PLATO_DDR_PUBL_MR3_OFFSET, 0x00000000);

	plato_write_reg32(ddrb_publ_regs, PLATO_DDR_PUBL_MR0_OFFSET, 0x00000114);
	plato_write_reg32(ddrb_publ_regs, PLATO_DDR_PUBL_MR1_OFFSET, 0x00000000);
	plato_write_reg32(ddrb_publ_regs, PLATO_DDR_PUBL_MR2_OFFSET, 0x00000028);
	plato_write_reg32(ddrb_publ_regs, PLATO_DDR_PUBL_MR3_OFFSET, 0x00000000);

	plato_write_reg32(ddra_publ_regs, PLATO_DDR_PUBL_DTPR0_OFFSET, 0x040F0406);
	plato_write_reg32(ddrb_publ_regs, PLATO_DDR_PUBL_DTPR0_OFFSET, 0x040F0406);

	plato_write_reg32(ddra_publ_regs, PLATO_DDR_PUBL_DTPR1_OFFSET, 0x28110402);
	plato_write_reg32(ddrb_publ_regs, PLATO_DDR_PUBL_DTPR1_OFFSET, 0x28110402);

	plato_write_reg32(ddra_publ_regs, PLATO_DDR_PUBL_DTPR2_OFFSET, 0x00030002);
	plato_write_reg32(ddrb_publ_regs, PLATO_DDR_PUBL_DTPR2_OFFSET, 0x00030002);

	plato_write_reg32(ddra_publ_regs, PLATO_DDR_PUBL_DTPR3_OFFSET, 0x02000101);
	plato_write_reg32(ddrb_publ_regs, PLATO_DDR_PUBL_DTPR3_OFFSET, 0x02000101);

	plato_write_reg32(ddra_publ_regs, PLATO_DDR_PUBL_DTPR4_OFFSET, 0x00190602);
	plato_write_reg32(ddrb_publ_regs, PLATO_DDR_PUBL_DTPR4_OFFSET, 0x00190602);

	plato_write_reg32(ddra_publ_regs, PLATO_DDR_PUBL_DTPR5_OFFSET, 0x0018040B);
	plato_write_reg32(ddrb_publ_regs, PLATO_DDR_PUBL_DTPR5_OFFSET, 0x0018040B);

	plato_write_reg32(ddra_publ_regs, PLATO_DDR_PUBL_PGCR1_OFFSET, 0x020046A0);
	plato_write_reg32(ddrb_publ_regs, PLATO_DDR_PUBL_PGCR1_OFFSET, 0x020046A0);

	/*
	 * Phase 4: Trigger PHY initialization: Impedance, PLL, and DDL;
	 * assert PHY reset
	 */
	plato_write_reg32(ddra_publ_regs, PLATO_DDR_PUBL_PIR_OFFSET, 0x00000073);
	plato_write_reg32(ddrb_publ_regs, PLATO_DDR_PUBL_PIR_OFFSET, 0x00000073);

	/*
	 * Phase 5: Monitor PHY initialization status by polling the
	 * PUB register PGSR0
	 */
	plato_write_reg32(ddra_ctrl_regs, PLATO_DDR_CTRL_SWCTL, 0x00000000);
	plato_write_reg32(ddrb_ctrl_regs, PLATO_DDR_CTRL_SWCTL, 0x00000000);

	plato_write_reg32(ddra_ctrl_regs, PLATO_DDR_CTRL_DFIMISC, 0x00000001);
	plato_write_reg32(ddrb_ctrl_regs, PLATO_DDR_CTRL_DFIMISC, 0x00000001);

	plato_write_reg32(ddra_ctrl_regs, PLATO_DDR_CTRL_SWCTL, 0x00000001);
	plato_write_reg32(ddrb_ctrl_regs, PLATO_DDR_CTRL_SWCTL, 0x00000001);

	/*
	 * Phase 6: Indicate to the PUB that the controller will perform SDRAM
	 * initialization by setting PIR.INIT and PIR.CTLDINIT, and
	 * poll PGSR0.IDONE
	 */
	plato_write_reg32(ddra_publ_regs, PLATO_DDR_PUBL_PIR_OFFSET, 0x00040001);
	plato_write_reg32(ddrb_publ_regs, PLATO_DDR_PUBL_PIR_OFFSET, 0x00040001);

	/*
	 * Phase 8: Wait for DWC_ddr_umctl2 to move to "normal" operating
	 * mode by monitoring STAT.operating_mode signal
	 */
	poll_pr(dev, ddra_ctrl_regs, PLATO_DDR_CTRL_STAT, 0x01, 0x01, 10, 100);
#if defined(PLATO_DUAL_CHANNEL_DDR)
	poll_pr(dev, ddrb_ctrl_regs, PLATO_DDR_CTRL_STAT, 0x01, 0x01, 10, 100);
#endif
	plato_sleep_us(100);

	/* Getting GPU And DDR A out of reset */
#if defined(PLATO_DUAL_CHANNEL_DDR)
	plato_write_reg32(aon_regs, PLATO_AON_CR_RESET_CTRL, 0x00000F12);
	poll_pr(dev, aon_regs,
			PLATO_AON_CR_RESET_CTRL,
			0x00000F12, 0x00000F12, -1, 100);
#else
	plato_write_reg32(aon_regs, PLATO_AON_CR_RESET_CTRL, 0x00000312);
	poll_pr(dev, aon_regs,
			PLATO_AON_CR_RESET_CTRL,
			0x00000312, 0x00000312, -1, 100);
#endif

	/* setting CR_ISO_CTRL:CR_GPU_CLK_E */
	plato_write_reg32(aon_regs, PLATO_AON_CR_ISO_CTRL, 0x000001F);

	return PLATO_INIT_SUCCESS;
}

#else /* Actual HW init */
static int plato_hw_init(struct plato_device *plato)
{
	int err = 0;
	u32 max_restart = PLATO_PDP_RELOADS_MAX;
	u32 restart_count = 0;

	PLATO_CHECKPOINT(plato);

	/* Config Plato until PDP registers become accessible */
	do {
		#if defined(PLATO_DUAL_CHANNEL_DDR)
		err = plato_dual_channel_init(plato);
		#else
		err = plato_single_channel_init(plato);
		#endif

		restart_count++;

		if (err == PLATO_INIT_SUCCESS)
			break;

	} while (restart_count < max_restart &&
			 err == PLATO_INIT_RETRY);

	plato_dev_info(dev, "%s: status %d, number of tries %d",
				__func__, err, restart_count);

	return err;
}

#endif

int plato_cfg_init(struct plato_device *plato)
{
	#if defined(EMULATOR)
	return plato_emu_init(plato);
	#elif defined(VIRTUAL_PLATFORM)
	return PLATO_INIT_SUCCESS;
	#else
	return plato_hw_init(plato);
	#endif
}
