// SPDX-License-Identifier: GPL-2.0
/*
 * sunxi_ksc.h
 *
 * Copyright (c) 2007-2024 Allwinnertech Co., Ltd.
 * Author: zhengxiaobin <zhengxiaobin@allwinnertech.com>
 *
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#ifndef _SUNXI_KSC_H
#define _SUNXI_KSC_H


enum csc_fmt {
	Rec601_Full = 0x0,
	Rec601_Tv,
	Rec709_Full,
	Rec709_Tv,
	Rec2020_Full,
	Rec2020_Tv,
};

enum ksc_pix_fmt {
	////YUV420:
	YUV420P = 0x0,//=YU12=I420, y...u...v...
	YVU420P,      //=YV12, y...v...u...
	YUV420SP,     //=NV12, y...uv...
	YVU420SP,     //=NV21, y...vu...

	////YUV422:
	YUV422P,   //=YU16=I422, y...u...v...
	YVU422P,   //=YV16, y...v...u...
	YUV422SP,  //=NV16, y...uv...
	YVU422SP,  //=NV61, y...vu...

	////YUV444:
	YUV444P,   //=YU24=I444, y...u...v...
	YVU444P,   //=YV24, y...v...u...
	YUV444SP,  //=NV24, y...uv...
	YVU444SP,  //=NV42, y...vu...

	////RGB24:
	RGB888,    //rgb24, rgb...rgb
	BGR888,    //bgr24, bgr...bgr

	////ARGB8888:
	ARGB8888, //argb32, argb...argb
	ABGR8888, //abgr32, abgr...abgr
	RGBA8888, //rgba32, rgba...rgba
	BGRA8888, //bgra32, bgra...bgra

	////ARGB4444:
	ARGB4444, //argb16, argb...argb
	ABGR4444, //abgr16, abgr...abgr
	RGBA4444, //rgba16, rgba...rgba
	BGRA4444, //bgra16, bgra...bgra

	////ARGB1555:
	ARGB1555, //argb16, argb...argb
	ABGR1555, //abgr16, abgr...abgr
	RGBA1555, //rgba16, rgba...rgba
	BGRA1555, //bgra16, bgra...bgra

	/////AYUV:
	AYUV
};

struct ksc_image_t {
	int w;
	int h;
	int w_align;
	int h_align;
	unsigned char bit_depth;
	unsigned long pdata[4];//physical address for all components
	int stride[4];
	int size;
	int fd;//memory file descriptor such as dma_heap fd or ion fd
	enum ksc_pix_fmt pix_fmt;
};

enum rot_angle {
	ROT_0 = 0x0,
	ROT_90,
	ROT_180,
	ROT_270,
};

enum ksc_run_mode {
	 ONLINE_MODE,
	 OFFLINE_MODE1,
	 OFFLINE_MODE2,
};

#define FILTER_PHASE_DEPTH (4)
#define FILTER_WIN_MAX (4)


struct ksc_pq_para {
	unsigned char csc_dns_type;//0/1, default=0
	unsigned char post_proc_en;//0/1,default=1
	unsigned char wht_gain;//[0,128],16=1,default=16
	unsigned char blk_gain;//[0,128],16=1,default=32
	unsigned char wht_clip_val;//[0,128],default=48
	unsigned char blk_clip_val;//[0,128],default=64
	unsigned char wht_clip_rat;//[0,128],16=1,default=32
	unsigned char blk_clip_rat;//[0,128],16=1,default=32

	unsigned char res_peak_rat;//[0,64],32=1,default=24
	unsigned char res_gain;//[0,64],16=1,default=16
	unsigned char res_max_clp; //[res_min_clp+1,64],default = 64;
	unsigned char res_min_clp;//[0,64],default = 4
	unsigned char res_str;//[0,128],0=all_off,64=adj,128=all_on; default=92

	unsigned char filt_type_bod;//0/1/2, default=0
	unsigned char filt_para_adj;//0/1,default=1
	unsigned char filt_type_thL;//[0,63],default=21; when filt_para_adj=0.
	unsigned char filt_type_thH;//[filt_type_thL+1,64],default=32; when filt_para_adj=0.

	unsigned char antialias_thL;//[0,63],default=22
	unsigned char antialias_thH;//[antialias_thL+2,64],default=35
	unsigned char antialias_str;//[0,128],0=all_off,64=adj,128=all_on; default=64

	unsigned char bod_pix_num_h0;//[0,15],default=0
	unsigned char bod_pix_dif_h0;//[0,31],default=16
	unsigned char bod_pix_num_h1;//[0,15],default=0
	unsigned char bod_pix_dif_h1;//[0,31],default=16
	unsigned char bod_pix_num_v0;//[0,15],default=0
	unsigned char bod_pix_dif_v0;//[0,31],default=16
	unsigned char bod_pix_num_v1;//[0,15],default=0
	unsigned char bod_pix_dif_v1;//[0,31],default=16

	unsigned short def_val_ch0;//[0,512],default=0
	unsigned short def_val_ch1;//[0,512],default=0x80
	unsigned short def_val_ch2;//[0,512],default=0x80
	unsigned short def_val_ch3;//[0,255],default=0

	unsigned int antialias_ref;
};

struct lut_para {
	int lut_stride;
	unsigned short roi_x_min;
	unsigned short roi_y_min;
	unsigned short roi_x_max;
	unsigned short roi_y_max;
	unsigned short roi_x_max_non_ali;
	unsigned short roi_y_max_non_ali;
	unsigned short roi_w_min;
	unsigned short roi_w_max;
	unsigned short roi_h_min;
	unsigned short roi_h_max;

};

struct sunxi_ksc_online_para {
	bool enable;
	enum ksc_pix_fmt in_fmt;
	int w;
	int h;
	int bit_depth;
	int fps;
	int clk_freq;
};


struct sunxi_ksc_para {
	bool ksc_en;
	bool scaler_en;
	bool bypass;
	bool perf_mode_en;//0/1,default=0
	//for debug purpose
	bool online_wb_en;
	/*
	 * 0: online mode
	 * 1: offline mode. data bypass or FE-only
	 * 2: offline mode. BE-only
	 */
	enum ksc_run_mode mode;

	unsigned char crop_en;
	unsigned short crop_x;
	unsigned short crop_y;
	unsigned short crop_w;
	unsigned short crop_h;

	int src_w;
	int src_h;
	int dns_w;
	int dns_h;
	int dst_w;
	int dst_h;
	//Online: de---(file_fmt)----->FE(KSC)--(pix_fmt)--->BE(KSC)---(wb_fmt)-->Display
	//Offline :File---(file_fmt)----->FE(KSC)--(pix_fmt)--->BE(KSC)---(wb_fmt)-->File
	enum ksc_pix_fmt file_fmt;
	//FE output pixel format
	//0=YUV420P,1=YVU420P,2=YUV420SP,3=YVU420SP,4=YUV422P,5=YVU422P,6=YUV422SP,7=YVU422SP,8=YUV444P,9=YVU444P,10=YUV444SP,11=YVU444SP;26=AYUV
	//On KSC110 online mode fix to 6=YUV422SP, On KSC110 offline mode fix
	//to 26=AYUV
	enum ksc_pix_fmt pix_fmt;
	//KSC output pixel format
	///0=YUV420P,1=YVU420P,2=YUV420SP,3=YVU420SP/,
	///4=YUV422P,5=YVU422P,6=YUV422SP,7=YVU422SP/,
	///8=YUV444P,9=YVU444P,10=YUV444SP,11=YVU444SP/,
	///12=RGB888,13=BGR888/,
	///14=ARGB8888,15=ABGR8888,16=RGBA8888,17=BGRA8888/,
	///18=ARGB4444,19=ABGR4444,20=RGBA4444,21=BGRA4444/,
	///22=ARGB1555,23=ABGR1555,24=RGBA1555,25=BGRA1555/
	enum ksc_pix_fmt wb_fmt;
	enum csc_fmt csc_fmt;

	unsigned char bit_depth;

	unsigned char rot_mode;//0:rotate resolution by rot_angle. 1:rotate view by view_angle.
	enum rot_angle rot_angle;
	short view_angle;//[-359,359]

	unsigned char  flip_h;
	unsigned char  flip_v;

	short ost_h;//[-w,w]
	short ost_v;//[-h,h]
	short phase_h;//[-15,15]
	short phase_v;//[-15,15]


	unsigned int stitch_mode_flag;
	unsigned int ksc_default_rgb_ch2;
	unsigned int ksc_default_rgb_ch1;
	unsigned int ksc_default_rgb_ch0;

	int scale_ratio_x;
	int scale_ratio_y;
	struct ksc_pq_para pq_para;
	struct lut_para lut;

	//memory offset of lut memory
	int interp_coef_offset;
	int dnscal_coef_offset;

	struct ksc_image_t src_img;//offline input
	struct ksc_image_t dst_img;//offline output
};




/*KSC ioctl command*/

#define KSC_IOC_MAGIC 'k'

#define KSC_SET_PARA    _IOW(KSC_IOC_MAGIC, 0x0, struct sunxi_ksc_para)
#define KSC_GET_PARA    _IOW(KSC_IOC_MAGIC, 0x1, struct sunxi_ksc_para)
#define KSC_ONLINE_ENABLE    _IOW(KSC_IOC_MAGIC, 0x2, struct sunxi_ksc_online_para)

#endif /*End of file*/
