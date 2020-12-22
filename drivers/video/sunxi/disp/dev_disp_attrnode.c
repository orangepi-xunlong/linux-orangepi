#include "dev_disp.h"

extern fb_info_t g_fbi;

extern s32 disp_video_set_dit_mode(u32 scaler_index, u32 mode);
extern s32 disp_video_get_dit_mode(u32 scaler_index);
extern s32 disp_get_frame_count(u32 screen_id, char* buf);

static u32 sel;
static u32 hid;
u32 fastboot;
//#define DISP_CMD_CALLED_BUFFER_LEN 100
//static u32 disp_cmd_called[DISP_CMD_CALLED_BUFFER_LEN];
//static u32 disp_cmd_called_index;

#define ____SEPARATOR_GLABOL_NODE____

static ssize_t disp_sel_show(struct device *dev,
    struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", sel);
}

static ssize_t disp_sel_store(struct device *dev,
        struct device_attribute *attr,
        const char *buf, size_t count)
{
	int err;
	unsigned long val;

	err = strict_strtoul(buf, 10, &val);
	if (err) {
		printk("Invalid size\n");
		return err;
	}

    if((val>2))
    {
        printk("Invalid value, 0/1/2 is expected!\n");
    }else
    {
        printk("%ld\n", val);
        sel = val;
  }

  return count;
}

static ssize_t disp_fastboot_show(struct device *dev,
    struct device_attribute *attr, char *buf)
{
  return sprintf(buf, "%d\n", fastboot);
}

static ssize_t disp_fastboot_store(struct device *dev,
        struct device_attribute *attr,
        const char *buf, size_t count)
{
  int err;
    unsigned long val;

  err = strict_strtoul(buf, 10, &val);
  if (err) {
    printk("Invalid size\n");
    return err;
  }

    if((val>1))
    {
        printk("Invalid value, 0/1 is expected!\n");
    }else
    {
        printk("%ld\n", val);
        fastboot = val;
  	}
  return count;
}

static ssize_t disp_hid_show(struct device *dev,
    struct device_attribute *attr, char *buf)
{
  return sprintf(buf, "%d\n", HANDTOID(hid));
}

static ssize_t disp_hid_store(struct device *dev,
        struct device_attribute *attr,
        const char *buf, size_t count)
{
  int err;
    unsigned long val;

  err = strict_strtoul(buf, 10, &val);
  if (err) {
    printk("Invalid size\n");
    return err;
  }

    if(val>3)
    {
        printk("Invalid value, 0~3 is expected!\n");
    }else
    {
        printk("%ld\n", val);
        hid = IDTOHAND(val);
  }

  return count;
}
static DEVICE_ATTR(sel, S_IRUGO|S_IWUSR|S_IWGRP,
    disp_sel_show, disp_sel_store);

static DEVICE_ATTR(fastboot, S_IRUGO|S_IWUSR|S_IWGRP,
    disp_fastboot_show, disp_fastboot_store);

static DEVICE_ATTR(hid, S_IRUGO|S_IWUSR|S_IWGRP,
    disp_hid_show, disp_hid_store);

#if defined(CONFIG_ARCH_SUN9IW1P1)
static ssize_t disp_hdmi_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	if(bsp_disp_get_output_type(sel) == DISP_OUTPUT_TYPE_HDMI)
    {
        return sprintf(buf, "screen%d hdmi on, mode=%d\n", sel, bsp_disp_hdmi_get_mode(sel));
    }
    else
    {
        return sprintf(buf, "screen%d hdmi off!\n", sel);
    }
}

static ssize_t disp_hdmi_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	int err;
    unsigned long val;

	err = strict_strtoul(buf, 10, &val);
	if (err) {
		printk("Invalid size\n");
		return err;
	}

    if((val==0xff))
    {
        bsp_disp_hdmi_disable(sel);
    }else
    {
        bsp_disp_hdmi_disable(sel);
        if(bsp_disp_hdmi_set_mode(sel,(disp_tv_mode)val) == 0)
        {
            bsp_disp_hdmi_enable(sel);
        }
	}

	return count;
}

static DEVICE_ATTR(hdmi, S_IRUGO|S_IWUSR|S_IWGRP,
		disp_hdmi_show, disp_hdmi_store);

