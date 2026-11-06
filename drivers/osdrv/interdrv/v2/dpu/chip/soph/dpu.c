#include "dpu.h"
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/clk.h>
#include <linux/mm.h>
#include <uapi/linux/sched/types.h>

#include <linux/comm_video.h>

#include "base_common.h"
#include <base_cb.h>
#include <vb.h>
#include <sys.h>
#include "dpu.h"
#include "vbq.h"
#include "bind.h"
#include "ion.h"

#include <linux/delay.h>

#include <linux/kernel.h>

#define IDLE_TIMEOUT_MS      30000
#define EOF_WAIT_TIMEOUT_MS  10000
#define HW_WAIT_TIMEOUT_MS  35


#define CTX_EVENT_WKUP       0x0001
#define CTX_EVENT_EOF        0x0002
#define CTX_EVENT_ERR        0x0004
#define ALIGN_4                  4
#define ALIGN_8                  8
#define ALIGN_16                 16
#define ALIGN_32                 32
#define ALIGN_64                 64
extern int hw_wait_time;
static unsigned long long reg_base;
static unsigned long long reg_base_sgbm_ld1_dma;
static unsigned long long reg_base_sgbm_ld2_dma;
static unsigned long long reg_base_sgbm_median_dma;
static unsigned long long reg_base_sgbm_bf_dma;
static unsigned long long reg_base_fgs_gx_dma;
static unsigned long long reg_base_fgs_ux_dma;
static unsigned long long reg_base_fgs_chfh_st_dma;
static unsigned long long reg_base_fgs_chfh_ld_dma;

static unsigned int dram_base_out_l;
static unsigned int dram_base_out_h;
static unsigned int dram_base_out_btcost_l;
static unsigned int dram_base_out_btcost_h;
static unsigned int dram_base_left_l;
static unsigned int dram_base_left_h;
static unsigned int dram_base_right_l;
static unsigned int dram_base_right_h;
static unsigned int dram_base_chfh_l;
static unsigned int dram_base_chfh_h;

static unsigned long timeout;

static struct dpu_dev_s *dpu_dev;
static dpu_reg_s dpu_reg;
static struct dpu_ctx_s *dpu_ctx[DPU_MAX_GRP_NUM] = { [0 ... DPU_MAX_GRP_NUM - 1] = NULL };

//Get Available Grp lock
static struct mutex dpu_get_grp_lock;
static unsigned char dpu_grp_used[DPU_MAX_GRP_NUM];
extern bool __clk_is_enabled(struct clk *clk);
struct dpu_jobs_ctx {
	struct vb_jobs_t ins[DPU_PIPE_IN_NUM];
	struct vb_jobs_t outs[DPU_MAX_CHN_NUM];
	struct mutex lock;
} dpu_jobs[DPU_MAX_GRP_NUM];

struct dpu_handler_ctx_s {
	unsigned char dpu_dev_id;	// index of handler_ctx
	enum handler_state hdl_state;

	dpu_grp working_grp;
	unsigned char working_mask;
	unsigned char int_mask;
	unsigned int events;
	struct timespec64 time;
	struct mutex mutex;
	dpu_mode_e dpu_mode;
	// struct dpu_ctx_s dpuCtxWork[1];

} handler_ctx[DPU_IP_NUM];

struct dpu_dev_s *dpu_get_dev(void)
{
	return dpu_dev;
}
//EXPORT_SYMBOL_GPL(dpu_get_dev);

static void dpu_enable_clk(void)
{
	if(dpu_dev->clk_sys[0])
		clk_prepare_enable(dpu_dev->clk_sys[0]);

	if(dpu_dev->clk_sys[1] && !__clk_is_enabled(dpu_dev->clk_sys[1]))
		clk_prepare_enable(dpu_dev->clk_sys[1]);
}

static void dpu_disable_clk(void)
{
	if(dpu_dev->clk_sys[0])
		clk_disable_unprepare(dpu_dev->clk_sys[0]);

	if(dpu_dev->clk_sys[1] && __clk_is_enabled(dpu_dev->clk_sys[1]))
		clk_disable_unprepare(dpu_dev->clk_sys[1]);
}

static void print_vbq_size(dpu_grp dpu_grp_id)
{
	TRACE_DPU(DBG_INFO,"dpu_jobs.ins[0].vbqSize: waitq(%d),workq(%d),doneq(%d)\n",
				FIFO_SIZE(&dpu_jobs[dpu_grp_id].ins[0].waitq),
				FIFO_SIZE(&dpu_jobs[dpu_grp_id].ins[0].workq),
				FIFO_SIZE(&dpu_jobs[dpu_grp_id].ins[0].doneq));
	TRACE_DPU(DBG_INFO,"dpu_jobs.ins[1].vbqSize: waitq(%d),workq(%d),doneq(%d)\n",
				FIFO_SIZE(&dpu_jobs[dpu_grp_id].ins[1].waitq),
				FIFO_SIZE(&dpu_jobs[dpu_grp_id].ins[1].workq),
				FIFO_SIZE(&dpu_jobs[dpu_grp_id].ins[1].doneq));
	TRACE_DPU(DBG_INFO,"dpu_jobs.outs[0].vbqSize: waitq(%d),workq(%d),doneq(%d)\n",
				FIFO_SIZE(&dpu_jobs[dpu_grp_id].outs[0].waitq),
				FIFO_SIZE(&dpu_jobs[dpu_grp_id].outs[0].workq),
				FIFO_SIZE(&dpu_jobs[dpu_grp_id].outs[0].doneq));
}

static unsigned int get_mask(unsigned int src,unsigned int bits ,unsigned int start_lsb){
    return ((((1 << bits) - 1) << start_lsb) & src) >> start_lsb;
}

int dpu_fill_videoframe2buffer(mmf_chn_s chn, const video_frame_info_s *video_frame_info,
	struct video_buffer *buf)
{
	unsigned int plane_size;
	vb_cal_config_s vb_config;
	unsigned char i = 0;

	common_getpicbufferconfig(video_frame_info->video_frame.width, video_frame_info->video_frame.height,
		video_frame_info->video_frame.pixel_format, DATA_BITWIDTH_8, COMPRESS_MODE_NONE,
		ALIGN_16, &vb_config);

	buf->size.width = video_frame_info->video_frame.width;
	buf->size.height = video_frame_info->video_frame.height;
	buf->pixel_format = video_frame_info->video_frame.pixel_format;
	buf->offset_left = video_frame_info->video_frame.offset_left;
	buf->offset_top = video_frame_info->video_frame.offset_top;
	buf->offset_right = video_frame_info->video_frame.offset_right;
	buf->offset_bottom = video_frame_info->video_frame.offset_bottom;
	buf->frm_num = video_frame_info->video_frame.time_ref;
	buf->pts = video_frame_info->video_frame.pts;
	memset(&buf->frame_crop, 0, sizeof(buf->frame_crop));

	for (i = 0; i < NUM_OF_PLANES; ++i) {
		if (i >= vb_config.plane_num) {
			buf->phy_addr[i] = 0;
			buf->length[i] = 0;
			buf->stride[i] = 0;
			continue;
		}

		plane_size = (i == 0) ? vb_config.main_y_size : vb_config.main_c_size;
		buf->phy_addr[i] = video_frame_info->video_frame.phyaddr[i];
		buf->length[i] = video_frame_info->video_frame.length[i];
		buf->stride[i] = video_frame_info->video_frame.stride[i];
		if (buf->length[i] < plane_size) {
			pr_err("Mod(%s) Dev(%d) Chn(%d) Plane[%d]\n"
				, sys_get_modname(chn.mod_id), chn.dev_id, chn.chn_id, i);
			pr_err(" length(%zu) less than expected(%d).\n"
				, buf->length[i], plane_size);
			return FAILURE;
		}
		if (buf->stride[i] % ALIGN_16) {
			pr_err("Mod(%s) Dev(%d) Chn(%d) Plane[%d]\n"
				, sys_get_modname(chn.mod_id), chn.dev_id, chn.chn_id, i);
			pr_err(" stride(%d) not aligned(%d).\n"
				, buf->stride[i], ALIGN_16);
			return FAILURE;
		}
		if (buf->phy_addr[i] % ALIGN_64) {
			pr_err("Mod(%s) Dev(%d) Chn(%d) Plane[%d]\n"
				, sys_get_modname(chn.mod_id), chn.dev_id, chn.chn_id, i);
			pr_err(" address(%llx) not aligned(%d).\n"
				, buf->phy_addr[i], ALIGN_64);
			return FAILURE;
		}
	}
	// [WA-01]
	if (vb_config.plane_num > 1) {
		if (((buf->phy_addr[0] & (vb_config.addr_align - 1))
		    != (buf->phy_addr[1] & (vb_config.addr_align - 1)))
		 || ((buf->phy_addr[0] & (vb_config.addr_align - 1))
		    != (buf->phy_addr[2] & (vb_config.addr_align - 1)))) {
			pr_err("Mod(%s) Dev(%d) Chn(%d)\n"
				, sys_get_modname(chn.mod_id), chn.dev_id, chn.chn_id);
			pr_err("plane address offset (%llx-%llx-%llx)"
				, buf->phy_addr[0], buf->phy_addr[1], buf->phy_addr[2]);
			pr_err("not aligned to %#x.\n", vb_config.addr_align);
			return FAILURE;
		}
	}
	return SUCCESS;
}

void register_sgbm_ld1_ld(unsigned int seg_len, unsigned int seg_num,unsigned int SRAM_DPU_BASE_H, \
							unsigned int SGBM_LEFT_IMG_ADDR,unsigned long long dmaBaseAddr, unsigned long long regBaseAddr)
{
    //unsigned int addr =DPU_ALIGN(SGBM_LEFT_IMG_ADDR,ADDR_ALIGN);
	unsigned int addr=SGBM_LEFT_IMG_ADDR;
    unsigned int width =seg_len;
    unsigned int width_align = DPU_ALIGN(seg_len,ALIGN_16);
    unsigned int height = seg_num ;

    unsigned int sgbm_ld_crop_h_end = height -1;
    unsigned int sgbm_ld_crop_h_str = 0;
    unsigned int sgbm_ld_crop_w_end = width -1;
    unsigned int sgbm_ld_crop_w_str = 0;
    unsigned int sgbm_ld_crop_height = height -1 ;
    unsigned int sgbm_ld_crop_width = width_align -1;

    unsigned int reg_2C = sgbm_ld_crop_h_end | (sgbm_ld_crop_h_str << 16) ;
    unsigned int reg_30 = sgbm_ld_crop_w_end | (sgbm_ld_crop_w_str << 16) ;
    unsigned int reg_34 = sgbm_ld_crop_height | (sgbm_ld_crop_width << 16) ;

	reg_write_mask(dmaBaseAddr + SYS_CONTROL_OFS,((1<<8)-1) <<8,SRAM_DPU_BASE_H << 8);

    reg_write_mask(dmaBaseAddr + SYS_CONTROL_OFS,((1<<1)-1) <<16,1 << 16);
    reg_write_mask(dmaBaseAddr + SYS_CONTROL_OFS,((1<<1)-1) <<17,1 << 17);
    reg_write_mask(dmaBaseAddr + SYS_CONTROL_OFS,((1<<1)-1) <<18,1 << 18);
    reg_write_mask(dmaBaseAddr + SYS_CONTROL_OFS,((1<<1)-1) <<19,1 << 19);

	write_reg(dmaBaseAddr + DMA_BASE_ADDR_OFS, addr);
    // bit reg_seglen
    write_reg(dmaBaseAddr + DMA_SEGLEN_OFS, width_align);
    //write_reg(dmaBaseAddr + DMA_SEGLEN_OFS, seg_len);
    // reg_segnum
    write_reg(dmaBaseAddr + DMA_SEGNUM_OFS, height);
    // reg_stride
    write_reg(dmaBaseAddr + DMA_STRIDE_OFS, width_align);

    write_reg(regBaseAddr + DPU_REG_2C_OFS,reg_2C);
    write_reg(regBaseAddr + DPU_REG_30_OFS,reg_30);
    write_reg(regBaseAddr + DPU_REG_34_OFS,reg_34);

}

void register_sgbm_ld2_ld(unsigned int seg_len, unsigned int seg_num,unsigned int SRAM_DPU_BASE_H,\
						    unsigned int SGBM_RIGHT_IMG_ADDR,unsigned long long dmaBaseAddr,unsigned long long regBaseAddr)
{
	unsigned int stride;

    //unsigned int addr =DPU_ALIGN(SGBM_RIGHT_IMG_ADDR,ADDR_ALIGN);
    unsigned int addr= SGBM_RIGHT_IMG_ADDR;
    unsigned int width_align = DPU_ALIGN(seg_len,ALIGN_16);
	reg_write_mask(dmaBaseAddr + SYS_CONTROL_OFS,((1<<8)-1) <<8,SRAM_DPU_BASE_H << 8);

    reg_write_mask(dmaBaseAddr + SYS_CONTROL_OFS,((1<<1)-1) <<16,1 << 16);
    reg_write_mask(dmaBaseAddr + SYS_CONTROL_OFS,((1<<1)-1) <<17,1 << 17);
    reg_write_mask(dmaBaseAddr + SYS_CONTROL_OFS,((1<<1)-1) <<18,1 << 18);
    reg_write_mask(dmaBaseAddr + SYS_CONTROL_OFS,((1<<1)-1) <<19,1 << 19);

    write_reg(dmaBaseAddr + DMA_BASE_ADDR_OFS, addr);

    // bit reg_seglen
    write_reg(dmaBaseAddr + DMA_SEGLEN_OFS, width_align);
    //write_reg(dmaBaseAddr + DMA_SEGLEN_OFS, seg_len);

    // reg_segnum
    write_reg(dmaBaseAddr + DMA_SEGNUM_OFS, seg_num);

    // reg_stride
    stride =DPU_ALIGN(seg_len,ALIGN_16);
    write_reg(dmaBaseAddr + DMA_STRIDE_OFS, stride);
}

void register_sgbm_bf_st_ld(unsigned int seg_len, unsigned int seg_num,unsigned int SRAM_DPU_BASE_H, \
								unsigned int SGBM_BTCOST_ST_ADDR,unsigned long long dmaBaseAddr,unsigned long long regBaseAddr)
{
	unsigned int reg_20 ;
    unsigned int reg_24 ;
    unsigned int reg_28 ;


    //unsigned int addr =DPU_ALIGN(SGBM_BTCOST_ST_ADDR,ADDR_ALIGN);
	unsigned int addr=SGBM_BTCOST_ST_ADDR;

    unsigned int width = (seg_len * 16);
    unsigned int height = seg_num ;
    unsigned int sgbm_bf_st_crop_h_end = height-1;
    unsigned int sgbm_bf_st_crop_h_str = 0;
    unsigned int sgbm_bf_st_crop_w_end = width-1;
    unsigned int sgbm_bf_st_crop_w_str = 0;
    unsigned int sgbm_bf_st_crop_height = height-1;
    unsigned int sgbm_bf_st_crop_width = width-1;
    seg_len = seg_len * 256;
	write_reg(dmaBaseAddr + DMA_BASE_ADDR_OFS, addr);
	reg_write_mask(dmaBaseAddr + SYS_CONTROL_OFS,((1<<8)-1) <<8,SRAM_DPU_BASE_H << 8);

    reg_write_mask(dmaBaseAddr + SYS_CONTROL_OFS,((1<<1)-1) <<16,1 << 16);
    reg_write_mask(dmaBaseAddr + SYS_CONTROL_OFS,((1<<1)-1) <<17,1 << 17);
    reg_write_mask(dmaBaseAddr + SYS_CONTROL_OFS,((1<<1)-1) <<18,1 << 18);
    reg_write_mask(dmaBaseAddr + SYS_CONTROL_OFS,((1<<1)-1) <<19,1 << 19);
    // bit reg_seglen
    write_reg(dmaBaseAddr + DMA_SEGLEN_OFS, seg_len);

    // reg_segnum
    write_reg(dmaBaseAddr + DMA_SEGNUM_OFS, seg_num);

    // reg_stride
    write_reg(dmaBaseAddr + DMA_STRIDE_OFS, seg_len);

    reg_20 = sgbm_bf_st_crop_h_end | (sgbm_bf_st_crop_h_str << 16) ;
    reg_24 = sgbm_bf_st_crop_w_end | (sgbm_bf_st_crop_w_str << 16) ;
    reg_28 = sgbm_bf_st_crop_height | (sgbm_bf_st_crop_width << 16) ;

    write_reg(regBaseAddr + DPU_REG_20_OFS,reg_20);
    write_reg(regBaseAddr + DPU_REG_24_OFS,reg_24);
    write_reg(regBaseAddr + DPU_REG_28_OFS,reg_28);

}

void register_sgbm_median_st_ld(unsigned int seg_len, unsigned int seg_num,unsigned int chooseDma,unsigned int SRAM_DPU_BASE_H, \
									unsigned int SGBM_MEDIAN_ST_ADDR,unsigned long long dmaBaseAddr,unsigned long long regBaseAddr)
{
	unsigned int reg_38 ;
    unsigned int reg_3C ;
    unsigned int reg_40 ;

    //unsigned int addr =DPU_ALIGN(SGBM_MEDIAN_ST_ADDR,ADDR_ALIGN);
	unsigned int addr=SGBM_MEDIAN_ST_ADDR;

    unsigned int width = seg_len;
    unsigned int width_align =DPU_ALIGN(seg_len,ALIGN_16);
    unsigned int height = seg_num ;
    unsigned int sgbm_mux_st_crop_h_end = height -1;
    unsigned int sgbm_mux_st_crop_h_str = 0;
    unsigned int sgbm_mux_st_crop_w_end = width -1;
    unsigned int sgbm_mux_st_crop_w_str = 0;
    unsigned int sgbm_mux_st_crop_height = height -1;
    unsigned int sgbm_mux_st_crop_width = width_align -1;

	write_reg(dmaBaseAddr + DMA_BASE_ADDR_OFS, addr);
	reg_write_mask(dmaBaseAddr + SYS_CONTROL_OFS,((1<<8)-1) <<8,SRAM_DPU_BASE_H << 8);

    reg_write_mask(dmaBaseAddr + SYS_CONTROL_OFS,((1<<1)-1) <<16,1 << 16);
    reg_write_mask(dmaBaseAddr + SYS_CONTROL_OFS,((1<<1)-1) <<17,1 << 17);
    reg_write_mask(dmaBaseAddr + SYS_CONTROL_OFS,((1<<1)-1) <<18,1 << 18);
    reg_write_mask(dmaBaseAddr + SYS_CONTROL_OFS,((1<<1)-1) <<19,1 << 19);
    if(chooseDma == 0){
        sgbm_mux_st_crop_width = width_align/2 -1;
        sgbm_mux_st_crop_w_end = seg_len/2 -1;
    } else if(chooseDma == 1) {
        width_align= width_align*2;

    } else if(chooseDma == 2){
        sgbm_mux_st_crop_width = width_align/2 -1;
        sgbm_mux_st_crop_w_end = seg_len/2 -1;
    } else{
        TRACE_DPU(DBG_ERR,"data_sel: not in 1-3");
    }
    TRACE_DPU(DBG_INFO,"align_width:[%d]\n",width_align);
    TRACE_DPU(DBG_INFO,"data_sel:[%d]\n",chooseDma);
    // bit reg_seglen
    write_reg(dmaBaseAddr + DMA_SEGLEN_OFS, width_align);
    //write_reg(dmaBaseAddr + DMA_SEGLEN_OFS, seg_len);

    // reg_segnum
    write_reg(dmaBaseAddr + DMA_SEGNUM_OFS, seg_num);

    // reg_stride
    write_reg(dmaBaseAddr + DMA_STRIDE_OFS, width_align);

    reg_38 = sgbm_mux_st_crop_h_end | (sgbm_mux_st_crop_h_str << 16) ;
    reg_3C = sgbm_mux_st_crop_w_end | (sgbm_mux_st_crop_w_str << 16) ;
    reg_40 = sgbm_mux_st_crop_height | (sgbm_mux_st_crop_width << 16) ;

    write_reg(regBaseAddr + DPU_REG_38_OFS,reg_38);
    write_reg(regBaseAddr + DPU_REG_3C_OFS,reg_3C);
    write_reg(regBaseAddr + DPU_REG_40_OFS,reg_40);
}


//rdma--read from bottom to top
void register_fgs_chfh_ld(unsigned int seg_len, unsigned int seg_num, unsigned int SRAM_DPU_BASE_H, \
							unsigned int FGS_CHFH_LD_ADDR,unsigned long long dmaBaseAddr,unsigned long long regBaseAddr){
    //reg_write_mask(dmaBaseAddr + SYS_CONTROL_OFS, REG_ENABLE_INV_MASK, reg_enable_inv);
	unsigned int reg_44 ;
    unsigned int reg_48 ;
    unsigned int reg_4C ;
	unsigned int stride ;

    //unsigned int addr =DPU_ALIGN(FGS_CHFH_LD_ADDR,ADDR_ALIGN);
	unsigned int addr =FGS_CHFH_LD_ADDR;

    unsigned int width =seg_len;
    unsigned int width_align = DPU_ALIGN(seg_len,ALIGN_16);
    unsigned int height = seg_num ;

    unsigned int fgs_chfh_crop_h_end = height -1;
    unsigned int fgs_chfh_crop_h_str = 0;
    unsigned int fgs_chfh_crop_w_end = width -1;
    unsigned int fgs_chfh_crop_w_str = 0;
    unsigned int fgs_chfh_crop_height = height -1;
    unsigned int fgs_chfh_crop_width = width_align -1;
	write_reg(dmaBaseAddr + DMA_BASE_ADDR_OFS, addr);
	reg_write_mask(dmaBaseAddr + SYS_CONTROL_OFS,((1<<8)-1) <<8,SRAM_DPU_BASE_H << 8);

    reg_write_mask(dmaBaseAddr + SYS_CONTROL_OFS,((1<<1)-1) <<16,0 << 16);
    reg_write_mask(dmaBaseAddr + SYS_CONTROL_OFS,((1<<1)-1) <<17,0 << 17);
    reg_write_mask(dmaBaseAddr + SYS_CONTROL_OFS,((1<<1)-1) <<18,0 << 18);
    reg_write_mask(dmaBaseAddr + SYS_CONTROL_OFS,((1<<1)-1) <<19,0 << 19);

    // bit reg_seglen
    write_reg(dmaBaseAddr + DMA_SEGLEN_OFS, width_align);
    //write_reg(dmaBaseAddr + DMA_SEGLEN_OFS, seg_len);

    // reg_segnum
    write_reg(dmaBaseAddr + DMA_SEGNUM_OFS, seg_num);

    // reg_stride
    stride =DPU_ALIGN(seg_len,ALIGN_16);
    write_reg(dmaBaseAddr + DMA_STRIDE_OFS, stride);

    reg_44 = fgs_chfh_crop_h_end | (fgs_chfh_crop_h_str << 16) ;
    reg_48 = fgs_chfh_crop_w_end | (fgs_chfh_crop_w_str << 16) ;
    reg_4C = fgs_chfh_crop_height | (fgs_chfh_crop_width << 16) ;

    write_reg(regBaseAddr + DPU_REG_44_OFS,reg_44);  //fgs_chfh
    write_reg(regBaseAddr + DPU_REG_48_OFS,reg_48);
    write_reg(regBaseAddr + DPU_REG_4C_OFS,reg_4C);

    write_reg(regBaseAddr + DPU_REG_7C_OFS,reg_44);  //independent
    write_reg(regBaseAddr + DPU_REG_80_OFS,reg_48);
    write_reg(regBaseAddr + DPU_REG_84_OFS,reg_4C);

    reg_write_mask(regBaseAddr + DPU_REG_98_OFS,((1<<4)-1) <<8,SRAM_DPU_BASE_H << 8);
    write_reg(regBaseAddr + DPU_REG_9C_OFS, addr);

}

//rdma
void register_fgs_gx_ld(unsigned int seg_len, unsigned int seg_num, unsigned int SRAM_DPU_BASE_H, \
						unsigned int FGS_GX_LD_ADDR,unsigned long long dmaBaseAddr,unsigned long long regBaseAddr){
    //unsigned int addr =DPU_ALIGN(FGS_GX_LD_ADDR,ADDR_ALIGN);
	unsigned int reg_50 ;
    unsigned int reg_54 ;
    unsigned int reg_58 ;
	unsigned int stride ;
	unsigned int addr=FGS_GX_LD_ADDR;
    unsigned int width =seg_len;
    unsigned int width_align = DPU_ALIGN(seg_len,ALIGN_16);
    unsigned int height = seg_num ;

    unsigned int fgs_gx_crop_h_end = height-1;
    unsigned int fgs_gx_crop_h_str = 0;
    unsigned int fgs_gx_crop_w_end = width-1;
    unsigned int fgs_gx_crop_w_str = 0;
    unsigned int fgs_gx_crop_height = height-1;
    unsigned int fgs_gx_crop_width = width_align-1;
	write_reg(dmaBaseAddr + DMA_BASE_ADDR_OFS, addr);

    reg_write_mask(dmaBaseAddr + SYS_CONTROL_OFS,((1<<1)-1) <<16,1 << 16);
    reg_write_mask(dmaBaseAddr + SYS_CONTROL_OFS,((1<<1)-1) <<17,0 << 17);
    reg_write_mask(dmaBaseAddr + SYS_CONTROL_OFS,((1<<1)-1) <<18,0 << 18);
    reg_write_mask(dmaBaseAddr + SYS_CONTROL_OFS,((1<<1)-1) <<19,0 << 19);

    reg_write_mask(dmaBaseAddr + SYS_CONTROL_OFS,((1<<8)-1) <<8,SRAM_DPU_BASE_H << 8);
    // bit reg_seglen
    write_reg(dmaBaseAddr + DMA_SEGLEN_OFS, width_align);
    //write_reg(dmaBaseAddr + DMA_SEGLEN_OFS, seg_len);

    // reg_segnum
    write_reg(dmaBaseAddr + DMA_SEGNUM_OFS, seg_num);

    // reg_stride
    stride =DPU_ALIGN(seg_len,ALIGN_16);
    write_reg(dmaBaseAddr + DMA_STRIDE_OFS, stride);

    reg_50 = fgs_gx_crop_h_end | (fgs_gx_crop_h_str << 16) ;
    reg_54 = fgs_gx_crop_w_end | (fgs_gx_crop_w_str << 16) ;
    reg_58 = fgs_gx_crop_height | (fgs_gx_crop_width << 16) ;

    write_reg(regBaseAddr + DPU_REG_50_OFS,reg_50);
    write_reg(regBaseAddr + DPU_REG_54_OFS,reg_54);
    write_reg(regBaseAddr + DPU_REG_58_OFS,reg_58);
}

//wdma
void register_fgs_chfh_st(unsigned int seg_len, unsigned int seg_num, unsigned int SRAM_DPU_BASE_H, \
							unsigned int FGS_CHFH_ST_ADDR,unsigned long long dmaBaseAddr,unsigned long long regBaseAddr){
    //unsigned int addr =DPU_ALIGN(FGS_CHFH_ST_ADDR,ADDR_ALIGN);
	unsigned int stride;
	unsigned int addr=FGS_CHFH_ST_ADDR;
    write_reg(dmaBaseAddr + DMA_BASE_ADDR_OFS, addr);

    reg_write_mask(dmaBaseAddr + SYS_CONTROL_OFS,((1<<8)-1) <<8,SRAM_DPU_BASE_H << 8);

    reg_write_mask(dmaBaseAddr + SYS_CONTROL_OFS,((1<<1)-1) <<16,1 << 16);
    reg_write_mask(dmaBaseAddr + SYS_CONTROL_OFS,((1<<1)-1) <<17,0 << 17);
    reg_write_mask(dmaBaseAddr + SYS_CONTROL_OFS,((1<<1)-1) <<18,0 << 18);
    reg_write_mask(dmaBaseAddr + SYS_CONTROL_OFS,((1<<1)-1) <<19,0 << 19);
    stride =DPU_ALIGN(seg_len,ALIGN_16);
    // bit reg_seglen
    write_reg(dmaBaseAddr + DMA_SEGLEN_OFS, stride);
    //write_reg(dmaBaseAddr + DMA_SEGLEN_OFS, seg_len);

    // reg_segnum
    write_reg(dmaBaseAddr + DMA_SEGNUM_OFS, seg_num);

    // reg_stride

    write_reg(dmaBaseAddr + DMA_STRIDE_OFS, stride);

    reg_write_mask(regBaseAddr + DPU_REG_98_OFS,((1<<4)-1) <<12,SRAM_DPU_BASE_H << 12);
    write_reg(regBaseAddr + DPU_REG_A0_OFS, addr);
}

