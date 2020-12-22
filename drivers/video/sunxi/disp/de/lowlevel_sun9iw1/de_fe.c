//*****************************************************************************
//  All Winner Tech, All Right Reserved. 2006-2012 Copyright (c)
//
//  File name   :        de_scal_bsp.c
//
//  Description :  display engine scaler base functions implement for aw1633
//
//  History     :
//                 2012/06/05      zchmin       v0.1    Initial version based aw1623
//******************************************************************************


#include "de_fe.h"

static volatile __de_scal_dev_t *scal_dev[3];
static u32 de_scal_ch0_offset;
static u32 de_scal_ch1_offset;
static u32 de_scal_ch2_offset;
static u32 de_scal_trd_fp_en = 0;
static u32 de_scal_trd_itl_en = 0;
static u32 de_scal_ch0r_offset;
static u32 de_scal_ch1r_offset;
static u32 de_scal_ch2r_offset;

//*********************************************************************************************
// function          : DE_SCAL_Set_Reg_Base(u8 sel, u32 base)
// description      : set scale reg base
// parameters     :
//                 sel <scaler select>
//                 base  <reg base>
// return              :
//               success
//***********************************************************************************************
s32 DE_SCAL_Set_Reg_Base(u32 sel, u32 base)
{
	scal_dev[sel]=(__de_scal_dev_t *)base;

	return 0;
}

//*********************************************************************************************
// function         : DE_SCAL_Get_Reg_Base(u8 sel)
// description     : get scale reg base
// parameters    :
//                 sel <scaler select>
// return            :
//               reg base
//***********************************************************************************************
u32 DE_SCAL_Get_Reg_Base(u8 sel)
{
	u32 ret = 0;

	ret = (u32)(scal_dev[sel]);

	return ret;
}

//*********************************************************************************************
// function       : DE_SCAL_Config_Src(u8 sel, __scal_buf_addr_t *addr, __scal_src_size_t *size,
//                                       __scal_src_type_t *type, u8 field, u8 dien)
// description    : scaler source concerning parameter configure
// parameters     :
//                 sel <scaler select>
//                 addr  <frame buffer address for 3 channel, 32 bit absolute address>
//                 size <scale region define,  src size, offset, scal size>
//                 type <src data type, include byte sequence, mode, format, pixel sequence>
//                 field <frame/field data get>
//                 dien <deinterlace enable>
// return         :
//               success
// history        : 2012/06/05, added rgb565/rgb1555/rgb4444 support
//***********************************************************************************************
s32 DE_SCAL_Config_Src(u8 sel, __scal_buf_addr_t *addr, __scal_src_size_t *size,
                         __scal_src_type_t *type, u8 field, u8 dien)
{
    u8 w_shift, h_shift;
	u32 image_w0, image_w1;
	u32 x_off0, y_off0, x_off1, y_off1;
	u32 in_w0, in_h0, in_w1, in_h1;
	u8 rgb16mode = 0;

	image_w0 = size->src_width;
	in_w0 = size->scal_width;
	in_h0 = size->scal_height;
	x_off0 = size->x_off;
	y_off0 = (field | dien) ? (size->y_off & 0xfffffffe) : size->y_off;  //scan mod enable or deinterlace, odd dy un-support

//    if(sel == 0)   //scaler 0 scaler 1
    {
        if(type->fmt == DE_SCAL_INYUV422 || type->fmt == DE_SCAL_INYUV420)
        {
            w_shift = 1;
        	image_w1 = (image_w0 + 0x1)>>w_shift;
        	in_w1 = (in_w0 + 0x1)>>w_shift;
        	x_off1 = (x_off0)>>w_shift;
        	if(type->mod == DE_SCAL_INTERLEAVED)
        	{
        	    image_w0 = (image_w0 + 0x1) & 0xfffffffe;
                image_w1 = image_w0>>1;
                in_w0 &= 0xfffffffe;
                in_w1 = in_w0>>0x1;
        	    x_off0 &= 0xfffffffe;
        		x_off1 = x_off0>>w_shift;
        	}
        }
        else if(type->fmt == DE_SCAL_INYUV411)
        {
            w_shift = 2;
        	image_w1 = (image_w0 + 0x3)>>w_shift;
        	in_w1 = (in_w0 + 0x3)>>w_shift;
        	x_off1 = (x_off0)>>w_shift;
        }
		else if(type->fmt == DE_SCAL_INRGB565 || type->fmt == DE_SCAL_INRGB4444 || type->fmt == DE_SCAL_INRGB1555)
		{
			w_shift = 0;
	        rgb16mode = 1;
	        image_w1 = image_w0;
	        in_w1 = in_w0;
	        x_off1 = x_off0;
		}
        else
        {
            w_shift = 0;
        	image_w1 = image_w0;
        	in_w1 = in_w0;
        	x_off1 = x_off0;
        }
        if(type->fmt == DE_SCAL_INYUV420)
        {
            h_shift = 1;
        	in_h1 = (in_h0 + 0x1)>>h_shift;
        	y_off1 = (y_off0)>>h_shift;
        }
        else
        {
            h_shift = 0;
        	in_h1 = in_h0;
        	y_off1 = y_off0;
        }
    }
    //added no-zero limited
    in_h0 = (in_h0!=0) ? in_h0 : 1;
	in_h1 = (in_h1!=0) ? in_h1 : 1;
	in_w0 = (in_w0!=0) ? in_w0 : 1;
	in_w1 = (in_w1!=0) ? in_w1 : 1;

	if(type->mod == DE_SCAL_PLANNAR)
	{
	    scal_dev[sel]->linestrd0.dwval = image_w0;
		scal_dev[sel]->linestrd1.dwval = image_w1;
		scal_dev[sel]->linestrd2.dwval = image_w1;

		if(type->fmt == DE_SCAL_INYUV420)
		{
#if defined(CONFIG_ARCH_SUN8IW3P1)
			scal_dev[sel]->linestrd0.dwval = ((image_w0+15)>>4)<<4;
			scal_dev[sel]->linestrd1.dwval = ((image_w1+15)>>4)<<4;
			scal_dev[sel]->linestrd2.dwval = ((image_w1+15)>>4)<<4;
#else
			scal_dev[sel]->linestrd0.dwval = ((image_w0+15)>>4)<<4;
			scal_dev[sel]->linestrd1.dwval = ((image_w1+7)>>3)<<3;
			scal_dev[sel]->linestrd2.dwval = ((image_w1+7)>>3)<<3;
#endif
		}
        de_scal_ch0_offset = image_w0 * y_off0 + x_off0;
        de_scal_ch1_offset = image_w1 * y_off1 + x_off1;
        de_scal_ch2_offset = image_w1 * y_off1 + x_off1;
		scal_dev[sel]->buf_addr0.dwval = addr->ch0_addr+ de_scal_ch0_offset;
		scal_dev[sel]->buf_addr1.dwval = addr->ch1_addr+ de_scal_ch1_offset;
		scal_dev[sel]->buf_addr2.dwval = addr->ch2_addr+ de_scal_ch2_offset;
	}
	else if(type->mod == DE_SCAL_INTERLEAVED)
	{
	    scal_dev[sel]->linestrd0.dwval = image_w0<<(0x2 - w_shift  - rgb16mode);
		scal_dev[sel]->linestrd1.dwval = 0x0;
		scal_dev[sel]->linestrd2.dwval = 0x0;

        de_scal_ch0_offset = ((image_w0 * y_off0 + x_off0)<<(0x2 - w_shift - rgb16mode));
        de_scal_ch1_offset = 0x0;
        de_scal_ch2_offset = 0x0;
		scal_dev[sel]->buf_addr0.dwval = addr->ch0_addr+ de_scal_ch0_offset;
		scal_dev[sel]->buf_addr1.dwval = addr->ch1_addr;
		scal_dev[sel]->buf_addr2.dwval = addr->ch2_addr;
	}
	else if(type->mod == DE_SCAL_UVCOMBINED)
	{
	    scal_dev[sel]->linestrd0.dwval = image_w0;
		scal_dev[sel]->linestrd1.dwval = image_w1<<1;
		scal_dev[sel]->linestrd2.dwval = 0x0;

        de_scal_ch0_offset = image_w0 * y_off0 + x_off0;
        de_scal_ch1_offset = (((image_w1) * (y_off1) + (x_off1))<<1);
        de_scal_ch2_offset = 0x0;
		scal_dev[sel]->buf_addr0.dwval = addr->ch0_addr+ de_scal_ch0_offset;
		scal_dev[sel]->buf_addr1.dwval = addr->ch1_addr+ de_scal_ch1_offset;
		scal_dev[sel]->buf_addr2.dwval = addr->ch2_addr;
	}
	else if(type->mod == DE_SCAL_PLANNARMB)
	{
	    image_w0 = (image_w0 + 0xf)&0xfff0;
        image_w1 = (image_w1 + (0xf>>w_shift)) & (~(0xf>>w_shift));

        //block offset
        scal_dev[sel]->tb_off0.bits.x_offset0 = (x_off0 & 0x0f);
        scal_dev[sel]->tb_off0.bits.y_offset0 = (y_off0 & 0x0f);
        scal_dev[sel]->tb_off0.bits.x_offset1 = (((x_off0 & 0x0f) & (0x0f)) + in_w0 + 0x0f) & 0x0f;
        scal_dev[sel]->tb_off1.bits.x_offset0 = ((x_off1)&(0x0f>>w_shift));
        scal_dev[sel]->tb_off1.bits.y_offset0 = ((y_off1)&(0x0f>>h_shift));
        scal_dev[sel]->tb_off1.bits.x_offset1 = ((((x_off1)&(0x0f>>w_shift)) & (0x0f>>w_shift)) + (in_w1) + (0x0f>>w_shift))&(0x0f>>w_shift);
		scal_dev[sel]->tb_off2.bits.x_offset0 = scal_dev[sel]->tb_off1.bits.x_offset0;
		scal_dev[sel]->tb_off2.bits.y_offset0 = scal_dev[sel]->tb_off1.bits.y_offset0;
		scal_dev[sel]->tb_off2.bits.x_offset1 = scal_dev[sel]->tb_off1.bits.x_offset1;

		scal_dev[sel]->linestrd0.dwval = (image_w0 - 0xf)<<4;
		scal_dev[sel]->linestrd1.dwval = ((image_w1) <<(0x04-h_shift)) - ((0xf>>h_shift)<<(0x04-w_shift));
		scal_dev[sel]->linestrd2.dwval = scal_dev[sel]->linestrd1.dwval;

        de_scal_ch0_offset = ((image_w0 + 0x0f)&0xfff0) * (y_off0&0xfff0) + ((y_off0&0x00f)<<4) + ((x_off0&0xff0)<<4);
        de_scal_ch1_offset = (((image_w1) + (0x0f>>w_shift)) &(0xfff0>>w_shift)) * ((y_off1) & (0xfff0>>h_shift)) +
                             ((((y_off1) & (0x00f>>h_shift))<<(0x4 - w_shift))) + (((x_off1) & (0xfff0>>w_shift))<<(0x4 - h_shift));
        de_scal_ch2_offset = de_scal_ch1_offset;
		scal_dev[sel]->buf_addr0.dwval = addr->ch0_addr+ de_scal_ch0_offset;
		scal_dev[sel]->buf_addr1.dwval = addr->ch1_addr+ de_scal_ch1_offset;
		scal_dev[sel]->buf_addr2.dwval = addr->ch2_addr+ de_scal_ch2_offset;
	}
	else if(type->mod == DE_SCAL_UVCOMBINEDMB)
	{
	    image_w0 = (image_w0 + 0x1f)&0xffffffe0;
		image_w1 = (image_w1 + 0x0f)&0xfffffff0;
		//block offset
		scal_dev[sel]->tb_off0.bits.x_offset0 = (x_off0 & 0x1f);
        scal_dev[sel]->tb_off0.bits.y_offset0 = (y_off0 & 0x1f);
		scal_dev[sel]->tb_off0.bits.x_offset1 = (((x_off0 & 0x1f) & 0x1f) + in_w0 + 0x1f) &0x1f;
		scal_dev[sel]->tb_off1.bits.x_offset0 = (((x_off1)<<1)&0x1f);
        scal_dev[sel]->tb_off1.bits.y_offset0 = ((y_off1)&0x1f);
        scal_dev[sel]->tb_off1.bits.x_offset1 = (((((x_off1)<<1)&0x1f) & 0x1e) + ((in_w1)<<1) + 0x1f) & 0x1f;

		scal_dev[sel]->linestrd0.dwval = (((image_w0 + 0x1f)&0xffe0) - 0x1f)<<0x05;
        scal_dev[sel]->linestrd1.dwval = (((((image_w1)<<1)+0x1f)&0xffe0) - 0x1f) << 0x05;
        scal_dev[sel]->linestrd2.dwval = 0x00;

        de_scal_ch0_offset = ((image_w0 + 0x1f) &0xffe0) * (y_off0& 0xffe0) + ((y_off0& 0x01f)<<5) + ((x_off0& 0xffe0)<<5);
        de_scal_ch1_offset = (((image_w1<< 0x01)+0x1f)&0xffe0) * ((y_off1) & 0xffe0) + (((y_off1) & 0x01f)<<5) + (((x_off1<<0x01) & 0xffe0)<<5);
        de_scal_ch2_offset = 0x0;
		scal_dev[sel]->buf_addr0.dwval = addr->ch0_addr+ de_scal_ch0_offset;
        scal_dev[sel]->buf_addr1.dwval = addr->ch1_addr+ de_scal_ch1_offset;
        scal_dev[sel]->buf_addr2.dwval = 0x0;
	}

	scal_dev[sel]->input_fmt.bits.byte_seq = type->byte_seq;
	scal_dev[sel]->input_fmt.bits.data_mod = type->mod;
	scal_dev[sel]->input_fmt.bits.data_fmt = type->fmt;
	scal_dev[sel]->input_fmt.bits.data_ps = type->ps;

	scal_dev[sel]->ch0_insize.bits.in_width = in_w0 - 1;
	scal_dev[sel]->ch0_insize.bits.in_height = in_h0 - 1;
	scal_dev[sel]->ch1_insize.bits.in_width = in_w1 - 1;
	scal_dev[sel]->ch1_insize.bits.in_height = in_h1 - 1;

	scal_dev[sel]->trd_ctrl.dwval = 0;
    return 0;
}


//*********************************************************************************************
// function       : DE_SCAL_Set_Fb_Addr(u8 sel, __scal_buf_addr_t *addr)
// description    : scaler change frame buffer address, only change start address parameters
// parameters     :
//                 sel <scaler select>
//                 addr  <frame buffer address for 3 channel, 32 bit absolute address>
// return         :
//               success
//***********************************************************************************************
s32 DE_SCAL_Set_Fb_Addr(u8 sel, __scal_buf_addr_t *addr)
{
    scal_dev[sel]->buf_addr0.dwval = addr->ch0_addr+ de_scal_ch0_offset;
    scal_dev[sel]->buf_addr1.dwval = addr->ch1_addr+ de_scal_ch1_offset;
    scal_dev[sel]->buf_addr2.dwval = addr->ch2_addr+ de_scal_ch2_offset;

    return 0;
}


