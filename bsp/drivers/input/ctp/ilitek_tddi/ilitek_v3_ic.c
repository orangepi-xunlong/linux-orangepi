/*
 * ILITEK Touch IC driver
 *
 * Copyright (C) 2011 ILI Technology Corporation.
 *
 * Author: Dicky Chiang <dicky_chiang@ilitek.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include "ilitek_v3.h"

#define PROTOCL_VER_NUM		8
static struct ilitek_protocol_info protocol_info[PROTOCL_VER_NUM] = {
	/* length -> fw, protocol, tp, key, panel, core, func, window, cdc, mp_info */
	[0] = {PROTOCOL_VER_500, 4, 4, 14, 30, 5, 5, 2, 8, 3, 8},
	[1] = {PROTOCOL_VER_510, 4, 3, 14, 30, 5, 5, 3, 8, 3, 8},
	[2] = {PROTOCOL_VER_520, 4, 4, 14, 30, 5, 5, 3, 8, 3, 8},
	[3] = {PROTOCOL_VER_530, 9, 4, 14, 30, 5, 5, 3, 8, 3, 8},
	[4] = {PROTOCOL_VER_540, 9, 4, 14, 30, 5, 5, 3, 8, 15, 8},
	[5] = {PROTOCOL_VER_550, 9, 4, 14, 30, 5, 5, 3, 8, 15, 14},
	[6] = {PROTOCOL_VER_560, 9, 4, 14, 30, 5, 5, 3, 8, 15, 14},
	[7] = {PROTOCOL_VER_570, 9, 4, 14, 30, 5, 5, 3, 8, 15, 14},
};

struct ilitek_ic_func_ctrl func_ctrl[FUNC_CTRL_NUM] = {
	/* cmd[3] = cmd, func, ctrl */
	/* rec_state 0:disable, 1: enable, 2: ignore record */
	[0] = {"sense", {0x1, 0x1, 0x0}, 3, 0x0, 2, 0xFF},
	[1] = {"sleep", {0x1, 0x2, 0x0}, 3, 0x0, 2, 0xFF},
	[2] = {"glove", {0x1, 0x6, 0x0}, 3, 0x0, 0, 0xFF},
	[3] = {"stylus", {0x1, 0x7, 0x0}, 3, 0x0, 0, 0xFF},
	[4] = {"lpwg", {0x1, 0xA, 0x0}, 3, 0x0, 2, 0xFF},
	[5] = {"proximity", {0x1, 0x10, 0x0}, 3, 0x0, 0, 0xFF},
	[6] = {"plug", {0x1, 0x11, 0x0}, 3, 0x1, 0, 0xFF},
	[7] = {"edge_palm", {0x1, 0x12, 0x0}, 3, 0x1, 0, 0xFF},
	[8] = {"lock_point", {0x1, 0x13, 0x0}, 3, 0x0, 0, 0xFF},
	[9] = {"active", {0x1, 0x14, 0x0}, 3, 0x0, 2, 0xFF},
	[10] = {"idle", {0x1, 0x19, 0x0}, 3, 0x1, 0, 0xFF},
	[11] = {"gesture_demo_en", {0x1, 0x16, 0x0}, 3, 0x0, 0, 0xFF},
	[12] = {"tp_recore", {0x1, 0x18, 0x0}, 3, 0x0, 2, 0xFF},
	[13] = {"knock_en", {0x1, 0xA, 0x8, 0x03, 0x0, 0x0}, 6, 0xFF, 0, 0xFF},
	[14] = {"int_trigger", {0x1, 0x1B, 0x0}, 3, 0x0, 2, 0xFF},
	[15] = {"ear_phone", {0x1, 0x17, 0x0}, 3, 0x0, 0, 0xFF},
	[16] = {"knuckle", {0x1, 0xF, 0x0}, 3, 0x0, 0, 0xFF},
};

static int ili_check_polling_bist(u8 GroupNumIdx, u32 SramRsultStatus)
{
	u32 Bist_Sram_Fail = 0xFFFFFFFF;
	int CheckCount = 10, ret = 0;
	u32 bist_done = 0, bist_fail = 0, bist_fail_bus = 0;

	ili_ice_mode_write(0x46000, 0, 4);

	ili_ice_mode_read(0x04600C, &bist_done, 1);
	ili_ice_mode_read(0x04600D, &bist_fail, 1);

	/* Check Clear MBist Status */
	if (ilits->sram_para.check_failbus == ENABLE) {
		ili_ice_mode_read(0x046010, &bist_fail_bus, sizeof(bist_fail_bus));

		if (((bist_done & BIT(0)) != 0x00) || ((bist_fail & BIT(0)) != 0x01) || (bist_fail_bus != 0xFFFFFFFF)) {
			ILI_ERR("Clear MBist Fail\n");
			ILI_ERR("bist_done = 0x%X, bist_fail = 0x%X, bist_fail_bus = 0x%X\n", bist_done, bist_fail, bist_fail_bus);
			ret = -1;
			goto out;
		}
	} else {
		if (((bist_done & BIT(0)) != 0x00) || ((bist_fail & BIT(0)) != 0x01)) {
			ILI_ERR("Clear MBist Fail\n");
			ILI_ERR("bist_done = 0x%X, bist_fail = 0x%X\n", bist_done, bist_fail);
			ret = -1;
			goto out;
		}
	}

	ili_ice_mode_write(0x046000, 0x00000001, 4);
	ili_ice_mode_write(0x046000, 0x00000101, 4);

	while (CheckCount > 0) {
		mdelay(2);
		ili_ice_mode_read(0x04600C, &bist_done, 1);

		if ((bist_done & BIT(0)) == 0x01) {
			break;
		}
		CheckCount--;
	}

	if (CheckCount <= 0) {
		ILI_ERR("Check Bist_Done TimeOut! bist_done = 0x%X\n", bist_done);
		ret = -1;
		goto out;
	}

	ili_ice_mode_read(0x04600D, &bist_fail, 1);

	if(ilits->sram_para.sram_group_en == ENABLE) {
		ili_ice_mode_read(ilits->sram_para.read_addr, &Bist_Sram_Fail, 4);

		if ((Bist_Sram_Fail & SramRsultStatus) > 0) {	
			ILI_ERR("Check Bist Fail Flag Fail!\n");
			ILI_ERR("Bist Sram Fail Status = 0x%X, Sram Rsult Status = 0x%X\n", Bist_Sram_Fail, SramRsultStatus);
			ret = -1;
			goto out;
		}
	} else {
		if (ilits->sram_para.check_failbus == ENABLE) {
			ili_ice_mode_read(0x046010, &bist_fail_bus, sizeof(bist_fail_bus));

			if (((bist_fail & BIT(0)) != 0x00) || (bist_fail_bus != 0x00000000)) {
				ILI_ERR("Check Bist Fail Flag Fail\n");
				ILI_DBG("bist_fail = %d, bist_fail_bus = %d\n", bist_fail, bist_fail_bus);
				ret = -1;
				goto out;
			}
		} else {
			if ((bist_fail & BIT(0)) != 0x00) {
				ILI_ERR("Check Bist Fail Flag Fail\n");
				ILI_DBG("bist_fail = %d\n", bist_fail);
				ret = -1;
				goto out;
			}
		}
	}

out:
	return ret;
}

static void ili_sram_test(u32 MaxAddr, u32 MinAddr)
{
	u32 SetValue;

	/* Strat Sram Test */
	ili_ice_mode_write_by_mode(0x40004, 0x00000001, 4, BOTH);
	ili_ice_mode_write_by_mode(0x40010, 0x00009878, 4, BOTH);
	ili_ice_mode_write_by_mode(0x40088, 0x00006B8A, 4, BOTH);

	/* Set MinAddr */
	ili_ice_mode_write_by_mode(0x46038, MinAddr, 2, BOTH);

	/* March_LR */ 
	SetValue = ((MaxAddr << 16) & 0xFFFF0000) | 0xFF00;

	ili_ice_mode_write_by_mode(0x46000, 0x00010000, 4, BOTH);
	ili_ice_mode_write_by_mode(0x46004, SetValue, 4, BOTH);
	ili_ice_mode_write_by_mode(0x46014, 0xA4409800, 4, BOTH);
	ili_ice_mode_write_by_mode(0x46018, 0xB480F590, 4, BOTH);
	ili_ice_mode_write_by_mode(0x4601C, 0x9000F560, 4, BOTH);
	ili_ice_mode_write_by_mode(0x46020, 0x9280C460, 4, BOTH);
	ili_ice_mode_write_by_mode(0x46024, 0x0000, 2, BOTH);
}

static int ili_sram_by_partial_area_test(void)
{
	int ret = 0;
	u8 nSramNumIdx;

	for(nSramNumIdx = 0; nSramNumIdx < ilits->sram_para.group_num; nSramNumIdx++) {
		ILI_DBG("Sram Partial Area : %s\n", ilits->sram_para.strSramPartialArea[nSramNumIdx]);

		ili_ice_mode_write_by_mode(ilits->sram_para.write_addr, ilits->sram_para.SramPartialAreaTest_en[nSramNumIdx], 4, BOTH);
		ILI_DBG("Write Sram Partial Area En : 0x%X\n", ilits->sram_para.SramPartialAreaTest_en[nSramNumIdx]);

		ili_sram_test(ilits->sram_para.MaxAddr, ilits->sram_para.MinAddr);

		if (ili_check_polling_bist(nSramNumIdx, ilits->sram_para.SramResult[nSramNumIdx]) < 0) {
			ILI_ERR("Check March_LR Bist Fail during partial area test\n");
			ret = -1;
		}
	}

	return ret;
}

static int ili_ic_cascade_polling_bist(void)
{
	int retry = 10, ret = 0;
	u32 bist_done = 0, bist_fail = 0, bist_fail_bus = 0;
	u32 bist_slave_done = 0, bist_slave_fail = 0, bist_slave_fail_bus = 0;
	u32 pen_bist_fail_bus = 0, pen_bist_slave_fail_bus = 0;

	ili_ice_mode_write_by_mode(0x046000, 0x000000, 4, BOTH);

#if (TDDI_INTERFACE == BUS_I2C)
		ilits->i2c_addr_change = SLAVE;
		ili_ice_mode_read(0x04600C, &bist_slave_done, sizeof(bist_slave_done));
		ili_ice_mode_read(0x04600D, &bist_slave_fail, sizeof(bist_slave_fail));


		ilits->i2c_addr_change = MASTER;
		ili_ice_mode_read(0x04600C, &bist_done, sizeof(bist_done));
		ili_ice_mode_read(0x04600D, &bist_fail, sizeof(bist_fail));

		if (ilits->sram_para.check_failbus == ENABLE) {
			ilits->i2c_addr_change = SLAVE;
			ili_ice_mode_read(0x046010, &bist_slave_fail_bus, sizeof(bist_slave_fail_bus));
			ili_ice_mode_read(0x046048, &pen_bist_slave_fail_bus, sizeof(pen_bist_slave_fail_bus));

			ilits->i2c_addr_change = MASTER;
			ili_ice_mode_read(0x046010, &bist_fail_bus, sizeof(bist_fail_bus));
			ili_ice_mode_read(0x046048, &pen_bist_fail_bus, sizeof(pen_bist_fail_bus));

			if (((bist_done & BIT(0)) != 0x00) || ((bist_fail & BIT(0)) != 0x01) || (bist_fail_bus != 0xFFFFFFFF)
				|| ((bist_slave_done & BIT(0)) != 0x00) || ((bist_slave_fail & BIT(0)) != 0x01)
				|| (bist_slave_fail_bus != 0xFFFFFFFF)) {

				ILI_ERR("SRAM TEST FAIL\n");
				ILI_ERR("bist_done = 0x%X, bist_fail = 0x%X, bist_fail_bus = 0x%X\n", bist_done, bist_fail, bist_fail_bus);
				ILI_ERR("bist_slave_done = 0x%X, bist_slave_fail = 0x%X, bist_slave_fail_bus = 0x%X\n", bist_slave_done, bist_slave_fail, bist_slave_fail_bus);
				ILI_ERR("pen_bist_fail_bus = 0x%X, pen_bist_slave_fail_bus = 0x%X\n", pen_bist_fail_bus, pen_bist_slave_fail_bus);
				ret = -1;
				goto out;
			}
		} else {
			if (((bist_done & BIT(0)) != 0x00) || ((bist_fail & BIT(0)) != 0x01) || ((bist_slave_done & BIT(0)) != 0x00)
				|| ((bist_slave_fail & BIT(0)) != 0x01)) {

				ILI_ERR("SRAM TEST FAIL\n");
				ILI_ERR("bist_done = 0x%X, bist_fail = 0x%X\n", bist_done, bist_fail);
				ILI_ERR("bist_slave_done = 0x%X, bist_slave_fail = 0x%X\n", bist_slave_done, bist_slave_fail);
				ret = -1;
				goto out;
			}
		}

		ili_ice_mode_write_by_mode(0x046000, 0x00000001, 4, BOTH);
		ili_ice_mode_write_by_mode(0x046000, 0x00000101, 4, BOTH);

		while (retry > 0) {
			mdelay(10);
			ilits->i2c_addr_change = SLAVE;
			ili_ice_mode_read(0x04600C, &bist_slave_done, sizeof(bist_slave_done));

			ilits->i2c_addr_change = MASTER;
			ili_ice_mode_read(0x04600C, &bist_done, sizeof(bist_done));
			ILI_ERR("bist_done = 0x%X, bist_slave_done = 0x%X\n", bist_done, bist_slave_done);


			if (((bist_done & BIT(0)) == 0x01) && ((bist_slave_done & BIT(0)) == 0x01)) {
				break;
			}
			retry--;
		}

		if (retry <= 0) {
			ILI_DBG("SRAM TEST FAIL\n");
			ret = -1;
			goto out;
		}

		ilits->i2c_addr_change = SLAVE;
		ili_ice_mode_read(0x04600D, &bist_slave_fail, sizeof(bist_slave_fail));

		ilits->i2c_addr_change = MASTER;
		ili_ice_mode_read(0x04600D, &bist_fail, sizeof(bist_fail));

		if (ilits->sram_para.check_failbus == ENABLE) {
			ilits->i2c_addr_change = SLAVE;
			ili_ice_mode_read(0x046010, &bist_slave_fail_bus, sizeof(bist_slave_fail_bus));
			ili_ice_mode_read(0x046048, &pen_bist_slave_fail_bus, sizeof(pen_bist_slave_fail_bus));

			ilits->i2c_addr_change = MASTER;
			ili_ice_mode_read(0x046010, &bist_fail_bus, sizeof(bist_fail_bus));
			ili_ice_mode_read(0x046048, &pen_bist_fail_bus, sizeof(pen_bist_fail_bus));

			if (((bist_fail & BIT(0)) != 0x00) || (bist_fail_bus != 0x00000000) || (((bist_slave_fail & BIT(24)) >> 24) != 0x00)
				|| (bist_slave_fail_bus != 0x00000000)) {

				ILI_ERR("SRAM TEST FAIL\n");
				ILI_ERR("bist_fail = 0x%X, bist_fail_bus = 0x%X\n", bist_fail, bist_fail_bus);
				ILI_ERR("bist_slave_fail = 0x%X, bist_slave_fail_bus = 0x%X\n", bist_slave_fail, bist_slave_fail_bus);
				ILI_ERR("pen_bist_fail_bus = 0x%X, pen_bist_slave_fail_bus = 0x%X\n", pen_bist_fail_bus, pen_bist_slave_fail_bus);
				ret = -1;
				goto out;
			}
		} else {
			if (((bist_fail & BIT(0)) != 0x00) || (((bist_slave_fail & BIT(24)) >> 24) != 0x00)) {
				ILI_ERR("SRAM TEST FAIL\n");
				ILI_ERR("bist_fail = 0x%X, bist_slave_fail = 0x%X\n", bist_fail, bist_slave_fail);
				ret = -1;
				goto out;
			}
		}
		

#elif (TDDI_INTERFACE == BUS_SPI)
		ili_ice_mode_write(READ_BACK_LOCK_REG, 0x01, 1);
		ili_ice_mode_read(0x04600C, &bist_done, sizeof(bist_done));
		ili_ice_mode_read(SLAVE_READ_BACK_WORD_REG, &bist_slave_done, sizeof(bist_slave_done));

		ili_ice_mode_write(READ_BACK_LOCK_REG, 0x01, 1);
		ili_ice_mode_read(0x04600D, &bist_fail, sizeof(bist_fail));
		ili_ice_mode_read(SLAVE_READ_BACK_WORD_REG, &bist_slave_fail, sizeof(bist_slave_fail));

		if (ilits->sram_para.check_failbus == ENABLE) {
			ili_ice_mode_write(READ_BACK_LOCK_REG, 0x01, 1);
			ili_ice_mode_read(0x046010, &bist_fail_bus, sizeof(bist_fail_bus));
			ili_ice_mode_read(SLAVE_READ_BACK_WORD_REG, &bist_slave_fail_bus, sizeof(bist_slave_fail_bus));

			ili_ice_mode_write(READ_BACK_LOCK_REG, 0x01, 1);
			ili_ice_mode_read(0x046048, &pen_bist_fail_bus, sizeof(pen_bist_fail_bus));
			ili_ice_mode_read(SLAVE_READ_BACK_WORD_REG, &pen_bist_slave_fail_bus, sizeof(pen_bist_slave_fail_bus));

			if (((bist_done & BIT(0)) != 0x00) || ((bist_fail & BIT(0)) != 0x01) || (bist_fail_bus != 0xFFFFFFFF)
				|| (((bist_slave_done & BIT(24)) >> 24) != 0x00) || (((bist_slave_fail & BIT(24)) >> 24) != 0x01)
				|| (bist_slave_fail_bus != 0xFFFFFFFF)) {

				ILI_ERR("SRAM TEST FAIL\n");
				ILI_ERR("bist_done = 0x%X, bist_fail = 0x%X, bist_fail_bus = 0x%X\n", bist_done, bist_fail, bist_fail_bus);
				ILI_ERR("bist_slave_done = 0x%X, bist_slave_fail = 0x%X, bist_slave_fail_bus = 0x%X\n", bist_slave_done, bist_slave_fail, bist_slave_fail_bus);
				ILI_ERR("pen_bist_fail_bus = 0x%X, pen_bist_slave_fail_bus = 0x%X\n", pen_bist_fail_bus, pen_bist_slave_fail_bus);
				ret = -1;
				goto out;
			}
		} else {
			if (((bist_done & BIT(0)) != 0x00) || ((bist_fail & BIT(0)) != 0x01) || (((bist_slave_done & BIT(24) >> 24)) != 0x00)
				|| (((bist_slave_fail & BIT(24)) >> 24) != 0x01)) {

				ILI_ERR("SRAM TEST FAIL\n");
				ILI_ERR("bist_done = 0x%X, bist_fail = 0x%X\n", bist_done, bist_fail);
				ILI_ERR("bist_slave_done = 0x%X, bist_slave_fail = 0x%X\n", bist_slave_done, bist_slave_fail);
				ret = -1;
				goto out;
			}
		}

		ili_ice_mode_write(0x046000, 0x00000001, 4);
		ili_ice_mode_write(0x046000, 0x00000101, 4);

		while (retry > 0) {
			mdelay(10);
			ili_ice_mode_write(READ_BACK_LOCK_REG, 0x01, 1);
			ili_ice_mode_read(0x04600C, &bist_done, sizeof(bist_done));
			ili_ice_mode_read(SLAVE_READ_BACK_WORD_REG, &bist_slave_done, sizeof(bist_slave_done));
			ILI_DBG("bist_done = 0x%X, bist_slave_done = 0x%X\n", bist_done, bist_slave_done);

			if (((bist_done & BIT(0)) == 0x01) && (((bist_slave_done & BIT(24)) >> 24) == 0x01)) {
				break;
			}
			retry--;
		}

		if (retry <= 0) {
			ILI_DBG("SRAM TEST FAIL\n");
			ret = -1;
			goto out;
		}

		ili_ice_mode_write(READ_BACK_LOCK_REG, 0x01, 1);
		ili_ice_mode_read(0x04600D, &bist_fail, sizeof(bist_fail));
		ili_ice_mode_read(SLAVE_READ_BACK_WORD_REG, &bist_slave_fail, sizeof(bist_slave_fail));

		if (ilits->sram_para.check_failbus == ENABLE) {
			ili_ice_mode_write(READ_BACK_LOCK_REG, 0x01, 1);
			ili_ice_mode_read(0x046010, &bist_fail_bus, sizeof(bist_fail_bus));
			ili_ice_mode_read(SLAVE_READ_BACK_WORD_REG, &bist_slave_fail_bus, sizeof(bist_slave_fail_bus));

			ili_ice_mode_write(READ_BACK_LOCK_REG, 0x01, 1);
			ili_ice_mode_read(0x046048, &pen_bist_fail_bus, sizeof(pen_bist_fail_bus));
			ili_ice_mode_read(SLAVE_READ_BACK_WORD_REG, &pen_bist_slave_fail_bus, sizeof(pen_bist_slave_fail_bus));

			if (((bist_fail & BIT(0)) != 0x00) || (bist_fail_bus != 0x00000000) || (((bist_slave_fail & BIT(24)) >> 24) != 0x00)
				|| (bist_slave_fail_bus != 0x00000000)) {

				ILI_ERR("SRAM TEST FAIL\n");
				ILI_ERR("bist_fail = 0x%X, bist_fail_bus = 0x%X\n", bist_fail, bist_fail_bus);
				ILI_ERR("bist_slave_fail = 0x%X, bist_slave_fail_bus = 0x%X\n", bist_slave_fail, bist_slave_fail_bus);
				ILI_ERR("pen_bist_fail_bus = 0x%X, pen_bist_slave_fail_bus = 0x%X\n", pen_bist_fail_bus, pen_bist_slave_fail_bus);

				ret = -1;
				goto out;
			}
		} else {
			if (((bist_fail & BIT(0)) != 0x00) || (((bist_slave_fail & BIT(24)) >> 24) != 0x00)) {
				ILI_ERR("SRAM TEST FAIL\n");
				ILI_ERR("bist_fail = 0x%X, bist_slave_fail = 0x%X\n", bist_fail, bist_slave_fail);

				ret = -1;
				goto out;
			}
		}
		
#endif
out:
	return ret;
}

