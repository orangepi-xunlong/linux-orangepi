#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/of_reserved_mem.h>
#include <linux/of_gpio.h>
#include <linux/platform_device.h>
#include <linux/proc_fs.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/uaccess.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/interrupt.h>
#include <linux/reboot.h>
#include <linux/cdev.h>
#include <linux/compat.h>

#include <linux/comm_mipi_tx.h>
#include "proc/mipi_tx_proc.h"
#include "base_ctx.h"
#include "disp.h"
#include "dsi_phy.h"


static DEFINE_MUTEX(reboot_lock);
struct mipi_tx_dev *reboot_info;
int dump_reg = 1;

/*
 * macro definition
 */
#define MIPI_TX_DEV_NAME "soph-mipi-tx"
#define MIPI_TX_DEV0_NAME "soph-mipi-tx0"
#define MIPI_TX_DEV1_NAME "soph-mipi-tx1"
#define MIPI_TX_PROC_NAME "soph_mipi_tx"

#define MIPI_TX_INFO(fmt, arg...) \
		pr_debug("%d:%s(): " fmt, __LINE__, __func__, ## arg)

#define MIPI_TX_ERR(fmt, arg...) \
		pr_err("%d:%s(): " fmt, __LINE__, __func__, ## arg)

struct mipi_tx_dev {
	struct device *dev;
	struct miscdevice miscdev[DISP_MAX_INST];
	struct combo_dev_cfg_s dev_cfg[DISP_MAX_INST];
	struct clk *clk_mipipll[2];
	struct clk *clk_dsi[4];
	int pid[DISP_MAX_INST];
	bool enable[DISP_MAX_INST];
} mipi_tx_dev_ctx;

static const char *const clk_mipipll_name[] = {
	"clk_vo_mipimpll0", "clk_vo_mipimpll1"
};

static const char *const clk_dsi_name[] = {
	"clk_vo_dsi_mac0", "clk_vo_dsi_tx_esc0",
	"clk_vo_dsi_mac1", "clk_vo_dsi_tx_esc1"
};

//to do: smooth from uboot
// static int smooth;

static int debug = 0;

/* debug: debug
 * bit[0]: dcs cmd mode. 0(hw)/1(sw)
 */
module_param(debug, int, 0644);

/*
 * global variables definition
 */

/*
 * function definition
 */
static int mipi_tx_check_comb_dev_cfg(struct combo_dev_cfg_s *dev_cfg)
{
	if (dev_cfg->output_mode != OUTPUT_MODE_DSI_VIDEO) {
		MIPI_TX_ERR("output_mode(%d) not supported!\n", dev_cfg->output_mode);
		return -EINVAL;
	}

	if (dev_cfg->video_mode != BURST_MODE) {
		MIPI_TX_ERR("video_mode(%d) not supported!\n", dev_cfg->video_mode);
		return -EINVAL;
	}

	return 0;
}

static void _fill_disp_timing(struct disp_timing *timing, struct sync_info_s *sync_info)
{
	timing->vtotal = sync_info->vid_vsa_lines + sync_info->vid_vbp_lines +
			 sync_info->vid_active_lines + sync_info->vid_vfp_lines - 1;
	timing->htotal = sync_info->vid_hsa_pixels + sync_info->vid_hbp_pixels +
			 sync_info->vid_hline_pixels + sync_info->vid_hfp_pixels - 1;
	timing->vsync_start = 0;
	timing->vsync_end = timing->vsync_start + sync_info->vid_vsa_lines - 1;
	timing->vfde_start = timing->vmde_start =
		timing->vsync_start + sync_info->vid_vsa_lines + sync_info->vid_vbp_lines;
	timing->vfde_end = timing->vmde_end =
		timing->vfde_start + sync_info->vid_active_lines - 1;
	timing->hsync_start = 0;
	timing->hsync_end = timing->hsync_start + sync_info->vid_hsa_pixels - 1;
	timing->hfde_start = timing->hmde_start =
		timing->hsync_start + sync_info->vid_hsa_pixels + sync_info->vid_hbp_pixels;
	timing->hfde_end = timing->hmde_end =
		timing->hfde_start + sync_info->vid_hline_pixels - 1;
	timing->vsync_pol = sync_info->vid_vsa_pos_polarity;
	timing->hsync_pol = sync_info->vid_hsa_pos_polarity;
}

static void _get_sync_info(struct disp_timing timing, struct sync_info_s *sync_info)
{
	sync_info->vid_hsa_pixels = timing.hsync_end - timing.hsync_start + 1;
	sync_info->vid_hbp_pixels = timing.hfde_start - timing.hsync_start - sync_info->vid_hsa_pixels;
	sync_info->vid_hline_pixels = timing.hfde_end - timing.hfde_start + 1;
	sync_info->vid_hfp_pixels = timing.htotal - sync_info->vid_hsa_pixels -
				    sync_info->vid_hbp_pixels - sync_info->vid_hline_pixels + 1;
	sync_info->vid_vsa_lines = timing.vsync_end - timing.vsync_start + 1;
	sync_info->vid_vbp_lines = timing.vfde_start - timing.vsync_start - sync_info->vid_vsa_lines;
	sync_info->vid_active_lines = timing.vfde_end - timing.vfde_start + 1;
	sync_info->vid_vfp_lines = timing.vtotal - sync_info->vid_vsa_lines -
				   sync_info->vid_vbp_lines - sync_info->vid_active_lines + 1;
	sync_info->vid_vsa_pos_polarity = timing.vsync_pol;
	sync_info->vid_hsa_pos_polarity = timing.hsync_pol;
}

static int mipi_tx_set_combo_dev_cfg(struct mipi_tx_dev *tdev, struct combo_dev_cfg_s *dev_cfg, int devno)
{
	int ret, i;
	bool data_en[LANE_MAX_NUM] = {false, false, false, false, false};

	u8 lane_num = 0;
	struct disp_timing timing;
	enum disp_dsi_fmt dsi_fmt;
	u8 bits;
	bool preamble_on = false;
	u16 fpsx1000;
	u16 fpsx1000_adjusted_table[6] = {24100, 25100, 30200, 48100, 50100, 60200};
	u16 fpsx1000_table[6][2]= {
		{23950, 24050}, {24950, 25050}, {29950, 30050}, {47950, 48050}
		, {49750, 50050}, {59750, 60150}};
	static u16 fpsx1000_record[2] = {0, 0};

	ret = mipi_tx_check_comb_dev_cfg(dev_cfg);
	if (ret < 0) {
		MIPI_TX_ERR("mipi_tx check combo_dev config failed!\n");
		return -EINVAL;
	}

	dphy_dsi_disable_lanes(devno);
	for (i = 0; i < LANE_MAX_NUM; i++) {
		if ((dev_cfg->lane_id[i] < 0) || (dev_cfg->lane_id[i] >= MIPI_TX_LANE_MAX)) {
			dphy_dsi_set_lane(devno, i, MIPI_TX_LANE_MAX, false, true);
			continue;
		}
		dphy_dsi_set_lane(devno, i, dev_cfg->lane_id[i], dev_cfg->lane_pn_swap[i], true);
		if (dev_cfg->lane_id[i] != MIPI_TX_LANE_CLK) {
			++lane_num;
			data_en[dev_cfg->lane_id[i] - 1] = true;
		}
	}
	if (lane_num == 0) {
		MIPI_TX_ERR("no active mipi-dsi lane\n");
		return -EINVAL;
	}

	disp_set_intf(devno, VO_DISP_INTF_DSI);
	_fill_disp_timing(&timing, &dev_cfg->sync_info);

	switch (dev_cfg->output_format) {
	case OUT_FORMAT_RGB_16_BIT:
		bits = 16;
		dsi_fmt = DISP_DSI_FMT_RGB565;
	break;

	case OUT_FORMAT_RGB_18_BIT:
		bits = 18;
		dsi_fmt = DISP_DSI_FMT_RGB666;
	break;

	case OUT_FORMAT_RGB_24_BIT:
		bits = 24;
		dsi_fmt = DISP_DSI_FMT_RGB888;
	break;

	case OUT_FORMAT_RGB_30_BIT:
		bits = 30;
		dsi_fmt = DISP_DSI_FMT_RGB101010;
	break;

	default:
	return -EINVAL;
	}

	fpsx1000 = (dev_cfg->pixel_clk * 1000000L) / (timing.vtotal + 1) / (timing.htotal + 1);
	for (i = 0; i < 6; ++i) {
		if ((fpsx1000 > fpsx1000_table[i][0]) && (fpsx1000 < fpsx1000_table[i][1]))
			break;
	}
	if (i != 6) {
		fpsx1000 = fpsx1000_adjusted_table[i];
	}
	if (((fpsx1000 / 100) == (fpsx1000_record[!devno] / 100))
		|| ((fpsx1000 * 2 / 100) == (fpsx1000_record[!devno] / 100))
		|| ((fpsx1000 / 100) == (fpsx1000_record[!devno] * 2 / 100))) {
		fpsx1000 -= 200;
	}
	dev_cfg->pixel_clk = (u64)fpsx1000 * (timing.vtotal + 1) * (timing.htotal + 1) / 1000000L;

	fpsx1000_record[devno] = fpsx1000;
	preamble_on = (dev_cfg->pixel_clk * bits / lane_num) > 2500000;

	dphy_dsi_lane_en(devno, true, data_en, preamble_on);
	dphy_dsi_set_pll(devno, dev_cfg->pixel_clk, lane_num, bits);
	disp_dsi_config(devno, lane_num, dsi_fmt, dev_cfg->sync_info.vid_hline_pixels);
	disp_set_timing(devno, &timing);
	disp_clk_enable(devno, true);
	disp_tgen_enable(devno, true);

	if (mipi_tx_dev_ctx.clk_mipipll[devno] && (!__clk_is_enabled(mipi_tx_dev_ctx.clk_mipipll[devno])))
		clk_prepare_enable(mipi_tx_dev_ctx.clk_mipipll[devno]);

	if (mipi_tx_dev_ctx.clk_dsi[devno * 2] && (!__clk_is_enabled(mipi_tx_dev_ctx.clk_dsi[devno * 2])))
		clk_prepare_enable(mipi_tx_dev_ctx.clk_dsi[devno * 2]);

	if (mipi_tx_dev_ctx.clk_dsi[devno * 2 + 1] && (!__clk_is_enabled(mipi_tx_dev_ctx.clk_dsi[devno * 2 + 1])))
		clk_prepare_enable(mipi_tx_dev_ctx.clk_dsi[devno * 2 + 1]);

	tdev->enable[devno] = true;

	pr_debug("lane_num(%d) preamble_on(%d) dsi_fmt(%d) bits(%d)\n", lane_num, preamble_on, dsi_fmt, bits);

	return ret;
}

int mipi_tx_get_combo_dev_cfg(struct combo_dev_cfg_s *dev_cfg, int devno)
{
	int ret = 0;
	char data_lan_num = 0;
	struct disp_timing timing;

	if (dev_cfg == NULL) {
		MIPI_TX_ERR("ptr dev_cfg is NULL!\n");
		return -EINVAL;
	}

	data_lan_num = dphy_dsi_get_lane(devno, dev_cfg->lane_id);
	dev_cfg->video_mode = BURST_MODE;
	dev_cfg->output_mode = OUTPUT_MODE_DSI_VIDEO;
	dev_cfg->output_format = OUT_FORMAT_RGB_24_BIT;

	dev_cfg->devno = devno;
	disp_get_hw_timing(devno, &timing);
	_get_sync_info(timing, &dev_cfg->sync_info);
	dphy_dsi_get_pixclk(devno, &dev_cfg->pixel_clk, data_lan_num, 24);

	return ret;
}

static int mipi_tx_set_cmd(struct cmd_info_s *cmd_info, int devno)
{
	int i;
	char str[160];

	if (cmd_info->cmd_size > CMD_MAX_NUM) {
		MIPI_TX_ERR("cmd_size(%d) can't exceed %d!\n", cmd_info->cmd_size, CMD_MAX_NUM);
		return -EINVAL;
	} else if ((cmd_info->cmd_size != 0) && (cmd_info->cmd == NULL)) {
		MIPI_TX_ERR("cmd is NULL, but cmd_size(%d) isn't zero!\n", cmd_info->cmd_size);
		return -EINVAL;
	}

	snprintf(str, 160, "%s: ", __func__);
	for (i = 0; i < cmd_info->cmd_size && i < 16; ++i)
		snprintf(str + strlen(str), 160 - strlen(str), "%#x ", cmd_info->cmd[i]);
	pr_debug("%s\n", str);

	return dsi_dcs_write_buffer(devno, cmd_info->data_type, cmd_info->cmd, cmd_info->cmd_size, debug & 0x01);
}

static int mipi_tx_get_cmd(struct get_cmd_info_s *get_cmd_info, int devno)
{
	if (get_cmd_info->get_data_size > RX_MAX_NUM) {
		MIPI_TX_ERR("get_data_size(%d) can't exceed %d!\n", get_cmd_info->get_data_size, RX_MAX_NUM);
		return -EINVAL;
	} else if ((get_cmd_info->get_data_size != 0) && (get_cmd_info->get_data == NULL)) {
		MIPI_TX_ERR("cmd is NULL, but cmd_size(%d) isn't zero!\n", get_cmd_info->get_data_size);
		return -EINVAL;
	}

	return dsi_dcs_read_buffer(devno, get_cmd_info->data_type, get_cmd_info->data_param
		, get_cmd_info->get_data, get_cmd_info->get_data_size);
}

static void mipi_tx_enable(int devno)
{
	dsi_set_mode(devno, DISP_DSI_MODE_HS);
	usleep_range(1000, 2000);
}

static void mipi_tx_disable(int devno)
{
	int ret = 0;
	int count = 0;

	dsi_set_mode(devno, DISP_DSI_MODE_IDLE);
	do {
		usleep_range(1000, 2000);
		ret = dsi_chk_mode_done(devno, DISP_DSI_MODE_IDLE);
	} while ((ret != 0) && (count++ < 20));
}

static long mipi_tx_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct mipi_tx_dev *tdev = &mipi_tx_dev_ctx;
	int rc = 0;
	int i, devno;

	switch (cmd) {
	case MIPI_TX_SET_DEV_CFG: {
		struct combo_dev_cfg_s stcombo_dev_cfg;
		CHECK_IOCTL_CMD(cmd, struct combo_dev_cfg_s);

		if (arg == 0) {
			MIPI_TX_ERR("NULL pointer.\n");
			rc = -EINVAL;
			break;
		}
		if (copy_from_user(&stcombo_dev_cfg, (void *)arg, sizeof(stcombo_dev_cfg))) {
			MIPI_TX_ERR("copy_from_user failed.\n");
			rc = -ENOMEM;
			break;
		}

		rc = mipi_tx_set_combo_dev_cfg(tdev, &stcombo_dev_cfg, stcombo_dev_cfg.devno);
		if (rc < 0)
			MIPI_TX_ERR("mipi_tx set combo_dev config failed!\n");
		else {
			tdev->dev_cfg[stcombo_dev_cfg.devno] = stcombo_dev_cfg;
			tdev->pid[stcombo_dev_cfg.devno] = current->pid;
		}
	}
	break;

	case MIPI_TX_GET_DEV_CFG: {
		struct combo_dev_cfg_s stcombo_dev_cfg;
		CHECK_IOCTL_CMD(cmd, struct combo_dev_cfg_s);

		if (arg == 0) {
			MIPI_TX_ERR("NULL pointer.\n");
			rc = -EINVAL;
			break;
		}

		for (i = 0; i < DISP_MAX_INST; ++i) {
			if (current->pid == tdev->pid[i]) {
				devno = tdev->dev_cfg[i].devno;
				break;
			}
		}
		if (i >= DISP_MAX_INST) {
			MIPI_TX_ERR("mipi_tx doesn't support mulyi-process!\n");
			rc = -EINVAL;
			break;
		}

		memset(&stcombo_dev_cfg, 0, sizeof(stcombo_dev_cfg));
		rc = mipi_tx_get_combo_dev_cfg(&stcombo_dev_cfg, devno);
		if (rc < 0)
			MIPI_TX_ERR("mipi_tx get combo_dev config failed!\n");

		if (copy_to_user((void *)arg, &stcombo_dev_cfg, sizeof(stcombo_dev_cfg))) {
			MIPI_TX_ERR("copy_to_user failed.\n");
			rc = -ENOMEM;
			break;
		}
	}
	break;

	case MIPI_TX_SET_CMD: {
		struct cmd_info_s cmd_info;
		CHECK_IOCTL_CMD(cmd, struct cmd_info_s);

		if (arg == 0) {
			MIPI_TX_ERR("NULL pointer.\n");
			rc = -EINVAL;
			break;
		}
		if (copy_from_user(&cmd_info, (void *)arg, sizeof(cmd_info))) {
			MIPI_TX_ERR("copy_from_user failed.\n");
			rc = -ENOMEM;
			break;
		}
		if (cmd_info.cmd_size == 0) {
			MIPI_TX_ERR("cmd_size zero.\n");
			rc = -EINVAL;
			break;
		}

		// if cmd is NULL, use cmd_size as cmd.
		if (cmd_info.cmd == NULL) {
			cmd_info.cmd = kmalloc(2, GFP_KERNEL);
			if (cmd_info.cmd == NULL) {
				MIPI_TX_ERR("kmalloc failed.\n");
				rc = -ENOMEM;
				break;
			}
			cmd_info.cmd[0] = cmd_info.cmd_size & 0xff;
			cmd_info.cmd[1] = (cmd_info.cmd_size >> 8) & 0xff;
			cmd_info.cmd_size = (cmd_info.data_type == 0x05) ? 1 : 2;
		} else {
			unsigned char *tmp_cmd = kmalloc(cmd_info.cmd_size, GFP_KERNEL);

			if (tmp_cmd == NULL) {
				MIPI_TX_ERR("kmalloc failed.\n");
				rc = -ENOMEM;
				break;
			}

			if (copy_from_user(tmp_cmd, (void __user *)cmd_info.cmd, cmd_info.cmd_size)) {
				MIPI_TX_ERR("cmd copy_from_user failed.\n");
				rc = -ENOMEM;
				break;
			}
			cmd_info.cmd = tmp_cmd;
		}

		rc = mipi_tx_set_cmd(&cmd_info, cmd_info.devno);
		if (rc < 0)
			MIPI_TX_ERR("mipi_tx set cmd failed!\n");
		kfree(cmd_info.cmd);
	}
	break;

	case MIPI_TX_GET_CMD: {
		struct get_cmd_info_s get_cmd_info;
		CHECK_IOCTL_CMD(cmd, struct get_cmd_info_s);

		if (arg == 0) {
			MIPI_TX_ERR("NULL pointer.\n");
			rc = -EINVAL;
			break;
		}
		if (copy_from_user(&get_cmd_info, (void *)arg, sizeof(get_cmd_info))) {
			MIPI_TX_ERR("copy_from_user failed.\n");
			rc = -ENOMEM;
			break;
		}
		for (i = 0; i < DISP_MAX_INST; ++i) {
			if (current->pid == tdev->pid[i]) {
				devno = tdev->dev_cfg[i].devno;
				break;
			}
		}
		if (i >= DISP_MAX_INST) {
			MIPI_TX_ERR("mipi_tx doesn't support mulyi-process!\n");
			rc = -EINVAL;
			break;
		}
		rc = mipi_tx_get_cmd(&get_cmd_info, devno);
		if (rc < 0) {
			MIPI_TX_ERR("mipi_tx get cmd failed!\n");
			break;
		}
		if (copy_to_user((void *)arg, &get_cmd_info, sizeof(get_cmd_info))) {
			MIPI_TX_ERR("copy_to_user failed.\n");
			rc = -ENOMEM;
			break;
		}
	}
	break;

	case MIPI_TX_ENABLE: {
		for (i = 0; i < DISP_MAX_INST; ++i) {
			if (current->pid == tdev->pid[i]) {
				devno = tdev->dev_cfg[i].devno;
				break;
			}
		}
		MIPI_TX_INFO("MIPI_TX_ENABLE devno(%d)\n", devno);
		if (i >= DISP_MAX_INST) {
			MIPI_TX_ERR("mipi_tx doesn't support mulyi-process!\n");
			rc = -EINVAL;
			break;
		}

		mipi_tx_enable(devno);
	}
	break;

	case MIPI_TX_DISABLE: {
		for (i = 0; i < DISP_MAX_INST; ++i) {
			if (current->pid == tdev->pid[i]) {
				devno = tdev->dev_cfg[i].devno;
				break;
			}
		}
		if (i >= DISP_MAX_INST) {
			MIPI_TX_ERR("mipi_tx doesn't support mulyi-process!\n");
			rc = -EINVAL;
			break;
		}

		mipi_tx_disable(devno);
	}
	break;

	case MIPI_TX_SET_HS_SETTLE: {
		struct hs_settle_s settle_cfg;
		CHECK_IOCTL_CMD(cmd, struct hs_settle_s);

		if (arg == 0) {
			MIPI_TX_ERR("NULL pointer.\n");
			rc = -EINVAL;
			break;
		}
		if (copy_from_user(&settle_cfg, (void *)arg, sizeof(settle_cfg))) {
			MIPI_TX_ERR("copy_from_user failed.\n");
			rc = -ENOMEM;
			break;
		}
		for (i = 0; i < DISP_MAX_INST; ++i) {
			if (current->pid == tdev->pid[i]) {
				devno = tdev->dev_cfg[i].devno;
				break;
			}
		}
		if (i >= DISP_MAX_INST) {
			MIPI_TX_ERR("mipi_tx doesn't support mulyi-process!\n");
			rc = -EINVAL;
			break;
		}

		MIPI_TX_INFO("Set hs settle: prepare(%d) zero(%d) trail(%d)\n",
				     settle_cfg.prepare, settle_cfg.zero, settle_cfg.trail);
		dphy_set_hs_settle(devno, settle_cfg.prepare, settle_cfg.zero, settle_cfg.trail);
	}
	break;

	case MIPI_TX_GET_HS_SETTLE: {
		struct hs_settle_s settle_cfg;
		CHECK_IOCTL_CMD(cmd, struct hs_settle_s);

		if (arg == 0) {
			MIPI_TX_ERR("NULL pointer.\n");
			rc = -EINVAL;
			break;
		}
		for (i = 0; i < DISP_MAX_INST; ++i) {
			if (current->pid == tdev->pid[i]) {
				devno = tdev->dev_cfg[i].devno;
				break;
			}
		}
		if (i >= DISP_MAX_INST) {
			MIPI_TX_ERR("mipi_tx doesn't support mulyi-process!\n");
			rc = -EINVAL;
			break;
		}
		dphy_get_hs_settle(devno, &settle_cfg.prepare, &settle_cfg.zero, &settle_cfg.trail);
		MIPI_TX_INFO("Get hs settle: prepare(%d) zero(%d) trail(%d)\n",
				     settle_cfg.prepare, settle_cfg.zero, settle_cfg.trail);
		if (copy_to_user((void *)arg, &settle_cfg, sizeof(settle_cfg))) {
			MIPI_TX_ERR("copy_to_user failed.\n");
			rc = -ENOMEM;
			break;
		}
	}
	break;

	case MIPI_TX_SUSPEND: {
		//display off
		u8 cmd = 0x28;
		int i = 0;

		for (i = DISP_MAX_INST - 1; i >= 0; --i) {
			mipi_tx_disable(i);
			dsi_dcs_write_buffer(i, 0x05, &cmd, 1, debug & 0x01);
			mipi_tx_enable(i);

			if (mipi_tx_dev_ctx.clk_mipipll[i] && __clk_is_enabled(mipi_tx_dev_ctx.clk_mipipll[i]))
				clk_disable_unprepare(mipi_tx_dev_ctx.clk_mipipll[i]);

			if (mipi_tx_dev_ctx.clk_dsi[i * 2] && __clk_is_enabled(mipi_tx_dev_ctx.clk_dsi[i * 2]))
				clk_disable_unprepare(mipi_tx_dev_ctx.clk_dsi[i]);

			if (mipi_tx_dev_ctx.clk_dsi[i * 2 + 1] && __clk_is_enabled(mipi_tx_dev_ctx.clk_dsi[i * 2 + 1]))
				clk_disable_unprepare(mipi_tx_dev_ctx.clk_dsi[i]);
		}
	}
	break;

	case MIPI_TX_RESUME: {
		//display on
		u8 cmd = 0x29;
		int i = 0;

		for (i = 0; i < DISP_MAX_INST; ++i) {
			if(tdev->enable[devno]) {
				if (mipi_tx_dev_ctx.clk_mipipll[i] &&
				    (!__clk_is_enabled(mipi_tx_dev_ctx.clk_mipipll[i])))
					clk_prepare_enable(mipi_tx_dev_ctx.clk_mipipll[i]);

				if (mipi_tx_dev_ctx.clk_dsi[i * 2] &&
				    (!__clk_is_enabled(mipi_tx_dev_ctx.clk_dsi[i * 2])))
					clk_prepare_enable(mipi_tx_dev_ctx.clk_dsi[i * 2]);

				if (mipi_tx_dev_ctx.clk_dsi[i * 2 + 1] &&
				    (!__clk_is_enabled(mipi_tx_dev_ctx.clk_dsi[i * 2 + 1])))
					clk_prepare_enable(mipi_tx_dev_ctx.clk_dsi[i * 2 + 1]);

				mipi_tx_disable(i);
				dsi_dcs_write_buffer(i, 0x05, &cmd, 1, debug & 0x01);
				mipi_tx_enable(i);
			}
		}
	}
	break;

	default: {
		MIPI_TX_ERR("invalid mipi_tx ioctl cmd\n");
		rc = -EINVAL;
	}
	break;
	}

	return rc;
}

#ifdef CONFIG_COMPAT
static long mipi_tx_compat_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	if (!file->f_op->unlocked_ioctl)
		return -ENOIOCTLCMD;

	return file->f_op->unlocked_ioctl(file, cmd, (unsigned long)compat_ptr(arg));
}
#endif

