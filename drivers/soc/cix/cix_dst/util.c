// SPDX-License-Identifier: GPL-2.0-only
/*
 * balong platform misc utilities function
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2012-2019. All rights reserved.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#include <linux/mm.h>
#include <linux/printk.h>
#include <linux/file.h>
#include <linux/namei.h>
#include <linux/io.h>
#include <linux/arm-smccc.h>
#include <linux/soc/cix/util.h>
#include <mntn_public_interface.h>
#include <linux/zlib.h>
#include <linux/mm.h>
#include "dst_print.h"

#define PATH_MAXLEN 128
#define ROOT_UID 0
#define SYSTEM_GID 1000
#define DIR_LIMIT (S_IRWXU | S_IRWXG)
#define FILE_LIMIT (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP)
#define CIX_SIP_SVC_DST_CMD 0xc2000012UL

static char himntn[HIMNTN_VALID_SIZE + 1] = "11111000000000000000000";
static struct mnt_idmap *util_mnt_idmap = &nop_mnt_idmap;

static char g_rdr_flag[] = "cix dst";
static struct mnt_idmap *rdr_mnt_idmap = &nop_mnt_idmap;

/*
 * Determine if the nv is open
 * input: feature:position of himntn
 */
int check_himntn(int feature)
{
	int ret = 0;

	if (feature >= HIMNTN_BOTTOM || feature < 0)
		goto out;

	if (himntn[feature] == '1')
		ret = 1;

out:
	return ret;
}

int get_himntn_value(int feature)
{
	int ret = 0;

	if (feature >= HIMNTN_BOTTOM || feature < 0)
		goto out;

	ret = himntn[feature];

out:
	return ret;
}

u32 checksum32(u32 *addr, u32 count)
{
	u64 sum = 0;
	u32 i;

	while (count > sizeof(u32) - 1) {
		/*  This is the inner loop */
		sum += *(addr++);
		count -= sizeof(u32);
	}

	if (count > 0) {
		u32 left = 0;

		i = 0;
		while (i <= count) {
			*((u8 *)&left + i) = *((u8 *)addr + i);
			i++;
		}

		sum += left;
	}

	while (sum >> 32)
		sum = (sum & 0xffffffff) + (sum >> 32);

	return (~sum);
}

static int __init plat_mntn_init(void)
{
	int index;
	u32 nv_value;
	u64 nv_addr = (uintptr_t)ioremap_wc(SKY1_DFD_NV_ADDR, sizeof(int));

	if (nv_addr) {
		nv_value = readl((void *)(uintptr_t)nv_addr);
		DST_DBG("DFD NV: value=0x%x\n", nv_value);

		for (index = 0; index < HIMNTN_VALID_SIZE; index++)
			himntn[index] = (nv_value & (1 << index)) ? '1' : '0';
	}

	DST_PN("DFD NV: platmntn=%s\n", himntn);
	return 0;
}
early_initcall(plat_mntn_init);

int rdr_file_exists(const char *path)
{
	struct kstat stat;

	return rdr_vfs_stat(path, &stat) ? 0 : 1;
}

int rdr_rm_file(const char *nodename)
{
	struct path parent;
	struct dentry *dentry;
	int deleted = 0;
	int err;

	dentry = kern_path_locked(nodename, &parent);
	if (IS_ERR(dentry))
		return PTR_ERR(dentry);

	if (d_really_is_positive(dentry)) {
		struct kstat stat;
		struct path p = { .mnt = parent.mnt, .dentry = dentry };

		err = vfs_getattr(&p, &stat, STATX_TYPE | STATX_MODE,
				  AT_STATX_SYNC_AS_STAT);
		if (!err) {
			struct iattr newattrs;
			/*
			 * before unlinking this node, reset permissions
			 * of possible references like hardlinks
			 */
			newattrs.ia_uid = GLOBAL_ROOT_UID;
			newattrs.ia_gid = GLOBAL_ROOT_GID;
			newattrs.ia_mode = stat.mode & ~0777;
			newattrs.ia_valid = ATTR_UID | ATTR_GID | ATTR_MODE;
			inode_lock(d_inode(dentry));
			notify_change(util_mnt_idmap, dentry, &newattrs, NULL);
			inode_unlock(d_inode(dentry));
			err = vfs_unlink(util_mnt_idmap, d_inode(parent.dentry),
					 dentry, NULL);
			if (!err || err == -ENOENT)
				deleted = 1;
		}
	} else {
		err = -ENOENT;
	}
	dput(dentry);
	inode_unlock(d_inode(parent.dentry));

	path_put(&parent);

	return err;
}

