/*
 * sunxi Camera Interface  driver
 * Author: raymonxiu
 */
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/version.h>
#include <linux/mutex.h>
#include <linux/videodev2.h>
#include <linux/delay.h>
#include <linux/string.h>

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 20)
#include <linux/freezer.h>
#endif

#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/moduleparam.h>
#include <media/v4l2-device.h>
#include <media/v4l2-ioctl.h>
#include <media/v4l2-common.h>
#include <media/v4l2-mediabus.h>
#include <media/v4l2-subdev.h>
#include <media/videobuf-dma-contig.h>

#include <linux/regulator/consumer.h>
#include <linux/ktime.h>
#include "vfe.h"

#include "bsp_common.h"
#include "lib/bsp_isp_algo.h"
#include "csi_cci/bsp_cci.h"
#include "csi_cci/cci_helper.h"
#include "config.h"
#include "device/camera_cfg.h"
#include "platform/vfe_resource.h"
#include "utility/sensor_info.h"
#include "utility/vfe_io.h"
#include <linux/ion_sunxi.h>

#define IS_FLAG(x,y) (((x)&(y)) == y)
#define CLIP_MAX(x,max) ((x) > max ? max : x )

#define VFE_MAJOR_VERSION 1
#define VFE_MINOR_VERSION 0
#define VFE_RELEASE       0
#define VFE_VERSION \
  KERNEL_VERSION(VFE_MAJOR_VERSION, VFE_MINOR_VERSION, VFE_RELEASE)
#define VFE_MODULE_NAME "sunxi_vfe"
//#define REG_DBG_EN

#define MCLK_OUT_RATE   (24*1000*1000)
#define MAX_FRAME_MEM   (150*1024*1024)
#define MIN_WIDTH       (32)
#define MIN_HEIGHT      (32)
#define MAX_WIDTH       (4800)
#define MAX_HEIGHT      (4800)
#define DUMP_CSI       (1 << 0)
#define DUMP_ISP      (1 << 1)

#define FLASH_EN_POL 1
#define FLASH_MODE_POL 1

#define _FLASH_FUNC_
//#define _REGULATOR_CHANGE_

static struct flash_dev_info fl_info;

static char ccm[I2C_NAME_SIZE] = "";
static uint i2c_addr = 0xff;

static char act_name[I2C_NAME_SIZE] = "";
static uint act_slave = 0xff;
static uint define_sensor_list = 0xff;
static uint vfe_i2c_dbg = 0;
static uint isp_log = 0;
static uint vips = 0xffff;

static int touch_flash_flag = 0;
static int ev_cumul=0;
static unsigned int isp_va_flag = 0;
static unsigned int isp_va_alloc_sel = 0;
static struct vfe_mm isp_load_mm;
static struct vfe_mm isp_saved_mm;

static unsigned int vfe_opened_num = 0;
unsigned int isp_reparse_flag = 0;

static unsigned int frame_cnt = 0;
static unsigned int vfe_dump = 0;
struct mutex probe_hdl_lock;

struct file* fp_dbg = NULL;
#ifdef CONFIG_ARCH_SUN8IW8P1
static char LogFileName[128] = "/mnt/extsd/log.bin";
static char ReparseIniPath[128] = "/mnt/extsd/hawkview/";
#else
static char LogFileName[128] = "/system/etc/hawkview/log.bin";
static char ReparseIniPath[128] = "/system/etc/hawkview/";
#endif

module_param_string(ccm, ccm, sizeof(ccm), S_IRUGO|S_IWUSR);
module_param(i2c_addr,uint, S_IRUGO|S_IWUSR);

module_param_string(act_name, act_name, sizeof(act_name), S_IRUGO|S_IWUSR);
module_param(act_slave,uint, S_IRUGO|S_IWUSR);
module_param(define_sensor_list,uint, S_IRUGO|S_IWUSR);
module_param(vfe_i2c_dbg,uint, S_IRUGO|S_IWUSR);
module_param(isp_log,uint, S_IRUGO|S_IWUSR);

module_param(vips,uint, S_IRUGO|S_IWUSR);
static ssize_t vfe_dbg_en_show(struct device *dev,
		    struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", vfe_dbg_en);
}

static ssize_t vfe_dbg_en_store(struct device *dev,
		    struct device_attribute *attr,
		    const char *buf, size_t count)
{
	int err;
	unsigned long val;

	err = strict_strtoul(buf, 10, &val);
	if (err) {
		vfe_print("Invalid size\n");
		return err;
	}

	if(val < 0 || val > 1) {
		vfe_print("Invalid value, 0~1 is expected!\n");
	} else {
		vfe_dbg_en = val;
		vfe_print("vfe_dbg_en = %ld\n", val);
	}

	return count;
}

static ssize_t vfe_dbg_lv_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", vfe_dbg_lv);
}

static ssize_t vfe_dbg_lv_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	int err;
	unsigned long val;

	err = strict_strtoul(buf, 10, &val);
	if (err) {
		vfe_print("Invalid size\n");
		return err;
	}

	if(val < 0 || val > 4) {
		vfe_print("Invalid value, 0~3 is expected!\n");
	} else {
		vfe_dbg_lv = val;
		vfe_print("vfe_dbg_lv = %d\n", vfe_dbg_lv);
	}

	return count;
}

static ssize_t isp_reparse_flag_show(struct device *dev,
		    struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", isp_reparse_flag);
}

static ssize_t isp_reparse_flag_store(struct device *dev,
		    struct device_attribute *attr,
		    const char *buf, size_t count)
{
	int err;
	unsigned long val;

	err = strict_strtoul(buf, 10, &val);
	if (err) {
		vfe_print("Invalid size\n");
		return err;
	}

	if(val < 0 || val > 4) {
		vfe_print("Invalid value, 0~1 is expected!\n");
	} else {
		isp_reparse_flag = val;
		vfe_print("isp_reparse_flag = %ld\n", val);
	}
	return count;
}
static ssize_t vfe_dbg_dump_show(struct device *dev,
		    struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", vfe_dump);
}

static ssize_t vfe_dbg_dump_store(struct device *dev,
		    struct device_attribute *attr,
		    const char *buf, size_t count)
{
	int err;
	unsigned long val;

	err = strict_strtoul(buf, 10, &val);
	if (err) {
		vfe_print("Invalid size\n");
		return err;
	}

	if(val < 0 || val > 3) {
		vfe_print("Invalid value, 0~3 is expected!\n");
	} else {
		vfe_dump = val;
		vfe_print("vfe_dump = %ld\n", val);
	}
	return count;
}

static DEVICE_ATTR(vfe_dbg_en, S_IRUGO|S_IWUSR|S_IWGRP,
		vfe_dbg_en_show, vfe_dbg_en_store);

static DEVICE_ATTR(vfe_dbg_lv, S_IRUGO|S_IWUSR|S_IWGRP,
		vfe_dbg_lv_show, vfe_dbg_lv_store);
static DEVICE_ATTR(vfe_dump, S_IRUGO|S_IWUSR|S_IWGRP,
		vfe_dbg_dump_show, vfe_dbg_dump_store);
static DEVICE_ATTR(isp_reparse_flag, S_IRUGO|S_IWUSR|S_IWGRP,
		isp_reparse_flag_show, isp_reparse_flag_store);

static struct attribute *vfe_attributes[] = {
	&dev_attr_vfe_dbg_en.attr,
	&dev_attr_vfe_dbg_lv.attr,
	&dev_attr_vfe_dump.attr,
	&dev_attr_isp_reparse_flag.attr,
	NULL
};

static struct attribute_group vfe_attribute_group = {
	.name = "vfe_attr",
	.attrs = vfe_attributes
};

static struct vfe_fmt formats[] = {
	{
		.name         = "planar YUV 422",
		.fourcc       = V4L2_PIX_FMT_YUV422P,
		.depth        = 16,
		.planes_cnt   = 3,
	},
	{
		.name         = "planar YUV 420",
		.fourcc       = V4L2_PIX_FMT_YUV420,
		.depth        = 12,
		.planes_cnt   = 3,
	},
	{
		.name         = "planar YVU 420",
		.fourcc       = V4L2_PIX_FMT_YVU420,
		.depth        = 12,
		.planes_cnt   = 3,
	},
	{
		.name         = "planar YUV 422 UV combined",
		.fourcc       = V4L2_PIX_FMT_NV16,
		.depth        = 16,
		.planes_cnt   = 2,
	},
	{
		.name         = "planar YUV 420 UV combined",
		.fourcc       = V4L2_PIX_FMT_NV12,
		.depth        = 12,
		.planes_cnt   = 2,
	},
	{
		.name         = "planar YUV 422 VU combined",
		.fourcc       = V4L2_PIX_FMT_NV61,
		.depth        = 16,
		.planes_cnt   = 2,
	},
	{
		.name         = "planar YUV 420 VU combined",
		.fourcc       = V4L2_PIX_FMT_NV21,
		.depth        = 12,
		.planes_cnt   = 2,
	},
	{
		.name         = "MB YUV420",
		.fourcc       = V4L2_PIX_FMT_HM12,
		.depth        = 12,
		.planes_cnt   = 2,
	},
	{
		.name         = "YUV422 YUYV",
		.fourcc       = V4L2_PIX_FMT_YUYV,
		.depth        = 16,
		.planes_cnt   = 1,
	},
	{
		.name         = "YUV422 YVYU",
		.fourcc       = V4L2_PIX_FMT_YVYU,
		.depth        = 16,
		.planes_cnt   = 1,
	},
	{
		.name         = "YUV422 UYVY",
		.fourcc       = V4L2_PIX_FMT_UYVY,
		.depth        = 16,
		.planes_cnt   = 1,
	},
	{
		.name         = "YUV422 VYUY",
		.fourcc       = V4L2_PIX_FMT_VYUY,
		.depth        = 16,
		.planes_cnt   = 1,
	},
	{
		.name         = "RAW Bayer BGGR 8bit",
		.fourcc       = V4L2_PIX_FMT_SBGGR8,
		.depth        = 8,
		.planes_cnt   = 1,
	},
	{
		.name         = "RAW Bayer GBRG 8bit",
		.fourcc       = V4L2_PIX_FMT_SGBRG8,
		.depth        = 8,
		.planes_cnt   = 1,
	},
	{
		.name         = "RAW Bayer GRBG 8bit",
		.fourcc       = V4L2_PIX_FMT_SGRBG8,
		.depth        = 8,
		.planes_cnt   = 1,
	},
	{
		.name         = "RAW Bayer RGGB 8bit",
		.fourcc       = V4L2_PIX_FMT_SGRBG8,
		.depth        = 8,
		.planes_cnt   = 1,
	},
	{
		.name         = "RAW Bayer BGGR 10bit",
		.fourcc       = V4L2_PIX_FMT_SBGGR10,
		.depth        = 8,
		.planes_cnt   = 1,
	},
	{
		.name         = "RAW Bayer GBRG 10bit",
		.fourcc       = V4L2_PIX_FMT_SGBRG10,
		.depth        = 8,
		.planes_cnt   = 1,
	},
	{
		.name         = "RAW Bayer GRBG 10bit",
		.fourcc       = V4L2_PIX_FMT_SGRBG10,
		.depth        = 8,
		.planes_cnt   = 1,
	},
	{
		.name         = "RAW Bayer RGGB 10bit",
		.fourcc       = V4L2_PIX_FMT_SGRBG10,
		.depth        = 8,
		.planes_cnt   = 1,
	},
	{
		.name         = "RAW Bayer BGGR 12bit",
		.fourcc       = V4L2_PIX_FMT_SBGGR12,
		.depth        = 8,
		.planes_cnt   = 1,
	},
	{
		.name         = "RAW Bayer GBRG 12bit",
		.fourcc       = V4L2_PIX_FMT_SGBRG12,
		.depth        = 8,
		.planes_cnt   = 1,
	},
	{
		.name         = "RAW Bayer GRBG 12bit",
		.fourcc       = V4L2_PIX_FMT_SGRBG12,
		.depth        = 8,
		.planes_cnt   = 1,
	},
	{
		.name         = "RAW Bayer RGGB 12bit",
		.fourcc       = V4L2_PIX_FMT_SGRBG12,
		.depth        = 8,
		.planes_cnt   = 1,
	},
};

static enum v4l2_mbus_pixelcode try_yuv422_bus[] = {
	V4L2_MBUS_FMT_UYVY10_20X1,
	V4L2_MBUS_FMT_UYVY8_16X1,

	V4L2_MBUS_FMT_YUYV10_2X10,
	V4L2_MBUS_FMT_YVYU10_2X10,
	V4L2_MBUS_FMT_UYVY8_2X8,
	V4L2_MBUS_FMT_VYUY8_2X8,
	V4L2_MBUS_FMT_YUYV8_2X8,
	V4L2_MBUS_FMT_YVYU8_2X8,

	V4L2_MBUS_FMT_YUYV10_1X20,
	V4L2_MBUS_FMT_YVYU10_1X20,
	V4L2_MBUS_FMT_UYVY8_1X16,
	V4L2_MBUS_FMT_VYUY8_1X16,
	V4L2_MBUS_FMT_YUYV8_1X16,
	V4L2_MBUS_FMT_YVYU8_1X16,

	V4L2_MBUS_FMT_YUV8_1X24,
};

#define N_TRY_YUV422 ARRAY_SIZE(try_yuv422_bus)

static enum v4l2_mbus_pixelcode try_yuv420_bus[] = {
	V4L2_MBUS_FMT_YY10_UYVY10_15X1,
	V4L2_MBUS_FMT_YY8_UYVY8_12X1,
};

#define N_TRY_YUV420 ARRAY_SIZE(try_yuv420_bus)

static enum v4l2_mbus_pixelcode try_bayer_rgb_bus[] = {
	V4L2_MBUS_FMT_SBGGR12_12X1,
	V4L2_MBUS_FMT_SGBRG12_12X1,
	V4L2_MBUS_FMT_SGRBG12_12X1,
	V4L2_MBUS_FMT_SRGGB12_12X1,
	V4L2_MBUS_FMT_SBGGR12_1X12,
	V4L2_MBUS_FMT_SGBRG12_1X12,
	V4L2_MBUS_FMT_SGRBG12_1X12,
	V4L2_MBUS_FMT_SRGGB12_1X12,
	V4L2_MBUS_FMT_SBGGR10_10X1,
	V4L2_MBUS_FMT_SGBRG10_10X1,
	V4L2_MBUS_FMT_SGRBG10_10X1,
	V4L2_MBUS_FMT_SRGGB10_10X1,
	V4L2_MBUS_FMT_SBGGR10_1X10,
	V4L2_MBUS_FMT_SGBRG10_1X10,
	V4L2_MBUS_FMT_SGRBG10_1X10,
	V4L2_MBUS_FMT_SRGGB10_1X10,
	V4L2_MBUS_FMT_SBGGR10_DPCM8_1X8,
	V4L2_MBUS_FMT_SGBRG10_DPCM8_1X8,
	V4L2_MBUS_FMT_SGRBG10_DPCM8_1X8,
	V4L2_MBUS_FMT_SRGGB10_DPCM8_1X8,
	V4L2_MBUS_FMT_SBGGR10_2X8_PADHI_BE,
	V4L2_MBUS_FMT_SBGGR10_2X8_PADHI_LE,
	V4L2_MBUS_FMT_SBGGR10_2X8_PADLO_BE,
	V4L2_MBUS_FMT_SBGGR10_2X8_PADLO_LE,
	V4L2_MBUS_FMT_SBGGR8_8X1,
	V4L2_MBUS_FMT_SGBRG8_8X1,
	V4L2_MBUS_FMT_SGRBG8_8X1,
	V4L2_MBUS_FMT_SRGGB8_8X1,
	V4L2_MBUS_FMT_SBGGR8_1X8,
	V4L2_MBUS_FMT_SGBRG8_1X8,
	V4L2_MBUS_FMT_SGRBG8_1X8,
	V4L2_MBUS_FMT_SRGGB8_1X8,
};

#define N_TRY_BAYER ARRAY_SIZE(try_bayer_rgb_bus)

static enum v4l2_mbus_pixelcode try_rgb565_bus[] = {
	V4L2_MBUS_FMT_RGB565_16X1,
	V4L2_MBUS_FMT_BGR565_2X8_BE,
	V4L2_MBUS_FMT_BGR565_2X8_LE,
	V4L2_MBUS_FMT_RGB565_2X8_BE,
	V4L2_MBUS_FMT_RGB565_2X8_LE,
};

#define N_TRY_RGB565 ARRAY_SIZE(try_rgb565_bus)

static enum v4l2_mbus_pixelcode try_rgb888_bus[] = {
	V4L2_MBUS_FMT_RGB888_24X1,
};

#define N_TRY_RGB888 ARRAY_SIZE(try_rgb888_bus)
/*
static int V4L2_AF_STATUS_TBL[] =
{
	V4L2_AUTO_FOCUS_STATUS_IDLE,  //AUTO_FOCUS_STATUS_IDLE
	V4L2_AUTO_FOCUS_STATUS_BUSY,  //AUTO_FOCUS_STATUS_BUSY
	V4L2_AUTO_FOCUS_STATUS_REACHED,  //AUTO_FOCUS_STATUS_REACHED
	V4L2_AUTO_FOCUS_STATUS_REACHED,  //AUTO_FOCUS_STATUS_APPROCH
	V4L2_AUTO_FOCUS_STATUS_BUSY,  //AUTO_FOCUS_STATUS_REFOCUS
	V4L2_AUTO_FOCUS_STATUS_REACHED,  //AUTO_FOCUS_STATUS_FINDED
	V4L2_AUTO_FOCUS_STATUS_FAILED,  //AUTO_FOCUS_STATUS_FAILED
};
*/
static int isp_resource_request(struct vfe_dev *dev)
{
	unsigned int isp_used_flag = 0,i;
	void *pa_base,*va_base,*dma_base;
	int ret;

	//requeset for isp table and statistic buffer
	for(i=0; i < dev->dev_qty; i++) {
		if(dev->ccm_cfg[i]->is_isp_used && dev->ccm_cfg[i]->is_bayer_raw) {
			dev->isp_lut_tbl_buf_mm[i].size = ISP_LINEAR_LUT_LENS_GAMMA_MEM_SIZE;
			ret = os_mem_alloc(&dev->isp_lut_tbl_buf_mm[i]);
			if(!ret) {
				pa_base = dev->isp_lut_tbl_buf_mm[i].phy_addr;
				va_base = dev->isp_lut_tbl_buf_mm[i].vir_addr;
				dma_base = dev->isp_lut_tbl_buf_mm[i].dma_addr;
				dev->isp_tbl_addr[i].isp_def_lut_tbl_paddr = (void*)(pa_base + ISP_LUT_MEM_OFS);
				dev->isp_tbl_addr[i].isp_def_lut_tbl_dma_addr = (void*)(dma_base + ISP_LUT_MEM_OFS);
				dev->isp_tbl_addr[i].isp_def_lut_tbl_vaddr = (void*)(va_base + ISP_LUT_MEM_OFS);
				dev->isp_tbl_addr[i].isp_lsc_tbl_paddr = (void*)(pa_base + ISP_LENS_MEM_OFS);
				dev->isp_tbl_addr[i].isp_lsc_tbl_dma_addr = (void*)(dma_base + ISP_LENS_MEM_OFS);
				dev->isp_tbl_addr[i].isp_lsc_tbl_vaddr = (void*)(va_base + ISP_LENS_MEM_OFS);
				dev->isp_tbl_addr[i].isp_gamma_tbl_paddr = (void*)(pa_base + ISP_GAMMA_MEM_OFS);
				dev->isp_tbl_addr[i].isp_gamma_tbl_dma_addr = (void*)(dma_base + ISP_GAMMA_MEM_OFS);
				dev->isp_tbl_addr[i].isp_gamma_tbl_vaddr = (void*)(va_base + ISP_GAMMA_MEM_OFS);
				
				dev->isp_tbl_addr[i].isp_linear_tbl_paddr = (void*)(pa_base + ISP_LINEAR_MEM_OFS);
				dev->isp_tbl_addr[i].isp_linear_tbl_dma_addr = (void*)(dma_base + ISP_LINEAR_MEM_OFS);
				dev->isp_tbl_addr[i].isp_linear_tbl_vaddr = (void*)(va_base + ISP_LINEAR_MEM_OFS);
				vfe_dbg(0,"isp_def_lut_tbl_vaddr[%d] = %p\n",i,dev->isp_tbl_addr[i].isp_def_lut_tbl_vaddr);
				vfe_dbg(0,"isp_lsc_tbl_vaddr[%d] = %p\n",i,dev->isp_tbl_addr[i].isp_lsc_tbl_vaddr);
				vfe_dbg(0,"isp_gamma_tbl_vaddr[%d] = %p\n",i,dev->isp_tbl_addr[i].isp_gamma_tbl_vaddr);
			} else {
				vfe_err("isp lut_lens_gamma table request pa failed!\n");
				return -ENOMEM;
			}
		}

		if(dev->ccm_cfg[i]->is_isp_used && dev->ccm_cfg[i]->is_bayer_raw) {
			dev->isp_drc_tbl_buf_mm[i].size = ISP_DRC_DISC_MEM_SIZE;
			ret = os_mem_alloc(&dev->isp_drc_tbl_buf_mm[i]);
			if(!ret) {
				pa_base = dev->isp_drc_tbl_buf_mm[i].phy_addr;
				va_base = dev->isp_drc_tbl_buf_mm[i].vir_addr;
				dma_base = dev->isp_drc_tbl_buf_mm[i].dma_addr;

				dev->isp_tbl_addr[i].isp_drc_tbl_paddr = (void*)(pa_base + ISP_DRC_MEM_OFS);
				dev->isp_tbl_addr[i].isp_drc_tbl_dma_addr = (void*)(dma_base + ISP_DRC_MEM_OFS);
				dev->isp_tbl_addr[i].isp_drc_tbl_vaddr = (void*)(va_base + ISP_DRC_MEM_OFS);
				
				dev->isp_tbl_addr[i].isp_disc_tbl_paddr = (void*)(pa_base + ISP_DISC_MEM_OFS);
				dev->isp_tbl_addr[i].isp_disc_tbl_dma_addr = (void*)(dma_base + ISP_DISC_MEM_OFS);
				dev->isp_tbl_addr[i].isp_disc_tbl_vaddr = (void*)(va_base + ISP_DISC_MEM_OFS);

				vfe_dbg(0,"isp_drc_tbl_vaddr[%d] = %p\n",i,dev->isp_tbl_addr[i].isp_drc_tbl_vaddr);
			} else {
				vfe_err("isp drc table request pa failed!\n");
				return -ENOMEM;
			}
		}
	}

	for(i=0; i < dev->dev_qty; i++) {
		if(dev->ccm_cfg[i]->is_isp_used && dev->ccm_cfg[i]->is_bayer_raw) {
			isp_used_flag = 1;
			break;
		}
	}

	if(isp_used_flag) {
		for(i=0; i < MAX_ISP_STAT_BUF; i++) {
			dev->isp_stat_buf_mm[i].size = ISP_STAT_TOTAL_SIZE;
			ret = os_mem_alloc(&dev->isp_stat_buf_mm[i]);
			if(!ret) {
				pa_base = dev->isp_stat_buf_mm[i].phy_addr;
				va_base = dev->isp_stat_buf_mm[i].vir_addr;
				dma_base = dev->isp_stat_buf_mm[i].dma_addr;
				INIT_LIST_HEAD(&dev->isp_stat_bq.isp_stat[i].queue);
				dev->isp_stat_bq.isp_stat[i].id = i;
				dev->isp_stat_bq.isp_stat[i].paddr = (void*)(pa_base);
				dev->isp_stat_bq.isp_stat[i].dma_addr = (void*)(dma_base);
				dev->isp_stat_bq.isp_stat[i].isp_stat_buf.stat_buf = (void*)(va_base);
				dev->isp_stat_bq.isp_stat[i].isp_stat_buf.buf_size = ISP_STAT_TOTAL_SIZE;
				dev->isp_stat_bq.isp_stat[i].isp_stat_buf.buf_status = BUF_IDLE;
				vfe_dbg(0,"dev->isp_stat_bq.isp_stat[i].isp_stat_buf.stat_buf[%d] = %p\n",i,dev->isp_stat_bq.isp_stat[i].isp_stat_buf.stat_buf);
			} else {
				vfe_err("isp statistic buffer request pa failed!\n");
				return -ENOMEM;
			}
		}
	}
	return 0;
}
static int vfe_device_regulator_get(struct ccm_config  *ccm_cfg);
static int vfe_device_regulator_put(struct ccm_config  *ccm_cfg);
static int vfe_set_sensor_power_on(struct vfe_dev *dev);
static int vfe_set_sensor_power_off(struct vfe_dev *dev);

static void isp_resource_release(struct vfe_dev *dev)
{
	unsigned int isp_used_flag = 0,i;

	//release isp table and statistic buffer
	for(i=0; i < dev->dev_qty; i++) {
		if(dev->ccm_cfg[i]->is_isp_used && dev->ccm_cfg[i]->is_bayer_raw) {
			os_mem_free(&dev->isp_lut_tbl_buf_mm[i]);
			os_mem_free(&dev->isp_drc_tbl_buf_mm[i]);
		}
	}

	for(i=0; i < dev->dev_qty; i++) {
		if(dev->ccm_cfg[i]->is_isp_used && dev->ccm_cfg[i]->is_bayer_raw) {
			isp_used_flag = 1;
			break;
		}
	}

	if(isp_used_flag) {
		for(i=0; i < MAX_ISP_STAT_BUF; i++) {
			os_mem_free(&dev->isp_stat_buf_mm[i]);
		}
	}
}

static int vfe_clk_get(struct vfe_dev *dev)
{
#ifdef VFE_CLK
	int ret;
	//Get Core clk!
	if(VFE_CORE_CLK)
	{
		dev->clock.vfe_core_clk = os_clk_get(NULL, VFE_CORE_CLK);
		if(VFE_CORE_CLK_SRC){
			dev->clock.vfe_core_clk_src = os_clk_get(NULL, VFE_CORE_CLK_SRC);
			if(dev->clock.vfe_core_clk && dev->clock.vfe_core_clk_src)
			{
				ret=os_clk_set_parent(dev->clock.vfe_core_clk, dev->clock.vfe_core_clk_src);
				if (ret != 0) {
					vfe_err(" vfe core clock set parent failed \n");
					return -1;
				}
			}
		}
		os_clk_set_rate(dev->clock.vfe_core_clk, VFE_CORE_CLK_RATE);
		vfe_dbg(0,"vfe core clk = %ld\n",clk_get_rate(dev->clock.vfe_core_clk));
	}
	//Get Master clk!
	if(dev->vip_sel == 0) {
		if(VFE_MASTER_CLK0){
			dev->clock.vfe_master_clk = os_clk_get(NULL, VFE_MASTER_CLK0);
		}
	}
	else if(dev->vip_sel == 1)
	{
		if(VFE_MASTER_CLK1){
			dev->clock.vfe_master_clk = os_clk_get(NULL, VFE_MASTER_CLK1);
		}
	}
	if(VFE_MASTER_CLK_24M_SRC){
		dev->clock.vfe_master_clk_24M_src = os_clk_get(NULL, VFE_MASTER_CLK_24M_SRC);
	}
	if(VFE_MASTER_CLK_PLL_SRC){
		dev->clock.vfe_master_clk_pll_src = os_clk_get(NULL, VFE_MASTER_CLK_PLL_SRC);
	}

	if(!dev->clock.vfe_master_clk || !dev->clock.vfe_core_clk)
	{
		vfe_err("vfe core clk or vfe master clk is NULL!\n");
		ret = -1;
	}
	//Get MIPI clk!
	if(VFE_MIPI_DPHY_CLK){
		dev->clock.vfe_dphy_clk = os_clk_get(NULL, VFE_MIPI_DPHY_CLK);
		if(VFE_MIPI_DPHY_CLK_SRC) {
			dev->clock.vfe_dphy_clk_src = os_clk_get(NULL, VFE_MIPI_DPHY_CLK_SRC);
		}
	}
	if(VFE_MIPI_CSI_CLK){
		dev->clock.vfe_mipi_csi_clk = os_clk_get(NULL, VFE_MIPI_CSI_CLK);
	}
#ifdef VFE_MISC_CLK
	dev->clock.vfe_misc_clk = os_clk_get(NULL, VFE_MISC_CLK);
#endif

#endif
  return 0;
}

static int vfe_dphy_clk_set(struct vfe_dev *dev, unsigned long freq)
{
#ifdef VFE_CLK
	if(VFE_MIPI_DPHY_CLK_SRC)
	{
		if(dev->clock.vfe_dphy_clk && dev->clock.vfe_dphy_clk_src) {
			if(os_clk_set_parent(dev->clock.vfe_dphy_clk, dev->clock.vfe_dphy_clk_src)) {
				vfe_err("set vfe dphy clock source failed \n");
				return -1;
			}
		} else {
			vfe_err("vfe dphy clock is null\n");
			return -1;
		}
	}
	if(VFE_MIPI_DPHY_CLK){
		if(dev->clock.vfe_dphy_clk) {
			if(os_clk_set_rate(dev->clock.vfe_dphy_clk, freq)) {
				vfe_err("set vip%d dphy clock error\n",dev->vip_sel);
				return -1;
			}
		} else {
			vfe_err("vfe master clock is null\n");
			return -1;
		}
	}
#endif
  return 0;
}

static int vfe_clk_enable(struct vfe_dev *dev)
{
	int ret = 0;
#ifdef VFE_CLK
	if(dev->clock.vfe_core_clk) {
		if(os_clk_prepare_enable(dev->clock.vfe_core_clk)){
			vfe_err("vfe core clock enable error\n");
			ret = -1;
		}

	} else {
		vfe_err("vfe core clock is null\n");
		ret = -1;
	}
	//
	if(dev->clock.vfe_dphy_clk) {
		if(os_clk_prepare_enable(dev->clock.vfe_dphy_clk)){
			vfe_err("vfe dphy clock enable error\n");
			ret = -1;
		}
	} else {
		vfe_dbg(0,"vfe dphy clock is null\n");
		ret = -1;
	}

	if(dev->clock.vfe_mipi_csi_clk) {
		if(os_clk_prepare_enable(dev->clock.vfe_mipi_csi_clk)){
			vfe_err("vfe mipi csi clock enable error\n");
			ret = -1;
		}
	} else {
		vfe_dbg(0,"vfe mipi csi clock  is null\n");
		ret = -1;
	}
	if(dev->clock.vfe_misc_clk) {
		if(os_clk_prepare_enable(dev->clock.vfe_misc_clk)){
			vfe_err("vfe misc clock enable error\n");
			ret = -1;
		}
	} else {
		vfe_dbg(0,"vfe misc clock is null\n");
		ret = -1;
	}
#endif
	return ret;
}

static void vfe_clk_disable(struct vfe_dev *dev)
{
#ifdef VFE_CLK
	if(dev->clock.vfe_core_clk)
		os_clk_disable_unprepare(dev->clock.vfe_core_clk);
	else
		vfe_dbg(0,"vfe core clock is null\n");

	if(dev->clock.vfe_dphy_clk)
		os_clk_disable_unprepare(dev->clock.vfe_dphy_clk);
	else
		vfe_dbg(0,"vfe dphy clock is null\n");

	if(dev->clock.vfe_misc_clk)
		os_clk_disable_unprepare(dev->clock.vfe_misc_clk);
	else
		vfe_dbg(0,"vfe dphy clock is null\n");

	if(dev->clock.vfe_mipi_csi_clk)
		os_clk_disable_unprepare(dev->clock.vfe_mipi_csi_clk);
	else
		vfe_dbg(0,"vfe mipi csi clock is null\n");
#endif
}

