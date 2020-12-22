#include "dev_disp.h"

extern fb_info_t g_fbi;

extern __s32 disp_video_set_dit_mode(__u32 scaler_index, __u32 mode);
extern __s32 disp_video_get_dit_mode(__u32 scaler_index);

static __u32 sel;
static __u32 hid;
//#define DISP_CMD_CALLED_BUFFER_LEN 100
//static __u32 disp_cmd_called[DISP_CMD_CALLED_BUFFER_LEN];
//static __u32 disp_cmd_called_index;

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

    if((val>1))
    {
        printk("Invalid value, 0/1 is expected!\n");
    }else
    {
        printk("%ld\n", val);
        sel = val;
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

static DEVICE_ATTR(hid, S_IRUGO|S_IWUSR|S_IWGRP,
    disp_hid_show, disp_hid_store);

static ssize_t disp_sys_status_show(struct device *dev,
    struct device_attribute *attr, char *buf)
{
  ssize_t count = 0;
	int num_screens, screen_id;
	int num_layers, layer_id;

	num_screens = bsp_disp_feat_get_num_screens();
	for(screen_id=0; screen_id < num_screens; screen_id ++) {
		count += sprintf(buf + count, "screen %d:\n", screen_id);
		/* output */
		if(bsp_disp_get_output_type(screen_id) == DISP_OUTPUT_TYPE_LCD) {
			count += sprintf(buf + count, "\tlcd output\tbacklight(%3d)", bsp_disp_lcd_get_bright(screen_id));
		} else if(bsp_disp_get_output_type(screen_id) == DISP_OUTPUT_TYPE_HDMI) {
			count += sprintf(buf + count, "\thdmi output");
			if(bsp_disp_hdmi_get_mode(screen_id) == DISP_TV_MOD_720P_50HZ) {
				count += sprintf(buf + count, "%16s", "720p50hz");
			} else if(bsp_disp_hdmi_get_mode(screen_id) == DISP_TV_MOD_720P_60HZ) {
				count += sprintf(buf + count, "%16s", "720p60hz");
			} else if(bsp_disp_hdmi_get_mode(screen_id) == DISP_TV_MOD_1080P_60HZ) {
				count += sprintf(buf + count, "%16s", "1080p60hz");
			} else if(bsp_disp_hdmi_get_mode(screen_id) == DISP_TV_MOD_1080P_50HZ) {
				count += sprintf(buf + count, "%16s", "1080p50hz");
			} else if(bsp_disp_hdmi_get_mode(screen_id) == DISP_TV_MOD_1080I_50HZ) {
				count += sprintf(buf + count, "%16s", "1080i50hz");
			} else if(bsp_disp_hdmi_get_mode(screen_id) == DISP_TV_MOD_1080I_60HZ) {
				count += sprintf(buf + count, "%16s", "1080i60hz");
			}
		}

		if(bsp_disp_get_output_type(screen_id) != DISP_OUTPUT_TYPE_NONE) {
			count += sprintf(buf + count, "\t%4dx%4d", bsp_disp_get_screen_width(screen_id), bsp_disp_get_screen_height(screen_id));
			count += sprintf(buf + count, "\tfps(%3d.%1d)", bsp_disp_get_fps(screen_id)/10, bsp_disp_get_fps(screen_id)%10);
			count += sprintf(buf + count, "\n");
		}
		/* hdmi hpd */
		if((bsp_disp_feat_get_supported_output_types(screen_id) & DISP_OUTPUT_TYPE_HDMI)) {
			int hpd;

			hpd = bsp_disp_hdmi_get_hpd_status(screen_id);
			count += sprintf(buf + count, "\t%11s\n", hpd? "hdmi plugin":"hdmi unplug");
		}
		count += sprintf(buf + count, "    type  |  status | id | pipe | z | pre_mult |    alpha   | colorkey |  format  | framebuffer |       source crop     |          frame        |   trd   |         address\n");
		count += sprintf(buf + count, "----------+---------+----+------+---+----------+------------+----------+----------+-------------+-----------------------+-----------------------+---------+-----------------------------\n");
		num_layers = bsp_disp_feat_get_num_layers(screen_id);
		/* layer info */
		for(layer_id=0; layer_id<num_layers; layer_id++) {
			__disp_layer_info_t layer_para;
			int ret;

			ret = bsp_disp_layer_get_para(screen_id, IDTOHAND(layer_id), &layer_para);
			if(ret == 0) {
				count += sprintf(buf + count, " %8s |", (layer_para.mode == DISP_LAYER_WORK_MODE_SCALER)? "SCALER":"NORAML");
				count += sprintf(buf + count, " %7s |", bsp_disp_layer_is_open(screen_id, IDTOHAND(layer_id))?"enable":"disable");
				count += sprintf(buf + count, " %2d |", layer_id);
				count += sprintf(buf + count, " %4d |", layer_para.pipe);
				count += sprintf(buf + count, " %1d |", layer_para.prio);
				count += sprintf(buf + count, " %8s |", (layer_para.fb.pre_multiply)? "Y":"N");
				count += sprintf(buf + count, " %5s(%3d) |", (layer_para.alpha_en)? "globl":"pixel", layer_para.alpha_val);
				count += sprintf(buf + count, " %8s |", (layer_para.ck_enable)? "enable":"disable");
				count += sprintf(buf + count, " %2d,%2d,%2d |", layer_para.fb.mode, layer_para.fb.format, layer_para.fb.seq);
				count += sprintf(buf + count, " [%4d,%4d] |", layer_para.fb.size.width, layer_para.fb.size.height);
				count += sprintf(buf + count, " [%4d,%4d,%4d,%4d] |", layer_para.src_win.x, layer_para.src_win.y, layer_para.src_win.width, layer_para.src_win.height);
				count += sprintf(buf + count, " [%4d,%4d,%4d,%4d] |", layer_para.scn_win.x, layer_para.scn_win.y, layer_para.scn_win.width, layer_para.scn_win.height);
				count += sprintf(buf + count, " [%1d%1d,%1d%1d] |", layer_para.fb.b_trd_src, layer_para.fb.trd_mode, layer_para.b_trd_out, layer_para.out_trd_mode);
				count += sprintf(buf + count, " [%8x,%8x,%8x]", layer_para.fb.addr[0], layer_para.fb.addr[1], layer_para.fb.addr[2]);
				count += sprintf(buf + count, "\n");
			}
		}
		if(bsp_disp_feat_get_smart_backlight_support(screen_id)) {
			__disp_rect_t window;
			count += sprintf(buf + count, "\n\tsmart backlight: %s", bsp_disp_drc_get_enable(screen_id)? "enable":"disable");
			if(bsp_disp_drc_get_enable(screen_id)) {
				bsp_disp_drc_get_window(screen_id, &window);
				count += sprintf(buf + count, "\twindow[%4d,%4d,%4d,%4d]", window.x, window.y, window.width, window.height);
			}
			count += sprintf(buf + count, "\n");
		}
	}

	return count;
}

static ssize_t disp_sys_status_store(struct device *dev,
        struct device_attribute *attr,
        const char *buf, size_t count)
{
  return count;
}

static DEVICE_ATTR(sys_status, S_IRUGO|S_IWUSR|S_IWGRP,
    disp_sys_status_show, disp_sys_status_store);

#define ____SEPARATOR_STANDBY____
static ssize_t disp_standby_show(struct device *dev,
    struct device_attribute *attr, char *buf)
{
  return sprintf(buf, "no thing here!\n");
}

extern int disp_suspend(struct platform_device *pdev, pm_message_t state);
extern int disp_resume(struct platform_device *pdev);
static ssize_t disp_standby_store(struct device *dev,
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

    if(val != 0)
    {
        pm_message_t state;
        memset(&state, 0, sizeof(pm_message_t));
				printk("enter standby!\n");
				disp_suspend(NULL, state);
    }else
    {
        printk("exit standby!\n");
				disp_resume(NULL);
		}

  return count;
}
static DEVICE_ATTR(standby, S_IRUGO|S_IWUSR|S_IWGRP,
    disp_standby_show, disp_standby_store);

#define ____SEPARATOR_REG_DUMP____
static __disp_mod_id_t mod_id;
static ssize_t disp_reg_dump_show(struct device *dev,
    struct device_attribute *attr, char *buf)
{
	return bsp_disp_print_reg(1, mod_id, buf);
}

static ssize_t disp_reg_dump_store(struct device *dev,
        struct device_attribute *attr,
        const char *buf, size_t count)
{
  if (count < 1)
        return -EINVAL;

    if(strnicmp(buf, "be0", 3) == 0)
    {
        mod_id = DISP_MOD_BE0;
    }else if(strnicmp(buf, "be1", 3) == 0)
    {
        mod_id = DISP_MOD_BE1;
    }else if(strnicmp(buf, "fe0", 3) == 0)
    {
        mod_id = DISP_MOD_FE0;
    }else if(strnicmp(buf, "fe1", 3) == 0)
    {
        mod_id = DISP_MOD_FE1;
    }else if(strnicmp(buf, "lcd0", 4) == 0)
    {
        mod_id = DISP_MOD_LCD0;
    }else if(strnicmp(buf, "lcd1", 4) == 0)
    {
        mod_id = DISP_MOD_LCD1;
    }else if(strnicmp(buf, "tve0", 4) == 0)
    {
        mod_id = DISP_MOD_TVE0;
    }else if(strnicmp(buf, "tve1", 4) == 0)
    {
        mod_id = DISP_MOD_TVE1;
    }else if(strnicmp(buf, "deu0", 4) == 0)
    {
        mod_id = DISP_MOD_DEU0;
    }else if(strnicmp(buf, "deu1", 4) == 0)
    {
        mod_id = DISP_MOD_DEU1;
    }else if(strnicmp(buf, "cmu0", 4) == 0)
    {
        mod_id = DISP_MOD_CMU0;
    }else if(strnicmp(buf, "cmu1", 4) == 0)
    {
        mod_id = DISP_MOD_CMU1;
    }else if(strnicmp(buf, "drc0", 4) == 0)
    {
        mod_id = DISP_MOD_DRC0;
    }else if(strnicmp(buf, "drc1", 4) == 0)
    {
        mod_id = DISP_MOD_DRC1;
    }else if(strnicmp(buf, "dsi", 3) == 0)
    {
        mod_id = DISP_MOD_DSI0;
    }else if(strnicmp(buf, "dsi_dphy", 8) == 0)
    {
        mod_id = DISP_MOD_DSI0_DPHY;
    }else if(strnicmp(buf, "hdmi", 4) == 0)
    {
        mod_id = DISP_MOD_HDMI;
    }else if(strnicmp(buf, "ccmu", 4) == 0)
    {
        mod_id = DISP_MOD_CCMU;
    }else if(strnicmp(buf, "pioc", 4) == 0)
    {
        mod_id = DISP_MOD_PIOC;
    }else if(strnicmp(buf, "pwm", 3) == 0)
    {
        mod_id = DISP_MOD_PWM;
    }else
    {
        printk("Invalid para!\n");
    }

  return count;
}

static DEVICE_ATTR(reg_dump, S_IRUGO|S_IWUSR|S_IWGRP,
    disp_reg_dump_show, disp_reg_dump_store);

#define ____SEPARATOR_PRINT_CMD_LEVEL____
static ssize_t disp_print_cmd_level_show(struct device *dev,
    struct device_attribute *attr, char *buf)
{
  return sprintf(buf, "%d\n", disp_get_print_cmd_level());
}

static ssize_t disp_print_cmd_level_store(struct device *dev,
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

    if((val != 0) && (val != 1))
    {
        printk("Invalid value, 0/1 is expected!\n");
    }else
    {
        disp_set_print_cmd_level(val);
  }

  return count;
}

static DEVICE_ATTR(print_cmd_level, S_IRUGO|S_IWUSR|S_IWGRP,
    disp_print_cmd_level_show, disp_print_cmd_level_store);

#define ____SEPARATOR_CMD_PRINT_LEVEL____
extern __u32 disp_cmd_print;
static ssize_t disp_cmd_print_show(struct device *dev,
    struct device_attribute *attr, char *buf)
{
  return sprintf(buf, "%x\n", disp_get_cmd_print());
}

static ssize_t disp_cmd_print_store(struct device *dev,
        struct device_attribute *attr,
        const char *buf, size_t count)
{
  int err;
    unsigned long val;

  err = strict_strtoul(buf, 16, &val);
  if (err) {
    printk("Invalid size\n");
    return err;
  }


    disp_set_cmd_print(val);

  return count;
}

static DEVICE_ATTR(cmd_print, S_IRUGO|S_IWUSR|S_IWGRP,
    disp_cmd_print_show, disp_cmd_print_store);


#define ____SEPARATOR_VINT_COUNT____
static ssize_t disp_int_count_show(struct device *dev,
    struct device_attribute *attr, char *buf)
{
  return sprintf(buf, "vint_count=%d, lint_count=%d\n", bsp_disp_get_vint_count(sel), bsp_disp_get_lint_count(sel));
}

static ssize_t disp_int_count_store(struct device *dev,
        struct device_attribute *attr,
        const char *buf, size_t count)
{
  return count;
}

static DEVICE_ATTR(int_count, S_IRUGO|S_IWUSR|S_IWGRP,
    disp_int_count_show, disp_int_count_store);



#define ____SEPARATOR_PRINT_LEVEL____
static ssize_t disp_debug_show(struct device *dev,
    struct device_attribute *attr, char *buf)
{
  return sprintf(buf, "debug=%s\n", bsp_disp_get_print_level()?"on" : "off");
}

static ssize_t disp_debug_store(struct device *dev,
        struct device_attribute *attr,
        const char *buf, size_t count)
{
  if (count < 1)
        return -EINVAL;

    if (strnicmp(buf, "on", 2) == 0 || strnicmp(buf, "1", 1) == 0)
    {
        bsp_disp_set_print_level(1);
  }
    else if (strnicmp(buf, "off", 3) == 0 || strnicmp(buf, "0", 1) == 0)
  {
        bsp_disp_set_print_level(0);
    }
    else if (strnicmp(buf, "2", 1) == 0)
	{
        bsp_disp_set_print_level(2);
    }
    else
    {
        return -EINVAL;
    }

  return count;
}

static DEVICE_ATTR(debug, S_IRUGO|S_IWUSR|S_IWGRP,
    disp_debug_show, disp_debug_store);

#define ____SEPARATOR_CONDITION____
static ssize_t disp_condition_show(struct device *dev,
    struct device_attribute *attr, char *buf)
{
  return sprintf(buf, "conditon=%d\n", bsp_disp_get_condition());
}

static ssize_t disp_condition_store(struct device *dev,
        struct device_attribute *attr,
        const char *buf, size_t count)
{
  unsigned long val;
  int err;

  if (count < 1)
        return -EINVAL;

  err = strict_strtoul(buf, 16, &val);
  if (err) {
    printk("Invalid size\n");
    return err;
  }

  bsp_disp_set_condition(val);

  return count;
}

static DEVICE_ATTR(conditon, S_IRUGO|S_IWUSR|S_IWGRP,
    disp_condition_show, disp_condition_store);

#define ____SEPARATOR_CFG_COUNT____
static ssize_t disp_cfg_cnt_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "cfg_cnt=%d\n", bsp_disp_cfg_get(sel));
}

