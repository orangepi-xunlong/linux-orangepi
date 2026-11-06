#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/of_platform.h>
#include <linux/of_reserved_mem.h>
#include <linux/of_gpio.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/iommu.h>
#include <linux/irq.h>
#include <linux/reset.h>
#include <generated/compile.h>
#include <linux/io.h>
#include <linux/clk.h>
#include <linux/defines.h>
#if defined(__CV186X__)
#include "pinctrl-cv186x.h"
#endif
#include <linux/ctype.h>
#include <linux/version.h>
#include <linux/compat.h>

#ifdef CONFIG_PROC_FS
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/uaccess.h>
#endif

#include "linux/comm_cif.h"
#include "vi_snsr.h"
#include "drv/cif_drv.h"
#include "cif.h"
#include <vi_sys.h>
#include <base_cb.h>
#include <cif_cb.h>

#define MIPI_IF
#define DVP_IF
#define BT601_IF
#define BT656_IF

#define SUBLVDS_IF
#define HISPI_IF
#define BT1120_IF
#define CUSTOM0_IF
#define BT_DEMUX_IF

#ifndef DEVICE_FROM_DTS
#define DEVICE_FROM_DTS 1
#endif

#define CIF_PROC_NAME "soph/mipi-rx"
#define MIPI_RX_DEV_NAME "soph-mipi-rx"
#define MAX_CIF_PROC_BUF 96

enum {
	LANE_SKEW_CROSS_CLK,
	LANE_SKEW_CROSS_DATA_NEAR,
	LANE_SKEW_CROSS_DATA_FAR,
	LANE_SKEW_CLK,
	LANE_SKEW_DATA,
	LANE_SKEW_NUM,
};

static int mclk0 = CAMPLL_FREQ_NONE;
module_param(mclk0, int, 0644);
MODULE_PARM_DESC(mclk0, "cam0 mclk");

static int mclk1 = CAMPLL_FREQ_NONE;
module_param(mclk1, int, 0644);
MODULE_PARM_DESC(mclk1, "cam1 mclk");

static int mclk2 = CAMPLL_FREQ_NONE;
module_param(mclk2, int, 0644);
MODULE_PARM_DESC(mclk2, "cam2 mclk");

static int mclk3 = CAMPLL_FREQ_NONE;
module_param(mclk3, int, 0644);
MODULE_PARM_DESC(mclk3, "cam3 mclk");

static int mclk4 = CAMPLL_FREQ_NONE;
module_param(mclk4, int, 0644);
MODULE_PARM_DESC(mclk4, "cam4 mclk");

static int mclk5 = CAMPLL_FREQ_NONE;
module_param(mclk5, int, 0644);
MODULE_PARM_DESC(mclk5, "cam5 mclk");

static int mclk6 = CAMPLL_FREQ_NONE;
module_param(mclk6, int, 0644);
MODULE_PARM_DESC(mclk6, "cam6 mclk");

static int lane_phase[LANE_SKEW_NUM] = {0x00, 0x03, 0x08, 0x00, 0x03};
static int count;
module_param_array(lane_phase, int, &count, 0664);

static int bypass_mac_clk;
module_param(bypass_mac_clk, int, 0644);
MODULE_PARM_DESC(bypass_mac_clk, "byass mac clk");

static unsigned int max_mac_clk = 900;
module_param(max_mac_clk, uint, 0644);
MODULE_PARM_DESC(max_mac_clk, "max mac clk");

#ifndef FPGA_PORTING
static void __iomem *mclk_enable;
static void __iomem *mclk_update;
static void __iomem *mclk_src;
#endif

static int cif_set_output_clk_edge(struct cif_dev *dev,
				   struct clk_edge_s *clk_edge);

const struct sync_code_s default_sync_code = {
	.norm_bk_sav = 0xAB0,
	.norm_bk_eav = 0xB60,
	.norm_sav = 0x800,
	.norm_eav = 0x9D0,
	.n0_bk_sav = 0x2B0,
	.n0_bk_eav = 0x360,
	.n1_bk_sav = 0x6B0,
	.n1_bk_eav = 0x760,
};

static struct link *ctx_to_link(const struct cif_ctx *ctx)
{
	return container_of(ctx, struct link, cif_ctx);
}

static struct cif_dev *file_cif_dev(struct file *file)
{
	return container_of(file->private_data, struct cif_dev, miscdev);
}

const char *_to_string_input_mode(enum input_mode_e input_mode)
{
	switch (input_mode) {
	case INPUT_MODE_MIPI:
		return "MIPI";
	case INPUT_MODE_SUBLVDS:
		return "SUBLVDS";
	case INPUT_MODE_HISPI:
		return "HISPI";
	case INPUT_MODE_CMOS:
		return "CMOS";
	case INPUT_MODE_BT1120:
		return "BT1120";
	case INPUT_MODE_BT601:
		return "INPUT_MODE_BT601";
	case INPUT_MODE_BT656_9B:
		return "INPUT_MODE_BT656_9B";
	case INPUT_MODE_BT656_9B_DDR:
		return "INPUT_MODE_BT656_9B_DDR";
	case INPUT_MODE_CUSTOM_0:
		return "INPUT_MODE_CUSTOM_0";
	case INPUT_MODE_BT_DEMUX:
		return "INPUT_MODE_BT_DEMUX";
	default:
		return "unknown";
	}
}

const char *_to_string_mac_clk(enum rx_mac_clk_e mac_clk)
{
	switch (mac_clk) {
	case RX_MAC_CLK_150M:
		return "150MHZ";
	case RX_MAC_CLK_200M:
		return "200MHZ";
	case RX_MAC_CLK_300M:
		return "300MHZ";
	case RX_MAC_CLK_400M:
		return "400MHZ";
	case RX_MAC_CLK_500M:
		return "500MHZ";
	case RX_MAC_CLK_600M:
		return "600MHZ";
	case RX_MAC_CLK_900M:
		return "900MHZ";
	default:
		return "unknown";
	}
}

const char *_to_string_cmd(unsigned int cmd)
{
	switch (cmd) {
	case MIPI_SET_DEV_ATTR:
		return "MIPI_SET_DEV_ATTR";
	case MIPI_SET_HS_MODE:
		return "MIPI_SET_HS_MODE";
	case MIPI_SET_OUTPUT_CLK_EDGE:
		return "MIPI_SET_OUTPUT_CLK_EDGE";
	case MIPI_RESET_MIPI:
		return "MIPI_RESET_MIPI";
	case MIPI_SET_CROP_TOP:
		return "MIPI_SET_CROP_TOP";
	case MIPI_SET_WDR_MANUAL:
		return "MIPI_SET_WDR_MANUAL";
	case MIPI_SET_LVDS_FP_VS:
		return "MIPI_SET_LVDS_FP_VS";
	case MIPI_RESET_SENSOR:
		return "MIPI_RESET_SENSOR";
	case MIPI_UNRESET_SENSOR:
		return "MIPI_UNRESET_SENSOR";
	case MIPI_ENABLE_SENSOR_CLOCK:
		return "MIPI_ENABLE_SENSOR_CLOCK";
	case MIPI_DISABLE_SENSOR_CLOCK:
		return "MIPI_DISABLE_SENSOR_CLOCK";
	case MIPI_RESET_LVDS:
		return "MIPI_RESET_LVDS";
	case MIPI_GET_CIF_ATTR:
		return "MIPI_GET_CIF_ATTR";
	case MIPI_SET_MAX_MAC_CLOCK:
		return "MIPI_SET_MAX_MAC_CLOCK";
	case MIPI_SET_CROP_WINDOW:
		return "MIPI_SET_CROP_WINDOW";
	default:
		return "unknown";
	}
	return "unknown";
}

const char *_to_string_raw_data_type(enum raw_data_type_e raw_data_type)
{
	switch (raw_data_type) {
	case RAW_DATA_8BIT:
		return "RAW8";
	case RAW_DATA_10BIT:
		return "RAW10";
	case RAW_DATA_12BIT:
		return "RAW12";
	case YUV422_8BIT:
		return "YUV422_8BIT";
	case YUV422_10BIT:
		return "YUV422_10BIT";
	default:
		return "unknown";
	}
}

const char *_to_string_mipi_wdr_mode(enum mipi_wdr_mode_e wdr)
{
	switch (wdr) {
	case MIPI_WDR_MODE_NONE:
		return "NONE";
	case MIPI_WDR_MODE_VC:
		return "VC";
	case MIPI_WDR_MODE_DT:
		return "DT";
	case MIPI_WDR_MODE_DOL:
		return "DOL";
	case MIPI_WDR_MODE_MANUAL:
		return "MANUAL";
	default:
		return "unknown";
	}
}

const char *_to_string_wdr_mode(enum wdr_mode_e wdr)
{
	switch (wdr) {
	case CIF_WDR_MODE_NONE:
		return "NONE";
	case CIF_WDR_MODE_2F:
		return "2To1";
	case CIF_WDR_MODE_3F:
		return "3To1";
	case CIF_WDR_MODE_DOL_2F:
		return "DOL2To1";
	case CIF_WDR_MODE_DOL_3F:
		return "DOL3To1";
	default:
		return "unknown";
	}
}

const char *_to_string_lvds_sync_mode(enum lvds_sync_mode_e mode)
{
	switch (mode) {
	case LVDS_SYNC_MODE_SOF:
		return "SOF";
	case LVDS_SYNC_MODE_SAV:
		return "SAV";
	default:
		return "unknown";
	}
}

const char *_to_string_bit_endian(enum lvds_bit_endian endian)
{
	switch (endian) {
	case LVDS_ENDIAN_LITTLE:
		return "LITTLE";
	case LVDS_ENDIAN_BIG:
		return "BIG";
	default:
		return "unknown";
	}
}

const char *_to_string_lvds_vsync_type(enum lvds_vsync_type_e type)
{
	switch (type) {
	case LVDS_VSYNC_NORMAL:
		return "NORMAL";
	case LVDS_VSYNC_SHARE:
		return "SHARE";
	case LVDS_VSYNC_HCONNECT:
		return "HCONNECT";
	default:
		return "unknown";
	}
}

const char *_to_string_lvds_fid_type(enum lvds_fid_type_e type)
{
	switch (type) {
	case LVDS_FID_NONE:
		return "FID_NONE";
	case LVDS_FID_IN_SAV:
		return "FID_IN_SAV";
	default:
		return "unknown";
	}
}

const char *_to_string_mclk(enum cam_pll_freq_e freq)
{
	switch (freq) {
	case CAMPLL_FREQ_NONE:
		return "CAMPLL_FREQ_NONE";
	case CAMPLL_FREQ_37P125M:
		return "CAMPLL_FREQ_37P125M";
	case CAMPLL_FREQ_25M:
		return "CAMPLL_FREQ_25M";
	case CAMPLL_FREQ_27M:
		return "CAMPLL_FREQ_27M";
	case CAMPLL_FREQ_24M:
		return "CAMPLL_FREQ_24M";
	case CAMPLL_FREQ_26M:
		return "CAMPLL_FREQ_26M";
	default:
		return "unknown";
	}
}

const char *_to_string_csi_decode(enum csi_decode_fmt_e fmt)
{
	switch (fmt) {
	case DEC_FMT_YUV422_8:
		return "yuv422-8";
	case DEC_FMT_YUV422_10:
		return "yuv422-10";
	case DEC_FMT_RAW8:
		return "raw8";
	case DEC_FMT_RAW10:
		return "raw10";
	case DEC_FMT_RAW12:
		return "raw12";
	default:
		return "unknown";
	}
}

const char *_to_string_dlane_state(enum mipi_dlane_state_e state)
{
	switch (state) {
	case HS_IDLE:
		return "hs_idle";
	case HS_SYNC:
		return "hs_sync";
	case HS_SKEW_CAL:
		return "skew_cal";
	case HS_ALT_CAL:
		return "alt_cal";
	case HS_PREAMPLE:
		return "preample";
	case HS_HST:
		return "hs_hst";
	case HS_ERR:
		return "hs_err";
	default:
		return "unknown";
	}
}

const char *_to_string_deskew_state(enum mipi_deskew_state_e state)
{
	switch (state) {
	case DESKEW_IDLE:
		return "idle";
	case DESKEW_START:
		return "start";
	case DESKEW_DONE:
		return "done";
	default:
		return "unknown";
	}
}

static void cif_dump_dev_attr(struct cif_dev *dev,
			      struct combo_dev_attr_s *attr)
{
	struct device *_dev = dev->miscdev.this_device;
	int i;

	dev_dbg(_dev, "devno = %d, input_mode = %s, mac_clk = %s\n",
		attr->devno,
		_to_string_input_mode(attr->input_mode),
		_to_string_mac_clk(attr->mac_clk));
	dev_dbg(_dev, "mclk%d = %s\n",
		attr->mclk.cam, _to_string_mclk(attr->mclk.freq));
	dev_dbg(_dev, "width = %d, height = %d\n",
		attr->img_size.width, attr->img_size.height);
	switch (attr->input_mode) {
	case INPUT_MODE_MIPI: {
		struct mipi_dev_attr_s *mipi = &attr->mipi_attr;

		dev_dbg(_dev, "raw_data_type = %s\n",
			_to_string_raw_data_type(mipi->raw_data_type));

		for (i = 0; i < MIPI_LANE_NUM + 1; i++) {
			dev_dbg(_dev, "lane_id[%d] = %d, pn_swap = %s", i,
				mipi->lane_id[i],
				mipi->pn_swap[i] ? "True" : "False");
		}
		dev_dbg(_dev, "wdr_mode = %s\n",
			_to_string_mipi_wdr_mode(mipi->wdr_mode));
		for (i = 0; i < WDR_VC_NUM; i++) {
			dev_dbg(_dev, "data_type[%d] = 0x%x\n",
				i, mipi->data_type[i]);
		}
	}
	break;
	case INPUT_MODE_SUBLVDS:
	case INPUT_MODE_HISPI: {
		struct lvds_dev_attr_s *lvds = &attr->lvds_attr;
		int j;

		dev_dbg(_dev, "wdr_mode = %s\n",
			_to_string_wdr_mode(lvds->wdr_mode));
		dev_dbg(_dev, "sync_mode = %s\n",
			_to_string_lvds_sync_mode(lvds->sync_mode));
		dev_dbg(_dev, "raw_data_type = %s\n",
			_to_string_raw_data_type(lvds->raw_data_type));
		dev_dbg(_dev, "data_endian = %s\n",
			_to_string_bit_endian(lvds->data_endian));
		dev_dbg(_dev, "sync_code_endian = %s\n",
			_to_string_bit_endian(lvds->sync_code_endian));
		for (i = 0; i < MIPI_LANE_NUM + 1; i++) {
			dev_dbg(_dev, "lane_id[%d] = %d, pn_swap = %s ", i,
				lvds->lane_id[i],
				lvds->pn_swap[i] ? "True" : "False");
		}
		dev_dbg(_dev, "sync code = {\n");
		for (i = 0; i < MIPI_LANE_NUM; i++) {
			dev_dbg(_dev, "\t{\n");
			for (j = 0; j < WDR_VC_NUM+1; j++) {
				dev_dbg(_dev,
					"\t\t{ %3x, %3x, %3x, %3x },\n",
					lvds->sync_code[i][j][0],
					lvds->sync_code[i][j][1],
					lvds->sync_code[i][j][2],
					lvds->sync_code[i][j][3]);
			}
			dev_dbg(_dev, "\t},\n");
		}
		dev_dbg(_dev, "}\n");
		dev_dbg(_dev, "vsync_type = %s\n",
			_to_string_lvds_vsync_type(lvds->vsync_type.sync_type));
		dev_dbg(_dev, "fid = %s\n",
			_to_string_lvds_fid_type(lvds->fid_type.fid));

	}
	break;
	case INPUT_MODE_CMOS:
		break;
	case INPUT_MODE_BT1120:
		break;
	default:
		break;
	}
}

static int LANE_IS_PORT1(int x)
{
	if (x < CIF_PHY_LANE_3) {
		return 0;
	} else if (x < CIF_PHY_LANE_6) {
		return 1;
	} else if (x < CIF_PHY_LANE_9) {
		return 2;
	} else if (x < CIF_PHY_LANE_12) {
		return 3;
	} else if (x < CIF_PHY_LANE_15) {
		return 4;
	} else if (x < CIF_PHY_LANE_NUM) {
		return 5;
	} else {
		return -EINVAL;
	}
}

static int LANE_IS_FRIST(uint16_t phy_mode, short lane_num, unsigned int devno)
{
	if (phy_mode == 0) {
		if (lane_num == CIF_PHY_LANE_0)
			return 1;
	} else if (phy_mode == 1) {
		if (devno == 0) {
			if (lane_num == CIF_PHY_LANE_0)
				return 1;
		} else if (devno == 3) {
			if (lane_num == CIF_PHY_LANE_9)
				return 1;
		}
	} else if (phy_mode == 2) {
		if (devno == 0) {
			if (lane_num == CIF_PHY_LANE_0)
				return 1;
		}
	} else if (phy_mode == 3) {
		if (devno == 0) {
			if (lane_num == CIF_PHY_LANE_0)
				return 1;
		} else if (devno == 3) {
			if (lane_num == CIF_PHY_LANE_9)
				return 1;
		} else if (devno == 4) {
			if (lane_num == CIF_PHY_LANE_12)
				return 1;
		}
	} else if (phy_mode == 4) {
		if (devno == 0) {
			if (lane_num == CIF_PHY_LANE_0)
				return 1;
		} else if (devno == 3) {
			if (lane_num == CIF_PHY_LANE_6)
				return 1;
		}
	} else if (phy_mode == 5) {
		if (devno == 0) {
			if (lane_num == CIF_PHY_LANE_0)
				return 1;
		}
	}
	return 0;
}

//#define LANE_IS_PORT1(x)	((x > 2) ? 1 : 0)
#define IS_SAME_PORT(x, y)	(!((LANE_IS_PORT1(x))^(LANE_IS_PORT1(y))))

#define PAD_CTRL_PU		BIT(2)
#define PAD_CTRL_PD		BIT(3)
#define PSD_CTRL_OFFSET(n)	((5 - n) * 8)

