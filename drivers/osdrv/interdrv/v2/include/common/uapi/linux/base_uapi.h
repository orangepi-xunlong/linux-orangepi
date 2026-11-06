#ifndef __BASE_UAPI_H__
#define __BASE_UAPI_H__


#include <linux/version.h>
#include <linux/types.h>
#include <linux/comm_sys.h>

#define MAX_ION_BUFFER_NAME 32

#define VB_POOL_NAME_LEN        (32)
#define VB_COMM_POOL_MAX_CNT    (16)

#define BASE_LOG_LEVEL_OFFSET       (0x0)
#define LOG_LEVEL_RSV_SIZE          (sizeof(int32_t) * ID_BUTT)

#define BASE_VERSION_INFO_OFFSET    (BASE_LOG_LEVEL_OFFSET + LOG_LEVEL_RSV_SIZE)
#define VERSION_INFO_RSV_SIZE       (sizeof(mmf_version_s))

#define BASE_SHARE_MEM_SIZE         ALIGN(BASE_VERSION_INFO_OFFSET + VERSION_INFO_RSV_SIZE, 0x1000)

enum VB_IOCTL {
	VB_IOCTL_SET_CONFIG,
	VB_IOCTL_GET_CONFIG,
	VB_IOCTL_INIT,
	VB_IOCTL_EXIT,
	VB_IOCTL_CREATE_POOL,
	VB_IOCTL_DESTROY_POOL,
	VB_IOCTL_PHYS_TO_HANDLE,
	VB_IOCTL_GET_BLK_INFO,
	VB_IOCTL_GET_POOL_CFG,
	VB_IOCTL_GET_BLOCK,
	VB_IOCTL_RELEASE_BLOCK,
	VB_IOCTL_GET_POOL_MAX_CNT,
	VB_IOCTL_PRINT_POOL,
	VB_IOCTL_UNIT_TEST,
	VB_IOCTL_MAX,
};

/*
 * blk_size: size of blk in the pool.
 * blk_cnt: number of blk in the pool.
 * remap_mode: remap mode.
 * name: pool name
 * pool_id: pool id
 * mem_base: phy start addr of this pool
 */
struct vb_pool_cfg {
	__u32 blk_size;
	__u32 blk_cnt;
	__u8 remap_mode;
	__u8 pool_name[VB_POOL_NAME_LEN];
	__u32 pool_id;
	__u64 mem_base;
};

/*
 * comm_pool_cnt: number of common pools used.
 * comm_pool: pool cfg for the pools.
 */
struct vb_cfg {
	__u32 comm_pool_cnt;
	struct vb_pool_cfg comm_pool[VB_COMM_POOL_MAX_CNT];
};

struct vb_blk_cfg {
	__u32 pool_id;
	__u32 blk_size;
	__u64 blk;
};

struct vb_blk_info {
	__u64 blk;
	__u32 pool_id;
	__u64 phy_addr;
	__u32 usr_cnt;
};

struct vb_ext_control {
	__u32 id;
	__u32 reserved[1];
	union {
		__s32 value;
		__s64 value64;
		void *ptr;
	};
} __attribute__ ((packed));

struct sys_cache_op {
	void *addr_v;
	__u64 addr_p;
	__u64 size;
	__s32 dma_fd;
};

struct sys_ion_data {
	__u32 size;
	__u32 cached;
	__u64 addr_p;
	__u8 name[MAX_ION_BUFFER_NAME];
};

struct sys_bind_cfg {
	__u32 is_bind;
	__u32 get_by_src;
	mmf_chn_s mmf_chn_src;
	mmf_chn_s mmf_chn_dst;
	mmf_bind_dest_s bind_dst;
};


struct vip_rect {
	__s32 left;
	__s32 top;
	__u32 width;
	__u32 height;
};

enum rgn_format {
	RGN_FMT_ARGB8888,
	RGN_FMT_ARGB4444,
	RGN_FMT_ARGB1555,
	RGN_FMT_256LUT,
	RGN_FMT_16LUT,
	RGN_FMT_FONT,
	RGN_FMT_MAX
};
struct rgn_param {
	enum rgn_format fmt;
	struct vip_rect rect;
	__u32 stride;
	__u64 phy_addr;
};

struct rgn_odec {
	__u8 enable;
	__u8 attached_ow;
	__u8 canvas_updated;
	__u32 bso_sz;
	__u64 canvas_mutex_lock;
	__u64 rgn_canvas_waitq;
	__u64 rgn_canvas_doneq;
};

struct rgn_lut_cfg {
	__u16 lut_length;
	__u16 lut_addr[256];
	__u8 lut_layer;
	// __u8 rgnex_en;
	__u8 is_updated;
};
struct rgn_cfg {
	struct rgn_param param[8];
	struct rgn_lut_cfg rgn_lut_cfg;
	struct rgn_odec odec;
	__u8 num_of_rgn;
	__u8 hscale_x2;
	__u8 vscale_x2;
	__u8 colorkey_en;
	__u32 colorkey;
};

struct rgn_coverex_param {
	struct vip_rect rect;
	__u32 color;
	__u8 enable;
};

struct rgn_coverex_cfg {
	struct rgn_coverex_param rgn_coverex_param[4];
};

struct rgn_mosaic_cfg {
	__u8 enable;
	__u8 blk_size;	//0: 8x8   1:16x16
	__u16 start_x;
	__u16 start_y;
	__u16 end_x;
	__u16 end_y;
	__u64 phy_addr;
};

#define IOCTL_BASE_MAGIC	'b'
#define BASE_VB_CMD		    _IOWR(IOCTL_BASE_MAGIC, 0x01, struct vb_ext_control)
#define BASE_SET_BINDCFG	_IOW(IOCTL_BASE_MAGIC, 0x02, struct sys_bind_cfg)
#define BASE_GET_BINDCFG	_IOWR(IOCTL_BASE_MAGIC, 0x03, struct sys_bind_cfg)
#define BASE_ION_ALLOC		_IOWR(IOCTL_BASE_MAGIC, 0x04, struct sys_ion_data)
#define BASE_ION_FREE		_IOW(IOCTL_BASE_MAGIC, 0x05, struct sys_ion_data)
#define BASE_CACHE_INVLD	_IOW(IOCTL_BASE_MAGIC, 0x06, struct sys_cache_op)
#define BASE_CACHE_FLUSH	_IOW(IOCTL_BASE_MAGIC, 0x07, struct sys_cache_op)


#endif