static int mipi_tx_open(struct inode *inode, struct file *file)
{
	struct mipi_tx_dev *tdev = &mipi_tx_dev_ctx;

	if (strcmp(file->f_path.dentry->d_iname, MIPI_TX_DEV0_NAME) == 0) {
		tdev->pid[0] = current->pid;
		tdev->dev_cfg[0].devno = 0;
	} else if (strcmp(file->f_path.dentry->d_iname, MIPI_TX_DEV1_NAME) == 0) {
		tdev->pid[1] = current->pid;
		tdev->dev_cfg[1].devno = 1;
	}

	return 0;
}

static int mipi_tx_release(struct inode *inode, struct file *file)
{
	return 0;
}

static const struct file_operations mipi_tx_fops = {
	.owner = THIS_MODULE,
	.open = mipi_tx_open,
	.release = mipi_tx_release,
	.unlocked_ioctl = mipi_tx_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = mipi_tx_compat_ioctl,
#endif
};

static int _device_register(struct mipi_tx_dev *tdev)
{
	int rc, devno;

	for (devno = 0; devno < DISP_MAX_INST; ++devno) {
		tdev->miscdev[devno].minor = MISC_DYNAMIC_MINOR;
		tdev->miscdev[devno].fops = &mipi_tx_fops;

		if (devno == 0) {
			tdev->miscdev[devno].name = MIPI_TX_DEV0_NAME;
		} else if (devno == 1) {
			tdev->miscdev[devno].name = MIPI_TX_DEV1_NAME;
		}
		dev_info(tdev->dev, "mipi_tx: misc_register name:%s.\n", tdev->miscdev[devno].name);
		rc = misc_register(&tdev->miscdev[devno]);
		if (rc) {
			dev_err(tdev->dev, "mipi_tx: failed to register misc device.\n");
			return rc;
		}
	}


	return rc;
}