#ifndef FPGA_PORTING
static int _cif_set_clk_buffer(struct cif_ctx *ctx, int clk_port, int min_port, int max_port)
{
	if (ctx->phy_mode == 0) {
		if (clk_port == 0) {
			if (max_port == 5) {
				cif_set_clk_dir(ctx, CIF_CLK_P02P1);
				cif_set_clk_dir(ctx, CIF_CLK_P12P2);
				cif_set_clk_dir(ctx, CIF_CLK_P22P3);
				cif_set_clk_dir(ctx, CIF_CLK_P32P4);
				cif_set_clk_dir(ctx, CIF_CLK_P42P5);
			} else if (max_port == 4) {
				cif_set_clk_dir(ctx, CIF_CLK_P02P1);
				cif_set_clk_dir(ctx, CIF_CLK_P12P2);
				cif_set_clk_dir(ctx, CIF_CLK_P22P3);
				cif_set_clk_dir(ctx, CIF_CLK_P32P4);
			} else if (max_port == 3) {
				cif_set_clk_dir(ctx, CIF_CLK_P02P1);
				cif_set_clk_dir(ctx, CIF_CLK_P12P2);
				cif_set_clk_dir(ctx, CIF_CLK_P22P3);
			} else if (max_port == 2) {
				cif_set_clk_dir(ctx, CIF_CLK_P02P1);
				cif_set_clk_dir(ctx, CIF_CLK_P12P2);
			} else if (max_port == 1) {
				cif_set_clk_dir(ctx, CIF_CLK_P02P1);
			}
		} else if (clk_port == 1) {
			if (max_port == 5 && min_port == 0) {
				cif_set_clk_dir(ctx, CIF_CLK_P12P0);
				cif_set_clk_dir(ctx, CIF_CLK_P12P2);
				cif_set_clk_dir(ctx, CIF_CLK_P22P3);
				cif_set_clk_dir(ctx, CIF_CLK_P32P4);
				cif_set_clk_dir(ctx, CIF_CLK_P42P5);
			} else if (max_port == 4 && min_port == 0) {
				cif_set_clk_dir(ctx, CIF_CLK_P12P0);
				cif_set_clk_dir(ctx, CIF_CLK_P12P2);
				cif_set_clk_dir(ctx, CIF_CLK_P22P3);
				cif_set_clk_dir(ctx, CIF_CLK_P32P4);
			} else if (max_port == 3 && min_port == 0) {
				cif_set_clk_dir(ctx, CIF_CLK_P12P0);
				cif_set_clk_dir(ctx, CIF_CLK_P12P2);
				cif_set_clk_dir(ctx, CIF_CLK_P22P3);
			} else if (max_port == 2 && min_port == 0) {
				cif_set_clk_dir(ctx, CIF_CLK_P12P0);
				cif_set_clk_dir(ctx, CIF_CLK_P12P2);
			} else if (max_port == 1 && min_port == 0) {
				cif_set_clk_dir(ctx, CIF_CLK_P12P0);
			} else if (max_port == 5 && min_port == 1) {
				cif_set_clk_dir(ctx, CIF_CLK_P12P2);
				cif_set_clk_dir(ctx, CIF_CLK_P22P3);
				cif_set_clk_dir(ctx, CIF_CLK_P32P4);
				cif_set_clk_dir(ctx, CIF_CLK_P42P5);
			} else if (max_port == 4 && min_port == 1) {
				cif_set_clk_dir(ctx, CIF_CLK_P12P2);
				cif_set_clk_dir(ctx, CIF_CLK_P22P3);
				cif_set_clk_dir(ctx, CIF_CLK_P32P4);
			} else if (max_port == 3 && min_port == 1) {
				cif_set_clk_dir(ctx, CIF_CLK_P12P2);
				cif_set_clk_dir(ctx, CIF_CLK_P22P3);
			} else if (max_port == 2 && min_port == 1) {
				cif_set_clk_dir(ctx, CIF_CLK_P12P2);
			} else if (max_port == 1 && min_port == 1) {
				cif_set_clk_dir(ctx, CIF_CLK_P12P0);
			}
		} else if (clk_port == 2) {
			if (max_port == 5 && min_port == 0) {
				cif_set_clk_dir(ctx, CIF_CLK_P22P1);
				cif_set_clk_dir(ctx, CIF_CLK_P12P0);
				cif_set_clk_dir(ctx, CIF_CLK_P22P3);
				cif_set_clk_dir(ctx, CIF_CLK_P32P4);
				cif_set_clk_dir(ctx, CIF_CLK_P42P5);
			} else if (max_port == 4 && min_port == 0) {
				cif_set_clk_dir(ctx, CIF_CLK_P22P1);
				cif_set_clk_dir(ctx, CIF_CLK_P12P0);
				cif_set_clk_dir(ctx, CIF_CLK_P22P3);
				cif_set_clk_dir(ctx, CIF_CLK_P32P4);
			} else if (max_port == 3 && min_port == 0) {
				cif_set_clk_dir(ctx, CIF_CLK_P22P1);
				cif_set_clk_dir(ctx, CIF_CLK_P12P0);
				cif_set_clk_dir(ctx, CIF_CLK_P22P3);
			} else if (max_port == 2 && min_port == 0) {
				cif_set_clk_dir(ctx, CIF_CLK_P22P1);
				cif_set_clk_dir(ctx, CIF_CLK_P12P0);
			} else if (max_port == 5 && min_port == 1) {
				cif_set_clk_dir(ctx, CIF_CLK_P22P1);
				cif_set_clk_dir(ctx, CIF_CLK_P22P3);
				cif_set_clk_dir(ctx, CIF_CLK_P32P4);
				cif_set_clk_dir(ctx, CIF_CLK_P42P5);
			} else if (max_port == 4 && min_port == 1) {
				cif_set_clk_dir(ctx, CIF_CLK_P22P1);
				cif_set_clk_dir(ctx, CIF_CLK_P22P3);
				cif_set_clk_dir(ctx, CIF_CLK_P32P4);
			} else if (max_port == 3 && min_port == 1) {
				cif_set_clk_dir(ctx, CIF_CLK_P22P1);
				cif_set_clk_dir(ctx, CIF_CLK_P22P3);
			} else if (max_port == 2 && min_port == 1) {
				cif_set_clk_dir(ctx, CIF_CLK_P22P1);
			} else if (max_port == 5 && min_port == 2) {
				cif_set_clk_dir(ctx, CIF_CLK_P22P3);
				cif_set_clk_dir(ctx, CIF_CLK_P32P4);
				cif_set_clk_dir(ctx, CIF_CLK_P42P5);
			} else if (max_port == 4 && min_port == 2) {
				cif_set_clk_dir(ctx, CIF_CLK_P22P3);
				cif_set_clk_dir(ctx, CIF_CLK_P32P4);
			} else if (max_port == 3 && min_port == 2) {
				cif_set_clk_dir(ctx, CIF_CLK_P22P3);
			} else if (max_port == 2 && min_port == 2) {
				cif_set_clk_dir(ctx, CIF_CLK_P22P1);
				cif_set_clk_dir(ctx, CIF_CLK_P12P0);
			}
		} else if (clk_port == 3) {
			if (max_port == 3 && min_port == 0) {
				cif_set_clk_dir(ctx, CIF_CLK_P32P2);
				cif_set_clk_dir(ctx, CIF_CLK_P22P1);
				cif_set_clk_dir(ctx, CIF_CLK_P12P0);
			} else if (max_port == 3 && min_port == 1) {
				cif_set_clk_dir(ctx, CIF_CLK_P32P2);
				cif_set_clk_dir(ctx, CIF_CLK_P22P1);
			} else if (max_port == 3 && min_port == 2) {
				cif_set_clk_dir(ctx, CIF_CLK_P32P2);
			} else if (max_port == 4 && min_port == 0) {
				cif_set_clk_dir(ctx, CIF_CLK_P32P2);
				cif_set_clk_dir(ctx, CIF_CLK_P22P1);
				cif_set_clk_dir(ctx, CIF_CLK_P12P0);
				cif_set_clk_dir(ctx, CIF_CLK_P32P4);
			} else if (max_port == 4 && min_port == 1) {
				cif_set_clk_dir(ctx, CIF_CLK_P32P2);
				cif_set_clk_dir(ctx, CIF_CLK_P22P1);
				cif_set_clk_dir(ctx, CIF_CLK_P32P4);
			} else if (max_port == 4 && min_port == 2) {
				cif_set_clk_dir(ctx, CIF_CLK_P32P2);
				cif_set_clk_dir(ctx, CIF_CLK_P32P4);
			} else if (max_port == 4 && min_port == 3) {
				cif_set_clk_dir(ctx, CIF_CLK_P32P4);
			} else if (max_port == 5 && min_port == 0) {
				cif_set_clk_dir(ctx, CIF_CLK_P32P2);
				cif_set_clk_dir(ctx, CIF_CLK_P22P1);
				cif_set_clk_dir(ctx, CIF_CLK_P12P0);
				cif_set_clk_dir(ctx, CIF_CLK_P32P4);
				cif_set_clk_dir(ctx, CIF_CLK_P42P5);
			} else if (max_port == 5 && min_port == 1) {
				cif_set_clk_dir(ctx, CIF_CLK_P32P2);
				cif_set_clk_dir(ctx, CIF_CLK_P22P1);
				cif_set_clk_dir(ctx, CIF_CLK_P32P4);
				cif_set_clk_dir(ctx, CIF_CLK_P42P5);
			} else if (max_port == 5 && min_port == 2) {
				cif_set_clk_dir(ctx, CIF_CLK_P32P2);
				cif_set_clk_dir(ctx, CIF_CLK_P32P4);
				cif_set_clk_dir(ctx, CIF_CLK_P42P5);
			} else if (max_port == 5 && min_port == 3) {
				cif_set_clk_dir(ctx, CIF_CLK_P32P4);
				cif_set_clk_dir(ctx, CIF_CLK_P42P5);
			} else if (max_port == 3 && min_port == 3) {
				cif_set_clk_dir(ctx, CIF_CLK_P32P2);
				cif_set_clk_dir(ctx, CIF_CLK_P22P1);
				cif_set_clk_dir(ctx, CIF_CLK_P12P0);
			}
		} else if (clk_port == 4) {
			if (max_port == 4 && min_port == 0) {
				cif_set_clk_dir(ctx, CIF_CLK_P42P3);
				cif_set_clk_dir(ctx, CIF_CLK_P32P2);
				cif_set_clk_dir(ctx, CIF_CLK_P22P1);
				cif_set_clk_dir(ctx, CIF_CLK_P12P0);
			} else if (max_port == 4 && min_port == 1) {
				cif_set_clk_dir(ctx, CIF_CLK_P32P2);
				cif_set_clk_dir(ctx, CIF_CLK_P22P1);
				cif_set_clk_dir(ctx, CIF_CLK_P42P3);
			} else if (max_port == 4 && min_port == 2) {
				cif_set_clk_dir(ctx, CIF_CLK_P32P2);
				cif_set_clk_dir(ctx, CIF_CLK_P42P3);
			} else if (max_port == 4 && min_port == 3) {
				cif_set_clk_dir(ctx, CIF_CLK_P42P3);
			} else if (max_port == 5 && min_port == 0) {
				cif_set_clk_dir(ctx, CIF_CLK_P32P2);
				cif_set_clk_dir(ctx, CIF_CLK_P22P1);
				cif_set_clk_dir(ctx, CIF_CLK_P12P0);
				cif_set_clk_dir(ctx, CIF_CLK_P42P3);
				cif_set_clk_dir(ctx, CIF_CLK_P42P5);
			} else if (max_port == 5 && min_port == 1) {
				cif_set_clk_dir(ctx, CIF_CLK_P32P2);
				cif_set_clk_dir(ctx, CIF_CLK_P22P1);
				cif_set_clk_dir(ctx, CIF_CLK_P42P3);
				cif_set_clk_dir(ctx, CIF_CLK_P42P5);
			} else if (max_port == 5 && min_port == 2) {
				cif_set_clk_dir(ctx, CIF_CLK_P32P2);
				cif_set_clk_dir(ctx, CIF_CLK_P42P3);
				cif_set_clk_dir(ctx, CIF_CLK_P42P5);
			} else if (max_port == 5 && min_port == 3) {
				cif_set_clk_dir(ctx, CIF_CLK_P42P3);
				cif_set_clk_dir(ctx, CIF_CLK_P42P5);
			} else if (max_port == 5 && min_port == 4) {
				cif_set_clk_dir(ctx, CIF_CLK_P42P5);
			} else if (min_port == 4 && max_port == 4) {
				cif_set_clk_dir(ctx, CIF_CLK_P42P3);
				cif_set_clk_dir(ctx, CIF_CLK_P32P2);
				cif_set_clk_dir(ctx, CIF_CLK_P22P1);
				cif_set_clk_dir(ctx, CIF_CLK_P12P0);
			}
		} else if (clk_port == 5) {
			if (max_port == 5 && min_port == 0) {
				cif_set_clk_dir(ctx, CIF_CLK_P32P2);
				cif_set_clk_dir(ctx, CIF_CLK_P22P1);
				cif_set_clk_dir(ctx, CIF_CLK_P12P0);
				cif_set_clk_dir(ctx, CIF_CLK_P42P3);
				cif_set_clk_dir(ctx, CIF_CLK_P52P4);
			} else if (max_port == 5 && min_port == 1) {
				cif_set_clk_dir(ctx, CIF_CLK_P32P2);
				cif_set_clk_dir(ctx, CIF_CLK_P22P1);
				cif_set_clk_dir(ctx, CIF_CLK_P42P3);
				cif_set_clk_dir(ctx, CIF_CLK_P52P4);
			} else if (max_port == 5 && min_port == 2) {
				cif_set_clk_dir(ctx, CIF_CLK_P32P2);
				cif_set_clk_dir(ctx, CIF_CLK_P42P3);
				cif_set_clk_dir(ctx, CIF_CLK_P52P4);
			} else if (max_port == 5 && min_port == 3) {
				cif_set_clk_dir(ctx, CIF_CLK_P42P3);
				cif_set_clk_dir(ctx, CIF_CLK_P52P4);
			} else if (max_port == 5 && min_port == 4) {
				cif_set_clk_dir(ctx, CIF_CLK_P52P4);
			} else if (max_port == 5 && min_port == 5) {
				cif_set_clk_dir(ctx, CIF_CLK_P52P4);
				cif_set_clk_dir(ctx, CIF_CLK_P52P4);
				cif_set_clk_dir(ctx, CIF_CLK_P42P3);
				cif_set_clk_dir(ctx, CIF_CLK_P32P2);
				cif_set_clk_dir(ctx, CIF_CLK_P22P1);
				cif_set_clk_dir(ctx, CIF_CLK_P12P0);
			}
		}
	} else if (ctx->phy_mode == 1) {
		if (clk_port == 0) {
			if (max_port == 2 && min_port == 0) {
				cif_set_clk_dir(ctx, CIF_CLK_P02P1);
				cif_set_clk_dir(ctx, CIF_CLK_P12P2);
			} else if (max_port == 1 && min_port == 0) {
				cif_set_clk_dir(ctx, CIF_CLK_P02P1);
			}
		} else if (clk_port == 1) {
			if (max_port == 2 && min_port == 0) {
				cif_set_clk_dir(ctx, CIF_CLK_P12P0);
				cif_set_clk_dir(ctx, CIF_CLK_P12P2);
			} else if (max_port == 1 && min_port == 0) {
				cif_set_clk_dir(ctx, CIF_CLK_P12P0);
			} else if (max_port == 2 && min_port == 1) {
				cif_set_clk_dir(ctx, CIF_CLK_P12P2);
			} else if (max_port == 1 && min_port == 1) {
				cif_set_clk_dir(ctx, CIF_CLK_P12P0);
			}
		} else if (clk_port == 2) {
			if (max_port == 2 && min_port == 0) {
				cif_set_clk_dir(ctx, CIF_CLK_P22P1);
				cif_set_clk_dir(ctx, CIF_CLK_P12P0);
			} else if (max_port == 2 && min_port == 1) {
				cif_set_clk_dir(ctx, CIF_CLK_P22P1);
			} else if (max_port == 2 && min_port == 2) {
				cif_set_clk_dir(ctx, CIF_CLK_P12P0);
				cif_set_clk_dir(ctx, CIF_CLK_P22P1);
			}
		} else if (clk_port == 3) {
			if (max_port == 4 && min_port == 3) {
				cif_set_clk_dir(ctx, CIF_CLK_P32P4);
			}
		}
	} else if (ctx->phy_mode == 2) {
		if (clk_port == 0) {
			if (max_port == 2 && min_port == 0) {
				cif_set_clk_dir(ctx, CIF_CLK_P02P1);
				cif_set_clk_dir(ctx, CIF_CLK_P12P2);
			} else if (max_port == 1 && min_port == 0) {
				cif_set_clk_dir(ctx, CIF_CLK_P02P1);
			}
		} else if (clk_port == 1) {
			if (max_port == 2 && min_port == 0) {
				cif_set_clk_dir(ctx, CIF_CLK_P12P0);
				cif_set_clk_dir(ctx, CIF_CLK_P12P2);
			} else if (max_port == 1 && min_port == 0) {
				cif_set_clk_dir(ctx, CIF_CLK_P12P0);
			} else if (max_port == 2 && min_port == 1) {
				cif_set_clk_dir(ctx, CIF_CLK_P12P2);
			} else if (max_port == 1 && min_port == 1) {
				cif_set_clk_dir(ctx, CIF_CLK_P12P0);
			}
		} else if (clk_port == 2) {
			if (max_port == 2 && min_port == 0) {
				cif_set_clk_dir(ctx, CIF_CLK_P22P1);
				cif_set_clk_dir(ctx, CIF_CLK_P12P0);
			} else if (max_port == 2 && min_port == 1) {
				cif_set_clk_dir(ctx, CIF_CLK_P22P1);
			} else if (max_port == 2 && min_port == 2) {
				cif_set_clk_dir(ctx, CIF_CLK_P12P0);
				cif_set_clk_dir(ctx, CIF_CLK_P22P1);
			}
		}
	} else if (ctx->phy_mode == 3) {
		if (clk_port == 0) {
			if (max_port == 1 && min_port == 0) {
				cif_set_clk_dir(ctx, CIF_CLK_P02P1);
			}
		} else if (clk_port == 1) {
			if (max_port == 1 && min_port == 0) {
				cif_set_clk_dir(ctx, CIF_CLK_P12P0);
			} else if (max_port == 1 && min_port == 1) {
				cif_set_clk_dir(ctx, CIF_CLK_P12P0);
			}
		} else if (clk_port == 2) {
			if (max_port == 3 && min_port == 2) {
				cif_set_clk_dir(ctx, CIF_CLK_P22P3);
			}
		} else if (clk_port == 3) {
			if (max_port == 3 && min_port == 2) {
				cif_set_clk_dir(ctx, CIF_CLK_P32P2);
			} else if (max_port == 3 && min_port == 3) {
				cif_set_clk_dir(ctx, CIF_CLK_P32P2);
			}
		} else if (clk_port == 4) {
			if (max_port == 5 && min_port == 4) {
				cif_set_clk_dir(ctx, CIF_CLK_P42P5);
			}
		} else if (clk_port == 5) {
			if (max_port == 5 && min_port == 4) {
				cif_set_clk_dir(ctx, CIF_CLK_P52P4);
			} else if (max_port == 5 && min_port == 5) {
				cif_set_clk_dir(ctx, CIF_CLK_P52P4);
			}
		}
	} else if (ctx->phy_mode == 4) {
		if (clk_port == 0) {
			if (max_port == 1 && min_port == 0) {
				cif_set_clk_dir(ctx, CIF_CLK_P02P1);
			}
		} else if (clk_port == 1) {
			if (max_port == 1 && min_port == 0) {
				cif_set_clk_dir(ctx, CIF_CLK_P12P0);
			} else if (max_port == 1 && min_port == 0) {
				cif_set_clk_dir(ctx, CIF_CLK_P12P0);
			}
		} else if (clk_port == 2) {
			if (max_port == 3 && min_port == 2) {
				cif_set_clk_dir(ctx, CIF_CLK_P22P3);
			}
		} else if (clk_port == 3) {
			if (max_port == 3 && min_port == 2) {
				cif_set_clk_dir(ctx, CIF_CLK_P32P2);
			} else if (max_port == 3 && min_port == 3) {
				cif_set_clk_dir(ctx, CIF_CLK_P32P2);
			}
		}
	} else if (ctx->phy_mode == 5) {
		if (clk_port == 0) {
			if (max_port == 1 && min_port == 0) {
				cif_set_clk_dir(ctx, CIF_CLK_P02P1);
			}
		} else if (clk_port == 1) {
			if (max_port == 1 && min_port == 0) {
				cif_set_clk_dir(ctx, CIF_CLK_P12P0);
			} else if (max_port == 1 && min_port == 1) {
				cif_set_clk_dir(ctx, CIF_CLK_P12P0);
			}
		}
	}

	return 0;
}
#endif

