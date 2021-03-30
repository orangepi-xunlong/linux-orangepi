/*
 * sunxi mipi bsp interface
 * Author:wangxuan
 */
#include "bsp_mipi_csi.h"
#include "../utility/vin_io.h"
#define MAX_CSI 2
volatile void __iomem *addr;
static unsigned int glb_mipicsi2_version[MAX_CSI];

static void mipi_csi2_reg_default(unsigned int sel)
{
	vin_reg_writel(addr + 0x004, 0);
	vin_reg_writel(addr + 0x004, 0xb8c39bec);
	vin_reg_writel(addr + 0x008, 0);
	vin_reg_writel(addr + 0x008, 0xb8d257f8);
	vin_reg_writel(addr + 0x010, 0);
	vin_reg_writel(addr + 0x010, 0xb8df698e);
	vin_reg_writel(addr + 0x018, 0);
	vin_reg_writel(addr + 0x018, 0xb8c8a30c);
	vin_reg_writel(addr + 0x01c, 0);
	vin_reg_writel(addr + 0x01c, 0xb8df8ad7);
	vin_reg_writel(addr + 0x028, 0);
	vin_reg_writel(addr + 0x02c, 0);
	vin_reg_writel(addr + 0x030, 0);
	vin_reg_writel(addr + 0x008, 0);
	vin_reg_writel(addr + 0x104, 0);
	vin_reg_writel(addr + 0x10c, 0);
	vin_reg_writel(addr + 0x110, 0);
	vin_reg_writel(addr + 0x100, 0);
	vin_reg_writel(addr + 0x100, 0xb8c64f24);
}

static void mipi_dphy_init(unsigned int sel)
{
	vin_reg_writel(addr + 0x010, 0x00000000);
	vin_reg_writel(addr + 0x010, 0x00000000);
	vin_reg_writel(addr + 0x010, 0x00000100);
	vin_reg_writel(addr + 0x010, 0x00000100);
	vin_reg_writel(addr + 0x010, 0x00000100);
	vin_reg_writel(addr + 0x010, 0x00000100);

	vin_reg_writel(addr + 0x010, 0x80000100);
	vin_reg_writel(addr + 0x010, 0x80008100);
	vin_reg_writel(addr + 0x010, 0x80008000);
	vin_reg_writel(addr + 0x030, 0xa0200000);
}

static void mipi_ctl_init(unsigned int sel)
{
	vin_reg_writel(addr + 0x004, 0x80000000);
	vin_reg_writel(addr + 0x100, 0x12200000);
}

static void mipi_ctl_enable(unsigned int sel)
{
	vin_reg_writel(addr + 0x100, vin_reg_readl(addr + 0x100) | 0x80000000);
}

static void mipi_ctl_disable(unsigned int sel)
{
	vin_reg_writel(addr + 0x100, vin_reg_readl(addr + 0x100) & 0x7fffffff);
}

static void mipi_s_lane(unsigned int sel, unsigned char lane_num)
{
	vin_reg_writel(addr + 0x100,
		       vin_reg_readl(addr + 0x100) | ((lane_num - 1) << 4));
}

static void mipi_s_ch_num(unsigned int sel, unsigned char ch_num)
{
	vin_reg_writel(addr + 0x100,
		       vin_reg_readl(addr + 0x100) | ((ch_num - 1) << 16));
}

static void mipi_s_pkt(unsigned int sel, unsigned char ch, unsigned char vc,
		       enum pkt_fmt mipi_pkt_fmt)
{
	switch (ch) {
	case 0:
		vin_reg_writel(addr + 0x104,
			       vin_reg_readl(addr + 0x104) | (vc << 6));
		vin_reg_writel(addr + 0x104,
			       vin_reg_readl(addr + 0x104) | mipi_pkt_fmt);
		break;
	case 1:
		vin_reg_writel(addr + 0x104,
			       vin_reg_readl(addr + 0x104) | (vc << 14));
		vin_reg_writel(addr + 0x104,
			       vin_reg_readl(addr +
					     0x104) | (mipi_pkt_fmt << 8));
		break;
	case 2:
		vin_reg_writel(addr + 0x104,
			       vin_reg_readl(addr + 0x104) | (vc << 22));
		vin_reg_writel(addr + 0x104,
			       vin_reg_readl(addr +
					     0x104) | (mipi_pkt_fmt << 16));
		break;
	case 3:
		vin_reg_writel(addr + 0x104,
			       vin_reg_readl(addr + 0x104) | (vc << 30));
		vin_reg_writel(addr + 0x104,
			       vin_reg_readl(addr +
					     0x104) | (mipi_pkt_fmt << 24));
		break;
	default:
		vin_reg_writel(addr + 0x104,
			       vin_reg_readl(addr + 0x104) | 0xc0804000);
		break;
	}
}

void bsp_mipi_csi_set_version(unsigned int sel, unsigned int ver)
{
	glb_mipicsi2_version[sel] = ver;
}

int bsp_mipi_csi_set_base_addr(unsigned int sel, unsigned long addr_base)
{
	addr = (volatile void __iomem *)addr_base;
	return 0;
}

int bsp_mipi_dphy_set_base_addr(unsigned int sel, unsigned long addr_base)
{
	return 0;
}

void bsp_mipi_csi_dphy_init(unsigned int sel)
{
	mipi_csi2_reg_default(sel);
	mipi_dphy_init(sel);
	mipi_ctl_init(sel);
}

void bsp_mipi_csi_dphy_exit(unsigned int sel)
{
}

void bsp_mipi_csi_dphy_enable(unsigned int sel)
{
}

void bsp_mipi_csi_dphy_disable(unsigned int sel)
{
}

void bsp_mipi_csi_protocol_enable(unsigned int sel)
{
	mipi_ctl_enable(sel);
}

void bsp_mipi_csi_protocol_disable(unsigned int sel)
{
	mipi_ctl_disable(sel);
}

static void bsp_mipi_csi_set_dphy_timing(unsigned int sel,
					 unsigned int *mipi_bps,
					 unsigned int dphy_clk,
					 unsigned int mode)
{

}

static void bsp_mipi_csi_set_lane(unsigned int sel, unsigned char lane_num)
{
	mipi_s_lane(sel, lane_num);
}

static void bsp_mipi_csi_set_total_ch(unsigned int sel, unsigned char ch_num)
{
	mipi_s_ch_num(sel, ch_num);
}

static void bsp_mipi_csi_set_pkt_header(unsigned int sel, unsigned char ch,
					unsigned char vc,
					enum pkt_fmt mipi_pkt_fmt)
{
	mipi_s_pkt(sel, ch, vc, mipi_pkt_fmt);
}

void bsp_mipi_csi_set_para(unsigned int sel, struct mipi_para *para)
{
	bsp_mipi_csi_set_dphy_timing(sel, &para->bps, para->dphy_freq,
				     para->auto_check_bps);
	bsp_mipi_csi_set_lane(sel, para->lane_num);
	bsp_mipi_csi_set_total_ch(sel, para->total_rx_ch);
}

void bsp_mipi_csi_set_fmt(unsigned int sel, unsigned int total_rx_ch,
			  struct mipi_fmt_cfg *fmt)
{
	unsigned int i;

	for (i = 0; i < total_rx_ch; i++)
		bsp_mipi_csi_set_pkt_header(sel, i, fmt->vc[i],
					    fmt->packet_fmt[i]);
}