static void vfe_clk_release(struct vfe_dev *dev)
{
#ifdef VFE_CLK
	if(dev->clock.vfe_core_clk)
		os_clk_put(dev->clock.vfe_core_clk);
	else
		vfe_err("vfe core clock is null\n");

	if(dev->clock.vfe_master_clk)
		os_clk_put(dev->clock.vfe_master_clk);
	else
		vfe_err("vip%d master clock is null\n",dev->vip_sel);

	if(dev->clock.vfe_dphy_clk)
		os_clk_put(dev->clock.vfe_dphy_clk);
	else
		vfe_err("vfe dphy clock is null\n");
	if(dev->clock.vfe_mipi_csi_clk)
		os_clk_put(dev->clock.vfe_mipi_csi_clk);
	else
		vfe_warn("vfe mipi csi clock is null\n");

	if(dev->clock.vfe_core_clk_src)
		os_clk_put(dev->clock.vfe_core_clk_src);
	else
		vfe_warn("vfe core clock source is null\n");

	if(dev->clock.vfe_master_clk_24M_src)
		os_clk_put(dev->clock.vfe_master_clk_24M_src);
	else
		vfe_err("vfe master clock 24M source is null\n");

	if(dev->clock.vfe_master_clk_pll_src)
		os_clk_put(dev->clock.vfe_master_clk_pll_src);
	else
		vfe_warn("vfe master clock pll source is null\n");

	if(dev->clock.vfe_dphy_clk_src)
		os_clk_put(dev->clock.vfe_dphy_clk_src);
	else
		vfe_warn("vfe dphy clock source is null\n");
#endif
}

static void vfe_reset_enable(struct vfe_dev *dev)
{
  os_clk_reset_assert(dev->clock.vfe_core_clk);
//  os_clk_reset_assert(dev->clock.vfe_ahb_clk);
}

static void vfe_reset_disable(struct vfe_dev *dev)
{
  os_clk_reset_deassert(dev->clock.vfe_core_clk);
//  os_clk_reset_deassert(dev->clock.vfe_ahb_clk);
}

static int inline vfe_is_generating(struct vfe_dev *dev)
{
	return test_bit(0, &dev->generating);
}

static void inline vfe_start_generating(struct vfe_dev *dev)
{
	set_bit(0, &dev->generating);
	return;
}

static void inline vfe_stop_generating(struct vfe_dev *dev)
{
	dev->first_flag = 0;
	clear_bit(0, &dev->generating);
	return;
}

static int vfe_is_opened(struct vfe_dev *dev)
{
	 int ret;
	 mutex_lock(&dev->opened_lock);
	 ret = test_bit(0, &dev->opened);
	 mutex_unlock(&dev->opened_lock);
	 return ret;
}

static void vfe_start_opened(struct vfe_dev *dev)
{
	 mutex_lock(&dev->opened_lock);
	 set_bit(0, &dev->opened);
	 mutex_unlock(&dev->opened_lock);
}

static void vfe_stop_opened(struct vfe_dev *dev)
{
	 mutex_lock(&dev->opened_lock);
	 clear_bit(0, &dev->opened);
	 mutex_unlock(&dev->opened_lock);
}

static void update_ccm_info(struct vfe_dev *dev , struct ccm_config *ccm_cfg)
{
	dev->sd                  = ccm_cfg->sd;
	dev->sd_act                = ccm_cfg->sd_act;
	dev->ctrl_para.vflip         = ccm_cfg->vflip;
	dev->ctrl_para.hflip         = ccm_cfg->hflip;
	dev->ctrl_para.vflip_thumb   = ccm_cfg->vflip_thumb;
	dev->ctrl_para.hflip_thumb   = ccm_cfg->hflip_thumb;
	dev->is_isp_used             = ccm_cfg->is_isp_used;
	dev->is_bayer_raw            = ccm_cfg->is_bayer_raw;
	dev->power                   = &ccm_cfg->power;
	dev->gpio                    = &ccm_cfg->gpio;
	dev->flash_used              = ccm_cfg->flash_used;
	dev->flash_type              = ccm_cfg->flash_type;

	/* print change */
	vfe_dbg(0,"ccm_cfg pt = %p\n",ccm_cfg);
	vfe_dbg(0,"ccm_cfg->sd = %p\n",ccm_cfg->sd);
	vfe_dbg(0,"module vflip = %d hflip = %d\n",dev->ctrl_para.vflip,dev->ctrl_para.hflip);
	vfe_dbg(0,"module vflip_thumb = %d hflip_thumb = %d\n",dev->ctrl_para.vflip_thumb,dev->ctrl_para.hflip_thumb);
	vfe_dbg(0,"module is_isp_used = %d is_bayer_raw= %d\n",dev->is_isp_used,dev->is_bayer_raw);
}

static void update_isp_setting(struct vfe_dev *dev)
{
	dev->isp_3a_result_pt = &dev->isp_3a_result[dev->input];
	dev->isp_gen_set_pt = &dev->isp_gen_set[dev->input];
	dev->isp_gen_set_pt->module_cfg.isp_platform_id = dev->platform_id;
	if(dev->is_bayer_raw) {
		mutex_init(&dev->isp_3a_result_mutex);
		dev->isp_gen_set_pt->module_cfg.lut_src0_table = dev->isp_tbl_addr[dev->input].isp_def_lut_tbl_vaddr;
		dev->isp_gen_set_pt->module_cfg.gamma_table = dev->isp_tbl_addr[dev->input].isp_gamma_tbl_vaddr;
		dev->isp_gen_set_pt->module_cfg.lens_table = dev->isp_tbl_addr[dev->input].isp_lsc_tbl_vaddr;
		dev->isp_gen_set_pt->module_cfg.linear_table= dev->isp_tbl_addr[dev->input].isp_linear_tbl_vaddr;
		dev->isp_gen_set_pt->module_cfg.disc_table = dev->isp_tbl_addr[dev->input].isp_disc_tbl_vaddr;
		bsp_isp_update_lut_lens_gamma_table(&dev->isp_tbl_addr[dev->input]);

		dev->isp_gen_set_pt->module_cfg.drc_table = dev->isp_tbl_addr[dev->input].isp_drc_tbl_vaddr;
		bsp_isp_update_drc_table(&dev->isp_tbl_addr[dev->input]);
	}
}

static int get_mbus_config(struct vfe_dev *dev, struct v4l2_mbus_config *mbus_config)
{
	int ret;

	ret = v4l2_subdev_call(dev->sd,video,g_mbus_config,mbus_config);
	if (ret < 0) {
		vfe_err("v4l2 sub device g_mbus_config error!\n");
		return -EFAULT;
	}

	return 0;
};
//static int isp_addr_curr = 0;
//static int isp_addr_pst = 0;
static inline void vfe_set_addr(struct vfe_dev *dev,struct vfe_buffer *buffer)
{
	struct vfe_buffer *buf = buffer;
	dma_addr_t addr_org;
	struct videobuf_buffer *vb_buf = &buf->vb;
	struct videobuf_queue *vq = &dev->vb_vidq;
	void * vaddr;
	
	if(vb_buf == NULL || vb_buf->priv == NULL)
	{
		vfe_err("videobuf_buffer->priv is NULL!\n");
		return;
	}

	//vfe_dbg(3,"buf ptr=%p\n",buf);
	addr_org = videobuf_to_dma_contig(vb_buf) - CPU_DRAM_PADDR_ORG + HW_DMA_OFFSET;
	//isp_addr_curr = vfe_reg_readl((volatile void __iomem*)(0xf1cb8210));
	//if(isp_addr_pst != isp_addr_curr)
	//{
	//	vfe_warn("isp_addr_pst = %d, isp_addr_curr = %d.......\n", isp_addr_pst, isp_addr_curr);
	//}
	//isp_addr_pst = addr_org /4;
	if(dev->is_isp_used) {
		vaddr = videobuf_queue_to_vaddr(vq, &buf->vb);
#ifdef CONFIG_ARCH_SUN8IW8P1
	if(ALIGN_16B(buf->vb.height) != buf->vb.height){
		memset((unsigned char*)vaddr + ALIGN_16B(buf->vb.width)*ALIGN_16B(buf->vb.height)*3/2 - 16*ALIGN_16B(buf->vb.width), 128, 16*ALIGN_16B(buf->vb.width));
		memset((unsigned char*)vaddr + buf->vb.size - 16*ALIGN_16B(buf->vb.width), 128, 16*ALIGN_16B(buf->vb.width));
		flush_dcache_all();
	}
#endif
		bsp_isp_set_output_addr(addr_org);
		dev->isp_gen_set_pt->output_addr = addr_org;
	} else {
		bsp_csi_set_addr(dev->vip_sel, addr_org);
	}
	buf->image_quality = dev->isp_3a_result_pt->image_quality.dwval;
	vfe_dbg(3,"csi_buf_addr_orginal=%x\n", addr_org);
}

static unsigned int common_af_status_to_v4l2(enum auto_focus_status af_status)
{
	switch(af_status) {
		case AUTO_FOCUS_STATUS_IDLE:
			return V4L2_AUTO_FOCUS_STATUS_IDLE;
		case AUTO_FOCUS_STATUS_BUSY:
			return V4L2_AUTO_FOCUS_STATUS_BUSY;
		case AUTO_FOCUS_STATUS_REACHED:
			return V4L2_AUTO_FOCUS_STATUS_REACHED;
		case AUTO_FOCUS_STATUS_APPROCH:
			return V4L2_AUTO_FOCUS_STATUS_BUSY;
		case AUTO_FOCUS_STATUS_REFOCUS:
			return V4L2_AUTO_FOCUS_STATUS_BUSY;
		case AUTO_FOCUS_STATUS_FINDED:
			return V4L2_AUTO_FOCUS_STATUS_BUSY;
		case AUTO_FOCUS_STATUS_FAILED:
			return V4L2_AUTO_FOCUS_STATUS_FAILED;
		default:
		      return V4L2_AUTO_FOCUS_STATUS_IDLE;
	}
}
static void vfe_dump_csi_regs(struct vfe_dev *dev)
{
	int i = 0;
	if(vfe_dump & DUMP_CSI)
	{
		if(5 == frame_cnt % 10)
		{
			printk("Vfe dump CSI regs :\n");
			for(i = 0; i < 0xb0; i = i + 4)
			{
				if(i % 0x10 == 0)
					printk("0x%08x:    ", i);
				printk("0x%08x, ", vfe_reg_readl(dev->regs.csi_regs + i));
				if(i % 0x10 == 0xc)
					printk("\n");
			}
		}
	}
}
static void vfe_dump_isp_regs(struct vfe_dev *dev)
{
	int i = 0;
	if(vfe_dump & DUMP_ISP)
	{
		if(9 == (frame_cnt % 10))
		{
			printk("Vfe dump ISP regs :\n");
			for(i = 0; i < 0x40; i = i + 4)
			{
				if(i % 0x10 == 0)
					printk("0x%08x:  ", i);
				printk("0x%08x, ", vfe_reg_readl(dev->regs.isp_regs + i));
				if(i % 0x10 == 0xc)
					printk("\n");
			}
			for(i = 0x40; i < 0x240; i = i + 4)
			{
				if(i % 0x10 == 0)
					printk("0x%08x:  ", i);
				printk("0x%08x, ", vfe_reg_readl(dev->regs.isp_load_regs + i));
				if(i % 0x10 == 0xc)
					printk("\n");
			}
		}
	}
}

static void vfe_init_isp_log(struct vfe_dev *dev)
{
	if(isp_log == 1)
	{
		fp_dbg = cfg_open_file(LogFileName);
		dev->isp_gen_set[0].enable_log = 1;
		dev->isp_gen_set[1].enable_log = 1;
		if(IS_ERR(fp_dbg)){
			vfe_err("open log.txt error.");
		}else{
			//if(cfg_write_file(fp_dbg, "0123456789abcdef\n", 16) < 0)
			//{
			//	vfe_err("/system/etc/hawkview/log.txt write test failed.");
			//}
			;
		}
	}else{
		dev->isp_gen_set[0].enable_log = 0;
		dev->isp_gen_set[1].enable_log = 0;
	}

}
static void vfe_exit_isp_log(struct vfe_dev *dev)
{
	if(isp_log == 1)
	{
		cfg_close_file(fp_dbg);
	}
}
static void vfe_dump_isp_log(struct vfe_dev *dev)
{

	//dump isp log.
	if(isp_log == 1 && (frame_cnt % 4 == 0))
	{
		if(cfg_write_file(fp_dbg, dev->isp_gen_set_pt->stat.hist_buf, ISP_STAT_HIST_MEM_SIZE) < 0)
		{
			vfe_err("dump isp hist faild.");
			return;
		}
		if(cfg_write_file(fp_dbg, dev->isp_gen_set_pt->stat.ae_buf, ISP_STAT_AE_MEM_SIZE) < 0)
		{
			vfe_err("dump isp ae faild.");
		}
		if(cfg_write_file(fp_dbg, (char *)dev->isp_gen_set_pt->awb_buf, 3*ISP_STAT_AWB_WIN_MEM_SIZE) < 0)
		{
			vfe_err("dump awb log faild.");
		}
		
		//if(cfg_write_file(fp_dbg, dev->isp_gen_set_pt->stat.af_buf, ISP_STAT_AF_MEM_SIZE) < 0)
		//{
		//	vfe_err("dump isp log faild.");
		//}
		///if(cfg_write_file(fp_dbg, "0123456789abcdef\n", 16) < 0)
		///{
		//	vfe_err("/system/etc/hawkview/log.txt write test failed.");
		//}
	}
}

static void isp_isr_bh_handle(struct work_struct *work)
{
	struct actuator_ctrl_word_t  vcm_ctrl;
	struct vfe_dev *dev = container_of(work,struct vfe_dev,isp_isr_bh_task);

	FUNCTION_LOG;
	vfe_dump_isp_regs(dev);
	if(dev->is_bayer_raw) {
		mutex_lock(&dev->isp_3a_result_mutex);
		if(1 == isp_reparse_flag)
		{
			vfe_print("ISP reparse ini file!\n");
			if(read_ini_info(dev,dev->input, ReparseIniPath))
			{
				vfe_warn("ISP reparse ini fail, please check isp config!\n");
				goto ISP_REPARSE_END;
			}
			isp_param_init(dev->isp_gen_set_pt);
			isp_config_init(dev->isp_gen_set_pt);
			isp_module_init(dev->isp_gen_set_pt, dev->isp_3a_result_pt);
ISP_REPARSE_END:
			isp_reparse_flag = 0;
		}
		if(2 == isp_reparse_flag)
		{
			vfe_reg_set(IO_ADDRESS(ISP_REGS_BASE+0x10), (1 << 20));
		}
		if(3 == isp_reparse_flag)
		{
			vfe_reg_clr_set(IO_ADDRESS(ISP_REGS_BASE+0x10), (0xF << 16), (1 << 16));
			vfe_reg_set(IO_ADDRESS(ISP_REGS_BASE+0x10), (1 << 20));
		}
		if(4 == isp_reparse_flag)
		{
			//vfe_reg_clr_set(IO_ADDRESS(ISP_REGS_BASE+0x10), (0xF << 16), (1 << 16));
			vfe_reg_clr(IO_ADDRESS(ISP_REGS_BASE+0x10), (1 << 20));
			vfe_reg_clr(IO_ADDRESS(ISP_REGS_BASE+0x10), (0xF << 16));
		}
		vfe_dump_isp_log(dev);
		isp_isr(dev->isp_gen_set_pt,dev->isp_3a_result_pt);
		if((dev->ctrl_para.prev_focus_pos != dev->isp_3a_result_pt->real_vcm_pos  ||
				dev->isp_gen_set_pt->isp_ini_cfg.isp_test_settings.isp_test_mode != 0 ||
				dev->isp_gen_set_pt->isp_ini_cfg.isp_test_settings.af_en == 0) && dev->sd_act)
		{
			vcm_ctrl.code =  dev->isp_3a_result_pt->real_vcm_pos;
			vcm_ctrl.sr = 0x0;
			if(v4l2_subdev_call(dev->sd_act,core,ioctl,ACT_SET_CODE,&vcm_ctrl))
			{
				vfe_warn("set vcm error!\n");
			} else {
				dev->ctrl_para.prev_focus_pos = dev->isp_3a_result_pt->real_vcm_pos;
			}
		}

		mutex_unlock(&dev->isp_3a_result_mutex);
	} else {
		isp_isr(dev->isp_gen_set_pt,NULL);
	}

	FUNCTION_LOG;
}

int set_sensor_shutter(struct vfe_dev *dev, int shutter)
{
	struct v4l2_control ctrl;
	if(shutter <= 0)
	{
		return -EINVAL;
	}
	ctrl.id = V4L2_CID_EXPOSURE;
	ctrl.value = shutter;
	if(v4l2_subdev_call(dev->sd,core,s_ctrl,&ctrl) != 0)
	{
		vfe_err("set sensor exposure line error!\n");
		return -1;
	}
	else
	{
		dev->ctrl_para.prev_exp_line = shutter;
		return 0;
	}
}

int set_sensor_gain(struct vfe_dev *dev, int gain)
{
	struct v4l2_control ctrl;
	if(gain < 16)
	{
		return -EINVAL;
	}
	ctrl.id = V4L2_CID_GAIN;
	ctrl.value = gain;
	if(v4l2_subdev_call(dev->sd,core,s_ctrl,&ctrl) != 0)
	{
		vfe_err("set sensor gain error!\n");
		return -1;
	} else {
		dev->ctrl_para.prev_ana_gain = gain;
		return 0;
	}
}
int set_sensor_shutter_and_gain(struct vfe_dev *dev)
{
	struct sensor_exp_gain exp_gain;
	exp_gain.exp_val = dev->isp_3a_result_pt->exp_line_num;
	exp_gain.gain_val = dev->isp_3a_result_pt->exp_analog_gain;
	if(exp_gain.gain_val < 16 || exp_gain.exp_val <= 0)
	{
		return -EINVAL;
	}
	if(v4l2_subdev_call(dev->sd,core,ioctl,ISP_SET_EXP_GAIN,&exp_gain) != 0)
	{
		vfe_warn("set ISP_SET_EXP_GAIN error, set V4L2_CID_EXPOSURE!\n");
		return -1;
	}
	else
	{
		dev->ctrl_para.prev_exp_line = exp_gain.exp_val;
		dev->ctrl_para.prev_ana_gain = exp_gain.gain_val;
		return 0;
	}
}

static int isp_s_ctrl_torch_open(struct vfe_dev *dev)
{
	if(dev->isp_gen_set_pt->exp_settings.flash_mode == FLASH_MODE_OFF)
	{
		return 0;
	}
	if(((dev->isp_gen_set_pt->exp_settings.tbl_cnt > (dev->isp_gen_set_pt->exp_settings.tbl_max_ind - 25)) ||
			dev->isp_gen_set_pt->exp_settings.flash_mode == FLASH_MODE_ON))
	{
		vfe_dbg(0,"open flash when nigth mode\n");
		io_set_flash_ctrl(dev->sd, SW_CTRL_TORCH_ON, dev->fl_dev_info);
		touch_flash_flag = 1;
	}
	return 0;
}
static int isp_s_ctrl_torch_close(struct vfe_dev *dev)
{
	if(dev->isp_gen_set_pt->exp_settings.flash_mode == FLASH_MODE_OFF)
	{
		return 0;
	}
	if(touch_flash_flag == 1)
	{
		vfe_dbg(0,"close flash when nigth mode\n");
		io_set_flash_ctrl(dev->sd, SW_CTRL_FLASH_OFF, dev->fl_dev_info);
		touch_flash_flag = 0;
	}
	return 0;
}
static int isp_streamoff_torch_and_flash_close(struct vfe_dev *dev)
{
	if(dev->isp_gen_set_pt->exp_settings.flash_mode == FLASH_MODE_OFF)
	{
		return 0;
	}
	if(touch_flash_flag == 1 || dev->isp_gen_set_pt->exp_settings.flash_open == 1)
	{
		vfe_dbg(0,"close flash when nigth mode\n");
		io_set_flash_ctrl(dev->sd, SW_CTRL_FLASH_OFF, dev->fl_dev_info);
		touch_flash_flag = 0;
	}
	return 0;
}
static int isp_set_capture_flash(struct vfe_dev *dev)
{
	if(dev->isp_gen_set_pt->exp_settings.flash_mode == FLASH_MODE_OFF)
	{
		return 0;
	}

	if(dev->isp_gen_set_pt->take_pic_start_cnt == 1)
	{
		if(dev->isp_gen_set_pt->exp_settings.tbl_cnt > (dev->isp_gen_set_pt->exp_settings.tbl_max_ind - 40) ||
				dev->isp_gen_set_pt->exp_settings.flash_mode == FLASH_MODE_ON)
		{
			vfe_dbg(0,"open torch when nigth mode\n");
			io_set_flash_ctrl(dev->sd, SW_CTRL_TORCH_ON, dev->fl_dev_info);
			dev->isp_gen_set_pt->exp_settings.flash_open = 1;
		}
	}

	if(dev->isp_gen_set_pt->exp_settings.flash_open == 1 && dev->isp_gen_set_pt->take_pic_start_cnt ==
					dev->isp_gen_set_pt->isp_ini_cfg.isp_tunning_settings.flash_delay_frame)
	{
		vfe_dbg(0,"open flash when nigth mode\n");
			dev->isp_gen_set_pt->exp_settings.exposure_lock = ISP_TRUE;
			ev_cumul = get_pre_ev_cumul(dev->isp_gen_set_pt,dev->isp_3a_result_pt);
			if(ev_cumul >= 100)
			{
				dev->isp_gen_set_pt->exp_settings.tbl_cnt = CLIP(dev->isp_gen_set_pt->exp_settings.expect_tbl_cnt,1,
						dev->isp_gen_set_pt->exp_settings.tbl_max_ind);
			}
			else if(ev_cumul >= dev->isp_gen_set_pt->isp_ini_cfg.isp_tunning_settings.flash_gain*100/256 && ev_cumul < 100)
			{
				dev->isp_gen_set_pt->exp_settings.tbl_cnt = CLIP(dev->isp_gen_set_pt->exp_settings.expect_tbl_cnt,1,
						dev->isp_gen_set_pt->exp_settings.tbl_max_ind);
			}
			else if(ev_cumul >= -25 && ev_cumul < dev->isp_gen_set_pt->isp_ini_cfg.isp_tunning_settings.flash_gain*100/256)
			{
				dev->isp_gen_set_pt->exp_settings.tbl_cnt = CLIP(dev->isp_gen_set_pt->exp_settings.expect_tbl_cnt +
					 ev_cumul*dev->isp_gen_set_pt->isp_ini_cfg.isp_tunning_settings.flash_gain/256,1,
						dev->isp_gen_set_pt->exp_settings.tbl_max_ind);
			}
			else
			{
				dev->isp_gen_set_pt->exp_settings.tbl_cnt = CLIP(dev->isp_gen_set_pt->exp_settings.expect_tbl_cnt +
						ev_cumul*dev->isp_gen_set_pt->isp_ini_cfg.isp_tunning_settings.flash_gain/256,1,
						dev->isp_gen_set_pt->exp_settings.tbl_max_ind);
			}
			config_sensor_next_exposure(dev->isp_gen_set_pt,dev->isp_3a_result_pt);
			io_set_flash_ctrl(dev->sd, SW_CTRL_FLASH_OFF, dev->fl_dev_info);
	}
	if(dev->isp_gen_set_pt->exp_settings.flash_open == 1 && dev->isp_gen_set_pt->take_pic_start_cnt ==
					dev->isp_gen_set_pt->isp_ini_cfg.isp_tunning_settings.flash_delay_frame + 1)
	{
		io_set_flash_ctrl(dev->sd, SW_CTRL_FLASH_ON, dev->fl_dev_info);
	}
	if(dev->isp_gen_set_pt->take_pic_start_cnt == 7 + dev->isp_gen_set_pt->isp_ini_cfg.isp_tunning_settings.flash_delay_frame)
	{
		vfe_dbg(0,"close flash when nigth mode\n");
		io_set_flash_ctrl(dev->sd, SW_CTRL_FLASH_OFF, dev->fl_dev_info);
		dev->isp_gen_set_pt->exp_settings.tbl_cnt = CLIP(dev->isp_gen_set_pt->exp_settings.expect_tbl_cnt,
			1,dev->isp_gen_set_pt->exp_settings.tbl_max_ind);
		dev->isp_gen_set_pt->exp_settings.exposure_lock = ISP_FALSE;
		dev->isp_gen_set_pt->exp_settings.flash_open = 0;
	}

	if(dev->isp_gen_set_pt->exp_settings.flash_open == 0 && touch_flash_flag == 1 &&
			(dev->isp_3a_result_pt->af_status == AUTO_FOCUS_STATUS_REACHED ||
			dev->isp_3a_result_pt->af_status == AUTO_FOCUS_STATUS_FAILED ||
			dev->isp_3a_result_pt->af_status == AUTO_FOCUS_STATUS_FINDED))
	{
		vfe_dbg(0,"close flash when touch nigth mode \n");
		io_set_flash_ctrl(dev->sd, SW_CTRL_FLASH_OFF, dev->fl_dev_info);
		touch_flash_flag = 0;
	}
	return 0;
}
static void isp_isr_set_sensor_handle(struct work_struct *work)
{
	struct vfe_dev *dev = container_of(work,struct vfe_dev,isp_isr_set_sensor_task);
	if(dev->is_bayer_raw)
	{
		mutex_lock(&dev->isp_3a_result_mutex);
#ifdef _FLASH_FUNC_
		isp_set_capture_flash(dev);
#endif
		if(dev->isp_gen_set_pt->isp_ini_cfg.isp_3a_settings.adaptive_frame_rate == 1 ||
				dev->isp_gen_set_pt->isp_ini_cfg.isp_3a_settings.force_frame_rate == 1||
				dev->isp_gen_set_pt->isp_ini_cfg.isp_3a_settings.high_quality_mode_en == 1)
		{
			vfe_dbg(0,"combinate shutter = %d, gain =%d \n",dev->isp_3a_result_pt->exp_line_num,
					dev->isp_3a_result_pt->exp_analog_gain);
			if(set_sensor_shutter_and_gain(dev) != 0)
			{
				set_sensor_shutter(dev,dev->isp_3a_result_pt->exp_line_num);
				set_sensor_gain(dev,dev->isp_3a_result_pt->exp_analog_gain);
			}
		}
		else
		{
			vfe_dbg(0,"separate shutter = %d, gain =%d \n",dev->isp_3a_result_pt->exp_line_num/16,
					dev->isp_3a_result_pt->exp_analog_gain);
			set_sensor_shutter(dev,dev->isp_3a_result_pt->exp_line_num);
			set_sensor_gain(dev,dev->isp_3a_result_pt->exp_analog_gain);
		}
		mutex_unlock(&dev->isp_3a_result_mutex);
	}
	return;
}

static void vfe_isp_stat_parse(struct isp_gen_settings * isp_gen)
{
	unsigned int buffer_addr = (unsigned int)isp_gen->stat.stat_buf_whole->stat_buf;
	isp_gen->stat.hist_buf = (void*) (buffer_addr);
	isp_gen->stat.ae_buf =  (void*) (buffer_addr + ISP_STAT_AE_MEM_OFS);
	isp_gen->stat.awb_buf= (void*) (buffer_addr + ISP_STAT_AWB_MEM_OFS);
	isp_gen->stat.af_buf = (void*) (buffer_addr + ISP_STAT_AF_MEM_OFS);
	isp_gen->stat.afs_buf = (void*) (buffer_addr + ISP_STAT_AFS_MEM_OFS);
	isp_gen->stat.awb_win_buf = (void*) (buffer_addr + ISP_STAT_AWB_WIN_MEM_OFS);
}

/*
 *  the interrupt routine
 */

