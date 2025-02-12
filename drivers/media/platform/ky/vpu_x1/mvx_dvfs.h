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

#ifndef _MVX_DVFS_H_
#define _MVX_DVFS_H_

typedef void *mvx_session_id;

struct estimate_ddr_input
{
    int width;
    int height;
    int isAFBC;
    int fps;
    int isEnc;
};

struct estimate_ddr_output
{
    uint64_t estimated_read;
    uint64_t estimated_write;
};

/**
 * Initialize the DVFS module.
 *
 * Must be called before any other function in this module.
 *
 * @param dev Device
 */
void mvx_dvfs_init(struct device *dev);

/**
 * Deinitialize the DVFS module.
 *
 * All remaining sessions will be unregistered.
 *
 * @param dev Device
 */
void mvx_dvfs_deinit(struct device *dev);

/**
 * Register session in the DFVS module.
 *
 * @param session_id Session id
 * @return True when registration was successful,
 *         False, otherwise
 */
bool mvx_dvfs_register_session(const mvx_session_id session_id, bool is_encoder);

/**
 * Unregister session from the DFVS module.
 *
 * Usage of corresponding session is not permitted after this call.
 * @param session_id Session id
 */
void mvx_dvfs_unregister_session(const mvx_session_id session_id);

void mvx_dvfs_estimate_ddr_bandwidth(struct estimate_ddr_input* input, struct estimate_ddr_output* output);

void mvx_dvfs_session_update_ddr_qos(const mvx_session_id session_id, uint32_t read_value, uint32_t write_value);

/**
 * Suspend dvfs thread to adjust vpu clk when device enters suspend state.
 */
void mvx_dvfs_suspend_session(void);

/**
 * Resume dvfs thread to adjust vpu clk when device resumes from suspend state.
 */
void mvx_dvfs_resume_session(void);

#endif /* MVX_DVFS_H */
