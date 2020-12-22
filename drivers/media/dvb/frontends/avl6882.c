/*
 * Availink AVL6882 demod driver
 *
 * Copyright (C) 2015 Luis Alves <ljalvs@gmail.com>
 *
 *    This program is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License along
 *    with this program; if not, write to the Free Software Foundation, Inc.,
 *    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <linux/slab.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/string.h>
#include <linux/bitrev.h>

#include "dvb_frontend.h"
#include "avl6882.h"
#include "avl6882_priv.h"

#include "AVL_Demod_Patch_DVBSxFw_.h"
#include "AVL_Demod_Patch_DVBCFw_.h"
#include "AVL_Demod_Patch_DVBTxFw_.h"


static int avl6882_i2c_rd(struct avl6882_priv *priv, u8 *buf, int len)
{
	int ret;
	struct i2c_msg msg[] = {
		{
			.addr = priv->config->demod_address,
			.flags = 0,
			.len = 1,
			.buf = 0,
		},
		{
			.addr= priv->config->demod_address,
			.flags= I2C_M_RD,
			.len  = len,
			.buf  = buf,
		}
	};

	ret = i2c_transfer(priv->i2c, msg, 2);
	if (ret == 2) {
		ret = 0;
	} else {
		dev_warn(&priv->i2c->dev, "%s: i2c rd failed=%d " \
				"len=%d\n", KBUILD_MODNAME, ret, len);
		ret = -EREMOTEIO;
	}
	return ret;
}


static int avl6882_i2c_wr(struct avl6882_priv *priv, u8 *buf, int len)
{
	int ret;
	struct i2c_msg msg = {
		.addr= priv->config->demod_address,
		.flags = 0,
		.buf = buf,
		.len = len,
	};

	ret = i2c_transfer(priv->i2c, &msg, 1);
	if (ret == 1) {
		ret = 0;
	} else {
		dev_warn(&priv->i2c->dev, "%s: i2c wr failed=%d " \
				"len=%d\n", KBUILD_MODNAME, ret, len);
		ret = -EREMOTEIO;
	}
	return ret;
}


static int avl6882_i2c_wrm(struct avl6882_priv *priv, u8 *buf, int len)
{
	int ret;
	struct i2c_msg msg = {
		.addr= priv->config->demod_address,
		.flags = 1, /* ?? */
		.buf = buf,
		.len = len,
	};

	ret = i2c_transfer(priv->i2c, &msg, 1);
	if (ret == 1) {
		ret = 0;
	} else {
		dev_warn(&priv->i2c->dev, "%s: i2c wrm failed=%d " \
				"len=%d\n", KBUILD_MODNAME, ret, len);
		ret = -EREMOTEIO;
	}
	return ret;
}


/* write 32bit words at addr */
#define MAX_WORDS_WR_LEN	((MAX_II2C_WRITE_SIZE-3) / 4)
static int avl6882_i2c_wr_data(struct avl6882_priv *priv,
	u32 addr, u32 *data, int len)
{
	int ret = 0, this_len;
	u8 buf[MAX_II2C_WRITE_SIZE];
	u8 *p;

	while (len > 0) {
		p = buf;
		*(p++) = (u8) (addr >> 16);
		*(p++) = (u8) (addr >> 8);
		*(p++) = (u8) (addr);

		this_len = (len > MAX_WORDS_WR_LEN) ? MAX_WORDS_WR_LEN : len;
		len -= this_len;
		if (len)
			addr += this_len * 4;

		while (this_len--) {
			*(p++) = (u8) ((*data) >> 24);
			*(p++) = (u8) ((*data) >> 16);
			*(p++) = (u8) ((*data) >> 8);
			*(p++) = (u8) (*(data++));
		}

		if (len > 0)
			ret = avl6882_i2c_wrm(priv, buf, (int) (p - buf));
		else
			ret = avl6882_i2c_wr(priv, buf, (int) (p - buf));
		if (ret)
			break;
	}
	return ret;
}

static int avl6882_i2c_wr_reg(struct avl6882_priv *priv,
	u32 addr, u32 data, int reg_size)
{
	u8 buf[3 + 4];
	u8 *p = buf;

	*(p++) = (u8) (addr >> 16);
	*(p++) = (u8) (addr >> 8);
	*(p++) = (u8) (addr);

	switch (reg_size) {
	case 4:
		*(p++) = (u8) (data >> 24);
		*(p++) = (u8) (data >> 16);
	case 2:
		*(p++) = (u8) (data >> 8);
	case 1:
	default:
		*(p++) = (u8) (data);
		break;
	}

	return avl6882_i2c_wr(priv, buf, 3 + reg_size);
}

#define AVL6882_WR_REG8(_priv, _addr, _data) \
	avl6882_i2c_wr_reg(_priv, _addr, _data, 1)
#define AVL6882_WR_REG16(_priv, _addr, _data) \
	avl6882_i2c_wr_reg(_priv, _addr, _data, 2)
#define AVL6882_WR_REG32(_priv, _addr, _data) \
	avl6882_i2c_wr_reg(_priv, _addr, _data, 4)


static int avl6882_i2c_rd_reg(struct avl6882_priv *priv,
	u32 addr, u32 *data, int reg_size)
{
	int ret;
	u8 buf[3 + 4];
	u8 *p = buf;

	*(p++) = (u8) (addr >> 16);
	*(p++) = (u8) (addr >> 8);
	*(p++) = (u8) (addr);
	ret = avl6882_i2c_wr(priv, buf, 3);
	ret |= avl6882_i2c_rd(priv, buf, reg_size);

	*data = 0;
	p = buf;

	switch (reg_size) {
	case 4:
		*data |= (u32) (*(p++)) << 24;
		*data |= (u32) (*(p++)) << 16;
	case 2:
		*data |= (u32) (*(p++)) << 8;
	case 1:
	default:
		*data |= (u32) *(p);
		break;
	}
	return ret;
}

#define AVL6882_RD_REG8(_priv, _addr, _data) \
	avl6882_i2c_rd_reg(_priv, _addr, _data, 1)
#define AVL6882_RD_REG16(_priv, _addr, _data) \
	avl6882_i2c_rd_reg(_priv, _addr, _data, 2)
#define AVL6882_RD_REG32(_priv, _addr, _data) \
	avl6882_i2c_rd_reg(_priv, _addr, _data, 4)


static int avl6882_setup_pll(struct avl6882_priv *priv)
{
	int ret;

	// sys_pll
	ret = AVL6882_WR_REG32(priv, hw_E2_AVLEM61_sys_pll_divr, 2);
	ret |= AVL6882_WR_REG32(priv, hw_E2_AVLEM61_sys_pll_divf, 99);
	ret |= AVL6882_WR_REG32(priv, hw_E2_AVLEM61_sys_pll_divq, 7);
	ret |= AVL6882_WR_REG32(priv, hw_E2_AVLEM61_sys_pll_range, 1);
	ret |= AVL6882_WR_REG32(priv, hw_E2_AVLEM61_sys_pll_divq2, 11);
	ret |= AVL6882_WR_REG32(priv, hw_E2_AVLEM61_sys_pll_divq3, 13);
	ret |= AVL6882_WR_REG32(priv, hw_E2_AVLEM61_sys_pll_enable2, 0);
	ret |= AVL6882_WR_REG32(priv, hw_E2_AVLEM61_sys_pll_enable3, 0);

	//mpeg_pll
	ret |= AVL6882_WR_REG32(priv, hw_E2_AVLEM61_mpeg_pll_divr, 0);
	ret |= AVL6882_WR_REG32(priv, hw_E2_AVLEM61_mpeg_pll_divf, 35);
	ret |= AVL6882_WR_REG32(priv, hw_E2_AVLEM61_mpeg_pll_divq, 7);
	ret |= AVL6882_WR_REG32(priv, hw_E2_AVLEM61_mpeg_pll_range, 3);
	ret |= AVL6882_WR_REG32(priv, hw_E2_AVLEM61_mpeg_pll_divq2, 11);
	ret |= AVL6882_WR_REG32(priv, hw_E2_AVLEM61_mpeg_pll_divq3, 13);
	ret |= AVL6882_WR_REG32(priv, hw_E2_AVLEM61_mpeg_pll_enable2, 0);
	ret |= AVL6882_WR_REG32(priv, hw_E2_AVLEM61_mpeg_pll_enable3, 0);

	//adc_pll
	ret |= AVL6882_WR_REG32(priv, hw_E2_AVLEM61_adc_pll_divr, 2);
	ret |= AVL6882_WR_REG32(priv, hw_E2_AVLEM61_adc_pll_divf, 99);
	ret |= AVL6882_WR_REG32(priv, hw_E2_AVLEM61_adc_pll_divq, 7);
	ret |= AVL6882_WR_REG32(priv, hw_E2_AVLEM61_adc_pll_range, 1);
	ret |= AVL6882_WR_REG32(priv, hw_E2_AVLEM61_adc_pll_divq2, 11);
	ret |= AVL6882_WR_REG32(priv, hw_E2_AVLEM61_adc_pll_divq3, 13);
	ret |= AVL6882_WR_REG32(priv, hw_E2_AVLEM61_adc_pll_enable2, 1);
	ret |= AVL6882_WR_REG32(priv, hw_E2_AVLEM61_adc_pll_enable3, 1);

	ret |= AVL6882_WR_REG32(priv, hw_E2_AVLEM61_reset_register, 0);
	ret |= AVL6882_WR_REG32(priv, hw_E2_AVLEM61_reset_register, 1);
	msleep(20);

	ret |= AVL6882_WR_REG32(priv, hw_E2_AVLEM61_dll_out_phase, 96);
	ret |= AVL6882_WR_REG32(priv, hw_E2_AVLEM61_dll_rd_phase, 0);
	ret |= AVL6882_WR_REG32(priv, hw_E2_AVLEM61_deglitch_mode, 1);
	ret |= AVL6882_WR_REG32(priv, hw_E2_AVLEM61_dll_init, 1);
	ret |= AVL6882_WR_REG32(priv, hw_E2_AVLEM61_dll_init, 0);
	return ret;
}


#define DEMOD_WAIT_RETRIES	(10)
#define DEMOD_WAIT_MS		(20)
static int avl6882_wait_demod(struct avl6882_priv *priv)
{
	u32 cmd = 0;
	int ret, retry = DEMOD_WAIT_RETRIES;

	do {
		ret = AVL6882_RD_REG16(priv,0x200 + rc_fw_command_saddr_offset, &cmd);
		if ((ret == 0) && (cmd == 0))
			return ret;
		else
			msleep(DEMOD_WAIT_MS);
	} while (--retry);
	ret = -EBUSY;
	return ret;
}

/* TODO remove one of the waits */
static int avl6882_exec_n_wait(struct avl6882_priv *priv, u8 cmd)
{
	int ret;

	ret = avl6882_wait_demod(priv);
	if (ret)
		return ret;
	ret = AVL6882_WR_REG16(priv, 0x200 + rc_fw_command_saddr_offset, (u32) cmd);
	if (ret)
		return ret;
	return avl6882_wait_demod(priv);
}