struct getdents_callback {
	struct dir_context ctx;
	const char *path;
	uid_t user;
	gid_t group;
	bool recursion;
};

static int rdr_path_chown(const char *file_path, uid_t user, gid_t group)
{
	struct path path;
	int error;
	struct iattr newattrs;
	kuid_t uid;
	kgid_t gid;

	uid.val = user;
	gid.val = group;

	error = kern_path(file_path, LOOKUP_FOLLOW, &path);
	if (error) {
		DST_ERR("Failed to get file path(%s)\n", file_path);
		return -1;
	}

	newattrs.ia_uid = uid;
	newattrs.ia_gid = gid;
	newattrs.ia_valid = ATTR_UID | ATTR_GID;
	inode_lock(d_inode(path.dentry));
	if (notify_change(util_mnt_idmap, path.dentry, &newattrs, NULL)) {
		DST_ERR("change owner failed path:%s\n", file_path);
		inode_unlock(d_inode(path.dentry));
		return -1;
	}
	inode_unlock(d_inode(path.dentry));

	return 0;
}

static bool rdr_process_file_owner(struct dir_context *ctx, const char *name,
				   int namlen, loff_t offset, u64 ino,
				   unsigned int d_type)
{
	struct kstat stat;
	char fullname[PATH_MAXLEN];
	struct getdents_callback *cb =
		container_of(ctx, struct getdents_callback, ctx);

	memset(fullname, 0, PATH_MAXLEN);
	(void)snprintf(fullname, sizeof(fullname), "%s/%s", cb->path, name);

	if (rdr_vfs_stat(fullname, &stat) != 0) {
		DST_ERR("path:%s stat failed\n", fullname);
		return false;
	}

	if (S_ISDIR(stat.mode)) {
		DST_DBG("path:%s, name=%s\n", cb->path, name);
		if (strncmp(name, ".", 1) && strncmp(name, "..", 2))
			rdr_chown(fullname, cb->user, cb->group, cb->recursion);
		return true;
	} else if (S_ISREG(stat.mode)) {
		DST_DBG("file:%s\n", fullname);
		rdr_path_chown(fullname, cb->user, cb->group);
	}

	return true;
}

int rdr_chown(const char *folder_path, uid_t user, gid_t group, bool recursion)
{
	struct file *file;
	struct path kpath;
	int ret;
	struct getdents_callback cb = {
		.ctx.actor = rdr_process_file_owner,
		.user = user,
		.group = group,
		.recursion = recursion,
	};

	if (folder_path == NULL) {
		DST_ERR("rdr:path is null\n");
		return 0;
	}

	rdr_path_chown(folder_path, user, group);

	if (!recursion)
		goto not_recursion;

	ret = kern_path(folder_path, 0, &kpath);
	if (ret) {
		DST_ERR("Failed to get path:%s %d\n", folder_path, ret);
		return 0;
	}

	// open the directory
	file = dentry_open(&kpath, O_RDONLY, current_cred());
	if (IS_ERR(file)) {
		DST_ERR("open %s fail\n", folder_path);
		return 0;
	}

	cb.path = folder_path;
	// iterate over the directory entries
	ret = iterate_dir(file, &cb.ctx);

	// clean up
	fput(file);
	path_put(&kpath);

	if (ret) {
		DST_ERR("rdr failed to iterate directory: %d\n", ret);
		return 0;
	}

not_recursion:
	return 0;
}