static ssize_t disp_cfg_cnt_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	return count;
}

static DEVICE_ATTR(cfg_cnt, S_IRUGO|S_IWUSR|S_IWGRP,
		disp_cfg_cnt_show, disp_cfg_cnt_store);

static ssize_t disp_cache_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "cache=%s\n", bsp_disp_cmd_cache_get(sel)? "true":"false");
}

static ssize_t disp_cache_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	return count;
}

static DEVICE_ATTR(cache, S_IRUGO|S_IWUSR|S_IWGRP,
		disp_cache_show, disp_cache_store);

#define ____SEPARATOR_LAYER_PARA____
static ssize_t disp_layer_para_show(struct device *dev,
    struct device_attribute *attr, char *buf)
{
    __disp_layer_info_t layer_para;
    __s32 ret = 0;

    ret = bsp_disp_layer_get_para(sel, hid, &layer_para);
    if(ret != 0)
    {
        return sprintf(buf, "screen%d layer%d is not init\n", sel, HANDTOID(hid));
    }
    else
    {
        return sprintf(buf, "=== screen%d layer%d para ====\nmode: %d\naddr=<%x,%x,%x>\nfb.size=<%dx%d>\nfb.fmt=<%d, %d, %d>\ntrd_src=<%d, %d> trd_out=<%d, %d>\npipe:%d\tprio: %d\nalpha: <%d, %d>\tcolor_key_en: %d\nsrc_window:<%d,%d,%d,%d>\nscreen_window:<%d,%d,%d,%d>\npre_multiply=%d\n======= screen%d layer%d para ====\n",
        sel, HANDTOID(hid),layer_para.mode, layer_para.fb.addr[0], layer_para.fb.addr[1], layer_para.fb.addr[2],
        layer_para.fb.size.width, layer_para.fb.size.height, layer_para.fb.mode, layer_para.fb.format,
        layer_para.fb.seq, layer_para.fb.b_trd_src,  layer_para.fb.trd_mode,
        layer_para.b_trd_out, layer_para.out_trd_mode, layer_para.pipe,
        layer_para.prio, layer_para.alpha_en, layer_para.alpha_val,
        layer_para.ck_enable, layer_para.src_win.x, layer_para.src_win.y,
        layer_para.src_win.width, layer_para.src_win.height, layer_para.scn_win.x, layer_para.scn_win.y,
        layer_para.scn_win.width, layer_para.scn_win.height,layer_para.fb.pre_multiply, sel, HANDTOID(hid));
    }
}

