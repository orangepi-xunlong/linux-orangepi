/*
 * nand_osal_for_linux.c for  SUNXI NAND .
 *
 * Copyright (C) 2016 Allwinner.
 *
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */


#include "nand_osal_for_linux.h"
#include "nand_panic.h"

#define  NAND_DRV_VERSION_0		0x03
#define  NAND_DRV_VERSION_1		0x6085
#define  NAND_DRV_DATE			0x20220407
#define  NAND_DRV_TIME			0x18211952

int nand_type;

#define GPIO_BASE_ADDR			0x0300B000

int NAND_Snprintf(char *str, unsigned int size, const char *fmt, ...)
{
	struct va_format vaf;
	va_list args;
	int rtn;

	va_start(args, fmt);

	vaf.fmt = fmt;
	vaf.va = &args;

	rtn = snprintf(str, size, "%pV", &vaf);

	va_end(args);

	return rtn;
}

int NAND_Sprintf(char *str, const char *fmt, ...)
{
	struct va_format vaf;
	va_list args;
	int rtn;

	va_start(args, fmt);

	vaf.fmt = fmt;
	vaf.va = &args;

	rtn = sprintf(str, "%pV", &vaf);

	va_end(args);

	return rtn;
}

int NAND_Print(const char *fmt, ...)
{
	struct va_format vaf;
	va_list args;
	int rtn;

	va_start(args, fmt);

	vaf.fmt = fmt;
	vaf.va = &args;

	rtn = printk(KERN_ERR "%pV", &vaf);

	va_end(args);

	return rtn;
}

int NAND_Print_DBG(const char *fmt, ...)
{
	struct va_format vaf;
	va_list args;
	int rtn;

	va_start(args, fmt);

	vaf.fmt = fmt;
	vaf.va = &args;

	rtn = printk(KERN_DEBUG "%pV", &vaf);

	va_end(args);

	return rtn;
}

int NAND_ClkRequest(__u32 nand_index)
{
	long rate;

	NAND_Print_DBG("NAND_ClkRequest\n");

	pll6 = of_clk_get(ndfc_dev->of_node, 0);
	if ((pll6 == NULL) || IS_ERR(pll6)) {
		NAND_Print("%s: pll6 clock handle invalid!\n", __func__);
		return -1;
	}

	rate = clk_get_rate(pll6);
	NAND_Print_DBG("%s: get pll6 rate %dHZ\n", __func__, (__u32) rate);

	if (nand_index == 0) {
		nand0_dclk = of_clk_get(ndfc_dev->of_node, 1);

		if ((nand0_dclk == NULL) || IS_ERR(nand0_dclk)) {
			NAND_Print("%s: nand0 clock handle invalid!\n",
				__func__);
			return -1;
		}

		if (clk_set_parent(nand0_dclk, pll6))
			NAND_Print("%s:set nand0_dclk parent to pll6 failed\n",
				__func__);

		rate = clk_round_rate(nand0_dclk, 20000000);
		if (clk_set_rate(nand0_dclk, rate))
			NAND_Print("%s: set nand0_dclk rate to %dHZ failed!\n",
			       __func__, (__u32) rate);

		if (clk_prepare_enable(nand0_dclk))
			NAND_Print("%s: enable nand0_dclk failed!\n",
			       __func__);

		if (get_storage_type() == NAND_STORAGE_TYPE_RAWNAND) {
			nand0_cclk = of_clk_get(ndfc_dev->of_node, 2);

			if ((nand0_cclk == NULL) || IS_ERR(nand0_cclk)) {
				NAND_Print("%s: nand0 cclock handle invalid!\n", __func__);
				return -1;
			}

			if (clk_set_parent(nand0_cclk, pll6))
				NAND_Print("%s:set nand0_cclk parent to pll6 failed\n", __func__);

			rate = clk_round_rate(nand0_cclk, 20000000);
			if (clk_set_rate(nand0_cclk, rate))
				NAND_Print("%s: set nand0_cclk rate to %dHZ failed!\n", __func__, (__u32) rate);

			if (clk_prepare_enable(nand0_cclk))
				NAND_Print("%s: enable nand0_cclk failed!\n", __func__);
		}
	} else {
		NAND_Print("NAND_ClkRequest, nand_index error: 0x%x\n",
		       nand_index);
		return -1;
	}

	return 0;
}

void NAND_ClkRelease(__u32 nand_index)
{
	if (nand_index == 0) {
		if (nand0_dclk && !IS_ERR(nand0_dclk)) {
			clk_disable_unprepare(nand0_dclk);

			clk_put(nand0_dclk);
			nand0_dclk = NULL;
		}

		if (get_storage_type() == NAND_STORAGE_TYPE_RAWNAND) {
			if (nand0_cclk && !IS_ERR(nand0_cclk)) {
				clk_disable_unprepare(nand0_cclk);

				clk_put(nand0_cclk);
				nand0_cclk = NULL;
			}
		}
	} else
		NAND_Print("NAND_ClkRequest, nand_index error: 0x%x\n", nand_index);

	if (pll6 && !IS_ERR(pll6)) {
		clk_put(pll6);
		pll6 = NULL;
	}
}

