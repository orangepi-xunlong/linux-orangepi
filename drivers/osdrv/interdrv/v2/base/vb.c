#include <linux/module.h>
#include <linux/spinlock.h>
#include <linux/hashtable.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/platform_device.h>

#include "vb.h"
#include "ion.h"
#include "queue.h"
#include "base_common.h"
#include "base_debug.h"

uint32_t vb_max_pools = 512;
module_param(vb_max_pools, uint, 0644);
uint32_t vb_pool_max_blk = 128;
module_param(vb_pool_max_blk, uint, 0644);

static struct vb_cfg g_vb_config;
static struct vb_pool_ctx *g_vb_ctx;
static atomic_t ref_count = ATOMIC_INIT(0);

static DEFINE_MUTEX(g_lock);
static DEFINE_MUTEX(g_get_vb_lock);
static DEFINE_MUTEX(g_pool_lock);
static DEFINE_SPINLOCK(g_hash_lock);

DEFINE_HASHTABLE(vb_hash, 8);

#define CHECK_VB_HANDLE_NULL(x)							\
	do {									\
		if ((x) == NULL) {						\
			TRACE_BASE(DBG_ERR, " NULL VB HANDLE\n");		\
			return -EINVAL;				\
		}								\
	} while (0)

#define CHECK_VB_HANDLE_VALID(x)						\
	do {									\
		if ((x)->magic != VB_MAGIC) {	\
			TRACE_BASE(DBG_ERR, " invalid VB Handle\n");	\
			return -EINVAL;				\
		}								\
	} while (0)

#define CHECK_VB_POOL_VALID_WEAK(x)							\
	do {									\
		if ((x) == VB_STATIC_POOLID)					\
			break;							\
		if ((x) == VB_EXTERNAL_POOLID)					\
			break;							\
		if ((x) >= (vb_max_pools)) {					\
			TRACE_BASE(DBG_ERR, " invalid VB Pool(%d)\n", x);	\
			return -EINVAL;			\
		}								\
		if (!is_pool_inited(x)) {						\
			TRACE_BASE(DBG_ERR, "vb_pool(%d) isn't init yet.\n", x); \
			return -EINVAL;			\
		}								\
	} while (0)

#define CHECK_VB_POOL_VALID_STRONG(x)							\
	do {									\
		if ((x) >= (vb_max_pools)) {					\
			TRACE_BASE(DBG_ERR, " invalid VB Pool(%d)\n", x);	\
			return -EINVAL; 		\
		}								\
		if (!is_pool_inited(x)) { 					\
			TRACE_BASE(DBG_ERR, "vb_pool(%d) isn't init yet.\n", x); \
			return -EINVAL; 		\
		}								\
	} while (0)

static inline bool is_pool_inited(vb_pool poolid)
{
	return (g_vb_ctx[poolid].membase == 0) ? false : true;
}


static bool _vb_hash_del(uint64_t phy_addr)
{
	bool is_found = false;
	struct vb_s *obj;
	struct hlist_node *tmp;

	spin_lock(&g_hash_lock);
	hash_for_each_possible_safe(vb_hash, obj, tmp, node, phy_addr) {
		if (obj->phy_addr == phy_addr) {
			hash_del(&obj->node);
			is_found = true;
			break;
		}
	}
	spin_unlock(&g_hash_lock);

	return is_found;
}

static bool _vb_hash_find(uint64_t phy_addr, struct vb_s **vb)
{
	bool is_found = false;
	struct vb_s *obj;

	spin_lock(&g_hash_lock);
	hash_for_each_possible(vb_hash, obj, node, phy_addr) {
		if (obj->phy_addr == phy_addr) {
			is_found = true;
			break;
		}
	}
	spin_unlock(&g_hash_lock);

	if (is_found)
		*vb = obj;
	return is_found;
}

static bool _is_comm_vb_released(void)
{
	uint32_t i;

	for (i = 0; i < VB_MAX_COMM_POOLS; ++i) {
		if (is_pool_inited(i)) {
			if (FIFO_CAPACITY(&g_vb_ctx[i].freelist) != FIFO_SIZE(&g_vb_ctx[i].freelist)) {
				TRACE_BASE(DBG_INFO, "pool(%d) blk has not been all released yet\n", i);
				return false;
			}
		}
	}
	return true;
}

int32_t vb_print_pool(vb_pool poolid)
{
	struct vb_s *vb;
	int bkt, i;
	char str[64];

	CHECK_VB_POOL_VALID_STRONG(poolid);

	spin_lock(&g_hash_lock);
	hash_for_each(vb_hash, bkt, vb, node) {
		if (vb->poolid == poolid) {
			sprintf(str, "Pool[%d] vb paddr(%#llx) usr_cnt(%d) /",
				vb->poolid, vb->phy_addr, vb->usr_cnt.counter);

			for (i = 0; i < ID_BUTT; ++i) {
				if (atomic_long_read(&vb->mod_ids) & BIT(i)) {
					strncat(str, sys_get_modname(i), sizeof(str));
					strcat(str, "/");
				}
			}
			TRACE_BASE(DBG_INFO, "%s\n", str);
		}
	}
	spin_unlock(&g_hash_lock);

	return 0;
}
EXPORT_SYMBOL_GPL(vb_print_pool);

