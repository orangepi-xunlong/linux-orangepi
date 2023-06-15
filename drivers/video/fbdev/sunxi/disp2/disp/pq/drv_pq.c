/*
 * Allwinner SoCs display driver.
 *
 * Copyright (C) 2020 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <linux/file.h>
#include "../dev_disp.h"
#include <video/sunxi_display2.h>
#include "drv_pq.h"

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

	//need shadow protect?
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
					pr_err("regs copy from user failed!\n");
					return -1;
				}
				ret = pq_set_register((u32)ubuffer[1], reg, (u32)ubuffer[3]);
				if (ret < 0) {
					pr_err("pq set register failed!\n");
					ret = -EFAULT;
				}
				kfree(reg);
				reg = NULL;
				break;
			case PQ_GET_REG:
				reg = (struct pq_reg *)kzalloc(sizeof(struct pq_reg) * ubuffer[3],  GFP_KERNEL);
				if (reg == NULL) {
					pr_err("PQ GET REG malloc failed!\n");
					return -1;
				}
				if (copy_from_user(reg, (void __user *)ubuffer[2],
							sizeof(struct pq_reg) * ubuffer[3])) {
					pr_err("regs copy from user failed!\n");
					return -1;
				}

				ret = pq_get_register((u32)ubuffer[1], reg, ubuffer[3]);
				if (ret) {
					pr_err("GET PQ REG failed\n");
					ret = -EFAULT;
				}

				if (copy_to_user((void __user *)ubuffer[2], reg,
							sizeof(struct pq_reg) * ubuffer[3])) {
					pr_err("copy to user REG failed!\n");
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
				ret = 0;
				if (copy_from_user(&pq_mat, (void __user *)ubuffer[2],
							sizeof(struct color_matrix))) {
					pr_err("pq color_matrix copy from user failed!\n");
					return -1;
				}
				if (pq_mat.cmd == 0) {
					if (copy_from_user(&pq_ehance, (void __user *)ubuffer[2],
								sizeof(struct color_enhanc))) {
						pr_err("pq color_enhanc copy from user failed!\n");
						return -1;
					}
					pq_set_enhance(&pq_ehance, pq_mat.read);
					if (pq_mat.read) {
						if (copy_to_user((void __user *)ubuffer[2], &pq_ehance,
								sizeof(struct color_enhanc))) {
							pr_err("copy color_enhanc to user REG failed!\n");
							ret = -EFAULT;
						}
					}
				} else {
					struct matrix_user *mu =
						(struct matrix_user *)kzalloc(sizeof(struct matrix_user), GFP_KERNEL);
					if (mu == NULL) {
						pr_err("PQ matrix_user malloc failed!\n");
						return -1;
					}
					if (copy_from_user(mu, (void __user *)ubuffer[2],
								sizeof(struct matrix_user))) {
						pr_err("pq matrix_user copy from user failed!\n");
						kfree(mu);
						return -1;
					}
					pq_set_matrix(&mu->matrix, mu->choice, pq_mat.cmd - 1, !pq_mat.read);
					if (pq_mat.read) {
						if (copy_to_user((void __user *)ubuffer[2], mu,
								sizeof(struct matrix_user))) {
							pr_err("copy matrix_user to user REG failed!\n");
							ret = -EFAULT;
						}
					}
					kfree(mu);
				}
				disp_pq_force_flush(ubuffer[1]);
			}
			break;
			default:
			pr_warn("pq give a err ioctl%lu.\n" , ubuffer[0]);
		}
	}
	return ret;
}

s32 pq_init(struct disp_bsp_init_para *para)
{

	printk("[%s]+++\n", __func__);
	disp_register_ioctl_func(DISP_PQ_PROC, pq_ioctl);
#if defined(CONFIG_COMPAT)
	disp_register_compat_ioctl_func(DISP_PQ_PROC, pq_ioctl);
#endif
	g_pq_data.shadow_protect = para->shadow_protect;

	return 0;
}
