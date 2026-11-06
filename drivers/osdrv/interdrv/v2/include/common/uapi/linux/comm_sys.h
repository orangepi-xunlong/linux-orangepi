/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2020. All rights reserved.
 *
 * File Name: include/comm_sys.h
 * Description:
 *   The common sys type defination.
 */

#ifndef __COMM_SYS_H__
#define __COMM_SYS_H__

#include <linux/comm_video.h>

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

#define BIND_DEST_MAXNUM 64
#define BIND_NODE_MAXNUM 64

typedef struct _mmf_bind_dest_s {
	uint32_t num;
	mmf_chn_s mmf_chn[BIND_DEST_MAXNUM];
} mmf_bind_dest_s;

typedef struct _bind_node_s {
	uint8_t used;
	mmf_chn_s src;
	mmf_bind_dest_s dsts;
} bind_node_s;

typedef enum _vi_vpss_mode_e {
	VI_OFFLINE_VPSS_OFFLINE = 0,
	VI_OFFLINE_VPSS_ONLINE,
	VI_ONLINE_VPSS_OFFLINE,
	VI_ONLINE_VPSS_ONLINE,
	VI_BE_OFL_POST_OL_VPSS_OFL,
	VI_BE_OFL_POST_OFL_VPSS_OFL,
	VI_BE_OL_POST_OFL_VPSS_OFL,
	VI_BE_OL_POST_OL_VPSS_OFL,
	VI_VPSS_MODE_BUTT
} vi_vpss_mode_e;


typedef struct _vi_vpss_mode_s {
	vi_vpss_mode_e mode[VI_MAX_PIPE_NUM];
} vi_vpss_mode_s;

typedef struct _cdma_2d_s {
	uint64_t phy_addr_src;
	uint64_t phy_addr_dst;
	uint16_t width;
	uint16_t height;
	uint16_t stride_src;
	uint16_t stride_dst;
	uint16_t fixed_value;
	uint8_t enable_fixed;
} cdma_2d_s;

typedef struct _tdma_2d_s {
	uint64_t paddr_src;
	uint64_t paddr_dst;
	uint32_t w_bytes;
	uint32_t h;
	uint32_t stride_bytes_src;
	uint32_t stride_bytes_dst;
} tdma_2d_s;

typedef struct _vpss_venc_wrap_param_s {
	uint8_t all_online;

	uint32_t frame_rate;
	uint32_t full_linesstd;

	size_s large_stream_size;
	size_s small_stream_size;
} vpss_venc_wrap_param_s;

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif  /* __CVI_COMM_SYS_H__ */