static int32_t _vb_set_config(struct vb_cfg *vb_cfg)
{
	int i;

	if (vb_cfg->comm_pool_cnt > VB_COMM_POOL_MAX_CNT
		|| vb_cfg->comm_pool_cnt == 0) {
		TRACE_BASE(DBG_ERR, "Invalid comm_pool_cnt(%d)\n", vb_cfg->comm_pool_cnt);
		return -EINVAL;
	}

	for (i = 0; i < vb_cfg->comm_pool_cnt; ++i) {
		if (vb_cfg->comm_pool[i].blk_size == 0
			|| vb_cfg->comm_pool[i].blk_cnt == 0
			|| vb_cfg->comm_pool[i].blk_cnt > vb_pool_max_blk) {
			TRACE_BASE(DBG_ERR, "Invalid pool cfg, pool(%d), blk_size(%d), blk_cnt(%d)\n",
				i, vb_cfg->comm_pool[i].blk_size,
				vb_cfg->comm_pool[i].blk_cnt);
			return -EINVAL;
		}
	}
	g_vb_config = *vb_cfg;

	return 0;
}

static void _vb_cleanup(void)
{
	struct vb_s *vb;
	int bkt, i;
	struct hlist_node *tmp;
	struct vb_pool_ctx *pool_ctx;
	struct vb_req *req, *req_tmp;


	// free vb pool
	for (i = 0; i < VB_MAX_COMM_POOLS; ++i) {
		if (is_pool_inited(i)) {
			pool_ctx = &g_vb_ctx[i];
			mutex_lock(&pool_ctx->lock);
			FIFO_EXIT(&pool_ctx->freelist);
			base_ion_free(pool_ctx->membase);
			mutex_unlock(&pool_ctx->lock);
			mutex_destroy(&pool_ctx->lock);
			// free reqq
			mutex_lock(&pool_ctx->reqq_lock);
			if (!STAILQ_EMPTY(&pool_ctx->reqq)) {
				STAILQ_FOREACH_SAFE(req, &pool_ctx->reqq, stailq, req_tmp) {
					STAILQ_REMOVE(&pool_ctx->reqq, req, vb_req, stailq);
					kfree(req);
				}
			}
			mutex_unlock(&pool_ctx->reqq_lock);
			mutex_destroy(&pool_ctx->reqq_lock);
			memset(pool_ctx, 0, sizeof(struct vb_pool_ctx));
		}
	}

	// free comm vb blk
	spin_lock(&g_hash_lock);
	hash_for_each_safe(vb_hash, bkt, tmp, vb, node) {
		if ((vb->poolid >= VB_MAX_COMM_POOLS) && (vb->poolid < vb_max_pools))
			continue;
		if (vb->poolid == VB_STATIC_POOLID)
			base_ion_free(vb->phy_addr);
		hash_del(&vb->node);
	}
	spin_unlock(&g_hash_lock);

}

static int32_t _vb_create_pool(struct vb_pool_cfg *config, bool is_comm)
{
	uint32_t pool_size;
	struct vb_s *p;
	bool is_cache;
	char ion_name[10];
	int32_t ret, i;
	vb_pool pool_id = config->pool_id;
	void *ion_v = NULL;
	struct vb_pool_ctx *pool_ctx;

	pool_ctx = &g_vb_ctx[pool_id];
	pool_size = config->blk_size * config->blk_cnt;
	is_cache = (config->remap_mode == VB_REMAP_MODE_CACHED);

	snprintf(ion_name, 10, "VbPool%d", pool_id);
	ret = base_ion_alloc(&pool_ctx->membase, &ion_v, (uint8_t *)ion_name, pool_size, is_cache);
	if (ret) {
		TRACE_BASE(DBG_ERR, "base_ion_alloc fail! ret(%d)\n", ret);
		return ret;
	}
	config->mem_base = pool_ctx->membase;

	STAILQ_INIT(&pool_ctx->reqq);
	mutex_init(&pool_ctx->reqq_lock);
	mutex_init(&pool_ctx->lock);
	mutex_lock(&pool_ctx->lock);
	pool_ctx->poolid = pool_id;
	pool_ctx->ownerid = (is_comm) ? POOL_OWNER_COMMON : POOL_OWNER_PRIVATE;
	pool_ctx->vmembase = ion_v;
	pool_ctx->blk_cnt = config->blk_cnt;
	pool_ctx->blk_size = config->blk_size;
	pool_ctx->remap_mode = config->remap_mode;
	pool_ctx->is_comm_pool = is_comm;
	pool_ctx->free_blk_cnt = config->blk_cnt;
	pool_ctx->min_free_blk_cnt = pool_ctx->free_blk_cnt;
	if (strlen(config->pool_name) != 0)
		strncpy(pool_ctx->pool_name, config->pool_name,
			sizeof(pool_ctx->pool_name));
	else
		strncpy(pool_ctx->pool_name, "vbpool", sizeof(pool_ctx->pool_name));
	pool_ctx->pool_name[VB_POOL_NAME_LEN - 1] = '\0';

	FIFO_INIT(&pool_ctx->freelist, pool_ctx->blk_cnt);
	for (i = 0; i < pool_ctx->blk_cnt; ++i) {
		p = vzalloc(sizeof(*p));
		p->phy_addr = pool_ctx->membase + (i * pool_ctx->blk_size);
		p->vir_addr = pool_ctx->vmembase + (p->phy_addr - pool_ctx->membase);
		p->poolid = pool_id;
		atomic_set(&p->usr_cnt, 0);
		p->magic = VB_MAGIC;
		atomic_long_set(&p->mod_ids, 0);
		p->external = false;
		FIFO_PUSH(&pool_ctx->freelist, p);
		spin_lock(&g_hash_lock);
		hash_add(vb_hash, &p->node, p->phy_addr);
		spin_unlock(&g_hash_lock);
	}
	mutex_unlock(&pool_ctx->lock);

	return 0;
}

