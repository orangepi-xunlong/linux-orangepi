/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner SoCs display driver.
 *
 * Copyright (C) 2020 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include "../dev_disp.h"
#include <uapi/video/sunxi_display2.h>
#include "drv_pq.h"
#include <linux/file.h>

struct pq_private_data g_pq_data;

int pq_enable(u32 sel, u32 en)
{
	if (en)
		g_pq_data.enabled = 1;
	else
		g_pq_data.enabled = 0;
	return 0;
}

int pq_set_register(u32 sel, struct pq_reg *reg, u32 num)
{
	int i = 0;

	g_pq_data.shadow_protect(sel, 1);
	for (i = 0; i < num; i++) {
		pq_set_reg(sel, reg[i].offset, reg[i].value);
	}
	g_pq_data.shadow_protect(sel, 0);
	return 0;
}

int pq_get_register(u32 sel, struct pq_reg *reg, u32 num)
{
	int i = 0;

	/* need shadow protect? */
	for (i = 0; i < num; i++) {
		pq_get_reg(sel, reg[i].offset, &reg[i].value);
	}

	return 0;
}

static int pq_ioctl(unsigned int cmd, unsigned long arg)
{
	int ret = -EFAULT;
	struct pq_reg *reg;
	unsigned long *ubuffer;
	ubuffer = (unsigned long *)arg;

	if (cmd == DISP_PQ_PROC) {
		switch (ubuffer[0]) {
		case PQ_SET_REG:
				reg = (struct pq_reg *)kmalloc(sizeof(struct pq_reg) * ubuffer[3], GFP_KERNEL);
				if (copy_from_user(reg, (void __user *)ubuffer[2],
							sizeof(struct pq_reg) * ubuffer[3])) {
					DE_WARN("regs copy from user failed\n");
					return -1;
				}
				ret = pq_set_register((u32)ubuffer[1], reg, (u32)ubuffer[3]);
				if (ret < 0) {
					DE_WARN("pq set register failed\n");
					ret = -EFAULT;
				}
				kfree(reg);
				reg = NULL;
				break;
		case PQ_GET_REG:
				reg = (struct pq_reg *)kzalloc(sizeof(struct pq_reg) * ubuffer[3],  GFP_KERNEL);
				if (reg == NULL) {
					DE_WARN("PQ GET REG malloc failed\n");
					return -1;
				}
				if (copy_from_user(reg, (void __user *)ubuffer[2],
							sizeof(struct pq_reg) * ubuffer[3])) {
					DE_WARN("regs copy from user failed\n");
					return -1;
				}

				ret = pq_get_register((u32)ubuffer[1], reg, ubuffer[3]);
				if (ret) {
					DE_WARN("GET PQ REG failed\n");
					ret = -EFAULT;
				}

				if (copy_to_user((void __user *)ubuffer[2], reg,
							sizeof(struct pq_reg) * ubuffer[3])) {
					DE_WARN("copy to user REG failed\n");
					ret = -EFAULT;
				}
				kfree(reg);
				reg = NULL;
				break;
		case PQ_ENABLE:
				ret = pq_enable(ubuffer[1], ubuffer[2]);
				break;
		case PQ_COLOR_MATRIX:
			{
				struct color_matrix pq_mat;
				struct color_enhanc pq_ehance;
				struct disp_enhance *enhance;
				ret = 0;
				if (copy_from_user(&pq_mat, (void __user *)ubuffer[2],
							sizeof(struct color_matrix))) {
					DE_WARN("pq color_matrix copy from user failed\n");
					return -1;
				}
				if (pq_mat.cmd == 0) {
					if (copy_from_user(&pq_ehance, (void __user *)ubuffer[2],
								sizeof(struct color_enhanc))) {
						DE_WARN("pq color_enhanc copy from user failed\n");
						return -1;
					}
					pq_get_set_enhance(ubuffer[1], &pq_ehance, pq_mat.read);
					if (pq_mat.read) {
						if (copy_to_user((void __user *)ubuffer[2], &pq_ehance,
								sizeof(struct color_enhanc))) {
							DE_WARN("copy color_enhanc to user REG failed\n");
							ret = -EFAULT;
						}
					}
				} else {
					struct matrix_user *mu =
						(struct matrix_user *)kzalloc(sizeof(struct matrix_user), GFP_KERNEL);
					if (mu == NULL) {
						DE_WARN("PQ matrix_user malloc failed\n");
						return -1;
					}
					if (copy_from_user(mu, (void __user *)ubuffer[2],
								sizeof(struct matrix_user))) {
						DE_WARN("pq matrix_user copy from user failed\n");
						kfree(mu);
						return -1;
					}
					pq_get_set_matrix(&mu->matrix, mu->choice, pq_mat.cmd - 1, !pq_mat.read);
					if (pq_mat.read) {
						if (copy_to_user((void __user *)ubuffer[2], mu,
								sizeof(struct matrix_user))) {
							DE_WARN("copy matrix_user to user REG failed\n");
							ret = -EFAULT;
						}
					}
					kfree(mu);
				}
				enhance = disp_get_enhance(ubuffer[1]);
				enhance->pq_apply(enhance);
			}
			break;
#if defined(DE_VERSION_V35X)
		case PQ_FCM:
			{
				struct fcm_info *fcm_inf;
				fcm_inf = (struct fcm_info *)kzalloc(sizeof(struct fcm_info), GFP_KERNEL);

				if (fcm_inf == NULL) {
					pr_err("kzalloc struct fcm_inf failed!\n");
					return -EFAULT;
				}
				if (copy_from_user(fcm_inf, (void __user *)ubuffer[2],
								   sizeof(struct fcm_info))) {
					DE_WARN("regs copy from user failed\n");
					return -1;
				}

				g_pq_data.shadow_protect(ubuffer[1], 1);
				ret = disp_al_fcm_proc(ubuffer[1], fcm_inf->cmd, &fcm_inf->fcm_data);
				g_pq_data.shadow_protect(ubuffer[1], 0);
				if (fcm_inf->cmd == 2) { /*read*/
					if (copy_to_user((void __user *)ubuffer[2], fcm_inf,
									 sizeof(struct fcm_info))) {
						DE_WARN("copy to user REG failed\n");
						ret = -EFAULT;
					}
				}
			}
			break;
		case PQ_DCI:
		case PQ_DEBAND:
		case PQ_SHARP35X:
		case PQ_SNR:
		case PQ_GTM:
		case PQ_ASU:
			{
				void *pqinfo = (void *)kzalloc(ubuffer[3],  GFP_KERNEL);

				DE_INFO("cmd=%lu, disp=%lu, size=%lu\n", ubuffer[1] & DISP_MASK,
						ubuffer[1] & CMD_MASK, ubuffer[3]);
				if (copy_from_user(pqinfo, (void __user *)ubuffer[2],
								   ubuffer[3])) {
					DE_WARN("regs copy from user failed\n");
					return -1;
				}

				g_pq_data.shadow_protect(ubuffer[1] & DISP_MASK, 1);
				ret = disp_al_pq_proc(ubuffer[1] & DISP_MASK, ubuffer[0],
									  ubuffer[1] & CMD_MASK, pqinfo);
				g_pq_data.shadow_protect(ubuffer[1] & DISP_MASK, 0);
				if ((ubuffer[1] & CMD_MASK) == 16) { /*read*/
					if (copy_to_user((void __user *)ubuffer[2], pqinfo,
									 ubuffer[3])) {
						DE_WARN("copy to user REG failed\n");
						ret = -EFAULT;
					}
				}
			}
			break;
#endif
		default:
			DE_WARN("pq give a err ioctl%lu\n", ubuffer[0]);
		}
	}
	return ret;
}

s32 pq_init(struct disp_bsp_init_para *para)
{

	DE_DEBUG("\n");
	disp_register_ioctl_func(DISP_PQ_PROC, pq_ioctl);
#if IS_ENABLED(CONFIG_COMPAT)
	disp_register_compat_ioctl_func(DISP_PQ_PROC, pq_ioctl);
#endif
	g_pq_data.shadow_protect = para->shadow_protect;

	return 0;
}