static irqreturn_t vfe_isr(int irq, void *priv)
{
	int i;
	unsigned long flags;
	struct vfe_buffer *buf;
	struct vfe_dev *dev = (struct vfe_dev *)priv;
	struct vfe_dmaqueue *dma_q = &dev->vidq;
	struct csi_int_status status;
	struct vfe_isp_stat_buf_queue *isp_stat_bq = &dev->isp_stat_bq;
	struct vfe_isp_stat_buf *stat_buf_pt;
#ifdef CONFIG_ARCH_SUN8IW8P1		
	struct timespec timestamp;
#endif		

	FUNCTION_LOG;
	vfe_dbg(0,"vfe interrupt!!!\n");
	if(vfe_is_generating(dev) == 0)
	{
		bsp_csi_int_clear_status(dev->vip_sel, dev->cur_ch,CSI_INT_ALL);
		if(dev->is_isp_used)
			bsp_isp_clr_irq_status(ISP_IRQ_EN_ALL);
		return IRQ_HANDLED;
	}
	bsp_csi_int_get_status(dev->vip_sel, dev->cur_ch, &status);
	if( (status.capture_done==0) && (status.frame_done==0) && (status.vsync_trig==0) )
	{
		vfe_print("enter vfe int for nothing\n");
		bsp_csi_int_clear_status(dev->vip_sel, dev->cur_ch,CSI_INT_ALL);
		if(dev->is_isp_used)
			bsp_isp_clr_irq_status(ISP_IRQ_EN_ALL);
		return IRQ_HANDLED;
	}
	if(dev->is_isp_used && dev->is_bayer_raw)
	{
		//update_sensor_setting:
		if(status.vsync_trig)
		{
			if((dev->capture_mode == V4L2_MODE_VIDEO) || (dev->capture_mode == V4L2_MODE_PREVIEW))
			{
				vfe_dbg(3,"call set sensor task schedule! \n");
				schedule_work(&dev->isp_isr_set_sensor_task);
			}
			bsp_csi_int_clear_status(dev->vip_sel, dev->cur_ch,CSI_INT_VSYNC_TRIG);
			return IRQ_HANDLED;
		}
	}
	vfe_dump_csi_regs(dev);
	frame_cnt++;

	FUNCTION_LOG;
	//spin_lock(&dev->slock);
	spin_lock_irqsave(&dev->slock, flags);
	FUNCTION_LOG;
	if(dev->is_bayer_raw)
		vfe_dbg(0,"LV = %d, temp = %d\n",(dev->isp_gen_set_pt->isp_ini_cfg.isp_3a_settings.ae_max_lv - dev->isp_gen_set_pt->exp_settings.tbl_cnt*4), dev->isp_gen_set_pt->color_temp);
	//exception handle:
	if((status.buf_0_overflow) || (status.buf_1_overflow) || (status.buf_2_overflow) || (status.hblank_overflow))
	{
		if((status.buf_0_overflow) || (status.buf_1_overflow) || (status.buf_2_overflow)) {
			bsp_csi_int_clear_status(dev->vip_sel, dev->cur_ch,CSI_INT_BUF_0_OVERFLOW | CSI_INT_BUF_1_OVERFLOW \
										                                  | CSI_INT_BUF_2_OVERFLOW);
			vfe_err("fifo overflow\n");
		}
		if(status.hblank_overflow) {
			bsp_csi_int_clear_status(dev->vip_sel, dev->cur_ch,CSI_INT_HBLANK_OVERFLOW);
			vfe_err("hblank overflow\n");
		}
		vfe_err("reset csi module\n");
		bsp_csi_reset(dev->vip_sel);
		if(dev->is_isp_used)
			goto isp_exp_handle;
		else
			goto unlock;
	}

isp_exp_handle:
	if(dev->is_isp_used) {
		if(bsp_isp_get_irq_status(SRC0_FIFO_INT_EN)) {
			vfe_err("isp source0 fifo overflow\n");
			bsp_isp_clr_irq_status(SRC0_FIFO_INT_EN);
			goto unlock;
		}
	}
	vfe_dbg(3,"status vsync = %d, framedone = %d, capdone = %d\n",status.vsync_trig,status.frame_done,status.capture_done);
	if (dev->capture_mode == V4L2_MODE_IMAGE)
	{
		if(dev->is_isp_used)
			bsp_isp_irq_disable(FINISH_INT_EN);
		else
			bsp_csi_int_disable(dev->vip_sel, dev->cur_ch,CSI_INT_CAPTURE_DONE);
		vfe_print("capture image mode!\n");
		buf = list_entry(dma_q->active.next,struct vfe_buffer, vb.queue);
		list_del(&buf->vb.queue);
		buf->vb.state = VIDEOBUF_DONE;
		wake_up(&buf->vb.done);
		goto unlock;
	} else {
		if(dev->is_isp_used)
			bsp_isp_irq_disable(FINISH_INT_EN);
		else
			bsp_csi_int_disable(dev->vip_sel, dev->cur_ch,CSI_INT_FRAME_DONE);
		if (dev->first_flag == 0) {
			dev->first_flag++;
			vfe_print("capture video mode!\n");
			goto set_isp_stat_addr;
		}
		if (dev->first_flag == 1) {
			dev->first_flag++;
			vfe_print("capture video first frame done!\n");
		}

		//video buffer handle:
		if ((&dma_q->active) == dma_q->active.next->next->next) {
			vfe_warn("Only two buffer left for csi\n");
			dev->first_flag=0;
			goto unlock;
		}
		buf = list_entry(dma_q->active.next,struct vfe_buffer, vb.queue);

		/* Nobody is waiting on this buffer*/
		if (!waitqueue_active(&buf->vb.done)) {
			vfe_warn(" Nobody is waiting on this video buffer,buf = 0x%p\n",buf);
		}
		list_del(&buf->vb.queue);
		do_gettimeofday(&buf->vb.ts);
#ifdef CONFIG_ARCH_SUN8IW8P1		
               ktime_get_ts(&timestamp);
               buf->vb.ts.tv_sec = timestamp.tv_sec;
               buf->vb.ts.tv_usec = timestamp.tv_nsec / 1000;
#endif		
		buf->vb.field_count++;
		//if(frame_cnt%150 == 0){
		//	printk("video buffer fps = %ld\n",100000000/(buf->vb.ts.tv_sec*1000000+buf->vb.ts.tv_usec - (dev->sec*1000000+dev->usec)));
		//}
		vfe_dbg(2,"video buffer frame interval = %ld\n",buf->vb.ts.tv_sec*1000000+buf->vb.ts.tv_usec - (dev->sec*1000000+dev->usec));
		dev->sec = buf->vb.ts.tv_sec;
		dev->usec = buf->vb.ts.tv_usec;
		dev->ms += jiffies_to_msecs(jiffies - dev->jiffies);
		dev->jiffies = jiffies;
 		buf->vb.image_quality = dev->isp_3a_result_pt->image_quality.dwval;

		buf->vb.state = VIDEOBUF_DONE;
		buf->image_quality = dev->isp_3a_result_pt->image_quality.dwval;

		wake_up(&buf->vb.done);

		//isp_stat_handle:

		if(dev->is_isp_used && dev->is_bayer_raw) {
			list_for_each_entry(stat_buf_pt, &isp_stat_bq->locked, queue)
			{
				if(stat_buf_pt->isp_stat_buf.buf_status == BUF_LOCKED) {
					vfe_dbg(3,"find isp stat buf locked!\n");
					goto set_next_output_addr;
					vfe_dbg(3,"isp_stat_bq->locked = %p\n",&isp_stat_bq->locked);
					vfe_dbg(3,"isp_stat_bq->locked.next = %p\n",isp_stat_bq->locked.next);
					vfe_dbg(3,"isp_stat_bq->isp_stat[%d].queue = %p\n",stat_buf_pt->id,&isp_stat_bq->isp_stat[stat_buf_pt->id].queue);
					vfe_dbg(3,"isp_stat_bq->isp_stat[%d].queue.prev = %p\n",stat_buf_pt->id,isp_stat_bq->isp_stat[stat_buf_pt->id].queue.prev);
					vfe_dbg(3,"isp_stat_bq->isp_stat[%d].queue.next = %p\n",stat_buf_pt->id,isp_stat_bq->isp_stat[stat_buf_pt->id].queue.next);
				}
			}
			for (i = 0; i < MAX_ISP_STAT_BUF; i++)
			{
				stat_buf_pt = &isp_stat_bq->isp_stat[i];
				if(stat_buf_pt->isp_stat_buf.buf_status == BUF_IDLE) {
					vfe_dbg(3,"find isp stat buf idle!\n");
					list_move_tail(&stat_buf_pt->queue, &isp_stat_bq->active);
					stat_buf_pt->isp_stat_buf.buf_status = BUF_ACTIVE;
				}
			}

			vfe_dbg(3,"before list empty isp_stat_bq->active = %p\n",&isp_stat_bq->active);
			vfe_dbg(3,"before list empty isp_stat_bq->active.prev = %p\n",isp_stat_bq->active.prev);
			vfe_dbg(3,"before list empty isp_stat_bq->active.next = %p\n",isp_stat_bq->active.next);

			//judge if the isp stat queue has been written to the last
			if (list_empty(&isp_stat_bq->active)) {
				vfe_err("No active isp stat queue to serve\n");
				goto set_next_output_addr;
			}
			vfe_dbg(3,"after list empty isp_stat_bq->active = %p\n",&isp_stat_bq->active);
			vfe_dbg(3,"after list empty isp_stat_bq->active.prev = %p\n",isp_stat_bq->active.prev);
			vfe_dbg(3,"after list empty isp_stat_bq->active.next = %p\n",isp_stat_bq->active.next);


			//delete the ready buffer from the actvie queue
			//add the ready buffer to the locked queue
			//stat_buf_pt = list_first_entry(&isp_stat_bq->active, struct vfe_isp_stat_buf, queue);
			stat_buf_pt = list_entry(isp_stat_bq->active.next, struct vfe_isp_stat_buf, queue);

			list_move_tail(&stat_buf_pt->queue,&isp_stat_bq->locked);
			stat_buf_pt->isp_stat_buf.buf_status = BUF_LOCKED;
			dev->isp_gen_set_pt->stat.stat_buf_whole = &isp_stat_bq->isp_stat[stat_buf_pt->id].isp_stat_buf;
			vfe_isp_stat_parse(dev->isp_gen_set_pt);
			isp_stat_bq->isp_stat[stat_buf_pt->id].isp_stat_buf.frame_number++;

			if ((&isp_stat_bq->active) == isp_stat_bq->active.next->next) {
				vfe_warn("No more isp stat free frame on next time\n");
				goto set_next_output_addr;
			}
		}
	}

set_isp_stat_addr:
	if(dev->is_isp_used && dev->is_bayer_raw) {
		//stat_buf_pt = list_entry(isp_stat_bq->active.next->next, struct vfe_isp_stat_buf, queue);
		stat_buf_pt = list_entry(isp_stat_bq->active.next, struct vfe_isp_stat_buf, queue);
		bsp_isp_set_statistics_addr((unsigned int)(stat_buf_pt->dma_addr));
	}
set_next_output_addr:
//buf = list_entry(dma_q->active.next->next,struct vfe_buffer, vb.queue);
	if (list_empty(&dma_q->active) || dma_q->active.next->next == (&dma_q->active) ) {
		vfe_print("No active queue to serve\n");
		goto unlock;
	}
	buf = list_entry(dma_q->active.next->next,struct vfe_buffer, vb.queue);
	vfe_set_addr(dev,buf);

unlock:
	//spin_unlock(&dev->slock);
	spin_unlock_irqrestore(&dev->slock, flags);

	if ( ( (dev->capture_mode == V4L2_MODE_VIDEO)||(dev->capture_mode == V4L2_MODE_PREVIEW) )
							&& dev->is_isp_used && bsp_isp_get_irq_status(FINISH_INT_EN))
	{
		//if(bsp_isp_get_para_ready())
		{
			vfe_dbg(3,"call tasklet schedule! \n");
			bsp_isp_clr_para_ready();
			schedule_work(&dev->isp_isr_bh_task);
			bsp_isp_set_para_ready();
		}
	}
	if(dev->is_isp_used) {
		bsp_isp_clr_irq_status(FINISH_INT_EN);
		bsp_isp_irq_enable(FINISH_INT_EN);
	} else {
		bsp_csi_int_clear_status(dev->vip_sel, dev->cur_ch,CSI_INT_FRAME_DONE);
		//bsp_csi_int_clear_status(dev->vip_sel, dev->cur_ch,CSI_INT_VSYNC_TRIG);
		//bsp_csi_int_clear_status(dev->vip_sel, dev->cur_ch,CSI_INT_CAPTURE_DONE);
		if( (dev->capture_mode == V4L2_MODE_VIDEO) || (dev->capture_mode == V4L2_MODE_PREVIEW) )
			bsp_csi_int_enable(dev->vip_sel, dev->cur_ch,CSI_INT_FRAME_DONE);
	}
	return IRQ_HANDLED;
}

/*
 * Videobuf operations
 */
static int buffer_setup(struct videobuf_queue *vq, unsigned int *count, unsigned int *size)
{
	struct vfe_dev *dev = vq->priv_data;
	int buf_max_flag = 0;

	vfe_dbg(1,"buffer_setup\n");

	*size = dev->buf_byte_size;

	while (*size * *count > MAX_FRAME_MEM) {
		(*count)--;
		buf_max_flag = 1;
		if(*count == 0)
			vfe_err("one buffer size larger than max frame memory! buffer count = %d\n,",*count);
	}

	if(buf_max_flag == 0) {
		if(dev->capture_mode == V4L2_MODE_IMAGE) {
			if (*count != 1) {
				*count = 1;
				vfe_err("buffer count is set to 1 in image capture mode\n");
			}
		} else {
			if (*count < 3) {
				*count = 3;
				vfe_err("buffer count is invalid, set to 3 in video capture\n");
			}
		}
	}

	vfe_print("%s, buffer count=%d, size=%d\n", __func__,*count, *size);

	return 0;
}

static void free_buffer(struct videobuf_queue *vq, struct vfe_buffer *buf)
{
	vfe_dbg(1,"%s, state: %i\n", __func__, buf->vb.state);
	videobuf_waiton(vq, &buf->vb, 0, 0);
	videobuf_dma_contig_free(vq, &buf->vb);
	vfe_dbg(1,"free_buffer: freed\n");
	buf->vb.state = VIDEOBUF_NEEDS_INIT;
}

static int buffer_prepare(struct videobuf_queue *vq, struct videobuf_buffer *vb,
              enum v4l2_field field)
{
	struct vfe_dev *dev = vq->priv_data;
	struct vfe_buffer *buf = container_of(vb, struct vfe_buffer, vb);
	int rc;

	vfe_dbg(1,"buffer_prepare\n");

	BUG_ON(NULL == dev->fmt);

	if (dev->width  < MIN_WIDTH || dev->width  > MAX_WIDTH ||
						dev->height < MIN_HEIGHT || dev->height > MAX_HEIGHT) {
		return -EINVAL;
	}

	buf->vb.size = dev->buf_byte_size;

	if (0 != buf->vb.baddr && buf->vb.bsize < buf->vb.size) {
		return -EINVAL;
	}

	/* These properties only change when queue is idle, see s_fmt */
	buf->fmt       = dev->fmt;
	buf->vb.width  = dev->width;
	buf->vb.height = dev->height;
	buf->vb.field  = field;

	if (VIDEOBUF_NEEDS_INIT == buf->vb.state) {
		rc = videobuf_iolock(vq, &buf->vb, NULL);
		if (rc < 0) {
			goto fail;
		}
	}

	vb->boff = videobuf_to_dma_contig(vb);
	buf->vb.state = VIDEOBUF_PREPARED;
	return 0;

	fail:
	free_buffer(vq, buf);
	return rc;
}

static void buffer_queue(struct videobuf_queue *vq, struct videobuf_buffer *vb)
{
	struct vfe_dev *dev = vq->priv_data;
	struct vfe_buffer *buf = container_of(vb, struct vfe_buffer, vb);
	struct vfe_dmaqueue *vidq = &dev->vidq;

	vfe_dbg(1,"buffer_queue\n");
	buf->vb.state = VIDEOBUF_QUEUED;
	list_add_tail(&buf->vb.queue, &vidq->active);
}

static void buffer_release(struct videobuf_queue *vq,
         struct videobuf_buffer *vb)
{
	struct vfe_buffer *buf  = container_of(vb, struct vfe_buffer, vb);
	vfe_dbg(1,"buffer_release\n");
	free_buffer(vq, buf);
}

static struct videobuf_queue_ops vfe_video_qops = {
  .buf_setup    = buffer_setup,
  .buf_prepare  = buffer_prepare,
  .buf_queue    = buffer_queue,
  .buf_release  = buffer_release,
};

/*
 * IOCTL vidioc handling
 */
static int vidioc_querycap(struct file *file, void  *priv,
          struct v4l2_capability *cap)
{
	struct vfe_dev *dev = video_drvdata(file);

	strcpy(cap->driver, "sunxi-vfe");
	strcpy(cap->card, "sunxi-vfe");
	strlcpy(cap->bus_info, dev->v4l2_dev.name, sizeof(cap->bus_info));

	cap->version = VFE_VERSION;
	cap->capabilities = V4L2_CAP_VIDEO_CAPTURE | V4L2_CAP_STREAMING | \
						V4L2_CAP_READWRITE;
	return 0;
}

static int vidioc_enum_fmt_vid_cap(struct file *file, void  *priv,
          struct v4l2_fmtdesc *f)
{
	struct vfe_fmt *fmt;

	vfe_dbg(0,"vidioc_enum_fmt_vid_cap\n");

	if (f->index > ARRAY_SIZE(formats)-1) {
		return -EINVAL;
	}
	fmt = &formats[f->index];

	strlcpy(f->description, fmt->name, sizeof(f->description));
	f->pixelformat = fmt->fourcc;
	return 0;
}


static int vidioc_enum_framesizes(struct file *file, void *fh,
          struct v4l2_frmsizeenum *fsize)
{
	struct vfe_dev *dev = video_drvdata(file);

	vfe_dbg(0, "vidioc_enum_framesizes\n");

	if (dev == NULL || dev->sd->ops->video->enum_framesizes==NULL) {
		return -EINVAL;
	}

	return v4l2_subdev_call(dev->sd,video,enum_framesizes,fsize);
}



static int vidioc_g_fmt_vid_cap(struct file *file, void *priv,
          struct v4l2_format *f)
{
	struct vfe_dev *dev = video_drvdata(file);

	f->fmt.pix.width        = dev->width;
	f->fmt.pix.height       = dev->height;
	f->fmt.pix.field        = dev->vb_vidq.field;
	f->fmt.pix.pixelformat  = dev->fmt->bus_pix_code;
	//  f->fmt.pix.bytesperline = (f->fmt.pix.width * dev->fmt->depth) >> 3;
	//  f->fmt.pix.sizeimage    = f->fmt.pix.height * f->fmt.pix.bytesperline;

	return 0;
}

static enum bus_pixelcode bus_pix_code_v4l2_to_common(enum v4l2_mbus_pixelcode pix_code)
{
	return (enum bus_pixelcode)pix_code;
}

static enum pixel_fmt pix_fmt_v4l2_to_common(unsigned int pix_fmt)
{
	switch(pix_fmt) {
		case V4L2_PIX_FMT_RGB565:
			return PIX_FMT_RGB565;
		case V4L2_PIX_FMT_RGB24:
			return PIX_FMT_RGB888;
		case V4L2_PIX_FMT_RGB32:
			return PIX_FMT_PRGB888;
		case V4L2_PIX_FMT_YUYV:
			return PIX_FMT_YUYV;
		case V4L2_PIX_FMT_YVYU:
			return PIX_FMT_YVYU;
		case V4L2_PIX_FMT_UYVY:
			return PIX_FMT_UYVY;
		case V4L2_PIX_FMT_VYUY:
			return PIX_FMT_VYUY;
		case V4L2_PIX_FMT_YUV422P:
			return PIX_FMT_YUV422P_8;
		case V4L2_PIX_FMT_YUV420:
			return PIX_FMT_YUV420P_8;
		case V4L2_PIX_FMT_YVU420:
			return PIX_FMT_YVU420P_8;
		case V4L2_PIX_FMT_NV12:
			return PIX_FMT_YUV420SP_8;
		case V4L2_PIX_FMT_NV21:
			return PIX_FMT_YVU420SP_8;
		case V4L2_PIX_FMT_NV16:
			return PIX_FMT_YUV422SP_8;
		case V4L2_PIX_FMT_NV61:
			return PIX_FMT_YVU422SP_8;
		case V4L2_PIX_FMT_SBGGR8:
			return PIX_FMT_SBGGR_8;
		case V4L2_PIX_FMT_SGBRG8:
			return PIX_FMT_SGBRG_8;
		case V4L2_PIX_FMT_SGRBG8:
			return PIX_FMT_SGRBG_8;
		case V4L2_PIX_FMT_SRGGB8:
			return PIX_FMT_SRGGB_8;
		case V4L2_PIX_FMT_SBGGR10:
			return PIX_FMT_SBGGR_10;
		case V4L2_PIX_FMT_SGBRG10:
			return PIX_FMT_SGBRG_10;
		case V4L2_PIX_FMT_SGRBG10:
			return PIX_FMT_SGRBG_10;
		case V4L2_PIX_FMT_SRGGB10:
			return PIX_FMT_SRGGB_10;
		case V4L2_PIX_FMT_SBGGR12:
			return PIX_FMT_SBGGR_12;
		case V4L2_PIX_FMT_SGBRG12:
			return PIX_FMT_SGBRG_12;
		case V4L2_PIX_FMT_SGRBG12:
			return PIX_FMT_SGRBG_12;
		case V4L2_PIX_FMT_SRGGB12:
			return PIX_FMT_SRGGB_12;
		default:
			return PIX_FMT_SBGGR_8;
	}
}

static enum field field_fmt_v4l2_to_common(enum v4l2_field field)
{
	return (enum field)field;
}

static enum v4l2_mbus_pixelcode *try_fmt_internal(struct vfe_dev *dev,struct v4l2_format *f)
{
	enum pixel_fmt pix_fmt;
	enum pixel_fmt_type pix_fmt_type;
	struct v4l2_mbus_framefmt ccm_fmt;
	enum v4l2_mbus_pixelcode *bus_pix_code;
	int ret = 0;

	vfe_dbg(0,"try_fmt_internal\n");

	/*judge the resolution*/
	if(f->fmt.pix.width > MAX_WIDTH || f->fmt.pix.height > MAX_HEIGHT) {
		vfe_err("size is too large,automatically set to maximum!\n");
		f->fmt.pix.width = MAX_WIDTH;
		f->fmt.pix.height = MAX_HEIGHT;
	}

	pix_fmt = pix_fmt_v4l2_to_common(f->fmt.pix.pixelformat);
	pix_fmt_type = find_pixel_fmt_type(pix_fmt);

	ccm_fmt.width = f->fmt.pix.width;
	ccm_fmt.height = f->fmt.pix.height;

  //find the expect bus format via frame format list
  if(pix_fmt_type == YUV422_PL || pix_fmt_type == YUV422_SPL || \
    pix_fmt_type == YUV420_PL || pix_fmt_type == YUV420_SPL) {
    if(dev->is_isp_used&&dev->is_bayer_raw) {
     // vfe_print("using isp at %s!\n",__func__);
      for(bus_pix_code = try_bayer_rgb_bus; bus_pix_code < try_bayer_rgb_bus + N_TRY_BAYER; bus_pix_code++) {
        ccm_fmt.code  = *bus_pix_code;
        ccm_fmt.field = f->fmt.pix.field;
        ret = v4l2_subdev_call(dev->sd,video,try_mbus_fmt,&ccm_fmt);
        if (ret >= 0) {
          vfe_dbg(0,"try bayer bus ok when pix fmt is yuv422/yuv420 at %s!\n",__func__);
          break;
        }
      }
      if (ret < 0 ) {
        vfe_err("try bayer bus error when pix fmt is yuv422/yuv420 at %s!\n",__func__);
        for(bus_pix_code = try_yuv422_bus; bus_pix_code < try_yuv422_bus + N_TRY_YUV422; bus_pix_code++) {
          ccm_fmt.code  = *bus_pix_code;
          ccm_fmt.field = f->fmt.pix.field;
          ret = v4l2_subdev_call(dev->sd,video,try_mbus_fmt,&ccm_fmt);
          if (ret >= 0) {
            vfe_dbg(0,"try yuv22 bus ok when pix fmt is yuv422/yuv420 at %s!\n",__func__);
            break;
          }
        }
        if (ret < 0) {
          vfe_err("try yuv22 fmt error when pix fmt is yuv422/yuv420 at %s!\n",__func__);
          if(pix_fmt_type == YUV420_PL || pix_fmt_type == YUV420_SPL) {
            for(bus_pix_code = try_yuv420_bus; bus_pix_code < try_yuv420_bus + N_TRY_YUV420; bus_pix_code++) {
              ccm_fmt.code  = *bus_pix_code;
              ccm_fmt.field = f->fmt.pix.field;
              ret = v4l2_subdev_call(dev->sd,video,try_mbus_fmt,&ccm_fmt);
              if (ret >= 0) {
                vfe_dbg(0,"try yuv420 bus ok when pix fmt is yuv420 at %s!\n",__func__);
                break;
              }
            }
            if (ret < 0) {
              vfe_err("try yuv420 bus error when pix fmt is yuv420 at %s!\n",__func__);
              return NULL;
            }
          } else {
            return NULL;
          }
        }
      }
    } else {
     // vfe_dbg(0,"not using isp at %s!\n",__func__);
      for(bus_pix_code = try_yuv422_bus; bus_pix_code < try_yuv422_bus + N_TRY_YUV422; bus_pix_code++) {
        ccm_fmt.code  = *bus_pix_code;
        ccm_fmt.field = f->fmt.pix.field;
        ret = v4l2_subdev_call(dev->sd,video,try_mbus_fmt,&ccm_fmt);
        vfe_dbg(0,"v4l2_subdev_call try_mbus_fmt return %x\n",ret);
        if (ret >= 0) {
          vfe_dbg(0,"try yuv422 fmt ok when pix fmt is yuv422/yuv420 at %s!\n",__func__);
          break;
        }
      }
      if (ret < 0) {
        vfe_err("try yuv422 fmt error when pix fmt is yuv422/yuv420 at %s!\n",__func__);
        if(pix_fmt_type == YUV420_PL || pix_fmt_type == YUV420_SPL) {

        } else {
          return NULL;
        }
      }
    }
  } else if (pix_fmt_type == YUV422_INTLVD) {
    //vfe_dbg(0,"not using isp at %s!\n",__func__);
    for(bus_pix_code = try_yuv422_bus; bus_pix_code < try_yuv422_bus + N_TRY_YUV422; bus_pix_code++) {
      ccm_fmt.code  = *bus_pix_code;
      ccm_fmt.field = f->fmt.pix.field;
      ret = v4l2_subdev_call(dev->sd,video,try_mbus_fmt,&ccm_fmt);
      if (ret >= 0) {
        vfe_dbg(0,"try yuv422 bus ok when pix fmt is yuv422 interleaved at %s!\n",__func__);
        break;
      }
    }
    if (ret < 0) {
      vfe_err("try yuv422 bus error when pix fmt is yuv422 interleaved at %s!\n",__func__);
      return NULL;
    }
  } else if (pix_fmt_type == BAYER_RGB) {
   // vfe_dbg(0,"not using isp at %s!\n",__func__);
    for(bus_pix_code = try_bayer_rgb_bus; bus_pix_code < try_bayer_rgb_bus + N_TRY_BAYER; bus_pix_code++) {
      ccm_fmt.code  = *bus_pix_code;
      ccm_fmt.field = f->fmt.pix.field;
      ret = v4l2_subdev_call(dev->sd,video,try_mbus_fmt,&ccm_fmt);
      if (ret >= 0) {
        vfe_dbg(0,"try bayer bus ok when pix fmt is bayer rgb at %s!\n",__func__);
        break;
      }
    }
    if (ret < 0) {
      vfe_err("try bayer bus error when pix fmt is bayer rgb at %s!\n",__func__);
      return NULL;
    }
  } else if (pix_fmt_type == RGB565) {
  //  vfe_dbg(0,"not using isp at %s!\n",__func__);
    for(bus_pix_code = try_rgb565_bus; bus_pix_code < try_rgb565_bus + N_TRY_BAYER; bus_pix_code++) {
      ccm_fmt.code  = *bus_pix_code;
      ccm_fmt.field = f->fmt.pix.field;
      ret = v4l2_subdev_call(dev->sd,video,try_mbus_fmt,&ccm_fmt);
      if (ret >= 0) {
        vfe_dbg(0,"try rgb565 bus ok when pix fmt is rgb565 at %s!\n",__func__);
        break;
      }
    }
    if (ret < 0) {
      vfe_err("try rgb565 bus error when pix fmt is rgb565 at %s!\n",__func__);
      return NULL;
    }
  } else if (pix_fmt_type == RGB888 || pix_fmt_type == PRGB888) {
  //  vfe_dbg(0,"not using isp at %s!\n",__func__);
    for(bus_pix_code = try_rgb888_bus; bus_pix_code < try_rgb888_bus + N_TRY_BAYER; bus_pix_code++) {
      ccm_fmt.code  = *bus_pix_code;
      ccm_fmt.field = f->fmt.pix.field;
      ret = v4l2_subdev_call(dev->sd,video,try_mbus_fmt,&ccm_fmt);
      if (ret >= 0) {
        vfe_dbg(0,"try rgb888 bus ok when pix fmt is rgb888/prgb888 at %s!\n",__func__);
        break;
      }
    }
    if (ret < 0) {
      vfe_err("try rgb888 bus error when pix fmt is rgb888/prgb888 at %s!\n",__func__);
      return NULL;
    }
  } else {
    return NULL;
  }

  f->fmt.pix.width = ccm_fmt.width;
  f->fmt.pix.height = ccm_fmt.height;
  f->fmt.pix.sizeimage = ccm_fmt.height * f->fmt.pix.bytesperline;

  vfe_dbg(0,"bus pixel code = %x at %s\n",*bus_pix_code,__func__);
  vfe_dbg(0,"pix->width = %d at %s\n",f->fmt.pix.width,__func__);
  vfe_dbg(0,"pix->height = %d at %s\n",f->fmt.pix.height,__func__);
  vfe_dbg(0,"pix->sizeimage = %d at %s\n",f->fmt.pix.sizeimage,__func__);

  return bus_pix_code;
}

static enum pkt_fmt get_pkt_fmt(enum bus_pixelcode bus_pix_code)
{
  switch(bus_pix_code) {
    case BUS_FMT_RGB565_16X1:
      return MIPI_RGB565;
    case BUS_FMT_UYVY8_16X1:
      return MIPI_YUV422;
    case BUS_FMT_UYVY10_20X1:
      return MIPI_YUV422_10;
    case BUS_FMT_SBGGR8_8X1:
    case BUS_FMT_SGBRG8_8X1:
    case BUS_FMT_SGRBG8_8X1:
    case BUS_FMT_SRGGB8_8X1:
      return MIPI_RAW8;
    case BUS_FMT_SBGGR10_10X1:
    case BUS_FMT_SGBRG10_10X1:
    case BUS_FMT_SGRBG10_10X1:
    case BUS_FMT_SRGGB10_10X1:
      return MIPI_RAW10;
    case BUS_FMT_SBGGR12_12X1:
    case BUS_FMT_SGBRG12_12X1:
    case BUS_FMT_SGRBG12_12X1:
    case BUS_FMT_SRGGB12_12X1:
      return MIPI_RAW12;
    case BUS_FMT_YY8_UYVY8_12X1:
      return MIPI_YUV420;
    case BUS_FMT_YY10_UYVY10_15X1:
      return MIPI_YUV420_10;
    default:
      return MIPI_RAW8;
  }
}

static int vidioc_try_fmt_vid_cap(struct file *file, void *priv,
      struct v4l2_format *f)
{
	struct vfe_dev *dev = video_drvdata(file);
	//  struct vfe_fmt *vfe_fmt;
	//  struct v4l2_mbus_framefmt ccm_fmt;
	enum v4l2_mbus_pixelcode *bus_pix_code;

	vfe_dbg(0,"vidioc_try_fmt_vid_cap\n");

	bus_pix_code = try_fmt_internal(dev,f);
	if(!bus_pix_code) {
		vfe_err("pixel format (0x%08x) width %d height %d invalid at %s.\n", \
		f->fmt.pix.pixelformat,f->fmt.pix.width,f->fmt.pix.height,__func__);
		return -EINVAL;
	}

	return 0;
}

static int vidioc_s_fmt_vid_cap(struct file *file, void *priv,
          struct v4l2_format *f)
{
  struct vfe_dev *dev = video_drvdata(file);
  struct videobuf_queue *q = &dev->vb_vidq;
  struct v4l2_mbus_framefmt ccm_fmt;
  struct v4l2_mbus_config mbus_cfg;
  enum v4l2_mbus_pixelcode *bus_pix_code;
  enum pixel_fmt isp_fmt[ISP_MAX_CH_NUM];
  struct isp_size isp_size[ISP_MAX_CH_NUM];

  struct sensor_win_size win_cfg;
  struct isp_size_settings size_settings;
  struct isp_size ob_black_size,ob_valid_size;
  struct coor ob_start;

  unsigned char ch_num;
  unsigned int i,scale_ratio;
  int ret;

  vfe_dbg(0,"vidioc_s_fmt_vid_cap\n");

  if (vfe_is_generating(dev)) {
    vfe_err("%s device busy\n", __func__);
    return -EBUSY;
  }

  mutex_lock(&q->vb_lock);

  bus_pix_code = try_fmt_internal(dev,f);
  if(!bus_pix_code) {
    vfe_err("pixel format (0x%08x) width %d height %d invalid at %s.\n", \
    f->fmt.pix.pixelformat,f->fmt.pix.width,f->fmt.pix.height,__func__);
    ret = -EINVAL;
    goto out;
  }
  vfe_dbg(0,"bus pixel code = %x at %s\n",*bus_pix_code,__func__);
  vfe_dbg(0,"pix->width = %d at %s\n",f->fmt.pix.width,__func__);
  vfe_dbg(0,"pix->height = %d at %s\n",f->fmt.pix.height,__func__);

  //get current win configs
  memset(&win_cfg, 0, sizeof(struct sensor_win_size));
  ret=v4l2_subdev_call(dev->sd,core,ioctl,GET_CURRENT_WIN_CFG,&win_cfg);

  ret = get_mbus_config(dev, &mbus_cfg);
  if (ret < 0) {
    vfe_err("get_mbus_config failed!\n");
    goto out;
  }

  if (mbus_cfg.type == V4L2_MBUS_CSI2) {
    dev->bus_info.bus_if = CSI2;
    if(IS_FLAG(mbus_cfg.flags,V4L2_MBUS_CSI2_4_LANE))
      dev->mipi_para.lane_num = 4;
    else if(IS_FLAG(mbus_cfg.flags,V4L2_MBUS_CSI2_3_LANE))
      dev->mipi_para.lane_num = 3;
    else if(IS_FLAG(mbus_cfg.flags,V4L2_MBUS_CSI2_2_LANE))
      dev->mipi_para.lane_num = 2;
    else
      dev->mipi_para.lane_num = 1;

    ch_num = 0;
    dev->total_bus_ch = 0;
    if (IS_FLAG(mbus_cfg.flags,V4L2_MBUS_CSI2_CHANNEL_0)) {
      dev->mipi_fmt.vc[ch_num] = 0;
      dev->mipi_fmt.field[ch_num] = dev->frame_info.ch_field[ch_num];
      ch_num++;
      dev->total_bus_ch++;
    }
    if (IS_FLAG(mbus_cfg.flags,V4L2_MBUS_CSI2_CHANNEL_1)) {
      dev->mipi_fmt.vc[ch_num] = 1;
      dev->mipi_fmt.field[ch_num] = dev->frame_info.ch_field[ch_num];
      ch_num++;
      dev->total_bus_ch++;
    }
    if (IS_FLAG(mbus_cfg.flags,V4L2_MBUS_CSI2_CHANNEL_2)) {
      dev->mipi_fmt.vc[ch_num] = 2;
      dev->mipi_fmt.field[ch_num] = dev->frame_info.ch_field[ch_num];
      ch_num++;
      dev->total_bus_ch++;
    }
    if (IS_FLAG(mbus_cfg.flags,V4L2_MBUS_CSI2_CHANNEL_3)) {
      dev->mipi_fmt.vc[ch_num] = 3;
      dev->mipi_fmt.field[ch_num] = dev->frame_info.ch_field[ch_num];
      ch_num++;
      dev->total_bus_ch++;
    }

    dev->total_rx_ch = dev->total_bus_ch; //TODO
    dev->mipi_para.total_rx_ch = dev->total_rx_ch;
    vfe_print("V4L2_MBUS_CSI2,%d lane,bus%d channel,rx %d channel\n",dev->mipi_para.lane_num,dev->total_bus_ch,dev->total_rx_ch);
  } else if (mbus_cfg.type == V4L2_MBUS_PARALLEL) {
    dev->bus_info.bus_if = PARALLEL;
    if(IS_FLAG(mbus_cfg.flags,V4L2_MBUS_MASTER)) {
      if(IS_FLAG(mbus_cfg.flags,V4L2_MBUS_HSYNC_ACTIVE_HIGH)) {
        dev->bus_info.bus_tmg.href_pol = ACTIVE_HIGH;
      } else {
        dev->bus_info.bus_tmg.href_pol = ACTIVE_LOW;
      }
      if(IS_FLAG(mbus_cfg.flags,V4L2_MBUS_VSYNC_ACTIVE_HIGH)) {
        dev->bus_info.bus_tmg.vref_pol = ACTIVE_HIGH;
      } else {
        dev->bus_info.bus_tmg.vref_pol = ACTIVE_LOW;
      }
      if(IS_FLAG(mbus_cfg.flags,V4L2_MBUS_PCLK_SAMPLE_RISING)) {
        dev->bus_info.bus_tmg.pclk_sample = RISING;
      } else {
        dev->bus_info.bus_tmg.pclk_sample = FALLING;
      }
      if(IS_FLAG(mbus_cfg.flags,V4L2_MBUS_FIELD_EVEN_HIGH)) {
        dev->bus_info.bus_tmg.field_even_pol = ACTIVE_HIGH;
      } else {
        dev->bus_info.bus_tmg.field_even_pol = ACTIVE_LOW;
      }
    } else {
      vfe_err("Do not support MBUS SLAVE! get mbus_cfg.type = %d\n",mbus_cfg.type);
      goto out;
    }
  } else if (mbus_cfg.type == V4L2_MBUS_BT656) {
    dev->bus_info.bus_if = BT656;
  }



