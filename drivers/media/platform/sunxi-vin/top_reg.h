
#ifndef __CSIC__TOP__REG__H__
#define __CSIC__TOP__REG__H__

#include <linux/types.h>

#define MAX_CSIC_TOP_NUM 2
#define ISP_INPUT(x, y, i, j) ISP##x##_INPUT##y##_PARSER##i##_CH##j
#define VIPP_INPUT(x, i, j) VIPP##x##_INPUT_ISP##i##_CH##j

#define ISP_VIPP_SEL_SEPARATE

/*register value*/

/*
 * isp0 input0 source
 */
enum isp0_input0 {
	ISP_INPUT(0, 0, 0, 0),/*ISP0_INPUT0_PARSER0_CH0*/
	ISP_INPUT(0, 0, 1, 0),/*ISP0_INPUT0_PARSER1_CH0*/
	ISP_INPUT(0, 0, 1, 1),/*ISP0_INPUT0_PARSER1_CH1*/
};

enum isp0_input1 {
	ISP_INPUT(0, 1, 0, 1),
	ISP_INPUT(0, 1, 1, 1),
};

enum isp0_input2 {
	ISP_INPUT(0, 2, 0, 2),
	ISP_INPUT(0, 2, 1, 2),
};

enum isp0_input3 {
	ISP_INPUT(0, 3, 0, 3),
	ISP_INPUT(0, 3, 1, 3),
};

enum isp1_input0 {
	ISP_INPUT(1, 0, 1, 0),/*ISP1_INPUT0_PARSER1_CH0*/
	ISP_INPUT(1, 0, 0, 0),/*ISP1_INPUT0_PARSER0_CH0*/
	ISP_INPUT(1, 0, 0, 1),/*ISP1_INPUT0_PARSER0_CH1*/
};

enum isp1_input1 {
	ISP_INPUT(1, 1, 0, 1),
	ISP_INPUT(1, 1, 1, 1),
};

enum isp1_input2 {
	ISP_INPUT(1, 2, 0, 2),
	ISP_INPUT(1, 2, 1, 2),
};

enum isp1_input3 {
	ISP_INPUT(1, 3, 0, 3),
	ISP_INPUT(1, 3, 1, 3),
};

/*
 * vipp0 input source
 */
enum vipp0_input {
	VIPP_INPUT(0, 0, 0),/*VIPP0_INPUT_ISP0_CH0*/
	VIPP_INPUT(0, 1, 0),/*VIPP0_INPUT_ISP1_CH0*/
};

enum vipp1_input {
	VIPP_INPUT(1, 0, 0),
	VIPP_INPUT(1, 1, 0),
	VIPP_INPUT(1, 0, 1),
	VIPP_INPUT(1, 1, 1),
};

enum vipp2_input {
	VIPP_INPUT(2, 0, 0),
	VIPP_INPUT(2, 1, 0),
	VIPP_INPUT(2, 0, 2),
	VIPP_INPUT(2, 1, 2),
};

enum vipp3_input {
	VIPP_INPUT(3, 0, 0),
	VIPP_INPUT(3, 1, 0),
	VIPP_INPUT(3, 0, 3),
	VIPP_INPUT(3, 1, 3),
	VIPP_INPUT(3, 1, 1),
};

/*register data struct*/

#ifdef ISP_VIPP_SEL_SEPARATE
union csic_isp_input {
	enum isp0_input0 isp0_in0;
	enum isp0_input1 isp0_in1;
	enum isp0_input2 isp0_in2;
	enum isp0_input3 isp0_in3;
	enum isp1_input0 isp1_in0;
	enum isp1_input1 isp1_in1;
	enum isp1_input2 isp1_in2;
	enum isp1_input3 isp1_in3;
};
#else
union csic_vipp_input {
	enum vipp0_input vipp0_in;
	enum vipp1_input vipp1_in;
	enum vipp2_input vipp2_in;
	enum vipp3_input vipp3_in;
};
#endif

struct csic_feature_list {
	unsigned int dma_num;
	unsigned int vipp_num;
	unsigned int isp_num;
	unsigned int ncsi_num;
	unsigned int mcsi_num;
	unsigned int parser_num;
};

struct csic_version {
	unsigned int ver_big;
	unsigned int ver_small;
};

int csic_top_set_base_addr(unsigned int sel, unsigned long addr);
void csic_top_enable(unsigned int sel);
void csic_top_disable(unsigned int sel);
void csic_top_wdr_mode(unsigned int sel, unsigned int en);
void csic_top_sram_pwdn(unsigned int sel, unsigned int en);
void csic_top_switch_ncsi(unsigned int sel, unsigned int en);
void csic_top_version_read_en(unsigned int sel, unsigned int en);
#ifdef ISP_VIPP_SEL_SEPARATE
void csic_isp0_input0_select(unsigned int sel, enum isp0_input0 isp0_in0);
void csic_isp0_input1_select(unsigned int sel, enum isp0_input1 isp0_in1);
void csic_isp0_input2_select(unsigned int sel, enum isp0_input2 isp0_in2);
void csic_isp0_input3_select(unsigned int sel, enum isp0_input3 isp0_in3);

void csic_isp1_input0_select(unsigned int sel, enum isp1_input0 isp1_in0);
void csic_isp1_input1_select(unsigned int sel, enum isp1_input1 isp1_in1);
void csic_isp1_input2_select(unsigned int sel, enum isp1_input2 isp1_in2);
void csic_isp1_input3_select(unsigned int sel, enum isp1_input3 isp1_in3);

void csic_vipp0_input_select(unsigned int sel, enum vipp0_input vipp0_in);
void csic_vipp1_input_select(unsigned int sel, enum vipp1_input vipp1_in);
void csic_vipp2_input_select(unsigned int sel, enum vipp2_input vipp2_in);
void csic_vipp3_input_select(unsigned int sel, enum vipp3_input vipp3_in);
#else
void csic_isp_input_select(unsigned int sel, unsigned int x, unsigned int y,
				union csic_isp_input isp_in);
void csic_vipp_input_select(unsigned int sel, unsigned int x,
				union csic_vipp_input vipp_in);
#endif

void csic_feature_list_get(unsigned int sel, struct csic_feature_list *fl);
void csic_version_get(unsigned int sel, struct csic_version *v);

#endif /* __CSIC__TOP__REG__H__ */