//*********************************************************************************************
// function       : DE_SCAL_Set_Init_Phase(u8 sel, __scal_scan_mod_t *in_scan, __scal_src_size_t *in_size,
//                                           __scal_src_type_t *in_type, __scal_scan_mod_t *out_scan,
//                                           __scal_out_size_t *out_size, __scal_out_type_t *out_type, u8 dien)
// description    : set scaler init phase according to in/out information
// parameters     :
//                 sel <scaler select>
//                 in_scan <scale src data scan mode, if deinterlaceing open, the scan mode is progressive for scale>
//                 in_size <scale region define,  src size, offset, scal size>
//                 in_type <src data type>
//                 out_scan <scale output data scan mode>
//                 out_size <scale out size>
//                 out_type <output data format>
//                 dien <deinterlace enable>
// return         :
//               success
//note            : when 3D mode(when output mode is HDMI_FPI), the Function Set_3D_Ctrl msut carry out first.
//                  when 3D mode(HDMI_FPI), this function used once
//***********************************************************************************************
s32 DE_SCAL_Set_Init_Phase(u8 sel, __scal_scan_mod_t *in_scan, __scal_src_size_t *in_size,
                             __scal_src_type_t *in_type, __scal_scan_mod_t *out_scan,
                             __scal_out_size_t *out_size, __scal_out_type_t *out_type, u8 dien)
{
     s32 ch0_h_phase=0, ch0_v_phase0=0, ch0_v_phase1=0, ch12_h_phase=0, ch12_v_phase0=0, ch12_v_phase1=0;
	 u8 h_shift=0, w_shift=0;
     s32 in_h0, in_h1, out_h0, out_h1;


     //set register value
	 scal_dev[sel]->output_fmt.bits.scan_mod = out_scan->field;
     scal_dev[sel]->input_fmt.bits.scan_mod = out_scan->field ? 0x0 : in_scan->field;  //out scan and in scan are not valid at the same time
     if(de_scal_trd_itl_en == 0)   //added for 3D top_bottom mode, zchmin 2011-05-04, note: when HDMI_FPI, the input inscan mode must open,
 	{
		 scal_dev[sel]->field_ctrl.bits.field_loop_mod = 0x0;
		 scal_dev[sel]->field_ctrl.bits.valid_field_cnt = 0x1-0x1;
		 scal_dev[sel]->field_ctrl.bits.field_cnt = in_scan->bottom;
 	}


     //sampling method, phase
	 if(in_type->fmt == DE_SCAL_INYUV420)
	 {
	     if(in_type->sample_method == 0x0)  //
	     {
	         ch0_h_phase = 0x400;
			 ch0_v_phase0 = 0x400;
			 ch0_v_phase1 = 0x400;
			 ch12_h_phase = 0xfc400;   //-0.25
			 ch12_v_phase0 = 0xfc400;  //-0.25
			 ch12_v_phase1 = 0xfc400;  //-0.25
	     }
		 else
		 {
		     ch0_h_phase = 0x400;
			 ch0_v_phase0 = 0x400;
			 ch0_v_phase1 = 0x400;
			 ch12_h_phase = 0x400;       //0
			 ch12_v_phase0 = 0xfc400;  //-0.25
			 ch12_v_phase1 = 0xfc400;  //-0.25
		 }
	 }
	 else  //can added yuv411 or yuv420 init phase for sample method
	 {
	     ch0_h_phase = 0x400;
		 ch0_v_phase0 = 0x400;
		 ch0_v_phase1 = 0x400;
		 ch12_h_phase = 0x400;
		 ch12_v_phase0 = 0x400;
		 ch12_v_phase1 = 0x400;
	 }

     //location offset
     w_shift = (in_type->fmt == DE_SCAL_INYUV420 || in_type->fmt == DE_SCAL_INYUV422) ? 0x1 : ((in_type->fmt == DE_SCAL_INYUV411) ? 0x2 : 0x0);
	 h_shift = (in_type->fmt == DE_SCAL_INYUV420) ? 0x1 : 0x0;

     //deinterlace and in scan mode enable, //dy
     if(((dien == 0x01) || (in_scan->field== 0x1)) && (in_size->y_off & 0x1)&& (in_scan->bottom == 0x0))  //
     {
         ch0_v_phase0 = (ch0_v_phase0 + 0x10000) & SCALINITPASELMT;
         ch12_v_phase0 = (ch12_v_phase0 + 0x10000) & SCALINITPASELMT;
     }
     else
     {
         ch12_v_phase0 = (ch12_v_phase0 + (in_size->y_off & ((1<<h_shift)-1))*(0x10000>>h_shift)) & SCALINITPASELMT;
         ch12_v_phase1 = ch12_v_phase0;
     }

	 //dx
	 scal_dev[sel]->ch0_horzphase.bits.phase = ch0_h_phase;
	 scal_dev[sel]->ch1_horzphase.bits.phase = (ch12_h_phase + (in_size->x_off & ((1<<w_shift) - 1)) * (0x10000>>w_shift)) & SCALINITPASELMT;

     //outinterlace,
     if(out_scan->field == 0x1)  //outinterlace enable
     {
         in_h0 = in_size->scal_height;
         in_h1 = (in_type->fmt == DE_SCAL_INYUV420) ? (in_h0+0x1)>>1: in_h0;
         out_h0 = out_size->height;
         out_h1 = (out_type->fmt == DE_SCAL_OUTPYUV420) ? (out_h0+0x1)>>1 : out_h0;

		 //added no-zero limited
		in_h0 = (in_h0!=0) ? in_h0 : 1;
		in_h1 = (in_h1!=0) ? in_h1 : 1;
		out_h0 = (out_h0!=0) ? out_h0 : 1;
		out_h1 = (out_h1!=0) ? out_h1 : 1;

         if(in_scan->bottom == 0x0)
         {
	         scal_dev[sel]->ch0_vertphase0.bits.phase = ch0_v_phase0 + ((in_h0>>in_scan->field)<<16)/(out_h0);
             scal_dev[sel]->ch0_vertphase1.bits.phase = ch0_v_phase0;
             scal_dev[sel]->ch1_vertphase0.bits.phase = ch12_v_phase0 + ((in_h1>>in_scan->field)<<16)/(out_h1);
             scal_dev[sel]->ch1_vertphase1.bits.phase = ch12_v_phase0;
         }
         else
         {
             scal_dev[sel]->ch0_vertphase0.bits.phase = ch0_v_phase1 + ((in_h0>>in_scan->field)<<16)/(out_h0);
             scal_dev[sel]->ch0_vertphase1.bits.phase = ch0_v_phase1;
             scal_dev[sel]->ch1_vertphase0.bits.phase = ch12_v_phase1 + ((in_h1>>in_scan->field)<<16)/(out_h1);
             scal_dev[sel]->ch1_vertphase1.bits.phase = ch12_v_phase1;
         }
     }
     else  //outinterlace disable
     {
         scal_dev[sel]->ch0_vertphase0.bits.phase = ch0_v_phase0;
         scal_dev[sel]->ch0_vertphase1.bits.phase = ch0_v_phase1;
         scal_dev[sel]->ch1_vertphase0.bits.phase = ch12_v_phase0;
         scal_dev[sel]->ch1_vertphase1.bits.phase = ch12_v_phase1;

     }

	 return 0;
}

//*********************************************************************************************
// function       : DE_SCAL_Agth_Config(u8 sel, __scal_src_type_t *in_type,__scal_src_size_t *in_size,
//                                      __scal_out_size_t *out_size, u8 dien,u8 trden,__scal_3d_outmode_t outmode)
// description    : select sram mapping mothed and agorithm modify
// parameters     :
//                 sel <scaler select>
//                 in_size <scale region define,  src size, offset, scal size>
//                 in_type <src data type>
//                 out_size <scale out size, when output interlace, the height is 2*outheight ,for example 480i, the value is 480>
//                 dien <deinterlace enable>
//                 trden <3D mode enable>
//                 outmode <3D output mode>
// return         :
//                 0 success
//                 -1 failed
//***********************************************************************************************
s32 DE_SCAL_Agth_Config(u8 sel, __scal_src_type_t *in_type,__scal_src_size_t *in_size,__scal_out_size_t *out_size,
                          u8 dien,u8 trden,__scal_3d_outmode_t outmode)
{
	u32 inwidth,outwidth, linebuflen,cibuflen;
	inwidth = in_size->scal_width;
	outwidth = out_size->width;

	if(trden)
	{
		switch(outmode)
		{
			case DE_SCAL_3DOUT_HDMI_FPP:break;
			case DE_SCAL_3DOUT_HDMI_TB: break;
			case DE_SCAL_3DOUT_HDMI_FPI:break;
			case DE_SCAL_3DOUT_HDMI_FA: break;
			default: inwidth *=2; outwidth *=2; break;
		}
	}
	if((inwidth > 8192) || (outwidth > 8192))
		return -1;
	else if((inwidth > 4096) && (outwidth > 4096))
		return -1;
	else if((inwidth>2048) && (outwidth>2048))
	{
		if(dien)
		{
			return -1;
		}
		else if(in_type->fmt == DE_SCAL_INYUV420)
		{
			scal_dev[sel]->bypass.bits.sram_map_sel = 1; //4096 mode
			linebuflen = 4096;
			cibuflen = 2048;
		}
		else if((inwidth>2560)&&(outwidth>2560))
		{
			return -1;
		}
		else
		{
			scal_dev[sel]->bypass.bits.sram_map_sel = 2; //2560 mode
			linebuflen = 2560;
			cibuflen = 0;
		}
	}
	else
	{
		scal_dev[sel]->bypass.bits.sram_map_sel = 0; //normal mode
		linebuflen = 2048;
		cibuflen = dien ? 1280:2048;
	}

	if(trden && (outmode<DE_SCAL_3DOUT_LIRGB))
	{
		if(outwidth>cibuflen)
			return -1;
	}

	//modify agth
	if(linebuflen == 4096)
	{
		scal_dev[sel]->agth_sel.bits.linebuf_agth = 1;
	}
	else if(outwidth>linebuflen)
	{
		scal_dev[sel]->agth_sel.bits.linebuf_agth = 1;
	}
	else if(inwidth > linebuflen)
	{
		scal_dev[sel]->agth_sel.bits.linebuf_agth = 0;
	}

	return 0;
}


//*********************************************************************************************
// function       : DE_SCAL_Set_Scaling_Factor(u8 sel, __scal_scan_mod_t *in_scan, __scal_src_size_t *in_size,
//                                               __scal_src_type_t *in_type, __scal_scan_mod_t *out_scan,
//                                               __scal_out_size_t *out_size, __scal_out_type_t *out_type)
// description    : set scaler scaling factor, modify algorithm and tape offset
// parameters     :
//                 sel <scaler select>
//                 in_scan <scale src data scan mode, if deinterlaceing open, the scan mode is progressive for scale>
//                 in_size <scale region define,  src size, offset, scal size>
//                 in_type <src data type>
//                 out_scan <scale output data scan mode>
//                 out_size <scale out size, when output interlace, the height is 2*outheight ,for example 480i, the value is 480>
//                 out_type <output data format>
// return         :
//               success
// history        :  2011/03/31  modify channel 1/2 scaling factor
//                   2012/06/05  move algorithm to DE_SCAL_Set_Agth funtion
//***********************************************************************************************
s32 DE_SCAL_Set_Scaling_Factor(u8 sel, __scal_scan_mod_t *in_scan, __scal_src_size_t *in_size,
                                 __scal_src_type_t *in_type, __scal_scan_mod_t *out_scan,
                                 __scal_out_size_t *out_size, __scal_out_type_t *out_type)
{
    s32 in_w0, in_h0, out_w0, out_h0;
    s32 ch0_hstep, ch0_vstep ;
	s8 w_shift, h_shift;

    in_w0 = in_size->scal_width;
    in_h0 = in_size->scal_height;

    out_w0 = out_size->width;
    out_h0 = out_size->height + (out_scan->field & 0x1);	//modify by zchmin 110317

	if((in_type->mod == DE_SCAL_INTERLEAVED) && (in_type->fmt == DE_SCAL_INYUV422))
    {
        in_w0 &=0xfffffffe;
    }
    /*//algorithm select
    if(out_w0 > SCALLINEMAX)
    {
	    scal_dev[sel]->agth_sel.bits.linebuf_agth = 0x1;
        if(in_w0>SCALLINEMAX)  //
        {
            in_w0 = SCALLINEMAX;
        }
    }
    else
    {
        scal_dev[sel]->agth_sel.bits.linebuf_agth= 0x0;
    }*/

    w_shift = (in_type->fmt == DE_SCAL_INYUV411) ? 2 : ((in_type->fmt == DE_SCAL_INYUV420)||(in_type->fmt == DE_SCAL_INYUV422)) ? 1 : 0;
	h_shift = ((in_type->fmt == DE_SCAL_INYUV420)) ? 1 : 0;


    if((out_type->fmt == DE_SCAL_OUTPYUV420) || (out_type->fmt == DE_SCAL_OUTPYUV422))
    {
        w_shift -= 1 ;
    }
    else if(out_type->fmt == DE_SCAL_OUTPYUV411)
    {
        w_shift -= 2 ;
    }
    else
    {
        w_shift -= 0 ;;
    }
    if(out_type->fmt == DE_SCAL_OUTPYUV420)
    {
        h_shift -= 1;
    }
    else
    {
        h_shift -= 0;
    }
	//added no-zero limited
    in_h0 = (in_h0!=0) ? in_h0 : 1;
	in_w0 = (in_w0!=0) ? in_w0 : 1;
	out_h0 = (out_h0!=0) ? out_h0 : 1;
	out_w0 = (out_w0!=0) ? out_w0 : 1;

    //step factor
    ch0_hstep = (in_w0<<16)/out_w0;
    ch0_vstep = (out_scan->field)? ((in_h0<<16)/(out_h0)) : (((in_h0>>in_scan->field)<<16)/( out_h0 ));

	scal_dev[sel]->ch0_horzfact.dwval = ch0_hstep;
    scal_dev[sel]->ch0_vertfact.dwval = ch0_vstep<<(out_scan->field);
    scal_dev[sel]->ch1_horzfact.dwval = (w_shift>0) ? (ch0_hstep>>w_shift) : ch0_hstep<<(0-w_shift);
    scal_dev[sel]->ch1_vertfact.dwval = (h_shift>0) ? (ch0_vstep>>h_shift)<<(out_scan->field) : (ch0_vstep<<(0-h_shift))<<(out_scan->field);

	return 0;
}


//*********************************************************************************************
// function       : DE_SCAL_Set_Scaling_Coef(u8 sel, __scal_scan_mod_t *in_scan, __scal_src_size_t *in_size,
//                                             __scal_src_type_t *in_type, __scal_scan_mod_t *out_scan,
//                                             __scal_out_size_t *out_size, __scal_out_type_t *out_type, u8 smth_mode)
// description    : set scaler scaling filter coefficients
// parameters     :
//                 sel <scaler select>
//                 in_scan <scale src data scan mode, if deinterlaceing open, the scan mode is progressive for scale>
//                 in_size <scale region define,  src size, offset, scal size>
//                 in_type <src data type>
//                 out_scan <scale output data scan mode>
//                 out_size <scale out size>
//                 out_type <output data format,alpha_en, alpha_coef type>
//                 smth_mode <scaler filter effect select>
// return         :
//               success
// history        : 2012/06/05 added alpha coef select, modify alpha coef in future according to application
//***********************************************************************************************
s32 DE_SCAL_Set_Scaling_Coef(u8 sel, __scal_scan_mod_t *in_scan, __scal_src_size_t *in_size,
                               __scal_src_type_t *in_type, __scal_scan_mod_t *out_scan,
                               __scal_out_size_t *out_size, __scal_out_type_t *out_type, u8 smth_mode)
{
    s32 in_w0, in_h0, in_w1, in_h1, out_w0, out_h0, out_w1, out_h1;
    s32 ch0h_smth_level, ch0v_smth_level, ch1h_smth_level, ch1v_smth_level;
    u32 int_part, float_part;
    u32 zoom0_size, zoom1_size, zoom2_size, zoom3_size, zoom4_size, zoom5_size, al1_size;
    u32 ch0h_sc, ch0v_sc, ch1h_sc, ch1v_sc;
    u32 ch0v_fir_coef_addr, ch0h_fir_coef_addr, ch1v_fir_coef_addr, ch1h_fir_coef_addr;
    u32 ch0v_fir_coef_ofst, ch0h_fir_coef_ofst, ch1v_fir_coef_ofst, ch1h_fir_coef_ofst;
    s32 fir_ofst_tmp;
    u32 i;
#if defined CONFIG_ARCH_SUN8IW3P1
    u32 loop_count = 0;
#else
    u32 ch3h_fir_coef_addr, ch3v_fir_coef_addr;
#endif
    in_w0 = in_size->scal_width;
    in_h0 = in_size->scal_height;

    out_w0 = out_size->width;
    out_h0 = out_size->height;

    zoom0_size = 1;
    zoom1_size = 8;
    zoom2_size = 4;
    zoom3_size = 1;
    zoom4_size = 1;
    zoom5_size = 1;
    al1_size = zoom0_size + zoom1_size + zoom2_size + zoom3_size + zoom4_size + zoom5_size;

    if((in_type->mod == DE_SCAL_INTERLEAVED) && (in_type->fmt == DE_SCAL_INYUV422))
    {
        in_w0 &=0xfffffffe;
    }

    //channel 1,2 size
    if((in_type->fmt == DE_SCAL_INYUV420) || (in_type->fmt == DE_SCAL_INYUV422))
    {
        in_w1 = (in_w0 + 0x1)>>0x1;
    }
    else if(in_type->fmt == DE_SCAL_INYUV411)
    {
        in_w1 = (in_w0 + 0x3)>>0x2;
    }
    else
    {
        in_w1 = in_w0;
    }
    if((in_type->fmt == DE_SCAL_INYUV420))
    {
        in_h1 = (in_h0 + 0x1)>>0x1;
    }
    else
    {
        in_h1 = in_h0;
    }
    if((out_type->fmt == DE_SCAL_OUTPYUV420) || (out_type->fmt == DE_SCAL_OUTPYUV422))
    {
        out_w1 = (out_w0 + 0x1)>>0x1;
    }
    else if(out_type->fmt == DE_SCAL_OUTPYUV411)
    {
        out_w1 = (out_w0 + 0x3)>>0x2;
    }
    else
    {
        out_w1 = out_w0;
    }
    if(out_type->fmt == DE_SCAL_OUTPYUV420)
    {
        out_h1 = (out_h0+ 0x1)>>0x1;
    }
    else
    {
        out_h1 = out_h0;
    }

    //added no-zero limited
    in_h0 = (in_h0!=0) ? in_h0 : 1;
	in_h1 = (in_h1!=0) ? in_h1 : 1;
	in_w0 = (in_w0!=0) ? in_w0 : 1;
	in_w1 = (in_w1!=0) ? in_w1 : 1;
	out_h0 = (out_h0!=0) ? out_h0 : 1;
	out_h1 = (out_h1!=0) ? out_h1 : 1;
	out_w0 = (out_w0!=0) ? out_w0 : 1;
	out_w1 = (out_w1!=0) ? out_w1 : 1;

    //smooth level for channel 0,1 in vertical and horizontal direction
    ch0h_smth_level = (smth_mode&0x40)  ?  0 - (smth_mode&0x3f) : smth_mode&0x3f;
    ch0v_smth_level = ch0h_smth_level;
    if((smth_mode>>7) &0x01)
    {
      ch0v_smth_level = (smth_mode&0x4000) ? 0 - ((smth_mode&0x3f00)>>8) : ((smth_mode&0x3f00)>>8);
    }
    if(((u32)smth_mode>>31)&0x01)
    {
      ch1h_smth_level = (smth_mode&0x400000) ? 0 - ((smth_mode&0x3f0000)>>16) : ((smth_mode&0x3f0000)>>16);
      ch1v_smth_level = ch1h_smth_level;
      if(((u32)smth_mode >> 23)&0x1)
      {
        ch1v_smth_level = (smth_mode&0x40000000) ? 0 - ((smth_mode&0x3f000000)>>24) : ((smth_mode&0x3f000000)>>24);
      }
    }
    //
    ch0h_sc = (in_w0<<3)/out_w0;
    ch0v_sc = (in_h0<<(3-in_scan->field))/(out_h0);
    ch1h_sc = (in_w1<<3)/out_w1;
    ch1v_sc = (in_h1<<(3-in_scan->field))/(out_h1);

    //modify ch1 smooth level according to ratio to ch0
    if((((u32)smth_mode>>31)&0x01)==0x0)
    {
      if(!ch1h_sc)
      {
        ch1h_smth_level = 0;
      }
      else
      {
        ch1h_smth_level = ch0h_smth_level>>(ch0h_sc/ch1h_sc);
      }

      if(!ch1v_sc)
      {
        ch1v_smth_level = 0;
      }
      else
      {
        ch1v_smth_level = ch0v_smth_level>>(ch0v_sc/ch1v_sc);
      }
    }

      //comput the fir coefficient offset in coefficient table
      int_part = ch0v_sc>>3;
      float_part = ch0v_sc & 0x7;
      ch0v_fir_coef_ofst = (int_part==0)  ? zoom0_size :
                           (int_part==1)  ? zoom0_size + float_part :
                           (int_part==2)  ? zoom0_size + zoom1_size + (float_part>>1) :
                           (int_part==3)  ? zoom0_size + zoom1_size + zoom2_size :
                           (int_part==4)  ? zoom0_size + zoom1_size + zoom2_size +zoom3_size :
                           zoom0_size + zoom1_size + zoom2_size + zoom3_size + zoom4_size;
      int_part = ch0h_sc>>3;
      float_part = ch0h_sc & 0x7;
      ch0h_fir_coef_ofst = (int_part==0)  ? zoom0_size :
                           (int_part==1)  ? zoom0_size + float_part :
                           (int_part==2)  ? zoom0_size + zoom1_size + (float_part>>1) :
                           (int_part==3)  ? zoom0_size + zoom1_size + zoom2_size :
                           (int_part==4)  ? zoom0_size + zoom1_size + zoom2_size +zoom3_size :
                           zoom0_size + zoom1_size + zoom2_size + zoom3_size + zoom4_size;
      int_part = ch1v_sc>>3;
      float_part = ch1v_sc & 0x7;
      ch1v_fir_coef_ofst = (int_part==0)  ? zoom0_size :
                           (int_part==1)  ? zoom0_size + float_part :
                           (int_part==2)  ? zoom0_size + zoom1_size + (float_part>>1) :
                           (int_part==3)  ? zoom0_size + zoom1_size + zoom2_size :
                           (int_part==4)  ? zoom0_size + zoom1_size + zoom2_size +zoom3_size :
                           zoom0_size + zoom1_size + zoom2_size + zoom3_size + zoom4_size;
      int_part = ch1h_sc>>3;
      float_part = ch1h_sc & 0x7;
      ch1h_fir_coef_ofst =  (int_part==0)  ? zoom0_size :
                            (int_part==1)  ? zoom0_size + float_part :
                            (int_part==2)  ? zoom0_size + zoom1_size + (float_part>>1) :
                            (int_part==3)  ? zoom0_size + zoom1_size + zoom2_size :
                            (int_part==4)  ? zoom0_size + zoom1_size + zoom2_size +zoom3_size :
                            zoom0_size + zoom1_size + zoom2_size + zoom3_size + zoom4_size;
    //added smooth level for each channel in horizontal and vertical direction
    fir_ofst_tmp = ch0v_fir_coef_ofst + ch0v_smth_level;
    ch0v_fir_coef_ofst = (fir_ofst_tmp<0) ? 0 : fir_ofst_tmp;
    fir_ofst_tmp = ch0h_fir_coef_ofst + ch0h_smth_level;
    ch0h_fir_coef_ofst = (fir_ofst_tmp<0) ? 0 : fir_ofst_tmp;
    fir_ofst_tmp = ch1v_fir_coef_ofst + ch1v_smth_level;
    ch1v_fir_coef_ofst = (fir_ofst_tmp<0) ? 0 : fir_ofst_tmp;
    fir_ofst_tmp = ch1h_fir_coef_ofst + ch1h_smth_level;
    ch1h_fir_coef_ofst = (fir_ofst_tmp<0) ? 0 : fir_ofst_tmp;
    //modify coefficient offset
    ch0v_fir_coef_ofst = (ch0v_fir_coef_ofst > (al1_size - 1)) ? (al1_size - 1) : ch0v_fir_coef_ofst;
    ch1v_fir_coef_ofst = (ch1v_fir_coef_ofst > (al1_size - 1)) ? (al1_size - 1) : ch1v_fir_coef_ofst;
    ch0h_fir_coef_ofst = (ch0h_fir_coef_ofst > (al1_size - 1)) ? (al1_size - 1) : ch0h_fir_coef_ofst;
    ch1h_fir_coef_ofst = (ch1h_fir_coef_ofst > (al1_size - 1)) ? (al1_size - 1) : ch1h_fir_coef_ofst;

#if defined CONFIG_ARCH_SUN8IW3P1
    /* for  single buffer */
    ch0v_fir_coef_addr = (ch0v_fir_coef_ofst<<7);
    ch0h_fir_coef_addr = (ch0h_fir_coef_ofst<<7);
    ch1v_fir_coef_addr = (ch1v_fir_coef_ofst<<7);
    ch1h_fir_coef_addr = (ch1h_fir_coef_ofst<<7);
    scal_dev[sel]->frm_ctrl.bits.coef_access_ctrl= 1;
    while((scal_dev[sel]->status.bits.coef_access_status == 0) && (loop_count < 20)) {
        loop_count ++;
    }

    for(i=0; i<32; i++)
    {
      scal_dev[sel]->ch0_horzcoef0[i].dwval = fir_tab[(ch0h_fir_coef_addr>>2) + i];
      scal_dev[sel]->ch0_vertcoef[i].dwval  = fir_tab[(ch0v_fir_coef_addr>>2) + i];
      scal_dev[sel]->ch1_horzcoef0[i].dwval = fir_tab[(ch1h_fir_coef_addr>>2) + i];
      scal_dev[sel]->ch1_vertcoef[i].dwval  = fir_tab[(ch1v_fir_coef_addr>>2) + i];
    }

    scal_dev[sel]->frm_ctrl.bits.coef_access_ctrl = 0x0;
#else
    //compute the fir coeficient address for each channel in horizontal and vertical direction
    ch0v_fir_coef_addr = (ch0v_fir_coef_ofst<<7);
    ch0h_fir_coef_addr = ((al1_size)<<7) + (ch0h_fir_coef_ofst<<8);
    ch1v_fir_coef_addr = (ch1v_fir_coef_ofst<<7);
    ch1h_fir_coef_addr = ((al1_size)<<7) + (ch1h_fir_coef_ofst<<8);
    ch3h_fir_coef_addr = out_type->alpha_coef_type==0 ? ch0h_fir_coef_addr : ((al1_size)<<7);
    ch3v_fir_coef_addr = out_type->alpha_coef_type==0 ? ch0v_fir_coef_addr : (0)<<7;
    for(i=0; i<32; i++)
    {
      scal_dev[sel]->ch0_horzcoef0[i].dwval = fir_tab[(ch0h_fir_coef_addr>>2) + 2*i];
      scal_dev[sel]->ch0_horzcoef1[i].dwval = fir_tab[(ch0h_fir_coef_addr>>2) + 2*i + 1];
      scal_dev[sel]->ch0_vertcoef[i].dwval  = fir_tab[(ch0v_fir_coef_addr>>2) + i];
      scal_dev[sel]->ch1_horzcoef0[i].dwval = fir_tab[(ch1h_fir_coef_addr>>2) + 2*i];
      scal_dev[sel]->ch1_horzcoef1[i].dwval = fir_tab[(ch1h_fir_coef_addr>>2) + 2*i + 1];
      scal_dev[sel]->ch1_vertcoef[i].dwval  = fir_tab[(ch1v_fir_coef_addr>>2) + i];
    }
	//zchmin 2012/6/5, alpha ceof can be modify according to application
	if(out_type->alpha_en && (in_type->mod == DE_SCAL_INTERLEAVED))
	{
		for(i=0;i<32;i++)
		{
			scal_dev[sel]->ch3_horzcoef0[i].dwval = fir_tab[(ch3h_fir_coef_addr>>2) + 2*i];
			scal_dev[sel]->ch3_horzcoef1[i].dwval = fir_tab[(ch3h_fir_coef_addr>>2) + 2*i + 1];
	        scal_dev[sel]->ch3_vertcoef[i].dwval  = fir_tab[(ch3v_fir_coef_addr>>2) + i];
		}
	}

    scal_dev[sel]->frm_ctrl.bits.coef_rdy_en = 0x1;
#endif

    return 0;
}