int rdr_vfs_stat(const char *path, struct kstat *stat)
{
	struct path p;
	int ret = kern_path(path, 0, &p);

	if (ret)
		return ret;

	ret = vfs_getattr(&p, stat, STATX_BASIC_STATS, 0);
	if (ret)
		DST_ERR("failed to getattr: %s %d\n", path, ret);

	path_put(&p);

	return ret;
}

static int create_dir(const char *name, umode_t mode)
{
	struct dentry *dentry;
	struct path path;
	int err;

	DST_DBG("need create dir %s\n", name);
	dentry = kern_path_create(AT_FDCWD, name, &path, LOOKUP_DIRECTORY);
	if (IS_ERR(dentry)) {
		DST_ERR("failed to kern_path_create: %s %ld\n", name,
			PTR_ERR(dentry));
		return PTR_ERR(dentry);
	}

	err = vfs_mkdir(rdr_mnt_idmap, d_inode(path.dentry), dentry, mode);
	if (!err)
		/* mark as kernel-created inode */
		d_inode(dentry)->i_private = (void *)g_rdr_flag;
	else
		DST_ERR("failed to mkdir: %s %d\n", name, err);
	done_path_create(&path, dentry);
	return err;
}

static int __rdr_create_dir(const char *path)
{
	struct kstat m_stat;
	int err = 0;

	if (path == NULL) {
		DST_ERR("invalid parameter path\n");
		DST_PR_END();
		return -1;
	}
	if (rdr_vfs_stat(path, &m_stat) != 0)
		err = create_dir(path, DIR_LIMIT);

	if (err)
		DST_ERR("create dir %s err!!!\n", path);

	return err;
}

int rdr_create_dir(const char *path)
{
	char *cur_path;
	int index = 0;

	DST_PR_START();
	cur_path = kzalloc(PATH_MAX, GFP_KERNEL);
	if (path == NULL || IS_ERR_OR_NULL(cur_path)) {
		DST_ERR("invalid  parameter path\n");
		DST_PR_END();
		return -1;
	}
	memset(cur_path, 0, PATH_MAX);

	if (*path != '/')
		return -1;
	cur_path[index++] = *path++;

	while (*path != '\0') {
		if (*path == '/') {
			if (__rdr_create_dir(cur_path) < 0)
				DST_ERR("rdr: create dir failed\n");
		}
		cur_path[index] = *path;
		path++;
		index++;
	}
	kfree(cur_path);

	DST_PR_END();
	return 0;
}

int rdr_rm_dir(const char *name)
{
	struct path parent;
	struct dentry *dentry;
	struct dentry *child;
	int err;
	char path[PATH_MAXLEN];

	DST_DBG("rm dir:%s\n", name);

	dentry = kern_path_locked(name, &parent);
	if (IS_ERR(dentry))
		return PTR_ERR(dentry);

	if (!d_is_dir(dentry)) {
		err = -ENOENT;
		goto exit_rm;
	}

	list_for_each_entry(child, &dentry->d_subdirs, d_child) {
		if (!d_inode(child))
			continue;
		d_inode(child)->i_private = NULL;
		DST_DBG("want to delete: %s\n", child->d_name.name);
		if (d_is_dir(child)) {
			memset(path, 0, PATH_MAXLEN);
			err = snprintf(path, PATH_MAXLEN, "%s/%s", name,
				       child->d_name.name);
			if (err)
				err = rdr_rm_dir(path);
			else
				err = -EIO;
		} else {
			err = vfs_unlink(rdr_mnt_idmap, d_inode(dentry), child,
					 NULL);
		}
		if (err) {
			DST_ERR("rm failed: %s\n", child->d_name.name);
			goto exit_rm;
		}
	}

	err = vfs_rmdir(rdr_mnt_idmap, d_inode(parent.dentry), dentry);
	if (err)
		DST_ERR("rm dir %s failed\n", name);

exit_rm:
	dput(dentry);
	inode_unlock(d_inode(parent.dentry));
	path_put(&parent);
	return err;
}

