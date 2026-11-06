#include <linux/slab.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include "hdmi_debug.h"
#include "edid/edid.h"
#include <linux/comm_hdmi.h>
#include <linux/common.h>
#include "hdmi_proc.h"

#define HDMI_PROC_NAME          "soph/hdmi"
#define HDMI_VIDEO_PROC_NAME    "soph/hdmi_video"
#define HDMI_AUDIO_PROC_NAME    "soph/hdmi_audio"
#define HDMI_SINK_PROC_NAME     "soph/hdmi_sink"
#define MAX_PROC_STR_SIZE (160)

static char* _pix_fmt_to_string(hdmi_video_mode pixfmt, char* str)
{
	switch (pixfmt) {
		case HDMI_VIDEO_MODE_RGB888:
			strcpy(str, "RGB888");
			break;
		case HDMI_VIDEO_MODE_YCBCR444:
			strcpy(str, "YCbCr444");
			break;
		case HDMI_VIDEO_MODE_YCBCR422:
			strcpy(str, "YCbCr422");
			break;
		default:
			strcpy(str, "Unknown_Fmt");
			break;
	}
	return str;
}

static char* _quant_range_to_string(hdmi_rgb_quant_range quant_range, char* str)
{
	switch (quant_range) {
		case HDMI_RGB_QUANT_DEFAULT_RANGE:
			strcpy(str, "DEFAULT");
			break;
		case HDMI_RGB_QUANT_LIMITED_RANGE:
			strcpy(str, "LIMITED");
			break;
		case HDMI_RGB_QUANT_FULL_RANGE:
			strcpy(str, "FULLRANGE");
			break;
		case HDMI_RGB_QUANT_FULL_BUTT:
			strcpy(str, "RESERVED");
			break;
		default:
			strcpy(str, "Unknown");
			break;
	}
	return str;
}

/*************************************************************************
 *	HDMI proc functions
 *************************************************************************/
int hdmi_ctx_proc_show(struct seq_file *m, void *v)
{
	struct hdmi_tx_ctx *ctx = get_hdmi_ctx();

	seq_puts(m, "HDMI version: 2.0\n");
	seq_puts(m, "\n----------------------------- APP Attr ----------------------------------\n");
	seq_printf(m, "%30s%30s\n", "hdmi enable",  "audio enable");
	seq_printf(m, "%30s%30s\n",
					ctx->mode.hdmi_en ? "YES": "NO",
					ctx->mode.paudio.audio_en ? "YES": "NO");
	seq_printf(m, "\n");
	seq_printf(m, "%30s%30s\n", "auth mode enable",  "deep color mode");
	seq_printf(m, "%30s%30d\n","Reserved", ctx->mode.pvideo.mcolor_resolution);
	seq_printf(m, "\n");
	seq_printf(m, "%30s\n", "deep color adapt");
	seq_printf(m, "%30s\n", "NO");
	seq_puts(m, "\n----------------------------- sw status ---------------------------------\n");
#if 0
	thread run: YES run status: OPEN START
	kernel cnt: 0
	user cnt: 1 kernel callback: NO
	user callback cnt: 1 transit state: BOOT->APP
	emi enable: NO pcb len: 0
#endif
	seq_puts(m, "\n----------------------------- HW Status ---------------------------------\n");
	seq_printf(m, "%30s%30s\n", "hot plug",  "RX sense");
	seq_printf(m, "%30s%30s\n",
				ctx->hdmi_tx.snps_hdmi_ctrl.hpd ? "YES": "NO",
				ctx->hdmi_tx.snps_hdmi_ctrl.rx_sense ? "YES": "NO");
	seq_printf(m, "\n");
	seq_printf(m, "%30s%30s\n", "phy output enable", "phy power enable");
	seq_printf(m, "%30s%30s\n",
				ctx->hdmi_tx.snps_hdmi_ctrl.phy_enable ? "YES": "NO",
				ctx->hdmi_tx.snps_hdmi_ctrl.phy_power ? "YES": "NO");
	seq_printf(m, "\n");
	seq_printf(m, "%30s%30s\n", "TMDS mode", "HDMI avmute");
	seq_printf(m, "%30s%30s\n",
				ctx->hdmi_tx.snps_hdmi_ctrl.pixel_clock < 340000 ? "HDMI1.4": "HDMI2.0" ,
				ctx->hdmi_tx.snps_hdmi_ctrl.avmute ? "YES": "NO");
	seq_printf(m, "\n");
	seq_printf(m, "%30s%30s\n", "emi enable", "src_scramble");
	seq_printf(m, "%30s%30s\n",
				"--", ctx->hdmi_tx.snps_hdmi_ctrl.src_scramble ? "YES": "NO");
	seq_printf(m, "\n");
	seq_printf(m, "%30s%30s\n", "sink_scramble", "tmds_bit_clk_ratio");
	seq_printf(m, "%30s%27d/%d\n",
				ctx->hdmi_tx.snps_hdmi_ctrl.sink_scramble ? "YES": "NO",
				1,
				ctx->hdmi_tx.snps_hdmi_ctrl.high_tmds_ratio ? 40: 10);
	seq_puts(m, "\n---------------------------- Detect Timing -----------------------------\n");
	seq_printf(m, "%30s%30s\n", "SYNC sw enable", "progressive");
	seq_printf(m, "%30s%30s\n",
				"YES",
				ctx->mode.pvideo.mdtd.m_interlaced ? "YES":"NO");
	seq_printf(m, "\n");
	seq_printf(m, "%30s%30s\n", "HSYNC polarity", "VSYNC polarity");
	seq_printf(m, "%30s%30s\n",
				ctx->mode.pvideo.mdtd.m_hsync_polarity ? "YES": "NO",
				ctx->mode.pvideo.mdtd.m_vsync_polarity ? "YES": "NO");
	seq_printf(m, "\n");
	seq_printf(m, "%30s%30s\n", "HSYNC total", "VSYNC total");
	seq_printf(m, "%30d%30d\n",
				ctx->mode.pvideo.mdtd.m_hactive + ctx->mode.pvideo.mdtd.m_hblanking,
				ctx->mode.pvideo.mdtd.m_vactive + ctx->mode.pvideo.mdtd.m_vblanking);
	seq_printf(m, "\n");
	seq_printf(m, "%30s%30s\n", "Hactive", "Vactive");
	seq_printf(m, "%30d%30d\n",
				ctx->mode.pvideo.mdtd.m_hactive, ctx->mode.pvideo.mdtd.m_vactive);
#if 0
	------------------------------------------------ task id=1112 event pool[0] status ---------------------------------------
	-cnt|err total|hpd|unhpd|edid fail |rsen con|rsen dis
	wr:|0 |0 |0 |0 |0 |0
	rd:|0 |0 |0 |0 |0 |0
	memory[wkflg=0 |rdable= 0| rdptr=0 | wrptr=0 ]:
	seq_puts(m, "\n---------------------------- spec info ----------------------------------\n");
	user_seting |default_seting |cur_seting
	stage |first |second|third |fourth |first |second|third |fourth|
	i_de_main_clk |0 |0 |0 |0 |0 |0 |0 |0 |0
	i_de_main_data|1 |1 |1 |1 |1 |1 |1 |1 |1
	i_main_clk |20 |20 |20 |21 |20 |20 |20 |21 |20
	i_main_data |27 |27 |28 |33 |27 |27 |28 |33 |28
	ft_cap_clk |0 |0 |0 |0 |0 |0 |0 |0 |0
	ft_cap_data |0 |0 |3 |0 |0 |0 |3 |0 |3
	sync sw enable: NO hsync polarity: P
#endif

	return 0;
}

