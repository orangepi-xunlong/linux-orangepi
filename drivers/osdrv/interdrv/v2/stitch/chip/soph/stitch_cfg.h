#ifndef _STITCH_CFG_H_
#define _STITCH_CFG_H_

//#define STITCH_MAX_WIDTH 4608
//#define STITCH_MAX_HEIGHT 4320
#define STITCH_DMA_REG_NUM 28

//#define STITCH_INPUT_SIZE_ALIGN 2
//#define STITCH_BLD_SIZE_ALIGN   2
#define STITCH_STRIDE_ALIGN     16

#define STITCH_INTR_NUM         22

#define STITCH_MAGIC_SIZE       0x3FFF

#if 0
#define ADDR_STITCH_TOP_BASE    0x680b9e00
//wdma nbld
#define ADDR_WDMA_STCH_LEFTY0   0x680B8100
#define ADDR_WDMA_STCH_LEFTU0   0x680B8180
#define ADDR_WDMA_STCH_LEFTV0   0x680B8200

#define ADDR_WDMA_STCH_RIGHTY0  0x680B8280
#define ADDR_WDMA_STCH_RIGHTU0  0x680B8300
#define ADDR_WDMA_STCH_RIGHTV0  0x680B8380
//wdma bld
#define ADDR_WDMA_STCH_BLDY0    0x680B8400
#define ADDR_WDMA_STCH_BLDU0    0x680B8480
#define ADDR_WDMA_STCH_BLDV0    0x680B8500

#define ADDR_WDMA_STCH_BLDY1    0x680B8900
#define ADDR_WDMA_STCH_BLDU1    0x680B8980
#define ADDR_WDMA_STCH_BLDV1    0x680B8A00
//rdma
#define ADDR_RDMA_STCH_LEFTY0   0x680B8C00
#define ADDR_RDMA_STCH_LEFTU0   0x680B8C80
#define ADDR_RDMA_STCH_LEFTV0   0x680B8D00

#define ADDR_RDMA_STCH_RIGHTY0  0x680B8D80
#define ADDR_RDMA_STCH_RIGHTU0  0x680B8E00
#define ADDR_RDMA_STCH_RIGHTV0  0x680B8E80

#define ADDR_RDMA_STCH_LEFTY1   0x680B8F00
#define ADDR_RDMA_STCH_LEFTU1   0x680B8F80
#define ADDR_RDMA_STCH_LEFTV1   0x680B9000

#define ADDR_RDMA_STCH_RIGHTY1  0x680B9080
#define ADDR_RDMA_STCH_RIGHTU1  0x680B9100
#define ADDR_RDMA_STCH_RIGHTV1  0x680B9180
//rdma wht
#define ADDR_RDMA_STCH_ALPHA0   0x680B9200
#define ADDR_RDMA_STCH_BETA0    0x680B9280

#define ADDR_RDMA_STCH_ALPHA1   0x680B9300
#define ADDR_RDMA_STCH_BETA1    0x680B9380
#endif

enum stitch_dma_id {
	ID_WDMA_STCH_LEFTY0 = 0,//id:0
	ID_WDMA_STCH_LEFTU0,
	ID_WDMA_STCH_LEFTV0,

	ID_WDMA_STCH_RIGHTY0,  //id:3
	ID_WDMA_STCH_RIGHTU0,
	ID_WDMA_STCH_RIGHTV0,

	ID_WDMA_STCH_BLDY0,    //id:6
	ID_WDMA_STCH_BLDU0,
	ID_WDMA_STCH_BLDV0,

	ID_WDMA_STCH_BLDY1,    //id:9
	ID_WDMA_STCH_BLDU1,
	ID_WDMA_STCH_BLDV1,

	ID_RDMA_STCH_LEFTY0,   //id:12
	ID_RDMA_STCH_LEFTU0,
	ID_RDMA_STCH_LEFTV0,

	ID_RDMA_STCH_RIGHTY0,  //id:15
	ID_RDMA_STCH_RIGHTU0,
	ID_RDMA_STCH_RIGHTV0,

	ID_RDMA_STCH_LEFTY1,   //id:18
	ID_RDMA_STCH_LEFTU1,
	ID_RDMA_STCH_LEFTV1,

	ID_RDMA_STCH_RIGHTY1,  //id:21
	ID_RDMA_STCH_RIGHTU1,
	ID_RDMA_STCH_RIGHTV1,

	ID_RDMA_STCH_ALPHA0,   //id:24
	ID_RDMA_STCH_BETA0,

	ID_RDMA_STCH_ALPHA1,   //id:26
	ID_RDMA_STCH_BETA1,

	ID_DMA_STCH_MAX,
};

struct stitch_bj_size_param {
	unsigned int bld_w_str_left;
	unsigned int bld_w_end_left;
	unsigned int nbld_w_str_left;
	unsigned int nbld_w_end_left;
	unsigned int bld_w_str_right;
	unsigned int bld_w_end_right;
	unsigned int nbld_w_str_right;
	unsigned int nbld_w_end_right;
};

//rdma chn size cfg: y/uv/wgh
//cfg[0] :y rdma channel
//cfg[1] :uv rdma channel
//cfg[2] :wgt rdma channel
struct stitch_rdma_size_param {
	unsigned int crop_h_str_r_left;
	unsigned int crop_h_end_r_left;
	unsigned int crop_h_str_r_right;
	unsigned int crop_h_end_r_right;
	unsigned int crop_w_str_r_left;
	unsigned int crop_w_end_r_left;
	unsigned int crop_w_str_r_right;
	unsigned int crop_w_end_r_right;
	unsigned int img_height_r_left;
	unsigned int img_width_r_left;
	unsigned int img_height_r_right;
	unsigned int img_width_r_right;
};

//wdma nbld chn size cfg: y/uv
//cfg[0] :y wdma channel
//cfg[1] :uv wdma channel
struct stitch_wdma_nbld_size_param {
	unsigned int crop_h_str_w_left;
	unsigned int crop_h_end_w_left;
	unsigned int crop_h_str_w_right;
	unsigned int crop_h_end_w_right;
	unsigned int crop_w_str_w_left;
	unsigned int crop_w_end_w_left;
	unsigned int crop_w_str_w_right;
	unsigned int crop_w_end_w_right;
	unsigned int img_height_w_left;
	unsigned int img_width_w_left;
	unsigned int img_height_w_right;
	unsigned int img_width_w_right;
};

//wdma bld chn size cfg: y/uv
//cfg[0] :y wdma channel
//cfg[1] :uv wdma channel
struct stitch_wdma_bld_size_param {
	unsigned int crop_h_str_w_bld;
	unsigned int crop_h_end_w_bld;
	unsigned int crop_w_str_w_bld;
	unsigned int crop_w_end_w_bld;
	unsigned int img_height_w_bld;
	unsigned int img_width_w_bld;
};

struct stitch_crop2bj_param {
	unsigned int left_img_height;
	unsigned int left_img_width;
	unsigned int right_img_height;
	unsigned int right_img_width;
	unsigned int left_crop_h_str;
	unsigned int left_crop_h_end;
	unsigned int right_crop_h_str;
	unsigned int right_crop_h_end;
	unsigned int left_crop_w_str;
	unsigned int left_crop_w_end;
	unsigned int right_crop_w_str;
	unsigned int right_crop_w_end;
};

/******************************************/
/*          stitch DMA Definition         */
/******************************************/
struct stitch_dma_ctl {
	enum stitch_dma_id dma_id;
	unsigned long long addr;
	unsigned int width;
	unsigned int stride;
	unsigned int height;
};

#endif // _STITCH_CFG_H_