//wdma
void register_fgs_ux_st(unsigned int seg_len, unsigned int seg_num,unsigned int chooseChn, unsigned int SRAM_DPU_BASE_H,\
							unsigned int FGS_UX_ST_ADDR,unsigned long long dmaBaseAddr,unsigned long long regBaseAddr)
{
    //unsigned int addr =DPU_ALIGN(FGS_UX_ST_ADDR,ADDR_ALIGN);
	unsigned int stride;
	unsigned int reg_5C ;
    unsigned int reg_60 ;
    unsigned int reg_64 ;
	unsigned int addr=FGS_UX_ST_ADDR;

    unsigned int width =seg_len;
    unsigned int height = seg_num ;

    unsigned int fgs_ux_crop_h_end = height -1;
    unsigned int fgs_ux_crop_h_str = 0;
    unsigned int fgs_ux_crop_w_end = width/2 -1;
    unsigned int fgs_ux_crop_w_str = 0;
    unsigned int fgs_ux_crop_height = height-1 ;
    unsigned int fgs_ux_crop_width = DPU_ALIGN(width/2,16) -1;
    if(chooseChn == 1){
        fgs_ux_crop_w_end = width -1;
        fgs_ux_crop_width= DPU_ALIGN(width,32) -1;
    }

    stride =DPU_ALIGN(seg_len,32);

	write_reg(dmaBaseAddr + DMA_BASE_ADDR_OFS, addr);

    reg_write_mask(dmaBaseAddr + SYS_CONTROL_OFS,((1<<8)-1) <<8,SRAM_DPU_BASE_H << 8);

    reg_write_mask(dmaBaseAddr + SYS_CONTROL_OFS,((1<<1)-1) <<16,1 << 16);
    reg_write_mask(dmaBaseAddr + SYS_CONTROL_OFS,((1<<1)-1) <<17,0 << 17);
    reg_write_mask(dmaBaseAddr + SYS_CONTROL_OFS,((1<<1)-1) <<18,0 << 18);
    reg_write_mask(dmaBaseAddr + SYS_CONTROL_OFS,((1<<1)-1) <<19,0 << 19);
    // bit reg_seglen
    //write_reg(dmaBaseAddr + DMA_SEGLEN_OFS, seg_len);
    write_reg(dmaBaseAddr + DMA_SEGLEN_OFS, stride);

    // reg_segnum
    write_reg(dmaBaseAddr + DMA_SEGNUM_OFS, seg_num);

    // reg_stride

    write_reg(dmaBaseAddr + DMA_STRIDE_OFS, stride);

    reg_5C = fgs_ux_crop_h_end | (fgs_ux_crop_h_str << 16) ;
    reg_60 = fgs_ux_crop_w_end | (fgs_ux_crop_w_str << 16) ;
    reg_64 = fgs_ux_crop_height | (fgs_ux_crop_width << 16) ;

    write_reg(regBaseAddr + DPU_REG_5C_OFS,reg_5C);
    write_reg(regBaseAddr + DPU_REG_60_OFS,reg_60);
    write_reg(regBaseAddr + DPU_REG_64_OFS,reg_64);
}

void getsgbm_status(void)
{
	unsigned int reg_18,reg_68;
    TRACE_DPU(DBG_INFO,"[Print]Read sgbm debug status ...\n");
    reg_18 = read_reg(reg_base + DPU_REG_18_OFS);
	reg_68 = read_reg(reg_base + DPU_REG_68_OFS);
    TRACE_DPU(DBG_INFO,"[Print][DPU][reg_18]:  %d\n", reg_18);
    TRACE_DPU(DBG_INFO,"[Print][DPU][sgbm_line_cnt]:  %d\n", get_mask(reg_18, 11, 0));
    TRACE_DPU(DBG_INFO,"[Print][DPU][sgbm_pre_boxfilter_busy]:  %d\n", get_mask(reg_18, 1, 11));
    TRACE_DPU(DBG_INFO,"[Print][DPU][sgbm_boxfilter_busy]:  %d\n", get_mask(reg_18, 1, 12));
    TRACE_DPU(DBG_INFO,"[Print][DPU][sgbm_dcc_busy]:  %d\n", get_mask(reg_18, 1, 13));
    TRACE_DPU(DBG_INFO,"[Print][DPU][sgbm_wta_busy]:  %d\n", get_mask(reg_18, 1, 14));
    TRACE_DPU(DBG_INFO,"[Print][DPU][sgbm_dispinterp_busy]:  %d\n", get_mask(reg_18, 1, 15));
    TRACE_DPU(DBG_INFO,"[Print][DPU][sgbm_median_busy]:  %d\n", get_mask(reg_18, 1, 16));
    TRACE_DPU(DBG_INFO,"[Print][DPU][sgbm_busy]:  %d\n", get_mask(reg_18, 1, 17));

    TRACE_DPU(DBG_INFO,"[Print][DPU][sgbm_ld1_dma_idle]:  %d\n", get_mask(reg_18, 1, 18));
    TRACE_DPU(DBG_INFO,"[Print][DPU][sgbm_ld2_dma_idle]:  %d\n", get_mask(reg_18, 1, 19));
    TRACE_DPU(DBG_INFO,"[Print][DPU][sgbm_mux_st_dma_idle]:  %d\n", get_mask(reg_18, 1, 20));
    TRACE_DPU(DBG_INFO,"[Print][DPU][sgbm_boxfilter_st_dma_idle]:  %d\n", get_mask(reg_18, 1, 21));

	TRACE_DPU(DBG_INFO,"[Print][DPU][reg_dpu_sgbm_frame_done]:  %d\n", get_mask(reg_68, 1, 1));
    TRACE_DPU(DBG_INFO,"[Print]Read sgbm debug status done ...\n");
}

void getfgs_status(void)
{
	unsigned int reg_18;
	unsigned int reg_8c,reg_68;
    TRACE_DPU(DBG_INFO,"[Print]Read fgs debug status ...\n");
    reg_18 = read_reg(reg_base + DPU_REG_18_OFS);
    reg_8c = read_reg(reg_base + DPU_REG_8C_OFS);
	reg_68 = read_reg(reg_base + DPU_REG_68_OFS);
    TRACE_DPU(DBG_INFO,"[Print][DPU][reg_18]:  %d\n", reg_18);
    TRACE_DPU(DBG_INFO,"[Print][DPU][fgs_dma_idle_r_gx]:  %d\n", get_mask(reg_18, 1, 22));
    TRACE_DPU(DBG_INFO,"[Print][DPU][fgs_dma_idle_r_chfh]:  %d\n", get_mask(reg_18, 1, 23));
    TRACE_DPU(DBG_INFO,"[Print][DPU][fgs_dma_idle_w_ux]:  %d\n", get_mask(reg_18, 1, 24));
    TRACE_DPU(DBG_INFO,"[Print][DPU][fgs_dma_idle_w_chfh]:  %d\n", get_mask(reg_18, 1, 25));

    TRACE_DPU(DBG_INFO,"[Print][DPU][fgs_circle_cnt]:  %d\n", get_mask(reg_8c, 8, 0));
    TRACE_DPU(DBG_INFO,"[Print][DPU][fgs_line_cnt]:  %d\n", get_mask(reg_8c, 14, 8));
	TRACE_DPU(DBG_INFO,"[Print][DPU][reg_dpu_fgs_frame_done]:  %d\n", get_mask(reg_68, 1, 11));
    TRACE_DPU(DBG_INFO,"[Print]Read fgs debug status done ...\n");
}


static inline int check_dpu_grp_created(dpu_grp grp)
{
	if (!dpu_ctx[grp] || !dpu_ctx[grp]->iscreated) {
		TRACE_DPU(DBG_ERR, "Grp(%d) isn't created yet.\n", grp);
		return ERR_DPU_UNEXIST;
	}
	return SUCCESS;
}

static inline int check_dpu_grp_valid(dpu_grp dpu_grp_id)
{
	if ((dpu_grp_id >= DPU_MAX_GRP_NUM) || (dpu_grp_id < 0)) {
		TRACE_DPU(DBG_ERR, "Grp(%d) invalid .\n",
					dpu_grp_id);
		return ERR_DPU_ILLEGAL_PARAM;
	}
	return SUCCESS;
}

static inline int check_dpu_chn_valid(dpu_grp dpu_grp_id, dpu_chn dpu_chn_id)
{
	if ((dpu_chn_id >= DPU_MAX_CHN_NUM) || (dpu_chn_id < 0)) {
	TRACE_DPU(DBG_ERR, "Grp(%d) Chn-out(%d) invalid .\n",
				dpu_grp_id, dpu_chn_id);
	return ERR_DPU_ILLEGAL_PARAM;
	}

	return SUCCESS;
}

int check_dpu_id(dpu_grp dpu_grp_id, dpu_chn dpu_chn_id)
{
	int ret = SUCCESS;

	ret = check_dpu_grp_valid(dpu_grp_id);
	if (ret != SUCCESS)
		return ret;

	ret = check_dpu_grp_created(dpu_grp_id);
	if (ret != SUCCESS)
		return ret;

	ret = check_dpu_chn_valid(dpu_grp_id, dpu_chn_id);
	if (ret != SUCCESS)
		return ret;
	return ret;
}

void dpu_notify_wkup_evt(unsigned char dpu_dev_id)
{
	if (dpu_dev_id >= DPU_IP_NUM) {
		TRACE_DPU(DBG_ERR, "invalid dev(%d)\n", dpu_dev_id);
		return;
	}
	// mutex_lock(&handler_ctx[dpu_dev_id].mutex);
	handler_ctx[dpu_dev_id].events |= CTX_EVENT_WKUP;
	// mutex_unlock(&handler_ctx[dpu_dev_id].mutex);
	TRACE_DPU(DBG_INFO, "wake up IP(%d) event(%d)\n",dpu_dev_id,handler_ctx[dpu_dev_id].events);
	wake_up_interruptible(&dpu_dev->wait);
}

void dpu_notify_isr_evt(void)
{
	unsigned char i;

	for (i = 0; i < DPU_IP_NUM; i++) {
		if (handler_ctx[i].hdl_state == HANDLER_STATE_RUN) {
			// mutex_lock(&handler_ctx[i].mutex);
			handler_ctx[i].events |= CTX_EVENT_EOF;
			// mutex_unlock(&handler_ctx[i].mutex);
			TRACE_DPU(DBG_INFO, "handler_ctx[%d] state=%d, event=0x%x\n",
					i, handler_ctx[i].hdl_state,
					handler_ctx[i].events);
			break;
		} else
			TRACE_DPU(DBG_INFO, "handler_ctx[%d] state=%d, event=0x%x\n",
					i, handler_ctx[i].hdl_state,
					handler_ctx[i].events);
	}

	wake_up_interruptible(&dpu_dev->wait);
}

// static int _vb_dqbuf(mmf_chn_s chn, enum chn_type_e chn_type, vb_blk *blk)
// {
// 	struct vb_s *p;
// 	*blk = VB_INVALID_HANDLE;

// 	// get vb from workq which is done.
// 	if (base_mod_jobs_workq_empty(chn, chn_type)) {
// 		TRACE_DPU(DBG_ERR, "Mod(%d) ChnId(%d) No more vb for dequeue.\n",
// 			       chn.mod_id, chn.chn_id);
// 		return FAILURE;
// 	}
// 	p = (struct vb_s *)base_mod_jobs_workq_pop(chn, chn_type);
// 	if (!p)
// 		return FAILURE;

// 	*blk = (vb_blk)p;
// 	p->mod_ids &= ~BIT(chn.mod_id);

// 	return SUCCESS;
// }

int32_t _vb_qbuf(mmf_chn_s chn, enum chn_type_e chn_type, struct vb_jobs_t *jobs, vb_blk blk)
{
	struct vb_s *vb = (struct vb_s *)blk;
	int ret = SUCCESS;
	struct vb_s *vb_tmp;

	pr_debug("%s dev(%d) chn(%d) chnType(%d): phy-addr(%lld) cnt(%d)\n",
		     sys_get_modname(chn.mod_id), chn.dev_id, chn.chn_id, chn_type,
		     vb->phy_addr, vb->usr_cnt.counter);

	if (!jobs) {
		pr_err("mod(%s), vb_qbuf fail, error, empty jobs\n", sys_get_modname(chn.mod_id));
		return FAILURE;
	}
	if (!jobs->inited) {
		pr_err("mod(%s), vb_qbuf fail, jobs not initialized yet\n", sys_get_modname(chn.mod_id));
		return FAILURE;
	}

	mutex_lock(&jobs->lock);
	if (chn_type == CHN_TYPE_OUT) {
		if (FIFO_FULL(&jobs->workq)) {
			mutex_unlock(&jobs->lock);
			pr_err("%s workq is full. drop new one.\n"
				     , sys_get_modname(chn.mod_id));
			return -ENOBUFS;
		}
		vb->buf.dev_num = chn.chn_id;
		FIFO_PUSH(&jobs->workq, vb);
	} else {
		if (FIFO_FULL(&jobs->waitq)) {
			FIFO_POP(&jobs->waitq, &vb_tmp);
			vb_release_block((vb_blk)vb_tmp);
		}
		FIFO_PUSH(&jobs->waitq, vb);
		up(&jobs->sem);
	}
	mutex_unlock(&jobs->lock);

	atomic_fetch_add(1, &vb->usr_cnt);
	atomic_long_fetch_or(BIT(chn.mod_id), &vb->mod_ids);
	return ret;
}

static void release_buffers_working(dpu_grp dpu_grp_id)
{
	struct dpu_chn_cfg_s *stChnCfg;
	vb_blk blk_left;
	vb_blk blk_right;
	vb_blk blk_out[2] ;
	dpu_chn dpu_chn_id;
	mmf_chn_s chn_left = {.mod_id = ID_DPU, .dev_id = dpu_grp_id, .chn_id = 0};
	mmf_chn_s chn_right = {.mod_id = ID_DPU, .dev_id = dpu_grp_id, .chn_id = 1};
	mmf_chn_s chn_out[2] ={chn_left,chn_right};

	vb_dqbuf(chn_left, &dpu_jobs[dpu_grp_id].ins[0], &blk_left);
	if (blk_left != VB_INVALID_HANDLE)
		vb_release_block(blk_left);

	vb_dqbuf(chn_right, &dpu_jobs[dpu_grp_id].ins[1], &blk_right);
	if (blk_right != VB_INVALID_HANDLE)
		vb_release_block(blk_right);

	for (dpu_chn_id = 0; dpu_chn_id < dpu_ctx[dpu_grp_id]->chn_num; ++dpu_chn_id) {
		stChnCfg = &dpu_ctx[dpu_grp_id]->chn_cfgs[dpu_chn_id];
		chn_out[dpu_chn_id].mod_id = ID_DPU;
		chn_out[dpu_chn_id].dev_id = dpu_grp_id;
		chn_out[dpu_chn_id].chn_id = dpu_chn_id;
		if (!stChnCfg->isenabled)
			continue;

		// vb_cancel_block(chn_out[dpu_chn_id]);
		while (!base_mod_jobs_workq_empty(&dpu_jobs[dpu_grp_id].outs[dpu_chn_id])) {
			vb_dqbuf(chn_out[dpu_chn_id], &dpu_jobs[dpu_grp_id].outs[dpu_chn_id], &blk_out[dpu_chn_id]);
			if (blk_out[dpu_chn_id] != VB_INVALID_HANDLE)
				vb_release_block(blk_out[dpu_chn_id]);
		}
	}
}


static void hw_reset(dpu_grp dpu_grp_id)
{
	TRACE_DPU(DBG_INFO,"dpu reset !\n");
	release_buffers_working(dpu_grp_id);
	//dpu_reset();
}

int get_dev_info_by_chn(mmf_chn_s chn, enum chn_type_e chn_type)
{
	if (chn.mod_id != ID_DPU)
		return 0;
	if (dpu_ctx[chn.dev_id] == NULL) {
		TRACE_DPU(DBG_ERR, "Grp(%d) isn't created yet.\n", chn.dev_id);
		return 0;
	}

	return dpu_ctx[chn.dev_id]->dpu_dev_id;

}

static dpu_grp find_next_en_grp(dpu_grp working_grp, unsigned char dpu_dev_id)
{
	dpu_grp i = working_grp;
	unsigned char count = 0;
	struct vb_jobs_t *jobs_L;
	struct vb_jobs_t *jobs_R;

	do {
		++i;
		if (i >= DPU_MAX_GRP_NUM)
			i = 0;

		if (dpu_ctx[i] && dpu_ctx[i]->iscreated && dpu_ctx[i]->isstarted
			&& (dpu_ctx[i]->dpu_dev_id == dpu_dev_id)) {
			// TRACE_DPU(DBG_INFO, "iscreated(%d)  isstarted(%d)  dev(%d)  i(%d)\n",
			// 		dpu_ctx[i]->iscreated,dpu_ctx[i]->isstarted,dpu_ctx[i]->dpu_dev_id,i);
			if(!(&dpu_jobs[i].ins[0]) || !(&dpu_jobs[i].ins[1]) ){
				TRACE_DPU(DBG_INFO,"Grp(%d) pipe(%d)job ptr is null\n",working_grp,0);
				continue;
			}
			jobs_L = &dpu_jobs[i].ins[0];
			jobs_R = &dpu_jobs[i].ins[1];
			if ((!jobs_L) || (!jobs_R)) {
				TRACE_DPU(DBG_INFO, "get jobs failed\n");
				continue;
			}

			// if (!down_trylock(&jobs_L->sem) && !down_trylock(&jobs_R->sem))
			// 	return i;

			if ( (!base_mod_jobs_waitq_empty(&dpu_jobs[i].ins[0])) &&
					(!base_mod_jobs_waitq_empty(&dpu_jobs[i].ins[1]))){
						TRACE_DPU(DBG_INFO,"next enable grp(%d)\n",i);
						return i;
			}
		}
	} while (++count < DPU_MAX_GRP_NUM);

	return DPU_MAX_GRP_NUM;
}

static unsigned char get_work_mask(struct dpu_ctx_s *ctx)
{
	unsigned char mask = 0;
	dpu_chn dpu_chn_id;

	if (!ctx->iscreated || !ctx->isstarted)
		return 0;

	for (dpu_chn_id = 0; dpu_chn_id < ctx->chn_num; ++dpu_chn_id) {
		if (!ctx->chn_cfgs[dpu_chn_id].isenabled)
			continue;
		mask |= BIT(dpu_chn_id);
	}
	if (mask == 0)
		return 0;

	return mask;
}

void dpu_irq_handler(unsigned char intr_status, struct dpu_dev_s *wdev)
{
	TRACE_DPU(DBG_INFO," irq num(%d)\n",wdev->irq_num );
	if(intr_status == DPU_INTR_STATE_DONE){
		TRACE_DPU(DBG_INFO, "%s: intr_status(%d)\n", __func__,DPU_INTR_STATE_DONE);
		dpu_notify_isr_evt();
	} else {
		TRACE_DPU(DBG_ERR, "%s: intr_status(%d),Frame No Done!  \n", __func__,DPU_INTR_STATE_OTHERS);
		handler_ctx[0].events = 0;
		handler_ctx[0].working_mask =0;
		dpu_ctx[handler_ctx->working_grp]->grp_state = GRP_STATE_IDLE;
		hw_reset(handler_ctx->working_grp);

		handler_ctx[0].working_grp
			= find_next_en_grp(handler_ctx[0].working_grp, 0);

		// unfinished job found, need to re-trig event handler
		if (handler_ctx[0].working_grp != DPU_MAX_GRP_NUM)
			dpu_notify_wkup_evt(handler_ctx[0].dpu_dev_id);

	}
}

static unsigned char dpu_enable_handler_ctx(struct dpu_handler_ctx_s *ctx)
{
	unsigned char dpu_dev_id = ctx->dpu_dev_id;
	int i;

	for (i = 0; i < DPU_MAX_GRP_NUM; i++)
		if (dpu_ctx[i] && dpu_ctx[i]->iscreated && dpu_ctx[i]->isstarted &&
		    dpu_ctx[i]->dpu_dev_id == dpu_dev_id)
			return TRUE;

	return FALSE;
}

static struct vbq *_get_doneq(mmf_chn_s chn)
{
	struct vb_jobs_t *jobs = &dpu_jobs[chn.dev_id].outs[chn.chn_id];
	return &jobs->doneq;
}


// int dpu_get_chn_buffer(mmf_chn_s chn, vb_blk *blk,enum chn_type_e eChnType ,int timeout_ms)
// {
// 	int ret = FAILURE;
// 	struct vb_jobs_t *jobs = base_get_jobs_by_chn(chn, eChnType);
// 	struct vb_s *vb;
// 	struct vbq *doneq = _get_doneq(chn);
// 	struct snap_s *s;

// 	if (!jobs) {
// 		pr_err("mod(%s), get chn buf fail, jobs NULL\n", sys_get_modname(chn.mod_id));
// 		return FAILURE;
// 	}

int dpu_get_chn_buffer(mmf_chn_s chn, vb_blk *blk,enum chn_type_e eChnType ,int timeout_ms)
{
	int ret = FAILURE;
	struct vb_jobs_t *jobs = (eChnType == CHN_TYPE_OUT) ? &dpu_jobs[chn.dev_id].outs[chn.chn_id] : \
								&dpu_jobs[chn.dev_id].ins[chn.chn_id];
	struct vb_s *vb;
	struct vbq *doneq = _get_doneq(chn);
	struct snap_s *s;

	if (!jobs) {
		pr_err("mod(%s), get chn buf fail, jobs NULL\n", sys_get_modname(chn.mod_id));
		return FAILURE;
	}

	if (!jobs->inited) {
		pr_err("mod(%s) get chn buf fail, not inited yet\n", sys_get_modname(chn.mod_id));
		return FAILURE;
	}

	mutex_lock(&jobs->dlock);
	if (!FIFO_EMPTY(doneq)) {
		FIFO_POP(doneq, &vb);
		atomic_long_fetch_and(~BIT(chn.mod_id), &vb->mod_ids);
		atomic_long_fetch_or(BIT(ID_USER), &vb->mod_ids);
		mutex_unlock(&jobs->dlock);
		*blk = (vb_blk)vb;
		return SUCCESS;
	}

	s = vmalloc(sizeof(*s));
	if (!s) {
		mutex_unlock(&jobs->dlock);
		return FAILURE;
	}

	// if (!jobs->inited) {
	// 	pr_err("mod(%s) get chn buf fail, not inited yet\n", sys_get_modname(chn.mod_id));
	// 	return FAILURE;
	// }

	// mutex_lock(&jobs->dlock);
	// if (!FIFO_EMPTY(doneq)) {
	// 	FIFO_POP(doneq, &vb);
	// 	vb->mod_ids &= ~BIT(chn.mod_id);
	// 	vb->mod_ids |= BIT(ID_USER);
	// 	mutex_unlock(&jobs->dlock);
	// 	*blk = (vb_blk)vb;
	// 	return SUCCESS;
	// }

	// s = vmalloc(sizeof(*s));
	// if (!s) {
	// 	mutex_unlock(&jobs->dlock);
	// 	return FAILURE;
	// }

	init_waitqueue_head(&s->cond_queue);

	s->chn = chn;
	s->blk = VB_INVALID_HANDLE;
	s->avail = FALSE;

	if (timeout_ms < 0) {
		TAILQ_INSERT_TAIL(&jobs->snap_jobs, s, tailq);
		mutex_unlock(&jobs->dlock);
		ret = wait_event_interruptible(s->cond_queue, s->avail);
		// ret < 0, interrupt by a signal
		// ret = 0, condition true
	} else {
		TAILQ_INSERT_TAIL(&jobs->snap_jobs, s, tailq);
		mutex_unlock(&jobs->dlock);
		ret = wait_event_interruptible_timeout(s->cond_queue, s->avail, msecs_to_jiffies(timeout_ms));
		// ret < 0, interrupted by a signal
		// ret = 0, timeout
		// ret = 1, condition true
	}

	if (s->avail)
		ret = 0;
	else
		ret = -1;

	if (!ret) {
		*blk = s->blk;
	} else {
		mutex_lock(&jobs->dlock);
		TAILQ_REMOVE(&jobs->snap_jobs, s, tailq);
		mutex_unlock(&jobs->dlock);
		pr_err("Mod(%s) Grp(%d) Chn(%d), jobs wait(%d) work(%d) done(%d)\n"
			, sys_get_modname(chn.mod_id), chn.dev_id, chn.chn_id
			, FIFO_SIZE(&jobs->waitq), FIFO_SIZE(&jobs->workq), FIFO_SIZE(&jobs->doneq));
	}

	vfree(s);
	return ret;
}

int dpu_create_grp(dpu_grp dpu_grp_id,dpu_grp_attr_s *pstGrpAttr)
{
	int ret;
	unsigned char u8DevUsed;
	int i;

	ret = MOD_CHECK_NULL_PTR(ID_DPU, pstGrpAttr);
	if (ret != SUCCESS)
		return ret;

	ret = check_dpu_grp_valid(dpu_grp_id);
	if (ret != SUCCESS)
		return ret;

	if (dpu_ctx[dpu_grp_id]){
		TRACE_DPU(DBG_INFO, "DPU GRP existed!!!\n");
		return FAILURE;
	}


	u8DevUsed = 0;

	if (u8DevUsed >= DPU_IP_NUM) {
		u8DevUsed = DPU_IP_NUM - 1 ;
		TRACE_DPU(DBG_WARN, "DPU only allow DpuDev 0.\n");
	}

	dpu_ctx[dpu_grp_id] = kzalloc(sizeof(struct dpu_ctx_s), GFP_ATOMIC);
	if (!dpu_ctx[dpu_grp_id]) {
		TRACE_DPU(DBG_ERR, "dpu_ctx_s kzalloc fail.\n");
		return ERR_DPU_NOMEM;
	}

	if (pstGrpAttr->frame_rate.src_frame_rate < pstGrpAttr->frame_rate.dst_frame_rate) {
		TRACE_DPU(DBG_WARN, "Grp(%d) frame_rate ctrl, src(%d) < dst(%d), not support\n"
				, dpu_grp_id, pstGrpAttr->frame_rate.src_frame_rate
				, pstGrpAttr->frame_rate.dst_frame_rate);
		return ERR_DPU_ILLEGAL_PARAM;
	}

	mutex_init(&dpu_ctx[dpu_grp_id]->lock);
	// mutex_init(&dpu_jobs[dpu_grp_id].lock);
	dpu_ctx[dpu_grp_id]->iscreated = TRUE;
	dpu_ctx[dpu_grp_id]->chn_num = 1;
	dpu_ctx[dpu_grp_id]->pixel_format = PIXEL_FORMAT_YUV_400 ;
	dpu_ctx[dpu_grp_id]->cost_time_for_sec = 0 ;
	dpu_ctx[dpu_grp_id]->frame_num = 0;
	dpu_ctx[dpu_grp_id]->dpu_dev_id =0;
	dpu_ctx[dpu_grp_id]->chfh_blk = VB_INVALID_HANDLE;
	memcpy(&dpu_ctx[dpu_grp_id]->grp_attr, pstGrpAttr, sizeof(dpu_ctx[dpu_grp_id]->grp_attr));

	for(i=0; i<DPU_PIPE_IN_NUM; ++i){
		base_mod_jobs_init(&dpu_jobs[dpu_grp_id].ins[i], DPU_WAITQ_DEPTH_IN, DPU_WORKQ_DEPTH_IN, DPU_DONEQ_DEPTH_IN);
		TRACE_DPU(DBG_INFO, "Grp(%d) ChnIn(%d) Dev(%d)waitqDepth(%d) workqDepth(%d) doneqDepth(%d)\n",
		dpu_grp_id, i,dpu_ctx[dpu_grp_id]->dpu_dev_id, DPU_WAITQ_DEPTH_IN, DPU_WORKQ_DEPTH_IN, DPU_DONEQ_DEPTH_IN);
	}

	memset(&dpu_ctx[dpu_grp_id]->chn_cfgs, 0, sizeof(dpu_ctx[dpu_grp_id]->chn_cfgs));

	memset(&dpu_ctx[dpu_grp_id]->grp_work_wtatus, 0, sizeof(dpu_ctx[dpu_grp_id]->grp_work_wtatus));

	memset(&dpu_ctx[dpu_grp_id]->input_job_status, 0, sizeof(dpu_ctx[dpu_grp_id]->input_job_status));
	memset(&dpu_ctx[dpu_grp_id]->working_job_status, 0, sizeof(dpu_ctx[dpu_grp_id]->working_job_status));
	memset(&dpu_ctx[dpu_grp_id]->output_job_status, 0, sizeof(dpu_ctx[dpu_grp_id]->output_job_status));

	mutex_lock(&dpu_get_grp_lock);
	dpu_grp_used[dpu_grp_id] = TRUE;
	mutex_unlock(&dpu_get_grp_lock);

	TRACE_DPU(DBG_INFO, "Grp(%d)  chn_num(%d) iscreated(%d)\n",
		dpu_grp_id, dpu_ctx[dpu_grp_id]->chn_num,dpu_ctx[dpu_grp_id]->iscreated);

	return SUCCESS;
}
//EXPORT_SYMBOL_GPL(dpu_create_grp);

int dpu_destroy_grp(dpu_grp dpu_grp_id)
{
	int ret;
	int i;

	ret = check_dpu_grp_valid(dpu_grp_id);
	if (ret != SUCCESS)
		return ret;

	if (!dpu_ctx[dpu_grp_id])
		return SUCCESS;

	if (dpu_ctx[dpu_grp_id]->iscreated) {
		mutex_lock(&dpu_ctx[dpu_grp_id]->lock);
		dpu_ctx[dpu_grp_id]->iscreated = FALSE;
		for(i=0; i< DPU_PIPE_IN_NUM; ++i)
			base_mod_jobs_exit(&dpu_jobs[dpu_grp_id].ins[i]);
		// if(dpu_ctx[dpu_grp_id]->chfh_blk != VB_INVALID_HANDLE)
		// 	vb_release_block(dpu_ctx[dpu_grp_id]->chfh_blk);
		// TRACE_DPU(DBG_INFO, "chfh_blk(%llx)", dpu_ctx[dpu_grp_id]->chfh_blk);

		mutex_unlock(&dpu_ctx[dpu_grp_id]->lock);
	}
	kfree(dpu_ctx[dpu_grp_id]);
	dpu_ctx[dpu_grp_id] = NULL;

	mutex_lock(&dpu_get_grp_lock);
	dpu_grp_used[dpu_grp_id] = FALSE;
	mutex_unlock(&dpu_get_grp_lock);

	TRACE_DPU(DBG_INFO, "Grp(%d)\n", dpu_grp_id);

	return SUCCESS;
}
//EXPORT_SYMBOL_GPL(dpu_destroy_grp);