static int hdmi_proc_write_parse(char *input_data)
{
	char file_path[128];
	u32 edid;
	u32 avmute_enable;
	u32 audioMute_enable;
	u32 cbar_enable;
	u32 scdc, addr, data;
	u32 outclrspace;
	u32 ddc;
	u32 hdmi_mode;
	u32 cmd;
	u32 phy;

	if (strcmp(input_data, "help\n") == 0)
	{
		// input help for all commands
		pr_info("Available debug commands:\n");
		pr_info("1. Command: help(h) - echo help > /proc/soph/hdmi\n");
		pr_info("2. Command: edid(ed) - echo edid argv1 argv2 > /proc/soph/hdmi\n");
		pr_info("3. Command: avmute(a) - echo avmute argv1 > /proc/soph/hdmi\n");
		pr_info("4. Command: audio_mute(a) - echo audio_mute argv1 > /proc/soph/hdmi\n");
		pr_info("5. Command: cbar(c) - echo cbar argv1 > /proc/soph/hdmi\n");
		pr_info("6. Command: scdc(sc) - echo scdc argv1 argv2 argv3 > /proc/soph/hdmi\n");
		pr_info("7. Command: outclrspace(oc) - echo outclrspace argv1 > /proc/soph/hdmi\n");
		pr_info("8. Command: ddc(dc) - echo ddc argv1(Hz) > /proc/soph/hdmi\n");
		pr_info("9. Command: hdmimode(m) - echo hdmimode argv1 > /proc/soph/hdmi\n");
		pr_info("10. Command: cmd - echo cmd argv1 > /proc/soph/hdmi\n");
		pr_info("11. Command: phy - echo phy argv1 argv2 argv3 > /proc/soph/hdmi\n");
	} else if (sscanf(input_data, "edid %d %s", &edid, file_path) == 2){
		if (hdmi_proc_edid_cmd(edid, file_path) < 0) {
			pr_err("Read EDID failed \n");
			return -1;
		}

	} else if(sscanf(input_data, "avmute %d", &avmute_enable) == 1){
		if (avmute_enable != 0 && avmute_enable != 1) {
			pr_err("Invalid Param avmute_enable(%d) \n", avmute_enable);
			return -EINVAL;
		}

		if (hdmitx_set_avmute(avmute_enable)) {
			pr_err("%s: Set HDMI Avmute failed \n", __func__);
			return -1;
		}

	} else if(sscanf(input_data, "audio_mute %d", &audioMute_enable) == 1){
		if (audioMute_enable != 0 && audioMute_enable != 1) {
			pr_err("Invalid Param audioMute_enable(%d) \n", audioMute_enable);
			return -EINVAL;
		}

		if (hdmitx_set_audio_mute(audioMute_enable)) {
			pr_err("%s: Set HDMI Audio Mute failed \n", __func__);
			return -1;
		}

	} else if(sscanf(input_data, "cbar %d", &cbar_enable) == 1){
		if (hdmi_proc_cbar_cmd(cbar_enable)) {
			pr_err("%s: Set HDMI cbar failed \n", __func__);
			return -1;
		}

	} else if (sscanf(input_data, "scdc %d 0x%x %d", &scdc, &addr, &data) == 3) {
		if (hdmi_proc_scdc_cmd(scdc, addr, data)) {
			pr_err("%s: Set HDMI SCDC failed \n", __func__);
			return -1;
		}

	} else if (sscanf(input_data, "ddc %d ", &ddc) == 1) {
		if (hdmi_proc_ddc_cmd(ddc)) {
			pr_err("%s: Set DDC Rate failed \n", __func__);
			return -1;
		}

	} else if (sscanf(input_data, "outclrspace %d ", &outclrspace) == 1) {
		if (hdmi_proc_outclrspace_parse(outclrspace)) {
			pr_err("%s: Set HDMI OutClrspace failed \n", __func__);
			return -1;
		}

	} else if (sscanf(input_data, "hdmimode %d ", &hdmi_mode) == 1){
		if (hdmi_proc_mode_cmd(hdmi_mode)) {
			pr_err("%s: Set HDMI Mode Failed \n", __func__);
			return -1;
		}

	} else if (sscanf(input_data, "cmd %d ", &cmd) == 1){
		if (hdmi_proc_control_cmd(cmd)) {
			pr_err("%s: Control HDMI Switch Failed \n", __func__);
			return -1;
		}
	} else if (sscanf(input_data, "phy %d 0x%x %d", &phy, &addr, &data) == 3) {
		if (hdmi_proc_phy_cmd(phy, addr, data)) {
			pr_err("%s: Set Phy Failed \n", __func__);
			return -1;
		}
	}

	return 0;
}