#ifdef MIPI_IF
static int _cif_set_attr_mipi(struct cif_dev *dev,
			      struct cif_ctx *ctx,
			      struct mipi_dev_attr_s *attr,
				  uint32_t devno)
{
	struct combo_dev_attr_s *combo =
		container_of(attr, struct combo_dev_attr_s, mipi_attr);
	struct cif_param *param = ctx->cur_config;
	struct param_csi *csi = &param->cfg.csi;
	struct mipi_demux_info_s *info = &attr->demux;
	uint32_t tbl = 0x1FF;
	int i, j = 0, clk_port = 0, max_port = 0, min_port = 5, frist_lane_used = 0;
	uint32_t value;

	param->type = CIF_TYPE_CSI;
	/* config the bit mode. */
	switch (attr->raw_data_type) {
	case RAW_DATA_8BIT:
		csi->fmt = CSI_RAW_8;
		csi->decode_type = 0x2A;
		break;
	case RAW_DATA_10BIT:
		csi->fmt = CSI_RAW_10;
		csi->decode_type = 0x2B;
		break;
	case RAW_DATA_12BIT:
		csi->fmt = CSI_RAW_12;
		csi->decode_type = 0x2C;
		break;
	case RAW_DATA_16BIT:
		csi->fmt = CSI_RAW_16;
		//csi->decode_type = 0x2C;
		break;
	case YUV422_8BIT:
		csi->fmt = CSI_YUV422_8B;
		csi->decode_type = 0x1E;
		break;
	case YUV422_10BIT:
		csi->fmt = CSI_YUV422_10B;
		csi->decode_type = 0x1F;
		break;
	default:
		return -EINVAL;
	}
	/* config the lane id */
	for (i = 0; i < CIF_LANE_NUM; i++) {
		if (attr->lane_id[i] < 0)
			continue;
		if (attr->lane_id[i] >= CIF_PHY_LANE_NUM)
			return -EINVAL;

		frist_lane_used =  LANE_IS_FRIST(ctx->phy_mode, attr->lane_id[i], devno);

		if (!i)
			clk_port = LANE_IS_PORT1(attr->lane_id[i]);
		else {
			if (LANE_IS_PORT1(attr->lane_id[i]) >= max_port) {
				max_port = LANE_IS_PORT1(attr->lane_id[i]);
			}
			if (LANE_IS_PORT1(attr->lane_id[i]) <= min_port) {
				min_port = LANE_IS_PORT1(attr->lane_id[i]);
			}
		}
		cif_set_rx_bus_config(ctx, i, attr->lane_id[i]);
		cif_set_lane_id(ctx, i, attr->lane_id[i], attr->pn_swap[i]);
		/* clear pad ctrl pu/pd */
		if (dev->pad_ctrl) {
			value = ioread32(dev->pad_ctrl + PSD_CTRL_OFFSET(attr->lane_id[i]));
			value &= ~(PAD_CTRL_PU | PAD_CTRL_PD);
			iowrite32(value, dev->pad_ctrl + PSD_CTRL_OFFSET(attr->lane_id[i]));
			value = ioread32(dev->pad_ctrl + PSD_CTRL_OFFSET(attr->lane_id[i]) + 4);
			value &= ~(PAD_CTRL_PU | PAD_CTRL_PD);
			iowrite32(value, dev->pad_ctrl + PSD_CTRL_OFFSET(attr->lane_id[i]) + 4);
		}
		tbl &= ~(1<<attr->lane_id[i]);
		j++;
	}
	csi->lane_num = j - 1;
	while (ffs(tbl)) {
		uint32_t idx = ffs(tbl) - 1;

		cif_set_lane_id(ctx, j++, idx, 0);
		tbl &= ~(1 << idx);
	}
	//frist line for each port should be turned on, as the MAC will only receive clocks from one wire
	set_rx_frist_lane_enable(ctx, frist_lane_used);

	/* config  clock buffer direction.
	 * 1. When clock is between 0~2 and 1c4d, direction is P0->P1.
	 * 2. When clock is between 3~5 and 1c4d, direction is P1->P0.
	 * 3. When clock and data is between 0~2 and 1c2d, direction bit is freerun.
	 * 4. When clock is between 0~2 but data is not, direction is P0->P1.
	 * 5. When clock and data is between 3~5 and 1c2d and mac1 is used, direction bit is freerun.
	 * 6. When clock is between 3~5 but data is not, direction is P1->P0.
	 */
#ifndef FPGA_PORTING
	if (_cif_set_clk_buffer(ctx, clk_port, min_port, max_port) != 0) {
		return -EINVAL;
	}
	if (min_port >= 0 && max_port < 2) {
		cif_set_group(ctx, 0);
	} else if (min_port >= 2 && max_port < 4) {
		cif_set_group(ctx, 1);
	} else if (min_port >= 4 && max_port < 6) {
		cif_set_group(ctx, 2);
	} else if (min_port >= 0 && max_port < 4) {
		cif_set_group(ctx, 0);
		cif_set_group(ctx, 1);
	} else if (min_port >= 2 && max_port < 6) {
		cif_set_group(ctx, 1);
		cif_set_group(ctx, 2);
	} else {
		cif_set_group(ctx, 0);
		cif_set_group(ctx, 1);
		cif_set_group(ctx, 2);
	}

	cif_set_pd(ctx, attr->lane_id[0]);

	// if (csi->lane_num == 8) {
	// 	for (i = 0; (i < csi->lane_num + 1); i++) {
	// 		if (!i)
	// 			cif_set_lane_deskew(ctx, attr->lane_id[i],
	// 					lane_phase[LANE_SKEW_CROSS_CLK]);
	// 		else if (IS_SAME_PORT(attr->lane_id[0], attr->lane_id[i]))
	// 			cif_set_lane_deskew(ctx, attr->lane_id[i],
	// 					lane_phase[LANE_SKEW_CROSS_DATA_NEAR]);
	// 		else
	// 			cif_set_lane_deskew(ctx, attr->lane_id[i],
	// 					lane_phase[LANE_SKEW_CROSS_DATA_FAR]);
	// 	}
	// } else if (csi->lane_num == 4) {
	// 	for (i = 0; (i < csi->lane_num + 1); i++) {
	// 		if (!i)
	// 			cif_set_lane_deskew(ctx, attr->lane_id[i],
	// 					lane_phase[LANE_SKEW_CROSS_CLK]);
	// 		else if (IS_SAME_PORT(attr->lane_id[0], attr->lane_id[i]))
	// 			cif_set_lane_deskew(ctx, attr->lane_id[i],
	// 					lane_phase[LANE_SKEW_CROSS_DATA_NEAR]);
	// 		else
	// 			cif_set_lane_deskew(ctx, attr->lane_id[i],
	// 					lane_phase[LANE_SKEW_CROSS_DATA_FAR]);
	// 	}
	// } else if (csi->lane_num > 0) {
	// 	/* if clk and data are in the same port.*/
	// 	if (clk_port > 0) {
	// 		for (i = 0; i < (csi->lane_num + 1); i++) {
	// 			if (!i)
	// 				cif_set_lane_deskew(ctx, attr->lane_id[i],
	// 						lane_phase[LANE_SKEW_CLK]);
	// 			else
	// 				cif_set_lane_deskew(ctx, attr->lane_id[i],
	// 						lane_phase[LANE_SKEW_DATA]);
	// 		}
	// 	} else {
	// 		for (i = 0; i < (csi->lane_num + 1); i++) {
	// 			if (!i)
	// 				cif_set_lane_deskew(ctx, attr->lane_id[i],
	// 						lane_phase[LANE_SKEW_CROSS_CLK]);
	// 			else if (IS_SAME_PORT(attr->lane_id[0], attr->lane_id[i]))
	// 				cif_set_lane_deskew(ctx, attr->lane_id[i],
	// 						lane_phase[LANE_SKEW_CROSS_DATA_NEAR]);
	// 			else
	// 				cif_set_lane_deskew(ctx, attr->lane_id[i],
	// 						lane_phase[LANE_SKEW_CROSS_DATA_FAR]);
	// 		}
	// 	}
	// } else {
	// 	return -EINVAL;
	// }
#endif

	/* config the dphy seting. */
	if (attr->dphy.enable)
		cif_set_hs_settle(ctx, attr->dphy.hs_settle);
	/* config the wdr mode. */
	switch (attr->wdr_mode) {
	case MIPI_WDR_MODE_NONE:
		break;
	case MIPI_WDR_MODE_DT:
		csi->hdr_mode = CSI_HDR_MODE_DT;
		for (i = 0; i < MAX_WDR_FRAME_NUM; i++)
			csi->data_type[i] = attr->data_type[i];
		break;
	case MIPI_WDR_MODE_MANUAL:
		param->hdr_manual = combo->wdr_manu.manual_en;
		param->hdr_shift = combo->wdr_manu.l2s_distance;
		param->hdr_vsize = combo->wdr_manu.lsef_length;
		param->hdr_rm_padding = combo->wdr_manu.discard_padding_lines;
		cif_hdr_manual_config(ctx, param, !!combo->wdr_manu.update);
		break;
	case MIPI_WDR_MODE_VC:
		csi->hdr_mode = CSI_HDR_MODE_VC;
		break;
	case MIPI_WDR_MODE_DOL:
		csi->hdr_mode = CSI_HDR_MODE_DOL;
		break;
	default:
		return -EINVAL;
	}
	/* config vc mapping */
	if (info->demux_en) {
		for (i = 0; i < MIPI_DEMUX_NUM; i++) {
			csi->vc_mapping[i] = info->vc_mapping[i];
		}
	} else {
		for (i = 0; i < MIPI_DEMUX_NUM; i++) {
			csi->vc_mapping[i] = i;
		}
	}

	param->hdr_en = (attr->wdr_mode != MIPI_WDR_MODE_NONE);
	cif_streaming(ctx, 1, param->hdr_en);

	return 0;
}
#endif //MIPI_IF

#ifdef SUBLVDS_IF
static int _cif_set_lvds_vsync_type(struct cif_ctx *ctx,
				    struct lvds_dev_attr_s *attr,
				    struct param_sublvds *sublvds)
{
	struct combo_dev_attr_s *combo =
		container_of(attr, struct combo_dev_attr_s, lvds_attr);
	struct lvds_vsync_type_s *type = &attr->vsync_type;
	struct link *link = ctx_to_link(ctx);