#define DMA_MAX_TRIES	(20)
static int avl6882_patch_demod(struct avl6882_priv *priv, u32 *patch)
{
	int ret = 0;
	u8 unary_op, binary_op, addr_mode_op;
	u32 cmd, num_cmd_words, next_cmd_idx, num_cond_words, num_rvs;
	u32 condition = 0;
	u32 value = 0;
	u32 operation;
	u32 tmp_top_valid, core_rdy_word;
	u32 exp_crc_val, crc_result;
	u32 data = 0;
	u32 type, ref_addr, ref_size;
	u32 data_section_offset;
	u32 args_addr, src_addr, dest_addr, data_offset, length;
	u32 idx, len, i;
	u32 variable_array[PATCH_VAR_ARRAY_SIZE];

	for(i=0; i<PATCH_VAR_ARRAY_SIZE; i++)
		variable_array[i] = 0;

	//total_patch_len = patch[1];
	//standard = patch[2];
	idx = 3;
	args_addr = patch[idx++];
	data_section_offset = patch[idx++];
	/* reserved length */
	len = patch[idx++];
	idx += len;
	/* script length */
	len = patch[idx++];
	len += idx;

	while (idx < len) {
		num_cmd_words = patch[idx++];
		next_cmd_idx = idx + num_cmd_words - 1;
		num_cond_words = patch[idx++];
		if (num_cond_words == 0) {
			condition = 1;
		} else {
			for (i = 0; i < num_cond_words; i++) {
				operation = patch[idx++];
				value = patch[idx++];
				unary_op = (operation >> 8) & 0xff;
				binary_op = operation & 0xff;
				addr_mode_op = ((operation >> 16) & 0x3);

				if ((addr_mode_op == PATCH_OP_ADDR_MODE_VAR_IDX) &&
				    (binary_op != PATCH_OP_BINARY_STORE)) {
					value = variable_array[value]; //grab variable value
				}

				switch(unary_op) {
				case PATCH_OP_UNARY_LOGICAL_NEGATE:
					value = !value;
					break;
				case PATCH_OP_UNARY_BITWISE_NEGATE:
					value = ~value;
					break;
				default:
					break;
				}
				switch(binary_op) {
				case PATCH_OP_BINARY_LOAD:
					condition = value;
					break;
				case PATCH_OP_BINARY_STORE:
					variable_array[value] = condition;
					break;
				case PATCH_OP_BINARY_AND:
					condition = condition && value;
					break;
				case PATCH_OP_BINARY_OR:
					condition = condition || value;
					break;
				case PATCH_OP_BINARY_BITWISE_AND:
					condition = condition & value;
					break;
				case PATCH_OP_BINARY_BITWISE_OR:
					condition = condition | value;
					break;
				case PATCH_OP_BINARY_EQUALS:
					condition = condition == value;
					break;
				case PATCH_OP_BINARY_NOT_EQUALS:
					condition = condition != value;
					break;
				default:
					break;
				}
			}
		}

		AVL6882_RD_REG32(priv, 0x29A648, &tmp_top_valid);
		AVL6882_RD_REG32(priv, 0x0A0, &core_rdy_word);

		if (condition) {
			cmd = patch[idx++];
			switch(cmd) {
			case PATCH_CMD_PING:
				ret = avl6882_exec_n_wait(priv, AVL_FW_CMD_PING);
				num_rvs = patch[idx++];
				i = patch[idx];
				variable_array[i] = (ret == 0);
				break;
			case PATCH_CMD_VALIDATE_CRC:
				exp_crc_val = patch[idx++];
				src_addr = patch[idx++];
				length = patch[idx++];
				AVL6882_WR_REG32(priv,0x200 + rc_fw_command_args_addr_iaddr_offset, args_addr);
				AVL6882_WR_REG32(priv,args_addr+0, src_addr);
				AVL6882_WR_REG32(priv,args_addr+4, length);
				ret = avl6882_exec_n_wait(priv, AVL_FW_CMD_CALC_CRC);
				AVL6882_RD_REG32(priv,args_addr+8, &crc_result);
				num_rvs = patch[idx++];
				i = patch[idx];
				variable_array[i] = (crc_result == exp_crc_val);
				break;
			case PATCH_CMD_LD_TO_DEVICE:
				length = patch[idx++];
				dest_addr = patch[idx++];
				data_offset = patch[idx++];
				data_offset += data_section_offset;
				ret = avl6882_i2c_wr_data(priv, dest_addr, &patch[data_offset], length);
				num_rvs = patch[idx++];
				break;
			case PATCH_CMD_LD_TO_DEVICE_IMM:
				length = patch[idx++];
				dest_addr = patch[idx++];
				data = patch[idx++];
				ret = avl6882_i2c_wr_reg(priv, dest_addr, data, length);
				num_rvs = patch[idx++];
				break;
			case PATCH_CMD_RD_FROM_DEVICE:
				length = patch[idx++];
				src_addr = patch[idx++];
				num_rvs = patch[idx++];
				ret = avl6882_i2c_rd_reg(priv, src_addr, &data, length);
				i = patch[idx];
				variable_array[i] = data;
				break;
			case PATCH_CMD_DMA:
				dest_addr = patch[idx++];
				length = patch[idx++];
				if (length > 0)
					ret = avl6882_i2c_wr_data(priv, dest_addr, &patch[idx], length * 3);
				AVL6882_WR_REG32(priv,0x200 + rc_fw_command_args_addr_iaddr_offset, dest_addr);
				ret = avl6882_exec_n_wait(priv,AVL_FW_CMD_DMA);
				idx += length * 3;
				num_rvs = patch[idx++];
				break;
			case PATCH_CMD_DECOMPRESS:
				type = patch[idx++];
				src_addr = patch[idx++];
				dest_addr = patch[idx++];
				if(type == PATCH_CMP_TYPE_ZLIB) {
					ref_addr = patch[idx++];
					ref_size = patch[idx++];
				}
				AVL6882_WR_REG32(priv,0x200 + rc_fw_command_args_addr_iaddr_offset, args_addr);
				AVL6882_WR_REG32(priv,args_addr+0, type);
				AVL6882_WR_REG32(priv,args_addr+4, src_addr);
				AVL6882_WR_REG32(priv,args_addr+8, dest_addr);
				if(type == PATCH_CMP_TYPE_ZLIB) {
					AVL6882_WR_REG32(priv,args_addr+12, ref_addr);
					AVL6882_WR_REG32(priv,args_addr+16, ref_size);
				}
				ret = avl6882_exec_n_wait(priv,AVL_FW_CMD_DECOMPRESS);
				num_rvs = patch[idx++];
				break;
			case PATCH_CMD_ASSERT_CPU_RESET:
				ret |= AVL6882_WR_REG32(priv,0x110840, 1);
				num_rvs = patch[idx++];
				break;
			case PATCH_CMD_RELEASE_CPU_RESET:
				AVL6882_WR_REG32(priv, 0x110840, 0);
				num_rvs = patch[idx++];
				break;
			case PATCH_CMD_DMA_HW:
				dest_addr = patch[idx++];
				length = patch[idx++];
				if (length > 0)
					ret = avl6882_i2c_wr_data(priv, dest_addr, &patch[idx], length * 3);
				i = 0;
				do {
					if (i++ > DMA_MAX_TRIES)
						return -ENODEV;
					ret |= AVL6882_RD_REG32(priv, 0x110048, &data);
				} while (!(0x01 & data));

				if (data)
					ret |= AVL6882_WR_REG32(priv, 0x110050, dest_addr);
				idx += length * 3;
				num_rvs = patch[idx++];
				break;
			case PATCH_CMD_SET_COND_IMM:
				data = patch[idx++];
				num_rvs = patch[idx++];
				i = patch[idx];
				variable_array[i] = data;
				break;
			default:
				return -ENODEV;
				break;
			}
			idx += num_rvs;
		} else {
			idx = next_cmd_idx;
			continue;
		}
	}

	return ret;
}

#define DEMOD_WAIT_RETRIES_BOOT	(100)
#define DEMOD_WAIT_MS_BOOT	(20)
static int avl6882_wait_demod_boot(struct avl6882_priv *priv)
{
	int ret, retry = DEMOD_WAIT_RETRIES_BOOT;
	u32 ready_code = 0;
	u32 status = 0;

	do {
		ret = AVL6882_RD_REG32(priv, 0x110840, &status);
		ret |= AVL6882_RD_REG32(priv, rs_core_ready_word_iaddr_offset, &ready_code);
		if ((ret == 0) && (status == 0) && (ready_code == 0x5aa57ff7))
			return ret;
		else
			msleep(DEMOD_WAIT_MS_BOOT);
	} while (--retry);
	ret = -EBUSY;
	return ret;
}


/* firmware loader */
/* TODO: change to firmware loading from /lib/firmware */
static int avl6882_load_firmware(struct avl6882_priv *priv)
{
	int ret = 0;
	u8 *fw_data;
	u32 *patch, *ptr;
	u32 i, fw_size;

	switch (priv->delivery_system) {
	case SYS_DVBC_ANNEX_A:
		fw_data = AVL_Demod_Patch_DVBCFw;
		fw_size = sizeof(AVL_Demod_Patch_DVBCFw);
		break;
	case SYS_DVBS:
	case SYS_DVBS2:
		fw_data = AVL_Demod_Patch_DVBSxFw;
		fw_size = sizeof(AVL_Demod_Patch_DVBSxFw);
		break;
	case SYS_DVBT:
	case SYS_DVBT2:
	default:
		fw_data = AVL_Demod_Patch_DVBTxFw;
		fw_size = sizeof(AVL_Demod_Patch_DVBTxFw);
		break;
	}

	fw_size &= 0xfffffffc;
	patch = kzalloc(fw_size, GFP_KERNEL);
	if (patch == NULL) {
	  	ret = -ENOMEM;
		goto err;
	}

	ptr = patch;
	for (i = 0; i < fw_size; i += 4) {
	  	*(ptr++) = (fw_data[i]   << 24) |
			   (fw_data[i+1] << 16) |
			   (fw_data[i+2] <<  8) |
			   fw_data[i+3];
	}

	/* check valid FW */
	if ((patch[0] & 0xf0000000) != 0x10000000) {
		ret = -EINVAL;
		goto err1;
	}
	ret |= AVL6882_WR_REG32(priv,0x110010, 1);

	// Configure the PLL
	ret |= avl6882_setup_pll(priv);
	if (ret)
		goto err1;

	ret |= AVL6882_WR_REG32(priv, 0x0a4 + rs_core_ready_word_iaddr_offset, 0x00000000);
	ret |= AVL6882_WR_REG32(priv, 0x110010, 0);

	if ((patch[0] & 0xff) == 1) /* patch version */
		ret |= avl6882_patch_demod(priv, patch);
	else
		ret = -EINVAL;
	if (ret)
		return ret;

	ret = avl6882_wait_demod_boot(priv);
err1:
	kfree(patch);
err:
	return ret;
}



int  ErrorStatMode_Demod( struct avl6882_priv *priv,AVL_ErrorStatConfig stErrorStatConfig )
{
	int r = AVL_EC_OK;
	u64 time_tick_num = 270000 *  stErrorStatConfig.uiTimeThresholdMs;

	r = AVL6882_WR_REG32(priv,0x132050 + esm_mode_offset,(u32) stErrorStatConfig.eErrorStatMode);
	r |= AVL6882_WR_REG32(priv,0x132050 + tick_type_offset,(u32) stErrorStatConfig.eAutoErrorStatType);

	r |= AVL6882_WR_REG32(priv,0x132050 + time_tick_low_offset, (u32) (time_tick_num));
	r |= AVL6882_WR_REG32(priv,0x132050 + time_tick_high_offset, (u32) (time_tick_num >> 32));

	r |= AVL6882_WR_REG32(priv,0x132050 + byte_tick_low_offset, stErrorStatConfig.uiTimeThresholdMs);
	r |= AVL6882_WR_REG32(priv,0x132050 + byte_tick_high_offset, 0);//high 32-bit is not used

	if(stErrorStatConfig.eErrorStatMode == AVL_ERROR_STAT_AUTO)//auto mode
	{
		//reset auto error stat
		r |= AVL6882_WR_REG32(priv,0x132050 + tick_clear_offset,0);
		r |= AVL6882_WR_REG32(priv,0x132050 + tick_clear_offset,1);
		r |= AVL6882_WR_REG32(priv,0x132050 + tick_clear_offset,0);
	}

	return (r);
}


int  ResetPER_Demod(  struct avl6882_priv *priv)
{
	int r = AVL_EC_OK;
	u32 uiTemp = 0;

	r |= AVL6882_RD_REG32(priv,0x132050 + esm_cntrl_offset, &uiTemp);
	uiTemp |= 0x00000001;
	r |= AVL6882_WR_REG32(priv,0x132050 + esm_cntrl_offset, uiTemp);

	r |= AVL6882_RD_REG32(priv,0x132050 + esm_cntrl_offset, &uiTemp);
	uiTemp |= 0x00000008;
	r |= AVL6882_WR_REG32(priv,0x132050 + esm_cntrl_offset, uiTemp);
	uiTemp |= 0x00000001;
	r |= AVL6882_WR_REG32(priv,0x132050 + esm_cntrl_offset, uiTemp);
	uiTemp &= 0xFFFFFFFE;
	r |= AVL6882_WR_REG32(priv,0x132050 + esm_cntrl_offset, uiTemp);

	return r;
}

static int InitErrorStat_Demod( struct avl6882_priv *priv )
{
	int r = AVL_EC_OK;
	AVL_ErrorStatConfig stErrorStatConfig;

	stErrorStatConfig.eErrorStatMode = AVL_ERROR_STAT_AUTO;
	stErrorStatConfig.eAutoErrorStatType = AVL_ERROR_STAT_TIME;
	stErrorStatConfig.uiTimeThresholdMs = 3000;
	stErrorStatConfig.uiNumberThresholdByte = 0;

	r = ErrorStatMode_Demod(priv,stErrorStatConfig);
	r |= ResetPER_Demod(priv);

	return r;
}





