#ifndef __CVITEK_MIPIPLL_H__
#define __CVITEK_MIPIPLL_H__

#include <drm/drm_print.h>

void mipi_dphy_set_pll(u8 dsi_id, u32 clkkHz, u8 lane, u8 bits);
void dphy_lvds_set_pll(u8 lvds_id, u32 clkkHz, u8 link);

#endif /* __CVITEK_MIPIPLL_H__ */