static ssize_t disp_layer_para_store(struct device *dev,
        struct device_attribute *attr,
        const char *buf, size_t count)
{
    printk("there no room for anything\n");

  return count;
}

static DEVICE_ATTR(layer_para, S_IRUGO|S_IWUSR|S_IWGRP,
    disp_layer_para_show, disp_layer_para_store);


#define ____SEPARATOR_SCRIPT_DUMP____
static ssize_t disp_script_dump_show(struct device *dev,
    struct device_attribute *attr, char *buf)
{
  if(sel == 0)
    {
        script_dump_mainkey("disp_init");
        script_dump_mainkey("lcd0_para");
        script_dump_mainkey("hdmi_para");
        script_dump_mainkey("power_sply");
        script_dump_mainkey("clock");
        script_dump_mainkey("dram_para");
    }else if(sel == 1)
    {
        script_dump_mainkey("lcd1_para");
    }

    return sprintf(buf, "%s\n", "oh yeah!");
}

static ssize_t disp_script_dump_store(struct device *dev,
        struct device_attribute *attr,
        const char *buf, size_t count)
{
  char main_key[32];

    if(strlen(buf) == 0) {
    printk("Invalid para\n");
    return -1;
  }

    memcpy(main_key, buf, strlen(buf)+1);

    if(sel == 0)
    {
        script_dump_mainkey("disp_init");
        script_dump_mainkey("lcd0_para");
        script_dump_mainkey("hdmi_para");
        script_dump_mainkey("power_sply");
        script_dump_mainkey("clock");
        script_dump_mainkey("dram_para");
    }else if(sel == 1)
    {
        script_dump_mainkey("lcd1_para");
    }

  return count;
}

static DEVICE_ATTR(script_dump, S_IRUGO|S_IWUSR|S_IWGRP,
    disp_script_dump_show, disp_script_dump_store);


#define ____SEPARATOR_LCD____
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
        DRV_lcd_close(sel);
    }else
    {
        bsp_disp_hdmi_close(sel);
        DRV_lcd_open(sel);
  }

  return count;
}

static DEVICE_ATTR(lcd, S_IRUGO|S_IWUSR|S_IWGRP,
    disp_lcd_show, disp_lcd_store);

static ssize_t disp_lcd_bl_show(struct device *dev,
    struct device_attribute *attr, char *buf)
{
  return sprintf(buf, "%d\n", bsp_disp_lcd_get_bright(sel));
}

static ssize_t disp_lcd_bl_store(struct device *dev,
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

    if((val < 0) || (val > 255))
    {
        printk("Invalid value, 0~255 is expected!\n");
    }else
    {
        bsp_disp_lcd_set_bright(sel, val, 0);
  }

  return count;
}

