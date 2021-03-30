
#ifndef __CSIC__PARSER__REG__H__
#define __CSIC__PARSER__REG__H__

#include <linux/types.h>

#define MAX_CSIC_PRS_NUM 2

/*register value*/

enum prs_mode {
	PRS_NCSI = 0,
	PRS_MCSI,
};

enum csi_if {
	/* YUV(seperate syncs) */
	CSI_IF_INTLV = 0x00,	/* YUYV422 Interleaved or RAW
				 * (All data in one data bus) */
	CSI_IF_INTLV_16BIT = 0x01, /* 16 bit YUYV422 Interleaved */

	/* CCIR656(embedded syncs) */
	CSI_IF_BT656_1CH = 0x04,	/* BT656 1 channel */
	CSI_IF_BT1120_1CH = 0x05,	/* 16bit BT656(BT1120 like) 1 channel */
	CSI_IF_BT656_2CH = 0x0c,	/* BT656 2 channels (All data
					 * interleaved in one data bus) */
	CSI_IF_BT1120_2CH = 0x0d,	/* 16bit BT656(BT1120 like) 2 channels
					 * (All data interleaved in one data bus) */
	CSI_IF_BT656_4CH = 0x0e, 	/* BT656 4 channels (All data
					 * interleaved in one data bus) */
	CSI_IF_BT1120_4CH = 0x0f,	/* 16bit BT656(BT1120 like) 4 channels
					 * (All data interleaved in one data bus) */
};

enum output_mode {
	FIELD_MODE = 0,
	FRAME_MODE = 1,
};

enum input_seq {
	/* valid input is yuv422 or yuv420*/
	SEQ_YUYV = 0,
	SEQ_YVYU,
	SEQ_UYVY,
	SEQ_VYUY,
};

enum if_data_width {
	DW_8BIT = 0,
	DW_10BIT = 1,
	DW_12BIT = 2,
	DW_8PLUS2 = 3,
	DW_2MULTI8 = 4,
};

enum seq_8plus2 {
	B6_D98_D70 = 0,
	D92_B6_D10 = 1,
	D70_D98_B6 = 2,
	D70_B6_D98 = 3,
};

/*
 *  field dt mode
 */
enum field_dt_mode {
	FIELD_VSYNC = 0,
	FIELD_ONLY = 1,
	VSYNC_ONLY = 2,
};

/*
 * data clock edge
 */
enum clk_pol {
	CLK_RISING,		/* active rising */
	CLK_FALLING,	/* active falling */
};

/*
 * input reference polarity
 */
enum ref_pol {
	REF_NEGATIVE,		/* active low */
	REF_POSITIVE,		/* active high */
};

enum field_pol {
	/* For YUV HV timing, Field polarity */
	FIELD_NEG = 0,	/* field=0 indicate odd, field=1 indicate even */
	FIELD_POS = 1,	/* field=1 indicate odd, field=0 indicate even */
	/* For BT656 timing, Field sequence */
	FIELD_TF = 0,	/* top filed first */
	FIELD_BF = 1,	/* bottom field first */
};

enum src_type {
	PROGRESSED = 0,	/* progressed */
	INTERLACE = 1,	/* interlace */
};

enum cap_mode {
	SCAP = 1,
	VCAP,
};

enum prs_input_fmt {
	FMT_RAW = 0,		/* raw stream */
	FMT_YUV422 = 3,		/* yuv422 */
	FMT_YUV420 = 4,		/* yuv420 */
};


/*
 * parser interrupt select
 */
enum prs_int_sel {
	PRS_INT_PARA0 = 0X1,
	PRS_INT_PARA1 = 0X2,
	PRS_INT_MUL_ERR = 0X4,
};

/*register data struct*/

struct prs_ncsi_if_cfg {
	unsigned int pclk_shift;
	enum src_type type;
	enum field_pol field;
	enum ref_pol vref;
	enum ref_pol href;
	enum clk_pol clk;
	enum field_dt_mode field_dt;
	unsigned int ddr_sample;
	enum seq_8plus2 seq_8_2;
	enum if_data_width dw;
	enum input_seq seq;
	enum output_mode mode;
	enum csi_if intf;
};

struct prs_mcsi_if_cfg {
	enum input_seq input_seq;
	enum output_mode output_mode;
};

struct prs_cap_mode {
	unsigned int cap_mask;
	enum cap_mode mode;
};

struct prs_signal_status {
	unsigned int pclk_sta;
	unsigned int data_sta;
};

struct prs_ncsi_bt656_header {
	unsigned int ch0_id;
	unsigned int ch1_id;
	unsigned int ch2_id;
	unsigned int ch3_id;
};

struct prs_output_size {
	unsigned int hor_len;
	unsigned int hor_start;
	unsigned int ver_len;
	unsigned int ver_start;
};

struct prs_input_para {
	unsigned int src_type;
	unsigned int input_vt;
	unsigned int input_ht;
	unsigned int input_vb;
	unsigned int input_hb;
	unsigned int input_y;
	unsigned int input_x;
};

struct prs_int_status {
	bool input_src_pd0;
	bool input_src_pd1;
	bool mul_err_pd;
};

int csic_prs_set_base_addr(unsigned int sel, unsigned long addr);
void csic_prs_enable(unsigned int sel);
void csic_prs_disable(unsigned int sel);
void csic_prs_mode(unsigned int sel, enum prs_mode mode);
void csic_prs_pclk_en(unsigned int sel, unsigned int en);
void csic_prs_pclk_en(unsigned int sel, unsigned int en);
void csic_prs_ncsi_en(unsigned int sel, unsigned int en);
void csic_prs_mcsi_en(unsigned int sel, unsigned int en);

void csic_prs_ncsi_if_cfg(unsigned int sel, struct prs_ncsi_if_cfg *if_cfg);
void csic_prs_mcsi_if_cfg(unsigned int sel, struct prs_mcsi_if_cfg *if_cfg);

void csic_prs_capture(unsigned int sel, unsigned int ch,
				struct prs_cap_mode *mode);
void csic_prs_signal_status(unsigned int sel,
				struct prs_signal_status *status);
void csic_prs_ncsi_bt656_header_cfg(unsigned int sel,
				struct prs_ncsi_bt656_header *header);
void csic_prs_input_fmt_cfg(unsigned int sel, unsigned int ch,
				enum prs_input_fmt fmt);
void csic_prs_output_size_cfg(unsigned int sel, unsigned int ch,
				struct prs_output_size *size);

/* for debug */
void csic_prs_input_para_get(unsigned int sel, unsigned int ch,
				struct prs_input_para *para);
void csic_prs_int_enable(unsigned int sel, unsigned int ch,
				enum prs_int_sel interrupt);
void csic_prs_int_disable(unsigned int sel, unsigned int ch,
				enum prs_int_sel interrupt);
void csic_prs_int_get_status(unsigned int sel, unsigned int ch,
				struct prs_int_status *status);
void csic_prs_int_clear_status(unsigned int sel, unsigned int ch,
				enum prs_int_sel interrupt);

#endif /* __CSIC__PARSER__REG__H__ */