dpu_grp dpu_get_available_grp(void){
	dpu_grp grp = 0;
	dpu_grp ret = -1;

	mutex_lock(&dpu_get_grp_lock);
	for(grp = 0; grp < DPU_MAX_GRP_NUM; grp++){
		if(!dpu_grp_used[grp]){
			dpu_grp_used[grp] = TRUE;
			ret = grp;
			break;
		}
	}
	mutex_unlock(&dpu_get_grp_lock);

	if(dpu_ctx[ret]){
		dpu_disable_chn(ret, 0);
		dpu_stop_grp(ret);
		dpu_destroy_grp(ret);
		mutex_lock(&dpu_get_grp_lock);
		dpu_grp_used[ret] = TRUE;
		mutex_unlock(&dpu_get_grp_lock);
	}

	return ret;
}

int dpu_set_grp_attr(dpu_grp dpu_grp_id,const dpu_grp_attr_s *pstGrpAttr)
{
	int ret;

	ret = MOD_CHECK_NULL_PTR(ID_DPU, pstGrpAttr);
	if (ret != SUCCESS)
		return ret;

	ret = check_dpu_grp_valid(dpu_grp_id);
	if (ret != SUCCESS)
		return ret;

	ret = check_dpu_grp_created(dpu_grp_id);
	if (ret != SUCCESS)
		return ret;

	if (pstGrpAttr->frame_rate.src_frame_rate < pstGrpAttr->frame_rate.dst_frame_rate) {
		TRACE_DPU(DBG_WARN, "Grp(%d) frame_rate ctrl, src(%d) < dst(%d), not support\n"
				, dpu_grp_id, pstGrpAttr->frame_rate.src_frame_rate
				, pstGrpAttr->frame_rate.dst_frame_rate);
		return ERR_DPU_ILLEGAL_PARAM;
	}

	mutex_lock(&dpu_ctx[dpu_grp_id]->lock);
	memcpy(&dpu_ctx[dpu_grp_id]->grp_attr,pstGrpAttr,sizeof(dpu_ctx[dpu_grp_id]->grp_attr));
	//dpu_ctx[dpu_grp_id]->grp_attr = *pstGrpAttr;
	if(dpu_ctx[dpu_grp_id]->grp_attr.isbtcostout ){
		dpu_ctx[dpu_grp_id]->chn_num=2;
	}else{
		dpu_ctx[dpu_grp_id]->chn_num=1;
	}
	mutex_unlock(&dpu_ctx[dpu_grp_id]->lock);

	TRACE_DPU(DBG_INFO, "Grp(%d) DpuDev(%d) chn_num(%d) \n",
		dpu_grp_id, dpu_ctx[dpu_grp_id]->dpu_dev_id, dpu_ctx[dpu_grp_id]->chn_num);
	return SUCCESS;
}
//EXPORT_SYMBOL_GPL(dpu_set_grp_attr);

int dpu_get_grp_attr(dpu_grp dpu_grp_id,dpu_grp_attr_s *pstGrpAttr)
{
	int ret;

	ret = MOD_CHECK_NULL_PTR(ID_DPU, pstGrpAttr);
	if (ret != SUCCESS)
		return ret;

	ret = check_dpu_grp_valid(dpu_grp_id);
	if (ret != SUCCESS)
		return ret;

	ret = check_dpu_grp_created(dpu_grp_id);
	if (ret != SUCCESS)
		return ret;

	mutex_lock(&dpu_ctx[dpu_grp_id]->lock);
	*pstGrpAttr = dpu_ctx[dpu_grp_id]->grp_attr;
	mutex_unlock(&dpu_ctx[dpu_grp_id]->lock);

	return SUCCESS;
}
//EXPORT_SYMBOL_GPL(dpu_get_grp_attr);

int dpu_start_grp(dpu_grp dpu_grp_id)
{
	unsigned char dpu_dev_id;
	int ret;

	ret = check_dpu_grp_valid(dpu_grp_id);
	if (ret != SUCCESS)
		return ret;

	ret = check_dpu_grp_created(dpu_grp_id);
	if (ret != SUCCESS)
		return ret;

	// if (dpu_ctx[dpu_grp_id]->isstarted) {
	// 	TRACE_DPU(DBG_ERR, "Grp(%d) already started.\n", dpu_grp_id);
	// 	return SUCCESS;
	// }

	dpu_dev_id=dpu_ctx[dpu_grp_id]->dpu_dev_id;
	mutex_lock(&dpu_ctx[dpu_grp_id]->lock);
	dpu_ctx[dpu_grp_id]->isstarted = TRUE;
	dpu_ctx[dpu_grp_id]->grp_state = GRP_STATE_IDLE;

	/* Only change state from stop to run */
	mutex_lock(&handler_ctx[dpu_dev_id].mutex);
	if (handler_ctx[dpu_dev_id].hdl_state == HANDLER_STATE_STOP)
		handler_ctx[dpu_dev_id].hdl_state = HANDLER_STATE_RUN;

	mutex_unlock(&handler_ctx[dpu_dev_id].mutex);
	mutex_unlock(&dpu_ctx[dpu_grp_id]->lock);

	TRACE_DPU(DBG_INFO, "Grp(%d) isStart(%d)\n", dpu_grp_id,dpu_ctx[dpu_grp_id]->isstarted);

	return SUCCESS;
}
//EXPORT_SYMBOL_GPL(dpu_start_grp);

int dpu_stop_grp(dpu_grp dpu_grp_id)
{
	int ret;
	unsigned char dpu_dev_id;
	unsigned char enabled;
	enum handler_state state;

	ret = check_dpu_grp_valid(dpu_grp_id);
	if (ret != SUCCESS)
		return ret;

	if (!dpu_ctx[dpu_grp_id])
		return SUCCESS;
	if (!dpu_ctx[dpu_grp_id]->isstarted)
		return SUCCESS;

	dpu_dev_id = dpu_ctx[dpu_grp_id]->dpu_dev_id;

	mutex_lock(&dpu_ctx[dpu_grp_id]->lock);
	if(handler_ctx[dpu_dev_id].working_grp == dpu_grp_id)
		dpu_reset();
	dpu_ctx[dpu_grp_id]->isstarted = FALSE;

	dpu_ctx[dpu_grp_id]->grp_state = GRP_STATE_IDLE;

	/* Only change state from run to stop */
	mutex_lock(&handler_ctx[dpu_dev_id].mutex);
	enabled = dpu_enable_handler_ctx(&handler_ctx[dpu_dev_id]);
	state = handler_ctx[dpu_dev_id].hdl_state;
	TRACE_DPU(DBG_INFO, "dpu_enable_handler_ctx(%d)\n", enabled);
	TRACE_DPU(DBG_INFO, "handler_ctx.hdl_state (%d)\n", state);
	if (!enabled && state == HANDLER_STATE_RUN) {
		handler_ctx[dpu_dev_id].hdl_state = HANDLER_STATE_STOP;
		handler_ctx[dpu_dev_id].working_grp = DPU_MAX_GRP_NUM;
		handler_ctx[dpu_dev_id].working_mask = 0;
		handler_ctx[dpu_dev_id].events = 0;

		dpu_dev->bbusy = FALSE;
	}
	mutex_unlock(&handler_ctx[dpu_dev_id].mutex);
	mutex_unlock(&dpu_ctx[dpu_grp_id]->lock);
	TRACE_DPU(DBG_INFO, "dpu_dev->bbusy(%d)\n", dpu_dev->bbusy);
	TRACE_DPU(DBG_INFO, "Grp(%d)\n", dpu_grp_id);

	return SUCCESS;
}
//EXPORT_SYMBOL_GPL(dpu_stop_grp);

int dpu_set_chn_attr(dpu_grp dpu_grp_id,dpu_chn  dpu_chn_id,const dpu_chn_attr_s *pstChnAttr)
{
	vb_cal_config_s vb_config_out;
	vb_cal_config_s vb_config_btcost;
	int ret;

	ret = MOD_CHECK_NULL_PTR(ID_DPU, pstChnAttr);
	if (ret != SUCCESS)
		return ret;

	ret = check_dpu_grp_valid(dpu_grp_id);
	if (ret != SUCCESS)
		return ret;

	ret = check_dpu_grp_created(dpu_grp_id);
	if (ret != SUCCESS)
		return ret;

	ret = check_dpu_chn_valid(dpu_grp_id, dpu_chn_id);
	if (ret != SUCCESS)
		return ret;


	mutex_lock(&dpu_ctx[dpu_grp_id]->lock);

	if(dpu_chn_id == 0){
		if(dpu_ctx[dpu_grp_id]->grp_attr.dpu_mode == DPU_MODE_DEFAULT ||
		dpu_ctx[dpu_grp_id]->grp_attr.dpu_mode == DPU_MODE_SGBM_MUX0 ||
		dpu_ctx[dpu_grp_id]->grp_attr.dpu_mode == DPU_MODE_SGBM_MUX2 ) {
			common_getpicbufferconfig(pstChnAttr->img_size.width, pstChnAttr->img_size.height, \
			PIXEL_FORMAT_YUV_PLANAR_420, DATA_BITWIDTH_8, COMPRESS_MODE_NONE, ALIGN_16, &vb_config_out);
			dpu_ctx[dpu_grp_id]->chn_cfgs[dpu_chn_id].pixel_format = PIXEL_FORMAT_YUV_PLANAR_420;
			dpu_ctx[dpu_grp_id]->chn_cfgs[0].align = ALIGN_16;
			dpu_ctx[dpu_grp_id]->chn_cfgs[dpu_chn_id].blk_size = vb_config_out.vb_size;
			dpu_ctx[dpu_grp_id]->chn_cfgs[dpu_chn_id].stride=vb_config_out.main_stride;

		}else if(dpu_ctx[dpu_grp_id]->grp_attr.dpu_mode == DPU_MODE_FGS_MUX0 ||
				dpu_ctx[dpu_grp_id]->grp_attr.dpu_mode == DPU_MODE_SGBM_FGS_ONLINE_MUX0){
					common_getpicbufferconfig(pstChnAttr->img_size.width, pstChnAttr->img_size.height, \
					PIXEL_FORMAT_YUV_PLANAR_420, DATA_BITWIDTH_8, COMPRESS_MODE_NONE, ALIGN_32, &vb_config_out);
					dpu_ctx[dpu_grp_id]->chn_cfgs[dpu_chn_id].pixel_format = PIXEL_FORMAT_YUV_PLANAR_420;
					dpu_ctx[dpu_grp_id]->chn_cfgs[0].align = ALIGN_32;
					dpu_ctx[dpu_grp_id]->chn_cfgs[dpu_chn_id].blk_size = vb_config_out.vb_size;
					dpu_ctx[dpu_grp_id]->chn_cfgs[dpu_chn_id].stride=vb_config_out.main_stride;
		} else if(dpu_ctx[dpu_grp_id]->grp_attr.dpu_mode == DPU_MODE_SGBM_MUX1 ){
			common_getpicbufferconfig(pstChnAttr->img_size.width, pstChnAttr->img_size.height, \
			PIXEL_FORMAT_YUV_400, DATA_BITWIDTH_8
			, COMPRESS_MODE_NONE, ALIGN_16, &vb_config_out);
			dpu_ctx[dpu_grp_id]->chn_cfgs[dpu_chn_id].pixel_format = PIXEL_FORMAT_YUV_400;
			dpu_ctx[dpu_grp_id]->chn_cfgs[0].align = ALIGN_16;
			dpu_ctx[dpu_grp_id]->chn_cfgs[dpu_chn_id].blk_size = vb_config_out.vb_size*2;
			dpu_ctx[dpu_grp_id]->chn_cfgs[dpu_chn_id].stride=vb_config_out.main_stride*2;

		} else if(dpu_ctx[dpu_grp_id]->grp_attr.dpu_mode == DPU_MODE_SGBM_FGS_ONLINE_MUX1 ||
			dpu_ctx[dpu_grp_id]->grp_attr.dpu_mode == DPU_MODE_SGBM_FGS_ONLINE_MUX2 ||
			dpu_ctx[dpu_grp_id]->grp_attr.dpu_mode == DPU_MODE_FGS_MUX1) {
			common_getpicbufferconfig(pstChnAttr->img_size.width, pstChnAttr->img_size.height, \
			PIXEL_FORMAT_YUV_400, DATA_BITWIDTH_8, COMPRESS_MODE_NONE, ALIGN_32, &vb_config_out);
			dpu_ctx[dpu_grp_id]->chn_cfgs[dpu_chn_id].pixel_format = PIXEL_FORMAT_YUV_400;
			dpu_ctx[dpu_grp_id]->chn_cfgs[0].align = ALIGN_32;
			dpu_ctx[dpu_grp_id]->chn_cfgs[dpu_chn_id].blk_size = vb_config_out.vb_size*2;
			dpu_ctx[dpu_grp_id]->chn_cfgs[dpu_chn_id].stride=vb_config_out.main_stride*2;
		}else {
		TRACE_DPU(DBG_ERR,"[%s] INVALID DPU MODE!\n",__func__);
		return FAILURE;
		}

		memcpy(&dpu_ctx[dpu_grp_id]->chn_cfgs[0].chn_attr, pstChnAttr,
		sizeof(dpu_ctx[dpu_grp_id]->chn_cfgs[0].chn_attr));

		// dpu_ctx[dpu_grp_id]->chn_cfgs[dpu_chn_id].pixel_format = PIXEL_FORMAT_YUV_400;
		dpu_ctx[dpu_grp_id]->chn_cfgs[dpu_chn_id].is_cfg_changed = TRUE;
		dpu_ctx[dpu_grp_id]->chn_cfgs[dpu_chn_id].vb_pool=VB_INVALID_POOLID;
		memcpy(&dpu_ctx[dpu_grp_id]->chn_cfgs[0].vb_config,&vb_config_out,sizeof(vb_cal_config_s));
	}else if(dpu_chn_id == 1){
		common_getpicbufferconfig(pstChnAttr->img_size.width * 128, pstChnAttr->img_size.height,
			PIXEL_FORMAT_YUV_400, DATA_BITWIDTH_16
			, COMPRESS_MODE_NONE, ALIGN_16, &vb_config_btcost);
		memcpy(&dpu_ctx[dpu_grp_id]->chn_cfgs[1].chn_attr, pstChnAttr,
		sizeof(dpu_ctx[dpu_grp_id]->chn_cfgs[1].chn_attr));
		dpu_ctx[dpu_grp_id]->chn_cfgs[dpu_chn_id].blk_size = vb_config_btcost.vb_size;

		dpu_ctx[dpu_grp_id]->chn_cfgs[dpu_chn_id].pixel_format = PIXEL_FORMAT_YUV_400;
		dpu_ctx[dpu_grp_id]->chn_cfgs[dpu_chn_id].stride=vb_config_btcost.main_stride;
		dpu_ctx[dpu_grp_id]->chn_cfgs[dpu_chn_id].is_cfg_changed = TRUE;
		dpu_ctx[dpu_grp_id]->chn_cfgs[dpu_chn_id].vb_pool = VB_INVALID_POOLID;
		memcpy(&dpu_ctx[dpu_grp_id]->chn_cfgs[1].vb_config,&vb_config_btcost,sizeof(vb_cal_config_s));
	}

	mutex_unlock(&dpu_ctx[dpu_grp_id]->lock);

	TRACE_DPU(DBG_INFO, "Grp(%d) Chn(%d) width(%d), height(%d)\n"
		, dpu_grp_id, dpu_chn_id, pstChnAttr->img_size.width, pstChnAttr->img_size.height);

	return SUCCESS;
}
//EXPORT_SYMBOL_GPL(dpu_set_chn_attr);

int dpu_get_chn_attr(dpu_grp dpu_grp_id,dpu_chn dpu_chn_id,dpu_chn_attr_s *pstChnAttr)
{
	int ret;

	ret = MOD_CHECK_NULL_PTR(ID_DPU, pstChnAttr);
	if (ret != SUCCESS)
		return ret;

	ret = check_dpu_grp_valid(dpu_grp_id);
	if (ret != SUCCESS)
		return ret;

	ret = check_dpu_grp_created(dpu_grp_id);
	if (ret != SUCCESS)
		return ret;

	ret = check_dpu_chn_valid(dpu_grp_id, dpu_chn_id);
	if (ret != SUCCESS)
		return ret;

	memcpy(pstChnAttr, &dpu_ctx[dpu_grp_id]->chn_cfgs[dpu_chn_id].chn_attr, sizeof(*pstChnAttr));
	return SUCCESS;
}
//EXPORT_SYMBOL_GPL(dpu_get_chn_attr);

int dpu_enable_chn(dpu_grp dpu_grp_id,dpu_chn dpu_chn_id)
{
	struct dpu_chn_cfg_s *chn_cfg;
	int ret;
	unsigned char dpu_dev_id;

	ret = check_dpu_grp_valid(dpu_grp_id);
	if (ret != SUCCESS)
		return ret;

	ret = check_dpu_grp_created(dpu_grp_id);
	if (ret != SUCCESS)
		return ret;

	ret = check_dpu_chn_valid(dpu_grp_id, dpu_chn_id);
	if (ret != SUCCESS)
		return ret;

	chn_cfg = &dpu_ctx[dpu_grp_id]->chn_cfgs[dpu_chn_id];
	if (chn_cfg->isenabled) {
		TRACE_DPU(DBG_INFO, "Grp(%d) Chn(%d) already enabled\n", dpu_grp_id, dpu_chn_id);
		return SUCCESS;
	}
	dpu_dev_id = dpu_ctx[dpu_grp_id]->dpu_dev_id;
	mutex_lock(&dpu_ctx[dpu_grp_id]->lock);
	base_mod_jobs_init(&dpu_jobs[dpu_grp_id].outs[dpu_chn_id], DPU_WAITQ_DEPTH_OUT,  DPU_WORKQ_DEPTH_OUT,  DPU_DONEQ_DEPTH_OUT);

	chn_cfg->isenabled = TRUE;
	mutex_unlock(&dpu_ctx[dpu_grp_id]->lock);
	TRACE_DPU(DBG_INFO, "Grp(%d) Chn(%d)\n", dpu_grp_id, dpu_chn_id);

	return SUCCESS;
}
//EXPORT_SYMBOL_GPL(dpu_enable_chn);

int dpu_disable_chn(dpu_grp dpu_grp_id,dpu_chn dpu_chn_id)
{
	int ret;

	ret = check_dpu_grp_valid(dpu_grp_id);
	if (ret != SUCCESS)
		return ret;

	ret = check_dpu_grp_created(dpu_grp_id);
	if (ret != SUCCESS)
		return ret;

	ret = check_dpu_chn_valid(dpu_grp_id, dpu_chn_id);
	if (ret != SUCCESS)
		return ret;

	if (!dpu_ctx[dpu_grp_id]->iscreated) {
		TRACE_DPU(DBG_INFO, "Grp(%d) not created yet\n", dpu_grp_id);
		return SUCCESS;
	}
	if (!dpu_ctx[dpu_grp_id]->chn_cfgs[dpu_chn_id].isenabled) {
		TRACE_DPU(DBG_INFO, "Grp(%d) Chn(%d) not enabled yet\n", dpu_grp_id, dpu_chn_id);
		return SUCCESS;
	}

	mutex_lock(&dpu_ctx[dpu_grp_id]->lock);
	dpu_ctx[dpu_grp_id]->chn_cfgs[dpu_chn_id].isenabled = FALSE;

	base_mod_jobs_exit(&dpu_jobs[dpu_grp_id].outs[dpu_chn_id]);

	mutex_unlock(&dpu_ctx[dpu_grp_id]->lock);
	TRACE_DPU(DBG_INFO, "Grp(%d) Chn(%d)\n", dpu_grp_id, dpu_chn_id);

	return SUCCESS;
}
//EXPORT_SYMBOL_GPL(dpu_disable_chn);

int dpu_send_frame(dpu_grp dpu_grp_id,\
                                const video_frame_info_s *pstLeftFrame,\
                                const video_frame_info_s *pstRightFrame,\
                                int s32_millisec)
{

	mmf_chn_s chn_left = {.mod_id = ID_DPU, .dev_id = dpu_grp_id, .chn_id = 0};
	mmf_chn_s chn_right = {.mod_id = ID_DPU, .dev_id = dpu_grp_id, .chn_id = 1};
	vb_blk blk_right;
	vb_blk blk_left;
	int ret;
	struct vb_s *vb_l;
	struct vb_s *vb_r;
	// ret = wait_event_interruptible_timeout(dpu_dev->send_frame_wait,
	// 		!dpu_dev->bbusy ,msecs_to_jiffies(3000));
	TRACE_DPU(DBG_INFO, " dpu_send_frame          +\n");
	ret = MOD_CHECK_NULL_PTR(ID_DPU, pstLeftFrame);
	if (ret != SUCCESS){
		TRACE_DPU(DBG_ERR, "left Frame null ptr.\n");
		return ret;
	}

	ret = MOD_CHECK_NULL_PTR(ID_DPU, pstRightFrame);
	if (ret != SUCCESS){
		TRACE_DPU(DBG_ERR, "right Frame null ptr.\n");
		return ret;
	}

	ret = check_dpu_grp_valid(dpu_grp_id);
	if (ret != SUCCESS)
		return ret;

	ret = check_dpu_grp_created(dpu_grp_id);
	if (ret != SUCCESS)
		return ret;

	if (!DPU_GRP_SUPPORT_FMT(pstLeftFrame->video_frame.pixel_format)) {
		TRACE_DPU(DBG_ERR, "Grp(%d) leftFrame PixelFormat(%d) mismatch.\n"
			, dpu_grp_id, pstLeftFrame->video_frame.pixel_format);
		return ERR_DPU_ILLEGAL_PARAM;
	}
	if (!DPU_GRP_SUPPORT_FMT(pstRightFrame->video_frame.pixel_format)) {
		TRACE_DPU(DBG_ERR, "Grp(%d) RightFrame PixelFormat(%d) mismatch.\n"
			, dpu_grp_id, pstRightFrame->video_frame.pixel_format);
		return ERR_DPU_ILLEGAL_PARAM;
	}
	if ((dpu_ctx[dpu_grp_id]->grp_attr.left_image_size.width  != pstLeftFrame->video_frame.width)
	 || (dpu_ctx[dpu_grp_id]->grp_attr.left_image_size.height  != pstLeftFrame->video_frame.height)) {
		TRACE_DPU(DBG_ERR, "Grp(%d) leftFrame Size(%d * %d) mismatch.\n"
			, dpu_grp_id, pstLeftFrame->video_frame.width, pstLeftFrame->video_frame.height);
		return ERR_DPU_ILLEGAL_PARAM;
	}
	if ((dpu_ctx[dpu_grp_id]->grp_attr.right_image_size.width  != pstRightFrame->video_frame.width)
	 || (dpu_ctx[dpu_grp_id]->grp_attr.right_image_size.height  != pstRightFrame->video_frame.height)) {
		TRACE_DPU(DBG_ERR, "Grp(%d) rightFrame Size(%d * %d) mismatch.\n"
			, dpu_grp_id, pstRightFrame->video_frame.width, pstRightFrame->video_frame.height);
		return ERR_DPU_ILLEGAL_PARAM;
	}
	if (IS_FRAME_OFFSET_INVALID(pstLeftFrame->video_frame)) {
		TRACE_DPU(DBG_ERR, "Grp(%d) Leftframe offset (%d %d %d %d) invalid\n",
			dpu_grp_id, pstLeftFrame->video_frame.offset_left, pstLeftFrame->video_frame.offset_right,
			pstLeftFrame->video_frame.offset_top, pstLeftFrame->video_frame.offset_bottom);
		return ERR_DPU_ILLEGAL_PARAM;
	}
	if (IS_FRAME_OFFSET_INVALID(pstRightFrame->video_frame)) {
		TRACE_DPU(DBG_ERR, "Grp(%d) Rightframe offset (%d %d %d %d) invalid\n",
			dpu_grp_id, pstRightFrame->video_frame.offset_left, pstRightFrame->video_frame.offset_right,
			pstRightFrame->video_frame.offset_top, pstRightFrame->video_frame.offset_bottom);
		return ERR_DPU_ILLEGAL_PARAM;
	}
	// mutex_lock(&dpu_jobs[dpu_grp_id].lock);
	blk_left = vb_phys_addr2handle(pstLeftFrame->video_frame.phyaddr[0]);
	if (blk_left == VB_INVALID_HANDLE) {
		// TRACE_DPU(DBG_WARN, " blk_left VB_INVALID_HANDLE Grp(%d) .\n", dpu_grp_id);
		blk_left = vb_create_block(pstLeftFrame->video_frame.phyaddr[0], NULL, VB_EXTERNAL_POOLID, TRUE);
		if (blk_left == VB_INVALID_HANDLE) {
			TRACE_DPU(DBG_ERR, "left frame Grp(%d) no space for malloc.\n", dpu_grp_id);
			return ERR_DPU_NOMEM;
		}
	}

	blk_right = vb_phys_addr2handle(pstRightFrame->video_frame.phyaddr[0]);
	if (blk_right == VB_INVALID_HANDLE) {
		// TRACE_DPU(DBG_WARN, " blk_left VB_INVALID_HANDLE Grp(%d) .\n", dpu_grp_id);
		blk_right = vb_create_block(pstRightFrame->video_frame.phyaddr[0], NULL, VB_EXTERNAL_POOLID, TRUE);
		if (blk_right == VB_INVALID_HANDLE) {
			TRACE_DPU(DBG_ERR, "Right frame Grp(%d) no space for malloc.\n", dpu_grp_id);
			return ERR_DPU_NOMEM;
		}
	}

	if (dpu_fill_videoframe2buffer(chn_left, pstLeftFrame, &((struct vb_s *)blk_left)->buf) != SUCCESS) {
		TRACE_DPU(DBG_ERR, "Left frame Grp(%d) Invalid parameter\n", dpu_grp_id);
		return ERR_DPU_ILLEGAL_PARAM;
	}

	if (dpu_fill_videoframe2buffer(chn_right, pstRightFrame, &((struct vb_s *)blk_right)->buf) != SUCCESS) {
		TRACE_DPU(DBG_ERR, "Right frame Grp(%d) Invalid parameter\n", dpu_grp_id);
		return ERR_DPU_ILLEGAL_PARAM;
	}

	if (_vb_qbuf(chn_left, CHN_TYPE_IN, &dpu_jobs[dpu_grp_id].ins[0], blk_left) != SUCCESS) {
		TRACE_DPU(DBG_ERR, "Left frame Grp(%d) qbuf failed\n", dpu_grp_id);
		return ERR_DPU_BUSY;
	}
	vb_l =(struct vb_s *)blk_left;
	TRACE_DPU(DBG_INFO, "vb_l cnt(%d) \n",vb_l->usr_cnt.counter);
	vb_release_block(blk_left);
	TRACE_DPU(DBG_INFO, "vb_l cnt(%d) \n",vb_l->usr_cnt.counter);

	if (_vb_qbuf(chn_right, CHN_TYPE_IN, &dpu_jobs[dpu_grp_id].ins[1], blk_right) != SUCCESS) {
		TRACE_DPU(DBG_ERR, "Right frame Grp(%d) qbuf failed\n", dpu_grp_id);
		if(!base_mod_jobs_waitq_empty(&dpu_jobs[dpu_grp_id].ins[0])){
			base_mod_jobs_waitq_pop(&dpu_jobs[dpu_grp_id].ins[0]);
			if(!down_trylock(&dpu_jobs[dpu_grp_id].ins[0].sem)){
				TRACE_DPU(DBG_ERR, "down try lock sem of left buf\n");
			}
		}
		return ERR_DPU_BUSY;
	}
	print_vbq_size(dpu_grp_id);
	vb_r =(struct vb_s *)blk_right;
	TRACE_DPU(DBG_INFO, "vb_r cnt(%d) \n",vb_r->usr_cnt.counter);
	vb_release_block(blk_right);
	// mutex_unlock(&dpu_jobs[dpu_grp_id].lock);
	TRACE_DPU(DBG_INFO, "vb_r cnt(%d) \n",vb_r->usr_cnt.counter);

	TRACE_DPU(DBG_INFO, "Left frame Grp(%d), phy-address(0x%llx)\n",
			dpu_grp_id, pstLeftFrame->video_frame.phyaddr[0]);

	TRACE_DPU(DBG_INFO, "Right frame Grp(%d), phy-address(0x%llx)\n",
			dpu_grp_id, pstRightFrame->video_frame.phyaddr[0]);

	dpu_ctx[dpu_grp_id]->stride_arry[0]= pstLeftFrame->video_frame.stride[0];
	dpu_ctx[dpu_grp_id]->stride_arry[1]= pstRightFrame->video_frame.stride[0];

	//dpu_notify_wkup_evt(handler_ctx[0].dpu_dev_id);
	if(dpu_dev->bbusy == FALSE){
		dpu_notify_wkup_evt(handler_ctx[0].dpu_dev_id);
	}
	TRACE_DPU(DBG_INFO, " dpu_dev->bbusy(%d) \n",dpu_dev->bbusy);
	TRACE_DPU(DBG_INFO, " dpu_send_frame          -\n");
	return SUCCESS;
}
//EXPORT_SYMBOL_GPL(dpu_send_frame);

