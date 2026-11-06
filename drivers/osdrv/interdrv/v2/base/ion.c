#include <linux/dma-buf.h>
#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/mod_devicetable.h>
#include <linux/uaccess.h>
#include <linux/version.h>
#include <linux/slab.h>
#include <linux/hashtable.h>
#include <asm/cacheflush.h>
#if (KERNEL_VERSION(5, 10, 0) <= LINUX_VERSION_CODE)
#include <linux/dma-map-ops.h>
#endif

#include <linux/defines.h>
#include <linux/base_uapi.h>
#include "ion.h"
#include "ion/ion.h"
#include "ion/cvitek/cvitek_ion_alloc.h"
#include <linux/dma-mapping.h>
#include "base_debug.h"

struct mem_mapping {
	uint64_t phy_addr;
	int32_t dmabuf_fd;
	int32_t size;
	void *vir_addr;
	void *dmabuf;
	pid_t fd_pid;
	struct hlist_node node;
};

static int ion_debug_alloc_free;
module_param(ion_debug_alloc_free, int, 0644);

static DEFINE_SPINLOCK(ion_lock);
static DEFINE_HASHTABLE(ion_hash, 8);

static int32_t mem_put(struct mem_mapping *mem_info)
{
	struct mem_mapping *p;

	p = kmalloc(sizeof(*p), GFP_KERNEL);
	if (!p) {
		TRACE_BASE(DBG_ERR, "kmalloc failed\n");
		return -1;
	}
	memcpy(p, mem_info, sizeof(*p));

	spin_lock(&ion_lock);
	hash_add(ion_hash, &p->node, p->phy_addr);
	spin_unlock(&ion_lock);

	return 0;
}

static int32_t mem_get(struct mem_mapping *mem_info)
{
	int32_t ret = -1;
	struct mem_mapping *obj;
	struct hlist_node *tmp;
	uint64_t key = mem_info->phy_addr;

	spin_lock(&ion_lock);
	hash_for_each_possible_safe(ion_hash, obj, tmp, node, key) {
		if (obj->phy_addr == key) {
			memcpy(mem_info, obj, sizeof(*mem_info));
			hash_del(&obj->node);
			kfree(obj);
			ret = 0;
			break;
		}
	}
	spin_unlock(&ion_lock);

	return ret;
}

static int32_t mem_dump(void)
{
	int32_t cnt = 0, bkt;
	struct mem_mapping *obj;

	spin_lock(&ion_lock);
	hash_for_each(ion_hash, bkt, obj, node) {
		TRACE_BASE(DBG_INFO, "ion addr=0x%llx, dmabuf_fd=%d\n",
			obj->phy_addr, obj->dmabuf_fd);
		cnt++;
	}
	spin_unlock(&ion_lock);

	TRACE_BASE(DBG_INFO, "ion block total=%d\n", cnt);
	return cnt;
}