int  DVBSx_Diseqc_Initialize_Demod( struct avl6882_priv *priv,AVL_Diseqc_Para *pDiseqcPara)
{
	int r = AVL_EC_OK;
	u32 i1 = 0;

	r |= AVL6882_WR_REG32(priv,0x16c000 + hw_diseqc_srst_offset, 1);

	r |= AVL6882_WR_REG32(priv,0x16c000 + hw_diseqc_samp_frac_n_offset, 2000000); 	  //2M=200*10kHz
	r |= AVL6882_WR_REG32(priv,0x16c000 + hw_diseqc_samp_frac_d_offset, 166666667);  //uiDDCFrequencyHz  166666667

	r |= AVL6882_WR_REG32(priv,0x16c000 + hw_diseqc_tone_frac_n_offset, ((pDiseqcPara->uiToneFrequencyKHz)<<1));
	r |= AVL6882_WR_REG32(priv,0x16c000 + hw_diseqc_tone_frac_d_offset, (166666667/1000));//uiDDCFrequencyHz  166666667

	// Initialize the tx_control
	r |= AVL6882_RD_REG32(priv,0x16c000 + hw_diseqc_tx_cntrl_offset, &i1);
	i1 &= 0x00000300;
	i1 |= 0x20; 	//reset tx_fifo
	i1 |= ((u32)(pDiseqcPara->eTXGap) << 6);
	i1 |= ((u32)(pDiseqcPara->eTxWaveForm) << 4);
	i1 |= (1<<3);			//enable tx gap.
	r |= AVL6882_WR_REG32(priv,0x16c000 + hw_diseqc_tx_cntrl_offset, i1);
	i1 &= ~(0x20);	//release tx_fifo reset
	r |= AVL6882_WR_REG32(priv,0x16c000 + hw_diseqc_tx_cntrl_offset, i1);

	// Initialize the rx_control
	i1 = ((u32)(pDiseqcPara->eRxWaveForm) << 2);
	i1 |= (1<<1);	//active the receiver
	i1 |= (1<<3);	//envelop high when tone present
	r |= AVL6882_WR_REG32(priv,0x16c000 + hw_diseqc_rx_cntrl_offset, i1);
	i1 = (u32)(pDiseqcPara->eRxTimeout);
	r |= AVL6882_WR_REG32(priv,0x16c000 + hw_diseqc_rx_msg_tim_offset, i1);

	r |= AVL6882_WR_REG32(priv,0x16c000 + hw_diseqc_srst_offset, 0);

	if( AVL_EC_OK == r )
	{
		priv->config->eDiseqcStatus = AVL_DOS_Initialized;
	}

	return (r);
}


static int avl6882_init_dvbs(struct dvb_frontend *fe)
{
	struct avl6882_priv *priv = fe->demodulator_priv;
	int ret;
	AVL_Diseqc_Para stDiseqcConfig;

	ret = AVL6882_WR_REG16(priv, 0xe00 + rc_DVBSx_int_mpeg_clk_MHz_saddr_offset,27000);
	ret |= AVL6882_WR_REG16(priv, 0xe00 + rc_DVBSx_int_fec_clk_MHz_saddr_offset,25000);

	ret |= AVL6882_WR_REG16(priv, 0xe00 + rc_DVBSx_int_adc_clk_MHz_saddr_offset,12500);// uiADCFrequencyHz  125000000
	ret |= AVL6882_WR_REG16(priv, 0xe00 + rc_DVBSx_int_dmd_clk_MHz_saddr_offset,166666667/10000); //uiDDCFrequencyHz  166666667

	ret |= AVL6882_WR_REG32(priv, 0xe00 + rc_DVBSx_rfagc_pol_iaddr_offset,AVL_AGC_INVERTED);

	ret |= AVL6882_WR_REG32(priv, 0xe00 + rc_DVBSx_format_iaddr_offset, AVL_OFFBIN);//Offbin
	ret |= AVL6882_WR_REG32(priv, 0xe00 + rc_DVBSx_input_iaddr_offset, AVL_ADC_IN);//ADC in

	ret |= AVL6882_WR_REG16(priv, 0xe00 + rc_DVBSx_IF_Offset_10kHz_saddr_offset,0);

	/* enble agc */
	ret |= AVL6882_WR_REG32(priv, REG_GPIO_BASE + GPIO_AGC_DVBS, 6);

	stDiseqcConfig.eRxTimeout = AVL_DRT_150ms;
	stDiseqcConfig.eRxWaveForm = AVL_DWM_Normal;
	stDiseqcConfig.uiToneFrequencyKHz = 22;
	stDiseqcConfig.eTXGap = AVL_DTXG_15ms;
	stDiseqcConfig.eTxWaveForm = AVL_DWM_Normal;

	ret |= DVBSx_Diseqc_Initialize_Demod(priv, &stDiseqcConfig);
	return ret;
}


static int avl6882_init_dvbc(struct dvb_frontend *fe)
{
	struct avl6882_priv *priv = fe->demodulator_priv;
	int ret;

	ret = AVL6882_WR_REG32(priv, 0x600 + rc_DVBC_dmd_clk_Hz_iaddr_offset, 250000000);
	ret |= AVL6882_WR_REG32(priv, 0x600 + rc_DVBC_fec_clk_Hz_iaddr_offset, 250000000);
	ret |= AVL6882_WR_REG8(priv, 0x600 + rc_DVBC_rfagc_pol_caddr_offset,AVL_AGC_NORMAL);
	ret |= AVL6882_WR_REG32(priv, 0x600 + rc_DVBC_if_freq_Hz_iaddr_offset, 5000000);
	ret |= AVL6882_WR_REG8(priv, 0x600 + rc_DVBC_adc_sel_caddr_offset, (u8) AVL_IF_Q);
	ret |= AVL6882_WR_REG32(priv, 0x600 + rc_DVBC_symbol_rate_Hz_iaddr_offset, 6875000);
	ret |= AVL6882_WR_REG8(priv, 0x600 + rc_DVBC_j83b_mode_caddr_offset, AVL_DVBC_J83A);

	//DDC configuration
	ret |= AVL6882_WR_REG8(priv, 0x600 + rc_DVBC_input_format_caddr_offset, AVL_ADC_IN); //ADC in
	ret |= AVL6882_WR_REG8(priv, 0x600 + rc_DVBC_input_select_caddr_offset, AVL_OFFBIN); //RX_OFFBIN
	ret |= AVL6882_WR_REG8(priv, 0x600 + rc_DVBC_tuner_type_caddr_offset, AVL_DVBC_IF); //IF

	//ADC configuration
	ret |= AVL6882_WR_REG8(priv, 0x600 + rc_DVBC_adc_use_pll_clk_caddr_offset, 0);
	ret |= AVL6882_WR_REG32(priv, 0x600 + rc_DVBC_sample_rate_Hz_iaddr_offset, 30000000);

	/* enable agc */
    	ret |= AVL6882_WR_REG32(priv, REG_GPIO_BASE + GPIO_AGC_DVBTC, 6);

	return ret;
}


static int avl6882_init_dvbt(struct dvb_frontend *fe)
{
	struct avl6882_priv *priv = fe->demodulator_priv;
	int ret;

	ret = AVL6882_WR_REG32(priv, 0xa00 + rc_DVBTx_sample_rate_Hz_iaddr_offset, 30000000);
	ret |= AVL6882_WR_REG32(priv, 0xa00 + rc_DVBTx_mpeg_clk_rate_Hz_iaddr_offset, 270000000);

	/* DDC configuration */
	ret |= AVL6882_WR_REG8(priv, 0xa00 + rc_DVBTx_input_format_caddr_offset, AVL_OFFBIN);
	ret |= AVL6882_WR_REG8(priv, 0xa00 + rc_DVBTx_input_select_caddr_offset, AVL_ADC_IN);
	ret |= AVL6882_WR_REG8(priv, 0xa00 + rc_DVBTx_tuner_type_caddr_offset, AVL_DVBTX_REAL_IF);
	ret |= AVL6882_WR_REG8(priv, 0xa00 + rc_DVBTx_rf_agc_pol_caddr_offset, 0);
	ret |= AVL6882_WR_REG32(priv, 0xa00 + rc_DVBTx_nom_carrier_freq_Hz_iaddr_offset, 5000000);

	/* ADC configuration */
	ret |= AVL6882_WR_REG8(priv, 0xa00 + rc_DVBTx_adc_sel_caddr_offset, (u8)AVL_IF_Q);
	ret |= AVL6882_WR_REG8(priv, 0xa00 + rc_DVBTx_adc_use_pll_clk_caddr_offset, 0);

	/* enable agc */
    	ret |= AVL6882_WR_REG32(priv, REG_GPIO_BASE + GPIO_AGC_DVBTC, 6);
	return ret;
}


static int avl6882_read_status(struct dvb_frontend *fe, enum fe_status *status)
{
	struct avl6882_priv *priv = fe->demodulator_priv;
	int ret;
	u32 reg;

	*status = 0;

	switch (priv->delivery_system) {
	case SYS_DVBC_ANNEX_A:
		ret = AVL6882_RD_REG32(priv,0x400 + rs_DVBC_mode_status_iaddr_offset, &reg);
		if ((reg & 0xff) == 0x15)
			reg = 1;
		else
		  	reg = 0;
		break;
	case SYS_DVBS:
	case SYS_DVBS2:
		ret = AVL6882_RD_REG16(priv, 0xc00 + rs_DVBSx_fec_lock_saddr_offset, &reg);
		break;
	case SYS_DVBT:
	case SYS_DVBT2:
	default:
		ret = AVL6882_RD_REG8(priv, 0x800 + rs_DVBTx_fec_lock_caddr_offset, &reg);
		break;
	}
	if (ret) {
	  	*status = 0;
		return ret;
	}

	if (reg)
		*status = FE_HAS_SIGNAL | FE_HAS_CARRIER |
			FE_HAS_VITERBI | FE_HAS_SYNC | FE_HAS_LOCK;

	return ret;
}




static int avl6882_i2c_gate_ctrl(struct dvb_frontend *fe, int enable)
{
	struct avl6882_priv *priv = fe->demodulator_priv;
	int ret;

	dev_dbg(&priv->i2c->dev, "%s: %d\n", __func__, enable);

	if (enable) {
		ret = AVL6882_WR_REG32(priv,0x118000 + tuner_i2c_bit_rpt_cntrl_offset, 0x07);
		ret = AVL6882_WR_REG32(priv,0x118000 + tuner_i2c_bit_rpt_cntrl_offset, 0x07);
	} else
		ret = AVL6882_WR_REG32(priv,0x118000 + tuner_i2c_bit_rpt_cntrl_offset, 0x06);

	return ret;
}


static int avl6882_set_dvbs(struct dvb_frontend *fe)
{
	struct avl6882_priv *priv = fe->demodulator_priv;
	struct dtv_frontend_properties *c = &fe->dtv_property_cache;
	int ret;

	//printk("[avl6882_set_dvbs] Freq:%d Mhz,sym:%d Khz\n", c->frequency, c->symbol_rate);

	ret = AVL6882_WR_REG16(priv, 0xc00 + rs_DVBSx_fec_lock_saddr_offset, 0);
	ret |= AVL6882_WR_REG16(priv, 0xe00 + rc_DVBSx_decode_mode_saddr_offset, 0x14);
	ret |= AVL6882_WR_REG16(priv, 0xe00 + rc_DVBSx_fec_bypass_coderate_saddr_offset, 0); //DVBS auto lock
	ret |= AVL6882_WR_REG16(priv, 0xe00 + rc_DVBSx_iq_mode_saddr_offset, 1); //enable spectrum auto detection
	ret |= AVL6882_WR_REG16(priv, 0xe00 + rc_DVBSx_decode_mode_saddr_offset, 0x14);
	ret |= AVL6882_WR_REG16(priv, 0xe00 + rc_DVBSx_fec_bypass_coderate_saddr_offset, 0);
	ret |= AVL6882_WR_REG32(priv, 0xe00 + rc_DVBSx_int_sym_rate_MHz_iaddr_offset, c->symbol_rate);
	ret |= avl6882_exec_n_wait(priv,AVL_FW_CMD_ACQUIRE);
	return ret;
}


static int avl6882_set_dvbc(struct dvb_frontend *fe)
{
	struct avl6882_priv *priv = fe->demodulator_priv;
	struct dtv_frontend_properties *c = &fe->dtv_property_cache;
	int ret;

	//printk("[avl6882_set_dvbc] Freq:%d Mhz,sym:%d\n", c->frequency, c->symbol_rate);

	ret = AVL6882_WR_REG32(priv, 0x600 + rc_DVBC_qam_mode_scan_control_iaddr_offset, 0x0101);
	ret |= AVL6882_WR_REG32(priv, 0x600 + rc_DVBC_symbol_rate_Hz_iaddr_offset, c->symbol_rate);
	ret |= avl6882_exec_n_wait(priv, AVL_FW_CMD_ACQUIRE);
	return ret;
}


