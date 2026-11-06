#include <linux/module.h>
#include <linux/hashtable.h>
#include <linux/uaccess.h>
#include <uapi/linux/sched/types.h>

#include <base_ctx.h>
#include <linux/comm_errno.h>
#include <linux/delay.h>
#include <linux/random.h>

#include <rgn.h>
#include "proc/rgn_proc.h"
#include <cif_cb.h>
#include <vpss_cb.h>
#include <vo_cb.h>
#include <rgn_cb.h>
#include <cmdqu_cb.h>
#include "sys.h"
#include "ion.h"
#include "base_common.h"

/*******************************************************
 *  type definition
 ******************************************************/
struct rgn_mem_info {
	__u64 phy_addr;
	void *vir_addr;
	__u32 buf_len;
};

/*******************************************************
 *  Global variables
 ******************************************************/
//Keep every ctx include rgn_ctx and rgn_proc_ctx
struct rgn_ctx rgn_prc_ctx[RGN_MAX_NUM];
struct rgn_canvas_ctx rgn_canvas_ctx[RGN_MAX_NUM][RGN_MAX_BUF_NUM];
static struct rgn_mem_info mosaic_buf[VPSS_MAX_GRP_NUM][VPSS_MAX_CHN_NUM];

static unsigned int rgn_num;
static struct mutex hdlslock, g_rgnlock, g_rgnhashlock;

DECLARE_HASHTABLE(rgn_hash, 6);

/*******************************************************
 *  Internal APIs
 ******************************************************/
static bool _rgn_hash_find(rgn_handle handle, struct rgn_ctx **ctx, bool del)
{
	bool is_found = false;
	struct rgn_ctx *obj;
	int bkt;
	struct hlist_node *tmp;

#if 0 //Linux Hashmap for print every element
	unsigned int  bkt;

	CVI_TRACE_RGN(RGN_INFO, "_rgn_hash_find find handle(%d), del(%d)\n", handle, del);
	hash_for_each(rgn_hash, bkt, obj, node) {
		CVI_TRACE_RGN(RGN_INFO, "rgn_hash: handle(%d), created:(%d), region.type:(%d)\n"
			, obj->handle, obj->created, obj->region.type);
	}
#endif

	mutex_lock(&g_rgnhashlock);
	// hash_for_each_possible(rgn_hash, obj, node, handle) {
	hash_for_each_safe(rgn_hash, bkt, tmp, obj, node) {
		if (obj->handle == handle) {
			if (del)
				hash_del(&obj->node);
			is_found = true;
			break;
		}
	}

	if (is_found)
		*ctx = obj;

	mutex_unlock(&g_rgnhashlock);
	return is_found;
}

static int check_rgn_handle(struct rgn_ctx **ctx, rgn_handle handle)
{
	if (!_rgn_hash_find(handle, ctx, false)) {
		CVI_TRACE_RGN(RGN_ERR, "rgn_handle(%d) is non-existent.\n", handle);
		return ERR_RGN_UNEXIST;
	}

	return 0;
}

static int _rgn_call_cb(unsigned int m_id, unsigned int cmd_id, void *data)
{
	struct base_exe_m_cb exe_cb;

	exe_cb.callee = m_id;
	exe_cb.caller = E_MODULE_RGN;
	exe_cb.cmd_id = cmd_id;
	exe_cb.data   = (void *)data;

	return base_exe_module_cb(&exe_cb);
}

int32_t _rgn_init(void)
{
	int i = 0;

	hash_init(rgn_hash);
	memset(rgn_prc_ctx, 0, sizeof(rgn_prc_ctx));
	rgn_num = 0;
	mutex_init(&hdlslock);
	mutex_init(&g_rgnlock);
	mutex_init(&g_rgnhashlock);
	for (i = 0; i < RGN_MAX_NUM; ++i)
		mutex_init(&rgn_prc_ctx[i].rgn_canvas_q_lock);
	CVI_TRACE_RGN(RGN_INFO, "-\n");
	return 0;
}

int32_t _rgn_exit(void)
{
	unsigned int bkt;
	struct rgn_ctx *obj;
	struct hlist_node *tmp;
	int i = 0;

	hash_for_each_safe(rgn_hash, bkt, tmp, obj, node) {
		rgn_detach_from_chn(obj->handle, &obj->chn);
		rgn_destory(obj->handle);
	}
	memset(rgn_prc_ctx, 0, sizeof(rgn_prc_ctx));
	rgn_num = 0;
	mutex_destroy(&hdlslock);
	mutex_destroy(&g_rgnlock);
	mutex_destroy(&g_rgnhashlock);
	for (i = 0; i < RGN_MAX_NUM; ++i)
		mutex_destroy(&rgn_prc_ctx[i].rgn_canvas_q_lock);
	CVI_TRACE_RGN(RGN_INFO, "-\n");
	return 0;
}

unsigned int _rgn_proc_get_idx(rgn_handle hhandle)
{
	unsigned int i, idx = 0;

	for (i = 0; i < RGN_MAX_NUM; ++i) {
		if (rgn_prc_ctx[i].handle == hhandle && rgn_prc_ctx[i].created) {
			idx = i;
			break;
		}
	}

	if (i == RGN_MAX_NUM)
		CVI_TRACE_RGN(RGN_ERR, "cannot find corresponding rgn in rgn_prc_ctx, handle: %d\n", hhandle);

	return idx;
}

static inline int _rgn_get_bytesperline(pixel_format_e pixel_format, unsigned int width, unsigned int *bytesperline)
{
	switch (pixel_format) {
	case PIXEL_FORMAT_ARGB_8888:
		*bytesperline = width << 2;
		break;
	case PIXEL_FORMAT_ARGB_4444:
	case PIXEL_FORMAT_ARGB_1555:
		*bytesperline = width << 1;
		break;
	case PIXEL_FORMAT_8BIT_MODE:
		*bytesperline = width;
		break;
	default:
		CVI_TRACE_RGN(RGN_ERR, "not supported pxl-fmt(%d).\n", pixel_format);
		return ERR_RGN_ILLEGAL_PARAM;
	}
	return 0;
}

bool is_rect_overlap(rect_s *r0, rect_s *r1)
{
	// If one rectangle is on left side of other
	if (((u32)r0->x >= (r1->x + r1->width)) || ((u32)r1->x >= (r0->x + r0->width)))
		return false;

	// If one rectangle is above other
	if (((u32)r0->y >= (r1->y + r1->height)) || ((u32)r1->y >= (r0->y + r0->height)))
		return false;

	return true;
}

static int _rgn_get_rect(struct rgn_ctx *ctx, rect_s *rect)
{
	if (ctx->region.type == OVERLAY_RGN) {
		rect->x = ctx->chn_attr.unchn_attr.overlay_chn.point.x;
		rect->y = ctx->chn_attr.unchn_attr.overlay_chn.point.y;
		rect->width = ctx->region.unattr.overlay.size.width;
		rect->height = ctx->region.unattr.overlay.size.height;
		return 0;
	}
	if (ctx->region.type == OVERLAYEX_RGN) {
		rect->x = ctx->chn_attr.unchn_attr.overlay_ex_chn.point.x;
		rect->y = ctx->chn_attr.unchn_attr.overlay_ex_chn.point.y;
		rect->width = ctx->region.unattr.overlay_ex.size.width;
		rect->height = ctx->region.unattr.overlay_ex.size.height;
		return 0;
	}
	if (ctx->region.type == COVER_RGN) {
		*rect = ctx->chn_attr.unchn_attr.cover_chn.rect;
		return 0;
	}
	if (ctx->region.type == COVEREX_RGN) {
		*rect = ctx->chn_attr.unchn_attr.cover_ex_chn.rect;
		return 0;
	}
	if (ctx->region.type == MOSAIC_RGN) {
		*rect = ctx->chn_attr.unchn_attr.mosaic_chn.rect;
		return 0;
	}
	return -1;
}

static int _rgn_get_layer(struct rgn_ctx *ctx, unsigned int *layer)
{
	if (ctx->region.type == OVERLAY_RGN)
		*layer = ctx->chn_attr.unchn_attr.overlay_chn.layer;
	else if (ctx->region.type == OVERLAYEX_RGN)
		*layer = ctx->chn_attr.unchn_attr.overlay_ex_chn.layer;
	else if (ctx->region.type == COVER_RGN)
		*layer = ctx->chn_attr.unchn_attr.cover_chn.layer;
	else if (ctx->region.type == COVEREX_RGN)
		*layer = ctx->chn_attr.unchn_attr.cover_ex_chn.layer;
	else if (ctx->region.type == MOSAIC_RGN)
		*layer = ctx->chn_attr.unchn_attr.mosaic_chn.layer;
	else
		return -1;

	return 0;
}


/* _rgn_check_order: check if r1's order should before r0.
 *
 */
unsigned char _rgn_check_order(rect_s *r0, rect_s *r1)
{
	if (r0->x > r1->x)
		return 1;
	if ((r0->x == r1->x) && (r0->y > r1->y))
		return 1;
	return 0;
}

int _rgn_insert(rgn_handle hdls[], unsigned char size, rgn_handle hdl)
{
	struct rgn_ctx *ctx = NULL;
	rect_s rect_old, rect_new;
	unsigned char i, j;
	int ret;

	ret = check_rgn_handle(&ctx, hdl);
	if (ret != 0)
		return ret;

	if (ctx->canvas_info[ctx->canvas_idx].compressed
		&& hdls[0] != RGN_INVALID_HANDLE) {
		CVI_TRACE_RGN(RGN_ERR, "cannot insert hdl(%d), only allow one ow when odec on\n",
			hdl);
		return ERR_RGN_NOT_PERM;
	}
	if (_rgn_get_rect(ctx, &rect_new) != 0) {
		CVI_TRACE_RGN(RGN_ERR, "rgn_handle(%d) can't get rect.\n", hdl);
		return -1;
	}
	if (hdls[size - 1] != RGN_INVALID_HANDLE) {
		CVI_TRACE_RGN(RGN_ERR, "rgn_handle(%d) rgn's count is full.\n", hdl);
		return -1;
	}

	//Check if previously attached rgns overlap new rgn before inserting new rgn
	for (i = 0; i < size; ++i) {
		if (hdls[i] == RGN_INVALID_HANDLE)
			break;

		ret = check_rgn_handle(&ctx, hdls[i]);
		if (ret != 0)
			return ret;
		if (_rgn_get_rect(ctx, &rect_old) != 0) {
			CVI_TRACE_RGN(RGN_ERR, "rgn_handle(%d) can't get rect.\n", hdls[i]);
			return -1;
		}

		if (is_rect_overlap(&rect_old, &rect_new)) {
			CVI_TRACE_RGN(RGN_ERR, "rgn_handle(%d) is overlapped on another rgn_handle(%d).\n"
				, hdl, hdls[i]);
			CVI_TRACE_RGN(RGN_DEBUG, "(%d %d %d %d) <-> (%d %d %d %d)\n"
				, rect_new.x, rect_new.y, rect_new.width, rect_new.height
				, rect_old.x, rect_old.y, rect_old.width, rect_old.height);
			return ERR_RGN_NOT_PERM;
		}
	}

	for (i = 0; i < size; ++i) {
		if (hdls[i] == RGN_INVALID_HANDLE) {
			hdls[i] = hdl;
			CVI_TRACE_RGN(RGN_DEBUG, "rgn_handle(%d) at index(%d).\n", hdl, i);
			return 0;
		}

		ret = check_rgn_handle(&ctx, hdls[i]);
		if (ret != 0)
			return ret;
		if (_rgn_get_rect(ctx, &rect_old) != 0) {
			CVI_TRACE_RGN(RGN_ERR, "rgn_handle(%d) can't get rect.\n", hdls[i]);
			return -1;
		}

		if (_rgn_check_order(&rect_old, &rect_new)) {
			if ((i + 1) < size)
				for (j = size - 1; j > i; j--)
					hdls[j] = hdls[j - 1];
			hdls[i] = hdl;
			CVI_TRACE_RGN(RGN_DEBUG, "rgn_handle(%d) at index(%d).\n", hdl, i);
			return 0;
		}
	}
	return -1;
}

int _rgn_ex_insert(rgn_handle hdls[], unsigned char size, rgn_handle hdl)
{
	struct rgn_ctx *ctx = NULL;
	unsigned int layer_new, layer_old;
	unsigned char i, j;
	int ret;

	ret = check_rgn_handle(&ctx, hdl);
	if (ret != 0)
		return ret;

	if (hdls[size - 1] != RGN_INVALID_HANDLE) {
		CVI_TRACE_RGN(RGN_ERR, "rgn_handle(%d) rgn_ex's count is full.\n", hdl);
		return -1;
	}

	if (_rgn_get_layer(ctx, &layer_new) != 0) {
		CVI_TRACE_RGN(RGN_ERR, "rgn_handle(%d) can't get layer.\n", hdl);
		return -1;
	}

	for (i = 0; i < size; ++i) {
		if (hdls[i] == RGN_INVALID_HANDLE) {
			hdls[i] = hdl;
			CVI_TRACE_RGN(RGN_DEBUG, "rgn_handle(%d) at index(%d).\n", hdl, i);
			return 0;
		}

		ret = check_rgn_handle(&ctx, hdls[i]);
		if (ret != 0)
			return ret;
		if (_rgn_get_layer(ctx, &layer_old) != 0) {
			CVI_TRACE_RGN(RGN_ERR, "rgn_handle(%d) can't get layer.\n", hdls[i]);
			return -1;
		}

		if (layer_new < layer_old) {
			if ((i + 1) < size)
				for (j = size - 1; j > i; j--)
					hdls[j] = hdls[j - 1];
			hdls[i] = hdl;
			CVI_TRACE_RGN(RGN_DEBUG, "rgn_handle(%d) at index(%d).\n", hdl, i);
			return 0;
		}
	}
	return -1;
}