int NAND_SetClk(__u32 nand_index, __u32 nand_clk0, __u32 nand_clk1)
{
	long rate;

	if (nand_index == 0) {
		if ((nand0_dclk == NULL) || IS_ERR(nand0_dclk)) {
			NAND_Print("%s: clock handle invalid!\n", __func__);
			return -1;
		}
		if (get_storage_type() == NAND_STORAGE_TYPE_RAWNAND)
			rate = clk_round_rate(nand0_dclk, nand_clk0 * 2000000);
		else if (get_storage_type() == NAND_STORAGE_TYPE_SPINAND)
			rate = clk_round_rate(nand0_dclk, nand_clk0 * 1000000);
		if (clk_set_rate(nand0_dclk, rate))
			NAND_Print("%s: set nand0_dclk to %dHZ failed! nand_clk: 0x%x\n",
			       __func__, (__u32) rate, nand_clk0);

		if (get_storage_type() == NAND_STORAGE_TYPE_RAWNAND) {
			if ((nand0_cclk == NULL) || IS_ERR(nand0_cclk)) {
				NAND_Print("%s: clock handle invalid!\n", __func__);
				return -1;
			}

			rate = clk_round_rate(nand0_cclk, nand_clk1 * 1000000);
			if (clk_set_rate(nand0_cclk, rate))
				NAND_Print("%s: set nand0_cclk to %dHZ failed! nand_clk: 0x%x\n",
				       __func__, (__u32) rate, nand_clk1);
		}
	} else {
		NAND_Print("NAND_SetClk, nand_index error: 0x%x\n", nand_index);
		return -1;
	}

	return 0;
}

int NAND_GetClk(__u32 nand_index, __u32 *pnand_clk0, __u32 *pnand_clk1)
{
	long rate = 0;

	if (nand_index == 0) {
		if ((nand0_dclk == NULL) || IS_ERR(nand0_dclk)) {
			NAND_Print("%s: clock handle invalid!\n", __func__);
			return -1;
		}
		rate = clk_get_rate(nand0_dclk);
		if (get_storage_type() == NAND_STORAGE_TYPE_RAWNAND)
			*pnand_clk0 = (rate / 2000000);
		else if (get_storage_type() == NAND_STORAGE_TYPE_SPINAND)
			*pnand_clk0 = (rate / 1000000);

		rate = 0;
		if (get_storage_type() == NAND_STORAGE_TYPE_RAWNAND) {
			if ((nand0_cclk == NULL) || IS_ERR(nand0_cclk)) {
				NAND_Print("%s: clock handle invalid!\n", __func__);
				return -1;
			}
			rate = clk_get_rate(nand0_cclk);
		}
		*pnand_clk1 = (rate / 1000000);
	} else {
		NAND_Print("NAND_GetClk, nand_index error: 0x%x\n", nand_index);
		return -1;
	}

	return 0;
}

void eLIBs_CleanFlushDCacheRegion_nand(void *adr, size_t bytes)
{
/*  __flush_dcache_area(adr, bytes + (1 << 5) * 2 - 2);*/
}

__s32 NAND_CleanFlushDCacheRegion(void *buff_addr, __u32 len)
{
	eLIBs_CleanFlushDCacheRegion_nand((void *)buff_addr, (size_t) len);
	return 0;
}

__s32 NAND_InvaildDCacheRegion(__u32 rw, __u32 buff_addr, __u32 len)
{
	return 0;
}

void *NAND_DMASingleMap(__u32 rw, void *buff_addr, __u32 len)
{
	void *mem_addr;

	if (is_on_panic())
		return (void *)nand_panic_dma_map(rw, buff_addr, len);

	if (rw == 1) {
		mem_addr = (void *)dma_map_single(ndfc_dev, (void *)buff_addr,
						len, DMA_TO_DEVICE);
	} else {
		mem_addr = (void *)dma_map_single(ndfc_dev, (void *)buff_addr,
						len, DMA_BIDIRECTIONAL);
	}
	if (dma_mapping_error(ndfc_dev, (dma_addr_t) mem_addr))
		NAND_Print("dma mapping error\n");

	return mem_addr;
}

void *NAND_DMASingleUnmap(__u32 rw, void *buff_addr, __u32 len)
{
	void *mem_addr = buff_addr;

	if (is_on_panic()) {
		nand_panic_dma_unmap(rw, (dma_addr_t)buff_addr, len);
		return mem_addr;
	}

	if (rw == 1) {
		dma_unmap_single(ndfc_dev, (dma_addr_t) mem_addr, len,
				 DMA_TO_DEVICE);
	} else {
		dma_unmap_single(ndfc_dev, (dma_addr_t) mem_addr, len,
				 DMA_BIDIRECTIONAL);
	}

	return mem_addr;
}

void *NAND_VA_TO_PA(void *buff_addr)
{
	return (void *)(__pa((void *)buff_addr));
}

__s32 NAND_PIORequest(__u32 nand_index)
{
	struct pinctrl *pinctrl = NULL;

	PRINT_LEVEL = NAND_Print_level();

	pinctrl = pinctrl_get_select(ndfc_dev, "default");
	if (!pinctrl || IS_ERR(pinctrl)) {
		NAND_Print("NAND_PIORequest: set nand0 pin error!\n");
		return -1;
	}

	return 0;
}

__s32 NAND_3DNand_Request(void)
{
	u32 cfg;
	void __iomem *gpio_ptr = ioremap(GPIO_BASE_ADDR, 0x400);

	cfg = *((volatile __u32 *)gpio_ptr + 208);
	cfg |= 0x4;
	*((volatile __u32 *)gpio_ptr + 208) = cfg;
	NAND_Print("Change PC_Power Mode Select to 1.8V\n");

	iounmap(gpio_ptr);
	return 0;
}