static int32_t _vb_destroy_pool(vb_pool poolid)
{
	struct vb_pool_ctx *pool_ctx = &g_vb_ctx[poolid];
	struct vb_s *vb;
	struct vb_req *req, *req_tmp;

	TRACE_BASE(DBG_INFO, "vb destroy pool, pool[%d]: capacity(%d) size(%d).\n"
		, poolid, FIFO_CAPACITY(&pool_ctx->freelist), FIFO_SIZE(&pool_ctx->freelist));
	if (FIFO_CAPACITY(&pool_ctx->freelist) != FIFO_SIZE(&pool_ctx->freelist)) {
		TRACE_BASE(DBG_INFO, "pool(%d) blk should be all released before destroy pool\n", poolid);
		vb_print_pool(pool_ctx->poolid);
		return -1;
	}

	mutex_lock(&pool_ctx->lock);
	while (!FIFO_EMPTY(&pool_ctx->freelist)) {
		FIFO_POP(&pool_ctx->freelist, &vb);
		_vb_hash_del(vb->phy_addr);
		vfree(vb);
	}
	FIFO_EXIT(&pool_ctx->freelist);
	base_ion_free(pool_ctx->membase);
	mutex_unlock(&pool_ctx->lock);
	mutex_destroy(&pool_ctx->lock);

	// free reqq
	mutex_lock(&pool_ctx->reqq_lock);
	if (!STAILQ_EMPTY(&pool_ctx->reqq)) {
		STAILQ_FOREACH_SAFE(req, &pool_ctx->reqq, stailq, req_tmp) {
			STAILQ_REMOVE(&pool_ctx->reqq, req, vb_req, stailq);
			kfree(req);
		}
	}
	mutex_unlock(&pool_ctx->reqq_lock);
	mutex_destroy(&pool_ctx->reqq_lock);

	memset(pool_ctx, 0, sizeof(struct vb_pool_ctx));
	return 0;
}

static int32_t _vb_init(void)
{
	uint32_t i;
	int32_t ret;

	mutex_lock(&g_lock);
	if (atomic_read(&ref_count) == 0) {
		for (i = 0; i < g_vb_config.comm_pool_cnt; ++i) {
			g_vb_config.comm_pool[i].pool_id = i;
			ret = _vb_create_pool(&g_vb_config.comm_pool[i], true);
			if (ret) {
				TRACE_BASE(DBG_ERR, "_vb_create_pool fail, ret(%d)\n", ret);
				goto VB_INIT_FAIL;
			}
		}

		TRACE_BASE(DBG_INFO, "_vb_init -\n");
	}
	atomic_add(1, &ref_count);
	mutex_unlock(&g_lock);
	return 0;

VB_INIT_FAIL:
	for (i = 0; i < g_vb_config.comm_pool_cnt; ++i) {
		if (is_pool_inited(i))
			_vb_destroy_pool(i);
	}

	mutex_unlock(&g_lock);
	return ret;
}

static int32_t _vb_exit(void)
{
	int i;

	mutex_lock(&g_lock);
	if (atomic_read(&ref_count) == 0) {
		TRACE_BASE(DBG_INFO, "vb has already exited\n");
		mutex_unlock(&g_lock);
		return 0;
	}
	if (atomic_sub_return(1, &ref_count) > 0) {
		mutex_unlock(&g_lock);
		return 0;
	}

	if (!_is_comm_vb_released()) {
		TRACE_BASE(DBG_INFO, "vb has not been all released\n");
		for (i = 0; i < VB_MAX_COMM_POOLS; ++i) {
			if (is_pool_inited(i))
				vb_print_pool(i);
		}
	}
	_vb_cleanup();
	mutex_unlock(&g_lock);
	TRACE_BASE(DBG_INFO, "_vb_exit -\n");
	return 0;
}

vb_pool find_vb_pool(uint32_t blk_size)
{
	vb_pool poolid = VB_INVALID_POOLID;
	int i;

	for (i = 0; i < VB_COMM_POOL_MAX_CNT; ++i) {
		if (!is_pool_inited(i))
			continue;
		if (g_vb_ctx[i].ownerid != POOL_OWNER_COMMON)
			continue;
		if (g_vb_ctx[i].free_blk_cnt == 0)
			continue;
		if (blk_size > g_vb_ctx[i].blk_size)
			continue;
		if ((poolid == VB_INVALID_POOLID)
			|| (g_vb_ctx[poolid].blk_size > g_vb_ctx[i].blk_size))
			poolid = i;
	}
	return poolid;
}
EXPORT_SYMBOL_GPL(find_vb_pool);

