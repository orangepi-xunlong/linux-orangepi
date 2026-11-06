#include <linux/clk.h>
#include <linux/component.h>
#include <linux/module.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/of_irq.h>
#include <linux/of_graph.h>
#include <video/mipi_display.h>
#include <linux/log2.h>

#include <drm/drm_atomic_helper.h>
#include <drm/drm_bridge.h>
#include <drm/drm_of.h>
#include <drm/drm_probe_helper.h>
#include <drm/drm_fb_helper.h>

#include "cvitek_vo_sys_reg.h"
#include "cvitek_drm.h"
#include "cvitek_disp.h"
#include "cvitek_dsi.h"
#include "cvitek_mipipll_cfg.h"

static u8 data_0_lane;

#define MIN(a, b) (((a) < (b))?(a):(b))
#define MAX(a, b) (((a) > (b))?(a):(b))
#define UPPER(x, y) (((x) + ((1 << (y)) - 1)) >> (y))   // for alignment

#define host_to_dsi(host) \
	container_of(host, struct cvitek_dsi, host)

#define ACK_WR       0x02
#define GEN_READ_LP  0x1A
#define GEN_READ_SP1 0x11
#define GEN_READ_SP2 0x12
#define DCS_READ_LP  0x1C
#define DCS_READ_SP1 0x21
#define DCS_READ_SP2 0x22

static void cvitek_dsi_encoder_mode_set(struct drm_encoder *encoder,
				      struct drm_display_mode *mode,
				 struct drm_display_mode *adj_mode)
{
	struct cvitek_dsi *dsi = encoder_to_dsi(encoder);

	DRM_DEBUG_DRIVER("---- enter cvitek dsi encoder mode set. ----\n");

	drm_display_mode_to_videomode(adj_mode, &dsi->ctx.vm);
}

static int mipi_check_mode(struct cvitek_dsi *dsi)
{
	struct dsi_context *ctx = &dsi->ctx;
	u32 vid_mode = MODE_NONE;

	if (dsi->slave->mode_flags & MIPI_DSI_MODE_VIDEO) {
		if (dsi->slave->mode_flags & MIPI_DSI_MODE_VIDEO_BURST)
			vid_mode = BURST_MODE;
		else if (dsi->slave->mode_flags & MIPI_DSI_MODE_VIDEO_SYNC_PULSE)
			vid_mode = NON_BURST_MODE_SYNC_PULSES;
		else
			vid_mode = NON_BURST_MODE_SYNC_EVENTS;
	}

	if (vid_mode != BURST_MODE)
		return -EINVAL;

	ctx->bits = mipi_dsi_pixel_format_to_bpp(dsi->slave->format);
	if (ctx->bits < 0)
		return -EINVAL;

	return 0;
}

static void mipi_dphy_power_on(u8 dsi_id)
{
	_reg_write(REG_DSI_PHY_PD(dsi_id), 0x0);
	_reg_write(REG_DSI_PHY_ESC_INIT(dsi_id), 0x100);
	_reg_write(REG_DSI_PHY_ESC_WAKE(dsi_id), 0x100);
	_reg_write(REG_DSI_PHY_EXT_GPIO(dsi_id), 0x0);
	_reg_write(REG_DSI_PHY_POWER_DOWN_CFG(dsi_id), 0x0);
	_reg_write(REG_DSI_PHY_LVDS_EN(dsi_id), 0x0);
}

static int mipi_dphy_set_lane(u8 dsi_id, u8 lane_num, enum mipi_tx_lane_id lane_id, bool pn_swap)
{
	if ((lane_num > LANE_MAX_NUM - 1) || (lane_id > MIPI_TX_LANE_MAX))
		return -1;

	_reg_write_mask(REG_DSI_PHY_LANE_SEL(dsi_id), 0x7 << (4 * lane_num), lane_id << (4 * lane_num));
	_reg_write_mask(REG_DSI_PHY_LANE_PN_SWAP(dsi_id), BIT(lane_num), pn_swap << lane_num);

	if (lane_id == MIPI_TX_LANE_CLK)
		_reg_write_mask(REG_DSI_PHY_LANE_SEL(dsi_id), 0x1f << 24, ((1 << 24) << lane_num));

	if (lane_id == MIPI_TX_LANE_0)
		data_0_lane = lane_num;

	return 0;
}

static void mipi_dphy_lane_en(u8 dsi_id, bool *data_en, bool preamble_en)
{
	u8 val = 0, i = 0;

	for (i = 0; i < LANE_MAX_NUM; ++i)
		val |= (data_en[i] << i);
	if (preamble_en)
		val |= 0x20;
	_reg_write_mask(REG_DSI_PHY_EN(dsi_id), 0x3f, val);
}

static int mipi_dphy_lane_init(struct cvitek_dsi *dsi)
{
	struct dsi_context *ctx = &dsi->ctx;
	bool lanes_en[LANE_MAX_NUM] = {false, false, false, false, false};
	bool preamble_on = false;
	int ret, i;

	for (i = 0; i < LANE_MAX_NUM; i++) {
		if ((ctx->lanes_map[i] < 0) || (ctx->lanes_map[i] >= MIPI_TX_LANE_MAX)) {
			ret = mipi_dphy_set_lane(ctx->dsi_id, i, MIPI_TX_LANE_MAX, false);
			if (ret) {
				drm_err(dsi->drm, "mipi_dphy_set_lane [%d] fail.\n", i);
				return ret;
			}
		} else {
			ret = mipi_dphy_set_lane(ctx->dsi_id, i, ctx->lanes_map[i], ctx->lanes_swap[i]);
			if (!ret) {
				lanes_en[ctx->lanes_map[i]] = true;
			} else {
				drm_err(dsi->drm, "mipi_dphy_set_lane [%d] fail.\n", i);
				return ret;
			}
		}
	}

	// preamble_on = (ctx->vm.pixelclock / 1000 * ctx->bits / ctx->data_lanes_num) > 1500000;

	mipi_dphy_lane_en(ctx->dsi_id, lanes_en, preamble_on);

	return 0;
}

