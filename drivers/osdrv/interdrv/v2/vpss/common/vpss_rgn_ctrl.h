#ifndef _VPSS_RGN_CTRL_H_
#define _VPSS_RGN_CTRL_H_

s32 vpss_get_rgn_hdls(vpss_grp vpss_grp, vpss_chn vpss_chn, u8 layer,
	rgn_type_e type, rgn_handle hdls[]);
s32 vpss_set_rgn_hdls(vpss_grp vpss_grp, vpss_chn vpss_chn, u32 layer,
		rgn_type_e type, rgn_handle hdls[]);
s32 vpss_set_rgn_cfg(vpss_grp vpss_grp, vpss_chn vpss_chn, u32 layer, struct rgn_cfg *cfg);
s32 vpss_set_rgn_coverex_cfg(vpss_grp vpss_grp, vpss_chn vpss_chn, struct rgn_coverex_cfg *cfg);
s32 vpss_set_rgn_mosaic_cfg(vpss_grp vpss_grp, vpss_chn vpss_chn, struct rgn_mosaic_cfg *cfg);
s32 vpss_get_rgn_ow_addr(vpss_grp vpss_grp, vpss_chn vpss_chn, u32 layer,
	rgn_handle handle, u64 *addr);
s32 vpss_set_rgn_lut_cfg(vpss_grp vpss_grp, vpss_chn vpss_chn, struct rgn_lut_cfg *cfg);

#endif /* _VPSS_RGN_CTRL_H_ */