static int32_t _base_ion_alloc(uint64_t *addr_p, void **addr_v, uint32_t len,
	uint32_t is_cached, uint8_t *name)
{
	int32_t dmabuf_fd = 0, ret = 0;
	struct dma_buf *dmabuf;
	struct ion_buffer *ionbuf;
	uint8_t *owner_name = NULL;
	void *vmap_addr = NULL;
	struct mem_mapping mem_info;

	//vpp heap
	dmabuf_fd = bm_ion_alloc(0x1, len, is_cached);
	if (dmabuf_fd < 0) {
		TRACE_BASE(DBG_ERR, "bm_ion_alloc len=0x%x failed\n", len);
		return -ENOMEM;
	}

	dmabuf = dma_buf_get(dmabuf_fd);
	if (!dmabuf) {
		TRACE_BASE(DBG_ERR, "allocated get dmabuf failed\n");
		return -ENOMEM;
	}

	ionbuf = (struct ion_buffer *)dmabuf->priv;
	owner_name = vmalloc(MAX_ION_BUFFER_NAME);
	if (name)
		strncpy(owner_name, name, MAX_ION_BUFFER_NAME);
	else
		strncpy(owner_name, "anonymous", MAX_ION_BUFFER_NAME);

	ionbuf->name = owner_name;

	ret = dma_buf_begin_cpu_access(dmabuf, DMA_TO_DEVICE);
	if (ret < 0) {
		TRACE_BASE(DBG_ERR, "dma_buf_begin_cpu_access failed\n");
		dma_buf_put(dmabuf);
		return ret;
	}

	vmap_addr = ionbuf->vaddr;
	if (IS_ERR(vmap_addr)) {
		ret = -EINVAL;
		return ret;
	}

	//push into memory manager
	mem_info.dmabuf = (void *)dmabuf;
	mem_info.dmabuf_fd = dmabuf_fd;
	mem_info.vir_addr = vmap_addr;
	mem_info.phy_addr = ionbuf->paddr;
	mem_info.size = len;
	mem_info.fd_pid = current->pid;
	if (mem_put(&mem_info)) {
		TRACE_BASE(DBG_ERR, "allocate mm put failed\n");
		return -ENOMEM;
	}

	if (ion_debug_alloc_free) {
		TRACE_BASE(DBG_INFO, "ion alloc: pid=%d name=%s phy_addr=0x%llx size=%d\n",
			mem_info.fd_pid, ionbuf->name, mem_info.phy_addr, mem_info.size);
	}

	*addr_p = ionbuf->paddr;
	*addr_v = vmap_addr;

	return ret;
}

static int32_t _base_ion_free(uint64_t addr_p, int32_t *size)
{
	struct mem_mapping mem_info;
	struct ion_buffer *ionbuf;
	struct dma_buf *dmabuf;

	//get from memory manager
	memset(&mem_info, 0, sizeof(struct mem_mapping));
	mem_info.phy_addr = addr_p;
	if (mem_get(&mem_info)) {
		TRACE_BASE(DBG_ERR, "dmabuf_fd get failed, addr:0x%llx\n", addr_p);
		return -ENOMEM;
	}

	dmabuf = (struct dma_buf *)(mem_info.dmabuf);
	ionbuf = (struct ion_buffer *)dmabuf->priv;

	if (ion_debug_alloc_free) {
		TRACE_BASE(DBG_INFO, "ion free: pid=%d name=%s phy_addr=0x%llx size=%d\n",
			mem_info.fd_pid, ionbuf->name, mem_info.phy_addr, mem_info.size);
	}

	dma_buf_end_cpu_access(dmabuf, DMA_TO_DEVICE);
	dma_buf_put(dmabuf);
	bm_ion_free(mem_info.dmabuf_fd);

	if (size)
		*size = mem_info.size;

	return 0;
}

int32_t base_ion_free2(uint64_t phy_addr, int32_t *size)
{
	return _base_ion_free(phy_addr, size);
}

int32_t base_ion_free(uint64_t phy_addr)
{
	return _base_ion_free(phy_addr, NULL);
}
EXPORT_SYMBOL_GPL(base_ion_free);

int32_t base_ion_alloc(uint64_t *p_paddr, void **pp_vaddr, uint8_t *buf_name, uint32_t buf_len, bool is_cached)
{
	return _base_ion_alloc(p_paddr, pp_vaddr, buf_len, is_cached, buf_name);
}
EXPORT_SYMBOL_GPL(base_ion_alloc);

int32_t base_ion_cache_invalidate(uint64_t addr_p, void *addr_v, uint32_t len)
{
	arch_sync_dma_for_device(addr_p, len, DMA_FROM_DEVICE);

	/*	*/
	smp_mb();
	return 0;
}
EXPORT_SYMBOL_GPL(base_ion_cache_invalidate);

int32_t base_ion_cache_flush(uint64_t addr_p, void *addr_v, uint32_t len)
{
	arch_sync_dma_for_device(addr_p, len, DMA_TO_DEVICE);

	/*	*/
	smp_mb();
	return 0;
}
EXPORT_SYMBOL_GPL(base_ion_cache_flush);

int32_t base_ion_dump(void)
{
	return mem_dump();
}
EXPORT_SYMBOL_GPL(base_ion_dump);