static vb_blk _vb_get_block_static(uint32_t blk_size)
{
	int32_t ret = 0;
	uint64_t phy_addr = 0;
	void *ion_v = NULL;

	//allocate with ion
	ret = base_ion_alloc(&phy_addr, &ion_v, "static_pool", blk_size, true);
	if (ret) {
		TRACE_BASE(DBG_ERR, "base_ion_alloc fail! ret(%d)\n", ret);
		return VB_INVALID_HANDLE;
	}

	return vb_create_block(phy_addr, ion_v, VB_STATIC_POOLID, false);
}

/* _vb_get_block: acquice a vb_blk with specific size from pool.
 *
 * @param pool: the pool to acquice blk.
 * @param blk_size: the size of vb_blk to acquire.
 * @param modId: the Id of mod which acquire this blk
 * @return: the vb_blk if available. otherwise, VB_INVALID_HANDLE.
 */
static vb_blk _vb_get_block(struct vb_pool_ctx *pool_ctx, u32 blk_size, mod_id_e mod_id)
{
	struct vb_s *p;

	if (blk_size > pool_ctx->blk_size) {
		TRACE_BASE(DBG_ERR, "PoolID(%#x) blksize(%d) > pool's(%d).\n"
			, pool_ctx->poolid, blk_size, pool_ctx->blk_size);
		return VB_INVALID_HANDLE;
	}

	mutex_lock(&pool_ctx->lock);
	if (FIFO_EMPTY(&pool_ctx->freelist)) {
		TRACE_BASE(DBG_INFO, "vb_pool owner(%#x) poolid(%#x) pool is empty.\n",
			pool_ctx->ownerid, pool_ctx->poolid);
		mutex_unlock(&pool_ctx->lock);
		vb_print_pool(pool_ctx->poolid);
		return VB_INVALID_HANDLE;
	}

	FIFO_POP(&pool_ctx->freelist, &p);
	pool_ctx->free_blk_cnt--;
	pool_ctx->min_free_blk_cnt =
		(pool_ctx->free_blk_cnt < pool_ctx->min_free_blk_cnt) ?
		pool_ctx->free_blk_cnt : pool_ctx->min_free_blk_cnt;
	atomic_set(&p->usr_cnt, 1);
	atomic_long_set(&p->mod_ids, BIT(mod_id));
	mutex_unlock(&pool_ctx->lock);
	TRACE_BASE(DBG_DEBUG, "Mod(%s) phy-addr(%#llx).\n", sys_get_modname(mod_id), p->phy_addr);
	return (vb_blk)p;
}

static int32_t _vb_get_blk_info(struct vb_blk_info *blk_info)
{
	vb_blk blk = (vb_blk)blk_info->blk;
	struct vb_s *vb;

	vb = (struct vb_s *)blk;
	CHECK_VB_HANDLE_NULL(vb);
	CHECK_VB_HANDLE_VALID(vb);

	blk_info->phy_addr = vb->phy_addr;
	blk_info->pool_id = vb->poolid;
	blk_info->usr_cnt = vb->usr_cnt.counter;
	return 0;
}

static int32_t _vb_get_pool_cfg(struct vb_pool_cfg *pool_cfg)
{
	vb_pool poolid = pool_cfg->pool_id;

	if (atomic_read(&ref_count) == 0) {
		TRACE_BASE(DBG_ERR, "vb module hasn't inited yet.\n");
		return VB_INVALID_POOLID;
	}
	CHECK_VB_POOL_VALID_STRONG(poolid);

	pool_cfg->blk_cnt = g_vb_ctx[poolid].blk_cnt;
	pool_cfg->blk_size = g_vb_ctx[poolid].blk_size;
	pool_cfg->remap_mode = g_vb_ctx[poolid].remap_mode;
	pool_cfg->mem_base = g_vb_ctx[poolid].membase;

	return 0;
}

/**************************************************************************
 *	 global APIs.
 **************************************************************************/
int32_t vb_get_pool_info(struct vb_pool_ctx **pool_info, uint32_t *max_pool, uint32_t *max_blk)
{
	CHECK_VB_HANDLE_NULL(pool_info);
	CHECK_VB_HANDLE_NULL(g_vb_ctx);

	*pool_info = g_vb_ctx;
	*max_pool = vb_max_pools;
	*max_blk = vb_pool_max_blk;

	return 0;
}

void vb_cleanup(void)
{
	mutex_lock(&g_lock);
	if (atomic_read(&ref_count) == 0) {
		TRACE_BASE(DBG_INFO, "vb has already exited\n");
		mutex_unlock(&g_lock);
		return;
	}
	_vb_cleanup();
	atomic_set(&ref_count, 0);
	mutex_unlock(&g_lock);
	TRACE_BASE(DBG_INFO, "vb_cleanup done\n");
}