int _rgn_remove(rgn_handle hdls[], unsigned char size, rgn_handle hdl)
{
	unsigned char i;

	for (i = 0; i < size; ++i) {
		if (hdls[i] == RGN_INVALID_HANDLE)
			return ERR_RGN_UNEXIST;

		if (hdl == hdls[i]) {
			if ((i + 1) < size)
				memmove(&hdls[i], &hdls[i + 1], sizeof(hdls[i]) * (size - 1 - i));
			hdls[size - 1] = RGN_INVALID_HANDLE;
			CVI_TRACE_RGN(RGN_DEBUG, "rgn_handle(%d) at index(%d).\n", hdl, i);
			break;
		}
	}
	return 0;
}

/* _rgn_fill_pattern: fill buffer of length with pattern.
 *
 * @buf: buffer address
 * @len: buffer length
 * @color: color pattern. It depends on the pixel-format.
 * @bpp: bytes-per-pixel. Only 2 or 4 works.
 */
void _rgn_fill_pattern(void *buf, unsigned int len, unsigned int color, unsigned char bpp)
{
	unsigned int *p = buf;
	unsigned int i;
	unsigned int pattern = color;

	if (bpp == 2)
		pattern |= (pattern << 16);
	for (i = 0; i < len / sizeof(u32); ++i)
		p[i] = pattern;

	if (len & 0x02)
		*(((unsigned short *)&p[i]) + 1) = color;
}

/* _rgn_update_cover_canvas: fill cover/coverex if needed.
 *
 * @ctx: the region ctx to be updated.
 * @pchn_attr: could be NULL. If not, will be used to check if update needed.
 */
static int _rgn_update_cover_canvas(struct rgn_ctx *ctx, const rgn_chn_attr_s *pchn_attr)
{
	pixel_format_e pixel_format = PIXEL_FORMAT_ARGB_1555;
	unsigned int bytesperline;
	rect_s rect;
	unsigned int buf_len;
	unsigned int stride;
	unsigned char bfill = 0;
	unsigned int proc_idx;

	if (!ctx || !pchn_attr)
		return ERR_RGN_NULL_PTR;

	rect = (pchn_attr->type == COVER_RGN)
	     ? pchn_attr->unchn_attr.cover_chn.rect
	     : pchn_attr->unchn_attr.cover_ex_chn.rect;

	// check if color changed.
	bfill = (((ctx->chn_attr.type == COVER_RGN)
		&& (ctx->chn_attr.unchn_attr.cover_chn.color
		 != pchn_attr->unchn_attr.cover_chn.color))) ||
		(((ctx->chn_attr.type == COVEREX_RGN)
		&& (ctx->chn_attr.unchn_attr.cover_ex_chn.color
		 != pchn_attr->unchn_attr.cover_ex_chn.color)));

	_rgn_get_bytesperline(pixel_format, rect.width, &bytesperline);
	stride = ALIGN(bytesperline, 32);
	buf_len = stride * rect.height;

	proc_idx = _rgn_proc_get_idx(ctx->handle);
	if (ctx->ion_len == 0) {
		ctx->canvas_idx = rgn_prc_ctx[proc_idx].canvas_idx = 0;
		rgn_prc_ctx[proc_idx].canvas_info[0] = *ctx->canvas_info;
	}

	// update canvas
	if (buf_len > ctx->ion_len) {
		if (ctx->ion_len != 0)
			base_ion_free(ctx->canvas_info[0].phy_addr);

		ctx->canvas_info[0].pixel_format = pixel_format;
		ctx->canvas_info[0].size.width = rect.width;
		ctx->canvas_info[0].size.height = rect.height;
		ctx->canvas_info[0].stride = stride;
		ctx->ion_len = ctx->max_need_ion = buf_len;
		if (base_ion_alloc(&ctx->canvas_info[0].phy_addr, (void **)&ctx->canvas_info[0].virt_addr,
						  "rgn_canvas", ctx->ion_len, false) != 0) {
			CVI_TRACE_RGN(RGN_ERR, "Region(%d) Can't acquire ion for Canvas.\n", ctx->handle);
			return ERR_RGN_NOBUF;
		}
		bfill = 1;
	} else if (buf_len <= ctx->ion_len) {
		ctx->canvas_info[0].size.width = rect.width;
		ctx->canvas_info[0].size.height = rect.height;
		ctx->canvas_info[0].stride = stride;
	}
	rgn_prc_ctx[proc_idx].canvas_info[0] = ctx->canvas_info[0];

	// fill color pattern to cover/coverex.
	if (bfill) {
		unsigned short color;

		color = (ctx->chn_attr.type == COVEREX_RGN)
		      ? RGB888_2_ARGB1555(pchn_attr->unchn_attr.cover_ex_chn.color)
		      : RGB888_2_ARGB1555(pchn_attr->unchn_attr.cover_chn.color);
		_rgn_fill_pattern(ctx->canvas_info[0].virt_addr, ctx->ion_len, color, 2);
	}

	return 0;
}

static int _rgn_update_hw_cfg_to_module(rgn_handle *hdls, void *cfg, mmf_chn_s*pchn,
			rgn_type_e type, unsigned char cmpr)
{
	struct _rgn_hdls_cb_param rgn_hdls_arg;

	rgn_hdls_arg.chn = *pchn;
	rgn_hdls_arg.hdls = hdls;
	rgn_hdls_arg.type = type;
	rgn_hdls_arg.layer = 0;

	//Set VO module hdls and rgn_cfg
	if (pchn->mod_id == ID_VO) {
		if (_rgn_call_cb(E_MODULE_VO, VO_CB_SET_RGN_HDLS, &rgn_hdls_arg) != 0) {
			CVI_TRACE_RGN(RGN_ERR, "VO_CB_SET_RGN_HDLS is failed\n");
			return ERR_RGN_ILLEGAL_PARAM;
		}

		if (type == OVERLAY_RGN || type == COVER_RGN) {
			struct _rgn_cfg_cb_param rgn_cfg_arg;

			rgn_cfg_arg.chn = *pchn;
			rgn_cfg_arg.rgn_cfg = *(struct rgn_cfg *)cfg;
			if (_rgn_call_cb(E_MODULE_VO, VO_CB_SET_RGN_CFG, &rgn_cfg_arg) != 0) {
				CVI_TRACE_RGN(RGN_ERR, "VO_CB_SET_RGN_CFG is failed\n");
				return ERR_RGN_ILLEGAL_PARAM;
			}
		} else {
			CVI_TRACE_RGN(RGN_ERR, "VO unsupported type(%d)\n", type);
			return ERR_RGN_ILLEGAL_PARAM;
		}
	} else if (pchn->mod_id == ID_VPSS) {
		unsigned int osd_layer = cmpr ? RGN_ODEC_LAYER_VPSS : RGN_NORMAL_LAYER_VPSS;

		if (type == OVERLAY_RGN || type == COVER_RGN)
			rgn_hdls_arg.layer = osd_layer;

		//Set VPSS module rgn hdls
		if (_rgn_call_cb(E_MODULE_VPSS, VPSS_CB_SET_RGN_HDLS, &rgn_hdls_arg) != 0) {
			CVI_TRACE_RGN(RGN_ERR, "VPSS_CB_SET_RGN_HDLS is failed\n");
			return ERR_RGN_ILLEGAL_PARAM;
		}

		//Set VPSS module rgn_cfg
		if (type == OVERLAY_RGN || type == COVER_RGN) {
			struct _rgn_cfg_cb_param rgn_cfg_arg;

			rgn_cfg_arg.chn = *pchn;
			rgn_cfg_arg.rgn_cfg = *(struct rgn_cfg *)cfg;
			rgn_cfg_arg.layer = osd_layer;
			if (_rgn_call_cb(E_MODULE_VPSS, VPSS_CB_SET_RGN_CFG, &rgn_cfg_arg) != 0) {
				CVI_TRACE_RGN(RGN_ERR, "VPSS_CB_SET_RGN_CFG is failed\n");
				return ERR_RGN_ILLEGAL_PARAM;
			}
		} else if (type == COVEREX_RGN) {
			//Set VPSS module cover_ex_cfg
			struct _rgn_coverex_cfg_cb_param rgn_coverex_cfg_arg;

			rgn_coverex_cfg_arg.chn = *pchn;
			rgn_coverex_cfg_arg.rgn_coverex_cfg = *(struct rgn_coverex_cfg *)cfg;
			if (_rgn_call_cb(E_MODULE_VPSS, VPSS_CB_SET_COVEREX_CFG, &rgn_coverex_cfg_arg) != 0) {
				CVI_TRACE_RGN(RGN_ERR, "VPSS_CB_SET_COVEREX_CFG is failed\n");
				return ERR_RGN_ILLEGAL_PARAM;
			}
		} else if (type == OVERLAYEX_RGN) {
			//Set VPSS module rgn_ex_cfg
			struct _rgn_ex_cfg_cb_param rgnex_cfg_arg;

			rgnex_cfg_arg.chn = *pchn;
			rgnex_cfg_arg.rgn_ex_cfg = *(struct rgn_ex_cfg *)cfg;
			if (_rgn_call_cb(E_MODULE_VPSS, VPSS_CB_SET_RGNEX_CFG, &rgnex_cfg_arg) != 0) {
				CVI_TRACE_RGN(RGN_ERR, "VPSS_CB_SET_RGNEX_CFG is failed\n");
				return ERR_RGN_ILLEGAL_PARAM;
			}
		} else if (type == MOSAIC_RGN) {
			struct _rgn_mosaic_cfg_cb_param rgn_mosaic_cfg_arg;

			rgn_mosaic_cfg_arg.chn = *pchn;
			rgn_mosaic_cfg_arg.rgn_mosaic_cfg = *(struct rgn_mosaic_cfg *)cfg;
			if (_rgn_call_cb(E_MODULE_VPSS, VPSS_CB_SET_MOSAIC_CFG, &rgn_mosaic_cfg_arg) != 0) {
				CVI_TRACE_RGN(RGN_ERR, "VPSS_CB_SET_MOSAIC_CFG is failed\n");
				return ERR_RGN_ILLEGAL_PARAM;
			}
		}
	} else {
		CVI_TRACE_RGN(RGN_ERR, "Unsupported mod_id(%d)\n", pchn->mod_id);
		return ERR_RGN_ILLEGAL_PARAM;
	}

	return 0;
}

/* _rgn_set_hw_cfg: update cfg based on the rgn attached.
 *
 * @hdls: rgn handles on chn.
 * @size: size of gop acceptable.
 * @pchn: attached chn.
 * @type: rgn type.
 * @cmpr: rgn compress enable.
 */
static int _rgn_set_hw_cfg(rgn_handle hdls[], unsigned char size, mmf_chn_s*pchn, rgn_type_e type, unsigned char cmpr)
{
	struct rgn_ctx *ctx = NULL;
	rect_s rect;
	unsigned char rgn_idx = 0;
	unsigned char i;
	int ret;
	struct rgn_cfg cfg = {0};

	for (i = 0; i < size; ++i) {
		if (hdls[i] == RGN_INVALID_HANDLE)
			break;

		ret = check_rgn_handle(&ctx, hdls[i]);
		if (ret != 0)
			return ret;

		if (!ctx->chn_attr.show)
			continue;
		if (_rgn_get_rect(ctx, &rect) != 0)
			continue;

		switch (ctx->canvas_info[ctx->canvas_idx].pixel_format) {
		case PIXEL_FORMAT_ARGB_8888:
			cfg.param[rgn_idx].fmt = RGN_FMT_ARGB8888;
			break;
		case PIXEL_FORMAT_ARGB_4444:
			cfg.param[rgn_idx].fmt = RGN_FMT_ARGB4444;
			break;
		case PIXEL_FORMAT_8BIT_MODE:
			cfg.param[rgn_idx].fmt = RGN_FMT_256LUT;
			break;
		case PIXEL_FORMAT_ARGB_1555:
		default:
			cfg.param[rgn_idx].fmt = RGN_FMT_ARGB1555;
			break;
		}
		cfg.param[rgn_idx].rect.left	= rect.x;
		cfg.param[rgn_idx].rect.top	= rect.y;
		cfg.param[rgn_idx].rect.width	= ctx->canvas_info[ctx->canvas_idx].size.width;
		cfg.param[rgn_idx].rect.height	= ctx->canvas_info[ctx->canvas_idx].size.height;
		cfg.param[rgn_idx].phy_addr	= ctx->canvas_info[ctx->canvas_idx].phy_addr;
		cfg.param[rgn_idx].stride	= ctx->canvas_info[ctx->canvas_idx].stride;

		//only one OSD window can be attached.
		if (ctx->canvas_info[ctx->canvas_idx].compressed) {
			// this function is first called by rgn_attach_to_chn, but cmpr data is not valid.
			// so, do not enable odec at first time to void garbage
			if (ctx->odec_data_valid) {
				cfg.odec.enable	= true;
			} else {
				cfg.odec.enable	= false;
			}
			cfg.odec.attached_ow	= rgn_idx;
			cfg.odec.bso_sz	= ctx->canvas_info[ctx->canvas_idx].compressed_size;
			cfg.odec.canvas_mutex_lock = (u64)&ctx->rgn_canvas_q_lock;
			cfg.odec.rgn_canvas_waitq = (u64)&ctx->rgn_canvas_waitq;
			cfg.odec.rgn_canvas_doneq = (u64)&ctx->rgn_canvas_doneq;
			cfg.odec.canvas_updated = ctx->canvas_updated;
			break;
		} else
			cfg.odec.enable	= false;
		++rgn_idx;
	}
	cfg.num_of_rgn = (cfg.odec.enable) ? 1 : rgn_idx;

	cfg.hscale_x2 = false;
	cfg.vscale_x2 = false;
	cfg.colorkey_en = false;

	ret = _rgn_update_hw_cfg_to_module(hdls, (void *)&cfg, pchn, type, cmpr);
	if (ret != 0) {
		CVI_TRACE_RGN(RGN_ERR, "_rgn_update_hw_cfg_to_module failed\n");
	}

	return ret;
}

