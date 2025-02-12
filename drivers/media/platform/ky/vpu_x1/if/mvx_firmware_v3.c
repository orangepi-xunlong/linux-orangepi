/*
 * The confidential and proprietary information contained in this file may
 * only be used by a person authorised under and to the extent permitted
 * by a subsisting licensing agreement from Arm Technology (China) Co., Ltd.
 *
 *            (C) COPYRIGHT 2021-2021 Arm Technology (China) Co., Ltd.
 *                ALL RIGHTS RESERVED
 *
 * This entire notice must be reproduced on all copies of this file
 * and copies of this file may only be made by a person if such person is
 * permitted to do so under the terms of a subsisting license agreement
 * from Arm Technology (China) Co., Ltd.
 * 
 * SPDX-License-Identifier: GPL-2.0-only
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 * 
 */

/****************************************************************************
 * Includes
 ****************************************************************************/

#include "mvx_firmware_priv.h"
#include "fw_v3/mve_protocol_def.h"

/****************************************************************************
 * Static functions
 ****************************************************************************/

static int get_region_v3(enum mvx_fw_region region,
			 uint32_t *begin,
			 uint32_t *end)
{
	switch (region) {
	case MVX_FW_REGION_CORE_0:
		*begin = MVE_MEM_REGION_FW_INSTANCE0_ADDR_BEGIN;
		*end = MVE_MEM_REGION_FW_INSTANCE0_ADDR_END;
		break;
	case MVX_FW_REGION_CORE_1:
		*begin = MVE_MEM_REGION_FW_INSTANCE1_ADDR_BEGIN;
		*end = MVE_MEM_REGION_FW_INSTANCE1_ADDR_END;
		break;
	case MVX_FW_REGION_CORE_2:
		*begin = MVE_MEM_REGION_FW_INSTANCE2_ADDR_BEGIN;
		*end = MVE_MEM_REGION_FW_INSTANCE2_ADDR_END;
		break;
	case MVX_FW_REGION_CORE_3:
		*begin = MVE_MEM_REGION_FW_INSTANCE3_ADDR_BEGIN;
		*end = MVE_MEM_REGION_FW_INSTANCE3_ADDR_END;
		break;
	case MVX_FW_REGION_CORE_4:
		*begin = MVE_MEM_REGION_FW_INSTANCE4_ADDR_BEGIN;
		*end = MVE_MEM_REGION_FW_INSTANCE4_ADDR_END;
		break;
	case MVX_FW_REGION_CORE_5:
		*begin = MVE_MEM_REGION_FW_INSTANCE5_ADDR_BEGIN;
		*end = MVE_MEM_REGION_FW_INSTANCE5_ADDR_END;
		break;
	case MVX_FW_REGION_CORE_6:
		*begin = MVE_MEM_REGION_FW_INSTANCE6_ADDR_BEGIN;
		*end = MVE_MEM_REGION_FW_INSTANCE6_ADDR_END;
		break;
	case MVX_FW_REGION_CORE_7:
		*begin = MVE_MEM_REGION_FW_INSTANCE7_ADDR_BEGIN;
		*end = MVE_MEM_REGION_FW_INSTANCE7_ADDR_END;
		break;
	case MVX_FW_REGION_PROTECTED:
		*begin = MVE_MEM_REGION_PROTECTED_ADDR_BEGIN;
		*end = MVE_MEM_REGION_PROTECTED_ADDR_END;
		break;
	case MVX_FW_REGION_FRAMEBUF:
		*begin = MVE_MEM_REGION_FRAMEBUF_ADDR_BEGIN;
		*end = MVE_MEM_REGION_FRAMEBUF_ADDR_END;
		break;
	case MVX_FW_REGION_MSG_HOST:
		*begin = MVE_COMM_MSG_INQ_ADDR;
		*end = MVE_COMM_MSG_INQ_ADDR + MVE_PAGE_SIZE;
		break;
	case MVX_FW_REGION_MSG_MVE:
		*begin = MVE_COMM_MSG_OUTQ_ADDR;
		*end = MVE_COMM_MSG_OUTQ_ADDR + MVE_PAGE_SIZE;
		break;
	case MVX_FW_REGION_BUF_IN_HOST:
		*begin = MVE_COMM_BUF_INQ_ADDR;
		*end = MVE_COMM_BUF_INQ_ADDR + MVE_PAGE_SIZE;
		break;
	case MVX_FW_REGION_BUF_IN_MVE:
		*begin = MVE_COMM_BUF_INRQ_ADDR;
		*end = MVE_COMM_BUF_INRQ_ADDR + MVE_PAGE_SIZE;
		break;
	case MVX_FW_REGION_BUF_OUT_HOST:
		*begin = MVE_COMM_BUF_OUTQ_ADDR;
		*end = MVE_COMM_BUF_OUTQ_ADDR + MVE_PAGE_SIZE;
		break;
	case MVX_FW_REGION_BUF_OUT_MVE:
		*begin = MVE_COMM_BUF_OUTRQ_ADDR;
		*end = MVE_COMM_BUF_OUTRQ_ADDR + MVE_PAGE_SIZE;
		break;
	case MVX_FW_REGION_RPC:
		*begin = MVE_COMM_RPC_ADDR;
		*end = MVE_COMM_RPC_ADDR + MVE_PAGE_SIZE;
		break;
	case MVX_FW_REGION_PRINT_RAM:
		*begin = MVE_FW_PRINT_RAM_ADDR;
		*end = MVE_FW_PRINT_RAM_ADDR + MVE_FW_PRINT_RAM_SIZE;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int to_mve_profile_v3(unsigned int mvx_profile,
			     uint16_t *mve_profile)
{
	int ret = 0;

	switch (mvx_profile) {
	case MVX_PROFILE_H264_HIGH_10:
		*mve_profile = MVE_OPT_PROFILE_H264_HIGH_10;
		break;
	default:
		ret = mvx_fw_to_mve_profile_v2(mvx_profile, mve_profile);
	}

	return ret;
}

/****************************************************************************
 * Exported functions
 ****************************************************************************/

int mvx_fw_construct_v3(struct mvx_fw *fw,
			struct mvx_fw_bin *fw_bin,
			struct mvx_mmu *mmu,
			struct mvx_session *session,
			struct mvx_client_ops *client_ops,
			struct mvx_client_session *csession,
			unsigned int ncores,
			unsigned char major,
			unsigned char minor)
{
	int ret;

	ret = mvx_fw_construct_v2(fw, fw_bin, mmu, session, client_ops,
				  csession, ncores, major, minor);
	if (ret != 0)
		return ret;

	fw->ops.get_region = get_region_v3;
	fw->ops_priv.to_mve_profile = to_mve_profile_v3;

	if (major == 3 && minor >= 1)
		fw->ops_priv.send_idle_ack = mvx_fw_send_idle_ack_v2;

	return 0;
}