unsigned long dst_get_phy_addr(unsigned long addr)
{
	struct mm_struct *mm = NULL;
	struct vm_area_struct *vma;
	struct page *page;
	unsigned long val = 0;

	if (is_ttbr0_addr(addr)) {
		mm = current->active_mm;
		DST_DBG("user addr, mm: %px\n", mm);
	} else if (is_ttbr1_addr(addr)) {
		/* TTBR1 */
		mm = &init_mm;
		DST_DBG("kernel addr, mm: %px\n", mm);
	}

	if (IS_ERR_OR_NULL(mm))
		mm = &init_mm;
	DST_DBG("mm: %px\n", mm);

	vma = find_vma(mm, untagged_addr(addr));
	DST_DBG("vma: %px\n", vma);
	if (IS_ERR_OR_NULL(vma))
		return 0;

	DST_DBG("vma_start: 0x%lx, vma_end: 0x%lx\n", vma->vm_start,
		vma->vm_end);
	if (addr < vma->vm_start || addr > vma->vm_end)
		return 0;

	page = follow_page(vma, addr, FOLL_GET);
	DST_DBG("page: %px\n", page);
	if (IS_ERR_OR_NULL(page))
		return 0;

	val = page_to_phys(page) | (addr & ~PAGE_MASK);
	put_page(page);
	return val;
}

/*
 * func descripton:
 *  append(save) data to path
 * func args:
 *  char*  path,        path of save file
 *  char*  name,        name of save file
 *  void*  buf,         save data
 *  u32 len,            data lenght
 *  u32 is_append,      determine whether write with append
 * return
 *  >=len fail
 *  ==len success
 */
int rdr_savebuf2fs(const char *logpath, const char *filename, const void *buf,
		   u32 len, u32 is_append)
{
	int ret, flags;
	struct file *fp = NULL;
	char path[PATH_MAXLEN];

	if (logpath == NULL || filename == NULL || buf == NULL || len <= 0) {
		DST_ERR("invalid  parameter. path:%px, name:%px buf:%px len:0x%x\n",
			logpath, filename, buf, len);
		ret = -1;
		goto end;
	}

	(void)snprintf(path, PATH_MAXLEN, "%s/%s", logpath, filename);

	flags = O_CREAT | O_RDWR | (is_append ? O_APPEND : O_TRUNC);
	fp = filp_open(path, flags, FILE_LIMIT);
	if (IS_ERR(fp)) {
		DST_ERR("create file %s err. fp=0x%pK\n", path, fp);
		ret = -1;
		goto end;
	}
	vfs_llseek(fp, 0L, SEEK_END);
	ret = kernel_write(fp, buf, len, &(fp->f_pos));
	if (ret != len) {
		DST_ERR("write file %s exception with ret %d\n", path, ret);
		goto close;
	}

	vfs_fsync(fp, 0);
close:
	filp_close(fp, NULL);

	/*
	 * According to the permission requirements,
	 * the hisi_logs directory and subdirectory group are adjusted to root-system.
	 */
	ret = (int)rdr_chown((const char __user *)path, ROOT_UID, SYSTEM_GID,
			     false);
	if (ret)
		DST_ERR("chown %s uid [%d] gid [%d] failed err [%d]\n", path,
			ROOT_UID, SYSTEM_GID, ret);
end:
	return ret;
}
EXPORT_SYMBOL(rdr_savebuf2fs);

#define CRC32_POLYNOMIAL 0xEDB88320
static void generate_crc32_table(u32 *crc32_table)
{
	u32 i, j;
	u32 crc;

	for (i = 0; i < 256; i++) {
		crc = i;
		for (j = 8; j > 0; j--) {
			if (crc & 1)
				crc = (crc >> 1) ^ CRC32_POLYNOMIAL;
			else
				crc >>= 1;
		}
		crc32_table[i] = crc;
	}
}