	switch (type->sync_type) {
	case LVDS_VSYNC_NORMAL:
		sublvds->hdr_mode = CIF_SLVDS_HDR_PAT1;
		/* [TODO] use other api to set the fp */
		link->distance_fp = 15;
		cif_set_lvds_vsync_gen(ctx, 15);
		break;
	case LVDS_VSYNC_HCONNECT:
		sublvds->hdr_mode = CIF_SLVDS_HDR_PAT2;
		/* [TODO] use other api to set the fp */
		link->distance_fp = 1;
		cif_set_lvds_vsync_gen(ctx, 1);
		sublvds->hdr_hblank[0] = type->hblank1;
		sublvds->hdr_hblank[1] = type->hblank2;
		sublvds->h_size = combo->img_size.width;
		/* [TODO] use other api to strip the info line*/
		/* strip the top info line. */
		cif_crop_info_line(ctx, 1, 1);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int _cif_set_attr_sublvds(struct cif_dev *dev,
				 struct cif_ctx *ctx,
				 struct lvds_dev_attr_s *attr,
				 uint32_t devno)
{
	struct cif_param *param = ctx->cur_config;
	struct link *link = ctx_to_link(ctx);
	struct param_sublvds *sublvds = &param->cfg.sublvds;
	struct sublvds_sync_code *sc;
	uint32_t tbl = 0x1FF;
	int i, j = 0, clk_port = 0, max_port = 0, min_port = 5, frist_lane_used = 0;
	int rc = 0;
	uint32_t value;

	param->type = CIF_TYPE_SUBLVDS;
	/* config the bit mode. */
	switch (attr->raw_data_type) {
	case RAW_DATA_8BIT:
		sublvds->fmt = CIF_SLVDS_8_BIT;
		break;
	case RAW_DATA_10BIT:
		sublvds->fmt = CIF_SLVDS_10_BIT;
		break;
	case RAW_DATA_12BIT:
		sublvds->fmt = CIF_SLVDS_12_BIT;
		break;
	default:
		return -EINVAL;
	}
	/* config the endian. */
	if (attr->data_endian == LVDS_ENDIAN_BIG &&
	    attr->sync_code_endian == LVDS_ENDIAN_BIG) {
		sublvds->endian = CIF_SLVDS_ENDIAN_MSB;
		sublvds->wrap_endian = CIF_SLVDS_ENDIAN_MSB;

	} else if (attr->data_endian == LVDS_ENDIAN_LITTLE &&
		   attr->sync_code_endian == LVDS_ENDIAN_BIG) {
		sublvds->endian = CIF_SLVDS_ENDIAN_LSB;
		sublvds->wrap_endian = CIF_SLVDS_ENDIAN_MSB;

	} else if (attr->data_endian == LVDS_ENDIAN_BIG &&
		   attr->sync_code_endian == LVDS_ENDIAN_LITTLE) {
		sublvds->endian = CIF_SLVDS_ENDIAN_LSB;
		sublvds->wrap_endian = CIF_SLVDS_ENDIAN_LSB;
	} else {
		sublvds->endian = CIF_SLVDS_ENDIAN_MSB;
		sublvds->wrap_endian = CIF_SLVDS_ENDIAN_LSB;
	}
	/* check the sync mode. */
	if (attr->sync_mode != LVDS_SYNC_MODE_SAV)
		return -EINVAL;
	/* config the lane id*/
	for (i = 0; i < CIF_LANE_NUM; i++) {
		if (attr->lane_id[i] < 0)
			continue;
		if (attr->lane_id[i] >= CIF_PHY_LANE_NUM)
			return -EINVAL;

		frist_lane_used = LANE_IS_FRIST(ctx->phy_mode, attr->lane_id[i], devno);

		if (!i)
			clk_port = LANE_IS_PORT1(attr->lane_id[i]);
		else {
			if (LANE_IS_PORT1(attr->lane_id[i]) >= max_port) {
				max_port = LANE_IS_PORT1(attr->lane_id[i]);
			}
			if (LANE_IS_PORT1(attr->lane_id[i]) <= min_port) {
				min_port = LANE_IS_PORT1(attr->lane_id[i]);
			}
		}
		cif_set_rx_bus_config(ctx, i, attr->lane_id[i]);
		cif_set_lane_id(ctx, i, attr->lane_id[i], attr->pn_swap[i]);
		/* clear pad ctrl pu/pd */
		if (dev->pad_ctrl) {
			value = ioread32(dev->pad_ctrl + PSD_CTRL_OFFSET(attr->lane_id[i]));
			value &= ~(PAD_CTRL_PU | PAD_CTRL_PD);
			iowrite32(value, dev->pad_ctrl + PSD_CTRL_OFFSET(attr->lane_id[i]));
			value = ioread32(dev->pad_ctrl + PSD_CTRL_OFFSET(attr->lane_id[i]) + 4);
			value &= ~(PAD_CTRL_PU | PAD_CTRL_PD);
			iowrite32(value, dev->pad_ctrl + PSD_CTRL_OFFSET(attr->lane_id[i]) + 4);
		}
		tbl &= ~(1<<attr->lane_id[i]);
		j++;
	}
	sublvds->lane_num = j - 1;
	while (ffs(tbl)) {
		uint32_t idx = ffs(tbl) - 1;

		cif_set_lane_id(ctx, j++, idx, 0);
		tbl &= ~(1 << idx);
	}
	//frist line for each port should be turned on, as the MAC will only receive clocks from one wire
	set_rx_frist_lane_enable(ctx, frist_lane_used);
	/* config  clock buffer direction.
	 * 1. When clock is between 0~2 and 1c4d, direction is P0->P1.
	 * 2. When clock is between 3~5 and 1c4d, direction is P1->P0.
	 * 3. When clock and data is between 0~2 and 1c2d, direction bit is freerun.
	 * 4. When clock is between 0~2 but data is not, direction is P0->P1.
	 * 5. When clock and data is between 3~5 and 1c2d and mac1 is used, direction bit is freerun.
	 * 6. When clock is between 3~5 but data is not, direction is P1->P0.
	 */
#ifndef FPGA_PORTING
	if (_cif_set_clk_buffer(ctx, clk_port, min_port, max_port) != 0) {
		return -EINVAL;
	}
	if (min_port >= 0 && max_port < 2) {
		cif_set_group(ctx, 0);
	} else if (min_port >= 2 && max_port < 4) {
		cif_set_group(ctx, 1);
	} else if (min_port >= 4 && max_port < 6) {
		cif_set_group(ctx, 2);
	} else if (min_port >= 0 && max_port < 4) {
		cif_set_group(ctx, 0);
		cif_set_group(ctx, 1);
	} else if (min_port >= 2 && max_port < 6) {
		cif_set_group(ctx, 1);
		cif_set_group(ctx, 2);
	} else {
		cif_set_group(ctx, 0);
		cif_set_group(ctx, 1);
		cif_set_group(ctx, 2);
	}

	cif_set_pd(ctx, attr->lane_id[0]);

	// if (sublvds->lane_num == 8) {
	// 	for (i = 0; (i < sublvds->lane_num + 1); i++) {
	// 		if (!i)
	// 			cif_set_lane_deskew(ctx, attr->lane_id[i],
	// 					lane_phase[LANE_SKEW_CROSS_CLK]);
	// 		else if (IS_SAME_PORT(attr->lane_id[0], attr->lane_id[i]))
	// 			cif_set_lane_deskew(ctx, attr->lane_id[i],
	// 					lane_phase[LANE_SKEW_CROSS_DATA_NEAR]);
	// 		else
	// 			cif_set_lane_deskew(ctx, attr->lane_id[i],
	// 					lane_phase[LANE_SKEW_CROSS_DATA_FAR]);
	// 	}
	// } else if (sublvds->lane_num == 4) {
	// 	for (i = 0; (i < sublvds->lane_num + 1); i++) {
	// 		if (!i)
	// 			cif_set_lane_deskew(ctx, attr->lane_id[i],
	// 					lane_phase[LANE_SKEW_CROSS_CLK]);
	// 		else if (IS_SAME_PORT(attr->lane_id[0], attr->lane_id[i]))
	// 			cif_set_lane_deskew(ctx, attr->lane_id[i],
	// 					lane_phase[LANE_SKEW_CROSS_DATA_NEAR]);
	// 		else
	// 			cif_set_lane_deskew(ctx, attr->lane_id[i],
	// 					lane_phase[LANE_SKEW_CROSS_DATA_FAR]);
	// 	}
	// } else if (sublvds->lane_num > 0) {
	// 	/* if clk and data are in the same port.*/
	// 	if (clk_port > 0) {
	// 		for (i = 0; i < (sublvds->lane_num + 1); i++) {
	// 			if (!i)
	// 				cif_set_lane_deskew(ctx, attr->lane_id[i],
	// 						lane_phase[LANE_SKEW_CLK]);
	// 			else
	// 				cif_set_lane_deskew(ctx, attr->lane_id[i],
	// 						lane_phase[LANE_SKEW_DATA]);
	// 		}
	// 	} else {
	// 		for (i = 0; i < (sublvds->lane_num + 1); i++) {
	// 			if (!i)
	// 				cif_set_lane_deskew(ctx, attr->lane_id[i],
	// 						lane_phase[LANE_SKEW_CROSS_CLK]);
	// 			else if (IS_SAME_PORT(attr->lane_id[0], attr->lane_id[i]))
	// 				cif_set_lane_deskew(ctx, attr->lane_id[i],
	// 						lane_phase[LANE_SKEW_CROSS_DATA_NEAR]);
	// 			else
	// 				cif_set_lane_deskew(ctx, attr->lane_id[i],
	// 						lane_phase[LANE_SKEW_CROSS_DATA_FAR]);
	// 		}
	// 	}
	// } else {
	// 	return -EINVAL;
	// }
#endif

	/* config the sync code */
	memcpy(&sublvds->sync_code, &default_sync_code,
	       sizeof(default_sync_code));
	sc = &sublvds->sync_code.slvds;
	sc->n0_lef_sav = attr->sync_code[0][0][0];
	sc->n0_lef_eav = attr->sync_code[0][0][1];
	sc->n1_lef_sav = attr->sync_code[0][0][2];
	sc->n1_lef_eav = attr->sync_code[0][0][3];
	sc->n0_sef_sav = attr->sync_code[0][1][0];
	sc->n0_sef_eav = attr->sync_code[0][1][1];
	sc->n1_sef_sav = attr->sync_code[0][1][2];
	sc->n1_sef_eav = attr->sync_code[0][1][3];
	sc->n0_lsef_sav = attr->sync_code[0][2][0];
	sc->n0_lsef_eav = attr->sync_code[0][2][1];
	sc->n1_lsef_sav = attr->sync_code[0][2][2];
	sc->n1_lsef_eav = attr->sync_code[0][2][3];

	/* config the wdr */
	switch (attr->wdr_mode) {
	case CIF_WDR_MODE_NONE:
		/* [TODO] use other api to set the fp */
		link->distance_fp = 6;
		cif_set_lvds_vsync_gen(ctx, 6);
		break;
	case CIF_WDR_MODE_DOL_2F:
	case CIF_WDR_MODE_DOL_3F:
		/* [TODO] 3 exposure hdr hw is not ready. */
		/* config th Vsync type */
		rc = _cif_set_lvds_vsync_type(ctx, attr, sublvds);
		if (rc < 0)
			return rc;
		break;
	default:
		return -EINVAL;
	}
	param->hdr_en = (attr->wdr_mode != CIF_WDR_MODE_NONE);
	/* [TODO] config the fid type. */
	cif_streaming(ctx, 1, attr->wdr_mode != CIF_WDR_MODE_NONE);

	return 0;
}
#endif // SUBLVDS_IF

#ifdef HISPI_IF
static int _cif_set_hispi_vsync_type(struct cif_ctx *ctx,
				     struct lvds_dev_attr_s *attr,
				     struct cif_param *param)
{
	struct combo_dev_attr_s *combo =
		container_of(attr, struct combo_dev_attr_s, lvds_attr);
	struct lvds_vsync_type_s *type = &attr->vsync_type;

	switch (type->sync_type) {
	case LVDS_VSYNC_NORMAL:
		break;
	case LVDS_VSYNC_SHARE:
		param->hdr_manual = combo->wdr_manu.manual_en;
		param->hdr_shift = combo->wdr_manu.l2s_distance;
		param->hdr_vsize = combo->wdr_manu.lsef_length;
		param->hdr_rm_padding = combo->wdr_manu.discard_padding_lines;
		cif_hdr_manual_config(ctx, param, !!combo->wdr_manu.update);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int _cif_set_attr_hispi(struct cif_dev *dev,
			       struct cif_ctx *ctx,
			       struct lvds_dev_attr_s *attr,
				   uint32_t devno)
{
	struct combo_dev_attr_s *combo =
		container_of(attr, struct combo_dev_attr_s, lvds_attr);
	struct cif_param *param = ctx->cur_config;
	struct param_hispi *hispi = &param->cfg.hispi;
	struct hispi_sync_code *sc;
	uint32_t tbl = 0x1FF;
	int i, j = 0, clk_port = 0, max_port = 0, min_port = 5, frist_lane_used = 0;
	int rc = 0;
	uint32_t value;

	param->type = CIF_TYPE_HISPI;
	/* config the bit mode. */
	switch (attr->raw_data_type) {
	case RAW_DATA_8BIT:
		hispi->fmt = CIF_SLVDS_8_BIT;
		break;
	case RAW_DATA_10BIT:
		hispi->fmt = CIF_SLVDS_10_BIT;
		break;
	case RAW_DATA_12BIT:
		hispi->fmt = CIF_SLVDS_12_BIT;
		break;
	default:
		return -EINVAL;
	}
	/* config the endian. */
	if (attr->data_endian == LVDS_ENDIAN_BIG &&
	    attr->sync_code_endian == LVDS_ENDIAN_BIG) {
		hispi->endian = CIF_SLVDS_ENDIAN_MSB;
		hispi->wrap_endian = CIF_SLVDS_ENDIAN_MSB;
	} else if (attr->data_endian == LVDS_ENDIAN_LITTLE &&
		   attr->sync_code_endian == LVDS_ENDIAN_BIG) {
		hispi->endian = CIF_SLVDS_ENDIAN_LSB;
		hispi->wrap_endian = CIF_SLVDS_ENDIAN_MSB;
	} else if (attr->data_endian == LVDS_ENDIAN_BIG &&
		   attr->sync_code_endian == LVDS_ENDIAN_LITTLE) {
		hispi->endian = CIF_SLVDS_ENDIAN_LSB;
		hispi->wrap_endian = CIF_SLVDS_ENDIAN_LSB;
	} else {
		hispi->endian = CIF_SLVDS_ENDIAN_MSB;
		hispi->wrap_endian = CIF_SLVDS_ENDIAN_LSB;
	}
	/* check the sync mode. */
	if (attr->sync_mode == LVDS_SYNC_MODE_SOF) {
		hispi->mode = CIF_HISPI_MODE_PKT_SP;
	} else if (attr->sync_mode == LVDS_SYNC_MODE_SAV) {
		hispi->mode = CIF_HISPI_MODE_STREAM_SP;
		hispi->h_size = combo->img_size.width;
	} else {
		return -EINVAL;
	}
	/* config the lane id */
	for (i = 0; i < CIF_LANE_NUM; i++) {
		if (attr->lane_id[i] < 0)
			continue;
		if (attr->lane_id[i] >= CIF_PHY_LANE_NUM)
			return -EINVAL;

		frist_lane_used = LANE_IS_FRIST(ctx->phy_mode, attr->lane_id[i], devno);

		if (!i)
			clk_port = LANE_IS_PORT1(attr->lane_id[i]);
		else {
			if (LANE_IS_PORT1(attr->lane_id[i]) >= max_port) {
				max_port = LANE_IS_PORT1(attr->lane_id[i]);
			}
			if (LANE_IS_PORT1(attr->lane_id[i]) <= min_port) {
				min_port = LANE_IS_PORT1(attr->lane_id[i]);
			}
		}
		cif_set_rx_bus_config(ctx, i, attr->lane_id[i]);
		cif_set_lane_id(ctx, i, attr->lane_id[i], attr->pn_swap[i]);
		/* clear pad ctrl pu/pd */
		if (dev->pad_ctrl) {
			value = ioread32(dev->pad_ctrl + PSD_CTRL_OFFSET(attr->lane_id[i]));
			value &= ~(PAD_CTRL_PU | PAD_CTRL_PD);
			iowrite32(value, dev->pad_ctrl + PSD_CTRL_OFFSET(attr->lane_id[i]));
			value = ioread32(dev->pad_ctrl + PSD_CTRL_OFFSET(attr->lane_id[i]) + 4);
			value &= ~(PAD_CTRL_PU | PAD_CTRL_PD);
			iowrite32(value, dev->pad_ctrl + PSD_CTRL_OFFSET(attr->lane_id[i]) + 4);
		}
		tbl &= ~(1<<attr->lane_id[i]);
		j++;
	}
	hispi->lane_num = j - 1;
	while (ffs(tbl)) {
		uint32_t idx = ffs(tbl) - 1;

		cif_set_lane_id(ctx, j++, idx, 0);
		tbl &= ~(1 << idx);
	}
	//frist line for each port should be turned on, as the MAC will only receive clocks from one wire
	set_rx_frist_lane_enable(ctx, frist_lane_used);
	/* config  clock buffer direction.
	 * 1. When clock is between 0~2 and 1c4d, direction is P0->P1.
	 * 2. When clock is between 3~5 and 1c4d, direction is P1->P0.
	 * 3. When clock and data is between 0~2 and 1c2d, direction bit is freerun.
	 * 4. When clock is between 0~2 but data is not, direction is P0->P1.
	 * 5. When clock and data is between 3~5 and 1c2d and mac1 is used, direction bit is freerun.
	 * 6. When clock is between 3~5 but data is not, direction is P1->P0.
	 */
#ifndef FPGA_PORTING
	if (_cif_set_clk_buffer(ctx, clk_port, min_port, max_port) != 0) {
		return -EINVAL;
	}
	if (min_port >= 0 && max_port < 2) {
		cif_set_group(ctx, 0);
	} else if (min_port >= 2 && max_port < 4) {
		cif_set_group(ctx, 1);
	} else if (min_port >= 4 && max_port < 6) {
		cif_set_group(ctx, 2);
	} else if (min_port >= 0 && max_port < 4) {
		cif_set_group(ctx, 0);
		cif_set_group(ctx, 1);
	} else if (min_port >= 2 && max_port < 6) {
		cif_set_group(ctx, 1);
		cif_set_group(ctx, 2);
	} else {
		cif_set_group(ctx, 0);
		cif_set_group(ctx, 1);
		cif_set_group(ctx, 2);
	}

	cif_set_pd(ctx, attr->lane_id[0]);

	// if (hispi->lane_num == 8) {
	// 	for (i = 0; (i < hispi->lane_num + 1); i++) {
	// 		if (!i)
	// 			cif_set_lane_deskew(ctx, attr->lane_id[i],
	// 					lane_phase[LANE_SKEW_CROSS_CLK]);
	// 		else if (IS_SAME_PORT(attr->lane_id[0], attr->lane_id[i]))
	// 			cif_set_lane_deskew(ctx, attr->lane_id[i],
	// 					lane_phase[LANE_SKEW_CROSS_DATA_NEAR]);
	// 		else
	// 			cif_set_lane_deskew(ctx, attr->lane_id[i],
	// 					lane_phase[LANE_SKEW_CROSS_DATA_FAR]);
	// 	}
	// } else if (hispi->lane_num == 4) {
	// 	for (i = 0; (i < hispi->lane_num + 1); i++) {
	// 		if (!i)
	// 			cif_set_lane_deskew(ctx, attr->lane_id[i],
	// 					lane_phase[LANE_SKEW_CROSS_CLK]);
	// 		else if (IS_SAME_PORT(attr->lane_id[0], attr->lane_id[i]))
	// 			cif_set_lane_deskew(ctx, attr->lane_id[i],
	// 					lane_phase[LANE_SKEW_CROSS_DATA_NEAR]);
	// 		else
	// 			cif_set_lane_deskew(ctx, attr->lane_id[i],
	// 					lane_phase[LANE_SKEW_CROSS_DATA_FAR]);
	// 	}
	// } else if (hispi->lane_num > 0) {
	// 	/* if clk and data are in the same port.*/
	// 	if (clk_port > 0) {
	// 		for (i = 0; i < (hispi->lane_num + 1); i++) {
	// 			if (!i)
	// 				cif_set_lane_deskew(ctx, attr->lane_id[i],
	// 						lane_phase[LANE_SKEW_CLK]);
	// 			else
	// 				cif_set_lane_deskew(ctx, attr->lane_id[i],
	// 						lane_phase[LANE_SKEW_DATA]);
	// 		}
	// 	} else {
	// 		for (i = 0; i < (hispi->lane_num + 1); i++) {
	// 			if (!i)
	// 				cif_set_lane_deskew(ctx, attr->lane_id[i],
	// 						lane_phase[LANE_SKEW_CROSS_CLK]);
	// 			else if (IS_SAME_PORT(attr->lane_id[0], attr->lane_id[i]))
	// 				cif_set_lane_deskew(ctx, attr->lane_id[i],
	// 						lane_phase[LANE_SKEW_CROSS_DATA_NEAR]);
	// 			else
	// 				cif_set_lane_deskew(ctx, attr->lane_id[i],
	// 						lane_phase[LANE_SKEW_CROSS_DATA_FAR]);
	// 		}
	// 	}
	// } else {
	// 	return -EINVAL;
	// }
#endif

	/* config the sync code */
	memcpy(&hispi->sync_code, &default_sync_code,
	       sizeof(default_sync_code));
	sc = &hispi->sync_code.hispi;
	sc->t1_sol = attr->sync_code[0][0][0];
	sc->t1_eol = attr->sync_code[0][0][1];
	sc->t1_sof = attr->sync_code[0][0][2];
	sc->t1_eof = attr->sync_code[0][0][3];
	sc->t2_sol = attr->sync_code[0][1][0];
	sc->t2_eol = attr->sync_code[0][1][1];
	sc->t2_sof = attr->sync_code[0][1][2];
	sc->t2_eof = attr->sync_code[0][1][3];
	sc->vsync_gen = sc->t1_sof;

	/* config the wdr */
	switch (attr->wdr_mode) {
	case CIF_WDR_MODE_NONE:
	case CIF_WDR_MODE_2F:
	case CIF_WDR_MODE_3F: /* [TODO] 3 exposure hdr hw is not ready. */
		break;
	default:
		return -EINVAL;
	}
	/* config th Vsync type */
	rc = _cif_set_hispi_vsync_type(ctx, attr, param);
	if (rc < 0)
		return rc;

	param->hdr_en = (attr->wdr_mode != CIF_WDR_MODE_NONE);
	/* [TODO] config the fid type. */
	cif_streaming(ctx, 1, attr->wdr_mode != CIF_WDR_MODE_NONE);

	return 0;
}
#endif // HISPI_IF

#ifdef DVP_IF
#ifndef FPGA_PORTING
static void cif_config_pinmux(enum ttl_src_e vi, uint32_t pad)
{
	if (vi == TTL_VI_SRC_VI0) {
		switch (pad) {
			case 0:
				PINMUX_CONFIG(PAD_MIPI_RX14P, VI0_D0, PHY);
				break;
			case 1:
				PINMUX_CONFIG(PAD_MIPI_RX14N, VI0_D1, PHY);
				break;
			case 2:
				PINMUX_CONFIG(PAD_MIPI_RX15P, VI0_D2, PHY);
				break;
			case 3:
				PINMUX_CONFIG(PAD_MIPI_RX15N, VI0_D3, PHY);
				break;
			case 4:
				PINMUX_CONFIG(PAD_MIPI_RX16P, VI0_D4, PHY);
				break;
			case 5:
				PINMUX_CONFIG(PAD_MIPI_RX16N, VI0_D5, PHY);
				break;
			case 6:
				PINMUX_CONFIG(PAD_MIPI_RX17P, VI0_D6, PHY);
				break;
			case 7:
				PINMUX_CONFIG(PAD_MIPI_RX17N, VI0_D7, PHY);
				break;
			case 8:
				PINMUX_CONFIG(PAD_MIPI_RX11N, VI0_D8, PHY);
				break;
			case 9:
				PINMUX_CONFIG(PAD_MIPI_RX11P, VI0_D9, PHY);
				break;
			case 10:
				PINMUX_CONFIG(PAD_MIPI_RX10N, VI0_D10, PHY);
				break;
			case 11:
				PINMUX_CONFIG(PAD_MIPI_RX10P, VI0_D11, PHY);
				break;
			case 12:
				PINMUX_CONFIG(PAD_MIPI_RX9N, VI0_D12, PHY);
				break;
			case 13:
				PINMUX_CONFIG(PAD_MIPI_RX9P, VI0_D13, PHY);
				break;
			case 14:
				PINMUX_CONFIG(PAD_MIPI_RX8N, VI0_D14, PHY);
				break;
			case 15:
				PINMUX_CONFIG(PAD_MIPI_RX8P, VI0_D15, PHY);
				break;
			case 16:
				PINMUX_CONFIG(PAD_MIPI_RX12N, VI0_D16, PHY);
				break;
			case 17:
				PINMUX_CONFIG(PAD_MIPI_RX13P, VI0_D17, PHY);
				break;
			case 18:
				PINMUX_CONFIG(PAD_MIPI_RX13N, VI0_D18, PHY);
				break;
			case 19:
				PINMUX_CONFIG(PAD_MIPI_RX3P, VI0_D19, PHY);
				break;
			case 20:
				PINMUX_CONFIG(PAD_MIPI_RX3N, VI0_D20, PHY);
				break;
			case 21:
				PINMUX_CONFIG(PAD_MIPI_RX4P, VI0_D21, PHY);
				break;
			case 22:
				PINMUX_CONFIG(PAD_MIPI_RX4N, VI0_D22, PHY);
				break;
			case 23:
				PINMUX_CONFIG(PAD_MIPI_RX5P, VI0_D23, PHY);
				break;
			case 24:
				PINMUX_CONFIG(PAD_MIPI_RX5N, VI0_D24, PHY);
				break;
			case 25:
				PINMUX_CONFIG(PAD_MIPI_RX6N, VI0_D25, PHY);
				break;
			case 26:
				PINMUX_CONFIG(PAD_MIPI_RX7P, VI0_D26, PHY);
				break;
			case 27:
				PINMUX_CONFIG(PAD_MIPI_RX7N, VI0_D27, PHY);
				break;
			default:
				break;
		}
	} else if (vi == TTL_VI_SRC_VI1) {
		switch (pad) {
			case 0:
				PINMUX_CONFIG(PAD_MIPI_RX8P, VI1_D0, PHY);
				break;
			case 1:
				PINMUX_CONFIG(PAD_MIPI_RX8N, VI1_D1, PHY);
				break;
			case 2:
				PINMUX_CONFIG(PAD_MIPI_RX9P, VI1_D2, PHY);
				break;
			case 3:
				PINMUX_CONFIG(PAD_MIPI_RX9N, VI1_D3, PHY);
				break;
			case 4:
				PINMUX_CONFIG(PAD_MIPI_RX10P, VI1_D4, PHY);
				break;
			case 5:
				PINMUX_CONFIG(PAD_MIPI_RX10N, VI1_D5, PHY);
				break;
			case 6:
				PINMUX_CONFIG(PAD_MIPI_RX11P, VI1_D6, PHY);
				break;
			case 7:
				PINMUX_CONFIG(PAD_MIPI_RX11N, VI1_D7, PHY);
				break;
			case 8:
				PINMUX_CONFIG(PAD_MIPI_RX5N, VI1_D8, PHY);
				break;
			case 9:
				PINMUX_CONFIG(PAD_MIPI_RX5P, VI1_D9, PHY);
				break;
			case 10:
				PINMUX_CONFIG(PAD_MIPI_RX4N, VI1_D10, PHY);
				break;
			case 11:
				PINMUX_CONFIG(PAD_MIPI_RX4P, VI1_D11, PHY);
				break;
			case 12:
				PINMUX_CONFIG(PAD_MIPI_RX3N, VI1_D12, PHY);
				break;
			case 13:
				PINMUX_CONFIG(PAD_MIPI_RX3P, VI1_D13, PHY);
				break;
			case 14:
				PINMUX_CONFIG(PAD_MIPI_RX6N, VI1_D14, PHY);
				break;
			case 15:
				PINMUX_CONFIG(PAD_MIPI_RX7P, VI1_D15, PHY);
				break;
			case 16:
				PINMUX_CONFIG(PAD_MIPI_RX7N, VI1_D16, PHY);
				break;
			default:
				break;
		}
	} else if (vi == TTL_VI_SRC_VI2) {
		switch (pad) {
			case 0:
				PINMUX_CONFIG(PAD_MIPI_RX2P, VI2_D0, PHY);
				break;
			case 1:
				PINMUX_CONFIG(PAD_MIPI_RX2N, VI2_D1, PHY);
				break;
			case 2:
				PINMUX_CONFIG(PAD_MIPI_RX3P, VI2_D2, PHY);
				break;
			case 3:
				PINMUX_CONFIG(PAD_MIPI_RX3N, VI2_D3, PHY);
				break;
			case 4:
				PINMUX_CONFIG(PAD_MIPI_RX4P, VI2_D4, PHY);
				break;
			case 5:
				PINMUX_CONFIG(PAD_MIPI_RX4N, VI2_D5, PHY);
				break;
			case 6:
				PINMUX_CONFIG(PAD_MIPI_RX5P, VI2_D6, PHY);
				break;
			case 7:
				PINMUX_CONFIG(PAD_MIPI_RX5N, VI2_D7, PHY);
				break;
			case 8:
				PINMUX_CONFIG(PAD_MIPI_RX0N, VI2_D8, PHY);
				break;
			case 9:
				PINMUX_CONFIG(PAD_MIPI_RX1P, VI2_D9, PHY);
				break;
			case 10:
				PINMUX_CONFIG(PAD_MIPI_RX1N, VI2_D10, PHY);
				break;
			default:
				break;
		}
	}
}

#endif//FPGA_PORTING

static int _cif_set_attr_cmos(struct cif_dev *dev,
			      struct cif_ctx *ctx,
			      struct combo_dev_attr_s *attr)
{
	struct cif_param *param = ctx->cur_config;
	struct param_ttl *ttl = &param->cfg.ttl;
	enum ttl_src_e vi = attr->ttl_attr.vi;
	int i;

	if (vi == TTL_VI_SRC_VI2)
		return -EINVAL;

	/* config the pinmux */
	for (i = 0; i < VI_FUNC_NUM; i++) {
		if (attr->ttl_attr.func[i] < 0)
			continue;
		if (attr->ttl_attr.func[i] >= MAX_PAD_NUM)
			return -EINVAL;
		cif_set_ttl_pinmux(ctx, (enum ttl_vi_from_e)vi, i, attr->ttl_attr.func[i]);
#ifndef FPGA_PORTING
		cif_config_pinmux(vi, attr->ttl_attr.func[i]);
#endif
	}
#ifndef FPGA_PORTING
	if (vi == TTL_VI_SRC_VI0){
		if (attr->ttl_attr.vi0_clk == 0)
			PINMUX_CONFIG(PAD_MIPI_RX12P, VI0_CLK0, PHY);
		else if (attr->ttl_attr.vi0_clk == 1)
			PINMUX_CONFIG(PAD_MIPI_RX6P, VI0_CLK1, PHY);
	} else if (vi == TTL_VI_SRC_VI1) {
		PINMUX_CONFIG(PAD_MIPI_RX6P, VI1_CLK0, PHY);
	} else {
		PINMUX_CONFIG(PAD_MIPI_RX0P, VI2_CLK0, PHY);
	}
#endif
	switch (attr->ttl_attr.raw_data_type) {
	case RAW_DATA_8BIT:
		ttl->sensor_fmt = TTL_SENSOR_8_BIT;
		break;
	case RAW_DATA_10BIT:
		ttl->sensor_fmt = TTL_SENSOR_10_BIT;
		break;
	case RAW_DATA_12BIT:
		ttl->sensor_fmt = TTL_SENSOR_12_BIT;
		break;
	default:
		return -EINVAL;
	}

	switch (attr->ttl_attr.ttl_fmt) {
	case TTL_SYNC_PAT:
		ttl->fmt = TTL_SYNC_PAT_SENSOR;
		break;
	case TTL_VHS_11B:
		ttl->fmt = TTL_VHS_SENSOR;
		break;
	case TTL_VDE_11B:
		ttl->fmt = TTL_VDE_SENSOR;
		break;
	case TTL_VSDE_11B:
		ttl->fmt = TTL_VSDE_SENSOR;
		break;
	default:
		return -EINVAL;
	}

	param->type = CIF_TYPE_TTL;
	ttl->vi_from = (enum ttl_vi_from_e)vi;
	ttl->vi_sel = VI_RAW;

	cif_streaming(ctx, 1, 0);

	return 0;
}
#endif // DVP_IF

#ifdef BT1120_IF
static int _cif_set_attr_bt1120(struct cif_dev *dev,
				struct cif_ctx *ctx,
				struct combo_dev_attr_s *attr)
{
	struct cif_param *param = ctx->cur_config;
	struct param_ttl *ttl = &param->cfg.ttl;
	struct link *link = ctx_to_link(ctx);
	enum ttl_src_e vi = attr->ttl_attr.vi;
	int i;

	if (vi == TTL_VI_SRC_VI2)
		return -EINVAL;

	/* config the pinmux */
	for (i = 0; i < VI_FUNC_NUM; i++) {
		if (attr->ttl_attr.func[i] < 0)
			continue;
		if (attr->ttl_attr.func[i] >= MAX_PAD_NUM)
			return -EINVAL;
		cif_set_ttl_pinmux(ctx, (enum ttl_vi_from_e)vi, i, attr->ttl_attr.func[i]);
#ifndef FPGA_PORTING
		cif_config_pinmux(vi, attr->ttl_attr.func[i]);
#endif
	}
#ifndef FPGA_PORTING
	if (vi == TTL_VI_SRC_VI0){
		if (attr->ttl_attr.vi0_clk == 0)
			PINMUX_CONFIG(PAD_MIPI_RX12P, VI0_CLK0, PHY);
		else if (attr->ttl_attr.vi0_clk == 1)
			PINMUX_CONFIG(PAD_MIPI_RX6P, VI0_CLK1, PHY);
	} else if (vi == TTL_VI_SRC_VI1) {
		PINMUX_CONFIG(PAD_MIPI_RX6P, VI1_CLK0, PHY);
	} else {
		PINMUX_CONFIG(PAD_MIPI_RX0P, VI2_CLK0, PHY);
	}
#endif
	param->type = CIF_TYPE_TTL;
	ttl->fmt = TTL_SYNC_PAT_17B_BT1120;
	ttl->width = attr->img_size.width - 1;
	ttl->height = attr->img_size.height - 1;
	ttl->sensor_fmt = TTL_SENSOR_12_BIT;
	ttl->clk_inv = link->clk_edge;
	ttl->vi_sel = VI_BT1120;
	ttl->v_bp = (!attr->ttl_attr.v_bp) ? 9 : attr->ttl_attr.v_bp;
	ttl->h_bp = (!attr->ttl_attr.h_bp) ? 8 : attr->ttl_attr.h_bp;

	cif_streaming(ctx, 1, 0);

	return 0;
}
#endif // BT1120_IF

#ifdef BT601_IF
static int _cif_set_attr_bt601(struct cif_dev *dev,
				       struct cif_ctx *ctx,
				       struct combo_dev_attr_s *attr)
{
	struct cif_param *param = ctx->cur_config;
	struct param_ttl *ttl = &param->cfg.ttl;
	struct link *link = ctx_to_link(ctx);
	enum ttl_src_e vi = attr->ttl_attr.vi;
	int i;

	if (vi == TTL_VI_SRC_VI2)
		return -EINVAL;

	/* config the pinmux */
	for (i = 0; i < VI_FUNC_NUM; i++) {
		if (attr->ttl_attr.func[i] < 0)
			continue;
		if (attr->ttl_attr.func[i] >= MAX_PAD_NUM)
			return -EINVAL;
		cif_set_ttl_pinmux(ctx, (enum ttl_vi_from_e)vi, i, attr->ttl_attr.func[i]);
#ifndef FPGA_PORTING
		cif_config_pinmux(vi, attr->ttl_attr.func[i]);
#endif
	}
#ifndef FPGA_PORTING
	if (vi == TTL_VI_SRC_VI0){
		if (attr->ttl_attr.vi0_clk == 0)
			PINMUX_CONFIG(PAD_MIPI_RX12P, VI0_CLK0, PHY);
		else if (attr->ttl_attr.vi0_clk == 1)
			PINMUX_CONFIG(PAD_MIPI_RX6P, VI0_CLK1, PHY);
	} else if (vi == TTL_VI_SRC_VI1) {
		PINMUX_CONFIG(PAD_MIPI_RX6P, VI1_CLK0, PHY);
	} else {
		PINMUX_CONFIG(PAD_MIPI_RX0P, VI2_CLK0, PHY);
	}
#endif
	switch (attr->ttl_attr.raw_data_type) {
	case RAW_DATA_8BIT:
		ttl->sensor_fmt = TTL_SENSOR_8_BIT;
		break;
	case RAW_DATA_10BIT:
		ttl->sensor_fmt = TTL_SENSOR_10_BIT;
		break;
	case RAW_DATA_12BIT:
		ttl->sensor_fmt = TTL_SENSOR_12_BIT;
		break;
	default:
		return -EINVAL;
	}

	switch (attr->ttl_attr.ttl_fmt) {
	case TTL_VHS_11B:
		ttl->fmt = TTL_VHS_11B_BT601;
		break;
	case TTL_VHS_19B:
		ttl->fmt = TTL_VHS_19B_BT601;
		break;
	case TTL_VDE_11B:
		ttl->fmt = TTL_VDE_11B_BT601;
		break;
	case TTL_VDE_19B:
		ttl->fmt = TTL_VDE_19B_BT601;
		break;
	case TTL_VSDE_11B:
		ttl->fmt = TTL_VSDE_11B_BT601;
		break;
	case TTL_VSDE_19B:
		ttl->fmt = TTL_VSDE_19B_BT601;
		break;
	default:
		return -EINVAL;
	}

	param->type = CIF_TYPE_TTL;
	ttl->vi_from = (enum ttl_vi_from_e)vi;
	ttl->width = attr->img_size.width - 1;
	ttl->height = attr->img_size.height - 1;
	ttl->clk_inv = link->clk_edge;
	ttl->vi_sel = VI_BT601;
	ttl->v_bp = (!attr->ttl_attr.v_bp) ? 0x23 : attr->ttl_attr.v_bp;
	ttl->h_bp = (!attr->ttl_attr.h_bp) ? 0xbf : attr->ttl_attr.h_bp;

	cif_streaming(ctx, 1, 0);

	return 0;
}
#endif // BT601_IF

#ifdef BT_DEMUX_IF
static int _cif_set_attr_bt_demux(struct cif_dev *dev,
					struct cif_ctx *ctx,
					struct combo_dev_attr_s *attr)
{
	struct cif_param *param = ctx->cur_config;
	struct param_btdemux *btdemux = &param->cfg.btdemux;
	struct link *link = ctx_to_link(ctx);
	struct bt_demux_attr_s *info = &attr->bt_demux_attr;
#ifndef FPGA_PORTING
	enum ttl_src_e vi = info->vi;
#else
	enum ttl_src_e vi = TTL_VI_SRC_VI1;
#endif
	int i;

#ifndef FPGA_PORTING
	if (vi == TTL_VI_SRC_VI0){
		printk("BT_DEMUX can not use VI0\n");
		return -EINVAL;
	} else if (vi == TTL_VI_SRC_VI1) {
		PINMUX_CONFIG(PAD_MIPI_RX6P, VI1_CLK0, PHY);
	} else {
		PINMUX_CONFIG(PAD_MIPI_RX0P, VI2_CLK0, PHY);
	}
#endif

	/* config the pinmux */
	for (i = 0; i < VI_FUNC_NUM; i++) {
		if (attr->bt_demux_attr.func[i] < 0)
			continue;
		if (attr->bt_demux_attr.func[i] >= MAX_PAD_NUM)
			return -EINVAL;
		//cif_set_mac_vi_pinmux(&dev->link[ctx->cur_mac_num].cif_ctx,
		//	(enum ttl_vi_from_e)vi, i, attr->bt_demux_attr.func[i]);
		cif_set_mac_vi_pinmux(ctx, (enum ttl_vi_from_e)vi, i, attr->bt_demux_attr.func[i]);
#ifndef FPGA_PORTING
		cif_config_pinmux(vi, attr->bt_demux_attr.func[i]);
#endif
	}
	param->type = CIF_TYPE_BT_DMUX;
	btdemux->fmt = TTL_SYNC_PAT_9B_BT656;
	btdemux->width = attr->img_size.width - 1;
	btdemux->height = attr->img_size.height - 1;
	btdemux->clk_inv = link->clk_edge;
	btdemux->v_fp = (!info->v_fp) ? 0x0f : info->v_fp;
	btdemux->h_fp = (!info->h_fp) ? 0x0f : info->h_fp;
	btdemux->sync_code_part_a[0] = info->sync_code_part_a[0];
	btdemux->sync_code_part_a[1] = info->sync_code_part_a[1];
	btdemux->sync_code_part_a[2] = info->sync_code_part_a[2];
	for (i = 0; i < BT_DEMUX_NUM; i++) {
		btdemux->sync_code_part_b[i].sav_vld = info->sync_code_part_b[i].sav_vld;
		btdemux->sync_code_part_b[i].sav_blk = info->sync_code_part_b[i].sav_blk;
		btdemux->sync_code_part_b[i].eav_vld = info->sync_code_part_b[i].eav_vld;
		btdemux->sync_code_part_b[i].eav_blk = info->sync_code_part_b[i].eav_blk;
	}
	btdemux->demux = (enum cif_btdmux_mode_e)info->mode;
	btdemux->yc_exchg = info->yc_exchg;

	cif_streaming(ctx, 1, 0);

	return 0;
}
#endif // BT_DEMUX_IF

#ifdef BT656_IF
static int _cif_set_attr_bt656_9b(struct cif_dev *dev,
				  struct cif_ctx *ctx,
				  struct combo_dev_attr_s *attr)
{
	struct cif_param *param = ctx->cur_config;
	struct param_ttl *ttl = &param->cfg.ttl;
	struct link *link = ctx_to_link(ctx);
	enum ttl_src_e vi = attr->ttl_attr.vi;
	int i;

	/* config the pinmux */
	for (i = 0; i < VI_FUNC_NUM; i++) {
		if (attr->ttl_attr.func[i] < 0)
			continue;
		if (attr->ttl_attr.func[i] >= MAX_PAD_NUM)
			return -EINVAL;
		cif_set_ttl_pinmux(ctx, (enum ttl_vi_from_e)vi, i, attr->ttl_attr.func[i]);
#ifndef FPGA_PORTING
		cif_config_pinmux(vi, attr->ttl_attr.func[i]);
#endif
	}
#ifndef FPGA_PORTING
	if (vi == TTL_VI_SRC_VI0){
		if (attr->ttl_attr.vi0_clk == 0)
			PINMUX_CONFIG(PAD_MIPI_RX12P, VI0_CLK0, PHY);
		else if (attr->ttl_attr.vi0_clk == 1)
			PINMUX_CONFIG(PAD_MIPI_RX6P, VI0_CLK1, PHY);
	} else if (vi == TTL_VI_SRC_VI1) {
		PINMUX_CONFIG(PAD_MIPI_RX6P, VI1_CLK0, PHY);
	} else {
		PINMUX_CONFIG(PAD_MIPI_RX0P, VI2_CLK0, PHY);
	}
#endif
	param->type = CIF_TYPE_TTL;
	ttl->vi_from = (enum ttl_vi_from_e)vi;
	ttl->fmt_out = TTL_BT_FMT_OUT_CBYCRY;
	ttl->fmt = TTL_SYNC_PAT_9B_BT656;
	ttl->width = attr->img_size.width - 1;
	ttl->height = attr->img_size.height - 1;
	ttl->sensor_fmt = TTL_SENSOR_12_BIT;
	ttl->clk_inv = link->clk_edge;
	ttl->vi_sel = VI_BT656;
	ttl->v_bp = (!attr->ttl_attr.v_bp) ? 0xf : attr->ttl_attr.v_bp;
	ttl->h_bp = (!attr->ttl_attr.h_bp) ? 0xf : attr->ttl_attr.h_bp;

	cif_streaming(ctx, 1, 0);

	return 0;
}

static int _cif_set_attr_bt656_9b_ddr(struct cif_dev *dev,
				  struct cif_ctx *ctx,
				  struct combo_dev_attr_s *attr)
{
	struct cif_param *param = ctx->cur_config;
	struct param_ttl *ttl = &param->cfg.ttl;
	struct link *link = ctx_to_link(ctx);
	enum ttl_src_e vi = attr->ttl_attr.vi;
	int i;

#ifndef FPGA_PORTING
	if (vi == TTL_VI_SRC_VI0){
		printk("BT_656_DDR can not use VI0\n");
		return -EINVAL;
	} else if (vi == TTL_VI_SRC_VI1) {
		PINMUX_CONFIG(PAD_MIPI_RX6P, VI1_CLK0, PHY);
	} else {
		PINMUX_CONFIG(PAD_MIPI_RX0P, VI2_CLK0, PHY);
	}
#endif

	/* config the pinmux */
	for (i = 0; i < VI_FUNC_NUM; i++) {
		if (attr->ttl_attr.func[i] < 0)
			continue;
		if (attr->ttl_attr.func[i] >= MAX_PAD_NUM)
			return -EINVAL;
		cif_set_mac_vi_pinmux(ctx, (enum ttl_vi_from_e)vi, i, attr->ttl_attr.func[i]);
#ifndef FPGA_PORTING
		cif_config_pinmux(vi, attr->ttl_attr.func[i]);
#endif
	}
	param->type = CIF_TYPE_TTL;
	ttl->vi_from = (enum ttl_vi_from_e)vi;
	ttl->fmt_out = TTL_BT_FMT_OUT_CBYCRY;
	ttl->fmt = TTL_SYNC_PAT_9B_BT656;
	ttl->width = attr->img_size.width - 1;
	ttl->height = attr->img_size.height - 1;
	ttl->sensor_fmt = TTL_SENSOR_12_BIT;
	ttl->clk_inv = link->clk_edge;
	ttl->vi_sel = VI_BT656;
	ttl->v_bp = (!attr->ttl_attr.v_bp) ? 0xf : attr->ttl_attr.v_bp;
	ttl->h_bp = (!attr->ttl_attr.h_bp) ? 0xf : attr->ttl_attr.h_bp;

	cif_streaming(ctx, 1, 0);

	return 0;
}
#endif // BT656_IF

#ifdef	CUSTOM0_IF
static int _cif_set_attr_custom0(struct cif_dev *dev,
				struct cif_ctx *ctx,
				struct combo_dev_attr_s *attr)
{
	struct cif_param *param = ctx->cur_config;
	struct param_ttl *ttl = &param->cfg.ttl;
	enum ttl_src_e vi = attr->ttl_attr.vi;
	int i;

	if (vi == TTL_VI_SRC_VI2)
		return -EINVAL;

	/* config the pinmux */
	for (i = 0; i < VI_FUNC_NUM; i++) {
		if (attr->ttl_attr.func[i] < 0)
			continue;
		if (attr->ttl_attr.func[i] >= MAX_PAD_NUM)
			return -EINVAL;
		cif_set_ttl_pinmux(ctx, (enum ttl_vi_from_e)vi, i, attr->ttl_attr.func[i]);
#ifndef FPGA_PORTING
		cif_config_pinmux(vi, attr->ttl_attr.func[i]);
#endif
	}
#ifndef FPGA_PORTING
	if (vi == TTL_VI_SRC_VI0){
		if (attr->ttl_attr.vi0_clk == 0)
			PINMUX_CONFIG(PAD_MIPI_RX12P, VI0_CLK0, PHY);
		else if (attr->ttl_attr.vi0_clk == 1)
			PINMUX_CONFIG(PAD_MIPI_RX6P, VI0_CLK1, PHY);
	} else if (vi == TTL_VI_SRC_VI1) {
		PINMUX_CONFIG(PAD_MIPI_RX6P, VI1_CLK0, PHY);
	} else {
		PINMUX_CONFIG(PAD_MIPI_RX0P, VI2_CLK0, PHY);
	}
#endif
	param->type = CIF_TYPE_TTL;
	ttl->vi_from = (enum ttl_vi_from_e)vi;
	ttl->fmt_out = TTL_BT_FMT_OUT_CBYCRY;
	ttl->fmt = TTL_CUSTOM_0;
	ttl->width = attr->img_size.width - 1;
	ttl->height = attr->img_size.height - 1;
	ttl->sensor_fmt = TTL_SENSOR_12_BIT;
	ttl->vi_sel = VI_BT601;
	ttl->v_bp = (!attr->ttl_attr.v_bp) ? 4095 : attr->ttl_attr.v_bp;
	ttl->h_bp = (!attr->ttl_attr.h_bp) ? 4 : attr->ttl_attr.h_bp;

	cif_streaming(ctx, 1, 0);

	return 0;
}
#endif // CUSTOM0_IF

/*
 * 1. Precondition: the mac_max = 400M by default, can be adjust by ioctl (app).
 * 2. If mac_max is below 400M, the vip_sys_2 is 400M (parent: mipimpll).
 * 3. If mac_max = 500M, the vip_sys_2 is 500M. (parent: fpll, 1835 OD case)
 * 4. If mac_max = 600M, the vip_sys_2 is 600M. (parent: mipimpll 1838 case)
 * 5. Return error in the others.
 */
static inline unsigned int _cif_mac_enum_to_value(enum rx_mac_clk_e mac_clk)
{
	unsigned int val;

	switch (mac_clk) {
	case RX_MAC_CLK_150M:
		val = 150;
		break;
	case RX_MAC_CLK_200M:
		val = 200;
		break;
	case RX_MAC_CLK_300M:
		val = 300;
		break;
	case RX_MAC_CLK_500M:
		val = 500;
		break;
	case RX_MAC_CLK_600M:
		val = 600;
		break;
	case RX_MAC_CLK_900M:
		val = 900;
		break;
	case RX_MAC_CLK_400M:
	default:
		val = 400;
		break;
	}
	return val;
}

static int _cif_set_mac_clk(struct cif_dev *cdev, uint32_t devno,
		enum rx_mac_clk_e mac_clk)
{
	struct link *link = &cdev->link[devno];
	struct cif_ctx *ctx;
	u32 addr, data, mask, tmp;
	u32 clk_val = _cif_mac_enum_to_value(mac_clk);

	link->mac_clk = mac_clk;
	ctx = &cdev->link[devno].cif_ctx;

#ifdef FPGA_PORTING
	// FPGA don't set mac spped.
	return 0;
#endif
	/* select the source to vip_sys2 */
	switch (ctx->mac_num) {
	case CIF_MAC_1:
		addr = VI_SYS_CLK_CSI_MAC1_SRC_SEL;
		mask = VI_SYS_CLK_CSI_MAC1_SRC_SEL_MASK;
		data = 0x2 << VI_SYS_CLK_CSI_MAC1_SRC_SEL_OFFSET;
		break;
	case CIF_MAC_2:
		addr = VI_SYS_CLK_CSI_MAC2_SRC_SEL;
		mask = VI_SYS_CLK_CSI_MAC2_SRC_SEL_MASK;
		data = 0x2 << VI_SYS_CLK_CSI_MAC2_SRC_SEL_OFFSET;
		break;
	case CIF_MAC_3:
		addr = VI_SYS_CLK_CSI_MAC3_SRC_SEL;
		mask = VI_SYS_CLK_CSI_MAC3_SRC_SEL_MASK;
		data = 0x2 << VI_SYS_CLK_CSI_MAC3_SRC_SEL_OFFSET;
		break;
	case CIF_MAC_4:
		addr = VI_SYS_CLK_CSI_MAC4_SRC_SEL;
		mask = VI_SYS_CLK_CSI_MAC4_SRC_SEL_MASK;
		data = 0x2 << VI_SYS_CLK_CSI_MAC4_SRC_SEL_OFFSET;
		break;
	case CIF_MAC_5:
		addr = VI_SYS_CLK_CSI_MAC5_SRC_SEL;
		mask = VI_SYS_CLK_CSI_MAC5_SRC_SEL_MASK;
		data = 0x2 << VI_SYS_CLK_CSI_MAC5_SRC_SEL_OFFSET;
		break;
	case CIF_MAC_0:
	default:
		addr = VI_SYS_CLK_CSI_MAC0_SRC_SEL;
		mask = VI_SYS_CLK_CSI_MAC0_SRC_SEL_MASK;
		data = 0x2 << VI_SYS_CLK_CSI_MAC0_SRC_SEL_OFFSET;
		break;
	}
	vi_sys_reg_write_mask(addr, mask, data);

	if (bypass_mac_clk)
		return 0;

	// if (clk_val > cdev->max_mac_clk) {
	// 	dev_err(link->dev, "cannot reach %dM since the max clk is %dM\n", clk_val, cdev->max_mac_clk);
	// 	return -EINVAL;
	// }

	/*A2 no need to set paernt clk, mipipll0 == mipipll1 = 1800M*/
	//if (cdev->max_mac_clk == 900) {
	//	dev_dbg(link->dev, "use div1 fpll as vip_sys2 source\n");
	//	clk_set_parent(cdev->vip_sys2.clk_o, cdev->clk_fpll.clk_o);
	//} else {
	//	dev_dbg(link->dev, "use div0 clk_disppll as vip_sys2 source\n");
	//	clk_set_parent(cdev->vip_sys2.clk_o, cdev->clk_disppll.clk_o);
	//}

	{
	switch (ctx->mac_num) {
	case CIF_MAC_0:
		/* target = source * (1 + ratio) / 32, ratio <= 0x1F */
		/* ratio = target * 32 / source -1*/
		tmp = clk_val * 32 / 900;
		/* roundup */
		if ((clk_val * 32) % 900)
			tmp++;
		tmp--;
		if (clk_val >= 900) {
			tmp = 31;
		}
		break;
	case CIF_MAC_1:
	case CIF_MAC_2:
	case CIF_MAC_3:
	case CIF_MAC_4:
	case CIF_MAC_5:
		/* target = source * (1 + ratio) / 32, ratio <= 0x1F */
		/* ratio = target * 32 / source -1*/
		tmp = clk_val * 32 / 600;
		/* roundup */
		if ((clk_val * 32) % 600)
			tmp++;
		tmp--;
		if (clk_val >= 600) {
			tmp = 31;
		}
		break;
	default:
		break;
	}

	switch (ctx->mac_num) {
	case CIF_MAC_1:
		VI_SYS_NORM_CLK_RATIO_CONFIG(VAL_CSI_MAC1, tmp);
		VI_SYS_NORM_CLK_RATIO_CONFIG(EN_CSI_MAC1, 1);
		VI_SYS_UPDATE_CLK_RATIO(SEL_CSI_MAC1);
		break;
	case CIF_MAC_2:
		VI_SYS_NORM_CLK_RATIO_CONFIG(VAL_CSI_MAC2, tmp);
		VI_SYS_NORM_CLK_RATIO_CONFIG(EN_CSI_MAC2, 1);
		VI_SYS_UPDATE_CLK_RATIO(SEL_CSI_MAC2);
		break;
	case CIF_MAC_3:
		VI_SYS_NORM_CLK_RATIO_CONFIG(VAL_CSI_MAC3, tmp);
		VI_SYS_NORM_CLK_RATIO_CONFIG(EN_CSI_MAC3, 1);
		VI_SYS_UPDATE_CLK_RATIO(SEL_CSI_MAC3);
		break;
	case CIF_MAC_4:
		VI_SYS_NORM_CLK_RATIO_CONFIG(VAL_CSI_MAC4, tmp);
		VI_SYS_NORM_CLK_RATIO_CONFIG(EN_CSI_MAC4, 1);
		VI_SYS_UPDATE_CLK_RATIO(SEL_CSI_MAC4);
		break;
	case CIF_MAC_5:
		VI_SYS_NORM_CLK_RATIO_CONFIG(VAL_CSI_MAC5, tmp);
		VI_SYS_NORM_CLK_RATIO_CONFIG(EN_CSI_MAC5, 1);
		VI_SYS_UPDATE_CLK_RATIO(SEL_CSI_MAC5);
		break;
	case CIF_MAC_0:
	default:
		VI_SYS_NORM_CLK_RATIO_CONFIG(VAL_CSI_MAC0, tmp);
		VI_SYS_NORM_CLK_RATIO_CONFIG(EN_CSI_MAC0, 1);
		VI_SYS_UPDATE_CLK_RATIO(SEL_CSI_MAC0);
		break;
	}

	//clk_set_rate(clk_get_parent(cdev->vip_sys2.clk_o), cdev->max_mac_clk * 1000000UL);
	dev_dbg(link->dev, "ratio %d, set rate %dM\n", tmp, (tmp + 1) * cdev->max_mac_clk / 32);
	udelay(5);
	}

	return 0;
}

static inline int cif_set_mac_clk(struct cif_dev *dev, uint32_t devno,
		enum rx_mac_clk_e mac_clk)
{
	return _cif_set_mac_clk(dev, devno, mac_clk);
}

static int cif_set_dev_attr(struct cif_dev *dev,
			    struct combo_dev_attr_s *attr)
{
	struct device *_dev = dev->miscdev.this_device;
	enum input_mode_e input_mode = attr->input_mode;
	struct cif_ctx *ctx;
	struct cif_param *param;
	struct combo_dev_attr_s *rx_attr;
	int rc = 0;

	/* force the btdmux to devno 0*/
#ifdef FPGA_PORTING
	if (input_mode == INPUT_MODE_BT_DEMUX) {
		attr->devno = CIF_MAC_VI_0;
	}
#endif

	ctx = &dev->link[attr->devno].cif_ctx;
	param = &dev->link[attr->devno].param;
	rx_attr = &dev->link[attr->devno].attr;

	cif_dump_dev_attr(dev, attr);

	memset(param, 0, sizeof(*param));
	ctx->cur_config = param;
	ctx->phy_mode = attr->cif_mode;
	memcpy(rx_attr, attr, sizeof(*rx_attr));

#ifndef FPGA_PORTING
	/* Setting for serial interface. */
	if (input_mode == INPUT_MODE_MIPI ||
	    input_mode == INPUT_MODE_SUBLVDS ||
	    input_mode == INPUT_MODE_HISPI) {
		struct clk_edge_s clk_edge;

		/* set the default clock edge. */
		clk_edge.devno = attr->devno;
		clk_edge.edge = CLK_DOWN_EDGE;
		//cif_set_output_clk_edge(dev, &clk_edge);
	}
#endif
	/* set mac clk */
	if (attr->devno < CIF_MAC_VI_0) {
		rc = cif_set_mac_clk(dev, attr->devno, attr->mac_clk);
		if (rc < 0)
			return rc;
	}

	/* decide the mclk */
	if (!attr->mclk.cam)
		mclk0 = attr->mclk.freq;
	else if (attr->mclk.cam == 1)
		mclk1 = attr->mclk.freq;
	else if (attr->mclk.cam == 2)
		mclk2 = attr->mclk.freq;
	else if (attr->mclk.cam == 3)
		mclk3 = attr->mclk.freq;
	else if (attr->mclk.cam == 4)
		mclk4 = attr->mclk.freq;
	else if (attr->mclk.cam == 5)
		mclk5 = attr->mclk.freq;
	else if (attr->mclk.cam == 6)
		mclk6 = attr->mclk.freq;

	switch (input_mode) {
#ifdef MIPI_IF
	case INPUT_MODE_MIPI:
		rc = _cif_set_attr_mipi(dev, ctx, &rx_attr->mipi_attr, attr->devno);
		break;
#endif
#ifdef SUBLVDS_IF
	case INPUT_MODE_SUBLVDS:
		rc = _cif_set_attr_sublvds(dev, ctx, &rx_attr->lvds_attr, attr->devno);
		break;
#endif
#ifdef HISPI_IF
	case INPUT_MODE_HISPI:
		rc = _cif_set_attr_hispi(dev, ctx, &rx_attr->lvds_attr, attr->devno);
		break;
#endif
#ifdef DVP_IF
	case INPUT_MODE_CMOS:
		rc = _cif_set_attr_cmos(dev, ctx, rx_attr);
		break;
#endif
#ifdef BT1120_IF
	case INPUT_MODE_BT1120:
		rc = _cif_set_attr_bt1120(dev, ctx, rx_attr);
		break;
#endif
#ifdef BT601_IF
	case INPUT_MODE_BT601:
		rc = _cif_set_attr_bt601(dev, ctx, rx_attr);
		break;
#endif
#ifdef BT656_IF
	case INPUT_MODE_BT656_9B:
		rc = _cif_set_attr_bt656_9b(dev, ctx, rx_attr);
		break;
	case INPUT_MODE_BT656_9B_DDR:
		rc = _cif_set_attr_bt656_9b_ddr(dev, ctx, rx_attr);
		break;
#endif
#ifdef CUSTOM0_IF
	case INPUT_MODE_CUSTOM_0:
		rc = _cif_set_attr_custom0(dev, ctx, rx_attr);
		break;
#endif
#ifdef BT_DEMUX_IF
	case INPUT_MODE_BT_DEMUX:
		rc = _cif_set_attr_bt_demux(dev, ctx, rx_attr);
		break;
#endif
	default:
		return -EINVAL;
	}
	if (rc < 0) {
		dev_err(_dev, "set attr fail %d\n", rc);
		return rc;
	}

	dev->link[attr->devno].is_on = 1;
	/* unmask the interrupts */
	if (attr->devno < CIF_MAX_CSI_NUM)
		cif_unmask_csi_int_sts(ctx, 0x1F);

	return 0;
}

static int cif_set_output_clk_edge(struct cif_dev *dev,
				   struct clk_edge_s *clk_edge)
{
	struct cif_ctx *ctx = &dev->link[clk_edge->devno].cif_ctx;
	enum phy_lane_id_e phy_id;

	dev->link[clk_edge->devno].clk_edge = clk_edge->edge;

	for (phy_id = CIF_PHY_LANE_0; phy_id < CIF_PHY_LANE_NUM; phy_id++) {
		cif_set_clk_edge(ctx, phy_id,
				 (clk_edge->edge == CLK_UP_EDGE)
				 ? CIF_CLK_RISING : CIF_CLK_FALLING);
	}

	return 0;
}

static void cif_reset_param(struct link *link)
{
	link->is_on = 0;
	link->clk_edge = CLK_UP_EDGE;
	link->msb = OUTPUT_NORM_MSB;
	link->crop_top = 0;
	link->distance_fp = 0;
	memset(&link->param, 0, sizeof(struct cif_param));
	memset(&link->attr, 0, sizeof(struct combo_dev_attr_s));
	memset(&link->sts_csi, 0, sizeof(struct csi_status));
	memset(&link->sts_lvds, 0, sizeof(struct lvds_status));
}

static int cif_reset_mipi(struct cif_dev *dev, uint32_t devno)
{
	union vi_sys_reset mask;
	struct link *link = &dev->link[devno];
	mask.raw = 0;
	/* mask the interrupts */
	if (link->is_on)
		cif_mask_csi_int_sts(&link->cif_ctx, 0x1F);

	/* reset phy */
//#ifndef FPGA_PORTING
//	if (link->phy_reset) {
//		reset_control_assert(link->phy_reset);
//		udelay(5);
//		reset_control_deassert(link->phy_reset);
//	}
//#endif
	/* sw reset mac by vip register */
	switch (devno) {
	case 0:
		mask.b.csi_mac0 = 1;
		break;
	case 1:
		mask.b.csi_mac1 = 1;
		break;
	case 2:
		mask.b.csi_mac2 = 1;
		break;
	case 3:
		mask.b.csi_mac3 = 1;
		break;
	case 4:
		mask.b.csi_mac4 = 1;
		break;
	case 5:
		mask.b.csi_mac5 = 1;
		break;
	case 6:
		mask.b.csi_mac6 = 1;
		break;
	case 7:
		mask.b.csi_mac7 = 1;
		break;
	default:
		break;
	}
	vi_sys_toggle_reset(mask);
	/* reset parameters. */
	cif_reset_param(link);

	return 0;
}

static inline int cif_set_crop_top(struct cif_dev *dev,
			    struct crop_top_s *crop)
{
	struct cif_ctx *ctx = &dev->link[crop->devno].cif_ctx;

	dev->link[crop->devno].crop_top = crop->crop_top;
	/* strip the top info line. */
	cif_crop_info_line(ctx, crop->crop_top, crop->update);

	return 0;
}

static inline int cif_set_windowing(struct cif_dev *dev,
			    struct cif_crop_win_s *win)
{
	struct cif_ctx *ctx = &dev->link[win->devno].cif_ctx;
	struct cif_crop_s crop;

	crop.x = win->x;
	crop.y = win->y;
	crop.w = win->w;
	crop.h = win->h;
	crop.enable = win->enable;

	cif_set_crop(ctx, &crop);

	return 0;
}

static inline int cif_set_wdr_manual(struct cif_dev *dev,
			      struct manual_wdr_s *manual)
{
	struct cif_ctx *ctx = &dev->link[manual->devno].cif_ctx;
	struct cif_param *param = &dev->link[manual->devno].param;

	param->hdr_manual = manual->attr.manual_en;
	param->hdr_shift = manual->attr.l2s_distance;
	param->hdr_vsize = manual->attr.lsef_length;
	param->hdr_rm_padding = manual->attr.discard_padding_lines;

	cif_hdr_manual_config(ctx, param, !!manual->attr.update);

	return 0;
}

static inline int cif_get_cif_attr(struct cif_dev *dev,
			    struct cif_attr_s *cif_attr)
{
	struct cif_param *param = &dev->link[cif_attr->devno].param;

	if (param->type == CIF_TYPE_CSI && param->hdr_en) {
		struct param_csi *csi = &param->cfg.csi;

		cif_attr->stagger_vsync = (csi->hdr_mode == CSI_HDR_MODE_VC);
	} else
		cif_attr->stagger_vsync = 0;

	return 0;
}

static inline int cif_set_lvds_fp_vs(struct cif_dev *dev,
			      struct vsync_gen_s *vs)
{
	struct cif_ctx *ctx = &dev->link[vs->devno].cif_ctx;

	dev->link[vs->devno].distance_fp = vs->distance_fp;
	cif_set_lvds_vsync_gen(ctx, vs->distance_fp);

	return 0;
}

struct cam_pll_s {
	unsigned int	div_val_sel;
	unsigned int	src_sel;
	unsigned int	div_val;
};

const struct cam_pll_s cam_pll_setting[CAMPLL_FREQ_NUM] = {
	[CAMPLL_FREQ_37P125M] = {
		.div_val_sel = 1,
		.src_sel = 3,
		.div_val = 16,
	},
	[CAMPLL_FREQ_25M] = {
		.div_val_sel = 1,
		.src_sel = 0,
		.div_val = 72,
	},
	[CAMPLL_FREQ_27M] = {
		.div_val_sel = 1,
		.src_sel = 3,
		.div_val = 22,
	},
	[CAMPLL_FREQ_24M] = {
		.div_val_sel = 1,
		.src_sel = 0,
		.div_val = 75,
	},
	[CAMPLL_FREQ_26M] = {
		.div_val_sel = 1,
		.src_sel = 2,
		.div_val = 25,
	},
};

void write_value_range(u32 *data, unsigned int start_bit, unsigned int end_bit, unsigned int value)
{
	unsigned int num_bits;
	unsigned int max_value;
	unsigned int mask;

	if (start_bit > end_bit) {
		printk("[ERROR]: start_bit must be less than or equal to end_bit.\n");
		return;
	}

	num_bits = end_bit - start_bit + 1;
	max_value = (1U << num_bits) - 1;

	if (value > max_value) {
		printk("[ERROR]: value exceeds the range defined by start_bit and end_bit.\n");
		return;
	}

	mask = max_value << start_bit;
	*data = (*data & ~mask) | ((value << start_bit) & mask);
}

static int _cif_enable_snsr_clk(struct device *dev,
				struct cif_dev *cdev,
				uint32_t devno, uint8_t on)
{
#ifndef FPGA_PORTING
	u32 clk_div, clk_src, clk_update;

	if (mclk0 > CAMPLL_FREQ_NONE && mclk0 < CAMPLL_FREQ_NUM) {
		if (on) {
			if (!cdev->clk_cam0.is_on) {
				printk("start set mclk0 \n");
				mclk_enable = ioremap(CIF_MCLK_GEN_BASE + CIF_MCLK_GROUP0_PLL + CIF_MCLK0_GEN_OFFSET,
					sizeof(u32));
				mclk_src = ioremap(CIF_MCLK_GEN_BASE + CIF_MCLK_GROUP0_PLL + CIF_MCLK0_SRC_OFFSET,
					sizeof(u32));
				mclk_update = ioremap(CIF_MCLK_GEN_BASE + CIF_MCLK_GROUP0_PLL + CIF_MCLK_UPDATE_OFFSET,
					sizeof(u32));
				if (!mclk_enable || !mclk_src || !mclk_update) {
					printk("Failed to map mclk0 register\n");
					return -ENOMEM;
				}
				clk_div = readl(mclk_enable);
				clk_src = readl(mclk_src);
				clk_update = readl(mclk_update);
				write_value_range(&clk_div, 16, 23, cam_pll_setting[mclk0].div_val);
				write_value_range(&clk_src, 2, 3, cam_pll_setting[mclk0].src_sel);
				write_value_range(&clk_update, 0, 0, cam_pll_setting[mclk0].div_val_sel);
				writel(clk_div, mclk_enable);
				writel(clk_src, mclk_src);
				writel(clk_update, mclk_update);
				iounmap(mclk_enable);
				iounmap(mclk_src);
				iounmap(mclk_update);
				cdev->clk_cam0.is_on = 1;
			}
		} else {
			if (cdev->clk_cam0.is_on) {
				cdev->clk_cam0.is_on = 0;
			}
		}
	}

	if (mclk1 > CAMPLL_FREQ_NONE && mclk1 < CAMPLL_FREQ_NUM) {
		printk("start set mclk1 \n");
		if (on) {
			if (!cdev->clk_cam1.is_on) {
				mclk_enable = ioremap(CIF_MCLK_GEN_BASE + CIF_MCLK_GROUP0_PLL + CIF_MCLK1_GEN_OFFSET,
					sizeof(u32));
				mclk_src = ioremap(CIF_MCLK_GEN_BASE + CIF_MCLK_GROUP0_PLL + CIF_MCLK1_SRC_OFFSET,
					sizeof(u32));
				mclk_update = ioremap(CIF_MCLK_GEN_BASE + CIF_MCLK_GROUP0_PLL + CIF_MCLK_UPDATE_OFFSET,
					sizeof(u32));
				if (!mclk_enable || !mclk_src || !mclk_update) {
					printk("Failed to map mclk1 register\n");
					return -ENOMEM;
				}
				clk_div = readl(mclk_enable);
				clk_src = readl(mclk_src);
				clk_update = readl(mclk_update);
				write_value_range(&clk_div, 16, 23, cam_pll_setting[mclk1].div_val);
				write_value_range(&clk_src, 2, 3, cam_pll_setting[mclk1].src_sel);
				write_value_range(&clk_update, 0, 0, cam_pll_setting[mclk1].div_val_sel);
				writel(clk_div, mclk_enable);
				writel(clk_src, mclk_src);
				writel(clk_update, mclk_update);
				iounmap(mclk_enable);
				iounmap(mclk_src);
				iounmap(mclk_update);
				cdev->clk_cam1.is_on = 1;
			}
		} else {
			if (cdev->clk_cam1.is_on) {
				cdev->clk_cam1.is_on = 0;
			}
		}
	}

	if (mclk2 > CAMPLL_FREQ_NONE && mclk2 < CAMPLL_FREQ_NUM) {
		printk("start set mclk2 \n");
		if (on) {
			if (!cdev->clk_cam2.is_on) {
				mclk_enable = ioremap(CIF_MCLK_GEN_BASE + CIF_MCLK_GROUP0_PLL + CIF_MCLK2_GEN_OFFSET,
					sizeof(u32));
				mclk_src = ioremap(CIF_MCLK_GEN_BASE + CIF_MCLK_GROUP0_PLL + CIF_MCLK2_SRC_OFFSET,
					sizeof(u32));
				mclk_update = ioremap(CIF_MCLK_GEN_BASE + CIF_MCLK_GROUP0_PLL + CIF_MCLK_UPDATE_OFFSET,
					sizeof(u32));
				if (!mclk_enable || !mclk_src || !mclk_update) {
					printk("Failed to map mclk2 register\n");
					return -ENOMEM;
				}
				clk_div = readl(mclk_enable);
				clk_src = readl(mclk_src);
				clk_update = readl(mclk_update);
				write_value_range(&clk_div, 16, 23, cam_pll_setting[mclk2].div_val);
				write_value_range(&clk_src, 2, 3, cam_pll_setting[mclk2].src_sel);
				write_value_range(&clk_update, 0, 0, cam_pll_setting[mclk2].div_val_sel);
				writel(clk_div, mclk_enable);
				writel(clk_src, mclk_src);
				writel(clk_update, mclk_update);
				iounmap(mclk_enable);
				iounmap(mclk_src);
				iounmap(mclk_update);
				cdev->clk_cam2.is_on = 1;
			}
		} else {
			if (cdev->clk_cam2.is_on) {
				cdev->clk_cam2.is_on = 0;
			}
		}
	}

	if (mclk3 > CAMPLL_FREQ_NONE && mclk3 < CAMPLL_FREQ_NUM) {
		printk("start set mclk3 \n");
		if (on) {
			if (!cdev->clk_cam3.is_on) {
				mclk_enable = ioremap(CIF_MCLK_GEN_BASE + CIF_MCLK_GROUP1_PLL + CIF_MCLK0_GEN_OFFSET,
					sizeof(u32));
				mclk_src = ioremap(CIF_MCLK_GEN_BASE + CIF_MCLK_GROUP1_PLL + CIF_MCLK0_SRC_OFFSET,
					sizeof(u32));
				mclk_update = ioremap(CIF_MCLK_GEN_BASE + CIF_MCLK_GROUP1_PLL + CIF_MCLK_UPDATE_OFFSET,
					sizeof(u32));
				if (!mclk_enable || !mclk_src || !mclk_update) {
					printk("Failed to map mclk3 register\n");
					return -ENOMEM;
				}
				clk_div = readl(mclk_enable);
				clk_src = readl(mclk_src);
				clk_update = readl(mclk_update);
				write_value_range(&clk_div, 16, 23, cam_pll_setting[mclk3].div_val);
				write_value_range(&clk_src, 2, 3, cam_pll_setting[mclk3].src_sel);
				write_value_range(&clk_update, 0, 0, cam_pll_setting[mclk3].div_val_sel);
				writel(clk_div, mclk_enable);
				writel(clk_src, mclk_src);
				writel(clk_update, mclk_update);
				iounmap(mclk_enable);
				iounmap(mclk_src);
				iounmap(mclk_update);
				cdev->clk_cam3.is_on = 1;
			}
		} else {
			if (cdev->clk_cam3.is_on) {
				cdev->clk_cam3.is_on = 0;
			}
		}
	}

	if (mclk4 > CAMPLL_FREQ_NONE && mclk4 < CAMPLL_FREQ_NUM) {
		printk("start set mclk4 \n");
		if (on) {
			if (!cdev->clk_cam4.is_on) {
				mclk_enable = ioremap(CIF_MCLK_GEN_BASE + CIF_MCLK_GROUP1_PLL + CIF_MCLK1_GEN_OFFSET,
					sizeof(u32));
				mclk_src = ioremap(CIF_MCLK_GEN_BASE + CIF_MCLK_GROUP1_PLL + CIF_MCLK1_SRC_OFFSET,
					sizeof(u32));
				mclk_update = ioremap(CIF_MCLK_GEN_BASE + CIF_MCLK_GROUP1_PLL + CIF_MCLK_UPDATE_OFFSET,
					sizeof(u32));
				if (!mclk_enable || !mclk_src || !mclk_update) {
					printk("Failed to map mclk4 register\n");
					return -ENOMEM;
				}
				clk_div = readl(mclk_enable);
				clk_src = readl(mclk_src);
				clk_update = readl(mclk_update);
				write_value_range(&clk_div, 16, 23, cam_pll_setting[mclk4].div_val);
				write_value_range(&clk_src, 2, 3, cam_pll_setting[mclk4].src_sel);
				write_value_range(&clk_update, 0, 0, cam_pll_setting[mclk4].div_val_sel);
				writel(clk_div, mclk_enable);
				writel(clk_src, mclk_src);
				writel(clk_update, mclk_update);
				iounmap(mclk_enable);
				iounmap(mclk_src);
				iounmap(mclk_update);
				cdev->clk_cam4.is_on = 1;
			}
		} else {
			if (cdev->clk_cam4.is_on) {
				cdev->clk_cam4.is_on = 0;
			}
		}
	}

	if (mclk5 > CAMPLL_FREQ_NONE && mclk5 < CAMPLL_FREQ_NUM) {
		printk("start set mclk5 \n");
		if (on) {
			if (!cdev->clk_cam5.is_on) {
				mclk_enable = ioremap(CIF_MCLK_GEN_BASE + CIF_MCLK_GROUP1_PLL + CIF_MCLK2_GEN_OFFSET,
					sizeof(u32));
				mclk_src = ioremap(CIF_MCLK_GEN_BASE + CIF_MCLK_GROUP1_PLL + CIF_MCLK2_SRC_OFFSET,
					sizeof(u32));
				mclk_update = ioremap(CIF_MCLK_GEN_BASE + CIF_MCLK_GROUP1_PLL + CIF_MCLK_UPDATE_OFFSET,
					sizeof(u32));
				if (!mclk_enable || !mclk_src || !mclk_update) {
					printk("Failed to map mclk5 register\n");
					return -ENOMEM;
				}
				clk_div = readl(mclk_enable);
				clk_src = readl(mclk_src);
				clk_update = readl(mclk_update);
				write_value_range(&clk_div, 16, 23, cam_pll_setting[mclk5].div_val);
				write_value_range(&clk_src, 2, 3, cam_pll_setting[mclk5].src_sel);
				write_value_range(&clk_update, 0, 0, cam_pll_setting[mclk5].div_val_sel);
				writel(clk_div, mclk_enable);
				writel(clk_src, mclk_src);
				writel(clk_update, mclk_update);
				iounmap(mclk_enable);
				iounmap(mclk_src);
				iounmap(mclk_update);
				cdev->clk_cam5.is_on = 1;
			}
		} else {
			if (cdev->clk_cam5.is_on) {
				cdev->clk_cam5.is_on = 0;
			}
		}
	}

	if (mclk6 > CAMPLL_FREQ_NONE && mclk6 < CAMPLL_FREQ_NUM) {
		printk("start set mclk6 \n");
		if (on) {
			if (!cdev->clk_cam6.is_on) {
				mclk_enable = ioremap(CIF_MCLK_GEN_BASE + CIF_MCLK_GROUP2_PLL + CIF_MCLK0_GEN_OFFSET,
					sizeof(u32));
				mclk_src = ioremap(CIF_MCLK_GEN_BASE + CIF_MCLK_GROUP2_PLL + CIF_MCLK0_SRC_OFFSET,
					sizeof(u32));
				mclk_update = ioremap(CIF_MCLK_GEN_BASE + CIF_MCLK_GROUP2_PLL + CIF_MCLK_UPDATE_OFFSET,
					sizeof(u32));
				if (!mclk_enable || !mclk_src || !mclk_update) {
					printk("Failed to map mclk6 register\n");
					return -ENOMEM;
				}
				clk_div = readl(mclk_enable);
				clk_src = readl(mclk_src);
				clk_update = readl(mclk_update);
				write_value_range(&clk_div, 16, 23, cam_pll_setting[mclk6].div_val);
				write_value_range(&clk_src, 2, 3, cam_pll_setting[mclk6].src_sel);
				write_value_range(&clk_update, 0, 0, cam_pll_setting[mclk6].div_val_sel);
				writel(clk_div, mclk_enable);
				writel(clk_src, mclk_src);
				writel(clk_update, mclk_update);
				iounmap(mclk_enable);
				iounmap(mclk_src);
				iounmap(mclk_update);
				cdev->clk_cam6.is_on = 1;
			}
		} else {
			if (cdev->clk_cam6.is_on) {
				cdev->clk_cam6.is_on = 0;
			}
		}
	}

#endif
	return 0;
}

static inline int cif_enable_snsr_clk(struct cif_dev *dev,
				uint32_t devno, uint8_t on)
{
	struct device *_dev = dev->miscdev.this_device;

	return _cif_enable_snsr_clk(_dev, dev, devno, on);
}

static int cif_reset_snsr_gpio(struct cif_dev *dev,
			       sns_rst_config *sns_rst_config, uint8_t on)
{
	struct link *link;
	int ret = 0;
	int reset_pin = 0, reset_port = 0;

	if (sns_rst_config->devno >= MAX_LINK_NUM)
		return -EINVAL;

	reset_port = sns_rst_config->gpio_pin / 32;
	reset_pin = sns_rst_config->gpio_pin % 32;

	if (reset_port == 0) {
		reset_pin = reset_pin + 480;
	} else if (reset_port == 1) {
		reset_pin = reset_pin + 448;
	} else if (reset_port == 2) {
		reset_pin = reset_pin + 416;
	} else if (reset_port == 3) {
		reset_pin = reset_pin + 384;
	} else if (reset_port == 4) {
		reset_pin = reset_pin + 352;
	} else if (reset_port == 5) {
		reset_pin = reset_pin + 320;
	}

	link = &dev->link[sns_rst_config->devno];

	printk("config rst GPIO pin [%d] \n", reset_pin);

    // ret = gpio_request(reset_pin, "snsr-rst-gpio");
    // if (ret < 0) {
    //     printk("Failed to request GPIO pin [%d] \n", reset_pin);
    //     return ret;
    // }

	if (!gpio_is_valid(reset_pin))
		return 0;

	if (on) {
		gpio_free(reset_pin);
		if (sns_rst_config->gpio_active == RST_ACTIVE_LOW) {
			ret = gpio_request_one(reset_pin, GPIOF_OUT_INIT_LOW, "snsr-rst-gpio");
		} else if (sns_rst_config->gpio_active == RST_ACTIVE_HIGH) {
			ret = gpio_request_one(reset_pin, GPIOF_OUT_INIT_HIGH, "snsr-rst-gpio");
		}
		if (ret < 0) {
			pr_err("Failed to set GPIO reset : %d\n", ret);
			return 0;
		}
	} else {
		gpio_free(reset_pin);
		if (sns_rst_config->gpio_active == RST_ACTIVE_LOW) {
			ret = gpio_request_one(reset_pin, GPIOF_OUT_INIT_HIGH, "snsr-rst-gpio");
		} else if (sns_rst_config->gpio_active == RST_ACTIVE_HIGH) {
			ret = gpio_request_one(reset_pin, GPIOF_OUT_INIT_LOW, "snsr-rst-gpio");
		}
		if (ret < 0) {
			pr_err("Failed to set GPIO unreset : %d\n", ret);
			return 0;
		}
	}

	return 0;
}

static inline int cif_reset_lvds(struct cif_dev *dev,
				unsigned int devno)
{
	return 0;
}

static inline int cif_bt_fmt_out(struct cif_dev *dev,
			  struct bt_fmt_out_s *fmt_out)
{
	struct cif_ctx *ctx = &dev->link[fmt_out->devno].cif_ctx;

	dev->link[fmt_out->devno].bt_fmt_out = (enum ttl_bt_fmt_out)fmt_out->fmt_out;
	cif_set_bt_fmt_out(ctx, (enum ttl_bt_fmt_out)fmt_out->fmt_out);

	return 0;
}

static long _cif_ioctl(struct cif_dev *dev, unsigned int cmd,
		       unsigned long arg, unsigned int from_user)
{
	struct device *_dev = dev->miscdev.this_device;
	struct cif_ctx *ctx = NULL;
	uint32_t devno;

	dev_dbg(_dev, "%s\n", _to_string_cmd(cmd));

	if (arg == 0) {
		dev_err(_dev, "null pointer\n");
		return -EINVAL;
	}

	switch (cmd) {
	case MIPI_SET_DEV_ATTR:
	{
		struct combo_dev_attr_s attr;

		if (from_user) {
			if (copy_from_user(&attr, (void *)arg, sizeof(attr))) {
				dev_err(_dev, "copy_from_user failed.\n");
				return -ENOMEM;
			}
		} else
			memcpy(&attr, (void *)arg, sizeof(attr));

		return cif_set_dev_attr(dev, &attr);
	}
	case MIPI_SET_OUTPUT_CLK_EDGE:
	{
		struct clk_edge_s clk;

		if (from_user) {
			if (copy_from_user(&clk, (void *)arg, sizeof(clk))) {
				dev_err(_dev, "copy_from_user failed.\n");
				return -ENOMEM;
			}
		} else
			memcpy(&clk, (void *)arg, sizeof(clk));

		return cif_set_output_clk_edge(dev, &clk);
	}
	case MIPI_RESET_MIPI:
		if (from_user) {
			if (copy_from_user(&devno, (void *)arg, sizeof(devno))) {
				dev_err(_dev, "copy_from_user failed.\n");
				return -ENOMEM;
			}
		} else
			devno = *(uint32_t *)arg;

		return cif_reset_mipi(dev, devno);
	case MIPI_SET_CROP_TOP: // remove info line
	{
		struct crop_top_s crop;

		if (from_user) {
			if (copy_from_user(&crop, (void *)arg, sizeof(crop))) {
				dev_err(_dev, "copy_from_user failed.\n");
				return -ENOMEM;
			}
		} else
			memcpy(&crop, (void *)arg, sizeof(crop));

		return cif_set_crop_top(dev, &crop);
	}
	case MIPI_SET_CROP_WINDOW: // crop input image
	{
		struct cif_crop_win_s win;

		if (from_user) {
			if (copy_from_user(&win, (void *)arg, sizeof(win))) {
				dev_err(_dev, "copy_from_user failed.\n");
				return -ENOMEM;
			}
		} else
			memcpy(&win, (void *)arg, sizeof(win));

		return cif_set_windowing(dev, &win);
	}
	case MIPI_SET_WDR_MANUAL:
	{
		struct manual_wdr_s wdr_manu;

		if (from_user) {
			if (copy_from_user(&wdr_manu, (void *)arg, sizeof(wdr_manu))) {
				dev_err(_dev, "copy_from_user failed.\n");
				return -ENOMEM;
			}
		} else
			memcpy(&wdr_manu, (void *)arg, sizeof(wdr_manu));

		return cif_set_wdr_manual(dev, &wdr_manu);
	}
	case MIPI_SET_LVDS_FP_VS:
	{
		struct vsync_gen_s vsync;

		if (from_user) {
			if (copy_from_user(&vsync, (void *)arg, sizeof(vsync))) {
				dev_err(_dev, "copy_from_user failed.\n");
				return -ENOMEM;
			}
		} else
			memcpy(&vsync, (void *)arg, sizeof(vsync));

		return cif_set_lvds_fp_vs(dev, &vsync);
	}
	case MIPI_RESET_SENSOR:
	{
		sns_rst_config rst_config;

		if (from_user) {
			if (copy_from_user(&rst_config, (void *)arg, sizeof(rst_config))) {
				dev_err(_dev, "copy_from_user failed.\n");
				return -ENOMEM;
			}
		} else
			memcpy(&rst_config, (void *)arg, sizeof(rst_config));
		return cif_reset_snsr_gpio(dev, &rst_config, 1);
	}
	case MIPI_UNRESET_SENSOR:
	{
		sns_rst_config rst_config;

		if (from_user) {
			if (copy_from_user(&rst_config, (void *)arg, sizeof(rst_config))) {
				dev_err(_dev, "copy_from_user failed.\n");
				return -ENOMEM;
			}
		} else
			memcpy(&rst_config, (void *)arg, sizeof(rst_config));
		return cif_reset_snsr_gpio(dev, &rst_config, 0);
	}
	case MIPI_ENABLE_SENSOR_CLOCK:
		if (from_user) {
			if (copy_from_user(&devno, (void *)arg, sizeof(devno))) {
				dev_err(_dev, "copy_from_user failed.\n");
				return -ENOMEM;
			}
		} else
			devno = *(uint32_t *)arg;
		return cif_enable_snsr_clk(dev, devno, 1);
	case MIPI_DISABLE_SENSOR_CLOCK:
		if (from_user) {
			if (copy_from_user(&devno, (void *)arg, sizeof(devno))) {
				dev_err(_dev, "copy_from_user failed.\n");
				return -ENOMEM;
			}
		} else
			devno = *(uint32_t *)arg;
		return cif_enable_snsr_clk(dev, devno, 0);
	case MIPI_RESET_LVDS:
	case CIF_CB_RESET_LVDS:
		if (from_user) {
			if (copy_from_user(&devno, (void *)arg, sizeof(devno))) {
				dev_err(_dev, "copy_from_user failed.\n");
				return -ENOMEM;
			}
		} else
			devno = *(uint32_t *)arg;
		return cif_reset_lvds(dev, devno);
	case MIPI_GET_CIF_ATTR:
	case CIF_CB_GET_CIF_ATTR:
	{
		struct cif_attr_s cif_attr;

		if (from_user) {
			if (copy_from_user(&cif_attr, (void *)arg, sizeof(cif_attr))) {
				dev_err(_dev, "copy_from_user failed.\n");
				return -ENOMEM;
			}
		} else
			memcpy(&cif_attr, (void *)arg, sizeof(cif_attr));

		cif_get_cif_attr(dev, &cif_attr);

		if (from_user) {
			if (copy_to_user((void *)arg, &cif_attr, sizeof(cif_attr))) {
				dev_err(_dev, "copy_to_user failed.\n");
				return -ENOMEM;
			}
		} else
			memcpy((void *)arg, &cif_attr, sizeof(cif_attr));
		return 0;
	}
	case MIPI_SET_BT_FMT_OUT:
	{
		struct bt_fmt_out_s bt_fmt;

		if (from_user) {
			if (copy_from_user(&bt_fmt, (void *)arg, sizeof(bt_fmt))) {
				dev_err(_dev, "copy_from_user failed.\n");
				return -ENOMEM;
			}
		} else
			memcpy(&bt_fmt, (void *)arg, sizeof(bt_fmt));

		return cif_bt_fmt_out(dev, &bt_fmt);
	}
	case MIPI_SET_SENSOR_CLOCK:
	{
		struct mclk_pll_s mclk;

		if (from_user) {
			if (copy_from_user(&mclk, (void *)arg, sizeof(mclk))) {
				dev_err(_dev, "copy_from_user failed.\n");
				return -ENOMEM;
			}
		} else
			memcpy(&mclk, (void *)arg, sizeof(mclk));

		/* sensor enable */
		if (mclk.cam >= MAX_LINK_NUM)
			return -EINVAL;
		if (!mclk.cam)
			mclk0 = mclk.freq;
		else if (mclk.cam == 1)
			mclk1 = mclk.freq;
		else if (mclk.cam == 2)
			mclk2 = mclk.freq;
		else if (mclk.cam == 3)
			mclk3 = mclk.freq;
		else if (mclk.cam == 4)
			mclk4 = mclk.freq;
		else if (mclk.cam == 5)
			mclk5 = mclk.freq;
		else if (mclk.cam == 6)
			mclk6 = mclk.freq;

		return _cif_enable_snsr_clk(_dev, dev, mclk.cam, mclk.freq != CAMPLL_FREQ_NONE);
	}
	case MIPI_SET_MAX_MAC_CLOCK:
		if (from_user) {
			if (copy_from_user(&dev->max_mac_clk, (void *)arg, sizeof(u32))) {
				dev_err(_dev, "copy_from_user failed.\n");
				return -ENOMEM;
			}
		} else
			dev->max_mac_clk = *(uint32_t *)arg;

		if (dev->max_mac_clk <= 400)
			dev->max_mac_clk = 396;
		else if (dev->max_mac_clk <= 500)
			dev->max_mac_clk = 500;
		else
			dev->max_mac_clk = 594;
		break;
	case MIPI_SET_YUV_SWAP:
	{
		struct cif_yuv_swap_s swap;

		if (from_user) {
			if (copy_from_user(&swap, (void *)arg, sizeof(swap))) {
				dev_err(_dev, "copy_from_user failed.\n");
				return -ENOMEM;
			}
		} else
			memcpy(&swap, (void *)arg, sizeof(swap));

		ctx = &dev->link[swap.devno].cif_ctx;

		return cif_swap_yuv(ctx, swap.uv_swap, swap.yc_swap);
	}
	default:
		return -ENOIOCTLCMD;
	}
	return 0;
}

static long cif_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct cif_dev *dev = file_cif_dev(file);

	return _cif_ioctl(dev, cmd, arg, 1);
}

#ifdef CONFIG_COMPAT
static long cif_compat_ptr_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	if (!file->f_op->unlocked_ioctl)
		return -ENOIOCTLCMD;

	return file->f_op->unlocked_ioctl(file, cmd, (unsigned long)compat_ptr(arg));
}
#endif

static int cif_open(struct inode *inode, struct file *file)
{
	return 0;
}

static int cif_release(struct inode *inode, struct file *file)
{
	return 0;
}

static const struct file_operations cif_fops = {
	.owner = THIS_MODULE,
	.open = cif_open,
	.release = cif_release,
	.unlocked_ioctl = cif_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = cif_compat_ptr_ioctl,
#endif
};

static int cif_cb(void *dev, enum enum_modules_id caller, u32 cmd, void *arg)
{
	return _cif_ioctl((struct cif_dev *)dev, cmd, (unsigned long)arg, 0);
}

static int cif_rm_cb(void)
{
	return base_rm_module_cb(E_MODULE_CIF);
}

static int cif_register_cb(struct cif_dev *dev)
{
	struct base_m_cb_info reg_cb;

	reg_cb.module_id	= E_MODULE_CIF;
	reg_cb.dev		= (void *)dev;
	reg_cb.cb		= cif_cb;

	return base_reg_module_cb(&reg_cb);
}

static int cif_init_miscdev(struct platform_device *pdev, struct cif_dev *dev)
{
	int rc, i;

	dev->miscdev.minor = MISC_DYNAMIC_MINOR;
	dev->miscdev.name = MIPI_RX_DEV_NAME;
	dev->miscdev.fops = &cif_fops;

	rc = misc_register(&dev->miscdev);
	if (rc) {
		dev_err(&pdev->dev, "cif: failed to register misc device.\n");
		return rc;
	}

	/* init cif */
	for (i = 0; i < MAX_LINK_NUM; i++) {
		struct cif_ctx *ctx = &dev->link[i].cif_ctx;

		ctx->mac_phys_regs = cif_get_mac_phys_reg_bases(i);
		ctx->wrap_phys_regs = cif_get_wrap_phys_reg_bases(i);
	}

	/* register cif_cb */
	rc = cif_register_cb(dev);
	if (rc)
		dev_err(&pdev->dev, "cif: failed to register callbacks.\n");

	return rc;
}

static irqreturn_t cif_isr(int irq, void *_link)
{
	struct link *link = (struct link *)_link;
	struct cif_ctx *ctx = &link->cif_ctx;

	if (cif_check_csi_int_sts(ctx, CIF_INT_STS_ECC_ERR_MASK))
		link->sts_csi.errcnt_ecc++;
	if (cif_check_csi_int_sts(ctx, CIF_INT_STS_CRC_ERR_MASK))
		link->sts_csi.errcnt_crc++;
	if (cif_check_csi_int_sts(ctx, CIF_INT_STS_WC_ERR_MASK))
		link->sts_csi.errcnt_wc++;
	if (cif_check_csi_int_sts(ctx, CIF_INT_STS_HDR_ERR_MASK))
		link->sts_csi.errcnt_hdr++;
	if (cif_check_csi_int_sts(ctx, CIF_INT_STS_FIFO_FULL_MASK))
		link->sts_csi.fifo_full++;

	if (link->sts_csi.errcnt_ecc > 0xFFFF ||
		link->sts_csi.errcnt_crc > 0xFFFF ||
		link->sts_csi.errcnt_hdr > 0xFFFF ||
		link->sts_csi.fifo_full > 0xFFFF ||
		link->sts_csi.errcnt_wc > 0xFFFF) {

		cif_mask_csi_int_sts(ctx, 0x1F);
		dev_err(link->dev, "mask the interrupt since err cnt is full\n");
		dev_err(link->dev, "ecc = %u, crc = %u, wc = %u, hdr = %u, fifo_full = %u\n",
				link->sts_csi.errcnt_ecc,
				link->sts_csi.errcnt_crc,
				link->sts_csi.errcnt_wc,
				link->sts_csi.errcnt_hdr,
				link->sts_csi.fifo_full);
	}

	cif_clear_csi_int_sts(ctx);

	return IRQ_HANDLED;
}

static char irq_name[MAX_LINK_NUM][20] = {
	"cif-irq0",
	"cif-irq1",
	"cif-irq2",
	"cif-irq3",
	"cif-irq4",
	"cif-irq5",
	"cif-irq6",
	"cif-irq7"
};

static int _init_resource(struct platform_device *pdev)
{
#if (DEVICE_FROM_DTS)
	struct resource *res = NULL;
	void *reg_base[20];
	struct cif_dev *dev;
	int i;
	struct link *link;

	dev = dev_get_drvdata(&pdev->dev);
	if (!dev) {
		dev_err(&pdev->dev, "Can not get cif drvdata\n");
		return -EINVAL;
	}
#ifndef FPGA_PORTING
	/* cam clk shall not depend on the link so we separate them from link. */
	dev->clk_cam0.clk_o = devm_clk_get(&pdev->dev, "clk_cam0");
	if (!IS_ERR_OR_NULL(dev->clk_cam0.clk_o))
		dev_info(&pdev->dev, "cam0 clk installed\n");
	dev->clk_cam1.clk_o = devm_clk_get(&pdev->dev, "clk_cam1");
	if (!IS_ERR_OR_NULL(dev->clk_cam1.clk_o))
		dev_info(&pdev->dev, "cam1 clk installed\n");
	dev->clk_cam2.clk_o = devm_clk_get(&pdev->dev, "clk_cam2");
	if (!IS_ERR_OR_NULL(dev->clk_cam2.clk_o))
		dev_info(&pdev->dev, "cam2 clk installed\n");
	dev->vip_sys2.clk_o = devm_clk_get(&pdev->dev, "clk_sys_2");
	if (!IS_ERR_OR_NULL(dev->vip_sys2.clk_o))
		dev_info(&pdev->dev, "vip_sys_2 clk installed\n");
	dev->clk_mipimpll.clk_o = devm_clk_get(&pdev->dev, "clk_mipimpll");
	if (!IS_ERR_OR_NULL(dev->clk_mipimpll.clk_o))
		dev_info(&pdev->dev, "clk_mipimpll clk installed %p\n", dev->clk_mipimpll.clk_o);
	dev->clk_disppll.clk_o = devm_clk_get(&pdev->dev, "clk_disppll");
	if (!IS_ERR_OR_NULL(dev->clk_disppll.clk_o))
		dev_info(&pdev->dev, "clk_disppll clk installed %p\n", dev->clk_disppll.clk_o);
	dev->clk_fpll.clk_o = devm_clk_get(&pdev->dev, "clk_fpll");
	if (!IS_ERR_OR_NULL(dev->clk_fpll.clk_o))
		dev_info(&pdev->dev, "clk_fpll clk installed %p\n", dev->clk_fpll.clk_o);
#endif
	for (i = 0; i < (MAX_LINK_NUM * 2 - 1); ++i) {
		res = platform_get_resource(pdev, IORESOURCE_MEM, i);
		if (!res) {
			// dev_err(&pdev->dev, "get memsource [%d] failed\n", i);
			break;
		}
#if (KERNEL_VERSION(5, 10, 0) <= LINUX_VERSION_CODE)
		reg_base[i] = devm_ioremap(&pdev->dev, res->start, res->end - res->start);
#else
		reg_base[i] = devm_ioremap_nocache(&pdev->dev, res->start, res->end - res->start);
#endif

		dev_info(&pdev->dev,
			 "(%d) res-reg: start: 0x%llx, end: 0x%llx.",
			 i, res->start, res->end);
		dev_info(&pdev->dev, " virt-addr(%p)\n", reg_base[i]);
	}
	if (i > 1)
		cif_set_base_addr(0, reg_base[0], reg_base[1]);
	if (i > 2)
		cif_set_base_addr(1, reg_base[2], reg_base[1]);
	if (i > 3)
		cif_set_base_addr(2, reg_base[3], reg_base[1]);
	if (i > 4)
		cif_set_base_addr(3, reg_base[4], reg_base[1]);
	if (i > 5)
		cif_set_base_addr(4, reg_base[5], reg_base[1]);
	if (i > 6)
		cif_set_base_addr(5, reg_base[6], reg_base[1]);
	if (i > 7)
		cif_set_base_addr(6, reg_base[7], reg_base[1]);
	if (i > 8)
		cif_set_base_addr(7, reg_base[8], reg_base[1]);
	if (i > 9)
		cif_set_base_addr(8, reg_base[9], reg_base[1]);
	/* init pad_ctrl. */
	res = platform_get_resource(pdev, IORESOURCE_MEM, i);
	if (!res) {
		dev_info(&pdev->dev, "no pad_ctrl for cif\n");
	} else {
#if (KERNEL_VERSION(5, 10, 0) <= LINUX_VERSION_CODE)
		dev->pad_ctrl = devm_ioremap(&pdev->dev, res->start, res->end - res->start);
#else
		dev->pad_ctrl = devm_ioremap_nocache(&pdev->dev, res->start, res->end - res->start);
#endif
		dev_info(&pdev->dev,
			 "pad-ctrl res-reg: start: 0x%llx, end: 0x%llx.",
			 res->start, res->end);
	}

	/* Init max mac clock. */
	if (max_mac_clk <= 150)
		dev->max_mac_clk = 150;
	else if (max_mac_clk <= 300)
		dev->max_mac_clk = 300;
	else if (max_mac_clk <= 600)
		dev->max_mac_clk = 600;
	else
		dev->max_mac_clk = 900;

	/* Interrupt */
	for (i = 0; i < CIF_MAX_CSI_NUM; ++i) {
		link = &dev->link[i];

		link->irq_num = platform_get_irq(pdev, i);
		if (link->irq_num < 0)
			break;
		if (devm_request_irq(&pdev->dev, link->irq_num, cif_isr, IRQF_SHARED, irq_name[i], link))
			break;
		dev_info(&pdev->dev, "request irq-%d as %s\n",
			 link->irq_num, irq_name[i]);

		/* set the port id */
		link->cif_ctx.mac_num = i;
	}

	/* reset pin */
	for (i = 0; i < MAX_LINK_NUM; ++i) {
		link = &dev->link[i];
		link->dev = &pdev->dev;
		link->mac_clk = RX_MAC_CLK_400M;
		// link->snsr_rst_pin = of_get_named_gpio_flags(pdev->dev.of_node,
		// 		"snsr-reset", i, &link->snsr_rst_pol);
		// if (link->snsr_rst_pin < 0)
		// 	break;

		// if (gpio_request(link->snsr_rst_pin, "snsr-rst-gpio"))
		// 	return 0;

		// dev_info(&pdev->dev, "rst_pin = %d, pol = %d\n",
		// 	link->snsr_rst_pin, link->snsr_rst_pol);
	}
#ifndef FPGA_PORTING
	/* sw reset */
	link = &dev->link[0];
	link->phy_reset = devm_reset_control_get(&pdev->dev, "cam0");
	if (link->phy_reset)
		dev_info(&pdev->dev, "cam0 sw reset installed\n");
	link = &dev->link[1];
	link->phy_reset = devm_reset_control_get(&pdev->dev, "cam1");
	if (link->phy_reset)
		dev_info(&pdev->dev, "cam1 sw reset installed\n");
	link = &dev->link[2];
	link->phy_reset = devm_reset_control_get(&pdev->dev, "cam2");
	if (link->phy_reset)
		dev_info(&pdev->dev, "cam2 sw reset installed\n");

#endif

#endif

	return 0;
}

#ifdef CONFIG_PROC_FS
static void cif_show_dev_attr(struct seq_file *m,
			      struct combo_dev_attr_s *attr)
{
	int i;
	char buf[32] = {0};
	char buf2[32] = {0};
	char *ptr;
	struct mipi_dev_attr_s *mipi;

	seq_printf(m, "%8s%10s%10s%10s%10s%12s%16s\n",
		"Devno", "WorkMode", "DataType", "WDRMode",
		"SyncMode", "DataEndian", "SyncCodeEndian");

	seq_printf(m, "%8d%10s", attr->devno,
		_to_string_input_mode(attr->input_mode));

	switch (attr->input_mode) {
	case INPUT_MODE_MIPI: {
		mipi = &attr->mipi_attr;
		ptr = buf;

		seq_printf(m, "%10s%10s%10s%12s%16s\n",
			_to_string_raw_data_type(mipi->raw_data_type),
			_to_string_mipi_wdr_mode(mipi->wdr_mode),
			"N/A", "N/A", "N/A");

		seq_printf(m, "%17s%28s\n", "LinkId", "PN Swap");
		for (i = 0; i < CIF_LANE_NUM; i++) {
			sprintf(ptr, "%2d,", mipi->lane_id[i]);
			ptr += 3;
		}
		*(ptr - 1) = '\0';
		ptr = buf2;
		for (i = 0; i < CIF_LANE_NUM; i++) {
			sprintf(ptr, "%2d,", mipi->pn_swap[i]);
			ptr += 3;
		}
		*(ptr - 1) = '\0';
		seq_printf(m, "%15s%30s\n", buf, buf2);

	}
	break;
	case INPUT_MODE_SUBLVDS:
	case INPUT_MODE_HISPI: {
		struct lvds_dev_attr_s *lvds = &attr->lvds_attr;
		char *ptr = buf;

		seq_printf(m, "%10s%10s%10s%12s%16s\n",
			_to_string_raw_data_type(lvds->raw_data_type),
			_to_string_wdr_mode(lvds->wdr_mode),
			_to_string_lvds_sync_mode(lvds->sync_mode),
			_to_string_bit_endian(lvds->data_endian),
			_to_string_bit_endian(lvds->sync_code_endian));

		seq_printf(m, "%17s%28s\n", "LinkId", "PN Swap");
		for (i = 0; i < CIF_LANE_NUM; i++) {
			sprintf(ptr, "%2d,", lvds->lane_id[i]);
			ptr += 3;
		}
		*(ptr - 1) = '\0';
		ptr = buf2;
		for (i = 0; i < CIF_LANE_NUM; i++) {
			sprintf(ptr, "%2d,", lvds->pn_swap[i]);
			ptr += 3;
		}
		*(ptr - 1) = '\0';
		seq_printf(m, "%15s%30s\n", buf, buf2);
	}
	break;
	case INPUT_MODE_CMOS:
	case INPUT_MODE_BT1120:
		seq_printf(m, "%10s%10s%15s%10s%12s%16s\n",
			"N/A", "N/A", "N/A", "N/A", "N/A", "N/A");
		break;
	default:
		break;
	}
}

static void cif_show_mipi_sts(struct seq_file *m,
			      struct link *link)
{
	struct csi_status *sts = &link->sts_csi;

	seq_printf(m, "%6s%7s%7s%7s%6s%9s%9s\n",
		   "Devno", "EccErr", "CrcErr", "HdrErr", "WcErr", "fifofull", "decode");
	seq_printf(m, "%6d%7d%7d%7d%6d%9d%9s\n",
		   link->attr.devno,
		   sts->errcnt_ecc, sts->errcnt_crc, sts->errcnt_hdr,
		   sts->errcnt_wc, sts->fifo_full,
		   _to_string_csi_decode(cif_get_csi_decode_fmt(&link->cif_ctx)));
}

static void cif_show_phy_sts(struct seq_file *m,
			      struct link *link)
{
	union mipi_phy_state state;

	cif_get_csi_phy_state(&link->cif_ctx, &state);

	seq_printf(m, "%11s%9s%9s%9s%9s%9s%9s%9s%9s%9s\n",
		   "Physical:", "D0", "D1", "D2", "D3", "D4", "D5", "D6", "D7", "D8");
	seq_printf(m, "%11s%9x%9x%9x%9x%9x%9x%9x%9x%9x\n",
		   " ",
		   cif_get_lane_data(&link->cif_ctx, CIF_PHY_LANE_0),
		   cif_get_lane_data(&link->cif_ctx, CIF_PHY_LANE_1),
		   cif_get_lane_data(&link->cif_ctx, CIF_PHY_LANE_2),
		   cif_get_lane_data(&link->cif_ctx, CIF_PHY_LANE_3),
		   cif_get_lane_data(&link->cif_ctx, CIF_PHY_LANE_4),
		   cif_get_lane_data(&link->cif_ctx, CIF_PHY_LANE_5),
		   cif_get_lane_data(&link->cif_ctx, CIF_PHY_LANE_6),
		   cif_get_lane_data(&link->cif_ctx, CIF_PHY_LANE_7),
		   cif_get_lane_data(&link->cif_ctx, CIF_PHY_LANE_8));

	seq_printf(m, "%11s%9s%9s%9s%9s%9s%9s%9s%9s%9s\n",
		   "Physical:", "D9", "D10", "D11", "D12", "D13", "D14", "D15", "D16", "D17");
	seq_printf(m, "%11s%9x%9x%9x%9x%9x%9x%9x%9x%9x\n",
		   " ",
		   cif_get_lane_data(&link->cif_ctx, CIF_PHY_LANE_9),
		   cif_get_lane_data(&link->cif_ctx, CIF_PHY_LANE_10),
		   cif_get_lane_data(&link->cif_ctx, CIF_PHY_LANE_11),
		   cif_get_lane_data(&link->cif_ctx, CIF_PHY_LANE_12),
		   cif_get_lane_data(&link->cif_ctx, CIF_PHY_LANE_13),
		   cif_get_lane_data(&link->cif_ctx, CIF_PHY_LANE_14),
		   cif_get_lane_data(&link->cif_ctx, CIF_PHY_LANE_15),
		   cif_get_lane_data(&link->cif_ctx, CIF_PHY_LANE_16),
		   cif_get_lane_data(&link->cif_ctx, CIF_PHY_LANE_17));

	switch (link->cif_ctx.mac_num)
	{
		case CIF_MAC_0:
		{
			seq_printf(m, "%11s%9s%9s%9s%9s%9s%9s%9s%9s\n",
				"Digital:", "D0", "D1", "D2", "D3", "D4", "D5", "D6", "D7");
			seq_printf(m, "%11s%9s%9s%9s%9s%9s%9s%9s%9s\n",
				" ",
				_to_string_dlane_state(state.bits_datahs.d0_datahs_state),
				_to_string_dlane_state(state.bits_datahs.d1_datahs_state),
				_to_string_dlane_state(state.bits_datahs.d2_datahs_state),
				_to_string_dlane_state(state.bits_datahs.d3_datahs_state),
				_to_string_dlane_state(state.bits_datahs.d4_datahs_state),
				_to_string_dlane_state(state.bits_datahs.d5_datahs_state),
				_to_string_dlane_state(state.bits_datahs.d6_datahs_state),
				_to_string_dlane_state(state.bits_datahs.d7_datahs_state));
			seq_printf(m, "%11s%9s%9s%9s%9s%9s\n",
				"Digital:", "CK_HS", "CK_ULPS", "CK_STOP", "CK_ERR", "Deskew");
			seq_printf(m, "%11s%9d%9d%9d%9d%9s\n",
				" ",
				state.bits_ckhs.clk_hs_state,
				state.bits_ckhs.clk_ulps_state,
				state.bits_ckhs.clk_stop_state,
				state.bits_ckhs.clk_err_state,
				_to_string_deskew_state(state.bits_ckhs.deskew_state));
			break;
		}
		case CIF_MAC_3:
		case CIF_MAC_4:
		{
			seq_printf(m, "%11s%9s%9s%9s%9s%9s%9s%9s%9s%9s\n",
				"Digital:", "D0", "D1", "D2", "D3", "CK_HS", "CK_ULPS", "CK_STOP", "CK_ERR", "Deskew");
			seq_printf(m, "%11s%9s%9s%9s%9s%9d%9d%9d%9d%9s\n",
				" ",
				_to_string_dlane_state(state.bits_datahs.d0_datahs_state),
				_to_string_dlane_state(state.bits_datahs.d1_datahs_state),
				_to_string_dlane_state(state.bits_datahs.d2_datahs_state),
				_to_string_dlane_state(state.bits_datahs.d3_datahs_state),
				state.bits_ckhs.clk_hs_state,
				state.bits_ckhs.clk_ulps_state,
				state.bits_ckhs.clk_stop_state,
				state.bits_ckhs.clk_err_state,
				_to_string_deskew_state(state.bits_ckhs.deskew_state));
			break;
		}
		case CIF_MAC_1:
		case CIF_MAC_2:
		case CIF_MAC_5:
		{
			seq_printf(m, "%11s%9s%9s%9s%9s%9s%9s%9s\n",
				"Digital:", "D0", "D1", "CK_HS", "CK_ULPS", "CK_STOP", "CK_ERR", "Deskew");
			seq_printf(m, "%11s%9s%9s%9d%9d%9d%9d%9s\n",
				" ",
				_to_string_dlane_state(state.bits_datahs.d0_datahs_state),
				_to_string_dlane_state(state.bits_datahs.d1_datahs_state),
				state.bits_ckhs.clk_hs_state,
				state.bits_ckhs.clk_ulps_state,
				state.bits_ckhs.clk_stop_state,
				state.bits_ckhs.clk_err_state,
				_to_string_deskew_state(state.bits_ckhs.deskew_state));
			break;
		}
		default:
			break;
	}
}

static int proc_cif_show(struct seq_file *m, void *v)
{
	struct cif_dev *dev = (struct cif_dev *)m->private;
	int i;

	seq_printf(m, "\nModule: [MIPI_RX], Build Time[%s]\n",
			UTS_VERSION);
	seq_puts(m, "\n------------Combo DEV ATTR--------------\n");
	for (i = 0; i < MAX_LINK_NUM; i++)
		if (dev->link[i].is_on)
			cif_show_dev_attr(m, &dev->link[i].attr);

	// seq_puts(m, "\n------------MIPI info-------------------\n");
	// for (i = 0; i < MAX_LINK_NUM; i++)
	// 	if (dev->link[i].is_on
	// 		&& (dev->link[i].attr.input_mode == INPUT_MODE_MIPI)) {
	// 		cif_show_mipi_sts(m, &dev->link[i]);
	// 		cif_show_phy_sts(m, &dev->link[i]);
	// 	}
	seq_puts(m, "\n------------MIPI info-------------------\n");
	for (i = 0; i < MAX_LINK_NUM; i++)
		if (dev->link[i].is_on) {
			cif_show_mipi_sts(m, &dev->link[i]);
			cif_show_phy_sts(m, &dev->link[i]);
		}
	return 0;
}

static u8 *dbg_type[] = {
	"reset",
	"hs_s",
	"snsr_r",
	"snsr_on",
	"bt_fmt",
	"mac_clk"
};

static void dbg_print_usage(struct device *dev)
{
	dev_info(dev, "\n------------DBG USAGE-------------------\n");
	dev_info(dev, "reset [devno]: reset mipi-rx error count\n");
	dev_info(dev, "hs_s [devno] [value]: set mipi-rx hs_settle, value : 0~255, def: 16\n");
	dev_info(dev, "snsr_r [devno] [reset]: reset : 1 - reset, 0 - unreset sensor gpio\n");
	dev_info(dev, "snsr_on [cam_num] [on] [mclk]: enable/disable sensor clk\n");
	dev_info(dev, "                               cam_num : 0 - cam0, 1 - cam1\n");
	dev_info(dev, "                               on : 0 - off, 1 - on\n");
	dev_info(dev, "                               mclk : 0 - off, 1 - 37.125M, 2 - 25M, 3 - 27M\n");
	dev_info(dev, "                                    : 4 - 24M, 5 - 26M\n");
	dev_info(dev, "bt_fmt [devno] [0~3]: set bt format CbY/CrY/YCb/YCr\n");
	dev_info(dev, "mac_clk [devno] [200/400/600]: set mac clock (MHz)\n");
}

static int dbg_hdler(struct cif_dev *dev, char const *input)
{
	struct link *link = &dev->link[0];
	struct cif_ctx *ctx;
	//int reset;
	u32 num;
	u8 str[80] = {0};
	u8 t = 0;
	u32 a, v, v2;
	u8 i, n;
	u8 *p;

	num = sscanf(input, "%s %d %d %d", str, &a, &v, &v2);
	if (num > 4) {
		dbg_print_usage(link->dev);
		return -EINVAL;
	}

	dev_info(link->dev, "input = %s %d\n", str, num);
	/* convert to lower case for following type compare */
	p = str;
	for (; *p; ++p)
		*p = tolower(*p);
	n = ARRAY_SIZE(dbg_type);
	for (i = 0; i < n; i++) {
		if (!strcmp(str, dbg_type[i])) {
			t = i;
			break;
		}
	}
	if (i == n) {
		dev_info(link->dev, "unknown type(%s)!\n", str);
		dbg_print_usage(link->dev);
		return -EINVAL;
	}

	switch (t) {
	case 0:
		/* reset */
		if (a > MAX_LINK_NUM)
			return -EINVAL;

		link = &dev->link[a];
		ctx = &link->cif_ctx;

		if (link->is_on) {
			link->sts_csi.errcnt_ecc = 0;
			link->sts_csi.errcnt_crc = 0;
			link->sts_csi.errcnt_wc = 0;
			link->sts_csi.errcnt_hdr = 0;
			link->sts_csi.fifo_full = 0;
			cif_clear_csi_int_sts(ctx);
			cif_unmask_csi_int_sts(ctx, 0x0F);
		}

		break;
	case 1:
		/* hs-settle */
		if (a > MAX_LINK_NUM)
			return -EINVAL;

		link = &dev->link[a];
		ctx = &link->cif_ctx;

		cif_set_hs_settle(ctx, v);
		break;
	case 2:
		/* sensor reset */
		// if (a > MAX_LINK_NUM)
		// 	return -EINVAL;

		// link = &dev->link[a];
		// ctx = &link->cif_ctx;

		// reset = (link->snsr_rst_pol == OF_GPIO_ACTIVE_LOW) ? 0 : 1;

		// if (!gpio_is_valid(link->snsr_rst_pin))
		// 	return 0;
		// if (v)
		// 	gpio_direction_output(link->snsr_rst_pin, reset);
		// else
		// 	gpio_direction_output(link->snsr_rst_pin, !reset);
		break;
	case 3:
		/* sensor enable */
		if (a >= MAX_LINK_NUM)
			return -EINVAL;
		if (!a)
			mclk0 = v2;
		else if (a == 1)
			mclk1 = v2;
		else if (a == 2)
			mclk2 = v2;
		else if (a == 3)
			mclk3 = v2;
		else if (a == 4)
			mclk4 = v2;
		else if (a == 5)
			mclk5 = v2;
		else if (a == 6)
			mclk6 = v2;

		link = &dev->link[a];
		_cif_enable_snsr_clk(link->dev, dev, a, v);
		break;
	case 4:
		/* bt format out */
		if (a > MAX_LINK_NUM)
			return -EINVAL;

		link = &dev->link[a];
		ctx = &link->cif_ctx;

		cif_set_bt_fmt_out(ctx, v);
		break;
	case 5:
		/* set mac clock */
		if (v <= 150)
			_cif_set_mac_clk(dev, a, RX_MAC_CLK_150M);
		else if (v <= 200)
			_cif_set_mac_clk(dev, a, RX_MAC_CLK_200M);
		else if (v <= 300)
			_cif_set_mac_clk(dev, a, RX_MAC_CLK_300M);
		else if (v <= 400)
			_cif_set_mac_clk(dev, a, RX_MAC_CLK_400M);
		else if (v <= 500)
			_cif_set_mac_clk(dev, a, RX_MAC_CLK_500M);
		else if (v <= 600)
			_cif_set_mac_clk(dev, a, RX_MAC_CLK_600M);
		else
			_cif_set_mac_clk(dev, a, RX_MAC_CLK_900M);
		break;
	default:
		dbg_print_usage(link->dev);
		break;
	}

	return 0;
}

static ssize_t cif_proc_write(struct file *file, const char __user *user_buf, size_t count, loff_t *ppos)
{
	struct cif_dev *dev = PDE_DATA(file_inode(file));
#if (KERNEL_VERSION(5, 10, 0) <= LINUX_VERSION_CODE)
	char txt_buff[MAX_CIF_PROC_BUF];

	count = simple_write_to_buffer(txt_buff, MAX_CIF_PROC_BUF, ppos,
					user_buf, count);

	dbg_hdler(dev, txt_buff);
#else

	dbg_hdler(dev, user_buf);
#endif

	return count;
}

static int proc_cif_open(struct inode *inode, struct file *file)
{
	struct cif_dev *dev = PDE_DATA(inode);

	return single_open(file, proc_cif_show, dev);
}

#if (KERNEL_VERSION(5, 10, 0) <= LINUX_VERSION_CODE)
static const struct proc_ops cif_proc_fops = {
	.proc_open		= proc_cif_open,
	.proc_read		= seq_read,
	.proc_write		= cif_proc_write,
	.proc_lseek		= seq_lseek,
	.proc_release	= single_release,
};
#else
static const struct file_operations cif_proc_fops = {
	.owner		= THIS_MODULE,
	.open		= proc_cif_open,
	.read		= seq_read,
	.write		= cif_proc_write,
	.llseek		= seq_lseek,
	.release	= single_release,
};
#endif

static struct proc_dir_entry *cif_proc_entry;
#endif

static int cif_probe(struct platform_device *pdev)
{
	int rc = 0;
	struct cif_dev *dev;
	/* allocate main cif state structure */
	dev = devm_kzalloc(&pdev->dev, sizeof(*dev), GFP_KERNEL);
	if (!dev)
		return -ENOMEM;

	/* initialize locks */
	spin_lock_init(&dev->lock);
	mutex_init(&dev->mutex);

	dev_set_drvdata(&pdev->dev, dev);
	platform_set_drvdata(pdev, dev);

	rc = cif_init_miscdev(pdev, dev);
	if (rc < 0) {
		dev_err(&pdev->dev, "Failed to init miscdev for cif, %d\n", rc);
		return rc;
	}

	rc = _init_resource(pdev);
	if (rc < 0) {
		dev_err(&pdev->dev, "Failed to init res for cif, %d\n", rc);
		return rc;
	}

#ifdef CONFIG_PROC_FS
	cif_proc_entry = proc_create_data(CIF_PROC_NAME, 0, NULL,
					  &cif_proc_fops, dev);
	if (!cif_proc_entry)
		dev_err(&pdev->dev, "cif: can't init procfs.\n");
#endif

	return 0;
}

static int cif_remove(struct platform_device *pdev)
{
	struct cif_dev *dev;

	if (!pdev) {
		dev_err(&pdev->dev, "invalid param");
		return -EINVAL;
	}

	/* rm cif_cb */
	if (cif_rm_cb())
		dev_err(&pdev->dev, "cif: failed to rm cb.\n");

	dev = dev_get_drvdata(&pdev->dev);
	if (!dev) {
		dev_err(&pdev->dev, "Can not get cif drvdata");
		return 0;
	}

	misc_deregister(&dev->miscdev);
	dev_set_drvdata(&pdev->dev, NULL);

#ifdef CONFIG_PROC_FS
	proc_remove(cif_proc_entry);
#endif
	return 0;
}

static const struct of_device_id cif_dt_match[] = {
	{.compatible = "cvitek,cif"},
	{}
};

#if (!DEVICE_FROM_DTS)
static void cif_pdev_release(struct device *dev)
{
}

static struct platform_device cif_pdev = {
	.name		= "cif",
	.dev.release	= cif_pdev_release,
};
#endif

static struct platform_driver cif_pdrv = {
	.probe      = cif_probe,
	.remove     = cif_remove,
	.driver     = {
		.name		= "cif",
		.owner		= THIS_MODULE,
#if (DEVICE_FROM_DTS)
		.of_match_table	= cif_dt_match,
#endif
	},
};

static int __init _cif_init(void)
{
	int rc;

#if (DEVICE_FROM_DTS)
	rc = platform_driver_register(&cif_pdrv);
#else
	rc = platform_device_register(&cif_pdev);
	if (rc)
		return rc;

	rc = platform_driver_register(&cif_pdrv);
	if (rc)
		platform_device_unregister(&cif_pdev);
#endif

	return rc;
}

static void __exit cif_exit(void)
{
	platform_driver_unregister(&cif_pdrv);
#if (!DEVICE_FROM_DTS)
	platform_device_unregister(&cif_pdev);
#endif
}

MODULE_DESCRIPTION("Cvitek Camera Interface Driver");
MODULE_AUTHOR("Saxen Ko");
MODULE_LICENSE("GPL");
module_init(_cif_init);
module_exit(cif_exit);