int32_t vb_get_config(struct vb_cfg *vb_config)
{
	if (!vb_config) {
		TRACE_BASE(DBG_ERR, "vb_get_config NULL ptr!\n");
		return -EINVAL;
	}

	*vb_config = g_vb_config;
	return 0;
}
EXPORT_SYMBOL_GPL(vb_get_config);

int32_t vb_create_pool(struct vb_pool_cfg *config)
{
	uint32_t i;
	int32_t ret;

	config->pool_id = VB_INVALID_POOLID;
	if ((config->blk_size == 0) || (config->blk_cnt == 0)
		|| (config->blk_cnt > vb_pool_max_blk)) {
		TRACE_BASE(DBG_ERR, "Invalid pool cfg, blk_size(%d), blk_cnt(%d)\n",
				config->blk_size, config->blk_cnt);
		return -EINVAL;
	}

	mutex_lock(&g_pool_lock);
	for (i = VB_MAX_COMM_POOLS; i < vb_max_pools; ++i) {
		if (!is_pool_inited(i))
			break;
	}
	if (i >= vb_max_pools) {
		TRACE_BASE(DBG_ERR, "Exceed vb_max_pools cnt: %d\n", vb_max_pools);
		mutex_unlock(&g_pool_lock);
		return -ENOMEM;
	}

	config->pool_id = i;
	ret = _vb_create_pool(config, false);
	if (ret) {
		TRACE_BASE(DBG_ERR, "_vb_create_pool fail, ret(%d)\n", ret);
		mutex_unlock(&g_pool_lock);
		return ret;
	}
	mutex_unlock(&g_pool_lock);
	return 0;
}
EXPORT_SYMBOL_GPL(vb_create_pool);


int32_t vb_destroy_pool(vb_pool pool_id)
{
	CHECK_VB_POOL_VALID_STRONG(pool_id);

	return _vb_destroy_pool(pool_id);
}
EXPORT_SYMBOL_GPL(vb_destroy_pool);

/* vb_create_block: create a vb blk per phy-addr given.
 *
 * @param phy_addr: phy-address of the buffer for this new vb.
 * @param vir_addr: virtual-address of the buffer for this new vb.
 * @param pool_id: the pool of the vb belonging.
 * @param is_external: if the buffer is not allocated by mmf
 */
vb_blk vb_create_block(uint64_t phy_addr, void *vir_addr, vb_pool pool_id, bool is_external)
{
	struct vb_s *p = NULL;

	p = vmalloc(sizeof(*p));
	if (!p) {
		TRACE_BASE(DBG_ERR, "vmalloc failed.\n");
		return VB_INVALID_HANDLE;
	}

	p->phy_addr = phy_addr;
	p->vir_addr = vir_addr;
	p->poolid = pool_id;
	atomic_set(&p->usr_cnt, 1);
	p->magic = VB_MAGIC;
	atomic_long_set(&p->mod_ids, 0);
	p->external = is_external;
	spin_lock(&g_hash_lock);
	hash_add(vb_hash, &p->node, p->phy_addr);
	spin_unlock(&g_hash_lock);

	return (vb_blk)p;
}
EXPORT_SYMBOL_GPL(vb_create_block);

vb_blk vb_get_block_with_id(vb_pool pool_id, uint32_t blk_size, mod_id_e mod_id)
{
	vb_blk blk = VB_INVALID_HANDLE;

	mutex_lock(&g_get_vb_lock);
	// common pool
	if (pool_id == VB_INVALID_POOLID) {
		pool_id = find_vb_pool(blk_size);
		if (pool_id == VB_INVALID_POOLID) {
			TRACE_BASE(DBG_ERR, "No valid pool for size(%d).\n", blk_size);
			goto get_vb_done;
		}
	} else if (pool_id == VB_STATIC_POOLID) {
		blk = _vb_get_block_static(blk_size);		//need not mapping pool, allocate vb block directly
		goto get_vb_done;
	} else if (pool_id >= vb_max_pools) {
		TRACE_BASE(DBG_ERR, " invalid VB Pool(%d)\n", pool_id);
		goto get_vb_done;
	} else {
		if (!is_pool_inited(pool_id)) {
			TRACE_BASE(DBG_ERR, "vb_pool(%d) isn't init yet.\n", pool_id);
			goto get_vb_done;
		}

		if (blk_size > g_vb_ctx[pool_id].blk_size) {
			TRACE_BASE(DBG_ERR, "required size(%d) > pool(%d)'s blk-size(%d).\n", blk_size, pool_id,
					 g_vb_ctx[pool_id].blk_size);
			goto get_vb_done;
		}
	}
	blk = _vb_get_block(&g_vb_ctx[pool_id], blk_size, mod_id);

get_vb_done:
	mutex_unlock(&g_get_vb_lock);
	return blk;
}
EXPORT_SYMBOL_GPL(vb_get_block_with_id);

