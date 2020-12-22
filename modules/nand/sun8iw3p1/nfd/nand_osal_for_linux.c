/*
*********************************************************************************************************
*											        eBIOS
*						            the Easy Portable/Player Develop Kits
*									           dma sub system
*
*						        (c) Copyright 2006-2008, David China
*											All	Rights Reserved
*
* File    : clk_for_nand.c
* By      : Richard
* Version : V1.00
*********************************************************************************************************
*/
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/list.h>
#include <linux/fs.h>
#include <linux/blkdev.h>
#include <linux/blkpg.h>
#include <linux/spinlock.h>
#include <linux/hdreg.h>
#include <linux/init.h>
#include <linux/semaphore.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/interrupt.h>
#include <asm/uaccess.h>
#include <linux/timer.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/mutex.h>
//#include <mach/clock.h>
#include <mach/platform.h> 
//#include <mach/hardware.h> 
#include <mach/sys_config.h>
#include <linux/dma-mapping.h>
//#include <mach/dma.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <asm/cacheflush.h>
#include <mach/gpio.h>
#include <linux/pinctrl/consumer.h>
#include <linux/pinctrl/pinconf-sunxi.h>
#include <linux/gpio.h>
//#include "nand_lib.h"
#include "nand_blk.h"

#ifdef CONFIG_DMA_ENGINE
#include <linux/dmaengine.h>
#include <linux/dma-mapping.h>
#include <linux/dma/sunxi-dma.h>
#endif

#define  NAND_DRV_VERSION_0		0x2
#define  NAND_DRV_VERSION_1		0x25
#define  NAND_DRV_DATE			0x20150409
#define  NAND_DRV_TIME			0x1122

struct clk *pll6;
struct clk *nand0_clk;
struct clk *ahb_nand0;


int seq=0;
int nand_handle=0;

struct dma_chan *dma_hdl;	

#ifdef __LINUX_NAND_SUPPORT_INT__
static int nandrb_ready_flag[2] = {1, 1};
static int nanddma_ready_flag[2] = {1, 1};
static DECLARE_WAIT_QUEUE_HEAD(NAND_RB_WAIT_CH0);
static DECLARE_WAIT_QUEUE_HEAD(NAND_RB_WAIT_CH1);

static DECLARE_WAIT_QUEUE_HEAD(NAND_DMA_WAIT_CH0);
static DECLARE_WAIT_QUEUE_HEAD(NAND_DMA_WAIT_CH1);

#endif

//#define RB_INT_MSG_ON
#ifdef  RB_INT_MSG_ON
#define dbg_rbint(fmt, args...) printk(fmt, ## args)
#else
#define dbg_rbint(fmt, ...)  ({})
#endif

#define RB_INT_WRN_ON
#ifdef  RB_INT_WRN_ON
#define dbg_rbint_wrn(fmt, args...) printk(fmt, ## args)
#else
#define dbg_rbint_wrn(fmt, ...)  ({})
#endif

//#define DMA_INT_MSG_ON
#ifdef  DMA_INT_MSG_ON
#define dbg_dmaint(fmt, args...) printk(fmt, ## args)
#else
#define dbg_dmaint(fmt, ...)  ({})
#endif

#define DMA_INT_WRN_ON
#ifdef  DMA_INT_WRN_ON
#define dbg_dmaint_wrn(fmt, args...) printk(fmt, ## args)
#else
#define dbg_dmaint_wrn(fmt, ...)  ({})
#endif

//for rb int
extern void NFC_RbIntEnable(void);
extern void NFC_RbIntDisable(void);
extern void NFC_RbIntClearStatus(void);
extern __u32 NFC_RbIntGetStatus(void);
extern __u32 NFC_GetRbSelect(void);
extern __u32 NFC_GetRbStatus(__u32 rb);
extern __u32 NFC_RbIntOccur(void);

extern void NFC_DmaIntEnable(void);
extern void NFC_DmaIntDisable(void);
extern void NFC_DmaIntClearStatus(void);
extern __u32 NFC_DmaIntGetStatus(void);
extern __u32 NFC_DmaIntOccur(void);

extern __u32 NAND_GetCurrentCH(void);
extern __u32 NAND_SetCurrentCH(__u32 nand_index);

extern void * NAND_Malloc(unsigned int Size);
extern void NAND_Free(void *pAddr, unsigned int Size);
extern __u32 NAND_GetNdfcVersion(void);
/*
*********************************************************************************************************
*                                               DMA TRANSFER END ISR
*
* Description: dma transfer end isr.
*
* Arguments  : none;
*
* Returns    : EPDK_TRUE/ EPDK_FALSE
*********************************************************************************************************
*/
int NAND_ClkRequest(__u32 nand_index)
{
	long rate;

	pll6 = clk_get(NULL, "pll6");
	if(NULL == pll6 || IS_ERR(pll6)) {
		printk("%s: pll6 clock handle invalid!\n", __func__);
		return -1;
	}

	rate = clk_get_rate(pll6);
	printk("%s: get pll6 rate %dHZ\n", __func__, (__u32)rate);

	if (nand_index == 0) {

		nand0_clk = clk_get(NULL, "nand");
	
		if(NULL == nand0_clk || IS_ERR(nand0_clk)){
			printk("%s: nand0 clock handle invalid!\n", __func__);
			return -1;
		}

		if(clk_set_parent(nand0_clk, pll6))
			printk("%s: set nand0_clk parent to pll6 failed!\n", __func__);

		rate = clk_round_rate(nand0_clk, 20000000);
		if(clk_set_rate(nand0_clk, rate))
			printk("%s: set nand0_clk rate to %dHZ failed!\n", __func__, (__u32)rate);

		if(clk_prepare_enable(nand0_clk))
			printk("%s: enable nand0_clk failed!\n", __func__);
	} else {
		printk("NAND_ClkRequest, nand_index error: 0x%x\n", nand_index);
		return -1;
	}

	return 0;	
}