int ili_tddi_ic_sram_test(void)
{
	int ret = 0;

	ili_ice_mode_write_by_mode(0x047003, 0x00, 1, BOTH);
	ili_ice_mode_write_by_mode(0x04001C, 0x01, 1, BOTH);
	ili_ice_mode_write_by_mode(0x071001, 0x02, 1, BOTH);
	ili_ice_mode_write_by_mode(0x071005, 0x3C, 1, BOTH);
	ili_ice_mode_write_by_mode(0x046003, 0x00, 1, BOTH);
	ili_ice_mode_write_by_mode(0x046000, 0x010001, 3, BOTH);
	ili_ice_mode_write_by_mode(0x046002, 0x00, 1, BOTH);

	if (ilits->sram_para.sram_group_en == ENABLE) {
		ret = ili_sram_by_partial_area_test();
		goto out;
	}

	ili_sram_test(ilits->sram_para.MaxAddr, ilits->sram_para.MinAddr);

	if (ilits->cascade_info_block.nNum != 0) {
		ret = ili_ic_cascade_polling_bist();
	} else {
		ret = ili_check_polling_bist(false, false);
	}
out:
#if (TDDI_INTERFACE == BUS_SPI)
	if (ilits->cascade_info_block.nNum != 0) {
		ili_core_spi_setup(SPI_CLK);
	}
#endif
	return ret;
}

#ifdef ROI
int ili_config_knuckle_roi_ctrl(cmd_types cmd)
{
	int ret = 0;
	switch (cmd) {
	case CMD_DISABLE:
		ILI_DBG("knuckle roi disbale\n");
		ret = ili_ic_func_ctrl("knuckle", cmd);
		break;
	case CMD_ENABLE:
		ILI_DBG("knuckle roi enable\n");
		ret = ili_ic_func_ctrl("knuckle", cmd);
		break;
	case CMD_STATUS:
		ILI_DBG("knuckle roi read status\n");
		ret = ili_ic_func_ctrl("knuckle", cmd);
		break;
	case CMD_ROI_DATA:
		ILI_DBG("knuckle roi read data\n");
		ret = ili_ic_func_ctrl("knuckle", cmd);
		break;
	default:
		ILI_ERR("knuckle roi unknown cmd: %d\n", cmd);
		return -EINVAL;
	}
	return ret;
}

int ili_config_get_knuckle_roi_status(void)
{
	int ret = 0;
	u8 roi_status = 0;

	ret = ili_config_knuckle_roi_ctrl(CMD_STATUS);
	if (ret) {
		ILI_ERR("config to read roi status failed\n");
		return ret;
	}

	msleep(1);
	ret = ilits->wrapper(NULL, 0, &roi_status, sizeof(roi_status), OFF, OFF);
	if (ret) {
		ILI_ERR("read data failed\n");
		return ret;
	}

	ILI_INFO("roi %s\n", roi_status ? "enable" : "disable");

	return roi_status;
}
#endif

static int ilitek_tddi_ic_check_info(u32 pid, u16 id)
{
	ILI_INFO("ILITEK CHIP found.\n");

	ilits->chip->pid = pid;

	ilits->chip->reset_key = TDDI_WHOLE_CHIP_RST_WITH_FLASH_KEY;
	ilits->chip->wtd_key = 0x9881;

	if (((pid & 0xFFFFFF00) == ILI9881N_AA) || ((pid & 0xFFFFFF00) == ILI9881O_AA)) {
		ilits->chip->dma_reset = ENABLE;
	} else {
		ilits->chip->dma_reset = DISABLE;
	}

	ilits->chip->no_bk_shift = RAWDATA_NO_BK_SHIFT;
	ilits->chip->max_count = 0x1FFFF;
	return 0;
}

int ili_ice_mode_bit_mask_write(u32 addr, u32 mask, u32 value)
{
	int ret = 0;
	u32 data = 0;

	if (ili_ice_mode_read(addr, &data, sizeof(u32)) < 0) {
		ILI_ERR("Read data error\n");
		return -1;
	}

	data &= (~mask);
	data |= (value & mask);

	ILI_DBG("mask value data = %x\n", data);

	ret = ili_ice_mode_write(addr, data, sizeof(u32));
	if (ret < 0)
		ILI_ERR("Failed to re-write data in ICE mode, ret = %d\n", ret);

	return ret;
}

int ili_ice_mode_write(u32 addr, u32 data, int len)
{
	int ret = 0, i;
	u8 txbuf[64] = {0};

	if (!atomic_read(&ilits->ice_stat)) {
		ILI_ERR("ice mode not enabled\n");
		return -1;
	}

	txbuf[0] = 0x25;
	txbuf[1] = (char)((addr & 0x000000FF) >> 0);
	txbuf[2] = (char)((addr & 0x0000FF00) >> 8);
	txbuf[3] = (char)((addr & 0x00FF0000) >> 16);

	for (i = 0; i < len; i++)
		txbuf[i + 4] = (char)(data >> (8 * i));

	ret = ilits->wrapper(txbuf, len + 4, NULL, 0, OFF, OFF);
	if (ret < 0)
		ILI_ERR("Failed to write data in ice mode, ret = %d\n", ret);

	return ret;
}

int ili_ice_mode_read(u32 addr, u32 *data, int len)
{
	int ret = 0;
	u8 rxbuf[4] = {0};
	u8 txbuf[4] = {0};

	if (len > sizeof(u32)) {
		ILI_ERR("ice mode read lenght = %d, must less than or equal to 4 bytes\n", len);
		len = 4;
	}

	if (!atomic_read(&ilits->ice_stat)) {
		ILI_ERR("ice mode not enabled\n");
		return -1;
	}

	txbuf[0] = 0x25;
	txbuf[1] = (char)((addr & 0x000000FF) >> 0);
	txbuf[2] = (char)((addr & 0x0000FF00) >> 8);
	txbuf[3] = (char)((addr & 0x00FF0000) >> 16);

	ret = ilits->wrapper(txbuf, sizeof(txbuf), NULL, 0, OFF, OFF);
	if (ret < 0)
		goto out;

	ret = ilits->wrapper(NULL, 0, rxbuf, len, OFF, OFF);
	if (ret < 0)
		goto out;

	*data = 0;
	if (len == 1)
		*data = rxbuf[0];
	else if (len == 2)
		*data = (rxbuf[0] | rxbuf[1] << 8);
	else if (len == 3)
		*data = (rxbuf[0] | rxbuf[1] << 8 | rxbuf[2] << 16);
	else
		*data = (rxbuf[0] | rxbuf[1] << 8 | rxbuf[2] << 16 | rxbuf[3] << 24);

out:
	if (ret < 0)
		ILI_ERR("Failed to read data in ice mode, ret = %d\n", ret);

	return ret;
}

void ili_cascade_sync_ctrl(bool mode)
{
	bool stop = false, backup_addr = MASTER;
	u8 cmd[2] = {0};


	backup_addr = ilits->i2c_addr_change;
	ilits->i2c_addr_change = MASTER;

	if (mode == stop) {
		cmd[0] = 0x10;
		cmd[1] = 0x10;
		if (ilits->wrapper(cmd, sizeof(cmd), NULL, 0, OFF, OFF) < 0) {
			ILI_ERR("Failed to stop Sync Cascade\n");
		} else {
			atomic_set(&ilits->sync_stat, ENABLE);
			ILI_DBG("Stop Sync Cascade, Set atomic sync stat = %d\n", atomic_read(&ilits->sync_stat));
		}

		mdelay(1);
	} else {
		cmd[0] = 0x10;
		cmd[1] = 0x11;
		if (ilits->wrapper(cmd, sizeof(cmd), NULL, 0, OFF, OFF) < 0) {
			ILI_ERR("Failed to start Sync Cascade\n");
		} else {
			atomic_set(&ilits->sync_stat, DISABLE);
			ILI_DBG("Start Sync Cascade, Set atomic sync stat = %d\n", atomic_read(&ilits->sync_stat));
		}
	}
	ilits->i2c_addr_change = backup_addr;
}

int ili_ice_mode_ctrl(bool enable, bool mcu)
{
	int ret = 0;
	u8 cmd_open[4] = {0x25, 0x62, 0x10, 0x18};
	u8 cmd_close[4] = {0x1B, 0x62, 0x10, 0x18};

	if (enable) {
		if (atomic_read(&ilits->ice_stat)) {
			ILI_INFO("ice mode already enabled\n");
			return 0;
		}

		/* Cascade Stop Sync */
#if (TDDI_INTERFACE == BUS_I2C && ENABLE_CASCADE)
		if (ilits->cascade_ctrl_mode == BOTH) {
			if (ilits->i2c_addr_change == SLAVE && atomic_read(&ilits->sync_stat) == DISABLE) {
				ili_cascade_sync_ctrl(false);
			}				
		} else {
			if (atomic_read(&ilits->sync_stat) == DISABLE) {
				ili_cascade_sync_ctrl(false);
			}
		}	
#elif (TDDI_INTERFACE == BUS_SPI && ENABLE_CASCADE)
		if (atomic_read(&ilits->sync_stat) == DISABLE) {
			ili_cascade_sync_ctrl(false);
		}
#endif

		if (mcu)
			cmd_open[0] = 0x1F;

		atomic_set(&ilits->ice_stat, ENABLE);

		if (ilits->wrapper(cmd_open, sizeof(cmd_open), NULL, 0, OFF, OFF) < 0) {
			ILI_ERR("write ice mode cmd error\n");
			atomic_set(&ilits->ice_stat, DISABLE);
		}
		ilits->pll_clk_wakeup = false;

#if (TDDI_INTERFACE == BUS_I2C)
		if (ilits->ice_flash_cs_ctrl && !mcu) {
		if (ili_ice_mode_write(FLASH_BASED_ADDR, 0x1, 1) < 0)
			ILI_ERR("Write cs high failed\n"); /* CS high */
			if (ili_ice_mode_write(FLASH0_dual_mode, 0x0, 1) < 0)
				ILI_ERR("Write daul mode close failed\n"); /* daul mode close */
		} else {
			ILI_INFO("ignore ice mode flash cs control\n");
		}
#endif
	} else {
		if (!atomic_read(&ilits->ice_stat)) {
			ILI_INFO("ice mode already disabled\n");
			return 0;
		}
#if (TDDI_INTERFACE == BUS_SPI && ENABLE_CASCADE)
		/* Slave exit ice mode */
		if (atomic_read(&ilits->slave_ice_stat)) {
			if (ili_mspi_write_slave_register(CONTROL_SLAVE_EXIT_ICE_ADDR, 0, 0) < 0) {
				ILI_ERR("Disable Slave ICE mode Fail\n");
			} else {
				atomic_set(&ilits->slave_ice_stat, DISABLE);
				ILI_INFO("Set slave ice state atomic : %d, Disable Slave ICE mode\n", atomic_read(&ilits->slave_ice_stat));
			}
		}
#endif
		ret = ilits->wrapper(cmd_close, sizeof(cmd_close), NULL, 0, OFF, OFF);
		if (ret < 0) {
			ILI_ERR("Exit to ICE Mode failed !!\n");
			atomic_set(&ilits->ice_stat, ENABLE);
		} else {
			atomic_set(&ilits->ice_stat, DISABLE);
#if (!ENABLE_CASCADE)
			ilits->pll_clk_wakeup = true;
#endif
			atomic_set(&ilits->ignore_report, END);
		}

		/* Cascade Start Sync */
#if (TDDI_INTERFACE == BUS_I2C && ENABLE_CASCADE)
		if (ilits->cascade_ctrl_mode == BOTH) {
			if (ilits->i2c_addr_change == MASTER && atomic_read(&ilits->sync_stat) == ENABLE) {
				ilits->pll_clk_wakeup = true;
				ili_cascade_sync_ctrl(true);
			}
		} else {
			if (atomic_read(&ilits->sync_stat) == ENABLE) {
				ilits->pll_clk_wakeup = true;
				ili_cascade_sync_ctrl(true);
			}
		}
#elif (TDDI_INTERFACE == BUS_SPI && ENABLE_CASCADE)
		if (atomic_read(&ilits->sync_stat) == ENABLE) {
			ilits->pll_clk_wakeup = true;
			ili_cascade_sync_ctrl(true);
		}
#endif
	}
	ILI_INFO("%s ICE mode, mcu on = %d\n", (enable ? "Enable" : "Disable"), mcu);

	return ret;
}