static int hdmi_proc_show(struct seq_file *m, void *v)
{
	return hdmi_ctx_proc_show(m, v);
}

static ssize_t hdmi_proc_write(struct file *file, const char __user *user_buf, size_t count, loff_t *ppos)
{
	char cproc_input_data[MAX_PROC_STR_SIZE] = {'\0'};

	if (user_buf == NULL || count >= MAX_PROC_STR_SIZE) {
		pr_err("Invalid input value\n");
		return -EINVAL;
	}

	if (copy_from_user(cproc_input_data, user_buf, count)) {
		pr_err("copy_from_user fail\n");
		return -EFAULT;
	}

	if(hdmi_proc_write_parse(cproc_input_data) < 0)
		return -EFAULT;

	return count;
}

static int hdmi_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, hdmi_proc_show, PDE_DATA(inode));
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
static const struct proc_ops hdmi_proc_fops = {
	.proc_open = hdmi_proc_open,
	.proc_read = seq_read,
	.proc_write = hdmi_proc_write,
	.proc_lseek = seq_lseek,
	.proc_release = single_release,
};
#else
static const struct file_operations hdmi_proc_fops = {
	.owner = THIS_MODULE,
	.open = hdmi_proc_open,
	.read = seq_read,
	.write = hdmi_proc_write,
	.llseek = seq_lseek,
	.release = single_release,
};
#endif

int hdmi_proc_init(struct hdmitx_dev *dev)
{
	struct proc_dir_entry *entry;

	entry = proc_create_data(HDMI_PROC_NAME, 0644, NULL,
				 &hdmi_proc_fops, dev);
	if (!entry) {
		CVI_TRACE_HDMI(CVI_DBG_ERR, "hdmi proc creation failed\n");
		return -ENOMEM;
	}

	return 0;
}

int hdmi_proc_remove(struct hdmitx_dev *dev)
{
	remove_proc_entry(HDMI_PROC_NAME, NULL);
	return 0;
}

/*************************************************************************
 *	HDMI Video Proc functions
 *************************************************************************/