  //get mipi bps from win configs
  if(mbus_cfg.type == V4L2_MBUS_CSI2) {
    dev->mipi_para.bps = win_cfg.mipi_bps;
    dev->mipi_para.auto_check_bps = 0;  //TODO
    dev->mipi_para.dphy_freq = DPHY_CLK; //TODO

    for(i=0; i<dev->total_rx_ch; i++) {//TODO
      dev->bus_info.bus_ch_fmt[i] = bus_pix_code_v4l2_to_common(*bus_pix_code);
//      printk("dev->bus_info.bus_ch_fmt[%d] = %x\n",i,dev->bus_info.bus_ch_fmt[i]);
      dev->mipi_fmt.packet_fmt[i] = get_pkt_fmt(dev->bus_info.bus_ch_fmt[i]);
//      printk("dev->mipi_fmt.packet_fmt[%d] = %x\n",i,dev->mipi_fmt.packet_fmt[i]);
    }
    bsp_mipi_csi_dphy_init(dev->mipi_sel);
    bsp_mipi_csi_set_para(dev->mipi_sel, &dev->mipi_para);
    bsp_mipi_csi_set_fmt(dev->mipi_sel, dev->mipi_para.total_rx_ch, &dev->mipi_fmt);

    //for dphy clock async
    bsp_mipi_csi_dphy_disable(dev->mipi_sel);
    usleep_range(1000,2000);

    if(dev->clock.vfe_dphy_clk) {
      os_clk_disable(dev->clock.vfe_dphy_clk);
    } else {
      vfe_warn("vfe dphy clock is null\n");
    }

    bsp_mipi_csi_dphy_enable(dev->mipi_sel);

    if(dev->clock.vfe_dphy_clk) {
      if(os_clk_enable(dev->clock.vfe_dphy_clk))
        vfe_err("vfe dphy clock enable error\n");
    } else {
      vfe_warn("vfe dphy clock is null\n");
    }
    usleep_range(10000,12000);
  }

  //init device
  ccm_fmt.code = *bus_pix_code;
  ccm_fmt.width = f->fmt.pix.width;
  ccm_fmt.height = f->fmt.pix.height;
  ccm_fmt.field = f->fmt.pix.field;

  ret = v4l2_subdev_call(dev->sd,video,s_mbus_fmt,&ccm_fmt);
  if (ret < 0) {
    vfe_err("v4l2 sub device s_fmt error!\n");
    goto out;
  }

  //prepare the vfe bsp parameter
  //assuming using single channel
  dev->bus_info.ch_total_num = dev->total_rx_ch;

  //format and size info
  for(i=0;i<dev->total_bus_ch;i++)
    dev->bus_info.bus_ch_fmt[i] = bus_pix_code_v4l2_to_common(*bus_pix_code);

  for(i=0;i<dev->total_rx_ch;i++) {
    dev->frame_info.pix_ch_fmt[i] = pix_fmt_v4l2_to_common(f->fmt.pix.pixelformat);
    dev->frame_info.ch_field[i] = field_fmt_v4l2_to_common(ccm_fmt.field);
    dev->frame_info.ch_size[i].width = ccm_fmt.width;
    dev->frame_info.ch_size[i].height = ccm_fmt.height;
    dev->frame_info.ch_offset[i].hoff = win_cfg.hoffset;
    dev->frame_info.ch_offset[i].voff = win_cfg.voffset;
  }

  dev->frame_info.arrange.row = dev->arrange.row;
  dev->frame_info.arrange.column = dev->arrange.column;

  //set vfe bsp
  ret = bsp_csi_set_fmt(dev->vip_sel, &dev->bus_info,&dev->frame_info);
  if (ret < 0) {
    vfe_err("bsp_csi_set_fmt error at %s!\n",__func__);
    goto out;
  }
  //return buffer byte size via dev->frame_info.frm_byte_size
  ret = bsp_csi_set_size(dev->vip_sel, &dev->bus_info,&dev->frame_info);
  if (ret < 0) {
    vfe_err("bsp_csi_set_size error at %s!\n",__func__);
    goto out;
  }
  //save the info to the channel info
  for(i=0;i<dev->total_bus_ch;i++) {
    dev->ch[i].fmt.bus_pix_code = *bus_pix_code;
    dev->ch[i].fmt.fourcc = f->fmt.pix.pixelformat;
    dev->ch[i].fmt.field = ccm_fmt.field;
    dev->ch[i].size.width = ccm_fmt.width;
    dev->ch[i].size.height = ccm_fmt.height;
    dev->ch[i].size.hoffset = ccm_fmt.reserved[0];
    dev->ch[i].size.voffset = ccm_fmt.reserved[1];
  }

  if(dev->is_isp_used) {
    isp_fmt[MAIN_CH] = pix_fmt_v4l2_to_common(f->fmt.pix.pixelformat);
    isp_size[MAIN_CH].width = ccm_fmt.width;
    isp_size[MAIN_CH].height = ccm_fmt.height;
	//todo for rotation
	if(f->fmt.pix.rot_angle != 0)
    {
        isp_fmt[ROT_CH] = pix_fmt_v4l2_to_common(f->fmt.pix.pixelformat);
	}
    else
    {
		isp_fmt[ROT_CH] = PIX_FMT_NONE;
	}

    if (f->fmt.pix.subchannel)
	{
	  if(f->fmt.pix.subchannel->rot_angle != 0)
        isp_fmt[ROT_CH] = pix_fmt_v4l2_to_common(f->fmt.pix.subchannel->pixelformat);
	}

    if(f->fmt.pix.subchannel)
    {
      isp_fmt[SUB_CH] = pix_fmt_v4l2_to_common(f->fmt.pix.subchannel->pixelformat);
      isp_size[SUB_CH].width = f->fmt.pix.subchannel->width;
      isp_size[SUB_CH].height = f->fmt.pix.subchannel->height;
      dev->thumb_width  = f->fmt.pix.subchannel->width;
      dev->thumb_height = f->fmt.pix.subchannel->height;
      if(f->fmt.pix.subchannel->height > f->fmt.pix.height || f->fmt.pix.subchannel->width > f->fmt.pix.width)
      {
      		vfe_err("subchannel size > main channel size!!!");
      }
    } else {

      isp_fmt[SUB_CH] = PIX_FMT_NONE;
      isp_size[SUB_CH].width = 0;
      isp_size[SUB_CH].height = 0;
      scale_ratio = 0;
    }

    vfe_dbg(0,"*bus_pix_code = %d, isp_fmt = %p\n",*bus_pix_code,isp_fmt);

    if((f->fmt.pix.rot_angle == 0 && f->fmt.pix.subchannel == NULL)||
        (f->fmt.pix.rot_angle == 0 && f->fmt.pix.subchannel != NULL && f->fmt.pix.subchannel->rot_angle == 0))
    {
      isp_fmt[ROT_CH] = PIX_FMT_NONE;
    }

    bsp_isp_set_fmt(find_bus_type(*bus_pix_code),isp_fmt);

    //only has one rotation ch, priority: main ch > sub ch
    //from main ch first
    if(f->fmt.pix.rot_angle == 0) {
    	bsp_isp_set_rot(MAIN_CH,ANGLE_0);
        isp_size[ROT_CH].width = ccm_fmt.width;
        isp_size[ROT_CH].height = ccm_fmt.height;
    } else if(f->fmt.pix.rot_angle == 90) {
    	bsp_isp_set_rot(MAIN_CH,ANGLE_90);
        isp_size[ROT_CH].height = ccm_fmt.width;
        isp_size[ROT_CH].width = ccm_fmt.height;
    } else if(f->fmt.pix.rot_angle == 180) {
    	bsp_isp_set_rot(MAIN_CH,ANGLE_180);
        isp_size[ROT_CH].width = ccm_fmt.width;
        isp_size[ROT_CH].height = ccm_fmt.height;
    } else if(f->fmt.pix.rot_angle == 270) {
    	bsp_isp_set_rot(MAIN_CH,ANGLE_270);
        isp_size[ROT_CH].height = ccm_fmt.width;
        isp_size[ROT_CH].width = ccm_fmt.height;
    } else {
    	bsp_isp_set_rot(MAIN_CH,ANGLE_0);
    }
    //from sub ch second

    if(f->fmt.pix.subchannel){
    	if(f->fmt.pix.subchannel->rot_angle == 0) {
	    	bsp_isp_set_rot(SUB_CH,ANGLE_0);

            isp_size[ROT_CH].width = f->fmt.pix.subchannel->width;
            isp_size[ROT_CH].height = f->fmt.pix.subchannel->height;
	    } else if(f->fmt.pix.subchannel->rot_angle == 90) {
            isp_size[ROT_CH].height = f->fmt.pix.subchannel->width;
            isp_size[ROT_CH].width = f->fmt.pix.subchannel->height;
	    	bsp_isp_set_rot(SUB_CH,ANGLE_90);
	    } else if(f->fmt.pix.subchannel->rot_angle == 180) {
            isp_size[ROT_CH].width = f->fmt.pix.subchannel->width;
            isp_size[ROT_CH].height = f->fmt.pix.subchannel->height;
	    	bsp_isp_set_rot(SUB_CH,ANGLE_180);
	    } else if(f->fmt.pix.subchannel->rot_angle == 270) {
            isp_size[ROT_CH].height = f->fmt.pix.subchannel->width;
            isp_size[ROT_CH].width = f->fmt.pix.subchannel->height;
	    	bsp_isp_set_rot(SUB_CH,ANGLE_270);
	    } else {
	    	bsp_isp_set_rot(SUB_CH,ANGLE_0);
	    }
    }
	if(0 == win_cfg.width || 0 == win_cfg.height)
	{
		win_cfg.width = isp_size[MAIN_CH].width;
		win_cfg.height = isp_size[MAIN_CH].height;
	}
	if(0 == win_cfg.width_input || 0 == win_cfg.height_input)
	{
		win_cfg.width_input = win_cfg.width;
		win_cfg.height_input = win_cfg.height;
	}
	vfe_print("width_input = %d, height_input = %d, width = %d, height = %d\n", win_cfg.width_input,win_cfg.height_input,  win_cfg.width,  win_cfg.height );
	ob_black_size.width= win_cfg.width_input + 2*win_cfg.hoffset; //OK
	ob_black_size.height= win_cfg.height_input + 2*win_cfg.voffset;//OK
	ob_valid_size.width = win_cfg.width_input;
	ob_valid_size.height = win_cfg.height_input;
	ob_start.hor =  win_cfg.hoffset;  //OK
	ob_start.ver =  win_cfg.voffset;  //OK
	if((f->fmt.pix.rot_angle == 0 && f->fmt.pix.subchannel == NULL)||
				(f->fmt.pix.rot_angle == 0 && f->fmt.pix.subchannel != NULL && f->fmt.pix.subchannel->rot_angle == 0))
	{
		isp_size[ROT_CH].height = 0;
		isp_size[ROT_CH].width = 0;
		isp_fmt[ROT_CH] = PIX_FMT_NONE;
	}
	
	if(f->fmt.pix.subchannel != NULL)	{
		dev->isp_gen_set_pt->double_ch_flag = 1;
	} else {
		dev->isp_gen_set_pt->double_ch_flag = 0;
	}
  
	//dev->buf_byte_size = bsp_isp_set_size(isp_fmt,&ob_black_size, &ob_valid_size, &isp_size[MAIN_CH],&isp_size[ROT_CH],&ob_start,&isp_size[SUB_CH]);
	size_settings.full_size = isp_size[MAIN_CH];
	size_settings.scale_size = isp_size[SUB_CH];
	size_settings.ob_black_size = ob_black_size;
	size_settings.ob_start = ob_start;
	size_settings.ob_valid_size = ob_valid_size;
	size_settings.ob_rot_size = isp_size[ROT_CH];

	dev->buf_byte_size = bsp_isp_set_size(isp_fmt,&size_settings);

    vfe_print("dev->buf_byte_size = %d, double_ch_flag = %d\n",dev->buf_byte_size, dev->isp_gen_set_pt->double_ch_flag);
  } else {
    dev->buf_byte_size = dev->frame_info.frm_byte_size;
    dev->thumb_width  = 0;
    dev->thumb_height = 0;
  }

  dev->vb_vidq.field = ccm_fmt.field;
  dev->width  = ccm_fmt.width;
  dev->height = ccm_fmt.height;

	dev->mbus_type = mbus_cfg.type;
	if(dev->is_isp_used ==1 )
	{
		vfe_dbg(0,"isp_module_init start!\n");
		if(dev->is_bayer_raw == 1)
		{
			dev->isp_gen_set_pt->stat.pic_size.width = win_cfg.width_input;
			dev->isp_gen_set_pt->stat.pic_size.height= win_cfg.height_input;

			dev->isp_gen_set_pt->stat.hoffset = win_cfg.hoffset;
			dev->isp_gen_set_pt->stat.voffset = win_cfg.voffset;
			dev->isp_gen_set_pt->stat.hts = win_cfg.hts;
			dev->isp_gen_set_pt->stat.vts = win_cfg.vts;
			dev->isp_gen_set_pt->stat.pclk = win_cfg.pclk;
			dev->isp_gen_set_pt->stat.fps_fixed = win_cfg.fps_fixed;
			dev->isp_gen_set_pt->stat.bin_factor = win_cfg.bin_factor;
			dev->isp_gen_set_pt->stat.intg_min = win_cfg.intg_min;
			dev->isp_gen_set_pt->stat.intg_max = win_cfg.intg_max;
			dev->isp_gen_set_pt->stat.gain_min = win_cfg.gain_min;
			dev->isp_gen_set_pt->stat.gain_max = win_cfg.gain_max;
			if(V4L2_MODE_IMAGE == dev->capture_mode)
			{
				dev->isp_gen_set_pt->sensor_mod = CAPTURE_MODE;
			}
			else if(V4L2_MODE_VIDEO == dev->capture_mode)
			{
				dev->isp_gen_set_pt->sensor_mod = VIDEO_MODE;
			}
			else
			{
				dev->isp_gen_set_pt->sensor_mod = PREVIEW_MODE;
			}

			isp_module_init(dev->isp_gen_set_pt, dev->isp_3a_result_pt);
			dev->ctrl_para.prev_exp_line = 0;
			dev->ctrl_para.prev_ana_gain = 1;
			if(set_sensor_shutter_and_gain(dev) != 0)
			{
				set_sensor_shutter(dev,dev->isp_3a_result_pt->exp_line_num);
				set_sensor_gain(dev,dev->isp_3a_result_pt->exp_analog_gain);
			}
			if(f->fmt.pix.subchannel)
			{
				if(isp_fmt[SUB_CH] >= PIX_FMT_SBGGR_8&& isp_fmt[SUB_CH] <= PIX_FMT_SRGGB_12)
				{
					//after crop		
					vfe_reg_clr_set(IO_ADDRESS(ISP_REGS_BASE+0x10), (0xF << 16), (1 << 16));
					vfe_reg_set(IO_ADDRESS(ISP_REGS_BASE+0x10), (1 << 20));
					dev->isp_gen_set_pt->sensor_mod = VIDEO_MODE;
				}
			}

			usleep_range(50000,60000);
		}
		else
		{
			isp_module_init(dev->isp_gen_set_pt, NULL);
		}

		vfe_dbg(0,"isp_module_init end!\n");
	}
  ret = 0;
out:
  mutex_unlock(&q->vb_lock);
  return ret;
}

static int vidioc_reqbufs(struct file *file, void *priv,
        struct v4l2_requestbuffers *p)
{
	struct vfe_dev *dev = video_drvdata(file);

	vfe_dbg(0,"vidioc_reqbufs\n");

	return videobuf_reqbufs(&dev->vb_vidq, p);
}

static int vidioc_querybuf(struct file *file, void *priv, struct v4l2_buffer *p)
{
	struct vfe_dev *dev = video_drvdata(file);

	return videobuf_querybuf(&dev->vb_vidq, p);
}

static int vidioc_qbuf(struct file *file, void *priv, struct v4l2_buffer *p)
{
	struct vfe_dev *dev = video_drvdata(file);

	return videobuf_qbuf(&dev->vb_vidq, p);
}

static int vidioc_dqbuf(struct file *file, void *priv, struct v4l2_buffer *p)
{
	int ret = 0;
	struct vfe_dev *dev = video_drvdata(file);
	vfe_dbg(2,"vidioc dqbuf\n");
	ret = videobuf_dqbuf(&dev->vb_vidq, p, file->f_flags & O_NONBLOCK);
	//if (dev->isp_3a_result_pt != NULL && dev->is_bayer_raw)
	//{
	//	mutex_lock(&dev->isp_3a_result_mutex);
	//	p->reserved = dev->isp_3a_result_pt->image_quality;
	//	mutex_unlock(&dev->isp_3a_result_mutex);
	//}

	return ret;
}

#ifdef CONFIG_VIDEO_V4L1_COMPAT
static int vidiocgmbuf(struct file *file, void *priv, struct video_mbuf *mbuf)
{
	struct vfe_dev *dev = video_drvdata(file);

	return videobuf_cgmbuf(&dev->vb_vidq, mbuf, 8);
}
#endif

static int vidioc_streamon(struct file *file, void *priv, enum v4l2_buf_type i)
{
  struct vfe_dev *dev = video_drvdata(file);
  struct vfe_dmaqueue *dma_q = &dev->vidq;
  struct vfe_isp_stat_buf_queue *isp_stat_bq = &dev->isp_stat_bq;
  struct vfe_buffer *buf;
  struct vfe_isp_stat_buf *stat_buf_pt;

  int ret = 0;
  mutex_lock(&dev->stream_lock);
//  spin_lock(&dev->slock);//debug
  vfe_dbg(0,"video stream on\n");
  if (i != V4L2_BUF_TYPE_VIDEO_CAPTURE) {
    ret = -EINVAL;
    goto streamon_unlock;
  }

  if (vfe_is_generating(dev)) {
    vfe_err("stream has been already on\n");
    ret = -1;
    goto streamon_unlock;
  }

  bsp_csi_enable(dev->vip_sel);
  bsp_csi_disable(dev->vip_sel);
  bsp_csi_enable(dev->vip_sel);
  if(dev->is_isp_used)
    bsp_isp_enable();
  /* Resets frame counters */
  dev->ms = 0;
  dev->jiffies = jiffies;

  dma_q->frame = 0;
  dma_q->ini_jiffies = jiffies;

  if (dev->is_isp_used && dev->is_bayer_raw) {
    /* initial for isp statistic buffer queue */
    INIT_LIST_HEAD(&isp_stat_bq->active);
    INIT_LIST_HEAD(&isp_stat_bq->locked);
    for(i=0; i < MAX_ISP_STAT_BUF; i++) {
      isp_stat_bq->isp_stat[i].isp_stat_buf.buf_status = BUF_ACTIVE;
      list_add_tail(&isp_stat_bq->isp_stat[i].queue, &isp_stat_bq->active);
    }
  }

  ret = videobuf_streamon(&dev->vb_vidq);
  if (ret)
    goto streamon_unlock;

  buf = list_entry(dma_q->active.next,struct vfe_buffer, vb.queue);
  vfe_set_addr(dev,buf);
  if (dev->is_isp_used && dev->is_bayer_raw) {
    stat_buf_pt = list_entry(isp_stat_bq->active.next, struct vfe_isp_stat_buf, queue);
	if(NULL == stat_buf_pt){
		vfe_err("stat_buf_pt =null");
	}else{
		bsp_isp_set_statistics_addr((unsigned int)(stat_buf_pt->dma_addr));
	}
  }

  if (dev->is_isp_used) {
  bsp_isp_set_para_ready();
    bsp_isp_clr_irq_status(ISP_IRQ_EN_ALL);
    bsp_isp_irq_enable(FINISH_INT_EN | SRC0_FIFO_INT_EN);
		if (dev->is_isp_used && dev->is_bayer_raw)
			bsp_csi_int_enable(dev->vip_sel, dev->cur_ch, CSI_INT_VSYNC_TRIG);
  } else {
    bsp_csi_int_clear_status(dev->vip_sel, dev->cur_ch,CSI_INT_ALL);
    bsp_csi_int_enable(dev->vip_sel, dev->cur_ch, CSI_INT_CAPTURE_DONE | \
                                                  CSI_INT_FRAME_DONE | \
                                                  CSI_INT_BUF_0_OVERFLOW | \
                                                  CSI_INT_BUF_1_OVERFLOW | \
                                                  CSI_INT_BUF_2_OVERFLOW | \
                                                  CSI_INT_HBLANK_OVERFLOW);
  }

#if defined (CONFIG_ARCH_SUN8IW8P1)
	if(dev->mbus_type == V4L2_MBUS_CSI2)
		bsp_mipi_csi_protocol_enable(dev->mipi_sel);
	usleep_range(10000,11000);

	if (dev->capture_mode == V4L2_MODE_IMAGE) {
		if (dev->is_isp_used)
			bsp_isp_image_capture_start();
		bsp_csi_cap_start(dev->vip_sel, dev->total_rx_ch,CSI_SCAP);
	} else {
		if (dev->is_isp_used)
			bsp_isp_video_capture_start();
		bsp_csi_cap_start(dev->vip_sel, dev->total_rx_ch,CSI_VCAP);
	}
#else
	if (dev->capture_mode == V4L2_MODE_IMAGE) {
		if (dev->is_isp_used)
			bsp_isp_image_capture_start();
		bsp_csi_cap_start(dev->vip_sel, dev->total_rx_ch,CSI_SCAP);
	} else {
		if (dev->is_isp_used)
			bsp_isp_video_capture_start();
		bsp_csi_cap_start(dev->vip_sel, dev->total_rx_ch,CSI_VCAP);
	}
	if(dev->mbus_type == V4L2_MBUS_CSI2)
		bsp_mipi_csi_protocol_enable(dev->mipi_sel);
#endif
	vfe_start_generating(dev);

streamon_unlock:
	//  spin_unlock(&dev->slock);//debug
  mutex_unlock(&dev->stream_lock);

  return ret;
}

static int vidioc_streamoff(struct file *file, void *priv, enum v4l2_buf_type i)
{
  struct vfe_dev *dev = video_drvdata(file);
  struct vfe_dmaqueue *dma_q = &dev->vidq;
  int ret = 0;

  mutex_lock(&dev->stream_lock);
  vfe_dbg(0,"video stream off\n");
  if (!vfe_is_generating(dev)) {
    vfe_err("stream has been already off\n");
    ret = 0;
    goto streamoff_unlock;
  }
 #ifdef _FLASH_FUNC_
	isp_streamoff_torch_and_flash_close(dev);
#endif
  vfe_stop_generating(dev);
  /* Resets frame counters */
  dev->ms = 0;
  dev->jiffies = jiffies;

  dma_q->frame = 0;
  dma_q->ini_jiffies = jiffies;

	if (dev->is_isp_used) {
		vfe_dbg(0,"disable isp int in streamoff\n");
		bsp_isp_irq_disable(ISP_IRQ_EN_ALL);
		bsp_isp_clr_irq_status(ISP_IRQ_EN_ALL);
	} else {
		vfe_dbg(0,"disable csi int in streamoff\n");
		bsp_csi_int_disable(dev->vip_sel, dev->cur_ch, CSI_INT_ALL);
		bsp_csi_int_clear_status(dev->vip_sel, dev->cur_ch, CSI_INT_ALL);
	}

	if (dev->capture_mode == V4L2_MODE_IMAGE) {
		bsp_csi_cap_stop(dev->vip_sel, dev->total_rx_ch,CSI_SCAP);
		if (dev->is_isp_used)
			bsp_isp_image_capture_stop();
		vfe_dbg(0,"dev->capture_mode = %d\n",dev->capture_mode);
	} else {
		bsp_csi_cap_stop(dev->vip_sel, dev->total_rx_ch,CSI_VCAP);
		if (dev->is_isp_used)
			bsp_isp_video_capture_stop();
		vfe_dbg(0,"dev->capture_mode = %d\n",dev->capture_mode);
	}
  if (i != V4L2_BUF_TYPE_VIDEO_CAPTURE) {
    ret =  -EINVAL;
    goto streamoff_unlock;
  }
  if(dev->mbus_type == V4L2_MBUS_CSI2)
    bsp_mipi_csi_protocol_disable(dev->mipi_sel);

  ret = videobuf_streamoff(&dev->vb_vidq);
  if (ret!=0) {
    vfe_err("videobu_streamoff error!\n");
    goto streamoff_unlock;
  }
  /* Release all active buffers */
  while (!list_empty(&dma_q->active)) {
    vfe_err("dma_q->active is not empty!\n");
  }
  //printk("%s, line: %d\n", __FUNCTION__, __LINE__);
  if(dev->is_isp_used)
    bsp_isp_disable();
  bsp_csi_disable(dev->vip_sel);
streamoff_unlock:
  //spin_unlock(&dev->slock);//debug
  mutex_unlock(&dev->stream_lock);

  return ret;
}

static int vidioc_enum_input(struct file *file, void *priv,
        struct v4l2_input *inp)
{
	struct vfe_dev *dev = video_drvdata(file);
	if (inp->index > dev->dev_qty-1) {
		vfe_err("input index(%d) > dev->dev_qty(%d)-1 invalid!\n", inp->index, dev->dev_qty);
		return -EINVAL;
	}
	if (0 == dev->device_valid_flag[inp->index]) {
		vfe_err("input index(%d) > dev->dev_qty(%d)-1 invalid!, device_valid_flag[%d] = %d\n", inp->index, dev->dev_qty,inp->index, dev->device_valid_flag[inp->index]);
		return -EINVAL;
	}
	inp->type = V4L2_INPUT_TYPE_CAMERA;
	return 0;
}

static int vidioc_g_input(struct file *file, void *priv, unsigned int *i)
{
	struct vfe_dev *dev = video_drvdata(file);

	*i = dev->input;
	return 0;
}
static int vfe_ctrl_para_reset(struct vfe_dev *dev)
{
	dev->ctrl_para.gsensor_rot = 0;
	dev->ctrl_para.prev_exp_line = 16;
	dev->ctrl_para.prev_ana_gain = 16;
	dev->ctrl_para.prev_focus_pos = 50;
	return 0;
}

static int internal_s_input(struct vfe_dev *dev, unsigned int i)
{
	struct v4l2_control ctrl;
	struct sensor_item sensor_info;
	int ret;

	if (i > dev->dev_qty-1) {
		vfe_err("set input i(%d)>dev_qty(%d)-1 error!\n", i, dev->dev_qty);
		return -EINVAL;
	}

	if (i == dev->input)
		return 0;

	if(dev->input != -1) {
	/*Power down current device*/
		if(dev->sd_act!=NULL)
		{
			v4l2_subdev_call(dev->sd_act,core,ioctl,ACT_SOFT_PWDN,0);
		}
		if(dev->power->stby_mode == NORM_STBY)
		{
			ret = v4l2_subdev_call(dev->sd,core, s_power, CSI_SUBDEV_STBY_ON);
		}
		else //POWER_OFF
		{
			ret = vfe_set_sensor_power_off(dev);
		}
		if(ret < 0)
			goto altend;
	}
	vfe_dbg(0,"input_num = %d\n",i);

	dev->input = i;

	/* Alternate the device info and select target device*/
	update_ccm_info(dev, dev->ccm_cfg[i]);

	//set vfe core clk rate for each sensor!
	if(get_sensor_info(dev->ccm_cfg[i]->ccm, &sensor_info) == 0)
	{
		os_clk_set_rate(dev->clock.vfe_core_clk, sensor_info.core_clk_for_sensor);
		vfe_print("Set vfe core clk = %d, after Set vfe core clk = %ld \n",sensor_info.core_clk_for_sensor, clk_get_rate(dev->clock.vfe_core_clk));
	}
	else
	{
		os_clk_set_rate(dev->clock.vfe_core_clk, VFE_CORE_CLK_RATE);
		vfe_warn("Not find this sensor info, Set vfe core clk = %d, after Set vfe core clk = %ld \n",VFE_CORE_CLK_RATE, clk_get_rate(dev->clock.vfe_core_clk));
	}

	//alternate isp setting
	update_isp_setting(dev);
	if(dev->is_bayer_raw)
	{
		isp_param_init(dev->isp_gen_set_pt);
	}

#ifdef _FLASH_FUNC_
	if(dev->flash_used==1)
	{
		dev->fl_dev_info=&fl_info;
		dev->fl_dev_info->dev_if=0;
		dev->fl_dev_info->en_pol=FLASH_EN_POL;
		dev->fl_dev_info->fl_mode_pol=FLASH_MODE_POL;
		dev->fl_dev_info->light_src=0x01;
		dev->fl_dev_info->flash_intensity=400;
		dev->fl_dev_info->flash_level=0x01;
		dev->fl_dev_info->torch_intensity=200;
		dev->fl_dev_info->torch_level=0x01;
		dev->fl_dev_info->timeout_counter=300*1000;

		dev->fl_dev_info->flash_driver_ic = dev->flash_type;

		vfe_print("init flash mode[V4L2_FLASH_LED_MODE_NONE]\n");
		config_flash_mode(dev->sd, V4L2_FLASH_LED_MODE_FLASH,
		dev->fl_dev_info);
		io_set_flash_ctrl(dev->sd, SW_CTRL_FLASH_OFF, dev->fl_dev_info);
	}
#endif
	//vfe_mclk_out_set(dev,dev->ccm_info->mclk);
	/* Initial target device */
	if(dev->power->stby_mode == NORM_STBY)
	{
		ret = v4l2_subdev_call(dev->sd,core, s_power, CSI_SUBDEV_STBY_OFF);
	}
	else //POWER_OFF
	{
		ret = vfe_set_sensor_power_on(dev);
#ifdef USE_SPECIFIC_CCI
		csi_cci_init_helper(dev->vip_sel);
#endif
	}
	if (ret!=0) {
		vfe_err("sensor standby off error when selecting target device!\n");
		goto altend;
	}

	ret = v4l2_subdev_call(dev->sd,core, init, 0);
	if (ret!=0) {
		vfe_err("sensor initial error when selecting target device!\n");
		goto altend;
	}

	if(dev->sd_act!=NULL)
	{
		struct actuator_para_t vcm_para;
		vcm_para.active_min = dev->isp_gen_set_pt->isp_ini_cfg.isp_3a_settings.vcm_min_code;
		vcm_para.active_max = dev->isp_gen_set_pt->isp_ini_cfg.isp_3a_settings.vcm_max_code;
		vfe_dbg(0,"min/max=%d/%d \n", dev->isp_gen_set_pt->isp_ini_cfg.isp_3a_settings.vcm_min_code,
		dev->isp_gen_set_pt->isp_ini_cfg.isp_3a_settings.vcm_max_code);
		v4l2_subdev_call(dev->sd_act,core,ioctl,ACT_INIT,&vcm_para);
	}

	bsp_csi_disable(dev->vip_sel);
	if(dev->is_isp_used) {
		vfe_ctrl_para_reset(dev);
		bsp_isp_disable();
		bsp_isp_enable();
		bsp_isp_init(&dev->isp_init_para);
		/* Set the initial flip */
		if(dev->ctrl_para.vflip == 0)
		{
			bsp_isp_flip_disable(MAIN_CH);
			bsp_isp_flip_disable(SUB_CH);
		}
		else
		{
			bsp_isp_flip_enable(MAIN_CH);
			bsp_isp_flip_enable(SUB_CH);
		}
		if(dev->ctrl_para.hflip == 0)
		{
			bsp_isp_mirror_disable(MAIN_CH);
			bsp_isp_mirror_disable(SUB_CH);
		}
		else
		{
			bsp_isp_mirror_enable(MAIN_CH);
			bsp_isp_mirror_enable(SUB_CH);
		}
		dev->isp_gen_set_pt->hflip = dev->ctrl_para.hflip;
		dev->isp_gen_set_pt->vflip = dev->ctrl_para.vflip;
	} else {
		bsp_isp_exit();
		/* Set the initial flip */
		ctrl.id = V4L2_CID_VFLIP;
		ctrl.value = dev->ctrl_para.vflip;
		ret = v4l2_subdev_call(dev->sd,core, s_ctrl, &ctrl);
		if (ret!=0) {
			vfe_err("sensor sensor_s_ctrl V4L2_CID_VFLIP error when vidioc_s_input!input_num = %d\n",i);
		}

		ctrl.id = V4L2_CID_HFLIP;
		ctrl.value = dev->ctrl_para.hflip;
		ret = v4l2_subdev_call(dev->sd,core, s_ctrl, &ctrl);
		if (ret!=0) {
			vfe_err("sensor sensor_s_ctrl V4L2_CID_HFLIP error when vidioc_s_input!input_num = %d\n",i);
		}
	}

	ret = 0;
altend:
	dev->vfe_s_input_flag = 1;
	return ret;
}