#if 0
//*********************************************************************************************
// function       : DE_SCAL_Set_Scaling_Coef_for_video(u8 sel, __scal_scan_mod_t *in_scan, __scal_src_size_t *in_size,
//                                             __scal_src_type_t *in_type, __scal_scan_mod_t *out_scan,
//                                             __scal_out_size_t *out_size, __scal_out_type_t *out_type, u8 smth_mode)
// description    : set scaler scaling filter coefficients for video playback
// parameters     :
//                 sel <scaler select>
//                 in_scan <scale src data scan mode, if deinterlaceing open, the scan mode is progressive for scale>
//                 in_size <scale region define,  src size, offset, scal size>
//                 in_type <src data type>
//                 out_scan <scale output data scan mode>
//                 out_size <scale out size>
//                 out_type <output data format>
//                 smth_mode <scaler filter effect select>
// return         :
//               success
//***********************************************************************************************
s32 DE_SCAL_Set_Scaling_Coef_for_video(u8 sel, __scal_scan_mod_t *in_scan, __scal_src_size_t *in_size,
                               __scal_src_type_t *in_type, __scal_scan_mod_t *out_scan,
                               __scal_out_size_t *out_size, __scal_out_type_t *out_type, u32 smth_mode)
{
    s32 in_w0, in_h0, in_w1, in_h1, out_w0, out_h0, out_w1, out_h1;
    s32 ch0h_smth_level=0, ch0v_smth_level=0, ch1h_smth_level=0, ch1v_smth_level=0;
    u32 int_part, float_part;
    u32 zoom0_size, zoom1_size, zoom2_size, zoom3_size, zoom4_size, zoom5_size, al1_size;
    u32 ch0h_sc, ch0v_sc, ch1h_sc, ch1v_sc;
    u32 ch0v_fir_coef_addr, ch0h_fir_coef_addr, ch1v_fir_coef_addr, ch1h_fir_coef_addr;
    u32 ch0v_fir_coef_ofst, ch0h_fir_coef_ofst, ch1v_fir_coef_ofst, ch1h_fir_coef_ofst;
    s32 fir_ofst_tmp;
    u32 i;

    in_w0 = in_size->scal_width;
    in_h0 = in_size->scal_height;

    out_w0 = out_size->width;
    out_h0 = out_size->height;

    zoom0_size = 1;
    zoom1_size = 8;
    zoom2_size = 4;
    zoom3_size = 1;
    zoom4_size = 1;
    zoom5_size = 1;
    al1_size = zoom0_size + zoom1_size + zoom2_size + zoom3_size + zoom4_size + zoom5_size;

    if((in_type->mod == DE_SCAL_INTERLEAVED) && (in_type->fmt == DE_SCAL_INYUV422))
    {
        in_w0 &=0xfffffffe;
    }

    //channel 1,2 size
    if((in_type->fmt == DE_SCAL_INYUV420) || (in_type->fmt == DE_SCAL_INYUV422))
    {
        in_w1 = (in_w0 + 0x1)>>0x1;
    }
    else if(in_type->fmt == DE_SCAL_INYUV411)
    {
        in_w1 = (in_w0 + 0x3)>>0x2;
    }
    else
    {
        in_w1 = in_w0;
    }
    if((in_type->fmt == DE_SCAL_INYUV420))
    {
        in_h1 = (in_h0 + 0x1)>>0x1;
    }
    else
    {
        in_h1 = in_h0;
    }
    if((out_type->fmt == DE_SCAL_OUTPYUV420) || (out_type->fmt == DE_SCAL_OUTPYUV422))
    {
        out_w1 = (out_w0 + 0x1)>>0x1;
    }
    else if(out_type->fmt == DE_SCAL_OUTPYUV411)
    {
        out_w1 = (out_w0 + 0x3)>>0x2;
    }
    else
    {
        out_w1 = out_w0;
    }
    if(out_type->fmt == DE_SCAL_OUTPYUV420)
    {
        out_h1 = (out_h0+ 0x1)>>0x1;
    }
    else
    {
        out_h1 = out_h0;
    }

    //added no-zero limited
    in_h0 = (in_h0!=0) ? in_h0 : 1;
	in_h1 = (in_h1!=0) ? in_h1 : 1;
	in_w0 = (in_w0!=0) ? in_w0 : 1;
	in_w1 = (in_w1!=0) ? in_w1 : 1;
	out_h0 = (out_h0!=0) ? out_h0 : 1;
	out_h1 = (out_h1!=0) ? out_h1 : 1;
	out_w0 = (out_w0!=0) ? out_w0 : 1;
	out_w1 = (out_w1!=0) ? out_w1 : 1;

    //smooth level for channel 0,1 in vertical and horizontal direction
    ch0h_smth_level = (smth_mode&0x40)  ?  0 - (smth_mode&0x3f) : smth_mode&0x3f;
    ch0v_smth_level = ch0h_smth_level;
    if((smth_mode>>7) &0x01)
    {
      ch0v_smth_level = (smth_mode&0x4000) ? 0 - ((smth_mode&0x3f00)>>8) : ((smth_mode&0x3f00)>>8);
    }
    if((smth_mode>>31)&0x01)
    {
      ch1h_smth_level = (smth_mode&0x400000) ? 0 - ((smth_mode&0x3f0000)>>16) : ((smth_mode&0x3f0000)>>16);
      ch1v_smth_level = ch1h_smth_level;
      if((smth_mode >> 23)&0x1)
      {
        ch1v_smth_level = (smth_mode&0x40000000) ? 0 - ((smth_mode&0x3f000000)>>24) : ((smth_mode&0x3f000000)>>24);
      }
    }
    //
    ch0h_sc = (in_w0<<3)/out_w0;
    ch0v_sc = (in_h0<<(3-in_scan->field))/(out_h0);
    ch1h_sc = (in_w1<<3)/out_w1;
    ch1v_sc = (in_h1<<(3-in_scan->field))/(out_h1);

    //modify ch1 smooth level according to ratio to ch0
    if(((smth_mode>>31)&0x01)==0x0)
    {
      if(!ch1h_sc)
      {
        ch1h_smth_level = 0;
      }
      else
      {
        ch1h_smth_level = ch0h_smth_level>>(ch0h_sc/ch1h_sc);
      }

      if(!ch1v_sc)
      {
        ch1v_smth_level = 0;
      }
      else
      {
        ch1v_smth_level = ch0v_smth_level>>(ch0v_sc/ch1v_sc);
      }
    }

      //comput the fir coefficient offset in coefficient table
      int_part = ch0v_sc>>3;
      float_part = ch0v_sc & 0x7;
      ch0v_fir_coef_ofst = (int_part==0)  ? zoom0_size :
                           (int_part==1)  ? zoom0_size + float_part :
                           (int_part==2)  ? zoom0_size + zoom1_size + (float_part>>1) :
                           (int_part==3)  ? zoom0_size + zoom1_size + zoom2_size :
                           (int_part==4)  ? zoom0_size + zoom1_size + zoom2_size +zoom3_size :
                           zoom0_size + zoom1_size + zoom2_size + zoom3_size + zoom4_size;
      int_part = ch0h_sc>>3;
      float_part = ch0h_sc & 0x7;
      ch0h_fir_coef_ofst = (int_part==0)  ? zoom0_size :
                           (int_part==1)  ? zoom0_size + float_part :
                           (int_part==2)  ? zoom0_size + zoom1_size + (float_part>>1) :
                           (int_part==3)  ? zoom0_size + zoom1_size + zoom2_size :
                           (int_part==4)  ? zoom0_size + zoom1_size + zoom2_size +zoom3_size :
                           zoom0_size + zoom1_size + zoom2_size + zoom3_size + zoom4_size;
      int_part = ch1v_sc>>3;
      float_part = ch1v_sc & 0x7;
      ch1v_fir_coef_ofst = (int_part==0)  ? zoom0_size :
                           (int_part==1)  ? zoom0_size + float_part :
                           (int_part==2)  ? zoom0_size + zoom1_size + (float_part>>1) :
                           (int_part==3)  ? zoom0_size + zoom1_size + zoom2_size :
                           (int_part==4)  ? zoom0_size + zoom1_size + zoom2_size +zoom3_size :
                           zoom0_size + zoom1_size + zoom2_size + zoom3_size + zoom4_size;
      int_part = ch1h_sc>>3;
      float_part = ch1h_sc & 0x7;
      ch1h_fir_coef_ofst =  (int_part==0)  ? zoom0_size :
                            (int_part==1)  ? zoom0_size + float_part :
                            (int_part==2)  ? zoom0_size + zoom1_size + (float_part>>1) :
                            (int_part==3)  ? zoom0_size + zoom1_size + zoom2_size :
                            (int_part==4)  ? zoom0_size + zoom1_size + zoom2_size +zoom3_size :
                            zoom0_size + zoom1_size + zoom2_size + zoom3_size + zoom4_size;
    //added smooth level for each channel in horizontal and vertical direction
    fir_ofst_tmp = ch0v_fir_coef_ofst + ch0v_smth_level;
    ch0v_fir_coef_ofst = (fir_ofst_tmp<0) ? 0 : fir_ofst_tmp;
    fir_ofst_tmp = ch0h_fir_coef_ofst + ch0h_smth_level;
    ch0h_fir_coef_ofst = (fir_ofst_tmp<0) ? 0 : fir_ofst_tmp;
    fir_ofst_tmp = ch1v_fir_coef_ofst + ch1v_smth_level;
    ch1v_fir_coef_ofst = (fir_ofst_tmp<0) ? 0 : fir_ofst_tmp;
    fir_ofst_tmp = ch1h_fir_coef_ofst + ch1h_smth_level;
    ch1h_fir_coef_ofst = (fir_ofst_tmp<0) ? 0 : fir_ofst_tmp;
    //modify coefficient offset
    ch0v_fir_coef_ofst = (ch0v_fir_coef_ofst > (al1_size - 1)) ? (al1_size - 1) : ch0v_fir_coef_ofst;
    ch1v_fir_coef_ofst = (ch1v_fir_coef_ofst > (al1_size - 1)) ? (al1_size - 1) : ch1v_fir_coef_ofst;
    ch0h_fir_coef_ofst = (ch0h_fir_coef_ofst > (al1_size - 1)) ? (al1_size - 1) : ch0h_fir_coef_ofst;
    ch1h_fir_coef_ofst = (ch1h_fir_coef_ofst > (al1_size - 1)) ? (al1_size - 1) : ch1h_fir_coef_ofst;

    //compute the fir coeficient address for each channel in horizontal and vertical direction
    ch0v_fir_coef_addr = (ch0v_fir_coef_ofst<<7);
    ch0h_fir_coef_addr = ((al1_size)<<7) + (ch0h_fir_coef_ofst<<8);
    ch1v_fir_coef_addr = (ch1v_fir_coef_ofst<<7);
    ch1h_fir_coef_addr = ((al1_size)<<7) + (ch1h_fir_coef_ofst<<8);

    for(i=0; i<32; i++)
    {
	    scal_dev[sel]->ch0_horzcoef0[i].dwval = fir_tab_video[(ch0h_fir_coef_addr>>2) + 2*i];
		scal_dev[sel]->ch0_horzcoef1[i].dwval = fir_tab_video[(ch0h_fir_coef_addr>>2) + 2*i + 1];
        scal_dev[sel]->ch0_vertcoef[i].dwval  = fir_tab_video[(ch0v_fir_coef_addr>>2) + i];
		scal_dev[sel]->ch1_horzcoef0[i].dwval = fir_tab_video[(ch1h_fir_coef_addr>>2) + 2*i];
		scal_dev[sel]->ch1_horzcoef1[i].dwval = fir_tab_video[(ch1h_fir_coef_addr>>2) + 2*i + 1];
        scal_dev[sel]->ch1_vertcoef[i].dwval  = fir_tab_video[(ch1v_fir_coef_addr>>2) + i];
    }

    scal_dev[sel]->frm_ctrl.bits.coef_rdy_en = 0x1;

    return 0;
}
#endif

