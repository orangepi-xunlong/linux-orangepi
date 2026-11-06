#ifndef _LDC_CFG_H_
#define _LDC_CFG_H_

#define LDC_ADDR_ALIGN 64
#define LDC_SIZE_ALIGN 64
#define LDC_BASE_ADDR_SHIFT 6
#define LDC_MAX_WIDTH 4608
#define LDC_MAX_HEIGHT 4608

#define YUV400		0
#define NV21		1

#define YUV420p		0
#define RGB888p		1
#define LDC_CMDQ_MAX_REG_CNT (100)
#define INTERPOLATION_COEF_FBITS 6
#define INTERPOLATION_PRECISION_BITS 5
#define COEFFICIENT_PHASE_NUM (1 << INTERPOLATION_PRECISION_BITS)

enum ldc_dst_mode {
	LDC_DST_FLAT = 0,
	LDC_DST_XY_FLIP,
	LDC_DST_ROT_90,
	LDC_DST_ROT_270,
	LDC_DST_MAX,
};

struct gdc_buf {
	unsigned int addrl;
	unsigned int addrh;
	unsigned int pitch;
	unsigned short offset_x;
	unsigned short offset_y;
};

struct ldc_cfg {
	unsigned char pix_fmt;       // ldc: 0: Y only, 1: NV21 dwa: 0: YUV420, 1: RGB, 2: Y only
	bool map_bypass;

	unsigned short ras_width;      // output width
	unsigned short ras_height;     // output height
	unsigned long long map_base;

	unsigned long long src_y_base;
	unsigned long long src_c_base;
	unsigned int src_width;      // src width, including padding
	unsigned int src_height;     // src height, including padding
	unsigned short src_xstart;
	unsigned short src_xend;
	unsigned int bgcolor;        // data outside start/end if used in opeartion

	unsigned long long dst_y_base;
	unsigned long long dst_c_base;
	unsigned int extend_haddr;//dram bit[35:33]
	enum ldc_dst_mode dst_mode;
	bool use_cmdq;

	unsigned char output_target;   // 0: to scaler, 1: to DRAM
	unsigned int bdcolor;        // R[0:7], G[15:8], B[23:16]
	struct gdc_buf src_buf[3];
	int dst_width;
	int dst_height;
	struct gdc_buf dst_buf[3];
	unsigned long long mesh_id;
};

#endif // _LDC_CFG_H_