int hdmi_video_ctx_proc_show(struct seq_file *m, void *v)
{
	struct hdmi_tx_ctx *ctx = get_hdmi_ctx();
	char str[30];
	seq_puts(m, "HDMI version: 2.0\n");
	seq_puts(m, "\n-------------------- VideoAttr ------------------------------- AVIInfo ------------------\n");
	seq_printf(m, "%30s%40s\n", "video timing", "avi infoframe enable");
	seq_printf(m, "%22d*%dp%d %d:%d%40s\n",
				ctx->mode.pvideo.mdtd.m_hactive,
				ctx->mode.pvideo.mdtd.m_vactive,
				dtd_get_refresh_rate(&ctx->mode.pvideo.mdtd)/1000,
				ctx->mode.pvideo.mdtd.m_himage_size,
				ctx->mode.pvideo.mdtd.m_vimage_size,
				"YES");
	seq_printf(m, "\n");
	seq_printf(m, "%30s%40s\n", "disp fmt", "current format");
	seq_printf(m, "%27dP@%d%26d*%dp%d %d:%d(vic=%d)\n",
				ctx->mode.pvideo.mdtd.m_vactive,
				dtd_get_refresh_rate(&ctx->mode.pvideo.mdtd)/1000,
				ctx->mode.pvideo.mdtd.m_hactive,
				ctx->mode.pvideo.mdtd.m_vactive,
				dtd_get_refresh_rate(&ctx->mode.pvideo.mdtd)/1000,
				ctx->mode.pvideo.mdtd.m_himage_size,
				ctx->mode.pvideo.mdtd.m_vimage_size,
				ctx->mode.pvideo.mdtd.m_code);
	seq_printf(m, "\n");
	seq_printf(m, "%30s%40s\n", "pixel clk", "vsif format");
	seq_printf(m, "%30d%40s\n",
				ctx->hdmi_tx.snps_hdmi_ctrl.pixel_clock,
				"Reserved");
	seq_printf(m, "\n");
	seq_printf(m, "%30s%40s\n", "in bit depth", "bar data present");
	seq_printf(m, "%30s%40s\n" ,"8 bit",  "NONE");
	seq_printf(m, "\n");
	seq_printf(m, "%30s%40s\n", "in color space", "color space");
	seq_printf(m, "%30s", _pix_fmt_to_string((hdmi_video_mode)ctx->mode.pvideo.mencodingin, str));
	seq_printf(m, "%40s\n", _pix_fmt_to_string((hdmi_video_mode)ctx->mode.pvideo.mencodingout, str));
	seq_printf(m, "\n");
	seq_printf(m, "%30s%40s\n", "colorimetry", "pixel repeat");
	seq_printf(m, "%30s%40d\n",
				ctx->mode.pvideo.mcolorimetry == 1 ? "ITU R BT.601": "ITU R BT.709",
				ctx->hdmi_tx.snps_hdmi_ctrl.pixel_repetition);
	seq_printf(m, "\n");
	seq_printf(m, "%30s%40s\n", "pic aspect ratio", "act aspect ratio");
	seq_printf(m, "%28d:%d%40s\n",
				ctx->mode.pvideo.mdtd.m_himage_size,
				ctx->mode.pvideo.mdtd.m_vimage_size,
				"Reserved");
	seq_printf(m, "\n");
	seq_printf(m, "%30s%40s\n", "ycc quantization", "Vactive");
	seq_printf(m, "%30s%40s\n",
				"LIMITED",
				_quant_range_to_string(ctx->mode.pvideo.mrgb_quantization_range, str));
	seq_printf(m, "\n");
	seq_printf(m, "%30s%40s\n", "ext colorimetry", "ext colorimetry");
	seq_printf(m, "%30s%40s\n", "Reserved", "Reserved");
	seq_printf(m, "\n");
	seq_printf(m, "%30s%40s\n", "stereo mode", "it content valid");
	seq_printf(m, "%30s%40s\n", "NONE" ,"NO");
	seq_printf(m, "\n");
	seq_printf(m, "%30s%40s\n", "hsync pol", "vsync pol");
	seq_printf(m, "%30d%40d\n",
				ctx->mode.pvideo.mdtd.m_hsync_polarity,
				ctx->mode.pvideo.mdtd.m_vsync_polarity);
	seq_printf(m, "\n");
	seq_puts(m, "------------------- video path -----------------------------------\n");
	seq_printf(m, "%30s%40s\n", "video mute", "pic scaling");
	seq_printf(m, "%30s%40s\n",
				ctx->hdmi_tx.snps_hdmi_ctrl.avmute ? "YES": "NO",
				"UNKNOWN");
	seq_printf(m, "\n");
	seq_printf(m, "%30s%40s\n", "out bit depth", "act fmt present");
	seq_printf(m, "%30d%40s\n",
				ctx->mode.pvideo.mencodingout == 2 ? 12: 8,
				"Reserved");
	seq_printf(m, "\n");
	seq_printf(m, "%30s%40s\n", "out color space", "scan info");
	seq_printf(m, "%30s%40s\n",
				_pix_fmt_to_string((hdmi_video_mode)ctx->mode.pvideo.mencodingout, str),
				"NONE");
	seq_printf(m, "\n");
	seq_printf(m, "%30s%40s\n", "it content type", "ycbcr422_444");
	seq_printf(m, "%30s%40s\n", "GRAPHICS", "NO");
	seq_printf(m, "\n");
	seq_printf(m, "%30s%40s\n", "ycbcr420_422", "ycbcr444_422");
	seq_printf(m, "%30s%40s\n", "NO",
				(ctx->mode.pvideo.mencodingin == 1 &&
				ctx->mode.pvideo.mencodingout == 2) ? "YES": "NO");
	seq_printf(m, "\n");
	seq_printf(m, "%30s%40s\n", "ycbcr422_420", "rgb2ycbcr");
	seq_printf(m, "%30s%40s\n", "NO",
				(ctx->mode.pvideo.mencodingin == 0 &&
				(ctx->mode.pvideo.mencodingout == 1 ||
				 ctx->mode.pvideo.mencodingout == 2)) ? "YES": "NO");
	seq_printf(m, "\n");
	seq_printf(m, "%30s\n", "ycbcr2rgb");
	seq_printf(m, "%30s\n",
				(ctx->mode.pvideo.mencodingin == 1 &&
				 ctx->mode.pvideo.mencodingout == 0) ? "YES": "NO");
	seq_printf(m, "\n");
	seq_printf(m, "%30s%40s\n", "dither", "deep color mode");
	seq_printf(m, "%30s%40s\n", "Reserved", "24 bit(OFF)");
	seq_printf(m, "\n");
	seq_printf(m, "%30s\n", "avi info raw data");
	seq_printf(m, "%30s\n", "Reserved");
	seq_printf(m, "\n");
	seq_printf(m, "%30s\n", "vsif info raw data");
	seq_printf(m, "%30s\n", "Reserved");

	return 0;
}

static int hdmi_video_proc_show(struct seq_file *m, void *v)
{
	return hdmi_video_ctx_proc_show(m, v);
}

static ssize_t hdmi_video_proc_write(struct file *file, const char __user *user_buf, size_t count, loff_t *ppos)
{
	return count;
}

static int hdmi_video_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, hdmi_video_proc_show, PDE_DATA(inode));
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
static const struct proc_ops hdmi_video_proc_fops = {
	.proc_open = hdmi_video_proc_open,
	.proc_read = seq_read,
	.proc_write = hdmi_video_proc_write,
	.proc_lseek = seq_lseek,
	.proc_release = single_release,
};
#else
static const struct file_operations hdmi_video_proc_fops = {
	.owner = THIS_MODULE,
	.open = hdmi_video_proc_open,
	.read = seq_read,
	.write = hdmi_video_proc_write,
	.llseek = seq_lseek,
	.release = single_release,
};
#endif

int hdmi_video_proc_init(struct hdmitx_dev *dev)
{
	struct proc_dir_entry *entry;

	entry = proc_create_data(HDMI_VIDEO_PROC_NAME, 0644, NULL,
				 &hdmi_video_proc_fops, dev);
	if (!entry) {
		CVI_TRACE_HDMI(CVI_DBG_ERR, "hdmi_video proc creation failed\n");
		return -ENOMEM;
	}

	return 0;
}

int hdmi_video_proc_remove(struct hdmitx_dev *dev)
{
	remove_proc_entry(HDMI_VIDEO_PROC_NAME, NULL);
	return 0;
}

/*************************************************************************
 *	HDMI Audio Proc functions
 *************************************************************************/