//*********************************************************************************************
// function       : DE_SCAL_Set_CSC_Coef(u8 sel, u8 in_csc_mode, u8 out_csc_mode, u8 incs, u8 outcs, u32  in_br_swap, u32 out_br_swap)
// description    : set scaler input/output color space convert coefficients
// parameters     :
//                 sel <scaler select>
//                 in_csc_mode <color space select, bt601, bt709, ycc, xycc>
//                 out_csc_mode <color space select, bt601, bt709, ycc, xycc>
//                 incs <source color space>
//                 |    0  rgb
//                 |    1  yuv
//                 outcs <destination color space>
//                 |    0  rgb
//                 |    1  yuv
//                 in_br_swap <swap b r component>
//                 |    0  normal
//                 |    1  swap enable, note: when input yuv, then u v swap
//                 out_br_swap <swap output b r component>
//                 |    0  normal
//                 |    1  swap enable, note: when output yuv, then u v swap
// return         :
//               success
//***********************************************************************************************
s32 DE_SCAL_Set_CSC_Coef(u8 sel, u8 in_csc_mode, u8 out_csc_mode, u8 incs, u8 outcs, u32  in_br_swap, u32 out_br_swap)
{
    u8  csc_pass;
    u32 csc_coef_addr;
    u32 i;


    //compute csc bypass enable
    if(incs == 0x0)  //rgb
    {
        if(outcs == 0x0) //rgb
        {
            csc_pass = 0x01;
            csc_coef_addr = (((in_csc_mode&0x3)<<7) + ((in_csc_mode&0x3)<<6)) + 0x60;
        }
        else
        {
        	csc_pass = 0x0;
            csc_coef_addr = (((in_csc_mode&0x3)<<7) + ((in_csc_mode&0x3)<<6)) + 0x60 + 0x30;
        }
    }
    else
    {
    	if(outcs == 0x0)
        {
            csc_pass = 0x00;
            csc_coef_addr = (((in_csc_mode&0x3)<<7) + ((in_csc_mode&0x3)<<6));
        }
        else
        {
            csc_pass = 0x01;
            csc_coef_addr = (((in_csc_mode&0x3)<<7) + ((in_csc_mode&0x3)<<6)) + 0x30;
        }
    }

    if(in_br_swap || out_br_swap)
   	{
   		csc_pass = 0;
   	}
   	if(!csc_pass)
    {
        for(i=0; i<4; i++)
        {
            scal_dev[sel]->csc_coef[i].dwval = csc_tab[(csc_coef_addr>>2) + i];
			scal_dev[sel]->csc_coef[i+4 + out_br_swap * 4].dwval = csc_tab[(csc_coef_addr>>2) + i + 4 + in_br_swap * 4];
			scal_dev[sel]->csc_coef[i+8 - out_br_swap * 4].dwval = csc_tab[(csc_coef_addr>>2) + i + 8 - in_br_swap * 4];

        }
    }
    scal_dev[sel]->bypass.bits.csc_bypass_en = csc_pass;


    return 0;
}


//*********************************************************************************************
// function         : DE_SCAL_Set_Out_Format(u8 sel, __scal_out_type_t *out_type)
// description      : set scaler set output format
// parameters       :
//                 sel <scaler select>
//                 out_type <output data format>
// return           :
//                 success
// history          : 2012/06/05, added ouput alpha enable,
// note             : alpha can only be enable when input fmt interleaved and ouput fmt argb
//***********************************************************************************************
s32 DE_SCAL_Set_Out_Format(u8 sel, __scal_out_type_t *out_type)
{
	scal_dev[sel]->output_fmt.bits.byte_seq = out_type->byte_seq;
	scal_dev[sel]->output_fmt.bits.data_fmt = out_type->fmt;
	if(out_type->fmt == DE_SCAL_OUTI0RGB888 || out_type->fmt == DE_SCAL_OUTI1RGB888)
	{
		scal_dev[sel]->output_fmt.bits.alpha_en = out_type->alpha_en;
	}
	else
	{
		scal_dev[sel]->output_fmt.bits.alpha_en = 0x0;
	}
	//scal_dev[sel]->output_fmt.bits.alpha_en = out_type->alpha_en;

    return 0;
}


//*********************************************************************************************
// function         : DE_SCAL_Set_Out_Size(u8 sel, __scal_scan_mod_t *out_scan, __scal_out_type_t *out_type,
//                                         __scal_out_size_t *out_size)
// description      : set scaler set output size
// parameters       :
//                 sel <scaler select>
//                 out_scan <output data scan mode>
//                 out_type <output data format>
//                 out_size <scale out size>
// return           :
//                 success
//***********************************************************************************************
s32 DE_SCAL_Set_Out_Size(u8 sel, __scal_scan_mod_t *out_scan, __scal_out_type_t *out_type,
                           __scal_out_size_t *out_size)
{
    u32 out_w1, out_h1, out_w0, out_h0;
	//sc0
    if((out_type->fmt == DE_SCAL_OUTPYUV420) || (out_type->fmt == DE_SCAL_OUTPYUV422))
    {
        out_w1 = (out_size->width+ 0x1) >> 1;
    }
    else if(out_type->fmt == DE_SCAL_OUTPYUV411)
    {
        out_w1 = (out_size->width+ 0x3) >> 2;
    }
    else
    {
        out_w1 = out_size->width;
    }

    if(out_type->fmt == DE_SCAL_OUTPYUV420)
    {
        out_h1 = (out_size->height + 0x1) >> 1;
    }
    else
    {
        out_h1 = out_size->height;
    }
	out_h0 = out_size->height;
	out_w0 = out_size->width;
	//added no-zero limited
    out_h0 = (out_h0!=0) ? out_h0 : 1;
	out_h1 = (out_h1!=0) ? out_h1 : 1;
	out_w0 = (out_w0!=0) ? out_w0 : 1;
	out_w1 = (out_w1!=0) ? out_w1 : 1;

	scal_dev[sel]->ch0_outsize.bits.out_height = ((out_h0 + (out_scan->field & 0x1))>>out_scan->field) - 1;
    scal_dev[sel]->ch0_outsize.bits.out_width = out_w0 - 1;
    scal_dev[sel]->ch1_outsize.bits.out_height = ((out_h1 + (out_scan->field & 0x1)) >>out_scan->field) - 1;
    scal_dev[sel]->ch1_outsize.bits.out_width = out_w1 - 1;
    return 0;
}


//*********************************************************************************************
// function         : DE_SCAL_Set_Trig_Line(u8 sel, u32 line)
// description      : set scaler output trigger line
// parameters       :
//                 sel <scaler select>, //un support
//                 line <line number, only valid for scaler output to display>
// return           :
//               success
//***********************************************************************************************
s32 DE_SCAL_Set_Trig_Line(u8 sel, u32 line)
{
	scal_dev[sel]->lint_ctrl.bits.field_sel = 0x0;
    scal_dev[sel]->lint_ctrl.bits.trig_line = line;
    return 0;
}

//*********************************************************************************************
// function         : DE_SCAL_Set_Int_En(u8 sel, u32 int_num)
// description      : set scaler interrupt enable bit
// parameters       :
//                 sel <scaler select>, //un support
//                 int_num <7, 9, 10>
//                 |    7   write back interrupt
//                 |    9   line interrupt
//                 |    10  register ready load interrupt
// return           :
//               success
//***********************************************************************************************
s32 DE_SCAL_Set_Int_En(u8 sel, u32 int_num)
{
    if(int_num == 7)
    {
        scal_dev[sel]->int_en.bits.wb_en = 0x1;
    }
    else if(int_num == 9)
    {
        scal_dev[sel]->int_en.bits.line_en = 0x1;
    }
    else if(int_num == 10)
    {
        scal_dev[sel]->int_en.bits.reg_load_en = 0x1;
    }

    return 0;
}

//*********************************************************************************************
// function       : DE_SCAL_Set_Di_Ctrl(u8 sel, u8 en, u8 mode, u8 diagintp_en, u8 tempdiff_en)
// description    : set scaler deinterlace control parameter
// parameters     :
//                 sel <scaler select>,
//                 en <0,1>
//                 |    0  deinterlace disable
//                 |    1  deinterlace enable
//                 mode <0,1,2,3>
//                 |    0   weave
//                 |    1   bob
//                 |    2   reserved
//                 |    3   pixel_motion_adptive
//                 diagintp_en <0, 1>
//                 tempdiff_en <0,1>
// return         :
//                 success
// note           : flag out is enabled in default, so current flag address must be set before
//***********************************************************************************************
s32 DE_SCAL_Set_Di_Ctrl(u8 sel, u8 en, u8 mode, u8 diagintp_en, u8 tempdiff_en)
{
	scal_dev[sel]->di_ctrl.bits.en = en;
	scal_dev[sel]->di_ctrl.bits.flag_out_en = 0x0;  //for flag out, default enable
    scal_dev[sel]->di_ctrl.bits.mod = mode;
    scal_dev[sel]->di_ctrl.bits.diagintp_en = diagintp_en;
    scal_dev[sel]->di_ctrl.bits.tempdiff_en = tempdiff_en;
    return 0;
}


//*********************************************************************************************
// function       : DE_SCAL_Set_Di_PreFrame_Addr(u8 sel, u32 luma_addr, u32 chroma_addr)
// description    : set scaler deinterlace pre frame luma/chroma address
// parameters     :
//                 sel <scaler select>,
//                 luma_addr/chroma_addr <pre frame luma/chroma address>
// return         :
//                 success
//***********************************************************************************************
s32 DE_SCAL_Set_Di_PreFrame_Addr(u8 sel, u32 luma_addr, u32 chroma_addr)
{
    scal_dev[sel]->di_preluma.dwval = luma_addr + de_scal_ch0_offset;
	scal_dev[sel]->di_prechroma.dwval = chroma_addr + de_scal_ch1_offset;

	return 0;
}

//*********************************************************************************************
// function       : DE_SCAL_Set_Di_MafFlag_Src(u8 sel, u32 cur_addr, u32 pre_addr, u32 stride)
// description    : set scaler deinterlace maf flag address and linestride
// parameters     :
//                 sel <scaler select>,
//                 cur_addr/pre_addr <maf flag address>
//		           stride <maf line stride>
// return         :
//                 success
//***********************************************************************************************
s32 DE_SCAL_Set_Di_MafFlag_Src(u8 sel, u32 cur_addr, u32 pre_addr, u32 stride)
{
    scal_dev[sel]->di_tileflag0.dwval = pre_addr;
	scal_dev[sel]->di_tileflag1.dwval = cur_addr;
    scal_dev[sel]->di_flaglinestrd.dwval = stride;
    return 0;
}


//**********************************************************************************
// function       : DE_SCAL_Start(u8 sel)
// description    : scaler module  start set
// parameters     :
//                 sel <scaler select>
// return         :
//                  success
//***********************************************************************************
s32 DE_SCAL_Start(u8 sel)
{
	scal_dev[sel]->frm_ctrl.bits.frm_start = 0x1;

    return 0;
}

//**********************************************************************************
// function       : DE_SCAL_Set_Filtercoef_Ready(u8 sel)
// description    : scaler filter coefficient set ready
// parameters     :
//                 sel <scaler select>
// return         :
//                 success
//***********************************************************************************
s32 DE_SCAL_Set_Filtercoef_Ready(u8 sel)
{
    scal_dev[sel]->frm_ctrl.bits.coef_rdy_en = 0x1;

    return 0;
}

//**********************************************************************************
// function       : DE_SCAL_Set_Reg_Rdy(u8 sel)
// description    : scaler configure registers set ready
// parameters     :
//                 sel <scaler select>
// return         :
//                 success
//***********************************************************************************
s32 DE_SCAL_Set_Reg_Rdy(u8 sel)
{
    scal_dev[sel]->frm_ctrl.bits.reg_rdy_en = 0x1;

    return 0;
}


//**********************************************************************************
// function       : DE_SCAL_Reset(u8 sel)
// description    : scaler module reset(reset module status machine)
// parameters     :
//                 sel <scaler select>
// return         :
//                 success
//***********************************************************************************
s32 DE_SCAL_Reset(u8 sel)
{
    scal_dev[sel]->frm_ctrl.bits.frm_start = 0x0;

    //clear wb err
    scal_dev[sel]->status.bits.wb_err_status = 0x0;
    scal_dev[sel]->status.bits.wb_err_losedata = 0x0;
    scal_dev[sel]->status.bits.wb_err_sync = 0x0;

    return 0;
}

//**********************************************************************************
// function       : DE_SCAL_Input_Port_Select(u8 sel, u8 port)
// description    : scaler input source select
// parameters     :
//                 sel <scaler select>
//                 port <scaler input port>
//                 |    0   dram
//                 |    4   interface of image0 to lcd
//                 |    5   interface of image1 to lcd
//                 |    6   image0
//                 |    7   image1
// return         :
//                 success
//***********************************************************************************
s32 DE_SCAL_Input_Select(u8 sel, u32 source)
{
    scal_dev[sel]->frm_ctrl.bits.in_ctrl = source;
    return 0;
}


//**********************************************************************************
// function       : DE_SCAL_Output_Select(u8 sel)
// description    : scaler output select
// parameters     :
//                 sel <scaler select>
//                 out<0:be0; 1:be1; 3:writeback>
// return         :
//                 success
//***********************************************************************************
s32 DE_SCAL_Output_Select(u8 sel, u8 out)
{
    if(out == 3)//write back
    {
        scal_dev[sel]->frm_ctrl.bits.out_ctrl = 1;//disable scaler output to be
        scal_dev[sel]->frm_ctrl.bits.out_port_sel = 0;
    }
    else if(out < 3)
    {
        scal_dev[sel]->frm_ctrl.bits.out_ctrl = 0;//enable scaler output to be
        if((sel == 0) || (sel ==1))
            scal_dev[sel]->frm_ctrl.bits.out_port_sel = 0x0;  //only to be0
        else
            scal_dev[sel]->frm_ctrl.bits.out_port_sel = out;
    }

    return 0;
}

//**********************************************************************************
// function       : DE_SCAL_Writeback_Enable(u8 sel)
// description    : scaler write back enable
// parameters     :
//                 sel <scaler select>
// return         :
//                success
//***********************************************************************************
s32 DE_SCAL_Writeback_Enable(u8 sel)
{
    scal_dev[sel]->frm_ctrl.bits.wb_en = 0x1;

    return 0;
}

//**********************************************************************************
// function       : DE_SCAL_Writeback_Disable(u8 sel)
// description    : scaler write back enable
// parameters     :
//                sel <0,1>
//                |    0  scaler0
//                |    1  scaler1
// return         :
//                success
//***********************************************************************************
s32 DE_SCAL_Writeback_Disable(u8 sel)
{
	scal_dev[sel]->frm_ctrl.bits.wb_en = 0x0;

	return 0;
}

//**********************************************************************************
// function       : DE_SCAL_Writeback_Linestride_Enable(u8 sel)
// description    : scaler write back linestride enable
// parameters     :
//                sel <0,1>
//                |    0  scaler0
//                |    1  scaler1
// return         :
//                success
//***********************************************************************************
s32 DE_SCAL_Writeback_Linestride_Enable(u8 sel)
{
	scal_dev[sel]->wb_linestrd_en.dwval = 0x1;

	return 0;
}

//**********************************************************************************
// function       : DE_SCAL_Writeback_Linestride_Disable(u8 sel)
// description    : scaler write back linestride disable
// parameters     :
//                sel <0,1>
//                |    0  scaler0
//                |    1  scaler1
// return         :
//                success
//***********************************************************************************
s32 DE_SCAL_Writeback_Linestride_Disable(u8 sel)
{
	scal_dev[sel]->wb_linestrd_en.dwval = 0x0;

	return 0;
}


//**********************************************************************************
// function       : DE_SCAL_Enable(u8 sel)
// description    : scaler module enable
// parameters     :
//                 sel <scaler select>
// return         :
//                success
//***********************************************************************************
s32 DE_SCAL_Enable(u8 sel)
{
	de_scal_trd_fp_en = 0;
	de_scal_trd_itl_en = 0;
    scal_dev[sel]->modl_en.bits.en = 0x1;
    //scal_dev[sel]->field_ctrl.sync_edge= 0x1;

    return 0;
}

//**********************************************************************************
// function       : DE_SCAL_Disable(u8 sel)
// description    : scaler module disable
// parameters     :
//                 sel <scaler select>
// return         :
//                success
//***********************************************************************************
s32 DE_SCAL_Disable(u8 sel)
{
    scal_dev[sel]->modl_en.bits.en = 0x0;

    return 0;
}


//**********************************************************************************
// function       : DE_SCAL_Set_Writeback_Addr(u8 sel, __scal_buf_addr_t *addr)
// description    : scaler write back address set
// parameters     :
//                 sel <scaler select>
//                 addr <address for wb>
// return         :
//                success
//***********************************************************************************
s32 DE_SCAL_Set_Writeback_Addr(u8 sel, __scal_buf_addr_t *addr)
{
    scal_dev[sel]->wb_addr0.dwval = addr->ch0_addr;
    scal_dev[sel]->wb_addr1.dwval = addr->ch1_addr;
    scal_dev[sel]->wb_addr2.dwval = addr->ch2_addr;


    return 0;
}


//*********************************************************************************
// function       : DE_SCAL_Get_Input_Format(u8 sel)
// description    : scaler input format get
// parameters     :
//                sel <scaler select>
// return         :
//                format<0,1,2,3,4,5>
//                |    0  yuv444
//                |    1  yuv422
//                |    2  yuv420
//                |    3  yuv411
//                |    4  rgb565
//                |    5  argb8888
//                |    6  argb4444
//                |    7  argb1555
//*********************************************************************************
u8 DE_SCAL_Get_Input_Format(u8 sel)
{
    u8 fmt_ret;
    fmt_ret = scal_dev[sel]->input_fmt.bits.data_fmt;

    return fmt_ret;
}

//*********************************************************************************
// function       : DE_SCAL_Get_Input_Mode(u8 sel)
// description    : scaler input mode get
// parameters     :
//                sel <scaler select>
// return         :
//                 mode<0,1,2,4,6>
//                 |    0  non-macro block plannar data
//                 |    1  interleaved data
//                 |    2  non-macro block uv combined data
//                 |    4  macro block plannar data
//                 |    6  macro block uv combined data
//*********************************************************************************
u8 DE_SCAL_Get_Input_Mode(u8 sel)
{
    u8 mode_ret;
    mode_ret = scal_dev[sel]->input_fmt.bits.data_mod;

    return mode_ret;
}



