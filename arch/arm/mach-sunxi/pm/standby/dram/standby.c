/********************************************************************************
 * Allwinner Technology, All Right Reserved. 2006-2010 Copyright (c)
 *
 * File: standby.c
 *
 * Description:  This file implements standby for AW1681 DRAM controller
 *
 * History:
 *
 * date		author		version		note
 *
 * 20140926	liuke		v0.1		initial
 *
 *
 *
 *
 ********************************************************************************/
#define STANDBY_TEST

#ifdef STANDBY_TEST
#include "dram.h"

//default config
__u32 dram_crc_en  = 0;
__u32 dram_crc_start     = 0x40000000;
__u32 dram_crc_len     = (1024 * 1024);

__s32 standby_set_dram_crc_paras(__u32 en, __u32 start, __u32 len)
{
    dram_crc_en = en;
    dram_crc_start    = start;
    dram_crc_len    = len;

    return 0;
}

__u32 standby_dram_crc(void)
{
    __u32 *pdata = (__u32 *)(dram_crc_start);
    __u32 crc = 0;

    printk("crc begin...\n");
    if(0 != dram_crc_en){
	printk("start:%x len:%x\n", dram_crc_start, dram_crc_len);
	while (pdata < (__u32 *)(dram_crc_start + dram_crc_len))
	{
	    crc += *pdata;
	    pdata++;
	}
    }
    printk("crc finish...\n");
    return crc;
}

unsigned int dram_power_save_process()
{
	unsigned int reg_val =0;
	unsigned int i,n = 2;

	/* disable all master access  */
	mctl_write_w(0,MC_MAER);//close all master

	/* DRAM power down. */
	//1.enter self refresh
	reg_val = mctl_read_w(PWRCTL);
	reg_val |= 0x1<<0;
	reg_val |= (0x1<<8);
	mctl_write_w(reg_val,PWRCTL);
	//confirm dram controller has enter selfrefresh
	while(((mctl_read_w(STATR)&0x7) != 0x3));

	/* 2.disable CK and power down pad include AC/DX/ pad,
	 * ZQ calibration module power down
	 */

	for(i=0; i<n; i++)	//DXIO POWER DOWN
	{
		reg_val = mctl_read_w(DXnGCR0(i));
		reg_val &= ~(0x3U<<22);
		reg_val |= (0x1U<<22);		//DQS receiver off
		reg_val &= ~(0xfU<<12);
		reg_val |= (0x5U<<12);		//POWER DOWN RECEIVER/DRICER OFF
		reg_val |= (0x2U<<2);		//OE mode disable
		reg_val &= ~(0x1U<<0);		//DQ GROUP OFF
		mctl_write_w(reg_val,DXnGCR0(i));
	}
	reg_val = mctl_read_w(CAIOCR0);		//CA IO POWER DOWN
	reg_val |= (0x3U<<8);		//CKE ENABLE
	reg_val &= ~(0x3U<<6);		//CK OUTPUT DISABLE
//	reg_val &= ~(0x1U<<3);		//CA OE OFF	//do not close for 1681/1680
	reg_val |= (0x3U<<0);		//CA POWER DOWN RECEIVER/DRIVER ON
	mctl_write_w(reg_val,CAIOCR0);

	/* 4.disable global clk and pll-ddr0/pll-ddr1 */
	mctl_write_w(0x0,MC_CLKEN);
	//disable PLL_DDR1
	reg_val =mctl_read_w(_CCM_PLL_DDR1_REG);
	reg_val &=~(0x1U<<31);
	mctl_write_w(reg_val,_CCM_PLL_DDR1_REG);

	/* close DRAM AHB gate */
	reg_val =mctl_read_w(BUS_CLK_GATE_REG0);
	reg_val &=~(0x1U<<14);
	mctl_write_w(reg_val,BUS_CLK_GATE_REG0);

	/* close MBUS CLK gate */
	reg_val = mctl_read_w(MBUS_CLK_CTL_REG);
	reg_val &=~(1U<<31);
	mctl_write_w(reg_val, MBUS_CLK_CTL_REG);

	return DRAM_RET_OK;
}

unsigned int dram_power_up_process(__dram_para_t *para)
{
	unsigned int reg_val =0;
	unsigned int i,n = 2;

	/* Open MBUS CLK gate */
	reg_val = mctl_read_w(MBUS_CLK_CTL_REG);
	reg_val |= (1U<<31);
	mctl_write_w(reg_val, MBUS_CLK_CTL_REG);

	/* Open DRAM AHB gate */
	reg_val =mctl_read_w(BUS_CLK_GATE_REG0);
	reg_val |=(0x1U<<14);
	mctl_write_w(reg_val,BUS_CLK_GATE_REG0);

	//enable PLL_DDR1,and wait it stable
	reg_val =mctl_read_w(_CCM_PLL_DDR1_REG);
	reg_val |=(0x1U<<31);
	mctl_write_w(reg_val,_CCM_PLL_DDR1_REG);
	while(mctl_read_w(_CCM_PLL_DDR1_REG) == 1);

	//enable global clk
	mctl_write_w(0xC00e,MC_CLKEN);

	//controller enter self refresh
	reg_val = mctl_read_w(PWRCTL);
	reg_val |= 0x1<<0;
	reg_val |= (0x1<<8);
	mctl_write_w(reg_val,PWRCTL);
	//confirm dram controller has enter selfrefresh
	while(((mctl_read_w(STATR)&0x7) != 0x3));

	//CA Power down dynamic
	reg_val = mctl_read_w(CAIOCR0);
	reg_val |= (0x3U<<8);		//CKE ENABLE
	reg_val |= (0x1U<<6);		//CK OUTPUT enable
//	reg_val |= (0x1U<<3);		//CA OE on //according to standby enter status
	reg_val &= ~(0x3U<<0);		//CA POWER DOWN RECEIVER/DRIVER off
	mctl_write_w(reg_val,CAIOCR0);

	for(i=0; i<n; i++)	//DXIO POWER DOWN dynamic
	{
		reg_val = mctl_read_w(DXnGCR0(i));
		reg_val &= ~(0x3U<<22);		//DQS receiver dynamic
		reg_val &= ~(0xfU<<12);		//POWER DOWN RECEIVER/DRICER OFF dynamic
		reg_val &= ~(0x3U<<2);		//OE mode dynamic
		reg_val |= (0x1U<<0);		//DQ GROUP ON
		mctl_write_w(reg_val,DXnGCR0(i));
	}

	reg_val = mctl_read_w(PWRCTL);
	reg_val &= ~(0x1<<0);
	reg_val &= ~(0x1<<8);
	mctl_write_w( reg_val,PWRCTL);
	//confirm dram controller has exit from selfrefresh
	while(((mctl_read_w(STATR)&0x7) != 0x1));

	/* enable all master access  */
	mctl_write_w(0xffffffff,MC_MAER);

	return DRAM_RET_OK;
}
#endif
