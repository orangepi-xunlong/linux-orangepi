#ifndef _STITCH_REG_CFG_H_
#define _STITCH_REG_CFG_H_

#include "stitch_cfg.h"
#include "reg.h"

#define _STITCH_OFST(_BLK_T, _REG)       ((uint64_t)&(((struct _BLK_T *)0)->_REG))

#define STITCH_RD_REG(_BA, _BLK_T, _REG) \
	(_reg_read(_BA+_STITCH_OFST(_BLK_T, _REG)))

#define STITCH_RD_BITS(_BA, _BLK_T, _REG, _FLD) \
	({\
		typeof(((struct _BLK_T *)0)->_REG) _r;\
		_r.raw = _reg_read(_BA+_STITCH_OFST(_BLK_T, _REG));\
		_r.bits._FLD;\
	})

#define STITCH_WR_REG(_BA, _BLK_T, _REG, _V) \
	(_reg_write((_BA+_STITCH_OFST(_BLK_T, _REG)), _V))

#define STITCH_WR_REG_OFT(_BA, _BLK_T, _REG, _OFT, _V) \
		(_reg_write((_BA+_STITCH_OFST(_BLK_T, _REG) + _OFT), _V))

#define STITCH_WR_BITS(_BA, _BLK_T, _REG, _FLD, _V) \
	do {\
		typeof(((struct _BLK_T *)0)->_REG) _r;\
		_r.raw = _reg_read(_BA+_STITCH_OFST(_BLK_T, _REG));\
		_r.bits._FLD = _V;\
		_reg_write((_BA+_STITCH_OFST(_BLK_T, _REG)), _r.raw);\
	} while (0)

#define STITCH_WO_BITS(_BA, _BLK_T, _REG, _FLD, _V) \
	do {\
		typeof(((struct _BLK_T *)0)->_REG) _r;\
		_r.raw = 0;\
		_r.bits._FLD = _V;\
		_reg_write((_BA+_STITCH_OFST(_BLK_T, _REG)), _r.raw);\
	} while (0)



void stitch_set_base_addr(void *base, void *dma_base);

void stitch_reset_init(void);

void stitch_enable(void);

void stitch_disable(void);

//enable top_wp0/top_wp1
//top_id: top_wp0/top_wp1
void stitch_valid_param(unsigned char top_id);

void stitch_invalid_param(unsigned char top_id);
void stitch_disable_wgt(unsigned char top_id);
void stitch_disable_bld(unsigned char top_id);
void stitch_disable_left_right_wdma(unsigned char top_id);
void stitch_disable_left_right_rdma(unsigned char top_id);
void stitch_disable_left_rdma(unsigned char top_id);
void stitch_disable_right_rdma(unsigned char top_id);
void stitch_disable_left_wdma(unsigned char top_id);
void stitch_disable_right_wdma(unsigned char top_id);
void stitch_disable_uv(unsigned char sel);

void stitch_r_uv_bypass(unsigned char sel);
void stitch_w_uv_bypass(unsigned char sel);
void stitch_r_uv_half_bypass(unsigned char sel);
void stitch_w_uv_half_bypass(unsigned char sel);
void stitch_uv_bypass_clr(unsigned char sel);

//0 = from dma data; 1 = from stitch data
void stitch_src_sel(unsigned char sel);

//wgt mode，0：yuv share wgt; 1: y wgt, uv share wgt; 2: y wgt, u wgt, v wgt
void stitch_mode_sel(unsigned char sel);

//blending judge start and end for left&right img.
void stitch_bj_image_size_cfg(unsigned char top_id, struct stitch_bj_size_param *cfg);

//rdma chn size cfg: y/uv/wgh
//cfg[0] :y rdma channel
//cfg[1] :uv rdma channel
//cfg[2] :wgt rdma channel
void stitch_rdma_image_size_cfg(unsigned char top_id, struct stitch_rdma_size_param *cfg);

//wdma nbld chn size cfg: y/uv
//nbld_cfg[0] :y wdma channel
//nbld_cfg[1] :uv wdma channel

//wdma bld chn size cfg: y/uv
//bld_cfg[0] :y wdma channel
//bld_cfg[1] :uv wdma channel
void stitch_wdma_image_size_cfg(unsigned char top_id, struct stitch_wdma_nbld_size_param *nbld_cfg, struct stitch_wdma_bld_size_param *ld_cfg);

//cfg dma register, total 28 chn
void stitch_dma_cfg(struct stitch_dma_ctl *cfg);

void stitch_dma_cfg_clr_all(void);

//crop src img send to blending judge, left img drop right part, right img drop left part.
void stitch_crop2bj_cfg(unsigned char top_id, struct stitch_crop2bj_param *cfg);


void stitch_intr_clr(void);

//read intr status,0x3:stitch success, 0x0:fail
unsigned char stitch_intr_status(void);

//alpha_pixel + beta_pixel = 8’d255
//blend_out = (alpha_pixel*(left_pixel<<8) + beta_pixel*(right_pixel<<8))>>reg_x
void stitch_set_regx(unsigned char top_id, unsigned char regx);

void stitch_dump_register(void);

#endif // _STITCH_REG_CFG_H_