/* _rgn_ex_set_hw_cfg: update cfg based on the rgn_ex attached.
 *
 * @hdls: rgn_ex handles on chn.
 * @size: max num of rgn.
 * @pchn: attached chn.
 * @type: rgn type.
 */
static int _rgn_ex_set_hw_cfg(rgn_handle hdls[], unsigned char size, mmf_chn_s*pchn, rgn_type_e type)
{
	struct rgn_ctx *ctx = NULL;
	rect_s rect;
	unsigned char rgn_idx = 0;
	unsigned char rgn_tile = 0; // 1: left tile, 2: right tile
	unsigned int left_w = 0, bpp = 0;
	unsigned char i;
	int ret;
	struct rgn_ex_cfg ex_cfg = {0};

	for (i = 0; i < size; ++i) {
		if (hdls[i] == RGN_INVALID_HANDLE)
			break;

		ret = check_rgn_handle(&ctx, hdls[i]);
		if (ret != 0)
			return ret;
		if (!ctx->chn_attr.show)
			continue;
		if (_rgn_get_rect(ctx, &rect) != 0)
			continue;

		if (rect.width > RGN_EX_MAX_WIDTH) {
			unsigned int byte_offset;

			left_w = rect.width / 2;
			bpp = (ctx->canvas_info[ctx->canvas_idx].pixel_format == PIXEL_FORMAT_ARGB_8888)
				? 4 : 2;

			byte_offset = (bpp * left_w) & (0x20 - 1);
			if (byte_offset) {
				left_w -= (byte_offset / bpp);
			}
			rgn_tile = 1;
		}

		do {
			switch (ctx->canvas_info[ctx->canvas_idx].pixel_format) {
			case PIXEL_FORMAT_ARGB_8888:
				ex_cfg.rgn_ex_param[rgn_idx].fmt = RGN_FMT_ARGB8888;
				break;
			case PIXEL_FORMAT_ARGB_4444:
				ex_cfg.rgn_ex_param[rgn_idx].fmt = RGN_FMT_ARGB4444;
				break;
			case PIXEL_FORMAT_ARGB_1555:
			default:
				ex_cfg.rgn_ex_param[rgn_idx].fmt = RGN_FMT_ARGB1555;
				break;
			}
			ex_cfg.rgn_ex_param[rgn_idx].stride = ctx->canvas_info[ctx->canvas_idx].stride;

			if (rgn_tile == 1) { // left tile
				ex_cfg.rgn_ex_param[rgn_idx].phy_addr = ctx->canvas_info[ctx->canvas_idx].phy_addr;
				ex_cfg.rgn_ex_param[rgn_idx].rect.left = rect.x;
				ex_cfg.rgn_ex_param[rgn_idx].rect.top = rect.y;
				ex_cfg.rgn_ex_param[rgn_idx].rect.width = left_w;
				ex_cfg.rgn_ex_param[rgn_idx].rect.height
						= ctx->canvas_info[ctx->canvas_idx].size.height;
				rgn_tile = 2;
			} else if (rgn_tile == 2) { // right tile
				ex_cfg.rgn_ex_param[rgn_idx].phy_addr
					= ctx->canvas_info[ctx->canvas_idx].phy_addr + (left_w * bpp);
				ex_cfg.rgn_ex_param[rgn_idx].rect.left = rect.x + left_w;
				ex_cfg.rgn_ex_param[rgn_idx].rect.top = rect.y;
				ex_cfg.rgn_ex_param[rgn_idx].rect.width
						= ctx->canvas_info[ctx->canvas_idx].size.width - left_w;
				ex_cfg.rgn_ex_param[rgn_idx].rect.height
						= ctx->canvas_info[ctx->canvas_idx].size.height;
				rgn_tile = 0;
			} else { // no tile
				ex_cfg.rgn_ex_param[rgn_idx].phy_addr = ctx->canvas_info[ctx->canvas_idx].phy_addr;
				ex_cfg.rgn_ex_param[rgn_idx].rect.left = rect.x;
				ex_cfg.rgn_ex_param[rgn_idx].rect.top = rect.y;
				ex_cfg.rgn_ex_param[rgn_idx].rect.width
						= ctx->canvas_info[ctx->canvas_idx].size.width;
				ex_cfg.rgn_ex_param[rgn_idx].rect.height
						= ctx->canvas_info[ctx->canvas_idx].size.height;
				++rgn_idx;
				break;
			}
			++rgn_idx;
		} while (rgn_tile);
	}
	ex_cfg.num_of_rgn_ex = rgn_idx;

	ex_cfg.hscale_x2 = false;
	ex_cfg.vscale_x2 = false;
	ex_cfg.colorkey_en = false;

	ret = _rgn_update_hw_cfg_to_module(hdls, (void *)&ex_cfg, pchn, type, false);
	if (ret != 0) {
		CVI_TRACE_RGN(RGN_ERR, "_rgn_update_hw_cfg_to_module failed\n");
	}

	return ret;
}

static int _rgn_coverex_set_hw_cfg(rgn_handle hdls[], unsigned char size, mmf_chn_s*pchn, rgn_type_e type)
{
	struct rgn_ctx *ctx = NULL;
	rect_s rect;
	unsigned char rgn_idx = 0;
	unsigned char i;
	int ret;
	struct rgn_coverex_cfg coverex_cfg = {0};

	for (i = 0; i < size; ++i) {
		if (hdls[i] == RGN_INVALID_HANDLE)
			break;

		ret = check_rgn_handle(&ctx, hdls[i]);
		if (ret != 0)
			return ret;

		if (!ctx->chn_attr.show)
			continue;
		if (_rgn_get_rect(ctx, &rect) != 0)
			continue;

		coverex_cfg.rgn_coverex_param[rgn_idx].rect.left = rect.x;
		coverex_cfg.rgn_coverex_param[rgn_idx].rect.top = rect.y;
		coverex_cfg.rgn_coverex_param[rgn_idx].rect.width = rect.width;
		coverex_cfg.rgn_coverex_param[rgn_idx].rect.height = rect.height;
		coverex_cfg.rgn_coverex_param[rgn_idx].enable = 1;
		coverex_cfg.rgn_coverex_param[rgn_idx].color = ctx->chn_attr.unchn_attr.cover_ex_chn.color;
		++rgn_idx;
	}

	ret = _rgn_update_hw_cfg_to_module(hdls, (void *)&coverex_cfg, pchn, type, false);
	if (ret != 0) {
		CVI_TRACE_RGN(RGN_ERR, "_rgn_update_hw_cfg_to_module failed\n");
	}

	return ret;
}

static int _rgn_mosaic_set_hw_cfg(rgn_handle hdls[], unsigned char size, mmf_chn_s*pchn, rgn_type_e type)
{
	int ret;
	struct rgn_ctx *ctx = NULL;
	rect_s rect[RGN_MOSAIC_MAX_NUM];
	unsigned char rgn_idx = 0;
	unsigned char i, j, k, tmp;
	mosaic_blk_size_e blk_size = MOSAIC_BLK_SIZE_16;
	unsigned char grid_size = 16, grid_offset;
	unsigned int buf_size;
	int start_x = 0xffff, end_x = 0, start_y = 0xffff, end_y = 0;
	unsigned short stride, grid_num_w, grid_num_h, x_offset, y_offset, index;
	u64 phy_addr;
	void *virt_addr;
	unsigned char *pdata;
	struct rgn_mem_info *ion_buf = &mosaic_buf[pchn->dev_id][pchn->chn_id];
	struct rgn_mosaic_cfg mosaic_cfg = {0};

	for (i = 0; i < size; ++i) {
		if (hdls[i] == RGN_INVALID_HANDLE)
			break;
		ret = check_rgn_handle(&ctx, hdls[i]);
		if (ret != 0)
			return ret;
		if (!ctx->chn_attr.show)
			continue;
		if (_rgn_get_rect(ctx, &rect[rgn_idx]) != 0)
			continue;
		if (ctx->chn_attr.unchn_attr.mosaic_chn.blk_size == MOSAIC_BLK_SIZE_8) {
			blk_size = MOSAIC_BLK_SIZE_8;
			grid_size = 8;
		}
		start_x = MIN(start_x, rect[rgn_idx].x);
		start_y = MIN(start_y, rect[rgn_idx].y);
		rgn_idx++;
	}

	if (rgn_idx == 0) {
		if (ion_buf->buf_len != 0) {
			base_ion_free(ion_buf->phy_addr);
			ion_buf->phy_addr = 0;
			ion_buf->buf_len = 0;
			ion_buf->vir_addr = NULL;
		}
		goto RGN_UPDATE_HW_CFG;
	}

	//rect align to 8x8 or 16x16
	for (i = 0; i < rgn_idx; ++i) {
		grid_offset = (rect[i].x - start_x) % grid_size;
		rect[i].x -= grid_offset;
		rect[i].width = ALIGN(rect[i].width + grid_offset, grid_size);
		grid_offset = (rect[i].y - start_y) % grid_size;
		rect[i].y -= grid_offset;
		rect[i].height = ALIGN(rect[i].height + grid_offset, grid_size);

		end_x = MAX(end_x, rect[i].x + rect[i].width);
		end_y = MAX(end_y, rect[i].y + rect[i].height);
	}

	grid_num_w = (end_x - start_x) / grid_size;
	grid_num_h = (end_y - start_y) / grid_size;
	stride = ALIGN(grid_num_w, 16); //byte base, must be 16-byte align
	buf_size = stride * grid_num_h;

	if (buf_size > ion_buf->buf_len) {
		if (ion_buf->buf_len != 0) {
			base_ion_free(ion_buf->phy_addr);
			ion_buf->phy_addr = 0;
			ion_buf->buf_len = 0;
			ion_buf->vir_addr = NULL;
		}
		if (base_ion_alloc(&phy_addr, &virt_addr, "mosaic_canvas", buf_size, false) != 0) {
			CVI_TRACE_RGN(RGN_ERR, "Region(%d) Can't acquire ion for Canvas.\n", ctx->handle);
			return ERR_RGN_NOBUF;
		}
		ion_buf->phy_addr = phy_addr;
		ion_buf->buf_len = buf_size;
		ion_buf->vir_addr = virt_addr;
	} else {
		virt_addr = ion_buf->vir_addr;
		phy_addr = ion_buf->phy_addr;
		buf_size = ion_buf->buf_len;
	}

	memset(virt_addr, 0, buf_size);
	pdata = (unsigned char *)virt_addr;
	for (i = 0; i < rgn_idx; ++i) {
		y_offset = (rect[i].y - start_y) / grid_size * stride;
		for (j = 0; j < (rect[i].height / grid_size); ++j) {
			for (k = 0; k < (rect[i].width / grid_size); ++k) {
				x_offset = (rect[i].x - start_x) / grid_size;
				index = y_offset + j * stride + x_offset + k;
				tmp = (unsigned char)get_random_u32();
				pdata[index] = tmp ? tmp : 1;
			}
		}
	}

	CVI_TRACE_RGN(RGN_DEBUG, "mosaic num(%d),rect(%d %d %d %d), phy_addr:0x%llx buf_len:%d.\n",
		rgn_idx, start_x, start_y, end_x, end_y, phy_addr, buf_size);

	mosaic_cfg.start_x = start_x;
	mosaic_cfg.start_y = start_y;
	mosaic_cfg.end_x = end_x;
	mosaic_cfg.end_y = end_y;
	mosaic_cfg.enable = 1;
	mosaic_cfg.blk_size = blk_size;
	mosaic_cfg.phy_addr = phy_addr;

RGN_UPDATE_HW_CFG:
	ret = _rgn_update_hw_cfg_to_module(hdls, (void *)&mosaic_cfg, pchn, type, false);
	if (ret != 0) {
		CVI_TRACE_RGN(RGN_ERR, "_rgn_update_hw_cfg_to_module failed\n");
	}

	return ret;
}


/* _rgn_get_chn_info: get info of chn for rgn.
 *
 * @pchn: the chn want to check.
 * @numOfLayer: number of gop layer supported on the chn.
 * @size: number of ow supported on the chn.
 */