static int _init_resources(struct platform_device *pdev)
{
	int rc = 0, i;
	struct mipi_tx_dev *tdev;

	tdev = dev_get_drvdata(&pdev->dev);
	if (!tdev) {
		dev_err(&pdev->dev, "Can not get mipi_tx drvdata");
		return -ENODEV;
	}

	for (i = 0; i < ARRAY_SIZE(clk_mipipll_name); ++i) {
		tdev->clk_mipipll[i] = devm_clk_get(&pdev->dev, clk_mipipll_name[i]);
		if (IS_ERR(tdev->clk_mipipll[i])) {
			dev_err(&pdev->dev, "Cannot get clk for %s\n", clk_mipipll_name[i]);
			return PTR_ERR(tdev->clk_mipipll[i]);
		}
		clk_prepare_enable(tdev->clk_mipipll[i]);
		clk_disable_unprepare(tdev->clk_mipipll[i]);
	}

	for (i = 0; i < ARRAY_SIZE(clk_dsi_name); ++i) {
		tdev->clk_dsi[i] = devm_clk_get(&pdev->dev, clk_dsi_name[i]);
		if (IS_ERR(tdev->clk_dsi[i])) {
			dev_err(&pdev->dev, "Cannot get clk for %s\n", clk_dsi_name[i]);
			return PTR_ERR(tdev->clk_dsi[i]);
		}
		clk_prepare_enable(tdev->clk_dsi[i]);
		clk_disable_unprepare(tdev->clk_dsi[i]);
	}

	return rc;
}