int dpu_send_chn_frame(dpu_grp dpu_grp_id, dpu_chn dpu_chn_id
	, const video_frame_info_s *vdeo_frame_info, int s32MilliSec)
{
	int ret;
	mmf_chn_s chn = {.mod_id = ID_DPU, .dev_id = dpu_grp_id, .chn_id = dpu_chn_id};
	vb_blk blk;
	struct vb_s *vb;
	struct vb_jobs_t *jobs;

	TRACE_DPU(DBG_INFO, " dpu_send_chn_frame \n");

	ret = MOD_CHECK_NULL_PTR(ID_DPU, vdeo_frame_info);
	if (ret != SUCCESS)
		return ret;

	ret = check_dpu_grp_valid(dpu_grp_id);
	if (ret != SUCCESS)
		return ret;

	ret = check_dpu_grp_created(dpu_grp_id);
	if (ret != SUCCESS)
		return ret;

	ret = check_dpu_chn_valid(dpu_grp_id, dpu_chn_id);
	if (ret != SUCCESS)
		return ret;

	UNUSED(s32MilliSec);

	blk = vb_phys_addr2handle(vdeo_frame_info->video_frame.phyaddr[0]);
	if (blk == VB_INVALID_HANDLE) {
		blk = vb_create_block(vdeo_frame_info->video_frame.phyaddr[0], NULL, VB_EXTERNAL_POOLID, TRUE);
		if (blk == VB_INVALID_HANDLE) {
			TRACE_DPU(DBG_ERR, "dpu_chn_id frame Chn(%d) no space for malloc.\n", dpu_chn_id);
			return ERR_DPU_NOMEM;
		}
	}

	if(base_fill_videoframe2buffer(chn, vdeo_frame_info, &((struct vb_s *)blk)->buf) != SUCCESS){
		TRACE_DPU(DBG_ERR, "Grp(%d) Chn(%d) Invalid parameter\n", dpu_grp_id, dpu_chn_id);
		return ERR_DPU_NOMEM;
	}

	jobs = &dpu_jobs[dpu_grp_id].outs[dpu_chn_id];
	if (!jobs) {
		TRACE_DPU(DBG_ERR, "Grp(%d) Chn(%d) get job failed\n",
				dpu_grp_id, dpu_chn_id);
		return FAILURE;
	}

	vb = (struct vb_s *)blk;
	mutex_lock(&jobs->lock);
	if (FIFO_FULL(&jobs->waitq)) {
		TRACE_DPU(DBG_ERR, "Grp(%d) Chn(%d) waitq is full\n", dpu_grp_id, dpu_chn_id);
		mutex_unlock(&jobs->lock);
		return FAILURE;
	}
	FIFO_PUSH(&jobs->waitq, vb);
	atomic_fetch_add(1, &vb->usr_cnt);
	mutex_unlock(&jobs->lock);

	TRACE_DPU(DBG_DEBUG, "Grp(%d) Chn(%d)\n", dpu_grp_id, dpu_chn_id);
	TRACE_DPU(DBG_INFO, "Output frame Grp(%d), phy-address(0x%llx)\n",
			dpu_grp_id, vdeo_frame_info->video_frame.phyaddr[0]);
	TRACE_DPU(DBG_INFO, " dpu_send_chn_frame \n");
	return ret;
}

int dpu_get_frame(dpu_grp dpu_grp_id,\
						 dpu_chn dpu_chn_id,\
						 video_frame_info_s *vdeo_frame_info,\
						 int millisec)
{
	int ret, i;
	vb_blk blk = VB_INVALID_HANDLE;
	struct vb_s *vb;
	mmf_chn_s chn = {.mod_id = ID_DPU, .dev_id = dpu_grp_id, .chn_id = dpu_chn_id};
	TRACE_DPU(DBG_INFO, " dpu_get_frame          +\n");
	ret = MOD_CHECK_NULL_PTR(ID_DPU, vdeo_frame_info);
	if (ret != SUCCESS)
		return ret;

	ret = check_dpu_grp_valid(dpu_grp_id);
	if (ret != SUCCESS)
		return ret;

	ret = check_dpu_grp_created(dpu_grp_id);
	if (ret != SUCCESS)
		return ret;

	ret = check_dpu_chn_valid(dpu_grp_id, dpu_chn_id);
	if (ret != SUCCESS)
		return ret;

	if (!dpu_ctx[dpu_grp_id]->isstarted) {
		TRACE_DPU(DBG_ERR, "Grp(%d) not yet started.\n", dpu_grp_id);
		return ERR_DPU_NOTREADY;
	}
	if (!dpu_ctx[dpu_grp_id]->chn_cfgs[dpu_chn_id].isenabled) {
		TRACE_DPU(DBG_ERR, "Grp(%d) Chn(%d) not yet enabled.\n", dpu_grp_id, dpu_chn_id);
		return ERR_DPU_NOTREADY;
	}

	memset(vdeo_frame_info, 0, sizeof(*vdeo_frame_info));
	ret = base_get_chn_buffer(chn, &dpu_jobs[dpu_grp_id].outs[dpu_chn_id],&blk,millisec);

	if (ret != 0 || blk == VB_INVALID_HANDLE) {
		TRACE_DPU(DBG_ERR, "Grp(%d) Chn(%d) get chn frame fail, s32MilliSec=%d, ret=%d\n",
		dpu_grp_id, dpu_chn_id, millisec, ret);
		// dpu_check_reg_read();
		// getsgbm_status();
		// getfgs_status();
		return ERR_DPU_BUF_EMPTY;
	}

	vb = (struct vb_s *)blk;
	if (!vb->buf.phy_addr[0] || !vb->buf.size.width) {
		TRACE_DPU(DBG_ERR, "buf already released\n");
		return ERR_DPU_BUF_EMPTY;
	}

	vdeo_frame_info->video_frame.pixel_format = dpu_ctx[dpu_grp_id]->chn_cfgs[dpu_chn_id].pixel_format;
	vdeo_frame_info->video_frame.width = vb->buf.size.width;
	vdeo_frame_info->video_frame.height = vb->buf.size.height;
	vdeo_frame_info->video_frame.time_ref = vb->buf.frm_num;
	vdeo_frame_info->video_frame.pts = vb->buf.pts;
	for (i = 0; i < 3; ++i) {
		if(vb->buf.length[i] ==0 )
			continue;

		vdeo_frame_info->video_frame.phyaddr[i] = vb->buf.phy_addr[i];
		vdeo_frame_info->video_frame.length[i] = vb->buf.length[i];
		vdeo_frame_info->video_frame.stride[i] = vb->buf.stride[i];
	}

	vdeo_frame_info->video_frame.offset_top = vb->buf.offset_top;
	vdeo_frame_info->video_frame.offset_bottom = vb->buf.offset_bottom;
	vdeo_frame_info->video_frame.offset_left = vb->buf.offset_left;
	vdeo_frame_info->video_frame.offset_right = vb->buf.offset_right;
	vdeo_frame_info->video_frame.private_data = vb;

	TRACE_DPU(DBG_INFO, "Grp(%d) Chn(%d) end to set vdeo_frame_info width:%d height:%d buf:0x%llx\n"
			, dpu_grp_id, dpu_chn_id, vdeo_frame_info->video_frame.width, vdeo_frame_info->video_frame.height,
			vdeo_frame_info->video_frame.phyaddr[0]);
	print_vbq_size(dpu_grp_id);
	TRACE_DPU(DBG_INFO, " dpu_get_frame          -\n");
	return SUCCESS;
}

int dpu_release_frame(dpu_grp dpu_grp_id,\
							dpu_chn dpu_chn_id,\
                            const video_frame_info_s *video_frame_info)
{
	vb_blk blk;
	int ret;
	TRACE_DPU(DBG_INFO, " dpu_release_frame          +\n");
	ret = MOD_CHECK_NULL_PTR(ID_DPU, video_frame_info);
	if (ret != SUCCESS)
		return ret;

	ret = check_dpu_grp_valid(dpu_grp_id);
	if (ret != SUCCESS)
		return ret;

	ret = check_dpu_grp_created(dpu_grp_id);
	if (ret != SUCCESS)
		return ret;

	ret = check_dpu_chn_valid(dpu_grp_id, dpu_chn_id);
	if (ret != SUCCESS)
		return ret;

	blk = vb_phys_addr2handle(video_frame_info->video_frame.phyaddr[0]);
	if (blk == VB_INVALID_HANDLE) {
		if (video_frame_info->video_frame.private_data == 0) {
			TRACE_DPU(DBG_ERR, "Grp(%d) Chn(%d) phy-address(0x%llx) invalid to locate.\n"
				      , dpu_grp_id, dpu_chn_id, (unsigned long long)video_frame_info->video_frame.phyaddr[0]);
			return ERR_DPU_ILLEGAL_PARAM;
		}
		blk = (vb_blk)video_frame_info->video_frame.private_data;
	}

	if (vb_release_block(blk) != SUCCESS)
		return FAILURE;

	TRACE_DPU(DBG_INFO, "Grp(%d) Chn(%d) buf:0x%llx\n",
			dpu_grp_id, dpu_chn_id, video_frame_info->video_frame.phyaddr[0]);

	TRACE_DPU(DBG_INFO, " dpu_release_frame          -\n");
	return SUCCESS;
}

void dpu_set_base_addr(void *base)
{
	reg_base = (uintptr_t)base;
}


void dpu_set_base_addr_sgbm_dma(void *base1,void *base2,void *base3,void *base4)
{
	reg_base_sgbm_ld1_dma    = (uintptr_t)base1;
	reg_base_sgbm_ld2_dma    = (uintptr_t)base2;
	reg_base_sgbm_median_dma = (uintptr_t)base3;
	reg_base_sgbm_bf_dma     = (uintptr_t)base4;

}

void dpu_set_base_addr_fgs_dma(void *base1,void *base2,void *base3,void *base4)
{
	reg_base_fgs_gx_dma      = (uintptr_t)base1;
	reg_base_fgs_chfh_ld_dma = (uintptr_t)base2;
	reg_base_fgs_chfh_st_dma = (uintptr_t)base3;
	reg_base_fgs_ux_dma      = (uintptr_t)base4;
}

void dpu_reset(void)
{
	int cnt = 0;
	unsigned int reg_68 = read_reg(reg_base + DPU_REG_68_OFS);
	TRACE_DPU(DBG_INFO, " dpu_reset_done(%d) \n",get_mask(reg_68, 2, 7));
	TRACE_DPU(DBG_INFO, " dpu_reset          +\n");
	reg_write_mask(reg_base + DPU_REG_68_OFS,((1 << 1)-1) << 6,1<< 6);
	while(cnt < 50){
		reg_68 = read_reg(reg_base + DPU_REG_68_OFS);
		if(get_mask(reg_68, 2, 7) ==3)
			break;
		//TRACE_DPU(DBG_INFO, " dpu_reset_done(%d) \n",get_mask(reg_68, 2, 7));
		reg_write_mask(reg_base + DPU_REG_68_OFS,((1 << 1)-1) << 6,1<< 6);
		cnt++;
		udelay(20);
	}
	dpu_dev->hw_busy = FALSE;
	TRACE_DPU(DBG_INFO, " dpu_reset          -\n");
}

void dpu_intr_clr()
{
	TRACE_DPU(DBG_INFO, " dpu_intr_clr          +\n");
	reg_write_mask(reg_base + DPU_REG_68_OFS, ((1 << 1)-1) << 9, 1 << 9);
	TRACE_DPU(DBG_INFO, " dpu_intr_clr          -\n");
}

unsigned char dpu_intr_status(void)
{
	unsigned char intr_status ;
	unsigned int value;
	unsigned int reg_68 = read_reg(reg_base + DPU_REG_68_OFS);
	TRACE_DPU(DBG_INFO, " dpu_intr_status          +\n");
	if(handler_ctx[0].dpu_mode == DPU_MODE_DEFAULT  ||
		handler_ctx[0].dpu_mode == DPU_MODE_SGBM_MUX0 ||
		handler_ctx[0].dpu_mode == DPU_MODE_SGBM_MUX1 ||
		handler_ctx[0].dpu_mode == DPU_MODE_SGBM_MUX2 ){
			value = get_mask(reg_68, 1, 1);
	}else {
		value = get_mask(reg_68, 1, 11);
	}

	if(value ==1)
		intr_status = DPU_INTR_STATE_DONE;
	else
		intr_status = DPU_INTR_STATE_OTHERS;

	dpu_intr_clr();
	TRACE_DPU(DBG_INFO, " dpu_intr_status          -\n");
	return intr_status;
}

void dpu_sel_intr(unsigned int intr_type)
{
	TRACE_DPU(DBG_INFO,"dpu_sel_intr(%d)\n",intr_type);
    reg_write_mask(reg_base+DPU_REG_6C_OFS,((1 << 8)-1) << 24,intr_type<< 24);
}

void dpu_engine(dpu_grp workinggrp_id)
{
	// start dpu
	if(dpu_ctx[workinggrp_id]->grp_attr.dpu_mode == DPU_MODE_DEFAULT  ||
		dpu_ctx[workinggrp_id]->grp_attr.dpu_mode == DPU_MODE_SGBM_MUX0 ||
		dpu_ctx[workinggrp_id]->grp_attr.dpu_mode == DPU_MODE_SGBM_MUX1 ||
		dpu_ctx[workinggrp_id]->grp_attr.dpu_mode == DPU_MODE_SGBM_MUX2  ){
		dpu_sel_intr(1);
		reg_write_mask(reg_base+DPU_REG_68_OFS,((1 << 1)-1) << 0,1 << 0);
		TRACE_DPU(DBG_INFO,"triger sgbm frame start\n");
	}else if(dpu_ctx[workinggrp_id]->grp_attr.dpu_mode == DPU_MODE_SGBM_FGS_ONLINE_MUX0 ||
		dpu_ctx[workinggrp_id]->grp_attr.dpu_mode == DPU_MODE_SGBM_FGS_ONLINE_MUX1 ||
		dpu_ctx[workinggrp_id]->grp_attr.dpu_mode == DPU_MODE_SGBM_FGS_ONLINE_MUX2 ){
		dpu_sel_intr(2);
		reg_write_mask(reg_base+DPU_REG_68_OFS,((1 << 1)-1) << 10,1 << 10);
		reg_write_mask(reg_base+DPU_REG_68_OFS,((1 << 1)-1) << 0,1 << 0);
		TRACE_DPU(DBG_INFO,"triger sgbm frame start\n");
	}else if(dpu_ctx[workinggrp_id]->grp_attr.dpu_mode == DPU_MODE_FGS_MUX0 ||
	dpu_ctx[workinggrp_id]->grp_attr.dpu_mode == DPU_MODE_FGS_MUX1){
		dpu_sel_intr(2);
		reg_write_mask(reg_base+DPU_REG_68_OFS,((1 << 1)-1) << 10,1 << 10);
		TRACE_DPU(DBG_INFO,"triger fgs frame start\n");
	}else{
		TRACE_DPU(DBG_INFO,"unexpected DPU MODE\n");
	}

}

unsigned char dpu_is_busy(void)
{
	return FALSE;
}

struct dpu_ctx_s **dpu_get_shdw_ctx(void)
{
	return dpu_ctx;
}

struct dpu_handler_ctx_s *dpu_get_handler_ctx(void)
{
	return handler_ctx;
}

int dpu_check_param(dpu_grp grp)
{

	unsigned int width;
	unsigned int height;
	unsigned int min_d;
	unsigned int dispRange;
	unsigned int stride_left;
	unsigned int stride_right;
	unsigned int width_left =dpu_ctx[grp]->grp_attr.left_image_size.width;
	unsigned int width_right =dpu_ctx[grp]->grp_attr.right_image_size.width;
	unsigned int height_left =dpu_ctx[grp]->grp_attr.left_image_size.height;
	unsigned int height_right =dpu_ctx[grp]->grp_attr.right_image_size.height;
	TRACE_DPU(DBG_INFO, " dpu_check_param          +\n");
	if(width_left != width_right || height_left != height_right){
		TRACE_DPU(DBG_INFO,"[%s]the size of Left image not the same as Right image!\n ",__func__);
		TRACE_DPU(DBG_INFO,"left width(%d),left height(%d),right width(%d) right height(%d)",width_left,height_left,width_right,height_right);
		return FAILURE;
	}

	width =width_left;
	height =height_left;
	if(width <64 || width > 1920 ){
		TRACE_DPU(DBG_INFO,"[%s]the size of image out of the range!\n ",__func__);
		TRACE_DPU(DBG_INFO,"width(%d),height(%d)",width,height);
		return FAILURE;
	}
	if((width % ALIGN_4) != 0){
		TRACE_DPU(DBG_INFO,"[%s]the width is not 4 ailgn !\n ",__func__);
		TRACE_DPU(DBG_INFO,"width(%d)\n",width);
		return FAILURE;
	}
	stride_left=dpu_ctx[grp]->stride_arry[0];
	stride_right=dpu_ctx[grp]->stride_arry[1];
	if(((stride_left % ALIGN_16) != 0) || ((stride_right % ALIGN_16) != 0)){
		TRACE_DPU(DBG_INFO,"[%s]the stride is not 16 ailgn !\n ",__func__);
		TRACE_DPU(DBG_INFO,"stride left(%d),stride right(%d)",stride_left,stride_right);
		return FAILURE;
	}

	min_d =dpu_ctx[grp]->grp_attr.dispstartpos;
	if(min_d >239){
		TRACE_DPU(DBG_INFO,"[%s]the min_d out of the range !\n ",__func__);
		TRACE_DPU(DBG_INFO,"min_d(%d)",min_d);
		return FAILURE;
	}
	if(dpu_ctx[grp]->grp_attr.disp_range == DPU_DISP_RANGE_DEFAULT ||
		dpu_ctx[grp]->grp_attr.disp_range == DPU_DISP_RANGE_16) {
		dispRange = 16;
	}else if(dpu_ctx[grp]->grp_attr.disp_range == DPU_DISP_RANGE_32){
		dispRange = 32;
	}else if(dpu_ctx[grp]->grp_attr.disp_range == DPU_DISP_RANGE_48){
		dispRange = 48;
	}else if(dpu_ctx[grp]->grp_attr.disp_range == DPU_DISP_RANGE_64){
		dispRange = 64;
	}else if(dpu_ctx[grp]->grp_attr.disp_range == DPU_DISP_RANGE_80){
		dispRange = 80;
	}else if(dpu_ctx[grp]->grp_attr.disp_range == DPU_DISP_RANGE_96){
		dispRange = 96;
	}else if(dpu_ctx[grp]->grp_attr.disp_range == DPU_DISP_RANGE_112){
		dispRange = 112;
	}else if(dpu_ctx[grp]->grp_attr.disp_range == DPU_DISP_RANGE_128){
		dispRange = 128;
	}else{
		TRACE_DPU(DBG_INFO,"[%s]the dispRange enum idex out of the range !\n ",__func__);
		return FAILURE;
	}

	if((min_d +dispRange)>255 || (min_d+dispRange)>(width-48)) {
		TRACE_DPU(DBG_INFO,"[%s]min_d +dispRange out of the range !\n ",__func__);
		TRACE_DPU(DBG_INFO,"min_dt(%d),dispRange(%d),width(%d)",min_d,dispRange,width);
		return FAILURE;
	}

	TRACE_DPU(DBG_INFO, " dpu_check_param          -\n");
	return SUCCESS;
}

int dpu_reg_config(dpu_grp grp)
{
	int ret;
	TRACE_DPU(DBG_INFO, " dpu_reg_config          +\n");
	ret = dpu_check_param(grp);
	if(ret != SUCCESS){
		TRACE_DPU(DBG_INFO,"[%s]dou check param fail !\n ",__func__);
		return FAILURE;
	}
	handler_ctx[0].dpu_mode = dpu_ctx[grp]->grp_attr.dpu_mode;
	dpu_reg.reg_dpu_enable =1;
	dpu_reg.reg_dpu_img_width = dpu_ctx[grp]->grp_attr.left_image_size.width;
	dpu_reg.reg_dpu_img_height = dpu_ctx[grp]->grp_attr.left_image_size.height;
	dpu_reg.reg_dpu_fgs_img_width = dpu_ctx[grp]->grp_attr.left_image_size.width;
	dpu_reg.reg_dpu_fgs_img_height = dpu_ctx[grp]->grp_attr.left_image_size.height;

	if(dpu_ctx[grp]->grp_attr.mask_mode == DPU_MASK_MODE_DEFAULT ||
		dpu_ctx[grp]->grp_attr.mask_mode == DPU_MASK_MODE_7x7){
		dpu_reg.reg_dpu_bfw_size =3;
	} else if(dpu_ctx[grp]->grp_attr.mask_mode == DPU_MASK_MODE_1x1){
		dpu_reg.reg_dpu_bfw_size =0;
	} else if(dpu_ctx[grp]->grp_attr.mask_mode == DPU_MASK_MODE_3x3){
		dpu_reg.reg_dpu_bfw_size =1;
	} else if(dpu_ctx[grp]->grp_attr.mask_mode == DPU_MASK_MODE_5x5){
		dpu_reg.reg_dpu_bfw_size =2;
	} else {
		TRACE_DPU(DBG_INFO,"[%s]the index of mask_mode out of the range !\n ",__func__);
		return FAILURE;
	}

	if(dpu_ctx[grp]->grp_attr.disp_range == DPU_DISP_RANGE_DEFAULT ||
		dpu_ctx[grp]->grp_attr.disp_range == DPU_DISP_RANGE_16) {
		dpu_reg.reg_dpu_disp_range = 0;
	}else if(dpu_ctx[grp]->grp_attr.disp_range == DPU_DISP_RANGE_32){
		dpu_reg.reg_dpu_disp_range = 1;
	}else if(dpu_ctx[grp]->grp_attr.disp_range == DPU_DISP_RANGE_48){
		dpu_reg.reg_dpu_disp_range = 2;
	}else if(dpu_ctx[grp]->grp_attr.disp_range == DPU_DISP_RANGE_64){
		dpu_reg.reg_dpu_disp_range = 3;
	}else if(dpu_ctx[grp]->grp_attr.disp_range == DPU_DISP_RANGE_80){
		dpu_reg.reg_dpu_disp_range = 4;
	}else if(dpu_ctx[grp]->grp_attr.disp_range == DPU_DISP_RANGE_96){
		dpu_reg.reg_dpu_disp_range = 5;
	}else if(dpu_ctx[grp]->grp_attr.disp_range == DPU_DISP_RANGE_112){
		dpu_reg.reg_dpu_disp_range = 6;
	}else if(dpu_ctx[grp]->grp_attr.disp_range == DPU_DISP_RANGE_128){
		dpu_reg.reg_dpu_disp_range = 7;
	}else{
		TRACE_DPU(DBG_INFO,"[%s]the index of disp_range out of the range !\n ",__func__);
		return FAILURE;
	}

	if(dpu_ctx[grp]->grp_attr.dcc_dir == DPU_DCC_DIR_DEFAULT ||
		dpu_ctx[grp]->grp_attr.dcc_dir == DPU_DCC_DIR_A12){
		dpu_reg.reg_dpu_dcc_a234 = 0;
	}else if(dpu_ctx[grp]->grp_attr.dcc_dir == DPU_DCC_DIR_A13) {
		dpu_reg.reg_dpu_dcc_a234 = 1;
	}else if(dpu_ctx[grp]->grp_attr.dcc_dir == DPU_DCC_DIR_A14) {
		dpu_reg.reg_dpu_dcc_a234 = 2;
	}else{
		TRACE_DPU(DBG_INFO,"[%s]the index of dcc_dir out of the range !\n ",__func__);
		return FAILURE;
	}

	dpu_reg.reg_dpu_min_d = dpu_ctx[grp]->grp_attr.dispstartpos;
	dpu_reg.reg_dpu_rshift1 = dpu_ctx[grp]->grp_attr.rshift1;
	dpu_reg.reg_dpu_rshift2 = dpu_ctx[grp]->grp_attr.rshift2;
	dpu_reg.reg_dpu_ca_p1 =  dpu_ctx[grp]->grp_attr.cap1;
	dpu_reg.reg_dpu_ca_p2 =  dpu_ctx[grp]->grp_attr.cap2;
	dpu_reg.reg_dpu_uniq_ratio = dpu_ctx[grp]->grp_attr.uniqratio;
	dpu_reg.reg_dpu_disp_shift = dpu_ctx[grp]->grp_attr.dispshift;
	dpu_reg.reg_dpu_census_shift = dpu_ctx[grp]->grp_attr.censusshift;


	dpu_reg.reg_dpu_fxbaseline = dpu_ctx[grp]->grp_attr.fxbaseline;
	dpu_reg.reg_dpu_fgs_max_count = dpu_ctx[grp]->grp_attr.fgsmaxcount;
	dpu_reg.reg_dpu_fgs_max_t= dpu_ctx[grp]->grp_attr.fgsmaxt;

	if(dpu_ctx[grp]->grp_attr.dpu_depth_unit == DPU_DEPTH_UNIT_DEFAULT ||
		dpu_ctx[grp]->grp_attr.dpu_depth_unit == DPU_DEPTH_UNIT_MM){
			dpu_reg.reg_dpu_fgs_output_unit_choose = 0;
	}else if(dpu_ctx[grp]->grp_attr.dpu_depth_unit == DPU_DEPTH_UNIT_CM){
		dpu_reg.reg_dpu_fgs_output_unit_choose = 1;
	}else if(dpu_ctx[grp]->grp_attr.dpu_depth_unit == DPU_DEPTH_UNIT_DM){
		dpu_reg.reg_dpu_fgs_output_unit_choose = 2;
	}else if(dpu_ctx[grp]->grp_attr.dpu_depth_unit == DPU_DEPTH_UNIT_M){
		dpu_reg.reg_dpu_fgs_output_unit_choose = 3;
	}else{
		TRACE_DPU(DBG_INFO,"[%s]the index of dpu_depth_unit out of the range !\n ",__func__);
		return FAILURE;
	}

	if(dpu_ctx[grp]->grp_attr.dpu_mode == DPU_MODE_DEFAULT ||
		dpu_ctx[grp]->grp_attr.dpu_mode == DPU_MODE_SGBM_MUX0){
		dpu_reg.reg_sgbm2fgs_online = 0;
		dpu_reg.reg_dpu_sgbm_enable =1;
		dpu_reg.reg_dpu_fgs_enable =0;

		dpu_reg.reg_dpu_data_sel = 0;

		dpu_reg.reg_sgbm_ld1_dma_enable =1;
		dpu_reg.reg_sgbm_ld2_dma_enable =1;
		dpu_reg.reg_sgbm_mux_st_dma_enable =1;
		dpu_reg.reg_sgbm_bf_st_dma_enable =0;

		dpu_reg.reg_dma_enable_fgs1 =0;
		dpu_reg.reg_dma_enable_fgs2 =0;
		dpu_reg.reg_dma_enable_fgs3 =0;
		dpu_reg.reg_dma_enable_fgs4 =0;

		dpu_reg.reg_sgbm_ld1_crop_enable =1;
		dpu_reg.reg_sgbm_ld2_crop_enable =1;
		dpu_reg.reg_sgbm_mux_st_crop_enable =1;
		dpu_reg.reg_sgbm_bf_st_crop_enable =0;

		dpu_reg.reg_crop_enable_fgs_gx = 0;
		dpu_reg.reg_crop_enable_fgs_chfh =0;
		dpu_reg.reg_crop_enable_fgs_independent =0;
		dpu_reg.reg_crop_enable_fgs_ux =0;

		dpu_reg.reg_dpu_fgs_output_bit_choose =0;
		dpu_reg.reg_dpu_src_disp_mux =0;
	}else if(dpu_ctx[grp]->grp_attr.dpu_mode == DPU_MODE_SGBM_MUX1){
		dpu_reg.reg_sgbm2fgs_online = 0;
		dpu_reg.reg_dpu_sgbm_enable =1;
		dpu_reg.reg_dpu_fgs_enable =0;

		dpu_reg.reg_dpu_data_sel = 1;

		dpu_reg.reg_sgbm_ld1_dma_enable =1;
		dpu_reg.reg_sgbm_ld2_dma_enable =1;
		dpu_reg.reg_sgbm_mux_st_dma_enable =1;
		dpu_reg.reg_sgbm_bf_st_dma_enable =0;

		dpu_reg.reg_dma_enable_fgs1 =0;
		dpu_reg.reg_dma_enable_fgs2 =0;
		dpu_reg.reg_dma_enable_fgs3 =0;
		dpu_reg.reg_dma_enable_fgs4 =0;

		dpu_reg.reg_sgbm_ld1_crop_enable =1;
		dpu_reg.reg_sgbm_ld2_crop_enable =1;
		dpu_reg.reg_sgbm_mux_st_crop_enable =1;
		dpu_reg.reg_sgbm_bf_st_crop_enable =0;

		dpu_reg.reg_crop_enable_fgs_gx = 0;
		dpu_reg.reg_crop_enable_fgs_chfh =0;
		dpu_reg.reg_crop_enable_fgs_independent =0;
		dpu_reg.reg_crop_enable_fgs_ux =0;

		dpu_reg.reg_dpu_fgs_output_bit_choose =0;
		dpu_reg.reg_dpu_src_disp_mux =0;
	}else if(dpu_ctx[grp]->grp_attr.dpu_mode == DPU_MODE_SGBM_MUX2){
		dpu_reg.reg_sgbm2fgs_online = 0;
		dpu_reg.reg_dpu_sgbm_enable =1;
		dpu_reg.reg_dpu_fgs_enable =0;

		dpu_reg.reg_dpu_data_sel = 2;

		dpu_reg.reg_sgbm_ld1_dma_enable =1;
		dpu_reg.reg_sgbm_ld2_dma_enable =1;
		dpu_reg.reg_sgbm_mux_st_dma_enable =1;
		dpu_reg.reg_sgbm_bf_st_dma_enable =0;

		dpu_reg.reg_dma_enable_fgs1 =0;
		dpu_reg.reg_dma_enable_fgs2 =0;
		dpu_reg.reg_dma_enable_fgs3 =0;
		dpu_reg.reg_dma_enable_fgs4 =0;

		dpu_reg.reg_sgbm_ld1_crop_enable =1;
		dpu_reg.reg_sgbm_ld2_crop_enable =1;
		dpu_reg.reg_sgbm_mux_st_crop_enable =1;
		dpu_reg.reg_sgbm_bf_st_crop_enable =0;

		dpu_reg.reg_crop_enable_fgs_gx = 0;
		dpu_reg.reg_crop_enable_fgs_chfh =0;
		dpu_reg.reg_crop_enable_fgs_independent =0;
		dpu_reg.reg_crop_enable_fgs_ux =0;

		dpu_reg.reg_dpu_fgs_output_bit_choose =0;
		dpu_reg.reg_dpu_src_disp_mux =0;
	}else if(dpu_ctx[grp]->grp_attr.dpu_mode == DPU_MODE_SGBM_FGS_ONLINE_MUX0){
		dpu_reg.reg_sgbm2fgs_online = 1;
		dpu_reg.reg_dpu_sgbm_enable =1;
		dpu_reg.reg_dpu_fgs_enable =1;

		dpu_reg.reg_dpu_data_sel = 2;

		dpu_reg.reg_sgbm_ld1_dma_enable =1;
		dpu_reg.reg_sgbm_ld2_dma_enable =1;
		dpu_reg.reg_sgbm_mux_st_dma_enable =0;
		dpu_reg.reg_sgbm_bf_st_dma_enable =0;

		dpu_reg.reg_dma_enable_fgs1 =1;
		dpu_reg.reg_dma_enable_fgs2 =1;
		dpu_reg.reg_dma_enable_fgs3 =1;
		dpu_reg.reg_dma_enable_fgs4 =1;

		dpu_reg.reg_sgbm_ld1_crop_enable =1;
		dpu_reg.reg_sgbm_ld2_crop_enable =1;
		dpu_reg.reg_sgbm_mux_st_crop_enable =0;
		dpu_reg.reg_sgbm_bf_st_crop_enable =0;

		dpu_reg.reg_crop_enable_fgs_gx = 0;
		dpu_reg.reg_crop_enable_fgs_chfh =1;
		dpu_reg.reg_crop_enable_fgs_independent =1;
		dpu_reg.reg_crop_enable_fgs_ux =1;

		dpu_reg.reg_dpu_fgs_output_bit_choose =0;
		dpu_reg.reg_dpu_src_disp_mux =0;
	}else if(dpu_ctx[grp]->grp_attr.dpu_mode == DPU_MODE_SGBM_FGS_ONLINE_MUX1){
		dpu_reg.reg_sgbm2fgs_online = 1;
		dpu_reg.reg_dpu_sgbm_enable =1;
		dpu_reg.reg_dpu_fgs_enable =1;

		dpu_reg.reg_dpu_data_sel = 2;

		dpu_reg.reg_sgbm_ld1_dma_enable =1;
		dpu_reg.reg_sgbm_ld2_dma_enable =1;
		dpu_reg.reg_sgbm_mux_st_dma_enable =0;
		dpu_reg.reg_sgbm_bf_st_dma_enable =0;

		dpu_reg.reg_dma_enable_fgs1 =1;
		dpu_reg.reg_dma_enable_fgs2 =1;
		dpu_reg.reg_dma_enable_fgs3 =1;
		dpu_reg.reg_dma_enable_fgs4 =1;

		dpu_reg.reg_sgbm_ld1_crop_enable =1;
		dpu_reg.reg_sgbm_ld2_crop_enable =1;
		dpu_reg.reg_sgbm_mux_st_crop_enable =0;
		dpu_reg.reg_sgbm_bf_st_crop_enable =0;

		dpu_reg.reg_crop_enable_fgs_gx = 0;
		dpu_reg.reg_crop_enable_fgs_chfh =1;
		dpu_reg.reg_crop_enable_fgs_independent =1;
		dpu_reg.reg_crop_enable_fgs_ux =1;

		dpu_reg.reg_dpu_fgs_output_bit_choose =1;
		dpu_reg.reg_dpu_src_disp_mux =0;
	}else if(dpu_ctx[grp]->grp_attr.dpu_mode == DPU_MODE_SGBM_FGS_ONLINE_MUX2){
		dpu_reg.reg_sgbm2fgs_online = 1;
		dpu_reg.reg_dpu_sgbm_enable =1;
		dpu_reg.reg_dpu_fgs_enable =1;

		dpu_reg.reg_dpu_data_sel = 2;

		dpu_reg.reg_sgbm_ld1_dma_enable =1;
		dpu_reg.reg_sgbm_ld2_dma_enable =1;
		dpu_reg.reg_sgbm_mux_st_dma_enable =0;
		dpu_reg.reg_sgbm_bf_st_dma_enable =0;

		dpu_reg.reg_dma_enable_fgs1 =1;
		dpu_reg.reg_dma_enable_fgs2 =1;
		dpu_reg.reg_dma_enable_fgs3 =1;
		dpu_reg.reg_dma_enable_fgs4 =1;

		dpu_reg.reg_sgbm_ld1_crop_enable =1;
		dpu_reg.reg_sgbm_ld2_crop_enable =1;
		dpu_reg.reg_sgbm_mux_st_crop_enable =0;
		dpu_reg.reg_sgbm_bf_st_crop_enable =0;

		dpu_reg.reg_crop_enable_fgs_gx = 0;
		dpu_reg.reg_crop_enable_fgs_chfh =1;
		dpu_reg.reg_crop_enable_fgs_independent =1;
		dpu_reg.reg_crop_enable_fgs_ux =1;

		dpu_reg.reg_dpu_fgs_output_bit_choose =1;
		dpu_reg.reg_dpu_src_disp_mux =1;
	}else if(dpu_ctx[grp]->grp_attr.dpu_mode == DPU_MODE_FGS_MUX0){
		dpu_reg.reg_sgbm2fgs_online = 0;
		dpu_reg.reg_dpu_sgbm_enable =0;
		dpu_reg.reg_dpu_fgs_enable =1;

		dpu_reg.reg_dpu_data_sel = 2;

		dpu_reg.reg_sgbm_ld1_dma_enable =0;
		dpu_reg.reg_sgbm_ld2_dma_enable =0;
		dpu_reg.reg_sgbm_mux_st_dma_enable =0;
		dpu_reg.reg_sgbm_bf_st_dma_enable =0;

		dpu_reg.reg_dma_enable_fgs1 =1;
		dpu_reg.reg_dma_enable_fgs2 =1;
		dpu_reg.reg_dma_enable_fgs3 =1;
		dpu_reg.reg_dma_enable_fgs4 =1;

		dpu_reg.reg_sgbm_ld1_crop_enable =0;
		dpu_reg.reg_sgbm_ld2_crop_enable =0;
		dpu_reg.reg_sgbm_mux_st_crop_enable =0;
		dpu_reg.reg_sgbm_bf_st_crop_enable =0;

		dpu_reg.reg_crop_enable_fgs_gx = 0;
		dpu_reg.reg_crop_enable_fgs_chfh =1;
		dpu_reg.reg_crop_enable_fgs_independent =1;
		dpu_reg.reg_crop_enable_fgs_ux =1;

		dpu_reg.reg_dpu_fgs_output_bit_choose =0;
		dpu_reg.reg_dpu_src_disp_mux =0;
	}else if(dpu_ctx[grp]->grp_attr.dpu_mode == DPU_MODE_FGS_MUX1){
		dpu_reg.reg_sgbm2fgs_online = 0;
		dpu_reg.reg_dpu_sgbm_enable =0;
		dpu_reg.reg_dpu_fgs_enable =1;

		dpu_reg.reg_dpu_data_sel = 2;

		dpu_reg.reg_sgbm_ld1_dma_enable =0;
		dpu_reg.reg_sgbm_ld2_dma_enable =0;
		dpu_reg.reg_sgbm_mux_st_dma_enable =0;
		dpu_reg.reg_sgbm_bf_st_dma_enable =0;

		dpu_reg.reg_dma_enable_fgs1 =1;
		dpu_reg.reg_dma_enable_fgs2 =1;
		dpu_reg.reg_dma_enable_fgs3 =1;
		dpu_reg.reg_dma_enable_fgs4 =1;

		dpu_reg.reg_sgbm_ld1_crop_enable =0;
		dpu_reg.reg_sgbm_ld2_crop_enable =0;
		dpu_reg.reg_sgbm_mux_st_crop_enable =0;
		dpu_reg.reg_sgbm_bf_st_crop_enable =0;

		dpu_reg.reg_crop_enable_fgs_gx = 0;
		dpu_reg.reg_crop_enable_fgs_chfh =1;
		dpu_reg.reg_crop_enable_fgs_independent =1;
		dpu_reg.reg_crop_enable_fgs_ux =1;

		dpu_reg.reg_dpu_fgs_output_bit_choose =1;
		dpu_reg.reg_dpu_src_disp_mux =0;
	}

	if(dpu_ctx[grp]->grp_attr.isbtcostout){
		dpu_reg.reg_sgbm_bf_st_dma_enable =1;
		dpu_reg.reg_sgbm_bf_st_crop_enable =1;
	}
	TRACE_DPU(DBG_INFO, " dpu_reg_config          -\n");
	return SUCCESS;
}