static DEVICE_ATTR(lcd_bl, S_IRUGO|S_IWUSR|S_IWGRP,
    disp_lcd_bl_show, disp_lcd_bl_store);



static ssize_t disp_lcd_src_show(struct device *dev,
    struct device_attribute *attr, char *buf)
{
  return sprintf(buf, "screen%d lcd src=%d\n", sel, bsp_disp_lcd_get_src(sel));
}

static ssize_t disp_lcd_src_store(struct device *dev,
        struct device_attribute *attr,
        const char *buf, size_t count)
{
    unsigned long lcd_src;

    if (strnicmp(buf, "ch1", 3) == 0)
    {
        lcd_src = DISP_LCDC_SRC_DE_CH1;
    }
    else if (strnicmp(buf, "ch2", 3) == 0)
    {
        lcd_src = DISP_LCDC_SRC_DE_CH2;
    }
    else if (strnicmp(buf, "dma888", 6) == 0)
    {
        lcd_src = DISP_LCDC_SRC_DMA888;
    }
    else if (strnicmp(buf, "dma565", 6) == 0)
    {
        lcd_src = DISP_LCDC_SRC_DMA888;
    }
    else if (strnicmp(buf, "white", 5) == 0)
    {
        lcd_src = DISP_LCDC_SRC_WHITE;
    }
    else if (strnicmp(buf, "black", 5) == 0)
    {
        lcd_src = DISP_LCDC_SRC_BLACK;
    }
    else
    {
        pr_warn("not supported lcd source! \n");
        return -EINVAL;
    }

    bsp_disp_lcd_set_src(sel, lcd_src);

  return count;
}

static DEVICE_ATTR(lcd_src, S_IRUGO|S_IWUSR|S_IWGRP,
    disp_lcd_src_show, disp_lcd_src_store);


static ssize_t disp_fps_show(struct device *dev,
    struct device_attribute *attr, char *buf)
{
  __u32 screen_fps = bsp_disp_get_fps(sel);
    return sprintf(buf, "screen%d fps=%d.%d\n", sel, screen_fps/10,screen_fps%10);
}

static ssize_t disp_fps_store(struct device *dev,
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

    if(val > 75)
    {
        printk("Invalid value, <=75 is expected!\n");
    }else
    {
        bsp_disp_lcd_set_fps(sel, val);
  }

    return count;
}

static DEVICE_ATTR(fps, S_IRUGO|S_IWUSR|S_IWGRP,
    disp_fps_show, disp_fps_store);



#define ____SEPARATOR_HDMI____
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
    __disp_tv_mode_t tv_mode = 0xff;

    bsp_disp_hdmi_close(sel);

    if (strnicmp(buf, "720p50hz", 8) == 0)
    {
        tv_mode = DISP_TV_MOD_720P_50HZ;
    }
    else if (strnicmp(buf, "720p60hz", 8) == 0)
    {
        tv_mode = DISP_TV_MOD_720P_60HZ;
    }
    else if (strnicmp(buf, "1080p50hz", 9) == 0)
    {
        tv_mode = DISP_TV_MOD_1080P_50HZ;
    }
    else if (strnicmp(buf, "1080p60hz", 9) == 0)
    {
        tv_mode = DISP_TV_MOD_1080P_60HZ;
    }
    else if (strnicmp(buf, "1080p24hz", 9) == 0)
    {
        tv_mode = DISP_TV_MOD_1080P_24HZ;
    }
    else if (strnicmp(buf, "576p", 4) == 0)
    {
        tv_mode = DISP_TV_MOD_576P;
    }
    else if (strnicmp(buf, "480p", 4) == 0)
    {
        tv_mode = DISP_TV_MOD_480P;
    }
    else
    {
        pr_warn("not supported hdmi mode\n");
        return -EINVAL;
    }

    if(bsp_disp_hdmi_set_mode(sel, tv_mode) == 0)
    {
        bsp_disp_hdmi_open(sel);
    }

  return count;
}

static DEVICE_ATTR(hdmi, S_IRUGO|S_IWUSR|S_IWGRP,
    disp_hdmi_show, disp_hdmi_store);

static ssize_t disp_hdmi_hpd_show(struct device *dev,
    struct device_attribute *attr, char *buf)
{

    return sprintf(buf, "screen%d hdmi hpd=%d\n", sel, bsp_disp_hdmi_get_hpd_status(sel));
}

static ssize_t disp_hdmi_hpd_store(struct device *dev,
        struct device_attribute *attr,
        const char *buf, size_t count)
{
  return count;
}

static DEVICE_ATTR(hdmi_hpd, S_IRUGO|S_IWUSR|S_IWGRP,
    disp_hdmi_hpd_show, disp_hdmi_hpd_store);


static ssize_t disp_hdmi_cts_show(struct device *dev,
    struct device_attribute *attr, char *buf)
{
  return sprintf(buf, "cts %s\n", bsp_disp_hdmi_get_cts_enable()? "on":"off");
}

static ssize_t disp_hdmi_cts_store(struct device *dev,
        struct device_attribute *attr,
        const char *buf, size_t count)
{
  if (count < 1)
        return -EINVAL;

    if (strnicmp(buf, "on", 2) == 0 || strnicmp(buf, "1", 1) == 0)
    {
        bsp_disp_hdmi_cts_enable(1);
  }
    else if (strnicmp(buf, "off", 3) == 0 || strnicmp(buf, "0", 1) == 0)
  {
        bsp_disp_hdmi_cts_enable(0);
    }
    else
    {
        return -EINVAL;
    }

  return count;
}

static DEVICE_ATTR(hdmi_cts, S_IRUGO|S_IWUSR|S_IWGRP,
    disp_hdmi_cts_show, disp_hdmi_cts_store);


static ssize_t disp_hdmi_test_mode_show(struct device *dev,
    struct device_attribute *attr, char *buf)
{
  return sprintf(buf, "screen%d hdmi_test_mode=%d\n", sel, bsp_disp_hdmi_get_test_mode(sel));
}

static ssize_t disp_hdmi_test_mode_store(struct device *dev,
        struct device_attribute *attr,
        const char *buf, size_t count)
{
  if (count < 1)
        return -EINVAL;

    if (strnicmp(buf, "720p50hz", 8) == 0)
    {
        bsp_disp_hdmi_set_test_mode(sel, DISP_TV_MOD_720P_50HZ);
  }
    else if (strnicmp(buf, "720p60hz", 8) == 0)
    {
        bsp_disp_hdmi_set_test_mode(sel, DISP_TV_MOD_720P_60HZ);
  }
    else if (strnicmp(buf, "1080p50hz", 9) == 0)
    {
        bsp_disp_hdmi_set_test_mode(sel, DISP_TV_MOD_1080P_50HZ);
  }
    else if (strnicmp(buf, "1080p60hz", 9) == 0)
    {
        bsp_disp_hdmi_set_test_mode(sel, DISP_TV_MOD_1080P_60HZ);
  }
    else if (strnicmp(buf, "1080p24hz", 9) == 0)
    {
        bsp_disp_hdmi_set_test_mode(sel, DISP_TV_MOD_1080P_24HZ);
  }
    else if (strnicmp(buf, "576p", 4) == 0)
    {
        bsp_disp_hdmi_set_test_mode(sel, DISP_TV_MOD_576P);
  }
    else if (strnicmp(buf, "480p", 4) == 0)
    {
        bsp_disp_hdmi_set_test_mode(sel, DISP_TV_MOD_480P);
  }
    else
    {
        return -EINVAL;
    }

  return count;
}