int hdmi_audio_ctx_proc_show(struct seq_file *m, void *v)
{
	struct hdmi_tx_ctx *ctx = get_hdmi_ctx();
	seq_puts(m, "HDMI version: 2.0\n");
	seq_puts(m, "------------------- Audio Attr ------------------ Audio Info ----------\n");
	seq_printf(m, "%30s%30s\n", "sound intf", "audio info enable");
	seq_printf(m, "%30s%30s\n", "AHB-DMA", "YES");
	seq_printf(m, "\n");
	seq_printf(m, "%30s%30s\n", "code type", "code type");
	seq_printf(m, "%30s%30s\n", "PCM", "PCM");
	seq_printf(m, "\n");
	seq_printf(m, "%30s%30s\n", "channel cnt", "channel cnt");
	seq_printf(m, "%30d%30d\n",
				ctx->hdmi_tx.snps_hdmi_ctrl.channel_cnt,
				ctx->hdmi_tx.snps_hdmi_ctrl.channel_cnt);
	seq_printf(m, "\n");
	seq_printf(m, "%30s%30s\n", "sample freq", "sample freq");
	seq_printf(m, "%30d%30d\n",
				ctx->mode.paudio.msampling_frequency,
				ctx->mode.paudio.msampling_frequency);
	seq_printf(m, "\n");
	seq_printf(m, "%30s%30s\n", "sample depth", "sample depth");
	seq_printf(m, "%30d%30d\n",
				ctx->mode.paudio.msample_size,
				ctx->mode.paudio.msample_size);
	seq_printf(m, "\n");
	seq_printf(m, "%30s%30s\n", "down sample", "sample size");
	seq_printf(m, "%30s%30s\n", "NO", "STR_HEADER");
	seq_printf(m, "\n");
	seq_printf(m, "------------------- audio path --------------------------------------------\n");
	seq_printf(m, "%30s%30s\n", "audio enable", "level shift value");
	seq_printf(m, "%30s%28ddB\n",
				ctx->mode.paudio.audio_en ? "YES": "NO",
				ctx->mode.paudio.mLevel_shift_value);
	seq_printf(m, "\n");
	seq_printf(m, "%30s%30s\n", "audio mute", "lfe playback");
	seq_printf(m, "%30s%30s\n",
				ctx->hdmi_tx.snps_hdmi_ctrl.audio_mute ? "YES": "NO",
				"UNKNOWN");
	seq_printf(m, "\n");
	seq_printf(m, "%30s%30s\n", "sound intf", "channel/speakeralloc");
	seq_printf(m, "%30s%30x\n",
				"AHB-DMA",
				ctx->mode.paudio.mchannel_allocation);
	seq_printf(m, "\n");
	seq_printf(m, "%30s%30s\n", "channel cnt", "audio info raw data");
	seq_printf(m, "%30d%30s\n",
				ctx->hdmi_tx.snps_hdmi_ctrl.channel_cnt,
				" ");
	seq_printf(m, "\n");
	seq_printf(m, "%30s%30s\n", "sample freq", "sample depth");
	seq_printf(m, "%30d%30d\n",
				ctx->mode.paudio.msampling_frequency,
				ctx->mode.paudio.msample_size);
	seq_printf(m, "\n");
	seq_printf(m, "%30s%30s\n", "down sample", "down mix inhibit");
	seq_printf(m, "%30s%30s\n", "NO",
				ctx->mode.paudio.mdown_mix_inhibit_flag ? "YES": "NO");
	seq_printf(m, "\n");
	seq_printf(m, "%30s%30s\n", "cts value", "n value");
	seq_printf(m, "%30d%30d\n",
				ctx->hdmi_tx.snps_hdmi_ctrl.cts,
				ctx->hdmi_tx.snps_hdmi_ctrl.n);

	return 0;
}

static int hdmi_audio_proc_show(struct seq_file *m, void *v)
{
	return hdmi_audio_ctx_proc_show(m, v);
}

static ssize_t hdmi_audio_proc_write(struct file *file, const char __user *user_buf, size_t count, loff_t *ppos)
{
	return count;
}

static int hdmi_audio_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, hdmi_audio_proc_show, PDE_DATA(inode));
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
static const struct proc_ops hdmi_audio_proc_fops = {
	.proc_open = hdmi_audio_proc_open,
	.proc_read = seq_read,
	.proc_write = hdmi_audio_proc_write,
	.proc_lseek = seq_lseek,
	.proc_release = single_release,
};
#else
static const struct file_operations hdmi_audio_proc_fops = {
	.owner = THIS_MODULE,
	.open = hdmi_audio_proc_open,
	.read = seq_read,
	.write = hdmi_audio_proc_write,
	.llseek = seq_lseek,
	.release = single_release,
};
#endif

int hdmi_audio_proc_init(struct hdmitx_dev *dev)
{
	struct proc_dir_entry *entry;

	entry = proc_create_data(HDMI_AUDIO_PROC_NAME, 0644, NULL,
				 &hdmi_audio_proc_fops, dev);
	if (!entry) {
		CVI_TRACE_HDMI(CVI_DBG_ERR, "hdmi_audio proc creation failed\n");
		return -ENOMEM;
	}

	return 0;
}

int hdmi_audio_proc_remove(struct hdmitx_dev *dev)
{
	remove_proc_entry(HDMI_AUDIO_PROC_NAME, NULL);
	return 0;
}

/*************************************************************************
 *	HDMI Sink Proc functions
 *************************************************************************/