void NAND_ClkRelease(__u32 nand_index)
{
	if (nand_index == 0) {
		if (NULL != nand0_clk && !IS_ERR(nand0_clk)) {

			clk_disable_unprepare(nand0_clk);

			clk_put(nand0_clk);
			nand0_clk = NULL;
		}
	} else {
		printk("NAND_ClkRequest, nand_index error: 0x%x\n", nand_index);
	}

	if(NULL != pll6 && !IS_ERR(pll6)) {
		clk_put(pll6);
		pll6 = NULL;
	}	
	
}

int NAND_SetClk(__u32 nand_index, __u32 nand_clk0, __u32 nand_clk1)
{
	long    rate;

	if (nand_index == 0) {
		
		if (NULL == nand0_clk || IS_ERR(nand0_clk)) {
			printk("%s: clock handle invalid!\n", __func__);
			return -1;
		}

		rate = clk_round_rate(nand0_clk, nand_clk0*2000000);
		if(clk_set_rate(nand0_clk, rate))
			printk("%s: set nand0_clk rate to %dHZ failed! nand_clk: 0x%x\n", __func__, (__u32)rate, nand_clk0);	
	} else {
		printk("NAND_SetClk, nand_index error: 0x%x\n", nand_index);
		return -1;
	}
	
	return 0;
}

int NAND_GetClk(__u32 nand_index, __u32 *pnand_clk0, __u32 *pnand_clk1)
{
	long rate;

	if(nand_index == 0) {
		if(NULL == nand0_clk || IS_ERR(nand0_clk)){
			printk("%s: clock handle invalid!\n", __func__);
			return -1;
		}
		rate = clk_get_rate(nand0_clk);
	} else {
		printk("NAND_GetClk, nand_index error: 0x%x\n", nand_index);
		return -1;
	}

	*pnand_clk0 = (rate/2000000);
	*pnand_clk1 = 0;

	return 0;
}

void eLIBs_CleanFlushDCacheRegion_nand(void *adr, size_t bytes)
{
	__cpuc_flush_dcache_area(adr, bytes + (1 << 5) * 2 - 2);
}


__s32 NAND_CleanFlushDCacheRegion(__u32 buff_addr, __u32 len)
{
	//eLIBs_CleanFlushDCacheRegion_nand((void *)buff_addr, (size_t)len);
    return 0;
}


//__u32 tmp_dma_phy_addr[2] = {0, 0};
//__u32 tmp_dma_len[2] = {0, 0};

__u32 NAND_DMASingleMap(__u32 rw, __u32 buff_addr, __u32 len)
{
    __u32 mem_addr;
//    __u32 nand_index = NAND_GetCurrentCH();
    
    if (rw == 1) 
    {
	    mem_addr = (__u32)dma_map_single(NULL, (void *)buff_addr, len, DMA_TO_DEVICE);
	} 
	else 
    {
	    mem_addr = (__u32)dma_map_single(NULL, (void *)buff_addr, len, DMA_FROM_DEVICE);
	}

//	tmp_dma_phy_addr[nand_index] = mem_addr;
//	tmp_dma_len[nand_index] = len;
	
	return mem_addr;
}

__u32 NAND_DMASingleUnmap(__u32 rw, __u32 buff_addr, __u32 len)
{
	__u32 mem_addr = buff_addr;
//	__u32 nand_index = NAND_GetCurrentCH();	

//    if(tmp_dma_phy_addr[nand_index]!=mem_addr)
//    {
//    	printk("NAND_DMASingleUnmap, dma addr not match! nand_index: 0x%x, tmp_dma_phy_addr:0x%x, mem_addr: 0x%x\n", nand_index,tmp_dma_phy_addr[nand_index], mem_addr);
//	mem_addr = tmp_dma_phy_addr[nand_index];
//    }
//
//    if(tmp_dma_len[nand_index] != len)
//    {
//	    printk("NAND_DMASingleUnmap, dma len not match! nand_index: 0x%x, tmp_dma_len:0x%x, len: 0x%x\n", nand_index,tmp_dma_len[nand_index], len);
//	    len = tmp_dma_len[nand_index];
//	}
    	

	if (rw == 1) 
	{
	    dma_unmap_single(NULL, (dma_addr_t)mem_addr, len, DMA_TO_DEVICE);
	} 
	else 
	{
	    dma_unmap_single(NULL, (dma_addr_t)mem_addr, len, DMA_FROM_DEVICE);
	}
	
	return mem_addr;
}

