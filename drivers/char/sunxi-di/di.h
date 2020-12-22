//*****************************************************************************
//  All Winner Tech, All Right Reserved. 2006-2014 Copyright (c)
//
//  File name   :        di.h
//
//  Description :
//
//  History     :
//
//
//******************************************************************************
#ifndef __DI_H__
#define __DI_H__
#include <linux/types.h>

typedef enum
{
	DI_FORMAT_NV12      =0x00,
	DI_FORMAT_NV21      =0x01,
	DI_FORMAT_MB32_12   =0x02, //UV mapping like NV12
	DI_FORMAT_MB32_21   =0x03, //UV mapping like NV21

}__di_pixel_fmt_t;

typedef struct
{
	__u32 width;
	__u32 height;
}__di_rectsz_t;

typedef struct
{
	__u32                   addr[2];    // frame buffer�����ݵ�ַ
	__di_rectsz_t         	size;		//��λ��pixel
	__di_pixel_fmt_t      	format;
}__di_fb_t;

typedef struct
{
	__di_fb_t       input_fb;	//current frame fb
	__di_fb_t       pre_fb;     //previous frame fb
	__di_rectsz_t   source_regn; //current frame and previous frame process region
	__di_fb_t       output_fb;  //output frame fb
	__di_rectsz_t   out_regn;	//output frame region
	u32             field;      //process field <0-top field ; 1-bottom field>
	u32             top_field_first; //video infomation <0-is not top_field_first; 1-is top_field_first>
}__di_para_t;


int di_set_reg_base(unsigned int base);
unsigned int di_get_reg_base(void);
int di_set_init(__s32 mode);
int di_reset(void);
int di_start(void);
int di_irq_enable(unsigned int enable);
int di_get_status(void);
int di_irq_clear(void);
int di_set_para(__di_para_t *para, unsigned int in_flag_add, unsigned int out_flag_add, unsigned int field);
int di_sw_para_to_reg(__u8 type, __u8 format);
int di_internal_clk_enable(void);
int di_internal_clk_disable(void);

#endif
