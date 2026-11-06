#ifndef _COMM_DPU_H_
#define _COMM_DPU_H_

#include <linux/defines.h>
#include <linux/common.h>
#include <linux/comm_video.h>


typedef unsigned long long  dpu_hanlde;

typedef enum _dpu_disp_range_e {
	DPU_DISP_RANGE_DEFAULT = 0x0,
	DPU_DISP_RANGE_16     = 0x1,
	DPU_DISP_RANGE_32     = 0x2,
	DPU_DISP_RANGE_48     = 0x3,
	DPU_DISP_RANGE_64     = 0x4,
	DPU_DISP_RANGE_80     = 0x5,
	DPU_DISP_RANGE_96     = 0x6,
	DPU_DISP_RANGE_112    = 0x7,
	DPU_DISP_RANGE_128    = 0x8,
	DPU_DISP_RANGE_BUTT
}dpu_disp_range_e;

typedef enum _dpu_mask_mode_e {
	DPU_MASK_MODE_DEFAULT = 0x0,
	DPU_MASK_MODE_1x1     = 0x1,
	DPU_MASK_MODE_3x3     = 0x2,
	DPU_MASK_MODE_5x5     = 0x3,
	DPU_MASK_MODE_7x7     = 0x4,
	DPU_MASK_MODE_BUTT
}dpu_mask_mode_e;

typedef enum _dpu_depth_unit_e {
	DPU_DEPTH_UNIT_DEFAULT = 0x0,
	DPU_DEPTH_UNIT_MM      = 0x1,
	DPU_DEPTH_UNIT_CM      = 0x2,
	DPU_DEPTH_UNIT_DM      = 0x3,
	DPU_DEPTH_UNIT_M       = 0x4,
	DPU_DEPTH_UNIT_BUTT
}dpu_depth_unit_e;

typedef enum _dpu_dcc_dir_e {
	DPU_DCC_DIR_DEFAULT  = 0x0,
	DPU_DCC_DIR_A12      = 0x1,
	DPU_DCC_DIR_A13      = 0x2,
	DPU_DCC_DIR_A14      = 0x3,
	DPU_DCC_DIR_BUTT
}dpu_dcc_dir_e;

typedef enum _dpu_mode_e {
	DPU_MODE_DEFAULT = 0x0,               //only sgbm,u8 disp out(no post process),16 align
	DPU_MODE_SGBM_MUX0 = 0x1,			  //only sgbm,u8 disp out(no post process),16 align
	DPU_MODE_SGBM_MUX1 = 0x2,			  //only sgbm,u16 disp out(post process),16 align
	DPU_MODE_SGBM_MUX2 = 0x3,			  //only sgbm,u8 disp out(post process),16 align
	DPU_MODE_SGBM_FGS_ONLINE_MUX0 = 0x4,  //sgbm 2 fgs online, fgs u8 disp out,16 align
	DPU_MODE_SGBM_FGS_ONLINE_MUX1 = 0x5,  //sgbm 2 fgs online, fgs u16 depth out,32 align
	DPU_MODE_SGBM_FGS_ONLINE_MUX2 = 0x6,  //sgbm 2 fgs online, sgbm u16 depth out,32 align
	DPU_MODE_FGS_MUX0 = 0x7,              //only fgs, u8 disp out,16 align
	DPU_MODE_FGS_MUX1 = 0x8,			  //only fgs, u16 depth out,32 align
	DPU_MODE_BUTT
}dpu_mode_e;


typedef struct _dpu_grp_attr_s {
	size_s left_image_size;           //minsize(64*64) maxsize(1920*1080)
	size_s right_image_size;
	dpu_mode_e dpu_mode;
	dpu_mask_mode_e mask_mode;
	dpu_disp_range_e disp_range;
	unsigned short dispstartpos;          // [0,width)
	unsigned int rshift1;               // [0,7]
	unsigned int rshift2;				  // [0,7]
	unsigned int cap1;                  // [0,65535]
	unsigned int cap2;				  // [0,65535]
	unsigned int uniqratio;             // [0,100]
	unsigned int dispshift;			  // [0,15]
	unsigned int censusshift;           // [0,255]
	unsigned int fxbaseline;            // [0,1048575]
	dpu_dcc_dir_e dcc_dir;
	unsigned int fgsmaxcount;			  // [0,31]
	unsigned int fgsmaxt;               // [0,127]
	dpu_depth_unit_e dpu_depth_unit;
	unsigned char isbtcostout;            // [0,1]
	unsigned char need_src_frame;
	frame_rate_ctrl_s frame_rate;
} dpu_grp_attr_s;

typedef struct _dpu_chn_attr_s {
	size_s img_size;
} dpu_chn_attr_s;

#endif