static int _rgn_get_chn_info(struct rgn_ctx *ctx,
	rgn_handle *hdls, unsigned char *size)
{
	struct _rgn_hdls_cb_param rgn_hdls_arg;

	rgn_hdls_arg.chn = ctx->chn;
	rgn_hdls_arg.hdls = hdls;
	rgn_hdls_arg.type = ctx->region.type;

	if (ctx->chn.mod_id == ID_VO) {
		rgn_hdls_arg.layer = 0; // vo only has one layer gop
		if (_rgn_call_cb(E_MODULE_VO, VO_CB_GET_RGN_HDLS, &rgn_hdls_arg) != 0) {
			CVI_TRACE_RGN(RGN_ERR, "VO_CB_GET_RGN_HDLS is failed\n");
			return ERR_RGN_ILLEGAL_PARAM;
		}
		if (ctx->region.type == OVERLAY_RGN || ctx->region.type == COVER_RGN)
			*size = RGN_MAX_NUM_VO;
		else if (ctx->region.type == COVEREX_RGN)
			*size = RGN_COVEREX_MAX_NUM;
	} else if (ctx->chn.mod_id == ID_VPSS) {
		if (ctx->region.type == OVERLAY_RGN || ctx->region.type == COVER_RGN) {
			rgn_hdls_arg.layer = (ctx->canvas_info[ctx->canvas_idx].compressed) ?
				RGN_ODEC_LAYER_VPSS : RGN_NORMAL_LAYER_VPSS;
			*size = RGN_MAX_NUM_VPSS;
		} else if (ctx->region.type == COVEREX_RGN) {
			rgn_hdls_arg.layer = 0; //invalid
			*size = RGN_COVEREX_MAX_NUM;
		} else if (ctx->region.type == OVERLAYEX_RGN) {
			rgn_hdls_arg.layer = 0; //invalid
			*size = RGN_EX_MAX_NUM_VPSS;
		} else if (ctx->region.type == MOSAIC_RGN) {
			rgn_hdls_arg.layer = 0; //invalid
			*size = RGN_MOSAIC_MAX_NUM;
		}

		if (_rgn_call_cb(E_MODULE_VPSS, VPSS_CB_GET_RGN_HDLS, &rgn_hdls_arg) != 0) {
			CVI_TRACE_RGN(RGN_ERR, "VPSS_CB_GET_RGN_HDLS is failed\n");
			return ERR_RGN_ILLEGAL_PARAM;
		}
	} else {
		return ERR_RGN_INVALID_DEVID;
	}

	return 0;
}

static int _rgn_update_hw(struct rgn_ctx *ctx, enum rgn_op op)
{
	unsigned char size;
	int ret;
	rgn_handle hdls[RGN_EX_MAX_NUM_VPSS];

	mutex_lock(&hdlslock);
	ret = _rgn_get_chn_info(ctx, hdls, &size);
	if (ret != 0) {
		mutex_unlock(&hdlslock);
		return ret;
	}

	if (op == RGN_OP_INSERT) {
		if (ctx->region.type == OVERLAY_RGN || ctx->region.type == COVER_RGN) {
			ret = _rgn_insert(hdls, size, ctx->handle);
			if (ret != 0) {
				mutex_unlock(&hdlslock);
				return ret;
			}
		} else {
			ret = _rgn_ex_insert(hdls, size, ctx->handle);
			if (ret != 0) {
				mutex_unlock(&hdlslock);
				return ret;
			}
		}
	} else if (op == RGN_OP_REMOVE) {
		ret = _rgn_remove(hdls, size, ctx->handle);
		if (ret != 0) {
			mutex_unlock(&hdlslock);
			CVI_TRACE_RGN(RGN_ERR, "rgn_handle(%d) not at CHN(%s-%d-%d).\n", ctx->handle
			, sys_get_modname(ctx->chn.mod_id), ctx->chn.dev_id, ctx->chn.chn_id);
			return ret;
		}
	} else if (op == RGN_OP_UPDATE) {
		_rgn_remove(hdls, size, ctx->handle);
		if (ctx->region.type == OVERLAY_RGN || ctx->region.type == COVER_RGN) {
			ret = _rgn_insert(hdls, size, ctx->handle);
			if (ret != 0) {
				mutex_unlock(&hdlslock);
				return ret;
			}
		} else {
			ret = _rgn_ex_insert(hdls, size, ctx->handle);
			if (ret != 0) {
				mutex_unlock(&hdlslock);
				return ret;
			}
		}
	}

	if (ctx->region.type == OVERLAY_RGN || ctx->region.type == COVER_RGN) {
		ret = _rgn_set_hw_cfg(hdls, size, &ctx->chn, ctx->region.type,
					ctx->canvas_info[ctx->canvas_idx].compressed);
		if (ret != 0) {
			CVI_TRACE_RGN(RGN_ERR, "_rgn_set_hw_cfg failed\n");
			mutex_unlock(&hdlslock);
			return ret;
		}
	} else if (ctx->region.type == COVEREX_RGN) {
		ret = _rgn_coverex_set_hw_cfg(hdls, size, &ctx->chn, ctx->region.type);
		if (ret != 0) {
			CVI_TRACE_RGN(RGN_ERR, "_rgn_coverex_set_hw_cfg failed\n");
			mutex_unlock(&hdlslock);
			return ret;
		}
	} else if (ctx->region.type == OVERLAYEX_RGN) {
		ret = _rgn_ex_set_hw_cfg(hdls, size, &ctx->chn, ctx->region.type);
		if (ret != 0) {
			CVI_TRACE_RGN(RGN_ERR, "_rgn_ex_set_hw_cfg failed\n");
			mutex_unlock(&hdlslock);
			return ret;
		}
	} else if (ctx->region.type == MOSAIC_RGN) {
		ret = _rgn_mosaic_set_hw_cfg(hdls, size, &ctx->chn, ctx->region.type);
		if (ret != 0) {
			CVI_TRACE_RGN(RGN_ERR, "_rgn_mosaic_set_hw_cfg failed\n");
			mutex_unlock(&hdlslock);
			return ret;
		}
	} else {
		mutex_unlock(&hdlslock);
		return ERR_RGN_NOT_SUPPORT;
	}

	base_ion_cache_flush(ctx->canvas_info[ctx->canvas_idx].phy_addr,
		ctx->canvas_info[ctx->canvas_idx].virt_addr, ctx->ion_len);

	mutex_unlock(&hdlslock);

	return 0;
}

int _rgn_check_chn_attr(rgn_handle handle, const mmf_chn_s*pchn, const rgn_chn_attr_s *pchn_attr)
{
	rect_s rect_chn, rect_rgn;
	struct rgn_ctx *ctx;

	check_rgn_handle(&ctx, handle);

	if ((pchn_attr->type == OVERLAYEX_RGN || pchn_attr->type == MOSAIC_RGN) &&
			(pchn->mod_id != ID_VPSS)) {
		CVI_TRACE_RGN(RGN_ERR, "overlayex/mosaic only vpss support!\n");
		return ERR_RGN_NOT_SUPPORT;
	}

	if (pchn->mod_id == ID_VO) {
		struct _rgn_chn_size_cb_param rgn_chn_arg;

		rgn_chn_arg.chn = *pchn;

		if (_rgn_call_cb(E_MODULE_VO, VO_CB_GET_CHN_SIZE, &rgn_chn_arg)) {
			CVI_TRACE_RGN(RGN_ERR, "VO_CB_GET_CHN_SIZE failed!\n");
			return ERR_RGN_INVALID_CHNID;
		}
		rect_chn = rgn_chn_arg.rect;
	} else if (pchn->mod_id == ID_VPSS) {
		struct _rgn_chn_size_cb_param rgn_chn_arg;

		rgn_chn_arg.chn = *pchn;

		if (_rgn_call_cb(E_MODULE_VPSS, VPSS_CB_GET_CHN_SIZE, &rgn_chn_arg)) {
			CVI_TRACE_RGN(RGN_ERR, "VPSS_CB_GET_CHN_SIZE failed!\n");
			return ERR_RGN_INVALID_CHNID;
		}
		rect_chn = rgn_chn_arg.rect;
	} else
		return ERR_RGN_ILLEGAL_PARAM;

	if (pchn_attr->type == COVEREX_RGN) {
		if (pchn_attr->unchn_attr.cover_ex_chn.cover_type != AREA_RECT) {
			CVI_TRACE_RGN(RGN_ERR, "AREA_RECT only now.\n");
			return ERR_RGN_ILLEGAL_PARAM;
		}
		rect_rgn = pchn_attr->unchn_attr.cover_ex_chn.rect;
	} else if (pchn_attr->type == COVER_RGN) {
		if (pchn_attr->unchn_attr.cover_chn.cover_type != AREA_RECT) {
			CVI_TRACE_RGN(RGN_ERR, "AREA_RECT only now.\n");
			return ERR_RGN_ILLEGAL_PARAM;
		}
		if (pchn_attr->unchn_attr.cover_chn.coordinate != RGN_ABS_COOR) {
			CVI_TRACE_RGN(RGN_ERR, "abs-coordinate only now.\n");
			return ERR_RGN_ILLEGAL_PARAM;
		}
		rect_rgn = pchn_attr->unchn_attr.cover_chn.rect;
	} else if (pchn_attr->type == OVERLAY_RGN) {
		rect_rgn.x = pchn_attr->unchn_attr.overlay_chn.point.x;
		rect_rgn.y = pchn_attr->unchn_attr.overlay_chn.point.y;
		rect_rgn.width = ctx->region.unattr.overlay.size.width;
		rect_rgn.height = ctx->region.unattr.overlay.size.height;
	} else if (pchn_attr->type == OVERLAYEX_RGN) {
		rect_rgn.x = pchn_attr->unchn_attr.overlay_ex_chn.point.x;
		rect_rgn.y = pchn_attr->unchn_attr.overlay_ex_chn.point.y;
		rect_rgn.width = ctx->region.unattr.overlay_ex.size.width;
		rect_rgn.height = ctx->region.unattr.overlay_ex.size.height;
	} else if (pchn_attr->type == MOSAIC_RGN) {
		if (pchn_attr->unchn_attr.mosaic_chn.blk_size != MOSAIC_BLK_SIZE_8
			&& pchn_attr->unchn_attr.mosaic_chn.blk_size != MOSAIC_BLK_SIZE_16) {
			CVI_TRACE_RGN(RGN_ERR, "Mosaic only support block size 8/16.\n");
			return ERR_RGN_ILLEGAL_PARAM;
		}
		rect_rgn = pchn_attr->unchn_attr.mosaic_chn.rect;
	} else {
		return ERR_RGN_NOT_SUPPORT;
	}

	if (rect_rgn.x < 0 || rect_rgn.y < 0) {
		CVI_TRACE_RGN(RGN_ERR, "The start point can't be less than 0, start point(%d %d).\n",
			rect_rgn.x, rect_rgn.y);
		return ERR_RGN_ILLEGAL_PARAM;
	}
	if ((rect_rgn.width + rect_rgn.x) > rect_chn.width
		|| (rect_rgn.height + rect_rgn.y) > rect_chn.height) {
		CVI_TRACE_RGN(RGN_ERR, "size(%d %d %d %d) larger than chnsize(%d %d).\n",
			rect_rgn.x, rect_rgn.width, rect_rgn.y, rect_rgn.height,
			rect_chn.width, rect_chn.height);
		return ERR_RGN_ILLEGAL_PARAM;
	}

	return 0;
}

/**************************************************************************
 *   Sinking RGN APIs related functions.
 **************************************************************************/
