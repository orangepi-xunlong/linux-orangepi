#ifndef __LDC_CB_H__
#define __LDC_CB_H__

#ifdef __cplusplus
	extern "C" {
#endif

#include <linux/comm_sys.h>
#include <linux/comm_vb.h>
#include <base_ctx.h>

#define DEFAULT_MESH_PADDR	0x80000000
#define GDC_SUPPORT_FMT(fmt)                                                   \
	((fmt == PIXEL_FORMAT_NV12) || (fmt == PIXEL_FORMAT_NV21) ||           \
	 (fmt == PIXEL_FORMAT_YUV_400))

#define DWA_SUPPORT_FMT(fmt) \
	((fmt == PIXEL_FORMAT_YUV_PLANAR_420) || (fmt == PIXEL_FORMAT_YUV_PLANAR_444) ||		   \
	 (fmt == PIXEL_FORMAT_RGB_888_PLANAR) || PIXEL_FORMAT_BGR_888_PLANAR || (fmt == PIXEL_FORMAT_YUV_400))

enum gdc_usage {
	GDC_USAGE_ROTATION,
	GDC_USAGE_FISHEYE,
	GDC_USAGE_LDC,
	GDC_USAGE_MAX
};

typedef void (*gdc_cb)(void *, vb_blk);

struct mesh_gdc_cfg {
	enum gdc_usage usage;
	const void *usage_param;
	struct vb_s *vb_in;
	pixel_format_e pix_format;
	unsigned long long mesh_addr;
	unsigned char sync_io;
	void *cb_param;
	unsigned int cb_param_size;
	rotation_e rotation;
};

struct ldc_op_done_cfg {
	void *param;
	vb_blk blk;
};

enum ldc_cb_cmd {
	LDC_CB_MESH_GDC_OP,
	LDC_CB_VPSS_SBM_DONE,
	LDC_CB_GDC_OP_DONE = 100,	/* Skip VI/VPSS/VO self cmd */
	LDC_CB_MAX
};

#ifdef __cplusplus
}
#endif

#endif /* __LDC_CB_H__ */
