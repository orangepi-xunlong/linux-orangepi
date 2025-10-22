/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner SoCs eink sys driver.
 *
 * Copyright (C) 2019 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#ifndef _EINK_SYS_SOURCE_H_
#define _EINK_SYS_SOURCE_H_

#include "eink_driver.h"

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/uaccess.h>
#include <asm/memory.h>
#include <asm/unistd.h>
#include <linux/vmalloc.h>
#include <linux/pinctrl/consumer.h>
#include <linux/regulator/consumer.h>
#include <linux/fs.h>
#include <linux/dma-buf.h>
#include <linux/dma-mapping.h>
#include <linux/err.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/debugfs.h>
#include <linux/timer.h>
#include <linux/gpio.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include <linux/of_iommu.h>
#include <linux/of_device.h>
#include <linux/of_platform.h>
#include <linux/of_gpio.h>
#include <linux/compat.h>
#include <asm/barrier.h>
#include <linux/clk-provider.h>
#include <asm/cacheflush.h>
#include <linux/interrupt.h>

#include <video/sunxi_display2.h>

#define EINK_PRINT_LEVEL 0x0

/* ********************* FOR DEBUG ************************************ */
/**/
/* #define DE_WB_DEBUG */	/* use a default pic to debug DE WB */
/* #define WAVEDATA_DEBUG */    /* use a default wavfile to debug */
/* #define INDEX_DEBUG */
/* #define ION_DEBUG_DUMP */
/* #define PIPELINE_DEBUG */
/* #define VIRTUAL_REGISTER */   /* for test on local */
/* ******************************************************************* */

#ifdef DE_WB_DEBUG
#define DEFAULT_GRAY_PIC_PATH "/system/eink_image.bin"
#endif

#ifdef WAVEDATA_DEBUG
#define DEFAULT_INIT_WAV_PATH "/system/init_wf.bin"
#define DEFAULT_GC16_WAV_PATH "/system/gc16_wf.bin"
#endif

typedef struct file ES_FILE;