int rgn_create(rgn_handle handle, const rgn_attr_s *pregion)
{
	struct rgn_ctx *ctx = NULL;
	int ret;
	unsigned int proc_idx;

	mutex_lock(&g_rgnlock);

	if (_rgn_hash_find(handle, &ctx, false)) {
		CVI_TRACE_RGN(RGN_ERR, "rgn_handle(%d) is already existing.\n", handle);
		mutex_unlock(&g_rgnlock);
		return ERR_RGN_EXIST;
	}

	if (rgn_num >= RGN_MAX_NUM) {
		CVI_TRACE_RGN(RGN_ERR, "over max rgn number %d\n", RGN_MAX_NUM);
		mutex_unlock(&g_rgnlock);
		return ERR_RGN_NOT_PERM;
	}

	if (pregion->type == OVERLAYEX_RGN) {
		CVI_TRACE_RGN(RGN_ERR, "rgn extension only support on vpss rgnex mode.\n");
		mutex_unlock(&g_rgnlock);
		return ERR_RGN_NOT_SUPPORT;
	}

	// check parameters.
	if (pregion->type == OVERLAY_RGN || pregion->type == OVERLAYEX_RGN) {
		pixel_format_e pixel_format;
		unsigned int canvas_num;
		unsigned char check_fail = 0;

		if (pregion->type == OVERLAY_RGN) {
			pixel_format = pregion->unattr.overlay.pixel_format;
			canvas_num = pregion->unattr.overlay.canvas_num;
		} else {
			pixel_format = pregion->unattr.overlay_ex.pixel_format;
			canvas_num = pregion->unattr.overlay_ex.canvas_num;
		}

		if (canvas_num == 0 || canvas_num > RGN_MAX_BUF_NUM) {
			CVI_TRACE_RGN(RGN_ERR, "invalid canvas_num(%d).\n", canvas_num);
			check_fail = 1;
		}

		if ((pixel_format != PIXEL_FORMAT_ARGB_8888)
		 && (pixel_format != PIXEL_FORMAT_ARGB_4444)
		 && (pixel_format != PIXEL_FORMAT_ARGB_1555)
		 && (pixel_format != PIXEL_FORMAT_8BIT_MODE)) {
			CVI_TRACE_RGN(RGN_ERR, "unsupported pxl-fmt(%d).\n", pixel_format);
			check_fail = 1;
		}

		if (check_fail) {
			mutex_unlock(&g_rgnlock);
			return ERR_RGN_ILLEGAL_PARAM;
		}
	} else if (pregion->type >= RGN_BUTT) {
		CVI_TRACE_RGN(RGN_ERR, "type(%d) not supported yet.\n", pregion->type);
		mutex_unlock(&g_rgnlock);
		return ERR_RGN_NOT_SUPPORT;
	}

	ctx = kzalloc(sizeof(*ctx), GFP_ATOMIC);
	if (ctx == NULL) {
		CVI_TRACE_RGN(RGN_ERR, "malloc failed.\n");
		ret = ERR_RGN_NOMEM;
		goto RGN_CTX_MALLOC_FAIL;
	}
	ctx->handle = handle;
	ctx->region = *pregion;

	mutex_lock(&g_rgnhashlock);
	hash_add(rgn_hash, &ctx->node, handle);
	mutex_unlock(&g_rgnhashlock);
	rgn_num++;

#if 0 //Linux Hashmap for print every element
	unsigned int  bkt;
	struct rgn_ctx *cur;

	hash_for_each(rgn_hash, bkt, cur, node) {
		CVI_TRACE_RGN(RGN_INFO, "handle:%d, created:%d, region.type:%d\n"
			, cur->handle, cur->created, cur->region.type);
	}
#endif

	// update rgn proc info
	for (proc_idx = 0; proc_idx < RGN_MAX_NUM; ++proc_idx) {
		if (!rgn_prc_ctx[proc_idx].created) {
			rgn_prc_ctx[proc_idx].created = true;
			rgn_prc_ctx[proc_idx].handle = ctx->handle;
			rgn_prc_ctx[proc_idx].region = ctx->region;
			break;
		}
	}

	mutex_unlock(&g_rgnlock);
	return 0;

RGN_CTX_MALLOC_FAIL:
	mutex_unlock(&g_rgnlock);
	return ret;

}

int rgn_destory(rgn_handle handle)
{
	struct rgn_ctx *ctx = NULL;
	unsigned int proc_idx;

	mutex_lock(&g_rgnlock);

	if (!_rgn_hash_find(handle, &ctx, false)) {
		CVI_TRACE_RGN(RGN_ERR, "rgn_handle(%d) is non-existent.\n", handle);
		mutex_unlock(&g_rgnlock);
		return ERR_RGN_UNEXIST;
	}

	if (ctx->chn.mod_id != ID_BASE) {
		_rgn_update_hw(ctx, RGN_OP_REMOVE);
	}

	CVI_TRACE_RGN(RGN_INFO, "rgn_handle(%d) canvas fmt(%d) size(%d * %d) stride(%d).\n"
	, handle, ctx->canvas_info[0].pixel_format, ctx->canvas_info[0].size.width
	, ctx->canvas_info[0].size.height, ctx->canvas_info[0].stride);

	//remove ctx in rgn_hash
	if (!_rgn_hash_find(handle, &ctx, true)) {
		CVI_TRACE_RGN(RGN_ERR, "rgn_handle(%d) remove failed.\n", handle);
		mutex_unlock(&g_rgnlock);
		return ERR_RGN_UNEXIST;
	}

	// update rgn proc info
	proc_idx = _rgn_proc_get_idx(handle);
	memset(&rgn_prc_ctx[proc_idx], 0, sizeof(rgn_prc_ctx[proc_idx]));
	rgn_num--;

	mutex_unlock(&g_rgnlock);

	kfree(ctx);

	return 0;
}

int rgn_get_attr(rgn_handle handle, rgn_attr_s *pregion)
{
	struct rgn_ctx *ctx = NULL;
	int ret;

	ret = check_rgn_handle(&ctx, handle);
	if (ret != 0)
		return ret;

	*pregion = ctx->region;

	return 0;
}

int rgn_set_attr(rgn_handle handle, const rgn_attr_s *pregion)
{
	struct rgn_ctx *ctx = NULL;
	unsigned int proc_idx;
	int ret;

	ret = check_rgn_handle(&ctx, handle);
	if (ret != 0)
		return ret;

	proc_idx = _rgn_proc_get_idx(handle);

	ctx->region = rgn_prc_ctx[proc_idx].region = *pregion;

	return 0;
}

int rgn_set_bit_map(rgn_handle handle, const bitmap_s *pbitmap)
{
	struct rgn_ctx *ctx = NULL;
	unsigned int bytesperline;
	rgn_canvas_info_s *pcanvas_info;
	unsigned int proc_idx;
	pixel_format_e pixel_format;
	unsigned int canvas_num;
	size_s rgn_size;
	unsigned short h;
	int ret;

	ret = check_rgn_handle(&ctx, handle);
	if (ret != 0)
		return ret;

	proc_idx = _rgn_proc_get_idx(handle);

	if (ctx->canvas_info[0].compressed) {
		CVI_TRACE_RGN(RGN_ERR, "This function only suppors OSD_COMPRESS_MODE_NONE.\n");
		return ERR_RGN_NOT_SUPPORT;
	}

	if (ctx->region.type != OVERLAY_RGN && ctx->region.type != OVERLAYEX_RGN) {
		CVI_TRACE_RGN(RGN_ERR, "Only Overlay/OverlayEx support. type(%d).\n", ctx->region.type);
		return ERR_RGN_NOT_SUPPORT;
	}

	if (ctx->canvas_get) {
		CVI_TRACE_RGN(RGN_ERR, "Need CVI_RGN_UpdateCanvas() first.\n");
		return ERR_RGN_NOT_PERM;
	}

	if (ctx->chn.mod_id == ID_BASE) {
		CVI_TRACE_RGN(RGN_ERR, "rgn_handle(%d) not attached to any chn yet.\n", handle);
		return ERR_RGN_NOT_CONFIG;
	}

	if (ctx->region.type == OVERLAY_RGN) {
		pixel_format = ctx->region.unattr.overlay.pixel_format;
		canvas_num = ctx->region.unattr.overlay.canvas_num;
		rgn_size = ctx->region.unattr.overlay.size;
	} else {
		pixel_format = ctx->region.unattr.overlay_ex.pixel_format;
		canvas_num = ctx->region.unattr.overlay_ex.canvas_num;
		rgn_size = ctx->region.unattr.overlay_ex.size;
	}

	if ((pbitmap->width > rgn_size.width)
	 || (pbitmap->height > rgn_size.height)) {
		CVI_TRACE_RGN(RGN_ERR, "size of bitmap (%d * %d) > region (%d * %d).\n"
			, pbitmap->width, pbitmap->height
			, rgn_size.width, rgn_size.height);
		return ERR_RGN_ILLEGAL_PARAM;
	}

	if (pbitmap->pixel_format != pixel_format) {
		CVI_TRACE_RGN(RGN_ERR, "pxl-fmt of bitmap(%d) != region(%d).\n"
			, pbitmap->pixel_format, pixel_format);
		return ERR_RGN_ILLEGAL_PARAM;
	}

	CVI_TRACE_RGN(RGN_INFO, "bitmap size(%d * %d) fmt(%d), region size(%d * %d) fmt(%d).\n"
		, pbitmap->width, pbitmap->height, pbitmap->pixel_format
		, rgn_size.width, rgn_size.height, pixel_format);

	if (_rgn_get_bytesperline(pbitmap->pixel_format, pbitmap->width, &bytesperline) != 0)
		return ERR_RGN_ILLEGAL_PARAM;

	// single/double buffer update per canvas_num.
	if (canvas_num == 1)
		pcanvas_info = &ctx->canvas_info[0];
	else {
		ctx->canvas_idx = 1 - ctx->canvas_idx;
		pcanvas_info = &ctx->canvas_info[ctx->canvas_idx];
	}

	pcanvas_info->pixel_format = pbitmap->pixel_format;
	pcanvas_info->size.width = pbitmap->width;
	pcanvas_info->size.height = pbitmap->height;
	pcanvas_info->stride = ALIGN(bytesperline, 32);

	CVI_TRACE_RGN(RGN_INFO, "canvas fmt(%d) size(%d * %d) stride(%d).\n"
		, pcanvas_info->pixel_format, pcanvas_info->size.width
		, pcanvas_info->size.height, pcanvas_info->stride);

	CVI_TRACE_RGN(RGN_INFO, "canvas v_addr(0x%lx), p_addr(0x%llx) size(%x)\n"
		, (uintptr_t)pcanvas_info->virt_addr, pcanvas_info->phy_addr
		, pcanvas_info->stride * pcanvas_info->size.height);

	CVI_TRACE_RGN(RGN_INFO, "pbitmap->data(0x%lx)\n", (uintptr_t)pbitmap->data);

	for (h = 0; h < pcanvas_info->size.height; ++h) {
		if (copy_from_user(pcanvas_info->virt_addr + pcanvas_info->stride * h,
					pbitmap->data + bytesperline * h, bytesperline)) {
			CVI_TRACE_RGN(RGN_ERR, "pbitmap->data, copy_from_user failed.\n");
			return ERR_RGN_ILLEGAL_PARAM;
		}
	}

	// update rgn proc canvas info
	if (canvas_num == 1)
		rgn_prc_ctx[proc_idx].canvas_info[0] = ctx->canvas_info[0];
	else {
		rgn_prc_ctx[proc_idx].canvas_idx = ctx->canvas_idx;
		rgn_prc_ctx[proc_idx].canvas_info[ctx->canvas_idx]
			= ctx->canvas_info[ctx->canvas_idx];
	}

	// update hw.
	return _rgn_update_hw(ctx, RGN_OP_UPDATE);
}