static DEVICE_ATTR(hdmi_test_mode, S_IRUGO|S_IWUSR|S_IWGRP,
    disp_hdmi_test_mode_show, disp_hdmi_test_mode_store);

static ssize_t disp_hdmi_src_show(struct device *dev,
    struct device_attribute *attr, char *buf)
{
  return sprintf(buf, "screen%d lcd src=%d\n", sel, bsp_disp_hdmi_get_src(sel));
}

static ssize_t disp_hdmi_src_store(struct device *dev,
        struct device_attribute *attr,
        const char *buf, size_t count)
{
    unsigned long lcd_src;

    if (strnicmp(buf, "ch1", 3) == 0)
    {
        lcd_src = DISP_LCDC_SRC_DE_CH1;
    }
    else if (strnicmp(buf, "ch2", 3) == 0)
    {
        lcd_src = DISP_LCDC_SRC_DE_CH2;
    }
    else if (strnicmp(buf, "blue", 4) == 0)
    {
        lcd_src = DISP_LCDC_SRC_BLUE;
    }
    else
    {
        pr_warn("not supported hdmi source! \n");
        return -EINVAL;
    }


    bsp_disp_hdmi_set_src(sel, lcd_src);

  return count;
}

static DEVICE_ATTR(hdmi_src, S_IRUGO|S_IWUSR|S_IWGRP,
    disp_hdmi_src_show, disp_hdmi_src_store);


#define ____SEPARATOR_VSYNC_EVENT____
static ssize_t disp_vsync_event_enable_show(struct device *dev,
    struct device_attribute *attr, char *buf)
{
  return 0xff;
}

static ssize_t disp_vsync_event_enable_store(struct device *dev,
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
        bsp_disp_vsync_event_enable(sel, val);
  }

  return count;
}

static DEVICE_ATTR(vsync_event_enable, S_IRUGO|S_IWUSR|S_IWGRP,
    disp_vsync_event_enable_show, disp_vsync_event_enable_store);


#define ____SEPARATOR_LAYER____
static ssize_t disp_layer_mode_show(struct device *dev,
    struct device_attribute *attr, char *buf)
{
    __disp_layer_info_t para;
    int ret;

    ret = bsp_disp_layer_get_para(sel, hid, &para);
    if(0 == ret)
  {
      return sprintf(buf, "%d\n", para.mode);
    }else
    {
        return sprintf(buf, "%s\n", "not used!");
    }
}

static ssize_t disp_layer_mode_store(struct device *dev,
        struct device_attribute *attr,
        const char *buf, size_t count)
{
  int err;
    unsigned long val;
    int ret;
    __disp_layer_info_t para;

  err = strict_strtoul(buf, 10, &val);
  if (err) {
    printk("Invalid size\n");
    return err;
  }

    if((val>4))
    {
        printk("Invalid value, <5 is expected!\n");

    }else
    {
        ret = bsp_disp_layer_get_para(sel, hid, &para);
        if(0 == ret)
        {
            para.mode = (__disp_layer_work_mode_t)val;
            if(para.mode == DISP_LAYER_WORK_MODE_SCALER)
            {
                para.scn_win.width = bsp_disp_get_screen_width(sel);
                para.scn_win.height = bsp_disp_get_screen_height(sel);
            }
            bsp_disp_layer_set_para(sel, hid, &para);
        }else
        {
            printk("not used!\n");
        }
  }

  return count;
}

static DEVICE_ATTR(layer_mode, S_IRUGO|S_IWUSR|S_IWGRP,
    disp_layer_mode_show, disp_layer_mode_store);



#define ____SEPARATOR_VIDEO_NODE____
static ssize_t disp_video_dit_mode_show(struct device *dev,
    struct device_attribute *attr, char *buf)
{
  return sprintf(buf, "%d\n", (unsigned int)disp_video_get_dit_mode(sel));
}

static ssize_t disp_video_dit_mode_store(struct device *dev,
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

    if((val>3))
    {
        printk("Invalid value, 0~3 is expected!\n");
    }else
    {
        printk("%ld\n", val);
        disp_video_set_dit_mode(sel, (unsigned int)val);
  }

  return count;
}
static DEVICE_ATTR(video_dit_mode, S_IRUGO|S_IWUSR|S_IWGRP,
    disp_video_dit_mode_show, disp_video_dit_mode_store);

static ssize_t disp_video_info_show(struct device *dev,
    struct device_attribute *attr, char *buf)
{
  __disp_layer_info_t layer_para;
    __disp_video_fb_t video_fb;
    __s32 ret = 0;
    __u32 i;
    __u32 cnt = 0;

    for(i=100; i<104; i++)
    {
        ret = bsp_disp_layer_get_para(sel, i, &layer_para);
        if(ret == 0)
        {
            ret = bsp_disp_video_get_fb(sel, i, &video_fb);
            if(ret == 0)
            {
                cnt += sprintf(buf+cnt, "=== screen%d layer%d para ====\nmode: %d\nfb.size=<%dx%d>\nfb.fmt=<%d, %d, %d>\ntrd_src=<%d, %d> trd_out=<%d, %d>\npipe:%d\tprio: %d\nalpha: <%d, %d>\tcolor_key_en: %d\nsrc_window:<%d,%d,%d,%d>\nscreen_window:<%d,%d,%d,%d>\npre_multiply=%d\n======= screen%d layer%d para ====\n",
                        sel, HANDTOID(i),layer_para.mode, layer_para.fb.size.width,
                        layer_para.fb.size.height, layer_para.fb.mode, layer_para.fb.format,
                        layer_para.fb.seq, layer_para.fb.b_trd_src,  layer_para.fb.trd_mode,
                        layer_para.b_trd_out, layer_para.out_trd_mode, layer_para.pipe,
                        layer_para.prio, layer_para.alpha_en, layer_para.alpha_val,
                        layer_para.ck_enable, layer_para.src_win.x, layer_para.src_win.y,
                        layer_para.src_win.width, layer_para.src_win.height, layer_para.scn_win.x, layer_para.scn_win.y,
                        layer_para.scn_win.width, layer_para.scn_win.height,layer_para.fb.pre_multiply, sel, HANDTOID(i));

                cnt += sprintf(buf+cnt, "=== video info ==\nid=%d\n%s\nmaf_valid=%d\tfre_frame_valid=%d\ttop_field_first=%d\n=== video info ===\n",
                    video_fb.id, (video_fb.interlace? "Interlace":"Progressive"), video_fb.maf_valid, video_fb.pre_frame_valid, video_fb.top_field_first);
            }
        }
    }