__s32 NAND_AllocMemoryForDMADescs(__u32 *cpu_addr, __u32 *phy_addr)
{
	void *p = NULL;
	__u32 physical_addr  = 0;

#if 1

	//void *dma_alloc_coherent(struct device *dev, size_t size, dma_addr_t *handle, gfp_t gfp);
	p = (void *)dma_alloc_coherent(NULL, PAGE_SIZE,
				(dma_addr_t *)&physical_addr, GFP_KERNEL);

	if (p == NULL) {
		printk("NAND_AllocMemoryForDMADescs(): alloc dma des failed\n");
		return -1;
	} else {
		*cpu_addr = (__u32)p;
		*phy_addr = physical_addr;
		printk("NAND_AllocMemoryForDMADescs(): cpu: 0x%x    physic: 0x%x\n",
			*cpu_addr, *phy_addr);
	}

#else

	p = (void *)NAND_Malloc(1024);

	if (p == NULL) {
		printk("NAND_AllocMemoryForDMADescs(): alloc dma des failed\n");
		return -1;
	} else {
		*cpu_addr = (__u32)p;
		*phy_addr = (__u32)p;
		printk("NAND_AllocMemoryForDMADescs(): cpu: 0x%x    physic: 0x%x\n",
			*cpu_addr, *phy_addr);
	}

#endif

	return 0;
}

__s32 NAND_FreeMemoryForDMADescs(__u32 *cpu_addr, __u32 *phy_addr)
{
#if 1

	//void dma_free_coherent(struct device *dev, size_t size, void *cpu_addr, dma_addr_t handle);
	//dma_free_coherent(NULL, PAGE_SIZE, (void *)(*cpu_addr), *phy_addr);

#else

	NAND_Free((void *)(*cpu_addr), 1024);

#endif

	*cpu_addr = 0;
	*phy_addr = 0;

	return 0;
}


#ifdef __LINUX_SUPPORT_DMA_INT__
void NAND_EnDMAInt(void)
{
	__u32 nand_index;

	nand_index = NAND_GetCurrentCH();
	if(nand_index >1)
		printk("NAND_ClearDMAInt, nand_index error: 0x%x\n", nand_index);
	
	//clear interrupt
	#if 0
	NFC_DmaIntClearStatus();
	if(NFC_DmaIntGetStatus())
	{
		dbg_rbint_wrn("nand clear dma int status error in int enable \n");
		dbg_rbint_wrn("dma status: 0x%x\n", NFC_DmaIntGetStatus());
	}
	#endif
	nanddma_ready_flag[nand_index] = 0;

	//enable interrupt
	NFC_DmaIntEnable();

	dbg_rbint("dma int en\n");
}

void NAND_ClearDMAInt(void)
{
	__u32 nand_index;

	nand_index = NAND_GetCurrentCH();
	if(nand_index >1)
		printk("NAND_ClearDMAInt, nand_index error: 0x%x\n", nand_index);
    
	//disable interrupt
	NFC_DmaIntDisable();
	dbg_dmaint("ch %d dma int clear\n", nand_index);

	//clear interrupt
	//NFC_DmaIntClearStatus();
	//if(NFC_DmaIntGetStatus())
	//{
	//	dbg_dmaint_wrn("nand clear dma int status error in int clear \n");
	//	dbg_dmaint_wrn("dma status: 0x%x\n", NFC_DmaIntGetStatus());
	//}
	
	nanddma_ready_flag[nand_index] = 0;
}

void NAND_DMAInterrupt(void)
{
	__u32 nand_index;

	nand_index = NAND_GetCurrentCH();
	if(nand_index >1)
		printk("NAND_ClearDMAInt, nand_index error: 0x%x\n", nand_index);
	
	dbg_dmaint("ch %d dma int occor! \n", nand_index);
	if(!NFC_DmaIntGetStatus())
	{
		dbg_dmaint_wrn("nand dma int late \n");
	}
    
    NAND_ClearDMAInt();
    
    nanddma_ready_flag[nand_index] = 1;
    if(nand_index == 0)
	wake_up( &NAND_DMA_WAIT_CH0 );
    else if(nand_index == 1)
	wake_up( &NAND_DMA_WAIT_CH1 );    
}