int rgn_attach_to_chn(rgn_handle handle, const mmf_chn_s*pchn, const rgn_chn_attr_s *pchn_attr)
{
	struct rgn_ctx *ctx = NULL;
	unsigned int proc_idx;
	int ret;
	unsigned char i;

	ret = check_rgn_handle(&ctx, handle);
	if (ret != 0)
		return ret;

	proc_idx = _rgn_proc_get_idx(handle);

	if (ctx->region.type != pchn_attr->type) {
		CVI_TRACE_RGN(RGN_ERR, "type(%d) is different with chn type(%d).\n"
			, ctx->region.type, pchn_attr->type);
		return ERR_RGN_ILLEGAL_PARAM;
	}

	if (pchn->mod_id != ID_VPSS && pchn->mod_id != ID_VO) {
		CVI_TRACE_RGN(RGN_ERR, "rgn can only be attached to vpss or vo.\n");
		return ERR_RGN_NOT_SUPPORT;
	}

	if (ctx->chn.mod_id != ID_BASE) {
		CVI_TRACE_RGN(RGN_ERR, "rgn_handle(%d) has been attached to CHN(%s-%d-%d).\n", handle
			, sys_get_modname(ctx->chn.mod_id), ctx->chn.dev_id, ctx->chn.chn_id);
		return ERR_RGN_NOT_CONFIG;
	}

	if (ctx->region.type == OVERLAYEX_RGN) {
		CVI_TRACE_RGN(RGN_ERR, "rgn extension only support on vpss rgnex mode.\n");
		return ERR_RGN_NOT_SUPPORT;
	}

	if (_rgn_check_chn_attr(handle, pchn, pchn_attr) != 0)
		return ERR_RGN_ILLEGAL_PARAM;

	mutex_lock(&g_rgnlock);
	if (pchn_attr->type == COVER_RGN) {
		ret = _rgn_update_cover_canvas(ctx, pchn_attr);

		if (ret != 0) {
			CVI_TRACE_RGN(RGN_ERR, "rgn_handle(%d) fill cover failed.\n", handle);
			mutex_unlock(&g_rgnlock);
			return ret;
		}
	}
	ctx->chn_attr = rgn_prc_ctx[proc_idx].chn_attr = *pchn_attr;
	ctx->chn = rgn_prc_ctx[proc_idx].chn = *pchn;

	if (ctx->region.type == OVERLAY_RGN || ctx->region.type == OVERLAYEX_RGN) {
		pixel_format_e pixel_format;
		unsigned int canvas_num, bgColor, compressed_size;
		size_s rgn_size;
		unsigned int bytesperline = 0;

		if (ctx->region.type == OVERLAY_RGN) {
			pixel_format = ctx->region.unattr.overlay.pixel_format;
			canvas_num = ctx->region.unattr.overlay.canvas_num;
			rgn_size = ctx->region.unattr.overlay.size;
			bgColor = ctx->region.unattr.overlay.bg_color;
			switch (ctx->region.unattr.overlay.compress_info.osd_compress_mode) {
			case OSD_COMPRESS_MODE_SW:
				compressed_size = ctx->region.unattr.overlay.compress_info.Est_compressed_size;
				break;
			case OSD_COMPRESS_MODE_HW:
				compressed_size = ctx->region.unattr.overlay.compress_info.compressed_size;
				break;
			default:
				compressed_size = 0;
				break;
			}
		} else {
			pixel_format = ctx->region.unattr.overlay_ex.pixel_format;
			canvas_num = ctx->region.unattr.overlay_ex.canvas_num;
			rgn_size = ctx->region.unattr.overlay_ex.size;
			bgColor = ctx->region.unattr.overlay_ex.bg_color;
			switch (ctx->region.unattr.overlay_ex.compress_info.osd_compress_mode) {
			case OSD_COMPRESS_MODE_SW:
				compressed_size = ctx->region.unattr.overlay_ex.compress_info.Est_compressed_size;
				break;
			case OSD_COMPRESS_MODE_HW:
				compressed_size = ctx->region.unattr.overlay_ex.compress_info.compressed_size;
				break;
			default:
				compressed_size = 0;
				break;
			}
		}

		ctx->canvas_idx = 0;

		ret = _rgn_get_bytesperline(pixel_format, rgn_size.width, &bytesperline);
		if (ret != 0) {
			goto RGN_FMT_INCORRECT;
		}

		ctx->canvas_info[0].pixel_format = pixel_format;
		ctx->canvas_info[0].size = rgn_size;
		ctx->canvas_info[0].stride = ALIGN(bytesperline, 32);
		if (ctx->region.unattr.overlay.compress_info.osd_compress_mode == OSD_COMPRESS_MODE_NONE) {
			ctx->canvas_info[0].compressed = false;
			ctx->canvas_info[0].osd_compress_mode = OSD_COMPRESS_MODE_NONE;
			ctx->canvas_info[0].compressed_size = 0;
			ctx->ion_len = ctx->max_need_ion =
				ctx->canvas_info[0].stride * ctx->canvas_info[0].size.height;
		} else {
			ctx->canvas_info[0].compressed = true;
			ctx->canvas_info[0].osd_compress_mode =
				ctx->region.unattr.overlay.compress_info.osd_compress_mode;
			ctx->canvas_info[0].compressed_size = compressed_size;
			ctx->ion_len = ctx->max_need_ion = compressed_size;
		}

		CVI_TRACE_RGN(RGN_INFO, "rgn_handle(%d) canvas fmt(%d) size(%d * %d) stride(%d).\n"
			, handle, ctx->canvas_info[0].pixel_format, ctx->canvas_info[0].size.width
			, ctx->canvas_info[0].size.height, ctx->canvas_info[0].stride);

		if (ctx->canvas_info[0].compressed)
			CVI_TRACE_RGN(RGN_INFO, "rgn_handle(%d) compressed canvas size(%d).\n"
				, handle, ctx->canvas_info[0].compressed_size);

		for (i = 1; i < canvas_num; ++i)
			ctx->canvas_info[i] = ctx->canvas_info[0];

		for (i = 0; i < canvas_num; ++i) {
			if (base_ion_alloc(&ctx->canvas_info[i].phy_addr,
							(void **)&ctx->canvas_info[i].virt_addr,
							"rgn_canvas", ctx->ion_len, true) != 0) {
				CVI_TRACE_RGN(RGN_ERR, "Region(%d) Can't acquire ion for Canvas-%d.\n", handle, i);
				ret = ERR_RGN_NOBUF;
				goto RGN_ION_ALLOC_FAIL;
			}
			_rgn_fill_pattern(ctx->canvas_info[i].virt_addr, ctx->ion_len, bgColor
				, (pixel_format == PIXEL_FORMAT_ARGB_8888) ? 4 :
				((pixel_format == PIXEL_FORMAT_8BIT_MODE) ? 1 : 2));
		}
		if (ctx->canvas_info[0].compressed && (canvas_num > 1)) {
			FIFO_INIT(&ctx->rgn_canvas_waitq, canvas_num);
			FIFO_INIT(&ctx->rgn_canvas_doneq, canvas_num);
			rgn_canvas_ctx[proc_idx][0].phy_addr = ctx->canvas_info[0].phy_addr;
			rgn_canvas_ctx[proc_idx][0].virt_addr = ctx->canvas_info[0].virt_addr;
			rgn_canvas_ctx[proc_idx][0].len = ctx->ion_len;
			rgn_canvas_ctx[proc_idx][1].phy_addr = ctx->canvas_info[1].phy_addr;
			rgn_canvas_ctx[proc_idx][1].virt_addr = ctx->canvas_info[1].virt_addr;
			rgn_canvas_ctx[proc_idx][1].len = ctx->ion_len;
			FIFO_PUSH(&ctx->rgn_canvas_doneq, &rgn_canvas_ctx[proc_idx][0]);
			FIFO_PUSH(&ctx->rgn_canvas_waitq, &rgn_canvas_ctx[proc_idx][1]);
		}

		 // update rgn proc canvas info
		if (ctx->region.type == OVERLAY_RGN) {
			rgn_prc_ctx[proc_idx].canvas_idx = ctx->canvas_idx;
			for (i = 0; i < ctx->region.unattr.overlay.canvas_num; ++i)
				rgn_prc_ctx[proc_idx].canvas_info[i] = ctx->canvas_info[i];
		} else if (ctx->region.type == OVERLAYEX_RGN) {
			rgn_prc_ctx[proc_idx].canvas_idx = ctx->canvas_idx;
			for (i = 0; i < ctx->region.unattr.overlay_ex.canvas_num; ++i)
				rgn_prc_ctx[proc_idx].canvas_info[i] = ctx->canvas_info[i];
		}
		rgn_prc_ctx[proc_idx].max_need_ion = ctx->max_need_ion;
	}

	ret = _rgn_update_hw(ctx, RGN_OP_INSERT);
	if (ret == 0) {
		// only update rgn_prc_ctx after _rgn_update_hw success
		rgn_prc_ctx[proc_idx].used = true;
	}
	mutex_unlock(&g_rgnlock);

	return ret;
RGN_ION_ALLOC_FAIL:
	if (ctx->region.type == OVERLAY_RGN) {
		for (i = 0; i < ctx->region.unattr.overlay.canvas_num; ++i)
			if (ctx->canvas_info[i].phy_addr)
				base_ion_free(ctx->canvas_info[i].phy_addr);
	} else if (ctx->region.type == OVERLAYEX_RGN) {
		for (i = 0; i < ctx->region.unattr.overlay_ex.canvas_num; ++i)
			if (ctx->canvas_info[i].phy_addr)
				base_ion_free(ctx->canvas_info[i].phy_addr);
	}
RGN_FMT_INCORRECT:
	mutex_unlock(&g_rgnlock);
	return ret;
}

int rgn_detach_from_chn(rgn_handle handle, const mmf_chn_s*pchn)
{
	struct rgn_ctx *ctx = NULL;
	int ret = -EINVAL;
	unsigned int proc_idx;
	int ret_tmp;
	unsigned char i;

	ret_tmp = check_rgn_handle(&ctx, handle);
	if (ret_tmp != 0)
		return ret_tmp;

	proc_idx = _rgn_proc_get_idx(handle);

	if (ctx->chn.mod_id == ID_BASE) {
		CVI_TRACE_RGN(RGN_ERR, "rgn_handle(%d) not attached to any chn yet.\n", handle);
		return ERR_RGN_NOT_CONFIG;
	}

	mutex_lock(&g_rgnlock);
	ret = _rgn_update_hw(ctx, RGN_OP_REMOVE);
	if (ret == 0) {
		ctx->chn.mod_id = rgn_prc_ctx[proc_idx].chn.mod_id = ID_BASE;
		rgn_prc_ctx[proc_idx].used = false;
	}
	ctx->odec_data_valid = false;

	if (ctx->chn_attr.type == OVERLAY_RGN) {
		for (i = 0; i < ctx->region.unattr.overlay.canvas_num; ++i)
			if (ctx->canvas_info[i].phy_addr)
				base_ion_free(ctx->canvas_info[i].phy_addr);
	} else if (ctx->chn_attr.type == OVERLAYEX_RGN) {
		for (i = 0; i < ctx->region.unattr.overlay_ex.canvas_num; ++i)
			if (ctx->canvas_info[i].phy_addr)
				base_ion_free(ctx->canvas_info[i].phy_addr);
	} else {
		if (ctx->canvas_info[0].phy_addr)
			base_ion_free(ctx->canvas_info[0].phy_addr);
	}

	if (ctx->region.type == OVERLAY_RGN || ctx->region.type == OVERLAYEX_RGN) {
		FIFO_EXIT(&ctx->rgn_canvas_waitq);
		FIFO_EXIT(&ctx->rgn_canvas_doneq);
	}

	mutex_unlock(&g_rgnlock);

	return ret;
}

int rgn_set_display_attr(rgn_handle handle, const mmf_chn_s*pchn, const rgn_chn_attr_s *pchn_attr)
{
	struct rgn_ctx *ctx = NULL;
	unsigned int proc_idx;
	rgn_chn_attr_s chn_attr;
	int ret = -EINVAL;

	ret = check_rgn_handle(&ctx, handle);
	if (ret != 0)
		return ret;

	proc_idx = _rgn_proc_get_idx(handle);

	if (ctx->chn.mod_id == ID_BASE
		|| (ctx->chn.mod_id != pchn->mod_id
		|| ctx->chn.dev_id != pchn->dev_id
		|| ctx->chn.chn_id != pchn->chn_id)) {
		CVI_TRACE_RGN(RGN_ERR, "rgn_handle(%d) is not attached on ModId(%d) dev_id(%d) chn_id(%d)\n",
			handle, pchn->mod_id, pchn->dev_id, pchn->chn_id);
		return ERR_RGN_NOT_CONFIG;
	}
	if (ctx->chn_attr.type != pchn_attr->type) {
		CVI_TRACE_RGN(RGN_ERR, "rgn_handle(%d) type not allowed to changed.\n", handle);
		return ERR_RGN_NOT_PERM;
	}

	if (_rgn_check_chn_attr(handle, pchn, pchn_attr) != 0)
		return ERR_RGN_ILLEGAL_PARAM;

	if (ctx->chn_attr.type == COVER_RGN) {
		ret = _rgn_update_cover_canvas(ctx, pchn_attr);

		if (ret != 0) {
			CVI_TRACE_RGN(RGN_ERR, "rgn_handle(%d) fill cover failed.\n", handle);
			return ret;
		}
	}

	memcpy(&chn_attr, &ctx->chn_attr, sizeof(rgn_chn_attr_s));
	ctx->chn_attr = rgn_prc_ctx[proc_idx].chn_attr = *pchn_attr;

	ret = _rgn_update_hw(ctx, RGN_OP_UPDATE);
	if (ret != 0) {
		ctx->chn_attr = rgn_prc_ctx[proc_idx].chn_attr = chn_attr;
	}

	return ret;
}

int rgn_get_display_attr(rgn_handle handle, const mmf_chn_s*pchn, rgn_chn_attr_s *pchn_attr)
{
	struct rgn_ctx *ctx = NULL;
	int ret;

	ret = check_rgn_handle(&ctx, handle);
	if (ret != 0)
		return ret;

	*pchn_attr = ctx->chn_attr;

	return 0;
}

int rgn_get_canvas_info(rgn_handle handle, rgn_canvas_info_s *pcanvas_info)
{
	struct rgn_ctx *ctx = NULL;
	unsigned int proc_idx;
	unsigned int canvas_num;
	int ret;
	struct rgn_canvas_ctx *rgn_canvas;
	int cnt = 0;

	ret = check_rgn_handle(&ctx, handle);
	if (ret != 0)
		return ret;

	proc_idx = _rgn_proc_get_idx(handle);

	if (ctx->region.type != OVERLAY_RGN && ctx->region.type != OVERLAYEX_RGN) {
		CVI_TRACE_RGN(RGN_ERR, "Only Overlay/OverlayEx support. type(%d).\n", ctx->region.type);
		return ERR_RGN_NOT_SUPPORT;
	}
	if (ctx->canvas_get) {
		CVI_TRACE_RGN(RGN_ERR, "Need CVI_RGN_UpdateCanvas() first.\n");
		return ERR_RGN_NOT_PERM;
	}

	if (ctx->region.type == OVERLAY_RGN)
		canvas_num = ctx->region.unattr.overlay.canvas_num;
	else
		canvas_num = ctx->region.unattr.overlay_ex.canvas_num;

	if (canvas_num == 1)
		*pcanvas_info = ctx->canvas_info[0];
	else {
		ctx->canvas_idx = rgn_prc_ctx[proc_idx].canvas_idx = 1 - ctx->canvas_idx;
		*pcanvas_info = ctx->canvas_info[ctx->canvas_idx];
	}
	CVI_TRACE_RGN(RGN_INFO, "rgn_handle(%d) canvas fmt(%d) size(%d * %d) stride(%d) compressed(%d).\n"
		, handle, pcanvas_info->pixel_format, pcanvas_info->size.width
		, pcanvas_info->size.height, pcanvas_info->stride, pcanvas_info->compressed);

	if (ctx->canvas_info[0].compressed) {
		while (FIFO_EMPTY(&ctx->rgn_canvas_waitq)) {
			cnt++;
			if (cnt % 5000 == 0) {
				CVI_TRACE_RGN(RGN_WARN, "handle(%d) waitq fifo empty for too long!", handle);
				cnt = 0;
			}
			usleep_range(1000, 2000);
		}
		mutex_lock(&ctx->rgn_canvas_q_lock);
		FIFO_POP(&ctx->rgn_canvas_waitq, &rgn_canvas);
		FIFO_PUSH(&ctx->rgn_canvas_doneq, rgn_canvas);
		mutex_unlock(&ctx->rgn_canvas_q_lock);
	}
	ctx->canvas_get = 1;

	return 0;
}

