// SPDX-License-Identifier: GPL-2.0-only
/*
 * Stack tracing support
 *
 * Copyright (C) 2025 CIX Ltd.
 */
#include <linux/syscalls.h>
#include <linux/mount.h>
#include <linux/namei.h>
#include "include/rdr_ap_adapter.h"
#include "include/rdr_ap_logbuf.h"
#include "../rdr_print.h"

#define LOGMEM_PROP_INIT(name) PROPERTY_INIT(ap_log_##name##_size)
#define IS_PSTORE BIT(0)

static struct bbox_mem g_logmem[PSTORE_TYPE_MAX];
static struct vfsmount *pstore_mnt;
static bool g_pstore_mounted;
static struct property_table g_logmem_prop[PSTORE_TYPE_MAX] = {
	[PSTORE_TYPE_DMESG] = LOGMEM_PROP_INIT(dmesg),
	[PSTORE_TYPE_CONSOLE] = LOGMEM_PROP_INIT(console)
};

void pstore_dump_mount(void)
{
	struct file_system_type *fs_type;

	g_pstore_mounted = true;
	fs_type = get_fs_type("pstore");
	if (IS_ERR_OR_NULL(fs_type)) {
		BB_ERR("pstore is not exist!\n");
		return;
	}

	pstore_mnt = kern_mount(fs_type);
	if (IS_ERR_OR_NULL(pstore_mnt))
		BB_ERR("pstore mount fail!\n");
	else
		BB_PN("pstore mount success!\n");
	put_filesystem(fs_type);
}

int pstore_dump_init(struct platform_device *pdev,
		     struct rdr_safemem_pool *pool)
{
	int ret = 0;

	ret = ap_prop_table_init(&pdev->dev, g_logmem_prop,
				 ARRAY_SIZE(g_logmem_prop));
	if (ret) {
		BB_ERR("g_logmem_prop init failed!\n");
		return ret;
	}

	for (int i = 0; i < PSTORE_TYPE_MAX; i++) {
		if (g_logmem_prop[i].size == 0)
			continue;
		if (rdr_safemem_alloc(pool, MEMID_PSTORE + i,
				      g_logmem_prop[i].size, &g_logmem[i])) {
			BB_ERR("there is no enough space for modu [%d] to dump mem!\n",
			       i);
			break;
		}
		BB_DBG("logmem_addr [0x%px] logmem_size [0x%llx]!\n",
		       g_logmem[i].vaddr, g_logmem[i].size);
	}
	return 0;
}

void logmem_add(enum pstore_type_id id, void *buf, u32 size)
{
	u32 offset = sizeof(struct pstore_head);
	u32 cp_size = 0;
	void *cp_addr;
	struct pstore_head *lhead = NULL;

	if (IS_ERR_OR_NULL(g_logmem))
		return;

	cp_size = min(size, g_logmem[id].size - offset);
	lhead = g_logmem[id].vaddr;

	if (lhead->flag == IS_PSTORE)
		return;
	lhead->flag = IS_PSTORE;
	lhead->type = id;
	lhead->size = cp_size;
	cp_addr = buf + size - cp_size;
	if (g_logmem[id].size) {
		BB_PN("add %s log ok, orign size: %d, cp_size: %d\n",
		      pstore_type_to_name(id), size, cp_size);
		memcpy(g_logmem[id].vaddr + offset, cp_addr, cp_size);
	}
}
EXPORT_SYMBOL(logmem_add);

/*Cleartext will only occur after pstore is mounted.*/
int ap_pstore_cleartext(const char *dir_path, u64 log_addr, u32 log_len)
{
	struct file *fp;
	struct pstore_head *lhead;
	ssize_t ret = 0;

	if (!g_pstore_mounted)
		return -EPERM;

	for (int i = 0; i < PSTORE_TYPE_MAX; i++) {
		if (g_logmem[i].size == 0)
			continue;
		lhead = g_logmem[i].vaddr;
		if (lhead->flag != IS_PSTORE)
			continue;

		fp = bbox_cleartext_get_filep(
			dir_path, (char *)pstore_type_to_name(lhead->type));
		if (IS_ERR_OR_NULL(fp))
			continue;

		ret = kernel_write(fp, g_logmem[i].vaddr + sizeof(*lhead),
				   lhead->size, &(fp->f_pos));
		if (ret != lhead->size)
			BB_PN("%s write %ld bytes is not equal %u bytes\n",
			      (char *)pstore_type_to_name(lhead->type), ret,
			      lhead->size);
		bbox_cleartext_end_filep(fp);
		/*clear log buf*/
		memset(g_logmem[i].vaddr, 0, g_logmem[i].size);
	}

	return 0;
}
