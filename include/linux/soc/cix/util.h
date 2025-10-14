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

extern int check_himntn(int feature);
extern int get_himntn_value(int feature);
extern u32 atoi(char *s);
extern u32 checksum32(u32 *addr, u32 count);

static inline void __flush_dcache_all(void)
{
	__asm__ __volatile__("dmb sy\n\t"
			     "mrs x0, clidr_el1\n\t"
			     "and x3, x0, #0x7000000\n\t"
			     "lsr x3, x3, #23\n\t"
			     "cbz x3, 5f\n\t"
			     "mov x10, #0\n\t"
			     "1:\n\t"
			     "add x2, x10, x10, lsr #1\n\t"
			     "lsr x1, x0, x2\n\t"
			     "and x1, x1, #7\n\t"
			     "cmp x1, #2\n\t"
			     "b.lt    4f\n\t"
			     "mrs x9, daif\n\t"
			     "msr daifset, #2\n\t"
			     "msr csselr_el1, x10\n\t"
			     "isb\n\t"
			     "mrs x1, ccsidr_el1\n\t"
			     "msr daif, x9\n\t"
			     "and x2, x1, #7\n\t"
			     "add x2, x2, #4\n\t"
			     "mov x4, #0x3ff\n\t"
			     "and x4, x4, x1, lsr #3\n\t"
			     "clz w5, w4\n\t"
			     "mov x7, #0x7fff\n\t"
			     "and x7,x7, x1, lsr #13\n\t"
			     "2:\n\t"
			     "mov x9, x4\n\t"
			     "3:\n\t"
			     "lsl x6, x9, x5\n\t"
			     "orr x11, x10, x6\n\t"
			     "lsl x6, x7, x2\n\t"
			     "orr x11, x11, x6\n\t"
			     "dc  csw, x11\n\t"
			     "subs    x9, x9, #1\n\t"
			     "b.ge    3b\n\t"
			     "subs    x7, x7, #1\n\t"
			     "b.ge    2b\n\t"
			     "4:\n\t"
			     "add x10, x10, #2\n\t"
			     "cmp x3, x10\n\t"
			     "b.gt    1b\n\t"
			     "5:\n\t"
			     "mov x10, #0\n\t"
			     "msr csselr_el1, x10\n\t"
			     "dsb sy\n\t"
			     "isb\n\t");
}

inline int rdr_file_exists(const char *path);
int rdr_vfs_stat(const char *path, struct kstat *stat);
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
int rdr_savebuf2fs(const char *path, const char *name, const void *buf, u32 len,
		   u32 is_append);

int rdr_savebuf2fs_compressed(const char *logpath, const char *filename,
			      const void *buf, u32 len);
int dst_sec_call(char cmd, uint64_t arg0, uint64_t arg1, uint64_t arg2);

#endif