static ssize_t disp_fcount_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return disp_get_frame_count(sel, buf);
}

static ssize_t disp_fcount_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	return count;
}

static DEVICE_ATTR(fcount, S_IRUGO|S_IWUSR|S_IWGRP,
		disp_fcount_show, disp_fcount_store);

#endif

static ssize_t disp_xres_show(struct device *dev,
    struct device_attribute *attr, char *buf)
{
  return sprintf(buf, "%d\n", bsp_disp_get_screen_width(sel));
}

static ssize_t disp_yres_show(struct device *dev,
    struct device_attribute *attr, char *buf)
{
  return sprintf(buf, "%d\n", bsp_disp_get_screen_height(sel));
}

static ssize_t disp_xres_store(struct device *dev,
        struct device_attribute *attr,
        const char *buf, size_t count)
{
  return count;
}

static ssize_t disp_yres_store(struct device *dev,
        struct device_attribute *attr,
        const char *buf, size_t count)
{
  return count;
}
static DEVICE_ATTR(xres, S_IRUGO|S_IWUSR|S_IWGRP,
    disp_xres_show, disp_xres_store);
static DEVICE_ATTR(yres, S_IRUGO|S_IWUSR|S_IWGRP,
    disp_yres_show, disp_yres_store);

static ssize_t disp_video_fps_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "screen_id %d : %dHZ\n", sel, bsp_disp_get_fps(sel));
}

static ssize_t disp_video_fps_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	return count;
}

static DEVICE_ATTR(fps, S_IRUGO|S_IWUSR|S_IWGRP,
		disp_video_fps_show, disp_video_fps_store);

static ssize_t disp_sys_show(struct device *dev,
    struct device_attribute *attr, char *buf)
{
  ssize_t count = 0;
	int num_screens, screen_id;
	int num_layers, layer_id;
	//int hpd;