static void _power_off(struct mipi_tx_dev *tdev)
{
	u8 cmd = 0x28;
	int i = 0;

	// send mipi display off
	for (i = 0; i < DISP_MAX_INST; ++i) {
		if (tdev->pid[i] != -1) {
			mipi_tx_disable(tdev->dev_cfg[i].devno);
			dsi_dcs_write_buffer(tdev->dev_cfg[i].devno, 0x05, &cmd, 1, debug & 0x01);
		}
	}
}

static int _reboot_notify(struct notifier_block *nb,
			       unsigned long code, void *unused)
{
	if (code != SYS_RESTART)
		return NOTIFY_DONE;

	mutex_lock(&reboot_lock);

	if (!reboot_info)
		goto out;

	dev_info(reboot_info->dev, "%s\n", __func__);
	_power_off(reboot_info);

out:
	mutex_unlock(&reboot_lock);

	return NOTIFY_DONE;
}

static struct notifier_block _reboot_notifier = {
	.notifier_call = _reboot_notify,
};

static int mipi_tx_probe(struct platform_device *pdev)
{
	int rc = 0, devno;

	dev_set_drvdata(&pdev->dev, &mipi_tx_dev_ctx);
	platform_set_drvdata(pdev,  &mipi_tx_dev_ctx);
	mipi_tx_dev_ctx.dev = &pdev->dev;

	mipi_tx_dev_ctx.pid[0] = -1;
	mipi_tx_dev_ctx.pid[1] = -1;
	_init_resources(pdev);

	for (devno = 0; devno < DISP_MAX_INST; ++devno) {
		rc = mipi_tx_proc_init(devno);
		if (rc) {
			dev_err(&pdev->dev, "mipi_tx proc init fail\n");
		}
	}

	rc = _device_register(&mipi_tx_dev_ctx);
	if (rc)
		goto err_res;

	mutex_lock(&reboot_lock);
	if (!reboot_info)
		reboot_info = &mipi_tx_dev_ctx;
	mutex_unlock(&reboot_lock);

	register_reboot_notifier(&_reboot_notifier);

	return rc;

err_res:
	dev_set_drvdata(&pdev->dev, NULL);

	dev_err(&pdev->dev, "failed with rc(%d).\n", rc);
	return rc;
}