__s32 NAND_WaitDmaFinish(void)
{
	__u32 nand_index;

	nand_index = NAND_GetCurrentCH();
	if(nand_index >1)
		printk("NAND_ClearDMAInt, nand_index error: 0x%x\n", nand_index);

	NAND_EnDMAInt();
	
	//wait_event(NAND_RB_WAIT, nandrb_ready_flag);
	dbg_dmaint("ch %d dma wait\n", nand_index);

	if(nanddma_ready_flag[nand_index])
	{
		dbg_dmaint("ch %d fast dma int\n", nand_index);
		NAND_ClearDMAInt();
		return 0;
	}

	if(NFC_DmaIntGetStatus())
	{
		dbg_dmaint("ch %d dma fast ready \n", nand_index);
		NAND_ClearDMAInt();
		return 0;
	}

	if(nand_index == 0)
	{
		if(wait_event_timeout(NAND_DMA_WAIT_CH0, nanddma_ready_flag[nand_index], 1*HZ)==0)
		{
			dbg_dmaint_wrn("+nand wait dma int time out, ch: 0x%d, dma_status: %x, dma_ready_flag: 0x%x\n", nand_index, NFC_DmaIntGetStatus(),nanddma_ready_flag[nand_index]);
			while(!NFC_DmaIntGetStatus());
			dbg_dmaint_wrn("-nand wait dma int time out, ch: 0x%d, dma_status: %x, dma_ready_flag: 0x%x\n", nand_index, NFC_DmaIntGetStatus(),nanddma_ready_flag[nand_index]);
			NAND_ClearDMAInt();
		}
		else
		{
			dbg_dmaint("nand wait dma ready ok\n");
			NAND_ClearDMAInt();
		}
	}
	else if(nand_index ==1)
	{
		if(wait_event_timeout(NAND_DMA_WAIT_CH1, nanddma_ready_flag[nand_index], 1*HZ)==0)
		{
			dbg_dmaint_wrn("+nand wait dma int time out, ch: 0x%d, dma_status: %x, dma_ready_flag: 0x%x\n", nand_index, NFC_DmaIntGetStatus(),nanddma_ready_flag[nand_index]);
			while(!NFC_DmaIntGetStatus());	
			dbg_dmaint_wrn("-nand wait dma int time out, ch: 0x%d, dma_status: %x, dma_ready_flag: 0x%x\n", nand_index, NFC_DmaIntGetStatus(),nanddma_ready_flag[nand_index]);
			NAND_ClearDMAInt();
		}
		else
		{
			dbg_dmaint("nand wait dma ready ok\n");
			NAND_ClearDMAInt();
		}
	}
	else
	{
		NAND_ClearDMAInt();
		printk("NAND_WaitDmaFinish, error nand_index: 0x%x\n", nand_index);
	}
    	
    return 0;
}

#else
__s32 NAND_WaitDmaFinish(void)
{
	
	#if 0
	__u32 rw;
	__u32 buff_addr;
	__u32 len;
	
	wait_event(DMA_wait, nanddma_completed_flag);

	if(rw_flag==(__u32)NAND_READ)
		rw = 0;
	else
		rw = 1;
	buff_addr = dma_phy_address;
	len = dma_len_temp;
	NAND_DMASingleUnmap(rw, buff_addr, len);

	rw_flag = 0x1234;
	#endif 
	
    return 0;
}

#endif


#ifdef __LINUX_SUPPORT_RB_INT__
void NAND_EnRbInt(void)
{
	__u32 nand_index;

	nand_index = NAND_GetCurrentCH();
	if(nand_index >1)
		printk("NAND_ClearDMAInt, nand_index error: 0x%x\n", nand_index);
	
	//clear interrupt
	NFC_RbIntClearStatus();
	
	nandrb_ready_flag[nand_index] = 0;

	//enable interrupt
	NFC_RbIntEnable();

	dbg_rbint("rb int en\n");
}


void NAND_ClearRbInt(void)
{
	__u32 nand_index;

	nand_index = NAND_GetCurrentCH();
	if(nand_index >1)
		printk("NAND_ClearDMAInt, nand_index error: 0x%x\n", nand_index);
	
	//disable interrupt
	NFC_RbIntDisable();;

	dbg_rbint("rb int clear\n");

	//clear interrupt
	NFC_RbIntClearStatus();
	
	//check rb int status
	if(NFC_RbIntGetStatus())
	{
		dbg_rbint_wrn("nand %d clear rb int status error in int clear \n", nand_index);
	}
	
	nandrb_ready_flag[nand_index] = 0;
}


void NAND_RbInterrupt(void)
{
	__u32 nand_index;

	nand_index = NAND_GetCurrentCH();
	if(nand_index >1)
		printk("NAND_ClearDMAInt, nand_index error: 0x%x\n", nand_index);

	dbg_rbint("rb int occor! \n");
	if(!NFC_RbIntGetStatus())
	{
		dbg_rbint_wrn("nand rb int late \n");
	}
    
    NAND_ClearRbInt();
    
    nandrb_ready_flag[nand_index] = 1;
    if(nand_index == 0)
		wake_up( &NAND_RB_WAIT_CH0 );
    else if(nand_index ==1)
    	wake_up( &NAND_RB_WAIT_CH1 );

}

__s32 NAND_WaitRbReady(void)
{
	__u32 rb;
	__u32 nand_index;
	
	dbg_rbint("NAND_WaitRbReady... \n");
	
	nand_index = NAND_GetCurrentCH();
	if(nand_index >1)
		printk("NAND_ClearDMAInt, nand_index error: 0x%x\n", nand_index);

	
	NAND_EnRbInt();
	
	//wait_event(NAND_RB_WAIT, nandrb_ready_flag);
	dbg_rbint("rb wait \n");

	if(nandrb_ready_flag[nand_index])
	{
		dbg_rbint("fast rb int\n");
		NAND_ClearRbInt();
		return 0;
	}

	rb=  NFC_GetRbSelect();
	if(NFC_GetRbStatus(rb))
	{
		dbg_rbint("rb %u fast ready \n", rb);
		NAND_ClearRbInt();
		return 0;
	}

	//printk("NAND_WaitRbReady, ch %d\n", nand_index);

	if(nand_index == 0)
	{
		if(wait_event_timeout(NAND_RB_WAIT_CH0, nandrb_ready_flag[nand_index], 1*HZ)==0)
		{
			dbg_rbint_wrn("nand wait rb int time out, ch: %d\n", nand_index);
			NAND_ClearRbInt();
		}
		else
		{	NAND_ClearRbInt();
			dbg_rbint("nand wait rb ready ok\n");
		}
	}
	else if(nand_index ==1)
	{
		if(wait_event_timeout(NAND_RB_WAIT_CH1, nandrb_ready_flag[nand_index], 1*HZ)==0)
		{
			dbg_rbint_wrn("nand wait rb int time out, ch: %d\n", nand_index);
			NAND_ClearRbInt();
		}
		else
		{	NAND_ClearRbInt();
			dbg_rbint("nand wait rb ready ok\n");
		}
	}
	else
	{
		NAND_ClearRbInt();
	}
		
    return 0;
}
#else
__s32 NAND_WaitRbReady(void)
{
    return 0;
}
#endif

