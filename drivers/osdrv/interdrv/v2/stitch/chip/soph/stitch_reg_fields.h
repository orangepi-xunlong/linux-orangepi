/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File name:stitch_reg_fields.h
 * Description:HW register description
 */

#ifndef _STITCH_REG_FIELDS_H_
#define _STITCH_REG_FIELDS_H_

#ifdef __cplusplus
extern "C" {
#endif

/******************************************/
/*           Module Definition            */
/******************************************/
union REG_STITCHING_BLENDING_0 {
	unsigned int raw;
	struct {
		unsigned int FRAME_START_W1P                 : 1;
		unsigned int _rsv_1                          : 1;
		unsigned int FRAME_DONE                      : 1;
		unsigned int _rsv_3                          : 1;
		unsigned int AVG_MODE_BLD0                   : 1;
		unsigned int _rsv_5                          : 3;
		unsigned int AVG_MODE_LEFT0                  : 1;
		unsigned int _rsv_9                          : 3;
		unsigned int AVG_MODE_RIGHT0                 : 1;
		unsigned int _rsv_13                         : 3;
		unsigned int BLD_DISABLE0                    : 1;
		unsigned int _rsv_17                         : 3;
		unsigned int DROP_MODE_BLD0                  : 1;
		unsigned int _rsv_21                         : 1;
		unsigned int DROP_MODE_LEFT0                 : 1;
		unsigned int _rsv_23                         : 1;
		unsigned int DROP_MODE_RIGHT0                : 1;
	} bits;
};

union REG_STITCHING_BLENDING_1 {
	unsigned int raw;
	struct {
		unsigned int H_ERROR_LEFT0                   : 1;
		unsigned int _rsv_1                          : 7;
		unsigned int H_ERROR_RIGHT0                  : 1;
		unsigned int _rsv_9                          : 7;
		unsigned int W_ERROR_LEFT0                   : 1;
		unsigned int _rsv_17                         : 7;
		unsigned int W_ERROR_RIGHT0                  : 1;
	} bits;
};

union REG_STITCHING_BLENDING_2 {
	unsigned int raw;
	struct {
		unsigned int BLD_W_STR_LEFT0                 : 14;
		unsigned int _rsv_14                         : 2;
		unsigned int BLD_W_END_LEFT0                 : 14;
	} bits;
};

union REG_STITCHING_BLENDING_3 {
	unsigned int raw;
	struct {
		unsigned int BLD_W_STR_RIGHT0                : 14;
		unsigned int _rsv_14                         : 2;
		unsigned int BLD_W_END_RIGHT0                : 14;
	} bits;
};

union REG_STITCHING_BLENDING_4 {
	unsigned int raw;
	struct {
		unsigned int CROP_H_STR_BLDY0                : 14;
		unsigned int _rsv_14                         : 2;
		unsigned int CROP_H_END_BLDY0                : 14;
	} bits;
};

union REG_STITCHING_BLENDING_5 {
	unsigned int raw;
	struct {
		unsigned int CROP_H_STR_R_LEFTY0             : 14;
		unsigned int _rsv_14                         : 2;
		unsigned int CROP_H_END_R_LEFTY0             : 14;
	} bits;
};

union REG_STITCHING_BLENDING_6 {
	unsigned int raw;
	struct {
		unsigned int CROP_H_STR_R_RIGHTY0            : 14;
		unsigned int _rsv_14                         : 2;
		unsigned int CROP_H_END_R_RIGHTY0            : 14;
	} bits;
};

union REG_STITCHING_BLENDING_7 {
	unsigned int raw;
	struct {
		unsigned int CROP_W_STR_BLDY0                : 14;
		unsigned int _rsv_14                         : 2;
		unsigned int CROP_W_END_BLDY0                : 14;
	} bits;
};

union REG_STITCHING_BLENDING_8 {
	unsigned int raw;
	struct {
		unsigned int CROP_W_STR_R_LEFTY0             : 14;
		unsigned int _rsv_14                         : 2;
		unsigned int CROP_W_END_R_LEFTY0             : 14;
	} bits;
};

union REG_STITCHING_BLENDING_9 {
	unsigned int raw;
	struct {
		unsigned int CROP_W_STR_R_RIGHTY0            : 14;
		unsigned int _rsv_14                         : 2;
		unsigned int CROP_W_END_R_RIGHTY0            : 14;
	} bits;
};

union REG_STITCHING_BLENDING_10 {
	unsigned int raw;
	struct {
		unsigned int IMG_HEIGHT_BLDY0                : 14;
		unsigned int _rsv_14                         : 2;
		unsigned int IMG_WIDTH_BLDY0                 : 14;
	} bits;
};

union REG_STITCHING_BLENDING_11 {
	unsigned int raw;
	struct {
		unsigned int IMG_HEIGHT_R_LEFTY0             : 14;
		unsigned int _rsv_14                         : 2;
		unsigned int IMG_WIDTH_R_LEFTY0              : 14;
	} bits;
};

union REG_STITCHING_BLENDING_12 {
	unsigned int raw;
	struct {
		unsigned int IMG_HEIGHT_R_RIGHTY0            : 14;
		unsigned int _rsv_14                         : 2;
		unsigned int IMG_WIDTH_R_RIGHTY0             : 14;
	} bits;
};

union REG_STITCHING_BLENDING_13 {
	unsigned int raw;
	struct {
		unsigned int NBLD_W_END_LEFT0                : 14;
		unsigned int _rsv_14                         : 2;
		unsigned int NBLD_W_END_RIGHT0               : 14;
	} bits;
};

union REG_STITCHING_BLENDING_14 {
	unsigned int raw;
	struct {
		unsigned int NBLD_W_STR_LEFT0                : 14;
		unsigned int _rsv_14                         : 2;
		unsigned int NBLD_W_STR_RIGHT0               : 14;
	} bits;
};

union REG_STITCHING_BLENDING_15 {
	unsigned int raw;
	struct {
		unsigned int NODELAY_DUPLICATE_MODE_LEFT0    : 1;
		unsigned int _rsv_1                          : 7;
		unsigned int NODELAY_DUPLICATE_MODE_RIGHT0   : 1;
	} bits;
};

union REG_STITCHING_BLENDING_16 {
	unsigned int raw;
	struct {
		unsigned int RDMA_STCH_ALPHA0_ENABLE         : 1;
		unsigned int _rsv_1                          : 3;
		unsigned int RDMA_STCH_BETA0_ENABLE          : 1;
		unsigned int _rsv_5                          : 3;
		unsigned int RDMA_STCH_LEFTY0_ENABLE         : 1;
		unsigned int _rsv_9                          : 3;
		unsigned int RDMA_STCH_LEFTU0_ENABLE         : 1;
		unsigned int _rsv_13                         : 3;
		unsigned int RDMA_STCH_LEFTV0_ENABLE         : 1;
		unsigned int _rsv_17                         : 3;
		unsigned int RDMA_STCH_RIGHTY0_ENABLE        : 1;
		unsigned int _rsv_21                         : 3;
		unsigned int RDMA_STCH_RIGHTU0_ENABLE        : 1;
		unsigned int _rsv_25                         : 3;
		unsigned int RDMA_STCH_RIGHTV0_ENABLE        : 1;
	} bits;
};

union REG_STITCHING_BLENDING_17 {
	unsigned int raw;
	struct {
		unsigned int SCALE_DISABLE_LEFT0             : 1;
		unsigned int _rsv_1                          : 3;
		unsigned int SCALE_DISABLE_RIGHT0            : 1;
	} bits;
};

union REG_STITCHING_BLENDING_18 {
	unsigned int raw;
	struct {
		unsigned int WDMA_STCH_LEFTY0_ENABLE         : 1;
		unsigned int _rsv_1                          : 3;
		unsigned int WDMA_STCH_LEFTU0_ENABLE         : 1;
		unsigned int _rsv_5                          : 3;
		unsigned int WDMA_STCH_LEFTV0_ENABLE         : 1;
		unsigned int _rsv_9                          : 3;
		unsigned int WDMA_STCH_RIGHTY0_ENABLE        : 1;
		unsigned int _rsv_13                         : 3;
		unsigned int WDMA_STCH_RIGHTU0_ENABLE        : 1;
		unsigned int _rsv_17                         : 3;
		unsigned int WDMA_STCH_RIGHTV0_ENABLE        : 1;
		unsigned int _rsv_21                         : 3;
		unsigned int WDMA_STCH_BLDY0_ENABLE          : 1;
		unsigned int _rsv_25                         : 3;
		unsigned int WDMA_STCH_BLDU0_ENABLE          : 1;
		unsigned int _rsv_29                         : 2;
		unsigned int WDMA_STCH_BLDV0_ENABLE          : 1;
	} bits;
};

union REG_STITCHING_BLENDING_19 {
	unsigned int raw;
	struct {
		unsigned int X_0                             : 5;
	} bits;
};

union REG_STITCHING_BLENDING_20 {
	unsigned int raw;
	struct {
		unsigned int BYPASS_H_BLD0                   : 1;
		unsigned int _rsv_1                          : 1;
		unsigned int BYPASS_V_BLD0                   : 1;
		unsigned int _rsv_3                          : 1;
		unsigned int BYPASS_H_LEFTIN0                : 1;
		unsigned int _rsv_5                          : 1;
		unsigned int BYPASS_V_LEFTIN0                : 1;
		unsigned int _rsv_7                          : 1;
		unsigned int BYPASS_H_LEFTOUT0               : 1;
		unsigned int _rsv_9                          : 1;
		unsigned int BYPASS_V_LEFTOUT0               : 1;
		unsigned int _rsv_11                         : 1;
		unsigned int BYPASS_H_RIGHTIN0               : 1;
		unsigned int _rsv_13                         : 1;
		unsigned int BYPASS_V_RIGHTIN0               : 1;
		unsigned int _rsv_15                         : 1;
		unsigned int BYPASS_H_RIGHTOUT0              : 1;
		unsigned int _rsv_17                         : 1;
		unsigned int BYPASS_V_RIGHTOUT0              : 1;
	} bits;
};

union REG_STITCHING_BLENDING_21 {
	unsigned int raw;
	struct {
		unsigned int AVG_MODE_BLD1                   : 1;
		unsigned int _rsv_1                          : 3;
		unsigned int AVG_MODE_LEFT1                  : 1;
		unsigned int _rsv_5                          : 3;
		unsigned int AVG_MODE_RIGHT1                 : 1;
		unsigned int _rsv_9                          : 7;
		unsigned int BLD_DISABLE1                    : 1;
		unsigned int _rsv_17                         : 3;
		unsigned int DROP_MODE_BLD1                  : 1;
		unsigned int _rsv_21                         : 1;
		unsigned int DROP_MODE_LEFT1                 : 1;
		unsigned int _rsv_23                         : 1;
		unsigned int DROP_MODE_RIGHT1                : 1;
	} bits;
};

union REG_STITCHING_BLENDING_22 {
	unsigned int raw;
	struct {
		unsigned int H_ERROR_LEFT1                   : 1;
		unsigned int _rsv_1                          : 7;
		unsigned int H_ERROR_RIGHT1                  : 1;
		unsigned int _rsv_9                          : 7;
		unsigned int W_ERROR_LEFT1                   : 1;
		unsigned int _rsv_17                         : 7;
		unsigned int W_ERROR_RIGHT1                  : 1;
	} bits;
};

union REG_STITCHING_BLENDING_23 {
	unsigned int raw;
	struct {
		unsigned int BLD_W_STR_LEFT1                 : 14;
		unsigned int _rsv_14                         : 2;
		unsigned int BLD_W_END_LEFT1                 : 14;
	} bits;
};

union REG_STITCHING_BLENDING_24 {
	unsigned int raw;
	struct {
		unsigned int BLD_W_STR_RIGHT1                : 14;
		unsigned int _rsv_14                         : 2;
		unsigned int BLD_W_END_RIGHT1                : 14;
	} bits;
};

union REG_STITCHING_BLENDING_25 {
	unsigned int raw;
	struct {
		unsigned int CROP_H_STR_BLDY1                : 14;
		unsigned int _rsv_14                         : 2;
		unsigned int CROP_H_END_BLDY1                : 14;
	} bits;
};

union REG_STITCHING_BLENDING_26 {
	unsigned int raw;
	struct {
		unsigned int CROP_H_STR_R_LEFTY1             : 14;
		unsigned int _rsv_14                         : 2;
		unsigned int CROP_H_END_R_LEFTY1             : 14;
	} bits;
};

union REG_STITCHING_BLENDING_27 {
	unsigned int raw;
	struct {
		unsigned int CROP_H_STR_R_RIGHTY1            : 14;
		unsigned int _rsv_14                         : 2;
		unsigned int CROP_H_END_R_RIGHTY1            : 14;
	} bits;
};

union REG_STITCHING_BLENDING_28 {
	unsigned int raw;
	struct {
		unsigned int CROP_W_STR_BLDY1                : 14;
		unsigned int _rsv_14                         : 2;
		unsigned int CROP_W_END_BLDY1                : 14;
	} bits;
};

union REG_STITCHING_BLENDING_29 {
	unsigned int raw;
	struct {
		unsigned int CROP_W_STR_R_LEFTY1             : 14;
		unsigned int _rsv_14                         : 2;
		unsigned int CROP_W_END_R_LEFTY1             : 14;
	} bits;
};

union REG_STITCHING_BLENDING_30 {
	unsigned int raw;
	struct {
		unsigned int CROP_W_STR_R_RIGHTY1            : 14;
		unsigned int _rsv_14                         : 2;
		unsigned int CROP_W_END_R_RIGHTY1            : 14;
	} bits;
};

union REG_STITCHING_BLENDING_31 {
	unsigned int raw;
	struct {
		unsigned int IMG_HEIGHT_BLDY1                : 14;
		unsigned int _rsv_14                         : 2;
		unsigned int IMG_WIDTH_BLDY1                 : 14;
	} bits;
};

union REG_STITCHING_BLENDING_32 {
	unsigned int raw;
	struct {
		unsigned int IMG_HEIGHT_R_LEFTY1             : 14;
		unsigned int _rsv_14                         : 2;
		unsigned int IMG_WIDTH_R_LEFTY1              : 14;
	} bits;
};

union REG_STITCHING_BLENDING_33 {
	unsigned int raw;
	struct {
		unsigned int IMG_HEIGHT_R_RIGHTY1            : 14;
		unsigned int _rsv_14                         : 2;
		unsigned int IMG_WIDTH_R_RIGHTY1             : 14;
	} bits;
};

union REG_STITCHING_BLENDING_34 {
	unsigned int raw;
	struct {
		unsigned int NBLD_W_END_LEFT1                : 14;
		unsigned int _rsv_14                         : 2;
		unsigned int NBLD_W_END_RIGHT1               : 14;
	} bits;
};

union REG_STITCHING_BLENDING_35 {
	unsigned int raw;
	struct {
		unsigned int NBLD_W_STR_LEFT1                : 14;
		unsigned int _rsv_14                         : 2;
		unsigned int NBLD_W_STR_RIGHT1               : 14;
	} bits;
};

union REG_STITCHING_BLENDING_36 {
	unsigned int raw;
	struct {
		unsigned int NODELAY_DUPLICATE_MODE_LEFT1    : 1;
		unsigned int _rsv_1                          : 7;
		unsigned int NODELAY_DUPLICATE_MODE_RIGHT1   : 1;
	} bits;
};

union REG_STITCHING_BLENDING_37 {
	unsigned int raw;
	struct {
		unsigned int RDMA_STCH_ALPHA1_ENABLE         : 1;
		unsigned int _rsv_1                          : 3;
		unsigned int RDMA_STCH_BETA1_ENABLE          : 1;
		unsigned int _rsv_5                          : 3;
		unsigned int RDMA_STCH_LEFTY1_ENABLE         : 1;
		unsigned int _rsv_9                          : 3;
		unsigned int RDMA_STCH_LEFTU1_ENABLE         : 1;
		unsigned int _rsv_13                         : 3;
		unsigned int RDMA_STCH_LEFTV1_ENABLE         : 1;
		unsigned int _rsv_17                         : 3;
		unsigned int RDMA_STCH_RIGHTY1_ENABLE        : 1;
		unsigned int _rsv_21                         : 3;
		unsigned int RDMA_STCH_RIGHTU1_ENABLE        : 1;
		unsigned int _rsv_25                         : 3;
		unsigned int RDMA_STCH_RIGHTV1_ENABLE        : 1;
	} bits;
};

union REG_STITCHING_BLENDING_38 {
	unsigned int raw;
	struct {
		unsigned int SCALE_DISABLE_LEFT1             : 1;
		unsigned int _rsv_1                          : 3;
		unsigned int SCALE_DISABLE_RIGHT1            : 1;
	} bits;
};

union REG_STITCHING_BLENDING_39 {
	unsigned int raw;
	struct {
		unsigned int WDMA_STCH_LEFTY1_ENABLE         : 1;
		unsigned int _rsv_1                          : 3;
		unsigned int WDMA_STCH_LEFTU1_ENABLE         : 1;
		unsigned int _rsv_5                          : 3;
		unsigned int WDMA_STCH_LEFTV1_ENABLE         : 1;
		unsigned int _rsv_9                          : 3;
		unsigned int WDMA_STCH_RIGHTY1_ENABLE        : 1;
		unsigned int _rsv_13                         : 3;
		unsigned int WDMA_STCH_RIGHTU1_ENABLE        : 1;
		unsigned int _rsv_17                         : 3;
		unsigned int WDMA_STCH_RIGHTV1_ENABLE        : 1;
	} bits;
};

union REG_STITCHING_BLENDING_40 {
	unsigned int raw;
	struct {
		unsigned int _rsv_0                          : 24;
		unsigned int WDMA_STCH_BLDY1_ENABLE          : 1;
		unsigned int _rsv_25                         : 3;
		unsigned int WDMA_STCH_BLDU1_ENABLE          : 1;
		unsigned int _rsv_29                         : 2;
		unsigned int WDMA_STCH_BLDV1_ENABLE          : 1;
	} bits;
};

union REG_STITCHING_BLENDING_41 {
	unsigned int raw;
	struct {
		unsigned int X_1                             : 5;
	} bits;
};

union REG_STITCHING_BLENDING_42 {
	unsigned int raw;
	struct {
		unsigned int BYPASS_H_BLD1                   : 1;
		unsigned int _rsv_1                          : 3;
		unsigned int BYPASS_H_LEFTIN1                : 1;
		unsigned int _rsv_5                          : 3;
		unsigned int BYPASS_H_RIGHTIN1               : 1;
		unsigned int _rsv_9                          : 3;
		unsigned int BYPASS_H_LEFTOUT1               : 1;
		unsigned int _rsv_13                         : 3;
		unsigned int BYPASS_H_RIGHTOUT1              : 1;
	} bits;
};

union REG_STITCHING_BLENDING_43 {
	unsigned int raw;
	struct {
		unsigned int BYPASS_V_BLD1                   : 1;
		unsigned int _rsv_1                          : 3;
		unsigned int BYPASS_V_LEFTIN1                : 1;
		unsigned int _rsv_5                          : 3;
		unsigned int BYPASS_V_RIGHTIN1               : 1;
		unsigned int _rsv_9                          : 3;
		unsigned int BYPASS_V_LEFTOUT1               : 1;
		unsigned int _rsv_13                         : 3;
		unsigned int BYPASS_V_RIGHTOUT1              : 1;
	} bits;
};

union REG_STITCHING_BLENDING_FLOW_CONTROL {
	unsigned int raw;
	struct {
		unsigned int INTERRUPT_CLR_W1P               : 1;
		unsigned int SUBSYS_RESET_W1P                : 1;
		unsigned int STITCH_INTER_FLOW               : 1;
		unsigned int RESET_DONE                      : 2;
		unsigned int SHADOW_DONE                     : 1;
		unsigned int SHADOW_CLR_W1P                  : 1;
		unsigned int SHADOW_INT_MASK                 : 1;
		unsigned int TRIG_CNT                        : 8;
		unsigned int SHADOW_CNT                      : 8;
		unsigned int RESET_CNT                       : 8;
	} bits;
};

union REG_STITCHING_BLENDING_STCH_SHDW_SEL {
	unsigned int raw;
	struct {
		unsigned int STCH_SHDW_SEL                   : 1;
	} bits;
};

union REG_STITCHING_BLENDING_44 {
	unsigned int raw;
	struct {
		unsigned int IMG_HEIGHT_R_LEFTUV0            : 14;
		unsigned int _rsv_14                         : 2;
		unsigned int IMG_WIDTH_R_LEFTUV0             : 14;
	} bits;
};

union REG_STITCHING_BLENDING_45 {
	unsigned int raw;
	struct {
		unsigned int IMG_HEIGHT_R_RIGHTUV0           : 14;
		unsigned int _rsv_14                         : 2;
		unsigned int IMG_WIDTH_R_RIGHTUV0            : 14;
	} bits;
};

union REG_STITCHING_BLENDING_46 {
	unsigned int raw;
	struct {
		unsigned int IMG_HEIGHT_R_LEFTUV1            : 14;
		unsigned int _rsv_14                         : 2;
		unsigned int IMG_WIDTH_R_LEFTUV1             : 14;
	} bits;
};

union REG_STITCHING_BLENDING_47 {
	unsigned int raw;
	struct {
		unsigned int IMG_HEIGHT_R_RIGHTUV1           : 14;
		unsigned int _rsv_14                         : 2;
		unsigned int IMG_WIDTH_R_RIGHTUV1            : 14;
	} bits;
};

union REG_STITCHING_BLENDING_48 {
	unsigned int raw;
	struct {
		unsigned int CROP_H_STR_R_LEFTUV0            : 14;
		unsigned int _rsv_14                         : 2;
		unsigned int CROP_H_END_R_LEFTUV0            : 14;
	} bits;
};

union REG_STITCHING_BLENDING_49 {
	unsigned int raw;
	struct {
		unsigned int CROP_H_STR_R_RIGHTUV0           : 14;
		unsigned int _rsv_14                         : 2;
		unsigned int CROP_H_END_R_RIGHTUV0           : 14;
	} bits;
};

union REG_STITCHING_BLENDING_50 {
	unsigned int raw;
	struct {
		unsigned int CROP_W_STR_R_LEFTUV0            : 14;
		unsigned int _rsv_14                         : 2;
		unsigned int CROP_W_END_R_LEFTUV0            : 14;
	} bits;
};

union REG_STITCHING_BLENDING_51 {
	unsigned int raw;
	struct {
		unsigned int CROP_W_STR_R_RIGHTUV0           : 14;
		unsigned int _rsv_14                         : 2;
		unsigned int CROP_W_END_R_RIGHTUV0           : 14;
	} bits;
};

union REG_STITCHING_BLENDING_52 {
	unsigned int raw;
	struct {
		unsigned int CROP_H_STR_R_LEFTUV1            : 14;
		unsigned int _rsv_14                         : 2;
		unsigned int CROP_H_END_R_LEFTUV1            : 14;
	} bits;
};

union REG_STITCHING_BLENDING_53 {
	unsigned int raw;
	struct {
		unsigned int CROP_H_STR_R_RIGHTUV1           : 14;
		unsigned int _rsv_14                         : 2;
		unsigned int CROP_H_END_R_RIGHTUV1           : 14;
	} bits;
};

union REG_STITCHING_BLENDING_54 {
	unsigned int raw;
	struct {
		unsigned int CROP_W_STR_R_LEFTUV1            : 14;
		unsigned int _rsv_14                         : 2;
		unsigned int CROP_W_END_R_LEFTUV1            : 14;
	} bits;
};

union REG_STITCHING_BLENDING_55 {
	unsigned int raw;
	struct {
		unsigned int CROP_W_STR_R_RIGHTUV1           : 14;
		unsigned int _rsv_14                         : 2;
		unsigned int CROP_W_END_R_RIGHTUV1           : 14;
	} bits;
};

union REG_STITCHING_BLENDING_56 {
	unsigned int raw;
	struct {
		unsigned int IMG_HEIGHT_ALPHA0               : 14;
		unsigned int _rsv_14                         : 2;
		unsigned int IMG_WIDTH_ALPHA0                : 14;
	} bits;
};

union REG_STITCHING_BLENDING_57 {
	unsigned int raw;
	struct {
		unsigned int IMG_HEIGHT_BETA0                : 14;
		unsigned int _rsv_14                         : 2;
		unsigned int IMG_WIDTH_BETA0                 : 14;
	} bits;
};

union REG_STITCHING_BLENDING_58 {
	unsigned int raw;
	struct {
		unsigned int IMG_HEIGHT_ALPHA1               : 14;
		unsigned int _rsv_14                         : 2;
		unsigned int IMG_WIDTH_ALPHA1                : 14;
	} bits;
};

union REG_STITCHING_BLENDING_59 {
	unsigned int raw;
	struct {
		unsigned int IMG_HEIGHT_BETA1                : 14;
		unsigned int _rsv_14                         : 2;
		unsigned int IMG_WIDTH_BETA1                 : 14;
	} bits;
};

union REG_STITCHING_BLENDING_60 {
	unsigned int raw;
	struct {
		unsigned int CROP_H_STR_ALPHA0               : 14;
		unsigned int _rsv_14                         : 2;
		unsigned int CROP_H_END_ALPHA0               : 14;
	} bits;
};

union REG_STITCHING_BLENDING_61 {
	unsigned int raw;
	struct {
		unsigned int CROP_H_STR_BETA0                : 14;
		unsigned int _rsv_14                         : 2;
		unsigned int CROP_H_END_BETA0                : 14;
	} bits;
};

union REG_STITCHING_BLENDING_62 {
	unsigned int raw;
	struct {
		unsigned int CROP_W_STR_ALPHA0               : 14;
		unsigned int _rsv_14                         : 2;
		unsigned int CROP_W_END_ALPHA0               : 14;
	} bits;
};

union REG_STITCHING_BLENDING_63 {
	unsigned int raw;
	struct {
		unsigned int CROP_W_STR_BETA0                : 14;
		unsigned int _rsv_14                         : 2;
		unsigned int CROP_W_END_BETA0                : 14;
	} bits;
};

union REG_STITCHING_BLENDING_64 {
	unsigned int raw;
	struct {
		unsigned int CROP_H_STR_ALPHA1               : 14;
		unsigned int _rsv_14                         : 2;
		unsigned int CROP_H_END_ALPHA1               : 14;
	} bits;
};

union REG_STITCHING_BLENDING_65 {
	unsigned int raw;
	struct {
		unsigned int CROP_H_STR_BETA1                : 14;
		unsigned int _rsv_14                         : 2;
		unsigned int CROP_H_END_BETA1                : 14;
	} bits;
};

union REG_STITCHING_BLENDING_66 {
	unsigned int raw;
	struct {
		unsigned int CROP_W_STR_ALPHA1               : 14;
		unsigned int _rsv_14                         : 2;
		unsigned int CROP_W_END_ALPHA1               : 14;
	} bits;
};

union REG_STITCHING_BLENDING_67 {
	unsigned int raw;
	struct {
		unsigned int CROP_W_STR_BETA1                : 14;
		unsigned int _rsv_14                         : 2;
		unsigned int CROP_W_END_BETA1                : 14;
	} bits;
};

union REG_STITCHING_BLENDING_68 {
	unsigned int raw;
	struct {
		unsigned int CROP_H_STR_BLDUV0               : 14;
		unsigned int _rsv_14                         : 2;
		unsigned int CROP_H_END_BLDUV0               : 14;
	} bits;
};

union REG_STITCHING_BLENDING_69 {
	unsigned int raw;
	struct {
		unsigned int CROP_H_STR_W_LEFTY0             : 14;
		unsigned int _rsv_14                         : 2;
		unsigned int CROP_H_END_W_LEFTY0             : 14;
	} bits;
};

union REG_STITCHING_BLENDING_70 {
	unsigned int raw;
	struct {
		unsigned int CROP_H_STR_W_RIGHTY0            : 14;
		unsigned int _rsv_14                         : 2;
		unsigned int CROP_H_END_W_RIGHTY0            : 14;
	} bits;
};

union REG_STITCHING_BLENDING_71 {
	unsigned int raw;
	struct {
		unsigned int CROP_W_STR_BLDUV0               : 14;
		unsigned int _rsv_14                         : 2;
		unsigned int CROP_W_END_BLDUV0               : 14;
	} bits;
};

union REG_STITCHING_BLENDING_72 {
	unsigned int raw;
	struct {
		unsigned int CROP_W_STR_W_LEFTY0             : 14;
		unsigned int _rsv_14                         : 2;
		unsigned int CROP_W_END_W_LEFTY0             : 14;
	} bits;
};

union REG_STITCHING_BLENDING_73 {
	unsigned int raw;
	struct {
		unsigned int CROP_W_STR_W_RIGHTY0            : 14;
		unsigned int _rsv_14                         : 2;
		unsigned int CROP_W_END_W_RIGHTY0            : 14;
	} bits;
};

union REG_STITCHING_BLENDING_74 {
	unsigned int raw;
	struct {
		unsigned int IMG_HEIGHT_BLDUV0               : 14;
		unsigned int _rsv_14                         : 2;
		unsigned int IMG_WIDTH_BLDUV0                : 14;
	} bits;
};

union REG_STITCHING_BLENDING_75 {
	unsigned int raw;
	struct {
		unsigned int IMG_HEIGHT_W_LEFTY0             : 14;
		unsigned int _rsv_14                         : 2;
		unsigned int IMG_WIDTH_W_LEFTY0              : 14;
	} bits;
};

union REG_STITCHING_BLENDING_76 {
	unsigned int raw;
	struct {
		unsigned int IMG_HEIGHT_W_RIGHTY0            : 14;
		unsigned int _rsv_14                         : 2;
		unsigned int IMG_WIDTH_W_RIGHTY0             : 14;
	} bits;
};

union REG_STITCHING_BLENDING_77 {
	unsigned int raw;
	struct {
		unsigned int CROP_H_STR_BLDUV1               : 14;
		unsigned int _rsv_14                         : 2;
		unsigned int CROP_H_END_BLDUV1               : 14;
	} bits;
};

union REG_STITCHING_BLENDING_78 {
	unsigned int raw;
	struct {
		unsigned int CROP_W_STR_BLDUV1               : 14;
		unsigned int _rsv_14                         : 2;
		unsigned int CROP_W_END_BLDUV1               : 14;
	} bits;
};

union REG_STITCHING_BLENDING_79 {
	unsigned int raw;
	struct {
		unsigned int IMG_HEIGHT_BLDUV1               : 14;
		unsigned int _rsv_14                         : 2;
		unsigned int IMG_WIDTH_BLDUV1                : 14;
	} bits;
};

union REG_STITCHING_BLENDING_80 {
	unsigned int raw;
	struct {
		unsigned int CROP_H_STR_W_LEFTUV0            : 14;
		unsigned int _rsv_14                         : 2;
		unsigned int CROP_H_END_W_LEFTUV0            : 14;
	} bits;
};

union REG_STITCHING_BLENDING_81 {
	unsigned int raw;
	struct {
		unsigned int CROP_H_STR_W_RIGHTUV0           : 14;
		unsigned int _rsv_14                         : 2;
		unsigned int CROP_H_END_W_RIGHTUV0           : 14;
	} bits;
};

union REG_STITCHING_BLENDING_82 {
	unsigned int raw;
	struct {
		unsigned int CROP_W_STR_W_LEFTUV0            : 14;
		unsigned int _rsv_14                         : 2;
		unsigned int CROP_W_END_W_LEFTUV0            : 14;
	} bits;
};

union REG_STITCHING_BLENDING_83 {
	unsigned int raw;
	struct {
		unsigned int CROP_W_STR_W_RIGHTUV0           : 14;
		unsigned int _rsv_14                         : 2;
		unsigned int CROP_W_END_W_RIGHTUV0           : 14;
	} bits;
};

union REG_STITCHING_BLENDING_84 {
	unsigned int raw;
	struct {
		unsigned int IMG_HEIGHT_W_LEFTUV0            : 14;
		unsigned int _rsv_14                         : 2;
		unsigned int IMG_WIDTH_W_LEFTUV0             : 14;
	} bits;
};

union REG_STITCHING_BLENDING_85 {
	unsigned int raw;
	struct {
		unsigned int IMG_HEIGHT_W_RIGHTUV0           : 14;
		unsigned int _rsv_14                         : 2;
		unsigned int IMG_WIDTH_W_RIGHTUV0            : 14;
	} bits;
};

union REG_STITCHING_BLENDING_86 {
	unsigned int raw;
	struct {
		unsigned int CROP_ENABLE_R_LEFTY0            : 1;
		unsigned int CROP_ENABLE_R_LEFTUV0           : 1;
		unsigned int CROP_ENABLE_R_RIGHTY0           : 1;
		unsigned int CROP_ENABLE_R_RIGHTUV0          : 1;
		unsigned int CROP_ENABLE_ALPHA0              : 1;
		unsigned int CROP_ENABLE_BETA0               : 1;
		unsigned int CROP_ENABLE_W_LEFTY0            : 1;
		unsigned int CROP_ENABLE_W_LEFTUV0           : 1;
		unsigned int CROP_ENABLE_W_RIGHTY0           : 1;
		unsigned int CROP_ENABLE_W_RIGHTUV0          : 1;
		unsigned int CROP_ENABLE_BLDY0               : 1;
		unsigned int CROP_ENABLE_BLDUV0              : 1;
		unsigned int CROP2BJ_LEFT_ENABLE0            : 1;
		unsigned int CROP2BJ_RIGHT_ENABLE0           : 1;
		unsigned int _rsv_14                         : 2;
		unsigned int CROP_ENABLE_R_LEFTY1            : 1;
		unsigned int CROP_ENABLE_R_LEFTUV1           : 1;
		unsigned int CROP_ENABLE_R_RIGHTY1           : 1;
		unsigned int CROP_ENABLE_R_RIGHTUV1          : 1;
		unsigned int CROP_ENABLE_ALPHA1              : 1;
		unsigned int CROP_ENABLE_BETA1               : 1;
		unsigned int CROP_ENABLE_BLDY1               : 1;
		unsigned int CROP_ENABLE_BLDUV1              : 1;
		unsigned int CROP2BJ_LEFT_ENABLE1            : 1;
		unsigned int CROP2BJ_RIGHT_ENABLE1           : 1;
	} bits;
};

union REG_STITCHING_BLENDING_MODE_SEL {
	unsigned int raw;
	struct {
		unsigned int MODE_SEL                        : 2;
	} bits;
};

union REG_STITCHING_BLENDING_SRC_SEL {
	unsigned int raw;
	struct {
		unsigned int SRC_SEL_LEFT0                   : 1;
		unsigned int SRC_SEL_RIGHT0                  : 1;
		unsigned int _rsv_2                          : 14;
		unsigned int SRC_SEL_LEFT1                   : 1;
		unsigned int SRC_SEL_RIGHT1                  : 1;
	} bits;
};

union REG_STITCHING_BLENDING_87 {
	unsigned int raw;
	struct {
		unsigned int CROP2BJ_LEFT_IMG_HEIGHT0        : 14;
		unsigned int _rsv_14                         : 2;
		unsigned int CROP2BJ_LEFT_IMG_WIDTH0         : 14;
	} bits;
};

union REG_STITCHING_BLENDING_88 {
	unsigned int raw;
	struct {
		unsigned int CROP2BJ_RIGHT_IMG_HEIGHT0       : 14;
		unsigned int _rsv_14                         : 2;
		unsigned int CROP2BJ_RIGHT_IMG_WIDTH0        : 14;
	} bits;
};

union REG_STITCHING_BLENDING_89 {
	unsigned int raw;
	struct {
		unsigned int CROP2BJ_LEFT_CROP_H_STR0        : 14;
		unsigned int _rsv_14                         : 2;
		unsigned int CROP2BJ_LEFT_CROP_H_END0        : 14;
	} bits;
};

union REG_STITCHING_BLENDING_90 {
	unsigned int raw;
	struct {
		unsigned int CROP2BJ_RIGHT_CROP_H_STR0       : 14;
		unsigned int _rsv_14                         : 2;
		unsigned int CROP2BJ_RIGHT_CROP_H_END0       : 14;
	} bits;
};

union REG_STITCHING_BLENDING_91 {
	unsigned int raw;
	struct {
		unsigned int CROP2BJ_LEFT_CROP_W_STR0        : 14;
		unsigned int _rsv_14                         : 2;
		unsigned int CROP2BJ_LEFT_CROP_W_END0        : 14;
	} bits;
};

union REG_STITCHING_BLENDING_92 {
	unsigned int raw;
	struct {
		unsigned int CROP2BJ_RIGHT_CROP_W_STR0       : 14;
		unsigned int _rsv_14                         : 2;
		unsigned int CROP2BJ_RIGHT_CROP_W_END0       : 14;
	} bits;
};

union REG_STITCHING_BLENDING_93 {
	unsigned int raw;
	struct {
		unsigned int CROP2BJ_LEFT_IMG_HEIGHT1        : 14;
		unsigned int _rsv_14                         : 2;
		unsigned int CROP2BJ_LEFT_IMG_WIDTH1         : 14;
	} bits;
};

union REG_STITCHING_BLENDING_94 {
	unsigned int raw;
	struct {
		unsigned int CROP2BJ_RIGHT_IMG_HEIGHT1       : 14;
		unsigned int _rsv_14                         : 2;
		unsigned int CROP2BJ_RIGHT_IMG_WIDTH1        : 14;
	} bits;
};

union REG_STITCHING_BLENDING_95 {
	unsigned int raw;
	struct {
		unsigned int CROP2BJ_LEFT_CROP_H_STR1        : 14;
		unsigned int _rsv_14                         : 2;
		unsigned int CROP2BJ_LEFT_CROP_H_END1        : 14;
	} bits;
};

union REG_STITCHING_BLENDING_96 {
	unsigned int raw;
	struct {
		unsigned int CROP2BJ_RIGHT_CROP_H_STR1       : 14;
		unsigned int _rsv_14                         : 2;
		unsigned int CROP2BJ_RIGHT_CROP_H_END1       : 14;
	} bits;
};

union REG_STITCHING_BLENDING_97 {
	unsigned int raw;
	struct {
		unsigned int CROP2BJ_LEFT_CROP_W_STR1        : 14;
		unsigned int _rsv_14                         : 2;
		unsigned int CROP2BJ_LEFT_CROP_W_END1        : 14;
	} bits;
};

union REG_STITCHING_BLENDING_98 {
	unsigned int raw;
	struct {
		unsigned int CROP2BJ_RIGHT_CROP_W_STR1       : 14;
		unsigned int _rsv_14                         : 2;
		unsigned int CROP2BJ_RIGHT_CROP_W_END1       : 14;
	} bits;
};

union REG_STITCHING_BLENDING_INT_MASK {
	unsigned int raw;
	struct {
		unsigned int INTERRUPT_MASK                  : 32;
	} bits;
};

union REG_STITCH_DMA_CTL_SYS_CONTROL {
	unsigned int raw;
	struct {
		unsigned int QOS_SEL						 : 1;
		unsigned int SW_QOS 						 : 1;
		unsigned int ENABLE_INV 					 : 1;
		unsigned int _rsv_3 						 : 5;
		unsigned int BASEH							 : 8;
		unsigned int BASE_SEL						 : 1;
		unsigned int STRIDE_SEL 					 : 1;
		unsigned int SEGLEN_SEL 					 : 1;
		unsigned int SEGNUM_SEL 					 : 1;
		unsigned int SLICE_ENABLE					 : 1;
		unsigned int UPDATE_BASE_ADDR				 : 1;
		unsigned int _rsv_22						 : 6;
		unsigned int DBG_SEL						 : 3;
	} bits;
};

union REG_STITCH_DMA_CTL_BASE_ADDR {
	unsigned int raw;
	struct {
		unsigned int BASEL							 : 32;
	} bits;
};

union REG_STITCH_DMA_CTL_DMA_SEGLEN {
	unsigned int raw;
	struct {
		unsigned int SEGLEN 						 : 28;
	} bits;
};

union REG_STITCH_DMA_CTL_DMA_STRIDE {
	unsigned int raw;
	struct {
		unsigned int STRIDE 						 : 28;
	} bits;
};

union REG_STITCH_DMA_CTL_DMA_SEGNUM {
	unsigned int raw;
	struct {
		unsigned int SEGNUM 						 : 14;
	} bits;
};

union REG_STITCH_DMA_CTL_DMA_STATUS {
	unsigned int raw;
	struct {
		unsigned int STATUS 						 : 32;
	} bits;
};

union REG_STITCH_DMA_CTL_DMA_SLICESIZE {
	unsigned int raw;
	struct {
		unsigned int SLICE_SIZE 					 : 6;
	} bits;
};

union REG_STITCH_DMA_CTL_DMA_DUMMY {
	unsigned int raw;
	struct {
		unsigned int DUMMY							 : 16;
		unsigned int PERF_PATCH_ENABLE				 : 1;
		unsigned int SEGLEN_LESS16_ENABLE			 : 1;
		unsigned int SYNC_PATCH_ENABLE				 : 1;
		unsigned int TRIG_PATCH_ENABLE				 : 1;
	} bits;
};

#ifdef __cplusplus
}
#endif

#endif /* _STITCH_REG_FIELDS_H_ */