static int vidioc_s_input(struct file *file, void *priv, unsigned int i)
{
	struct vfe_dev *dev = video_drvdata(file);
	int ret;

	vfe_dbg(0,"%s ,input_num = %d\n",__func__,i);
	ret = internal_s_input(dev , i);
	//up(&dev->standby_seq_sema);
	return ret;
}

static enum power_line_frequency v4l2_bf_to_common(enum v4l2_power_line_frequency band_stop_mode)
{
  return band_stop_mode;
}

static enum colorfx v4l2_colorfx_to_common(enum v4l2_colorfx colorfx)
{
  return colorfx;
}

static enum exposure_mode v4l2_ae_mode_to_common(enum v4l2_exposure_auto_type ae_mode)
{
  switch(ae_mode) {
    case V4L2_EXPOSURE_AUTO:
    case V4L2_EXPOSURE_SHUTTER_PRIORITY:
    case V4L2_EXPOSURE_APERTURE_PRIORITY:
      return EXP_AUTO;
    case V4L2_EXPOSURE_MANUAL:
      return EXP_MANUAL;
    default:
      return EXP_AUTO;
  }
}

static enum white_balance_mode v4l2_wb_preset_to_common(enum v4l2_auto_n_preset_white_balance wb_preset)
{
  return wb_preset;
}

static enum iso_mode v4l2_iso_mode_to_common(enum v4l2_iso_sensitivity_auto_type iso_mode)
{
  return iso_mode;
}

static enum scene_mode v4l2_scene_to_common(enum v4l2_scene_mode scene_mode)
{
  return scene_mode;
}

static enum auto_focus_range v4l2_af_range_to_common(enum v4l2_auto_focus_range af_range)
{
  return af_range;
}

static enum flash_mode v4l2_flash_mode_to_common(enum v4l2_flash_led_mode flash_mode)
{
	return flash_mode;
}

static int vidioc_queryctrl(struct file *file, void *priv,
          struct v4l2_queryctrl *qc)
{
  struct vfe_dev *dev = video_drvdata(file);
  int ret;
  if(dev->is_isp_used && dev->is_bayer_raw) {
    /* Fill in min, max, step and default value for these controls. */
    /* see include/linux/videodev2.h for details */
    /* see sensor_s_parm and sensor_g_parm for the meaning of value */
    switch (qc->id) {
      //V4L2_CID_BASE
      case V4L2_CID_BRIGHTNESS:
		return v4l2_ctrl_query_fill(qc, 0, 255, 1, 128);
      case V4L2_CID_CONTRAST:
		return v4l2_ctrl_query_fill(qc, 0, 128, 1, 0);
      case V4L2_CID_SATURATION:
        return v4l2_ctrl_query_fill(qc, -4, 4, 1, 0);
      case V4L2_CID_HUE:
        return v4l2_ctrl_query_fill(qc, -180, 180, 1, 0);
      case V4L2_CID_AUTO_WHITE_BALANCE:
        return v4l2_ctrl_query_fill(qc, 0, 1, 1, 1);
      case V4L2_CID_EXPOSURE:
        return v4l2_ctrl_query_fill(qc, 0, 65536*16, 1, 0);
	case V4L2_CID_HOR_VISUAL_ANGLE:
		return v4l2_ctrl_query_fill(qc, 0, 360, 1, 60);
	case V4L2_CID_VER_VISUAL_ANGLE:
		return v4l2_ctrl_query_fill(qc, 0, 360, 1, 60);
	case V4L2_CID_FOCUS_LENGTH:
		return v4l2_ctrl_query_fill(qc, 0, 1000, 1, 280);
	case V4L2_CID_R_GAIN:
		return v4l2_ctrl_query_fill(qc, 32, 1024, 1, 256);
	case V4L2_CID_G_GAIN:
		return v4l2_ctrl_query_fill(qc, 32, 1024, 1, 256);
	case V4L2_CID_B_GAIN:
		return v4l2_ctrl_query_fill(qc, 32, 1024, 1, 256);
      case V4L2_CID_AUTOGAIN:
        return v4l2_ctrl_query_fill(qc, 0, 1, 1, 1);
      case V4L2_CID_GAIN:
        return v4l2_ctrl_query_fill(qc, 0, 0x7fffffff, 1, 0);
      case V4L2_CID_VFLIP:
        ret = v4l2_ctrl_query_fill(qc, 0, 1, 1, 0);
        qc->default_value = dev->ctrl_para.vflip;
        return ret;
      case V4L2_CID_HFLIP:
        ret = v4l2_ctrl_query_fill(qc, 0, 1, 1, 0);
        qc->default_value = dev->ctrl_para.hflip;
        return ret;
      case V4L2_CID_POWER_LINE_FREQUENCY:
        return v4l2_ctrl_query_fill(qc, 0, 3, 1, 3);
      case V4L2_CID_HUE_AUTO:
        return v4l2_ctrl_query_fill(qc, 0, 1, 1, 1);
      case V4L2_CID_WHITE_BALANCE_TEMPERATURE:
        return v4l2_ctrl_query_fill(qc, 2800, 10000, 1, 6500);
      case V4L2_CID_SHARPNESS:
		return v4l2_ctrl_query_fill(qc, -32, 32, 1, 0);
      case V4L2_CID_CHROMA_AGC:
        return v4l2_ctrl_query_fill(qc, 0, 1, 1, 1);
      case V4L2_CID_COLORFX:
        return v4l2_ctrl_query_fill(qc, 0, 15, 1, 0);
      case V4L2_CID_AUTOBRIGHTNESS:
        return v4l2_ctrl_query_fill(qc, 0, 1, 1, 1);
      case V4L2_CID_BAND_STOP_FILTER:
        return v4l2_ctrl_query_fill(qc, 0, 1, 1, 1);
      case V4L2_CID_ILLUMINATORS_1:
      case V4L2_CID_ILLUMINATORS_2:
        return v4l2_ctrl_query_fill(qc, 0, 1, 1, 0);
      //V4L2_CID_CAMERA_CLASS_BASE
      case V4L2_CID_EXPOSURE_AUTO:
        return v4l2_ctrl_query_fill(qc, 0, 3, 1, 0);
      case V4L2_CID_EXPOSURE_ABSOLUTE:
        return v4l2_ctrl_query_fill(qc, 1, 1000000, 1, 1);
      case V4L2_CID_EXPOSURE_AUTO_PRIORITY:
        return v4l2_ctrl_query_fill(qc, 0, 1, 1, 0);
      case V4L2_CID_FOCUS_ABSOLUTE:
        return v4l2_ctrl_query_fill(qc, 0, 127, 1, 0);
      case V4L2_CID_FOCUS_RELATIVE:
        return v4l2_ctrl_query_fill(qc, -127, 127, 1, 0);
      case V4L2_CID_FOCUS_AUTO:
        return v4l2_ctrl_query_fill(qc, 0, 1, 1, 1);
      case V4L2_CID_AUTO_EXPOSURE_BIAS:
        return v4l2_ctrl_query_fill(qc, -4, 4, 1, 0);
      case V4L2_CID_AUTO_N_PRESET_WHITE_BALANCE:
        return v4l2_ctrl_query_fill(qc, 0, 10, 1, 1);
      case V4L2_CID_WIDE_DYNAMIC_RANGE:
        return v4l2_ctrl_query_fill(qc, 0, 1, 1, 0);
      case V4L2_CID_IMAGE_STABILIZATION:
        return v4l2_ctrl_query_fill(qc, 0, 1, 1, 0);
      case V4L2_CID_ISO_SENSITIVITY:
        return v4l2_ctrl_query_fill(qc, 500, 16000, 100, 1000);
      case V4L2_CID_ISO_SENSITIVITY_AUTO:
        return v4l2_ctrl_query_fill(qc, 0, 1, 1, 1);
      case V4L2_CID_EXPOSURE_METERING:
        return -EINVAL;
      case V4L2_CID_SCENE_MODE:
        return v4l2_ctrl_query_fill(qc, 0, 13, 1, 0);
      case V4L2_CID_3A_LOCK:
        return v4l2_ctrl_query_fill(qc, 0, 4, 1, 0);
      case V4L2_CID_AUTO_FOCUS_START:
        return v4l2_ctrl_query_fill(qc, 0, 0, 0, 0);
      case V4L2_CID_AUTO_FOCUS_STOP:
        return v4l2_ctrl_query_fill(qc, 0, 0, 0, 0);
      case V4L2_CID_AUTO_FOCUS_STATUS:  //Read-Only
        return v4l2_ctrl_query_fill(qc, 0, 0, 0, 0);
      case V4L2_CID_AUTO_FOCUS_RANGE:
        return v4l2_ctrl_query_fill(qc, 0, 3, 1, 0);
      //V4L2_CID_FLASH_CLASS_BASE
      case V4L2_CID_FLASH_LED_MODE:
        return v4l2_ctrl_query_fill(qc, 0, 4, 1, 0);
      //V4L2_CID_PRIVATE_BASE
      case V4L2_CID_HFLIP_THUMB:
        ret = v4l2_ctrl_query_fill(qc, 0, 1, 1, 0);
        qc->default_value = dev->ctrl_para.hflip_thumb;
        return ret;
      case V4L2_CID_VFLIP_THUMB:
        ret = v4l2_ctrl_query_fill(qc, 0, 1, 1, 0);
        qc->default_value = dev->ctrl_para.vflip_thumb;
        return ret;
      case V4L2_CID_AUTO_FOCUS_WIN_NUM:
        return v4l2_ctrl_query_fill(qc, 0, 10, 1, 0);
      case V4L2_CID_AUTO_FOCUS_INIT:
      case V4L2_CID_AUTO_FOCUS_RELEASE:
        return v4l2_ctrl_query_fill(qc, 0, 0, 0, 0);
      case V4L2_CID_AUTO_EXPOSURE_WIN_NUM:
        return v4l2_ctrl_query_fill(qc, 0, 10, 1, 0);
      case V4L2_CID_GSENSOR_ROTATION:
        return v4l2_ctrl_query_fill(qc, -180, 180, 90, 0);
	case V4L2_CID_TAKE_PICTURE:
		return v4l2_ctrl_query_fill(qc, 0, 16, 1, 1);
	case V4L2_CID_HDR:
		return v4l2_ctrl_query_fill(qc, 0, 10, 1, 0);
	case V4L2_CID_SENSOR_TYPE:
	        return v4l2_ctrl_query_fill(qc, 0, 1, 1, 0);
    }
    return -EINVAL;
  } else {
    if(qc->id == V4L2_CID_FOCUS_ABSOLUTE)
    { //TODO!
    return v4l2_ctrl_query_fill(qc, 0, 127, 1, 0);
    }
    else if(qc->id == V4L2_CID_SENSOR_TYPE)
    {
		return v4l2_ctrl_query_fill(qc, 0, 1, 1, 0);
    }
    ret = v4l2_subdev_call(dev->sd,core,queryctrl,qc);
    if (ret < 0)
    {
      if(qc->id != V4L2_CID_GAIN)
      {
        vfe_warn("v4l2 sub device queryctrl %s unsuccess!\n", v4l2_ctrl_get_name(qc->id));
      }
    }
  }

  return ret;
}

static int vidioc_g_ctrl(struct file *file, void *priv,
       struct v4l2_control *ctrl)
{
  struct vfe_dev *dev = video_drvdata(file);
  struct v4l2_queryctrl qc;
  int ret;
  unsigned int i;
  qc.id = ctrl->id;
  ret = vidioc_queryctrl(file, priv, &qc);
  if (ret < 0)
  {
    return ret;
  }

  if(dev->is_isp_used && dev->is_bayer_raw) {
    switch (ctrl->id) {
      //V4L2_CID_BASE
      case V4L2_CID_BRIGHTNESS:
		ctrl->value = dev->isp_gen_set_pt->ae_lum/100;
        break;
      case V4L2_CID_CONTRAST:
        ctrl->value = dev->ctrl_para.contrast;
        break;
      case V4L2_CID_SATURATION:
        ctrl->value = dev->ctrl_para.saturation;
        break;
      case V4L2_CID_HUE:
        ctrl->value = dev->ctrl_para.hue;
        break;
	case V4L2_CID_HOR_VISUAL_ANGLE:
		ctrl->value = dev->isp_gen_set_pt->isp_ini_cfg.isp_tunning_settings.hor_visual_angle;
		break;
	case V4L2_CID_VER_VISUAL_ANGLE:
		ctrl->value = dev->isp_gen_set_pt->isp_ini_cfg.isp_tunning_settings.ver_visual_angle;
		break;
	case V4L2_CID_FOCUS_LENGTH:
		ctrl->value = dev->isp_gen_set_pt->isp_ini_cfg.isp_tunning_settings.focus_length;
		break;
	  case V4L2_CID_AUTO_WHITE_BALANCE:
		ctrl->value = dev->ctrl_para.auto_wb;
		break;
	  case V4L2_CID_EXPOSURE:
	  	ctrl->value = dev->isp_3a_result_pt->exp_line_num;
		break;
	  case V4L2_CID_AUTOGAIN:
		ctrl->value = dev->ctrl_para.auto_gain;
		break;
	  case V4L2_CID_GAIN:
	  	if(dev->isp_gen_set_pt->isp_ini_cfg.isp_test_settings.isp_test_mode == ISP_TEST_ALL_ENABLE ||
	  				dev->isp_gen_set_pt->isp_ini_cfg.isp_test_settings.isp_test_mode == ISP_TEST_MANUAL) {
		  	ctrl->value = CLIP(CLIP(dev->isp_3a_result_pt->exp_analog_gain, 16, 255) |
		  				(CLIP(dev->isp_gen_set_pt->sharp_cfg_to_hal[1], 0, 4095)/*level*/ << V4L2_SHARP_LEVEL_SHIFT)  |
		  				(CLIP(dev->isp_gen_set_pt->sharp_cfg_to_hal[0], 0, 63)/*min*/ << V4L2_SHARP_MIN_SHIFT) |
		  				(CLIP(dev->isp_gen_set_pt->isp_ini_cfg.isp_tunning_settings.color_denoise_level, 0, 31)  << V4L2_NDF_SHIFT) ,0,0xffffffff);
	  	}else{
		  	ctrl->value = CLIP(dev->isp_3a_result_pt->exp_analog_gain, 16, 255);
	  	}
		break;
	case V4L2_CID_R_GAIN:
		ctrl->value = dev->isp_gen_set_pt->module_cfg.wb_gain_cfg.wb_gain.r_gain;
		break;
	case V4L2_CID_G_GAIN:
		ctrl->value = dev->isp_gen_set_pt->module_cfg.wb_gain_cfg.wb_gain.gr_gain;
		break;
	case V4L2_CID_B_GAIN:
		ctrl->value = dev->isp_gen_set_pt->module_cfg.wb_gain_cfg.wb_gain.b_gain;
		break;
	  case V4L2_CID_HFLIP:
		ctrl->value = dev->ctrl_para.hflip;
        break;
      case V4L2_CID_VFLIP:
        ctrl->value = dev->ctrl_para.vflip;
        break;
      case V4L2_CID_POWER_LINE_FREQUENCY:
        ctrl->value = dev->ctrl_para.band_stop_mode;
        break;
      case V4L2_CID_HUE_AUTO:
        ctrl->value = dev->ctrl_para.auto_hue;
        break;
      case V4L2_CID_WHITE_BALANCE_TEMPERATURE:
        ctrl->value = dev->ctrl_para.wb_temperature;
        break;
      case V4L2_CID_SHARPNESS:
        ctrl->value = dev->ctrl_para.sharpness;
        break;
      case V4L2_CID_CHROMA_AGC:
        ctrl->value = dev->ctrl_para.chroma_agc;
        break;
      case V4L2_CID_COLORFX:
        ctrl->value = dev->ctrl_para.colorfx;
        break;
      case V4L2_CID_AUTOBRIGHTNESS:
        ctrl->value = dev->ctrl_para.auto_brightness;
        break;
      case V4L2_CID_BAND_STOP_FILTER:
        ctrl->value = dev->ctrl_para.band_stop_filter;
        break;
      case V4L2_CID_ILLUMINATORS_1:
        ctrl->value = dev->ctrl_para.illuminator1;
        break;
      case V4L2_CID_ILLUMINATORS_2:
        ctrl->value = dev->ctrl_para.illuminator2;
        break;
      //V4L2_CID_CAMERA_CLASS_BASE
      case V4L2_CID_EXPOSURE_AUTO:
        ctrl->value = dev->ctrl_para.exp_auto_mode;
        break;
      case V4L2_CID_EXPOSURE_ABSOLUTE:
		ctrl->value = dev->isp_3a_result_pt->exp_time;
        break;
      case V4L2_CID_EXPOSURE_AUTO_PRIORITY:
        ctrl->value = dev->ctrl_para.exp_auto_pri;
        break;
      case V4L2_CID_FOCUS_ABSOLUTE:
        ctrl->value = CLIP(((dev->isp_3a_result_pt->real_vcm_pos - dev->isp_gen_set_pt->stat.vcm_cfg.vcm_min_code ) << 10) / 
        			(dev->isp_gen_set_pt->stat.vcm_cfg.vcm_max_code - 
        			dev->isp_gen_set_pt->stat.vcm_cfg.vcm_min_code ), 0, 1023);
        break;
      case V4L2_CID_FOCUS_RELATIVE:
        ctrl->value = dev->ctrl_para.focus_rel;
        break;
      case V4L2_CID_FOCUS_AUTO:
        ctrl->value = dev->ctrl_para.auto_focus;
        break;
      case V4L2_CID_AUTO_EXPOSURE_BIAS:
        ctrl->value = dev->ctrl_para.exp_bias;
        break;
      case V4L2_CID_AUTO_N_PRESET_WHITE_BALANCE:
        ctrl->value = dev->ctrl_para.wb_preset;
        break;
      case V4L2_CID_WIDE_DYNAMIC_RANGE:
        ctrl->value = dev->ctrl_para.wdr;
        break;
      case V4L2_CID_IMAGE_STABILIZATION:
        ctrl->value = dev->ctrl_para.image_stabl;
        break;
      case V4L2_CID_ISO_SENSITIVITY:
        ctrl->value = dev->ctrl_para.iso;
        break;
      case V4L2_CID_ISO_SENSITIVITY_AUTO:
        ctrl->value = dev->ctrl_para.iso_mode;
        break;
      case V4L2_CID_EXPOSURE_METERING:
        return -EINVAL;
      case V4L2_CID_SCENE_MODE:
        ctrl->value = dev->ctrl_para.scene_mode;
        break;
      case V4L2_CID_3A_LOCK:
        if(dev->isp_gen_set_pt->exp_settings.exposure_lock == ISP_TRUE) {
          dev->ctrl_para.ae_lock = 1;
          ctrl->value |= V4L2_LOCK_EXPOSURE;
        } else {
          dev->ctrl_para.ae_lock = 0;
          ctrl->value &= ~V4L2_LOCK_EXPOSURE;
        }

        if(dev->isp_gen_set_pt->wb_settings.white_balance_lock == ISP_TRUE) {
          dev->ctrl_para.awb_lock = 1;
          ctrl->value |= V4L2_LOCK_WHITE_BALANCE;
        } else {
          dev->ctrl_para.awb_lock = 0;
          ctrl->value &= ~V4L2_LOCK_WHITE_BALANCE;
        }

        if(dev->isp_gen_set_pt->af_settings.focus_lock == ISP_TRUE) {
          dev->ctrl_para.af_lock = 1;
          ctrl->value |= V4L2_LOCK_FOCUS;
        } else {
          dev->ctrl_para.af_lock = 0;
          ctrl->value &= ~V4L2_LOCK_FOCUS;
        }
        break;
      case V4L2_CID_AUTO_FOCUS_START:   //write-only
        return -EINVAL;
      case V4L2_CID_AUTO_FOCUS_STOP:    //write-only
        return -EINVAL;
      case V4L2_CID_AUTO_FOCUS_STATUS:  //Read-Only
        //TODO
        ret=common_af_status_to_v4l2(dev->isp_3a_result_pt->af_status);
        return ret;
        break;
      case V4L2_CID_AUTO_FOCUS_RANGE:
        ctrl->value = dev->ctrl_para.af_range;
        break;
      //V4L2_CID_FLASH_CLASS_BASE
      case V4L2_CID_FLASH_LED_MODE:
        ctrl->value = dev->ctrl_para.flash_mode;
        break;
      //V4L2_CID_PRIVATE_BASE
      case V4L2_CID_HFLIP_THUMB:
        ctrl->value = dev->ctrl_para.hflip_thumb;
        break;
      case V4L2_CID_VFLIP_THUMB:
        ctrl->value = dev->ctrl_para.vflip_thumb;
        break;
      case V4L2_CID_AUTO_FOCUS_WIN_NUM:
        if(ctrl->value > MAX_AF_WIN_NUM || ctrl->value == 0) {
          return -EINVAL;
        } else {
          struct v4l2_win_coordinate *win_coor = (struct v4l2_win_coordinate *)(ctrl->user_pt);
          for(i = 0; i < ctrl->value; i++) {
            win_coor[i].x1 = dev->ctrl_para.af_coor[i].x1;
            win_coor[i].y1 = dev->ctrl_para.af_coor[i].y1;
            win_coor[i].x2 = dev->ctrl_para.af_coor[i].x2;
            win_coor[i].y2 = dev->ctrl_para.af_coor[i].y2;
          }
        }
        ctrl->value = dev->ctrl_para.af_win_num;
        break;
      case V4L2_CID_AUTO_FOCUS_INIT:
        return 0;
      case V4L2_CID_AUTO_FOCUS_RELEASE:
        return 0;
      case V4L2_CID_AUTO_EXPOSURE_WIN_NUM:
        if(ctrl->value > MAX_AE_WIN_NUM || ctrl->value == 0) {
          return -EINVAL;
        } else {
          struct v4l2_win_coordinate *win_coor = (struct v4l2_win_coordinate *)(ctrl->user_pt);
          for(i = 0; i < ctrl->value; i++) {
            win_coor[i].x1 = dev->ctrl_para.ae_coor[i].x1;
            win_coor[i].y1 = dev->ctrl_para.ae_coor[i].y1;
            win_coor[i].x2 = dev->ctrl_para.ae_coor[i].x2;
            win_coor[i].y2 = dev->ctrl_para.ae_coor[i].y2;
          }
        }
        ctrl->value = dev->ctrl_para.ae_win_num;
        break;
      case V4L2_CID_GSENSOR_ROTATION:
        ctrl->value = dev->ctrl_para.gsensor_rot;
        break;
      case V4L2_CID_TAKE_PICTURE:   //write-only
        return -EINVAL;
	case V4L2_CID_HDR:
	{
		struct isp_hdr_setting_t hdr_setting;
		ctrl->value = dev->isp_gen_set_pt->hdr_setting.frames_count - 1;
		memcpy(&hdr_setting,&dev->isp_gen_set_pt->hdr_setting,sizeof(struct isp_hdr_setting_t));
		ctrl->user_pt = (unsigned int)&hdr_setting;
		break;
	}
	case V4L2_CID_SENSOR_TYPE:
		ctrl->value = dev->is_bayer_raw;
		break;
      default:
        return -EINVAL;
    }
    return 0;
  } else {
  	if(V4L2_CID_SENSOR_TYPE == ctrl->id)
  	{
		ctrl->value = dev->is_bayer_raw;
  	}
  	else
  	{
		ret = v4l2_subdev_call(dev->sd,core,g_ctrl,ctrl);
		if (ret < 0)
			vfe_warn("v4l2 sub device g_ctrl error!\n");
  	}
  }
  return ret;
}

static int vidioc_s_ctrl(struct file *file, void *priv,
        struct v4l2_control *ctrl)
{
  struct vfe_dev *dev = video_drvdata(file);
  struct v4l2_queryctrl qc;
  unsigned int i;
  int ret;
  //TO DO!
  struct actuator_ctrl_word_t vcm_ctrl;

  qc.id = ctrl->id;
  ret = vidioc_queryctrl(file, priv, &qc);

  //printk("vidioc_queryctrl!\n ret = %d",ret);
  if (ret < 0)
    return ret;

  if (qc.type == V4L2_CTRL_TYPE_MENU || qc.type == V4L2_CTRL_TYPE_INTEGER \
                                     || qc.type == V4L2_CTRL_TYPE_BOOLEAN) {
    if (ctrl->value < qc.minimum || ctrl->value > qc.maximum) {

      vfe_warn("value: %d, min: %d, max: %d\n ", ctrl->value, qc.minimum, qc.maximum);
      return -ERANGE;
    }
  }

