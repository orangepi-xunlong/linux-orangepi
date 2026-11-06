#ifndef	__CVI_SARADC_H__
#define	__CVI_SARADC_H__

#include <linux/cdev.h>
#include <linux/completion.h>
#include <linux/iio/iio.h>
#include <linux/list.h>
#include <linux/wait.h>

#define	SARADC_CTRL	  0x004	// control register
#define	SARADC_STATUS	  0x008	// staus  register
#define	SARADC_CYC_SET	  0x00c	// saradc waveform setting register
#define	SARADC_CH1_RESULT 0x014	// channel 1 result	register
#define	SARADC_CH2_RESULT 0x018	// channel 2 result	register
#define	SARADC_CH3_RESULT 0x01c	// channel 3 result	register
#define	SARADC_INTR_EN	  0x020	// interrupt enable	register
#define	SARADC_INTR_CLR	  0x024	// interrupt clear register
#define	SARADC_INTR_STA	  0x028	// interrupt status	register
#define	SARADC_INTR_RAW	  0x02c	// interrupt raw status	register

#define	SARADC_EN_SHIFT	 0x0
#define	SARADC_SEL_SHIFT 0x4
#define	SARADC_CHAN_NUM	 6
#define	SARADC_REGS_SIZE 0x2c +	4
#define	SARADC_REGS_NUM	 (SARADC_REGS_SIZE / 4)

struct cvi_saradc_device {
	struct device *dev;
	struct reset_control *rst_saradc;
	struct iio_chan_spec iio_channels[SARADC_CHAN_NUM];
	struct clk *clk_saradc;
	void __iomem *saradc_vaddr;
	void __iomem *top_saradc_base_addr;
	void __iomem *rtcsys_saradc_base_addr;
	int	saradc_irq;
	spinlock_t close_lock;
	bool enable[SARADC_CHAN_NUM];
	void *private_data;
	int	channel_index;
	uint32_t saradc_saved_top_regs[SARADC_REGS_NUM];
	uint32_t saradc_saved_rtc_regs[SARADC_REGS_NUM];
};

#endif /* __CVI_SARADC_H__ */