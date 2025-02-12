/* SPDX-License-Identifier: GPL-2.0 */
#ifndef X1_ISP_REG_H
#define X1_ISP_REG_H

#include <media/x1/x1_isp_drv_uapi.h>

#include <linux/types.h>

#define ISP_REG_MASK		0x7ffff
#define ISP_REG_BASE_OFFSET	0x30000

#define ISP_REG_OFFSET_TOP_PIPE(n) (0x31700 + 0x8000 * (n))
#define ISP_REG_OFFSET_GLOBALRESET 0x158
#define ISP_REG_IRQ_STATUS	0x80
#define ISP_REG_IRQ_MASK	0x84
#define ISP_RE_IRQ_RAW		0x88
#define ISP_REG_IDI_GAP_OFFSET	0x134

//RGBIR AVG
#define ISP_REG_OFFSET_AVG0	0x150
#define ISP_REG_OFFSET_AVG1	0x154

//dma base addr 0xc0241000, isp base 0xc0230000
#define REG_ISP_OFFSET_DMASYS	0x41000
//dma mux ctrl
#define REG_ISP_OFFSET_DMA_MUX_CTRL	0x40038
//dma channel master
#define REG_ISP_DMA_CHANNEL_MASTER(n)	(0x410d0 + 4 * (n))
//pdc dma master
#define REG_ISP_PDC_DMA_MASTER		(REG_ISP_OFFSET_DMASYS + 0x108)

//write pitch
#define REG_ISP_DMA_CHANNEL_WR_PITCH(n)	(0x41098 + 4 * (n))
//dma irq mask
#define REG_ISP_DMA_IRQ_MASK1		(REG_ISP_OFFSET_DMASYS + 0x124)	//ch0~ch10(except ch10 err)
#define REG_ISP_DMA_IRQ_MASK2		(REG_ISP_OFFSET_DMASYS + 0x128)	//ch10(only err)~ch15, read ch0~ch2

//dma channel addr, ch0 ~ ch13
#define REG_ISP_DMA_Y_ADDR(n)		(REG_ISP_OFFSET_DMASYS + ((n) * 0x8))

//dma channel high addr, ch0 ~ ch13
#define REG_ISP_DMA_Y_HIGH_ADDR(n)	(REG_ISP_OFFSET_DMASYS + 0X158 + ((n) * 0x8))

//pdc dma channel addr, p0 and p1
#define REG_ISP_PDC_DMA_BASE_ADDR(n)	(REG_ISP_OFFSET_DMASYS + 0x70 + ((n) * 0x10))
#define REG_ISP_PDC_DMA_HIGH_BASE_ADDR(n)	(REG_ISP_OFFSET_DMASYS + 0X1c8 + ((n) * 0x10))

//stat mem result
#define REG_STAT_AEM_RESULT_MEM(n)	(0x34000 + ((n) * 0x8000) + 0x50)
#define REG_STAT_AFC_RESULT_MEM(n)	(0x32000 + ((n) * 0x8000) + 0x2c)
#define REG_STAT_WBM_RESULT_MEM(n)	(0x32800 + ((n) * 0x8000) + 0x46c)
#define REG_STAT_LTM_RESULT_MEM(n)	(0x35000 + ((n) * 0x8000) + 0xc14)

#define REG_ISP_PDC_BASE(n)		(0x30100 + ((n) * 0x8000))

/*isp irq info*/
enum isp_host_irq_bit {
	ISP_IRQ_BIT_PIPE_SOF = 0,
	ISP_IRQ_BIT_PDC_SOF,
	ISP_IRQ_BIT_PDF_SOF,
	ISP_IRQ_BIT_BPC_SOF,
	ISP_IRQ_BIT_LSC_SOF,
	ISP_IRQ_BIT_DENOISE_SOF,
	ISP_IRQ_BIT_BINNING_SOF,
	ISP_IRQ_BIT_DEMOSAIC_SOF,
	ISP_IRQ_BIT_HDR_SOF,
	ISP_IRQ_BIT_LTM_SOF,
	ISP_IRQ_BIT_MCU_TRIGGER,
	ISP_IRQ_BIT_STAT_ERR,	// 11
	ISP_IRQ_BIT_SDE_SOF,	// 12
	ISP_IRQ_BIT_SDE_EOF,	// 13
	ISP_IRQ_BIT_RESET_DONE,
	ISP_IRQ_BIT_IDI_SHADOW_DONE,
	ISP_IRQ_BIT_PIPE_EOF,
	ISP_IRQ_BIT_PDC_EOF,
	ISP_IRQ_BIT_PDF_EOF,
	ISP_IRQ_BIT_BPC_EOF,
	ISP_IRQ_BIT_LSC_EOF,
	ISP_IRQ_BIT_DENOISE_EOF,
	ISP_IRQ_BIT_BINNING_EOF,
	ISP_IRQ_BIT_DEMOSAIC_EOF,
	ISP_IRQ_BIT_HDR_EOF,
	ISP_IRQ_BIT_LTM_EOF,	//25
	ISP_IRQ_BIT_AEM_EOF,
	ISP_IRQ_BIT_WBM_EOF,
	ISP_IRQ_BIT_LSCM_EOF,
	ISP_IRQ_BIT_AFC_EOF,	// 29
	ISP_IRQ_BIT_FICKER_EOF,
	ISP_IRQ_BIT_ISP_ERR,
	ISP_IRQ_BIT_MAX_NUM,
};

void x1isp_reg_set_base_addr(ulong __iomem base_reg_addr, ulong __iomem end_reg_addr);

/**
 * x1isp_reg_write_brust - write some registers together.
 * @reg_data: pointer to reg memory contains some struct isp_reg_unit, come from user space or ourself.
 * @reg_size: the number of struct isp_reg_unit.
 * @user_space: true if the regs come from user space.
 * @kvir_addr: the kernel virtual addr for reg memory which alloced by userspace. Only valid when
 *      user_space is true.
 *
 * The return values:
 * 0  : success.
 * <0 : failed.
 */
int x1isp_reg_write_brust(void *reg_data, u32 reg_size, bool user_space, void *kvir_addr);

int x1isp_reg_read_brust(struct isp_regs_info *regs_info);

int x1isp_reg_write_single(struct isp_reg_unit *reg_unit);

ulong x1isp_reg_readl(ulong __iomem addr);

int x1isp_reg_writel(ulong __iomem addr, ulong value, ulong mask);

#endif //X1_ISP_REG_H