  if(dev->is_isp_used && dev->is_bayer_raw) {
	vfe_dbg(0,"s_ctrl: %s, value: 0x%x\n",v4l2_ctrl_get_name(ctrl->id),ctrl->value);
    switch (ctrl->id) {
      //V4L2_CID_BASE
      case V4L2_CID_BRIGHTNESS:
#ifdef CONFIG_ARCH_SUN8IW8P1
#else
		bsp_isp_s_brightness(dev->isp_gen_set_pt, ctrl->value);

#endif
		dev->ctrl_para.brightness = ctrl->value;
		break;
	  case V4L2_CID_CONTRAST:
		bsp_isp_s_contrast(dev->isp_gen_set_pt, ctrl->value);
        dev->ctrl_para.contrast = ctrl->value;
        break;
      case V4L2_CID_SATURATION:
        bsp_isp_s_saturation(dev->isp_gen_set_pt, ctrl->value * 25);
        dev->ctrl_para.saturation = ctrl->value;
        break;
      case V4L2_CID_HUE:
        bsp_isp_s_hue(dev->isp_gen_set_pt, ctrl->value );
        dev->ctrl_para.hue = ctrl->value;
        break;
      case V4L2_CID_AUTO_WHITE_BALANCE:
        if(ctrl->value == 0)
          bsp_isp_s_auto_white_balance(dev->isp_gen_set_pt, WB_MANUAL);
        else
          bsp_isp_s_auto_white_balance(dev->isp_gen_set_pt, WB_AUTO);
        dev->ctrl_para.auto_wb = ctrl->value;
        break;
      case V4L2_CID_EXPOSURE:
        return v4l2_subdev_call(dev->sd,core,s_ctrl,ctrl);
      case V4L2_CID_AUTOGAIN:
        if(ctrl->value == 0)
          bsp_isp_s_exposure(dev->isp_gen_set_pt, ISO_MANUAL);
        else
          bsp_isp_s_exposure(dev->isp_gen_set_pt, ISO_AUTO);
        dev->ctrl_para.auto_gain = ctrl->value;
        break;
      case V4L2_CID_GAIN:
        return v4l2_subdev_call(dev->sd,core,s_ctrl,ctrl);
      case V4L2_CID_HFLIP:
		if(ctrl->value == 0){
			bsp_isp_mirror_disable(MAIN_CH);
			bsp_isp_mirror_disable(SUB_CH);

		}else{
			bsp_isp_mirror_enable(MAIN_CH);
			bsp_isp_mirror_enable(SUB_CH);
		}
		dev->isp_gen_set_pt->hflip = ctrl->value;
		dev->ctrl_para.hflip = ctrl->value;
		break;
      case V4L2_CID_VFLIP:
		if(ctrl->value == 0) {
			bsp_isp_flip_disable(MAIN_CH);
			bsp_isp_flip_disable(SUB_CH);
		} else {
			bsp_isp_flip_enable(MAIN_CH);
			bsp_isp_flip_enable(SUB_CH);
		}
		dev->isp_gen_set_pt->vflip = ctrl->value;
		dev->ctrl_para.vflip = ctrl->value;
		if(dev->is_isp_used) {
			bsp_isp_set_output_addr(dev->isp_gen_set_pt->output_addr);
		}
		break;
      case V4L2_CID_POWER_LINE_FREQUENCY:
        bsp_isp_s_power_line_frequency(dev->isp_gen_set_pt, v4l2_bf_to_common(ctrl->value));
        dev->ctrl_para.band_stop_mode = ctrl->value;
        break;
      case V4L2_CID_HUE_AUTO:
        bsp_isp_s_hue_auto(dev->isp_gen_set_pt, ctrl->value);
        dev->ctrl_para.auto_hue = ctrl->value;
        break;
      case V4L2_CID_WHITE_BALANCE_TEMPERATURE:
        bsp_isp_s_white_balance_temperature(dev->isp_gen_set_pt, ctrl->value);
        dev->ctrl_para.wb_temperature = ctrl->value;
        break;
      case V4L2_CID_SHARPNESS:
		bsp_isp_s_sharpness(dev->isp_gen_set_pt, ctrl->value);
        dev->ctrl_para.sharpness = ctrl->value;
        break;
      case V4L2_CID_CHROMA_AGC:
        bsp_isp_s_chroma_agc(dev->isp_gen_set_pt, ctrl->value);
        dev->ctrl_para.chroma_agc = ctrl->value;
        break;
      case V4L2_CID_COLORFX:
        bsp_isp_s_colorfx(dev->isp_gen_set_pt, v4l2_colorfx_to_common(ctrl->value));
        dev->ctrl_para.colorfx = ctrl->value;
        break;
      case V4L2_CID_AUTOBRIGHTNESS:
        bsp_isp_s_auto_brightness(dev->isp_gen_set_pt, ctrl->value);
        dev->ctrl_para.auto_brightness = ctrl->value;
        break;
      case V4L2_CID_BAND_STOP_FILTER:
        bsp_isp_s_band_stop_filter(dev->isp_gen_set_pt, ctrl->value);
        dev->ctrl_para.band_stop_filter = ctrl->value;
        break;
      case V4L2_CID_ILLUMINATORS_1:
        bsp_isp_s_illuminators_1(dev->isp_gen_set_pt, ctrl->value);
        dev->ctrl_para.illuminator1 = ctrl->value;
        break;
      case V4L2_CID_ILLUMINATORS_2:
        bsp_isp_s_illuminators_2(dev->isp_gen_set_pt, ctrl->value);
        dev->ctrl_para.illuminator2 = ctrl->value;
        break;
      //V4L2_CID_CAMERA_CLASS_BASE
      case V4L2_CID_EXPOSURE_AUTO:
        bsp_isp_s_exposure_auto(dev->isp_gen_set_pt, v4l2_ae_mode_to_common(ctrl->value));
        dev->ctrl_para.exp_auto_mode = ctrl->value;
        break;
      case V4L2_CID_EXPOSURE_ABSOLUTE:
        bsp_isp_s_exposure_absolute(dev->isp_gen_set_pt, ctrl->value);
        dev->ctrl_para.exp_abs = ctrl->value;
        break;
      case V4L2_CID_EXPOSURE_AUTO_PRIORITY:
        bsp_isp_s_exposure_auto_priority(dev->isp_gen_set_pt, ctrl->value);
        dev->ctrl_para.exp_auto_pri = ctrl->value;
        break;
      case V4L2_CID_FOCUS_ABSOLUTE:
        bsp_isp_s_focus_absolute(dev->isp_gen_set_pt, ctrl->value);
        dev->ctrl_para.focus_abs = ctrl->value;
        break;
      case V4L2_CID_FOCUS_RELATIVE:
        bsp_isp_s_focus_relative(dev->isp_gen_set_pt, ctrl->value);
        dev->ctrl_para.focus_rel = ctrl->value;
        break;
      case V4L2_CID_FOCUS_AUTO:
      //printk("set V4L2_CID_FOCUS_AUTO ctrl->value = %d!\n",ctrl->value);
          dev->isp_3a_result_pt->af_status = AUTO_FOCUS_STATUS_REFOCUS;
        bsp_isp_s_focus_auto(dev->isp_gen_set_pt, ctrl->value);
        dev->ctrl_para.auto_focus = ctrl->value;
        break;
      case V4L2_CID_AUTO_EXPOSURE_BIAS:
        bsp_isp_s_auto_exposure_bias(dev->isp_gen_set_pt, ctrl->value);
      //printk("ctrl->value=%d\n",ctrl->value);
        dev->ctrl_para.exp_bias = ctrl->value;
        break;
      case V4L2_CID_AUTO_N_PRESET_WHITE_BALANCE:
        bsp_isp_s_auto_n_preset_white_balance(dev->isp_gen_set_pt, v4l2_wb_preset_to_common(ctrl->value));
        dev->ctrl_para.wb_preset = ctrl->value;
        break;
      case V4L2_CID_WIDE_DYNAMIC_RANGE:
        bsp_isp_s_wide_dynamic_rage(dev->isp_gen_set_pt, ctrl->value);
        dev->ctrl_para.wdr = ctrl->value;
        break;
      case V4L2_CID_IMAGE_STABILIZATION:
        bsp_isp_s_image_stabilization(dev->isp_gen_set_pt, ctrl->value);
        dev->ctrl_para.image_stabl = ctrl->value;
        break;
      case V4L2_CID_ISO_SENSITIVITY:
        bsp_isp_s_iso_sensitivity(dev->isp_gen_set_pt, ctrl->value);
        dev->ctrl_para.iso = ctrl->value;
        break;
      case V4L2_CID_ISO_SENSITIVITY_AUTO:
        bsp_isp_s_iso_sensitivity_auto(dev->isp_gen_set_pt, v4l2_iso_mode_to_common(ctrl->value));
        dev->ctrl_para.iso_mode = ctrl->value;
        break;
      case V4L2_CID_EXPOSURE_METERING:
        return -EINVAL;
      case V4L2_CID_SCENE_MODE:
        bsp_isp_s_scene_mode(dev->isp_gen_set_pt, v4l2_scene_to_common(ctrl->value));
        dev->ctrl_para.scene_mode = ctrl->value;
        break;
      case V4L2_CID_3A_LOCK:
        //printk("V4L2_CID_3A_LOCK = %x!\n",ctrl->value);
        if(dev->ctrl_para.exp_auto_mode != V4L2_EXPOSURE_MANUAL) {
          if(IS_FLAG(ctrl->value,V4L2_LOCK_EXPOSURE)) {
            dev->isp_gen_set_pt->exp_settings.exposure_lock = ISP_TRUE;
            dev->ctrl_para.ae_lock = 1;
          } else {
            dev->isp_gen_set_pt->exp_settings.exposure_lock = ISP_FALSE;
            dev->ctrl_para.ae_lock = 0;
          }
        }
        if(dev->ctrl_para.auto_wb == 1) {
          if(IS_FLAG(ctrl->value,V4L2_LOCK_WHITE_BALANCE)) {
            dev->isp_gen_set_pt->wb_settings.white_balance_lock = ISP_TRUE;
            dev->ctrl_para.awb_lock = 1;
          } else {
            dev->isp_gen_set_pt->wb_settings.white_balance_lock = ISP_FALSE;
            dev->ctrl_para.awb_lock = 0;
          }
        }
        if(dev->ctrl_para.auto_focus == 1) {
          //printk("V4L2_CID_3A_LOCK AF!\n");
          if(IS_FLAG(ctrl->value,V4L2_LOCK_FOCUS)) {
            dev->isp_gen_set_pt->af_settings.focus_lock = ISP_TRUE;
            dev->ctrl_para.af_lock = 1;
          } else {
            dev->isp_gen_set_pt->af_settings.focus_lock = ISP_FALSE;
            dev->ctrl_para.af_lock = 0;
          }
        }
        break;
      case V4L2_CID_AUTO_FOCUS_START:
        dev->isp_3a_result_pt->af_status = AUTO_FOCUS_STATUS_REFOCUS;
        bsp_isp_s_auto_focus_start(dev->isp_gen_set_pt, ctrl->value);
#ifdef _FLASH_FUNC_
	isp_s_ctrl_torch_open(dev);
#endif
		break;
	  case V4L2_CID_AUTO_FOCUS_STOP:
		//if(dev->ctrl_para.auto_focus == 0)

		vfe_dbg(0,"V4L2_CID_AUTO_FOCUS_STOP\n");
		bsp_isp_s_auto_focus_stop(dev->isp_gen_set_pt, ctrl->value);
#ifdef _FLASH_FUNC_
		isp_s_ctrl_torch_close(dev);
#endif
		break;
      case V4L2_CID_AUTO_FOCUS_STATUS:  //Read-Only
        return -EINVAL;
      case V4L2_CID_AUTO_FOCUS_RANGE:
        bsp_isp_s_auto_focus_range(dev->isp_gen_set_pt, v4l2_af_range_to_common(ctrl->value));
        dev->ctrl_para.af_range = ctrl->value;
        break;
      //V4L2_CID_FLASH_CLASS_BASE
      case V4L2_CID_FLASH_LED_MODE:
        bsp_isp_s_flash_mode(dev->isp_gen_set_pt, v4l2_flash_mode_to_common(ctrl->value));
        dev->ctrl_para.flash_mode = ctrl->value;
        if(ctrl->value == V4L2_FLASH_LED_MODE_TORCH)
        {
		  io_set_flash_ctrl(dev->sd, SW_CTRL_TORCH_ON, dev->fl_dev_info);
        }
        else if(ctrl->value == V4L2_FLASH_LED_MODE_NONE)
        {
          io_set_flash_ctrl(dev->sd, SW_CTRL_FLASH_OFF, dev->fl_dev_info);
        }
        break;
      //V4L2_CID_PRIVATE_BASE
      case V4L2_CID_HFLIP_THUMB:
		dev->ctrl_para.hflip_thumb = ctrl->value;
		break;
      case V4L2_CID_VFLIP_THUMB:
		dev->ctrl_para.vflip_thumb = ctrl->value;
		break;
      case V4L2_CID_AUTO_FOCUS_WIN_NUM:
        if(ctrl->value == 0) {
          bsp_isp_s_auto_focus_win_num(dev->isp_gen_set_pt, AF_AUTO_WIN, NULL);
        } else if(ctrl->value > MAX_AF_WIN_NUM) {
          return -EINVAL;
        } else {
		struct v4l2_win_coordinate *win_coor = (struct v4l2_win_coordinate *)(ctrl->user_pt);
		for(i = 0; i < ctrl->value; i++)
		{
			if(dev->ctrl_para.vflip == 1)
			{
				dev->ctrl_para.af_coor[i].y1 = - win_coor[0].y1;
				dev->ctrl_para.af_coor[i].y2 = - win_coor[0].y2;
			}
			else
			{
				dev->ctrl_para.af_coor[i].y1 = win_coor[i].y1;
				dev->ctrl_para.af_coor[i].y2 = win_coor[i].y2;
			}
			if(dev->ctrl_para.hflip == 1)
			{
				dev->ctrl_para.af_coor[i].x1 = - win_coor[0].x1;
				dev->ctrl_para.af_coor[i].x2 = - win_coor[0].x2;
			}
			else
			{
				dev->ctrl_para.af_coor[i].x1 = win_coor[0].x1;
				dev->ctrl_para.af_coor[i].x2 = win_coor[0].x2;
			}
		}
		bsp_isp_s_auto_focus_win_num(dev->isp_gen_set_pt, AF_NUM_WIN, &dev->ctrl_para.af_coor[0]);
        }
        dev->ctrl_para.af_win_num = ctrl->value;
        break;
      case V4L2_CID_AUTO_FOCUS_INIT:
        return 0;
      case V4L2_CID_AUTO_FOCUS_RELEASE:
        return 0;
      case V4L2_CID_AUTO_EXPOSURE_WIN_NUM:

        if(ctrl->value == 0) {
          bsp_isp_s_auto_exposure_win_num(dev->isp_gen_set_pt, AE_AUTO_WIN, NULL);
        } else if(ctrl->value > MAX_AE_WIN_NUM) {
          return -EINVAL;
        } else {
		struct v4l2_win_coordinate *win_coor = (struct v4l2_win_coordinate *)(ctrl->user_pt);
		for(i = 0; i < ctrl->value; i++)
		{
			if(dev->ctrl_para.vflip == 1)
			{
				dev->ctrl_para.ae_coor[i].y1 = - win_coor[0].y1;
				dev->ctrl_para.ae_coor[i].y2 = - win_coor[0].y2;
			}
			else
			{
				dev->ctrl_para.ae_coor[i].y1 = win_coor[i].y1;
				dev->ctrl_para.ae_coor[i].y2 = win_coor[i].y2;
			}
			if(dev->ctrl_para.hflip == 1)
			{
				dev->ctrl_para.ae_coor[i].x1 = - win_coor[0].x1;
				dev->ctrl_para.ae_coor[i].x2 = - win_coor[0].x2;
			}
			else
			{
				dev->ctrl_para.ae_coor[i].x1 = win_coor[0].x1;
				dev->ctrl_para.ae_coor[i].x2 = win_coor[0].x2;
			}
			//printk("V4L2_CID_AUTO_EXPOSURE_WIN_NUM, [%d, %d, %d, %d]\n", win_coor[i].x1, win_coor[i].y1, win_coor[i].x2, win_coor[i].y2);
		}
          //TODO
          dev->isp_gen_set_pt->win.hist_coor.x1 = win_coor[0].x1;
          dev->isp_gen_set_pt->win.hist_coor.y1 = win_coor[0].y1;
          dev->isp_gen_set_pt->win.hist_coor.x2 = win_coor[0].x2;
          dev->isp_gen_set_pt->win.hist_coor.y2 = win_coor[0].y2;
          bsp_isp_s_auto_exposure_win_num(dev->isp_gen_set_pt, AE_SINGLE_WIN, &dev->ctrl_para.ae_coor[0]);

        }
        dev->ctrl_para.ae_win_num = ctrl->value;
    /**/
        break;
      case V4L2_CID_GSENSOR_ROTATION:
        bsp_isp_s_gsensor_rotation(dev->isp_gen_set_pt, ctrl->value);
        dev->ctrl_para.gsensor_rot = ctrl->value;
        break;
		case V4L2_CID_TAKE_PICTURE:
			bsp_isp_s_take_pic(dev->isp_gen_set_pt,ctrl->value);
			break;
		  case V4L2_CID_HDR:
		  	//struct isp_hdr_setting_t hdr;
		        bsp_isp_s_hdr(dev->isp_gen_set_pt, (struct hdr_setting_t *)(ctrl->user_pt));
		        dev->isp_3a_result_pt->image_quality.bits.hdr_cnt = 0;
		  	break;
		case V4L2_CID_R_GAIN:
			bsp_isp_s_r_gain(dev->isp_gen_set_pt, ctrl->value);
			break;
		case V4L2_CID_G_GAIN:
			bsp_isp_s_g_gain(dev->isp_gen_set_pt, ctrl->value);
			break;
		case V4L2_CID_B_GAIN:
			bsp_isp_s_b_gain(dev->isp_gen_set_pt, ctrl->value);
			break;
	  default:
		return -EINVAL;
	}
	return 0;
  } else {

    if(ctrl->id == V4L2_CID_FOCUS_ABSOLUTE)
    {
      //TO DO !
      vcm_ctrl.code = ctrl->value;
      vcm_ctrl.sr = 0x0;
      //printk("ACT_SET_CODE start code = %d!\n",vcm_ctrl.code);
        v4l2_subdev_call(dev->sd_act,core,ioctl,ACT_SET_CODE,&vcm_ctrl);
    }

    ret = v4l2_subdev_call(dev->sd,core,s_ctrl,ctrl);
    if (ret < 0)
    {
      vfe_warn("v4l2 sub device s_ctrl fail!\n");
    }
  }
  return ret;
}

static int vidioc_g_parm(struct file *file, void *priv,
    struct v4l2_streamparm *parms)
{
	struct vfe_dev *dev = video_drvdata(file);
	int ret;

	ret = v4l2_subdev_call(dev->sd,video,g_parm,parms);
	if (ret < 0)
		vfe_warn("v4l2 sub device g_parm fail!\n");

	return ret;
}

static int vidioc_s_parm(struct file *file, void *priv,
    struct v4l2_streamparm *parms)
{
	struct vfe_dev *dev = video_drvdata(file);
	int ret;

	if(parms->parm.capture.capturemode != V4L2_MODE_VIDEO && \
				parms->parm.capture.capturemode != V4L2_MODE_IMAGE && \
				parms->parm.capture.capturemode != V4L2_MODE_PREVIEW) {
		parms->parm.capture.capturemode = V4L2_MODE_PREVIEW;
	}

	dev->capture_mode = parms->parm.capture.capturemode;

	ret = v4l2_subdev_call(dev->sd,video,s_parm,parms);
	if (ret < 0)
		vfe_warn("v4l2 sub device s_parm error!\n");

	return ret;
}

int isp_ae_stat_req(struct file *file, struct v4l2_fh *fh,	struct isp_stat_buf *ae_buf)
{
	struct vfe_dev *dev = video_drvdata(file);
	int ret=0;
	ae_buf->buf_size = ISP_STAT_AE_MEM_SIZE;
	ret = copy_to_user(ae_buf->buf,
			    dev->isp_gen_set_pt->stat.ae_buf,
			    ae_buf->buf_size);
	return ret;
}

int isp_gamma_req(struct file *file, struct v4l2_fh *fh,	struct isp_stat_buf *gamma_buf)
{
	struct vfe_dev *dev = video_drvdata(file);
	int ret=0;
	gamma_buf->buf_size = ISP_GAMMA_MEM_SIZE;
	ret = copy_to_user(gamma_buf->buf,
			    dev->isp_gen_set_pt->isp_ini_cfg.isp_tunning_settings.gamma_tbl_post,
			    gamma_buf->buf_size);
	return ret;
}

int isp_hist_stat_req(struct file *file, struct v4l2_fh *fh, struct isp_stat_buf *hist_buf)
{
	struct vfe_dev *dev = video_drvdata(file);
	int ret=0;
	hist_buf->buf_size = ISP_STAT_HIST_MEM_SIZE;
	ret = copy_to_user(hist_buf->buf,
			    dev->isp_gen_set_pt->stat.hist_buf,
			    hist_buf->buf_size);
	return ret;
}

int isp_af_stat_req(struct file *file, struct v4l2_fh *fh, struct isp_stat_buf *af_buf)
{
	struct vfe_dev *dev = video_drvdata(file);
	int ret=0;
	af_buf->buf_size = ISP_STAT_AF_MEM_SIZE;

	ret = copy_to_user(af_buf->buf,
			    dev->isp_gen_set_pt->stat.af_buf,
			    af_buf->buf_size);
	return 0;
}

static int isp_exif_req (struct file *file, struct v4l2_fh *fh, struct isp_exif_attribute *exif_attr)
{
	struct vfe_dev *dev = video_drvdata(file);
	struct sensor_exif_attribute exif;
	if(dev->isp_gen_set_pt && dev->is_bayer_raw)
	{
		exif_attr->fnumber = dev->isp_gen_set_pt->isp_ini_cfg.isp_3a_settings.fno;
		if(exif_attr->fnumber < 40)
			exif_attr->fnumber = 240;
		exif_attr->focal_length = dev->isp_gen_set_pt->isp_ini_cfg.isp_tunning_settings.focus_length;
		if(dev->isp_gen_set_pt->isp_ini_cfg.isp_test_settings.isp_test_mode == ISP_TEST_ALL_ENABLE){
			if(exif_attr->focal_length < 40)
				exif_attr->focal_length = 400;
		}else{
			exif_attr->focal_length = dev->isp_3a_result_pt->real_vcm_pos;//400;
		}
		exif_attr->brightness = dev->isp_gen_set_pt->ae_lum;
		exif_attr->exposure_bias = dev->isp_gen_set_pt->exp_settings.exp_compensation;
		exif_attr->flash_fire = dev->isp_gen_set_pt->exp_settings.flash_open;
		exif_attr->iso_speed = (dev->isp_3a_result_pt->exp_analog_gain*dev->isp_3a_result_pt->exp_digital_gain)*50/4096;
		exif_attr->exposure_time.numerator = 1;
		exif_attr->shutter_speed.numerator = 1;
		if(dev->isp_3a_result_pt->exp_time != 0)
		{
			exif_attr->exposure_time.denominator = 1000000/dev->isp_3a_result_pt->exp_time;
			exif_attr->shutter_speed.denominator = 1000000/dev->isp_3a_result_pt->exp_time;
		}
		else
		{
			exif_attr->exposure_time.denominator = 10000;
			exif_attr->shutter_speed.denominator = 10000;
		}
		exif_attr->reserved[0] = dev->isp_3a_result_pt->real_vcm_pos;
		exif_attr->reserved[1] = dev->isp_gen_set_pt->color_temp;
		exif_attr->reserved[2] = dev->isp_3a_result_pt->lv;
	}
	else
	{
		if(v4l2_subdev_call(dev->sd,core,ioctl,GET_SENSOR_EXIF,&exif) != 0)
		{
			exif_attr->fnumber = 240;
			exif_attr->focal_length = 180;
			exif_attr->brightness = 128;
			exif_attr->exposure_bias = dev->ctrl_para.exp_bias;
			exif_attr->flash_fire = 0;
			exif_attr->iso_speed = 200;
			exif_attr->exposure_time.numerator = 1;
			exif_attr->exposure_time.denominator = 20;
			exif_attr->shutter_speed.numerator = 1;
			exif_attr->shutter_speed.denominator = 24;
		}
		else
		{
			exif_attr->fnumber = exif.fnumber;
			exif_attr->focal_length = exif.focal_length;
			exif_attr->brightness = exif.brightness;
			exif_attr->exposure_bias = dev->ctrl_para.exp_bias;
			exif_attr->flash_fire = exif.flash_fire;
			exif_attr->iso_speed = exif.iso_speed;
			exif_attr->exposure_time.numerator = exif.exposure_time_num;
			exif_attr->exposure_time.denominator = exif.exposure_time_den;
			exif_attr->shutter_speed.numerator = exif.exposure_time_num;
			exif_attr->shutter_speed.denominator = exif.exposure_time_den;
		}
	}
	return 0;
}


static ssize_t vfe_read(struct file *file, char __user *data, size_t count, loff_t *ppos)
{
	struct vfe_dev *dev = video_drvdata(file);

	if(vfe_is_generating(dev)) {
		return videobuf_read_stream(&dev->vb_vidq, data, count, ppos, 0,
					file->f_flags & O_NONBLOCK);
	} else {
		vfe_err("csi is not generating!\n");
		return -EINVAL;
	}
}

static unsigned int vfe_poll(struct file *file, struct poll_table_struct *wait)
{
	struct vfe_dev *dev = video_drvdata(file);
	struct videobuf_queue *q = &dev->vb_vidq;

	if(vfe_is_generating(dev)) {
		return videobuf_poll_stream(file, q, wait);
	} else {
		vfe_err("csi is not generating!\n");
		return -EINVAL;
	}
}

void vfe_clk_open(struct vfe_dev *dev)
{
	//hardware
	vfe_print("..........................vfe clk open!.......................\n");
	vfe_dphy_clk_set(dev,DPHY_CLK);
	vfe_clk_enable(dev);
	vfe_reset_disable(dev);
}
void vfe_clk_close(struct vfe_dev *dev)
{
	vfe_print("..........................vfe clk close!.......................\n");
	vfe_clk_disable(dev);
	if(vfe_opened_num < 2)
	{
		vfe_reset_enable(dev);
	}
}
static int vfe_open(struct file *file)
{
	struct vfe_dev *dev = video_drvdata(file);
	int ret;//,input_num;

	vfe_print("vfe_open\n");
	if (vfe_is_opened(dev)) {
		vfe_err("device open busy\n");
		ret = -EBUSY;
		goto open_end;
	}
	if(down_trylock(&dev->standby_seq_sema)){
		vfe_err("device not ready\n");
		return -EAGAIN;
	}
	vfe_clk_open(dev);
#ifdef USE_SPECIFIC_CCI
	csi_cci_init_helper(dev->vip_sel);
#endif
	if(dev->ccm_cfg[0]->is_isp_used || dev->ccm_cfg[1]->is_isp_used) {
		//must be after ahb and core clock enable
		bsp_isp_set_dma_load_addr((unsigned int)dev->regs.isp_load_regs_dma_addr);
		bsp_isp_set_dma_saved_addr((unsigned int)dev->regs.isp_saved_regs_dma_addr);
		//resource
		ret = isp_resource_request(dev);
		if(ret) {
			vfe_err("isp_resource_request error at %s\n",__func__);
			return ret;
		}
		vfe_dbg(0,"tasklet init ! \n");
		INIT_WORK(&dev->isp_isr_bh_task, isp_isr_bh_handle);
		INIT_WORK(&dev->isp_isr_set_sensor_task, isp_isr_set_sensor_handle);
	}
	dev->input = -1;//default input null
	dev->first_flag = 0;
	vfe_start_opened(dev);
	vfe_init_isp_log(dev);
	open_end:
	if (ret != 0){
		//up(&dev->standby_seq_sema);
		vfe_print("vfe_open busy\n");
	}
	else
	{
		vfe_print("vfe_open ok\n");
		vfe_opened_num ++;
        if (dev->input == -1) 
        {
            int i;
            for (i = 0; i < dev->dev_qty; i++) 
            {
                if (!dev->device_valid_flag[i]) break;
            }
            if (i > 0) i -= 1;
            ret = internal_s_input(dev , i);
            if (!ret) vfe_dbg(0, "vfe set a valid input %d\n", i);
        }
	}
	return ret;
}

static int vfe_close(struct file *file)
{
	struct vfe_dev *dev = video_drvdata(file);
	int ret;

	vfe_print("vfe_close\n");
	//device
	vfe_stop_generating(dev);
	if(dev->vfe_s_input_flag == 1)
	{
		if(dev->sd_act!=NULL)
		{
			v4l2_subdev_call(dev->sd_act,core,ioctl,ACT_SOFT_PWDN,0);
		}
		if(dev->power->stby_mode == NORM_STBY) {
			ret=v4l2_subdev_call(dev->sd,core, s_power, CSI_SUBDEV_STBY_ON);
			if (ret!=0) {
				vfe_err("sensor standby on error at device when csi close!\n");
			}
		} else {//POWER_OFF
			//close the device power
			ret = vfe_set_sensor_power_off(dev);
			if (ret!=0) {
				vfe_err("sensor power off error at device number when csi close!\n");
			}
		}
		dev->vfe_s_input_flag = 0;
	}
	else
	{
		vfe_print("vfe select input flag = %d, s_input have not be used .\n", dev->vfe_s_input_flag);
	}
	//hardware
	bsp_csi_int_disable(dev->vip_sel, dev->cur_ch,CSI_INT_ALL);
	bsp_csi_cap_stop(dev->vip_sel, dev->cur_ch,CSI_VCAP);
	bsp_csi_disable(dev->vip_sel);
	if(dev->is_isp_used)
		bsp_isp_disable();
	if(dev->mbus_type == V4L2_MBUS_CSI2) {
		bsp_mipi_csi_protocol_disable(dev->mipi_sel);
		bsp_mipi_csi_dphy_disable(dev->mipi_sel);
		bsp_mipi_csi_dphy_exit(dev->mipi_sel);
	}
	bsp_isp_exit();
	vfe_clk_close(dev);
	flush_work(&dev->resume_work);
	flush_delayed_work(&dev->probe_work);

	if(dev->ccm_cfg[0]->is_isp_used || dev->ccm_cfg[1]->is_isp_used) {
		flush_work(&dev->isp_isr_bh_task);
		flush_work(&dev->isp_isr_set_sensor_task);
		//tasklet_kill(&dev->isp_isr_bh_task);
		//resource
		isp_resource_release(dev);
	}
	mutex_destroy(&dev->isp_3a_result_mutex);
	//software
	videobuf_stop(&dev->vb_vidq);
	videobuf_mmap_free(&dev->vb_vidq);
	vfe_stop_opened(dev);
	dev->ctrl_para.prev_exp_line = 0;
	dev->ctrl_para.prev_ana_gain = 1;
	vfe_print("vfe_close end\n");
	up(&dev->standby_seq_sema);
	vfe_exit_isp_log(dev);
	vfe_opened_num--;
	return 0;
}

static int vfe_mmap(struct file *file, struct vm_area_struct *vma)
{
	struct vfe_dev *dev = video_drvdata(file);
	int ret;
	vfe_dbg(0,"mmap called, vma=0x%08lx\n", (unsigned long)vma);
	ret = videobuf_mmap_mapper(&dev->vb_vidq, vma);
	vfe_dbg(0,"vma start=0x%08lx, size=%ld, ret=%d\n", (unsigned long)vma->vm_start,
			(unsigned long)vma->vm_end - (unsigned long)vma->vm_start, ret);
	return ret;
}

static const struct v4l2_file_operations vfe_fops = {
	.owner          = THIS_MODULE,
	.open           = vfe_open,
	.release        = vfe_close,
	.read           = vfe_read,
	.poll           = vfe_poll,
	.ioctl          = video_ioctl2,
	//.unlocked_ioctl =
	.mmap           = vfe_mmap,
};

static const struct v4l2_ioctl_ops vfe_ioctl_ops = {
	.vidioc_querycap          = vidioc_querycap,
	.vidioc_enum_fmt_vid_cap  = vidioc_enum_fmt_vid_cap,
	.vidioc_enum_framesizes   = vidioc_enum_framesizes,
	.vidioc_g_fmt_vid_cap     = vidioc_g_fmt_vid_cap,
	.vidioc_try_fmt_vid_cap   = vidioc_try_fmt_vid_cap,
	.vidioc_s_fmt_vid_cap     = vidioc_s_fmt_vid_cap,
	.vidioc_reqbufs           = vidioc_reqbufs,
	.vidioc_querybuf          = vidioc_querybuf,
	.vidioc_qbuf              = vidioc_qbuf,
	.vidioc_dqbuf             = vidioc_dqbuf,
	.vidioc_enum_input        = vidioc_enum_input,
	.vidioc_g_input           = vidioc_g_input,
	.vidioc_s_input           = vidioc_s_input,
	.vidioc_streamon          = vidioc_streamon,
	.vidioc_streamoff         = vidioc_streamoff,
	.vidioc_queryctrl         = vidioc_queryctrl,
	.vidioc_g_ctrl            = vidioc_g_ctrl,
	.vidioc_s_ctrl            = vidioc_s_ctrl,
	.vidioc_g_parm            = vidioc_g_parm,
	.vidioc_s_parm            = vidioc_s_parm,
#ifdef CONFIG_VIDEO_V4L1_COMPAT
	.vidiocgmbuf              = vidiocgmbuf,
#endif
	.vidioc_isp_ae_stat_req   = isp_ae_stat_req,
	.vidioc_isp_hist_stat_req = isp_hist_stat_req,
	.vidioc_isp_af_stat_req   = isp_af_stat_req,
	.vidioc_isp_gamma_req   = isp_gamma_req,
	.vidioc_isp_exif_req   = isp_exif_req ,
};

static struct video_device vfe_template[] =
{
	[0] = {
		.name       = "vfe_0",
		.fops       = &vfe_fops,
		.ioctl_ops  = &vfe_ioctl_ops,
		.release    = video_device_release,
	},

        [1] = {
            .name       = "vfe_1",
            .fops       = &vfe_fops,
            .ioctl_ops  = &vfe_ioctl_ops,
            .release    = video_device_release,
        },
};

static int vfe_request_pin(struct vfe_dev *dev, int enable)
{
	int ret = 0;
#ifdef VFE_GPIO
	//to cheat the pinctrl
	dev_set_name(&dev->pdev->dev,"csi%d",dev->id);
	if(!IS_ERR_OR_NULL(dev->pctrl))
	{
		devm_pinctrl_put(dev->pctrl);
	}
	if(1 == enable){
		dev->pctrl = devm_pinctrl_get_select(&dev->pdev->dev, "default");
		if (IS_ERR_OR_NULL(dev->pctrl)) {
			vfe_err("vip%d request pinctrl handle for device [%s] failed!\n", dev->id, dev_name(&dev->pdev->dev));
			return -EINVAL;
		}
	}
	else
	{
		dev->pctrl = devm_pinctrl_get_select(&dev->pdev->dev, "suspend");
		if (IS_ERR_OR_NULL(dev->pctrl)) {
			vfe_err("vip%d disable pinctrl device [%s] failed!\n", dev->id, dev_name(&dev->pdev->dev));
			return -EINVAL;
		}
	}
	//to uncheat the pinctrl
	dev_set_name(&dev->pdev->dev,"sunxi_vfe.%d",dev->id);
	usleep_range(5000, 6000);
#endif

#ifdef FPGA_PIN
	//pin for FPGA
	vfe_print("directly write pin config @ FPGA\n");
	writel((volatile void*)0x33333333,(volatile void __iomem*)(GPIO_REGS_VBASE+0x90));
	writel((volatile void*)0x33333333,(volatile void __iomem*)(GPIO_REGS_VBASE+0x94));
	writel((volatile void*)0x03333333,(volatile void __iomem*)(GPIO_REGS_VBASE+0x98));
	//writel((volatile void*)((readl((volatile void __iomem*)(GPIO_REGS_VBASE+0x24)) & 0xff00ffff) | 0x220000), (volatile void __iomem*)(GPIO_REGS_VBASE+0x24));
	//gpiob[4]:sck gpiob[5]:sda
#endif
	return ret;
}
static int vfe_pin_release(struct vfe_dev *dev)
{
#ifdef VFE_GPIO
	devm_pinctrl_put(dev->pctrl);
#endif
	return 0;
}

