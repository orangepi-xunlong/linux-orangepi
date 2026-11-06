#ifndef __VI_CTX_H__
#define __VI_CTX_H__

#ifdef __cplusplus
	extern "C" {
#endif

#include <linux/defines.h>
#include <linux/comm_vi.h>

#define VI_SHARE_MEM_SIZE           (0x2000)

struct sop_vi_ctx {
	vi_state_e vi_stt;
	__u8 total_chn_num;
	__u8 total_dev_num;
	__u8 is_chn_enable[VI_MAX_CHN_NUM + VI_MAX_EXT_CHN_NUM];
	__u8 is_dev_enable[VI_MAX_DEV_NUM];
	__u8 is_tile;

	// mod param
	vi_mode_param_s mod_param;

	// dev
	vi_dev_attr_s dev_attr[VI_MAX_DEV_NUM];
	vi_dev_attr_ex_s dev_attr_ex[VI_MAX_DEV_NUM];
	vi_dev_timing_attr_s timing_attr[VI_MAX_DEV_NUM];
	vi_dev_bind_pipe_s bind_pipe_attr[VI_MAX_DEV_NUM];

	// pipe
	__u8 is_pipe_created[VI_MAX_PIPE_NUM];
	__u8 is_dis_enable[VI_MAX_PIPE_NUM];
	vi_pipe_frame_source_e source[VI_MAX_PIPE_NUM];
	vi_pipe_attr_s pipe_attr[VI_MAX_PIPE_NUM];
	crop_info_s    pipe_crop[VI_MAX_PIPE_NUM];
	vi_dump_attr_s dump_attr[VI_MAX_PIPE_NUM];

	// chn
	vi_chn_attr_s chn_attr[VI_MAX_CHN_NUM];
	vi_chn_status_s chn_status[VI_MAX_CHN_NUM];
	vi_crop_info_s chn_crop[VI_MAX_CHN_NUM];
	rotation_e rotation[VI_MAX_CHN_NUM];
	vi_ldc_attr_s ldc_attr[VI_MAX_CHN_NUM];
	vi_early_interrupt_s ealy_int[VI_MAX_CHN_NUM];

	__u32 blk_size[VI_MAX_CHN_NUM];
	__u32 timeout_cnt;
	__u8 bypass_frm[VI_MAX_CHN_NUM];
#ifdef __arm__
	__u32 vi_raw_blk[2];
#else
	__u64 vi_raw_blk[2];
#endif
	__s32 chn_bind[VI_MAX_CHN_NUM][VI_MAX_EXTCHN_BIND_PER_CHN];
	vi_ext_chn_attr_s ext_chn_attr[VI_MAX_EXT_CHN_NUM];
};

#ifdef __cplusplus
}
#endif

#endif /* __U_VI_CTX_H__ */
