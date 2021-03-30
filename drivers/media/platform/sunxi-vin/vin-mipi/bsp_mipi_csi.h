/*
 * sunxi mipi csi bsp header file
 * Author:raymonxiu
*/

#ifndef __MIPI__CSI__H__
#define __MIPI__CSI__H__

#include "protocol.h"
#include "../utility/bsp_common.h"
#define MAX_MIPI  1
#define MAX_MIPI_CH 4

struct mipi_para {
	unsigned int auto_check_bps;
	unsigned int bps;
	unsigned int dphy_freq;
	unsigned int lane_num;
	unsigned int total_rx_ch;
};

struct mipi_fmt_cfg {
	enum v4l2_field field[MAX_MIPI_CH];
	enum pkt_fmt packet_fmt[MAX_MIPI_CH];
	unsigned int vc[MAX_MIPI_CH];
};
extern void bsp_mipi_csi_set_version(unsigned int sel, unsigned int ver);
extern int bsp_mipi_csi_set_base_addr(unsigned int sel,
				      unsigned long addr_base);
extern int bsp_mipi_dphy_set_base_addr(unsigned int sel,
				       unsigned long addr_base);
extern void bsp_mipi_csi_dphy_init(unsigned int sel);
extern void bsp_mipi_csi_dphy_exit(unsigned int sel);
extern void bsp_mipi_csi_dphy_enable(unsigned int sel);
extern void bsp_mipi_csi_dphy_disable(unsigned int sel);
extern void bsp_mipi_csi_protocol_enable(unsigned int sel);
extern void bsp_mipi_csi_protocol_disable(unsigned int sel);
extern void bsp_mipi_csi_set_para(unsigned int sel, struct mipi_para *para);
extern void bsp_mipi_csi_set_fmt(unsigned int sel, unsigned int total_rx_ch,
				 struct mipi_fmt_cfg *fmt);
#endif /*__MIPI__CSI__H__*/
