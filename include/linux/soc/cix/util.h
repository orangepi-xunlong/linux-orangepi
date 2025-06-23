/*
 * util.h
 *
 * balong platform misc utilities function.
 *
 * Copyright (c) 2012-2020 Huawei Technologies Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */
#ifndef __MACH_UTIL_H__
#define __MACH_UTIL_H__

#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/sysctl.h>

#define HIMNTN_VALID_SIZE 32
#define DST_TMP_PATH_MAX_LEN 64

extern int check_himntn(int feature);
extern int get_himntn_value(int feature);
extern u32 atoi(char *s);
extern u32 checksum32(u32 *addr, u32 count);

inline int rdr_file_exists(const char *path);
int rdr_vfs_stat(const char* path, struct kstat *stat);
int rdr_rm_file(const char *nodename);
int rdr_chown(const char *folder_path, uid_t user, gid_t group, bool recursion);
int rdr_create_dir(const char *path);
int rdr_rm_dir(const char *name);
int rdr_dir_size(const char *path, u32 path_len, bool recursion);

unsigned long dst_get_phy_addr(unsigned long addr);

/*
 * func name: rdr_savebuf2fs
 * append(save) data to path.
 * func args:
 *  char*  path,     path of save file.
 *  void*  buf,      save data.
 *  u32 len,         data lenght.
 * return
 *  >=len fail
 *  ==len success
 */
int rdr_savebuf2fs(const char *path, const char *name, const void *buf, u32 len, u32 is_append);

int rdr_savebuf2fs_compressed(const char *logpath, const char *filename, const void *buf, u32 len);

#endif