static int mipi_tx_remove(struct platform_device *pdev)
{
	int devno, i;

	unregister_reboot_notifier(&_reboot_notifier);
	mutex_lock(&reboot_lock);
	if (reboot_info == &mipi_tx_dev_ctx)
		reboot_info = NULL;
	mutex_unlock(&reboot_lock);
	_power_off(&mipi_tx_dev_ctx);

	for (i = ARRAY_SIZE(clk_mipipll_name) - 1; i >= 0; --i) {
		if (mipi_tx_dev_ctx.clk_mipipll[i] && __clk_is_enabled(mipi_tx_dev_ctx.clk_mipipll[i]))
			clk_disable_unprepare(mipi_tx_dev_ctx.clk_mipipll[i]);
	}

	for (i = 0; i < ARRAY_SIZE(clk_dsi_name); ++i) {
		if (mipi_tx_dev_ctx.clk_dsi[i] && __clk_is_enabled(mipi_tx_dev_ctx.clk_dsi[i]))
			clk_disable_unprepare(mipi_tx_dev_ctx.clk_dsi[i]);
	}

	for (devno = 0; devno < DISP_MAX_INST; ++devno) {
		dev_info(mipi_tx_dev_ctx.dev, "mipi_tx: misc_deregister name:%s.\n",
			 mipi_tx_dev_ctx.miscdev[devno].name);
		misc_deregister(&mipi_tx_dev_ctx.miscdev[devno]);
		mipi_tx_proc_remove(devno);
	}
	dev_set_drvdata(&pdev->dev, NULL);

	return 0;
}

