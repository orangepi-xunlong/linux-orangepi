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

#ifndef _MVX_FIRMWARE_PRIV_H_
#define _MVX_FIRMWARE_PRIV_H_

/****************************************************************************
 * Includes
 ****************************************************************************/

#include "mvx_firmware.h"

/****************************************************************************
 * Defines
 ****************************************************************************/

#if !defined(MVE_REQUEST_CODE_IDLE_ACK)
#define MVE_REQUEST_CODE_IDLE_ACK (1012)
#endif

/****************************************************************************
 * Firmware
 ****************************************************************************/

struct mvx_client_ops;
struct mvx_client_session;
struct mvx_fw_bin;
struct mvx_mmu;
struct mvx_session;

/**
 * mvx_firmware_construct() - Firmware constructor.
 * @fw:		Pointer to firmware object.
 * @fw_bin:	Pointer to firmware binary.
 * @mmu:	Pointer to MMU.
 * @session:	Pointer to session.
 * @client_ops:	Pointer to client operations.
 * @csession:	Pointer to client session.
 * ncores:	Number of cores.
 *
 * Return: 0 on success, else error code.
 */
int mvx_fw_construct(struct mvx_fw *fw,
		     struct mvx_fw_bin *fw_bin,
		     struct mvx_mmu *mmu,
		     struct mvx_session *session,
		     struct mvx_client_ops *client_ops,
		     struct mvx_client_session *csession,
		     unsigned int ncores);

/****************************************************************************
 * Firmware v2
 ****************************************************************************/

/**
 * mvx_fw_construct_v2() - Construct the object for the firmware v2 interface.
 * @fw:		Pointer to firmware object.
 * @fw_bin:	Pointer to firmware binary.
 * @mmu:	Pointer to MMU.
 * @session:	Pointer to session.
 * @client_ops:	Pointer to client operations.
 * @csession:	Pointer to client session.
 * ncores:	Number of cores.
 * @major:	Major firmware version.
 * @minor:	Minor firmware version.
 *
 * Return: 0 on success, else error code.
 */
int mvx_fw_construct_v2(struct mvx_fw *fw,
			struct mvx_fw_bin *fw_bin,
			struct mvx_mmu *mmu,
			struct mvx_session *session,
			struct mvx_client_ops *client_ops,
			struct mvx_client_session *csession,
			unsigned int ncores,
			unsigned char major,
			unsigned char minor);

/**
 * mvx_fw_send_idle_ack_v2() - Send idle ack.
 * @fw:		Pointer to firmware object.
 *
 * Return: 0 on success, else error code.
 */
int mvx_fw_send_idle_ack_v2(struct mvx_fw *fw);

/**
 * mvx_fw_to_mve_profile_v2() - Convert MVX to MVE profile.
 * @mvx_profile:	Input profile.
 * @mve_profile:	Output profile.
 *
 * Return: 0 on success, else error code.
 */
int mvx_fw_to_mve_profile_v2(unsigned int mvx_profile,
			     uint16_t *mve_profile);

/**
 * mvx_fw_to_mve_level_v2() - Convert MVX to MVE level.
 * @mvx_level:		Input level.
 * @mve_level:		Output level.
 *
 * Return: 0 on success, else error code.
 */
int mvx_fw_to_mve_level_v2(unsigned int mvx_level,
			   uint16_t *mve_level);

/****************************************************************************
 * Firmware v3
 ****************************************************************************/

/**
 * mvx_fw_construct_v3() - Construct the object for the firmware v3 interface.
 * @fw:		Pointer to firmware object.
 * @fw_bin:	Pointer to firmware binary.
 * @mmu:	Pointer to MMU.
 * @session:	Pointer to session.
 * @client_ops:	Pointer to client operations.
 * @csession:	Pointer to client session.
 * ncores:	Number of cores.
 * @major:	Major firmware version.
 * @minor:	Minor firmware version.
 *
 * Return: 0 on sucess, else error code.
 */
int mvx_fw_construct_v3(struct mvx_fw *fw,
			struct mvx_fw_bin *fw_bin,
			struct mvx_mmu *mmu,
			struct mvx_session *session,
			struct mvx_client_ops *client_ops,
			struct mvx_client_session *csession,
			unsigned int ncores,
			unsigned char major,
			unsigned char minor);

#endif