static int mipi_dphy_config(u8 dsi_id, u8 lane_num, u8 bits, enum cvi_dsi_fmt fmt, u16 width)
{
	u32 val = 0;

	if ((lane_num != 1) && (lane_num != 2) && (lane_num != 4))
		return -EINVAL;
	if (fmt >= DSI_FORMAT_UNSUPPORT)
		return -EINVAL;

	lane_num >>= 1;
	val = (fmt << 30) | (lane_num << 24);
	_reg_write_mask(REG_DSI_HS_0(dsi_id), 0xc3000000, val);
	val = (width / 10) << 16 | UPPER(width * bits, 3);
	_reg_write(REG_DSI_HS_1(dsi_id), val);

	return 0;
}

/* convert from drm format to cvi format */
static u32 dsi_get_format(u32 pixel_format)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(dsi_formats); i++)
		if (dsi_formats[i].pixel_format == pixel_format)
			return dsi_formats[i].hw_format;

	/* not found */
	DRM_ERROR("Not found pixel format!!fourcc_format= %d\n",
		  pixel_format);
	return DSI_FORMAT_UNSUPPORT;
}

static void dsi_clr_mode(u8 dsi_id)
{
	u32 mode = _reg_read(REG_DSI_MAC_EN(dsi_id)) & 0x7;

	if (mode != DSI_MODE_IDLE)
		_reg_write_mask(REG_DSI_MAC_EN(dsi_id), 0x7, mode);
}

static int dsi_set_mode(u8 dsi_id, enum dsi_mode mode)
{
	if (mode >= DSI_MODE_MAX)
		return -1;

	if (mode == DSI_MODE_IDLE) {
		dsi_clr_mode(dsi_id);
		return 0;
	}

	_reg_write_mask(REG_DSI_MAC_EN(dsi_id), 0x7, mode);

	return 0;
}

static int dsi_chk_mode_done(u8 dsi_id, enum dsi_mode mode)
{
	u32 val = 0;

	if ((mode == DSI_MODE_ESC) || (mode == DSI_MODE_SPKT)) {
		val = _reg_read(REG_DSI_MAC_EN(dsi_id)) & 0x30;
		return (val ^ (mode << 4)) ? -1 : 0;
	}

	if ((mode == DSI_MODE_IDLE) || (mode == DSI_MODE_HS)) {
		val = _reg_read(REG_DSI_MAC_EN(dsi_id)) & 0x7;
		return (val == (mode)) ? 0 : -1;
	}

	return -1;
}

static void dphy_set_hs_settle(u8 dsi_id, u8 prepare, u8 zero, u8 trail)
{
	_reg_write_mask(REG_DSI_PHY_HS_CFG1(dsi_id), 0xffffff00, (trail << 24) | (zero << 16) | (prepare << 8));
}

static void mipi_tx_enable(u8 dsi_id)
{
	int ret = 0;
	int count = 0;

	dsi_set_mode(dsi_id, DSI_MODE_HS);
	do {
		usleep_range(1000, 2000);
		ret = dsi_chk_mode_done(dsi_id, DSI_MODE_HS);
	} while ((ret != 0) && (count++ < 20));
}

static void mipi_tx_disable(u8 dsi_id)
{
	int ret = 0;
	int count = 0;

	dsi_set_mode(dsi_id, DSI_MODE_IDLE);
	do {
		usleep_range(1000, 2000);
		ret = dsi_chk_mode_done(dsi_id, DSI_MODE_IDLE);
	} while ((ret != 0) && (count++ < 20));
}

static int mipi_dsi_init(struct cvitek_dsi *dsi)
{
	struct dsi_context *ctx = &dsi->ctx;
	int ret;

	ret = mipi_check_mode(dsi);
	if (ret) {
		drm_err(dsi->drm, "only support mode.\n");
		return ret;
	}

	mipi_tx_disable(ctx->dsi_id);

	mipi_dphy_power_on(ctx->dsi_id);

	ret = mipi_dphy_lane_init(dsi);
	if (ret) {
		drm_err(dsi->drm, "mipi_dphy_lane_init fail.\n");
		return ret;
	}

	mipi_dphy_set_pll(ctx->dsi_id, ctx->vm.pixelclock / 1000, ctx->data_lanes_num, ctx->bits);
	DRM_DEBUG_DRIVER("pixelclock(%lu), data_lanes(%u), bits(%d)\n", ctx->vm.pixelclock / 1000, ctx->data_lanes_num, ctx->bits);

	ret = mipi_dphy_config(ctx->dsi_id, ctx->data_lanes_num, ctx->bits, dsi_get_format(dsi->slave->format), ctx->vm.hactive);
	if (ret) {
		drm_err(dsi->drm, "mipi_dphy_config fail.\n");
		return ret;
	}

	return ret;
}

static int _dsi_chk_and_clean_mode(u8 dsi_id, enum dsi_mode mode)
{
	int i, ret;

	for (i = 0; i < 5; ++i) {
		udelay(20);
		ret = dsi_chk_mode_done(dsi_id, mode);
		if (ret == 0) {
			dsi_clr_mode(dsi_id);
			break;
		}
	}
	return ret;
}