int hdmi_sink_ctx_proc_show(struct seq_file *m, void *v)
{
	struct hdmi_tx_ctx *ctx = get_hdmi_ctx();
	int i = 0;

	if(!ctx->mode.sink_cap) {
		seq_puts(m, "hdmi_tx is not connected with sink\n");
		return 0;
	}

	seq_puts(m, "HDMI version: 2.0\n");
	seq_puts(m, "---------------------------- edid raw data ----------------------------\n");
	seq_printf(m, "/* 00H: */");
	for(i = 0; i < 16; i++)
	{
		seq_printf(m, "0x%02x, ", ((u8*)&ctx->mode.edid)[i]);
	}
	seq_printf(m, "\n/* 0fH: */");
	for(i = 0; i < 16; i++)
	{
		seq_printf(m, "0x%02x, ", ((u8*)(&ctx->mode.edid))[i + 16]);
	}
	seq_printf(m, "\n/* 1fH: */");
	for(i = 0; i < 16; i++)
	{
		seq_printf(m, "0x%02x, ", ((u8*)&ctx->mode.edid)[i + 32]);
	}
	seq_printf(m, "\n/* 2fH: */");
	for(i = 0; i < 16; i++)
	{
		seq_printf(m, "0x%02x, ", ((u8*)&ctx->mode.edid)[i + 48]);
	}
	seq_printf(m, "\n/* 3fH: */");
	for(i = 0; i < 16; i++)
	{
		seq_printf(m, "0x%02x, ", ((u8*)&ctx->mode.edid)[i + 64]);
	}
	seq_printf(m, "\n/* 4fH: */");
	for(i = 0; i < 16; i++)
	{
		seq_printf(m, "0x%02x, ", ((u8*)&ctx->mode.edid)[i + 80]);
	}
	seq_printf(m, "\n/* 5fH: */");
	for(i = 0; i < 16; i++)
	{
		seq_printf(m, "0x%02x, ", ((u8*)&ctx->mode.edid)[i + 96]);
	}
	seq_printf(m, "\n/* 6fH: */");
	for(i = 0; i < 16; i++)
	{
		seq_printf(m, "0x%02x, ", ((u8*)&ctx->mode.edid)[i + 112]);
	}
	seq_printf(m, "\n/* 7fH: */");
	for(i = 0; i < 16; i++)
	{
		seq_printf(m, "0x%02x, ", ctx->mode.edid_ext[0][i]);
	}
	seq_printf(m, "\n/* 8fH: */");
	for(i = 0; i < 16; i++)
	{
		seq_printf(m, "0x%02x, ", ctx->mode.edid_ext[0][i + 16]);
	}
	seq_printf(m, "\n/* 9fH: */");
	for(i = 0; i < 16; i++)
	{
		seq_printf(m, "0x%02x, ", ctx->mode.edid_ext[0][i + 32]);
	}
	seq_printf(m, "\n/* afH: */");
	for(i = 0; i < 16; i++)
	{
		seq_printf(m, "0x%02x, ", ctx->mode.edid_ext[0][i + 48]);
	}
	seq_printf(m, "\n/* bfH: */");
	for(i = 0; i < 16; i++)
	{
		seq_printf(m, "0x%02x, ", ctx->mode.edid_ext[0][i + 64]);
	}
	seq_printf(m, "\n/* cfH: */");
	for(i = 0; i < 16; i++)
	{
		seq_printf(m, "0x%02x, ", ctx->mode.edid_ext[0][i + 80]);
	}
	seq_printf(m, "\n/* dfH: */");
	for(i = 0; i < 16; i++)
	{
		seq_printf(m, "0x%02x, ", ctx->mode.edid_ext[0][i + 96]);
	}
	seq_printf(m, "\n/* efH: */");
	for(i = 0; i < 16; i++)
	{
		seq_printf(m, "0x%02x, ", ctx->mode.edid_ext[0][i + 112]);
	}
	seq_printf(m, "\n");
	seq_printf(m, "\n");

	if(ctx->mode.sink_cap) {
		seq_puts(m, "-------------------------------- sw status ------------------------------\n");
		seq_printf(m, "%30s%20s\n", "cap from sink", "raw update err cnt");
		seq_printf(m, "%30s%20s\n",
					ctx->mode.sink_cap ? "YES" : "NO",
					"Reserved");
		seq_printf(m, "\n");
		seq_printf(m, "%30s%20s\n", "cap is valid", "parse error type");
		seq_printf(m, "%30s%20s\n",
					ctx->mode.sink_cap ? "YES" : "NO",
					"Reserved");
		seq_printf(m, "\n");
		seq_printf(m, "%30s%20s\n", "raw is valid", "parse warn type");
		seq_printf(m, "%30s%20s\n",
					(&ctx->mode.edid && ctx->mode.edid_ext) ? "YES" : "NO",
					"Reserved");
		seq_printf(m, "\n");
		seq_printf(m, "%30s%20s\n", "raw get err cnt", "raw length");
		seq_printf(m, "%30s%20s\n","Reserved", "256");
		seq_printf(m, "\n");
		seq_puts(m, "-------------------------------- basic cap ------------------------------\n");
		seq_printf(m, "%30s%20s\n", "hdmi1.4 support", "1st block version");
		seq_printf(m, "%30s%18d.%d\n",
					ctx->mode.pvideo.mhdmi ? "YES" : "NO",
					ctx->mode.edid.version,
					ctx->mode.edid.revision);
		seq_printf(m, "\n");
		seq_printf(m, "%30s%20s\n", "hdmi2.0 support", "manufacturer name");
		seq_printf(m, "%30s%20s\n",
					ctx->mode.sink_cap->edid_m20sink ? "YES" : "NO",
					ctx->mode.sink_cap->edid_mmonitor_name);
		seq_printf(m, "\n");
		seq_printf(m, "%30s%20s\n", "max tmds clock(MHz)", "product code");
		seq_printf(m, "%30d%20s\n",
					ctx->mode.sink_cap->edid_mhdmi_forumvsdb.m_max_tmds_char_rate,
					"Reserved");
		seq_printf(m, "\n");
		seq_printf(m, "%30s%20s\n", "serial number", "week of manufacture");
		seq_printf(m, "%30s%20s\n", "Reserved", "Reserved");
		seq_printf(m, "\n");
		seq_printf(m, "%30s%20s\n", "max disp width", "maxdisp height");
		seq_printf(m, "%30s%20s\n", "Reserved", "Reserved");
		seq_printf(m, "\n");
		seq_printf(m, "%30s%20s\n", "scdc support", "year of manufacture");
		seq_printf(m, "%30s%20s\n",
					ctx->mode.sink_cap->edid_mhdmi_forumvsdb.m_scdc_Present ? "YES" : "NO",
					"Reserved");
		seq_printf(m, "\n");
		seq_printf(m, "%30s%20s\n", "dvi dual support", "cec addr is valid");
		seq_printf(m, "%30s%20s\n", "Reserved", "Reserved");
		seq_printf(m, "\n");
		seq_printf(m, "%30s%20s\n", "ai support", "cec addr");
		seq_printf(m, "%30s%20s\n", "Reserved", "Reserved");
		seq_printf(m, "\n");
		seq_printf(m, "%30s%20s\n", "ext block cnt", "speaker support");
		seq_printf(m, "%30s%20s\n", "Reserved", "Reserved");
		seq_printf(m, "\n");
		seq_printf(m, "%30s%20s\n", "rgb quan selectable", "ycc quan selectable");
		seq_printf(m, "%30s%20s\n",
					ctx->mode.sink_cap->edid_mvideo_capability_datablock.mquantization_range_selectable ? "YES" : "NO",
					ctx->mode.sink_cap->edid_mvideo_capability_datablock.mquantization_range_selectable ? "YES" : "NO");
		seq_printf(m, "\n");
		seq_puts(m, "-------------------------------- video cap ------------------------------\n");
		seq_printf(m, "%30s\n", "native format");
		seq_printf(m, "%16dx%d%s@%d %d:%d\n",
					ctx->mode.sink_capinfo[0].sink_cap_info.m_hactive,
					ctx->mode.sink_capinfo[0].sink_cap_info.m_vactive,
					ctx->mode.sink_capinfo[0].sink_cap_info.m_interlaced ? "i" : "p",
					ctx->mode.sink_capinfo[0].fresh_rate / 1000,
					ctx->mode.sink_capinfo[0].sink_cap_info.m_himage_size,
					ctx->mode.sink_capinfo[0].sink_cap_info.m_vimage_size);
		seq_printf(m, "\n");
		seq_printf(m, "%30s\n", "color space");
		seq_printf(m, "%12s %s %s %s\n",
					"RGB888",
					ctx->mode.sink_cap->edid_mycc444_support ? "YCbCr444" : " ",
					ctx->mode.sink_cap->edid_mycc422_support ? "YCbCr422" : " ",
					ctx->mode.sink_cap->edid_mycc420_support ? "YCbCr420" : " ");
		seq_printf(m, "\n");
		seq_printf(m, "%30s\n", "deep color");
		seq_printf(m, "%30s %s %s\n",
					ctx->mode.sink_cap->edid_mhdmivsdb.m_deep_color30 ? "RGB_30bit" : " ",
					ctx->mode.sink_cap->edid_mhdmivsdb.m_deep_color36 ? "RGB_36bit" : " ",
					ctx->mode.sink_cap->edid_mhdmivsdb.m_deep_color48 ? "RGB_48bit" : " ");
		seq_printf(m, "%30s\n", "ycbcr420 deep color");
		seq_printf(m, "%30s %s %s\n",
					ctx->mode.sink_cap->edid_mhdmi_forumvsdb.mdc_30bit_420 ? "YCbCr420_30bit" : " ",
					ctx->mode.sink_cap->edid_mhdmi_forumvsdb.mdc_36bit_420 ? "YCbCr420_36bit" : " ",
					ctx->mode.sink_cap->edid_mhdmi_forumvsdb.mdc_48bit_420 ? "YCbCr420_48bit" : " ");
		seq_printf(m, "%30s\n", "ycbcr420[also]");
		for(i = 0; ctx->mode.sink_capinfo[i].sink_cap_info.m_code != 0; i++) {
			if(ctx->mode.sink_capinfo[i].sink_cap_info.m_ycc420) {
				seq_printf(m, "%30s:%d ", "vic", ctx->mode.sink_capinfo[i].sink_cap_info.m_code);
			}
		}
		seq_printf(m, "\n");
		seq_printf(m, "%30s\n", "ycbcr420[only]");
		for(i = 0; ctx->mode.sink_capinfo[i].sink_cap_info.m_code != 0; i++) {
			if(ctx->mode.sink_capinfo[i].sink_cap_info.m_limited_to_ycc420) {
				seq_printf(m, "%30s:%d ", "vic",ctx->mode.sink_capinfo[i].sink_cap_info.m_code);
			}
		}
		seq_printf(m, "\n");
		seq_printf(m, "%30s\n", "colorimetry");
		seq_printf(m, "%30s ", ctx->mode.sink_cap->xv_ycc601 ? "xv_ycc601" : "");
		seq_printf(m, "%30s ", ctx->mode.sink_cap->xv_ycc709 ? "xv_ycc709" : "");
		seq_printf(m, "%30s ", ctx->mode.sink_cap->s_ycc601 ? "s_ycc601" : "");
		seq_printf(m, "%30s ", ctx->mode.sink_cap->adobe_rgb ? "adobe_rgb" : "");
		seq_printf(m, "%30s ", ctx->mode.sink_cap->adobe_ycc601 ? "adobe_ycc601" : "");
		seq_printf(m, "\n");
		seq_puts(m, "-------------------------------- format cap -----------------------------\n");
		for(i = 1; ctx->mode.sink_capinfo[i-1].sink_cap_info.m_code != 0; i++) {
			seq_printf(m, "%30dx%d%s@%d %d:%d(m_code %d)\n",
					ctx->mode.sink_capinfo[i-1].sink_cap_info.m_hactive,
					ctx->mode.sink_capinfo[i-1].sink_cap_info.m_interlaced ?
					ctx->mode.sink_capinfo[i-1].sink_cap_info.m_vactive * 2 :
					ctx->mode.sink_capinfo[i-1].sink_cap_info.m_vactive,
					ctx->mode.sink_capinfo[i-1].sink_cap_info.m_interlaced ? "i" : "p",
					ctx->mode.sink_capinfo[i-1].fresh_rate / 1000,
					ctx->mode.sink_capinfo[i-1].sink_cap_info.m_himage_size,
					ctx->mode.sink_capinfo[i-1].sink_cap_info.m_vimage_size,
					ctx->mode.sink_capinfo[i-1].sink_cap_info.m_code);
		}
		seq_puts(m, "---------------------------------- audio cap ----------------------------\n");

		seq_printf(m, "%30s\n", "NO.0");

		seq_printf(m, "%30s%30s\n", "code type", "max channel num");
		seq_printf(m, "%30s%30d\n",
						"L-PCM",
						ctx->mode.sink_cap->edid_msad->m_max_channels);
		seq_printf(m, "\n");
		seq_printf(m, "%30s%30s\n", "max bit rate(KHz)", "bit depth");
		seq_printf(m, "%28dhz%20s",
						ctx->mode.sink_cap->support_sample_rate[0],
					" ");
		for(i = 0; ctx->mode.sink_cap->support_bit_depth[i] != 0; i++) {
			seq_printf(m, " %d  ", ctx->mode.sink_cap->support_bit_depth[i]);
		}
		seq_printf(m, "\n");
		seq_printf(m, "\n");
		seq_printf(m, "%30s\n", "sample rate(Hz)");
		seq_printf(m, "%30s", " ");
		for(i = 0; ctx->mode.sink_cap->support_bit_depth[i] != 0; i++) {
			seq_printf(m, "%s%d  ", " ", ctx->mode.sink_cap->support_sample_rate[i]);
		}
		seq_printf(m, "\n");
		seq_puts(m, "---------------------------------- detail timing ----------------------------\n");
		seq_puts(m, "[NO.]:  hact|vact|p/i |pclk  |aspw|asph|hfb |hpw |hbb |vfb |vpw |vbb |img_w|img_h|ihs |ivs |idv\n");

		for(i = 0; ctx->mode.sink_capinfo[i].sink_cap_info.m_code != 0; i++) {
			seq_printf(m, "[NO.%2d]:%4d|%4d|%2s  |%6d|%4d|%4d|%4d|%4d|%4d|%4d|%4d|%4d|%4d |%4d |%3s |%3s |%3s\n",
						i,
						ctx->mode.sink_capinfo[i].sink_cap_info.m_hactive,
						ctx->mode.sink_capinfo[i].sink_cap_info.m_interlaced ?
						ctx->mode.sink_capinfo[i].sink_cap_info.m_vactive * 2 :
						ctx->mode.sink_capinfo[i].sink_cap_info.m_vactive,
						ctx->mode.sink_capinfo[i].sink_cap_info.m_interlaced ? "i" : "p",
						ctx->mode.sink_capinfo[i].sink_cap_info.m_pixel_clock,
						ctx->mode.sink_capinfo[i].sink_cap_info.m_himage_size,
						ctx->mode.sink_capinfo[i].sink_cap_info.m_vimage_size,
						ctx->mode.sink_capinfo[i].sink_cap_info.m_hsync_offset,
						ctx->mode.sink_capinfo[i].sink_cap_info.m_hsync_pulse_width,
						ctx->mode.sink_capinfo[i].sink_cap_info.m_hblanking -
						ctx->mode.sink_capinfo[i].sink_cap_info.m_hsync_offset -
						ctx->mode.sink_capinfo[i].sink_cap_info.m_hsync_pulse_width,
						ctx->mode.sink_capinfo[i].sink_cap_info.m_vsync_offset,
						ctx->mode.sink_capinfo[i].sink_cap_info.m_vsync_pulse_width,
						ctx->mode.sink_capinfo[i].sink_cap_info.m_vblanking -
						ctx->mode.sink_capinfo[i].sink_cap_info.m_vsync_offset -
						ctx->mode.sink_capinfo[i].sink_cap_info.m_vsync_pulse_width,
						ctx->mode.sink_capinfo[i].sink_cap_info.m_himage_size,
						ctx->mode.sink_capinfo[i].sink_cap_info.m_vimage_size,
						ctx->mode.sink_capinfo[i].sink_cap_info.m_hsync_polarity ? "YES" : "NO",
						ctx->mode.sink_capinfo[i].sink_cap_info.m_vsync_polarity ? "YES" : "NO",
						ctx->hdmi_tx.snps_hdmi_ctrl.data_enable_polarity ? "YES" : "NO");
		}
	}

	return 0;
}