static int vfe_request_gpio(struct vfe_dev *dev)
{
#ifdef VFE_GPIO
	unsigned int i;
	for(i = 0; i < dev->dev_qty; i++)
	{
		dev->ccm_cfg[i]->gpio.power_en_io = os_gpio_request(&dev->ccm_cfg[i]->gpio.power_en,1);
		dev->ccm_cfg[i]->gpio.reset_io = os_gpio_request(&dev->ccm_cfg[i]->gpio.reset,1);
		dev->ccm_cfg[i]->gpio.pwdn_io = os_gpio_request(&dev->ccm_cfg[i]->gpio.pwdn,1);
		dev->ccm_cfg[i]->gpio.flash_en_io = os_gpio_request(&dev->ccm_cfg[i]->gpio.flash_en,1);
		dev->ccm_cfg[i]->gpio.flash_mode_io = os_gpio_request(&dev->ccm_cfg[i]->gpio.flash_mode,1);
		dev->ccm_cfg[i]->gpio.af_pwdn_io = os_gpio_request(&dev->ccm_cfg[i]->gpio.af_pwdn,1);
		dev->ccm_cfg[i]->gpio.mclk_io = os_gpio_request(&dev->ccm_cfg[i]->gpio.mclk, 1);
		os_gpio_set(&dev->ccm_cfg[i]->gpio.power_en,1);
		os_gpio_set(&dev->ccm_cfg[i]->gpio.reset,1);
		os_gpio_set(&dev->ccm_cfg[i]->gpio.pwdn,1);
		os_gpio_set(&dev->ccm_cfg[i]->gpio.flash_en,1);
		os_gpio_set(&dev->ccm_cfg[i]->gpio.flash_mode,1);
		os_gpio_set(&dev->ccm_cfg[i]->gpio.af_pwdn,1);
		os_gpio_set(&dev->ccm_cfg[i]->gpio.mclk,1);
	}
#endif
	return 0;
}

static int vfe_gpio_config(struct vfe_dev *dev, int bon)
{
#ifdef VFE_GPIO
	struct vfe_gpio_cfg gpio_item;
	if(1 == bon)
	{
		os_gpio_set(&dev->gpio->power_en,1);
		os_gpio_set(&dev->gpio->reset,1);
		os_gpio_set(&dev->gpio->pwdn,1);
		os_gpio_set(&dev->gpio->flash_en,1);
		os_gpio_set(&dev->gpio->flash_mode,1);
		os_gpio_set(&dev->gpio->af_pwdn,1);
		os_gpio_set(&dev->gpio->mclk,1);
	}
	else
	{
		memcpy(&gpio_item, &(dev->gpio->power_en), sizeof(struct vfe_gpio_cfg));
		gpio_item.mul_sel = GPIO_DISABLE;
		os_gpio_set(&gpio_item,1);
		memcpy(&gpio_item, &(dev->gpio->reset), sizeof(struct vfe_gpio_cfg));
		gpio_item.mul_sel = GPIO_DISABLE;
		os_gpio_set(&gpio_item,1);
		memcpy(&gpio_item, &(dev->gpio->pwdn), sizeof(struct vfe_gpio_cfg));
		gpio_item.mul_sel = GPIO_DISABLE;
		os_gpio_set(&gpio_item,1);
		memcpy(&gpio_item, &(dev->gpio->flash_en), sizeof(struct vfe_gpio_cfg));
		gpio_item.mul_sel = GPIO_DISABLE;
		os_gpio_set(&gpio_item,1);
		memcpy(&gpio_item, &(dev->gpio->flash_mode), sizeof(struct vfe_gpio_cfg));
		gpio_item.mul_sel = GPIO_DISABLE;
		os_gpio_set(&gpio_item,1);
		memcpy(&gpio_item, &(dev->gpio->af_pwdn), sizeof(struct vfe_gpio_cfg));
		gpio_item.mul_sel = GPIO_DISABLE;
		os_gpio_set(&gpio_item,1);
		memcpy(&gpio_item, &(dev->gpio->mclk), sizeof(struct vfe_gpio_cfg));
		gpio_item.mul_sel = GPIO_DISABLE;
		os_gpio_set(&gpio_item,1);
	}
#endif
	return 0;
}

static void vfe_gpio_release(struct vfe_dev *dev)
{
#ifdef VFE_GPIO
	unsigned int i;
	for(i = 0; i < dev->dev_qty; i++)
	{
		os_gpio_release(dev->ccm_cfg[i]->gpio.power_en_io,1);
		os_gpio_release(dev->ccm_cfg[i]->gpio.reset_io,1);
		os_gpio_release(dev->ccm_cfg[i]->gpio.pwdn_io,1);
		os_gpio_release(dev->ccm_cfg[i]->gpio.flash_en_io,1);
		os_gpio_release(dev->ccm_cfg[i]->gpio.flash_mode_io,1);
		os_gpio_release(dev->ccm_cfg[i]->gpio.af_pwdn_io,1);
		os_gpio_release(dev->ccm_cfg[i]->gpio.mclk_io, 1);
	}
#endif
}

static int vfe_resource_request(struct platform_device *pdev ,struct vfe_dev *dev)
{
	struct resource   *regs_res;
	void __iomem      *regs;
	char vfe_res_csi[16] = "csi";
	char vfe_res_isp[16] = "isp";
#ifdef USE_SPECIFIC_CCI
	char vfe_res_csi_cci[16] = "csi_cci";
#endif
	char *vfe_res_name[] = {
		[0] = vfe_res_csi,
		[1] = vfe_res_isp,
#ifdef USE_SPECIFIC_CCI
		[2] = vfe_res_csi_cci,
#endif
	};

	struct resource   *res;
	int ret,i,j;

	/* get io resource */
	vfe_dbg(0,"get io resource num = %d\n", pdev->num_resources);
	for(i = 0, j = 0; i < pdev->num_resources; i++)
	{
		if(pdev->resource[i].flags != IORESOURCE_MEM)
			continue;
		vfe_dbg(0,"get resource, name = %s\n",vfe_res_name[j]);

		if(isp_va_flag == 1 && !strcmp(vfe_res_name[j], vfe_res_isp))
		{
			regs_res = NULL;
			regs = (void __iomem *)(ISP_REGS_BASE + 0xf0000000);
		}
		else
		{
			res = platform_get_resource_byname(pdev, IORESOURCE_MEM, vfe_res_name[j]);
			if (!res) {
				vfe_err("failed to find the registers, name = %s\n",vfe_res_name[j]);
				return -ENOENT;
			}
			regs_res = request_mem_region(res->start, resource_size(res), dev_name(&pdev->dev));
			if (!regs_res) {
				vfe_err("failed to obtain register region\n");
				return -ENOENT;
			}
			regs = ioremap(res->start, resource_size(res));
			if (!regs) {
				vfe_err("failed to map registers\n");
				return -ENXIO;
			}
		}
		if(!strcmp(vfe_res_name[j], vfe_res_csi)) {
			dev->regs.csi_regs_res = regs_res;
			dev->regs.csi_regs = regs;
			vfe_dbg(0,"dev->regs.csi_regs = %p\n",dev->regs.csi_regs);
		} else if (!strcmp(vfe_res_name[j], vfe_res_isp)) {
			dev->regs.isp_regs_res = regs_res;
			dev->regs.isp_regs = regs;
			vfe_dbg(0,"dev->regs.isp_regs = %p\n",dev->regs.isp_regs);
		}
		j++;
	}

	dev->isp_load_reg_mm.size = ISP_LOAD_REG_SIZE;
	dev->isp_save_reg_mm.size = ISP_SAVED_REG_SIZE;

	if(isp_va_flag == 0)
	{
		os_mem_alloc(&dev->isp_load_reg_mm);
		os_mem_alloc(&dev->isp_save_reg_mm);
		isp_load_mm = dev->isp_load_reg_mm;
		isp_saved_mm = dev->isp_save_reg_mm;
		isp_va_flag = 1;
		isp_va_alloc_sel = dev->vip_sel;
	}
	else
	{
		dev->isp_load_reg_mm = isp_load_mm;
		dev->isp_save_reg_mm = isp_saved_mm;
	}

	if(dev->isp_load_reg_mm.phy_addr != NULL) {
		dev->regs.isp_load_regs_paddr = (void*)(dev->isp_load_reg_mm.phy_addr);
		dev->regs.isp_load_regs_dma_addr = (void*)(dev->isp_load_reg_mm.dma_addr);
		dev->regs.isp_load_regs = (void*)(dev->isp_load_reg_mm.vir_addr);
		vfe_dbg(0,"isp load paddr = %p\n",dev->regs.isp_load_regs_paddr);
		vfe_dbg(0,"isp load dma_addr = %p\n",dev->regs.isp_load_regs_dma_addr);
		vfe_dbg(0,"isp load addr = %p\n",dev->regs.isp_load_regs);
		if(!dev->regs.isp_load_regs)
		{
			vfe_err("isp load regs va requset failed!\n");
			return -ENOMEM;
		}
	} else {
		vfe_err("isp load regs pa requset failed!\n");
		return -ENOMEM;
	}

	if(dev->isp_save_reg_mm.phy_addr != NULL) {
		dev->regs.isp_saved_regs_paddr = (void*)(dev->isp_save_reg_mm.phy_addr);
		dev->regs.isp_saved_regs_dma_addr = (void*)(dev->isp_save_reg_mm.dma_addr);
		dev->regs.isp_saved_regs = (void*)(dev->isp_save_reg_mm.vir_addr);
		vfe_dbg(0,"isp saved paddr = %p\n",dev->regs.isp_saved_regs_paddr);
		vfe_dbg(0,"isp saved dma_addr = %p\n",dev->regs.isp_saved_regs_dma_addr);
		vfe_dbg(0,"isp saved addr = %p\n",dev->regs.isp_saved_regs);
		if(!dev->regs.isp_saved_regs){
			vfe_err("isp save regs va requset failed!\n");
			return -ENOMEM;
		}
	} else {
		vfe_err("isp save regs pa requset failed!\n");
		return -ENOMEM;
	}

	vfe_dbg(0,"get irq resource\n");
	/*get irq resource*/
	res = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	if (!res) {
		vfe_err("failed to get IRQ resource\n");
		return -ENXIO;
	}

	dev->irq = res->start;
	//  sprintf(vfe_name,"sunxi-vfe.%d",dev->id);
#ifndef FPGA_VER
	ret = os_request_irq(dev->irq, vfe_isr, IRQF_DISABLED, pdev->name, dev);
#else
	ret = os_request_irq(dev->irq, vfe_isr, IRQF_SHARED, pdev->name, dev);
#endif

	if (ret) {
		vfe_err("failed to install irq (%d)\n", ret);
		return -ENXIO;
	}

	vfe_dbg(0,"clock resource\n");
	/*clock resource*/
	if (vfe_clk_get(dev)) {
		vfe_err("vfe clock get failed!\n");
		return -ENXIO;
	}

	vfe_dbg(0,"get pin resource\n");

	/* pin resource */
	/* request gpio */
	vfe_request_pin(dev, 1);
	vfe_request_gpio(dev);
	return 0;
}

static void vfe_resource_release(struct vfe_dev *dev)
{
	if(dev->regs.csi_regs)
		iounmap(dev->regs.csi_regs);
	if(dev->regs.isp_regs && isp_va_flag)
		iounmap(dev->regs.isp_regs);
	if(dev->regs.csi_regs_res) {
		release_resource(dev->regs.csi_regs_res);
		kfree(dev->regs.csi_regs_res);
	}
	if(dev->regs.isp_regs_res) {
		release_resource(dev->regs.isp_regs_res);
		kfree(dev->regs.isp_regs_res);
	}
	if(isp_va_alloc_sel == dev->vip_sel)
	{
		os_mem_free(&dev->isp_load_reg_mm);
		os_mem_free(&dev->isp_save_reg_mm);
		isp_va_flag = 0;
	}
	vfe_gpio_release(dev);
	vfe_pin_release(dev);
	vfe_clk_release(dev);
	if(dev->irq > 0)
		free_irq(dev->irq, dev);
}
static int vfe_set_sensor_power_on(struct vfe_dev *dev)
{
	int ret = 0;
#ifdef _REGULATOR_CHANGE_
	vfe_device_regulator_get(dev->ccm_cfg[dev->input]);
	dev->power = &dev->ccm_cfg[dev->input]->power;
#endif

#ifdef CONFIG_ARCH_SUN8IW6P1
#else
	if(!IS_ERR_OR_NULL(dev->sd))
	{
		vfe_set_pmu_channel(dev->sd, IOVDD, ON);
	}
	usleep_range(10000,12000);
#endif
	ret = v4l2_subdev_call(dev->sd,core, s_power, CSI_SUBDEV_PWR_ON);
	dev->vfe_sensor_power_cnt++;
	vfe_dbg(0,"power_on______________________________\n");
	return ret;
}
static int vfe_set_sensor_power_off(struct vfe_dev *dev)
{
	int ret = 0;
	if(dev->vfe_sensor_power_cnt > 0)
	{
		ret = v4l2_subdev_call(dev->sd,core, s_power, CSI_SUBDEV_PWR_OFF);
		dev->vfe_sensor_power_cnt--;
	}
	else
	{
		vfe_warn("Sensor is already power off!\n");
		dev->vfe_sensor_power_cnt = 0;
	}
#ifdef CONFIG_ARCH_SUN8IW6P1
#else
	usleep_range(10000,12000);
	if(!IS_ERR_OR_NULL(dev->sd))
	{
		vfe_set_pmu_channel(dev->sd, IOVDD, OFF);
	}
#endif

#ifdef _REGULATOR_CHANGE_
	vfe_device_regulator_put(dev->ccm_cfg[dev->input]);
#endif
	vfe_dbg(0,"power_off______________________________\n");
	return ret;
}

static void resume_work_handle(struct work_struct *work);
static void __exit vfe_exit(void);

#ifdef CONFIG_ES
static void vfe_early_suspend(struct early_suspend *h);
static void vfe_late_resume(struct early_suspend *h);
#endif

static const char *vfe_regulator_name[] =
{
	VFE_ISP_REGULATOR,
	VFE_CSI_REGULATOR,
};
static int vfe_get_regulator(struct vfe_dev *dev)
{
	struct regulator *regul;
	unsigned int i;
	for (i = 0; i < ARRAY_SIZE(vfe_regulator_name); ++i) {
		if(strcmp(vfe_regulator_name[i],"") != 0)
		{
			regul = regulator_get(NULL, vfe_regulator_name[i]);
			if (IS_ERR_OR_NULL(regul))
			{
				vfe_err("get regulator vfe system power error, i = %d!\n",i);
				regul = NULL;
			}
		}else{
			regul = NULL;
		}
		dev->vfe_system_power[i] = regul;
	}
	if(VFE_VPU_CLK){
		dev->clock.vfe_vpu_clk = os_clk_get(NULL, VFE_VPU_CLK);
	}else{
		dev->clock.vfe_vpu_clk = NULL;
	}
	return 0;
}
static int vfe_enable_regulator_all(struct vfe_dev *dev)
{
	unsigned int i;
	for (i = 0; i < ARRAY_SIZE(vfe_regulator_name); ++i) {
		if(dev->vfe_system_power[i] != NULL)
		{
			regulator_enable(dev->vfe_system_power[i]);
		}
	}
	usleep_range(5000,6000);
	if(dev->clock.vfe_vpu_clk)
		os_clk_prepare_enable(dev->clock.vfe_vpu_clk);
	else
		vfe_warn("vfe vpu clock is null\n");
	return 0;
}
static int vfe_disable_regulator_all(struct vfe_dev *dev)
{
	unsigned int i;
	for (i = 0; i < ARRAY_SIZE(vfe_regulator_name); ++i) {
		if(dev->vfe_system_power[i] != NULL)
		{
			regulator_disable(dev->vfe_system_power[i]);
		}
	}
	if(dev->clock.vfe_vpu_clk)
		os_clk_disable_unprepare(dev->clock.vfe_vpu_clk);
	else
		vfe_warn("vfe vpu clock is null\n");
	return 0;
}
static int vfe_put_regulator(struct vfe_dev *dev)
{
	unsigned int i;
	for (i = 0; i < ARRAY_SIZE(vfe_regulator_name); ++i) {
		if(dev->vfe_system_power[i] != NULL)
		{
		  	regulator_put(dev->vfe_system_power[i]);
		}
	}
	if(dev->clock.vfe_vpu_clk)
		os_clk_put(dev->clock.vfe_vpu_clk);
	else
		vfe_warn("vfe vpu clock is null\n");
	return 0;
}

/*
static void vfe_enable_regulator(struct vfe_dev *dev, enum vfe_regulator regulator)
{
	regulator_enable(dev->vfe_system_power[regulator]);
	usleep_range(5000,6000);
}
static void vfe_disable_regulator(struct vfe_dev *dev, enum vfe_regulator regulator)
{
	regulator_disable(dev->vfe_system_power[regulator]);
}
*/
static int vfe_device_regulator_get(struct ccm_config  *ccm_cfg)
{
#ifdef VFE_PMU
	/*power issue*/
	ccm_cfg->power.iovdd = NULL;
	ccm_cfg->power.avdd = NULL;
	ccm_cfg->power.dvdd = NULL;
	ccm_cfg->power.afvdd = NULL;

	if(strcmp(ccm_cfg->iovdd_str,"")) {
		ccm_cfg->power.iovdd = regulator_get(NULL, ccm_cfg->iovdd_str);
		if (IS_ERR_OR_NULL(ccm_cfg->power.iovdd)) {
			vfe_err("get regulator csi_iovdd error!\n");
			goto regulator_get_err;
		}
	}
	if(strcmp(ccm_cfg->avdd_str,"")) {
		ccm_cfg->power.avdd = regulator_get(NULL, ccm_cfg->avdd_str);
		if (IS_ERR_OR_NULL(ccm_cfg->power.avdd)) {
			vfe_err("get regulator csi_avdd error!\n");
			goto regulator_get_err;
		}
	}
	if(strcmp(ccm_cfg->dvdd_str,"")) {
		ccm_cfg->power.dvdd = regulator_get(NULL, ccm_cfg->dvdd_str);
		if (IS_ERR_OR_NULL(ccm_cfg->power.dvdd)) {
			vfe_err("get regulator csi_dvdd error!\n");
			goto regulator_get_err;
		}
	}
	if(strcmp(ccm_cfg->afvdd_str,"")) {
		ccm_cfg->power.afvdd = regulator_get(NULL, ccm_cfg->afvdd_str);
		if (IS_ERR_OR_NULL(ccm_cfg->power.afvdd)) {
			vfe_err("get regulator csi_afvdd error!\n");
			goto regulator_get_err;
		}
	}
	return 0;
regulator_get_err:
	return -1;
#else
	return 0;
#endif
}

static int vfe_device_regulator_put(struct ccm_config  *ccm_cfg)
{
	/*power issue*/
	if(!IS_ERR_OR_NULL(ccm_cfg->power.iovdd))
		regulator_put(ccm_cfg->power.iovdd);
	if(!IS_ERR_OR_NULL(ccm_cfg->power.avdd))
		regulator_put(ccm_cfg->power.avdd);
	if(!IS_ERR_OR_NULL(ccm_cfg->power.dvdd))
		regulator_put(ccm_cfg->power.dvdd);
	if(!IS_ERR_OR_NULL(ccm_cfg->power.afvdd))
		regulator_put(ccm_cfg->power.afvdd);
	return 0;
}
static int vfe_sensor_check(struct vfe_dev *dev)
{
	int ret = 0;
	struct v4l2_subdev *sd = dev->sd;
	vfe_print("Check sensor!\n");
#ifdef CONFIG_ARCH_SUN8IW8P1	
	return 0;
#endif
	vfe_set_sensor_power_on(dev);
#ifdef USE_SPECIFIC_CCI
	csi_cci_init_helper(dev->vip_sel);
#endif
	ret = v4l2_subdev_call(sd,core, init, 0);
	if(dev->power->stby_mode == NORM_STBY){
		if(ret < 0)
		{
			vfe_set_sensor_power_off(dev);
			ret = -1;
		}
		else
		{
			v4l2_subdev_call(sd,core, s_power, CSI_SUBDEV_STBY_ON);
			ret = 0;
		}
	}
	else// if(dev->power->stby_mode == POWER_OFF)
	{
		ret = (ret< 0)?-1:0;
		vfe_set_sensor_power_off(dev);
	}
	if(vfe_i2c_dbg == 1)
	{
		vfe_print("NOTE: Sensor i2c dbg, it's always power on and register success!..................\n");
		ret = 0;
		vfe_set_sensor_power_on(dev);
	}
#ifdef USE_SPECIFIC_CCI
	csi_cci_exit_helper(dev->vip_sel);
#endif
	return ret;
}

#ifdef USE_SPECIFIC_CCI
static int vfe_sensor_subdev_register_check(struct vfe_dev *dev,struct v4l2_device *v4l2_dev,
							struct ccm_config  *ccm_cfg, struct i2c_board_info *sensor_i2c_board)
{
	int ret;
	ccm_cfg->sd= NULL;
	ccm_cfg->sd = cci_bus_match(ccm_cfg->ccm, dev->id, sensor_i2c_board->addr);// ccm_cfg->i2c_addr >> 1);
	if(ccm_cfg->sd)
	{
		ret = v4l2_device_register_subdev(&dev->v4l2_dev,ccm_cfg->sd);
		vfe_print("v4l2_device_register_subdev return %d\n",ret);
		if(ret < 0)
		{
			ccm_cfg->sd = NULL;
		}
	}
	update_ccm_info(dev, ccm_cfg);
	if (IS_ERR_OR_NULL(ccm_cfg->sd) )
	{
		vfe_err("Error registering v4l2 subdevice No such device!\n");
		return -ENODEV;
	}
	else
	{
		vfe_print("registered sensor subdev is OK!\n");
	}
	//Subdev register is OK, check sensor init!
	return vfe_sensor_check(dev);
}
static int vfe_sensor_subdev_unregister(struct v4l2_device *v4l2_dev,
								struct ccm_config  *ccm_cfg, struct i2c_board_info *sensor_i2c_board)
{
	struct cci_driver *cci_driv = v4l2_get_subdevdata(ccm_cfg->sd);
	if (IS_ERR_OR_NULL(cci_driv))
		return -ENODEV;
	vfe_print("vfe sensor subdev unregister!\n");
	v4l2_device_unregister_subdev(ccm_cfg->sd);
	cci_bus_match_cancel(cci_driv);
	return 0;
}
static int vfe_actuator_subdev_register( struct vfe_dev *dev, struct ccm_config  *ccm_cfg, struct i2c_board_info *act_i2c_board)
{
	ccm_cfg->sd_act= NULL;
	ccm_cfg->sd_act = cci_bus_match(ccm_cfg->act_name, dev->id, act_i2c_board->addr);// ccm_cfg->i2c_addr >> 1);
	//reg_sd_act:
	if (!ccm_cfg->sd_act) {
		vfe_err("Error registering v4l2 act subdevice!\n");
		return  -EINVAL;
	} else{
		vfe_print("registered actuator device succeed! act_name is %s\n",ccm_cfg->act_name);
	}
	ccm_cfg->act_ctrl = (struct actuator_ctrl_t *)container_of(ccm_cfg->sd_act,struct actuator_ctrl_t, sdev);
	//printk("ccm_cfg->act_ctrl=%x\n",(unsigned int )ccm_cfg->act_ctrl);
	return 0;
}

#else // NOT defind USE_SPECIFIC_CCI

static int vfe_sensor_subdev_register_check(struct vfe_dev *dev,struct v4l2_device *v4l2_dev,
							struct ccm_config  *ccm_cfg, struct i2c_board_info *sensor_i2c_board)
{
	struct i2c_adapter *i2c_adap = i2c_get_adapter(ccm_cfg->twi_id);
	if (i2c_adap == NULL)
	{
		vfe_err("request i2c adapter failed!\n");
		return -EFAULT;
	}
	ccm_cfg->sd = v4l2_i2c_new_subdev_board(v4l2_dev, i2c_adap, sensor_i2c_board, NULL);
	if (IS_ERR_OR_NULL(ccm_cfg->sd) )
	{
		i2c_put_adapter(i2c_adap);
		vfe_err("Error registering v4l2 subdevice No such device!\n");
		return -ENODEV;
	}
	else
	{
		vfe_print("registered sensor subdev is OK!\n");
	}
	update_ccm_info(dev, ccm_cfg);
	//Subdev register is OK, check sensor init!
	return vfe_sensor_check(dev);
}
static int vfe_sensor_subdev_unregister(struct v4l2_device *v4l2_dev,
								struct ccm_config  *ccm_cfg, struct i2c_board_info *sensor_i2c_board)
{
	struct i2c_client *client = v4l2_get_subdevdata(ccm_cfg->sd);
	struct i2c_adapter *adapter;
	if (!client)
		return -ENODEV;
	vfe_print("vfe sensor subdev unregister!\n");
	v4l2_device_unregister_subdev(ccm_cfg->sd);
	adapter = client->adapter;
	i2c_unregister_device(client);
	if (adapter)
		i2c_put_adapter(adapter);
	return 0;
}
static int vfe_actuator_subdev_register(struct vfe_dev *dev, struct ccm_config  *ccm_cfg, struct i2c_board_info *act_i2c_board)
{
	struct v4l2_device *v4l2_dev = &dev->v4l2_dev;
	struct i2c_adapter *i2c_adap_act = i2c_get_adapter(ccm_cfg->twi_id);//must use the same twi_channel with sensor
	if (i2c_adap_act == NULL)
	{
		vfe_err("request act i2c adapter failed\n");
		return  -EINVAL;
	}

	ccm_cfg->sd_act = NULL;
	//vfe_print("registered act sub device, slave=0x%x~~~\n",ccm_cfg->act_slave);
	act_i2c_board->addr = (unsigned short)(ccm_cfg->act_slave>>1);
	strcpy(act_i2c_board->type,ccm_cfg->act_name);
	ccm_cfg->sd_act = v4l2_i2c_new_subdev_board(v4l2_dev,i2c_adap_act,act_i2c_board,NULL);
	//reg_sd_act:
	if (!ccm_cfg->sd_act) {
		vfe_err("Error registering v4l2 act subdevice!\n");
		return  -EINVAL;
	} else{
		vfe_print("registered actuator device succeed!\n");
	}
	ccm_cfg->act_ctrl = (struct actuator_ctrl_t *)container_of(ccm_cfg->sd_act,struct actuator_ctrl_t, sdev);
	//printk("ccm_cfg->act_ctrl=%x\n",(unsigned int )ccm_cfg->act_ctrl);
	return 0;
}
#endif
static void cpy_ccm_power_settings(struct ccm_config *ccm_cfg)
{
	strcpy(ccm_cfg->iovdd_str, ccm_cfg->sensor_cfg_ini->sub_power_str[ENUM_IOVDD]);
	ccm_cfg->power.iovdd_vol = ccm_cfg->sensor_cfg_ini->sub_power_vol[ENUM_IOVDD];

	strcpy(ccm_cfg->avdd_str, ccm_cfg->sensor_cfg_ini->sub_power_str[ENUM_AVDD]);
	ccm_cfg->power.avdd_vol = ccm_cfg->sensor_cfg_ini->sub_power_vol[ENUM_AVDD];

	strcpy(ccm_cfg->dvdd_str, ccm_cfg->sensor_cfg_ini->sub_power_str[ENUM_DVDD]);
	ccm_cfg->power.dvdd_vol = ccm_cfg->sensor_cfg_ini->sub_power_vol[ENUM_DVDD];

	strcpy(ccm_cfg->afvdd_str, ccm_cfg->sensor_cfg_ini->sub_power_str[ENUM_AFVDD]);
	ccm_cfg->power.afvdd_vol = ccm_cfg->sensor_cfg_ini->sub_power_vol[ENUM_AFVDD];
}

static int cpy_ccm_sub_device_cfg(struct ccm_config *ccm_cfg, int n)
{
	strcpy(ccm_cfg->ccm ,ccm_cfg->sensor_cfg_ini->camera_inst[n].name);
	if(strcmp(ccm_cfg->sensor_cfg_ini->camera_inst[n].isp_cfg_name,""))
	{
		strcpy(ccm_cfg->isp_cfg_name ,ccm_cfg->sensor_cfg_ini->camera_inst[n].isp_cfg_name);
	}
	else
	{
		strcpy(ccm_cfg->isp_cfg_name , ccm_cfg->ccm);
	}
	ccm_cfg->i2c_addr = ccm_cfg->sensor_cfg_ini->camera_inst[n].i2c_addr;
	ccm_cfg->hflip = ccm_cfg->sensor_cfg_ini->camera_inst[n].hflip;
	ccm_cfg->vflip = ccm_cfg->sensor_cfg_ini->camera_inst[n].vflip;
	ccm_cfg->hflip_thumb= ccm_cfg->sensor_cfg_ini->camera_inst[n].hflip;
	ccm_cfg->vflip_thumb = ccm_cfg->sensor_cfg_ini->camera_inst[n].vflip;
	ccm_cfg->power.stby_mode = ccm_cfg->sensor_cfg_ini->camera_inst[n].stdby_mode;
	if(ccm_cfg->sensor_cfg_ini->camera_inst[n].sensor_type == SENSOR_RAW)
	{
		ccm_cfg->is_bayer_raw = 1;
		ccm_cfg->is_isp_used = 1;
	}
	else if(ccm_cfg->sensor_cfg_ini->camera_inst[n].sensor_type == SENSOR_YUV)
	{
		ccm_cfg->is_bayer_raw = 0;
		ccm_cfg->is_isp_used = 0;
	}
	else
	{
		ccm_cfg->is_bayer_raw = 0;
		ccm_cfg->is_isp_used = 1;
	}
	strcpy(ccm_cfg->act_name,ccm_cfg->sensor_cfg_ini->camera_inst[n].act_name);
	if(strcmp(ccm_cfg->act_name,""))
	{
		ccm_cfg->act_used = 1;
		vfe_print("VCM driver name is \"%s\".\n",ccm_cfg->act_name);
	}
	ccm_cfg->act_slave = ccm_cfg->sensor_cfg_ini->camera_inst[n].act_i2c_addr;
	return 0;
}
const char *sensor_info_type[] =
{
	"YUV",
	"RAW"
};
static struct v4l2_subdev *vfe_sensor_register_check(struct vfe_dev *dev,struct v4l2_device *v4l2_dev,struct ccm_config  *ccm_cfg,
					struct i2c_board_info *sensor_i2c_board,int input_num )
{
	int sensor_cnt,ret, sensor_num;
	struct sensor_item sensor_info;
	if(dev->vip_define_sensor_list == 1)
	{
		sensor_num = ccm_cfg->sensor_cfg_ini->detect_sensor_num;
		if(ccm_cfg->sensor_cfg_ini->detect_sensor_num == 0)	{
			sensor_num = 1;
		}
	} else {
		sensor_num = 1;
	}
	for(sensor_cnt=0; sensor_cnt<sensor_num; sensor_cnt++)
	{
		if(dev->vip_define_sensor_list == 1)
		{
			if(ccm_cfg->sensor_cfg_ini->detect_sensor_num > 0)
				cpy_ccm_sub_device_cfg(ccm_cfg, sensor_cnt);
		}
		if(get_sensor_info(ccm_cfg->ccm, &sensor_info) == 0)
		{
			if(ccm_cfg->i2c_addr != sensor_info.i2c_addr)
			{
				vfe_warn("Sensor info \"%s\" i2c_addr is different from sys_config!\n", sensor_info.sensor_name );
				//vfe_warn("Sensor info i2c_addr = %d, sys_config i2c_addr = %d!\n", sensor_info.i2c_addr, ccm_cfg->i2c_addr);
				//ccm_cfg->i2c_addr = sensor_info.i2c_addr;
			}
			if(ccm_cfg->is_bayer_raw != sensor_info.sensor_type)
			{
				vfe_warn("Camer detect \"%s\" fmt is different from sys_config!\n", sensor_info_type[sensor_info.sensor_type]);
				vfe_warn("Apply detect  fmt = %d replace sys_config fmt = %d!\n", sensor_info.sensor_type, ccm_cfg->is_bayer_raw);
				ccm_cfg->is_bayer_raw = sensor_info.sensor_type;
			}
			if(sensor_info.sensor_type == SENSOR_RAW)
			{
				ccm_cfg->is_isp_used = 1;
			}
			else
			{
				ccm_cfg->act_used = 0;
			}
			vfe_print("Find sensor name is \"%s\", i2c address is %x, type is \"%s\" !\n",sensor_info.sensor_name,sensor_info.i2c_addr,
							sensor_info_type[sensor_info.sensor_type]);
		}
		sensor_i2c_board->addr = (unsigned short)(ccm_cfg->i2c_addr>>1);
		strcpy(sensor_i2c_board->type,ccm_cfg->ccm);

		vfe_print("Sub device register \"%s\" i2c_addr = 0x%x start!\n",sensor_i2c_board->type, ccm_cfg->i2c_addr);
		ret = vfe_sensor_subdev_register_check(dev,v4l2_dev,ccm_cfg,sensor_i2c_board);
		if( ret == -1)
		{
			vfe_sensor_subdev_unregister(v4l2_dev,ccm_cfg,sensor_i2c_board);
			vfe_print("Sub device register \"%s\" failed!\n",sensor_i2c_board->type);
			ccm_cfg->sd =NULL;
			continue;
		}
		else if(ret == ENODEV ||ret == EFAULT)
		{
			continue;
		}
		else if(ret == 0)
		{
			vfe_print("Sub device register \"%s\" is OK!\n",sensor_i2c_board->type);
			break;
		}
	}
	return ccm_cfg->sd;
}