int rgn_update_canvas(rgn_handle handle)
{
	struct rgn_ctx *ctx = NULL;
	int ret = -EINVAL;
	int ret_tmp;
	unsigned int proc_idx;

	ret_tmp = check_rgn_handle(&ctx, handle);
	if (ret_tmp != 0)
		return ret_tmp;

	proc_idx = _rgn_proc_get_idx(handle);

	if (ctx->region.type != OVERLAY_RGN && ctx->region.type != OVERLAYEX_RGN) {
		CVI_TRACE_RGN(RGN_ERR, "Only Overlay/OverlayEx support. type(%d).\n", ctx->region.type);
		return ERR_RGN_NOT_SUPPORT;
	}
	if (!ctx->canvas_get) {
		CVI_TRACE_RGN(RGN_ERR, "CVI_RGN_GetCanvasInfo() first.\n");
		return ERR_RGN_NOT_SUPPORT;
	}

	if (ctx->canvas_info[0].compressed) {
		#if 0 /*bs size is passed from ioctl*/
		if (ctx->canvas_info[0].osd_compress_mode == OSD_COMPRESS_MODE_SW) {
			rgn_canvas_info_s *pcanvas_info = &ctx->canvas_info[ctx->canvas_idx];
			// first 8 bytes restores compress data header, original header:
			// bit[0:7] version
			// bit[8:11] osd_format
			// bit[12:14] reserved
			// bit[15:22] palette_cache_size
			// bit[23:24] alpha truncate
			// bit[25:26] reserved
			// bit[27:28] rgb truncate
			// bit[29:30] reserved
			// bit[31:46] image_width minus 1
			// bit[47:62] image_height minus 1

			// bitstream size is saved in bit[32:63], after bitstream size is get,
			// restore it to image width and height
			// bit[0:7] version
			// bit[8:11] osd_format
			// bit[12:14] reserved
			// bit[15:22] palette_cache_size
			// bit[23:24] alpha truncate
			// bit[25:26] reserved
			// bit[27:28] rgb truncate
			// bit[29:30] reserved
			// bit[32:63] bitstream size
			pcanvas_info->compressed_size =
				*((unsigned int *)pcanvas_info->virt_addr + 1);
			*((unsigned int *)pcanvas_info->virt_addr + 1) =
				(((pcanvas_info->size.width - 1) & 0xFFFF) |
				(((pcanvas_info->size.height - 1) << 16) & 0xFFFF0000)) >> 1;
		}
		#endif
		ctx->odec_data_valid = true;
	}

	ctx->canvas_updated = true;
	ret = _rgn_update_hw(ctx, RGN_OP_UPDATE);
	ctx->canvas_updated = false;

	ctx->canvas_get = 0;
	return ret;
}

/* CVI_RGN_Invert_Color - invert color per luma statistics of video content
 *   Chns' pixel-format should be YUV.
 *   RGN's pixel-format should be ARGB1555.
 *
 * @param handle: RGN handle
 * @param pchn: the chn which rgn attached
 * @param pcolor: rgn's content
 */
int rgn_invert_color(rgn_handle handle, mmf_chn_s*pchn, unsigned int *pcolor)
{
	int ret = -EINVAL;
	struct rgn_ctx *ctx = NULL;
	rgn_canvas_info_s canvas_info;
	int str_len;
	unsigned int luma_thresh;
	unsigned int proc_idx;
	unsigned int canvas_num;
	overlay_invert_color_s invert_color;
	point_s point;
	int ret_tmp;

	ret_tmp = check_rgn_handle(&ctx, handle);
	if (ret_tmp != 0)
		return ret_tmp;

	proc_idx = _rgn_proc_get_idx(handle);

	if ((pchn->mod_id != ctx->chn.mod_id) || (pchn->dev_id != ctx->chn.dev_id) ||
		(pchn->chn_id != ctx->chn.chn_id)) {
		CVI_TRACE_RGN(RGN_ERR, "rgn_handle(%d) not attached to chn(%d-%d-%d) yet.\n",
			handle,  pchn->mod_id, pchn->dev_id, pchn->chn_id);
		return ERR_RGN_ILLEGAL_PARAM;
	}

	if (ctx->region.type != OVERLAY_RGN && ctx->region.type != OVERLAYEX_RGN) {
		CVI_TRACE_RGN(RGN_ERR, "CVI_RGN_Invert_Color only support Overlay/OverlayEx (%d).\n"
			, ctx->region.type);
		return ERR_RGN_NOT_SUPPORT;
	}

	if (pchn->mod_id != ID_VPSS) {
		CVI_TRACE_RGN(RGN_ERR, "CVI_RGN_Invert_Color GRN did not attach to vpss.\n");
		return ERR_RGN_NOT_SUPPORT;
	}

	if (ctx->region.type == OVERLAY_RGN) {
		canvas_num = ctx->region.unattr.overlay.canvas_num;
		invert_color = ctx->chn_attr.unchn_attr.overlay_chn.invert_color;
		point = ctx->chn_attr.unchn_attr.overlay_chn.point;
	} else {
		canvas_num = ctx->region.unattr.overlay_ex.canvas_num;
		invert_color = ctx->chn_attr.unchn_attr.overlay_ex_chn.invert_color;
		point = ctx->chn_attr.unchn_attr.overlay_ex_chn.point;
	}

	if (invert_color.inv_col_en != 1) {
		CVI_TRACE_RGN(RGN_ERR, "CVI_RGN_Invert_Color inv_col_en hasn't been set\n");
		return ERR_RGN_SYS_NOTREADY;
	}

	if (canvas_num == 1)
		canvas_info = ctx->canvas_info[0];
	else {
		ctx->canvas_idx = rgn_prc_ctx[proc_idx].canvas_idx = 1 - ctx->canvas_idx;
		canvas_info = ctx->canvas_info[ctx->canvas_idx];
	}
	str_len = canvas_info.size.width / invert_color.inv_col_area.width;

	luma_thresh = invert_color.lum_thresh;

	return ret;
}

rgn_component_info_s convert_info[RGN_COLOR_FMT_BUTT] = {
		{ 0, 4, 4, 4 }, /*RGB444*/
		{ 4, 4, 4, 4 }, /*ARGB4444*/
		{ 0, 5, 5, 5 }, /*RGB555*/
		{ 0, 5, 6, 5 }, /*RGB565*/
		{ 1, 5, 5, 5 }, /*ARGB1555*/
		{ 0, 0, 0, 0 }, /*RESERVED*/
		{ 0, 8, 8, 8 }, /*RGB888*/
		{ 8, 8, 8, 8 }, /*ARGB8888*/
		{ 4, 4, 4, 4 }, /*ARGB4444*/
		{ 1, 5, 5, 5 }, /*ARGB1555*/
		{ 8, 8, 8, 8 }  /*ARGB8888*/
};

unsigned short RGN_MAKECOLOR_U16_A(unsigned char a, unsigned char r, unsigned char g, unsigned char b, rgn_component_info_s input_fmt)
{
	unsigned char a1, r1, g1, b1;
	unsigned short pixel;

	pixel = a1 = r1 = g1 = b1 = 0;
	a1 = ((input_fmt.alen - 4) > 0) ? a >> (input_fmt.alen - 4) :
		(input_fmt.alen ? a << (4 - input_fmt.alen) : a);
	r1 = ((input_fmt.rlen - 4) > 0) ? r >> (input_fmt.rlen - 4) : r;
	g1 = ((input_fmt.glen - 4) > 0) ? g >> (input_fmt.glen - 4) : g;
	b1 = ((input_fmt.blen - 4) > 0) ? b >> (input_fmt.blen - 4) : b;

	pixel = (b1 | (g1 << 4) | (r1 << 8 | (a1 << 12)));

	return pixel;
}

int rgn_set_chn_palette(rgn_handle handle, const mmf_chn_s*pchn, rgn_palette *ppalette,
			rgn_rgbquad_s *input_pixel_table)
{
	struct rgn_ctx *ctx;
	struct rgn_lut_cfg lut_cfg;
	int ret = 0;
	unsigned int idx, u32pixel, osd_layer;
	unsigned short pixel;
	unsigned char r, g, b, a;
	// rgn_rgbquad_s *input_pixel_table;
	struct _rgn_lut_cb_param rgn_lut_arg;
	int ret_tmp;

	ret_tmp = check_rgn_handle(&ctx, handle);
	if (ret_tmp != 0)
		return ret_tmp;

	if (ppalette->lut_length > 256) {
		CVI_TRACE_RGN(RGN_ERR, "RGN_LUT_index(%d) is over maximum(256).\n", ppalette->lut_length);
		return -1;
	}

	osd_layer = (ctx->canvas_info[ctx->canvas_idx].compressed) ?
				RGN_ODEC_LAYER_VPSS : RGN_NORMAL_LAYER_VPSS;

#if 0 /* todo---copy_from_user here is not right, why??? */
	input_pixel_table = (rgn_rgbquad_s *)kmalloc_array(ppalette->lut_length,
		 sizeof(rgn_rgbquad_s), GFP_KERNEL);
	if (input_pixel_table == NULL) {
		CVI_TRACE_RGN(RGN_ERR, "kmalloc failed.\n");
		return ERR_RGN_NOMEM;
	}

	if (copy_from_user(input_pixel_table, (void *)(ppalette->ppalette_table),
		lut_cfg.lut_length * sizeof(rgn_rgbquad_s))) {
		CVI_TRACE_RGN(RGN_ERR, "lut copy_from_user failed.\n");
		ret = ERR_RGN_NOMEM;
		kfree(input_pixel_table);
		return ret;
	}

	for (idx = 0; idx < ppalette->lut_length ; idx++) {
		CVI_TRACE_RGN(RGN_INFO, "index:%d pixel:%#x\n",
			idx, (input_pixel_table[idx].argb_blue | input_pixel_table[idx].argb_green << 8) |
				(input_pixel_table[idx].argb_red << 16 | input_pixel_table[idx].argb_alpha << 24));
	}
#endif

	/* start color convert to ARGB4444 */
	for (idx = 0 ; idx < ppalette->lut_length ; idx++) {
		switch (ppalette->pixel_format) {
		case RGN_COLOR_FMT_RGB444:
		case RGN_COLOR_FMT_RGB555:
		case RGN_COLOR_FMT_RGB565:
		case RGN_COLOR_FMT_RGB888:
			a = 0xf; /*alpha*/
			r = input_pixel_table[idx].argb_red;
			g = input_pixel_table[idx].argb_green;
			b = input_pixel_table[idx].argb_blue;
			break;

		case RGN_COLOR_FMT_RGB1555:
		case RGN_COLOR_FMT_RGB4444:
		case RGN_COLOR_FMT_RGB8888:
		case RGN_COLOR_FMT_ARGB8888:
		case RGN_COLOR_FMT_ARGB4444:
		case RGN_COLOR_FMT_ARGB1555:
		default:
			a = input_pixel_table[idx].argb_alpha;
			r = input_pixel_table[idx].argb_red;
			g = input_pixel_table[idx].argb_green;
			b = input_pixel_table[idx].argb_blue;
			break;
		}

		u32pixel = (b | g << 8 | r << 16 | a << 24);
		CVI_TRACE_RGN(RGN_INFO, "Input Table index(%d) (0x%x).\n", idx, u32pixel);

		pixel = RGN_MAKECOLOR_U16_A(a, r, g, b, convert_info[ppalette->pixel_format]);
		CVI_TRACE_RGN(RGN_INFO, "Output data = (0x%x).\n", pixel);

		lut_cfg.lut_addr[idx] = pixel;
	}

	/* Write output_pixel_table and lut_length into cfg */
	lut_cfg.lut_length = ppalette->lut_length;

#if 0 // no rgnex for now
	/* Get related device number to update LUT but RGNEX use device 0. */
	if (ctx->region.type == OVERLAY_RGN || ctx->region.type == COVER_RGN)
		lut_cfg.rgnex_en = false;
	else
		lut_cfg.rgnex_en = true;
#endif
	lut_cfg.is_updated = true;
	// Need to record for gop0 or gop1
	lut_cfg.lut_layer = osd_layer;

	/* Uupdate device LUT in kernel space. */
	rgn_lut_arg.chn = ctx->chn;
	rgn_lut_arg.lut_cfg = lut_cfg;
	if (_rgn_call_cb(E_MODULE_VPSS, VPSS_CB_SET_RGN_LUT_CFG, &rgn_lut_arg) != 0) {
		CVI_TRACE_RGN(RGN_ERR, "VPSS_CB_SET_LUT_CFG is failed\n");
		return ERR_RGN_ILLEGAL_PARAM;
	}

	return ret;
}

static int _rgn_sw_init(struct rgn_dev *rdev)
{
	return _rgn_init();
}

static int _rgn_release_op(struct rgn_dev *rdev)
{
	return _rgn_exit();
}

static int _rgn_create_proc(struct rgn_dev *rdev)
{
	int ret = -EINVAL;

	if (rgn_proc_init() < 0) {
		CVI_TRACE_RGN(RGN_ERR, "rgn proc init failed\n");
		goto err;
	}
	ret = 0;

err:
	return ret;
}

static void _rgn_destroy_proc(struct rgn_dev *rdev)
{
	rgn_proc_remove();
}

