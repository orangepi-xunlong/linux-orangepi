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
#include "linux/printk.h"
#include <linux/file.h>
#include <linux/namei.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/device.h>
#include <linux/io.h>
#include <linux/kmod.h>
#include <linux/slab.h>
#include <linux/proc_fs.h>
#include <linux/sysctl.h>
#include <linux/soc/cix/util.h>
#include <linux/uaccess.h> /* For copy_to_user */
#include <linux/delay.h>
#include <linux/of.h>
#include <mntn_public_interface.h>
#include <linux/zlib.h>
#include <linux/crc32.h>
#include "dst_print.h"

#define PATH_MAXLEN 128
#define ROOT_UID 0
#define SYSTEM_GID 1000
#define DIR_LIMIT (S_IRWXU | S_IRWXG)
#define FILE_LIMIT (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP)

static char himntn[HIMNTN_VALID_SIZE + 1] = "11111000000000000000000";
static struct user_namespace *util_user_ns = &init_user_ns;

static char g_rdr_flag[] = "cix dst";
static struct user_namespace* rdr_user_ns = &init_user_ns;

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
		DST_PRINT_DBG("DFD NV: value=0x%x\n", nv_value);

		for (index = 0; index < HIMNTN_VALID_SIZE; index++) {
			himntn[index] = (nv_value & (1 << index)) ? '1' : '0';
		}
	}

	DST_PRINT_PN("DFD NV: platmntn=%s\n", himntn);
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
			notify_change(util_user_ns, dentry, &newattrs, NULL);
			inode_unlock(d_inode(dentry));
			err = vfs_unlink(util_user_ns, d_inode(parent.dentry),
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
		pr_err("Failed to get file path(%s)\n", file_path);
		return -1;
	}

	newattrs.ia_uid = uid;
	newattrs.ia_gid = gid;
	newattrs.ia_valid = ATTR_UID | ATTR_GID;
	inode_lock(d_inode(path.dentry));
	if (notify_change(util_user_ns, path.dentry, &newattrs, NULL)) {
		pr_err("[%s,%d] change owner failed \
			path:%s \n",
		       __func__, __LINE__, file_path);
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
		pr_err("[%s,%d] path:%s stat failed \n", __func__, __LINE__,
		       fullname);
		return false;
	}

	if (S_ISDIR(stat.mode)) {
		pr_debug("[%s,%d] path:%s, name=%s \n", __func__, __LINE__,
			 cb->path, name);
		if (strncmp(name, ".", 1) && strncmp(name, "..", 2)) {
			rdr_chown(fullname, cb->user, cb->group, cb->recursion);
		}
		return true;
	} else if (S_ISREG(stat.mode)) {
		pr_debug("[%s,%d] file:%s \n", __func__, __LINE__, fullname);
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
		pr_err("rdr:path is null\n");
		return 0;
	}

	rdr_path_chown(folder_path, user, group);

	if (!recursion)
		goto not_recursion;

	ret = kern_path(folder_path, 0, &kpath);
	if (ret) {
		printk(KERN_ERR "Failed to get path:%s %d\n", folder_path, ret);
		return 0;
	}

	// open the directory
	file = dentry_open(&kpath, O_RDONLY, current_cred());
	if (IS_ERR(file)) {
		pr_err("rdr:%s(),open %s fail\n", __func__, folder_path);
		return 0;
	}

	cb.path = folder_path;
	// iterate over the directory entries
	ret = iterate_dir(file, &cb.ctx);

	// clean up
	fput(file);
	path_put(&kpath);

	if (ret) {
		pr_err("rdr failed to iterate directory: %d\n", ret);
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

	path_put(&p);

	return ret;
}

static int create_dir(const char *name, umode_t mode)
{
	struct dentry *dentry;
	struct path path;
	int err;

	dentry = kern_path_create(AT_FDCWD, name, &path, LOOKUP_DIRECTORY);
	if (IS_ERR(dentry))
		return PTR_ERR(dentry);

	err = vfs_mkdir(rdr_user_ns, d_inode(path.dentry), dentry, mode);
	if (!err)
		/* mark as kernel-created inode */
		d_inode(dentry)->i_private = g_rdr_flag;
	done_path_create(&path, dentry);
	return err;
}

static int __rdr_create_dir(const char *path)
{
	struct kstat m_stat;
	int err = 0;

	if (path == NULL) {
		DST_PRINT_ERR("invalid parameter path\n");
		DST_PRINT_END();
		return -1;
	}
	if (rdr_vfs_stat(path, &m_stat) != 0) {
		DST_PRINT_PN("rdr: need create dir %s\n", path);
		err = create_dir(path, DIR_LIMIT);
		if (!err)
			DST_PRINT_PN("rdr: create dir %s successed!!!\n", path);
	}

	return err;
}

int rdr_create_dir(const char *path)
{
	char cur_path[DST_TMP_PATH_MAX_LEN];
	int index = 0;

	DST_PRINT_START();
	if (path == NULL) {
		DST_PRINT_ERR("invalid  parameter path\n");
		DST_PRINT_END();
		return -1;
	}
	memset(cur_path, 0, DST_TMP_PATH_MAX_LEN);

	if (*path != '/')
		return -1;
	cur_path[index++] = *path++;

	while (*path != '\0') {
		if (*path == '/') {
			if (__rdr_create_dir(cur_path) < 0)
				DST_PRINT_ERR("rdr: create dir failed\n");
		}
		cur_path[index] = *path;
		path++;
		index++;
	}

	DST_PRINT_END();
	return 0;
}

