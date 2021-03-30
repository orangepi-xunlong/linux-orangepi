
#ifndef __CSIC__DMA__REG__H__
#define __CSIC__DMA__REG__H__

#include <linux/types.h>

#define MAX_CSIC_DMA_NUM 4

/*register value*/

/*
 * output data format
 */
enum output_fmt {
	/* only when input is raw */
	FIELD_RAW_8 = 0,
	FIELD_RAW_10 = 1,
	FIELD_RAW_12 = 2,
	FIELD_RGB565 = 4,
	FIELD_RGB888 = 5,
	FIELD_PRGB888 = 6,
	FRAME_RAW_8 = 8,
	FRAME_RAW_10 = 9,
	FRAME_RAW_12 = 10,
	FRAME_RGB565 = 12,
	FRAME_RGB888 = 13,
	FRAME_PRGB888 = 14,

	/* only when input is yuv422/yuv420 */
	FIELD_PLANAR_YUV422 = 0,
	FIELD_PLANAR_YUV420 = 1,
	FRAME_PLANAR_YUV420 = 2,
	FRAME_PLANAR_YUV422 = 3,
	FIELD_UV_CB_YUV422 = 4,
	FIELD_UV_CB_YUV420 = 5,
	FRAME_UV_CB_YUV420 = 6,
	FRAME_UV_CB_YUV422 = 7,
	FIELD_VU_CB_YUV422 = 8,
	FIELD_VU_CB_YUV420 = 9,
	FRAME_VU_CB_YUV420 = 10,
	FRAME_VU_CB_YUV422 = 11,
	FIELD_UV_CB_YUV422_10 = 12,
	FIELD_UV_CB_YUV420_10 = 13,
	FIELD_VU_CB_YUV422_10 = 14,
	FIELD_VU_CB_YUV420_10 = 15,
#if 0
	/* only when input is yuv420 */
	FIELD_PLANAR_YUV420 = 1,
	FRAME_PLANAR_YUV420 = 2,
	FIELD_UV_CB_YUV420 = 5,
	FRAME_UV_CB_YUV420 = 6,
	FIELD_VU_CB_YUV420 = 9,
	FRAME_VU_CB_YUV420 = 10,
	FIELD_UV_CB_YUV420_10 = 13,
	FIELD_VU_CB_YUV420_10 = 15,
#endif
};

/*
 * input field selection, only when input is ccir656
 */
enum field_sel {
	FIELD_1,	/* odd field */
	FIELD_2,	/* even field */
	FIELD_EITHER,	/* either field */
};

/*
 * fps down sample
 */
enum fps_ds {
	FPS_NO_DS,	/* no down sample */
	FPS_2_DS,	/* only receives the first frame every 2 frames */
	FPS_3_DS,	/* only receives the first frame every 3 frames */
	FPS_4_DS,	/* only receives the first frame every 3 frames */
};

/*
 * Minimum size of SDRAM block write
 */
enum min_sdr_wr_size {
	SDR_256B,	/* if hflip is enable, always select 256 bytes */
	SDR_512B,
	SDR_1K,
	SDR_2K,
};

/*
 * dma fifo buffer select
 */
enum fifo_buf_sel {
	FIFO0_BUF_A = 0,
	FIFO1_BUF_A,
	FIFO2_BUF_A,
};

/*
 * dma interrupt select
 */
enum dma_int_sel {
	DMA_INT_CAPTURE_DONE = 0X1,
	DMA_INT_FRAME_DONE = 0X2,
	DMA_INT_BUF_0_OVERFLOW = 0X4,
	DMA_INT_BUF_1_OVERFLOW = 0X8,
	DMA_INT_BUF_2_OVERFLOW = 0X10,
	DMA_INT_LINE_CNT = 0X20,
	DMA_INT_HBLANK_OVERFLOW = 0X40,
	DMA_INT_VSYNC_TRIG = 0X80,
	DMA_INT_ALL = 0XFF,
};

/*register data struct*/

struct csic_dma_flip {
	unsigned int hflip_en;
	unsigned int vflip_en;
};

struct csic_dma_cfg {
	enum output_fmt fmt;
	struct csic_dma_flip flip;
	enum field_sel field;
	enum fps_ds ds;
	enum min_sdr_wr_size block;
};

struct dma_output_size {
	unsigned int hor_len;
	unsigned int hor_start;
	unsigned int ver_len;
	unsigned int ver_start;
};

struct dma_buf_len {
	unsigned int buf_len_y;
	unsigned int buf_len_c;
};

struct dma_flip_size {
	unsigned int ver_len;
	unsigned int hor_len;
};

struct dma_capture_status {
	bool field_sta;
	bool vcap_sta;
	bool scap_sta;
};

struct dma_int_status {
	bool capture_done;
	bool frame_done;
	bool buf_0_overflow;
	bool buf_1_overflow;
	bool buf_2_overflow;
	bool line_cnt_flag;
	bool hblank_overflow;
	bool vsync_trig;
};

struct dma_fifo_threshold {
	unsigned int fifo_nearly_full_th;
	unsigned int fifo_thrs;
};

struct dma_pclk_statistic {
	unsigned int pclk_cnt_line_max;
	unsigned int pclk_cnt_line_min;
};

int csic_dma_set_base_addr(unsigned int sel, unsigned long addr);
void csic_dma_enable(unsigned int sel);
void csic_dma_disable(unsigned int sel);
void csic_dma_clk_cnt_en(unsigned int sel, unsigned int en);
void csic_dma_clk_cnt_sample(unsigned int sel, unsigned int en);

void csic_dma_output_fmt_cfg(unsigned int sel, enum output_fmt fmt);
void csic_dma_flip_en(unsigned int sel, struct csic_dma_flip *flip);
void csic_dma_config(unsigned int sel, struct csic_dma_cfg *cfg);
void csic_dma_output_size_cfg(unsigned int sel, struct dma_output_size *size);
void csic_dma_buffer_address(unsigned int sel, enum fifo_buf_sel buf,
				unsigned long addr);
void csic_dma_buffer_length(unsigned int sel, struct dma_buf_len *buf_len);
void csic_dma_flip_size(unsigned int sel, struct dma_flip_size *flip_size);
void csic_dma_cap_status(unsigned int sel, struct dma_capture_status *status);
void csic_dma_int_enable(unsigned int sel,	enum dma_int_sel interrupt);
void csic_dma_int_disable(unsigned int sel, enum dma_int_sel interrupt);
void csic_dma_int_get_status(unsigned int sel, struct dma_int_status *status);
void csic_dma_int_clear_status(unsigned int sel, enum dma_int_sel interrupt);

/*for debug*/
void csic_dma_line_count(unsigned int sel, unsigned int line_cnt);
void csic_dma_frame_clk_cnt(unsigned int sel, unsigned int clk_cnt);
void csic_dma_acc_itnl_cnt(unsigned int sel, unsigned int acc_clk_cnt,
				unsigned int itnl_clk_cnt);
void csic_dma_fifo_statistic(unsigned int sel, unsigned int fifo_frm_max);
void csic_dma_fifo_threshold(unsigned int sel, struct dma_fifo_threshold *threshold);
void csic_dma_pclk_statistic(unsigned int sel, struct dma_pclk_statistic *pclk_stat);

#endif /* __CSIC__DMA__REG__H__ */