static void probe_work_handle(struct work_struct *work)
{
	struct vfe_dev *dev= container_of(work, struct vfe_dev, probe_work.work);
	int ret = 0;
	int input_num;
	struct video_device *vfd;
	char vfe_name[16] = {0};
	mutex_lock(&probe_hdl_lock);
	vfe_print("probe_work_handle start!\n");
	vfe_dbg(0,"v4l2_device_register\n");
	if(dev->dev_qty == 0)
		goto probe_hdl_clk_close;
	/* v4l2 device register */
	ret = v4l2_device_register(&dev->pdev->dev, &dev->v4l2_dev);
	if (ret) {
		vfe_err("Error registering v4l2 device\n");
		goto probe_hdl_free_dev;
	}
	dev_set_drvdata(&dev->pdev->dev, (dev));
	vfe_dbg(0,"v4l2 subdev register\n");
	/* v4l2 subdev register */
	dev->is_same_module = 0;
	for(input_num=0; input_num<dev->dev_qty; input_num++)
	{
		vfe_print("v4l2 subdev register input_num = %d\n",input_num);
		if(!strcmp(dev->ccm_cfg[input_num]->ccm,""))
		{
			vfe_err("Sensor name is NULL!\n");
			goto snesor_register_end;
		}
		if(dev->is_same_module)
		{
			dev->ccm_cfg[input_num]->sd = dev->ccm_cfg[input_num-1]->sd;
			vfe_dbg(0,"num = %d , sd_0 = %p,sd_1 = %p\n",input_num,dev->ccm_cfg[input_num]->sd,dev->ccm_cfg[input_num-1]->sd);
			goto snesor_register_end;
		}

		if((dev->dev_qty > 1) && (input_num+1<dev->dev_qty))
		{
			if((!strcmp(dev->ccm_cfg[input_num]->ccm,dev->ccm_cfg[input_num+1]->ccm)))
				dev->is_same_module = 1;
		}
		if(dev->vip_define_sensor_list == 1)
		{
			if(dev->ccm_cfg[input_num]->sensor_cfg_ini->power_settings_enable == 1)
			{
				cpy_ccm_power_settings(dev->ccm_cfg[input_num]);
			}
		}
#ifdef _REGULATOR_CHANGE_
#else
		if(vfe_device_regulator_get(dev->ccm_cfg[input_num]))
		{
			vfe_err("vfe_device_regulator_get error at input_num = %d\n",input_num);
			goto snesor_register_end;
		}
#endif

		//dev->dev_sensor[input_num].addr = (unsigned short)(dev->ccm_cfg[input_num]->i2c_addr>>1);
		//strcpy(dev->dev_sensor[input_num].type,dev->ccm_cfg[input_num]->ccm);
		vfe_print("vfe sensor detect start! input_num = %d\n",input_num);
		dev->input = input_num;
		if(vfe_sensor_register_check(dev,&dev->v4l2_dev,dev->ccm_cfg[input_num],&dev->dev_sensor[input_num],input_num) == NULL)
		{
			vfe_warn("vfe sensor register check error at input_num = %d\n",input_num);
			dev->device_valid_flag[input_num] = 0;
			goto probe_hdl_unreg_dev;
		}
		else{
			dev->device_valid_flag[input_num] = 1;
		}
		if(dev->ccm_cfg[input_num]->is_isp_used && dev->ccm_cfg[input_num]->is_bayer_raw)
		{
			if(read_ini_info(dev,input_num, "/system/etc/hawkview/"))
			{
				vfe_warn("read ini info fail\n");
			}
		}

		if(dev->ccm_cfg[input_num]->act_used == 1)
		{
			dev->dev_act[input_num].addr = (unsigned short)(dev->ccm_cfg[input_num]->act_slave>>1);
			strcpy(dev->dev_act[input_num].type,dev->ccm_cfg[input_num]->act_name);
			if(vfe_actuator_subdev_register(dev,dev->ccm_cfg[input_num], &dev->dev_act[input_num]) != 0)
				;//goto probe_hdl_free_dev;
		}
		snesor_register_end:
		vfe_dbg(0,"dev->ccm_cfg[%d] = %p\n",input_num,dev->ccm_cfg[input_num]);
		vfe_dbg(0,"dev->ccm_cfg[%d]->sd = %p\n",input_num,dev->ccm_cfg[input_num]->sd);
		//    vfe_dbg(0,"dev->ccm_cfg[%d]->ccm_info = %p\n",input_num,&dev->ccm_cfg[input_num]->ccm_info);
		//    vfe_dbg(0,"dev->ccm_cfg[%d]->ccm_info.mclk = %ld\n",input_num,dev->ccm_cfg[input_num]->ccm_info.mclk);
		vfe_dbg(0,"dev->ccm_cfg[%d]->power.iovdd = %p\n",input_num,dev->ccm_cfg[input_num]->power.iovdd);
		vfe_dbg(0,"dev->ccm_cfg[%d]->power.avdd = %p\n",input_num,dev->ccm_cfg[input_num]->power.avdd);
		vfe_dbg(0,"dev->ccm_cfg[%d]->power.dvdd = %p\n",input_num,dev->ccm_cfg[input_num]->power.dvdd);
		vfe_dbg(0,"dev->ccm_cfg[%d]->power.afvdd = %p\n",input_num,dev->ccm_cfg[input_num]->power.afvdd);
		//    dev->ccm_cfg[input_num]->ccm_info.mclk = MCLK_OUT_RATE;
		//    dev->ccm_cfg[input_num]->ccm_info.stby_mode = dev->ccm_cfg[input_num]->ccm_info.stby_mode;
	}
	dev->input = -1;
	/*video device register */
	ret = -ENOMEM;
	vfd = video_device_alloc();
	if (!vfd)
	{
		goto probe_hdl_unreg_dev;
	}
	*vfd = vfe_template[dev->id];
	vfd->v4l2_dev = &dev->v4l2_dev;
	sprintf(vfe_name,"vfe-%d",dev->id);
	dev_set_name(&vfd->dev, vfe_name);
	ret = video_register_device(vfd, VFL_TYPE_GRABBER, dev->id);
	if (ret < 0)
	{
		goto probe_hdl_rel_vdev;
	}
	video_set_drvdata(vfd, dev);
	/*add device list*/
	/* Now that everything is fine, let's add it to device list */
	list_add_tail(&dev->devlist, &devlist);

	dev->vfd = vfd;
	vfe_print("V4L2 device registered as %s\n",video_device_node_name(vfd));

	/*initial video buffer queue*/
	videobuf_queue_dma_contig_init(&dev->vb_vidq, &vfe_video_qops, NULL, &dev->slock,
				V4L2_BUF_TYPE_VIDEO_CAPTURE,
				V4L2_FIELD_NONE,//default format, can be changed by s_fmt
				sizeof(struct vfe_buffer), dev,NULL);

	//vfe_print("videobuf_queue_dma_contig_init @ probe handle!\n");
	ret = sysfs_create_group(&dev->pdev->dev.kobj, &vfe_attribute_group);
	//vfe_print("sysfs_create_group @ probe handle!\n");

#ifdef CONFIG_ES
	dev->early_suspend.level = EARLY_SUSPEND_LEVEL_DISABLE_FB + 1;
	dev->early_suspend.suspend = vfe_early_suspend;
	dev->early_suspend.resume = vfe_late_resume;
	register_early_suspend(&dev->early_suspend);
	vfe_print("register_early_suspend @ probe handle!\n");
#endif

probe_hdl_clk_close:
#ifdef USE_SPECIFIC_CCI
	vfe_clk_close(dev);
#endif

	vfe_print("probe_work_handle end!\n");
	mutex_unlock(&probe_hdl_lock);
	return ;

	probe_hdl_rel_vdev:
	video_device_release(vfd);
	vfe_print("video_device_release @ probe_hdl!\n");
	probe_hdl_unreg_dev:
	vfe_print("v4l2_device_unregister @ probe_hdl!\n");
	v4l2_device_unregister(&dev->v4l2_dev);
	probe_hdl_free_dev:
	vfe_print("vfe_resource_release @ probe_hdl!\n");
#ifdef USE_SPECIFIC_CCI
	csi_cci_exit_helper(dev->vip_sel);
	vfe_clk_close(dev);
#endif
	//vfe_resource_release(dev);
	vfe_print("vfe_exit @ probe_hdl!\n");
	//vfe_exit();

	vfe_warn("Failed to install at probe handle\n");
	mutex_unlock(&probe_hdl_lock);
	return ;
}


static int vfe_probe(struct platform_device *pdev)
{
	struct vfe_dev *dev;
	//  struct video_device *vfd;
	//  struct i2c_adapter *i2c_adap;
	//  struct i2c_adapter *i2c_adap_act;
	struct sunxi_vip_platform_data *pdata = NULL;
	int ret = 0;
	int input_num;
	unsigned int i;

	vfe_dbg(0,"vfe_probe\n");

	/*request mem for dev*/
	dev = kzalloc(sizeof(struct vfe_dev), GFP_KERNEL);
	if (!dev) {
		vfe_err("request dev mem failed!\n");
		return -ENOMEM;
	}

#if defined CONFIG_ARCH_SUN8IW1P1
	dev->platform_id = ISP_PLATFORM_SUN8IW1P1;
#elif defined CONFIG_ARCH_SUN8IW3P1
	dev->platform_id = ISP_PLATFORM_SUN8IW3P1;
#elif defined CONFIG_ARCH_SUN9IW1P1
	dev->platform_id = ISP_PLATFORM_SUN9IW1P1;
#elif defined CONFIG_ARCH_SUN8IW5P1
	dev->platform_id = ISP_PLATFORM_SUN8IW5P1;
#elif defined CONFIG_ARCH_SUN8IW6P1
	dev->platform_id = ISP_PLATFORM_SUN8IW6P1;
#elif defined CONFIG_ARCH_SUN8IW7P1
	dev->platform_id = ISP_PLATFORM_SUN8IW7P1;
#elif defined CONFIG_ARCH_SUN8IW8P1
	dev->platform_id = ISP_PLATFORM_SUN8IW8P1;
#elif defined CONFIG_ARCH_SUN8IW9P1
	dev->platform_id = ISP_PLATFORM_SUN8IW9P1;

#endif

	dev->id = pdev->id;
	dev->pdev = pdev;
	pdata = pdev->dev.platform_data;
	dev->mipi_sel = pdata->mipi_sel;
	dev->vip_sel = pdata->vip_sel;
	dev->isp_sel = pdata->isp_sel;
	dev->generating = 0;
	dev->opened = 0;
	dev->vfe_sensor_power_cnt = 0;
	dev->vfe_s_input_flag = 0;
	vfe_print("pdev->id = %d\n",pdev->id);
	vfe_print("dev->mipi_sel = %d\n",pdata->mipi_sel);
	vfe_print("dev->vip_sel = %d\n",pdata->vip_sel);
	vfe_print("dev->isp_sel = %d\n",pdata->isp_sel);

	//to cheat the pinctrl
	dev_set_name(&dev->pdev->dev,"csi%d",dev->id);

	spin_lock_init(&dev->slock);

	vfe_dbg(0,"fetch sys_config1\n");
	/* fetch sys_config! */
	for(input_num=0; input_num < MAX_INPUT_NUM; input_num++)
	{
		dev->ccm_cfg[input_num] = &dev->ccm_cfg_content[input_num];
		vfe_dbg(0,"dev->ccm_cfg[%d] = %p\n",input_num,dev->ccm_cfg[input_num]);
		dev->ccm_cfg[input_num]->i2c_addr = i2c_addr;
		strcpy(dev->ccm_cfg[input_num]->ccm , ccm);
		strcpy(dev->ccm_cfg[input_num]->isp_cfg_name, ccm);
		dev->ccm_cfg[input_num]->act_slave = act_slave;
		strcpy(dev->ccm_cfg[input_num]->act_name , act_name);
		dev->vip_define_sensor_list = define_sensor_list;
	}

	ret = fetch_config(dev);
	if (ret) {
		vfe_err("Error at fetch_config\n");
		goto error;
	}

	if(vips!=0xffff)
	{
		printk("vips input 0x%x\n",vips);
		dev->ccm_cfg[0]->is_isp_used=(vips&0xf0)>>4;
		dev->ccm_cfg[0]->is_bayer_raw=(vips&0xf00)>>8;
		if((vips&0xff0)==0)
			dev->ccm_cfg[0]->act_used=0;
	}
	vfe_get_regulator(dev);
	vfe_enable_regulator_all(dev);
	ret = vfe_resource_request(pdev,dev);
	if(ret < 0)
		goto free_resource;
#ifdef USE_SPECIFIC_CCI
	vfe_clk_open(dev);
#endif
	//to uncheat the pinctrl
	dev_set_name(&dev->pdev->dev,"sunxi_vfe.%d",dev->id);

	ret = bsp_csi_set_base_addr(dev->vip_sel, (unsigned int)dev->regs.csi_regs);
	if(ret < 0)
		goto free_resource;
#if defined (CONFIG_ARCH_SUN8IW3P1) || defined(CONFIG_ARCH_SUN8IW5P1) || defined(CONFIG_ARCH_SUN8IW7P1)

#else
	if(dev->mipi_sel == 0) {
		//todo for mipi lib.
		if(sunxi_get_soc_ver() > SUN9IW1P1_REV_B)
		{
			bsp_mipi_csi_set_version(dev->mipi_sel, 1);
		}
		else
		{
			bsp_mipi_csi_set_version(dev->mipi_sel, 0);
		}
#if defined (CONFIG_ARCH_SUN8IW6P1) || defined(CONFIG_ARCH_SUN8IW8P1)		//fix
		bsp_mipi_csi_set_base_addr(dev->mipi_sel, MIPI_CSI0_REGS_BASE + 0xf0000000);
		bsp_mipi_dphy_set_base_addr(dev->mipi_sel, MIPI_CSI0_REGS_BASE + 0xf0000000 + 0x1000);
#else
		ret = bsp_mipi_csi_set_base_addr(dev->mipi_sel, 0);
		if(ret < 0)
			goto free_resource;
		ret = bsp_mipi_dphy_set_base_addr(dev->mipi_sel, 0);    
		if(ret < 0)
			goto free_resource;
#endif
	}
#endif

	bsp_isp_init_platform(dev->platform_id);
	bsp_isp_set_base_addr((unsigned int)dev->regs.isp_regs);
	bsp_isp_set_map_load_addr((unsigned int)dev->regs.isp_load_regs);
	bsp_isp_set_map_saved_addr((unsigned int)dev->regs.isp_saved_regs);
	memset((unsigned int*)dev->regs.isp_load_regs,0,ISP_LOAD_REG_SIZE);
	memset((unsigned int*)dev->regs.isp_saved_regs,0,ISP_SAVED_REG_SIZE);

	/*initial parameter */
	strcpy(dev->ch[0].fmt.name,"channel 0 format");
	strcpy(dev->ch[1].fmt.name,"channel 1 format");
	strcpy(dev->ch[2].fmt.name,"channel 2 format");
	strcpy(dev->ch[3].fmt.name,"channel 3 format");

	dev->fmt = &dev->ch[0].fmt;
	dev->total_bus_ch = 1;
	dev->total_rx_ch = 1;
	dev->cur_ch = 0;
	dev->arrange.row = 1;
	dev->arrange.column = 1;
	dev->isp_init_para.isp_src_ch_mode = ISP_SINGLE_CH;
	for(i=0;i<MAX_ISP_SRC_CH_NUM;i++)
		dev->isp_init_para.isp_src_ch_en[i] = 0;
	dev->isp_init_para.isp_src_ch_en[dev->id] = 1;

	//=======================================

	/* init video dma queues */
	INIT_LIST_HEAD(&dev->vidq.active);

	//init_waitqueue_head(&dev->vidq.wq);
	INIT_WORK(&dev->resume_work, resume_work_handle);
	INIT_DELAYED_WORK(&dev->probe_work, probe_work_handle);
	mutex_init(&dev->standby_lock);
	mutex_init(&dev->stream_lock);
	mutex_init(&dev->opened_lock);
	sema_init(&dev->standby_seq_sema,1);

	schedule_delayed_work(&dev->probe_work,msecs_to_jiffies(1));

	/* initial state */
	dev->capture_mode = V4L2_MODE_PREVIEW;

	//=======================================
	return 0;

	//rel_vdev:
	//  video_device_release(vfd);
	//unreg_dev:
	//  v4l2_device_unregister(&dev->v4l2_dev);
	free_resource:
	//vfe_resource_release(dev);
	error:
	vfe_err("failed to install\n");
	return ret;
}

static int vfe_release(void)
{
	struct vfe_dev *dev;
	struct list_head *list;
	//  unsigned int input_num;
	//  int ret;
	vfe_dbg(0,"vfe_release\n");
	while (!list_empty(&devlist))
	{
		list = devlist.next;
		list_del(list);
		dev = list_entry(list, struct vfe_dev, devlist);
#ifdef CONFIG_ES
		unregister_early_suspend(&dev->early_suspend);
#endif
		kfree(dev);
	}
	vfe_print("vfe_release ok!\n");
	return 0;
}

static int __devexit vfe_remove(struct platform_device *pdev)
{
	struct vfe_dev *dev;
	int input_num;
	dev=(struct vfe_dev *)dev_get_drvdata(&(pdev)->dev);

	mutex_destroy(&dev->stream_lock);
	mutex_destroy(&dev->standby_lock);
	mutex_destroy(&dev->opened_lock);
#ifdef USE_SPECIFIC_CCI
	//disable csi cci:
	csi_cci_bus_unmatch_helper(dev->vip_sel);
	//csi_cci_exit_helper(dev->vip_sel);
#endif
	vfe_disable_regulator_all(dev);
	vfe_put_regulator(dev);
	for(input_num=0; input_num<dev->dev_qty; input_num++)
	{

#ifdef _REGULATOR_CHANGE_
#else
		vfe_device_regulator_put(dev->ccm_cfg[input_num]);
#endif
		if(!dev->ccm_cfg[input_num]->sensor_cfg_ini)
		{
			kfree(dev->ccm_cfg[input_num]->sensor_cfg_ini);
		}
	}
	vfe_resource_release(dev);
	v4l2_info(&dev->v4l2_dev, "unregistering %s\n", video_device_node_name(dev->vfd));
	//video_device_release(dev->vfd);
	video_unregister_device(dev->vfd);
	v4l2_device_unregister(&dev->v4l2_dev);
	//  kfree(dev);
	vfe_print("vfe_remove ok!\n");
	return 0;
}

static int vfe_suspend_helper(struct vfe_dev *dev)
{
	int ret = 0;
	unsigned int input_num;
	vfe_print("vfe_suspend_helper\n");
	if(!IS_ERR_OR_NULL(dev->power) && dev->power->stby_mode == NORM_STBY) {
		vfe_print("Set camera to standy off and power off !\n");
#ifdef USE_SPECIFIC_CCI
		vfe_clk_open(dev);
		csi_cci_init_helper(dev->vip_sel);
#endif
		//close all the device power
		for (input_num=0; input_num<dev->dev_qty; input_num++) {
			/* update target device info and select it */
			update_ccm_info(dev, dev->ccm_cfg[input_num]);
			ret = v4l2_subdev_call(dev->sd,core, s_power, CSI_SUBDEV_STBY_OFF);
			if (ret!=0) {
				vfe_err("sensor standby off error at device number %d when vfe_suspend!\n",input_num);
			} else {
				vfe_dbg(0,"sensor standby off ok at device number %d when vfe_suspend!\n",input_num);
			}
			ret = vfe_set_sensor_power_off(dev);
			if (ret!=0) {
				vfe_err("sensor power off error at device number %d when vfe_suspend!\n",input_num);
			} else {
				vfe_dbg(0,"sensor power off ok at device number %d when vfe_suspend!\n",input_num);
			}
		}
#ifdef USE_SPECIFIC_CCI
		csi_cci_exit_helper(dev->vip_sel);
		vfe_clk_close(dev);
#endif
	}//POWER_OFF do nothing
	dev->vfe_standby_poweroff_flag = 1;
	vfe_request_pin(dev, 0);
	vfe_gpio_config(dev, 0);
	vfe_disable_regulator_all(dev);
	vfe_print("vfe_suspend done!\n");
	return ret;
}
static void resume_work_handle(struct work_struct *work)
{
	struct vfe_dev *dev= container_of(work, struct vfe_dev, resume_work);
	int ret = 0;
	unsigned int input_num;
	mutex_lock(&dev->standby_lock);
	vfe_print("resume_work_handle, vip_sel = %d!\n",dev->vip_sel);
	vfe_enable_regulator_all(dev);
	if(0 == dev->vfe_standby_poweroff_flag)
		goto resume_end;
	vfe_request_pin(dev, 1);
	vfe_gpio_config(dev, 1);
	if(!IS_ERR_OR_NULL(dev->power) && dev->power->stby_mode == NORM_STBY) {

#ifdef USE_SPECIFIC_CCI
		vfe_clk_open(dev);
#endif
		//open all the device power
		for (input_num=0; input_num<dev->dev_qty; input_num++) {
			/* update target device info and select it */
			update_ccm_info(dev, dev->ccm_cfg[input_num]);
			ret = vfe_set_sensor_power_on(dev);
			if (ret!=0) {
				vfe_err("sensor power on error at device number %d when vfe_resume!\n",input_num);
			}	else {
				vfe_dbg(0,"sensor power on ok at device number %d when vfe_resume!\n",input_num);
			}
#ifdef USE_SPECIFIC_CCI
			csi_cci_init_helper(dev->vip_sel);
#endif
			ret = v4l2_subdev_call(dev->sd,core, s_power, CSI_SUBDEV_STBY_ON);
			if (ret!=0) {
				vfe_err("sensor standby on error at device number %d when vfe_resume!\n",input_num);
			}	else {
				vfe_dbg(0,"sensor standby on ok at device number %d when vfe_resume!\n",input_num);
			}
#ifdef USE_SPECIFIC_CCI
			csi_cci_exit_helper(dev->vip_sel);
#endif
		}
#ifdef USE_SPECIFIC_CCI
			vfe_clk_close(dev);
#endif
	}//POWER_OFF do nothing
resume_end:
	dev->vfe_standby_poweroff_flag = 0;
	up(&dev->standby_seq_sema);
	vfe_print("vfe resume work end!\n");
	mutex_unlock(&dev->standby_lock);
}

#ifdef CONFIG_ES
static int vfe_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct vfe_dev *dev=(struct vfe_dev *)dev_get_drvdata(&pdev->dev);
	mutex_lock(&dev->standby_lock);
	vfe_print("vfe suspend\n");
	if(0 == dev->early_suspend_valid_flag)
	{
		if(down_timeout(&dev->standby_seq_sema, 4*HZ))
		{
			vfe_err("Get standby sema Error!\n");
		}
		if(vfe_is_opened(dev)) {
			vfe_err("FIXME: dev %s, err happened when calling %s.", dev_name(&dev->pdev->dev), __func__);
			goto suspend_end;
		}
		vfe_print("vfe power off in suspend\n");
		vfe_suspend_helper(dev);
	}
suspend_end:
	mutex_unlock(&dev->standby_lock);
	return 0;
}
static int vfe_resume(struct platform_device *pdev)
{
	struct vfe_dev *dev=(struct vfe_dev *)dev_get_drvdata(&(pdev)->dev);
	vfe_print("vfe resume\n");
	if(0 == dev->early_suspend_valid_flag)
	{
		vfe_print("vfe power on in resume\n");
		schedule_work(&dev->resume_work);
	}
	return 0;
}

static void vfe_early_suspend(struct early_suspend *h)
{
	struct vfe_dev *dev = container_of(h, struct vfe_dev, early_suspend);
	vfe_print("vfe early suspend\n");
	mutex_lock(&dev->standby_lock);
	if(down_timeout(&dev->standby_seq_sema, 2*HZ))
	{
		vfe_err("Enter early suspend, Get standby sema Error!\n");
	}
	if(vfe_is_opened(dev)) {
		vfe_print("Enter early suspend, but vfe is opened, power off vfe in suspend later!");
		dev->early_suspend_valid_flag = 0;
		goto suspend_end;
	}
	vfe_print("vfe power off in early suspend\n");
	vfe_suspend_helper(dev);
	dev->early_suspend_valid_flag = 1;
suspend_end:
	mutex_unlock(&dev->standby_lock);
}
static void vfe_late_resume(struct early_suspend *h)
{
	struct vfe_dev *dev = container_of(h, struct vfe_dev, early_suspend);
	vfe_print("vfe late resume\n");
	if(1 == dev->early_suspend_valid_flag)
	{
		vfe_print("vfe power on in late resume\n");
		schedule_work(&dev->resume_work);
	}
}
#else
static int vfe_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct vfe_dev *dev=(struct vfe_dev *)dev_get_drvdata(&pdev->dev);
	vfe_print("vfe suspend\n");
	mutex_lock(&dev->standby_lock);

	if(down_timeout(&dev->standby_seq_sema, 4*HZ))
	{
		vfe_err("Get standby sema Error!\n");
	}
	if(vfe_is_opened(dev)) {
		vfe_err("FIXME: dev %s, err happened when calling %s.", dev_name(&dev->pdev->dev), __func__);
		goto suspend_end;
	}
	vfe_print("vfe power off in suspend\n");
	vfe_suspend_helper(dev);
suspend_end:
	mutex_unlock(&dev->standby_lock);
	return 0;
}
static int vfe_resume(struct platform_device *pdev)
{
	struct vfe_dev *dev=(struct vfe_dev *)dev_get_drvdata(&(pdev)->dev);
	vfe_print("vfe resume\n");
	schedule_work(&dev->resume_work);
	return 0;
}
#endif
static void vfe_shutdown(struct platform_device *pdev)
{
#if defined (CONFIG_ES) || defined(CONFIG_SUSPEND)
	vfe_print("Defined suspend!\n");
#else
	struct vfe_dev *dev=(struct vfe_dev *)dev_get_drvdata(&pdev->dev);
	unsigned int input_num;
	int ret = 0;
	//close all the device power
	vfe_print("vfe_opened_num = %d\n", vfe_opened_num);
	if(vfe_opened_num > 0)
	{
		for (input_num=0; input_num<dev->dev_qty; input_num++) {
			/* update target device info and select it */
			update_ccm_info(dev, dev->ccm_cfg[input_num]);
			ret = vfe_set_sensor_power_off(dev);
			if (ret!=0) {
				vfe_err("sensor power off error at device number %d when csi close!\n",input_num);
			}
		}
	}
#endif
	vfe_print("Vfe Shutdown!\n");
}

static struct platform_driver vfe_driver = {
	.probe    = vfe_probe,
	.remove   = __devexit_p(vfe_remove),
	.suspend  = vfe_suspend,
	.resume   = vfe_resume,
	.shutdown = vfe_shutdown,
	//.id_table = vfe_driver_ids,
	.driver = {
		.name   = VFE_MODULE_NAME,
		.owner  = THIS_MODULE,
	}
};

static struct sunxi_vip_platform_data sunxi_vip0_pdata[] = {
	{
		.mipi_sel  = 0,
		.vip_sel   = 0,
		.isp_sel   = 0,
	},
};

static struct sunxi_vip_platform_data sunxi_vip1_pdata[] = {
	{
		.mipi_sel  = 1,
		.vip_sel   = 1,
		.isp_sel   = 0,
	},
};

static void vfe_device_release(struct device *dev)
{
	//struct vfe_dev *vfe_dev = (struct vfe_dev *)dev_get_drvdata(dev);
	vfe_print("vfe_device_release\n");
};

static struct platform_device vfe_device[] = {
	[0] = {
		.name  = VFE_MODULE_NAME,
		.id = 0,
		.num_resources = ARRAY_SIZE(vfe_vip0_resource),
		.resource = vfe_vip0_resource,
		.dev = {
			.platform_data  = sunxi_vip0_pdata,
			.release        = vfe_device_release,
		},
	},
	[1] = {
		.name = VFE_MODULE_NAME,
		.id = 1,
		.num_resources = ARRAY_SIZE(vfe_vip1_resource),
		.resource = vfe_vip1_resource,
		.dev = {
			.platform_data = sunxi_vip1_pdata,
			.release = vfe_device_release,
		},
	},
};

static int __init vfe_init(void)
{
	int ret,i;
	unsigned int vfe_used[MAX_VFE_INPUT];

#ifdef VFE_SYS_CONFIG
	script_item_u   val;
	script_item_value_type_e  type;

	char vfe_para[16] = {0};
	for(i=0; i<MAX_VFE_INPUT; i++) {
		sprintf(vfe_para, "csi%d", i);
		type = script_get_item(vfe_para,"vip_used", &val);
		if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
			vfe_err("fetch %s vip_used from sys_config failed\n",vfe_para);
			return -1;
		} else {
			vfe_used[i] = val.val;
		}
	}
#else
#if defined (CONFIG_ARCH_SUN8IW3P1) || defined(CONFIG_ARCH_SUN8IW5P1) || defined(CONFIG_ARCH_SUN8IW6P1) || defined (CONFIG_ARCH_SUN8IW7P1)
	vfe_used[0] = 1;
	vfe_used[1] = 0;
#else
	vfe_used[0] = 0;
	vfe_used[1] = 1;
#endif
#endif

	vfe_print("Welcome to Video Front End driver\n");
	mutex_init(&probe_hdl_lock);
	for(i=0; i<MAX_VFE_INPUT; i++) {
		if(vfe_used[i]) {
		ret = platform_device_register(&vfe_device[i]);
		if (ret)
			vfe_err("platform device %d register failed\n",i);
		}
	}
	ret = platform_driver_register(&vfe_driver);
	if (ret) {
		vfe_err("platform driver register failed\n");
		return ret;
	}
	vfe_print("vfe_init end\n");
	return 0;
}

static void __exit vfe_exit(void)
{
	unsigned int vfe_used[MAX_VFE_INPUT];
	int i;
#ifdef VFE_SYS_CONFIG
	char vfe_para[16] = {0};
	script_item_u   val;
	script_item_value_type_e  type;
	for(i=0; i<MAX_VFE_INPUT; i++) {
		sprintf(vfe_para, "csi%d", i);
		type = script_get_item(vfe_para,"vip_used", &val);
		if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
			vfe_err("fetch %s vip_used from sys_config failed\n",vfe_para);
			return ;
		} else {
			vfe_used[i] = val.val;
		}
	}
#else
#if defined (CONFIG_ARCH_SUN8IW3P1) || defined(CONFIG_ARCH_SUN8IW5P1) || defined (CONFIG_ARCH_SUN8IW7P1)
	vfe_used[0] = 1;
	vfe_used[1] = 0;
#else
	vfe_used[0] = 0;
	vfe_used[1] = 1;
#endif
#endif
	vfe_print("vfe_exit\n");
	for(i=0; i<MAX_VFE_INPUT; i++) {
		if(vfe_used[i])
		{
			platform_device_unregister(&vfe_device[i]);
			vfe_print("platform_device_unregister[%d]\n",i);
		}
	}
	platform_driver_unregister(&vfe_driver);
	vfe_print("platform_driver_unregister\n");
	vfe_release();
	mutex_destroy(&probe_hdl_lock);
	vfe_print("vfe_exit end\n");
}

module_init(vfe_init);
module_exit(vfe_exit);

MODULE_AUTHOR("raymonxiu");
MODULE_LICENSE("Dual BSD/GPL");
MODULE_DESCRIPTION("Video front end driver for sunxi");