	num_screens = bsp_disp_feat_get_num_screens();
	for(screen_id=0; screen_id < num_screens; screen_id ++) {
		count += sprintf(buf + count, "screen %d:\n", screen_id);
		/* output */
		if(bsp_disp_get_output_type(screen_id) == DISP_OUTPUT_TYPE_LCD) {
			count += sprintf(buf + count, "\tlcd output\tbacklight(%3d)", bsp_disp_lcd_get_bright(screen_id));
			count += sprintf(buf + count, "\t%4dx%4d\n", bsp_disp_get_screen_width(screen_id), 			bsp_disp_get_screen_height(screen_id));
		}
		#if 0
		else if(bsp_disp_get_output_type(screen_id) == DISP_OUTPUT_TYPE_HDMI) {
			count += sprintf(buf + count, "\thdmi output");
			if(bsp_disp_hdmi_get_mode(screen_id) == DISP_TV_MOD_720P_50HZ) {
				count += sprintf(buf + count, "\t%16s", "720p50hz");
			} else if(BSP_disp_hdmi_get_mode(screen_id) == DISP_TV_MOD_720P_60HZ) {
				count += sprintf(buf + count, "\t%16s", "720p60hz");
			} else if(BSP_disp_hdmi_get_mode(screen_id) == DISP_TV_MOD_1080P_60HZ) {
				count += sprintf(buf + count, "\t%16s", "1080p60hz");
			} else if(BSP_disp_hdmi_get_mode(screen_id) == DISP_TV_MOD_1080P_50HZ) {
				count += sprintf(buf + count, "\t%16s", "1080p50hz");
			} else if(BSP_disp_hdmi_get_mode(screen_id) == DISP_TV_MOD_1080I_50HZ) {
				count += sprintf(buf + count, "\t%16s", "1080i50hz");
			} else if(BSP_disp_hdmi_get_mode(screen_id) == DISP_TV_MOD_1080I_60HZ) {
				count += sprintf(buf + count, "\t%16s", "1080i60hz");
			}
			count += sprintf(buf + count, "\t%4dx%4d\n", BSP_disp_get_screen_width(screen_id), BSP_disp_get_screen_height(screen_id));
		}

		hpd = bsp_disp_hdmi_get_hpd_status(screen_id);
		count += sprintf(buf + count, "\t%11s\n", hpd? "hdmi plugin":"hdmi unplug");
		#endif
		count += sprintf(buf + count, "    type  |  status  |  id  | pipe | z | pre_mult |    alpha   | colorkey | fmt | framebuffer |  	   source crop      |       frame       |   trd   |         address\n");
		count += sprintf(buf + count, "----------+--------+------+------+---+----------+------------+----------+-----+-------------+-----------------------+-------------------+---------+-----------------------------\n");
		num_layers = bsp_disp_feat_get_num_layers(screen_id);
		/* layer info */
		for(layer_id=0; layer_id<num_layers; layer_id++) {
			disp_layer_info layer_para;
			int ret;
			int enabled = 0;

			ret = bsp_disp_layer_get_info(screen_id, layer_id, &layer_para);
			enabled = bsp_disp_layer_is_enabled(screen_id, layer_id);
			if(ret == 0) {
				count += sprintf(buf + count, " %8s |", (layer_para.mode == DISP_LAYER_WORK_MODE_SCALER)? "SCALER":"NORAML");
				count += sprintf(buf + count, " %8s |", (enabled==1)?"enable":"disable");
				count += sprintf(buf + count, " %4d |", layer_id);
				count += sprintf(buf + count, " %4d |", layer_para.pipe);
				count += sprintf(buf + count, " %1d |", layer_para.zorder);
				count += sprintf(buf + count, " %8s |", (layer_para.fb.pre_multiply)? "Y":"N");
				count += sprintf(buf + count, " %5s(%3d) |", (layer_para.alpha_mode)? "globl":"pixel", layer_para.alpha_value);
				count += sprintf(buf + count, " %8s |", (layer_para.ck_enable)? "enable":"disable");
				count += sprintf(buf + count, " %3d |", layer_para.fb.format);
				count += sprintf(buf + count, " [%4d,%4d] |", layer_para.fb.size.width, layer_para.fb.size.height);
				count += sprintf(buf + count, " [%4d,%4d,%4d,%4d] |", layer_para.fb.src_win.x, layer_para.fb.src_win.y, layer_para.fb.src_win.width, layer_para.fb.src_win.height);
				count += sprintf(buf + count, " [%4d,%4d,%4d,%4d] |", layer_para.screen_win.x, layer_para.screen_win.y, layer_para.screen_win.width, layer_para.screen_win.height);
				count += sprintf(buf + count, " [%1d%1d,%1d%1d] |", layer_para.fb.b_trd_src, layer_para.fb.trd_mode, layer_para.b_trd_out, layer_para.out_trd_mode);
				count += sprintf(buf + count, " [%8x,%8x,%8x] |", layer_para.fb.addr[0], layer_para.fb.addr[1], layer_para.fb.addr[2]);
				count += sprintf(buf + count, "\n");
			}
		}
		//count += sprintf(buf + count, "\n\tsmart backlight: %s\n", bsp_disp_drc_get_enable(screen_id)? "enable":"disable");

	}

	return count;
}

static ssize_t disp_sys_store(struct device *dev,
        struct device_attribute *attr,
        const char *buf, size_t count)
{
  return count;
}

static DEVICE_ATTR(sys, S_IRUGO|S_IWUSR|S_IWGRP,
    disp_sys_show, disp_sys_store);

#define ____SEPARATOR_DEBUG____
static ssize_t disp_debug_show(struct device *dev,
    struct device_attribute *attr, char *buf)
{
  return sprintf(buf, "%s\n", bsp_disp_get_print_level()?"on" : "off");
}

static ssize_t disp_debug_store(struct device *dev,
        struct device_attribute *attr,
        const char *buf, size_t count)
{
	if (count < 1)
		return -EINVAL;

	if (strnicmp(buf, "on", 2) == 0 || strnicmp(buf, "1", 1) == 0){
		bsp_disp_set_print_level(1);
	}	else if (strnicmp(buf, "off", 3) == 0 || strnicmp(buf, "0", 1) == 0) {
		bsp_disp_set_print_level(0);
	}	else if (strnicmp(buf, "2", 1) == 0) {
		bsp_disp_set_print_level(2);
	}	else {
		return -EINVAL;
	}

  return count;
}