static int avl6882_set_dvbt(struct dvb_frontend *fe)
{
	struct avl6882_priv *priv = fe->demodulator_priv;
	struct dtv_frontend_properties *c = &fe->dtv_property_cache;
	u32 bw_fft;
	int ret;

	//printk("[avl6882_set_dvbtx] Freq:%d bw:%d\n", c->frequency, c->bandwidth_hz);

	/* set bandwidth */
	if(c->bandwidth_hz <= 1700000) {
		bw_fft = 1845070;
	} else if(c->bandwidth_hz <= 5000000) {
		bw_fft = 5714285;
	} else if(c->bandwidth_hz <= 6000000) {
		bw_fft = 6857143;
	} else if(c->bandwidth_hz <= 7000000) {
		bw_fft = 8000000;
	} else { // if(c->bandwidth_hz <= 8000) {
		bw_fft = 9142857;
	}
    	ret = AVL6882_WR_REG32(priv, 0xa00 + rc_DVBTx_fund_rate_Hz_iaddr_offset, bw_fft);
	ret |= AVL6882_WR_REG8(priv, 0xa00 + rc_DVBTx_l1_proc_only_caddr_offset, 0);

	/* spectrum inversion */
	ret |= AVL6882_WR_REG8(priv, 0xa00 + rc_DVBTx_spectrum_invert_caddr_offset, AVL_SPECTRUM_AUTO);

	switch (c->delivery_system) {
	case SYS_DVBT:
		ret |= AVL6882_WR_REG8(priv, 0xa00 + rc_DVBTx_acquire_mode_caddr_offset, (u8) AVL_DVBTx_LockMode_T_ONLY);
		ret |= AVL6882_WR_REG8(priv, 0xa00 + rc_DVBTx_dvbt_layer_select_caddr_offset, 0);
		ret |= AVL6882_WR_REG8(priv, 0xa00 + rc_DVBTx_data_PLP_ID_caddr_offset, 0);
		ret |= AVL6882_WR_REG8(priv, 0xa00 + rc_DVBTx_common_PLP_ID_caddr_offset, 0);
		ret |= AVL6882_WR_REG8(priv, 0xa00 + rc_DVBTx_common_PLP_present_caddr_offset, 0);
		break;
	case SYS_DVBT2:
	default:
		ret |= AVL6882_WR_REG8(priv, 0xa00 + rc_DVBTx_acquire_mode_caddr_offset, AVL_DVBTx_LockMode_ALL);
		ret |= AVL6882_WR_REG8(priv, 0xa00 + rc_DVBTx_data_PLP_ID_caddr_offset, c->stream_id);
		ret |= AVL6882_WR_REG8(priv, 0xa00 + rc_DVBTx_common_PLP_ID_caddr_offset, 0);
		ret |= AVL6882_WR_REG8(priv, 0xa00 + rc_DVBTx_common_PLP_present_caddr_offset, 2);
		break;
	}
	ret |= avl6882_exec_n_wait(priv, AVL_FW_CMD_ACQUIRE);
	return ret;
}

static int avl6882_read_ucblocks(struct dvb_frontend *fe, u32 *ucblocks)
{
	//printk("%s()\n", __func__);
	*ucblocks = 0x00;
	return 0;
}

static void avl6882_release(struct dvb_frontend *fe)
{
	struct avl6882_priv *priv = fe->demodulator_priv;
	//printk("%s()\n", __func__);
	kfree(priv);
}

static int avl6882_read_ber(struct dvb_frontend *fe, u32 *ber)
{
	//printk("%s()\n", __func__);
	return 0;
}

static int avl6882_burst(struct dvb_frontend *fe, enum fe_sec_mini_cmd burst)
{
	//printk("%s()\n", __func__);
	return 0;
}

static int avl6882_set_tone(struct dvb_frontend* fe, enum fe_sec_tone_mode tone)
{
	struct avl6882_priv *priv = fe->demodulator_priv;
	int ret;
	u32 reg;

	//printk("set tone: %d\n",tone);

	ret = AVL6882_RD_REG32(priv, 0x16c000 + hw_diseqc_tx_cntrl_offset, &reg);
	if (ret)
		return ret;

	switch(tone) {
	case SEC_TONE_ON:
		reg &= 0xfffffff8;
		reg |= 0x3;	// continuous mode
		reg |= (1<<10);	// on
		break;
	case SEC_TONE_OFF:
		reg &= 0xfffff3ff;
		break;
	default:
		return -EINVAL;
	}
	return AVL6882_WR_REG32(priv,0x16c000 + hw_diseqc_tx_cntrl_offset, reg);
}

static int avl6882_set_voltage(struct dvb_frontend* fe, enum fe_sec_voltage voltage)
{
	struct avl6882_priv *priv = fe->demodulator_priv;
	u32 pwr, vol;
	int ret;

	//printk("set voltage: %d\n",voltage);

	switch (voltage) {
	case SEC_VOLTAGE_OFF:
		pwr = GPIO_1;
		vol = GPIO_0;
		break;
	case SEC_VOLTAGE_13:
		//power on
		pwr = GPIO_0;
		vol = GPIO_0;
		break;
	case SEC_VOLTAGE_18:
		//power on
		pwr = GPIO_0;
		vol = GPIO_Z;
		break;
	default:
		return -EINVAL;
	}
	ret = AVL6882_WR_REG32(priv, REG_GPIO_BASE + GPIO_LNB_PWR, pwr);
	ret |= AVL6882_WR_REG32(priv, REG_GPIO_BASE + GPIO_LNB_VOLTAGE, vol);
	return ret;
}

static int avl6882_init(struct dvb_frontend *fe)
{
	return 0;
}


#define I2C_RPT_DIV ((0x2A)*(250000)/(240*1000))	//m_CoreFrequency_Hz 250000000

static int avl6882_set_dvbmode(struct dvb_frontend *fe,
		enum fe_delivery_system delsys)
{
	struct avl6882_priv *priv = fe->demodulator_priv;
	int ret;
	u32 reg;

	/* these modes use the same fw / config */
	if (delsys == SYS_DVBS2)
		delsys = SYS_DVBS;
	else if (delsys == SYS_DVBT2)
		delsys = SYS_DVBT;

	/* already in desired mode */
	if (priv->delivery_system == delsys)
		return 0;

	priv->delivery_system = delsys;
	//printk("initing demod for delsys=%d\n", delsys);

	ret = avl6882_load_firmware(priv);

	// Load the default configuration
	ret |= avl6882_exec_n_wait(priv, AVL_FW_CMD_LD_DEFAULT);
	ret |= avl6882_exec_n_wait(priv, AVL_FW_CMD_INIT_SDRAM);
	ret |= avl6882_exec_n_wait(priv, AVL_FW_CMD_INIT_ADC);

	switch (priv->delivery_system) {
	case SYS_DVBC_ANNEX_A:
		ret |= avl6882_init_dvbc(fe);
		break;
	case SYS_DVBS:
	case SYS_DVBS2:
		ret |= avl6882_init_dvbs(fe);
		break;
	case SYS_DVBT:
	case SYS_DVBT2:
	default:
		ret |= avl6882_init_dvbt(fe);
		break;
	}

	/* set gpio / turn off lnb, set 13V */
	ret = AVL6882_WR_REG32(priv, REG_GPIO_BASE + GPIO_LNB_PWR, GPIO_1);
	ret |= AVL6882_WR_REG32(priv, REG_GPIO_BASE + GPIO_LNB_VOLTAGE, GPIO_0);

	/* set TS mode */
	ret |= AVL6882_WR_REG8(priv,0x200 + rc_ts_serial_caddr_offset, AVL_TS_PARALLEL);
	ret |= AVL6882_WR_REG8(priv,0x200 + rc_ts_clock_edge_caddr_offset, AVL_MPCM_RISING);
	ret |= AVL6882_WR_REG8(priv,0x200 + rc_enable_ts_continuous_caddr_offset, AVL_TS_CONTINUOUS_ENABLE);

	/* TS serial pin */
	ret |= AVL6882_WR_REG8(priv,0x200 + rc_ts_serial_outpin_caddr_offset, AVL_MPSP_DATA0);
	/* TS serial order */
	ret |= AVL6882_WR_REG8(priv,0x200 + rc_ts_serial_msb_caddr_offset, AVL_MPBO_MSB);
	/* TS serial sync pulse */
	ret |= AVL6882_WR_REG8(priv,0x200 + rc_ts_sync_pulse_caddr_offset, AVL_TS_SERIAL_SYNC_1_PULSE);
	/* TS error pol */
	ret |= AVL6882_WR_REG8(priv,0x200 + rc_ts_error_polarity_caddr_offset, AVL_MPEP_Normal);
	/* TS valid pol */
	ret |= AVL6882_WR_REG8(priv,0x200 + rc_ts_valid_polarity_caddr_offset, AVL_MPVP_Normal);
	/* TS packet len */
	ret |= AVL6882_WR_REG8(priv,0x200 + rc_ts_packet_len_caddr_offset, AVL_TS_188);
	/* TS parallel order */
	ret |= AVL6882_WR_REG8(priv,0x200 + rc_ts_packet_order_caddr_offset, AVL_TS_PARALLEL_ORDER_NORMAL);
	/* TS parallel phase */
	ret |= AVL6882_WR_REG8(priv,0x200 + ts_clock_phase_caddr_offset, AVL_TS_PARALLEL_PHASE_0);

	/* TS output enable */
	ret |= AVL6882_WR_REG32(priv, REG_TS_OUTPUT, TS_OUTPUT_ENABLE);

	/* init tuner i2c repeater */
	/* hold in reset */
	ret |= AVL6882_WR_REG32(priv, 0x118000 + tuner_i2c_srst_offset, 1);
	/* close gate */
	ret |= AVL6882_WR_REG32(priv, 0x118000 + tuner_i2c_bit_rpt_cntrl_offset, 0x6);
	ret |= AVL6882_RD_REG32(priv, 0x118000 + tuner_i2c_cntrl_offset, &reg);
	reg &= 0xfffffffe;
	ret |= AVL6882_WR_REG32(priv, 0x118000 + tuner_i2c_cntrl_offset, reg);
	/* set bit clock */
	ret |= AVL6882_WR_REG32(priv, 0x118000 + tuner_i2c_bit_rpt_clk_div_offset, I2C_RPT_DIV);
	/* release from reset */
	ret |= AVL6882_WR_REG32(priv, 0x118000 + tuner_i2c_srst_offset, 0);

	ret |= InitErrorStat_Demod(priv);

	if (ret) {
		dev_err(&priv->i2c->dev, "%s: demod init failed",
				KBUILD_MODNAME);
	}

	return ret;
}

static int avl6882_sleep(struct dvb_frontend *fe)
{
	//printk("%s()\n", __func__);
	return 0;
}


static int avl6882fe_strength(struct dvb_frontend *fe, u16 *signal_strength)
{
	*signal_strength = 0x1234;
	return 0;
}

static int avl6882fe_snr(struct dvb_frontend *fe, u16 *snr)
{
	*snr = 0x1234;
	return 0;
}

static int avl6882_diseqc(struct dvb_frontend *fe,
	struct dvb_diseqc_master_cmd *d)
{
	//printk("%s()\n", __func__);
	return 0;
}

static int avl6882fe_algo(struct dvb_frontend *fe)
{
	return DVBFE_ALGO_HW;
}

static int avl6882_set_frontend(struct dvb_frontend *fe)
{
	struct avl6882_priv *priv = fe->demodulator_priv;
	struct dtv_frontend_properties *c = &fe->dtv_property_cache;
	u32 demod_mode;
	int ret;

	//printk("%s()\n", __func__);

	/* check that mode is correctly set */
	ret = AVL6882_RD_REG32(priv, 0x200 + rs_current_active_mode_iaddr_offset, &demod_mode);
	if (ret)
		return ret;

	/* setup tuner */
	if (fe->ops.tuner_ops.set_params) {
		if (fe->ops.i2c_gate_ctrl)
			fe->ops.i2c_gate_ctrl(fe, 1);
		ret = fe->ops.tuner_ops.set_params(fe);
		if (fe->ops.i2c_gate_ctrl)
			fe->ops.i2c_gate_ctrl(fe, 0);

		if (ret)
			return ret;
	}

	switch (c->delivery_system) {
	case SYS_DVBT:
	case SYS_DVBT2:
		if (demod_mode != AVL_DVBTX) {
			dev_err(&priv->i2c->dev, "%s: failed to enter DVBTx mode",
				KBUILD_MODNAME);
			ret = -EAGAIN;
			break;
		}
		ret = avl6882_set_dvbt(fe);
		break;
	case SYS_DVBC_ANNEX_A:
		if (demod_mode != AVL_DVBC) {
			dev_err(&priv->i2c->dev, "%s: failed to enter DVBC mode",
				KBUILD_MODNAME);
			ret = -EAGAIN;
			break;
		}
		ret = avl6882_set_dvbc(fe);
		break;
	case SYS_DVBS:
	case SYS_DVBS2:
		if (demod_mode != AVL_DVBSX) {
			dev_err(&priv->i2c->dev, "%s: failed to enter DVBSx mode",
				KBUILD_MODNAME);
			ret = -EAGAIN;
			break;
		}
		ret = avl6882_set_dvbs(fe);
		break;
	default:
		ret = -EINVAL;
		break;
	}

	return ret;
}

static int avl6882_tune(struct dvb_frontend *fe, bool re_tune,
	unsigned int mode_flags, unsigned int *delay, enum fe_status *status)
{
	*delay = HZ / 5;
	if (re_tune) {
		int ret = avl6882_set_frontend(fe);
		if (ret)
			return ret;
	}
	return avl6882_read_status(fe, status);
}

static int avl6882_get_frontend(struct dvb_frontend *fe)
{
	return 0;
}

static int avl6882_set_property(struct dvb_frontend *fe,
		struct dtv_property *p)
{
	int ret = 0;