int ili_ice_mode_ctrl_by_mode(bool enable, bool mcu, int mode)
{
	int ret = 0;
#if (TDDI_INTERFACE == BUS_I2C && ENABLE_CASCADE)
	ilits->cascade_ctrl_mode = mode;
	switch (mode) {
		case SLAVE:
		case BOTH:
			ilits->i2c_addr_change = SLAVE;
			if (ili_ice_mode_ctrl(enable, mcu) < 0) {
				ILI_ERR("Failed to write Slave ice mode\n");
				ret = -1;
			} else {
				ILI_DBG("Slave %s ice mode\n", enable ? "Enable" : "Disable");
			}
			if (mode == SLAVE) {
				ilits->i2c_addr_change = MASTER;
				break;
			} else {
				if (enable == ENABLE)
					atomic_set(&ilits->ice_stat, DISABLE);
				else
					atomic_set(&ilits->ice_stat, ENABLE);
			}
		case MASTER:
			ilits->i2c_addr_change = MASTER;
			if (ili_ice_mode_ctrl(enable, mcu) < 0) {
				ILI_ERR("Failed to write Master ice mode\n");
				ret = -1;
			} else {
				ILI_DBG("Master %s ice mode\n", enable ? "Enable" : "Disable");
			}
			break;
	default:
		break;
	}
	ilits->cascade_ctrl_mode = MASTER;
#else
	if (ili_ice_mode_ctrl(enable, mcu) < 0) {
		ILI_ERR("Failed to write ice mode\n");
	}
#endif
	return ret;
}

int ili_ice_mode_write_by_mode(u32 addr, u32 data, int len, int mode)
{
	int ret = 1;
#if (TDDI_INTERFACE == BUS_I2C && ENABLE_CASCADE)
	switch (mode) {
		case SLAVE:
		case BOTH:
			ilits->i2c_addr_change = SLAVE;
			if (ili_ice_mode_write(addr, data, len) < 0) {
				ILI_ERR("Failed to write Slave, addr: 0x%X\n", addr);
				ret = -1;
			}
			if (mode == SLAVE) {
				ilits->i2c_addr_change = MASTER;
				break;
			}
		case MASTER:
			ilits->i2c_addr_change = MASTER;
			if (ili_ice_mode_write(addr, data, len) < 0) {
				ILI_ERR("Failed to write Master, addr: 0x%X\n", addr);
				ret = -1;
			}
			break;
	default:
		break;
	}
#else
	if (ili_ice_mode_write(addr, data, len) < 0) {
		ILI_ERR("Failed to write addr: 0x%X\n", addr);
		ret = -1;
	}
#endif
	return ret;
}

int ili_ice_mode_read_by_mode(u32 addr, u32 *data, int len, int mode)
{
	int ret = 1;
#if (TDDI_INTERFACE == BUS_I2C && ENABLE_CASCADE)
	switch (mode) {
		case SLAVE:
		case BOTH:
			ilits->i2c_addr_change = SLAVE;
			if (ili_ice_mode_read(addr, data, len) < 0) {
				ILI_ERR("Failed to read Slave, addr: 0x%x\n", addr);
				ret = -1;
			} else {
				ILI_DBG("Slave [READ]:addr = 0x%06x, read = 0x%08x\n", addr, *data);
			}
			if (mode == SLAVE) {
				ilits->i2c_addr_change = MASTER;
				break;
			}
		case MASTER:
			ilits->i2c_addr_change = MASTER;
			if (ili_ice_mode_read(addr, data, len) < 0) {
				ILI_ERR("Failed to read Master, addr: 0x%x\n", addr);
				ret = -1;
			} else {
				ILI_DBG("Master [READ]:addr = 0x%06x, read = 0x%08x\n", addr, *data);
			}
			break;
	default:
		break;
	}
#else
	if (ili_ice_mode_read(addr, data, len) < 0) {
		ILI_ERR("Failed to write addr: 0x%x\n", addr);
		ret = -1;
	}  else {
		ILI_DBG("[READ]:addr = 0x%06x, read = 0x%08x\n", addr, *data);
	}
#endif
	return ret;
}

static void ili_mspi_init(void)
{
	if (ili_ice_mode_write(MSPI_REG, 0x01, 1) < 0)
		ILI_ERR("Write MSPI_REG failed\n");
	
	if (ili_ice_mode_write(MSPI_TRIG_BY_DMA_ADDR, 0x101, 2) < 0)
		ILI_ERR("Write MSPI_TRIG_BY_DMA_ADDR failed\n");

	if (ili_ice_mode_write(MSPI_CLOCK_ADDR, 0x02, 1) < 0)
		ILI_ERR("Write MSPI_CLOCK_ADDR failed\n");

	if (ili_ice_mode_write(MSPI_CS_ADDR, 0x01, 1) < 0)
		ILI_ERR("Write MSPI_CS_ADDR failed\n");
}

static void ili_dma_write_setting(void)
{
	if (ili_ice_mode_write(DMA_ONE_CLEAR_ADDR, 0x02, 1) < 0)
		ILI_ERR("Write DMA_ONE_CLEAR_ADDR failed\n");
	
	if (ili_ice_mode_write(SRC_ONE_ADDR, 0x308D0, 3) < 0)
		ILI_ERR("Write SRC_ONE_ADDR failed\n");

	if (ili_ice_mode_write(SRC_ONE_SET_ADDR, 0x82000004, sizeof(u32)) < 0)
		ILI_ERR("Write SRC_ONE_SET_ADDR failed\n");

	if (ili_ice_mode_write(SRC_TWO_EN_ADDR, 0x00, 1) < 0)
		ILI_ERR("Write SRC_TWO_EN_ADDR failed\n");

	if (ili_ice_mode_write(DEST_ADDR, 0x6205C, 3) < 0)
		ILI_ERR("Write DEST_ADDR failed\n");

	if (ili_ice_mode_write(DEST_SET_ADDR, 0x82000000, sizeof(u32)) < 0)
		ILI_ERR("Write DEST_SET_ADDR failed\n");

	if (ili_ice_mode_write(TRIG_SEL_MSPI_ADDR, 0x0D, 1) < 0)
		ILI_ERR("Write TRIG_SEL_MSPI_ADDR failed\n");
	
	if (ili_ice_mode_write(DMA_ONE_GROUP_ADDR, 0x00, 1) < 0)
		ILI_ERR("Write DMA_ONE_GROUP_ADDR failed\n");
}

static void ili_dma_read_setting(void)
{
	if (ili_ice_mode_write(DMA_ONE_CLEAR_ADDR, 0x02, 1) < 0)
		ILI_ERR("Write DMA_ONE_CLEAR_ADDR failed\n");
	
	if (ili_ice_mode_write(SRC_ONE_ADDR, 0x62060, 3) < 0)
		ILI_ERR("Write SRC_ONE_ADDR failed\n");

	if (ili_ice_mode_write(SRC_ONE_SET_ADDR, 0x80000000, sizeof(u32)) < 0)
		ILI_ERR("Write SRC_ONE_SET_ADDR failed\n");

	if (ili_ice_mode_write(SRC_TWO_EN_ADDR, 0x00, 1) < 0)
		ILI_ERR("Write SRC_TWO_EN_ADDR failed\n");

	if (ili_ice_mode_write(DEST_ADDR, 0x308D0, 3) < 0)
		ILI_ERR("Write DEST_ADDR failed\n");

	if (ili_ice_mode_write(DEST_SET_ADDR, 0x80000001, sizeof(u32)) < 0)
		ILI_ERR("Write DEST_SET_ADDR failed\n");

	if (ili_ice_mode_write(TRIG_SEL_MSPI_ADDR, 0x0E, 1) < 0)
		ILI_ERR("Write DEST_SET_ADDR failed\n");

	if (ili_ice_mode_write(DMA_ONE_GROUP_ADDR, 0x00, 1) < 0)
		ILI_ERR("Write DMA_ONE_GROUP_ADDR failed\n");
}

static void ili_trigger_mspi(void)
{
	u8 BusyCnt = 50;
	u32 BusyData;

	if (ili_ice_mode_write(MSPI_CS_ADDR, 0x00, 1) < 0)
		ILI_ERR("Write MSPI_CS_ADDR failed\n");

	if (ili_ice_mode_write(MSPI_REG, 0x0F, 1) < 0)
		ILI_ERR("Write MSPI_REG failed\n");

	if (ili_ice_mode_write(MSPI_TRIGGER_ADDR, 0x01, 1) < 0)
		ILI_ERR("Write WRITE_BUF_ADDR failed\n");

	/* Polling Done Flag */
	while (BusyCnt > 0) {
		if (ili_ice_mode_read(MSPI_DONE_FLAG_ADDR, &BusyData, 1) < 0)
			ILI_ERR("Read MSPI_DONE_FLAG_ADDR failed\n");

		if ( (BusyData & BIT(0)) == 0x01) {
			break;
		} else {
			ILI_ERR("MSPI Busy : 0x%X\n", BusyData);
		}

		mdelay(1);
		BusyCnt--;
	}

	if (ili_ice_mode_write(MSPI_DONE_FLAG_ADDR, 0x01, 1) < 0)
		ILI_ERR("Read MSPI_DONE_FLAG_ADDR failed\n");

	if (ili_ice_mode_write(MSPI_CS_ADDR, 0x01, 1) < 0)
		ILI_ERR("Write MSPI_CS_ADDR failed\n");
}

void ili_mspi_set_slave_to_qual_mode(void)
{
	u32 read_data, backup_mspi_init_en;

	ili_mspi_init();

	if (ili_ice_mode_read(MSPI_INIT_EN_ADDR, &backup_mspi_init_en, 1) < 0)
		ILI_ERR("Read MSPI_INIT_EN_ADDR failed\n");

	if (ili_ice_mode_write(MSPI_INIT_EN_ADDR, backup_mspi_init_en & 0xFE, 1) < 0)
		ILI_ERR("Write MSPI_INIT_EN_ADDR failed\n");

	ili_dma_write_setting();

	/* Set Slave to Qual Mode */
	if (ili_ice_mode_read(TRANSFER_CNT_ADDR, &read_data, sizeof(u32)) < 0)
		ILI_ERR("Read TRANSFER_CNT_ADDR failed\n");

	read_data = (read_data & 0xFFFC0000) + 0x18;

	if (ili_ice_mode_write(TRANSFER_CNT_ADDR, read_data, sizeof(u32)) < 0)
		ILI_ERR("Write TRANSFER_CNT_ADDR failed\n");

	if (ili_ice_mode_read(WRITE_BUF_SIZE_ADDR, &read_data, 2) < 0)
		ILI_ERR("Read WRITE_BUF_SIZE_ADDR failed\n");

	read_data = (read_data & 0xF800) + 0x18;

	if (ili_ice_mode_write(WRITE_BUF_SIZE_ADDR, read_data, 2) < 0)
		ILI_ERR("Write WRITE_BUF_SIZE_ADDR failed\n");

	if (ili_ice_mode_read(READ_BUF_SIZE_ADDR, &read_data, 2) < 0)
		ILI_ERR("Read READ_BUF_SIZE_ADDR failed\n");

	read_data = (read_data & 0xF800) + 0x00;

	if (ili_ice_mode_write(READ_BUF_SIZE_ADDR, read_data, 2) < 0)
		ILI_ERR("Write READ_BUF_SIZE_ADDR failed\n");

	if (ili_ice_mode_write(WRITE_BUF_ADDR, 0x40000040, sizeof(u32)) < 0)
		ILI_ERR("Write WRITE_BUF_ADDR failed\n");
	
	if (ili_ice_mode_write(0x308D0, 0x4044000, sizeof(u32)) < 0)
		ILI_ERR("Write failed\n");
	
	if (ili_ice_mode_write(0x308D4, 0x4000, sizeof(u32)) < 0)
		ILI_ERR("Write failed\n");

	if (ili_ice_mode_write(0x308D8, 0x4000, sizeof(u32)) < 0)
		ILI_ERR("Write failed\n");

	if (ili_ice_mode_write(0x308DC, 0x40040000, sizeof(u32)) < 0)
		ILI_ERR("Write failed\n");

	if (ili_ice_mode_write(0x308E0, 0x40000000, sizeof(u32)) < 0)
		ILI_ERR("Write failed\n");

	ili_trigger_mspi();

	/* Recovery Mspi int en */
	if (ili_ice_mode_write(MSPI_INIT_EN_ADDR, backup_mspi_init_en, 1) < 0)
		ILI_ERR("Write MSPI_INIT_EN_ADDR failed\n");
	
	ILI_INFO("Backup MSPI Init EN : 0x%X\n", backup_mspi_init_en);
}

int ili_mspi_write_slave_register(u32 addr, u32 data, int len)
{
	int ret = 0;
	u32 backup_mspi_init_en, write_data;
	
	ili_mspi_init();

	if (ili_ice_mode_read(MSPI_INIT_EN_ADDR, &backup_mspi_init_en, 1) < 0)
		ILI_ERR("Read MSPI_INIT_EN_ADDR failed\n");
	
	if (ili_ice_mode_write(MSPI_INIT_EN_ADDR, backup_mspi_init_en & 0xFE, 1) < 0)
		ILI_ERR("Write MSPI_INIT_EN_ADDR failed\n");

	ili_dma_write_setting();

	if (ili_ice_mode_read(TRANSFER_CNT_ADDR, &write_data, sizeof(u32)) < 0)
		ILI_ERR("Read TRANSFER_CNT_ADDR failed\n");

	write_data = (write_data & 0xFFFC0000) + 5 + len;

	if (ili_ice_mode_write(TRANSFER_CNT_ADDR, write_data, sizeof(u32)) < 0)
		ILI_ERR("Write TRANSFER_CNT_ADDR failed\n");

	if (ili_ice_mode_read(WRITE_BUF_SIZE_ADDR, &write_data, 2) < 0)
		ILI_ERR("Read WRITE_BUF_SIZE_ADDR failed\n");

	write_data = (write_data & 0xF800) + 5 + len;

	if (ili_ice_mode_write(WRITE_BUF_SIZE_ADDR, write_data, 2) < 0)
		ILI_ERR("Write WRITE_BUF_SIZE_ADDR failed\n");

	if (ili_ice_mode_read(READ_BUF_SIZE_ADDR, &write_data, 2) < 0)
		ILI_ERR("Read READ_BUF_SIZE_ADDR failed\n");

	write_data = write_data & 0xF800;

	if (ili_ice_mode_write(READ_BUF_SIZE_ADDR, write_data, 2) < 0)
		ILI_ERR("Write READ_BUF_SIZE_ADDR failed\n");

	if (addr == CONTROL_SLAVE_EXIT_ICE_ADDR) {
		write_data = ((addr & 0xFF00) << 16) + ((addr & 0xFF) << 16) + (0x1B << 8) + 0x82;
	} else {
		write_data = ((addr & 0xFF00) << 16) + ((addr & 0xFF) << 16) + (0x25 << 8) + 0x82;
	}

	if (ili_ice_mode_write(WRITE_BUF_ADDR, write_data, sizeof(u32)) < 0)
		ILI_ERR("Write WRITE_BUF_ADDR failed\n");
	
	write_data = ((data & 0xFF0000) << 8) + ((data & 0xFF00) << 8) + ((data & 0xFF) << 8) + ((addr & 0xFF0000) >> 16);

	if (ili_ice_mode_write(0x308D0, write_data, sizeof(u32)) < 0) {
		ILI_ERR("Write failed\n");
		ret = -1;
	}
	
	write_data = (data & 0xFF000000) >> 24;

	if (ili_ice_mode_write(0x308D4, write_data, sizeof(u32)) < 0) {
		ILI_ERR("Write failed\n");
		ret = -1;
	}

	ili_trigger_mspi();

	/* Recovery Mspi int en */
	if (ili_ice_mode_write(MSPI_INIT_EN_ADDR, backup_mspi_init_en, 1) < 0)
		ILI_ERR("Write MSPI_INIT_EN_ADDR failed\n");

	return ret;
}