//**********************************************************************************
// function       : DE_SCAL_Get_Output_Format(u8 sel)
// description    : display engine front-end output data format get
// parameters     :
//                sel <scaler select>
//return          :
//                format  <0, 1, 2, 4, 5, 6, 7>
//                |    0  plannar rgb output
//                |    1  interleaved argb ouptut only for writeback
//                |    2  interleaved bgra output only for writeback
//                |    4  plannar yuv444
//                |    5  plannar yuv420
//                |    6  plannar yuv422
//                |    7  plannar yuv411
//***********************************************************************************
u8 DE_SCAL_Get_Output_Format(u8 sel)
{
    u8 fmt_ret;
    fmt_ret = scal_dev[sel]->output_fmt.bits.data_fmt;

    return fmt_ret;
}


//*********************************************************************************
// function       : DE_SCAL_Get_Input_Width(u8 sel)
// description    : scaler input width get
// parameters     :
//                sel <scaler select>
//return          :
//                width  <8~8192>
//*********************************************************************************
u16 DE_SCAL_Get_Input_Width(u8 sel)
{
    u16 in_w;
    in_w = scal_dev[sel]->ch0_insize.bits.in_width + 0x1;

    return in_w;
}

//*********************************************************************************
// function       : DE_SCAL_Get_Input_Height(u8 sel)
// description    : scaler input height get
// parameters     :
//                sel <scaler select>
//return          :
//                 height  <8~8192>
//*********************************************************************************
u16 DE_SCAL_Get_Input_Height(u8 sel)
{
    u16 in_h;
    in_h = scal_dev[sel]->ch0_insize.bits.in_height + 0x1;

    return in_h;
}

//*********************************************************************************
// function       : DE_SCAL_Get_Output_Width(u8 sel)
// description    : scaler output width get
// parameters     :
//                sel <scaler select>
//return          :
//                width  <8~8192>
//*********************************************************************************
u16 DE_SCAL_Get_Output_Width(u8 sel)
{
  u16 out_w;
  out_w = scal_dev[sel]->ch0_outsize.bits.out_width + 0x1;

  return out_w;
}

//*********************************************************************************
// function       : DE_SCAL_Get_Output_Height(u8 sel)
// description    : scaler output height get
// parameters     :
//                sel <scaler select>
//return          :
//                height  <8~8192>
//*********************************************************************************
u16 DE_SCAL_Get_Output_Height(u8 sel)
{
    u16 out_h;
    out_h = scal_dev[sel]->ch0_outsize.bits.out_height + 0x1;

    return out_h;
}

//**********************************************************************************
// function       : DE_SCAL_Get_Start_Status(u8 sel)
// description    : scaler start status get
// parameters     :
//                sel <0,1>
//                |    0  scaler0
//                |    1  scaler1
// return         :
//                 0  scaler enable
//                 -1 scaler disable
//***********************************************************************************
s32 DE_SCAL_Get_Start_Status(u8 sel)
{
     if(scal_dev[sel]->modl_en.bits.en  && scal_dev[sel]->frm_ctrl.bits.frm_start)
     {
         return 0;
     }
     else
     {
         return -1;
     }
}

//**********************************************************************************
// function       : DE_SCAL_Get_Field_Status(u8 sel)
// description    : lcd field status
// parameters     :
//                sel <0,1>
//                |    0  scaler0
//                |    1  scaler1
// return         :
//                 0  top field
//                 1  bottom field
//***********************************************************************************
s32 DE_SCAL_Get_Field_Status(u8 sel)
{
	return scal_dev[sel]->status.bits.lcd_field;
}


//*********************************************************************************************
// function       : iDE_SCAL_Matrix_Mul(__scal_matrix4x4 in1, __scal_matrix4x4 in2, __scal_matrix4x4 *result)
// description    : matrix multiple of 4x4, m1 * m2
// parameters     :
//                 in1/in2 <4x4 matrix>
//                 result <>
// return         :
//                 success
//***********************************************************************************************
s32 iDE_SCAL_Matrix_Mul(__scal_matrix4x4 in1, __scal_matrix4x4 in2, __scal_matrix4x4 *result)
{
	__scal_matrix4x4 tmp;

	tmp.x00 = (in1.x00 * in2.x00 + in1.x01 * in2.x10 + in1.x02 * in2.x20 + in1.x03 * in2.x30) >> 10;
	tmp.x01 = (in1.x00 * in2.x01 + in1.x01 * in2.x11 + in1.x02 * in2.x21 + in1.x03 * in2.x31) >> 10;
	tmp.x02 = (in1.x00 * in2.x02 + in1.x01 * in2.x12 + in1.x02 * in2.x22 + in1.x03 * in2.x32) >> 10;
	tmp.x03 = (in1.x00 * in2.x03 + in1.x01 * in2.x13 + in1.x02 * in2.x23 + in1.x03 * in2.x33) >> 10;
	tmp.x10 = (in1.x10 * in2.x00 + in1.x11 * in2.x10 + in1.x12 * in2.x20 + in1.x13 * in2.x30) >> 10;
	tmp.x11 = (in1.x10 * in2.x01 + in1.x11 * in2.x11 + in1.x12 * in2.x21 + in1.x13 * in2.x31) >> 10;
	tmp.x12 = (in1.x10 * in2.x02 + in1.x11 * in2.x12 + in1.x12 * in2.x22 + in1.x13 * in2.x32) >> 10;
	tmp.x13 = (in1.x10 * in2.x03 + in1.x11 * in2.x13 + in1.x12 * in2.x23 + in1.x13 * in2.x33) >> 10;
	tmp.x20 = (in1.x20 * in2.x00 + in1.x21 * in2.x10 + in1.x22 * in2.x20 + in1.x23 * in2.x30) >> 10;
	tmp.x21 = (in1.x20 * in2.x01 + in1.x21 * in2.x11 + in1.x22 * in2.x21 + in1.x23 * in2.x31) >> 10;
	tmp.x22 = (in1.x20 * in2.x02 + in1.x21 * in2.x12 + in1.x22 * in2.x22 + in1.x23 * in2.x32) >> 10;
	tmp.x23 = (in1.x20 * in2.x03 + in1.x21 * in2.x13 + in1.x22 * in2.x23 + in1.x23 * in2.x33) >> 10;
	tmp.x30 = (in1.x30 * in2.x00 + in1.x31 * in2.x10 + in1.x32 * in2.x20 + in1.x33 * in2.x30) >> 10;
	tmp.x31 = (in1.x30 * in2.x01 + in1.x31 * in2.x11 + in1.x32 * in2.x21 + in1.x33 * in2.x31) >> 10;
	tmp.x32 = (in1.x30 * in2.x02 + in1.x31 * in2.x12 + in1.x32 * in2.x22 + in1.x33 * in2.x32) >> 10;
	tmp.x33 = (in1.x30 * in2.x03 + in1.x31 * in2.x13 + in1.x32 * in2.x23 + in1.x33 * in2.x33) >> 10;


	*result = tmp;

	return 0;
}


//*********************************************************************************************
// function       : iDE_SCAL_Csc_Lmt(__s64 *value, s32 min, s32 max, s32 shift, s32 validbit)
// description    : csc coefficient and constant limited
// parameters     :
//                value<coefficient or constant>
//                min/max <limited range>
// return         :
//                success
//***********************************************************************************************
s32 iDE_SCAL_Csc_Lmt(__s64 *value, s32 min, s32 max, s32 shift, s32 validbit)
{
    __s64 tmp;
    tmp = (*value)>>shift;
   if(tmp < min)
    *value = min & validbit;
   else if(tmp > max)
     *value = max & validbit;
   else
     *value = tmp & validbit;

   return 0;
}

//*********************************************************************************************
// function       : DE_SCAL_Set_CSC_Coef_Enhance(u8 sel, u8 in_csc_mode, u8 out_csc_mode, u8 incs, u8 outcs,
//                                                   s32  bright, s32 contrast, s32 saturaion, s32 hue,
//                                                   u32  in_br_swap, u32 out_br_swap)
// description    : set scaler input/output color space convert coefficients
// parameters     :
//                 sel <scaler select>
//                 in_csc_mode <color space select, bt601, bt709, ycc, xycc>
//                 out_csc_mode <color space select, bt601, bt709, ycc, xycc>
//                 incs <source color space>
//                 |    0  rgb
//                 |    1  yuv
//                 outcs <destination color space>
//                 |    0  rgb
//                 |    1  yuv
//                 brightness<0  ~ 63>  default 32
//                 contrast <0 ~ 63> (0.0 ~ 2.0)*32, default 32
//                 saturation<0~ 63> (0.0 ~ 2.0)*32, default 32
//                 hue <0 ~ 63>  default 32
//                 in_br_swap <swap b r component>
//                 |    0  normal
//                 |    1  swap enable, note: when input yuv, then u v swap
//                 out_br_swap <swap output b r component>
//                 |    0  normal
//                 |    1  swap enable, note: when output yuv, then u v swap
// return         :
//                 success
//***********************************************************************************************
s32 DE_SCAL_Set_CSC_Coef_Enhance(u8 sel, u8 in_csc_mode, u8 out_csc_mode, u8 incs, u8 outcs,
                                                   s32  bright, s32 contrast, s32 saturaion, s32 hue,
                                                   u32  in_br_swap, u32 out_br_swap)
{
	__scal_matrix4x4 matrixEn;
	__scal_matrix4x4 matrixconv, *ptmatrix;
	__scal_matrix4x4 matrixresult, tmpcoeff;
    __s64 *pt;
	u32 i;
	s32 sinv, cosv;   //sin_tab: 7 bit fractional

	bright = bright*64/100;
	saturaion = saturaion*64/100;
	contrast = contrast*64/100;
	hue = hue*64/100;

	sinv = image_enhance_tab[8*16 + (hue&0x3f)];
	cosv = image_enhance_tab[8*16 + 8*8 + (hue&0x3f)];

	matrixEn.x00 = contrast << 5;
	matrixEn.x01 = 0;
	matrixEn.x02 = 0;
	matrixEn.x03 = (((bright - 32) + 16) <<10) - ( contrast << 9);
	matrixEn.x10 = 0;
	matrixEn.x11 = (contrast * saturaion * cosv) >> 7;
	matrixEn.x12 = (contrast * saturaion * sinv) >> 7;
	matrixEn.x13 = (1<<17) - ((matrixEn.x11 + matrixEn.x12)<<7);
	matrixEn.x20 = 0;
	matrixEn.x21 = (-contrast * saturaion * sinv)>>7;
	matrixEn.x22 = (contrast * saturaion * cosv) >> 7;
	matrixEn.x23 = (1<<17) - ((matrixEn.x22 + matrixEn.x21)<<7);
	matrixEn.x30 = 0;
	matrixEn.x31 = 0;
	matrixEn.x32 = 0;
	matrixEn.x33 = 1024;

	if((incs == 0) && (outcs == 0))  //rgb to rgb
	{

		for(i=0; i<16; i++)
		{
			*((__s64 *)(&tmpcoeff.x00) + i) = ((__s64)*(image_enhance_tab + (in_csc_mode<<5) + i) <<32)>>32;		//RGB2YUV

		}

		ptmatrix = &tmpcoeff;

		//convolution of enhance matrix and rgb2yuv matrix
		iDE_SCAL_Matrix_Mul(matrixEn, *ptmatrix, &matrixconv);

		for(i=0; i<16; i++)
		{
			*((__s64 *)(&tmpcoeff.x00) + i) = ((__s64)*(image_enhance_tab + (in_csc_mode<<5) + 0x10 + i) <<32)>>32;	//YUV2RGB
		}

		ptmatrix = &tmpcoeff;

		//convert to RGB
		iDE_SCAL_Matrix_Mul(*ptmatrix, matrixconv, &matrixconv);

        matrixresult.x00 = (matrixconv.x11+8)/16;  matrixresult.x01 = (matrixconv.x10+8)/16;
        matrixresult.x02 = (matrixconv.x12+8)/16;  matrixresult.x03 = (matrixconv.x13+512)/1024;
        matrixresult.x10 = (matrixconv.x01+8)/16;  matrixresult.x11 = (matrixconv.x00+8)/16;
        matrixresult.x12 = (matrixconv.x02+8)/16;  matrixresult.x13 = (matrixconv.x03+512)/1024;
        matrixresult.x20 = (matrixconv.x21+8)/16;  matrixresult.x21 = (matrixconv.x20+8)/16;
        matrixresult.x22 = (matrixconv.x22+8)/16;  matrixresult.x23 = (matrixconv.x23+512)/1024;
        matrixresult.x30 = (matrixconv.x31+8)/16;  matrixresult.x31 = (matrixconv.x30+8)/16;
        matrixresult.x32 = (matrixconv.x32+8)/16;  matrixresult.x33 = (matrixconv.x33+8)/16;

	}
	else if((incs == 1) && (outcs == 0)) //yuv to rgb
	{
		for(i=0; i<16; i++)
		{
			*((__s64 *)(&tmpcoeff.x00) + i) = ((__s64)*(image_enhance_tab + (in_csc_mode<<5) + 0x10 + i) <<32)>>32;	//YUV2RGB
		}

		ptmatrix = &tmpcoeff;

		iDE_SCAL_Matrix_Mul(*ptmatrix, matrixEn, &matrixconv);
        matrixresult.x00 = matrixconv.x10/4;  matrixresult.x01 = matrixconv.x11/4;
        matrixresult.x02 = matrixconv.x12/4;  matrixresult.x03 = matrixconv.x13/256;
        matrixresult.x10 = matrixconv.x00/4;  matrixresult.x11 = matrixconv.x01/4;
        matrixresult.x12 = matrixconv.x02/4;  matrixresult.x13 = matrixconv.x03/256;
        matrixresult.x20 = matrixconv.x20/4;  matrixresult.x21 = matrixconv.x21/4;
        matrixresult.x22 = matrixconv.x22/4;  matrixresult.x23 = matrixconv.x23/256;
        matrixresult.x30 = matrixconv.x30/4;  matrixresult.x31 = matrixconv.x31/4;
        matrixresult.x32 = matrixconv.x32/4;  matrixresult.x33 = matrixconv.x33/4;

	}
	else if((incs == 0) && (outcs == 1)) //rgb to yuv
	{
		for(i=0; i<16; i++)
		{
			*((__s64 *)(&tmpcoeff.x00) + i) = ((__s64)*(image_enhance_tab + (in_csc_mode<<5) + i) <<32)>>32;	//RGB2YUV
		}

		ptmatrix = &tmpcoeff;

		iDE_SCAL_Matrix_Mul(matrixEn, *ptmatrix, &matrixconv);
        matrixresult.x00 = matrixconv.x01/4;  matrixresult.x01 = matrixconv.x00/4;
        matrixresult.x02 = matrixconv.x02/4;  matrixresult.x03 = matrixconv.x03/256;
        matrixresult.x10 = matrixconv.x11/4;  matrixresult.x11 = matrixconv.x10/4;
        matrixresult.x12 = matrixconv.x12/4;  matrixresult.x13 = matrixconv.x13/256;
        matrixresult.x20 = matrixconv.x21/4;  matrixresult.x21 = matrixconv.x20/4;
        matrixresult.x22 = matrixconv.x22/4;  matrixresult.x23 = matrixconv.x23/256;
        matrixresult.x30 = matrixconv.x31/4;  matrixresult.x31 = matrixconv.x30/4;
        matrixresult.x32 = matrixconv.x32/4;  matrixresult.x33 = matrixconv.x33/4;
	}
	else  //yuv to yuv
	{
		matrixresult = matrixEn;
	}

    //data bit convert, 1 bit  sign, 2 bit integer, 10 bits fractrional for coefficient; 1 bit sign,9 bit integer, 4 bit fractional for constant
    //range limited
    iDE_SCAL_Csc_Lmt(&matrixresult.x00, -4095, 4095, 0, 8191);
    iDE_SCAL_Csc_Lmt(&matrixresult.x01, -4095, 4095, 0, 8191);
    iDE_SCAL_Csc_Lmt(&matrixresult.x02, -4095, 4095, 0, 8191);
    iDE_SCAL_Csc_Lmt(&matrixresult.x03, -8191, 8191, 0, 16383);
    iDE_SCAL_Csc_Lmt(&matrixresult.x10, -4095, 4095, 0, 8191);
    iDE_SCAL_Csc_Lmt(&matrixresult.x11, -4095, 4095, 0, 8191);
    iDE_SCAL_Csc_Lmt(&matrixresult.x12, -4095, 4095, 0, 8191);
    iDE_SCAL_Csc_Lmt(&matrixresult.x13, -8191, 8191, 0, 16383);
    iDE_SCAL_Csc_Lmt(&matrixresult.x20, -4095, 4095, 0, 8191);
    iDE_SCAL_Csc_Lmt(&matrixresult.x21, -4095, 4095, 0, 8191);
    iDE_SCAL_Csc_Lmt(&matrixresult.x22, -4095, 4095, 0, 8191);
    iDE_SCAL_Csc_Lmt(&matrixresult.x23, -8191, 8191, 0, 16383);

    //write csc register
    pt = (__s64 *)&(matrixresult.x00);
    for(i=0; i<4; i++)
    {
        scal_dev[sel]->csc_coef[i].dwval = *(pt + i);
		scal_dev[sel]->csc_coef[i+4 + out_br_swap * 4].dwval =  *(pt + i + 4 + in_br_swap * 4);
		scal_dev[sel]->csc_coef[i+8 - out_br_swap * 4].dwval =  *(pt + i + 8 - in_br_swap * 4);
	}

    scal_dev[sel]->bypass.bits.csc_bypass_en = 0;

	return 0;
}

//*********************************************************************************************
// function       : DE_SCAL_Get_3D_In_Single_Size( __scal_3d_inmode_t inmode, __scal_src_size_t *fullsize,__scal_src_size_t *singlesize)
// description    : get single image size according to 3D inmode and full size
// parameters     :
//                 sel <scaler select>
//                 inmode <3D input mode>
//                 fullsize <3D source size, maybe double width of left image or double heigth of left height>
//                 singlesize <3D left image size>
// return         :
//                 success
//***********************************************************************************************
s32 DE_SCAL_Get_3D_In_Single_Size(__scal_3d_inmode_t inmode, __scal_src_size_t *fullsize,__scal_src_size_t *singlesize)
{
	switch(inmode)
	{
		case DE_SCAL_3DIN_TB:
			singlesize->src_width = fullsize->src_width;
			singlesize->src_height = fullsize->src_height>>1;
			singlesize->scal_width = fullsize->scal_width;
			singlesize->scal_height = fullsize->scal_height>>1;
			singlesize->x_off = fullsize->x_off;
			singlesize->y_off = fullsize->y_off;
			break;
		case DE_SCAL_3DIN_SSF:
		case DE_SCAL_3DIN_SSH:
			singlesize->src_width = fullsize->src_width>>1;
			singlesize->src_height = fullsize->src_height;
			singlesize->scal_width = fullsize->scal_width>>1;
			singlesize->scal_height = fullsize->scal_height;
			singlesize->x_off = fullsize->x_off;
			singlesize->y_off = fullsize->y_off;
			break;
		case DE_SCAL_3DIN_LI:
			singlesize->src_width = fullsize->src_width;
			singlesize->src_height = fullsize->src_height>>1;
			singlesize->scal_width = fullsize->scal_width;
			singlesize->scal_height = fullsize->scal_height>>1;
			singlesize->x_off = fullsize->x_off;
			singlesize->y_off = fullsize->y_off>>1;
			break;
		case DE_SCAL_3DIN_FP:
			singlesize->src_width = fullsize->src_width;
			singlesize->src_height = fullsize->src_height;
			singlesize->scal_width = fullsize->scal_width;
			singlesize->scal_height = fullsize->scal_height;
			singlesize->x_off = fullsize->x_off;
			singlesize->y_off = fullsize->y_off;
			break;
		default:
			//undefine input mode
			break;
	}

	return 0;
}