	switch (p->cmd) {
	case DTV_DELIVERY_SYSTEM:
		ret = avl6882_set_dvbmode(fe, p->u.data);
		switch (p->u.data) {
		case SYS_DVBC_ANNEX_A:
			fe->ops.info.frequency_min = 47000000;
			fe->ops.info.frequency_max = 862000000;
			fe->ops.info.frequency_stepsize = 62500;
			break;
		case SYS_DVBS:
		case SYS_DVBS2:
			fe->ops.info.frequency_min = 950000;
			fe->ops.info.frequency_max = 2150000;
			fe->ops.info.frequency_stepsize = 0;
			break;
		case SYS_DVBT:
		case SYS_DVBT2:
		default:
			fe->ops.info.frequency_min = 174000000;
			fe->ops.info.frequency_max = 862000000;
			fe->ops.info.frequency_stepsize = 250000;
			break;
		}

		break;
	default:
		break;
	}

	return ret;
}


static struct dvb_frontend_ops avl6882_ops = {
	.delsys = {SYS_DVBT, SYS_DVBT2, SYS_DVBC_ANNEX_A, SYS_DVBS, SYS_DVBS2},
	.info = {
		.name			= "Availink AVL6882",
		.frequency_min		= 0,
		.frequency_max		= 0,
		.frequency_stepsize	= 0,
		.frequency_tolerance	= 0,
		.symbol_rate_min	= 1000000,
		.symbol_rate_max	= 45000000,
		.caps = FE_CAN_FEC_1_2                 |
			FE_CAN_FEC_2_3                 |
			FE_CAN_FEC_3_4                 |
			FE_CAN_FEC_4_5                 |
			FE_CAN_FEC_5_6                 |
			FE_CAN_FEC_6_7                 |
			FE_CAN_FEC_7_8                 |
			FE_CAN_FEC_AUTO                |
			FE_CAN_QPSK                    |
			FE_CAN_QAM_16                  |
			FE_CAN_QAM_32                  |
			FE_CAN_QAM_64                  |
			FE_CAN_QAM_128                 |
			FE_CAN_QAM_256                 |
			FE_CAN_QAM_AUTO                |
			FE_CAN_TRANSMISSION_MODE_AUTO  |
			FE_CAN_GUARD_INTERVAL_AUTO     |
			FE_CAN_HIERARCHY_AUTO          |
			FE_CAN_MUTE_TS                 |
			FE_CAN_2G_MODULATION           |
			FE_CAN_MULTISTREAM             |
			FE_CAN_INVERSION_AUTO
	},

	.release			= avl6882_release,
	.init				= avl6882_init,

	.sleep				= avl6882_sleep,
	.i2c_gate_ctrl			= avl6882_i2c_gate_ctrl,

	.read_status			= avl6882_read_status,
	.read_ber = avl6882_read_ber,
	.read_signal_strength = avl6882fe_strength,
	.read_snr = avl6882fe_snr,
	.read_ucblocks = avl6882_read_ucblocks,
	.set_tone			= avl6882_set_tone,
	.set_voltage			= avl6882_set_voltage,
	.diseqc_send_master_cmd = avl6882_diseqc,
	.diseqc_send_burst = avl6882_burst,
	.get_frontend_algo		= avl6882fe_algo,
	.tune				= avl6882_tune,

	.set_property			= avl6882_set_property,
//	.get_property = avl6882_get_property,
	.set_frontend			= avl6882_set_frontend,
	.get_frontend = avl6882_get_frontend,
};


struct dvb_frontend *avl6882_attach(struct avl6882_config *config,
					struct i2c_adapter *i2c)
{
	struct avl6882_priv *priv;
	int ret;
	u32 id, fid;

	priv = kzalloc(sizeof(struct avl6882_priv), GFP_KERNEL);
	if (priv == NULL)
		goto err;

	memcpy(&priv->frontend.ops, &avl6882_ops,
		sizeof(struct dvb_frontend_ops));

	priv->frontend.demodulator_priv = priv;
	priv->config = config;
	priv->i2c = i2c;
	priv->g_nChannel_ts_total = 0,
	priv->delivery_system = -1;

	/* get chip id */
	ret = AVL6882_RD_REG32(priv, 0x108000, &id);
	/* get chip family id */
	ret |= AVL6882_RD_REG32(priv, 0x40000, &fid);
	if (ret) {
		dev_err(&priv->i2c->dev, "%s: attach failed reading id",
				KBUILD_MODNAME);
		goto err1;
	}

	if (fid != 0x68624955) {
		dev_err(&priv->i2c->dev, "%s: attach failed family id mismatch",
				KBUILD_MODNAME);
		goto err1;
	}

	dev_info(&priv->i2c->dev, "%s: found id=0x%x " \
				"family_id=0x%x", KBUILD_MODNAME, id, fid);

	return &priv->frontend;

err1:
	kfree(priv);
err:
	return NULL;
}
EXPORT_SYMBOL(avl6882_attach);

MODULE_DESCRIPTION("Availink AVL6882 DVB demodulator driver");
MODULE_AUTHOR("Luis Alves (ljalvs@gmail.com)");
MODULE_LICENSE("GPL");




#if 0
static int AVL_Demod_DVBSx_Diseqc_SendModulationData(struct avl6882_priv *priv, AVL_puchar pucBuff, u8 ucSize)
{
	int r = 0;
	u32 i1 = 0;
	u32 i2 = 0;
	//u8 pucBuffTemp[8] = {0};
	u8 Continuousflag = 0;
	u16 uiTempOutTh = 0;

	if (ucSize > 8) {
		r = AVL_EC_WARNING;
	} else {
		if (priv->config->eDiseqcStatus == AVL_DOS_InContinuous) {
			r |= AVL6882_RD_REG32(priv,0x16c000 + hw_diseqc_tx_cntrl_offset, &i1);
			if ((i1>>10) & 0x01) {
				Continuousflag = 1;
				i1 &= 0xfffff3ff;
				r |= AVL6882_WR_REG32(priv,0x16c000 + hw_diseqc_tx_cntrl_offset, i1);
				msleep(Diseqc_delay);		//delay 20ms
			}
		}
		//reset rx_fifo
		r |= AVL6882_RD_REG32(priv, 0x16c000 + hw_diseqc_rx_cntrl_offset, &i2);
		r |= AVL6882_WR_REG32(priv, 0x16c000 + hw_diseqc_rx_cntrl_offset, (i2|0x01));
		r |= AVL6882_WR_REG32(priv, 0x16c000 + hw_diseqc_rx_cntrl_offset, (i2&0xfffffffe));

		r |= AVL6882_RD_REG32(priv, 0x16c000 + hw_diseqc_tx_cntrl_offset, &i1);
		i1 &= 0xfffffff8;	//set to modulation mode and put it to FIFO load mode
		r |= AVL6882_WR_REG32(priv, 0x16c000 + hw_diseqc_tx_cntrl_offset, i1);

		for (i2=0; i2 < ucSize; i2++) {
			r |= AVL6882_WR_REG32(priv, 0x16c000 + hw_tx_fifo_map_offset, pucBuff[i2]);
		}
#if 0
		//trunk address
		ChunkAddr_Demod(0x16c000 + hw_tx_fifo_map_offset, pucBuffTemp);
		pucBuffTemp[3] = 0;
		pucBuffTemp[4] = 0;
		pucBuffTemp[5] = 0;
		for( i2=0; i2<ucSize; i2++ ) {
			pucBuffTemp[6] = pucBuff[i2];

			r |= II2C_Write_Demod(priv, pucBuffTemp, 7);
		}
#endif

		i1 |= (1<<2);  //start fifo transmit.
		r |= AVL6882_WR_REG32(priv,0x16c000 + hw_diseqc_tx_cntrl_offset, i1);

		if (AVL_EC_OK == r) {
			priv->config->eDiseqcStatus = AVL_DOS_InModulation;
		}
		do {
			msleep(1);
			if (++uiTempOutTh > 500) {
				r |= AVL_EC_TIMEOUT;
				return(r);
			}
			r = AVL6882_RD_REG32(priv,0x16c000 + hw_diseqc_tx_st_offset, &i1);
		} while (1 != ((i1 & 0x00000040) >> 6));

		msleep(Diseqc_delay);		//delay 20ms
		if (Continuousflag == 1) {			//resume to send out wave
			//No data in FIFO
			r |= AVL6882_RD_REG32(priv,0x16c000 + hw_diseqc_tx_cntrl_offset, &i1);
			i1 &= 0xfffffff8;
			i1 |= 0x03; 	//switch to continuous mode
			r |= AVL6882_WR_REG32(priv,0x16c000 + hw_diseqc_tx_cntrl_offset, i1);

			//start to send out wave
			i1 |= (1<<10);
			r |= AVL6882_WR_REG32(priv,0x16c000 + hw_diseqc_tx_cntrl_offset, i1);
			if (AVL_EC_OK == r) {
				priv->config->eDiseqcStatus = AVL_DOS_InContinuous;
			}
		}
	}
	return (r);
}


int  AVL_Demod_DVBSx_Diseqc_GetTxStatus( struct avl6882_priv *priv, AVL_Diseqc_TxStatus * pTxStatus)
{
	int r = 0;
	u32 i1 = 0;



	if( (AVL_DOS_InModulation == priv->config->eDiseqcStatus) || (AVL_DOS_InTone == priv->config->eDiseqcStatus) )
	{
		r |= AVL6882_RD_REG32(priv,0x16c000 + hw_diseqc_tx_st_offset, &i1);
		pTxStatus->m_TxDone = (u8)((i1 & 0x00000040)>>6);
		pTxStatus->m_TxFifoCount = (u8)((i1 & 0x0000003c)>>2);
	}
	else
	{
		r |= AVL_EC_GENERAL_FAIL;
	}

	return (r);
}


void AVL_SX_DiseqcSendCmd(struct avl6882_priv *priv, AVL_puchar pCmd, u8 CmdSize)
{
	int r = AVL_EC_OK;
	struct AVL_Diseqc_TxStatus TxStatus;

	r = AVL_Demod_DVBSx_Diseqc_SendModulationData(priv,pCmd, CmdSize);
	if(r != AVL_EC_OK) {
		printk("AVL_SX_DiseqcSendCmd failed !\n");
	} else {
		do {
			msleep(5);
			r |= AVL_Demod_DVBSx_Diseqc_GetTxStatus(priv,&TxStatus);
		} while(TxStatus.m_TxDone != 1);
		if (r == AVL_EC_OK ) {
		} else {
			printk("AVL_SX_DiseqcSendCmd Err. !\n");
		}
	}
}

#endif








#if 0
static int AVL_Demod_DVBSx_GetFunctionalMode( struct avl6882_priv *priv,AVL_FunctionalMode * pFunctionalMode)
{
	int r = AVL_EC_OK;
	u32 uiTemp = 0;

	r = AVL6882_RD_REG16(priv,0xe00 + rc_DVBSx_functional_mode_saddr_offset, &uiTemp);
	*pFunctionalMode = (AVL_FunctionalMode)(uiTemp & 0x0001);

	return(r);
}

#endif

#if 0
static int AVL_Demod_GetSNR(struct avl6882_priv *priv,u32 *snr_db)
{
	int r = AVL_EC_OK;

	*snr_db = 0;

	/* dvb-s */
	r = AVL6882_RD_REG32(priv,0xc00 + rs_DVBSx_int_SNR_dB_iaddr_offset, snr_db);
	if( (*puiSNR_db) > 10000 )
	{
		// Not get stable SNR value yet.
		*puiSNR_db = 0;
	}

	return (r);
}
#endif
#if 0
static int DVBSx_GetSignalQuality_Demod(struct avl6882_priv *priv, AVL_puint16 puiQuality )
{
	int r = AVL_EC_OK;
	u32 uiTemp = 0;

	r = DVBSx_GetSNR_Demod(priv,&uiTemp);
	if(uiTemp > 2500) {
		*puiQuality = 100;
	} else {
		*puiQuality = uiTemp*100/2500;
	}

	return r;
}

int AVL_Demod_GetSQI ( struct avl6882_priv *priv,AVL_puint16 pusSQI)
{
	int r = AVL_EC_OK;

	*pusSQI = 0;
	r=DVBSx_GetSignalQuality_Demod(priv,pusSQI);

	return (r);
}
#endif
#if 0
int AVL_Demod_GetSSI( struct avl6882_priv *priv,AVL_puint16 pusSSI)
{
	int r = AVL_EC_OK;
	u32 v;

	r = AVL6882_RD_REG16(priv,0x0a4 + rs_rf_agc_saddr_offset, &v);
	*pusSSI = (u16) v;

	return (r);
}
#endif













#if 0