void dpu_write_sgbm_all_reg(void)
{
	unsigned int reg_04;
	unsigned int reg_08;
	unsigned int reg_0C;
	unsigned int reg_10;
	unsigned int reg_14;
    TRACE_DPU(DBG_INFO,"[Write] Write dpu sgbm Reg configurations ...\n");
    //reg_00
    reg_write_mask(reg_base + DPU_REG_00_OFS,((1 << 1)-1) << 0,1);
    //reg_04
    reg_04 = (dpu_reg.reg_dpu_img_width-1) | ((dpu_reg.reg_dpu_img_height-1) << 16);
    write_reg(reg_base + DPU_REG_04_OFS ,reg_04);
    //reg_08
    reg_08 = dpu_reg.reg_dpu_min_d ;
    write_reg(reg_base + DPU_REG_08_OFS ,reg_08);
    //reg_0C
    reg_0C = (dpu_reg.reg_dpu_rshift1 << 8) \
                    | (dpu_reg.reg_dpu_rshift2 << 12);
    write_reg(reg_base+DPU_REG_0C_OFS ,reg_0C);
    //reg_10
    reg_10 = dpu_reg.reg_dpu_ca_p1 | (dpu_reg.reg_dpu_ca_p2 << 16);
    write_reg(reg_base+DPU_REG_10_OFS ,reg_10);
    //reg_14
    reg_14 = dpu_reg.reg_dpu_uniq_ratio | (dpu_reg.reg_dpu_disp_shift << 8) \
                    | (dpu_reg.reg_dpu_bfw_size << 16) \
                    | (dpu_reg.reg_dpu_census_shift << 18);
    write_reg(reg_base + DPU_REG_14_OFS ,reg_14);
    //reg_1C
    reg_write_mask(reg_base + DPU_REG_1C_OFS, ((1 << 1)-1) << 20,dpu_reg.reg_sgbm_bf_st_dma_enable << 20 );
    reg_write_mask(reg_base + DPU_REG_1C_OFS, ((1 << 1)-1) << 21,dpu_reg.reg_sgbm_ld1_dma_enable << 21 );
    reg_write_mask(reg_base + DPU_REG_1C_OFS, ((1 << 1)-1) << 22,dpu_reg.reg_sgbm_ld2_dma_enable << 22 );
    reg_write_mask(reg_base + DPU_REG_1C_OFS, ((1 << 1)-1) << 23,dpu_reg.reg_sgbm_mux_st_dma_enable << 23 );
    reg_write_mask(reg_base + DPU_REG_1C_OFS, ((1 << 1)-1) << 29,dpu_reg.reg_sgbm_bf_st_crop_enable << 29 );
    reg_write_mask(reg_base + DPU_REG_1C_OFS, ((1 << 1)-1) << 30,dpu_reg.reg_sgbm_ld1_crop_enable << 30 );
    reg_write_mask(reg_base + DPU_REG_1C_OFS, ((1 << 1)-1) << 31,dpu_reg.reg_sgbm_ld2_crop_enable << 31 );
    //reg_68
    reg_write_mask(reg_base + DPU_REG_68_OFS, ((1 << 1)-1) << 2,dpu_reg.reg_sgbm_mux_st_crop_enable << 2 );
    reg_write_mask(reg_base + DPU_REG_68_OFS, ((1 << 1)-1) << 12,dpu_reg.reg_dpu_sgbm_enable << 12 );
    reg_write_mask(reg_base + DPU_REG_68_OFS, ((1 << 1)-1) << 16,dpu_reg.reg_sgbm2fgs_online << 16 );
    //reg_74
    reg_write_mask(reg_base + DPU_REG_74_OFS, ((1 << 3)-1) << 29,dpu_reg.reg_dpu_disp_range << 29 );
    //reg_78
    reg_write_mask(reg_base + DPU_REG_78_OFS, ((1 << 3)-1) << 0,dpu_reg.reg_dpu_dcc_a234 << 0 );
    //reg_88
    reg_write_mask(reg_base + DPU_REG_88_OFS, ((1 << 2)-1) << 1,dpu_reg.reg_dpu_data_sel << 1);
    reg_write_mask(reg_base + DPU_REG_88_OFS, ((1 << 1)-1) << 3,dpu_reg.reg_sgbm_bf_st_crop_enable << 3);

    TRACE_DPU(DBG_INFO,"[Write] Write dpu sgbm Reg Done ...\n");
}

void dpu_write_fgs_all_reg(void)
{
	unsigned int reg_70;
    TRACE_DPU(DBG_INFO,"[Write] Write dpu fgs Reg configurations ...\n");

    //reg_1C
    reg_write_mask(reg_base + DPU_REG_1C_OFS, ((1 << 5)-1) << 0,dpu_reg.reg_dpu_fgs_max_count << 0 );
    reg_write_mask(reg_base + DPU_REG_1C_OFS, ((1 << 1)-1) << 24,dpu_reg.reg_dma_enable_fgs1 << 24 );
    reg_write_mask(reg_base + DPU_REG_1C_OFS, ((1 << 1)-1) << 25,dpu_reg.reg_dma_enable_fgs2 << 25 );
    reg_write_mask(reg_base + DPU_REG_1C_OFS, ((1 << 1)-1) << 26,dpu_reg.reg_dma_enable_fgs3 << 26 );
    reg_write_mask(reg_base + DPU_REG_1C_OFS, ((1 << 1)-1) << 27,dpu_reg.reg_dma_enable_fgs4 << 27 );
    //reg_68
    reg_write_mask(reg_base + DPU_REG_68_OFS, ((1 << 1)-1) << 3,dpu_reg.reg_crop_enable_fgs_chfh << 3 );
    reg_write_mask(reg_base + DPU_REG_68_OFS, ((1 << 1)-1) << 4,dpu_reg.reg_crop_enable_fgs_gx << 4 );
    reg_write_mask(reg_base + DPU_REG_68_OFS, ((1 << 1)-1) << 5,dpu_reg.reg_crop_enable_fgs_ux << 5 );
    reg_write_mask(reg_base + DPU_REG_68_OFS, ((1 << 1)-1) << 13,dpu_reg.reg_dpu_fgs_enable << 13 );
	reg_write_mask(reg_base + DPU_REG_68_OFS, ((1 << 1)-1) << 16,dpu_reg.reg_sgbm2fgs_online << 16 );
    //reg_70
    reg_70 = (dpu_reg.reg_dpu_fgs_img_width-1) | ((dpu_reg.reg_dpu_fgs_img_height-1) << 16);
    write_reg(reg_base + DPU_REG_70_OFS ,reg_70);

    //reg_74
    reg_write_mask(reg_base + DPU_REG_74_OFS, ((1 << 8)-1),dpu_reg.reg_dpu_nd_ds);
    reg_write_mask(reg_base + DPU_REG_74_OFS, ((1 << 20)-1) << 8,dpu_reg.reg_dpu_fxbaseline << 8);
    reg_write_mask(reg_base + DPU_REG_74_OFS, ((1 << 1)-1) << 28,dpu_reg.reg_dpu_fgs_output_bit_choose << 28);
    //reg_78
    reg_write_mask(reg_base + DPU_REG_78_OFS, ((1 << 2)-1) << 16,dpu_reg.reg_dpu_src_disp_mux << 16 );
    reg_write_mask(reg_base + DPU_REG_78_OFS, ((1 << 7)-1) << 18,dpu_reg.reg_dpu_fgs_max_t << 18);
    //reg_88
    reg_write_mask(reg_base + DPU_REG_88_OFS, ((1 << 1)-1) << 0,dpu_reg.reg_crop_enable_fgs_independent << 0 );
    //reg_98
    reg_write_mask(reg_base + DPU_REG_98_OFS, ((1 << 2)-1) << 16,dpu_reg.reg_dpu_fgs_output_unit_choose << 16);
    TRACE_DPU(DBG_INFO,"[Write] Write dpu fgs Reg configurations Done ...\n");
};

static int commit_hw_settings(dpu_grp working_grp)
{
	unsigned int seg_len;
	unsigned int seg_num;
	TRACE_DPU(DBG_INFO, " commit_hw_settings          +\n");

	write_reg(reg_base+DPU_REG_90_OFS,0);
    write_reg(reg_base+DPU_REG_94_OFS,0);
    reg_write_mask(reg_base+DPU_REG_98_OFS,((1<<1)-1) <<0,0 << 0);
    reg_write_mask(reg_base+DPU_REG_98_OFS,((1<<1)-1) <<1,0 << 1);
	if(dpu_ctx[working_grp]->grp_attr.isbtcostout){
		seg_len= dpu_reg.reg_dpu_img_width;
		seg_num= dpu_reg.reg_dpu_img_height;
		register_sgbm_bf_st_ld(seg_len, seg_num,dram_base_out_btcost_h, \
						dram_base_out_btcost_l,reg_base_sgbm_bf_dma, reg_base);
	}

	if(dpu_ctx[working_grp]->grp_attr.dpu_mode == DPU_MODE_DEFAULT  ||
		dpu_ctx[working_grp]->grp_attr.dpu_mode == DPU_MODE_SGBM_MUX0 ||
		dpu_ctx[working_grp]->grp_attr.dpu_mode == DPU_MODE_SGBM_MUX1 ||
		dpu_ctx[working_grp]->grp_attr.dpu_mode == DPU_MODE_SGBM_MUX2 ){
		seg_len= dpu_reg.reg_dpu_img_width;
		seg_num= dpu_reg.reg_dpu_img_height;
		register_sgbm_ld1_ld(seg_len, seg_num,dram_base_left_h, \
						dram_base_left_l,reg_base_sgbm_ld1_dma, reg_base);
		register_sgbm_ld2_ld(seg_len, seg_num,dram_base_right_h, \
						dram_base_right_l,reg_base_sgbm_ld2_dma, reg_base);
		register_sgbm_median_st_ld(seg_len, seg_num,dpu_reg.reg_dpu_data_sel,\
								dram_base_out_h,dram_base_out_l,reg_base_sgbm_median_dma, reg_base);
		dpu_write_sgbm_all_reg();
		TRACE_DPU(DBG_INFO,"[%s]write sgbm reg\n",__func__);
	}else if(dpu_ctx[working_grp]->grp_attr.dpu_mode == DPU_MODE_SGBM_FGS_ONLINE_MUX0 ||
			 dpu_ctx[working_grp]->grp_attr.dpu_mode == DPU_MODE_SGBM_FGS_ONLINE_MUX1 ||
			 dpu_ctx[working_grp]->grp_attr.dpu_mode == DPU_MODE_SGBM_FGS_ONLINE_MUX2 ){
		seg_len= dpu_reg.reg_dpu_img_width;
		seg_num= dpu_reg.reg_dpu_img_height;
		register_sgbm_ld1_ld(seg_len, seg_num,dram_base_left_h, \
						dram_base_left_l,reg_base_sgbm_ld1_dma, reg_base);
		register_sgbm_ld2_ld(seg_len, seg_num,dram_base_right_h, \
						dram_base_right_l,reg_base_sgbm_ld2_dma, reg_base);
		register_fgs_gx_ld(seg_len, seg_num,dram_base_left_h, \
						dram_base_left_l,reg_base_fgs_gx_dma, reg_base);
		register_fgs_chfh_ld(seg_len, seg_num,dram_base_right_h, \
						dram_base_right_l,reg_base_fgs_chfh_ld_dma, reg_base);
		register_fgs_chfh_st(seg_len, seg_num, dram_base_chfh_h, \
						dram_base_chfh_l,reg_base_fgs_chfh_st_dma, reg_base);
		register_fgs_ux_st(seg_len, seg_num,dpu_reg.reg_dpu_fgs_output_bit_choose,\
						dram_base_out_h, dram_base_out_l,reg_base_fgs_ux_dma, reg_base);
		dpu_write_sgbm_all_reg();
		dpu_write_fgs_all_reg();
		TRACE_DPU(DBG_INFO,"[%s]write sgbm reg\n",__func__);
		TRACE_DPU(DBG_INFO,"[%s]write fgs reg\n",__func__);
	}else if(dpu_ctx[working_grp]->grp_attr.dpu_mode == DPU_MODE_FGS_MUX0 ||
			 dpu_ctx[working_grp]->grp_attr.dpu_mode == DPU_MODE_FGS_MUX1 ){
		seg_len= dpu_reg.reg_dpu_fgs_img_width;
		seg_num= dpu_reg.reg_dpu_fgs_img_height;
		register_fgs_gx_ld(seg_len, seg_num,dram_base_left_h, \
						dram_base_left_l,reg_base_fgs_gx_dma, reg_base);
		register_fgs_chfh_ld(seg_len, seg_num,dram_base_right_h, \
						dram_base_right_l,reg_base_fgs_chfh_ld_dma, reg_base);
		register_fgs_chfh_st(seg_len, seg_num, dram_base_chfh_h, \
						dram_base_chfh_l,reg_base_fgs_chfh_st_dma, reg_base);
		register_fgs_ux_st(seg_len, seg_num,dpu_reg.reg_dpu_fgs_output_bit_choose,\
						dram_base_out_h, dram_base_out_l,reg_base_fgs_ux_dma, reg_base);
		dpu_write_fgs_all_reg();
		TRACE_DPU(DBG_INFO,"[%s]write fgs reg\n",__func__);
	}
	TRACE_DPU(DBG_INFO, " commit_hw_settings          -\n");
	return SUCCESS;
}

static int hw_start(dpu_grp workgrp)
{
	TRACE_DPU(DBG_INFO, "hw_start          +\n");
	ktime_get_ts64(&dpu_dev->time_start);
	dpu_engine(workgrp);
	TRACE_DPU(DBG_INFO, "hw_start          -\n");
	return SUCCESS;
}

static unsigned char dpu_handler_is_idle(void)
{
	int i;
	for (i = 0; i < DPU_MAX_GRP_NUM; i++)
		if (dpu_ctx[i] && dpu_ctx[i]->iscreated && dpu_ctx[i]->isstarted)
			return FALSE;

	return TRUE;
}

static int32_t dpu_base_get_frame_info(vb_cal_config_s vb_config,pixel_format_e fmt, size_s size, struct video_buffer *buf, unsigned long long mem_base,unsigned int blk_size)
{
	unsigned char i = 0;
	memset(buf, 0, sizeof(*buf));
	memcpy(&buf->size,&size,sizeof(buf->size));
	buf->pixel_format = fmt;
	for (i = 0; i < vb_config.plane_num; ++i) {
		buf->phy_addr[i] = mem_base;
		buf->length[i] = blk_size;
		buf->stride[i] = (i == 0) ? vb_config.main_stride : vb_config.c_stride;
		mem_base += buf->length[i];

		pr_debug("(%llx-%zu-%d)\n", buf->phy_addr[i], buf->length[i], buf->stride[i]);
	}

	return SUCCESS;
}

static void _dpu_fill_buffer(mmf_chn_s chn, struct vb_s *grp_vb_in,
		unsigned long long phy_addr, struct video_buffer *buf, struct dpu_ctx_s *ctx)
{

	size_s size;
	unsigned int blk_size;
	TRACE_DPU(DBG_INFO, "dpu_fill_buffer          +\n");
	size.width = ctx->chn_cfgs[chn.chn_id].chn_attr.img_size.width;
	size.height = ctx->chn_cfgs[chn.chn_id].chn_attr.img_size.height;
	blk_size = ctx->chn_cfgs[chn.chn_id].blk_size;
	dpu_base_get_frame_info( ctx->chn_cfgs[chn.chn_id].vb_config
			   , ctx->chn_cfgs[chn.chn_id].pixel_format
			   , size
			   , buf
			   , phy_addr
			   , blk_size);
	buf->offset_top = 0;
	buf->offset_bottom =0;
	buf->offset_left = 0;
	buf->offset_right =0;

	if (grp_vb_in) {
		buf->pts = grp_vb_in->buf.pts;
		buf->frm_num = grp_vb_in->buf.frm_num;
		buf->motion_lv = grp_vb_in->buf.motion_lv;
		memcpy(buf->motion_table, grp_vb_in->buf.motion_table, MO_TBL_SIZE);
	}
}


static int dpu_qbuf(mmf_chn_s chn, struct vb_s *grp_vb_in,
	vb_blk chn_vb_blk, struct dpu_ctx_s *ctx)
{

	vb_blk blk = chn_vb_blk;
	TRACE_DPU(DBG_INFO, "dpu_qbuf          +\n");
	_dpu_fill_buffer(chn, grp_vb_in, vb_handle2phys_addr(blk), &((struct vb_s *)blk)->buf, ctx);

	if (vb_qbuf(chn, CHN_TYPE_OUT, &dpu_jobs[chn.dev_id].outs[chn.chn_id], blk) == -ENOBUFS){
		TRACE_DPU(DBG_ERR, "Grp(%d) Chn(%d) qbuf failed\n", chn.dev_id, chn.chn_id);
		return FAILURE;
	}
	vb_release_block(blk);
	TRACE_DPU(DBG_INFO, "dpu_qbuf          -\n");
	return SUCCESS;
}