static u32 calculate_crc32(const unsigned char *data, size_t length)
{
	static u32 crc32_table[256];
	static int table_generated;
	u32 crc = 0xFFFFFFFF;
	u8 byte;
	u32 index;

	if (!table_generated) {
		generate_crc32_table(crc32_table);
		table_generated = 1;
	}

	for (size_t i = 0; i < length; i++) {
		byte = data[i];
		index = (crc ^ byte) & 0xFF;
		crc = (crc >> 8) ^ crc32_table[index];
	}
	return crc ^ 0xFFFFFFFF;
}

static int compress_kernel_data_gz(const unsigned char *src, size_t src_len,
				   unsigned char **dst, size_t *dst_len)
{
	int ret;
	struct z_stream_s stream;
	const int gzip_header_offset = 10; // gzip header 10 bytes
	u32 crc;
	const int gzip_footer_size = 8; // gzip tail 8 bytes
	size_t max_dst_len = src_len;

	*dst = vmalloc(max_dst_len + gzip_header_offset + gzip_footer_size);
	if (!*dst)
		return -ENOMEM;

	crc = calculate_crc32(src, src_len);
	memset(&stream, 0, sizeof(stream));
	stream.workspace =
		vmalloc(zlib_deflate_workspacesize(MAX_WBITS, MAX_MEM_LEVEL));
	if (!stream.workspace) {
		vfree(*dst);
		return -ENOMEM;
	}

	ret = zlib_deflateInit2(&stream, Z_DEFAULT_COMPRESSION, Z_DEFLATED,
				-MAX_WBITS, DEF_MEM_LEVEL, Z_DEFAULT_STRATEGY);

	// set gzip header
	memcpy(*dst, "\x1F\x8B\x08\x00\x00\x00\x00\x00\x00\xFF",
	       gzip_header_offset);
	// set input & output stream
	stream.next_in = (unsigned char *)src;
	stream.avail_in = src_len;
	stream.next_out = *dst + gzip_header_offset;
	stream.avail_out = max_dst_len;

	// begin to compress
	ret = zlib_deflate(&stream, Z_FINISH);
	zlib_deflateEnd(&stream); // end compression
	if (ret != Z_STREAM_END) {
		vfree(stream.workspace);
		vfree(*dst);
		return ret != Z_OK ? ret : -EIO;
	}

	// add gzip tail info: CRC32 & origin size
	*dst_len = stream.total_out + gzip_header_offset;
	memcpy(*dst + *dst_len, &crc, 4);
	*dst_len += 4;
	memcpy(*dst + *dst_len, &src_len, 4);
	*dst_len += 4;
	vfree(stream.workspace);
	return 0;
}

int rdr_savebuf2fs_compressed(const char *logpath, const char *filename,
			      const void *buf, u32 len)
{
	int ret;
	char xz_filename[PATH_MAXLEN];
	unsigned char *xzbuff;
	unsigned long xzlen;

	if (logpath == NULL || filename == NULL || buf == NULL || len <= 0) {
		DST_ERR("invalid  parameter. path:%pK, name:%pK buf:%pK len:0x%x\n",
			logpath, filename, buf, len);
		return -1;
	}

	ret = compress_kernel_data_gz(buf, len, &xzbuff, &xzlen);
	if (ret)
		return ret;

	(void)snprintf(xz_filename, PATH_MAXLEN, "%s.gz", filename);

	ret = rdr_savebuf2fs(logpath, xz_filename, xzbuff, xzlen, 0);

	vfree(xzbuff);

	return ret;
}
EXPORT_SYMBOL(rdr_savebuf2fs_compressed);

int dst_sec_call(char cmd, uint64_t arg0, uint64_t arg1, uint64_t arg2)
{
	struct arm_smccc_res res;

	arm_smccc_smc(CIX_SIP_SVC_DST_CMD, cmd, arg0, arg1, arg2, 0, 0, 0,
		      &res);
	if (res.a0)
		return -1;
	return 0;
}