int32_t vb_release_block(vb_blk blk)
{
	struct vb_s *vb = (struct vb_s *)blk;
	struct vb_s *vb_tmp;
	struct vb_pool_ctx *pool;
	int cnt;
	int32_t result;
	bool bReq = false;
	struct vb_req *req, *tmp;

	CHECK_VB_HANDLE_NULL(vb);
	CHECK_VB_HANDLE_VALID(vb);
	CHECK_VB_POOL_VALID_WEAK(vb->poolid);

	cnt = atomic_sub_return(1, &vb->usr_cnt);
	if (cnt <= 0) {
		TRACE_BASE(DBG_DEBUG, "%p phy-addr(%#llx) release.\n",
			__builtin_return_address(0), vb->phy_addr);

		if (vb->external) {
			TRACE_BASE(DBG_DEBUG, "external buffer phy-addr(%#llx) release.\n", vb->phy_addr);
			_vb_hash_del(vb->phy_addr);
			vfree(vb);
			return 0;
		}

		//free VB_STATIC_POOLID
		if (vb->poolid == VB_STATIC_POOLID) {
			int32_t ret = 0;

			ret = base_ion_free(vb->phy_addr);
			_vb_hash_del(vb->phy_addr);
			vfree(vb);
			return ret;
		}

		if (cnt < 0) {
			int i = 0;
			TRACE_BASE(DBG_INFO, "vb usr_cnt is zero.\n");
			pool = &g_vb_ctx[vb->poolid];
			mutex_lock(&pool->lock);
			FIFO_FOREACH(vb_tmp, &pool->freelist, i) {
				if (vb_tmp->phy_addr == vb->phy_addr) {
					mutex_unlock(&pool->lock);
					atomic_set(&vb->usr_cnt, 0);
					return 0;
				}
			}
			mutex_unlock(&pool->lock);
		}

		pool = &g_vb_ctx[vb->poolid];
		mutex_lock(&pool->lock);
		memset(&vb->buf, 0, sizeof(vb->buf));
		atomic_set(&vb->usr_cnt, 0);
		atomic_long_set(&vb->mod_ids, 0);
		FIFO_PUSH(&pool->freelist, vb);
		++pool->free_blk_cnt;
		mutex_unlock(&pool->lock);

		mutex_lock(&pool->reqq_lock);
		if (!STAILQ_EMPTY(&pool->reqq)) {
			STAILQ_FOREACH_SAFE(req, &pool->reqq, stailq, tmp) {
				if (req->poolid != pool->poolid)
					continue;

				TRACE_BASE(DBG_INFO, "pool(%d) vb(%#llx) release, Try acquire vb for %s\n", pool->poolid,
					vb->phy_addr, sys_get_modname(req->chn.mod_id));
				STAILQ_REMOVE(&pool->reqq, req, vb_req, stailq);
				bReq = true;
				break;
			}
		}
		mutex_unlock(&pool->reqq_lock);
		if (bReq) {
			result = req->fp(req->chn, req->data);
			if (result) { // req->fp return fail
				mutex_lock(&pool->reqq_lock);
				STAILQ_INSERT_TAIL(&pool->reqq, req, stailq);
				mutex_unlock(&pool->reqq_lock);
			} else
				kfree(req);
		}
	}
	return 0;
}
EXPORT_SYMBOL_GPL(vb_release_block);

vb_blk vb_phys_addr2handle(uint64_t phy_addr)
{
	struct vb_s *vb = NULL;

	if (!_vb_hash_find(phy_addr, &vb)) {
		TRACE_BASE(DBG_DEBUG, "Cannot find vb corresponding to phyAddr:%#llx\n", phy_addr);
		return VB_INVALID_HANDLE;
	} else
		return (vb_blk)vb;
}
EXPORT_SYMBOL_GPL(vb_phys_addr2handle);

uint64_t vb_handle2phys_addr(vb_blk blk)
{
	struct vb_s *vb = (struct vb_s *)blk;

	if ((!blk) || (blk == VB_INVALID_HANDLE) || (vb->magic != VB_MAGIC))
		return 0;
	return vb->phy_addr;
}
EXPORT_SYMBOL_GPL(vb_handle2phys_addr);

void *vb_handle2virt_addr(vb_blk blk)
{
	struct vb_s *vb = (struct vb_s *)blk;

	if ((!blk) || (blk == VB_INVALID_HANDLE) || (vb->magic != VB_MAGIC))
		return NULL;
	return vb->vir_addr;
}
EXPORT_SYMBOL_GPL(vb_handle2virt_addr);

vb_pool vb_handle2pool_id(vb_blk blk)
{
	struct vb_s *vb = (struct vb_s *)blk;

	if ((!blk) || (blk == VB_INVALID_HANDLE) || (vb->magic != VB_MAGIC))
		return VB_INVALID_POOLID;
	return vb->poolid;
}
EXPORT_SYMBOL_GPL(vb_handle2pool_id);

int32_t vb_inquire_user_cnt(vb_blk blk, uint32_t *cnt)
{
	struct vb_s *vb = (struct vb_s *)blk;

	CHECK_VB_HANDLE_NULL(vb);
	CHECK_VB_HANDLE_VALID(vb);

	*cnt = vb->usr_cnt.counter;
	return 0;
}
EXPORT_SYMBOL_GPL(vb_inquire_user_cnt);

/* vb_acquire_block: to register a callback to acquire vb_blk at VB_ReleaseBlock
 *						in case of VB_GetBlock failure.
 *
 * @param fp: callback to acquire blk for module.
 * @param chn: info of the module which needs this helper.
 */