static int fill_buffers(dpu_grp dpu_grp_id)
{
	dpu_chn dpu_chn_id ;
	vb_cal_config_s vb_config_chfh;
	unsigned long long phyAddr_out_arr[DPU_MAX_CHN_NUM] ;
	unsigned long long phyAddr_left ;
	unsigned long long phyAddr_right ;
	unsigned long long phyAddr_out;
	unsigned long long phyAddr_out_btcost;
	//vb_blk blk_chfh = VB_INVALID_HANDLE;
	vb_blk blk[DPU_MAX_CHN_NUM] = { [0 ... DPU_MAX_CHN_NUM - 1] = VB_INVALID_HANDLE };
	//struct vdev *d;
	mmf_chn_s chn_left = {.mod_id = ID_DPU, .dev_id = dpu_grp_id, .chn_id = 0};
	mmf_chn_s chn_right = {.mod_id = ID_DPU, .dev_id = dpu_grp_id, .chn_id = 1};
	mmf_chn_s chn_out[DPU_MAX_CHN_NUM] = {chn_left, chn_right};
	int ret = SUCCESS;
	struct dpu_chn_cfg_s *stChnCfg;
	struct vb_s *vb_in = NULL;

	struct video_buffer *buf_out[DPU_MAX_CHN_NUM];
	struct video_buffer *buf_left;
	struct video_buffer *buf_right;

	unsigned char is_true_left = FALSE;
	unsigned char is_true_right = FALSE;
	unsigned char is_true_out[DPU_MAX_CHN_NUM] = {FALSE};

	unsigned char is_null_out[DPU_MAX_CHN_NUM] = {TRUE};
	char chfh_ion_name[10] = "dpuChfh";
	//unsigned char is_true_chfh = FALSE;
	TRACE_DPU(DBG_INFO, "fill_buffers          +\n");
	if ( base_mod_jobs_waitq_empty(&dpu_jobs[dpu_grp_id].ins[0]) ||
		base_mod_jobs_waitq_empty(&dpu_jobs[dpu_grp_id].ins[1])){
			TRACE_DPU(DBG_ERR, "waitq empty.\n");
			print_vbq_size(dpu_grp_id);
			return ERR_DPU_BUF_EMPTY;
		}

	buf_left = base_mod_jobs_enque_work(&dpu_jobs[dpu_grp_id].ins[0]);
	if (buf_left == NULL) {
		TRACE_DPU(DBG_ERR, "left frame Grp(%d) qbuf failed.\n", dpu_grp_id);
		ret = ERR_DPU_NOTREADY;
		goto ERR_FILL_BUF;
	}
	is_true_left=TRUE;

	buf_right = base_mod_jobs_enque_work(&dpu_jobs[dpu_grp_id].ins[1]);
	if (buf_right == NULL) {
		TRACE_DPU(DBG_ERR, "left frame Grp(%d) qbuf failed.\n", dpu_grp_id);
		ret = ERR_DPU_NOTREADY;
		goto ERR_FILL_BUF;
	}
	is_true_right=TRUE;
	for (dpu_chn_id = 0; dpu_chn_id < dpu_ctx[dpu_grp_id]->chn_num; ++dpu_chn_id) {
		if (!base_mod_jobs_waitq_empty(&dpu_jobs[dpu_grp_id].outs[dpu_chn_id])) {
			is_null_out[dpu_chn_id]=FALSE;
		} else {
			is_null_out[dpu_chn_id]=TRUE;
		}
	}
	vb_in = container_of(buf_left, struct vb_s, buf);
	// get buffers.
	for (dpu_chn_id = 0; dpu_chn_id < dpu_ctx[dpu_grp_id]->chn_num; ++dpu_chn_id) {
		stChnCfg = &dpu_ctx[dpu_grp_id]->chn_cfgs[dpu_chn_id];
		if (!stChnCfg->isenabled)
			continue;

		// chn video_buffer from user
		if (!base_mod_jobs_waitq_empty(&dpu_jobs[dpu_grp_id].outs[dpu_chn_id])) {
			TRACE_DPU(DBG_INFO, "Grp(%d) Chn(%d) chn video_buffer from user.\n", dpu_grp_id, dpu_chn_id);

			buf_out[dpu_chn_id] = base_mod_jobs_enque_work(&dpu_jobs[dpu_grp_id].outs[dpu_chn_id]);
			if (!buf_out[dpu_chn_id]) {
				TRACE_DPU(DBG_ERR, "Grp(%d) Chn(%d) qbuf failed.\n", dpu_grp_id, dpu_chn_id);
				ret = ERR_DPU_NOTREADY;
				goto ERR_FILL_BUF;
			}
			is_true_out[dpu_chn_id]=TRUE;
			phyAddr_out_arr[dpu_chn_id] =buf_out[dpu_chn_id]->phy_addr[0];

			continue;
		}

		// chn video_buffer from pool
		blk[dpu_chn_id] = vb_get_block_with_id(dpu_ctx[dpu_grp_id]->chn_cfgs[dpu_chn_id].vb_pool,
						dpu_ctx[dpu_grp_id]->chn_cfgs[dpu_chn_id].blk_size, ID_DPU);
		if (blk[dpu_chn_id] == VB_INVALID_HANDLE) {
			TRACE_DPU(DBG_ERR, "Grp(%d) Chn(%d) Can't acquire VB BLK for DPU\n"
				, dpu_grp_id, dpu_chn_id);
			ret = ERR_DPU_NOBUF;
			goto ERR_FILL_BUF;
		}
		phyAddr_out_arr[dpu_chn_id] =vb_handle2phys_addr(blk[dpu_chn_id]);
		if(dpu_qbuf(chn_out[dpu_chn_id], vb_in, blk[dpu_chn_id], dpu_ctx[dpu_grp_id]) != SUCCESS){
			TRACE_DPU(DBG_ERR, "Grp(%d) chn-out(%d) qbuf fail.\n"
				, dpu_grp_id,dpu_chn_id);
			ret = FAILURE;
			goto ERR_FILL_BUF;
		}
	}

	if(dpu_ctx[dpu_grp_id]->grp_attr.dpu_mode == DPU_MODE_FGS_MUX0 ||
		dpu_ctx[dpu_grp_id]->grp_attr.dpu_mode == DPU_MODE_FGS_MUX1 ||
		dpu_ctx[dpu_grp_id]->grp_attr.dpu_mode == DPU_MODE_SGBM_FGS_ONLINE_MUX0 ||
		dpu_ctx[dpu_grp_id]->grp_attr.dpu_mode == DPU_MODE_SGBM_FGS_ONLINE_MUX1  ){

		common_getpicbufferconfig(dpu_ctx[dpu_grp_id]->grp_attr.left_image_size.width, \
			dpu_ctx[dpu_grp_id]->grp_attr.left_image_size.height, \
			PIXEL_FORMAT_YUV_400, DATA_BITWIDTH_8, COMPRESS_MODE_NONE, ALIGN_32, &vb_config_chfh);
		ret = base_ion_alloc(&dpu_dev->phyaddr_chfh, &dpu_dev->viraddr_chfh,
							(uint8_t *)chfh_ion_name,
							vb_config_chfh.vb_size*5,
							FALSE);
		// ret = base_ion_alloc(&dpu_ctx[dpu_grp_id]->phyaddr_chfh, &dpu_ctx[dpu_grp_id]->viraddr_chfh,
		// 	 (uint8_t *)chfh_ion_name, vb_config_chfh.vb_size*5, FALSE);
		if (ret) {
			TRACE_DPU(DBG_ERR, "base_ion_alloc fail! ret(%d)\n", ret);
			goto ERR_FILL_BUF;
		}
		// dram_base_chfh_l = dpu_ctx[dpu_grp_id]->phyaddr_chfh &(0xFFFFFFFF);
		// dram_base_chfh_h = dpu_ctx[dpu_grp_id]->phyaddr_chfh >> 32;
		dram_base_chfh_l = dpu_dev->phyaddr_chfh &(0xFFFFFFFF);
		dram_base_chfh_h = dpu_dev->phyaddr_chfh >> 32;
		TRACE_DPU(DBG_INFO, "chfh base(0x%llx) h(0x%x) l(0x%x) \n", dpu_dev->phyaddr_chfh,dram_base_chfh_h,dram_base_chfh_l);
	}

	phyAddr_out =phyAddr_out_arr[0];
	phyAddr_out_btcost =phyAddr_out_arr[1];
	phyAddr_left =buf_left->phy_addr[0];
	phyAddr_right =buf_right->phy_addr[0];

	dram_base_out_l = phyAddr_out &(0xFFFFFFFF);
	dram_base_out_h = phyAddr_out >> 32;

	dram_base_out_btcost_l = phyAddr_out_btcost &(0xFFFFFFFF);
	dram_base_out_btcost_h = phyAddr_out_btcost >> 32;

	dram_base_left_l = phyAddr_left &(0xFFFFFFFF);
	dram_base_left_h = phyAddr_left >> 32;

	dram_base_right_l = phyAddr_right &(0xFFFFFFFF);
	dram_base_right_h = phyAddr_right >> 32;

	dpu_ctx[dpu_grp_id]->pixel_format = vb_in->buf.pixel_format;
	TRACE_DPU(DBG_INFO, "left base(0x%llx) h(0x%x) l(0x%x) \n",phyAddr_left,dram_base_left_h,dram_base_left_l);
	TRACE_DPU(DBG_INFO, "right base(0x%llx) h(0x%x) l(0x%x) \n",phyAddr_right,dram_base_right_h,dram_base_right_l);
	TRACE_DPU(DBG_INFO, "out base(0x%llx) h(0x%x) l(0x%x) \n",phyAddr_out,dram_base_out_h,dram_base_out_l);
	TRACE_DPU(DBG_INFO, "btcost out base(0x%llx) h(0x%x) l(0x%x) \n",phyAddr_out_btcost,dram_base_out_btcost_h,dram_base_out_btcost_l);

	TRACE_DPU(DBG_INFO, "fill_buffers          -\n");
	return ret;
ERR_FILL_BUF:
	for (dpu_chn_id = 0; dpu_chn_id < dpu_ctx[dpu_grp_id]->chn_num; ++dpu_chn_id) {
		if (blk[dpu_chn_id] != VB_INVALID_HANDLE)
			vb_release_block(blk[dpu_chn_id]);
	}

	if(dpu_ctx[dpu_grp_id]->chfh_blk!= VB_INVALID_HANDLE)
		vb_release_block(dpu_ctx[dpu_grp_id]->chfh_blk);

	if(is_true_left){
		vb_blk BLK_temp = base_mod_jobs_workq_pop(&dpu_jobs[dpu_grp_id].ins[0]);
		if (BLK_temp != VB_INVALID_HANDLE)
			vb_release_block(BLK_temp);
	} else {
		vb_blk BLK_temp = base_mod_jobs_waitq_pop(&dpu_jobs[dpu_grp_id].ins[0]);
		if (BLK_temp != VB_INVALID_HANDLE)
			vb_release_block(BLK_temp);
	}

	if(is_true_right){
		vb_blk BLK_temp = base_mod_jobs_workq_pop(&dpu_jobs[dpu_grp_id].ins[1]);
		if (BLK_temp != VB_INVALID_HANDLE)
			vb_release_block(BLK_temp);
	} else {
		vb_blk BLK_temp = base_mod_jobs_waitq_pop(&dpu_jobs[dpu_grp_id].ins[1]);
		if (BLK_temp != VB_INVALID_HANDLE)
			vb_release_block(BLK_temp);
	}

	for (dpu_chn_id = 0; dpu_chn_id < dpu_ctx[dpu_grp_id]->chn_num; ++dpu_chn_id) {
		if(!is_null_out[dpu_chn_id]){
			if(is_true_out[dpu_chn_id]){
				vb_blk BLK_temp = base_mod_jobs_workq_pop(&dpu_jobs[dpu_grp_id].outs[dpu_chn_id]);
				if (BLK_temp != VB_INVALID_HANDLE)
					vb_release_block(BLK_temp);
			} else {
				vb_blk BLK_temp = base_mod_jobs_waitq_pop(&dpu_jobs[dpu_grp_id].outs[dpu_chn_id]);
				if (BLK_temp != VB_INVALID_HANDLE)
					vb_release_block(BLK_temp);
			}
		}

	}

	return ret;
}

static void _release_dpu_waitq(mmf_chn_s chn, enum chn_type_e chn_type)
{
	vb_blk blk_grp;
	TRACE_DPU(DBG_INFO, "release_dpu_waitq          +\n");
	if (chn_type == CHN_TYPE_OUT)
		blk_grp = base_mod_jobs_waitq_pop(&dpu_jobs[chn.dev_id].outs[chn.chn_id]);
	else
		blk_grp = base_mod_jobs_waitq_pop(&dpu_jobs[chn.dev_id].ins[chn.chn_id]);

	if (blk_grp != VB_INVALID_HANDLE)
		vb_release_block(blk_grp);

	TRACE_DPU(DBG_INFO, "release_dpu_waitq          -\n");
}

static int dpu_try_schedule_offline(struct dpu_handler_ctx_s *ctx)
{

	unsigned int ret;
	dpu_grp working_grp = ctx->working_grp;
	unsigned char working_mask = 0;

	unsigned int leftPipeId=0;
	unsigned int rightPipeId=1;
	mmf_chn_s chn = {.mod_id = ID_DPU, .dev_id = working_grp, .chn_id = 0};

	dpu_ctx[working_grp]->grp_work_wtatus.start_cnt ++;
	TRACE_DPU(DBG_INFO, "dpu_try_schedule_offline          +\n");
	ktime_get_ts64(&ctx->time);

	// sc's mask
	working_mask = get_work_mask(dpu_ctx[working_grp]);

	if (working_mask == 0) {
		TRACE_DPU(DBG_NOTICE, "grp(%d) working_mask zero.\n", working_grp);
		chn.chn_id=leftPipeId;
		_release_dpu_waitq(chn, CHN_TYPE_IN);
		chn.chn_id=rightPipeId;
		_release_dpu_waitq(chn, CHN_TYPE_IN);
		goto dpu_next_job;
	}

	ret =dpu_reg_config(working_grp);
	if(ret != SUCCESS){
		TRACE_DPU(DBG_ERR, "grp(%d) dpu para config failed.\n", working_grp);

		goto dpu_next_job;
	}
	// mutex_lock(&dpu_jobs[working_grp].lock);
	if (fill_buffers(working_grp) != SUCCESS) {
		TRACE_DPU(DBG_ERR, "grp(%d) fill video_buffer NG.\n", working_grp);

		goto dpu_next_job;
	}
	dpu_enable_clk();
	dpu_reset();
	// commit hw settings of this dpu-grp.
	TRACE_DPU(DBG_INFO, "reg_base(0x%llx) .\n",reg_base );
	TRACE_DPU(DBG_INFO, "reg_dma_sgbm_ld1(0x%llx) .\n",reg_base_sgbm_ld1_dma);
	TRACE_DPU(DBG_INFO, "reg_dma_sgbm_ld2(0x%llx) .\n",reg_base_sgbm_ld2_dma);
	TRACE_DPU(DBG_INFO, "reg_base_sgbm_median_dma(0x%llx) .\n",reg_base_sgbm_median_dma);
	TRACE_DPU(DBG_INFO, "reg_base_sgbm_bf_dma(0x%llx) .\n",reg_base_sgbm_bf_dma);
	TRACE_DPU(DBG_INFO, "reg_base_fgs_gx_dma(0x%llx) .\n",reg_base_fgs_gx_dma);
	TRACE_DPU(DBG_INFO, "reg_base_fgs_ux_dma(0x%llx) .\n",reg_base_fgs_ux_dma);
	TRACE_DPU(DBG_INFO, "reg_base_fgs_chfh_st_dma(0x%llx) .\n",reg_base_fgs_chfh_st_dma);
	TRACE_DPU(DBG_INFO, "reg_base_fgs_chfh_ld_dma(0x%llx) .\n",reg_base_fgs_chfh_ld_dma);
	commit_hw_settings(working_grp);

	/* Update state first, isr could occur immediately */
	ctx->working_grp = working_grp;
	ctx->working_mask = working_mask;

	dpu_ctx[working_grp]->grp_state = GRP_STATE_HW_STARTED;
	print_vbq_size(working_grp);
	hw_start(working_grp);

	dpu_dev->hw_busy= TRUE;
	// mutex_unlock(&dpu_jobs[working_grp].lock);

	/* Should use async sbm, the lock region is too big ! */

	TRACE_DPU(DBG_INFO, "ctx[%d] working_grp=%d\n",
			ctx->dpu_dev_id, ctx->working_grp);

	// wait for h/w done
	TRACE_DPU(DBG_INFO, "dpu_try_schedule_offline          -\n");
	return SUCCESS;

dpu_next_job:
	// job done.
	ctx->working_mask = 0;
	dpu_ctx[working_grp]->grp_state = GRP_STATE_IDLE;

	dpu_ctx[working_grp]->grp_work_wtatus.start_fail_cnt++;
	TRACE_DPU(DBG_INFO, "dpu_do_necxt_job         -\n");
	return FAILURE;
}

static void _dpu_handle_snap(mmf_chn_s chn, enum chn_type_e chn_type, vb_blk blk)
{

	struct vb_jobs_t *jobs = (chn_type == CHN_TYPE_OUT) ? &dpu_jobs[chn.dev_id].outs[chn.chn_id] : \
								&dpu_jobs[chn.dev_id].ins[chn.chn_id];
	struct vb_s *p = (struct vb_s *)blk;
	struct vbq *doneq;
	struct snap_s *s, *s_tmp;
	TRACE_DPU(DBG_INFO, "dpu_handle_snap          +\n");
	if (chn.mod_id != ID_DPU)
		return;

	if (jobs == NULL) {
		pr_err("handle snap fail, Null parameter\n");
		return;
	}

	if (!jobs->inited) {
		pr_err("handle snap fail, job not inited yet\n");
		return;
	}

	mutex_lock(&jobs->dlock);
	TAILQ_FOREACH_SAFE(s, &jobs->snap_jobs, tailq, s_tmp) {
		if (CHN_MATCH(&s->chn, &chn)) {
			TAILQ_REMOVE(&jobs->snap_jobs, s, tailq);
			s->blk = blk;
			atomic_fetch_add(1, &p->usr_cnt);
			atomic_long_fetch_or(BIT(ID_USER), &p->mod_ids);
			s->avail = TRUE;
			wake_up_all(&s->cond_queue);
			mutex_unlock(&jobs->dlock);
			return;
		}
	}

	doneq = _get_doneq(chn);
	// check if there is a snap-queue
	if (FIFO_CAPACITY(doneq)) {
		if (FIFO_FULL(doneq)) {
			struct vb_s *vb = NULL;

			FIFO_POP(doneq, &vb);
			atomic_long_fetch_and(~BIT(chn.mod_id), &vb->mod_ids);
			vb_release_block((vb_blk)vb);
		}
		atomic_fetch_add(1, &p->usr_cnt);
		atomic_long_fetch_or(BIT(chn.mod_id), &p->mod_ids);
		FIFO_PUSH(doneq, p);
		TRACE_DPU(DBG_INFO, "push doneq(%d)  \n",FIFO_SIZE(doneq));
	}

	mutex_unlock(&jobs->dlock);
	TRACE_DPU(DBG_INFO, "dpu_handle_snap          -\n");
}

int32_t dpu_vb_done_handler(mmf_chn_s chn, enum chn_type_e chn_type, vb_blk blk)
{
	mmf_bind_dest_s stBindDest;
	unsigned char i;
	int ret=0;
	struct  vb_s* vb;

	struct vb_jobs_t *jobs = (chn_type == CHN_TYPE_OUT) ? &dpu_jobs[chn.dev_id].outs[chn.chn_id] : \
								&dpu_jobs[chn.dev_id].ins[chn.chn_id];

	TRACE_DPU(DBG_INFO, "dpu_vb_done_handler          +\n");
	TRACE_DPU(DBG_INFO, "chn is %d    -\n", chn.chn_id);

	if (chn_type == CHN_TYPE_OUT) {
		_dpu_handle_snap(chn, chn_type, blk);
		if (bind_get_dst(&chn, &stBindDest) == SUCCESS) {
			for (i = 0; i < stBindDest.num; ++i) {
				vb_qbuf(stBindDest.mmf_chn[i], CHN_TYPE_IN, jobs, blk);
				pr_debug(" Mod(%s) chn(%d) dev(%d) -> Mod(%s) chn(%d) dev(%d)\n"
					     , sys_get_modname(chn.mod_id), chn.chn_id, chn.dev_id
					     , sys_get_modname(stBindDest.mmf_chn[i].mod_id)
					     , stBindDest.mmf_chn[i].chn_id
					     , stBindDest.mmf_chn[i].dev_id);
			}
		} else {
			// release if not found
			pr_debug("Mod(%s) chn(%d) dev(%d) src no dst release\n"
				     , sys_get_modname(chn.mod_id), chn.chn_id, chn.dev_id);
		}
	} else {
		pr_debug("Mod(%s) chn(%d) dev(%d) dst out release\n"
			     , sys_get_modname(chn.mod_id), chn.chn_id, chn.dev_id);
	}

	if (blk != VB_INVALID_HANDLE){
		vb = (struct  vb_s*)blk;
		TRACE_DPU(DBG_INFO, "dpu_vb_done_handler   vb cnt(%d)       -\n",vb->usr_cnt.counter);
		TRACE_DPU(DBG_INFO, "vb_release_block start \n");
		if(vb_release_block(blk) != SUCCESS)
			TRACE_DPU(DBG_INFO, "vb_release_block fail\n");

		TRACE_DPU(DBG_INFO, "vb_release_block done \n");
	}else{
		TRACE_DPU(DBG_INFO, "blk == VB_INVALID_HANDLE\n");
	}
	// if((struct  vb_s*)blk != NULL){
	// 	vb = (struct  vb_s*)blk;
	// 	TRACE_DPU(DBG_INFO, "dpu_vb_done_handler   vb cnt(%d)       -\n",vb->usr_cnt.counter);
	// }else{
	// 	TRACE_DPU(DBG_INFO, "dpu_vb_done_handler   vb NULL\n");
	// }

	TRACE_DPU(DBG_INFO, "dpu_vb_done_handler          -\n");
	return ret;
}

static void dpu_handle_frame_done(struct dpu_handler_ctx_s *ctx)
{

	struct timespec64 time;
	vb_blk blk_left;
	vb_blk blk_right;
	vb_blk blk_out[DPU_MAX_CHN_NUM];
	dpu_chn dpu_chn_id;
	unsigned long long duration_s;
	dpu_grp working_grp = ctx->working_grp;
	unsigned char working_mask = ctx->working_mask;

	mmf_chn_s chn_left = {.mod_id = ID_DPU, .dev_id = working_grp, .chn_id = 0};
	mmf_chn_s chn_right = {.mod_id = ID_DPU, .dev_id = working_grp, .chn_id = 1};
	mmf_chn_s chn_out[DPU_MAX_CHN_NUM] = {chn_left, chn_right};
	unsigned int duration = 0;
	TRACE_DPU(DBG_INFO, "dpu_handle_frame_done          +\n");
	TRACE_DPU(DBG_INFO, "ctx[%d] grp(%d) eof\n", ctx->dpu_dev_id, working_grp);

	dpu_ctx[working_grp]->grp_state = GRP_STATE_IDLE;

	vb_dqbuf(chn_left, &dpu_jobs[working_grp].ins[0], &blk_left);
	if (blk_left == VB_INVALID_HANDLE) {
		TRACE_DPU(DBG_ERR, "Mod(%d) can't get left vb-blk.\n", chn_left.mod_id);
	} else {
		dpu_vb_done_handler(chn_left, CHN_TYPE_IN, blk_left);
	}

	vb_dqbuf(chn_right, &dpu_jobs[working_grp].ins[1], &blk_right);
	if (blk_right == VB_INVALID_HANDLE) {
		TRACE_DPU(DBG_ERR, "Mod(%d) can't get right vb-blk.\n", chn_right.mod_id);
	} else {
		dpu_vb_done_handler(chn_right, CHN_TYPE_IN, blk_right);
	}

	dpu_chn_id = 0;
	do {
		if (!(working_mask & BIT(dpu_chn_id)))
			continue;

		working_mask &= ~BIT(dpu_chn_id);

		if (!dpu_ctx[working_grp]->chn_cfgs[dpu_chn_id].isenabled)
			continue;

		chn_out[dpu_chn_id].chn_id = dpu_chn_id;

		vb_dqbuf(chn_out[dpu_chn_id], &dpu_jobs[working_grp].outs[dpu_chn_id], &blk_out[dpu_chn_id]);
		if (blk_out[dpu_chn_id] == VB_INVALID_HANDLE) {
			TRACE_DPU(DBG_ERR, "Mod(%d) can't get out vb-blk.\n"
				     , chn_out[dpu_chn_id].mod_id);
			continue;
		}else {
			dpu_vb_done_handler(chn_out[dpu_chn_id], CHN_TYPE_OUT, blk_out[dpu_chn_id]);
		}

		TRACE_DPU(DBG_INFO, "grp(%d) chn(%d) end\n", working_grp, dpu_chn_id);

	} while (++dpu_chn_id < dpu_ctx[working_grp]->chn_num);

	ctx->working_mask = working_mask;

	dpu_ctx[working_grp]->grp_work_wtatus.send_pic_cnt ++ ;

	ktime_get_ts64(&time);
	duration = get_diff_in_us(ctx->time, time);
	dpu_ctx[working_grp]->cost_time_for_sec += duration;
	if(dpu_ctx[working_grp]->cost_time_for_sec <= (1000*1000)){
		dpu_ctx[working_grp]->frame_num ++;
	} else {
		dpu_ctx[working_grp]->grp_work_wtatus.frame_rate = \
		dpu_ctx[working_grp]->frame_num;
		dpu_ctx[working_grp]->cost_time_for_sec = 0;
		dpu_ctx[working_grp]->frame_num = 0;
	}

	dpu_ctx[working_grp]->grp_work_wtatus.cur_task_cost_tm = duration;
	if(dpu_ctx[working_grp]->grp_work_wtatus.cur_task_cost_tm \
		> dpu_ctx[working_grp]->grp_work_wtatus.max_task_cost_tm){
			dpu_ctx[working_grp]->grp_work_wtatus.max_task_cost_tm \
			= dpu_ctx[working_grp]->grp_work_wtatus.cur_task_cost_tm;
	}
	duration_s = duration;

	do_div(duration_s,(1000*1000));
	dpu_dev->run_time_info.run_tm = duration_s;
	ctx->events = 0;
	if(dpu_ctx[working_grp]->grp_attr.dpu_mode == DPU_MODE_FGS_MUX0 ||
		dpu_ctx[working_grp]->grp_attr.dpu_mode == DPU_MODE_FGS_MUX1 ||
		dpu_ctx[working_grp]->grp_attr.dpu_mode == DPU_MODE_SGBM_FGS_ONLINE_MUX0 ||
		dpu_ctx[working_grp]->grp_attr.dpu_mode == DPU_MODE_SGBM_FGS_ONLINE_MUX1  ){
		if (dpu_dev->phyaddr_chfh != 0) {
			base_ion_free(dpu_dev->phyaddr_chfh);
			dpu_dev->phyaddr_chfh = 0;
			TRACE_DPU(DBG_INFO, "phyaddr_chfh(0x%llx)", dpu_dev->phyaddr_chfh);
		}
	}
	dpu_dev->hw_busy = FALSE;
	dpu_disable_clk();
	TRACE_DPU(DBG_INFO, "dpu_handle_frame_done          -\n");
}

static void dpu_handle_offline(struct dpu_handler_ctx_s *ctx)
{

	struct timespec64 time;
	unsigned int state;
	unsigned long long duration64;
	dpu_grp working_grp = ctx->working_grp;
	unsigned char working_mask = ctx->working_mask;
	unsigned int events = ctx->events;
	TRACE_DPU(DBG_INFO, "dpu_handle_offline           +\n");
	TRACE_DPU(DBG_INFO, "working_mask(%d) ,working_grp(%d) ,dpu_dev_id(%d)\n",
			working_mask,working_grp,ctx->dpu_dev_id);
	if (working_mask == 0 && working_grp == DPU_MAX_GRP_NUM) {
		// find grp has job todo.
		working_grp = find_next_en_grp(working_grp, ctx->dpu_dev_id);
		ctx->working_grp = working_grp;

		if (working_grp >= DPU_MAX_GRP_NUM) {
			TRACE_DPU(DBG_INFO, "Grp(%d) invalid\n", working_grp);
			return;
		}
	}
	mutex_lock(&dpu_ctx[working_grp]->lock);

	// Sanity check

	if (!dpu_ctx[working_grp]) {
		TRACE_DPU(DBG_ERR, "Grp(%d) isn't created yet.\n", working_grp);
		return;
	}
	if (!dpu_ctx[working_grp]->iscreated || !dpu_ctx[working_grp]->isstarted) {
		TRACE_DPU(DBG_ERR, "Grp(%d) invalid, iscreated=%d, isstarted=%d\n",
				working_grp, dpu_ctx[working_grp]->iscreated, dpu_ctx[working_grp]->isstarted);
		return;
	}

	TRACE_DPU(DBG_INFO, "ctx[%d] event=0x%x, Grp(%d) , mask=0x%x\n",
			ctx->dpu_dev_id, events, working_grp,working_mask);
	state=dpu_ctx[working_grp]->grp_state;
	if (dpu_ctx[working_grp]->grp_state == GRP_STATE_IDLE) {
		dpu_try_schedule_offline(ctx);
	} else if (dpu_ctx[working_grp]->grp_state == GRP_STATE_HW_STARTED) {
		ktime_get_ts64(&time);
		duration64 = get_diff_in_us(ctx->time, time);
		do_div(duration64, 1000);

		if (ctx->events & CTX_EVENT_EOF) {
			dpu_handle_frame_done(ctx);
		} else {
			if (duration64 > (hw_wait_time-2)) {
				/* timeout */
				TRACE_DPU(DBG_INFO, "ctx[%d] event timeout on grp(%d)\n",
						ctx->dpu_dev_id, working_grp);
				//sclr_check_register();
				ctx->events = 0;
				ctx->working_mask =0;
				dpu_ctx[working_grp]->grp_state = GRP_STATE_IDLE;
				hw_reset(working_grp);

				// if(dpu_ctx[working_grp]->phyaddr_chfh!=0){
				// 	base_ion_free(dpu_ctx[working_grp]->phyaddr_chfh);
				// 	dpu_ctx[working_grp]->phyaddr_chfh=0;
				// }
				// TRACE_DPU(DBG_INFO, "phyaddr_chfh(0x%llx)", dpu_ctx[working_grp]->phyaddr_chfh);

			} else {
				// keep waiting
				TRACE_DPU(DBG_INFO, "ctx[%d] event no timeout on grp, but not done!(%d)\n",
						ctx->dpu_dev_id, working_grp);
			}
			dpu_ctx[working_grp]->grp_work_wtatus.start_fail_cnt++;

		}
	} else {
		TRACE_DPU(DBG_INFO, "ctx[%d] grp(%d) unexpected state=%d, events=0x%x\n",
				ctx->dpu_dev_id, working_grp, dpu_ctx[working_grp]->grp_state,
				ctx->events);
	}
	mutex_unlock(&dpu_ctx[working_grp]->lock);
	TRACE_DPU(DBG_INFO, "ctx[%d] evt=0x%x -> %x, mask=0x%x, Grp(%d) state 0x%x->0x%x\n",
			ctx->dpu_dev_id, events, ctx->events,ctx->working_mask, working_grp, state,
			dpu_ctx[working_grp]->grp_state);
	TRACE_DPU(DBG_INFO, "dpu_handle_offline           -\n");
}