static DEVICE_ATTR(debug, S_IRUGO|S_IWUSR|S_IWGRP,
    disp_debug_show, disp_debug_store);

#define ____SEPARATOR_LCD____
extern s32 drv_lcd_enable(u32 sel);
extern s32 drv_lcd_disable(u32 sel);
static ssize_t disp_lcd_show(struct device *dev,
    struct device_attribute *attr, char *buf)
{
  if(bsp_disp_get_output_type(sel) == DISP_OUTPUT_TYPE_LCD)
    {
        return sprintf(buf, "screen%d lcd on!\n", sel);
    }
    else
    {
        return sprintf(buf, "screen%d lcd off!\n", sel);
    }
}

static ssize_t disp_lcd_store(struct device *dev,
        struct device_attribute *attr,
        const char *buf, size_t count)
{
  int err;
    unsigned long val;

  err = strict_strtoul(buf, 10, &val);
  if (err) {
    printk("Invalid size\n");
    return err;
  }

    if((val==0))
    {
        drv_lcd_disable(sel);
    }else
    {
        drv_lcd_enable(sel);
  }

  return count;
}

static DEVICE_ATTR(lcd, S_IRUGO|S_IWUSR|S_IWGRP,
    disp_lcd_show, disp_lcd_store);

static ssize_t disp_lcdbl_show(struct device *dev,
    struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", bsp_disp_lcd_get_bright(sel));
}

static ssize_t disp_lcdbl_store(struct device *dev,
        struct device_attribute *attr,
        const char *buf, size_t count)
{
	int err;
	unsigned long val;

	err = strict_strtoul(buf, 10, &val);
	if (err) {
		printk("Invalid size\n");
		return err;
	}

	bsp_disp_lcd_set_bright(sel, val);

	return count;
}

#define ____SEPARATOR_COLORBAR____
static ssize_t disp_colorbar_show(struct device *dev,
    struct device_attribute *attr, char *buf)
{
  return sprintf(buf, "%s\n", "there is nothing here!");
}

extern __s32 fb_draw_colorbar(__u32 base, __u32 width, __u32 height, struct fb_var_screeninfo *var);

static ssize_t disp_colorbar_store(struct device *dev,
        struct device_attribute *attr,
        const char *buf, size_t count)
{
  int err;
    unsigned long val;

  err = strict_strtoul(buf, 10, &val);
  if (err) {
    printk("Invalid size\n");
    return err;
  }

    if((val>7)) {
        printk("Invalid value, 0~7 is expected!\n");
    }
    else {
        fb_draw_colorbar((__u32 __force)g_fbi.fbinfo[val]->screen_base, g_fbi.fbinfo[val]->var.xres,
            g_fbi.fbinfo[val]->var.yres, &(g_fbi.fbinfo[val]->var));;
  }

  return count;
}

static DEVICE_ATTR(colorbar, S_IRUGO|S_IWUSR|S_IWGRP,
    disp_colorbar_show, disp_colorbar_store);

static DEVICE_ATTR(lcdbl, S_IRUGO|S_IWUSR|S_IWGRP,
    disp_lcdbl_show, disp_lcdbl_store);

static struct attribute *disp_attributes[] = {
    &dev_attr_sel.attr,
	&dev_attr_fastboot.attr,
    &dev_attr_hid.attr,
    &dev_attr_lcd.attr,
    &dev_attr_lcdbl.attr,
#if defined(CONFIG_ARCH_SUN9IW1P1)
    &dev_attr_hdmi.attr,
    &dev_attr_fcount.attr,
#endif
    &dev_attr_xres.attr,
    &dev_attr_yres.attr,
    &dev_attr_sys.attr,
    &dev_attr_debug.attr,
    &dev_attr_colorbar.attr,
    &dev_attr_fps.attr,
    NULL
};

static struct attribute_group disp_attribute_group = {
  .name = "attr",
  .attrs = disp_attributes
};

int disp_attr_node_init(struct device *display_dev)
{
    unsigned int ret;

    ret = sysfs_create_group(&display_dev->kobj,
                             &disp_attribute_group);
    sel = 0;
    hid = 100;
	fastboot = 0;
    return 0;
}

int disp_attr_node_exit(void)
{
    return 0;
}