static unsigned char ecc(unsigned char *data)
{
	char D0, D1, D2, D3, D4, D5, D6, D7, D8, D9, D10, D11, D12;
	char D13, D14, D15, D16, D17, D18, D19, D20, D21, D22, D23;
	char P0, P1, P2, P3, P4, P5, P6, P7;

	D0  = data[0] & 0x01;
	D1  = (data[0] >> 1) & 0x01;
	D2  = (data[0] >> 2) & 0x01;
	D3  = (data[0] >> 3) & 0x01;
	D4  = (data[0] >> 4) & 0x01;
	D5  = (data[0] >> 5) & 0x01;
	D6  = (data[0] >> 6) & 0x01;
	D7  = (data[0] >> 7) & 0x01;

	D8  = data[1] & 0x01;
	D9  = (data[1] >> 1) & 0x01;
	D10 = (data[1] >> 2) & 0x01;
	D11 = (data[1] >> 3) & 0x01;
	D12 = (data[1] >> 4) & 0x01;
	D13 = (data[1] >> 5) & 0x01;
	D14 = (data[1] >> 6) & 0x01;
	D15 = (data[1] >> 7) & 0x01;

	D16 = data[2] & 0x01;
	D17 = (data[2] >> 1) & 0x01;
	D18 = (data[2] >> 2) & 0x01;
	D19 = (data[2] >> 3) & 0x01;
	D20 = (data[2] >> 4) & 0x01;
	D21 = (data[2] >> 5) & 0x01;
	D22 = (data[2] >> 6) & 0x01;
	D23 = (data[2] >> 7) & 0x01;

	P7 = 0;
	P6 = 0;
	P5 = (D10^D11^D12^D13^D14^D15^D16^D17^D18^D19^D21^D22^D23) & 0x01;
	P4 = (D4^D5^D6^D7^D8^D9^D16^D17^D18^D19^D20^D22^D23) & 0x01;
	P3 = (D1^D2^D3^D7^D8^D9^D13^D14^D15^D19^D20^D21^D23) & 0x01;
	P2 = (D0^D2^D3^D5^D6^D9^D11^D12^D15^D18^D20^D21^D22) & 0x01;
	P1 = (D0^D1^D3^D4^D6^D8^D10^D12^D14^D17^D20^D21^D22^D23) & 0x01;
	P0 = (D0^D1^D2^D4^D5^D7^D10^D11^D13^D16^D20^D21^D22^D23) & 0x01;

	return (P7 << 7) | (P6 << 6) | (P5 << 5) | (P4 << 4) |
		(P3 << 3) | (P2 << 2) | (P1 << 1) | P0;
}

#define POLY 0x8408
static u16 crc16(unsigned char *data_p, unsigned short length)
{
	u8 i, data;
	u16 crc = 0xffff;

	if (length == 0)
		return (~crc);

	do {
		for (i = 0, data = 0xff & *data_p++; i < 8; i++, data >>= 1) {
			if ((crc & 0x0001) ^ (data & 0x0001))
				crc = (crc >> 1) ^ POLY;
			else
				crc >>= 1;
		}
	} while (--length);

	return crc;
}

#define dcs_delay 1

static void _esc_entry(u8 dsi_id)
{
	_reg_write(REG_DSI_PHY_DATA_OV(dsi_id), 0x01010101 << data_0_lane);
	udelay(dcs_delay);
	_reg_write(REG_DSI_PHY_DATA_OV(dsi_id), 0x01010001 << data_0_lane);
	udelay(dcs_delay);
	_reg_write(REG_DSI_PHY_DATA_OV(dsi_id), 0x00010001 << data_0_lane);
	udelay(dcs_delay);
	_reg_write(REG_DSI_PHY_DATA_OV(dsi_id), 0x00010101 << data_0_lane);
	udelay(dcs_delay);
	_reg_write(REG_DSI_PHY_DATA_OV(dsi_id), 0x00010001 << data_0_lane);
	udelay(dcs_delay);
}

static void _esc_exit(u8 dsi_id)
{
	_reg_write(REG_DSI_PHY_DATA_OV(dsi_id), 0x00010001 << data_0_lane);
	udelay(dcs_delay);
	_reg_write(REG_DSI_PHY_DATA_OV(dsi_id), 0x01010001 << data_0_lane);
	udelay(dcs_delay);
	_reg_write(REG_DSI_PHY_DATA_OV(dsi_id), 0x01010101 << data_0_lane);
	udelay(dcs_delay);
}

static void _esc_data(u8 dsi_id, u8 data)
{
	u8 i = 0;

	for (i = 0; i < 8; ++i) {
		_reg_write(REG_DSI_PHY_DATA_OV(dsi_id),
			   ((data & (1 << i)) ? 0x01010001 : 0x00010101) << data_0_lane);
		udelay(dcs_delay);
		_reg_write(REG_DSI_PHY_DATA_OV(dsi_id), 0x00010001 << data_0_lane);
		udelay(dcs_delay);
	}
}

void dpyh_mipi_tx_manual_packet(u8 dsi_id, const u8 *data, u8 count)
{
	u8 i = 0;

	_esc_entry(dsi_id);
	_esc_data(dsi_id, 0x87); // LPDT
	for (i = 0; i < count; ++i)
		_esc_data(dsi_id, data[i]);
	_esc_exit(dsi_id);
	_reg_write(REG_DSI_PHY_DATA_OV(dsi_id), 0x0);
}