//*********************************************************************************************
// function       : DE_SCAL_Get_3D_Out_Single_Size( __scal_3d_outmode_t outmode, __scal_out_size_t *singlesize,__scal_out_size_t *fullsize)
// description    : get 3D output single size according to 3D outmode and full image size
// parameters     :
//                 sel <scaler select>
//                 inmode <3D output mode>
//                 fullsize <3D source size, maybe double width of left image or double heigth of left height>
//                 singlesize <3D left image size>
// return         :
//                 success
//***********************************************************************************************
s32 DE_SCAL_Get_3D_Out_Single_Size(__scal_3d_outmode_t outmode, __scal_out_size_t *singlesize,__scal_out_size_t *fullsize)
{
	switch(outmode)
	{
		case DE_SCAL_3DOUT_CI_1:
		case DE_SCAL_3DOUT_CI_2:
		case DE_SCAL_3DOUT_CI_3:
		case DE_SCAL_3DOUT_CI_4:
		case DE_SCAL_3DOUT_HDMI_SSF:
		case DE_SCAL_3DOUT_HDMI_SSH:
			singlesize->height = fullsize->height;
			singlesize->width  = fullsize->width>>1;
			break;
		case DE_SCAL_3DOUT_LIRGB:
		case DE_SCAL_3DOUT_HDMI_TB:
		case DE_SCAL_3DOUT_HDMI_FPP:
		case DE_SCAL_3DOUT_HDMI_LI:
			singlesize->height = fullsize->height>>1;
			singlesize->width  = fullsize->width;
			break;
		case DE_SCAL_3DOUT_HDMI_FPI:
			singlesize->height = fullsize->height>>2;
			singlesize->width  = fullsize->width;
			break;
		case DE_SCAL_3DOUT_HDMI_FA:  //
			singlesize->height = fullsize->height;
			singlesize->width  = fullsize->width;
		default:
			//undefined mode
			break;

	}
	return 0;
}

//*********************************************************************************************
// function       : DE_SCAL_Get_3D_Out_Full_Size(__scal_3d_outmode_t outmode, __scal_out_size_t *singlesize,__scal_out_size_t *fullsize)
// description    : get 3D output full size according to 3D outmode and left/right image size
// parameters     :
//                 sel <scaler select>
//                 inmode <3D output mode>
//                 fullsize <3D source size, maybe double width of left image or double heigth of left height>
//                 singlesize <3D left image size>
// return         :
//                 success
//***********************************************************************************************
s32 DE_SCAL_Get_3D_Out_Full_Size(__scal_3d_outmode_t outmode, __scal_out_size_t *singlesize,__scal_out_size_t *fullsize)
{
	switch(outmode)
	{
		case DE_SCAL_3DOUT_CI_1:
		case DE_SCAL_3DOUT_CI_2:
		case DE_SCAL_3DOUT_CI_3:
		case DE_SCAL_3DOUT_CI_4:
		case DE_SCAL_3DOUT_HDMI_SSF:
		case DE_SCAL_3DOUT_HDMI_SSH:
			fullsize->height = singlesize->height;
			fullsize->width  = singlesize->width<<1;
			break;
		case DE_SCAL_3DOUT_LIRGB:
		case DE_SCAL_3DOUT_HDMI_TB:
		case DE_SCAL_3DOUT_HDMI_FPP:
		case DE_SCAL_3DOUT_HDMI_LI:
			fullsize->height = singlesize->height<<1;
			fullsize->width  = singlesize->width;
			break;
		case DE_SCAL_3DOUT_HDMI_FPI:
			fullsize->height = singlesize->height<<2;
			fullsize->width  = singlesize->width;
			break;
		case DE_SCAL_3DOUT_HDMI_FA:  //
			fullsize->height = singlesize->height;
			fullsize->width  = singlesize->width;
		default:
			//undefined mode
			break;

	}
	return 0;
}


//*********************************************************************************************
// function       : DE_SCAL_Set_3D_Fb_Addr(u8 sel, __scal_buf_addr_t *addr, __scal_buf_addr_t *addrtrd)
// description    : scaler change frame buffer address, only change start address parameters
// parameters     :
//                 sel <scaler select>
//                 addr  <frame buffer address for 3 channel, 32 bit absolute address>
//                 addrtrd <3D source right image buffer address, only needed when 3dinmode is FP>
// return         :
//                 success
//***********************************************************************************************
s32 DE_SCAL_Set_3D_Fb_Addr(u8 sel, __scal_buf_addr_t *addr, __scal_buf_addr_t *addrtrd)
{
    scal_dev[sel]->buf_addr0.dwval = addr->ch0_addr+ de_scal_ch0_offset;
    scal_dev[sel]->buf_addr1.dwval = addr->ch1_addr+ de_scal_ch1_offset;
    scal_dev[sel]->buf_addr2.dwval = addr->ch2_addr+ de_scal_ch2_offset;
	if(de_scal_trd_fp_en)
	{
		scal_dev[sel]->trd_buf_addr0.dwval = addrtrd->ch0_addr + de_scal_ch0r_offset;
		scal_dev[sel]->trd_buf_addr1.dwval = addrtrd->ch1_addr + de_scal_ch1r_offset;
		scal_dev[sel]->trd_buf_addr2.dwval = addrtrd->ch2_addr + de_scal_ch2r_offset;
	}
	else
	{
		scal_dev[sel]->trd_buf_addr0.dwval = addr->ch0_addr + de_scal_ch0r_offset;
		scal_dev[sel]->trd_buf_addr1.dwval = addr->ch1_addr + de_scal_ch1r_offset;
		scal_dev[sel]->trd_buf_addr2.dwval = addr->ch2_addr + de_scal_ch2r_offset;
	}

    return 0;
}

//*********************************************************************************************
// function       : DE_SCAL_Set_3D_Di_PreFrame_Addr(u8 sel, __scal_buf_addr_t *addr, __scal_buf_addr_t *addrtrd)
// description    : set scaler deinterlace pre frame luma/chroma address
// parameters     :
//                 sel <scaler select>,
//                 addr  <pre frame buffer address for 3 channel, 32 bit absolute address>
//                 addrtrd <pre 3D source right image buffer address, only needed when 3dinmode is FP>
// return         :
//                 success
//***********************************************************************************************
s32 DE_SCAL_Set_3D_Di_PreFrame_Addr(u8 sel, __scal_buf_addr_t *addr, __scal_buf_addr_t *addrtrd)
{
    scal_dev[sel]->di_preluma.dwval = addr->ch0_addr+ de_scal_ch0_offset;
	scal_dev[sel]->di_prechroma.dwval = addr->ch1_addr+ de_scal_ch1_offset;
	if(de_scal_trd_fp_en)
	{
		scal_dev[sel]->trd_preluma.dwval = addrtrd->ch0_addr + de_scal_ch0r_offset;
		scal_dev[sel]->trd_prechroma.dwval = addrtrd->ch1_addr + de_scal_ch1r_offset;
	}
	else
	{
		scal_dev[sel]->trd_preluma.dwval = addr->ch0_addr + de_scal_ch0r_offset;
		scal_dev[sel]->trd_prechroma.dwval = addr->ch1_addr + de_scal_ch1r_offset;
	}
    return 0;
}


//*********************************************************************************************
// function       : DE_SCAL_Set_3D_Ctrl(u8 sel, u8 trden, __scal_3d_inmode_t inmode,
//								__scal_3d_outmode_t outmode)
// description    : scaler 3D control setting
// parameters     :
//                 sel <scaler select>
//                 trden  <3D enable, when 3D mode close, left picture >
//                 inmode <3D input mode>
//                 outmode <3D output mode>
// return         :
//                 success
//***********************************************************************************************
s32 DE_SCAL_Set_3D_Ctrl(u8 sel, u8 trden, __scal_3d_inmode_t inmode,
								__scal_3d_outmode_t outmode)
{
	u8 in_li_en=0;
	u8 out_ci_en=0, out_tb_en=0, out_ss_en=0, out_itl_en=0;
	u8 model_sel=0;
	u8 ci_mod=0;

	switch(inmode)
	{
		case DE_SCAL_3DIN_LI:;
			in_li_en = 1;
			break;
		default:
			in_li_en = 0;
			break;
	}

    if(trden)
    {
    	switch(outmode)
    	{
    		case DE_SCAL_3DOUT_CI_1:;
    			ci_mod = 0;out_ci_en = 1;break;
    		case DE_SCAL_3DOUT_CI_2:
    			ci_mod = 1;out_ci_en = 1;break;
    		case DE_SCAL_3DOUT_CI_3:
    			ci_mod = 2;out_ci_en = 1;break;
    		case DE_SCAL_3DOUT_CI_4:
    			ci_mod = 3;out_ci_en = 1;break;
    		case DE_SCAL_3DOUT_HDMI_SSF:;
    		case DE_SCAL_3DOUT_HDMI_SSH:
    			out_ss_en = 1;
    			break;
    		case DE_SCAL_3DOUT_HDMI_TB:;
    		case DE_SCAL_3DOUT_HDMI_FPP:
    			out_tb_en = 1;
    			break;
    		case DE_SCAL_3DOUT_HDMI_FPI:
    			out_tb_en = 1;
    			out_itl_en = 1;
    			break;
    		case DE_SCAL_3DOUT_HDMI_FA:  //
    			break;
    		default:
    			//undefined mode
    			break;
    	}
	}
	model_sel = trden? (out_tb_en ? 2 :1 ) : 0;

	scal_dev[sel]->trd_ctrl.bits.mod_sel = model_sel;
	scal_dev[sel]->trd_ctrl.bits.ci_out_en = out_ci_en;
	scal_dev[sel]->trd_ctrl.bits.ss_out_en = out_ss_en;
	scal_dev[sel]->trd_ctrl.bits.li_in_en = in_li_en;
	scal_dev[sel]->trd_ctrl.bits.tb_out_scan_mod = out_itl_en;
	scal_dev[sel]->trd_ctrl.bits.ci_out_mod = ci_mod;
	scal_dev[sel]->trd_ctrl.bits.tb_out_mod_field = out_tb_en ? (out_itl_en ? 3 : 1) : 0;
	scal_dev[sel]->field_ctrl.bits.valid_field_cnt = out_tb_en ? (out_itl_en ? 3 : 1) : 0;
	scal_dev[sel]->field_ctrl.bits.field_cnt = out_tb_en ? (out_itl_en ? 0xC : 2) : 0;
	de_scal_trd_itl_en = out_itl_en;
	return 0;
}

