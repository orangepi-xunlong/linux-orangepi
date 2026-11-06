#ifndef _MESH_H
#define _MESH_H

#include <ldc_cb.h>

#define GDC_MAGIC 0xbabeface
#define GDC_MESH_SIZE_ROT 0x60000

struct _gdc_cb_param {
	mmf_chn_s chn;
	enum gdc_usage usage;
};

int mesh_gdc_do_op(struct ldc_vdev *wdev, enum gdc_usage usage,
		   const void *usage_param, struct vb_s *vb_in,
		   pixel_format_e pix_format, unsigned long long mesh_addr, unsigned char sync_io,
		   void *cb_param, unsigned int cb_param_size, mod_id_e mod_id,
		   rotation_e rotation);

#endif /* _MESH_H */