void vb_acquire_block(vb_acquire_fp fp, mmf_chn_s chn, vb_pool pool_id, void *data)
{
	struct vb_req *req = NULL;

	if (pool_id == VB_INVALID_POOLID) {
		TRACE_BASE(DBG_ERR, "invalid poolid.\n");
		return;
	}
	if (pool_id >= vb_max_pools) {
		TRACE_BASE(DBG_ERR, " invalid VB Pool(%d)\n", pool_id);
		return;
	}
	if (!is_pool_inited(pool_id)) {
		TRACE_BASE(DBG_ERR, "vb_pool(%d) isn't init yet.\n", pool_id);
		return;
	}

	req = kmalloc(sizeof(*req), GFP_ATOMIC);
	if (!req) {
		//TRACE_BASE(DBG_ERR, "kmalloc failed.\n");	warning2error fail
		return;
	}

	req->fp = fp;
	req->chn = chn;
	req->poolid = pool_id;
	req->data = data;

	mutex_lock(&g_vb_ctx[pool_id].reqq_lock);
	STAILQ_INSERT_TAIL(&g_vb_ctx[pool_id].reqq, req, stailq);
	mutex_unlock(&g_vb_ctx[pool_id].reqq_lock);
}
EXPORT_SYMBOL_GPL(vb_acquire_block);

void vb_cancel_block(mmf_chn_s chn, vb_pool pool_id)
{
	struct vb_req *req, *tmp;

	if (pool_id == VB_INVALID_POOLID) {
		TRACE_BASE(DBG_ERR, "invalid poolid.\n");
		return;
	}
	if (pool_id >= vb_max_pools) {
		TRACE_BASE(DBG_ERR, " invalid VB Pool(%d)\n", pool_id);
		return;
	}
	if (!is_pool_inited(pool_id)) {
		TRACE_BASE(DBG_ERR, "vb_pool(%d) isn't init yet.\n", pool_id);
		return;
	}

	mutex_lock(&g_vb_ctx[pool_id].reqq_lock);
	if (!STAILQ_EMPTY(&g_vb_ctx[pool_id].reqq)) {
		STAILQ_FOREACH_SAFE(req, &g_vb_ctx[pool_id].reqq, stailq, tmp) {
			if (CHN_MATCH(&req->chn, &chn)) {
				STAILQ_REMOVE(&g_vb_ctx[pool_id].reqq, req, vb_req, stailq);
				kfree(req);
			}
		}
	}
	mutex_unlock(&g_vb_ctx[pool_id].reqq_lock);
}
EXPORT_SYMBOL_GPL(vb_cancel_block);