static int dpu_event_handler(void *arg)
{
	struct timespec64 time;
	unsigned int hwduration;
	unsigned int total_duration;
	int i, ret;
	unsigned long idle_timeout = msecs_to_jiffies(IDLE_TIMEOUT_MS);
	unsigned long eof_timeout = msecs_to_jiffies(EOF_WAIT_TIMEOUT_MS);
	unsigned long hw_timeout = msecs_to_jiffies(hw_wait_time);
	static struct timespec64 pre_time = {0};
	dpu_dev = (struct dpu_dev_s *)arg;
	timeout = idle_timeout;
	//TRACE_DPU(DBG_INFO, "dpu_event_handler           +\n");
	while (!kthread_should_stop()) {
		ret = wait_event_interruptible_timeout(dpu_dev->wait,
			handler_ctx[0].events ||kthread_should_stop(),timeout);
		/* -%ERESTARTSYS */
		if (ret < 0 || kthread_should_stop())
			break;

		/* timeout */
		if (!ret && dpu_handler_is_idle()){
			timeout = idle_timeout;
			continue;
		}
		mutex_lock(&dpu_dev->suspend_lock);
		if(dpu_dev->bsuspend){
			timeout = idle_timeout;
			continue;
		}
		mutex_unlock(&dpu_dev->suspend_lock);

		if(handler_ctx[0].events == CTX_EVENT_EOF){
			ktime_get_ts64(&time);
			hwduration = get_diff_in_us(dpu_dev->time_start, time);
			total_duration = get_diff_in_us(pre_time,time);
			memset(&pre_time,0,sizeof(struct timespec64));
			dpu_dev->cost_time_for_sec += hwduration;
			dpu_dev->total_hwduration += hwduration;
			dpu_dev->duty_ratio = dpu_dev->total_hwduration *100 / total_duration;
			if(dpu_dev->cost_time_for_sec <=(1000*1000)){
				dpu_dev->int_time_per_sec += hwduration;
				dpu_dev->int_num_per_sec += 1;
			} else {
				dpu_dev->run_time_info.cost_tm_per_sec = dpu_dev->int_time_per_sec;
				dpu_dev->int_time_per_sec = 0;

				if(dpu_dev->run_time_info.max_cost_tm_per_sec \
				< dpu_dev->run_time_info.cost_tm_per_sec){
					dpu_dev->run_time_info.max_cost_tm_per_sec = \
					dpu_dev->run_time_info.cost_tm_per_sec;
				}

				dpu_dev->run_time_info.cnt_per_sec = dpu_dev->int_num_per_sec;
				dpu_dev->int_num_per_sec = 0;
				if(dpu_dev->run_time_info.max_cnt_per_sec \
				< dpu_dev->run_time_info.cnt_per_sec){
					dpu_dev->run_time_info.max_cnt_per_sec = \
					dpu_dev->run_time_info.cnt_per_sec;
				}

				dpu_dev->cost_time_for_sec = 0;
			}

			dpu_dev->run_time_info.total_int_cost_tm += hwduration;
			dpu_dev->run_time_info.total_int_cnt ++;
			dpu_dev->run_time_info.cost_tm = hwduration;
			if(dpu_dev->run_time_info.cost_tm > dpu_dev->run_time_info.max_cost_tm)
				dpu_dev->run_time_info.max_cost_tm = dpu_dev->run_time_info.cost_tm;
		}

		dpu_dev->bbusy = TRUE;

		TRACE_DPU(DBG_INFO, "ctx[0] state=%d, events=0x%x, bbusy=%d\n",
				handler_ctx[0].hdl_state, handler_ctx[0].events,dpu_dev->bbusy );

		handler_ctx[0].events &= ~CTX_EVENT_WKUP;
		for (i = 0; i < DPU_IP_NUM; i++) {
			if (handler_ctx[i].hdl_state != HANDLER_STATE_RUN)
				continue;
			if(!(&handler_ctx[i])){
				TRACE_DPU(DBG_INFO,"handler_ctx[%d] null ptr\n",i);
				continue;
			}
			dpu_handle_offline(&handler_ctx[i]);
			// check if there are still unfinished jobs
			if (handler_ctx[i].working_mask ==0) {
				handler_ctx[i].working_grp
					= find_next_en_grp(handler_ctx[i].working_grp, i);

				// unfinished job found, need to re-trig event handler
				if (handler_ctx[i].working_grp != DPU_MAX_GRP_NUM)
					handler_ctx[i].events = CTX_EVENT_WKUP;

				dpu_dev->bbusy =FALSE;

			}


		}
		TRACE_DPU(DBG_INFO, "dpu_dev->bbusy(%d) \n",dpu_dev->bbusy);
		TRACE_DPU(DBG_INFO, "dpu_handler_is_idle(%d) \n",dpu_handler_is_idle());
		/* Adjust timeout */
		timeout = dpu_handler_is_idle() ? idle_timeout : ((handler_ctx[0].working_mask == 1) ? hw_timeout:eof_timeout);
		if(!dpu_enable_handler_ctx(&handler_ctx[0])){
			handler_ctx[0].working_grp = DPU_MAX_GRP_NUM;
			handler_ctx[0].working_mask = 0;
			handler_ctx[0].events = 0;
		}
		TRACE_DPU(DBG_INFO, "timeout(%ld) \n",timeout);
		TRACE_DPU(DBG_INFO, "idle_timeout(%ld) \n",idle_timeout);
		TRACE_DPU(DBG_INFO, "eof_timeout(%ld) \n",eof_timeout);
		TRACE_DPU(DBG_INFO, "hw_timeout(%ld) \n",hw_timeout);

	}
	TRACE_DPU(DBG_INFO, "dpu_event_handler           -\n");
	return 0;
}


void dpu_start_handler(struct dpu_dev_s *dpu_dev)
{
	int ret;
	unsigned char dpu_dev_id;
	struct sched_param tsk;
	//TRACE_DPU(DBG_INFO, "dpu_start_handler          +\n");
	memset(&dpu_dev->run_time_info,0,sizeof(dpu_dev->run_time_info));
	dpu_dev->run_time_info.cnt_per_sec = 0;
	dpu_dev->run_time_info.cost_tm = 0;
	dpu_dev->run_time_info.cost_tm_per_frm = 0;
	dpu_dev->run_time_info.cost_tm_per_sec = 0;
	dpu_dev->run_time_info.hw_cost_tm_per_frm = 0;
	dpu_dev->run_time_info.max_cnt_per_sec = 0;
	dpu_dev->run_time_info.max_cost_tm = 0;
	dpu_dev->run_time_info.max_cost_tm_per_sec = 0;
	dpu_dev->run_time_info.run_tm = 0;
	dpu_dev->run_time_info.total_int_cnt = 0;
	dpu_dev->run_time_info.total_int_cnt_last_sec = 0;
	dpu_dev->run_time_info.total_int_cost_tm = 0;
	dpu_dev->duty_ratio = 0;
	dpu_dev->total_hwduration = 0;

	dpu_dev->time_for_sec = 0;
	dpu_dev->int_num_per_sec = 0;

	dpu_dev->bbusy =FALSE;
	dpu_dev->hw_busy = FALSE;
	for (dpu_dev_id = 0; dpu_dev_id < DPU_IP_NUM; dpu_dev_id++) {
		handler_ctx[dpu_dev_id].dpu_dev_id = dpu_dev_id;
		handler_ctx[dpu_dev_id].hdl_state = HANDLER_STATE_STOP;
		handler_ctx[dpu_dev_id].working_grp = DPU_MAX_GRP_NUM;
		handler_ctx[dpu_dev_id].working_mask = 0;
		handler_ctx[dpu_dev_id].events = 0;
		mutex_init(&handler_ctx[dpu_dev_id].mutex);

		//FIFO_INIT(&handler_ctx[dpu_dev_id].rgnex_jobs.jobq, 16);
	}

	// Same as sched_set_fifo in linux 5.x
	tsk.sched_priority = MAX_USER_RT_PRIO - 10;

	dpu_dev->thread = kthread_run(dpu_event_handler, dpu_dev,
		"cvitask_dpu_hdl");
	if (IS_ERR(dpu_dev->thread)) {
		pr_err("failed to create dpu kthread, dpu_dev_id=%d\n", dpu_dev_id);
	}

	ret = sched_setscheduler(dpu_dev->thread, SCHED_FIFO, &tsk);
	if (ret)
		pr_warn("dpu thread priority update failed: %d\n", ret);
}

int dpu_get_handle_info(struct dpu_dev_s *dpu_wdev, struct file *file,
		struct dpu_handle_info_s **f_list)
{
	struct dpu_handle_info_s *h_info;

	mutex_lock(&dpu_wdev->dpu_lock);

	list_for_each_entry(h_info, &dpu_wdev->handle_list, list) {
		if (h_info->file == file) {
			*f_list = h_info;
			mutex_unlock(&dpu_wdev->dpu_lock);
			return 0;
		}
	}
	mutex_unlock(&dpu_wdev->dpu_lock);
	return -EINVAL;
}

void dpu_mode_deinit(dpu_grp dpu_grp_id){

	mutex_lock(&dpu_get_grp_lock);
	dpu_grp_used[dpu_grp_id] = FALSE;
	mutex_unlock(&dpu_get_grp_lock);
}

void dpu_init(void *arg)
{
	// int ret;
	unsigned char i;
	// char chfh_ion_name[10] ="dpuChfh";
	if (!arg)
		return;

	dpu_dev = (struct dpu_dev_s *)arg;

	mutex_init(&dpu_dev->dpu_lock);
	mutex_init(&dpu_get_grp_lock);
	INIT_LIST_HEAD(&dpu_dev->handle_list);

	mutex_lock(&dpu_get_grp_lock);
	for(i = 0; i < DPU_MAX_GRP_NUM; ++i){
		dpu_grp_used[i] = FALSE;
	}
	mutex_unlock(&dpu_get_grp_lock);

	// SYS_Init()
	dpu_enable_clk();
	dpu_disable_clk();
	init_waitqueue_head(&dpu_dev->wait);
	init_waitqueue_head(&dpu_dev->reset_wait);
	init_waitqueue_head(&dpu_dev->send_frame_wait);
	dpu_dev->reset_done = FALSE;
// ret = base_ion_alloc(&dpu_dev->phyaddr_chfh, &dpu_dev->viraddr_chfh,
// (uint8_t *)chfh_ion_name, 1920*1080*5, FALSE);
// if (ret) {
// TRACE_DPU(DBG_ERR,"base_ion_alloc fail! ret(%d)\n", ret);
// return;
// }
	TRACE_DPU(DBG_INFO, "phyaddr_chfh(0x%llx)", dpu_dev->phyaddr_chfh);
	dpu_start_handler(dpu_dev);
}

void dpu_deinit(void *arg)
{
	int ret;
	if (!arg)
		return;

	dpu_dev = (struct dpu_dev_s *)arg;
	// base_ion_free(dpu_dev->phyaddr_chfh);
	// TRACE_DPU(DBG_INFO, "phyaddr_chfh(0x%llx)", dpu_dev->phyaddr_chfh);

	if (!dpu_dev->thread) {
		pr_err("dpu thread not initialized yet\n");
		return ;
	}

	mutex_destroy(&dpu_dev->dpu_lock);
	mutex_destroy(&dpu_get_grp_lock);
	mutex_destroy(&dpu_dev->suspend_lock);
	mutex_destroy(&dpu_dev->mutex);

	ret = kthread_stop(dpu_dev->thread);
	if (ret)
		pr_err("fail to stop dpu thread, err=%d\n", ret);

	dpu_dev->thread = NULL;
}