#define EINK_INFO_MSG(fmt, args...) \
	do {\
		if (eink_get_print_level() >= 1)\
		sunxi_info(NULL, "[EINK-%-24s] line:%04d: " fmt, __func__, __LINE__, ##args);\
	} while (0)

#define EINK_DEBUG_MSG(fmt, args...) \
	do {\
		if (eink_get_print_level() == 2)\
		sunxi_info(NULL, "[EINK-%-24s] line:%04d: " fmt, __func__, __LINE__, ##args);\
	} while (0)

#define EINKALIGN(value, align) ((align == 0) ? \
				value : \
				(((value) + ((align) - 1)) & ~((align) - 1)))

struct fb_address_transfer {
	enum disp_pixel_format format;
	struct disp_rectsz size[3];
	unsigned int align[3];
	int depth;
	dma_addr_t dma_addr;
	unsigned long long addr[3];
	unsigned long long trd_right_addr[3];
};

struct eink_format_attr {
	enum upd_pixel_fmt format;
	unsigned int bits;
	unsigned int hor_rsample_u;
	unsigned int hor_rsample_v;
	unsigned int ver_rsample_u;
	unsigned int ver_rsample_v;
	unsigned int uvc;
	unsigned int interleave;
	unsigned int factor;
	unsigned int div;
};

struct dmabuf_item {
	struct list_head list;
	struct dma_buf *dmabuf;
	struct dma_buf_attachment *attachment;
	struct sg_table *sgt;
	dma_addr_t dma_addr;
	unsigned long long id;
};

#define EINK_IRQ_RETURN IRQ_HANDLED
#define EINK_PIN_STATE_ACTIVE "active"
#define EINK_PIN_STATE_SLEEP "sleep"

/* BMP(0x0000~0x000D14) */
#pragma pack(1)
typedef struct {
	u8	bfType[2];         /* BM */
	u32	bfSize;		   /* bmp  */
	u16	bfReserved1;       /* 0 */
	u16	bfReserved2;	   /* 0 */
	u32	bfOffBits;	   /*  */
} BMP_FILE_HEADER;

#define BMP_FILE_HEADER_SIZE	(sizeof(BMP_FILE_HEADER))

/* BMP(0x000E~0x003540) */
typedef struct {
	u32	biSize;               /*  */
	u32	biWidth;              /* BMP */
	u32	biHeight;             /* BMP */
	u16	biPlanes;	      /* 1 */
	u16	biBitCount;	      /*  */
	u32	biCompress;	      /*  */
	u32	biSizeImage;	      /*  */
	u32	biXPelsPerMeter;      /*  */
	u32	biYPelsPerMeter;      /*  */
	u32	biClrUsed;	      /* , 0 (2 ^ biBitCount)  Bytes */
	u32	biClrImportant;	      /*  */
} BMP_INFO_HEADER;
#pragma pack()

#define BMP_INFO_HEADER_SIZE	(sizeof(BMP_INFO_HEADER))
#define BMP_MIN_SIZE		(BMP_FILE_HEADER_SIZE + BMP_INFO_HEADER_SIZE)

/* BMP */
typedef struct {
	u8 r;				/* red */
	u8 g;				/* green */
	u8 b;				/* blue */
	u8 reserved;			/* alpha */
} ST_ARGB;

typedef struct {
	u32  width;	                /* BMP */
	u32  height;	                /* BMP */
	u16  bit_count;			/*  */
	u32  image_size;	        /*  */
	u32  color_tbl_size;		/* , 0 (2 ^ biBitCount)  Bytes */
} BMP_INFO;

#pragma pack(1)
typedef struct {
	u8 blue;
	u8 green;
	u8 red;
} ST_RGB;
#pragma pack()

#define BMP_COLOR_SIZE              (sizeof(ST_ARGB) * 256)
#define BMP_IMAGE_DATA_OFFSET       (BMP_FILE_HEADER_SIZE + BMP_INFO_HEADER_SIZE + BMP_COLOR_SIZE)

extern bool is_upd_win_zero(struct upd_win update_area);
extern int eink_sys_script_get_item(char *main_name, char *sub_name, int value[], int type);
extern s32 get_delt_ms_timer(struct timespec64 start_timer, struct timespec64 end_timer);
extern void save_upd_rmi_buffer(u32 order, u8 *buf, u32 len);
extern void save_one_wavedata_buffer(u8 *buf, bool is_edma);
extern void save_rearray_waveform_to_mem(u8 *buf, u32 len);
extern void *eink_malloc(u32 num_bytes, void *phys_addr);
extern void eink_free(void *virt_addr, void *phys_addr, u32 num_bytes);
extern void eink_cache_sync(void *start_addr, int size);
extern int init_eink_ion_mgr(struct eink_ion_mgr *ion_mgr);
extern void deinit_eink_ion_mgr(struct eink_ion_mgr *ion_mgr);
extern s32 eink_panel_pin_cfg(u32 en);
extern int eink_sys_gpio_request(struct eink_gpio_cfg *gpio_info);
extern int eink_sys_gpio_release(struct eink_gpio_cfg *gpio_info);
extern int eink_sys_gpio_set_value(u32 p_handler, u32 value_to_gpio, const char *gpio_name);


extern struct dmabuf_item *eink_dma_map(int fd);
extern void eink_dma_unmap(struct dmabuf_item *item);

extern void eink_put_gray_to_mem(u32 order, char *buf, u32 width, u32 height);
extern void eink_save_last_img(char *buf, u32 width, u32 height);
extern void eink_save_current_img(char *buf, u32 width, u32 height);
extern void save_waveform_to_mem(u32 order, u8 *buf, u32 frames, u32 bit_num);
extern int eink_get_default_file_from_mem(__u8 *buf, char *file_name, __u32 length, loff_t pos);
extern int save_as_bin_file(__u8 *buf, char *file_name, __u32 length, loff_t pos);
extern void print_free_pipe_list(struct pipe_manager *mgr);
extern void print_used_pipe_list(struct pipe_manager *mgr);
extern void print_used_img_list(struct buf_manager *mgr);
extern void print_coll_img_list(struct buf_manager *mgr);
extern void print_free_img_list(struct buf_manager *mgr);
extern void eink_print_register(unsigned long start_addr, unsigned long end_addr);
extern int atoi_float(char *buf);
extern s32 eink_set_print_level(u32 level);
extern s32 eink_get_print_level(void);

#endif