static int dsi_short_packet(u8 dsi_id, const struct mipi_dsi_msg *msg)
{
	int ret = 0;
	u32 val = 0;
	const u8 *data = msg->tx_buf;

	val = msg->type;
	if (msg->tx_len == 2)
		val |= (data[0] << 8) | (data[1] << 16);
	else
		val |= data[0] << 8;

#if 1
	if (data[0] == 0x10 || data[0] == 0x28) //for reboot or Shutdown
		mipi_tx_disable(dsi_id);

	_reg_write_mask(REG_DSI_HS_0(dsi_id), 0x00ffffff, val);
	dsi_set_mode(dsi_id, DSI_MODE_SPKT);

	ret = _dsi_chk_and_clean_mode(dsi_id, DSI_MODE_SPKT);
	if (ret != 0)
		DRM_ERROR("_dsi_chk_and_clean_mode\n");
#else
	val |= (ecc((u8 *)&val) << 24);
	dpyh_mipi_tx_manual_packet(dsi_id, (u8 *)&val, 4);
#endif

	return ret;
}

/**
 * dsi_get_mode - get current dsi mode
 *
 * @return: current dsi mode
 */
static int dsi_get_mode(u8 inst)
{
	return (_reg_read(REG_DSI_MAC_EN(inst)) & 0x07);
}

static int dsi_read_packet(u8 dsi_id, const struct mipi_dsi_msg *msg)
{
	int ret = 0;
	u32 rx_data;
	int i = 0;
	u8 *data = msg->rx_buf;

	if (dsi_get_mode(dsi_id) == DSI_MODE_HS) {
		DRM_ERROR("[mipi_tx]: not work in HS.\n");
		return -1;
	}

	/**
	 * [2:0] reg_esc_mode
	 * [15:12] reg_bta_rx_bc
	 * [15:12] reg_bta_rx_bc
	 * [16:16] reg_tx_bc_over: TX LPDT transfer over,0: Extend to next trigger,1: Transfer over in this trigger
	 * only set necessery bits
	*/

	_reg_write_mask(REG_DSI_ESC(dsi_id), 0x07, 0x04);

	dsi_short_packet(dsi_id, msg);

	dsi_set_mode(dsi_id, DSI_MODE_ESC);
	ret = _dsi_chk_and_clean_mode(dsi_id, DSI_MODE_ESC);
	if (ret != 0) {
		DRM_ERROR("%s: dsi chk and clean mode failed\n", __func__);
		return ret;
	}

	// check result
	rx_data = _reg_read(REG_DSI_ESC_RX0(dsi_id));
	switch (rx_data & 0xff) {
	case GEN_READ_SP1:
	case DCS_READ_SP1:
		data[0] = (rx_data >> 8) & 0xff;
		break;
	case GEN_READ_SP2:
	case DCS_READ_SP2:
		data[0] = (rx_data >> 8) & 0xff;
		data[1] = (rx_data >> 16) & 0xff;
		break;
	case GEN_READ_LP:
	case DCS_READ_LP:
		rx_data = _reg_read(REG_DSI_ESC_RX1(dsi_id));
		for (i = 0; i < msg->rx_len; ++i)
			data[i] = (rx_data >> (i * 8)) & 0xff;
		break;
	case ACK_WR:
		DRM_ERROR("[mipi_tx]: dcs read, ack with error(%#x %#x).\n"
			, (rx_data >> 8) & 0xff, (rx_data >> 16) & 0xff);
		ret = -1;
		break;
	default:
		DRM_ERROR("[mipi_tx]: unknown DT, %#x.", rx_data);
		ret = -1;
		break;
	}

	return ret;
}

int dsi_long_packet_raw(u8 dsi_id, const u8 *data, u8 count)
{
	void __iomem *addr = REG_DSI_ESC_TX0(dsi_id);
	u32 val = 0;
	u8 i = 0, packet_count, data_offset = 0;
	int ret;
	// char str[128];

	// DRM_DEBUG_DRIVER("%s; count(%d)\n", __func__, count);
	while (count != 0) {
		if (count <= MAX_DSI_LP) {
			packet_count = count;
		} else if (count == MAX_DSI_LP + 1) {
			// [HW WorkAround] LPDT over can't take just one byte
			packet_count = MAX_DSI_LP - 1;
		} else {
			packet_count = MAX_DSI_LP;
		}
		count -= packet_count;
		val = 0x01 | ((packet_count - 1) << 8) | (count ? 0 : 0x10000);
		_reg_write(REG_DSI_ESC(dsi_id), val);
		// DRM_DEBUG_DRIVER("%s: esc reg(%#x)\n", __func__, val);

		// snprintf(str, 128, "%s: packet_count(%d) data(", __func__, packet_count);
		for (i = 0; i < packet_count; i += 4) {
			if (packet_count - i < 4) {
				val = 0;
				memcpy(&val, &data[data_offset], packet_count - i);
				data_offset += packet_count - i;
				_reg_write(addr + i, val);
				// snprintf(str + strlen(str), 128 - strlen(str), "%#x ", val);
				break;
			}
			memcpy(&val, &data[data_offset], 4);
			data_offset += 4;
			_reg_write(addr + i, val);
			// snprintf(str + strlen(str), 128 - strlen(str), "%#x ", val);
		}
		// DRM_DEBUG_DRIVER("%s)\n", str);

		dsi_set_mode(dsi_id, DSI_MODE_ESC);
		ret = _dsi_chk_and_clean_mode(dsi_id, DSI_MODE_ESC);
		if (ret != 0) {
			DRM_ERROR("%s: packet_count(%d) data0(%#x)\n", __func__, packet_count, data[0]);
			break;
		}
	}

	return ret;
}