    return cnt;

}

static ssize_t disp_video_info_store(struct device *dev,
        struct device_attribute *attr,
        const char *buf, size_t count)
{
  return count;
}
static DEVICE_ATTR(video_info, S_IRUGO|S_IWUSR|S_IWGRP,
    disp_video_info_show, disp_video_info_store);

static ssize_t disp_video_fps_show(struct device *dev,
    struct device_attribute *attr, char *buf)
{
  __u32 i;
    __u32 fps;
    __u32 cnt = 0;

    for(i=100; i<104; i++)
    {
        if(bsp_disp_video_get_start(sel, i) == 1)
        {
            fps = bsp_disp_video_get_fps(sel, i);
            cnt += sprintf(buf+cnt, "screen%d layer%d fps=%d.%d\n", sel, HANDTOID(i), fps/10,fps%10);
        }
    }
    return cnt;
}

static ssize_t disp_video_fps_store(struct device *dev,
        struct device_attribute *attr,
        const char *buf, size_t count)
{
  return count;
}

static DEVICE_ATTR(video_fps, S_IRUGO|S_IWUSR|S_IWGRP,
    disp_video_fps_show, disp_video_fps_store);



#define ____SEPARATOR_DEU_NODE____
static ssize_t disp_deu_enable_show(struct device *dev,
    struct device_attribute *attr, char *buf)
{
  return sprintf(buf, "%d\n", bsp_disp_deu_get_enable(sel, hid));
}

static ssize_t disp_deu_enable_store(struct device *dev,
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
        bsp_disp_deu_enable(sel, hid, (unsigned int)val);
  }

  return count;
}


static ssize_t disp_deu_luma_sharp_level_show(struct device *dev,
    struct device_attribute *attr, char *buf)
{
  return sprintf(buf, "%d\n", bsp_disp_deu_get_luma_sharp_level(sel, hid));
}

static ssize_t disp_deu_luma_sharp_level_store(struct device *dev,
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

    if(val > 4)
    {
        printk("Invalid value, 0~4 is expected!\n");
    }else
    {
        printk("%ld\n", val);
        bsp_disp_deu_set_luma_sharp_level(sel, hid, (unsigned int)val);
  }

  return count;
}

static ssize_t disp_deu_chroma_sharp_level_show(struct device *dev,
    struct device_attribute *attr, char *buf)
{
  return sprintf(buf, "%d\n", bsp_disp_deu_get_chroma_sharp_level(sel, hid));
}

static ssize_t disp_deu_chroma_sharp_level_store(struct device *dev,
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

    if(val > 4)
    {
        printk("Invalid value, 0~4 is expected!\n");
    }else
    {
        printk("%ld\n", val);
        bsp_disp_deu_set_chroma_sharp_level(sel, hid, (unsigned int)val);
  }

  return count;
}

static ssize_t disp_deu_black_exten_level_show(struct device *dev,
    struct device_attribute *attr, char *buf)
{
  return sprintf(buf, "%d\n", bsp_disp_deu_get_black_exten_level(sel, hid));
}

static ssize_t disp_deu_black_exten_level_store(struct device *dev,
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

    if(val > 4)
    {
        printk("Invalid value, 0~4 is expected!\n");
    }else
    {
        printk("%ld\n", val);
        bsp_disp_deu_set_black_exten_level(sel, hid, (unsigned int)val);
  }

  return count;
}

static ssize_t disp_deu_white_exten_level_show(struct device *dev,
    struct device_attribute *attr, char *buf)
{
  return sprintf(buf, "%d\n", bsp_disp_deu_get_white_exten_level(sel, hid));
}

static ssize_t disp_deu_white_exten_level_store(struct device *dev,
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

    if(val > 4)
    {
        printk("Invalid value, 0~4 is expected!\n");
    }else
    {
        printk("%ld\n", val);
        bsp_disp_deu_set_white_exten_level(sel, hid, (unsigned int)val);
  }

  return count;
}

static DEVICE_ATTR(deu_en, S_IRUGO|S_IWUSR|S_IWGRP,
    disp_deu_enable_show, disp_deu_enable_store);

static DEVICE_ATTR(deu_luma_level, S_IRUGO|S_IWUSR|S_IWGRP,
    disp_deu_luma_sharp_level_show, disp_deu_luma_sharp_level_store);

static DEVICE_ATTR(deu_chroma_level, S_IRUGO|S_IWUSR|S_IWGRP,
    disp_deu_chroma_sharp_level_show, disp_deu_chroma_sharp_level_store);

static DEVICE_ATTR(deu_black_level, S_IRUGO|S_IWUSR|S_IWGRP,
    disp_deu_black_exten_level_show, disp_deu_black_exten_level_store);

static DEVICE_ATTR(deu_white_level, S_IRUGO|S_IWUSR|S_IWGRP,
    disp_deu_white_exten_level_show, disp_deu_white_exten_level_store);



#define ____SEPARATOR_LAYER_ENHANCE_NODE____
static ssize_t disp_layer_enhance_enable_show(struct device *dev,
    struct device_attribute *attr, char *buf)
{
  return sprintf(buf, "%d\n", bsp_disp_cmu_layer_get_enable(sel, hid));
}

static ssize_t disp_layer_enhance_enable_store(struct device *dev,
        struct device_attribute *attr,
        const char *buf, size_t count)
{
  int err;
    unsigned long bright_val;

  err = strict_strtoul(buf, 10, &bright_val);
    printk("string=%s, int=%ld\n", buf, bright_val);
  if (err) {
    printk("Invalid size\n");
    return err;
  }

    if(bright_val > 100)
    {
        printk("Invalid value, 0~100 is expected!\n");
    }else
    {
        printk("%ld\n", bright_val);
        bsp_disp_cmu_layer_enable(sel, hid, (unsigned int)bright_val);
  }

  return count;
}


static ssize_t disp_layer_bright_show(struct device *dev,
    struct device_attribute *attr, char *buf)
{
  return sprintf(buf, "%d\n", bsp_disp_cmu_layer_get_bright(sel, hid));
}

static ssize_t disp_layer_bright_store(struct device *dev,
        struct device_attribute *attr,
        const char *buf, size_t count)
{
  int err;
    unsigned long bright_val;

  err = strict_strtoul(buf, 10, &bright_val);
    printk("string=%s, int=%ld\n", buf, bright_val);
  if (err) {
    printk("Invalid size\n");
    return err;
  }

    if(bright_val > 100)
    {
        printk("Invalid value, 0~100 is expected!\n");
    }else
    {
        printk("%ld\n", bright_val);
        bsp_disp_cmu_layer_set_bright(sel, hid, (unsigned int)bright_val);
  }

  return count;
}