void ili_mspi_read_slave_register(u32 addr, u32 *data, int len)
{
	u32 read_data, backup_mspi_init_en;

	ili_mspi_write_slave_register(addr, 0, 0);
	
	if (ili_ice_mode_read(MSPI_INIT_EN_ADDR, &backup_mspi_init_en, 1) < 0)
		ILI_ERR("Read MSPI_INIT_EN_ADDR failed\n");

	
	if (ili_ice_mode_write(MSPI_INIT_EN_ADDR, backup_mspi_init_en & 0xFE, 1) < 0)
		ILI_ERR("Write MSPI_INIT_EN_ADDR failed\n");

	ili_dma_read_setting();

	if (ili_ice_mode_write(WRITE_BUF_ADDR, 0x83, 1) < 0)
		ILI_ERR("Write WRITE_BUF_ADDR failed\n");

	if (ili_ice_mode_read(TRANSFER_CNT_ADDR, &read_data, sizeof(u32)) < 0)
		ILI_ERR("Read TRANSFER_CNT_ADDR failed\n");

	read_data = (read_data & 0xFFFC0000) + len;

	if (ili_ice_mode_write(TRANSFER_CNT_ADDR, read_data, sizeof(u32)) < 0)
		ILI_ERR("Write TRANSFER_CNT_ADDR failed\n");

	if (ili_ice_mode_read(WRITE_BUF_SIZE_ADDR, &read_data, 2) < 0)
		ILI_ERR("Read WRITE_BUF_SIZE_ADDR failed\n");

	read_data = (read_data & 0xF800) + 0x01;

	if (ili_ice_mode_write(WRITE_BUF_SIZE_ADDR, read_data, 2) < 0)
		ILI_ERR("Write WRITE_BUF_SIZE_ADDR failed\n");

	if (ili_ice_mode_read(READ_BUF_SIZE_ADDR, &read_data, 2) < 0)
		ILI_ERR("Read READ_BUF_SIZE_ADDR failed\n");

	read_data = (read_data & 0xF800) + len;

	if (ili_ice_mode_write(READ_BUF_SIZE_ADDR, read_data, 2) < 0)
		ILI_ERR("Write READ_BUF_SIZE_ADDR failed\n");

	ili_trigger_mspi();

	/* Recovery Mspi int en */
	if (ili_ice_mode_write(MSPI_INIT_EN_ADDR, backup_mspi_init_en, 1) < 0)
		ILI_ERR("Write MSPI_INIT_EN_ADDR failed\n");

	if (ili_ice_mode_read(0x308D0, data, len) < 0)
		ILI_ERR("Read 0x308D0 failed\n");
}

int ili_set_bypass_mode(bool mcu)
{
#if (TDDI_INTERFACE == BUS_SPI)
	if (atomic_read(&ilits->ice_stat)) {
		ILI_INFO("ice mode already enabled\n");
		return 0;
	}

	ili_ice_mode_ctrl_by_mode(ENABLE, mcu, MASTER);

	/* set mspi 0 */
	if (ili_ice_mode_write_by_mode(MSPI_REG, 0x00, 1, MASTER) < 0) {
		ILI_ERR("Failed to write MSPI_REG in ice mode\n");
	}

	/* set s_to_q bit[1] = 1 */
	ili_ice_mode_write(SINGLE_TO_QUAL_REG, 0x02, 1);

	/* set slave ic to ice mode */
	atomic_set(&ilits->ice_stat, DISABLE);

	atomic_set(&ilits->slave_ice_stat, ENABLE);
	ILI_INFO("Set slave ice state atomic : %d\n", atomic_read(&ilits->slave_ice_stat));

	ili_ice_mode_ctrl_by_mode(ENABLE, mcu, SLAVE);

	ILI_INFO("Set ByPass Mode\n");
#endif
	return 0;
}

void ili_wdt_reset_status( bool enter_ice, bool exit_ice)
{
#if (TDDI_INTERFACE == BUS_I2C)
	if (enter_ice) {
		if (atomic_read(&ilits->ice_stat)) {
			ILI_INFO("ice mode already enabled\n");
		} else {
			ili_ice_mode_ctrl_by_mode(ENABLE, OFF, BOTH);
		}	
	}	
#else
	if (enter_ice) {
		if (atomic_read(&ilits->ice_stat)) {
			ILI_INFO("ice mode already enabled\n");
		} else {
			ili_set_bypass_mode(OFF);
		}
	}
#endif	
	if (ili_ice_mode_write_by_mode(WDT_ADDR, 0x01, 1, BOTH) > 0) {
		ILI_INFO("Clear Master & Slave WDT Status\n");
	}	

	if (exit_ice) {
		if (!atomic_read(&ilits->ice_stat)) {
			ILI_INFO("ice mode already disabled\n");
		} else {
			ili_ice_mode_ctrl_by_mode(DISABLE, OFF, BOTH);
		}
	}
}

void ili_ic_edge_palm_para_init(void)
{
#if ENABLE_EDGE_PALM_PARA
	int i;

	ilits->edge_palm_para[0] = P5_X_EDGE_PLAM_CTRL_1;
	ilits->edge_palm_para[1] = P5_X_EDGE_PALM_TUNING_PARA;
	ilits->edge_palm_para[2] = (ZONE_A_W & 0xFF00) >> 8;
	ilits->edge_palm_para[3] = ZONE_A_W & 0xFF;
	ilits->edge_palm_para[4] = (ZONE_B_W & 0xFF00) >> 8;
	ilits->edge_palm_para[5] = ZONE_B_W & 0xFF;
	ilits->edge_palm_para[6] = (ZONE_A_ROTATION_W & 0xFF00) >> 8;
	ilits->edge_palm_para[7] = ZONE_A_ROTATION_W & 0xFF;
	ilits->edge_palm_para[8] = (ZONE_B_ROTATION_W & 0xFF00) >> 8;
	ilits->edge_palm_para[9] = ZONE_B_ROTATION_W & 0xFF;
	ilits->edge_palm_para[10] = (ZONE_C_WIDTH & 0xFF00) >> 8;
	ilits->edge_palm_para[11] = ZONE_C_WIDTH & 0xFF;
	ilits->edge_palm_para[12] = (ZONE_C_HEIGHT & 0xFF00) >> 8;
	ilits->edge_palm_para[13] = ZONE_C_HEIGHT & 0xFF;
	ilits->edge_palm_para[14] = (ZONE_C_HEIGHT_LITTLE & 0xFF00) >> 8;
	ilits->edge_palm_para[15] = ZONE_C_HEIGHT_LITTLE & 0xFF;
	ilits->edge_palm_para[16] = (ZONE_A_W & 0xFF00) >> 8;
	ilits->edge_palm_para[17] = ZONE_A_W & 0xFF;
	ilits->edge_palm_para[18] = (ZONE_B_W & 0xFF00) >> 8;
	ilits->edge_palm_para[19] = ZONE_B_W & 0xFF;
	ilits->edge_palm_para[20] = (ZONE_A_ROTATION_W & 0xFF00) >> 8;
	ilits->edge_palm_para[21] = ZONE_A_ROTATION_W & 0xFF;
	ilits->edge_palm_para[22] = (ZONE_B_ROTATION_W & 0xFF00) >> 8;
	ilits->edge_palm_para[23] = ZONE_B_ROTATION_W & 0xFF;
	ilits->edge_palm_para[24] = (ZONE_C_WIDTH & 0xFF00) >> 8;
	ilits->edge_palm_para[25] = ZONE_C_WIDTH & 0xFF;
	ilits->edge_palm_para[26] = (ZONE_C_HEIGHT & 0xFF00) >> 8;
	ilits->edge_palm_para[27] = ZONE_C_HEIGHT & 0xFF;
	ilits->edge_palm_para[28] = (ZONE_C_HEIGHT_LITTLE & 0xFF00) >> 8;
	ilits->edge_palm_para[29] = ZONE_C_HEIGHT_LITTLE & 0xFF;
	ilits->edge_palm_para[30] = ili_calc_packet_checksum(ilits->edge_palm_para, P5_X_EDGE_PALM_PARA_LENGTH - 1);

	for (i = 0; i < P5_X_EDGE_PALM_PARA_LENGTH; i++) {
		ILI_DBG("edge_palm_para[%d] = 0x%2x\n", i, ilits->edge_palm_para[i]);
	}
#endif
}
void ili_ic_send_edge_palm_para(void)
{
#if ENABLE_EDGE_PALM_PARA
	int ret = 0;

	ILI_INFO("send edge palm para\n");

	ret = ilits->wrapper(ilits->edge_palm_para, P5_X_EDGE_PALM_PARA_LENGTH, NULL, 0, OFF, OFF);
	if (ret < 0)
		ILI_ERR("Write edge palm para function failed\n");
#endif
}

int ili_ic_func_ctrl_export(const char *name, int ctrl)
{
	int i = 0, ret;
	if (ERR_ALLOC_MEM(ilits)) {
		ILI_ERR("Failed to allocate ts memory, ilits is NULL.\n");
		return -ENOMEM;
	}

	mutex_lock(&ilits->touch_mutex);
	for (i = 0; i < FUNC_CTRL_NUM; i++) {
		if (ipio_strcmp(name, func_ctrl[i].name) == 0) {
			if (strlen(name) != strlen(func_ctrl[i].name))
				continue;
			break;
		}
	}

	if (i >= FUNC_CTRL_NUM) {
		ILI_ERR("Not found function ctrl, %s\n", name);
		ret = -1;
		goto out;
	}

	if (ilits->protocol->ver == PROTOCOL_VER_500) {
		ILI_ERR("Non support function ctrl with protocol v5.0\n");
		ret = -1;
		goto out;
	}

	if (ilits->protocol->ver >= PROTOCOL_VER_560) {
		if (ipio_strcmp(func_ctrl[i].name, "gesture") == 0 ||
			ipio_strcmp(func_ctrl[i].name, "phone_cover_window") == 0) {
			ILI_INFO("Non support %s function ctrl\n", func_ctrl[i].name);
			ret = -1;
			goto out;
		}
	}

	func_ctrl[i].cmd[2] = ctrl;

	ILI_INFO("func = %s, len = %d, cmd = 0x%x, 0%x, 0x%x\n", func_ctrl[i].name, func_ctrl[i].len,
		func_ctrl[i].cmd[0], func_ctrl[i].cmd[1], func_ctrl[i].cmd[2]);

	ret = ilits->wrapper(func_ctrl[i].cmd, func_ctrl[i].len, NULL, 0, OFF, OFF);
	if (ret < 0)
		ILI_ERR("Write TP function failed\n");

	if (func_ctrl[i].rec_state < 2) {
		if (ctrl == func_ctrl[i].def_cmd)
			func_ctrl[i].rec_state = DISABLE;
		else
			func_ctrl[i].rec_state = ENABLE;

		func_ctrl[i].rec_cmd = ctrl;
	}

	ILI_DBG("record %s func cmd %d, rec_state %d\n", func_ctrl[i].name, func_ctrl[i].rec_cmd, func_ctrl[i].rec_state);
out:
	mutex_unlock(&ilits->touch_mutex);
	return ret;
}
EXPORT_SYMBOL(ili_ic_func_ctrl_export);


int ili_ic_func_ctrl(const char *name, int ctrl)
{
	int i = 0, ret;

	for (i = 0; i < FUNC_CTRL_NUM; i++) {
		if (ipio_strcmp(name, func_ctrl[i].name) == 0) {
			if (strlen(name) != strlen(func_ctrl[i].name))
				continue;
			break;
		}
	}

	if (i >= FUNC_CTRL_NUM) {
		ILI_ERR("Not found function ctrl, %s\n", name);
		ret = -1;
		goto out;
	}

	if (ilits->protocol->ver == PROTOCOL_VER_500) {
		ILI_ERR("Non support function ctrl with protocol v5.0\n");
		ret = -1;
		goto out;
	}

	if (ilits->protocol->ver >= PROTOCOL_VER_560) {
		if (ipio_strcmp(func_ctrl[i].name, "gesture") == 0 ||
			ipio_strcmp(func_ctrl[i].name, "phone_cover_window") == 0) {
			ILI_INFO("Non support %s function ctrl\n", func_ctrl[i].name);
			ret = -1;
			goto out;
		}
	}

	if (ilits->chip->core_ver >= CORE_VER_1700) {
		if (ipio_strcmp(func_ctrl[i].name, "sense") == 0) {
			ILI_INFO("Non support %s function ctrl after core ver %x\n", func_ctrl[i].name, ilits->chip->core_ver);
			ret = -1;
			goto out;
		}
	}

	func_ctrl[i].cmd[2] = ctrl;

	ILI_INFO("func = %s, len = %d, cmd = 0x%x, 0x%x, 0x%x\n", func_ctrl[i].name, func_ctrl[i].len,
		func_ctrl[i].cmd[0], func_ctrl[i].cmd[1], func_ctrl[i].cmd[2]);

	ret = ilits->wrapper(func_ctrl[i].cmd, func_ctrl[i].len, NULL, 0, OFF, OFF);
	if (ret < 0)
		ILI_ERR("Write TP function failed\n");

	if (func_ctrl[i].rec_state < 2) {
		if (ctrl == func_ctrl[i].def_cmd)
			func_ctrl[i].rec_state = DISABLE;
		else
			func_ctrl[i].rec_state = ENABLE;

		func_ctrl[i].rec_cmd = ctrl;
	}

	ILI_DBG("record %s func cmd %d, rec_state %d\n", func_ctrl[i].name, func_ctrl[i].rec_cmd, func_ctrl[i].rec_state);
out:
	return ret;
}

void ili_ic_func_ctrl_reset(void)
{
	int i = 0;

	ili_ic_send_edge_palm_para();

	for (i = 0; i < FUNC_CTRL_NUM; i++) {
		if (func_ctrl[i].rec_state == ENABLE) {
			ILI_DBG("reset func ctrl %s, record status = %d, cmd = %d\n", func_ctrl[i].name,
				func_ctrl[i].rec_state, func_ctrl[i].rec_cmd);
			if (ili_ic_func_ctrl(func_ctrl[i].name, func_ctrl[i].rec_cmd) < 0)
				ILI_ERR("reset ic func ctrl %s failed\n", func_ctrl[i].name);
		}
	}
}

int ili_ic_code_reset(bool mcu)
{
	int ret;
	bool ice = atomic_read(&ilits->ice_stat);

	if (!ice) {
		if (ilits->cascade_info_block.nNum != 0) {
#if (TDDI_INTERFACE == BUS_I2C)
			ili_ice_mode_ctrl_by_mode(ENABLE, mcu, BOTH);
#else
			ili_set_bypass_mode(mcu);
#endif
		} else {
			if (ili_ice_mode_ctrl(ENABLE, mcu) < 0)
				ILI_ERR("Enable ice mode failed before code reset\n");
		}
	}
		
	ret = ili_ice_mode_write_by_mode(0x40040, 0xAE, 1, BOTH);
	if (ret < 0)
		ILI_ERR("ic code reset failed\n");

	if (!ice) {
		if (ilits->cascade_info_block.nNum != 0) {
#if (TDDI_INTERFACE == BUS_I2C)
			ili_ice_mode_ctrl_by_mode(DISABLE, mcu, BOTH);
#else
			ili_ice_mode_ctrl_by_mode(DISABLE, mcu, BOTH);
#endif
		} else {
			if (ili_ice_mode_ctrl(DISABLE, mcu) < 0)
				ILI_ERR("Enable ice mode failed after code reset\n");
		}
	}
	return ret;
}