#if defined(CONFIG_PM)
static int mipi_tx_suspend(struct platform_device *pdev, pm_message_t state)
{
	//display off
	u8 cmd = 0x28;
	int i = 0;

	dev_warn(&pdev->dev, "mipi_tx_suspend\n");

	for (i = 0; i < DISP_MAX_INST; ++i) {
		mipi_tx_disable(i);
		dsi_dcs_write_buffer(i, 0x05, &cmd, 1, debug & 0x01);
		mipi_tx_enable(i);
	}

	for (i = ARRAY_SIZE(clk_mipipll_name) - 1; i >= 0; --i) {
		if (mipi_tx_dev_ctx.clk_mipipll[i] && __clk_is_enabled(mipi_tx_dev_ctx.clk_mipipll[i]))
			clk_disable_unprepare(mipi_tx_dev_ctx.clk_mipipll[i]);
	}

	for (i = 0; i < ARRAY_SIZE(clk_dsi_name); ++i) {
		if (mipi_tx_dev_ctx.clk_dsi[i] && __clk_is_enabled(mipi_tx_dev_ctx.clk_dsi[i]))
			clk_disable_unprepare(mipi_tx_dev_ctx.clk_dsi[i]);
	}

	return 0;
}

static int mipi_tx_resume(struct platform_device *pdev)
{
	//display on
	u8 cmd = 0x29;
	int i = 0;

	dev_warn(&pdev->dev, "mipi_tx_resume\n");

	for (i = 0; i < DISP_MAX_INST; ++i) {
		if(mipi_tx_dev_ctx.enable[i]) {
			if (mipi_tx_dev_ctx.clk_mipipll[i] &&
			    (!__clk_is_enabled(mipi_tx_dev_ctx.clk_mipipll[i])))
				clk_prepare_enable(mipi_tx_dev_ctx.clk_mipipll[i]);

			if (mipi_tx_dev_ctx.clk_dsi[i * 2] &&
			    (!__clk_is_enabled(mipi_tx_dev_ctx.clk_dsi[i * 2])))
				clk_prepare_enable(mipi_tx_dev_ctx.clk_dsi[i * 2]);

			if (mipi_tx_dev_ctx.clk_dsi[i * 2 + 1] &&
			    (!__clk_is_enabled(mipi_tx_dev_ctx.clk_dsi[i * 2 + 1])))
				clk_prepare_enable(mipi_tx_dev_ctx.clk_dsi[i * 2 + 1]);

			mipi_tx_disable(i);
			dsi_dcs_write_buffer(i, 0x05, &cmd, 1, debug & 0x01);
			mipi_tx_enable(i);
		}
	}

	return 0;
}
#endif

static const struct of_device_id mipi_tx_dt_match[] = { { .compatible = "cvitek,mipi_tx" }, {} };

static struct platform_driver mipi_tx_pdrv = {
	.probe = mipi_tx_probe,
	.remove = mipi_tx_remove,
	.driver = {
		.name = MIPI_TX_DEV_NAME,
		.owner = THIS_MODULE,
		.of_match_table = mipi_tx_dt_match,
	},
#if defined(CONFIG_PM)
	.suspend = mipi_tx_suspend,
	.resume = mipi_tx_resume,
#endif
};

static int __init mipi_tx_init(void)
{
	return platform_driver_register(&mipi_tx_pdrv);
}

static void __exit mipi_tx_exit(void)
{
	platform_driver_unregister(&mipi_tx_pdrv);
}

MODULE_DESCRIPTION("Cvitek MIPI-TX Driver");
MODULE_AUTHOR("Jammy Huang");
MODULE_LICENSE("GPL");
module_init(mipi_tx_init);
module_exit(mipi_tx_exit);