#define NAND_CH0_INT_EN_REG    (0xf1c03000+0x8)
#define NAND_CH1_INT_EN_REG    (0xf1c05000+0x8)

#define NAND_CH0_INT_ST_REG    (0xf1c03000+0x4)
#define NAND_CH1_INT_ST_REG    (0xf1c05000+0x4)

#define NAND_RB_INT_BITMAP     (0x1)
#define NAND_DMA_INT_BITMAP    (0x4)
#define __NAND_REG(x)    (*(volatile unsigned int   *)(x))


void NAND_Interrupt(__u32 nand_index)
{
	if(nand_index >1)
		printk("NAND_Interrupt, nand_index error: 0x%x\n", nand_index);
#ifdef __LINUX_NAND_SUPPORT_INT__   

    //printk("nand interrupt!\n");
#ifdef __LINUX_SUPPORT_RB_INT__    
#if 0

    if(NFC_RbIntOccur())
    {
        dbg_rbint("nand rb int\n");
        NAND_RbInterrupt();
    }
#else
    if(nand_index == 0)
    {
    	if((__NAND_REG(NAND_CH0_INT_EN_REG)&NAND_RB_INT_BITMAP)&&(__NAND_REG(NAND_CH0_INT_ST_REG)&NAND_RB_INT_BITMAP))
		NAND_RbInterrupt();	
    }
    else if(nand_index == 1)
    {
    	if((__NAND_REG(NAND_CH1_INT_EN_REG)&NAND_RB_INT_BITMAP)&&(__NAND_REG(NAND_CH1_INT_ST_REG)&NAND_RB_INT_BITMAP))
		NAND_RbInterrupt();	
    }	
#endif
#endif    

#ifdef __LINUX_SUPPORT_DMA_INT__  
#if 0
    if(NFC_DmaIntOccur())
    {
        dbg_dmaint("nand dma int\n");
        NAND_DMAInterrupt();    
    }
#else
    if(nand_index == 0)
    {
    	if((__NAND_REG(NAND_CH0_INT_EN_REG)&NAND_DMA_INT_BITMAP)&&(__NAND_REG(NAND_CH0_INT_ST_REG)&NAND_DMA_INT_BITMAP))
		NAND_DMAInterrupt();	
    }
    else if(nand_index == 1)
    {
    	if((__NAND_REG(NAND_CH1_INT_EN_REG)&NAND_DMA_INT_BITMAP)&&(__NAND_REG(NAND_CH1_INT_ST_REG)&NAND_DMA_INT_BITMAP))
		NAND_DMAInterrupt();	
    }
	
#endif

#endif

#endif
}


__u32 NAND_VA_TO_PA(__u32 buff_addr)
{
    return (__u32)(__pa((void *)buff_addr));
}

void NAND_PIORequest(__u32 nand_index)
{

	script_item_u  *pin_list;
	int 		   pin_count;
	int 		   pin_index;


	/* get pin sys_config info */
	if(nand_index == 0)
	{
		pin_count = script_get_pio_list("nand0_para", &pin_list);
		printk("pin count:%d \n",pin_count);
	}
	else if(nand_index == 1)
		pin_count = script_get_pio_list("nand1_para", &pin_list);
	else
		return ;
	if (pin_count == 0) {
		/* "lcd0" have no pin configuration */
		printk("pin count 0\n");
		return ;
	}
#if 0
	/* get pin sys_config info */
	if(nand_index == 0)
		pin_count = script_get_pio_list("nand0", &pin_list);
	else if(nand_index == 0)
		pin_count = script_get_pio_list("nand1", &pin_list);
	else
		return ;
	if (pin_count == 0) {
		/* "lcd0" have no pin configuration */
		printk("pin count 0\n");
		return ;
	}
#endif
	/* request pin individually */
	for (pin_index = 0; pin_index < pin_count; pin_index++) {
		struct gpio_config *pin_cfg = &(pin_list[pin_index].gpio);
		char			   pin_name[SUNXI_PIN_NAME_MAX_LEN];
		unsigned long	   config;
		
		/* valid pin of sunxi-pinctrl, 
		 * config pin attributes individually.
		 */
		sunxi_gpio_to_name(pin_cfg->gpio, pin_name);
		config = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_FUNC, pin_cfg->mul_sel);
		pin_config_set(SUNXI_PINCTRL, pin_name, config);
		if (pin_cfg->pull != GPIO_PULL_DEFAULT) {
			config = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_PUD, pin_cfg->pull);
			pin_config_set(SUNXI_PINCTRL, pin_name, config);
		}
		if (pin_cfg->drv_level != GPIO_DRVLVL_DEFAULT) {
			config = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_DRV, pin_cfg->drv_level);
			pin_config_set(SUNXI_PINCTRL, pin_name, config);
		}
		if (pin_cfg->data != GPIO_DATA_DEFAULT) {
			config = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_DAT, pin_cfg->data);
			pin_config_set(SUNXI_PINCTRL, pin_name, config);
		}
	}
	
	return ;
}