//*********************************************************************************************
// function       : DE_SCAL_Config_3D_Src(u8 sel, __scal_buf_addr_t *addr, __scal_src_size_t *size,
//                           __scal_src_type_t *type, __scal_3d_inmode_t trdinmode, __scal_buf_addr_t *addrtrd)
// description    : scaler 3D source concerning parameter configure
// parameters     :
//                 sel <scaler select>
//                 addr  <3D left image frame buffer address for 3 channel, 32 bit absolute address>
//                 size <scale region define,  src size, offset, scal size>
//                 type <src data type, include byte sequence, mode, format, pixel sequence>
//                 trdinmode <3D input mode>
//                 addrtrd <3D right image frame buffer address for 3 channel, this address must be set when 3d inmode
//                              is FP_P/FP_M, for other mode, the right image buffer address can be get through left image address>
// return         :
//                 success
// history        : 2012/06/05, added uv combined 3D support
//***********************************************************************************************
s32 DE_SCAL_Config_3D_Src(u8 sel, __scal_buf_addr_t *addr, __scal_src_size_t *size,
                           __scal_src_type_t *type, __scal_3d_inmode_t trdinmode, __scal_buf_addr_t *addrtrd)
{
    u8 w_shift, h_shift;
	u32 image_w0, image_w1, image_h0, image_h1;
	u32 x_off0, y_off0, x_off1, y_off1;
	u32 in_w0, in_h0, in_w1, in_h1;
	u8  de_scal_ch0_dx0,de_scal_ch0_dx1,de_scal_ch0_dy0;
	u8  de_scal_ch1_dx0,de_scal_ch1_dx1,de_scal_ch1_dy0;

	image_w0 = size->src_width;
	image_h0 = size->src_height;   //must be set in 3D mode, because of right address based on it !!!!
	in_w0 = size->scal_width;
	in_h0 = size->scal_height;
	x_off0 = size->x_off;
	y_off0 = size->y_off;

	de_scal_trd_fp_en = 0;

    if(type->fmt == DE_SCAL_INYUV422 || type->fmt == DE_SCAL_INYUV420)
    {
        w_shift = 1;
        image_w0 = (image_w0 + 1)&0xfffffffe;
    	image_w1 = (image_w0)>>w_shift;
        in_w0 = in_w0 & 0xfffffffe;
    	in_w1 = (in_w0 + 0x1)>>w_shift;
        x_off0 = x_off0 & 0xfffffffe;
    	x_off1 = (x_off0)>>w_shift;
    }
    else if(type->fmt == DE_SCAL_INYUV411)
    {
        w_shift = 2;
    	image_w1 = (image_w0 + 0x3)>>w_shift;
        in_w0 &= 0xfffffffc;
    	in_w1 = (in_w0 + 0x3)>>w_shift;
        x_off0 &= 0xfffffffc;
    	x_off1 = (x_off0)>>w_shift;
    }
    else
    {
        w_shift = 0;
    	image_w1 = image_w0;
    	in_w1 = in_w0;
    	x_off1 = x_off0;
    }
    if(type->fmt == DE_SCAL_INYUV420)
    {
        h_shift = 1;
		image_h0 &= 0xfffffffe;
		image_h1 = ((image_h0 + 0x1) >> h_shift);
        in_h0 &= 0xfffffffe;
    	in_h1 = (in_h0 + 0x1)>>h_shift;
        y_off0 &= 0xfffffffe;
    	y_off1 = (y_off0)>>h_shift;
    }
    else
    {
        h_shift = 0;
		image_h1 = image_h0;
    	in_h1 = in_h0;
    	y_off1 = y_off0;
    }

	//added no-zero limited
    in_h0 = (in_h0!=0) ? in_h0 : 1;
	in_h1 = (in_h1!=0) ? in_h1 : 1;
	in_w0 = (in_w0!=0) ? in_w0 : 1;
	in_w1 = (in_w1!=0) ? in_w1 : 1;

	if(type->mod == DE_SCAL_PLANNAR)
	{
		if((trdinmode == DE_SCAL_3DIN_TB))
		{
		    scal_dev[sel]->linestrd0.dwval = image_w0;
			scal_dev[sel]->linestrd1.dwval = image_w1;
			scal_dev[sel]->linestrd2.dwval = image_w1;

	        de_scal_ch0_offset = image_w0 * y_off0 + x_off0;
	        de_scal_ch1_offset = image_w1 * y_off1 + x_off1;
	        de_scal_ch2_offset = image_w1 * y_off1 + x_off1;

			de_scal_ch0r_offset = image_w0 * image_h0 + de_scal_ch0_offset;
			de_scal_ch1r_offset = image_w1 * image_h1 + de_scal_ch1_offset;
			de_scal_ch2r_offset = image_w1 * image_h1 + de_scal_ch2_offset;

			scal_dev[sel]->buf_addr0.dwval = addr->ch0_addr+ de_scal_ch0_offset;
			scal_dev[sel]->buf_addr1.dwval = addr->ch1_addr+ de_scal_ch1_offset;
			scal_dev[sel]->buf_addr2.dwval = addr->ch2_addr+ de_scal_ch2_offset;

			scal_dev[sel]->trd_buf_addr0.dwval = addr->ch0_addr + de_scal_ch0r_offset;
			scal_dev[sel]->trd_buf_addr1.dwval = addr->ch1_addr + de_scal_ch1r_offset;
			scal_dev[sel]->trd_buf_addr2.dwval = addr->ch2_addr + de_scal_ch2r_offset;

		}
		else if((trdinmode == DE_SCAL_3DIN_FP))
		{
		    scal_dev[sel]->linestrd0.dwval = image_w0;
			scal_dev[sel]->linestrd1.dwval = image_w1;
			scal_dev[sel]->linestrd2.dwval = image_w1;

	        de_scal_ch0_offset = image_w0 * y_off0 + x_off0;
	        de_scal_ch1_offset = image_w1 * y_off1 + x_off1;
	        de_scal_ch2_offset = image_w1 * y_off1 + x_off1;

			de_scal_ch0r_offset = de_scal_ch0_offset;
			de_scal_ch1r_offset = de_scal_ch1_offset;
			de_scal_ch2r_offset = de_scal_ch2_offset;

			de_scal_trd_fp_en = 1;

			scal_dev[sel]->buf_addr0.dwval = addr->ch0_addr+ de_scal_ch0_offset;
			scal_dev[sel]->buf_addr1.dwval = addr->ch1_addr+ de_scal_ch1_offset;
			scal_dev[sel]->buf_addr2.dwval = addr->ch2_addr+ de_scal_ch2_offset;

			scal_dev[sel]->trd_buf_addr0.dwval = addrtrd->ch0_addr + de_scal_ch0r_offset;
			scal_dev[sel]->trd_buf_addr1.dwval = addrtrd->ch1_addr + de_scal_ch1r_offset;
			scal_dev[sel]->trd_buf_addr2.dwval = addrtrd->ch2_addr + de_scal_ch2r_offset;
		}
		else if((trdinmode == DE_SCAL_3DIN_SSF) || (trdinmode == DE_SCAL_3DIN_SSH))
		{
		    scal_dev[sel]->linestrd0.dwval = image_w0<<1;
			scal_dev[sel]->linestrd1.dwval = image_w1<<1;
			scal_dev[sel]->linestrd2.dwval = image_w1<<1;

	        de_scal_ch0_offset = (image_w0<<1) * y_off0 + x_off0;
	        de_scal_ch1_offset = (image_w1<<1) * y_off1 + x_off1;
	        de_scal_ch2_offset = (image_w1<<1) * y_off1 + x_off1;

			de_scal_ch0r_offset = image_w0 + de_scal_ch0_offset;
			de_scal_ch1r_offset = image_w1 + de_scal_ch1_offset;
			de_scal_ch2r_offset = image_w1 + de_scal_ch2_offset;

			scal_dev[sel]->buf_addr0.dwval = addr->ch0_addr+ de_scal_ch0_offset;
			scal_dev[sel]->buf_addr1.dwval = addr->ch1_addr+ de_scal_ch1_offset;
			scal_dev[sel]->buf_addr2.dwval = addr->ch2_addr+ de_scal_ch2_offset;

			scal_dev[sel]->trd_buf_addr0.dwval = addr->ch0_addr + de_scal_ch0r_offset;
			scal_dev[sel]->trd_buf_addr1.dwval = addr->ch1_addr + de_scal_ch1r_offset;
			scal_dev[sel]->trd_buf_addr2.dwval = addr->ch2_addr + de_scal_ch2r_offset;
		}
		else if((trdinmode == DE_SCAL_3DIN_LI))
		{
		    scal_dev[sel]->linestrd0.dwval = image_w0;
			scal_dev[sel]->linestrd1.dwval = image_w1;
			scal_dev[sel]->linestrd2.dwval = image_w1;

	        de_scal_ch0_offset = (image_w0) * (y_off0<<1) + x_off0;
	        de_scal_ch1_offset = (image_w1) * (y_off1<<1) + x_off1;
	        de_scal_ch2_offset = (image_w1) * (y_off1<<1) + x_off1;

			de_scal_ch0r_offset = image_w0 + de_scal_ch0_offset;
			de_scal_ch1r_offset = image_w1 + de_scal_ch1_offset;
			de_scal_ch2r_offset = image_w1 + de_scal_ch2_offset;

			scal_dev[sel]->buf_addr0.dwval = addr->ch0_addr+ de_scal_ch0_offset;
			scal_dev[sel]->buf_addr1.dwval = addr->ch1_addr+ de_scal_ch1_offset;
			scal_dev[sel]->buf_addr2.dwval = addr->ch2_addr+ de_scal_ch2_offset;

			scal_dev[sel]->trd_buf_addr0.dwval = addr->ch0_addr + de_scal_ch0r_offset;
			scal_dev[sel]->trd_buf_addr1.dwval = addr->ch1_addr + de_scal_ch1r_offset;
			scal_dev[sel]->trd_buf_addr2.dwval = addr->ch2_addr + de_scal_ch2r_offset;
		}
	}
	else if(type->mod == DE_SCAL_INTERLEAVED)
	{
		if((trdinmode == DE_SCAL_3DIN_FP))
		{
			scal_dev[sel]->linestrd0.dwval = image_w0<<(2-w_shift);
			scal_dev[sel]->linestrd1.dwval = 0;
			scal_dev[sel]->linestrd2.dwval = 0;

	        de_scal_ch0_offset = (image_w0<<(2-w_shift)) * y_off0 + x_off0;
	        de_scal_ch1_offset = 0;
	        de_scal_ch2_offset = 0;

			de_scal_ch0r_offset = de_scal_ch0_offset;
			de_scal_ch1r_offset = de_scal_ch1_offset;
			de_scal_ch2r_offset = de_scal_ch2_offset;

			de_scal_trd_fp_en = 1;

			scal_dev[sel]->buf_addr0.dwval = addr->ch0_addr+ de_scal_ch0_offset;
			scal_dev[sel]->buf_addr1.dwval = addr->ch1_addr+ de_scal_ch1_offset;
			scal_dev[sel]->buf_addr2.dwval = addr->ch2_addr+ de_scal_ch2_offset;

			scal_dev[sel]->trd_buf_addr0.dwval = addrtrd->ch0_addr + de_scal_ch0r_offset;
			scal_dev[sel]->trd_buf_addr1.dwval = addrtrd->ch1_addr + de_scal_ch1r_offset;
			scal_dev[sel]->trd_buf_addr2.dwval = addrtrd->ch2_addr + de_scal_ch2r_offset;
		}
		else if((trdinmode == DE_SCAL_3DIN_TB))
		{
			scal_dev[sel]->linestrd0.dwval = image_w0<<(2-w_shift);
			scal_dev[sel]->linestrd1.dwval = 0;
			scal_dev[sel]->linestrd2.dwval = 0;

	        de_scal_ch0_offset = (image_w0<<(2-w_shift)) * y_off0 + x_off0;
	        de_scal_ch1_offset = 0;
	        de_scal_ch2_offset = 0;

			de_scal_ch0r_offset = (image_w0<<(2-w_shift)) * image_h0 + de_scal_ch0_offset;
			de_scal_ch1r_offset = de_scal_ch1_offset;
			de_scal_ch2r_offset = de_scal_ch2_offset;

			de_scal_trd_fp_en = 0;

			scal_dev[sel]->buf_addr0.dwval = addr->ch0_addr+ de_scal_ch0_offset;
			scal_dev[sel]->buf_addr1.dwval = addr->ch1_addr+ de_scal_ch1_offset;
			scal_dev[sel]->buf_addr2.dwval = addr->ch2_addr+ de_scal_ch2_offset;

			scal_dev[sel]->trd_buf_addr0.dwval = addr->ch0_addr + de_scal_ch0r_offset;
			scal_dev[sel]->trd_buf_addr1.dwval = addr->ch1_addr + de_scal_ch1r_offset;
			scal_dev[sel]->trd_buf_addr2.dwval = addr->ch2_addr + de_scal_ch2r_offset;
		}
		else if((trdinmode == DE_SCAL_3DIN_SSF)||(trdinmode == DE_SCAL_3DIN_SSH))
		{
			scal_dev[sel]->linestrd0.dwval = image_w0<<(3-w_shift);
			scal_dev[sel]->linestrd1.dwval = 0;
			scal_dev[sel]->linestrd2.dwval = 0;

	        de_scal_ch0_offset = (image_w0<<(3-w_shift)) * y_off0 + x_off0;
	        de_scal_ch1_offset = 0;
	        de_scal_ch2_offset = 0;

			de_scal_ch0r_offset = (image_w0<<(2-w_shift)) + de_scal_ch0_offset;
			de_scal_ch1r_offset = de_scal_ch1_offset;
			de_scal_ch2r_offset = de_scal_ch2_offset;

			de_scal_trd_fp_en = 0;

			scal_dev[sel]->buf_addr0.dwval = addr->ch0_addr+ de_scal_ch0_offset;
			scal_dev[sel]->buf_addr1.dwval = addr->ch1_addr+ de_scal_ch1_offset;
			scal_dev[sel]->buf_addr2.dwval = addr->ch2_addr+ de_scal_ch2_offset;

			scal_dev[sel]->trd_buf_addr0.dwval = addr->ch0_addr + de_scal_ch0r_offset;
			scal_dev[sel]->trd_buf_addr1.dwval = addr->ch1_addr + de_scal_ch1r_offset;
			scal_dev[sel]->trd_buf_addr2.dwval = addr->ch2_addr + de_scal_ch2r_offset;
		}
		else if((trdinmode == DE_SCAL_3DIN_LI))
		{
			scal_dev[sel]->linestrd0.dwval = image_w0<<(2-w_shift);
			scal_dev[sel]->linestrd1.dwval = 0;
			scal_dev[sel]->linestrd2.dwval = 0;

	        de_scal_ch0_offset = (image_w0<<(2-w_shift)) * (y_off0<<1) + x_off0;
	        de_scal_ch1_offset = 0;
	        de_scal_ch2_offset = 0;

			de_scal_ch0r_offset = (image_w0<<(2-w_shift)) + de_scal_ch0_offset;
			de_scal_ch1r_offset = de_scal_ch1_offset;
			de_scal_ch2r_offset = de_scal_ch2_offset;

			de_scal_trd_fp_en = 0;

			scal_dev[sel]->buf_addr0.dwval = addr->ch0_addr+ de_scal_ch0_offset;
			scal_dev[sel]->buf_addr1.dwval = addr->ch1_addr+ de_scal_ch1_offset;
			scal_dev[sel]->buf_addr2.dwval = addr->ch2_addr+ de_scal_ch2_offset;

			scal_dev[sel]->trd_buf_addr0.dwval = addr->ch0_addr + de_scal_ch0r_offset;
			scal_dev[sel]->trd_buf_addr1.dwval = addr->ch1_addr + de_scal_ch1r_offset;
			scal_dev[sel]->trd_buf_addr2.dwval = addr->ch2_addr + de_scal_ch2r_offset;
		}
	}
	else if(type->mod == DE_SCAL_UVCOMBINED) //
	{
		if((trdinmode == DE_SCAL_3DIN_SSF) || (trdinmode == DE_SCAL_3DIN_SSH))
		{
			scal_dev[sel]->linestrd0.dwval = image_w0 * 2;
			scal_dev[sel]->linestrd1.dwval = image_w1 * 2 * 2;
			scal_dev[sel]->linestrd2.dwval = 0;

	        de_scal_ch0_offset = (2 * image_w0) * y_off0 + x_off0;
	        de_scal_ch1_offset = 2 * (2 * image_w1) * y_off1 + 2 * x_off1;
	        de_scal_ch2_offset = 0;

			de_scal_ch0r_offset = image_w0;
			de_scal_ch1r_offset = image_w1 * 2;
			de_scal_ch2r_offset = 0;

			de_scal_trd_fp_en = 0;

			scal_dev[sel]->buf_addr0.dwval = addr->ch0_addr+ de_scal_ch0_offset;
			scal_dev[sel]->buf_addr1.dwval = addr->ch1_addr+ de_scal_ch1_offset;
			scal_dev[sel]->buf_addr2.dwval = addr->ch2_addr+ de_scal_ch2_offset;

			scal_dev[sel]->trd_buf_addr0.dwval = addr->ch0_addr + de_scal_ch0r_offset;
			scal_dev[sel]->trd_buf_addr1.dwval = addr->ch1_addr + de_scal_ch1r_offset;
			scal_dev[sel]->trd_buf_addr2.dwval = addr->ch2_addr + de_scal_ch2r_offset;
		}
		else if(trdinmode == DE_SCAL_3DIN_TB)
		{
			scal_dev[sel]->linestrd0.dwval = image_w0;
			scal_dev[sel]->linestrd1.dwval = image_w1 * 2;
			scal_dev[sel]->linestrd2.dwval = 0;

	        de_scal_ch0_offset = (image_w0) * y_off0 + x_off0;
	        de_scal_ch1_offset = (image_w1 * 2) * y_off1 + 2 * x_off1;
	        de_scal_ch2_offset = 0;

			de_scal_ch0r_offset = (image_w0) * image_h0 + de_scal_ch0_offset;
			de_scal_ch1r_offset = (image_w1 * 2) * image_h1 + de_scal_ch1_offset;
			de_scal_ch2r_offset = 0;

			de_scal_trd_fp_en = 0;

			scal_dev[sel]->buf_addr0.dwval = addr->ch0_addr+ de_scal_ch0_offset;
			scal_dev[sel]->buf_addr1.dwval = addr->ch1_addr+ de_scal_ch1_offset;
			scal_dev[sel]->buf_addr2.dwval = addr->ch2_addr+ de_scal_ch2_offset;

			scal_dev[sel]->trd_buf_addr0.dwval = addr->ch0_addr + de_scal_ch0r_offset;
			scal_dev[sel]->trd_buf_addr1.dwval = addr->ch1_addr + de_scal_ch1r_offset;
			scal_dev[sel]->trd_buf_addr2.dwval = addr->ch2_addr + de_scal_ch2r_offset;
		}
		else if(trdinmode == DE_SCAL_3DIN_FP)
		{
			scal_dev[sel]->linestrd0.dwval = image_w0;
			scal_dev[sel]->linestrd1.dwval = image_w1 * 2;
			scal_dev[sel]->linestrd2.dwval = 0;

	        de_scal_ch0_offset = (image_w0) * y_off0 + x_off0;
	        de_scal_ch1_offset = (image_w1 * 2) * y_off1 + 2 * x_off1;
	        de_scal_ch2_offset = 0;

			de_scal_ch0r_offset = de_scal_ch0_offset;
			de_scal_ch1r_offset = de_scal_ch1_offset;
			de_scal_ch2r_offset = de_scal_ch2_offset;

			de_scal_trd_fp_en = 1;

			scal_dev[sel]->buf_addr0.dwval = addr->ch0_addr+ de_scal_ch0_offset;
			scal_dev[sel]->buf_addr1.dwval = addr->ch1_addr+ de_scal_ch1_offset;
			scal_dev[sel]->buf_addr2.dwval = addr->ch2_addr+ de_scal_ch2_offset;

			scal_dev[sel]->trd_buf_addr0.dwval = addrtrd->ch0_addr + de_scal_ch0r_offset;
			scal_dev[sel]->trd_buf_addr1.dwval = addrtrd->ch1_addr + de_scal_ch1r_offset;
			scal_dev[sel]->trd_buf_addr2.dwval = addrtrd->ch2_addr + de_scal_ch2r_offset;
		}
		else if(trdinmode == DE_SCAL_3DIN_LI)
		{
			scal_dev[sel]->linestrd0.dwval = image_w0;
			scal_dev[sel]->linestrd1.dwval = image_w1 * 2;
			scal_dev[sel]->linestrd2.dwval = 0;

	        de_scal_ch0_offset = (image_w0 * 2) * y_off0 + x_off0;
	        de_scal_ch1_offset = 2 * (image_w1 * 2) * y_off1 + 2 * x_off1;
	        de_scal_ch2_offset = 0;

			de_scal_ch0r_offset = de_scal_ch0_offset + image_w0;
			de_scal_ch1r_offset = de_scal_ch1_offset + image_w1 * 2;
			de_scal_ch2r_offset = de_scal_ch2_offset;

			de_scal_trd_fp_en = 0;

			scal_dev[sel]->buf_addr0.dwval = addr->ch0_addr+ de_scal_ch0_offset;
			scal_dev[sel]->buf_addr1.dwval = addr->ch1_addr+ de_scal_ch1_offset;
			scal_dev[sel]->buf_addr2.dwval = addr->ch2_addr+ de_scal_ch2_offset;

			scal_dev[sel]->trd_buf_addr0.dwval = addr->ch0_addr + de_scal_ch0r_offset;
			scal_dev[sel]->trd_buf_addr1.dwval = addr->ch1_addr + de_scal_ch1r_offset;
			scal_dev[sel]->trd_buf_addr2.dwval = addr->ch2_addr + de_scal_ch2r_offset;
		}
	}
	else if(type->mod == DE_SCAL_UVCOMBINEDMB)
	{
		if((trdinmode == DE_SCAL_3DIN_TB))
		{
		    scal_dev[sel]->linestrd0.dwval = (((image_w0+0x1f)&0xffe0) - 0x1f)<<0x05;;
			scal_dev[sel]->linestrd1.dwval = (((((image_w1)<<1)+0x1f)&0xffe0) - 0x1f) << 0x05;
			scal_dev[sel]->linestrd2.dwval = 0x00;

			//block offset
			de_scal_ch0_dx0 = (x_off0 & 0x1f);
			de_scal_ch0_dy0 = (y_off0 & 0x1f);
			de_scal_ch0_dx1 = ((de_scal_ch0_dx0 & 0x1f) + in_w0 + 0x1f) &0x1f;
			de_scal_ch1_dx0 = (((x_off1)<<1)&0x1f);
			de_scal_ch1_dy0 = ((y_off1)&0x1f);
			de_scal_ch1_dx1 = ((de_scal_ch1_dx0 & 0x1e) + ((in_w1)<<1) + 0x1f) & 0x1f;

			scal_dev[sel]->tb_off0.bits.x_offset0 = de_scal_ch0_dx0;
			scal_dev[sel]->tb_off0.bits.y_offset0 = de_scal_ch0_dy0;
			scal_dev[sel]->tb_off0.bits.x_offset1 = de_scal_ch0_dx1;
			scal_dev[sel]->tb_off1.bits.x_offset0 = de_scal_ch1_dx0;
			scal_dev[sel]->tb_off1.bits.y_offset0 = de_scal_ch1_dy0;
			scal_dev[sel]->tb_off1.bits.x_offset1 = de_scal_ch1_dx1;
			scal_dev[sel]->trd_tb_off0.bits.x_offset0 = de_scal_ch0_dx0;
			scal_dev[sel]->trd_tb_off0.bits.y_offset0 = (image_h0 + y_off0) & 0x1f;
			scal_dev[sel]->trd_tb_off0.bits.x_offset1 = de_scal_ch0_dx1;
			scal_dev[sel]->trd_tb_off1.bits.x_offset0 = de_scal_ch1_dx0;
			scal_dev[sel]->trd_tb_off1.bits.y_offset0 = (image_h1 + y_off1) & 0x1f;
			scal_dev[sel]->trd_tb_off1.bits.x_offset1 = de_scal_ch1_dx1;

			de_scal_ch0_offset = ((image_w0 + 0x1f) &0xffe0) * (y_off0& 0xffe0) + ((y_off0& 0x01f)<<5) +
	                              ((x_off0& 0xffe0)<<5);
	        de_scal_ch1_offset = (((image_w1<< 0x01)+0x1f)&0xffe0) * ((y_off1) & 0xffe0) +
	                              (((y_off1) & 0x01f)<<5) + (((x_off1<<0x01) & 0xffe0)<<5);
	        de_scal_ch2_offset = 0x0;

			de_scal_ch0r_offset = ((image_w0 + 0x1f) &0xffe0) * ((y_off0+ image_h0) & 0xffe0) +
							(((y_off0+ image_h0)& 0x01f)<<5) + ((x_off0& 0xffe0)<<5);
			de_scal_ch1r_offset = (((image_w1<< 0x01)+0x1f)&0xffe0) * ((y_off1+ image_h1) & 0xffe0) +
	                       (((y_off1+ image_h1) & 0x01f)<<5) + (((x_off1<<0x01) & 0xffe0)<<5);
			de_scal_ch2r_offset = 0x0;

			scal_dev[sel]->buf_addr0.dwval = addr->ch0_addr+ de_scal_ch0_offset;
			scal_dev[sel]->buf_addr1.dwval = addr->ch1_addr+ de_scal_ch1_offset;
			scal_dev[sel]->buf_addr2.dwval = addr->ch2_addr+ de_scal_ch2_offset;

			scal_dev[sel]->trd_buf_addr0.dwval = addr->ch0_addr + de_scal_ch0r_offset;
			scal_dev[sel]->trd_buf_addr1.dwval = addr->ch1_addr + de_scal_ch1r_offset;
			scal_dev[sel]->trd_buf_addr2.dwval = addr->ch2_addr + de_scal_ch2r_offset;
		}
		else if((trdinmode == DE_SCAL_3DIN_FP))
		{
			de_scal_trd_fp_en = 1;
		    scal_dev[sel]->linestrd0.dwval = (((image_w0+0x1f)&0xffe0) - 0x1f)<<0x05;;
			scal_dev[sel]->linestrd1.dwval = (((((image_w1)<<1)+0x1f)&0xffe0) - 0x1f) << 0x05;
			scal_dev[sel]->linestrd2.dwval = 0x00;

			//block offset
			de_scal_ch0_dx0 = (x_off0 & 0x1f);
			de_scal_ch0_dy0 = (y_off0 & 0x1f);
			de_scal_ch0_dx1 = ((de_scal_ch0_dx0 & 0x1f) + in_w0 + 0x1f) &0x1f;
			de_scal_ch1_dx0 = (((x_off1)<<1)&0x1f);
			de_scal_ch1_dy0 = ((y_off1)&0x1f);
			de_scal_ch1_dx1 = ((de_scal_ch1_dx0 & 0x1e) + ((in_w1)<<1) + 0x1f) & 0x1f;

			scal_dev[sel]->tb_off0.bits.x_offset0 = de_scal_ch0_dx0;
			scal_dev[sel]->tb_off0.bits.y_offset0 = de_scal_ch0_dy0;
			scal_dev[sel]->tb_off0.bits.x_offset1 = de_scal_ch0_dx1;
			scal_dev[sel]->tb_off1.bits.x_offset0 = de_scal_ch1_dx0;
			scal_dev[sel]->tb_off1.bits.y_offset0 = de_scal_ch1_dy0;
			scal_dev[sel]->tb_off1.bits.x_offset1 = de_scal_ch1_dx1;
			scal_dev[sel]->trd_tb_off0.bits.x_offset0 = de_scal_ch0_dx0;
			scal_dev[sel]->trd_tb_off0.bits.y_offset0 = de_scal_ch0_dy0;
			scal_dev[sel]->trd_tb_off0.bits.x_offset1 = de_scal_ch0_dx1;
			scal_dev[sel]->trd_tb_off1.bits.x_offset0 = de_scal_ch1_dx0;
			scal_dev[sel]->trd_tb_off1.bits.y_offset0 = de_scal_ch1_dy0;
			scal_dev[sel]->trd_tb_off1.bits.x_offset1 = de_scal_ch1_dx1;

			de_scal_ch0_offset = ((image_w0 + 0x1f) &0xffe0) * (y_off0& 0xffe0) + ((y_off0& 0x01f)<<5) +
	                              ((x_off0& 0xffe0)<<5);
	        de_scal_ch1_offset = (((image_w1<< 0x01)+0x1f)&0xffe0) * ((y_off1) & 0xffe0) +
	                              (((y_off1) & 0x01f)<<5) + (((x_off1<<0x01) & 0xffe0)<<5);
	        de_scal_ch2_offset = 0x0;

			de_scal_ch0r_offset = de_scal_ch0_offset;
			de_scal_ch1r_offset = de_scal_ch1_offset;
			de_scal_ch2r_offset = 0x0;

			scal_dev[sel]->buf_addr0.dwval = addr->ch0_addr+ de_scal_ch0_offset;
			scal_dev[sel]->buf_addr1.dwval = addr->ch1_addr+ de_scal_ch1_offset;
			scal_dev[sel]->buf_addr2.dwval = addr->ch2_addr+ de_scal_ch2_offset;

			scal_dev[sel]->trd_buf_addr0.dwval = addrtrd->ch0_addr + de_scal_ch0r_offset;
			scal_dev[sel]->trd_buf_addr1.dwval = addrtrd->ch1_addr + de_scal_ch1r_offset;
			scal_dev[sel]->trd_buf_addr2.dwval = addrtrd->ch2_addr + de_scal_ch2r_offset;
		}
		else if(((trdinmode == DE_SCAL_3DIN_SSF) ||(trdinmode == DE_SCAL_3DIN_SSH)))
		{
		    scal_dev[sel]->linestrd0.dwval = (((2*image_w0+0x1f)&0xffe0) - 0x1f)<<0x05;;
			scal_dev[sel]->linestrd1.dwval = (((((2*image_w1)<<1)+0x1f)&0xffe0) - 0x1f) << 0x05;
			scal_dev[sel]->linestrd2.dwval = 0x00;

			//block offset
			de_scal_ch0_dx0 = (x_off0 & 0x1f);
			de_scal_ch0_dy0 = (y_off0 & 0x1f);
			de_scal_ch0_dx1 = ((de_scal_ch0_dx0 & 0x1f) + in_w0 + 0x1f) &0x1f;
			de_scal_ch1_dx0 = (((x_off1)<<1)&0x1f);
			de_scal_ch1_dy0 = ((y_off1)&0x1f);
			de_scal_ch1_dx1 = ((de_scal_ch1_dx0 & 0x1e) + ((in_w1)<<1) + 0x1f) & 0x1f;

			scal_dev[sel]->tb_off0.bits.x_offset0 = de_scal_ch0_dx0;
			scal_dev[sel]->tb_off0.bits.y_offset0 = de_scal_ch0_dy0;
			scal_dev[sel]->tb_off0.bits.x_offset1 = de_scal_ch0_dx1;
			scal_dev[sel]->tb_off1.bits.x_offset0 = de_scal_ch1_dx0;
			scal_dev[sel]->tb_off1.bits.y_offset0 = de_scal_ch1_dy0;
			scal_dev[sel]->tb_off1.bits.x_offset1 = de_scal_ch1_dx1;
			scal_dev[sel]->trd_tb_off0.bits.x_offset0 = (image_w0 + x_off0) & 0x1f;
			scal_dev[sel]->trd_tb_off0.bits.y_offset0 = de_scal_ch0_dy0;
			scal_dev[sel]->trd_tb_off0.bits.x_offset1 = (((image_w0 + x_off0) & 0x1f) + in_w0 + 0x1f)&0x1f;
			scal_dev[sel]->trd_tb_off1.bits.x_offset0 = ((2 * (image_w1 + x_off1)) & 0x1f);
			scal_dev[sel]->trd_tb_off1.bits.y_offset0 = de_scal_ch1_dy0;
			scal_dev[sel]->trd_tb_off1.bits.x_offset1 = (((2 * (image_w1 + x_off1)) & 0x1f) + (in_w1<<1) + 0x1f)&0x1f;

			de_scal_ch0_offset = ((2 *image_w0 + 0x1f) &0xffe0) * (y_off0& 0xffe0) + ((y_off0& 0x01f)<<5) +
	                              ((x_off0& 0xffe0)<<5);
	        de_scal_ch1_offset = ((((2 * image_w1)<< 0x01)+0x1f)&0xffe0) * ((y_off1) & 0xffe0) +
	                              (((y_off1) & 0x01f)<<5) + (((x_off1<<0x01) & 0xffe0)<<5);
	        de_scal_ch2_offset = 0x0;

			de_scal_ch0r_offset = ((2 *image_w0 + 0x1f) &0xffe0) * (y_off0& 0xffe0) + ((y_off0& 0x01f)<<5) +
	                              (((image_w0 + x_off0) & 0xffe0)<<5);
			de_scal_ch1r_offset = ((((2 * image_w1)<< 0x01)+0x1f)&0xffe0) * ((y_off1) & 0xffe0) +
	                              (((y_off1) & 0x01f)<<5) + ((((image_w1 + x_off1)<<0x01) & 0xffe0)<<5);
			de_scal_ch2r_offset = 0x0;

			scal_dev[sel]->buf_addr0.dwval = addr->ch0_addr+ de_scal_ch0_offset;
			scal_dev[sel]->buf_addr1.dwval = addr->ch1_addr+ de_scal_ch1_offset;
			scal_dev[sel]->buf_addr2.dwval = addr->ch2_addr+ de_scal_ch2_offset;

			scal_dev[sel]->trd_buf_addr0.dwval = addr->ch0_addr + de_scal_ch0r_offset;
			scal_dev[sel]->trd_buf_addr1.dwval = addr->ch1_addr + de_scal_ch1r_offset;
			scal_dev[sel]->trd_buf_addr2.dwval = addr->ch2_addr + de_scal_ch2r_offset;
		}
		else if((trdinmode == DE_SCAL_3DIN_LI))
		{
		    scal_dev[sel]->linestrd0.dwval = ((((image_w0+0x1f)&0xffe0) - 0x1f)<<0x05);
			scal_dev[sel]->linestrd1.dwval = ((((((image_w1)<<1)+0x1f)&0xffe0) - 0x1f) << 0x05);
			scal_dev[sel]->linestrd2.dwval = 0x00;

			//block offset
			de_scal_ch0_dx0 = (x_off0 & 0x1f);
			de_scal_ch0_dy0 = ((2*y_off0) & 0x1f);
			de_scal_ch0_dx1 = ((de_scal_ch0_dx0 & 0x1f) + in_w0 + 0x1f) &0x1f;
			de_scal_ch1_dx0 = (((x_off1)<<1)&0x1f);
			de_scal_ch1_dy0 = ((2*y_off1)&0x1f);
			de_scal_ch1_dx1 = ((de_scal_ch1_dx0 & 0x1e) + ((in_w1)<<1) + 0x1f) & 0x1f;

			scal_dev[sel]->tb_off0.bits.x_offset0 = de_scal_ch0_dx0;
			scal_dev[sel]->tb_off0.bits.y_offset0 = de_scal_ch0_dy0;
			scal_dev[sel]->tb_off0.bits.x_offset1 = de_scal_ch0_dx1;
			scal_dev[sel]->tb_off1.bits.x_offset0 = de_scal_ch1_dx0;
			scal_dev[sel]->tb_off1.bits.y_offset0 = de_scal_ch1_dy0;
			scal_dev[sel]->tb_off1.bits.x_offset1 = de_scal_ch1_dx1;
			scal_dev[sel]->trd_tb_off0.bits.x_offset0 = de_scal_ch0_dx0;
			scal_dev[sel]->trd_tb_off0.bits.y_offset0 = (2*y_off0 + 1) & 0x1f;
			scal_dev[sel]->trd_tb_off0.bits.x_offset1 = de_scal_ch0_dx1;
			scal_dev[sel]->trd_tb_off1.bits.x_offset0 = de_scal_ch1_dx0;
			scal_dev[sel]->trd_tb_off1.bits.y_offset0 = (2*y_off1 + 1) & 0x1f;
			scal_dev[sel]->trd_tb_off1.bits.x_offset1 = de_scal_ch1_dx1;

			de_scal_ch0_offset = ((image_w0 + 0x1f) &0xffe0) * ((2*y_off0) & 0xffe0) + (((2*y_off0) & 0x01f)<<5) +
	                              ((x_off0& 0xffe0)<<5);
	        de_scal_ch1_offset = (((image_w1<< 0x01)+0x1f)&0xffe0) * ((2*y_off1) & 0xffe0) +
	                              (((2*y_off1) & 0x01f)<<5) + (((x_off1<<0x01) & 0xffe0)<<5);
	        de_scal_ch2_offset = 0x0;

			de_scal_ch0r_offset = de_scal_ch0_offset + 32;
			de_scal_ch1r_offset = de_scal_ch1_offset + 32;
			de_scal_ch2r_offset = 0x0;

			scal_dev[sel]->buf_addr0.dwval = addr->ch0_addr+ de_scal_ch0_offset;
			scal_dev[sel]->buf_addr1.dwval = addr->ch1_addr+ de_scal_ch1_offset;
			scal_dev[sel]->buf_addr2.dwval = addr->ch2_addr+ de_scal_ch2_offset;

			scal_dev[sel]->trd_buf_addr0.dwval = addr->ch0_addr + de_scal_ch0r_offset;
			scal_dev[sel]->trd_buf_addr1.dwval = addr->ch1_addr + de_scal_ch1r_offset;
			scal_dev[sel]->trd_buf_addr2.dwval = addr->ch2_addr + de_scal_ch2r_offset;
		}
	}


	scal_dev[sel]->input_fmt.bits.byte_seq = type->byte_seq;
	scal_dev[sel]->input_fmt.bits.data_mod = type->mod;
	scal_dev[sel]->input_fmt.bits.data_fmt = type->fmt;
	scal_dev[sel]->input_fmt.bits.data_ps = type->ps;

	scal_dev[sel]->ch0_insize.bits.in_width = in_w0 - 1;
	scal_dev[sel]->ch0_insize.bits.in_height = in_h0 - 1;
	scal_dev[sel]->ch1_insize.bits.in_width = in_w1 - 1;
	scal_dev[sel]->ch1_insize.bits.in_height = in_h1 - 1;


    return 0;
}