int rdr_rm_dir(const char *name)
{
	struct path parent;
	struct dentry *dentry;
	struct dentry *child;
	int err;

	DST_PRINT_DBG("rm dir:%s \n", name);

	dentry = kern_path_locked(name, &parent);
	if (IS_ERR(dentry))
		return PTR_ERR(dentry);

	if (d_is_dir(dentry)) {
		list_for_each_entry(child, &dentry->d_subdirs, d_child) {
			d_inode(child)->i_private = NULL;
			DST_PRINT_DBG("want to delete file: %s \n", child->d_name.name);
			err = vfs_unlink(rdr_user_ns, d_inode(dentry), child, NULL);
			if (err) {
				DST_PRINT_ERR("rm file failed: %s \n", child->d_name.name);
				goto exit_rm;
			}
		}

		err = vfs_rmdir(rdr_user_ns, d_inode(parent.dentry), dentry);
		if (err) {
			DST_PRINT_ERR("rm file failed \n");
		}
	} else {
		err = -ENOENT;
	}

exit_rm:
	dput(dentry);
	inode_unlock(d_inode(parent.dentry));
	path_put(&parent);
	return err;
}

unsigned long dst_get_phy_addr(unsigned long addr)
{
	struct mm_struct *mm;
	pgd_t *pgdp;
	pgd_t pgd;
	unsigned long val = 0;

	if (is_ttbr0_addr(addr)) {
		/* TTBR0 */
		mm = current->active_mm;
		if (mm == &init_mm) {
			pr_debug(
				"[%016lx] user address but active_mm is swapper\n",
				addr);
			return 0;
		}
	} else if (is_ttbr1_addr(addr)) {
		/* TTBR1 */
		mm = &init_mm;
	} else {
		pr_debug(
			"[%016lx] address between user and kernel address ranges\n",
			addr);
		return 0;
	}

	pgdp = pgd_offset(mm, addr);
	pgd = READ_ONCE(*pgdp);
	pr_debug("[%016lx] pgd=%016llx", addr, pgd_val(pgd));

	do {
		p4d_t *p4dp, p4d;
		pud_t *pudp, pud;
		pmd_t *pmdp, pmd;
		pte_t *ptep, pte;

		if (pgd_none(pgd) || pgd_bad(pgd))
			break;

		p4dp = p4d_offset(pgdp, addr);
		p4d = READ_ONCE(*p4dp);
		pr_debug(", p4d=%016llx", p4d_val(p4d));
		if (p4d_none(p4d) || p4d_bad(p4d))
			break;

		pudp = pud_offset(p4dp, addr);
		pud = READ_ONCE(*pudp);
		pr_debug(", pud=%016llx", pud_val(pud));
		if (pud_none(pud) || pud_bad(pud))
			break;

		pmdp = pmd_offset(pudp, addr);
		pmd = READ_ONCE(*pmdp);
		pr_debug(", pmd=%016llx", pmd_val(pmd));
		if (pmd_none(pmd) || pmd_bad(pmd))
			break;

		ptep = pte_offset_map(pmdp, addr);
		pte = READ_ONCE(*ptep);
		pr_debug(", pte=%016llx", pte_val(pte));
		val = pte_val(pte);
		pte_unmap(ptep);
	} while (0);

	pr_debug("\n");
	val &= 0x0000FFFFFFFFFFFF;
	val = (val & (0xFFFFFFFFFFFFFFFF << PAGE_SHIFT)) +
	      (addr & ((1 << PAGE_SHIFT) - 1));
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
		pr_err("invalid  parameter. path:%pK, name:%pK buf:%pK len:0x%x\n",
		       logpath, filename, buf, len);
		ret = -1;
		goto end;
	}

	(void)snprintf(path, PATH_MAXLEN, "%s/%s", logpath, filename);

	flags = O_CREAT | O_RDWR | (is_append ? O_APPEND : O_TRUNC);
	fp = filp_open(path, flags, FILE_LIMIT);
	if (IS_ERR(fp)) {
		pr_err("%s():create file %s err. fp=0x%pK\n", __func__, path,
		       fp);
		ret = -1;
		goto end;
	}
	vfs_llseek(fp, 0L, SEEK_END);
	ret = kernel_write(fp, buf, len, &(fp->f_pos));
	if (ret != len) {
		pr_err("%s():write file %s exception with ret %d\n", __func__,
		       path, ret);
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
		pr_err("[%s], chown %s uid [%d] gid [%d] failed err [%d]\n",
		       __func__, path, ROOT_UID, SYSTEM_GID, ret);
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
			if (crc & 1) {
				crc = (crc >> 1) ^ CRC32_POLYNOMIAL;
			} else {
				crc >>= 1;
			}
		}
		crc32_table[i] = crc;
	}
}

static u32 calculate_crc32(const unsigned char *data, size_t length)
{
	static u32 crc32_table[256];
	static int table_generated = 0;
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
	if (!*dst) {
		return -ENOMEM;
	}

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
		pr_err("invalid  parameter. path:%pK, name:%pK buf:%pK len:0x%x\n",
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