int dsi_long_packet(u8 dsi_id, const struct mipi_dsi_msg *msg)
{
	u8 count = msg->tx_len;
	const u8 *data = msg->tx_buf;

	u8 packet[128] = {msg->type, count & 0xff, count >> 8, 0};
	u16 crc;

	if (count > 128 - 6) {
		pr_err("%s: count(%d) invalid\n", __func__, count);
		return -1;
	}

	packet[3] = ecc(packet);
	memcpy(&packet[4], data, count);
	count += 4;
	crc = crc16(packet, count);
	packet[count++] = crc & 0xff;
	packet[count++] = crc >> 8;

#if 1
	return dsi_long_packet_raw(dsi_id, packet, count);
#else
	dpyh_mipi_tx_manual_packet(dsi_id, packet, count);
	return 0;
#endif
}

static void cvitek_dsi_encoder_enable(struct drm_encoder *encoder)
{
	struct cvitek_dsi *dsi = encoder_to_dsi(encoder);
	struct dsi_context *ctx = &dsi->ctx;
	int ret;

	DRM_DEBUG_DRIVER("---- enter cvitek dsi encoder enable. ----\n");

	// clk_prepare_enable(ctx->dsi_clk);

	ret = mipi_dsi_init(dsi);
	if (ret) {
		drm_err(dsi->drm, "failed to init mipi dsi\n");
		return;
	}

	if (dsi->panel) {
		ret = drm_panel_prepare(dsi->panel);
		if (ret) {
			drm_err(dsi->drm, "failed to prepare panel\n");
			return;
		}
		ret = drm_panel_enable(dsi->panel);
		if (ret) {
			drm_err(dsi->drm, "failed to enable panel\n");
			return;
		}
	}

	dphy_set_hs_settle(ctx->dsi_id, 6, 32, 1);
	mipi_tx_enable(ctx->dsi_id);
}

static void cvitek_dsi_encoder_disable(struct drm_encoder *encoder)
{
	struct cvitek_dsi *dsi = encoder_to_dsi(encoder);
	struct dsi_context *ctx = &dsi->ctx;
	int ret;

	DRM_DEBUG_DRIVER("---- enter cvitek dsi encoder disable. ----\n");

	mipi_tx_disable(ctx->dsi_id);
	// clk_disable_unprepare(ctx->dsi_clk);

	if (dsi->panel) {
		ret = drm_panel_disable(dsi->panel);
		if (ret) {
			drm_err(dsi->drm, "failed to disable panel\n");
			return;
		}
		ret = drm_panel_unprepare(dsi->panel);
		if (ret) {
			drm_err(dsi->drm, "failed to unprepare panel\n");
			return;
		}
	}
}

static const struct drm_encoder_helper_funcs cvitek_encoder_helper_funcs = {
	.mode_set	= cvitek_dsi_encoder_mode_set,
	.enable		= cvitek_dsi_encoder_enable,
	.disable	= cvitek_dsi_encoder_disable
};

static const struct drm_encoder_funcs cvitek_encoder_funcs = {
	.destroy = drm_encoder_cleanup,
};

static int cvitek_dsi_encoder_init(struct cvitek_dsi *dsi,
				 struct device *dev)
{
	struct drm_encoder *encoder = &dsi->encoder;
	u32 crtc_mask;
	int ret;

	DRM_DEBUG_DRIVER("---- enter cvitek dsi encoder init. ----\n");

	crtc_mask = drm_of_find_possible_crtcs(dsi->drm, dev->of_node);
	if (!crtc_mask) {
		drm_err(dsi->drm, "failed to find crtc mask\n");
		return -EINVAL;
	}

	drm_dbg(dsi->drm, "find possible crtcs: 0x%08x\n", crtc_mask);

	encoder->possible_crtcs = crtc_mask;
	ret = drm_encoder_init(dsi->drm, encoder, &cvitek_encoder_funcs,
			       DRM_MODE_ENCODER_DSI, NULL);
	if (ret) {
		drm_err(dsi->drm, "failed to init dsi encoder\n");
		return ret;
	}

	drm_encoder_helper_add(encoder, &cvitek_encoder_helper_funcs);

	return 0;
}

static void _cal_htt_extra(struct drm_display_mode *mode, int lane_num, int bits)
{
	unsigned short htt_old, htt_new, htt_new_extra;
	unsigned short vtt;
	unsigned long long fps;
	unsigned long long bit_rate_MHz;
	unsigned long long clk_hs_MHz;
	unsigned long long clk_hs_ns;
	unsigned long long line_rate_KHz, line_time_us;
	unsigned long long over_head;
	unsigned long long t_period_max, t_period_real;

	htt_old = mode->htotal;
	vtt = mode->vtotal;
	fps = (unsigned long long)mode->clock * 1000 / (htt_old * vtt / 1024); // fps 1024
	bit_rate_MHz = (unsigned long long) mode->clock  * 1024 * bits / (lane_num * 1000);//bit_rate_MHz 1024
	clk_hs_MHz = bit_rate_MHz / 2;
	clk_hs_ns = 1000 * 1024 * 1024 / clk_hs_MHz;//clk_hs_ns 1024
	line_rate_KHz = vtt * fps / 1000;//line_rate_KHz 1024
	line_time_us = 1000 * 1024 * 1024 / line_rate_KHz;//line_time_us 1024
	over_head = (3 * 50 * 2 * 3) + clk_hs_ns * 360 / 1024;
	t_period_max = line_time_us * 1000 / 1024 - over_head;
	t_period_real = clk_hs_ns * mode->hdisplay * bits / 4 / 2 / 1024;
	htt_new = htt_old * t_period_real / t_period_max;
	if (htt_new > htt_old) {
		if (htt_new & 0x0003)
			htt_new += (4 - (htt_new & 0x0003));
		htt_new_extra = htt_new - htt_old;
		mode->hsync_start += htt_new_extra;
		mode->hsync_end += htt_new_extra;
		mode->htotal += htt_new_extra;
		mode->clock = htt_new * vtt * fps / 1000 / 1024;
	}
}

