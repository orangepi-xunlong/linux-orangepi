#ifndef __VB_H__
#define __VB_H__

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

#include "linux/common.h"
#include "linux/comm_vb.h"
#include <linux/comm_sys.h>
#include <linux/base_uapi.h>
#include "base_ctx.h"

/*
 *
 * frame_crop: for dis
 * offsetxxx: equal to the offset member in video_frame_s.
 *               to show the invalid area in size.
 */
struct video_buffer {
	uint64_t phy_addr[NUM_OF_PLANES];
	size_t length[NUM_OF_PLANES];
	uint32_t stride[NUM_OF_PLANES];
	size_s size;
	uint64_t pts;
	uint8_t dev_num;
	pixel_format_e pixel_format;
	compress_mode_e compress_mode;
	uint64_t compress_expand_addr;
	uint32_t frm_num;
	struct crop_size frame_crop;

	int16_t offset_top;
	int16_t offset_bottom;
	int16_t offset_left;
	int16_t offset_right;

	uint8_t  motion_lv;
	uint8_t  motion_table[MO_TBL_SIZE];

	uint32_t frame_flag;
	uint32_t sequence;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
	struct timespec64 timestamp;
#else
	struct timeval timestamp;
#endif
};

/*
 * pool: the pool this blk belongs to.
 * phy_addr: physical address of the blk.
 * vir_addr: virtual address of the blk.
 * usr_cnt: ref-count of the blk.
 * buf: the usage which define planes of buffer.
 * magic: magic number to avoid wrong reference.
 * mod_ids: the users of this blk. BIT(MOD_ID) will be set is MOD using.
 */
struct vb_s {
	vb_pool poolid;
	uint64_t phy_addr;
	void *vir_addr;
	atomic_t usr_cnt;
	struct video_buffer buf;
	uint32_t magic;
	atomic64_t mod_ids;
	uint8_t external;
	struct hlist_node node;
};

FIFO_HEAD(vbq, vb_s*);

typedef int32_t(*vb_acquire_fp)(mmf_chn_s, void *);

struct vb_req {
	STAILQ_ENTRY(vb_req) stailq;
	vb_acquire_fp fp;
	mmf_chn_s chn;
	vb_pool poolid;
	void *data;
};

STAILQ_HEAD(vb_req_q, vb_req);


struct vb_pool_ctx {
	vb_pool poolid;
	int16_t ownerid;
	uint64_t membase;
	void *vmembase;
	struct vbq freelist;
	struct vb_req_q reqq;
	uint32_t blk_size;
	uint32_t blk_cnt;
	vb_remap_mode_e remap_mode;
	uint8_t is_comm_pool;
	uint32_t free_blk_cnt;
	uint32_t min_free_blk_cnt;
	char pool_name[VB_POOL_NAME_LEN];
	struct mutex lock;
	struct mutex reqq_lock;
};


/* Generall common pool use this owner id, module common pool use VB_UID as owner id */
#define POOL_OWNER_COMMON -1
/* Private pool use this owner id */
#define POOL_OWNER_PRIVATE -2

#define VB_MAGIC 0xbabeface

int32_t vb_get_pool_info(struct vb_pool_ctx **pool_info, uint32_t *max_pool, uint32_t *max_blk);
void vb_cleanup(void);
int32_t vb_get_config(struct vb_cfg *vb_config);

int32_t vb_create_pool(struct vb_pool_cfg *config);
int32_t vb_destroy_pool(uint32_t pool_id);

vb_blk vb_phys_addr2handle(uint64_t phy_addr);
uint64_t vb_handle2phys_addr(vb_blk blk);
void *vb_handle2virt_addr(vb_blk blk);
vb_pool vb_handle2pool_id(vb_blk blk);
int32_t vb_inquire_user_cnt(vb_blk blk, uint32_t *cnt);

vb_blk vb_get_block_with_id(vb_pool pool_id, uint32_t blk_size, mod_id_e mod_id);
vb_blk vb_create_block(uint64_t phy_addr, void *vir_addr, vb_pool pool_id, bool is_external);
int32_t vb_release_block(vb_blk blk);
vb_pool find_vb_pool(uint32_t blk_size);
void vb_acquire_block(vb_acquire_fp fp, mmf_chn_s chn, vb_pool pool_id, void *data);
void vb_cancel_block(mmf_chn_s chn, vb_pool pool_id);
long vb_ctrl(unsigned long arg);
int32_t vb_create_instance(void);
void vb_destroy_instance(void);
void vb_release(void);


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /* __VB_H__ */