__s32 NAND_Check_3DNand(void)
{
	u32 cfg;
	void __iomem *gpio_ptr = ioremap(GPIO_BASE_ADDR, 0x400);

	cfg = *((volatile __u32 *)gpio_ptr + 208);
	if ((cfg >> 2) == 0) {
		cfg |= 0x4;
		*((volatile __u32 *)gpio_ptr + 208) = cfg;
		NAND_Print("Change PC_Power Mode Select to 1.8V\n");
	}

	iounmap(gpio_ptr);
	return 0;
}

void NAND_PIORelease(__u32 nand_index)
{

	struct pinctrl *pinctrl = NULL;

	pinctrl = pinctrl_get_select(ndfc_dev, "sleep");
	if (!pinctrl || IS_ERR(pinctrl))
		NAND_Print("NAND_PIORelease: set nand0 pin error!\n");

}
void NAND_Bug(void)
{
	BUG();
}
void NAND_Mdelay(unsigned long ms)
{
	mdelay(ms);
}
void NAND_Memset(void *pAddr, unsigned char value, unsigned int len)
{
	memset(pAddr, value, len);
}

int NAND_Memcmp(const void *s1, const void *s2, size_t n)
{
	return memcmp(s1, s2, n);
}

int NAND_Strcmp(const char *s1, const char *s2)
{
	return strcmp(s1, s2);
}

void NAND_Memcpy(void *pAddr_dst, void *pAddr_src, unsigned int len)
{
	memcpy(pAddr_dst, pAddr_src, len);
}

void *NAND_Malloc(unsigned int Size)
{
	return kmalloc(Size, GFP_KERNEL);
}

void *NAND_DMA_Malloc(unsigned int Size)
{
	return kmalloc(Size, GFP_KERNEL);
	//return kmalloc(Size, GFP_KERNEL | GFP_DMA);
	//return vmalloc(Size);
}

void *NAND_VMalloc(unsigned int Size)
{
	return vmalloc(Size);
}

void NAND_VFree(void *ptr)
{
	return vfree(ptr);
}

void NAND_Free(void *pAddr, unsigned int Size)
{
	kfree(pAddr);
}