static int cvitek_dsi_get_modes(struct drm_connector *connector)
{
	struct cvitek_dsi *dsi = connector_to_dsi(connector);
	struct drm_display_mode *mode;
	struct list_head *pos;
	int ret;

	DRM_DEBUG_DRIVER("---- enter cvitek dsi get modes. ----\n");

	ret = drm_panel_get_modes(dsi->panel, connector);
	if (!ret) {
		drm_err(dsi->drm, "failed to drm_panel_get_modes\n");
		return ret;
	}
	list_for_each(pos, &connector->probed_modes) {
		mode = list_entry(pos, struct drm_display_mode, head);
		_cal_htt_extra(mode, dsi->ctx.data_lanes_num, dsi->ctx.bits);
	}
	return 1;
}

static int cvitek_dsi_atomic_check(struct drm_connector *connector,
				 struct drm_atomic_state *state)
{
	struct cvitek_dsi *dsi = connector_to_dsi(connector);
	struct drm_connector_state *new_state;
	struct drm_crtc_state *new_crtc_state;
	struct drm_connector_state *old_state;

	DRM_DEBUG_DRIVER("---- enter cvitek dsi atomic check. ----\n");

	new_state = drm_atomic_get_new_connector_state(state, connector);
	if (!new_state->crtc)
		return 0;

	old_state = drm_atomic_get_old_connector_state(state, connector);
	new_crtc_state = drm_atomic_get_new_crtc_state(state, new_state->crtc);

	if (old_state->tv.mode != new_state->tv.mode ||
	    old_state->tv.margins.left != new_state->tv.margins.left ||
	    old_state->tv.margins.right != new_state->tv.margins.right ||
	    old_state->tv.margins.top != new_state->tv.margins.top ||
	    old_state->tv.margins.bottom != new_state->tv.margins.bottom || dsi->ctx.connectors_changed) {
		/* Force a modeset. */
		if (dsi->ctx.connectors_changed)
			dsi->ctx.connectors_changed = false;

		new_crtc_state->connectors_changed = true;
	}

	return 0;
}

static const struct drm_connector_helper_funcs cvitek_dsi_connector_helper_funcs = {
	.get_modes	= cvitek_dsi_get_modes,
	.atomic_check = cvitek_dsi_atomic_check,
};

static enum drm_connector_status
cvitek_dsi_connector_detect(struct drm_connector *connector, bool force)
{
	struct cvitek_dsi *dsi = connector_to_dsi(connector);

	DRM_DEBUG_DRIVER("---- enter cvitek dsi connector detect. ----\n");

	return dsi->panel ? connector_status_connected :
						connector_status_disconnected;
}

static const struct drm_connector_funcs cvitek_dsi_connector_funcs = {
	.detect			= cvitek_dsi_connector_detect,
	.fill_modes		= drm_helper_probe_single_connector_modes,
	.destroy		= drm_connector_cleanup,
	.reset			= drm_atomic_helper_connector_reset,
	.atomic_duplicate_state	= drm_atomic_helper_connector_duplicate_state,
	.atomic_destroy_state	= drm_atomic_helper_connector_destroy_state,
};

static int cvitek_dsi_connector_init(struct cvitek_dsi *dsi,
				 struct device *dev)
{
	struct drm_encoder *encoder = &dsi->encoder;
	struct drm_connector *connector = &dsi->connector;
	int ret;

	DRM_DEBUG_DRIVER("---- enter cvitek dsi connector init. ----\n");

	ret = drm_connector_init(dsi->drm, connector,
				 &cvitek_dsi_connector_funcs,
				 DRM_MODE_CONNECTOR_DSI);
	if (ret) {
		drm_err(dsi->drm, "failed to init dsi connector\n");
		return ret;
	}

	drm_connector_helper_add(connector,
				 &cvitek_dsi_connector_helper_funcs);

	drm_connector_attach_encoder(connector, encoder);

	connector->status = connector_status_disconnected;

	return 0;
}

// for exp:
// clock-lane = <2>;				/* Tx PAD 2 as clk lane */
// data-lanes = <0 1 3 4>;			/* Tx PAD 0 as d0 , Tx PAD 1 as d1 ,Tx PAD 3 as d2 ,Tx PAD 4 as d3 ,*/
// lanes-swap = <0 0 0 0 0>;		/* pn swap 0 = false, 1 = true */
static int dsi_host_parse_lane_data(struct dsi_context *ctx,
				    struct device_node *np)
{
	struct property *prop;
	int ret, i, len, num_lanes;
	u32 clock_lane;
	u32 data_lanes[LANE_MAX_NUM - 1];