static int AVL_LockChannel_T2(struct avl6882_priv *priv, u32 Freq_Khz, u32 BandWidth_Khz, u8 T2_Profile, AVL_int32 PLP_ID)
{
	int ret;
	AVL_DVBTxBandWidth nBand = AVL_DVBTx_BW_8M;
	AVL_DVBTx_LockMode eDVBTxLockMode;
	AVL_DVBT2_PROFILE eDVTB2Profile = (AVL_DVBT2_PROFILE) T2_Profile;

	printk("[AVL_LockChannel_T2] Freq:%d Mhz,sym:%d Khz\n",Freq_Khz,BandWidth_Khz);

	//return_code = r848_lock_n_wait(priv, Freq_Khz, BandWidth_Khz);
	//AVL_Demod_DVBT2AutoLock(priv, nBand, , PLP_ID);

	ret = AVL6882_WR_REG8(priv, 0xa00 + rc_DVBTx_l1_proc_only_caddr_offset, 0);
	if (eDVTB2Profile == AVL_DVBT2_PROFILE_BASE) {
	    eDVBTxLockMode = AVL_DVBTx_LockMode_T2BASE;
	} else if (eDVTB2Profile == AVL_DVBT2_PROFILE_LITE) {
	    eDVBTxLockMode = AVL_DVBTx_LockMode_T2LITE;
	} else {
	    eDVBTxLockMode = AVL_DVBTx_LockMode_ALL;
	}

	nBand = Convert2DemodBand(BandWidth_Khz);
	ret |= DVBTx_SetBandWidth_Demod(priv, nBand);
	ret |= AVL6882_WR_REG8(priv, 0xa00 + rc_DVBTx_acquire_mode_caddr_offset, (u8) eDVBTxLockMode);
	ret |= AVL6882_WR_REG8(priv, 0xa00 + rc_DVBTx_spectrum_invert_caddr_offset, AVL_SPECTRUM_AUTO);
	ret |= AVL6882_WR_REG8(priv, 0xa00 + rc_DVBTx_data_PLP_ID_caddr_offset, PLP_ID);
	ret |= AVL6882_WR_REG8(priv, 0xa00 + rc_DVBTx_common_PLP_ID_caddr_offset, 0);
	ret |= AVL6882_WR_REG8(priv, 0xa00 + rc_DVBTx_common_PLP_present_caddr_offset, 2);
	ret |= avl6882_exec_n_wait(priv,AVL_FW_CMD_ACQUIRE);
	if (ret)
		printk("[AVL_LockChannel_DVBT2] Failed to lock the channel!\n");
	return ret;
}


static int AVL_LockChannel_T(struct avl6882_priv *priv,u32 Freq_Khz,u16 BandWidth_Khz, AVL_int32 DVBT_layer_info)
{
	int ret;
	AVL_DVBTxBandWidth nBand = AVL_DVBTx_BW_8M;

	printk("[AVL_LockChannel_T] Freq is %d MHz, Bandwide is %d MHz, Layer Info is %d (0 : LP; 1 : HP)\n",
			   Freq_Khz/1000, BandWidth_Khz/1000, DVBT_layer_info);

	//ret = r848_lock_n_wait(priv, Freq_Khz, BandWidth_Khz);
	nBand = Convert2DemodBand(BandWidth_Khz);
	//AVL_Demod_DVBTAutoLock
	ret = AVL6882_WR_REG8(priv, 0xa00 + rc_DVBTx_l1_proc_only_caddr_offset, 0);
	ret |= DVBTx_SetBandWidth_Demod(priv, nBand);
	ret |= AVL6882_WR_REG8(priv, 0xa00 + rc_DVBTx_acquire_mode_caddr_offset, (u8) AVL_DVBTx_LockMode_T_ONLY);
	ret |= AVL6882_WR_REG8(priv, 0xa00 + rc_DVBTx_spectrum_invert_caddr_offset, AVL_SPECTRUM_AUTO);
	ret |= AVL6882_WR_REG8(priv, 0xa00 + rc_DVBTx_dvbt_layer_select_caddr_offset, DVBT_layer_info);
	ret |= AVL6882_WR_REG8(priv, 0xa00 + rc_DVBTx_data_PLP_ID_caddr_offset, 0);
	ret |= AVL6882_WR_REG8(priv, 0xa00 + rc_DVBTx_common_PLP_ID_caddr_offset, 0);
	ret |= AVL6882_WR_REG8(priv, 0xa00 + rc_DVBTx_common_PLP_present_caddr_offset, 0);
	ret |= avl6882_exec_n_wait(priv, AVL_FW_CMD_ACQUIRE);
	if(ret)
	    printk("[AVL_LockChannel_T] Failed to lock the channel!\n");

	return ret;
}




static int IBase_SendRxOP_Demod(struct avl6882_priv *priv,u8 cmd)
{
	int ret = avl6882_wait_demod(priv);
	if (ret)
		return ret;
	return AVL6882_WR_REG16(priv, 0x200 + rc_fw_command_saddr_offset, (u32) cmd);
}

int  TestSDRAM_Demod( struct avl6882_priv *priv,AVL_puint32 puiTestResult, AVL_puint32 puiTestPattern)
{
	int r = AVL_EC_OK;
	u16 uiTimeDelay = 100;
	u16 uiMaxRetries = 200;
	u32 i=0;

	r = IBase_SendRxOP_Demod(priv,AVL_FW_CMD_SDRAM_TEST);
	if(AVL_EC_OK == r )
	{

		r |= avl6882_wait_demod(priv);
/*		while (AVL_EC_OK != IBase_GetRxOPStatus_Demod(priv))
		{
			if (uiMaxRetries < i++)
			{
				r |= AVL_EC_RUNNING;
				break;
			}
			msleep(uiTimeDelay);
		}*/

		r |= AVL6882_RD_REG32(priv,0x0a4 + rc_sdram_test_return_iaddr_offset, puiTestPattern);
		r |= AVL6882_RD_REG32(priv,0x0a4 + rc_sdram_test_result_iaddr_offset, puiTestResult);
	}

	return r;
}

static int avl6882_demod_lock_wait(struct avl6882_priv *priv, u8 *lock_flag)
{
	int ret, retry = 50;
	enum fe_status locked;
	do {
		ret = avl6882_read_status(&priv->frontend, &locked);
		if (ret) {
			*lock_flag = 0;
			break;
		}
		if (locked) {
			*lock_flag = 1;
			break;
		}
		msleep(20);
	} while (--retry);
	return ret;
}

int AVL_Demod_DVBTxChannelScan(struct avl6882_priv *priv, AVL_DVBTxBandWidth eBandWidth, AVL_DVBTx_LockMode eLockMode)
{
    int r = AVL_EC_OK;

    r = DVBTx_SetBandWidth_Demod(priv,eBandWidth);
    r |= AVL6882_WR_REG8(priv,0xa00 + rc_DVBTx_acquire_mode_caddr_offset, eLockMode);
    r |= AVL6882_WR_REG8(priv, 0xa00 + rc_DVBTx_l1_proc_only_caddr_offset, 1);
    r |= avl6882_exec_n_wait(priv,AVL_FW_CMD_ACQUIRE);

    return r;
}


int AVL_Demod_DVBTxGetScanInfo(struct avl6882_priv *priv, AVL_DVBTxScanInfo* pstDVBTxScanInfo)
{
    int r = AVL_EC_OK;
    u32 ucTemp0 = 0;
    u32 ucTemp1 = 0;
    enum fe_status ucDemodLockStatus;

    r = avl6882_read_status(&priv->frontend, &ucDemodLockStatus);
    if(ucDemodLockStatus == 0)
        return r;

    r |= AVL6882_RD_REG8(priv, 0x800 + rs_DVBTx_rx_mode_caddr_offset,
                         &ucTemp0);

    pstDVBTxScanInfo->eTxStandard = (AVL_DVBTx_Standard)ucTemp0;


    if(AVL_DVBTx_Standard_T == pstDVBTxScanInfo->eTxStandard)
    {
        r |= AVL6882_RD_REG8(priv, 0x8f0 + rs_DVBTx_hierarchy_caddr_offset,&ucTemp0);
    }
    else if(AVL_DVBTx_Standard_T2 == pstDVBTxScanInfo->eTxStandard)
    {
        r |= AVL6882_RD_REG8(priv,0x800 + rs_DVBTx_P1_S2_field_2_caddr_offset, &ucTemp1);
        r |= AVL6882_RD_REG8(priv, 0x800 + rs_DVBTx_T2_profile_caddr_offset, &ucTemp0);
    }

    pstDVBTxScanInfo->ucTxInfo = ucTemp0;
    pstDVBTxScanInfo->ucFEFInfo = ucTemp1;

    return r;
}




static int AVL_Demod_DVBT2GetPLPList(struct avl6882_priv *priv, AVL_puchar pucPLPIndexArray, AVL_puchar pucPLPNumber)
{
	int r = AVL_EC_OK;
	u32 ucTemp = 0;
	u32 uiPLPBuffer = 0x2912b4;
	u32 ucPLPID = 0;
	u32 ucPLPType = 0;
	u32 ucPLPGroupID = 0;
	u32 i = 0;
	u8 ucDataPLPNum = 0;
	u32 uiDelayMS = 20;
	u32 uiTimes = 10; //time-out window 10*20 = 200ms
	enum fe_status ucDemodLockStatus = 0;

	r = avl6882_read_status(&priv->frontend, &ucDemodLockStatus);
	if (ucDemodLockStatus == 0) {
		*pucPLPNumber = 0;
		return r;
	}

	for(i = 0; i < uiTimes; i++)
	{
	    msleep(uiDelayMS);
	    r |= AVL6882_RD_REG8(priv,0x800 + rs_DVBTx_plp_list_request_caddr_offset, &ucTemp);
	    if(ucTemp == 0)
	    {
		break;
	    }
	}

	if(i == uiTimes)
	{
	    r |= AVL_EC_GENERAL_FAIL;
	    return (r);
	}

	r |= AVL6882_RD_REG8(priv,0x830 + rs_DVBTx_NUM_PLP_caddr_offset, &ucTemp);


	for(i = 0; i<ucTemp; i++)
	{
	    r |= AVL6882_RD_REG8(priv,uiPLPBuffer++, &ucPLPID);
	    r |= AVL6882_RD_REG8(priv,uiPLPBuffer++, &ucPLPType);
	    r |= AVL6882_RD_REG8(priv,uiPLPBuffer++, &ucPLPGroupID);

	    if(ucPLPType != 0)
	    {
		*(pucPLPIndexArray + ucDataPLPNum) = ucPLPID;
		ucDataPLPNum++;
	    }
	}
	*pucPLPNumber = ucDataPLPNum;
	return (r);
}


