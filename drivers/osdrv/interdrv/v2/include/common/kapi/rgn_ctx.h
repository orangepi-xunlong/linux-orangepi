#ifndef __RGN_CTX_H__
#define __RGN_CTX_H__

#ifdef __cplusplus
	extern "C" {
#endif

#include <linux/comm_region.h>
#include <base_ctx.h>

/*
 * @ion_len: canvas's ion length.
 * @canvas_idx: the canvas buf-idx used by hw now.
 * @canvas_get: true if CVI_RGN_GetCanvasInfo(), false after CVI_RGN_UpdateCanvas().
 */
struct rgn_ctx {
	rgn_handle handle;
	unsigned char created;
	unsigned char used;
	rgn_attr_s region;
	mmf_chn_s chn;
	rgn_chn_attr_s chn_attr;
	rgn_canvas_info_s canvas_info[RGN_MAX_BUF_NUM];
	unsigned int max_need_ion;
	unsigned int ion_len;
	unsigned char canvas_idx;
	unsigned char canvas_get;
	unsigned char odec_data_valid;
	unsigned char canvas_updated;
	struct hlist_node node;
	struct rgn_canvas_q rgn_canvas_waitq;
	struct rgn_canvas_q rgn_canvas_doneq;
	struct mutex rgn_canvas_q_lock;
};

#ifdef __cplusplus
}
#endif

#endif /* __U_CVI_RGN_CTX_H__ */