void dpu_check_reg_read(void)
{
    unsigned int reg_sys;
	unsigned int reg_00 ;
	unsigned int reg_04;
	unsigned int reg_08;
	unsigned int reg_0C;
	unsigned int reg_10;
	unsigned int reg_14;
	unsigned int reg_1C;
	unsigned int reg_20;
	unsigned int reg_24;
	unsigned int reg_28;
	unsigned int reg_2C;
	unsigned int reg_30;
	unsigned int reg_34;
	unsigned int reg_38;
	unsigned int reg_3C;
	unsigned int reg_40;
	unsigned int reg_44;
	unsigned int reg_48;
	unsigned int reg_4C;
	unsigned int reg_50;
	unsigned int reg_54;
	unsigned int reg_58;
	unsigned int reg_5C;
	unsigned int reg_60;
	unsigned int reg_64;
	unsigned int reg_68;
	unsigned int reg_6C;
	unsigned int reg_70;
	unsigned int reg_74;
	unsigned int reg_78;
	unsigned int reg_7C;
	unsigned int reg_80;
	unsigned int reg_84;
	unsigned int reg_88;
	unsigned int reg_90;
	unsigned int reg_94;
	unsigned int reg_98;
	unsigned int reg_9C;
	unsigned int reg_A0;
	unsigned int reg_dma_sgbm_ld1_base_addr;
	unsigned int reg_dma_sgbm_ld1_seglen;
	unsigned int reg_dma_sgbm_ld1_stride;
	unsigned int reg_dma_sgbm_ld1_segnum;
	unsigned int reg_dma_sgbm_ld2_base_addr;
	unsigned int reg_dma_sgbm_ld2_seglen;
	unsigned int reg_dma_sgbm_ld2_stride;
	unsigned int reg_dma_sgbm_ld2_segnum;
	unsigned int reg_dma_sgbm_bf_base_addr;
	unsigned int reg_dma_sgbm_bf_seglen;
	unsigned int reg_dma_sgbm_bf_stride;
	unsigned int reg_dma_sgbm_bf_segnum;
	unsigned int reg_dma_sgbm_mux_base_addr;
	unsigned int reg_dma_sgbm_mux_seglen;
	unsigned int reg_dma_sgbm_mux_stride;
	unsigned int reg_dma_sgbm_mux_segnum;

	unsigned int reg_dma_fgs_chfh_ld_base_addr;
	unsigned int reg_dma_fgs_chfh_ld_seglen;
	unsigned int reg_dma_fgs_chfh_ld_stride;
	unsigned int reg_dma_fgs_chfh_ld_segnum;
	unsigned int reg_dma_fgs_chfh_st_base_addr;
	unsigned int reg_dma_fgs_chfh_st_seglen;
	unsigned int reg_dma_fgs_chfh_st_stride;
	unsigned int reg_dma_fgs_chfh_st_segnum;
	unsigned int reg_dma_fgs_gx_ld_base_addr;
	unsigned int reg_dma_fgs_gx_ld_seglen;
	unsigned int reg_dma_fgs_gx_ld_stride;
	unsigned int reg_dma_fgs_gx_ld_segnum;
	unsigned int reg_dma_fgs_ux_st_base_addr;
	unsigned int reg_dma_fgs_ux_st_seglen;
	unsigned int reg_dma_fgs_ux_st_stride;
	unsigned int reg_dma_fgs_ux_st_segnum;
    TRACE_DPU(DBG_INFO,"[Print] Print dpu Reg configurations ...\n");
    reg_00 = read_reg(reg_base + DPU_REG_00_OFS);
    TRACE_DPU(DBG_INFO,"[Print][DPU][reg_00]:  %d\n", reg_00);
    reg_04 = read_reg(reg_base + DPU_REG_04_OFS);
    TRACE_DPU(DBG_INFO,"[Print][DPU][reg_04]:  %d\n", reg_04);
    reg_08 = read_reg(reg_base + DPU_REG_08_OFS);
    TRACE_DPU(DBG_INFO,"[Print][DPU][reg_08]:  %d\n", reg_08);
    reg_0C = read_reg(reg_base + DPU_REG_0C_OFS);
    TRACE_DPU(DBG_INFO,"[Print][DPU][reg_0C]:  %d\n", reg_0C);

    reg_10 = read_reg(reg_base + DPU_REG_10_OFS);
    TRACE_DPU(DBG_INFO,"[Print][DPU][reg_10]:  %d\n", reg_10);
    reg_14 = read_reg(reg_base + DPU_REG_14_OFS);
    TRACE_DPU(DBG_INFO,"[Print][DPU][reg_14]:  %d\n", reg_14);
    reg_1C = read_reg(reg_base + DPU_REG_1C_OFS);
    TRACE_DPU(DBG_INFO,"[Print][DPU][reg_1C]:  %d\n", reg_1C);

    reg_20 = read_reg(reg_base + DPU_REG_20_OFS);
    TRACE_DPU(DBG_INFO,"[Print][DPU][reg_20]:  %d\n", reg_20);
    reg_24 = read_reg(reg_base + DPU_REG_24_OFS);
    TRACE_DPU(DBG_INFO,"[Print][DPU][reg_24]:  %d\n", reg_24);
    reg_28 = read_reg(reg_base + DPU_REG_28_OFS);
    TRACE_DPU(DBG_INFO,"[Print][DPU][reg_28]:  %d\n", reg_28);
    reg_2C = read_reg(reg_base + DPU_REG_2C_OFS);
    TRACE_DPU(DBG_INFO,"[Print][DPU][reg_2C]:  %d\n", reg_2C);

    reg_30 = read_reg(reg_base + DPU_REG_30_OFS);
    TRACE_DPU(DBG_INFO,"[Print][DPU][reg_30]:  %d\n", reg_30);
    reg_34 = read_reg(reg_base + DPU_REG_34_OFS);
    TRACE_DPU(DBG_INFO,"[Print][DPU][reg_34]:  %d\n", reg_34);
    reg_38 = read_reg(reg_base + DPU_REG_38_OFS);
    TRACE_DPU(DBG_INFO,"[Print][DPU][reg_38]:  %d\n", reg_38);
    reg_3C = read_reg(reg_base + DPU_REG_3C_OFS);
    TRACE_DPU(DBG_INFO,"[Print][DPU][reg_3C]:  %d\n", reg_3C);

    reg_40 = read_reg(reg_base + DPU_REG_40_OFS);
    TRACE_DPU(DBG_INFO,"[Print][DPU][reg_40]:  %d\n", reg_40);
    reg_44 = read_reg(reg_base + DPU_REG_44_OFS);
    TRACE_DPU(DBG_INFO,"[Print][DPU][reg_44]:  %d\n", reg_44);
    reg_48 = read_reg(reg_base + DPU_REG_48_OFS);
    TRACE_DPU(DBG_INFO,"[Print][DPU][reg_48]:  %d\n", reg_48);
    reg_4C = read_reg(reg_base + DPU_REG_4C_OFS);
    TRACE_DPU(DBG_INFO,"[Print][DPU][reg_4C]:  %d\n", reg_4C);

    reg_50 = read_reg(reg_base + DPU_REG_50_OFS);
    TRACE_DPU(DBG_INFO,"[Print][DPU][reg_50]:  %d\n", reg_50);
    reg_54 = read_reg(reg_base + DPU_REG_54_OFS);
    TRACE_DPU(DBG_INFO,"[Print][DPU][reg_54]:  %d\n", reg_54);
    reg_58 = read_reg(reg_base + DPU_REG_58_OFS);
    TRACE_DPU(DBG_INFO,"[Print][DPU][reg_58]:  %d\n", reg_58);
    reg_5C = read_reg(reg_base + DPU_REG_5C_OFS);
    TRACE_DPU(DBG_INFO,"[Print][DPU][reg_5C]:  %d\n", reg_5C);

    reg_60 = read_reg(reg_base + DPU_REG_60_OFS);
    TRACE_DPU(DBG_INFO,"[Print][DPU][reg_60]:  %d\n", reg_60);
    reg_64 = read_reg(reg_base + DPU_REG_64_OFS);
    TRACE_DPU(DBG_INFO,"[Print][DPU][reg_64]:  %d\n", reg_64);
    reg_68 = read_reg(reg_base + DPU_REG_68_OFS);
    TRACE_DPU(DBG_INFO,"[Print][DPU][reg_64]:  %d\n", reg_64);
    reg_6C = read_reg(reg_base + DPU_REG_6C_OFS);
    TRACE_DPU(DBG_INFO,"[Print][DPU][reg_6C]:  %d\n", reg_6C);

    reg_70 = read_reg(reg_base + DPU_REG_70_OFS);
    TRACE_DPU(DBG_INFO,"[Print][DPU][reg_70]:  %d\n", reg_70);
    reg_74 = read_reg(reg_base + DPU_REG_74_OFS);
    TRACE_DPU(DBG_INFO,"[Print][DPU][reg_74]:  %d\n", reg_74);
    reg_78 = read_reg(reg_base + DPU_REG_78_OFS);
    TRACE_DPU(DBG_INFO,"[Print][DPU][reg_78]:  %d\n", reg_78);
    reg_7C = read_reg(reg_base + DPU_REG_7C_OFS);
    TRACE_DPU(DBG_INFO,"[Print][DPU][reg_7C]:  %d\n", reg_7C);

    reg_80 = read_reg(reg_base + DPU_REG_80_OFS);
    TRACE_DPU(DBG_INFO,"[Print][DPU][reg_80]:  %d\n", reg_80);
    reg_84 = read_reg(reg_base + DPU_REG_84_OFS);
    TRACE_DPU(DBG_INFO,"[Print][DPU][reg_84]:  %d\n", reg_84);
    reg_88 = read_reg(reg_base + DPU_REG_88_OFS);
    TRACE_DPU(DBG_INFO,"[Print][DPU][reg_88]:  %d\n", reg_88);

    reg_sys = read_reg(reg_base_sgbm_ld1_dma + SYS_CONTROL_OFS);
    TRACE_DPU(DBG_INFO,"[Print][DMA][SGBM_LD1][base_h]: 0x%x\n", get_mask(reg_sys,8,8));
    TRACE_DPU(DBG_INFO,"[Print][DMA][SGBM_LD1][base_sel]: 0x%x\n", get_mask(reg_sys,1,16));
    TRACE_DPU(DBG_INFO,"[Print][DMA][SGBM_LD1][stride_sel]: 0x%x\n", get_mask(reg_sys,1,17));
    TRACE_DPU(DBG_INFO,"[Print][DMA][SGBM_LD1][seglen_sel]: 0x%x\n", get_mask(reg_sys,1,18));
    TRACE_DPU(DBG_INFO,"[Print][DMA][SGBM_LD1][segnum_sel]: 0x%x\n", get_mask(reg_sys,1,19));
    reg_dma_sgbm_ld1_base_addr = read_reg(reg_base_sgbm_ld1_dma + DMA_BASE_ADDR_OFS);
    TRACE_DPU(DBG_INFO,"[Print][DMA][SGBM_LD1][base_addr]: 0x%x\n", reg_dma_sgbm_ld1_base_addr);
    reg_dma_sgbm_ld1_seglen = read_reg(reg_base_sgbm_ld1_dma + DMA_SEGLEN_OFS);
    TRACE_DPU(DBG_INFO,"[Print][DMA][SGBM_LD1][seg_len]:  %d\n", reg_dma_sgbm_ld1_seglen);
    reg_dma_sgbm_ld1_stride = read_reg(reg_base_sgbm_ld1_dma + DMA_STRIDE_OFS);
    TRACE_DPU(DBG_INFO,"[Print][DMA][SGBM_LD1][seg_stride]:  %d\n", reg_dma_sgbm_ld1_stride);
    reg_dma_sgbm_ld1_segnum = read_reg(reg_base_sgbm_ld1_dma + DMA_SEGNUM_OFS);
    TRACE_DPU(DBG_INFO,"[Print][DMA][SGBM_LD1][seg_num]:  %d\n", reg_dma_sgbm_ld1_segnum);

    reg_sys = read_reg(reg_base_sgbm_ld2_dma + SYS_CONTROL_OFS);
    TRACE_DPU(DBG_INFO,"[Print][DMA][SGBM_LD2][base_h]: 0x%x\n", get_mask(reg_sys,8,8));
    TRACE_DPU(DBG_INFO,"[Print][DMA][SGBM_LD2][base_sel]: 0x%x\n", get_mask(reg_sys,1,16));
    TRACE_DPU(DBG_INFO,"[Print][DMA][SGBM_LD2][stride_sel]: 0x%x\n", get_mask(reg_sys,1,17));
    TRACE_DPU(DBG_INFO,"[Print][DMA][SGBM_LD2][seglen_sel]: 0x%x\n", get_mask(reg_sys,1,18));
    TRACE_DPU(DBG_INFO,"[Print][DMA][SGBM_LD2][segnum_sel]: 0x%x\n", get_mask(reg_sys,1,19));
    reg_dma_sgbm_ld2_base_addr = read_reg(reg_base_sgbm_ld2_dma + DMA_BASE_ADDR_OFS);
    TRACE_DPU(DBG_INFO,"[Print][DMA][SGBM_LD2][base_addr]: 0x%x\n", reg_dma_sgbm_ld2_base_addr);
    reg_dma_sgbm_ld2_seglen = read_reg(reg_base_sgbm_ld2_dma + DMA_SEGLEN_OFS);
    TRACE_DPU(DBG_INFO,"[Print][DMA][SGBM_LD2][seg_len]:  %d\n", reg_dma_sgbm_ld2_seglen);
    reg_dma_sgbm_ld2_stride = read_reg(reg_base_sgbm_ld2_dma + DMA_STRIDE_OFS);
    TRACE_DPU(DBG_INFO,"[Print][DMA][SGBM_LD2][seg_stride]:  %d\n", reg_dma_sgbm_ld2_stride);
    reg_dma_sgbm_ld2_segnum = read_reg(reg_base_sgbm_ld2_dma + DMA_SEGNUM_OFS);
    TRACE_DPU(DBG_INFO,"[Print][DMA][SGBM_LD2][seg_num]:  %d\n", reg_dma_sgbm_ld2_segnum);

    reg_sys = read_reg(reg_base_sgbm_bf_dma + SYS_CONTROL_OFS);
    TRACE_DPU(DBG_INFO,"[Print][DMA][SGBM_BF][base_h]: 0x%x\n", get_mask(reg_sys,8,8));
    TRACE_DPU(DBG_INFO,"[Print][DMA][SGBM_BF][base_sel]: 0x%x\n", get_mask(reg_sys,1,16));
    TRACE_DPU(DBG_INFO,"[Print][DMA][SGBM_BF][stride_sel]: 0x%x\n", get_mask(reg_sys,1,17));
    TRACE_DPU(DBG_INFO,"[Print][DMA][SGBM_BF][seglen_sel]: 0x%x\n", get_mask(reg_sys,1,18));
    TRACE_DPU(DBG_INFO,"[Print][DMA][SGBM_BF][segnum_sel]: 0x%x\n", get_mask(reg_sys,1,19));
    reg_dma_sgbm_bf_base_addr = read_reg(reg_base_sgbm_bf_dma + DMA_BASE_ADDR_OFS);
    TRACE_DPU(DBG_INFO,"[Print][DMA][SGBM_BF][base_addr]: 0x%x\n", reg_dma_sgbm_bf_base_addr);
    reg_dma_sgbm_bf_seglen = read_reg(reg_base_sgbm_bf_dma + DMA_SEGLEN_OFS);
    TRACE_DPU(DBG_INFO,"[Print][DMA][SGBM_BF][seg_len]:  %d\n", reg_dma_sgbm_bf_seglen);
    reg_dma_sgbm_bf_stride = read_reg(reg_base_sgbm_bf_dma + DMA_STRIDE_OFS);
    TRACE_DPU(DBG_INFO,"[Print][DMA][SGBM_BF][seg_stride]:  %d\n", reg_dma_sgbm_bf_stride);
    reg_dma_sgbm_bf_segnum = read_reg(reg_base_sgbm_bf_dma + DMA_SEGNUM_OFS);
    TRACE_DPU(DBG_INFO,"[Print][DMA][SGBM_BF][seg_num]:  %d\n", reg_dma_sgbm_bf_segnum);

    reg_sys = read_reg(reg_base_sgbm_median_dma + SYS_CONTROL_OFS);
    TRACE_DPU(DBG_INFO,"[Print][DMA][SGBM_MUX][base_h]: 0x%x\n", get_mask(reg_sys,8,8));
    TRACE_DPU(DBG_INFO,"[Print][DMA][SGBM_MUX][base_sel]: 0x%x\n", get_mask(reg_sys,1,16));
    TRACE_DPU(DBG_INFO,"[Print][DMA][SGBM_MUX][stride_sel]: 0x%x\n", get_mask(reg_sys,1,17));
    TRACE_DPU(DBG_INFO,"[Print][DMA][SGBM_MUX][seglen_sel]: 0x%x\n", get_mask(reg_sys,1,18));
    TRACE_DPU(DBG_INFO,"[Print][DMA][SGBM_MUX][segnum_sel]: 0x%x\n", get_mask(reg_sys,1,19));
    reg_dma_sgbm_mux_base_addr = read_reg(reg_base_sgbm_median_dma + DMA_BASE_ADDR_OFS);
    TRACE_DPU(DBG_INFO,"[Print][DMA][SGBM_MUX][base_addr]: 0x%x\n", reg_dma_sgbm_mux_base_addr);
    reg_dma_sgbm_mux_seglen = read_reg(reg_base_sgbm_median_dma + DMA_SEGLEN_OFS);
    TRACE_DPU(DBG_INFO,"[Print][DMA][SGBM_MUX][seg_len]:  %d\n", reg_dma_sgbm_mux_seglen);
    reg_dma_sgbm_mux_stride = read_reg(reg_base_sgbm_median_dma + DMA_STRIDE_OFS);
    TRACE_DPU(DBG_INFO,"[Print][DMA][SGBM_MUX][seg_stride]:  %d\n", reg_dma_sgbm_mux_stride);
    reg_dma_sgbm_mux_segnum = read_reg(reg_base_sgbm_median_dma + DMA_SEGNUM_OFS);
    TRACE_DPU(DBG_INFO,"[Print][DMA][SGBM_MUX][seg_num]:  %d\n", reg_dma_sgbm_mux_segnum);

    reg_sys = read_reg(reg_base_fgs_chfh_ld_dma + SYS_CONTROL_OFS);
    TRACE_DPU(DBG_INFO,"[Print][DMA][FGS_CHFH_LD][base_h]: 0x%x\n", get_mask(reg_sys,8,8));
    TRACE_DPU(DBG_INFO,"[Print][DMA][FGS_CHFH_LD][base_sel]: 0x%x\n", get_mask(reg_sys,1,16));
    TRACE_DPU(DBG_INFO,"[Print][DMA][FGS_CHFH_LD][stride_sel]: 0x%x\n", get_mask(reg_sys,1,17));
    TRACE_DPU(DBG_INFO,"[Print][DMA][FGS_CHFH_LD][seglen_sel]: 0x%x\n", get_mask(reg_sys,1,18));
    TRACE_DPU(DBG_INFO,"[Print][DMA][FGS_CHFH_LD][segnum_sel]: 0x%x\n", get_mask(reg_sys,1,19));
    reg_dma_fgs_chfh_ld_base_addr = read_reg(reg_base_fgs_chfh_ld_dma + DMA_BASE_ADDR_OFS);
    TRACE_DPU(DBG_INFO,"[Print][DMA][FGS_CHFH_LD][base_addr]: 0x%x\n", reg_dma_fgs_chfh_ld_base_addr);
    reg_dma_fgs_chfh_ld_seglen = read_reg(reg_base_fgs_chfh_ld_dma + DMA_SEGLEN_OFS);
    TRACE_DPU(DBG_INFO,"[Print][DMA][FGS_CHFH_LD][seg_len]:  %d\n", reg_dma_fgs_chfh_ld_seglen);
    reg_dma_fgs_chfh_ld_stride = read_reg(reg_base_fgs_chfh_ld_dma + DMA_STRIDE_OFS);
    TRACE_DPU(DBG_INFO,"[Print][DMA][FGS_CHFH_LD][seg_stride]:  %d\n", reg_dma_fgs_chfh_ld_stride);
    reg_dma_fgs_chfh_ld_segnum = read_reg(reg_base_fgs_chfh_ld_dma + DMA_SEGNUM_OFS);
    TRACE_DPU(DBG_INFO,"[Print][DMA][FGS_CHFH_LD][seg_num]:  %d\n", reg_dma_fgs_chfh_ld_segnum);

    reg_sys = read_reg(reg_base_fgs_chfh_st_dma + SYS_CONTROL_OFS);
    TRACE_DPU(DBG_INFO,"[Print][DMA][FGS_CHFH_ST][base_h]: 0x%x\n", get_mask(reg_sys,8,8));
    TRACE_DPU(DBG_INFO,"[Print][DMA][FGS_CHFH_ST][base_sel]: 0x%x\n", get_mask(reg_sys,1,16));
    TRACE_DPU(DBG_INFO,"[Print][DMA][FGS_CHFH_ST][stride_sel]: 0x%x\n", get_mask(reg_sys,1,17));
    TRACE_DPU(DBG_INFO,"[Print][DMA][FGS_CHFH_ST][seglen_sel]: 0x%x\n", get_mask(reg_sys,1,18));
    TRACE_DPU(DBG_INFO,"[Print][DMA][FGS_CHFH_ST][segnum_sel]: 0x%x\n", get_mask(reg_sys,1,19));
    reg_dma_fgs_chfh_st_base_addr = read_reg(reg_base_fgs_chfh_st_dma + DMA_BASE_ADDR_OFS);
    TRACE_DPU(DBG_INFO,"[Print][DMA][FGS_CHFH_ST][base_addr]: 0x%x\n", reg_dma_fgs_chfh_st_base_addr);
    reg_dma_fgs_chfh_st_seglen = read_reg(reg_base_fgs_chfh_st_dma + DMA_SEGLEN_OFS);
    TRACE_DPU(DBG_INFO,"[Print][DMA][FGS_CHFH_ST][seg_len]:  %d\n", reg_dma_fgs_chfh_st_seglen);
    reg_dma_fgs_chfh_st_stride = read_reg(reg_base_fgs_chfh_st_dma + DMA_STRIDE_OFS);
    TRACE_DPU(DBG_INFO,"[Print][DMA][FGS_CHFH_ST][seg_stride]:  %d\n", reg_dma_fgs_chfh_st_stride);
    reg_dma_fgs_chfh_st_segnum = read_reg(reg_base_fgs_chfh_st_dma + DMA_SEGNUM_OFS);
    TRACE_DPU(DBG_INFO,"[Print][DMA][FGS_CHFH_ST][seg_num]:  %d\n", reg_dma_fgs_chfh_st_segnum);

    reg_sys = read_reg(reg_base_fgs_gx_dma + SYS_CONTROL_OFS);
    TRACE_DPU(DBG_INFO,"[Print][DMA][FGS_GX_LD][base_h]: 0x%x\n", get_mask(reg_sys,8,8));
    TRACE_DPU(DBG_INFO,"[Print][DMA][FGS_GX_LD][base_sel]: 0x%x\n", get_mask(reg_sys,1,16));
    TRACE_DPU(DBG_INFO,"[Print][DMA][FGS_GX_LD][stride_sel]: 0x%x\n", get_mask(reg_sys,1,17));
    TRACE_DPU(DBG_INFO,"[Print][DMA][FGS_GX_LD][seglen_sel]: 0x%x\n", get_mask(reg_sys,1,18));
    TRACE_DPU(DBG_INFO,"[Print][DMA][FGS_GX_LD][segnum_sel]: 0x%x\n", get_mask(reg_sys,1,19));
    reg_dma_fgs_gx_ld_base_addr = read_reg(reg_base_fgs_gx_dma + DMA_BASE_ADDR_OFS);
    TRACE_DPU(DBG_INFO,"[Print][DMA][FGS_GX_LD][base_addr]: 0x%x\n", reg_dma_fgs_gx_ld_base_addr);
    reg_dma_fgs_gx_ld_seglen = read_reg(reg_base_fgs_gx_dma + DMA_SEGLEN_OFS);
    TRACE_DPU(DBG_INFO,"[Print][DMA][FGS_GX_LD][seg_len]:  %d\n", reg_dma_fgs_gx_ld_seglen);
    reg_dma_fgs_gx_ld_stride = read_reg(reg_base_fgs_gx_dma + DMA_STRIDE_OFS);
    TRACE_DPU(DBG_INFO,"[Print][DMA][FGS_GX_LD][seg_stride]:  %d\n", reg_dma_fgs_gx_ld_stride);
    reg_dma_fgs_gx_ld_segnum = read_reg(reg_base_fgs_gx_dma + DMA_SEGNUM_OFS);
    TRACE_DPU(DBG_INFO,"[Print][DMA][FGS_GX_LD][seg_num]:  %d\n", reg_dma_fgs_gx_ld_segnum);

    reg_sys = read_reg(	reg_base_fgs_ux_dma + SYS_CONTROL_OFS);
    TRACE_DPU(DBG_INFO,"[Print][DMA][FGS_UX_ST][base_h]: 0x%x\n", get_mask(reg_sys,8,8));
    TRACE_DPU(DBG_INFO,"[Print][DMA][FGS_UX_ST][base_sel]: 0x%x\n", get_mask(reg_sys,1,16));
    TRACE_DPU(DBG_INFO,"[Print][DMA][FGS_UX_ST][stride_sel]: 0x%x\n", get_mask(reg_sys,1,17));
    TRACE_DPU(DBG_INFO,"[Print][DMA][FGS_UX_ST][seglen_sel]: 0x%x\n", get_mask(reg_sys,1,18));
    TRACE_DPU(DBG_INFO,"[Print][DMA][FGS_UX_ST][segnum_sel]: 0x%x\n", get_mask(reg_sys,1,19));
    reg_dma_fgs_ux_st_base_addr = read_reg(reg_base_fgs_ux_dma + DMA_BASE_ADDR_OFS);
    TRACE_DPU(DBG_INFO,"[Print][DMA][FGS_UX_ST][base_addr]: 0x%x\n", reg_dma_fgs_ux_st_base_addr);
    reg_dma_fgs_ux_st_seglen = read_reg(reg_base_fgs_ux_dma + DMA_SEGLEN_OFS);
    TRACE_DPU(DBG_INFO,"[Print][DMA][FGS_UX_ST][seg_len]:  %d\n", reg_dma_fgs_ux_st_seglen);
    reg_dma_fgs_ux_st_stride = read_reg(reg_base_fgs_ux_dma + DMA_STRIDE_OFS);
    TRACE_DPU(DBG_INFO,"[Print][DMA][FGS_UX_ST][seg_stride]:  %d\n", reg_dma_fgs_ux_st_stride);
    reg_dma_fgs_ux_st_segnum = read_reg(reg_base_fgs_ux_dma + DMA_SEGNUM_OFS);
    TRACE_DPU(DBG_INFO,"[Print][DMA][FGS_UX_ST][seg_num]:  %d\n", reg_dma_fgs_ux_st_segnum);

    //reg00
    TRACE_DPU(DBG_INFO,"[Print][DPU][reg_dpu_enable]:  %d\n", get_mask(reg_00, 1, 0));
    TRACE_DPU(DBG_INFO,"[Print][DPU][reg_dpu_debug_mode]:  %d\n", get_mask(reg_00, 4, 4));
    //reg04
    TRACE_DPU(DBG_INFO,"[Print][DPU][reg_sgbm_img_width]:  %d\n", get_mask(reg_04, 16, 0));
    TRACE_DPU(DBG_INFO,"[Print][DPU][reg_sgbm_img_height]:  %d\n", get_mask(reg_04, 16, 16));
    //reg08
    TRACE_DPU(DBG_INFO,"[Print][DPU][reg_dpu_min_d]:  %d\n", get_mask(reg_08, 8, 0));
    //reg0C
    TRACE_DPU(DBG_INFO,"[Print][DPU][reg_dpu_rshift1]:  %d\n", get_mask(reg_0C, 3, 8));
    TRACE_DPU(DBG_INFO,"[Print][DPU][reg_dpu_rshift2]:  %d\n", get_mask(reg_0C, 3, 12));
    //reg10
    TRACE_DPU(DBG_INFO,"[Print][DPU][reg_dpu_ca_p1]:  %d\n", get_mask(reg_10, 16, 0));
    TRACE_DPU(DBG_INFO,"[Print][DPU][reg_dpu_ca_p2]:  %d\n", get_mask(reg_10, 16, 16));
    //reg14
    TRACE_DPU(DBG_INFO,"[Print][DPU][reg_dpu_uniq_ratio]:  %d\n", get_mask(reg_14, 8, 0));
    TRACE_DPU(DBG_INFO,"[Print][DPU][reg_dpu_disp_shift]:  %d\n", get_mask(reg_14, 4, 8));
    TRACE_DPU(DBG_INFO,"[Print][DPU][reg_dpu_bfw_size]:  %d\n", get_mask(reg_14, 2, 16));
    TRACE_DPU(DBG_INFO,"[Print][DPU][reg_dpu_census_shift]:  %d\n", get_mask(reg_14, 8, 18));
    //reg1C
    TRACE_DPU(DBG_INFO,"[Print][DPU][reg_dpu_fgs_max_count]:  %d\n", get_mask(reg_1C, 5, 0));
    TRACE_DPU(DBG_INFO,"[Print][DPU][reg_dpu_fgs_w_scale]:  %d\n", get_mask(reg_1C, 4, 8));
    TRACE_DPU(DBG_INFO,"[Print][DPU][reg_dpu_fgs_c_scale]:  %d\n", get_mask(reg_1C, 4, 12));
    TRACE_DPU(DBG_INFO,"[Print][DPU][reg_dpu_fgs_f_scale]:  %d\n", get_mask(reg_1C, 4, 16));
    TRACE_DPU(DBG_INFO,"[Print][DPU][reg_sgbm_bf_dma_en]:  %d\n", get_mask(reg_1C, 1, 20));
    TRACE_DPU(DBG_INFO,"[Print][DPU][reg_sgbm_ld1_dma_en]:  %d\n", get_mask(reg_1C, 1, 21));
    TRACE_DPU(DBG_INFO,"[Print][DPU][reg_sgbm_ld2_dma_en]:  %d\n", get_mask(reg_1C, 1, 22));
    TRACE_DPU(DBG_INFO,"[Print][DPU][reg_sgbm_mux_dma_en]:  %d\n", get_mask(reg_1C, 1, 23));
    TRACE_DPU(DBG_INFO,"[Print][DPU][reg_fgs1_dma_en]:  %d\n", get_mask(reg_1C, 1, 24));
    TRACE_DPU(DBG_INFO,"[Print][DPU][reg_fgs2_dma_en]:  %d\n", get_mask(reg_1C, 1, 25));
    TRACE_DPU(DBG_INFO,"[Print][DPU][reg_fgs3_dma_en]:  %d\n", get_mask(reg_1C, 1, 26));
    TRACE_DPU(DBG_INFO,"[Print][DPU][reg_fgs4_dma_en]:  %d\n", get_mask(reg_1C, 1, 27));
    TRACE_DPU(DBG_INFO,"[Print][DPU][reg_sgbm_bf_crop_en]:  %d\n", get_mask(reg_1C, 1, 29));
    TRACE_DPU(DBG_INFO,"[Print][DPU][reg_sgbm_ld1_crop_en]:  %d\n", get_mask(reg_1C, 1, 30));
    TRACE_DPU(DBG_INFO,"[Print][DPU][reg_sgbm_ld2_crop_en]:  %d\n", get_mask(reg_1C, 1, 31));
    //reg20
    TRACE_DPU(DBG_INFO,"[Print][DPU][reg_sgbm_bf_crop_h_en]:  %d\n", get_mask(reg_20, 14, 0));
    TRACE_DPU(DBG_INFO,"[Print][DPU][reg_sgbm_bf_crop_h_str]:  %d\n", get_mask(reg_20, 14, 16));
    //reg24
    TRACE_DPU(DBG_INFO,"[Print][DPU][reg_sgbm_bf_crop_w_en]:  %d\n", get_mask(reg_24, 14, 0));
    TRACE_DPU(DBG_INFO,"[Print][DPU][reg_sgbm_bf_crop_w_str]:  %d\n", get_mask(reg_24, 14, 16));
    //reg28
    TRACE_DPU(DBG_INFO,"[Print][DPU][reg_sgbm_bf_crop_height]:  %d\n", get_mask(reg_28, 14, 0));
    TRACE_DPU(DBG_INFO,"[Print][DPU][reg_sgbm_bf_crop_width]:  %d\n", get_mask(reg_28, 14, 16));
    //reg2C
    TRACE_DPU(DBG_INFO,"[Print][DPU][reg_sgbm_ld_crop_h_en]:  %d\n", get_mask(reg_2C, 14, 0));
    TRACE_DPU(DBG_INFO,"[Print][DPU][reg_sgbm_ld_crop_h_str]:  %d\n", get_mask(reg_2C, 14, 16));
    //reg30
    TRACE_DPU(DBG_INFO,"[Print][DPU][reg_sgbm_ld_crop_w_en]:  %d\n", get_mask(reg_30, 14, 0));
    TRACE_DPU(DBG_INFO,"[Print][DPU][reg_sgbm_ld_crop_w_str]:  %d\n", get_mask(reg_30, 14, 16));
    //reg34
    TRACE_DPU(DBG_INFO,"[Print][DPU][reg_sgbm_ld_crop_height]:  %d\n", get_mask(reg_34, 14, 0));
    TRACE_DPU(DBG_INFO,"[Print][DPU][reg_sgbm_ld_crop_width]:  %d\n", get_mask(reg_34, 14, 16));
    //reg38
    TRACE_DPU(DBG_INFO,"[Print][DPU][reg_sgbm_mux_crop_h_en]:  %d\n", get_mask(reg_38, 14, 0));
    TRACE_DPU(DBG_INFO,"[Print][DPU][reg_sgbm_mux_crop_h_str]:  %d\n", get_mask(reg_38, 14, 16));
    //reg3C
    TRACE_DPU(DBG_INFO,"[Print][DPU][reg_sgbm_mux_crop_w_en]:  %d\n", get_mask(reg_3C, 14, 0));
    TRACE_DPU(DBG_INFO,"[Print][DPU][reg_sgbm_mux_crop_w_str]:  %d\n", get_mask(reg_3C, 14, 16));
    //reg40
    TRACE_DPU(DBG_INFO,"[Print][DPU][reg_sgbm_mux_crop_height]:  %d\n", get_mask(reg_40, 14, 0));
    TRACE_DPU(DBG_INFO,"[Print][DPU][reg_sgbm_mux_crop_width]:  %d\n", get_mask(reg_40, 14, 16));
    //reg44
    TRACE_DPU(DBG_INFO,"[Print][DPU][reg_fgs_chfh_crop_h_en]:  %d\n", get_mask(reg_44, 14, 0));
    TRACE_DPU(DBG_INFO,"[Print][DPU][reg_fgs_chfh_crop_h_str]:  %d\n", get_mask(reg_44, 14, 16));
    //reg48
    TRACE_DPU(DBG_INFO,"[Print][DPU][reg_fgs_chfh_crop_w_en]:  %d\n", get_mask(reg_48, 14, 0));
    TRACE_DPU(DBG_INFO,"[Print][DPU][reg_fgs_chfh_crop_w_str]:  %d\n", get_mask(reg_48, 14, 16));
    //reg4C
    TRACE_DPU(DBG_INFO,"[Print][DPU][reg_fgs_chfh_crop_height]:  %d\n", get_mask(reg_4C, 14, 0));
    TRACE_DPU(DBG_INFO,"[Print][DPU][reg_fgs_chfh_crop_width]:  %d\n", get_mask(reg_4C, 14, 16));
    //reg50
    TRACE_DPU(DBG_INFO,"[Print][DPU][reg_fgs_gx_crop_h_en]:  %d\n", get_mask(reg_50, 14, 0));
    TRACE_DPU(DBG_INFO,"[Print][DPU][reg_fgs_gx_crop_h_str]:  %d\n", get_mask(reg_50, 14, 16));
    //reg54
    TRACE_DPU(DBG_INFO,"[Print][DPU][reg_fgs_gx_crop_w_en]:  %d\n", get_mask(reg_54, 14, 0));
    TRACE_DPU(DBG_INFO,"[Print][DPU][reg_fgs_gx_crop_w_str]:  %d\n", get_mask(reg_54, 14, 16));
    //reg58
    TRACE_DPU(DBG_INFO,"[Print][DPU][reg_fgs_gx_crop_height]:  %d\n", get_mask(reg_58, 14, 0));
    TRACE_DPU(DBG_INFO,"[Print][DPU][reg_fgs_gx_crop_width]:  %d\n", get_mask(reg_58, 14, 16));
    //reg5C
    TRACE_DPU(DBG_INFO,"[Print][DPU][reg_fgs_ux_crop_h_en]:  %d\n", get_mask(reg_5C, 14, 0));
    TRACE_DPU(DBG_INFO,"[Print][DPU][reg_fgs_ux_crop_h_str]:  %d\n", get_mask(reg_5C, 14, 16));
    //reg60
    TRACE_DPU(DBG_INFO,"[Print][DPU][reg_fgs_ux_crop_w_en]:  %d\n", get_mask(reg_60, 14, 0));
    TRACE_DPU(DBG_INFO,"[Print][DPU][reg_fgs_ux_crop_w_str]:  %d\n", get_mask(reg_60, 14, 16));
    //reg64
    TRACE_DPU(DBG_INFO,"[Print][DPU][reg_fgs_ux_crop_height]:  %d\n", get_mask(reg_64, 14, 0));
    TRACE_DPU(DBG_INFO,"[Print][DPU][reg_fgs_ux_crop_width]:  %d\n", get_mask(reg_64, 14, 16));
    //reg68
    TRACE_DPU(DBG_INFO,"[Print][DPU][reg_dpu_sgbm_frame_done]:  %d\n", get_mask(reg_68, 1, 1));
    TRACE_DPU(DBG_INFO,"[Print][DPU][reg_sgbm_mux_crop_en]:  %d\n", get_mask(reg_68, 1, 2));
    TRACE_DPU(DBG_INFO,"[Print][DPU][reg_fgs_chfh_crop_en]:  %d\n", get_mask(reg_68, 1, 3));
    TRACE_DPU(DBG_INFO,"[Print][DPU][reg_fgs_gx_crop_en]:  %d\n", get_mask(reg_68, 1, 4));
    TRACE_DPU(DBG_INFO,"[Print][DPU][reg_fgs_ux_crop_en]:  %d\n", get_mask(reg_68, 1, 5));
    TRACE_DPU(DBG_INFO,"[Print][DPU][reg_dpu_reset_done]:  %d\n", get_mask(reg_68, 2, 7));
    TRACE_DPU(DBG_INFO,"[Print][DPU][reg_dpu_sgbm_enable]:  %d\n", get_mask(reg_68, 1, 12));
    TRACE_DPU(DBG_INFO,"[Print][DPU][reg_dpu_fgs_enable]:  %d\n", get_mask(reg_68, 1, 13));
    TRACE_DPU(DBG_INFO,"[Print][DPU][reg_dpu_sgbm2fgs_online]:  %d\n", get_mask(reg_68, 1, 16));
	//reg68
	TRACE_DPU(DBG_INFO,"[Print][DPU][reg_dpu_interrupt_sel]:  %d\n", get_mask(reg_6C, 8, 24));
    //reg70
    TRACE_DPU(DBG_INFO,"[Print][DPU][reg_dpu_img_width_fgs]:  %d\n", get_mask(reg_70, 16, 0));
    TRACE_DPU(DBG_INFO,"[Print][DPU][reg_dpu_img_height_fgs]:  %d\n", get_mask(reg_70, 16, 16));
    //reg74
    TRACE_DPU(DBG_INFO,"[Print][DPU][reg_dpu_nd_ds_fgs]:  %d\n", get_mask(reg_74, 8, 0));
    TRACE_DPU(DBG_INFO,"[Print][DPU][reg_dpu_fxbaseline_fgs]:  %d\n", get_mask(reg_74, 20, 8));
    TRACE_DPU(DBG_INFO,"[Print][DPU][reg_dpu_output_bit_choose]:  %d\n", get_mask(reg_74, 1, 28));
    TRACE_DPU(DBG_INFO,"[Print][DPU][reg_dpu_disp_range]:  %d\n", get_mask(reg_74, 3, 29));
    //reg78
    TRACE_DPU(DBG_INFO,"[Print][DPU][reg_dpu_dcc_a234]:  %d\n", get_mask(reg_78, 3, 0));
    TRACE_DPU(DBG_INFO,"[Print][DPU][reg_dpu_invalid_def]:  %d\n", get_mask(reg_78, 8, 8));
    TRACE_DPU(DBG_INFO,"[Print][DPU][reg_dpu_src_disp_mux]:  %d\n", get_mask(reg_78, 2, 16));
    TRACE_DPU(DBG_INFO,"[Print][DPU][reg_dpu_fgs_max_t]:  %d\n", get_mask(reg_78, 7, 18));
    //reg7C
    TRACE_DPU(DBG_INFO,"[Print][DPU][reg_fgs_independent_crop_h_en]:  %d\n", get_mask(reg_7C, 14, 0));
    TRACE_DPU(DBG_INFO,"[Print][DPU][reg_fgs_independent_crop_h_str]:  %d\n", get_mask(reg_7C, 14, 16));
    //reg80
    TRACE_DPU(DBG_INFO,"[Print][DPU][reg_fgs_independent_crop_w_en]:  %d\n", get_mask(reg_80, 14, 0));
    TRACE_DPU(DBG_INFO,"[Print][DPU][reg_fgs_independent_crop_w_str]:  %d\n", get_mask(reg_80, 14, 16));
    //reg84
    TRACE_DPU(DBG_INFO,"[Print][DPU][reg_fgs_independent_crop_height]:  %d\n", get_mask(reg_84, 14, 0));
    TRACE_DPU(DBG_INFO,"[Print][DPU][reg_fgs_independent_crop_width]:  %d\n", get_mask(reg_84, 14, 16));
    //reg88
    TRACE_DPU(DBG_INFO,"[Print][DPU][reg_fgs_independent_crop_en]:  %d\n", get_mask(reg_88, 1, 0));
    TRACE_DPU(DBG_INFO,"[Print][DPU][reg_sgbm_data_sel]:  %d\n", get_mask(reg_88, 2, 1));
    TRACE_DPU(DBG_INFO,"[Print][DPU][reg_sgbm_bf_dma_en]:  %d\n", get_mask(reg_88, 1, 3));
    //reg90
    reg_90 = read_reg(reg_base + DPU_REG_90_OFS);
    TRACE_DPU(DBG_INFO,"[Print][DPU][reg_interrupt_mask_sgbm]:  %d\n", reg_90);
    //reg94
    reg_94 = read_reg(reg_base + DPU_REG_94_OFS);
    TRACE_DPU(DBG_INFO,"[Print][DPU][reg_interrupt_mask_fgs]:  %d\n", reg_94);
    //reg98
    reg_98 = read_reg(reg_base + DPU_REG_98_OFS);
    TRACE_DPU(DBG_INFO,"[Print][DPU][reg_shadow_int_mask_sgbm]:  %d\n", get_mask(reg_98, 1, 0));
    TRACE_DPU(DBG_INFO,"[Print][DPU][reg_shadow_int_mask_fgs]:  %d\n", get_mask(reg_98, 1, 1));
    TRACE_DPU(DBG_INFO,"[Print][DPU][reg_shadow_done_sgbm]:  %d\n", get_mask(reg_98, 1, 4));
    TRACE_DPU(DBG_INFO,"[Print][DPU][reg_shadow_done_fgs]:  %d\n", get_mask(reg_98, 1, 5));
    TRACE_DPU(DBG_INFO,"[Print][DPU][reg_fgs_ux_store_base_h]:  %d\n", get_mask(reg_98, 4, 8));
    TRACE_DPU(DBG_INFO,"[Print][DPU][reg_fgs_chfh_store_base_h]:  %d\n", get_mask(reg_98, 4, 12));
    TRACE_DPU(DBG_INFO,"[Print][DPU][reg_dpu_output_unit_choose]:  %d\n", get_mask(reg_98, 2, 16));
    //reg9C
    reg_9C = read_reg(reg_base + DPU_REG_9C_OFS);
    TRACE_DPU(DBG_INFO,"[Print][DPU][reg_fgs_ux_store_base_l]:  %x\n", reg_9C);
    //regA0
    reg_A0 = read_reg(reg_base + DPU_REG_A0_OFS);
    TRACE_DPU(DBG_INFO,"[Print][DPU][reg_fgs_chfh_store_base_l]:  %x\n", reg_A0);

    TRACE_DPU(DBG_INFO,"[Print]Print dpu Reg Done ...\n");
}

void dpu_write_reg_init(void)
{
    dpu_reg.reg_dpu_enable = 1;
    dpu_reg.reg_dpu_sgbm_enable = 1;
    dpu_reg.reg_dpu_img_width = 64;
    dpu_reg.reg_dpu_img_height = 64;
    dpu_reg.reg_dpu_min_d = 1;
    dpu_reg.reg_dpu_rshift1 = 1;
    dpu_reg.reg_dpu_rshift2 = 1;
    dpu_reg.reg_dpu_ca_p1 = 1;
    dpu_reg.reg_dpu_ca_p2 = 1;
    dpu_reg.reg_dpu_uniq_ratio = 1;
    dpu_reg.reg_dpu_disp_shift = 1;
    dpu_reg.reg_dpu_bfw_size = 1;
    dpu_reg.reg_dpu_census_shift = 1;

    dpu_reg.reg_dpu_nd_ds = 1;
    dpu_reg.reg_dpu_fxbaseline = 1;
    dpu_reg.reg_dpu_disp_range = 1;
    dpu_reg.reg_dpu_dcc_a234 =1;
    dpu_reg.reg_dpu_invalid_def =1;
    dpu_reg.reg_dpu_src_disp_mux =1;
    dpu_reg.reg_dpu_data_sel=1;
    //dpu_reg.reg_dpu_fgs_enable=1;

    dpu_reg.reg_dpu_fgs_enable = 1;
    dpu_reg.reg_dpu_fgs_max_count = 1;
    dpu_reg.reg_dpu_fgs_max_t = 1;
    dpu_reg.reg_dpu_fgs_img_width =64;
    dpu_reg.reg_dpu_fgs_img_height =64;
    dpu_reg.reg_dpu_fgs_output_bit_choose =1;
    dpu_reg.reg_dpu_fgs_output_unit_choose =1;

    dpu_reg.reg_sgbm_bf_st_dma_enable = 1;
    dpu_reg.reg_sgbm_ld1_dma_enable=1;
    dpu_reg.reg_sgbm_ld2_dma_enable=1;
    dpu_reg.reg_sgbm_mux_st_dma_enable = 1;
    dpu_reg.reg_dma_enable_fgs1 = 1;
    dpu_reg.reg_dma_enable_fgs2 = 1;
    dpu_reg.reg_dma_enable_fgs3 = 1;
    dpu_reg.reg_dma_enable_fgs4 = 1;
    dpu_reg.reg_sgbm_bf_st_crop_enable=1;
    dpu_reg.reg_sgbm_ld1_crop_enable=1;
    dpu_reg.reg_sgbm_ld2_crop_enable=1;
    dpu_reg.reg_sgbm_mux_st_crop_enable =1;

    dpu_reg.reg_crop_enable_fgs_independent=1;
    dpu_reg.reg_crop_enable_fgs_chfh=1;
    dpu_reg.reg_crop_enable_fgs_gx=1;
    dpu_reg.reg_crop_enable_fgs_ux=1;
    dpu_reg.reg_sgbm2fgs_online=1;
}

void dpu_check_reg_write(void)
{
	unsigned int seg_len;
	unsigned int seg_num;
	dpu_write_reg_init();
	dpu_write_sgbm_all_reg();
	dpu_write_fgs_all_reg();
	seg_len= dpu_reg.reg_dpu_img_width;
	seg_num= dpu_reg.reg_dpu_img_height;
	register_sgbm_ld1_ld(seg_len, seg_num,dram_base_left_h, \
						dram_base_left_l,reg_base_sgbm_ld1_dma, reg_base);
	register_sgbm_ld2_ld(seg_len, seg_num,dram_base_right_h, \
						dram_base_right_l,reg_base_sgbm_ld2_dma, reg_base);
	register_sgbm_median_st_ld(seg_len, seg_num,dpu_reg.reg_dpu_data_sel,\
								dram_base_out_h,dram_base_out_l,reg_base_sgbm_median_dma, reg_base);
	register_sgbm_bf_st_ld(seg_len, seg_num,dram_base_out_btcost_h, \
						dram_base_out_btcost_l,reg_base_sgbm_bf_dma, reg_base);

	seg_len= dpu_reg.reg_dpu_fgs_img_width;
	seg_num= dpu_reg.reg_dpu_fgs_img_height;
	register_fgs_gx_ld(seg_len, seg_num,dram_base_left_h, \
						dram_base_left_l,reg_base_fgs_gx_dma, reg_base);
	register_fgs_chfh_ld(seg_len, seg_num,dram_base_right_h, \
						dram_base_right_l,reg_base_fgs_chfh_ld_dma, reg_base);
	register_fgs_chfh_st(seg_len, seg_num, dram_base_chfh_h, \
						dram_base_chfh_l,reg_base_fgs_chfh_st_dma, reg_base);
	register_fgs_ux_st(seg_len, seg_num,dpu_reg.reg_dpu_fgs_output_bit_choose,\
						dram_base_out_h, dram_base_out_l,reg_base_fgs_ux_dma, reg_base);

	dpu_check_reg_read();
}