static int avl6882_set_dvbt(struct dvb_frontend *fe)
{

	u32 Freq_Khz = c->frequency/1000;
	u16 BandWidth_Khz = c->bandwidth_hz/1000;

	int return_code = AVL_EC_OK;
	AVL_DVBTxScanInfo stDVBTxScanInfo;
	AVL_DVBTxBandWidth nBand = AVL_DVBTx_BW_8M;
	u16 cur_index = 0;
	u8 ucLockFlag = 0;
	AVL_DVBT2_PROFILE ucT2Profile = AVL_DVBT2_PROFILE_UNKNOWN;
	u8 ucDataPLPArray[255] = {0};
	u8 ucDataPLPNumber = 0;
	u16 i;

	printk("[AVL_ChannelScan_Tx] Freq is %d MHz BW is %d MHz \n",
			Freq_Khz/1000, BandWidth_Khz/1000);

	priv->g_nChannel_ts_total = 0;

	//=====Tuner Lock=====//
	return_code = r848_lock_n_wait(priv, Freq_Khz, c->bandwidth_hz);
	//=====Demod Lock=====//
	nBand = Convert2DemodBand(BandWidth_Khz);
	return_code = AVL_Demod_DVBTxChannelScan(priv, nBand, AVL_DVBTx_LockMode_ALL);
	//=====Check Lock Status =====//
	avl6882_demod_lock_wait(priv, &ucLockFlag);

	if(ucLockFlag == 1) { //DVBTx is locked
		return_code |= AVL_Demod_DVBTxGetScanInfo(priv, &stDVBTxScanInfo);
		if(stDVBTxScanInfo.eTxStandard == AVL_DVBTx_Standard_T2) {
		  	//get PLP ID list only for DVBT2 signal, not for DVBT
			cur_index = priv->g_nChannel_ts_total;
			return_code = AVL_Demod_DVBT2GetPLPList(priv, ucDataPLPArray, &ucDataPLPNumber);

			for (i = 0; i < ucDataPLPNumber; i++) {
				printk("[DVB-T2_Scan_Info] DATA PLP ID is %d, profile = %d\n",
					ucDataPLPArray[i], stDVBTxScanInfo.ucTxInfo);

				//save channel RF frequency
				priv->global_channel_ts_table[cur_index].channel_freq_khz = Freq_Khz;
				// save channel bandwidth
				priv->global_channel_ts_table[cur_index].channel_bandwith_khz = BandWidth_Khz;
				// save data plp id
				priv->global_channel_ts_table[cur_index].data_plp_id = ucDataPLPArray[i];
				// 0 - DVBT; 1 - DVBT2.
				priv->global_channel_ts_table[cur_index].channel_type = AVL_DVBTx_Standard_T2;
				// 0 - Base profile; 1 - Lite profile.
				priv->global_channel_ts_table[cur_index].channel_profile = (AVL_DVBT2_PROFILE)stDVBTxScanInfo.ucTxInfo;
				cur_index++;
			}
			priv->g_nChannel_ts_total = cur_index % MAX_CHANNEL_INFO;
			if (stDVBTxScanInfo.ucFEFInfo == 1) {
				ucT2Profile = (AVL_DVBT2_PROFILE) stDVBTxScanInfo.ucTxInfo;
				if (ucT2Profile == AVL_DVBT2_PROFILE_BASE) {
					//profile is base
					//If T2 base is locked, try to lock T2 lite
					AVL_Demod_DVBTxChannelScan(priv, nBand, AVL_DVBTx_LockMode_T2LITE);
					ucT2Profile = AVL_DVBT2_PROFILE_LITE;
				} else {
					//If T2 lite is locked, try to lock T2 base
					AVL_Demod_DVBTxChannelScan(priv, nBand, AVL_DVBTx_LockMode_T2BASE);
					ucT2Profile = AVL_DVBT2_PROFILE_BASE;
				}
				avl6882_demod_lock_wait(priv, &ucLockFlag);
				if (ucLockFlag == 1) {
					//DVBTx is locked
					cur_index = priv->g_nChannel_ts_total;
					ucDataPLPNumber = 0;
					return_code = AVL_Demod_DVBT2GetPLPList(priv, ucDataPLPArray, &ucDataPLPNumber);

					// data PLP ID and common PLP ID pairing
					for (i = 0; i < ucDataPLPNumber; i++) {
						printk("[DVB-T2_Scan_Info] DATA PLP ID is %d, profile = %d\n",
								ucDataPLPArray[i], ucT2Profile);

						//save channel RF frequency
						priv->global_channel_ts_table[cur_index].channel_freq_khz = Freq_Khz;
						// save channel bandwidth
						priv->global_channel_ts_table[cur_index].channel_bandwith_khz = BandWidth_Khz;
						// save data plp id
						priv->global_channel_ts_table[cur_index].data_plp_id = ucDataPLPArray[i];
						// 0 - DVBT; 1 - DVBT2.
						priv->global_channel_ts_table[cur_index].channel_type = AVL_DVBTx_Standard_T2;
						// 0 - Base profile; 1 - Lite profile.
						priv->global_channel_ts_table[cur_index].channel_profile = ucT2Profile;

						cur_index++;
					}
					priv->g_nChannel_ts_total = cur_index % MAX_CHANNEL_INFO;
				}
			} else {
				printk("Lock DVB-T2: No FEFInfo\n");
			}
		} else {
			// DVBT
			cur_index = priv->g_nChannel_ts_total;
			// save channel RF frequency
			priv->global_channel_ts_table[cur_index].channel_freq_khz = Freq_Khz;
			// save channel bandwidth
			priv->global_channel_ts_table[cur_index].channel_bandwith_khz = BandWidth_Khz;
			// save data plp id(not used for DVBT, set to 0xff)
			priv->global_channel_ts_table[cur_index].data_plp_id = 0;
			// 0 - DVBT; 1 - DVBT2.
			priv->global_channel_ts_table[cur_index].channel_type = AVL_DVBTx_Standard_T;
			// 0 - Low priority layer, 1 - High priority layer
			priv->global_channel_ts_table[cur_index].dvbt_hierarchy_layer = 1;
			cur_index++;

			if(stDVBTxScanInfo.ucTxInfo == 1) {
				// for hierarchy
				// save channel RF frequency
				priv->global_channel_ts_table[cur_index].channel_freq_khz = Freq_Khz;
				// save channel bandwidth
				priv->global_channel_ts_table[cur_index].channel_bandwith_khz = BandWidth_Khz;
				// save data plp id(not used for DVBT, set to 0xff)
				priv->global_channel_ts_table[cur_index].data_plp_id = 0;
				// 0 - DVBT; 1 - DVBT2.
				priv->global_channel_ts_table[cur_index].channel_type = AVL_DVBTx_Standard_T;
				// 0 - Low priority layer, 1 - High priority layer
				priv->global_channel_ts_table[cur_index].dvbt_hierarchy_layer = 0;
				cur_index++;
			}
			priv->g_nChannel_ts_total = cur_index % MAX_CHANNEL_INFO;
		}
	} else {
		// return for unlock
		printk("[DVBTx_ScanChannel_Tx] DVBTx channel scan is fail,Err.\n");
	}

	/* lock channel */
	for(i = 0; i < priv->g_nChannel_ts_total; i++) {
		ucLockFlag = 0;
	        if(priv->global_channel_ts_table[i].channel_type == AVL_DVBTx_Standard_T) {
			//DVB-T signal..
			AVL_LockChannel_T(priv, Freq_Khz, BandWidth_Khz, priv->global_channel_ts_table[i].dvbt_hierarchy_layer);
		} else if (priv->global_channel_ts_table[i].channel_type == AVL_DVBTx_Standard_T2) {
			//DVB-T2 signal, do not process FEF...
			AVL_LockChannel_T2(priv, Freq_Khz, BandWidth_Khz,priv->global_channel_ts_table[i].channel_profile, priv->global_channel_ts_table[i].data_plp_id);
		}
		avl6882_demod_lock_wait(priv, &ucLockFlag);
	}

	return return_code;
}




int  AVL_Demod_DVBSx_Diseqc_SendTone(  struct avl6882_priv *priv,u8 ucTone, u8 ucCount)
{
	int r = 0;
	u32 i1 = 0;
	u32 i2 = 0;
	//u8 pucBuffTemp[8] = {0};
	u8 Continuousflag = 0;
	u16 uiTempOutTh = 0;

	if( ucCount>8 )
	{
		r = AVL_EC_WARNING;
	}
	else
	{
			if (priv->config->eDiseqcStatus == AVL_DOS_InContinuous)
			{
				r |= AVL6882_RD_REG32(priv,0x16c000 + hw_diseqc_tx_cntrl_offset, &i1);
				if ((i1>>10) & 0x01)
				{
					Continuousflag = 1;
					i1 &= 0xfffff3ff;
					r |= AVL6882_WR_REG32(priv,0x16c000 + hw_diseqc_tx_cntrl_offset, i1);
					msleep(Diseqc_delay);		//delay 20ms
				}
			}
			//No data in the FIFO.
			r |= AVL6882_RD_REG32(priv,0x16c000 + hw_diseqc_tx_cntrl_offset, &i1);
			i1 &= 0xfffffff8;  //put it into the FIFO load mode.
			if( 0 == ucTone )
			{
				i1 |= 0x01;
			}
			else
			{
				i1 |= 0x02;
			}
			r |= AVL6882_WR_REG32(priv,0x16c000 + hw_diseqc_tx_cntrl_offset, i1);


			for (i2 = 0; i2 < ucCount; i2++) {
				r |= AVL6882_WR_REG32(priv, 0x16c000 + hw_tx_fifo_map_offset, 1);
			}
#if 0
			//trunk address
			ChunkAddr_Demod(0x16c000 + hw_tx_fifo_map_offset, pucBuffTemp);
			pucBuffTemp[3] = 0;
			pucBuffTemp[4] = 0;
			pucBuffTemp[5] = 0;
			pucBuffTemp[6] = 1;

			for( i2=0; i2<ucCount; i2++ )
			{
				r |= II2C_Write_Demod(priv, pucBuffTemp, 7);
			}
#endif

			i1 |= (1<<2);  //start fifo transmit.
			r |= AVL6882_WR_REG32(priv,0x16c000 + hw_diseqc_tx_cntrl_offset, i1);
			if( AVL_EC_OK == r )
			{
				priv->config->eDiseqcStatus = AVL_DOS_InTone;
			}
			do 
			{
				msleep(1);
				if (++uiTempOutTh > 500)
				{
					r |= AVL_EC_TIMEOUT;
					return(r);
				}
				r = AVL6882_RD_REG32(priv,0x16c000 + hw_diseqc_tx_st_offset, &i1);
			} while ( 1 != ((i1 & 0x00000040) >> 6) );

			msleep(Diseqc_delay);		//delay 20ms
			if (Continuousflag == 1)			//resume to send out wave
			{
				//No data in FIFO
				r |= AVL6882_RD_REG32(priv,0x16c000 + hw_diseqc_tx_cntrl_offset, &i1);
				i1 &= 0xfffffff8; 
				i1 |= 0x03; 	//switch to continuous mode
				r |= AVL6882_WR_REG32(priv,0x16c000 + hw_diseqc_tx_cntrl_offset, i1);

				//start to send out wave
				i1 |= (1<<10);	
				r |= AVL6882_WR_REG32(priv,0x16c000 + hw_diseqc_tx_cntrl_offset, i1);

			}
		}
	return (r);
}





#endif




#if 0







int  AVL_Demod_DVBSxManualLock( struct avl6882_priv *priv,AVL_DVBSxManualLockInfo *pstManualLockInfo)
{
	int r = AVL_EC_OK;
	AVL_FunctionalMode eFuncMode = AVL_FuncMode_BlindScan;


	r = AVL_Demod_DVBSx_GetFunctionalMode(priv,&eFuncMode);
	if(eFuncMode == AVL_FuncMode_Demod)
	{
		r |= AVL6882_WR_REG16(priv,0xc00 + rs_DVBSx_fec_lock_saddr_offset, 0);	   

		r |= AVL6882_WR_REG16(priv,0xe00 + rc_DVBSx_fec_bypass_coderate_saddr_offset, 1);//DVBS manual lock

		if (pstManualLockInfo->eDVBSxStandard == AVL_DVBS )
		{
			r |= AVL6882_WR_REG32(priv, 0xe00 + rc_DVBSx_dvbs_fec_coderate_iaddr_offset, pstManualLockInfo->eDVBSCodeRate);
		}
		else if(pstManualLockInfo->eDVBSxStandard == AVL_DVBS2 )
		{
			r |= AVL6882_WR_REG16(priv, 0xe00 + rc_DVBSx_dvbs2_code_rate_saddr_offset, pstManualLockInfo->eDVBS2CodeRate);
			r |= AVL6882_WR_REG16(priv, 0xe00 + rc_DVBSx_dvbs2_modulation_saddr_offset, pstManualLockInfo->eDVBSxModulationMode);
		}
		else
		{
			return AVL_EC_NOT_SUPPORTED;
		}
		r |= AVL6882_WR_REG16(priv, 0xe00 + rc_DVBSx_decode_mode_saddr_offset, pstManualLockInfo->eDVBSxStandard);

		if(pstManualLockInfo->eDVBSxSpecInversion == AVL_SPECTRUM_AUTO)
		{
			r |= AVL6882_WR_REG16(priv, 0xe00 + rc_DVBSx_iq_mode_saddr_offset, 1);//enable spectrum auto detection
		}
		else
		{
			r |= AVL6882_WR_REG32(priv, 0xe00 + rc_DVBSx_specinv_iaddr_offset, pstManualLockInfo->eDVBSxSpecInversion);
			r |= AVL6882_WR_REG16(priv, 0xe00 + rc_DVBSx_iq_mode_saddr_offset, 0);
		}

		r |= AVL6882_WR_REG32(priv, 0xe00 + rc_DVBSx_int_sym_rate_MHz_iaddr_offset, pstManualLockInfo->uiDVBSxSymbolRateSps);

		r |= avl6882_exec_n_wait(priv,AVL_FW_CMD_ACQUIRE );
	}
	else if(eFuncMode == AVL_FuncMode_BlindScan)
	{
		return AVL_EC_NOT_SUPPORTED;
	}

	return (r);
}
int  AVL_Demod_DVBSxGetModulationInfo( struct avl6882_priv *priv,AVL_DVBSxModulationInfo *pstModulationInfo)
{
	int r = AVL_EC_OK;
	u32 uiTemp = 0;
	u32  temp_uchar = 0;

	r = AVL6882_RD_REG32(priv, 0xc00 + rs_DVBSx_pilot_iaddr_offset, &uiTemp);
	pstModulationInfo->eDVBSxPilot = (AVL_DVBSx_Pilot)(uiTemp);

	r |= AVL6882_RD_REG32(priv, 0xe00 + rc_DVBSx_internal_decode_mode_iaddr_offset,&uiTemp);
	pstModulationInfo->eDVBSxStandard = (AVL_DVBSx_Standard)uiTemp;

	if(AVL_DVBS == (AVL_DVBSx_Standard)uiTemp)
	{
		r |= AVL6882_RD_REG32(priv, 0xe00 + rc_DVBSx_dvbs_fec_coderate_iaddr_offset,&uiTemp);
		pstModulationInfo->eDVBSCodeRate = (AVL_DVBS_CodeRate)(uiTemp);
	}
	else
	{
		r |= AVL6882_RD_REG8(priv, 0xe00 + rc_DVBSx_dvbs2_fec_coderate_caddr_offset,&temp_uchar);
		pstModulationInfo->eDVBS2CodeRate = (AVL_DVBS2_CodeRate)(temp_uchar);
	}

	r |= AVL6882_RD_REG32(priv, 0xc00 + rs_DVBSx_modulation_iaddr_offset, &uiTemp);
	pstModulationInfo->eDVBSxModulationMode = (AVL_DVBSx_ModulationMode)(uiTemp);

	r |= AVL6882_RD_REG32(priv, 0xc00 + rs_DVBSx_detected_alpha_iaddr_offset, &uiTemp);
	pstModulationInfo->eDVBSxRollOff = (AVL_DVBSx_RollOff)(uiTemp);

	return (r);

}

