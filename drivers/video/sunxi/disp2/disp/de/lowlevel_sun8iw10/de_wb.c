//*****************************************************************************
//  All Winner Micro, All Right Reserved. 2006-2013 Copyright (c)
//
//  File name   :        de2_wb.c
//
//  Description :  DE2.0 Write-Back Controller  interface functions
//  History     :
//	Code by		:
//******************************************************************************

#include "de_wb_type.h"
#include <linux/kernel.h>
#include "../include.h"

//#include "../include.h"
#define WB_MODULE_NUMBER 1
#define WB_OFFSET 0x010000

static volatile __ewb_reg_t *wb_dev[WB_MODULE_NUMBER];
//static struct __einkwb_config_t wb_config;

int rgb2yuv601[3][4] = {
  {0x00bb,0x0274,0x003f,0x04200},
  {0x1f98,0x1ea5,0x01c1,0x20200},
  {0x01c1,0x1e67,0x1fd7,0x20200}};//rgb2yuv
int rgb2yuv709[3][4] = {
  {0x00da,0x02dc,0x0049,0x00200},
  {0xff8b,0xfe76,0x0200,0x20200},
  {0x0200,0xfe30,0xffd1,0x20200}};//rgb2yuv
int r2gray[3][4] = {
  {0x0155,0x0155,0x0156,0x0200},
  {0x0000,0x0000,0x0000,0x0000},
  {0x0000,0x0000,0x0000,0x0000}};//rgb2gray

//**********************************************************************************
// function       : WB_eink_Set_Reg_Base(unsigned int sel, unsigned int base)
// description    : setup write-back controller register base
// parameters     :
//                 	sel <controller select>
//					base <register base>
// return         :
//                  success
//***********************************************************************************
int WB_eink_Set_Reg_Base(unsigned int sel, unsigned int base)
{
	//__inf("sel=%d, base=0x%x\n", sel, base + WB_OFFSET);
	wb_dev[sel] = (__ewb_reg_t *)(base + WB_OFFSET);

	return 0;
}

//**********************************************************************************
// function       : WB_eink_Get_Reg_Base(unsigned int sel)
// description    : get write-back controller register base
// parameters     :
//                 	sel <controller select>
//
// return         :
//                  registers base
//***********************************************************************************
unsigned int WB_eink_Get_Reg_Base(unsigned int sel)
{
	unsigned int ret = 0;

	ret = (unsigned int)wb_dev[sel];

	return ret;
}

//**********************************************************************************
// function       : WB_eink_Init(unsigned int sel)
// description    : initial write-back controller registers
// parameters     :
//                 	sel <controller select>
//
// return         :
//                  success
//***********************************************************************************
int WB_eink_Init(unsigned int sel, unsigned int base_addr)
{
	WB_eink_Set_Reg_Base(sel, base_addr); //0x01000000
	return 0;
}

int WB_eink_Close(unsigned int sel)
{
	wb_dev[sel]->gctrl.dwval = 0;
	return 0;
}

//**********************************************************************************
// function       : WB_eink_Writeback_Enable(unsigned int sel)
// description    : enable write-back once
// parameters     :
//                 	sel <controller select>, en<0:disable; 1:enable>
// return         :
//                  success
//***********************************************************************************
int WB_eink_Writeback_Enable(unsigned int sel)
{
	wb_dev[sel]->gctrl.dwval |= 1;

	return 0;
}