static int hdmi_sink_proc_show(struct seq_file *m, void *v)
{
	return hdmi_sink_ctx_proc_show(m, v);
}

static ssize_t hdmi_sink_proc_write(struct file *file, const char __user *user_buf, size_t count, loff_t *ppos)
{
	return count;
}

static int hdmi_sink_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, hdmi_sink_proc_show, PDE_DATA(inode));
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
static const struct proc_ops hdmi_sink_proc_fops = {
	.proc_open = hdmi_sink_proc_open,
	.proc_read = seq_read,
	.proc_write = hdmi_sink_proc_write,
	.proc_lseek = seq_lseek,
	.proc_release = single_release,
};
#else
static const struct file_operations hdmi_sink_proc_fops = {
	.owner = THIS_MODULE,
	.open = hdmi_sink_proc_open,
	.read = seq_read,
	.write = hdmi_sink_proc_write,
	.llseek = seq_lseek,
	.release = single_release,
};
#endif

int hdmi_sink_proc_init(struct hdmitx_dev *dev)
{
	struct proc_dir_entry *entry;

	entry = proc_create_data(HDMI_SINK_PROC_NAME, 0644, NULL,
				 &hdmi_sink_proc_fops, dev);
	if (!entry) {
		CVI_TRACE_HDMI(CVI_DBG_ERR, "hdmi_sink proc creation failed\n");
		return -ENOMEM;
	}

	return 0;
}

int hdmi_sink_proc_remove(struct hdmitx_dev *dev)
{
	remove_proc_entry(HDMI_SINK_PROC_NAME, NULL);
	return 0;
}