static ssize_t disp_layer_contrast_show(struct device *dev,
    struct device_attribute *attr, char *buf)
{
  return sprintf(buf, "%d\n", bsp_disp_cmu_layer_get_contrast(sel, hid));
}

static ssize_t disp_layer_contrast_store(struct device *dev,
        struct device_attribute *attr,
        const char *buf, size_t count)
{
  int err;
    unsigned long contrast_val;

  err = strict_strtoul(buf, 10, &contrast_val);

  if (err) {
    printk("Invalid size\n");
    return err;
  }

    if(contrast_val > 100)
    {
        printk("Invalid value, 0~100 is expected!\n");
    }else
    {
        printk("%ld\n", contrast_val);
        bsp_disp_cmu_layer_set_contrast(sel, hid, (unsigned int)contrast_val);
  }

  return count;
}

static ssize_t disp_layer_saturation_show(struct device *dev,
    struct device_attribute *attr, char *buf)
{
  return sprintf(buf, "%d\n", bsp_disp_cmu_layer_get_saturation(sel, hid));
}

static ssize_t disp_layer_saturation_store(struct device *dev,
        struct device_attribute *attr,
        const char *buf, size_t count)
{
  int err;
    unsigned long saturation_val;

  err = strict_strtoul(buf, 10, &saturation_val);

  if (err) {
    printk("Invalid size\n");
    return err;
  }

    if(saturation_val > 100)
    {
        printk("Invalid value, 0~100 is expected!\n");
    }else
    {
        printk("%ld\n", saturation_val);
        bsp_disp_cmu_layer_set_saturation(sel, hid,(unsigned int)saturation_val);
  }

  return count;
}

static ssize_t disp_layer_hue_show(struct device *dev,
    struct device_attribute *attr, char *buf)
{
  return sprintf(buf, "%d\n", bsp_disp_cmu_layer_get_hue(sel,hid));
}

static ssize_t disp_layer_hue_store(struct device *dev,
        struct device_attribute *attr,
        const char *buf, size_t count)
{
  int err;
    unsigned long hue_val;

  err = strict_strtoul(buf, 10, &hue_val);

  if (err) {
    printk("Invalid size\n");
    return err;
  }

    if(hue_val > 100)
    {
        printk("Invalid value, 0~100 is expected!\n");
    }else
    {
        printk("%ld\n", hue_val);
        bsp_disp_cmu_layer_set_hue(sel, hid,(unsigned int)hue_val);
  }

  return count;
}

static ssize_t disp_layer_enhance_mode_show(struct device *dev,
    struct device_attribute *attr, char *buf)
{
  return sprintf(buf, "%d\n", bsp_disp_cmu_layer_get_mode(sel,hid));
}

static ssize_t disp_layer_enhance_mode_store(struct device *dev,
        struct device_attribute *attr,
        const char *buf, size_t count)
{
  int err;
    unsigned long mode_val;

  err = strict_strtoul(buf, 10, &mode_val);

  if (err) {
    printk("Invalid size\n");
    return err;
  }

    if(mode_val > 100)
    {
        printk("Invalid value, 0~100 is expected!\n");
    }else
    {
        printk("%ld\n", mode_val);
        bsp_disp_cmu_layer_set_mode(sel, hid,(unsigned int)mode_val);
  }

  return count;
}

#define ____SEPARATOR_SCREEN_ENHANCE_NODE____
static ssize_t disp_enhance_enable_show(struct device *dev,
    struct device_attribute *attr, char *buf)
{
  return sprintf(buf, "%d\n", bsp_disp_cmu_get_enable(sel));
}

static ssize_t disp_enhance_enable_store(struct device *dev,
        struct device_attribute *attr,
        const char *buf, size_t count)
{
  int err;
    unsigned long bright_val;

  err = strict_strtoul(buf, 10, &bright_val);
    printk("string=%s, int=%ld\n", buf, bright_val);
  if (err) {
    printk("Invalid size\n");
    return err;
  }

    if(bright_val > 100)
    {
        printk("Invalid value, 0~100 is expected!\n");
    }else
    {
        printk("%ld\n", bright_val);
        bsp_disp_cmu_enable(sel,(unsigned int)bright_val);
  }

  return count;
}



static ssize_t disp_bright_show(struct device *dev,
    struct device_attribute *attr, char *buf)
{
  return sprintf(buf, "%d\n", bsp_disp_cmu_get_bright(sel));
}

static ssize_t disp_bright_store(struct device *dev,
        struct device_attribute *attr,
        const char *buf, size_t count)
{
  int err;
    unsigned long bright_val;

  err = strict_strtoul(buf, 10, &bright_val);
    printk("string=%s, int=%ld\n", buf, bright_val);
  if (err) {
    printk("Invalid size\n");
    return err;
  }

    if(bright_val > 100)
    {
        printk("Invalid value, 0~100 is expected!\n");
    }else
    {
        printk("%ld\n", bright_val);
        bsp_disp_cmu_set_bright(sel, (unsigned int)bright_val);
  }

  return count;
}

static ssize_t disp_contrast_show(struct device *dev,
    struct device_attribute *attr, char *buf)
{
  return sprintf(buf, "%d\n", bsp_disp_cmu_get_contrast(sel));
}

static ssize_t disp_contrast_store(struct device *dev,
        struct device_attribute *attr,
        const char *buf, size_t count)
{
  int err;
    unsigned long contrast_val;

  err = strict_strtoul(buf, 10, &contrast_val);

  if (err) {
    printk("Invalid size\n");
    return err;
  }

    if(contrast_val > 100)
    {
        printk("Invalid value, 0~100 is expected!\n");
    }else
    {
        printk("%ld\n", contrast_val);
        bsp_disp_cmu_set_contrast(sel, (unsigned int)contrast_val);
  }

  return count;
}

static ssize_t disp_saturation_show(struct device *dev,
    struct device_attribute *attr, char *buf)
{
  return sprintf(buf, "%d\n", bsp_disp_cmu_get_saturation(sel));
}

static ssize_t disp_saturation_store(struct device *dev,
        struct device_attribute *attr,
        const char *buf, size_t count)
{
  int err;
    unsigned long saturation_val;

  err = strict_strtoul(buf, 10, &saturation_val);

  if (err) {
    printk("Invalid size\n");
    return err;
  }

    if(saturation_val > 100)
    {
        printk("Invalid value, 0~100 is expected!\n");
    }else
    {
        printk("%ld\n", saturation_val);
        bsp_disp_cmu_set_saturation(sel, (unsigned int)saturation_val);
  }

  return count;
}