/*******************************************************
 *  File operations for core
 ******************************************************/
static long _rgn_s_ctrl(struct rgn_dev *rdev, struct rgn_ext_control *p)
{
	int ret = -EINVAL;
	rgn_attr_s region;
	bitmap_s bitmap;
	mmf_chn_s chn;
	rgn_chn_attr_s chn_attr;
	unsigned int color, id, sdk_id;
	rgn_palette palette;
	rgn_handle handle;

	id	= p->id;
	sdk_id	= p->sdk_id;
	handle	= p->handle;

	switch (id) {
	case RGN_IOCTL_SC_SET_RGN: {
#if 0 //copy from vip_sc.c for V4L2_CID_DV_VIP_SC_SET_RGN
		if (copy_from_user(&sdev->vpss_chn_cfg[0].rgn_cfg, ext_ctrls[i].ptr,
						   sizeof(sdev->vpss_chn_cfg[0].rgn_cfg))) {
			CVI_TRACE_VPSS(CVI_DBG_ERR, "ioctl-%#x, copy_from_user failed.\n", ext_ctrls[i].id);
			rc = -ENOMEM;
			break;
		}
		rc = 0;
#endif
	} //RGN_IOCTL_SC_SET_RGN:
	break;

	case RGN_IOCTL_DISP_SET_RGN: {
#if 0 //copy from vip_disp.c for V4L2_CID_DV_VIP_DISP_SET_RGN
		struct sclr_disp_timing *timing = sclr_disp_get_timing();
		struct sclr_size size;
		struct rgn_cfg cfg;

		if (copy_from_user(&cfg, ext_ctrls[i].ptr, sizeof(struct rgn_cfg))) {
			dprintk(VIP_ERR, "ioctl-%#x, copy_from_user failed.\n", ext_ctrls[i].id);
			break;
		}

		size.w = timing->hfde_end - timing->hfde_start + 1;
		size.h = timing->vfde_end - timing->vfde_start + 1;
		rc = vip_set_rgn_cfg(SCL_GOP_DISP, &cfg, &size);
#endif
	} //RGN_IOCTL_DISP_SET_RGN:
	break;

	case RGN_IOCTL_SDK_CTRL: {
		switch (sdk_id) {
		case RGN_SDK_CREATE: {
			if (copy_from_user(&region, p->ptr1, sizeof(rgn_attr_s)) != 0) {
				CVI_TRACE_RGN(RGN_ERR, "Region create, copy_from_user failed.\n");
				ret = -EFAULT;
				break;
			}

			ret = rgn_create(handle, &region);
		}
		break;

		case RGN_SDK_DESTORY: {
			ret = rgn_destory(handle);
		}
		break;

		case RGN_SDK_SET_ATTR: {
			if (copy_from_user(&region, p->ptr1, sizeof(rgn_attr_s)) != 0) {
				CVI_TRACE_RGN(RGN_ERR, "Region set attribute, copy_from_user failed.\n");
				ret = -EFAULT;
				break;
			}

			ret = rgn_set_attr(handle, &region);
		}
		break;

		case RGN_SDK_SET_BIT_MAP: {
			if (copy_from_user(&bitmap, p->ptr1, sizeof(bitmap_s)) != 0) {
				CVI_TRACE_RGN(RGN_ERR, "Region set bitmap, copy_from_user failed.\n");
				ret = -EFAULT;
				break;
			}

			ret = rgn_set_bit_map(handle, &bitmap);
		}
		break;

		case RGN_SDK_ATTACH_TO_CHN: {
			if ((copy_from_user(&chn, p->ptr1, sizeof(mmf_chn_s)) != 0) ||
				(copy_from_user(&chn_attr, p->ptr2, sizeof(rgn_chn_attr_s)) != 0)) {
				CVI_TRACE_RGN(RGN_ERR, "Region attach to chn, copy_from_user failed.\n");
				ret = -EFAULT;
				break;
			}

			ret = rgn_attach_to_chn(handle, &chn, &chn_attr);
		}
		break;

		case RGN_SDK_DETACH_FROM_CHN: {
			if (copy_from_user(&chn, p->ptr1, sizeof(mmf_chn_s)) != 0) {
				CVI_TRACE_RGN(RGN_ERR, "Region detach from chn, copy_from_user failed.\n");
				ret = -EFAULT;
				break;
			}

			ret = rgn_detach_from_chn(handle, &chn);
		}
		break;

		case RGN_SDK_SET_DISPLAY_ATTR: {
			if ((copy_from_user(&chn, p->ptr1, sizeof(mmf_chn_s)) != 0) ||
				(copy_from_user(&chn_attr, p->ptr2, sizeof(rgn_chn_attr_s)) != 0)) {
				CVI_TRACE_RGN(RGN_ERR, "Region set display attribute, copy_from_user failed.\n");
				ret = -EFAULT;
				break;
			}

			ret = rgn_set_display_attr(handle, &chn, &chn_attr);
		}
		break;

		case RGN_SDK_UPDATE_CANVAS: {
			ret = rgn_update_canvas(handle);
		}
		break;

		case RGN_SDK_INVERT_COLOR: {
			if ((copy_from_user(&chn, p->ptr1, sizeof(mmf_chn_s)) != 0) ||
				(copy_from_user(&color, p->ptr2, sizeof(u32)) != 0)) {
				CVI_TRACE_RGN(RGN_ERR, "Region invert color, copy_from_user failed.\n");
				ret = -EFAULT;
				break;
			}

			ret = rgn_invert_color(handle, &chn, &color);
		}
		break;

		case RGN_SDK_SET_CHN_PALETTE: {
			rgn_rgbquad_s *input_pixel_table;

			if ((copy_from_user(&chn, p->ptr1, sizeof(mmf_chn_s)) != 0) ||
			    (copy_from_user(&palette, p->ptr2, sizeof(rgn_palette)) != 0)) {
				CVI_TRACE_RGN(RGN_ERR, "Region set chn palette, copy_from_user failed.\n");
				ret = -EFAULT;
				break;
			}

			input_pixel_table = kmalloc_array(palette.lut_length, sizeof(rgn_rgbquad_s), GFP_KERNEL);
			if (input_pixel_table == NULL) {
				CVI_TRACE_RGN(RGN_ERR, "kmalloc failed.\n");
				return ERR_RGN_NOMEM;
			}
			if (copy_from_user(input_pixel_table, (void *)(palette.ppalette_table),
				palette.lut_length * sizeof(rgn_rgbquad_s))) {
				CVI_TRACE_RGN(RGN_ERR, "lut copy_from_user failed.\n");
				ret = ERR_RGN_NOMEM;
				kfree(input_pixel_table);
				break;
			}

			ret = rgn_set_chn_palette(handle, &chn, &palette, input_pixel_table);
			kfree(input_pixel_table);
		}
		break;

		case RGN_SDK_SET_CMPR_SIZE: {
			int cmpr_sz;
			struct rgn_ctx *ctx = NULL;

			if (!_rgn_hash_find(handle, &ctx, false)) {
				CVI_TRACE_RGN(RGN_ERR, "rgn_handle(%d) is non-existent.\n", handle);
				mutex_unlock(&g_rgnlock);
				return ERR_RGN_UNEXIST;
			}
			if ((copy_from_user(&cmpr_sz, p->ptr1, sizeof(s32)) != 0)) {
				CVI_TRACE_RGN(RGN_ERR, "Region set compress size, copy_from_user failed.\n");
				ret = -EFAULT;
				break;
			}

			ctx->canvas_info[ctx->canvas_idx].compressed_size = cmpr_sz;
			ret = 0;
		}
		break;

		default:
			break;
		} //switch (sdk_id)

	} //case RGN_IOCTL_SDK_CTRL:
	break;

	default:
	break;
	} //switch (id)

	return ret;
}

static long _rgn_g_ctrl(struct rgn_dev *rdev, struct rgn_ext_control *p)
{
	int ret = -EINVAL;
	rgn_attr_s region;
	mmf_chn_s chn;
	rgn_chn_attr_s chn_attr;
	rgn_canvas_info_s canvas_info;
	unsigned int id, sdk_id;
	rgn_handle handle;

	id	= p->id;
	sdk_id	= p->sdk_id;
	handle	= p->handle;

	switch (id) {
	case RGN_IOCTL_SDK_CTRL: {
		switch (sdk_id) {
		case RGN_SDK_GET_ATTR: {
			ret = rgn_get_attr(handle, &region);

			if (copy_to_user(p->ptr1, &region, sizeof(rgn_attr_s)) != 0) {
				CVI_TRACE_RGN(RGN_ERR, "Region get attribute, copy_to_user failed.\n");
				ret = -EFAULT;
			}
		}
		break;

		case RGN_SDK_GET_DISPLAY_ATTR: {
			if (copy_from_user(&chn, p->ptr1, sizeof(mmf_chn_s)) != 0) {
				CVI_TRACE_RGN(RGN_ERR, "Region get display attribute, copy_from_user failed.\n");
				ret = -EFAULT;
				break;
			}

			ret = rgn_get_display_attr(handle, &chn, &chn_attr);

			if (copy_to_user(p->ptr2, &chn_attr, sizeof(rgn_chn_attr_s)) != 0) {
				CVI_TRACE_RGN(RGN_ERR, "Region get display attribute, copy_to_user failed.\n");
				ret = -EFAULT;
			}
		}
		break;

		case RGN_SDK_GET_CANVAS_INFO: {
			ret = rgn_get_canvas_info(handle, &canvas_info);

			if (copy_to_user(p->ptr1, &canvas_info, sizeof(rgn_canvas_info_s)) != 0) {
				CVI_TRACE_RGN(RGN_ERR, "Region get canvas info, copy_to_user failed.\n");
				ret = -EFAULT;
			}
		}
		break;

		case RGN_SDK_GET_ION_LEN: {
			struct rgn_ctx *ctx = NULL;

			ret = check_rgn_handle(&ctx, handle);
			if (ret != 0)
				break;
			if (copy_to_user(p->ptr1, &ctx->ion_len, sizeof(u32)) != 0) {
				CVI_TRACE_RGN(RGN_ERR, "Region get ion length, copy_to_user failed.\n");
				ret = -EFAULT;
			}

		}
		break;

		default:
			break;
		} //switch (sdk_id)

	} //case RGN_IOCTL_SDK_CTRL:
	break;

	default:
	break;
	} //switch (id)

	return ret;
}

long rgn_ioctl(struct file *file, u_int cmd, u_long arg)
{
	int ret = -EINVAL;
	struct rgn_dev *rdev = file->private_data;
	struct rgn_ext_control p;

	if (copy_from_user(&p, (void __user *)arg, sizeof(struct rgn_ext_control)))
		return ret;

	switch (cmd) {
	case RGN_IOC_S_CTRL:
		ret = _rgn_s_ctrl(rdev, &p);
		break;
	case RGN_IOC_G_CTRL:
		ret = _rgn_g_ctrl(rdev, &p);
		break;
	default:
		ret = -ENOTTY;
		break;
	}

	if (copy_to_user((void __user *)arg, &p, sizeof(struct rgn_ext_control)))
		return ret;

	return ret;
}

int rgn_open(struct inode *inode, struct file *file)
{
	int ret = 0;

	// only open once
	if (!atomic_read(&dev_open_cnt)) {
		struct rgn_dev *rdev =
		container_of(file->private_data, struct rgn_dev, miscdev);

		file->private_data = rdev;
		_rgn_sw_init(rdev);

		CVI_TRACE_RGN(RGN_INFO, "-\n");

		ret = 0;
	}

	atomic_inc(&dev_open_cnt);

	return ret;
}

int rgn_release(struct inode *inode, struct file *file)
{
	int ret = 0;

	// only exit once
	if (atomic_dec_and_test(&dev_open_cnt)) {
		struct rgn_dev *rdev =
		container_of(file->private_data, struct rgn_dev, miscdev);

		/* This should move to stop streaming */
		_rgn_release_op(rdev);

		CVI_TRACE_RGN(RGN_INFO, "-\n");

		ret = 0;
	}

	if (atomic_read(&dev_open_cnt) < 0)
		atomic_set(&dev_open_cnt, 0);

	return ret;
}

int rgn_cb(void *dev, enum enum_modules_id caller, unsigned int cmd, void *arg)
{
	int ret = 0;

	switch (cmd) {
	default:
		break;
	}

	return ret;
}

/*******************************************************
 *  Common interface for core
 ******************************************************/
int rgn_create_instance(struct platform_device *pdev)
{
	int ret = -EINVAL;
	struct rgn_dev *rdev;

	rdev = dev_get_drvdata(&pdev->dev);
	if (!rdev) {
		CVI_TRACE_RGN(RGN_ERR, "invalid data\n");
		goto err;
	}

	if (_rgn_create_proc(rdev)) {
		CVI_TRACE_RGN(RGN_ERR, "Failed to create proc\n");
		goto err;
	}
	ret = 0;

err:
	return ret;
}

int rgn_destroy_instance(struct platform_device *pdev)
{
	int ret = -EINVAL;
	struct rgn_dev *rdev;

	rdev = dev_get_drvdata(&pdev->dev);
	if (!rdev) {
		CVI_TRACE_RGN(RGN_ERR, "invalid data\n");
		goto err;
	}

	_rgn_destroy_proc(rdev);
	ret = 0;

err:
	return ret;
}