	s32 lanes_map_default[LANE_MAX_NUM] = {MIPI_TX_LANE_0, MIPI_TX_LANE_1, MIPI_TX_LANE_CLK, MIPI_TX_LANE_2, MIPI_TX_LANE_3};
	s32 lanes_map[LANE_MAX_NUM] = {-1, -1, -1, -1, -1};
	u32 lanes_swap[LANE_MAX_NUM] = {false, false, false, false, false};

	prop = of_find_property(np, "clock-lane", &len);
	if (!prop) {
		DRM_DEBUG_DRIVER("failed to find clock-lanes lane mapping, using default\n");
		goto default_lanes_maping;
	}

	num_lanes = len / sizeof(u32);

	if (num_lanes != CLK_LANE_NUM) {
		DRM_ERROR("bad number of clock lanes\n");
		return -EINVAL;
	}

	ret = of_property_read_u32(np, "clock-lane", &clock_lane);
	if (ret) {
		DRM_ERROR("failed to read lane data\n");
		return ret;
	}

	lanes_map[clock_lane] = MIPI_TX_LANE_CLK;

	prop = of_find_property(np, "data-lanes", &len);
	if (!prop) {
		DRM_DEBUG_DRIVER("failed to find data lane mapping, using default\n");
		goto default_lanes_maping;
	}

	num_lanes = len / sizeof(u32);

	if (num_lanes < 1 || num_lanes > LANE_MAX_NUM - CLK_LANE_NUM) {
		DRM_ERROR("bad number of data lanes\n");
		return -EINVAL;
	}

	ret = of_property_read_u32_array(np, "data-lanes", data_lanes,
					 num_lanes);
	if (ret) {
		DRM_ERROR("failed to read lane data\n");
		return ret;
	}

	for (i = 0; i < num_lanes; i++)
		lanes_map[data_lanes[i]] = MIPI_TX_LANE_0 + i;

	ctx->data_lanes_num = num_lanes;

	prop = of_find_property(np, "lanes-swap", &len);
	if (!prop) {
		DRM_DEBUG_DRIVER("failed to find data lane swap, using default\n");
		goto default_lanes_maping;
	}

	num_lanes = len / sizeof(u32);

	if (num_lanes != LANE_MAX_NUM) {
		DRM_ERROR("bad number of data lanes\n");
		return -EINVAL;
	}

	ret = of_property_read_u32_array(np, "lanes-swap", lanes_swap,
					 num_lanes);
	if (ret) {
		DRM_ERROR("failed to read lane data\n");
		return ret;
	}

	for (i = 0; i < LANE_MAX_NUM; i++) {
		ctx->lanes_map[i] = lanes_map[i];
		ctx->lanes_swap[i] = lanes_swap[i];
	}

	return 0;

default_lanes_maping:
	for (i = 0; i < LANE_MAX_NUM; i++) {
		ctx->lanes_map[i] = lanes_map_default[i];
		ctx->lanes_swap[i] = lanes_swap[i];
	}

	return 0;
}

static int cvitek_dsi_context_init(struct cvitek_dsi *dsi,
				 struct device *dev)
{
	// struct platform_device *pdev = to_platform_device(dev);
	// struct resource *res;
	int ret;
	struct dsi_context *ctx = &dsi->ctx;
	struct dsi_match_data *dsi_data;

	DRM_DEBUG_DRIVER("---- enter cvitek dsi context init. ----\n");

	dsi_data = (struct dsi_match_data *)of_device_get_match_data(dev);
	if (!dsi_data)
		return -EINVAL;

	ctx->dsi_id = dsi_data->dsi_id;

#if 0
		res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
		if (!res) {
			dev_err(dev, "failed to get I/O resource\n");
			return -EINVAL;
		}

		ctx->base = devm_ioremap(dev, res->start, resource_size(res));
		if (!ctx->base) {
			drm_err(dsi->drm, "failed to map dsi host registers\n");
			return -ENXIO;
		}
		DRM_DEBUG_DRIVER("(%d) res-reg: start: 0x%llx, end: 0x%llx, virt-addr(%p).\n",
			i, res->start, res->end, ctx->base);

	ctx->dsi_clk = devm_clk_get(dev, "clk_dsi");
	if (IS_ERR(ctx->dsi_clk)) {
		DRM_ERROR("failed to parse clk dsi\n");
		return ERR_PTR(-ENODEV);
	}
	DRM_DEBUG_DRIVER("dsi_clk: %ldkhz.\n", clk_get_rate(ctx->dsi_clk) / 1000);
#endif

	ret = dsi_host_parse_lane_data(ctx, dev->of_node);
	if (ret) {
		DRM_ERROR("failed to parse lane data\n");
		return ret;
	}

	DRM_DEBUG_DRIVER("data lanes: %d %d %d %d %d.	lanes swap: %d %d %d %d %d.\n",
				ctx->lanes_map[0], ctx->lanes_map[1], ctx->lanes_map[2], ctx->lanes_map[3], ctx->lanes_map[4],
				ctx->lanes_swap[0], ctx->lanes_swap[1], ctx->lanes_swap[2], ctx->lanes_swap[3], ctx->lanes_swap[4]);

	return 0;
}

static int cvitek_dsi_host_attach(struct mipi_dsi_host *host,
				struct mipi_dsi_device *slave)
{
	struct cvitek_dsi *dsi = host_to_dsi(host);
	struct dsi_context *ctx = &dsi->ctx;
	struct drm_panel *panel;

	int ret = drm_of_find_panel_or_bridge(host->dev->of_node, 1, 0,
					  &panel, NULL);
	if (ret)
		return ret;

	DRM_DEBUG_DRIVER("---- enter cvitek dsi host attach. ----\n");