void ili_get_dma1_config(struct ilitek_dma_config *dma)
{
	/* dma1 src1 address */
	if (ili_ice_mode_read(0x072104, &dma->src_addr, 4) < 0)
		ILI_ERR("read dma1 src1 address failed\n");
	/* dma1 src1 format */
	if (ili_ice_mode_read(0x072108, &dma->src_fmt, 4) < 0)
		ILI_ERR("read dma1 src1 format failed\n");
	/* dma1 dest address */
	if (ili_ice_mode_read(0x072114, &dma->dest_addr, 4) < 0)
		ILI_ERR("read dma1 src1 format failed\n");
	/* dma1 dest format */
	if (ili_ice_mode_read(0x072118, &dma->dest_fmt, 4) < 0)
		ILI_ERR("read dma1 dest format failed\n");
	/* Block size */
	if (ili_ice_mode_read(0x07211C, &dma->block_size, 4) < 0)
		ILI_ERR("read block size (%d) failed\n", dma->block_size);
	ILI_DBG("dma.src_addr=0x%x, dma.src_fmt=0x%x, dma.dest_addr=0x%x, dma.dest_fmt=0x%x, dma.block_size=0x%x\n"
		, dma->src_addr, dma->src_fmt, dma->dest_addr, dma->dest_fmt, dma->block_size);
}

void ili_set_dma1_config(struct ilitek_dma_config *dma)
{
	ILI_DBG("dma.src_addr=0x%x, dma.src_fmt=0x%x, dma.dest_addr=0x%x, dma.dest_fmt=0x%x, dma.block_size=0x%x\n"
		, dma->src_addr, dma->src_fmt, dma->dest_addr, dma->dest_fmt, dma->block_size);
	/* dma1 src1 address */
	if (ili_ice_mode_write(0x072104, dma->src_addr, 4) < 0)
		ILI_ERR("Write dma1 src1 address failed\n");
	/* dma1 src1 format */
	if (ili_ice_mode_write(0x072108, dma->src_fmt, 4) < 0)
		ILI_ERR("Write dma1 src1 format failed\n");
	/* dma1 dest address */
	if (ili_ice_mode_write(0x072114, dma->dest_addr, 4) < 0)
		ILI_ERR("Write dma1 src1 format failed\n");
	/* dma1 dest format */
	if (ili_ice_mode_write(0x072118, dma->dest_fmt, 4) < 0)
		ILI_ERR("Write dma1 dest format failed\n");
	/* Block size*/
	if (ili_ice_mode_write(0x07211C, dma->block_size, 4) < 0)
		ILI_ERR("Write block size (%d) failed\n", dma->block_size);
	/* Disable CRC calc settings */
	if (ili_ice_mode_write(0x041014, 0x0, 4) < 0)
		ILI_ERR("Write dma CRC calc settings failed\n");
	/* Dma1 stop */
	if (ili_ice_mode_write(0x072100, 0x02040000, 4) < 0)
		ILI_ERR("Write dma1 stop failed\n");
	/* clr int */
	if (ili_ice_mode_write(0x048006, 0x2, 1) < 0)
		ILI_ERR("Write clr int failed\n");
	/* Dma1 start */
	if (ili_ice_mode_write(0x072100, 0x01040000, 4) < 0)
		ILI_ERR("Write dma1 start failed\n");
}

int ili_ic_whole_reset(bool mcu, bool withflash)
{
	int ret = 0;
	bool ice = atomic_read(&ilits->ice_stat);

	if (withflash)
		ilits->chip->reset_key = TDDI_WHOLE_CHIP_RST_WITH_FLASH_KEY;
	else
		ilits->chip->reset_key = TDDI_WHOLE_CHIP_RST_WITHOUT_FLASH_KEY;

	if (!ice) {
#if (TDDI_INTERFACE == BUS_I2C && ENABLE_CASCADE)
		ili_ice_mode_ctrl_by_mode(ENABLE, mcu, BOTH);
#elif (TDDI_INTERFACE == BUS_SPI && ENABLE_CASCADE)
		ili_set_bypass_mode(mcu);
#else
		if (ili_ice_mode_ctrl(ENABLE, mcu) < 0)
			ILI_ERR("Enable ice mode failed before chip reset\n");
#endif
	}

	if (!withflash)	
		ili_ice_mode_write_by_mode(ilits->chip->reset_addr, TDDI_WHOLE_CHIP_RST_WITHOUT_FLASH_PRE_KEY, sizeof(u32), BOTH);

	ILI_INFO("ic whole reset key = 0x%x, edge_delay = %d, withflash = %d\n",
		ilits->chip->reset_key, ilits->rst_edge_delay, withflash);

	ret = ili_ice_mode_write_by_mode(ilits->chip->reset_addr, ilits->chip->reset_key, sizeof(u32), BOTH);
	if (ret < 0) {
		ILI_ERR("ic whole reset failed\n");
		goto out;
	}

	/* Need accurate power sequence, do not change it to msleep */
	mdelay(ilits->rst_edge_delay);

out:
	if (!ice) {
#if (TDDI_INTERFACE == BUS_I2C && ENABLE_CASCADE)
		ili_ice_mode_ctrl_by_mode(DISABLE, mcu, BOTH);
#elif (TDDI_INTERFACE == BUS_SPI && ENABLE_CASCADE)
		ili_ice_mode_ctrl_by_mode(DISABLE, mcu, BOTH);
#else
		if (ili_ice_mode_ctrl(DISABLE, mcu) < 0)
			ILI_ERR("Enable ice mode failed after chip reset\n");
#endif
	}

	return ret;
}

static void ilitek_tddi_ic_cascade_wr_pack(int packet)
{
	int ret = 0, retry = 100;
	u32 reg_data = 0;

	while (retry--) {	
#if (TDDI_INTERFACE == BUS_I2C)
			if (ili_ice_mode_read_by_mode(0x73010, &reg_data, sizeof(u8), ilits->slave_wr_ctrl) < 0)
				ILI_ERR("Read 0x73010 error\n");
#else
			if (ilits->slave_wr_ctrl) {
				ili_ice_mode_write(READ_BACK_LOCK_REG, 0x01, 1);
				ili_ice_mode_read(0x73010, &reg_data, sizeof(reg_data));
				ILI_DBG("check ok 0x73010 read Master 0x%X\n", reg_data);
				ili_ice_mode_read(SLAVE_READ_BACK_WORD_REG, &reg_data, sizeof(reg_data));
				ILI_DBG("check ok 0x62030 read Slave 0x%X\n", reg_data);
				reg_data = (reg_data >> 24) & 0xFF;
			} else {
				if (ili_ice_mode_read(0x73010, &reg_data, sizeof(u8)) < 0)
					ILI_ERR("Read 0x73010 error\n");
			}
#endif
		if ((reg_data & 0x02) == 0) {
			ILI_INFO("check ok 0x73010 read 0x%X retry = %d\n", reg_data, retry);
			break;
		}

		mdelay(10);
	}

	if (retry <= 0)
		ILI_INFO("check 0x73010 error read 0x%X\n", reg_data);

#if (TDDI_INTERFACE == BUS_I2C)
	ret = ili_ice_mode_write_by_mode(0x73000, packet, 4, ilits->slave_wr_ctrl);
#else
	if (ilits->slave_wr_ctrl) {
		ret = ili_mspi_write_slave_register(0x73000, packet, 4);

		/* set mspi 0 */
		if (ili_ice_mode_write_by_mode(MSPI_REG, 0x00, 1, MASTER) < 0)
			ILI_ERR("Failed to write MSPI_REG in ice mode\n");
	} else {
		ret = ili_ice_mode_write(0x73000, packet, 4);
	}
#endif
	if (ret < 0) {
		ILI_ERR("Write %x at 0x73000 Fail\n", packet);
	}
}

static u32 ilitek_tddi_ic_cascade_rd_pack(int packet)
{
	int ret = 0, retry = 100;
	u32 reg_data = 0;

	ilitek_tddi_ic_cascade_wr_pack(packet);

	while (retry--) {	
#if (TDDI_INTERFACE == BUS_I2C)
			if (ili_ice_mode_read_by_mode(0x4800A, &reg_data, sizeof(u8), ilits->slave_wr_ctrl) < 0)
				ILI_ERR("Read 0x4800A error\n");
#else
			if (ilits->slave_wr_ctrl) {
				ili_ice_mode_write(READ_BACK_LOCK_REG, 0x01, 1);
				ili_ice_mode_read(0x4800A, &reg_data, sizeof(reg_data));
				ILI_DBG("check ok 0x4800A read Master 0x%X\n", reg_data);
				ili_ice_mode_read(SLAVE_READ_BACK_WORD_REG, &reg_data, sizeof(reg_data));
				ILI_DBG("check ok 0x4800A read Slave 0x%X\n", reg_data);
				reg_data = (reg_data >> 24) & 0xFF;
			} else {
				if (ili_ice_mode_read(0x4800A, &reg_data, sizeof(u8)) < 0)
					ILI_ERR("Read 0x4800A error\n");
			}
#endif

		if ((reg_data & 0x02) == 0x02) {
			ILI_INFO("check  ok 0x4800A read 0x%X retry = %d\n", reg_data, retry);
			break;
		}
		mdelay(10);
	}

	if (retry <= 0)
		ILI_INFO("check 0x4800A error read 0x%X\n", reg_data);

#if (TDDI_INTERFACE == BUS_I2C)
	ret = ili_ice_mode_write_by_mode(0x4800A, 0x02, 1, ilits->slave_wr_ctrl);
#else
	if (ilits->slave_wr_ctrl) {
		ret = ili_mspi_write_slave_register(0x4800A, 0x02, 1);

		/* set mspi 0 */
		if (ili_ice_mode_write_by_mode(MSPI_REG, 0x00, 1, MASTER) < 0)
			ILI_ERR("Failed to write MSPI_REG in ice mode\n");
	} else {
		ret = ili_ice_mode_write(0x4800A, 0x02, 1);
	}
#endif
	if (ret < 0) {
		ILI_ERR("Write 0x2 at 0x4800A Fail\n");
	}

#if (TDDI_INTERFACE == BUS_I2C)
		if (ili_ice_mode_read_by_mode(0x73016, &reg_data, sizeof(u8), ilits->slave_wr_ctrl) < 0)
			ILI_ERR("Read 0x73016 error\n");
#else
		if (ilits->slave_wr_ctrl) {
			ili_ice_mode_write(READ_BACK_LOCK_REG, 0x01, 1);
			ili_ice_mode_read(0x73016, &reg_data, sizeof(reg_data));
			ILI_DBG("check ok 0x73016 read Master 0x%X\n", reg_data);
			ili_ice_mode_read(SLAVE_READ_BACK_WORD_REG, &reg_data, sizeof(reg_data));
			ILI_DBG("check ok 0x73016 read Slave 0x%X\n", reg_data);
			reg_data = (reg_data >> 24) & 0xFF;
		} else {
			if (ili_ice_mode_read(0x73016, &reg_data, sizeof(u8)) < 0)
				ILI_ERR("Read 0x73016 error\n");
		}
#endif

	return reg_data;
}

void ili_ic_set_cascade_ddi_reg_onepage(u8 page, u8 reg, u8 data, bool mcu)
{

	u32 setpage = 0x1FFFFF00 | page;
	u32 setreg = 0x1F000100 | (reg << 16) | data;
	bool ice = atomic_read(&ilits->ice_stat);
	u8 spiclk_num = 3;

	ILI_INFO("setpage =  0x%X setreg = 0x%X\n", setpage, setreg);

	if (ilits->slave_wr_ctrl) {
		ili_core_spi_setup(spiclk_num);
	}

	if (!ice) {
#if (TDDI_INTERFACE == BUS_I2C)
		ili_ice_mode_ctrl_by_mode(ENABLE, mcu, ilits->slave_wr_ctrl);
#else
		if (ilits->slave_wr_ctrl) {
			ili_set_bypass_mode(mcu);
		} else {
			ili_ice_mode_ctrl(ENABLE, mcu);
		}
#endif
	}

	/*TDI_WR_KEY*/
	ilitek_tddi_ic_cascade_wr_pack(0x1FFF9527);
	/*Switch to Page*/
	ilitek_tddi_ic_cascade_wr_pack(setpage);
	/* Page*/
	ilitek_tddi_ic_cascade_wr_pack(setreg);
	/*TDI_WR_KEY OFF*/
	ilitek_tddi_ic_cascade_wr_pack(0x1FFF9500);

	if (!ice) {
#if (TDDI_INTERFACE == BUS_I2C)
		ili_ice_mode_ctrl_by_mode(DISABLE, mcu, ilits->slave_wr_ctrl);
#else
		ili_ice_mode_ctrl_by_mode(DISABLE, mcu, ilits->slave_wr_ctrl);
#endif
	}

	if (ilits->slave_wr_ctrl) {
		ili_core_spi_setup(SPI_CLK);
	}
}

void ili_ic_get_cascade_ddi_reg_onepage(u8 page, u8 reg, u8 *data, bool mcu)
{
	int ret = 0;
	u32 setpage = 0x1FFFFF00 | page;
	u32 setreg = 0x2F000100 | (reg << 16);
	bool ice = atomic_read(&ilits->ice_stat);
	u8 spiclk_num = 3;

	ILI_INFO("setpage = 0x%X setreg = 0x%X\n", setpage, setreg);

	if (ilits->slave_wr_ctrl) {
		ili_core_spi_setup(spiclk_num);
	}

	if (!ice) {
#if (TDDI_INTERFACE == BUS_I2C)
		if (ili_ice_mode_ctrl_by_mode(ENABLE, mcu, ilits->slave_wr_ctrl) < 0)
			ILI_ERR("Enable ice mode failed before reading ddi reg\n");
#else
		if (ilits->slave_wr_ctrl) {
			ili_set_bypass_mode(mcu);
		} else {
			if (ili_ice_mode_ctrl(ENABLE, mcu) < 0)
			ILI_ERR("Enable ice mode failed before reading ddi reg\n");
		}
#endif
	}

	/*TDI_WR_KEY*/
	ilitek_tddi_ic_cascade_wr_pack(0x1FFF9527);
	/*Set Read Page reg*/
	ilitek_tddi_ic_cascade_wr_pack(setpage);

	/*TDI_RD_KEY*/
	ilitek_tddi_ic_cascade_wr_pack(0x1FFF9487);

	/*( *( __IO uint8 *)	(0x4800A) ) =0x2*/
#if (TDDI_INTERFACE == BUS_I2C)
	ret = ili_ice_mode_write_by_mode(0x4800A, 0x02, 1, ilits->slave_wr_ctrl);
#else
	if (ilits->slave_wr_ctrl) {
		ret = ili_mspi_write_slave_register(0x4800A, 0x02, 1);

		/* set mspi 0 */
		if (ili_ice_mode_write_by_mode(MSPI_REG, 0x00, 1, MASTER) < 0)
			ILI_ERR("Failed to write MSPI_REG in ice mode\n");
	} else {
		ret = ili_ice_mode_write(0x4800A, 0x02, 1);
	}
#endif
	if (ret < 0) {
		ILI_ERR("Write 0x2 at 0x4800A Fail\n");
	}

	*data = ilitek_tddi_ic_cascade_rd_pack(setreg);
	ILI_INFO("check page = 0x%X, reg = 0x%X, read 0x%X\n", page, reg, *data);

	/*TDI_RD_KEY OFF*/
	ilitek_tddi_ic_cascade_wr_pack(0x1FFF9400);
	/*TDI_WR_KEY OFF*/
	ilitek_tddi_ic_cascade_wr_pack(0x1FFF9500);

	if (!ice) {
#if (TDDI_INTERFACE == BUS_I2C)
		ili_ice_mode_ctrl_by_mode(DISABLE, mcu, ilits->slave_wr_ctrl);
#else
		ili_ice_mode_ctrl_by_mode(DISABLE, mcu, ilits->slave_wr_ctrl);
#endif
	}

	if (ilits->slave_wr_ctrl) {
		ili_core_spi_setup(SPI_CLK);
	}
}

static void ilitek_tddi_ic_wr_pack(int packet)
{
	int retry = 100;
	u32 reg_data = 0;

	while (retry--) {
		if (ili_ice_mode_read(0x73010, &reg_data, sizeof(u8)) < 0)
			ILI_ERR("Read 0x73010 error\n");

		if ((reg_data & 0x02) == 0) {
			ILI_INFO("check ok 0x73010 read 0x%X retry = %d\n", reg_data, retry);
			break;
		}
		mdelay(10);
	}

	if (retry <= 0)
		ILI_INFO("check 0x73010 error read 0x%X\n", reg_data);

	if (ili_ice_mode_write(0x73000, packet, 4) < 0)
		ILI_ERR("Write %x at 0x73000\n", packet);
}