static ssize_t disp_hue_show(struct device *dev,
    struct device_attribute *attr, char *buf)
{
  return sprintf(buf, "%d\n", bsp_disp_cmu_get_hue(sel));
}

static ssize_t disp_hue_store(struct device *dev,
        struct device_attribute *attr,
        const char *buf, size_t count)
{
  int err;
    unsigned long hue_val;

  err = strict_strtoul(buf, 10, &hue_val);

  if (err) {
    printk("Invalid size\n");
    return err;
  }

    if(hue_val > 100)
    {
        printk("Invalid value, 0~100 is expected!\n");
    }else
    {
        printk("%ld\n", hue_val);
        bsp_disp_cmu_set_hue(sel, (unsigned int)hue_val);
  }

  return count;
}

static ssize_t disp_enhance_mode_show(struct device *dev,
    struct device_attribute *attr, char *buf)
{
  return sprintf(buf, "%d\n", bsp_disp_cmu_get_mode(sel));
}

static ssize_t disp_enhance_mode_store(struct device *dev,
        struct device_attribute *attr,
        const char *buf, size_t count)
{
  int err;
    unsigned long mode_val;

  err = strict_strtoul(buf, 10, &mode_val);

  if (err) {
    printk("Invalid size\n");
    return err;
  }

    if(mode_val > 100)
    {
        printk("Invalid value, 0~100 is expected!\n");
    }else
    {
        printk("%ld\n", mode_val);
        bsp_disp_cmu_set_mode(sel, (unsigned int)mode_val);
  }

  return count;
}

static DEVICE_ATTR(layer_enhance_en, S_IRUGO|S_IWUSR|S_IWGRP,
    disp_layer_enhance_enable_show, disp_layer_enhance_enable_store);

static DEVICE_ATTR(layer_bright, S_IRUGO|S_IWUSR|S_IWGRP,
    disp_layer_bright_show, disp_layer_bright_store);

static DEVICE_ATTR(layer_contrast, S_IRUGO|S_IWUSR|S_IWGRP,
    disp_layer_contrast_show, disp_layer_contrast_store);

static DEVICE_ATTR(layer_saturation, S_IRUGO|S_IWUSR|S_IWGRP,
    disp_layer_saturation_show, disp_layer_saturation_store);

static DEVICE_ATTR(layer_hue, S_IRUGO|S_IWUSR|S_IWGRP,
    disp_layer_hue_show, disp_layer_hue_store);

static DEVICE_ATTR(layer_enhance_mode, S_IRUGO|S_IWUSR|S_IWGRP,
    disp_layer_enhance_mode_show, disp_layer_enhance_mode_store);


static DEVICE_ATTR(screen_enhance_en, S_IRUGO|S_IWUSR|S_IWGRP,
    disp_enhance_enable_show, disp_enhance_enable_store);

static DEVICE_ATTR(screen_bright, S_IRUGO|S_IWUSR|S_IWGRP,
    disp_bright_show, disp_bright_store);

static DEVICE_ATTR(screen_contrast, S_IRUGO|S_IWUSR|S_IWGRP,
    disp_contrast_show, disp_contrast_store);

static DEVICE_ATTR(screen_saturation, S_IRUGO|S_IWUSR|S_IWGRP,
    disp_saturation_show, disp_saturation_store);

static DEVICE_ATTR(screen_hue, S_IRUGO|S_IWUSR|S_IWGRP,
    disp_hue_show, disp_hue_store);

static DEVICE_ATTR(screen_enhance_mode, S_IRUGO|S_IWUSR|S_IWGRP,
    disp_enhance_mode_show, disp_enhance_mode_store);


#define ____SEPARATOR_DRC_NODE____
static ssize_t disp_drc_enable_show(struct device *dev,
    struct device_attribute *attr, char *buf)
{
  return sprintf(buf, "%d\n", bsp_disp_drc_get_enable(sel));
}

static ssize_t disp_drc_enable_store(struct device *dev,
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

    if((val != 0) && (val != 1))
    {
        printk("Invalid value, 0/1 is expected!\n");
    }else
    {
        printk("%ld\n", val);
        bsp_disp_drc_enable(sel, (unsigned int)val);
  }

  return count;
}


static DEVICE_ATTR(drc_en, S_IRUGO|S_IWUSR|S_IWGRP,
    disp_drc_enable_show, disp_drc_enable_store);

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

static struct attribute *disp_attributes[] = {
    &dev_attr_screen_enhance_en.attr,
    &dev_attr_screen_bright.attr,
    &dev_attr_screen_contrast.attr,
    &dev_attr_screen_saturation.attr,
    &dev_attr_screen_hue.attr,
    &dev_attr_screen_enhance_mode.attr,
    &dev_attr_layer_enhance_en.attr,
    &dev_attr_layer_bright.attr,
    &dev_attr_layer_contrast.attr,
    &dev_attr_layer_saturation.attr,
    &dev_attr_layer_hue.attr,
    &dev_attr_layer_enhance_mode.attr,
    &dev_attr_drc_en.attr,
    &dev_attr_deu_en.attr,
    &dev_attr_deu_luma_level.attr,
    &dev_attr_deu_chroma_level.attr,
    &dev_attr_deu_black_level.attr,
    &dev_attr_deu_white_level.attr,
    &dev_attr_video_dit_mode.attr,
    &dev_attr_sel.attr,
    &dev_attr_hid.attr,
    &dev_attr_reg_dump.attr,
    &dev_attr_layer_mode.attr,
    &dev_attr_vsync_event_enable.attr,
    &dev_attr_lcd.attr,
    &dev_attr_lcd_bl.attr,
    &dev_attr_hdmi.attr,
    &dev_attr_script_dump.attr,
    &dev_attr_colorbar.attr,
    &dev_attr_layer_para.attr,
    &dev_attr_hdmi_hpd.attr,
    &dev_attr_print_cmd_level.attr,
    &dev_attr_cmd_print.attr,
    &dev_attr_debug.attr,
    &dev_attr_fps.attr,
    &dev_attr_video_info.attr,
    &dev_attr_video_fps.attr,
    &dev_attr_lcd_src.attr,
    &dev_attr_hdmi_cts.attr,
    &dev_attr_hdmi_test_mode.attr,
    &dev_attr_int_count.attr,
    &dev_attr_hdmi_src.attr,
    &dev_attr_cfg_cnt.attr,
    &dev_attr_cache.attr,
    &dev_attr_sys_status.attr,
    &dev_attr_standby.attr,
    &dev_attr_conditon.attr,
    NULL
};

static struct attribute_group disp_attribute_group = {
  .name = "attr",
  .attrs = disp_attributes
};

int disp_attr_node_init(struct device  *display_dev)
{
    unsigned int ret;

    ret = sysfs_create_group(&display_dev->kobj,
                             &disp_attribute_group);
    sel = 0;
    hid = 100;
    return 0;
}

int disp_attr_node_exit(void)
{
    return 0;
}
