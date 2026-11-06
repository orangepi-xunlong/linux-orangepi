#ifndef __U_VI_UAPI_H__
#define __U_VI_UAPI_H__

#ifdef __cplusplus
	extern "C" {
#endif

#include <linux/version.h>
#if (KERNEL_VERSION(5, 10, 0) <= LINUX_VERSION_CODE)
#include <linux/time_types.h>
#endif
#include <linux/comm_vi.h>

#define VI_IOC_MAGIC		'V'
#define VI_IOC_BASE		0x20

#define VI_IOC_G_CTRL		_IOWR(VI_IOC_MAGIC, VI_IOC_BASE, struct vi_ext_control)
#define VI_IOC_S_CTRL		_IOWR(VI_IOC_MAGIC, VI_IOC_BASE + 1, struct vi_ext_control)

enum vi_ioctl_e {
	VI_IOCTL_ONLINE,
	VI_IOCTL_HDR,
	VI_IOCTL_3DNR,
	VI_IOCTL_NEW_3DNR,
	VI_IOCTL_TILE,
	VI_IOCTL_COMPRESS_EN,
	VI_IOCTL_AI_ISP_CFG,
	VI_IOCTL_AI_ISP_SET_BUF,
	VI_IOCTL_STS_MEM,
	VI_IOCTL_STS_GET,
	VI_IOCTL_STS_PUT,
	VI_IOCTL_POST_STS_GET,
	VI_IOCTL_POST_STS_PUT,
	VI_IOCTL_USR_PIC_CFG,
	VI_IOCTL_USR_PIC_ONOFF,
	VI_IOCTL_USR_PIC_PUT,
	VI_IOCTL_AE_CFG,
	VI_IOCTL_AWB_CFG,
	VI_IOCTL_AF_CFG,
	VI_IOCTL_USR_PIC_TIMING,
	VI_IOCTL_GET_LSC_PHY_BUF,
	VI_IOCTL_CSIBDG_CFG,
	VI_IOCTL_GET_TUN_ADDR,
	VI_IOCTL_SET_SNR_INFO,
	VI_IOCTL_SET_SNR_CFG_NODE,
	VI_IOCTL_GET_PIPE_DUMP,
	VI_IOCTL_PUT_PIPE_DUMP,
	VI_IOCTL_SET_RGBMAP_IDX,
	VI_IOCTL_HDR_DETAIL_EN,
	VI_IOCTL_YUV_BYPASS_PATH,
	VI_IOCTL_BE_ONLINE,
	VI_IOCTL_SUBLVDS_PATH,
	VI_IOCTL_GET_IP_INFO,
	VI_IOCTL_TRIG_PRERAW,
	VI_IOCTL_SET_PROC_CONTENT,
	VI_IOCTL_SET_SC_ONLINE,
	VI_IOCTL_GET_SC_ONLINE,
	VI_IOCTL_MMAP_GRID_SIZE,
	VI_IOCTL_RGBIR,
	VI_IOCTL_AWB_STS_GET,
	VI_IOCTL_AWB_STS_PUT,
	VI_IOCTL_GET_FSWDR_PHY_BUF,
	VI_IOCTL_GET_SCENE_INFO,
	VI_IOCTL_CLK_CTRL,
	VI_IOCTL_GET_BUF_SIZE,
	VI_IOCTL_SET_DMA_BUF_INFO,
	VI_IOCTL_ENQ_BUF,
	VI_IOCTL_DQEVENT,
	VI_IOCTL_START_STREAMING,
	VI_IOCTL_STOP_STREAMING,
	VI_IOCTL_SET_DEV_NUM,
	VI_IOCTL_GET_DEV_NUM,
	VI_IOCTL_SET_SLICE_BUF_EN,
	VI_IOCTL_GET_CLUT_TBL_IDX,
	VI_IOCTL_SDK_CTRL,
	VI_IOCTL_GET_RGBMAP_LE_PHY_BUF,
	VI_IOCTL_GET_RGBMAP_SE_PHY_BUF,
	VI_IOCTL_GET_IR_LE_PHY_BUF,
	VI_IOCTL_GET_IR_SE_PHY_BUF,
	VI_IOCTL_GET_AI_ISP_RAW,
	VI_IOCTL_PUT_AI_ISP_RAW,
	VI_IOCTL_MAX,
};

enum vi_sdk_ctrl_e {
	VI_SDK_SET_DEV_ATTR,
	VI_SDK_GET_DEV_ATTR,
	VI_SDK_SET_DEV_BIND_ATTR,
	VI_SDK_GET_DEV_BIND_ATTR,
	VI_SDK_SET_DEV_UNBIND_ATTR,
	VI_SDK_GET_PIPE_STATUS,
	VI_SDK_GET_CHN_STATUS,
	VI_SDK_ENABLE_DEV,
	VI_SDK_DISABLE_DEV,
	VI_SDK_CREATE_PIPE,
	VI_SDK_DESTROY_PIPE,
	VI_SDK_SET_PIPE_ATTR,
	VI_SDK_GET_PIPE_ATTR,
	VI_SDK_START_PIPE,
	VI_SDK_STOP_PIPE,
	VI_SDK_SET_CHN_ATTR,
	VI_SDK_GET_CHN_ATTR,
	VI_SDK_ENABLE_CHN,
	VI_SDK_DISABLE_CHN,
	VI_SDK_SET_MOTION_LV,
	VI_SDK_ENABLE_DIS,
	VI_SDK_DISABLE_DIS,
	VI_SDK_SET_DIS_INFO,
	VI_SDK_SET_BYPASS_FRM,
	VI_SDK_SET_PIPE_FRM_SRC,
	VI_SDK_GET_PIPE_FRM_SRC,
	VI_SDK_SEND_PIPE_RAW,
	VI_SDK_SET_DEV_TIMING_ATTR,
	VI_SDK_GET_DEV_TIMING_ATTR,
	VI_SDK_GET_CHN_FRAME,
	VI_SDK_RELEASE_CHN_FRAME,
	VI_SDK_SET_CHN_CROP,
	VI_SDK_GET_CHN_CROP,
	VI_SDK_SET_PIPE_CROP,
	VI_SDK_GET_PIPE_CROP,
	VI_SDK_GET_PIPE_FRAME,
	VI_SDK_RELEASE_PIPE_FRAME,
	VI_SDK_START_SMOOTH_RAWDUMP,
	VI_SDK_STOP_SMOOTH_RAWDUMP,
	VI_SDK_GET_SMOOTH_RAWDUMP,
	VI_SDK_PUT_SMOOTH_RAWDUMP,
	VI_SDK_SET_CHN_ROTATION,
	VI_SDK_GET_CHN_ROTATION,
	VI_SDK_SET_CHN_LDC,
	VI_SDK_GET_CHN_LDC,
	VI_SDK_SET_CHN_FLIP_MIRROR,
	VI_SDK_GET_CHN_FLIP_MIRROR,
	VI_SDK_ATTACH_VB_POOL,
	VI_SDK_DETACH_VB_POOL,
	VI_SDK_GET_PIPE_DUMP_ATTR,
	VI_SDK_SET_PIPE_DUMP_ATTR,
	VI_SDK_GET_DEV_STATUS
};

/*
 * Events
 */
enum vi_event_e {
	VI_EVENT_BASE,
	VI_EVENT_PRE0_SOF,
	VI_EVENT_PRE1_SOF,
	VI_EVENT_PRE2_SOF,
	VI_EVENT_PRE3_SOF,
	VI_EVENT_PRE4_SOF,
	VI_EVENT_PRE5_SOF,
	VI_EVENT_PRE0_EOF,
	VI_EVENT_PRE1_EOF,
	VI_EVENT_PRE2_EOF,
	VI_EVENT_PRE3_EOF,
	VI_EVENT_PRE4_EOF,
	VI_EVENT_PRE5_EOF,
	VI_EVENT_POST0_EOF,
	VI_EVENT_POST1_EOF,
	VI_EVENT_POST2_EOF,
	VI_EVENT_POST3_EOF,
	VI_EVENT_POST4_EOF,
	VI_EVENT_POST5_EOF,
	VI_EVENT_ISP_PROC_READ,
	VI_EVENT_MAX,
};

struct _vi_sdk_cfg {
	__s32 dev;
	__s32 pipe;
	__s32 chn;
	void *ptr;
	__s32 val;
};

struct vi_ext_control {
	__u32 id;
	__u32 sdk_id;
	struct _vi_sdk_cfg sdk_cfg;
	union {
		__s32 value;
		__s64 value64;
		void *ptr;
	};
} __attribute__ ((packed));

struct vi_plane {
	__u64 addr;
};

/*
 * @index: raw_num * VI_MAX_CHN_NUM + hw_chn_num
 * @length: length of planes
 * @planes: to describe buf
 * @reserved
 */
struct vi_buffer {
	__u32 index;
	__u32 length;
	struct vi_plane planes[3];
	__u32 reserved;
};

struct vi_event {
	__u32			dev_id;
	__u32			type;
	__u32			frame_sequence;
#if (KERNEL_VERSION(5, 10, 0) <= LINUX_VERSION_CODE)
	struct __kernel_timespec   timestamp;
#else
	struct timeval		timestamp;
#endif
};

#define MO_TBL_SIZE 256

struct mlv_info_s {
	__u8	sensor_num;
	__u32	frm_num;
	__u8	mlv;
	__u8	mtable[MO_TBL_SIZE];
};

struct crop_size_s {
	__u16 start_x;
	__u16 start_y;
	__u16 end_x;
	__u16 end_y;
};

struct dis_info_s {
	__u8   sensor_num;
	__u32  frm_num;
	struct crop_size_s dis_i;
};

struct vi_chn_rot_cfg {
	__s32 vi_pipe;
	__s32 vi_chn;
	rotation_e rotation;
};

struct vi_chn_ldc_cfg {
	__s32 vi_pipe;
	__s32 vi_chn;
	vi_ldc_attr_s ldc_attr;
	__u64 mesh_handle;
};

struct vi_chn_flip_mirror_cfg {
	__s32 vi_pipe;
	__s32 vi_chn;
	__u8  flip;
	__u8  mirror;
};

struct vi_vb_pool_cfg {
	__s32 vi_pipe;
	__s32 vi_chn;
	__u32 vb_pool;
};

/** ai isp define start **/
struct bmdrv_tensor_info {
	__u64 user_global_addr;
	__u64 compiled_global_addr;
	__u32 size;
} __attribute__ ((packed));

typedef struct bmdrv_api {
	int core_id;
	__u32 api_id;
	__u8 *api_addr;
	__u32 api_size;
} bmdrv_api_t;

typedef struct ai_isp_api {
	bmdrv_api_t bmdrv_api;
	__u32 vi_pipe;
	__u32 input_num;
	__u32 output_num;
	__u32 ai_isp_type;
	__u32 is_new_api_buf;
} ai_isp_api_t;

typedef struct ai_isp_bnr_cfg {
	__u64 swap_buf_index; // __32 *
	__u32 swap_buf_count;
} ai_isp_bnr_cfg_t;

typedef struct ai_isp_cfg {
	__u32 vi_pipe;
	__u32 ai_isp_cfg_type;
	__u32 ai_isp_type;
	__u64 param_addr;
	__u32 param_size;
} ai_isp_cfg_t;

enum ai_isp_type_e {
	AI_ISP_TYPE_BNR,
	AI_ISP_TYPE_TNR,
	AI_ISP_TYPE_BUTT,
};

enum ai_isp_cfg_type_e {
	AI_ISP_PIPE_LOAD,
	AI_ISP_PIPE_UNLOAD,
	AI_ISP_CFG_INIT,
	AI_ISP_CFG_DEINIT,
	AI_ISP_CFG_ENABLE,
	AI_ISP_CFG_DISABLE,
};
/** ai isp define end **/
#ifdef __cplusplus
	}
#endif

#endif /* __U_VI_UAPI_H__ */