long vb_ctrl(unsigned long arg)
{
	long ret = 0;
	struct vb_ext_control p;

	if (copy_from_user(&p, (void __user *)arg, sizeof(struct vb_ext_control)))
		return -EINVAL;

	switch (p.id) {
	case VB_IOCTL_SET_CONFIG: {
		struct vb_cfg cfg;

		if (atomic_read(&ref_count)) {
			TRACE_BASE(DBG_ERR, "vb has already inited, set_config cmd has no effect\n");
			break;
		}

		memset(&cfg, 0, sizeof(struct vb_cfg));
		if (copy_from_user(&cfg, p.ptr, sizeof(struct vb_cfg))) {
			TRACE_BASE(DBG_ERR, "VB_IOCTL_SET_CONFIG copy_from_user failed.\n");
			ret = -ENOMEM;
			break;
		}
		ret = _vb_set_config(&cfg);
		break;
	}

	case VB_IOCTL_GET_CONFIG: {
		if (copy_to_user(p.ptr, &g_vb_config, sizeof(struct vb_cfg))) {
			TRACE_BASE(DBG_ERR, "VB_IOCTL_GET_CONFIG copy_to_user failed.\n");
			ret = -ENOMEM;
		}
		break;
	}

	case VB_IOCTL_INIT:
		ret = _vb_init();
		break;

	case VB_IOCTL_EXIT:
		ret = _vb_exit();
		break;

	case VB_IOCTL_CREATE_POOL: {
		struct vb_pool_cfg cfg;

		memset(&cfg, 0, sizeof(struct vb_pool_cfg));
		if (copy_from_user(&cfg, p.ptr, sizeof(struct vb_pool_cfg))) {
			TRACE_BASE(DBG_ERR, "VB_IOCTL_CREATE_POOL copy_from_user failed.\n");
			ret = -ENOMEM;
			break;
		}

		ret = vb_create_pool(&cfg);
		if (ret == 0) {
			if (copy_to_user(p.ptr, &cfg, sizeof(struct vb_pool_cfg))) {
				TRACE_BASE(DBG_ERR, "VB_IOCTL_CREATE_POOL copy_to_user failed.\n");
				ret = -ENOMEM;
			}
		}
		break;
	}

	case VB_IOCTL_DESTROY_POOL: {
		vb_pool pool_id;

		pool_id = (vb_pool)p.value;
		ret = vb_destroy_pool(pool_id);
		break;
	}

	case VB_IOCTL_GET_BLOCK: {
		struct vb_blk_cfg cfg;
		vb_blk block;

		memset(&cfg, 0, sizeof(struct vb_blk_cfg));
		if (copy_from_user(&cfg, p.ptr, sizeof(struct vb_blk_cfg))) {
			TRACE_BASE(DBG_ERR, "VB_IOCTL_GET_BLOCK copy_from_user failed.\n");
			ret = -ENOMEM;
			break;
		}

		block = vb_get_block_with_id(cfg.pool_id, cfg.blk_size, ID_USER);
		if (block == VB_INVALID_HANDLE)
			ret = -ENOMEM;
		else {
			cfg.blk = (uint64_t)block;
			if (copy_to_user(p.ptr, &cfg, sizeof(struct vb_blk_cfg))) {
				TRACE_BASE(DBG_ERR, "VB_IOCTL_GET_BLOCK copy_to_user failed.\n");
				ret = -ENOMEM;
			}
		}
		break;
	}

	case VB_IOCTL_RELEASE_BLOCK: {
		vb_blk blk = (vb_blk)p.value64;

		ret = vb_release_block(blk);
		break;
	}

	case VB_IOCTL_PHYS_TO_HANDLE: {
		struct vb_blk_info blk_info;
		vb_blk block;

		memset(&blk_info, 0, sizeof(struct vb_blk_info));
		if (copy_from_user(&blk_info, p.ptr, sizeof(struct vb_blk_info))) {
			TRACE_BASE(DBG_ERR, "VB_IOCTL_PHYS_TO_HANDLE copy_from_user failed.\n");
			ret = -ENOMEM;
			break;
		}

		block = vb_phys_addr2handle(blk_info.phy_addr);
		if (block == VB_INVALID_HANDLE)
			ret = -EINVAL;
		else {
			blk_info.blk = (uint64_t)block;
			if (copy_to_user(p.ptr, &blk_info, sizeof(struct vb_blk_info))) {
				TRACE_BASE(DBG_ERR, "VB_IOCTL_PHYS_TO_HANDLE copy_to_user failed.\n");
				ret = -ENOMEM;
			}
		}
		break;
	}

	case VB_IOCTL_GET_BLK_INFO: {
		struct vb_blk_info blk_info;

		memset(&blk_info, 0, sizeof(struct vb_blk_info));
		if (copy_from_user(&blk_info, p.ptr, sizeof(struct vb_blk_info))) {
			TRACE_BASE(DBG_ERR, "VB_IOCTL_GET_BLK_INFO copy_from_user failed.\n");
			ret = -ENOMEM;
			break;
		}

		ret = _vb_get_blk_info(&blk_info);
		if (ret == 0) {
			if (copy_to_user(p.ptr, &blk_info, sizeof(struct vb_blk_info))) {
				TRACE_BASE(DBG_ERR, "VB_IOCTL_GET_BLK_INFO copy_to_user failed.\n");
				ret = -ENOMEM;
			}
		}
		break;
	}

	case VB_IOCTL_GET_POOL_CFG: {
		struct vb_pool_cfg pool_cfg;

		memset(&pool_cfg, 0, sizeof(struct vb_pool_cfg));
		if (copy_from_user(&pool_cfg, p.ptr, sizeof(struct vb_pool_cfg))) {
			TRACE_BASE(DBG_ERR, "VB_IOCTL_GET_POOL_CFG copy_from_user failed.\n");
			ret = -ENOMEM;
			break;
		}

		ret = _vb_get_pool_cfg(&pool_cfg);
		if (ret == 0) {
			if (copy_to_user(p.ptr, &pool_cfg, sizeof(struct vb_pool_cfg))) {
				TRACE_BASE(DBG_ERR, "VB_IOCTL_GET_POOL_CFG copy_to_user failed.\n");
				ret = -ENOMEM;
			}
		}
		break;
	}

	case VB_IOCTL_GET_POOL_MAX_CNT: {
		p.value = (int32_t)vb_max_pools;
		break;
	}

	case VB_IOCTL_PRINT_POOL: {
		vb_pool pool_id;

		pool_id = (vb_pool)p.value;
		ret = vb_print_pool(pool_id);
		break;
	}

	default:
		break;
	}
	if (copy_to_user((void __user *)arg, &p, sizeof(struct vb_ext_control)))
		return -EINVAL;
	return ret;
}

int32_t vb_create_instance(void)
{
	if (vb_max_pools < VB_MAX_COMM_POOLS) {
		TRACE_BASE(DBG_ERR, "vb_max_pools is too small!\n");
		return -EINVAL;
	}

	g_vb_ctx = vzalloc(sizeof(struct vb_pool_ctx) * vb_max_pools);
	if (!g_vb_ctx) {
		TRACE_BASE(DBG_ERR, "g_vb_ctx kzalloc fail!\n");
		return -ENOMEM;
	}
	TRACE_BASE(DBG_INFO, "vb_max_pools(%d) vb_pool_max_blk(%d)\n", vb_max_pools, vb_pool_max_blk);

	return 0;
}

void vb_destroy_instance(void)
{
	if (g_vb_ctx) {
		vfree(g_vb_ctx);
		g_vb_ctx = NULL;
	}
}

void vb_release(void)
{
	_vb_exit();
}