static u32 ilitek_tddi_ic_rd_pack(int packet)
{
	int retry = 100;
	u32 reg_data = 0;

	ilitek_tddi_ic_wr_pack(packet);

	while (retry--) {
		if (ili_ice_mode_read(0x4800A, &reg_data, sizeof(u8)) < 0)
			ILI_ERR("Read 0x4800A error\n");

		if ((reg_data & 0x02) == 0x02) {
			ILI_INFO("check  ok 0x4800A read 0x%X retry = %d\n", reg_data, retry);
			break;
		}
		mdelay(10);
	}
	if (retry <= 0)
		ILI_INFO("check 0x4800A error read 0x%X\n", reg_data);

	if (ili_ice_mode_write(0x4800A, 0x02, 1) < 0)
		ILI_ERR("Write 0x2 at 0x4800A\n");

	if (ili_ice_mode_read(0x73016, &reg_data, sizeof(u8)) < 0)
		ILI_ERR("Read 0x73016 error\n");

	return reg_data;
}

void ili_ic_set_ddi_reg_onepage(u8 page, u8 reg, u8 data, bool mcu)
{
	u32 setpage = 0x1FFFFF00 | page;
	u32 setreg = 0x1F000100 | (reg << 16) | data;
	bool ice = atomic_read(&ilits->ice_stat);

	ILI_INFO("setpage =  0x%X setreg = 0x%X\n", setpage, setreg);

	if (!ice)
		if (ili_ice_mode_ctrl(ENABLE, mcu) < 0)
			ILI_ERR("Enable ice mode failed before writing ddi reg\n");

	/*TDI_WR_KEY*/
	ilitek_tddi_ic_wr_pack(0x1FFF9527);
	/*Switch to Page*/
	ilitek_tddi_ic_wr_pack(setpage);
	/* Page*/
	ilitek_tddi_ic_wr_pack(setreg);
	/*TDI_WR_KEY OFF*/
	ilitek_tddi_ic_wr_pack(0x1FFF9500);

	if (!ice)
		if (ili_ice_mode_ctrl(DISABLE, mcu) < 0)
			ILI_ERR("Disable ice mode failed after writing ddi reg\n");
}

void ili_ic_get_ddi_reg_onepage(u8 page, u8 reg, u8 *data, bool mcu)
{
	u32 setpage = 0x1FFFFF00 | page;
	u32 setreg = 0x2F000100 | (reg << 16);
	bool ice = atomic_read(&ilits->ice_stat);

	ILI_INFO("setpage = 0x%X setreg = 0x%X\n", setpage, setreg);

	if (!ice)
		if (ili_ice_mode_ctrl(ENABLE, mcu) < 0)
			ILI_ERR("Enable ice mode failed before reading ddi reg\n");

	/*TDI_WR_KEY*/
	ilitek_tddi_ic_wr_pack(0x1FFF9527);
	/*Set Read Page reg*/
	ilitek_tddi_ic_wr_pack(setpage);

	/*TDI_RD_KEY*/
	ilitek_tddi_ic_wr_pack(0x1FFF9487);
	/*( *( __IO uint8 *)	(0x4800A) ) =0x2*/
	if (ili_ice_mode_write(0x4800A, 0x02, 1) < 0)
		ILI_ERR("Write 0x2 at 0x4800A\n");

	*data = ilitek_tddi_ic_rd_pack(setreg);
	ILI_INFO("check page = 0x%X, reg = 0x%X, read 0x%X\n", page, reg, *data);

	/*TDI_RD_KEY OFF*/
	ilitek_tddi_ic_wr_pack(0x1FFF9400);
	/*TDI_WR_KEY OFF*/
	ilitek_tddi_ic_wr_pack(0x1FFF9500);

	if (!ice)
		if (ili_ice_mode_ctrl(DISABLE, mcu) < 0)
			ILI_ERR("Disable ice mode failed after reading ddi reg\n");
}

void ili_ic_get_pc_counter(int stat)
{
	bool ice = atomic_read(&ilits->ice_stat);
	u32 pc = 0, pc_addr = ilits->chip->pc_counter_addr;
	u32 latch = 0, latch_addr = ilits->chip->pc_latch_addr;
	int ret = 0;
#if (TDDI_INTERFACE == BUS_SPI)
	u32 mcu_state = 0;
	bool read = 0;
#endif

	ILI_DBG("stat = %d\n", stat);

	if (ilits->cascade_info_block.nNum != 0) {
#if (TDDI_INTERFACE == BUS_SPI)
		if (stat == DO_SPI_RECOVER)
			mcu_state = OFF;
		else
			mcu_state = ON; 

        ili_cascade_rw_tp_reg(mcu_state, read, pc_addr, 0, 0, &pc);
		ili_cascade_rw_tp_reg(mcu_state, read, latch_addr, 0, 0, &latch);

		ilits->fw_pc = pc;
		ilits->fw_latch = latch;

		if (ilits->slave_wr_ctrl) {
			ILI_ERR("Read Slave counter (addr: 0x%x) = 0x%x, latch (addr: 0x%x) = 0x%x\n",
				pc_addr, ilits->fw_pc, latch_addr, ilits->fw_latch);
		} else {
			ILI_ERR("Read Master counter (addr: 0x%x) = 0x%x, latch (addr: 0x%x) = 0x%x\n",
				pc_addr, ilits->fw_pc, latch_addr, ilits->fw_latch);
		}

		/* Avoid screen abnormal. */
		if (stat == DO_SPI_RECOVER) {
			atomic_set(&ilits->ice_stat, DISABLE);
			return;
		}

#elif (TDDI_INTERFACE == BUS_I2C)
		if (ilits->slave_wr_ctrl) {
			ilits->i2c_addr_change = SLAVE;
			ILI_DBG("Change to Slave Address\n");
		} else {
			ilits->i2c_addr_change = MASTER;
			ILI_DBG("Change to Master Address\n");
		}

		if (!ice) {
			if (stat == DO_I2C_RECOVER)
				ret = ili_ice_mode_ctrl_by_mode(ENABLE, OFF, ilits->slave_wr_ctrl);
			else
				ret = ili_ice_mode_ctrl_by_mode(ENABLE, ON, ilits->slave_wr_ctrl);

			if (ret < 0)
				ILI_ERR("Enable ice mode failed while reading pc counter\n");
		}

		ili_ice_mode_read_by_mode(pc_addr, &pc, sizeof(u32), ilits->slave_wr_ctrl);
		ili_ice_mode_read_by_mode(latch_addr, &latch, sizeof(u32), ilits->slave_wr_ctrl);

		ilits->fw_pc = pc;
		ilits->fw_latch = latch;

		if (ilits->slave_wr_ctrl) {
			ILI_ERR("Read Slave counter (addr: 0x%x) = 0x%x, latch (addr: 0x%x) = 0x%x\n",
				pc_addr, ilits->fw_pc, latch_addr, ilits->fw_latch);
		} else {
			ILI_ERR("Read Master counter (addr: 0x%x) = 0x%x, latch (addr: 0x%x) = 0x%x\n",
				pc_addr, ilits->fw_pc, latch_addr, ilits->fw_latch);
		}

		if (!ice) {
			if (ili_ice_mode_ctrl_by_mode(DISABLE, ON, ilits->slave_wr_ctrl) < 0)
				ILI_ERR("Disable ice mode failed while reading pc counter\n");
		}
#endif
    } else {
        if (!ice) {
			if (stat == DO_SPI_RECOVER || stat == DO_I2C_RECOVER)
				ret = ili_ice_mode_ctrl(ENABLE, OFF);
			else
				ret = ili_ice_mode_ctrl(ENABLE, ON);

			if (ret < 0)
				ILI_ERR("Enable ice mode failed while reading pc counter\n");
		}

		if (ili_ice_mode_read(pc_addr, &pc, sizeof(u32)) < 0)
			ILI_ERR("Read pc conter error\n");

		if (ili_ice_mode_read(latch_addr, &latch, sizeof(u32)) < 0)
			ILI_ERR("Read pc latch error\n");

		ilits->fw_pc = pc;
		ilits->fw_latch = latch;
		ILI_ERR("Read counter (addr: 0x%x) = 0x%x, latch (addr: 0x%x) = 0x%x\n",
			pc_addr, ilits->fw_pc, latch_addr, ilits->fw_latch);

		/* Avoid screen abnormal. */
		if (stat == DO_SPI_RECOVER) {
			atomic_set(&ilits->ice_stat, DISABLE);
			return;
		}

		if (!ice) {
			if (ili_ice_mode_ctrl(DISABLE, ON) < 0)
				ILI_ERR("Disable ice mode failed while reading pc counter\n");
		}
	}
}

int ili_ic_int_trigger_ctrl(bool pulse)
{
	/* It's supported by fw, and the level will be kept at high until data was already prepared. */
	if (ili_ic_func_ctrl("int_trigger", pulse) < 0) {
		ILI_ERR("Write CMD error, set back to <%s> trigger\n", ilits->int_pulse ? "Level" : "Pulse");
		return -1;
	}

	ilits->int_pulse = pulse;
	ILI_INFO("INT Trigger = %s\n", ilits->int_pulse ? "Level" : "Pulse");
	return 0;
}

int ili_ic_check_int_level(bool level)
{
	int timer = 3000;
	int gpio = IRQ_GPIO_NUM; /* ilits->tp_int */

	/*
	 * If callers have a trouble to use the gpio that is passed by vendors,
	 * please utlises a physical gpio number instead or call a help from them.
	 */

	while (--timer > 0) {
		if (level) {
			if (gpio_get_value(gpio)) {
				ILI_DBG("INT high detected.\n");
				return 0;
			}
		} else {
			if (!gpio_get_value(gpio)) {
				ILI_DBG("INT low detected.\n");;
				return 0;
			}
		}
		mdelay(1);
	}
	ILI_ERR("Error! INT level no detected.\n");
	return -1;
}

int ili_ic_check_int_pulse(bool pulse)
{
	if (!wait_event_interruptible_timeout(ilits->inq, !atomic_read(&ilits->cmd_int_check), msecs_to_jiffies(ilits->wait_int_timeout))) {
		ILI_ERR("Error! INT pulse no detected. Timeout = %d ms\n", ilits->wait_int_timeout);
		atomic_set(&ilits->cmd_int_check, DISABLE);
		return -1;
	}
	ILI_DBG("INT pulse detected.\n");
	return 0;
}

int ili_ic_check_busy(int count, int delay, bool irq)
{
	u8 cmd[2] = {0};
	u8 busy = 0, rby = 0;

	cmd[0] = P5_X_READ_DATA_CTRL;
	cmd[1] = P5_X_CDC_BUSY_STATE;

	if (ilits->actual_tp_mode == P5_X_FW_AP_MODE)
		rby = 0x41;
	else if (ilits->actual_tp_mode == P5_X_FW_TEST_MODE)
		rby = 0x51;
	else {
		ILI_ERR("Unknown TP mode (0x%x)\n", ilits->actual_tp_mode);
		return -EINVAL;
	}

	ILI_INFO("read byte = %x, delay = %d\n", rby, delay);

	do {
		if (ilits->wrapper(cmd, sizeof(cmd), NULL, 0, OFF, OFF) < 0)
			ILI_ERR("Write check busy cmd failed\n");

		if (ilits->wrapper(&cmd[1], sizeof(u8), &busy, sizeof(u8), irq, OFF) < 0)
			ILI_ERR("Read check busy failed\n");

		ILI_DBG("busy = 0x%x\n", busy);

		if (busy == rby) {
			ILI_INFO("Check busy free\n");
			return 0;
		}

		mdelay(delay);
	} while (--count > 0);

	ILI_ERR("Check busy (0x%x) timeout !\n", busy);
	ili_ic_get_pc_counter(0);

	if (ilits->cascade_info_block.nNum != 0) {
		ilits->slave_wr_ctrl = 1;
		ili_ic_get_pc_counter(0);
		ilits->slave_wr_ctrl = 0;
	}	
	return -1;
}

int ili_ic_get_project_id(u8 *pdata, int size)
{
	int i;
	u32 tmp;
	bool ice = atomic_read(&ilits->ice_stat);

	if (!pdata) {
		ILI_ERR("pdata is null\n");
		return -ENOMEM;
	}

	ILI_INFO("Read size = %d\n", size);

	if (!ice)
		if (ili_ice_mode_ctrl(ENABLE, OFF) < 0)
			ILI_ERR("Enable ice mode failed while reading project id\n");

	if (ili_ice_mode_write(0x041000, 0x0, 1) < 0)
		ILI_ERR("Pull cs low failed\n");
	if (ili_ice_mode_write(0x041004, 0x66aa55, 3) < 0)
		ILI_ERR("Write key failed\n");

	if (ili_ice_mode_write(0x041008, 0x03, 1) < 0)
		ILI_ERR("Write 0x03 at 0x041008\n");

	if (ili_ice_mode_write(0x041008, (RSV_BK_ST_ADDR & 0xFF0000) >> 16, 1) < 0)
		ILI_ERR("Write address failed\n");
	if (ili_ice_mode_write(0x041008, (RSV_BK_ST_ADDR & 0x00FF00) >> 8, 1) < 0)
		ILI_ERR("Write address failed\n");
	if (ili_ice_mode_write(0x041008, (RSV_BK_ST_ADDR & 0x0000FF), 1) < 0)
		ILI_ERR("Write address failed\n");

	for (i = 0; i < size; i++) {
		if (ili_ice_mode_write(0x041008, 0xFF, 1) < 0)
			ILI_ERR("Write dummy failed\n");
		if (ili_ice_mode_read(0x41010, &tmp, sizeof(u8)) < 0)
			ILI_ERR("Read project id error\n");
		pdata[i] = tmp;
		ILI_INFO("project_id[%d] = 0x%x\n", i, pdata[i]);
	}

	ili_flash_clear_dma();

	if (ili_ice_mode_write(0x041000, 0x1, 1) < 0)
		ILI_ERR("Pull cs high\n");

	if (!ice)
		if (ili_ice_mode_ctrl(DISABLE, OFF) < 0)
			ILI_ERR("Disable ice mode failed while reading project id\n");

	return 0;
}

int ili_ic_get_core_ver(void)
{
	int i = 0, ret = 0;
	u8 cmd[2] = {0}, buf[10] = {0};

	ilits->protocol->core_ver_len = P5_X_CORE_VER_FOUR_LENGTH;

	if (ilits->info_from_hex) {
		buf[1] = ilits->fw_info[68];
		buf[2] = ilits->fw_info[69];
		buf[3] = ilits->fw_info[70];
		buf[4] = ilits->fw_info[71];
		goto out;
	}

	do {
		if (i == 0) {
			cmd[0] = P5_X_READ_DATA_CTRL;
			cmd[1] = P5_X_GET_CORE_VERSION_NEW;
		} else {
			cmd[0] = P5_X_READ_DATA_CTRL;
			cmd[1] = P5_X_GET_CORE_VERSION;
			ilits->protocol->core_ver_len = P5_X_CORE_VER_THREE_LENGTH;
		}

		if (ilits->wrapper(cmd, sizeof(cmd), NULL, 0, OFF, OFF) < 0)
			ILI_ERR("Write core ver cmd failed\n");

		if (ilits->wrapper(&cmd[1], sizeof(u8), buf, ilits->protocol->core_ver_len, ON, OFF) < 0)
			ILI_ERR("Write core ver (0x%x) failed\n", cmd[1]);

		ILI_DBG("header = 0x%x\n", buf[0]);

		if (buf[0] == P5_X_GET_CORE_VERSION ||
			buf[0] == P5_X_GET_CORE_VERSION_NEW)
			break;

	} while (++i < 2);

	if (buf[0] == P5_X_GET_CORE_VERSION)
		buf[4] = 0;

	if (i >= 2) {
		ILI_ERR("Invalid header (0x%x)\n", buf[0]);
		ret = -EINVAL;
	}

out:
	ILI_INFO("Core version = %d.%d.%d.%d\n", buf[1], buf[2], buf[3], buf[4]);
	ilits->chip->core_ver = buf[1] << 24 | buf[2] << 16 | buf[3] << 8 | buf[4];
	return ret;
}

void ili_fw_uart_ctrl(u8 ctrl)
{
	u8 cmd[4] = {0};

	if (ctrl > 1) {
		ILI_INFO("Unknown cmd, ignore\n");
		return;
	}

	ILI_INFO("%s UART mode\n", ctrl ? "Enable" : "Disable");

	cmd[0] = P5_X_I2C_UART;
	cmd[1] = 0x3;
	cmd[2] = 0;
	cmd[3] = ctrl;

	if (ilits->wrapper(cmd, sizeof(cmd), NULL, 0, OFF, OFF) < 0) {
		ILI_INFO("Write fw uart cmd failed\n");
		return;
	}
}