//**********************************************************************************
// function       : WB_eink_Set_Para(unsigned int sel,disp_capture_config *cfg)
// description    : setup write-back controller parameters
// parameters     :
//                 	sel <controller select>
//					wb_config <write-back information,include input_fb and output_fb information>
// return         :
//                  0	--	success
//					-1	--	fail
// note  		  : Don't support YUV input yet 14-02-28
//					when write-back format is yuv, default 16-235 output
//***********************************************************************************
int WB_eink_Set_Para(unsigned int sel, __einkwb_config_t *cfg)
{
	unsigned int in_port;
	unsigned int in_w,in_h;
	unsigned int crop_x,crop_y,crop_w,crop_h;
	unsigned int out_addr[3];
	unsigned int out_buf_w,out_buf_h;
	unsigned int out_fmt;
	unsigned int out_window_w,out_window_h,out_window_x,out_window_y;
	unsigned int csc_std;
	unsigned int self_sync;
	//get para
	in_port      = 0;//cfg->disp;
	in_w         = cfg->width;
	in_h         = cfg->height;
	crop_x       = 0;
	crop_y       = 0;
	crop_w       = cfg->width;
	crop_h       = cfg->height;
	out_addr[0]  = cfg->addr[0];
	out_addr[1]  = cfg->addr[1];
	out_addr[2]  = cfg->addr[2];
	out_buf_w    = cfg->width;
	out_buf_h    = cfg->height;
	out_fmt      = 9;//DISP_FORMAT_Y;//cfg->format; fixed format
	out_window_x = 0;
	out_window_y = 0;
	out_window_w = cfg->width;
	out_window_h = cfg->height;

	csc_std=cfg->csc_std;/////////add para in struct disp_capture_config to init this, else use default bt601
	self_sync=(out_fmt==9)?1:0;/////////add para in struct disp_capture_config to init this, else control by format


	/* (0x30000000|(in_port<<16)|(self_sync == 1)?0x20:0x0); */
	wb_dev[sel]->gctrl.dwval |= 0x20000020;

	wb_dev[sel]->size.dwval = (in_w-1)|((in_h-1)<<16);
	/* input crop window */
	wb_dev[sel]->crop_coord.dwval = crop_x|((crop_y)<<16);
	wb_dev[sel]->crop_size.dwval = (crop_w - 1)|((crop_h - 1)<<16);
	/* output fmt */
	wb_dev[sel]->fmt.dwval = 9;

	if(out_fmt == 9/*DISP_FORMAT_Y*/)
	{
		out_addr[0] += (out_window_y * out_buf_w + out_window_x);
		out_addr[1] = 0;
		out_addr[2] = 0;
		wb_dev[sel]->wb_addr_A0.dwval = out_addr[0];
		wb_dev[sel]->wb_addr_A1.dwval = 0;
		wb_dev[sel]->wb_addr_A2.dwval = 0;
		wb_dev[sel]->wb_pitch0.dwval = out_buf_w;
		wb_dev[sel]->wb_pitch1.dwval = 0;

	  /* CSC */
		wb_dev[sel]->bypass.dwval |= 0x00000001;
  	if(csc_std==0 || csc_std>2)//bt601_limit
  	{
  		wb_dev[sel]->c00.bits.coff=rgb2yuv601[0][0];
  		wb_dev[sel]->c01.bits.coff=rgb2yuv601[0][1];
  		wb_dev[sel]->c02.bits.coff=rgb2yuv601[0][2];
  		wb_dev[sel]->c03.bits.cont=rgb2yuv601[0][3];

  		wb_dev[sel]->c10.bits.coff=rgb2yuv601[1][0];
  		wb_dev[sel]->c11.bits.coff=rgb2yuv601[1][1];
  		wb_dev[sel]->c12.bits.coff=rgb2yuv601[1][2];
  		wb_dev[sel]->c13.bits.cont=rgb2yuv601[1][3];

  		wb_dev[sel]->c20.bits.coff=rgb2yuv601[2][0];
  		wb_dev[sel]->c21.bits.coff=rgb2yuv601[2][1];
  		wb_dev[sel]->c22.bits.coff=rgb2yuv601[2][2];
  		wb_dev[sel]->c23.bits.cont=rgb2yuv601[2][3];
  	}
  	else if(csc_std==1)//bit709_full
	  {
  		wb_dev[sel]->c00.bits.coff=rgb2yuv709[0][0];
  		wb_dev[sel]->c01.bits.coff=rgb2yuv709[0][1];
  		wb_dev[sel]->c02.bits.coff=rgb2yuv709[0][2];
  		wb_dev[sel]->c03.bits.cont=rgb2yuv709[0][3];

  		wb_dev[sel]->c10.bits.coff=rgb2yuv709[1][0];
  		wb_dev[sel]->c11.bits.coff=rgb2yuv709[1][1];
  		wb_dev[sel]->c12.bits.coff=rgb2yuv709[1][2];
  		wb_dev[sel]->c13.bits.cont=rgb2yuv709[1][3];

  		wb_dev[sel]->c20.bits.coff=rgb2yuv709[2][0];
  		wb_dev[sel]->c21.bits.coff=rgb2yuv709[2][1];
  		wb_dev[sel]->c22.bits.coff=rgb2yuv709[2][2];
  		wb_dev[sel]->c23.bits.cont=rgb2yuv709[2][3];
	  }
  	else if(csc_std==2)//gray
	  {
  		wb_dev[sel]->c00.bits.coff=r2gray[0][0];
  		wb_dev[sel]->c01.bits.coff=r2gray[0][1];
  		wb_dev[sel]->c02.bits.coff=r2gray[0][2];
  		wb_dev[sel]->c03.bits.cont=r2gray[0][3];

  		wb_dev[sel]->c10.bits.coff=r2gray[1][0];
  		wb_dev[sel]->c11.bits.coff=r2gray[1][1];
  		wb_dev[sel]->c12.bits.coff=r2gray[1][2];
  		wb_dev[sel]->c13.bits.cont=r2gray[1][3];

  		wb_dev[sel]->c20.bits.coff=r2gray[2][0];
  		wb_dev[sel]->c21.bits.coff=r2gray[2][1];
  		wb_dev[sel]->c22.bits.coff=r2gray[2][2];
  		wb_dev[sel]->c23.bits.cont=r2gray[2][3];
	  }
	}
	return 0;
}

int WB_eink_Get_Status(unsigned int sel)
{
	unsigned int status;

	status = wb_dev[sel]->status.dwval & 0x71;
	if(status == 0x11)
	{
		return EWB_OK;
	}
	else if(status & 0x20)
	{
		return EWB_OVFL;
	}
	else if(status & 0x40)
	{
		return EWB_TIMEOUT;
	}
	else if(status & 0x100)
	{
		return EWB_BUSY;
	}
	else
	{
		return EWB_ERR;
	}
}
/*
int WB_eink_irq()
{
	int status;

	status=WB_eink_Get_Status(0);

	if(status!=EWB_OK)//not ok
	{
	}

	//do some buffer switching things here.......

	WB_eink_ClearINT(0);

	return 0;
}
*/
///INTERUPT
int WB_eink_EnableINT(unsigned int sel)
{
	//register irq routine
	//os_request_irq();

	wb_dev[sel]->intr.dwval |= 0x00000001;
	return 0;
}

int WB_eink_DisableINT(unsigned int sel)
{
	//unregister irq routine

	wb_dev[sel]->intr.dwval &= 0xfffffffe;
	return 0;
}

//write 1 to clear
int WB_eink_ClearINT(unsigned int sel)
{
	wb_dev[sel]->status.dwval |= (WB_END_IE | WB_FINISH_IE | WB_FIFO_OVERFLOW_ERROR_IE | WB_TIMEOUT_ERROR_IE);
	return 0;
}