__s32 NAND_PIOFuncChange_DQSc(__u32 nand_index, __u32 en)
{
	unsigned int 	ndfc_version;
	script_item_u  *pin_list;
	int 		   pin_count;
	int 		   pin_index;

	ndfc_version = NAND_GetNdfcVersion();
	if (ndfc_version == 1) {
		printk("NAND_PIOFuncChange_EnDQScREc: invalid ndfc version!\n");
		return 0;
	}

	/* get pin sys_config info */
	if(nand_index == 0)
		pin_count = script_get_pio_list("nand0", &pin_list);
	else if(nand_index == 1)
		pin_count = script_get_pio_list("nand1", &pin_list);
	else {
		pin_count = 0;
		printk("NAND_PIOFuncChange_DQSc, wrong nand index %d\n", nand_index);
	}

	if (pin_count == 0) {
		/* "lcd0" have no pin configuration */
		return 0;
	}

	{
		struct gpio_config *pin_cfg;
		char			   pin_name[SUNXI_PIN_NAME_MAX_LEN];
		unsigned long	   config;

		/* change pin func from CE3 to DQSc */
		pin_index = 18;
		if (pin_index > pin_count) {
			printk("NAND_PIOFuncChange_EnDQSc: pin_index error, %d/%d\n", pin_index, pin_count);
			return -1;
		}
		pin_cfg = &(pin_list[pin_index].gpio);

		sunxi_gpio_to_name(pin_cfg->gpio, pin_name);
		config = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_FUNC, pin_cfg->mul_sel);

		if (en) {
			if ((config & 0xffff) == 0x3)
				printk("DQSc has already been enabled!\n");
			else if ((config & 0xffff) == 0x2){
				config &= ~(0xffff);
				config |= 0x3;
				pin_config_set(SUNXI_PINCTRL, pin_name, config);
			} else {
				printk("NAND_PIOFuncChange_EnDQSc: wrong pin func status: %d %d\n", pin_index, (__u32)(config & 0xffff));
			}
		} else {
			if ((config & 0xffff) == 0x2)
				printk("DQSc has already been disenabled!\n");
			else if ((config & 0xffff) == 0x3){
				config &= ~(0xffff);
				config |= 0x3;
				pin_config_set(SUNXI_PINCTRL, pin_name, config);
			} else {
				printk("NAND_PIOFuncChange_EnDQSc: wrong pin func status: %d %d\n", pin_index, (__u32)(config & 0xffff));
			}
		}
	}

	return 0;
}

__s32 NAND_PIOFuncChange_REc(__u32 nand_index, __u32 en)
{
	unsigned int 	ndfc_version;
	script_item_u  *pin_list;
	int 		   pin_count;
	int 		   pin_index;

	ndfc_version = NAND_GetNdfcVersion();
	if (ndfc_version == 1) {
		printk("NAND_PIOFuncChange_EnDQScREc: invalid ndfc version!\n");
		return 0;
	}

	/* get pin sys_config info */
	if(nand_index == 0)
		pin_count = script_get_pio_list("nand0", &pin_list);
	else if(nand_index == 1)
		pin_count = script_get_pio_list("nand1", &pin_list);
	else {
		pin_count = 0;
		printk("NAND_PIOFuncChange_DQSc, wrong nand index %d\n", nand_index);
	}

	if (pin_count == 0) {
		/* "lcd0" have no pin configuration */
		return 0;
	}

	{
		struct gpio_config *pin_cfg;
		char			   pin_name[SUNXI_PIN_NAME_MAX_LEN];
		unsigned long	   config;

		/* change pin func from CE2 to REc */
		pin_index = 17;
		if (pin_index > pin_count) {
			printk("NAND_PIOFuncChange_EnREc: pin_index error, %d/%d\n", pin_index, pin_count);
			return -1;
		}
		pin_cfg = &(pin_list[pin_index].gpio);

		sunxi_gpio_to_name(pin_cfg->gpio, pin_name);
		config = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_FUNC, pin_cfg->mul_sel);

		if (en) {
			if ((config & 0xffff) == 0x3)
				printk("REc has already been enabled!\n");
			else if ((config & 0xffff) == 0x2){
				config &= ~(0xffff);
				config |= 0x3;
				pin_config_set(SUNXI_PINCTRL, pin_name, config);
			} else {
				printk("NAND_PIOFuncChange_EnREc: wrong pin func status: %d %d\n", pin_index, (__u32)(config & 0xffff));
			}
		} else {
			if ((config & 0xffff) == 0x2)
				printk("REc has already been disenabled!\n");
			else if ((config & 0xffff) == 0x3){
				config &= ~(0xffff);
				config |= 0x3;
				pin_config_set(SUNXI_PINCTRL, pin_name, config);
			} else {
				printk("NAND_PIOFuncChange_EnREc: wrong pin func status: %d %d\n", pin_index, (__u32)(config & 0xffff));
			}
		}
	}

	return 0;
}