int ili_ic_get_fw_ver(void)
{
	int ret = 0;
	u8 cmd[2] = {0};
	u8 buf[10] = {0};

	if (ilits->info_from_hex) {
		buf[1] = ilits->fw_info[48];
		buf[2] = ilits->fw_info[49];
		buf[3] = ilits->fw_info[50];
		buf[4] = ilits->fw_info[51];
		buf[5] = ilits->fw_mp_ver[0];
		buf[6] = ilits->fw_mp_ver[1];
		buf[7] = ilits->fw_mp_ver[2];
		buf[8] = ilits->fw_mp_ver[3];
		goto out;
	}

	cmd[0] = P5_X_READ_DATA_CTRL;
	cmd[1] = P5_X_GET_FW_VERSION;

	if (ilits->wrapper(cmd, sizeof(cmd), NULL, 0, OFF, OFF) < 0) {
		ILI_ERR("Write pre cmd failed\n");
		ret = -EINVAL;
		goto out;

	}

	if (ilits->wrapper(&cmd[1], sizeof(u8), buf, ilits->protocol->fw_ver_len, ON, OFF) < 0) {
		ILI_ERR("Write fw version cmd failed\n");
		ret = -EINVAL;
		goto out;
	}

	if (buf[0] != P5_X_GET_FW_VERSION) {
		ILI_ERR("Invalid firmware ver\n");
		ret = -1;
	}

out:
	ILI_INFO("Firmware version = %d.%d.%d.%d\n", buf[1], buf[2], buf[3], buf[4]);
	ILI_INFO("Firmware MP version = %d.%d.%d.%d\n", buf[5], buf[6], buf[7], buf[8]);
	ilits->chip->fw_ver = buf[1] << 24 | buf[2] << 16 | buf[3] << 8 | buf[4];
	ilits->chip->fw_mp_ver = buf[5] << 24 | buf[6] << 16 | buf[7] << 8 | buf[8];
	return ret;
}

int ili_ic_get_panel_info(void)
{
	int ret = 0;
	u8 cmd = P5_X_GET_PANEL_INFORMATION;
	u8 buf[10] = {0};
	u8 len = ilits->protocol->panel_info_len;
	u8 data_type = P5_X_FW_SIGNAL_DATA_MODE;

	if (ilits->info_from_hex && (ilits->chip->core_ver >= CORE_VER_1410)) {
		buf[1] = ilits->fw_info[16];
		buf[2] = ilits->fw_info[17];
		buf[3] = ilits->fw_info[18];
		buf[4] = ilits->fw_info[19];
		ilits->panel_wid = buf[2] << 8 | buf[1];
		ilits->panel_hei = buf[4] << 8 | buf[3];
		ilits->trans_xy = (ilits->chip->core_ver >= CORE_VER_1430
			&& (ilits->rib.nReportByPixel > 0)) ? ON : OFF;
		goto out;
	}

	len = (ilits->chip->core_ver >= CORE_VER_1430) ? 6 : len;

	ret = ilits->wrapper(&cmd, sizeof(cmd), buf, len, ON, OFF);
	if (ret < 0)
		ILI_ERR("Read panel info error\n");

	if (buf[0] != cmd) {
		ILI_INFO("Invalid panel info, use default resolution\n");
		ilits->panel_wid = TOUCH_SCREEN_X_MAX;
		ilits->panel_hei = TOUCH_SCREEN_Y_MAX;
		ilits->trans_xy = OFF;
	} else {
		ilits->panel_wid = buf[1] << 8 | buf[2];
		ilits->panel_hei = buf[3] << 8 | buf[4];
		ilits->trans_xy = (ilits->chip->core_ver >= CORE_VER_1430) ? buf[5] : OFF;
	}

	if (ilits->chip->core_ver >= CORE_VER_1470) {
		cmd = P5_X_GET_REPORT_FORMAT;
		ret = ilits->wrapper(&cmd, sizeof(cmd), buf, 2, ON, OFF);
		if (ret < 0)
			ILI_ERR("Read report format info error\n");

		if (buf[0] != cmd) {
			ILI_INFO("Invalid report format info, use default report format resolution\n");
			ilits->rib.nReportResolutionMode = POSITION_LOW_RESOLUTION;
			ilits->rib.nCustomerType = POSITION_CUSTOMER_TYPE_OFF;
			ilits->PenType = POSITION_PEN_TYPE_OFF;
			if (ilits->chip->core_ver >= CORE_VER_1700) {
				ilits->customertype_off = POSITION_CUSTOMER_TYPE_OFF_3BITS;
			} else {
				ilits->customertype_off = POSITION_CUSTOMER_TYPE_OFF;
			}
		} else {
			ilits->rib.nReportResolutionMode = buf[1] & 0x07;
			
			if (ilits->chip->core_ver >= CORE_VER_1700) {
				ilits->rib.nCustomerType = (buf[1] >> 3) & 0x07;
				ilits->PenType = buf[1] >> 6;
				ilits->customertype_off = POSITION_CUSTOMER_TYPE_OFF_3BITS;
				
				if (ilits->PenType == POSITION_PEN_TYPE_ON) {
					ilits->touch_num = MAX_TOUCH_NUM + MAX_PEN_NUM;
					ili_set_tp_data_len(DATA_FORMAT_DEMO, false, &data_type);
					ILI_INFO("Panel support Pen Mode, Packet Len = %d, Touch Number = %d\n", ilits->tp_data_len, ilits->touch_num);
				}
			} else {
				ilits->PenType = POSITION_PEN_TYPE_OFF;
				ilits->rib.nCustomerType = buf[1] >> 3;
				ilits->customertype_off = POSITION_CUSTOMER_TYPE_OFF;
			}
		}
	} else {
		ilits->PenType = POSITION_PEN_TYPE_OFF;
		ilits->rib.nReportResolutionMode = POSITION_LOW_RESOLUTION;
		ilits->rib.nCustomerType = POSITION_CUSTOMER_TYPE_OFF;
		ilits->customertype_off = POSITION_CUSTOMER_TYPE_OFF;
	}
	ILI_INFO("PenType = 0x%x, Customer Type OFF = 0x%x\n", ilits->PenType, ilits->customertype_off);

#if ENABLE_PEN_MODE
	cmd = P5_X_GET_PEN_INFO;
	ret = ilits->wrapper(&cmd, sizeof(cmd), buf, 9, ON, OFF);
	ILI_INFO("cmd = 0x%X, buf[0] = 0x%X, buf[1] = 0x%X, buf[2] = 0x%X, buf[3] = 0x%X, buf[4] = 0x%X\n", cmd, buf[0], buf[1], buf[2], buf[3], buf[4]);
	if (ret < 0)
		ILI_ERR("Read report format info error\n");

	if (buf[0] != cmd) {
		ILI_INFO("Invalid Pen info, use default report format resolution\n");
		ilits->pen_info_block.nPxRaw = P5_X_PEN_INFO_X_DATA_LEN;
		ilits->pen_info_block.nPyRaw = P5_X_PEN_INFO_Y_DATA_LEN;
		ilits->pen_info_block.nPxVa = 1;
		ilits->pen_info_block.nPyVa = 1;
	} else {
		ilits->pen_info_block.nPxRaw = buf[1];
		ilits->pen_info_block.nPyRaw = buf[2];
		ilits->pen_info_block.nPxVa = buf[3];
		ilits->pen_info_block.nPyVa = buf[4];
	}
#endif
#if ENABLE_CASCADE
	cmd = P5_X_GET_CASCADE_INFO;
	ret = ilits->wrapper(&cmd, sizeof(cmd), buf, 5, ON, OFF);
	ILI_INFO("cmd = 0x%X, buf[0] = 0x%X, buf[1] = 0x%X, buf[2] = 0x%X, buf[3] = 0x%X\n", cmd, buf[0], buf[1], buf[2], buf[3]);
	if (ret < 0)
		ILI_ERR("Read report format info error\n");

	if (buf[0] != cmd) {
		ilits->cascade_info_block.nDisable = ENABLE;
		ilits->cascade_info_block.nNum = 2;
	} else {
		ilits->cascade_info_block.nDisable = buf[1] & BIT(0);
		ilits->cascade_info_block.nNum =  (buf[1] >> 1) & 0x07;
	}
#else
	ilits->cascade_info_block.nDisable = ENABLE;
	ilits->cascade_info_block.nNum = 0;
#endif
out:
	ILI_INFO("Panel info: width = %d, height = %d\n", ilits->panel_wid, ilits->panel_hei);
	ILI_INFO("Transfer touch coordinate = %s\n", ilits->trans_xy ? "ON" : "OFF");
	ILI_INFO("Customer Type = %X\n", ilits->rib.nCustomerType);
	ILI_INFO("Report Resolution Format Mode = %X\n", ilits->rib.nReportResolutionMode);

	if (ilits->chip->core_ver >= CORE_VER_1700) {
		ILI_INFO("Pen Type = %X\n", ilits->PenType);
		ILI_INFO("Pen Info Data, nPxRaw = %d, nPyRaw = %d, nPxVa = %d, nPyVa = %d\n", ilits->pen_info_block.nPxRaw, ilits->pen_info_block.nPyRaw, ilits->pen_info_block.nPxVa, ilits->pen_info_block.nPyVa);
		ILI_INFO("Cascade Info Data, nDisable = %d, nNum = %d\n", ilits->cascade_info_block.nDisable, ilits->cascade_info_block.nNum);
	}

	return ret;
}

int ili_ic_get_tp_info(void)
{
	int ret = 0;
	u8 cmd[2] = {0};
	u8 buf[20] = {0};

	if (ilits->info_from_hex  && (ilits->chip->core_ver >= CORE_VER_1410)) {
		buf[1] = ilits->fw_info[5];
		buf[2] = ilits->fw_info[7];
		buf[3] = ilits->fw_info[8];
		buf[4] = ilits->fw_info[9];
		buf[5] = ilits->fw_info[10];
		buf[6] = ilits->fw_info[11];
		buf[7] = ilits->fw_info[12];
		buf[8] = ilits->fw_info[14];
		if (ilits->chip->core_ver >= CORE_VER_1700) {
		buf[11] = ilits->fw_info[13];
		buf[12] = ilits->fw_info[15];
		} else {
			buf[11] = buf[7];
			buf[12] = buf[8];
		}
		goto out;
	}

	cmd[0] = P5_X_READ_DATA_CTRL;
	cmd[1] = P5_X_GET_TP_INFORMATION;

	if (ilits->wrapper(cmd, sizeof(cmd), NULL, 0, OFF, OFF) < 0) {
		ILI_ERR("Write tp info pre cmd failed\n");
		ret = -EINVAL;
		goto out;

	}

	ret = ilits->wrapper(&cmd[1], sizeof(u8), buf, ilits->protocol->tp_info_len, ON, OFF);
	if (ret < 0) {
		ILI_ERR("Read tp info error\n");
		goto out;
	}

	if (buf[0] != P5_X_GET_TP_INFORMATION) {
		ILI_ERR("Invalid tp info\n");
		ret = -1;
		goto out;
	}

out:
	ilits->min_x = buf[1];
	ilits->min_y = buf[2];
	ilits->max_x = buf[4] << 8 | buf[3];
	ilits->max_y = buf[6] << 8 | buf[5];
	ilits->xch_num = buf[7];
	ilits->ych_num = buf[8];
	ilits->stx = buf[11];
	ilits->srx = buf[12];

	ILI_INFO("TP Info: min_x = %d, min_y = %d, max_x = %d, max_y = %d\n", ilits->min_x, ilits->min_y, ilits->max_x, ilits->max_y);
	ILI_INFO("TP Info: xch = %d, ych = %d, stx = %d, srx = %d\n", ilits->xch_num, ilits->ych_num, ilits->stx, ilits->srx);
	return ret;
}

void ili_ic_get_compress_info(void)
{
#if ENABLE_COMPRESS_MODE
	int ret = 0;
	u8 cmd = P5_X_GET_COMPRESS_INFORMATION;
	u8 buf[10] = {0};

	if (ilits->info_from_hex  && (ilits->chip->core_ver >= CORE_VER_1700)) {
		buf[1] = ilits->fw_info[0];
		goto out;
	}

	ret = ilits->wrapper(&cmd, sizeof(cmd), buf, 5, ON, OFF);
	if (ret < 0)
		ILI_ERR("Read compress info error\n");

	if (buf[0] != cmd) {
		ILI_INFO("Invalid compress info, use default value\n");
		ilits->compress_disable = true;
		ilits->compress_handonly_disable = true;
		ilits->compress_penonly_disable = true;
	} else {
		ilits->compress_disable = (buf[1] & 0x20) >> 5;
		ilits->compress_handonly_disable = (buf[1] & 0x40) >> 6;
		ilits->compress_penonly_disable = (buf[1] & 0x80) >> 7;
	}

out:
	ilits->compress_disable = (buf[1] & 0x20) >> 5;
	ilits->compress_handonly_disable = (buf[1] & 0x40) >> 6;
	ilits->compress_penonly_disable = (buf[1] & 0x80) >> 7;
	ILI_INFO("compress_disable = %d, compress_handonly_disable = %d, compress_penonly_disable = %d\n", ilits->compress_disable, ilits->compress_handonly_disable, ilits->compress_penonly_disable);
#endif
}



