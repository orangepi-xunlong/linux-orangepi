// $Module: isp_dma_ctl $
// $RegisterBank Version: V 1.0.00 $
// $Author: brian $
// $Date: Wed, 03 Nov 2021 05:09:34 PM $
//

#ifndef __REG_ISP_DMA_CTL_STRUCT_H__
#define __REG_ISP_DMA_CTL_STRUCT_H__

typedef unsigned int uint32_t;
typedef union {
	struct {
		/*Client QoS source selection
		0 : HW QoS (from client)
		1 : SW QoS (from DMA register);*/
		uint32_t reg_qos_sel:1;
		/*SW QoS setting;*/
		uint32_t reg_sw_qos:1;
		uint32_t rsv_2_7:6;
		/*Base address of memory layout (H part = bit32) , unit = byte;*/
		uint32_t reg_baseh:8;
		/*Base address selection
		0 : hw mode
		1 : sw mode;*/
		uint32_t reg_base_sel:1;
		/*Stride select:
		0 : hw mode
		1 : sw mode;*/
		uint32_t reg_stride_sel:1;
		/*seglen sel
		0 : hw mode
		1 : sw mode;*/
		uint32_t reg_seglen_sel:1;
		/*segnum select:
		0 : hw mode
		1 : sw mode;*/
		uint32_t reg_segnum_sel:1;
		/*slice mode enable;*/
		uint32_t reg_slice_enable:1;
		uint32_t rsv_21_27:7;
		/*reg_dbg_sel;*/
		uint32_t reg_dbg_sel:3;
	};
	uint32_t val;
} isp_dma_ctl_sys_control_c;
typedef union {
	struct {
		/*Base address of memory layout (L part = bit31~0) , unit = byte
		limitation = 16-byte aligned;*/
		uint32_t reg_basel:32;
	};
	uint32_t val;
} isp_dma_ctl_base_addr_c;
typedef union {
	struct {
		/*Segment length, unit : byte
		limitation = 16-byte aligned;*/
		uint32_t reg_seglen:24;
	};
	uint32_t val;
} isp_dma_ctl_base_seglen_c;
typedef union {
	struct {
		/*Stride length, the distance between two segments, unit : byte
		limitation = 16-byte aligned;*/
		uint32_t reg_stride:24;
	};
	uint32_t val;
} isp_dma_ctl_dma_stride_c;
typedef union {
	struct {
		/*Total segment number;*/
		uint32_t reg_segnum:13;
	};
	uint32_t val;
} isp_dma_ctl_dma_segnum_c;
typedef union {
	struct {
		/*DMA Status;*/
		uint32_t reg_status:32;
	};
	uint32_t val;
} isp_dma_ctl_dma_status_c;
typedef union {
	struct {
		/*slice buffer Size;*/
		uint32_t reg_slice_size:6;
	};
	uint32_t val;
} isp_dma_ctl_dma_slicesize_c;
typedef union {
	struct {
		/*dummy register;*/
		uint32_t reg_dummy:16;
		/*reg performace patch enable;*/
		uint32_t reg_perf_patch_enable:1;
		/*enable patch for (seglen+base[3:0] < 16);*/
		uint32_t reg_seglen_less16_enable:1;
		/*enable patch for shadow reg update;*/
		uint32_t reg_sync_patch_enable:1;
		/*enable patch for ip trigger reg update;*/
		uint32_t reg_trig_patch_enable:1;
	};
	uint32_t val;
} isp_dma_ctl_dma_dummy_c;
typedef struct {
	volatile isp_dma_ctl_sys_control_c sys_control;
	volatile isp_dma_ctl_base_addr_c base_addr;
	volatile isp_dma_ctl_base_seglen_c dma_seglen;
	volatile isp_dma_ctl_dma_stride_c dma_stride;
	volatile isp_dma_ctl_dma_segnum_c dma_segnum;
	volatile isp_dma_ctl_dma_status_c dma_status;
	volatile isp_dma_ctl_dma_slicesize_c dma_slicesize;
	volatile isp_dma_ctl_dma_dummy_c dma_dummy;
} isp_dma_ctl_c;
#ifdef __cplusplus

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
static void isp_dma_ctl_dump_ini(FILE* fp, isp_dma_ctl_c* p) {
	fprintf(fp, "reg_qos_sel = 0x%x\n",p->sys_control.reg_qos_sel);
	fprintf(fp, "reg_sw_qos = 0x%x\n",p->sys_control.reg_sw_qos);
	fprintf(fp, "reg_baseh = 0x%x\n",p->sys_control.reg_baseh);
	fprintf(fp, "reg_base_sel = 0x%x\n",p->sys_control.reg_base_sel);
	fprintf(fp, "reg_stride_sel = 0x%x\n",p->sys_control.reg_stride_sel);
	fprintf(fp, "reg_seglen_sel = 0x%x\n",p->sys_control.reg_seglen_sel);
	fprintf(fp, "reg_segnum_sel = 0x%x\n",p->sys_control.reg_segnum_sel);
	fprintf(fp, "reg_slice_enable = 0x%x\n",p->sys_control.reg_slice_enable);
	fprintf(fp, "reg_dbg_sel = 0x%x\n",p->sys_control.reg_dbg_sel);
	fprintf(fp, "reg_basel = 0x%x\n",p->base_addr.reg_basel);
	fprintf(fp, "reg_seglen = 0x%x\n",p->dma_seglen.reg_seglen);
	fprintf(fp, "reg_stride = 0x%x\n",p->dma_stride.reg_stride);
	fprintf(fp, "reg_segnum = 0x%x\n",p->dma_segnum.reg_segnum);
	fprintf(fp, "reg_status = 0x%x\n",p->dma_status.reg_status);
	fprintf(fp, "reg_slice_size = 0x%x\n",p->dma_slicesize.reg_slice_size);
	fprintf(fp, "reg_dummy = 0x%x\n",p->dma_dummy.reg_dummy);
	fprintf(fp, "reg_perf_patch_enable = 0x%x\n",p->dma_dummy.reg_perf_patch_enable);
	fprintf(fp, "reg_seglen_less16_enable = 0x%x\n",p->dma_dummy.reg_seglen_less16_enable);
	fprintf(fp, "reg_sync_patch_enable = 0x%x\n",p->dma_dummy.reg_sync_patch_enable);
	fprintf(fp, "reg_trig_patch_enable = 0x%x\n",p->dma_dummy.reg_trig_patch_enable);

}
static void isp_dma_ctl_print(isp_dma_ctl_c* p) {
    fprintf(stderr, "isp_dma_ctl\n");
	fprintf(stderr, "\tSYS_CONTROL.reg_qos_sel = 0x%x\n", p->sys_control.reg_qos_sel);
	fprintf(stderr, "\tSYS_CONTROL.reg_sw_qos = 0x%x\n", p->sys_control.reg_sw_qos);
	fprintf(stderr, "\tSYS_CONTROL.reg_baseh = 0x%x\n", p->sys_control.reg_baseh);
	fprintf(stderr, "\tSYS_CONTROL.reg_base_sel = 0x%x\n", p->sys_control.reg_base_sel);
	fprintf(stderr, "\tSYS_CONTROL.reg_stride_sel = 0x%x\n", p->sys_control.reg_stride_sel);
	fprintf(stderr, "\tSYS_CONTROL.reg_seglen_sel = 0x%x\n", p->sys_control.reg_seglen_sel);
	fprintf(stderr, "\tSYS_CONTROL.reg_segnum_sel = 0x%x\n", p->sys_control.reg_segnum_sel);
	fprintf(stderr, "\tSYS_CONTROL.reg_slice_enable = 0x%x\n", p->sys_control.reg_slice_enable);
	fprintf(stderr, "\tSYS_CONTROL.reg_dbg_sel = 0x%x\n", p->sys_control.reg_dbg_sel);
	fprintf(stderr, "\tBASE_ADDR.reg_basel = 0x%x\n", p->base_addr.reg_basel);
	fprintf(stderr, "\tDMA_SEGLEN.reg_seglen = 0x%x\n", p->dma_seglen.reg_seglen);
	fprintf(stderr, "\tDMA_STRIDE.reg_stride = 0x%x\n", p->dma_stride.reg_stride);
	fprintf(stderr, "\tDMA_SEGNUM.reg_segnum = 0x%x\n", p->dma_segnum.reg_segnum);
	fprintf(stderr, "\tDMA_STATUS.reg_status = 0x%x\n", p->dma_status.reg_status);
	fprintf(stderr, "\tDMA_SLICESIZE.reg_slice_size = 0x%x\n", p->dma_slicesize.reg_slice_size);
	fprintf(stderr, "\tDMA_DUMMY.reg_dummy = 0x%x\n", p->dma_dummy.reg_dummy);
	fprintf(stderr, "\tDMA_DUMMY.reg_perf_patch_enable = 0x%x\n", p->dma_dummy.reg_perf_patch_enable);
	fprintf(stderr, "\tDMA_DUMMY.reg_seglen_less16_enable = 0x%x\n", p->dma_dummy.reg_seglen_less16_enable);
	fprintf(stderr, "\tDMA_DUMMY.reg_sync_patch_enable = 0x%x\n", p->dma_dummy.reg_sync_patch_enable);
	fprintf(stderr, "\tDMA_DUMMY.reg_trig_patch_enable = 0x%x\n", p->dma_dummy.reg_trig_patch_enable);

}
#pragma GCC diagnostic pop
#define DEFINE_isp_dma_ctl_c(X) \
	 isp_dma_ctl_c X = \
{\
	{	/* sys_control.reg_qos_sel = */0x0,\
	/*.sys_control.reg_sw_qos = */0x0,\
	/*uint32_t rsv_2_7:6;=*/0,\
	/*.sys_control.reg_baseh = */0x0,\
	/*.sys_control.reg_base_sel = */0x0,\
	/*.sys_control.reg_stride_sel = */0x0,\
	/*.sys_control.reg_seglen_sel = */0x0,\
	/*.sys_control.reg_segnum_sel = */0x0,\
	/*.sys_control.reg_slice_enable = */0x0,\
	/*uint32_t rsv_21_27:7;=*/0,\
	/*.sys_control.reg_dbg_sel = */0x0,\
	},\
	{	/*.base_addr.reg_basel = */0x0,\
	},\
	{	/*.dma_seglen.reg_seglen = */0x0,\
	},\
	{	/*.dma_stride.reg_stride = */0x0,\
	},\
	{	/*.dma_segnum.reg_segnum = */0x0,\
	},\
	{	/*.dma_status.reg_status = */0x0,\
	},\
	{	/*.dma_slicesize.reg_slice_size = */0x1,\
	},\
	{	/*.dma_dummy.reg_dummy = */0x5a5a,\
	/*.dma_dummy.reg_perf_patch_enable = */0x1,\
	/*.dma_dummy.reg_seglen_less16_enable = */0x1,\
	/*.dma_dummy.reg_sync_patch_enable = */0x1,\
	/*.dma_dummy.reg_trig_patch_enable = */0x1,\
	}\
};
#else /* !ifdef __cplusplus */
#define _DEFINE_isp_dma_ctl_c \
{\
	.sys_control.reg_qos_sel = 0x0,\
	.sys_control.reg_sw_qos = 0x0,\
	.sys_control.reg_baseh = 0x0,\
	.sys_control.reg_base_sel = 0x0,\
	.sys_control.reg_stride_sel = 0x0,\
	.sys_control.reg_seglen_sel = 0x0,\
	.sys_control.reg_segnum_sel = 0x0,\
	.sys_control.reg_slice_enable = 0x0,\
	.sys_control.reg_dbg_sel = 0x0,\
	.base_addr.reg_basel = 0x0,\
	.dma_seglen.reg_seglen = 0x0,\
	.dma_stride.reg_stride = 0x0,\
	.dma_segnum.reg_segnum = 0x0,\
	.dma_status.reg_status = 0x0,\
	.dma_slicesize.reg_slice_size = 0x1,\
	.dma_dummy.reg_dummy = 0x5a5a,\
	.dma_dummy.reg_perf_patch_enable = 0x1,\
	.dma_dummy.reg_seglen_less16_enable = 0x1,\
	.dma_dummy.reg_sync_patch_enable = 0x1,\
	.dma_dummy.reg_trig_patch_enable = 0x1,\
}
#endif /* ifdef __cplusplus */
#endif //__REG_ISP_DMA_CTL_STRUCT_H__