void NAND_PIORelease(__u32 nand_index)
{

	int	cnt;
	script_item_u *list = NULL;

	if(nand_index == 0)
	{
		//printk("[NAND] nand gpio_release\n");
	
		/* ??ȡgpio list */
		cnt = script_get_pio_list("nand0_para", &list);
		if(0 == cnt) {
			printk("get nand0_para gpio list failed\n");
			return;
		}

		/* ?ͷ?gpio */
		while(cnt--)
			gpio_free(list[cnt].gpio.gpio);
	}
	else if(nand_index == 1)
	{
		cnt = script_get_pio_list("nand1_para", &list);
		if(0 == cnt) {
			printk("get nand1_para gpio list failed\n");
			return;
		}

		/* ?ͷ?gpio */
		while(cnt--)
			gpio_free(list[cnt].gpio.gpio);
	}
	else
	{
		printk("NAND_PIORelease, nand_index error: 0x%x\n", nand_index);
	}	
}

void NAND_Memset(void* pAddr, unsigned char value, unsigned int len)
{
    memset(pAddr, value, len);   
}

void NAND_Memcpy(void* pAddr_dst, void* pAddr_src, unsigned int len)
{
    memcpy(pAddr_dst, pAddr_src, len);    
}

void* NAND_Malloc(unsigned int Size)
{
     	return kmalloc(Size, GFP_KERNEL);
}

void NAND_Free(void *pAddr, unsigned int Size)
{
    kfree(pAddr);
}

int NAND_Print(const char *fmt, ...)
{
	va_list args;
	int r;

	va_start(args, fmt);
	r = vprintk(fmt, args);
	va_end(args);
	
	return r;
}

void *NAND_IORemap(unsigned int base_addr, unsigned int size)
{
    return (void *)base_addr;
}

__u32 NAND_GetIOBaseAddrCH0(void)
{
	return 0xf1c03000;
}
	
__u32 NAND_GetIOBaseAddrCH1(void)
{
	return 0xf1c05000;
}

__u32 NAND_GetNdfcVersion(void)
{
	/*
		0: 
		1: A31/A31s/A21/A23
		2: 
	*/
	return 1;
}

__u32 NAND_GetNdfcDmaMode(void)
{
	writel(BIT(15), IO_ADDRESS(0x01c00024));
	if ( (readl(IO_ADDRESS(0x01c00024))>>16) == MAGIC0) {
		return 0; //gdma	
	} else {
		return 1; //idma
	}	
}

__u32 NAND_GetMaxChannelCnt(void)
{
	return 1;
}

__u32 NAND_GetPlatform(void)
{
	return 23;
}

DEFINE_SEMAPHORE(nand_physic_mutex);

int NAND_PhysicLockInit(void)
{
    return 0;
}

int NAND_PhysicLock(void)
{
	down(&nand_physic_mutex);
	return 0;
}

int NAND_PhysicUnLock(void)
{
    up(&nand_physic_mutex);
     return 0;
}

int NAND_PhysicLockExit(void)
{
     return 0;
}

/* request dma channel and set callback function */
int nand_request_dma(void)
{
	dma_cap_mask_t mask;

	printk("request DMA");

	/* Try to acquire a generic DMA engine slave channel */
	dma_cap_zero(mask);
	dma_cap_set(DMA_SLAVE, mask);

	if (dma_hdl == NULL) {
		dma_hdl = dma_request_channel(mask, NULL, NULL);
	    if (dma_hdl == NULL) {
	        printk("Request DMA failed!\n");
	        return -EINVAL;
	    }
	}

	return 0;
}

int NAND_ReleaseDMA(__u32 nand_index)
{
	if(dma_hdl !=NULL)
	{
		printk("nand release dma\n");
		dma_release_channel(dma_hdl);
		dma_hdl = NULL;
		return 0;
	}
	return 0;
	
}


void nand_dma_callback(void *arg)
{	
	__u32 nand_index;

	nand_index = NAND_GetCurrentCH();
	NAND_ClearDMAInt();
	nanddma_ready_flag[0] = 1;

	wake_up( &NAND_DMA_WAIT_CH0 );

//	printk("dma transfer finish\n");
	return;	
}