	dsi->slave = slave;
	dsi->panel = panel;

	if (ctx->data_lanes_num != slave->lanes) {
		DRM_ERROR("dts's data lanes num[%d] != panel's data lanes num[%d]\n",
		 ctx->data_lanes_num, slave->lanes);
		return -EINVAL;
	}

	ctx->bits = mipi_dsi_pixel_format_to_bpp(slave->format);
	if (ctx->bits < 0) {
		DRM_ERROR("ctx->bits < 0\n");
		return -EINVAL;
	}

	dsi->ctx.connectors_changed = true;

	drm_kms_helper_hotplug_event(dsi->drm);

	return 0;
}

static int cvitek_dsi_host_detach(struct mipi_dsi_host *host,
				struct mipi_dsi_device *slave)
{
	struct cvitek_dsi *dsi = host_to_dsi(host);

	DRM_DEBUG_DRIVER("---- enter cvitek dsi host detach. ----\n");

	drm_kms_helper_hotplug_event(dsi->drm);

	dsi->panel = NULL;

	return 0;
}

static ssize_t cvitek_dsi_host_transfer(struct mipi_dsi_host *host,
				      const struct mipi_dsi_msg *msg)
{
	struct cvitek_dsi *dsi = host_to_dsi(host);
	int ret;

	DRM_DEBUG_DRIVER("---- enter cvitek dsi host transfer. ----\n");

	switch (msg->type) {
	case MIPI_DSI_DCS_SHORT_WRITE:
	case MIPI_DSI_DCS_SHORT_WRITE_PARAM:
		ret = dsi_short_packet(dsi->ctx.dsi_id, msg);
		break;

	case MIPI_DSI_DCS_LONG_WRITE:
		ret = dsi_long_packet(dsi->ctx.dsi_id, msg);
		break;

	case MIPI_DSI_DCS_READ:
		mipi_dphy_set_pll(dsi->ctx.dsi_id, dsi->ctx.vm.pixelclock / 500, dsi->ctx.data_lanes_num, dsi->ctx.bits);
		udelay(1000);

		ret = dsi_read_packet(dsi->ctx.dsi_id, msg);

		mipi_dphy_set_pll(dsi->ctx.dsi_id, dsi->ctx.vm.pixelclock / 1000, dsi->ctx.data_lanes_num, dsi->ctx.bits);
		udelay(1000);
		break;
	default:
		ret = -EINVAL;
	}

	return ret;
}

static const struct mipi_dsi_host_ops cvitek_dsi_host_ops = {
	.attach = cvitek_dsi_host_attach,
	.detach = cvitek_dsi_host_detach,
	.transfer = cvitek_dsi_host_transfer,
};

static int cvitek_dsi_bind(struct device *dev, struct device *master, void *data)
{
	struct drm_device *drm = data;
	struct cvitek_dsi *dsi = dev_get_drvdata(dev);
	int ret;

	DRM_DEBUG_DRIVER("---- enter cvitek dsi bind. ----\n");

	dsi = devm_kzalloc(dev, sizeof(*dsi), GFP_KERNEL);
	if (!dsi)
		return -ENOMEM;

	dsi->drm = drm;
	dsi->host.ops = &cvitek_dsi_host_ops;
	dsi->host.dev = dev;

	dev_set_drvdata(dev, dsi);

	ret = cvitek_dsi_context_init(dsi, dev);
	if (ret)
		return ret;

	ret = mipi_dsi_host_register(&dsi->host);
	if (ret)
		return ret;

	ret = cvitek_dsi_encoder_init(dsi, dev);
	if (ret)
		return ret;

	ret = cvitek_dsi_connector_init(dsi, dev);
	if (ret)
		return ret;

	return 0;
}

static void cvitek_dsi_unbind(struct device *dev,
			    struct device *master, void *data)
{
	struct cvitek_dsi *dsi = dev_get_drvdata(dev);

	DRM_DEBUG_DRIVER("---- enter cvitek dsi unbind. ----\n");

	mipi_dsi_host_unregister(&dsi->host);
}

static const struct component_ops dsi_component_ops = {
	.bind	= cvitek_dsi_bind,
	.unbind	= cvitek_dsi_unbind,
};

struct dsi_match_data cv186x_dsi0 = {
	.dsi_id = 0,
};

struct dsi_match_data cv186x_dsi1 = {
	.dsi_id = 1,
};

static const struct of_device_id dsi_match_table[] = {
	{ .compatible = "cvitek,cv186x_mipi0",
	  .data = &cv186x_dsi0 },
#if IS_ENABLED(CONFIG_CVITEK_MIPI_DSI1)
	{ .compatible = "cvitek,cv186x_mipi1",
	  .data = &cv186x_dsi1 },
#endif
	{ /* end node */ },
};

static int cvitek_dsi_probe(struct platform_device *pdev)
{
	DRM_DEBUG_DRIVER("---- enter cvitek dsi probe. ----\n");
	return component_add(&pdev->dev, &dsi_component_ops);
}

static int cvitek_dsi_remove(struct platform_device *pdev)
{
	DRM_DEBUG_DRIVER("---- enter cvitek dsi remove. ----\n");
	component_del(&pdev->dev, &dsi_component_ops);
	return 0;
}

struct platform_driver cvitek_dsi_driver = {
	.probe = cvitek_dsi_probe,
	.remove = cvitek_dsi_remove,
	.driver = {
		.name = "cvitek-dsi-drv",
		.of_match_table = dsi_match_table,
	},
};
