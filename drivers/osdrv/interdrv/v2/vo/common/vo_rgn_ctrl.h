#ifndef _VO_RGN_CTRL_H_
#define _VO_RGN_CTRL_H_

int vo_cb_get_rgn_hdls(vo_layer layer, rgn_type_e type, rgn_handle hdls[]);
int vo_cb_set_rgn_hdls(vo_layer layer, rgn_type_e type, rgn_handle hdls[]);
int vo_cb_set_rgn_cfg(vo_layer layer, struct rgn_cfg *cfg);
int vo_cb_get_chn_size(vo_layer layer, rect_s *rect);

#endif /* _VO_RGN_CTRL_H_ */
