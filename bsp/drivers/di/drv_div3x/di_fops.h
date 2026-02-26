/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Copyright (c) 2007-2018 Allwinnertech Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#ifndef _DI_FOPS_H_
#define _DI_FOPS_H_

#include <linux/fs.h>

long di_ioctl(struct file *file, unsigned int cmd, unsigned long arg);
int di_open(struct inode *inode, struct file *file);
int di_release(struct inode *inode, struct file *file);
int di_mmap(struct file *file, struct vm_area_struct *vma);

#endif /* #ifndef _DI_FOPS_H_ */