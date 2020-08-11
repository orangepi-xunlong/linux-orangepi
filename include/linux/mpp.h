/*
 *  mpp.h - mpp debug file system
 *
 *  Copyright (C) 2017 yanggq <yanggq@allwinnertech.com>
 *  Copyright (C) 2017 Allwinnertech.
 *
 *	This program is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License version
 *	2 as published by the Free Software Foundation.
 *
 *  mpp is for people to use instead of /proc or /sys.
 */

#ifndef _MPP_H_
#define _MPP_H_

#include <linux/fs.h>
#include <linux/seq_file.h>
#include <linux/types.h>

extern struct dentry *debugfs_mpp_root;

#endif