int nand_dma_config_start(__u32 rw, dma_addr_t addr,__u32 length)
{
	//int ret = 0;
	//int nents = 0;
	struct dma_slave_config dma_conf = {0};
	struct dma_async_tx_descriptor *dma_desc = NULL;

	dma_conf.direction = DMA_DEV_TO_MEM;
	dma_conf.src_addr = 0x01c03300;
	dma_conf.dst_addr = 0x01c03300;
	dma_conf.src_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
	dma_conf.dst_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
	dma_conf.src_maxburst = 1;
	dma_conf.dst_maxburst = 1;
	dma_conf.slave_id = rw ? sunxi_slave_id(DRQDST_NAND0, DRQSRC_SDRAM) : sunxi_slave_id(DRQDST_SDRAM, DRQSRC_NAND0);
	dmaengine_slave_config(dma_hdl, &dma_conf);
	
	dma_desc = dmaengine_prep_slave_single(dma_hdl, addr, length, 
				(rw ? DMA_TO_DEVICE : DMA_FROM_DEVICE), DMA_PREP_INTERRUPT|DMA_CTRL_ACK);
	if (!dma_desc) {
		printk("dmaengine prepare failed!\n");
		return -1;
	}

	dma_desc->callback = nand_dma_callback;
	dma_desc->callback_param = NULL;
	dmaengine_submit(dma_desc);

	dma_async_issue_pending(dma_hdl);	

	return 0;
}

__u32 NAND_GetNandExtPara(__u32 para_num)
{	
	script_item_u nand_para;
    script_item_value_type_e type;
	
	if (para_num == 0) {
	    type = script_get_item("nand0_para", "nand_p0", &nand_para);
	    if(SCIRPT_ITEM_VALUE_TYPE_INT != type)
	    {
	        printk("nand0_para, nand_p0, nand type err! %d\n", type);
			return 0xffffffff;
	    }
		else
			return nand_para.val;
	} else if (para_num == 1) {
	    type = script_get_item("nand0_para", "nand_p1", &nand_para);
	    if(SCIRPT_ITEM_VALUE_TYPE_INT != type)
	    {
	        printk("nand0_para, nand_p1, nand type err! %d\n", type);
			return 0xffffffff;
	    }
		else
			return nand_para.val;	
	} else {
		printk("NAND_GetNandExtPara: wrong para num: %d\n", para_num);
		return 0xffffffff;
	}
}

__u32 NAND_GetNandIDNumCtrl(void)
{
    script_item_u id_number_ctl;
    script_item_value_type_e type;

    type = script_get_item("nand0_para", "id_number_ctl", &id_number_ctl);
    if(SCIRPT_ITEM_VALUE_TYPE_INT != type)
    {
        printk("nand_para0, id_number_ctl, nand type err! %d\n", type);
		return 0x0;
    } else {
    	printk("nand : get id_number_ctl from script,%x \n",id_number_ctl.val);	
    	return id_number_ctl.val;
    }	
}

__s32 Nand_support_sorting(void)
{
    script_item_u sorting_flag;
    script_item_value_type_e type;

    type = script_get_item("nand0_para", "sorting_flag", &sorting_flag);
    if(SCIRPT_ITEM_VALUE_TYPE_INT != type)
    {
        printk("nand_para0, sorting_flag, nand type err! %d\n", type);
		return 0x0;
    } else {
    	printk("nand : get sorting_flag from script,%x \n",sorting_flag.val);	
    	return sorting_flag.val;
    }	
}

static void dumphex32(char *name, char *base, int len)
{
	__u32 i;

	if (!((unsigned int)base&0xf0000000)) {
		printk("dumphex32: err para in kernel, %s 0x%x\n", name, (unsigned int)base);
		return ;
	}
	printk("dump %s registers:", name);
	for (i=0; i<len*4; i+=4) {
		if (!(i&0xf))
			printk("\n0x%p : ", base + i);
		printk("0x%08x ", *((volatile unsigned int *)(base + i)));
	}
	printk("\n");
}

void NAND_DumpReg(void)
{
	dumphex32("nand0 reg", (char*)0xf1c03000, 60);
  	dumphex32("gpio reg", (char*)0xf1c20848, 40);		  	
	dumphex32("clk reg", (char*)0xf1c20080, 8);
	dumphex32("dma reg part0", (char*)0xf1c02000, 8);			
	dumphex32("dma reg part1", (char*)0xf1c02100, 50);	    
}

void NAND_ShowEnv(__u32 type, char *name, __u32 len, __u32 *val)
{
	int i;
    
    if (len && (val==NULL)) {
    	printk("kernel:NAND_ShowEnv, para error!\n");	
    	return ;
    }
    
    if (type == 0) 
    {
    	printk("kernel:%s: ", name);
    	for (i=0; i<len; i++)
    	{
    		if (i && (i%8==0))
    			printk("\n");
    		printk("%x ", val[i]);
    	}
    	printk("\n");
    }
    else 
    {
    	printk("kernel:NAND_ShowEnv, type error, %d!\n", type);
    }
    
    return ;
}

void NAND_Print_Version(void)
{
	__u32 val[4] = {0};
	
	val[0] = NAND_DRV_VERSION_0;
	val[1] = NAND_DRV_VERSION_1;
	val[2] = NAND_DRV_DATE;
	val[3] = NAND_DRV_TIME;
	NAND_ShowEnv(0, "nand version", 4, val);

}

int NAND_get_storagetype(void)
{
#if 1
    script_item_value_type_e script_ret;
    script_item_u storage_type;
    
    script_ret = script_get_item("target","storage_type", &storage_type);
    if(script_ret!=SCIRPT_ITEM_VALUE_TYPE_INT)
    {
           printk("nand init fetch storage_type failed\n");
           storage_type.val=0;
           return storage_type.val;
    }

    return (int)storage_type.val;
#else
	return 1;
#endif
    
}