int  AVL_Demod_DVBSx_BlindScan_Start( struct avl6882_priv *priv,AVL_BlindScanPara * pBSPara, u16 uiTunerLPF_100kHz)
{
	int r = AVL_EC_OK;
	u16 uiCarrierFreq_100kHz = 0;
	u16 uiMinSymRate = 0;
	AVL_FunctionalMode enumFunctionalMode = AVL_FuncMode_Demod;

	r = AVL_Demod_DVBSx_GetFunctionalMode(priv,&enumFunctionalMode);

	if (enumFunctionalMode == AVL_FuncMode_BlindScan) {
		r |= AVL6882_WR_REG16(priv,0xe00 + rc_DVBSx_tuner_LPF_100kHz_saddr_offset, uiTunerLPF_100kHz);
		r |= AVL6882_WR_REG16(priv,0xe00 + rc_DVBSx_blind_scan_tuner_spectrum_inversion_saddr_offset, pBSPara->m_enumBSSpectrumPolarity);

		uiMinSymRate = pBSPara->m_uiMinSymRate_kHz - 200;		// give some tolerance

		if (uiMinSymRate < 800) 	  //Blind scan doesn't support symbol rate less then 1M, give 200K margin
		{
			uiMinSymRate = 800;
		}

		if( pBSPara->m_uiStartFreq_100kHz < pBSPara->m_uiStopFreq_100kHz )
		{
			if( AVL_EC_OK == r )
			{
				uiCarrierFreq_100kHz = ((pBSPara->m_uiStopFreq_100kHz)+(pBSPara->m_uiStartFreq_100kHz))>>1;
				r |= AVL6882_WR_REG16(priv,0xe00 + rc_DVBSx_tuner_frequency_100kHz_saddr_offset, uiCarrierFreq_100kHz);
				r |= AVL6882_WR_REG16(priv,0xe00 + rc_DVBSx_blind_scan_min_sym_rate_kHz_saddr_offset, uiMinSymRate);
				r |= AVL6882_WR_REG16(priv,0xe00 + rc_DVBSx_blind_scan_max_sym_rate_kHz_saddr_offset, (pBSPara->m_uiMaxSymRate_kHz)+200);
				r |= AVL6882_WR_REG16(priv,0xe00 + rc_DVBSx_blind_scan_start_freq_100kHz_saddr_offset, (pBSPara->m_uiStartFreq_100kHz));
				r |= AVL6882_WR_REG16(priv,0xe00 + rc_DVBSx_blind_scan_end_freq_100kHz_saddr_offset, (pBSPara->m_uiStopFreq_100kHz));

				if( AVL_EC_OK == r )
				{
					r = avl6882_exec_n_wait(priv,AVL_FW_CMD_BLIND_SCAN);
				}
			}
		}
		else
		{
			r = AVL_EC_GENERAL_FAIL;
		}
	}
	else
	{
		r = AVL_EC_GENERAL_FAIL;
	}

	return (r);
}
#endif

#if 0
// need fix read data size
int  AVL_Demod_DVBSx_BlindScan_GetStatus( struct avl6882_priv *priv,AVL_BSInfo * pBSInfo)
{
	int r = AVL_EC_OK;

	r = AVL6882_RD_REG16(priv,0xc00 + rs_DVBSx_blind_scan_progress_saddr_offset, &(pBSInfo->m_uiProgress));
	r |= AVL6882_RD_REG16(priv,0xc00 + rs_DVBSx_blind_scan_channel_count_saddr_offset, &(pBSInfo->m_uiChannelCount));
	r |= AVL6882_RD_REG16(priv,0xe00 + rc_DVBSx_blind_scan_start_freq_100kHz_saddr_offset, &(pBSInfo->m_uiNextStartFreq_100kHz));
	r |= AVL6882_RD_REG16(priv,0xc00 + rs_DVBSx_blind_scan_error_code_saddr_offset, &(pBSInfo->m_uiResultCode));
	if( pBSInfo->m_uiProgress > 100 )
	{
		pBSInfo->m_uiProgress = 0;
	}

	return(r);
}

int  AVL_Demod_DVBSx_BlindScan_Cancel( struct avl6882_priv *priv )
{
	int r;
	enum AVL_FunctionalMode enumFunctionalMode = AVL_FuncMode_Demod;



	r = AVL_Demod_DVBSx_GetFunctionalMode(priv,&enumFunctionalMode);

	if(enumFunctionalMode == AVL_FuncMode_BlindScan)
	{
		r |= avl6882_exec_n_wait(priv,AVL_FW_CMD_HALT);
	}
	else
	{
		r = AVL_EC_GENERAL_FAIL;
	}

	return(r);
}


int  AVL_Demod_DVBSx_BlindScan_ReadChannelInfo( struct avl6882_priv *priv,u16 uiStartIndex, AVL_puint16 pChannelCount, AVL_ChannelInfo * pChannel)
{
	int r = 0;
	u32 channel_addr = 0;
	u16 i1 = 0;
	u16 i2 = 0;
	u32 uiMinFreq = 0;
	u16 iMinIdx = 0;
	AVL_ChannelInfo sTempChannel;



	r = AVL6882_RD_REG16(priv,0xc00 + rs_DVBSx_blind_scan_channel_count_saddr_offset, &i1);
	if( (uiStartIndex + (*pChannelCount)) > (i1) )
	{
		*pChannelCount = i1-uiStartIndex;
	}
	r |= AVL6882_RD_REG16(priv,0xe00 + rc_DVBSx_blind_scan_channel_info_offset_saddr_offset, &i1);
	channel_addr = 0x200C00 + uiStartIndex*sizeof(AVL_ChannelInfo);
	for( i1=0; i1<(*pChannelCount); i1++ )
	{
#if 1  //for some processors which can not read 12 bytes        
		//dump the channel information
		r |= AVL6882_RD_REG32(priv, channel_addr, &(pChannel[i1].m_uiFrequency_kHz));
		channel_addr += 4;
		r |= AVL6882_RD_REG32(priv, channel_addr, &(pChannel[i1].m_uiSymbolRate_Hz));
		channel_addr += 4;
		r |= AVL6882_RD_REG32(priv, channel_addr, &(pChannel[i1].m_Flags));
		channel_addr += 4;
#endif
	}

	// Sort the results
	for(i1=0; i1<(*pChannelCount); i1++)
	{
		iMinIdx = i1;
		uiMinFreq = pChannel[i1].m_uiFrequency_kHz;
		for(i2=(i1+1); i2<(*pChannelCount); i2++)
		{
			if(pChannel[i2].m_uiFrequency_kHz < uiMinFreq)
			{
				uiMinFreq = pChannel[i2].m_uiFrequency_kHz;
				iMinIdx = i2;
			}
		}
		sTempChannel = pChannel[iMinIdx];
		pChannel[iMinIdx] = pChannel[i1];
		pChannel[i1] = sTempChannel;
	}

	return(r);
}


int  AVL_Demod_DVBSx_BlindScan_Reset( struct avl6882_priv *priv )
{
	return AVL6882_WR_REG16(priv,0xe00 + rc_DVBSx_blind_scan_reset_saddr_offset, 1);
}


int  AVL_Demod_DVBSx_SetFunctionalMode( struct avl6882_priv *priv,AVL_FunctionalMode enumFunctionalMode )
{
	int r = AVL_EC_OK;
	r = AVL6882_WR_REG16(priv,0xe00 + rc_DVBSx_functional_mode_saddr_offset, (u16)enumFunctionalMode);
	r |= AVL6882_WR_REG16(priv,0xe00 + rc_DVBSx_iq_mode_saddr_offset,0);
	return(r);
}

int  AVL_Demod_DVBSx_SetDishPointingMode(  struct avl6882_priv *priv,AVL_Switch enumOn_Off)
{
	int r = AVL_EC_OK;
	AVL_FunctionalMode enumFunctionalMode = AVL_FuncMode_BlindScan;

	r |= AVL_Demod_DVBSx_GetFunctionalMode(priv,&enumFunctionalMode);
	if(enumFunctionalMode == AVL_FuncMode_Demod)
	{
		if(enumOn_Off == AVL_ON)
		{
			r |= AVL6882_WR_REG16(priv,0xe00 + rc_DVBSx_aagc_acq_gain_saddr_offset, 12);
			r |= AVL6882_WR_REG16(priv,0xe00 + rc_DVBSx_dishpoint_mode_saddr_offset, 1);
		}
		else
		{
			r |= AVL6882_WR_REG16(priv,0xe00 + rc_DVBSx_aagc_acq_gain_saddr_offset, 10);
			r |= AVL6882_WR_REG16(priv,0xe00 + rc_DVBSx_dishpoint_mode_saddr_offset, 0);
		}
	}
	else
	{
		r = AVL_EC_GENERAL_FAIL;
	}

	return(r);
}
#endif







int  AVL_Demod_GetGPIOValue( struct avl6882_priv *priv, u8 ePinNumber, u8 *pePinValue)
{
	int r = AVL_EC_OK;


	switch(ePinNumber)
	{
	case AVL_Pin37:
		r = AVL6882_WR_REG32(priv,REG_GPIO_BASE + lnb_cntrl_0_sel_offset, GPIO_Z);
		r = AVL6882_RD_REG32(priv,REG_GPIO_BASE + lnb_cntrl_0_i_offset, (AVL_puint32)pePinValue);
		break;

	case AVL_Pin38:
		r = AVL6882_WR_REG32(priv,REG_GPIO_BASE + lnb_cntrl_1_sel_offset, GPIO_Z);
		r = AVL6882_RD_REG32(priv,REG_GPIO_BASE + lnb_cntrl_1_i_offset, (AVL_puint32)pePinValue);
		break;
	default:
		break;
	}

	return r;
}



int  AVL_Demod_SetGPIO( struct avl6882_priv *priv,u8 ePinNumber, u8 ePinValue)
{
	int r = AVL_EC_OK;

	switch (ePinNumber) {
	case AVL_Pin37:
		r = AVL6882_WR_REG32(priv,REG_GPIO_BASE + lnb_cntrl_0_sel_offset, GPIO_0);

		switch(ePinValue)
		{
		case GPIO_0:
			r = AVL6882_WR_REG32(priv,REG_GPIO_BASE + lnb_cntrl_0_sel_offset, ePinValue);
			break;
		case GPIO_1:
			r = AVL6882_WR_REG32(priv,REG_GPIO_BASE + lnb_cntrl_0_sel_offset, ePinValue);
			break;
		case GPIO_Z:
			r = AVL6882_WR_REG32(priv,REG_GPIO_BASE + lnb_cntrl_0_sel_offset, ePinValue);
			break;
		default:
			break;
		}
		break;
	case AVL_Pin38:
		r = AVL6882_WR_REG32(priv,REG_GPIO_BASE + lnb_cntrl_1_sel_offset, GPIO_0);

		switch(ePinValue)
		{
		case GPIO_0:
			r = AVL6882_WR_REG32(priv,REG_GPIO_BASE + lnb_cntrl_1_sel_offset, ePinValue);
			break;
		case GPIO_1:
			r = AVL6882_WR_REG32(priv,REG_GPIO_BASE + lnb_cntrl_1_sel_offset, ePinValue);
			break;
		case GPIO_Z:
			r = AVL6882_WR_REG32(priv,REG_GPIO_BASE + lnb_cntrl_1_sel_offset, ePinValue);
			break;
		default:
			break;
		}
		break;
	default:
		break;
	}

	return r;
}







#if 0
int  AVL_Demod_GetVersion( struct avl6882_priv *priv,AVL_DemodVersion *pstDemodVersion)
{
	int r = AVL_EC_OK;
	u32 uiTemp = 0;
	u8 ucBuff[4] = {0};

	r =  AVL6882_RD_REG32(priv,0x40000, &uiTemp);
	if( AVL_EC_OK == r )
	{
		pstDemodVersion->uiChip = uiTemp;
	}

	r |=  AVL6882_RD_REG32(priv,0x0a4 + rs_patch_ver_iaddr_offset, &uiTemp);
	if( AVL_EC_OK == r )
	{
		Chunk32_Demod(uiTemp, ucBuff);
		pstDemodVersion->stPatch.ucMajor = ucBuff[0];
		pstDemodVersion->stPatch.ucMinor = ucBuff[1];
		pstDemodVersion->stPatch.usBuild = ucBuff[2];
		pstDemodVersion->stPatch.usBuild = ((u16)((pstDemodVersion->stPatch.usBuild)<<8)) + ucBuff[3];
	}

	return r;
}
#endif