void *NAND_IORemap(void *base_addr, unsigned int size)
{
	return base_addr;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
DEFINE_SEMAPHORE(nand_physic_mutex);

int NAND_PhysicLockInit(void)
{
	return 0;
}

int NAND_PhysicLock(void)
{
	if (is_on_panic())
		return 0;
	down(&nand_physic_mutex);
	return 0;
}

int NAND_PhysicUnLock(void)
{
	if (is_on_panic())
		return 0;
	up(&nand_physic_mutex);
	return 0;
}

int NAND_PhysicLockExit(void)
{
	return 0;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
/* request dma channel and set callback function */
int spinand_request_tx_dma(void)
{
#if 0//#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 0, 0))
	dma_cap_mask_t mask;

	NAND_Print_DBG("request tx DMA\n");

	/* Try to acquire a generic DMA engine slave channel */
	dma_cap_zero(mask);
	dma_cap_set(DMA_SLAVE, mask);

	if (dma_hdl_tx == NULL) {
		dma_hdl_tx = dma_request_channel(mask, NULL, NULL);
		if (dma_hdl_tx == NULL) {
			NAND_Print_DBG("Request tx DMA failed!\n");
			return -EINVAL;
	    }
	}
#else
	enum spinand_dma_dir _dir = SPINAND_DMA_WDEV;

	NAND_Print_DBG("request tx DMA\n");

	if (dma_tx.chan == NULL) {
		dma_tx.chan = dma_request_chan(ndfc_dev, "tx");
		if (IS_ERR(dma_tx.chan)) {
			printk("Request DMA(dir %d) failed! chan: %p\n", _dir, dma_tx.chan);
			dma_tx.chan = NULL;
			return -EINVAL;
		}
	}
	dma_tx.dir = _dir;

	NAND_Print_DBG("tx DMA ch: %d\n", dma_tx.chan->chan_id);
#endif

	return 0;
}

int spinand_request_rx_dma(void)
{
#if 0//#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 0, 0))
	dma_cap_mask_t mask;

	NAND_Print_DBG("request rx DMA\n");

	/* Try to acquire a generic DMA engine slave channel */
	dma_cap_zero(mask);
	dma_cap_set(DMA_SLAVE, mask);

	if (dma_hdl_rx == NULL) {
		dma_hdl_rx = dma_request_channel(mask, NULL, NULL);
		if (dma_hdl_rx == NULL) {
			NAND_Print_DBG("Request rx DMA failed!\n");
			return -EINVAL;
	    }
	}
#else
	enum spinand_dma_dir _dir = SPINAND_DMA_RDEV;

	NAND_Print_DBG("request rx DMA\n");

	if (dma_rx.chan == NULL) {
		dma_rx.chan = dma_request_chan(ndfc_dev, "rx");
		if (IS_ERR(dma_rx.chan)) {
			printk("Request DMA(dir %d) failed! chan: %p\n", _dir, dma_rx.chan);
			dma_rx.chan = NULL;
			return -EINVAL;
		}
	}
	dma_rx.dir = _dir;

	NAND_Print_DBG("rx DMA ch: %d\n", dma_rx.chan->chan_id);
#endif
	return 0;
}

int spinand_releasetxdma(void)
{
#if 0//#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 0, 0))
	if (dma_hdl_tx != NULL) {
		printk("spinand release tx dma\n");
		dma_release_channel(dma_hdl_tx);
		dma_hdl_tx = NULL;
		return 0;
	}
#else
	if (dma_tx.chan == NULL) {
		NAND_Print_DBG("spinand release rx dma\n");
		dma_release_channel(dma_tx.chan);
		dma_tx.chan = NULL;
		return 0;
	}
#endif
	return 0;
}

int spinand_releaserxdma(void)
{
#if 0//#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 0, 0))
	if (dma_hdl_rx != NULL) {
		NAND_Print_DBG("spinand release rx dma\n");
		dma_release_channel(dma_hdl_rx);
		dma_hdl_rx = NULL;
		return 0;
	}
#else
	if (dma_rx.chan == NULL) {
		NAND_Print_DBG("spinand release rx dma\n");
		dma_release_channel(dma_rx.chan);
		dma_rx.chan = NULL;
		return 0;
	}
#endif
	return 0;
}

void prepare_spinand_dma_callback(void)
{
	/* clear it first */
	init_completion(&spinand_dma_done);
}

void spinand_dma_callback(void *arg)
{
	complete(&spinand_dma_done);
}

#if 0//#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 0, 0))
int tx_dma_config_start(void *addr, __u32 length)
{
	struct dma_slave_config dma_conf = {0};
	struct dma_async_tx_descriptor *dma_desc = NULL;

	dma_conf.direction = DMA_MEM_TO_DEV;
	dma_conf.dst_addr = SPI_BASE_ADDR + SPI_TX_DATA_REG;

	dma_conf.src_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
	dma_conf.dst_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
	dma_conf.src_maxburst = 8;
	dma_conf.dst_maxburst = 8;

	dma_conf.slave_id = sunxi_slave_id(DRQDST_SPI0_TX, DRQSRC_SDRAM);

	dmaengine_slave_config(dma_hdl_tx, &dma_conf);

	dma_desc = dmaengine_prep_slave_single(dma_hdl_tx, addr, length,
				DMA_TO_DEVICE, DMA_PREP_INTERRUPT|DMA_CTRL_ACK);
	if (!dma_desc) {
		NAND_Print_DBG("tx dmaengine prepare failed!\n");
		return -1;
	}

	prepare_spinand_dma_callback();
	dma_desc->callback = spinand_dma_callback;
	dma_desc->callback_param = NULL;
	dmaengine_submit(dma_desc);

	dma_async_issue_pending(dma_hdl_tx);

	return 0;
}

int rx_dma_config_start(void *addr, __u32 length)
{
	struct dma_slave_config dma_conf = {0};
	struct dma_async_tx_descriptor *dma_desc = NULL;

	dma_conf.direction = DMA_DEV_TO_MEM;
	dma_conf.src_addr = SPI_BASE_ADDR + SPI_RX_DATA_REG;

	dma_conf.src_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
	dma_conf.dst_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
	dma_conf.src_maxburst = 8;
	dma_conf.dst_maxburst = 8;
	dma_conf.slave_id = sunxi_slave_id(DRQDST_SDRAM, DRQSRC_SPI0_RX);

	dmaengine_slave_config(dma_hdl_rx, &dma_conf);

	dma_desc = dmaengine_prep_slave_single(dma_hdl_rx, addr, length,
				DMA_FROM_DEVICE, DMA_PREP_INTERRUPT|DMA_CTRL_ACK);
	if (!dma_desc) {
		NAND_Print_DBG("rx dmaengine prepare failed!\n");
		return -1;
	}

	prepare_spinand_dma_callback();
	dma_desc->callback = spinand_dma_callback;
	dma_desc->callback_param = NULL;
	dmaengine_submit(dma_desc);

	dma_async_issue_pending(dma_hdl_rx);

	return 0;
}
#else
//TODO
//from sunxi_spi_dmg_sg_cnt
static int sunxi_spinand_dmg_sg_cnt(void *addr, int len)
{
	int npages = 0;
	char *bufp = (char *)addr;
	int mapbytes = 0;
	int bytesleft = len;

	while (bytesleft > 0) {
		if (bytesleft < (PAGE_SIZE - offset_in_page(bufp)))
			mapbytes = bytesleft;
		else
			mapbytes = PAGE_SIZE - offset_in_page(bufp);

		npages++;
		bufp += mapbytes;
		bytesleft -= mapbytes;
	}
	return npages;
}

//from sunxi_spi_dma_init_sg
static int spinand_dma_init_sg(spinand_dma_info_t *info, void *addr, int len)
{
	int i;
	int npages = 0;
	void *bufp = addr;
	int mapbytes = 0;
	int bytesleft = len;

	npages = sunxi_spinand_dmg_sg_cnt(addr, len);
	WARN_ON(npages == 0);
#if 0
	printk("addr: %lx, npages = %d, len = %d\n", (unsigned long)addr, npages, len);
#endif
	if (npages > SPINAND_MAX_PAGES)
		npages = SPINAND_MAX_PAGES;

	sg_init_table(info->sg, npages);
	for (i = 0; i < npages; i++) {
		/* If there are less bytes left than what fits
		 * in the current page (plus page alignment offset)
		 * we just feed in this, else we stuff in as much
		 * as we can.
		 */
		if (bytesleft < (PAGE_SIZE - offset_in_page(bufp)))
			mapbytes = bytesleft;
		else
			mapbytes = PAGE_SIZE - offset_in_page(bufp);
#if 0
		printk("%d: len %d, offset %ld, addr %lx(%d)\n", i, mapbytes,
			offset_in_page(bufp), (unsigned long)bufp, virt_addr_valid(bufp));
#endif
		if (virt_addr_valid(bufp))
			sg_set_page(&info->sg[i], virt_to_page(bufp),
				    mapbytes, offset_in_page(bufp));
		else
			sg_set_page(&info->sg[i], vmalloc_to_page(bufp),
				    mapbytes, offset_in_page(bufp));

		bufp += mapbytes;
		bytesleft -= mapbytes;
	}

	WARN_ON(bytesleft);
	info->nents = npages;
	return 0;
}

int dma_config_start(void *addr, __u32 length, __u32 tx)
{
	int i;
	int ret = 0;
	int nents = 0;
	struct dma_slave_config dma_conf = {0};
	struct dma_async_tx_descriptor *dma_desc = NULL;
	spinand_dma_info_t *dma = tx ? &dma_tx : &dma_rx;
	enum dma_data_direction dma_data_dir = tx ? DMA_TO_DEVICE : DMA_FROM_DEVICE;
	enum dma_transfer_direction dma_xfer_dir = tx ? DMA_MEM_TO_DEV : DMA_DEV_TO_MEM;
	const char *str_dir = tx ? "dma tx" : "dma rx";

#if 0
	printk("[spinand] %s addr: %lx, length: %lu\n", str_dir, (unsigned long)addr, (unsigned long)length);
#endif

	dma_conf.direction = dma_xfer_dir;
	if (tx)
		dma_conf.dst_addr = (phys_addr_t)(SPI_BASE_ADDR + SPI_TX_DATA_REG);
	else
		dma_conf.src_addr = (phys_addr_t)(SPI_BASE_ADDR + SPI_RX_DATA_REG);

	ret = spinand_dma_init_sg(dma, addr, length);
	if (ret != 0) {
		printk("spinand_dma_init_sg fialed! ret: %d\n", ret);
		return ret;
	}

	if (length % DMA_SLAVE_BUSWIDTH_4_BYTES) {
		dma_conf.src_addr_width = DMA_SLAVE_BUSWIDTH_1_BYTE;
		dma_conf.dst_addr_width = DMA_SLAVE_BUSWIDTH_1_BYTE;
	} else {
		dma_conf.src_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
		dma_conf.dst_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
	}
	dma_conf.src_maxburst = 4;
	dma_conf.dst_maxburst = 4;


	dmaengine_slave_config(dma->chan, &dma_conf);

	nents = dma_map_sg(ndfc_dev, dma->sg, dma->nents, dma_data_dir);
	if (!nents) {
		printk("[spinand] dma_map_sg(%d) failed! ret: %d\n", dma->nents, nents);
		return -ENOMEM;
	}

	dma_desc = dmaengine_prep_slave_sg(dma->chan, dma->sg, nents, dma_xfer_dir, DMA_PREP_INTERRUPT|DMA_CTRL_ACK);
	if (!dma_desc) {
		printk("[spinand] dmaengine_prep_slave_sg() failed!\n");
		dma_unmap_sg(ndfc_dev, dma->sg, dma->nents, dma_data_dir);
		return -1;
	}

	prepare_spinand_dma_callback();
	dma_desc->callback = spinand_dma_callback;
	dma_desc->callback_param = NULL;
	dmaengine_submit(dma_desc);

	dma_async_issue_pending(dma->chan);
#if 0
	printk("[spinand] %s config end\n", str_dir);
#endif
	return 0;
}
#endif



int spinand_dma_config_start(__u32 rw, __u32 addr, __u32 length)
{
#if 0//#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 0, 0))
	int ret;
	dma_map_addr = NAND_DMASingleMap(rw, (void *)addr, length);

	if (rw)
		ret = tx_dma_config_start((dma_addr_t)dma_map_addr, length);
	else
		ret = rx_dma_config_start((dma_addr_t)dma_map_addr, length);

	if (ret != 0) /* fail */
		NAND_DMASingleUnmap(rw, dma_map_addr, length);

	return ret;
#else
	return dma_config_start(addr, length, rw);
#endif
}

int rawnand_dma_terminate(__u32 rw)
{
	return 0;
}

int spinand_dma_terminate(__u32 rw)
{
	int ret;
#if 0//#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 0, 0))
	if (rw)
		ret = dmaengine_terminate_sync(dma_hdl_tx);
	else
		ret = dmaengine_terminate_sync(dma_hdl_rx);
#else
	if (rw)
		ret = dmaengine_terminate_sync(dma_tx.chan);
	else
		ret = dmaengine_terminate_sync(dma_rx.chan);
#endif
	return ret;
}

int nand_dma_terminate(__u32 rw)
{
	int ret = 0;

	if (get_storage_type() == NAND_STORAGE_TYPE_RAWNAND)
		ret = rawnand_dma_terminate(rw);
	else if (get_storage_type() == NAND_STORAGE_TYPE_SPINAND)
		ret = spinand_dma_terminate(rw);

	return ret;
}


int NAND_WaitDmaFinish(__u32 tx_flag, __u32 rx_flag)
{
	__u32 timeout = 2000;
	__u32 timeout_flag = 0;

	if (is_on_panic())
		return 0;

	if (tx_flag && rx_flag)
		NAND_Print("only one completion, not support wait both tx and rx!!\n");

	if (tx_flag || rx_flag) {
		timeout_flag = wait_for_completion_timeout(&spinand_dma_done, msecs_to_jiffies(timeout));
		if (!timeout_flag) {
			NAND_Print_DBG("wait dma finish timeout!! tx_flag:%d rx_flag:%d\n", tx_flag, rx_flag);
			/* terminate dma to avoid chaos */
			if (rx_flag)
				nand_dma_terminate(0);
			if (tx_flag)
				nand_dma_terminate(1);

			return -1;
		}
	}

	return 0;
}

int Nand_Dma_End(__u32 rw, __u32 addr, __u32 length)
{
#if 0//#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 0, 0))
	NAND_DMASingleUnmap(rw, dma_map_addr, length);
#else
	spinand_dma_info_t *dma = rw ? &dma_tx : &dma_rx;
	enum dma_data_direction dma_data_dir = rw ? DMA_TO_DEVICE : DMA_FROM_DEVICE;

	dma_unmap_sg(ndfc_dev, dma->sg, dma->nents, dma_data_dir);
#endif
	return 0;
}

int rawnand_dma_config_start(__u32 rw, __u32 addr, __u32 length)
{
#if 0
/*no use extern  DMA*/
	struct dma_slave_config dma_conf = { 0 };
	struct dma_async_tx_descriptor *dma_desc = NULL;

	dma_conf.direction = DMA_DEV_TO_MEM;
	dma_conf.src_addr = 0x01c03300;
	dma_conf.dst_addr = 0x01c03300;
	dma_conf.src_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
	dma_conf.dst_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
	dma_conf.src_maxburst = 1;
	dma_conf.dst_maxburst = 1;
	dma_conf.slave_id =
	    rw ? sunxi_slave_id(DRQDST_NAND0,
				DRQSRC_SDRAM) : sunxi_slave_id(DRQDST_SDRAM,
							       DRQSRC_NAND0);
	dmaengine_slave_config(dma_hdl, &dma_conf);

	dma_desc = dmaengine_prep_slave_single(dma_hdl, addr, length,
					       (rw ? DMA_TO_DEVICE :
						DMA_FROM_DEVICE),
					       DMA_PREP_INTERRUPT |
					       DMA_CTRL_ACK);
	if (!dma_desc) {
		NAND_Print("dmaengine prepare failed!\n");
		return -1;
	}

	dma_desc->callback = (void *)nand_dma_callback;
	if (rw == 0)
		dma_desc->callback_param = NULL;
	else
		dma_desc->callback_param = (void *)(dma_desc);

	dmaengine_submit(dma_desc);

	dma_async_issue_pending(dma_hdl);
#endif
	return 0;
}

int nand_dma_config_start(__u32 rw, __u32 addr, __u32 length)
{
	int ret = 0;

	if (get_storage_type() == NAND_STORAGE_TYPE_RAWNAND)
		ret = rawnand_dma_config_start(rw, addr, length);
	else if (get_storage_type() == NAND_STORAGE_TYPE_SPINAND)
		ret = spinand_dma_config_start(rw, addr, length);

	return ret;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
__u32 NAND_GetNdfcDmaMode(void)
{
	return 1;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
__u32 NAND_GetNandExtPara(__u32 para_num)
{
	int ret = 0;
	int nand_para = 0xffffffff;

	if (para_num == 0) {	/*frequency */
		ret = of_property_read_u32(ndfc_dev->of_node, "nand0_p0",
					 &nand_para);
		if (ret) {
			NAND_Print("Failed to get nand_p0\n");
			return 0xffffffff;
		}

		if (nand_para == 0x55aaaa55) {
			NAND_Print_DBG("nand_p0 is no used\n");
			nand_para = 0xffffffff;
		} else
			NAND_Print_DBG("nand: get nand_p0 %x\n", nand_para);
	} else if (para_num == 1) {	/*SUPPORT_TWO_PLANE */
		ret = of_property_read_u32(ndfc_dev->of_node, "nand0_p1",
					 &nand_para);
		if (ret) {
			NAND_Print("Failed to get nand_p1\n");
			return 0xffffffff;
		}

		if (nand_para == 0x55aaaa55) {
		NAND_Print_DBG("nand_p1 is no used\n");
			nand_para = 0xffffffff;
		} else
			NAND_Print_DBG("nand : get nand_p1 %x\n", nand_para);
	} else if (para_num == 2) {	/*SUPPORT_VERTICAL_INTERLEAVE */
		ret = of_property_read_u32(ndfc_dev->of_node, "nand0_p2",
					 &nand_para);
		if (ret) {
			NAND_Print("Failed to get nand_p2\n");
			return 0xffffffff;
		}

		if (nand_para == 0x55aaaa55) {
			NAND_Print_DBG("nand_p2 is no used\n");
			nand_para = 0xffffffff;
		} else
			NAND_Print_DBG("nand : get nand_p2 %x\n", nand_para);
	} else if (para_num == 3) {	/*SUPPORT_DUAL_CHANNEL */
		ret = of_property_read_u32(ndfc_dev->of_node, "nand0_p3",
					 &nand_para);
		if (ret) {
			NAND_Print("Failed to get nand_p3\n");
			return 0xffffffff;
		}

		if (nand_para == 0x55aaaa55) {
			NAND_Print_DBG("nand_p3 is no used\n");
			nand_para = 0xffffffff;
		} else
			NAND_Print_DBG("nand: get nand_p3 %x\n", nand_para);
	} else {
		NAND_Print("NAND_GetNandExtPara: wrong para num: %d\n",
		       para_num);
		return 0xffffffff;
	}
	return nand_para;
}

__u32 NAND_GetNandIDNumCtrl(void)
{
	int ret;
	int id_number_ctl = 0;

	ret = of_property_read_u32(ndfc_dev->of_node, "nand0_id_number_ctl",
				 &id_number_ctl);
	if (ret) {
		NAND_Print_DBG("Failed to get id_number_ctl\n");
		id_number_ctl = 0;
	} else {
		if (id_number_ctl == 0x55aaaa55) {
			NAND_Print_DBG("id_number_ctl is no used\n");
			id_number_ctl = 0;
		} else
			NAND_Print_DBG("nand : get id_number_ctl %x\n",
				       id_number_ctl);
	}
	return id_number_ctl;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
__u32 NAND_GetMaxChannelCnt(void)
{
	return 1;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
int nand_request_dma(void)
{
#if 0//#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 0, 0))
	dma_cap_mask_t mask;

	NAND_Print_DBG("request DMA");

	/* Try to acquire a generic DMA engine slave channel */
	dma_cap_zero(mask);
	dma_cap_set(DMA_SLAVE, mask);

	if (dma_hdl == NULL) {
		dma_hdl = dma_request_channel(mask, NULL, NULL);
		if (dma_hdl == NULL) {
			NAND_Print("Request DMA failed!\n");
			return -EINVAL;
		}
	}
	NAND_Print_DBG("chan_id: %d", dma_hdl->chan_id);
#else
	NAND_Print_DBG("request g DMA\n");

	if (g_dma.chan == NULL) {
		g_dma.chan = dma_request_chan(ndfc_dev, "g");
		if (g_dma.chan == NULL) {
			printk("Request DMA failed! chan: %p\n", g_dma.chan);
			return -EINVAL;
		}
	}

	NAND_Print_DBG("g DMA ch: %d\n", g_dma.chan->chan_id);
#endif
	return 0;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
int NAND_ReleaseDMA(__u32 nand_index)
{
#if 0//#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 0, 0))
	if (dma_hdl != NULL) {
		NAND_Print_DBG("nand release dma\n");
		dma_release_channel(dma_hdl);
		dma_hdl = NULL;
		return 0;
	}
#else
	if (g_dma.chan == NULL) {
		NAND_Print_DBG("spinand release rx dma\n");
		dma_release_channel(g_dma.chan);
		g_dma.chan = NULL;
		return 0;
	}
#endif
	return 0;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
__u32 NAND_GetNdfcVersion(void)
{
	return 1;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
void *RAWNAND_GetIOBaseAddrCH0(void)
{
	return NDFC0_BASE_ADDR;
}

void *RAWNAND_GetIOBaseAddrCH1(void)
{
	return NDFC1_BASE_ADDR;
}
void *SPINAND_GetIOBaseAddrCH0(void)
{
	return SPIC0_IO_BASE;
}

void *SPINAND_GetIOBaseAddrCH1(void)
{
	return NULL;
}


/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         : wait rb
*****************************************************************************/
static DECLARE_WAIT_QUEUE_HEAD(NAND_RB_WAIT_CH0);
static DECLARE_WAIT_QUEUE_HEAD(NAND_RB_WAIT_CH1);

__s32 nand_rb_wait_time_out(__u32 no, __u32 *flag)
{
	__s32 ret;

	if (no == 0)
		ret = wait_event_timeout(NAND_RB_WAIT_CH0, *flag, HZ >> 1);
	else
		ret = wait_event_timeout(NAND_RB_WAIT_CH1, *flag, HZ >> 1);

	return ret;
}

__s32 nand_rb_wake_up(__u32 no)
{
	if (no == 0)
		wake_up(&NAND_RB_WAIT_CH0);
	else
		wake_up(&NAND_RB_WAIT_CH1);

	return 0;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         : wait dma
*****************************************************************************/
static DECLARE_WAIT_QUEUE_HEAD(NAND_DMA_WAIT_CH0);
static DECLARE_WAIT_QUEUE_HEAD(NAND_DMA_WAIT_CH1);

__s32 nand_dma_wait_time_out(__u32 no, __u32 *flag)
{
	__s32 ret;

	if (no == 0)
		ret = wait_event_timeout(NAND_DMA_WAIT_CH0, *flag, HZ >> 1);
	else
		ret = wait_event_timeout(NAND_DMA_WAIT_CH1, *flag, HZ >> 1);

	return ret;
}

__s32 nand_dma_wake_up(__u32 no)
{
	if (no == 0)
		wake_up(&NAND_DMA_WAIT_CH0);
	else
		wake_up(&NAND_DMA_WAIT_CH1);

	return 0;
}

__u32 nand_dma_callback(void *para)
{
	return 0;
}

int NAND_get_storagetype(void)
{
	return 0;
}

int NAND_GetVoltage(void)
{

	int ret = 0;
	const char *sti_vcc_nand = NULL;
	const char *sti_vcc_io = NULL;

	ret = of_property_read_string(ndfc_dev->of_node, "nand0_regulator1",
				    &sti_vcc_nand);
	NAND_Print_DBG("nand0_regulator1 %s\n", sti_vcc_nand);
	if (ret)
		NAND_Print_DBG("Failed to get vcc_nand\n");

	regu1 = regulator_get(NULL, sti_vcc_nand);
	if (IS_ERR(regu1))
		NAND_Print_DBG("nand:fail to get regulator vcc-nand!\n");
	else {
		/*enable regulator */
		ret = regulator_enable(regu1);
		if (IS_ERR(regu1)) {
			NAND_Print_DBG("nand:fail to enable regulator vcc-nand!\n");
			return -1;
		}
		NAND_Print_DBG("nand:get voltage vcc-nand ok:%p\n", regu1);
	}

	if (get_storage_type() == NAND_STORAGE_TYPE_RAWNAND) {
		ret = of_property_read_string(ndfc_dev->of_node,
			 "nand0_regulator2", &sti_vcc_io);
		NAND_Print_DBG("nand0_regulator2 %s\n", sti_vcc_io);
		if (ret)
			NAND_Print_DBG("Failed to get vcc_io\n");

		regu2 = regulator_get(NULL, sti_vcc_io);
		if (IS_ERR(regu2))
			NAND_Print_DBG("nand:fail to get regulator vcc-io!\n");
		else {
			/*enable regulator */
			ret = regulator_enable(regu2);
			if (IS_ERR(regu2)) {
				NAND_Print
				    ("fail to enable regulator vcc-io!\n");
				return -1;
			}
			NAND_Print_DBG("nand:get voltage vcc-io ok:%p\n", regu2);
		}
	}

	NAND_Print_DBG("nand:has already get voltage\n");

	return ret;

}

int NAND_ReleaseVoltage(void)
{
	int ret = 0;

	if (!IS_ERR(regu1)) {
		NAND_Print_DBG("nand release voltage vcc-nand\n");
		ret = regulator_disable(regu1);
		if (ret)
			NAND_Print_DBG("nand: regu1 disable fail, ret 0x%x\n", ret);
		if (IS_ERR(regu1))
			NAND_Print_DBG("nand: fail to disable regulator vcc-nand!");

		/*put regulator when module exit */
		regulator_put(regu1);

		regu1 = NULL;
	}

	if (get_storage_type() == NAND_STORAGE_TYPE_RAWNAND) {
		if (!IS_ERR(regu2)) {
			NAND_Print_DBG("nand release voltage vcc-io\n");
			ret = regulator_disable(regu2);
			if (ret)
				NAND_Print_DBG("nand: regu2 disable fail,ret 0x%x\n", ret);
			if (IS_ERR(regu2))
				NAND_Print_DBG("nand: fail to disable regulator vcc-io!\n");

			/*put regulator when module exit */
			regulator_put(regu2);

			regu2 = NULL;
		}
	}

	NAND_Print_DBG("nand had already release voltage\n");

	return ret;

}

int NAND_IS_Secure_sys(void)
{
	if (sunxi_soc_is_secure()) {
		NAND_Print_DBG("secure system\n");
		return 1;
	}
		NAND_Print_DBG("non secure\n");

		return 0;
}

__u32 NAND_Print_level(void)
{
	int ret;
	int print_level = 0xffffffff;

	ret = of_property_read_u32(ndfc_dev->of_node, "nand0_print_level",
				 &print_level);
	if (ret) {
		NAND_Print_DBG("Failed to get print_level\n");
		print_level = 0xffffffff;
	} else {
		if (print_level == 0x55aaaa55) {
			NAND_Print_DBG("print_level is no used\n");
			print_level = 0xffffffff;
		} else
			NAND_Print_DBG("nand : get print_level %x\n", print_level);
	}

	return print_level;
}

int NAND_Get_Dragonboard_Flag(void)
{
	int ret;
	int dragonboard_flag = 0;

	ret = of_property_read_u32(ndfc_dev->of_node, "nand0_dragonboard",
				 &dragonboard_flag);
	if (ret) {
		NAND_Print_DBG("Failed to get dragonboard_flag\n");
		dragonboard_flag = 0;
	} else {
		NAND_Print_DBG("nand: dragonboard_flag %x\n", dragonboard_flag);
	}

	return dragonboard_flag;
}

void NAND_Print_Version(void)
{
	int val[4] = { 0 };

	val[0] = NAND_DRV_VERSION_0;
	val[1] = NAND_DRV_VERSION_1;
	val[2] = NAND_DRV_DATE;
	val[3] = NAND_DRV_TIME;

	NAND_Print("kernel: nand version: %x %x %x %x\n", val[0],
			val[1], val[2], val[3]);
}

int NAND_Get_Version(int *ver_main, int *ver_sub, int *date, int *time)
{
	*ver_main = NAND_DRV_VERSION_0;
	*ver_sub = NAND_DRV_VERSION_1;
	*date = NAND_DRV_DATE;
	*time = NAND_DRV_TIME;
	return 0;
}

void nand_cond_resched(void)
{
	cond_resched();
}

__u32 get_storage_type(void)
{
	return nand_type;
}

char *nand_get_cur_task_name(void)
{
	return current->comm;
}

int nand_get_cur_task_pid(void)
{
	return (int)task_pid_nr(current);
}

void dump_buf(__u8 *buf, __u32 len)
{
	int i = 0;

	printk("\n");
	for (i = 0 ; i < len ; i++) {
		if ((i%16) == 0)
			printk("%08x: ", i);
		printk("%02x ", buf[i]);
		if (((i+1)%16) == 0)
			printk("\n");
	}
	printk("\n");
}