int ili_ic_get_all_info(void)
{
	int ret = 0;
#if (TDDI_INTERFACE == BUS_I2C && ENABLE_GET_ALL_INFO)
	u8 cmd = P5_X_GET_ALL_INFORMATION;
	u8 buf[100] = {0};
	bool get_all_info;

	if (ilits->wrapper(&cmd, sizeof(cmd), buf, 96, ON, OFF) < 0) {
		ILI_ERR("Write ALL info cmd failed\n");
		get_all_info = false;
		ret = -EINVAL;
	}

	if (buf[0] == P5_X_GET_ALL_INFORMATION) {
		get_all_info = true;

		/* CORE_VERSION */
		ilits->chip->core_ver = buf[25] << 24 | buf[26] << 16 | buf[27] << 8 | buf[28];

		/*GET_TP_INFORMATION*/
		ilits->min_x = buf[2];
		ilits->min_y = buf[3];
		ilits->max_x = buf[5] << 8 | buf[4];
		ilits->max_y = buf[7] << 8 | buf[6];
		ilits->xch_num = buf[8];
		ilits->ych_num = buf[9];
		ilits->stx = buf[12];
		ilits->srx = buf[13];

		/* GET_FW_VERSION */
		ilits->chip->fw_ver = buf[16] << 24 | buf[17] << 16 | buf[18] << 8 | buf[19];
		ilits->chip->fw_mp_ver = buf[20] << 24 | buf[21] << 16 | buf[22] << 8 | buf[23];

		/* GET_PANEL_INFORMATION */
		cmd = P5_X_GET_PANEL_INFORMATION;
		if (buf[46] != cmd) {
			ILI_INFO("Invalid panel info, use default resolution\n");
			ilits->panel_wid = TOUCH_SCREEN_X_MAX;
			ilits->panel_hei = TOUCH_SCREEN_Y_MAX;
			ilits->trans_xy = OFF;
		} else {
			ilits->panel_wid = buf[47] << 8 | buf[48];
			ilits->panel_hei = buf[49] << 8 | buf[50];
			ilits->trans_xy = (ilits->chip->core_ver >= CORE_VER_1430) ? buf[51] : OFF;
		}

		/* GET_COMPRESS_INFORMATION */
		cmd = P5_X_GET_COMPRESS_INFORMATION;
		if (buf[52] != cmd) {
			ILI_INFO("Invalid compress & report format info, use default report format resolution\n");
			ilits->compress_disable = true;
			ilits->compress_handonly_disable = true;
			ilits->compress_penonly_disable = true;
			ilits->rib.nReportResolutionMode = POSITION_LOW_RESOLUTION;
			ilits->rib.nCustomerType = POSITION_CUSTOMER_TYPE_OFF;
			ilits->PenType = POSITION_PEN_TYPE_OFF;
			if (ilits->chip->core_ver >= CORE_VER_1700) {
				ilits->customertype_off = POSITION_CUSTOMER_TYPE_OFF_3BITS;
			} else {
				ilits->customertype_off = POSITION_CUSTOMER_TYPE_OFF;
			}
		} else {
			/* GET_COMPRESS_INFORMATION(0x2B) */
			ilits->compress_disable = (buf[53] & 0x20) >> 5;
			ilits->compress_handonly_disable = (buf[53] & 0x40) >> 6;
			ilits->compress_penonly_disable = (buf[53] & 0x80) >> 7;
			/* GET_REPORT_FORMAT(0x37) */
			ilits->rib.nReportResolutionMode = buf[54] & 0x07;

			/* note : After core ver 1.7.0.0 support 0x2F cmd, so ignore before case */
			ilits->rib.nCustomerType = (buf[54] >> 3) & 0x07;
			ilits->PenType = buf[54] >> 6;
			ilits->customertype_off = POSITION_CUSTOMER_TYPE_OFF_3BITS;

			if (ilits->PenType == POSITION_PEN_TYPE_ON) {
				ilits->touch_num = MAX_TOUCH_NUM + MAX_PEN_NUM;
				ili_set_tp_data_len(DATA_FORMAT_DEMO, false, NULL);
				ILI_INFO("Panel support Pen Mode, Packet Len = %d, Touch Number = %d\n", ilits->tp_data_len, ilits->touch_num);
			}

			/* reserved : buf[55] ~ buf[56] = 0xFF */
		}

#if ENABLE_PEN_MODE
		/* GET_PEN_INFO */
		cmd = P5_X_GET_PEN_INFO;
		if (buf[57] != cmd) {
			ILI_INFO("Invalid Pen info, use default report format resolution\n");
			ilits->pen_info_block.nPxRaw = P5_X_PEN_INFO_X_DATA_LEN;
			ilits->pen_info_block.nPyRaw = P5_X_PEN_INFO_Y_DATA_LEN;
			ilits->pen_info_block.nPxVa = 1;
			ilits->pen_info_block.nPyVa = 1;
		} else {
			ilits->pen_info_block.nPxRaw = buf[58];
			ilits->pen_info_block.nPyRaw = buf[59];
			ilits->pen_info_block.nPxVa = buf[60];
			ilits->pen_info_block.nPyVa = buf[61];
		}
		/* reserved : buf[62] ~ buf[65] = 0xFF */
#endif

#if ENABLE_CASCADE
		/* GET_CASCADE_INFO */
		cmd = P5_X_GET_CASCADE_INFO;
		if (buf[66] != cmd) {
			ilits->cascade_info_block.nDisable = ENABLE;
			ilits->cascade_info_block.nNum = 2;
		} else {
			ilits->cascade_info_block.nDisable = buf[67] & BIT(0);
			ilits->cascade_info_block.nNum =  (buf[67] >> 1) & 0x07;
		}
#else
		ilits->cascade_info_block.nDisable = ENABLE;
		ilits->cascade_info_block.nNum = 0;
#endif
	} else {
		ILI_ERR("Read 0x2F error, buf[0] = 0x%X\n", buf[0]);
		get_all_info = false;
	}

	if (get_all_info == true) {
		ILI_INFO("Firmware version(0x%8X) = %d.%d.%d.%d\n",ilits->chip->fw_ver , buf[16], buf[17], buf[18], buf[19]);
		ILI_INFO("Firmware MP version(0x%8X) = %d.%d.%d.%d\n",ilits->chip->fw_mp_ver , buf[20], buf[21], buf[22], buf[23]);
		ILI_INFO("Core version = %d.%d.%d.%d\n", buf[25], buf[26], buf[27], buf[28]);
		ILI_INFO("TP Info: min_x = %d, min_y = %d, max_x = %d, max_y = %d\n", ilits->min_x, ilits->min_y, ilits->max_x, ilits->max_y);
		ILI_INFO("TP Info: xch = %d, ych = %d, stx = %d, srx = %d\n", ilits->xch_num, ilits->ych_num, ilits->stx, ilits->srx);
		ILI_INFO("Compress Info : compress_disable = %d, compress_handonly_disable = %d, compress_penonly_disable = %d\n",ilits->compress_disable, ilits->compress_handonly_disable, ilits->compress_penonly_disable);
		ILI_INFO("Pen Len Info : PxRaw = %d, PyRaw = %d, PxVa = %d, PyVa = %d\n", ilits->pen_info_block.nPxRaw, ilits->pen_info_block.nPyRaw, ilits->pen_info_block.nPxVa, ilits->pen_info_block.nPyVa);
		ILI_INFO("CASCADE Info : cascade_info_block.nDisable = %d, cascade_info_block.nNum = %d\n", ilits->cascade_info_block.nDisable, ilits->cascade_info_block.nNum);
		ILI_INFO("Panel info: width = %d, height = %d\n", ilits->panel_wid, ilits->panel_hei);
		ILI_INFO("Transfer touch coordinate = %s, Customer Type = %X, Report Resolution Format Mode = %X\n", ilits->trans_xy ? "ON" : "OFF", ilits->rib.nCustomerType, ilits->rib.nReportResolutionMode);
	} else {
		ili_ic_get_core_ver();
		ili_ic_get_protocl_ver();
		ili_ic_get_fw_ver();
		ili_ic_get_tp_info();
		ili_ic_get_panel_info();
		ili_ic_get_compress_info();
	}
#else
	ili_ic_get_core_ver();
	ili_ic_get_protocl_ver();
	ili_ic_get_fw_ver();
	ili_ic_get_tp_info();
	ili_ic_get_panel_info();
	ili_ic_get_compress_info();
#endif

	return ret;
}

static void ilitek_tddi_ic_check_protocol_ver(u32 pver)
{
	int i = 0;

	if (ilits->protocol->ver == pver) {
		ILI_DBG("same procotol version, do nothing\n");
		return;
	}

	for (i = 0; i < PROTOCL_VER_NUM - 1; i++) {
		if (protocol_info[i].ver == pver) {
			ilits->protocol = &protocol_info[i];
			ILI_INFO("update protocol version = %x\n", ilits->protocol->ver);
			return;
		}
	}

	ILI_ERR("Not found a correct protocol version in list, use newest version\n");
	ilits->protocol = &protocol_info[PROTOCL_VER_NUM - 1];
}

int ili_ic_get_protocl_ver(void)
{
	int ret = 0;
	u8 cmd[2] = {0};
	u8 buf[10] = {0};
	u32 ver;

	if (ilits->info_from_hex) {
		buf[1] = ilits->fw_info[72];
		buf[2] = ilits->fw_info[73];
		buf[3] = ilits->fw_info[74];
		goto out;
	}

	cmd[0] = P5_X_READ_DATA_CTRL;
	cmd[1] = P5_X_GET_PROTOCOL_VERSION;

	if (ilits->wrapper(cmd, sizeof(cmd), NULL, 0, OFF, OFF) < 0) {
		ILI_ERR("Write protocol ver pre cmd failed\n");
		ret = -EINVAL;
		goto out;

	}

	if (ilits->wrapper(&cmd[1], sizeof(u8), buf, ilits->protocol->pro_ver_len, ON, OFF) < 0) {
		ILI_ERR("Read protocol version error\n");
		ret = -EINVAL;
		goto out;
	}

	if (buf[0] != P5_X_GET_PROTOCOL_VERSION) {
		ILI_ERR("Invalid protocol ver\n");
		ret = -1;
		goto out;
	}

out:
	ver = buf[1] << 16 | buf[2] << 8 | buf[3];

	ilitek_tddi_ic_check_protocol_ver(ver);

	ILI_INFO("Protocol version = %d.%d.%d\n", ilits->protocol->ver >> 16,
		(ilits->protocol->ver >> 8) & 0xFF, ilits->protocol->ver & 0xFF);
	return ret;
}

int ili_ic_get_info(void)
{
	int ret = 0;
	u8 ddi_data = 0;

	if (!atomic_read(&ilits->ice_stat)) {
		ILI_ERR("ice mode doesn't enable\n");
		return -1;
	}


	if (ili_ice_mode_read(ilits->chip->pid_addr, &ilits->chip->pid, sizeof(u32)) < 0)
		ILI_ERR("Read chip pid error\n");

	if (ili_ice_mode_read(ilits->chip->otp_addr, &ilits->chip->otp_id, sizeof(u32)) < 0)
		ILI_ERR("Read otp id error\n");
	if (ili_ice_mode_read(ilits->chip->ana_addr, &ilits->chip->ana_id, sizeof(u32)) < 0)
		ILI_ERR("Read ana id error\n");

	ilits->chip->id = ilits->chip->pid >> 16;
	ilits->chip->type = (ilits->chip->pid & 0x0000FF00) >> 8;
	ilits->chip->ver = ilits->chip->pid & 0xFF;
	ilits->chip->otp_id &= 0xFF;
	ilits->chip->ana_id &= 0xFF;

	ilits->chip->product_id = ilits->chip->id;
	if (ilits->chip->id == ILI9882_CHIP) {
		ili_ic_get_ddi_reg_onepage(0x6, 0xF3, &ddi_data, OFF);
		if (ddi_data == 0x30)
			ilits->chip->product_id = ILI2882_CHIP;
	}

	ILI_INFO("CHIP ID = %x\n", ilits->chip->product_id);

	ret = ilitek_tddi_ic_check_info(ilits->chip->pid, ilits->chip->id);
	return ret;
}

int ili_cascade_ic_get_info(bool enter_ice, bool exit_ice)
{
	int ret = 0;

#if (TDDI_INTERFACE == BUS_I2C)
	if (enter_ice) {
		ili_ice_mode_ctrl_by_mode(ENABLE, OFF, BOTH);
	}

	if (ili_ice_mode_read_by_mode(ilits->chip->pid_addr, &ilits->chip->slave_pid, sizeof(u32), SLAVE) < 0)
		ILI_ERR("Read Slave chip pid error\n");


	ili_ic_get_info();

#else
	if (enter_ice) {
		ili_set_bypass_mode(OFF);
	}

	ili_ic_get_info();

	ili_mspi_read_slave_register(TDDI_PID_ADDR, &ilits->chip->slave_pid, sizeof(u32));

	/* set mspi 0 */
	if (ili_ice_mode_write_by_mode(MSPI_REG, 0x00, 1, MASTER) < 0) {
		ILI_ERR("Failed to write MSPI_REG in ice mode\n");
	}

#endif

	if (exit_ice) {
		ili_ice_mode_ctrl_by_mode(DISABLE, OFF, BOTH);
	}

	ilits->chip->slave_id = ilits->chip->slave_pid >> 16;

	if (ilits->chip->pid != ilits->chip->slave_pid) {
		ILI_ERR("Slave chip pid different from Master\n");
		ret = -1;
	}

	ILI_INFO("Read Slave CHIP PID = 0x%X, CHIP ID = 0x%X\n", ilits->chip->slave_pid, ilits->chip->slave_id);
	return ret;
}

int ili_ic_dummy_check(void)
{
	int ret = 0;
	u32 wdata = 0xA55A5AA5;
	u32 rdata = 0;
#if (ENGINEER_FLOW)
	int i = 0;
	if (!atomic_read(&ilits->ice_stat)) {
		ILI_ERR("ice mode doesn't enable\n");
		return -1;
	}

	for (i = 0; i < 3; i++) {
		if (ili_ice_mode_write(WDT9_DUMMY2, wdata, sizeof(u32)) < 0)
			ILI_ERR("Write dummy error\n");

		if (ili_ice_mode_read(WDT9_DUMMY2, &rdata, sizeof(u32)) < 0)
			ILI_ERR("Read dummy error\n");

		if (rdata == wdata || rdata == (u32)-wdata) {
			if (rdata == -wdata) {
				ilits->eng_flow = true;
			} else {
				ilits->eng_flow = false;
			}
			break;
		}
		mdelay(30);
	}
	if (i >= 3) {
		ILI_ERR("Dummy check incorrect, rdata = %x wdata = %x \n", rdata, wdata);
		return -1;
	}
	ILI_INFO("Ilitek IC check successe ilits->eng_flow = %d\n", ilits->eng_flow);
#else
	if (!atomic_read(&ilits->ice_stat)) {
		ILI_ERR("ice mode doesn't enable\n");
		return -1;
	}

	if (ili_ice_mode_write(WDT9_DUMMY2, wdata, sizeof(u32)) < 0)
		ILI_ERR("Write dummy error\n");


	if (ili_ice_mode_read(WDT9_DUMMY2, &rdata, sizeof(u32)) < 0)
		ILI_ERR("Read dummy error\n");

	if (rdata != wdata) {
		ILI_ERR("Dummy check incorrect, rdata = %x wdata = %x \n", rdata, wdata);
		return -1;
	}
	ILI_INFO("Ilitek IC check successe\n");
#endif
	return ret;
}

int ili_ic_report_rate_set(u8 mode)
{
	int ret = 0;
	u8 cmd[4] = {0};
	ILI_INFO("current report rate mode = %x\n", ilits->current_report_rate_mode);

	if (mode == ilits->current_report_rate_mode) {
		ILI_INFO("set mode = %x, same as current mode = %x\n", mode, ilits->current_report_rate_mode);
		return ret;
	}

	cmd[0] = 0x01;
	cmd[1] = 0x1F;
	cmd[2] = 0x00;
	cmd[3] = mode;

	ret = ilits->wrapper(cmd, 4, NULL, 0, OFF, OFF);

	if (ret < 0)
		ILI_ERR("Write TP function failed\n");

	ilits->current_report_rate_mode = mode;
	return ret;
}

int ili_ic_check_debug_lite_support(int format, bool send, u8 *data)
{
	u8 cmd[12] = {0}, buf[4] = {0}, ctrl = 0, debug_ctrl = 0, data_type = 0;
	int ret;
	if (ilits->chip->core_ver < CORE_VER_1700) {
		ILI_INFO("no support check debug lite\n");
		return -1;
	}

	if (data == NULL) {
		data_type = P5_X_FW_SIGNAL_DATA_MODE;
		ILI_INFO("Data Type is Null, Set Single data type\n");
	} else {
		data_type = data[0];
		ILI_INFO("Set data type = 0x%X \n", data[0]);
	}

	switch (format) {
	case DATA_FORMAT_DEBUG_LITE_ROI:
		debug_ctrl = DATA_FORMAT_DEBUG_LITE_ROI_CMD;
		ctrl = DATA_FORMAT_DEBUG_LITE_CMD;
		break;
	case DATA_FORMAT_DEBUG_LITE_WINDOW:
		debug_ctrl = DATA_FORMAT_DEBUG_LITE_WINDOW_CMD;
		ctrl = DATA_FORMAT_DEBUG_LITE_CMD;
		break;
	case DATA_FORMAT_DEBUG_LITE_AREA:
		if (data == NULL) {
			ILI_ERR("DATA_FORMAT_DEBUG_LITE_AREA error cmd\n");
			return -1;
		}
		debug_ctrl = DATA_FORMAT_DEBUG_LITE_AREA_CMD;
		ctrl = DATA_FORMAT_DEBUG_LITE_CMD;
		data_type = data[0];
		cmd[4] = data[1];
		cmd[5] = data[2];
		cmd[6] = data[3];
		cmd[7] = data[4];
		break;
	}
	cmd[0] = P5_X_NEW_CONTROL_FORMAT;
	cmd[1] = ctrl;
	cmd[2] = data_type;
	cmd[3] = debug_ctrl;
	cmd[9] = 0x0A;	/* pre command */

	ret = ilits->wrapper(cmd, 10, buf, 2, ON, OFF);
	if (ret < 0) {
		ILI_ERR("check debug lite function failed\n");
		return ret;
	}

	ILI_DBG("check ,0x%X\n", buf[0]);
	return buf[0];
}

int ili_ic_report_rate_get(void)
{
	int ret = 0;
	u8 cmd[3] = {0}, rxbuf[4] = {0};

	cmd[0] = 0x01;
	cmd[1] = 0x1F;
	cmd[2] = 0x01;

	ret = ilits->wrapper(cmd, 3, rxbuf, 4, OFF, OFF);
	if (ret < 0) {
		ILI_ERR("Write TP function failed\n");
		return ret;
	}
	ILI_INFO("get report rate mode = %x, rxbuf = %x %x %x %x \n", rxbuf[3], rxbuf[0], rxbuf[1], rxbuf[2], rxbuf[3]);
	ret = rxbuf[3];
	return ret;
}

int ili_ic_report_rate_register_set(void)
{
	int ret = 0;

	if (ili_ice_mode_ctrl(ENABLE, OFF) < 0)
		ILI_ERR("Enable ice mode failed\n");
	ILI_INFO("current_report_rate_mode: %X when reset\n", ilits->current_report_rate_mode);
	ret = ili_ice_mode_write(0x4005E, (0x5A00 | (ilits->current_report_rate_mode & 0xFF)), 2);
	if (ret < 0)
		ILI_ERR("write report rate password failed\n");

	return ret;
}

static struct ilitek_ic_info chip;

void ili_ic_init(void)
{
	chip.pid_addr =		   	TDDI_PID_ADDR;
	chip.pc_counter_addr = 		TDDI_PC_COUNTER_ADDR;
	chip.pc_latch_addr =		TDDI_PC_LATCH_ADDR;
	chip.otp_addr =		   	TDDI_OTP_ID_ADDR;
	chip.ana_addr =		   	TDDI_ANA_ID_ADDR;
	chip.reset_addr =	   	TDDI_CHIP_RESET_ADDR;

	ilits->protocol = &protocol_info[PROTOCL_VER_NUM - 1];
	ilits->chip = &chip;
}
