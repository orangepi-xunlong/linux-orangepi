#include "rgn_proc.h"

#define GENERATE_STRING(STRING)	(#STRING),
#define RGN_PROC_NAME "soph/rgn"

static const char *const MOD_STRING[] = FOREACH_MOD(GENERATE_STRING);
extern struct rgn_ctx rgn_prc_ctx;
/*************************************************************************
 *	Region proc functions
 *************************************************************************/
static void _pix_fmt_to_string(enum _pixel_format_e pix_fmt, char *str, int len)
{
	switch (pix_fmt) {
	case PIXEL_FORMAT_8BIT_MODE:
		strncpy(str, "256LUT", len);
		break;
	case PIXEL_FORMAT_ARGB_1555:
		strncpy(str, "ARGB_1555", len);
		break;
	case PIXEL_FORMAT_ARGB_4444:
		strncpy(str, "ARGB_4444", len);
		break;
	case PIXEL_FORMAT_ARGB_8888:
		strncpy(str, "ARGB_8888", len);
		break;
	default:
		strncpy(str, "Unknown Fmt", len);
		break;
	}
}

static int rgn_proc_show(struct seq_file *m, void *v)
{
	struct rgn_ctx *prgn_ctx = &rgn_prc_ctx;
	int i;
	char c[32];

	if (!prgn_ctx) {
		seq_puts(m, "rgn_prc_ctx = NULL\n");
		return -1;
	}

	seq_printf(m, "\nModule: [RGN], Build Time[%s]\n", UTS_VERSION);
	// Region status of overlay
	seq_puts(m, "\n------REGION STATUS OF OVERLAY--------------------------------------------\n");
	seq_printf(m, "%10s%10s%10s%20s%10s%10s%10s%20s%20s%10s%10s%7s%12s\n",
		"Hdl", "Type", "Used", "PiFmt", "W", "H", "BgColor", "Phy", "Virt", "Stride", "CnvsNum",
		"Cmpr", "MaxNeedIon");

	for (i = 0; i < RGN_MAX_NUM; ++i) {
		if (prgn_ctx[i].region.type == OVERLAY_RGN && prgn_ctx[i].created) {
			memset(c, 0, sizeof(c));
			_pix_fmt_to_string(prgn_ctx[i].region.unattr.overlay.pixel_format, c, sizeof(c));

			seq_printf(m, "%7s%3d%10d%10s%20s%10d%10d%10x%20llx%20lx%10d%10d%7s%12d\n",
				"#",
				prgn_ctx[i].handle,
				prgn_ctx[i].region.type,
				(prgn_ctx[i].used) ? "Y" : "N",
				c,
				prgn_ctx[i].region.unattr.overlay.size.width,
				prgn_ctx[i].region.unattr.overlay.size.height,
				prgn_ctx[i].region.unattr.overlay.bg_color,
				prgn_ctx[i].canvas_info[prgn_ctx[i].canvas_idx].phy_addr,
				(uintptr_t)prgn_ctx[i].canvas_info[prgn_ctx[i].canvas_idx].virt_addr,
				prgn_ctx[i].canvas_info[prgn_ctx[i].canvas_idx].stride,
				prgn_ctx[i].region.unattr.overlay.canvas_num,
				prgn_ctx[i].canvas_info[prgn_ctx[i].canvas_idx].compressed ? "Y" : "N",
				prgn_ctx[i].max_need_ion);
		}
	}

	// Region chn status of overlay
	seq_puts(m, "\n------REGION CHN STATUS OF OVERLAY----------------------------------------\n");
	seq_printf(m, "%10s%10s%10s%10s%10s%10s%10s%10s%10s\n",
		"Hdl", "Type", "Mod", "Dev", "Chn", "bShow", "X", "Y", "Layer");

	for (i = 0; i < RGN_MAX_NUM; ++i) {
		if (prgn_ctx[i].region.type == OVERLAY_RGN && prgn_ctx[i].created && prgn_ctx[i].used) {
			seq_printf(m, "%7s%3d%10d%10s%10d%10d%10s%10d%10d%10d\n",
				"#",
				prgn_ctx[i].handle,
				prgn_ctx[i].region.type,
				MOD_STRING[prgn_ctx[i].chn.mod_id],
				prgn_ctx[i].chn.dev_id,
				prgn_ctx[i].chn.chn_id,
				(prgn_ctx[i].chn_attr.show) ? "Y" : "N",
				prgn_ctx[i].chn_attr.unchn_attr.overlay_chn.point.x,
				prgn_ctx[i].chn_attr.unchn_attr.overlay_chn.point.y,
				prgn_ctx[i].chn_attr.unchn_attr.overlay_chn.layer);
		}
	}

	// Region status of cover
	seq_puts(m, "\n------REGION STATUS OF COVER----------------------------------------------\n");
	seq_printf(m, "%10s%10s%10s\n", "Hdl", "Type", "Used");
	for (i = 0; i < RGN_MAX_NUM; ++i) {
		if (prgn_ctx[i].region.type == COVER_RGN && prgn_ctx[i].created) {
			seq_printf(m, "%7s%3d%10d%10s\n", "#", prgn_ctx[i].handle, prgn_ctx[i].region.type,
				(prgn_ctx[i].used) ? "Y" : "N");
		}
	}

	// Region chn status of rect cover
	seq_puts(m, "\n------REGION CHN STATUS OF RECT COVER-------------------------------------\n");
	seq_printf(m, "%10s%10s%10s%10s%10s%10s\n%10s%10s%10s%10s%10s%10s%10s\n\n",
		"Hdl", "Type", "Mod", "Dev", "Chn", "bShow",
		"X", "Y", "W", "H", "Color", "Layer", "CoorType");

	for (i = 0; i < RGN_MAX_NUM; ++i) {
		if (prgn_ctx[i].region.type == COVER_RGN && prgn_ctx[i].created && prgn_ctx[i].used
			&& prgn_ctx[i].chn_attr.unchn_attr.cover_chn.cover_type == AREA_RECT) {
			seq_printf(m, "%7s%3d%10d%10s%10d%10d%10s\n%10d%10d%10d%10d%10X%10d%10s\n",
				"#",
				prgn_ctx[i].handle,
				prgn_ctx[i].region.type,
				MOD_STRING[prgn_ctx[i].chn.mod_id],
				prgn_ctx[i].chn.dev_id,
				prgn_ctx[i].chn.chn_id,
				(prgn_ctx[i].chn_attr.show) ? "Y" : "N",
				prgn_ctx[i].chn_attr.unchn_attr.cover_chn.rect.x,
				prgn_ctx[i].chn_attr.unchn_attr.cover_chn.rect.y,
				prgn_ctx[i].chn_attr.unchn_attr.cover_chn.rect.width,
				prgn_ctx[i].chn_attr.unchn_attr.cover_chn.rect.height,
				prgn_ctx[i].chn_attr.unchn_attr.cover_chn.color,
				prgn_ctx[i].chn_attr.unchn_attr.cover_chn.layer,
				(prgn_ctx[i].chn_attr.unchn_attr.cover_chn.coordinate == RGN_ABS_COOR) ?
					"ABS" : "RATIO");
		}
	}

	// Region status of coverex
	seq_puts(m, "\n------REGION STATUS OF COVEREX--------------------------------------------\n");
	seq_printf(m, "%10s%10s%10s\n", "Hdl", "Type", "Used");
	for (i = 0; i < RGN_MAX_NUM; ++i) {
		if (prgn_ctx[i].region.type == COVEREX_RGN && prgn_ctx[i].created) {
			seq_printf(m, "%7s%3d%10d%10s\n", "#", prgn_ctx[i].handle, prgn_ctx[i].region.type,
				(prgn_ctx[i].used) ? "Y" : "N");
		}
	}

	// Region chn status of rect coverex
	seq_puts(m, "\n------REGION CHN STATUS OF RECT COVEREX-----------------------------------\n");
	seq_printf(m, "%10s%10s%10s%10s%10s%10s\n%10s%10s%10s%10s%10s%10s\n\n",
		"Hdl", "Type", "Mod", "Dev", "Chn", "bShow",
		"X", "Y", "W", "H", "Color", "Layer");

	for (i = 0; i < RGN_MAX_NUM; ++i) {
		if (prgn_ctx[i].region.type == COVEREX_RGN && prgn_ctx[i].created && prgn_ctx[i].used
			&& prgn_ctx[i].chn_attr.unchn_attr.cover_ex_chn.cover_type == AREA_RECT) {
			seq_printf(m, "%7s%3d%10d%10s%10d%10d%10s\n%10d%10d%10d%10d%10X%10d\n",
				"#",
				prgn_ctx[i].handle,
				prgn_ctx[i].region.type,
				MOD_STRING[prgn_ctx[i].chn.mod_id],
				prgn_ctx[i].chn.dev_id,
				prgn_ctx[i].chn.chn_id,
				(prgn_ctx[i].chn_attr.show) ? "Y" : "N",
				prgn_ctx[i].chn_attr.unchn_attr.cover_ex_chn.rect.x,
				prgn_ctx[i].chn_attr.unchn_attr.cover_ex_chn.rect.y,
				prgn_ctx[i].chn_attr.unchn_attr.cover_ex_chn.rect.width,
				prgn_ctx[i].chn_attr.unchn_attr.cover_ex_chn.rect.height,
				prgn_ctx[i].chn_attr.unchn_attr.cover_ex_chn.color,
				prgn_ctx[i].chn_attr.unchn_attr.cover_ex_chn.layer);
		}
	}

	// Region status of overlayex
	seq_puts(m, "\n------REGION STATUS OF OVERLAYEX------------------------------------------\n");
	seq_printf(m, "%10s%10s%10s%20s%10s%10s%10s%20s%20s%10s%10s\n",
		"Hdl", "Type", "Used", "PiFmt", "W", "H", "BgColor", "Phy", "Virt", "Stride", "CnvsNum");

	for (i = 0; i < RGN_MAX_NUM; ++i) {
		if (prgn_ctx[i].region.type == OVERLAYEX_RGN && prgn_ctx[i].created) {
			memset(c, 0, sizeof(c));
			_pix_fmt_to_string(prgn_ctx[i].region.unattr.overlay_ex.pixel_format, c, sizeof(c));

			seq_printf(m, "%7s%3d%10d%10s%20s%10d%10d%10x%20llx%20lx%10d%10d\n",
				"#",
				prgn_ctx[i].handle,
				prgn_ctx[i].region.type,
				(prgn_ctx[i].used) ? "Y" : "N",
				c,
				prgn_ctx[i].region.unattr.overlay_ex.size.width,
				prgn_ctx[i].region.unattr.overlay_ex.size.height,
				prgn_ctx[i].region.unattr.overlay_ex.bg_color,
				prgn_ctx[i].canvas_info[prgn_ctx[i].canvas_idx].phy_addr,
				(uintptr_t)prgn_ctx[i].canvas_info[prgn_ctx[i].canvas_idx].virt_addr,
				prgn_ctx[i].canvas_info[prgn_ctx[i].canvas_idx].stride,
				prgn_ctx[i].region.unattr.overlay_ex.canvas_num);
		}
	}

	// Region chn status of overlayex
	seq_puts(m, "\n------REGION CHN STATUS OF OVERLAYEX--------------------------------------\n");
	seq_printf(m, "%10s%10s%10s%10s%10s%10s%10s%10s%10s\n",
		"Hdl", "Type", "Mod", "Dev", "Chn", "bShow", "X", "Y", "Layer");

	for (i = 0; i < RGN_MAX_NUM; ++i) {
		if (prgn_ctx[i].region.type == OVERLAYEX_RGN && prgn_ctx[i].created && prgn_ctx[i].used) {
			seq_printf(m, "%7s%3d%10d%10s%10d%10d%10s%10d%10d%10d\n",
				"#",
				prgn_ctx[i].handle,
				prgn_ctx[i].region.type,
				MOD_STRING[prgn_ctx[i].chn.mod_id],
				prgn_ctx[i].chn.dev_id,
				prgn_ctx[i].chn.chn_id,
				(prgn_ctx[i].chn_attr.show) ? "Y" : "N",
				prgn_ctx[i].chn_attr.unchn_attr.overlay_ex_chn.point.x,
				prgn_ctx[i].chn_attr.unchn_attr.overlay_ex_chn.point.y,
				prgn_ctx[i].chn_attr.unchn_attr.overlay_ex_chn.layer);
		}
	}

	return 0;
}

static int rgn_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, rgn_proc_show, PDE_DATA(inode));
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
static const struct proc_ops rgn_proc_fops = {
	.proc_open = rgn_proc_open,
	.proc_read = seq_read,
	.proc_lseek = seq_lseek,
	.proc_release = single_release,
};
#else
static const struct file_operations rgn_proc_fops = {
	.owner = THIS_MODULE,
	.open = rgn_proc_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};
#endif

int rgn_proc_init(void)
{
	int ret = 0;

	/* create the /proc file */
	if (proc_create_data(RGN_PROC_NAME, 0644, NULL, &rgn_proc_fops, NULL) == NULL) {
		pr_err("rgn proc creation failed\n");
		ret = -EINVAL;
	}

	return ret;
}

int rgn_proc_remove(void)
{
	remove_proc_entry(RGN_PROC_NAME, NULL);

	return 0;
}