s32 DE_SCAL_EnableINT(u8 sel,u32 irqsrc)
{
	scal_dev[sel]->int_en.dwval |= irqsrc;

	return 0;
}

s32 DE_SCAL_DisableINT(u8 sel, u32 irqsrc)
{
	scal_dev[sel]->int_en.dwval &= (~irqsrc);

	return 0;
}

u32 DE_SCAL_QueryINT(u8 sel)
{
	return scal_dev[sel]->int_status.dwval;
}

//write 1 to clear
u32 DE_SCAL_ClearINT(u8 sel,u32 irqsrc)
{
	scal_dev[sel]->int_status.dwval |= irqsrc; //DE_WB_END_IE;
	return 0;
}


//*********************************************************************************************
// function       : DE_SCAL_Set_Writeback_Addr_ex(u32 sel, __scal_buf_addr_t *addr, __scal_out_size_t *size,
//                         __scal_out_type_t *type)
// description    : write back address set(region writeback)
// parameters     :
//                 sel <scaler select>
//                 addr  <frame buffer address for write back, 32 bit absolute address>
//                 size <scale region define,  framebuffer size, offset, result size>
//                 type <output data type>
//                 dien <deinterlace enable>
// return         :
//               success
//***********************************************************************************************
s32 DE_SCAL_Set_Writeback_Addr_ex(u32 sel, __scal_buf_addr_t *addr, __scal_out_size_t *size,
                         __scal_out_type_t *type)
{
    u8 w_shift, h_shift;
	u32 image_w0, image_w1;
	u32 x_off0, y_off0, x_off1, y_off1;
	u32 in_w0, in_h0, in_w1, in_h1;

	image_w0 = size->fb_width;
	in_w0 = size->width;
	in_h0 = size->height;
	x_off0 = size->x_off;
	y_off0 = size->y_off;  //scan mod enable or deinterlace, odd dy un-support

//    if(sel == 0)   //scaler 0 scaler 1
    {
        if(type->fmt == DE_SCAL_OUTPYUV422 || type->fmt == DE_SCAL_OUTPYUV420)
        {
            w_shift = 1;
        	image_w1 = (image_w0 + 0x1)>>w_shift;
        	in_w1 = (in_w0 + 0x1)>>w_shift;
        	x_off1 = (x_off0)>>w_shift;
        }
        else if(type->fmt == DE_SCAL_OUTPYUV411)
        {
            w_shift = 2;
        	image_w1 = (image_w0 + 0x3)>>w_shift;
        	in_w1 = (in_w0 + 0x3)>>w_shift;
        	x_off1 = (x_off0)>>w_shift;
        }
        else
        {
            w_shift = 0;
        	image_w1 = image_w0;
        	in_w1 = in_w0;
        	x_off1 = x_off0;
        }
        if(type->fmt == DE_SCAL_OUTPYUV420)
        {
            h_shift = 1;
        	in_h1 = (in_h0 + 0x1)>>h_shift;
        	y_off1 = (y_off0)>>h_shift;
        }
        else
        {
            h_shift = 0;
        	in_h1 = in_h0;
        	y_off1 = y_off0;
        }
    }
    //added no-zero limited
    in_h0 = (in_h0!=0) ? in_h0 : 1;
	in_h1 = (in_h1!=0) ? in_h1 : 1;
	in_w0 = (in_w0!=0) ? in_w0 : 1;
	in_w1 = (in_w1!=0) ? in_w1 : 1;

	if((type->fmt == DE_SCAL_OUTPRGB888) || (type->fmt == DE_SCAL_OUTPYUV444) || (type->fmt == DE_SCAL_OUTPYUV420) || (type->fmt == DE_SCAL_OUTPYUV422) || (type->fmt == DE_SCAL_OUTPYUV411 ))
	{
	    scal_dev[sel]->wb_linestrd0.dwval = image_w0;
		scal_dev[sel]->wb_linestrd1.dwval = image_w1;
		scal_dev[sel]->wb_linestrd2.dwval = image_w1;

        de_scal_ch0_offset = image_w0 * y_off0 + x_off0;
        de_scal_ch1_offset = image_w1 * y_off1 + x_off1;
        de_scal_ch2_offset = image_w1 * y_off1 + x_off1;

        addr->ch0_addr = addr->ch0_addr+ de_scal_ch0_offset;
        addr->ch1_addr = addr->ch1_addr+ de_scal_ch1_offset;
        addr->ch2_addr = addr->ch2_addr+ de_scal_ch2_offset;

        DE_SCAL_Set_Writeback_Addr(sel,addr);
	}
	else if((type->fmt == DE_SCAL_OUTI0RGB888) || (type->fmt == DE_SCAL_OUTI1RGB888) )
	{
        scal_dev[sel]->wb_linestrd0.dwval = image_w0<<(0x2 - w_shift);
		scal_dev[sel]->wb_linestrd1.dwval = 0;
		scal_dev[sel]->wb_linestrd2.dwval = 0;

        de_scal_ch0_offset = ((image_w0 * y_off0 + x_off0)<<(0x2 - w_shift));
        de_scal_ch1_offset = 0x0;
        de_scal_ch2_offset = 0x0;

        addr->ch0_addr = addr->ch0_addr+ de_scal_ch0_offset;
        //addr->ch1_addr = addr->ch1_addr;
        //addr->ch2_addr = addr->ch2_addr;

        DE_SCAL_Set_Writeback_Addr(sel,addr);
	}

    return 0;
}

// for copy bootlogo to fb0
u32 DE_SCAL_Get_Fb_Addr0(u8 sel)
{
    printk("sel=%d,addr=%d\n", sel, scal_dev[sel]->buf_addr0.dwval);
    return scal_dev[sel]->buf_addr0.dwval;
}

u32 DE_SCAL_Set_Fb_Addr0(u8 sel, u32 *addr)
{
    scal_dev[sel]->buf_addr0.dwval = (u32)addr;
    return 0;
}

