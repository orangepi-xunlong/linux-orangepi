#ifndef __CIF_CB_H__
#define __CIF_CB_H__

#ifdef __cplusplus
	extern "C" {
#endif

enum CIF_CB_CMD {
	CIF_CB_RESET_LVDS,
	CIF_CB_GET_CIF_ATTR,
	CIF_CB_MAX
};

struct cif_attr_s {
	unsigned int	devno;
	unsigned int	stagger_vsync;
};

#ifdef __cplusplus
}
#endif

#endif /* __CIF_CB_H__ */
