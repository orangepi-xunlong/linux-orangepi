// SPDX-License-Identifier: GPL-2.0-only
/*
 * Ky x1 qspi controller driver
 *
 * Copyright (c) 2023, ky Corporation.
 *
 */

#include <linux/bitops.h>
#include <linux/clk.h>
#include <linux/completion.h>
#include <linux/delay.h>
#include <linux/dmaengine.h>
#include <linux/dma-mapping.h>
#include <linux/err.h>
#include <linux/errno.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/iopoll.h>
#include <linux/jiffies.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/pm_qos.h>
#include <linux/pm_runtime.h>
#include <linux/sizes.h>
#include <linux/spi/spi.h>
#include <linux/spi/spi-mem.h>
#include <linux/reset.h>

//#define X1_DUMP_QSPI_REG

#define QSPI_WAIT_TIMEOUT		(300) /* ms */
#define QSPI_AUTOSUSPEND_TIMEOUT	2000
#define X1_MPMU_ACGR			0xd4051024

/* QSPI PMUap register */
#define PMUA_QSPI_CLK_RES_CTRL		0xd4282860
#define QSPI_CLK_SEL(x)			((x) << 6)
#define QSPI_CLK_SEL_MASK		GENMASK(8, 6)
#define QSPI_CLK_EN			BIT(4)
#define QSPI_BUS_CLK_EN			BIT(3)
#define QSPI_CLK_RST			BIT(1)
#define QSPI_BUS_RST			BIT(0)

/* QSPI memory base */
#define QSPI_AMBA_BASE			0xb8000000
#define QSPI_FLASH_A1_BASE		QSPI_AMBA_BASE
#define QSPI_FLASH_A1_TOP		(QSPI_FLASH_A1_BASE + 0x4000000)
#define QSPI_FLASH_A2_BASE		QSPI_FLASH_A1_TOP
#define QSPI_FLASH_A2_TOP		(QSPI_FLASH_A2_BASE + 0x100000)
#define QSPI_FLASH_B1_BASE		QSPI_FLASH_A2_TOP
#define QSPI_FLASH_B1_TOP		(QSPI_FLASH_B1_BASE + 0x100000)
#define QSPI_FLASH_B2_BASE		QSPI_FLASH_B1_TOP
#define QSPI_FLASH_B2_TOP		(QSPI_FLASH_B2_BASE + 0x100000)

/* TX/RX/ABH buffer max */
#define QSPI_RX_BUFF_MAX 		SZ_128
#define QSPI_TX_BUFF_MAX 		SZ_256
#define QSPI_TX_BUFF_POP_MIN 		16
#define QSPI_AHB_BUFF_MAX_SIZE		SZ_512
#define QSPI_TX_DMA_BURST 		SZ_16

#define QSPI_WAIT_BIT_CLEAR		0
#define QSPI_WAIT_BIT_SET		1

/* QSPI Host Registers used by the driver */
#define QSPI_MCR			0x00
#define QSPI_MCR_ISD_MASK		GENMASK(19, 16)
#define QSPI_MCR_MDIS_MASK		BIT(14)
#define QSPI_MCR_CLR_TXF_MASK		BIT(11)
#define QSPI_MCR_CLR_RXF_MASK		BIT(10)
#define QSPI_MCR_DDR_EN_MASK		BIT(7)
#define QSPI_MCR_END_CFG_MASK		GENMASK(3, 2)
#define QSPI_MCR_SWRSTHD_MASK		BIT(1)
#define QSPI_MCR_SWRSTSD_MASK		BIT(0)

#define QSPI_TCR			0x04
#define QSPI_IPCR			0x08
#define QSPI_IPCR_SEQID(x)		((x) << 24)

#define QSPI_FLSHCR			0x0c

#define QSPI_BUF0CR			0x10
#define QSPI_BUF1CR			0x14
#define QSPI_BUF2CR			0x18
#define QSPI_BUF3CR			0x1c
#define QSPI_BUF3CR_ALLMST_MASK		BIT(31)
#define QSPI_BUF3CR_ADATSZ(x)		((x) << 8)
#define QSPI_BUF3CR_ADATSZ_MASK		GENMASK(15, 8)

#define QSPI_BFGENCR			0x20
#define QSPI_BFGENCR_SEQID(x)		((x) << 12)

#define QSPI_SOCCR			0x24

#define QSPI_BUF0IND			0x30
#define QSPI_BUF1IND			0x34
#define QSPI_BUF2IND			0x38

#define QSPI_SFAR			0x100
#define QSPI_SFACR			0x104

#define QSPI_SMPR			0x108
#define QSPI_SMPR_DDRSMP_MASK		GENMASK(18, 16)
#define QSPI_SMPR_FSDLY_MASK		BIT(6)
#define QSPI_SMPR_FSPHS_MASK		BIT(5)
#define QSPI_SMPR_FSPHS_CLK		(416000000)
#define QSPI_SMPR_HSENA_MASK		BIT(0)

#define QSPI_RBSR			0x10c

#define QSPI_RBCT			0x110
#define QSPI_RBCT_WMRK_MASK		GENMASK(4, 0)
#define QSPI_RBCT_RXBRD_MASK		BIT(8)

#define QSPI_TBSR			0x150
#define QSPI_TBDR			0x154
#define QSPI_TBCT			0x158
#define QSPI_TX_WMRK			(QSPI_TX_DMA_BURST / 4 - 1)

#define QSPI_SR				0x15c
#define QSPI_SR_BUSY			BIT(0)
#define QSPI_SR_IP_ACC_MASK		BIT(1)
#define QSPI_SR_AHB_ACC_MASK		BIT(2)
#define QSPI_SR_TXFULL			BIT(27)

#define QSPI_FR				0x160
#define QSPI_FR_TFF_MASK		BIT(0)
#define QSPI_FR_IPGEF			BIT(4)
#define QSPI_FR_IPIEF			BIT(6)
#define QSPI_FR_IPAEF			BIT(7)
#define QSPI_FR_IUEF			BIT(11)
#define QSPI_FR_ABOF			BIT(12)
#define QSPI_FR_AIBSEF			BIT(13)
#define QSPI_FR_AITEF			BIT(14)
#define QSPI_FR_ABSEF			BIT(15)
#define QSPI_FR_RBDF			BIT(16)
#define QSPI_FR_RBOF			BIT(17)
#define QSPI_FR_ILLINE			BIT(23)
#define QSPI_FR_TBUF			BIT(26)
#define QSPI_FR_TBFF			BIT(27)
#define BUFFER_FR_FLAG			(QSPI_FR_ABOF| QSPI_FR_RBOF| \
					QSPI_FR_TBUF)

#define COMMAND_FR_FLAG			(QSPI_FR_ABSEF | QSPI_FR_AITEF | \
					QSPI_FR_AIBSEF | QSPI_FR_IUEF | \
					QSPI_FR_IPAEF |QSPI_FR_IPIEF | \
					QSPI_FR_IPGEF)

#define QSPI_RSER			0x164
#define QSPI_RSER_TFIE			BIT(0)
#define QSPI_RSER_IPGEIE		BIT(4)
#define QSPI_RSER_IPIEIE		BIT(6)
#define QSPI_RSER_IPAEIE		BIT(7)
#define QSPI_RSER_IUEIE			BIT(11)
#define QSPI_RSER_ABOIE			BIT(12)
#define QSPI_RSER_AIBSIE		BIT(13)
#define QSPI_RSER_AITIE			BIT(14)
#define QSPI_RSER_ABSEIE		BIT(15)
#define QSPI_RSER_RBDIE			BIT(16)
#define QSPI_RSER_RBOIE			BIT(17)
#define QSPI_RSER_RBDDE			BIT(21)
#define QSPI_RSER_ILLINIE		BIT(23)
#define QSPI_RSER_TBFDE			BIT(25)
#define QSPI_RSER_TBUIE			BIT(26)
#define QSPI_RSER_TBFIE			BIT(27)
#define BUFFER_ERROR_INT		(QSPI_RSER_ABOIE| QSPI_RSER_RBOIE| \
					QSPI_RSER_TBUIE)

#define COMMAND_ERROR_INT		(QSPI_RSER_ABSEIE | QSPI_RSER_AITIE | \
					QSPI_RSER_AIBSIE | QSPI_RSER_IUEIE | \
					QSPI_RSER_IPAEIE |QSPI_RSER_IPIEIE | \
					QSPI_RSER_IPGEIE)

#define QSPI_SPNDST			0x168
#define QSPI_SPTRCLR			0x16c
#define QSPI_SPTRCLR_IPPTRC		BIT(8)
#define QSPI_SPTRCLR_BFPTRC		BIT(0)

#define QSPI_SFA1AD			0x180
#define QSPI_SFA2AD			0x184
#define QSPI_SFB1AD			0x188
#define QSPI_SFB2AD			0x18c
#define QSPI_DLPR			0x190
#define QSPI_RBDR(x)			(0x200 + ((x) * 4))

#define QSPI_LUTKEY			0x300
#define QSPI_LUTKEY_VALUE		0x5af05af0

#define QSPI_LCKCR			0x304
#define QSPI_LCKER_LOCK			BIT(0)
#define QSPI_LCKER_UNLOCK		BIT(1)

#define QSPI_LUT_BASE			0x310
/* 16Bytes per sequence */
#define QSPI_LUT_REG(seqid, i)		(QSPI_LUT_BASE + (seqid) * 16 + (i) * 4)

/*
 * QSPI Sequence index.
 * index 0 is preset at boot for AHB read,
 * index 1 is used for other command.
 */
#define	SEQID_LUT_AHBREAD_ID		0
#define	SEQID_LUT_SHARED_ID		1

/* QSPI Instruction set for the LUT register */
#define LUT_INSTR_STOP			0
#define LUT_INSTR_CMD			1
#define LUT_INSTR_ADDR			2
#define LUT_INSTR_DUMMY			3
#define LUT_INSTR_MODE			4
#define LUT_INSTR_MODE2			5
#define LUT_INSTR_MODE4			6
#define LUT_INSTR_READ			7
#define LUT_INSTR_WRITE			8
#define LUT_INSTR_JMP_ON_CS		9
#define LUT_INSTR_ADDR_DDR		10
#define LUT_INSTR_MODE_DDR		11
#define LUT_INSTR_MODE2_DDR		12
#define LUT_INSTR_MODE4_DDR		13
#define LUT_INSTR_READ_DDR		14
#define LUT_INSTR_WRITE_DDR		15
#define LUT_INSTR_DATA_LEARN		16

/*
 * The PAD definitions for LUT register.
 *
 * The pad stands for the number of IO lines [0:3].
 * For example, the quad read needs four IO lines,
 * so you should use LUT_PAD(4).
 */
#define LUT_PAD(x) (fls(x) - 1)

/*
 * One sequence must be consisted of 4 LUT enteries(16Bytes).
 * LUT entries with the following register layout:
 * b'31                                                                     b'0
 *  ---------------------------------------------------------------------------
 *  |INSTR1[15~10]|PAD1[9~8]|OPRND1[7~0] | INSTR0[15~10]|PAD0[9~8]|OPRND0[7~0]|
 *  ---------------------------------------------------------------------------
 */
#define LUT_DEF(idx, ins, pad, opr)	\
	((((ins) << 10) | ((pad) << 8) | (opr)) << (((idx) & 0x1) * 16))

#define READ_FROM_CACHE_OP		0x03
#define READ_FROM_CACHE_OP_Fast		0x0b
#define READ_FROM_CACHE_OP_X2		0x3b
#define READ_FROM_CACHE_OP_X4		0x6b
#define READ_FROM_CACHE_OP_DUALIO	0xbb
#define READ_FROM_CACHE_OP_QUADIO	0xeb

u32 reg_offset_table[] = {
	QSPI_MCR,	QSPI_TCR,	QSPI_IPCR,	QSPI_FLSHCR,
	QSPI_BUF0CR,	QSPI_BUF1CR,	QSPI_BUF2CR,	QSPI_BUF3CR,
	QSPI_BFGENCR,	QSPI_SOCCR,	QSPI_BUF0IND,	QSPI_BUF1IND,
	QSPI_BUF2IND,	QSPI_SFAR,	QSPI_SFACR,	QSPI_SMPR,
	QSPI_RBSR,	QSPI_RBCT,	QSPI_TBSR,	QSPI_TBDR,
	QSPI_TBCT,	QSPI_SR,	QSPI_FR,	QSPI_RSER,
	QSPI_SPNDST,	QSPI_SPTRCLR,	QSPI_SFA1AD,	QSPI_SFA2AD,
	QSPI_SFB1AD,	QSPI_SFB2AD,	QSPI_DLPR,	QSPI_LUTKEY,
	QSPI_LCKCR
};

/* x1 qspi host priv */
struct x1_qspi {
	struct device *dev;
	struct spi_controller *ctrl;
	void __iomem *io_map;
	phys_addr_t io_phys;

	void __iomem *ahb_map;
	phys_addr_t memmap_base;
	u32 memmap_size;

	u32 sfa1ad;
	u32 sfa2ad;
	u32 sfb1ad;
	u32 sfb2ad;

	u32 pmuap_reg;
	void __iomem *pmuap_addr;
	u32 mpmu_acgr_reg;
	void __iomem *mpmu_acgr;

	u32 rx_buf_size;
	u32 tx_buf_size;
	u32 ahb_buf_size;
	u32 ahb_read_enable;
	u32 tx_unit_size;
	u32 rx_unit_size;

	u32 cmd_interrupt;
	u32 fr_error_flag;

	u32 tx_dma_enable;
	u32 tx_wmrk;
	struct dma_chan *tx_dma;
	struct dma_slave_config tx_dma_cfg;

	u32 rx_dma_enable;
	struct dma_chan *rx_dma;

	struct sg_table sgt;
	struct completion dma_completion;

	u32 cs_selected;
	u32 max_hz;
	u32 endian_xchg;
	u32 dma_enable;

	struct clk *clk, *bus_clk;
	struct reset_control *resets;

	struct completion cmd_completion;
	struct mutex lock;
	int selected;

	u32 tx_underrun_err;
	u32 rx_overflow_err;
	u32 ahb_overflow_err;
};

enum qpsi_cs {
	QSPI_CS_A1 = 0,
	QSPI_CS_A2,
	QSPI_CS_B1,
	QSPI_CS_B2,
	QSPI_CS_MAX,
};
#define QSPI_DEFAULT_CS		(QSPI_CS_A1)

enum qpsi_mode {
	QSPI_NORMAL_MODE = 0,
	QSPI_DISABLE_MODE,
	QSPI_STOP_MODE,
};

static ssize_t qspi_info_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct x1_qspi *t_qspi = dev_get_drvdata(dev);
	return sprintf(buf, "%s: rx_dma_en=%d, rx_buf_size=0x%x, tx_dma_en=%d, tx_buf_size=0x%x,"
				"ahb_read_enable=%d, ahb_buf_size=0x%x\n",
				dev_name(dev),
				t_qspi->rx_dma_enable, t_qspi->rx_buf_size,
				t_qspi->tx_dma_enable, t_qspi->tx_buf_size,
				t_qspi->ahb_read_enable, t_qspi->ahb_buf_size);
}
static DEVICE_ATTR_RO(qspi_info);

static ssize_t qspi_err_resp_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct x1_qspi *t_qspi = dev_get_drvdata(dev);
	return sprintf(buf, "%s: tx_underrun (%d), rx_overflow (%d), ahb_overflow (%d)\n",
			dev_name(dev),
			t_qspi->tx_underrun_err, t_qspi->rx_overflow_err, t_qspi->ahb_overflow_err);
}
static DEVICE_ATTR_RO(qspi_err_resp);

static struct attribute *qspi_dev_attrs[] = {
	&dev_attr_qspi_info.attr,
	&dev_attr_qspi_err_resp.attr,
	NULL,
};

static struct attribute_group qspi_dev_group = {
	.name = "qspi_dev",
	.attrs = qspi_dev_attrs,
};

static void qspi_writel(struct x1_qspi *qspi, u32 val, void __iomem *addr)
{
	if (qspi->endian_xchg)
		iowrite32be(val, addr);
	else
		iowrite32(val, addr);
}

static u32 qspi_readl(struct x1_qspi *qspi, void __iomem *addr)
{
	if (qspi->endian_xchg)
		return ioread32be(addr);
	else
		return ioread32(addr);
}

static int qspi_set_func_clk(struct x1_qspi *qspi)
{
	int ret = 0;

	qspi->clk = devm_clk_get(qspi->dev, "qspi_clk");
	if (IS_ERR_OR_NULL(qspi->clk)) {
		dev_err(qspi->dev, "can not find the clock\n");
		return -EINVAL;
	}

	qspi->bus_clk = devm_clk_get(qspi->dev, "qspi_bus_clk");
	if (IS_ERR_OR_NULL(qspi->bus_clk)) {
		dev_err(qspi->dev, "can not find the bus clock\n");
		return -EINVAL;
	}

	ret = clk_set_rate(qspi->clk, qspi->max_hz);
	if (ret) {
		dev_err(qspi->dev, "fail to set clk, ret:%d\n", ret);
		return ret;
	}

	ret = clk_prepare_enable(qspi->clk);
	if (ret) {
		dev_err(qspi->dev, "fail to enable clk, ret:%d\n", ret);
		return ret;
	}

	clk_prepare_enable(qspi->bus_clk);

	dev_dbg(qspi->dev, "bus clock %dHz, PMUap reg[0x%08x]:0x%08x\n",
		qspi->max_hz, qspi->pmuap_reg, qspi_readl(qspi, qspi->pmuap_addr));

	return 0;
}

static void qspi_config_mfp(struct x1_qspi *qspi)
{
	int cs = qspi->cs_selected;

	/* TODO: only for FPGA */
#if 0
	void * __iomem mfpr_base = ioremap((phys_addr_t)0xd401e000, 0x200);
	if (!mfpr_base) {
		pr_err("%s: ioremap mfpr reg error.\n", __func__);
		return;
	}

	if (cs == QSPI_CS_A1 || cs == QSPI_CS_A2) {
		writel(0xa800, mfpr_base + 0x174); // QSPI_DAT3
		writel(0xa800, mfpr_base + 0x170); // QSPI_DAT2
		writel(0xa800, mfpr_base + 0x16C); // QSPI_DAT1
		writel(0xa800, mfpr_base + 0x168); // QSPI_DAT0
		writel(0xa800, mfpr_base + 0x17C); // QSPI_CLK
		writel(0xc800, mfpr_base + 0x178); // QSPI_CS1
		dev_err(qspi->dev, "config mfp for cs:[%d]\n", cs);
	}
#endif
	dev_info(qspi->dev, "config mfp for cs:[%d]\n", cs);
}

static int x1_qspi_readl_poll_tout(struct x1_qspi *qspi, void __iomem *base,
					u32 mask, u32 timeout_us, u8 wait_set)
{
	u32 reg;

	if (qspi->endian_xchg)
		mask = swab32(mask);

	if (wait_set)
		return readl_poll_timeout(base, reg, (reg & mask), 10, timeout_us);
	else
		return readl_poll_timeout(base, reg, !(reg & mask), 10, timeout_us);
}

static void qspi_reset(struct x1_qspi *qspi)
{
	uint32_t reg;
	int err;

	/* QSPI_SR[QSPI_SR_BUSY] must be 0 */
	err = x1_qspi_readl_poll_tout(qspi, qspi->io_map + QSPI_SR,
			QSPI_SR_BUSY, QSPI_WAIT_TIMEOUT*1000, QSPI_WAIT_BIT_CLEAR);
	if (err) {
		dev_err(qspi->dev, "failed to reset qspi host.\n");
	} else {
		/* qspi softreset first */
		reg = qspi_readl(qspi, qspi->io_map + QSPI_MCR);
		reg |= QSPI_MCR_SWRSTHD_MASK | QSPI_MCR_SWRSTSD_MASK;
		qspi_writel(qspi, reg, qspi->io_map + QSPI_MCR);
		reg = qspi_readl(qspi, qspi->io_map + QSPI_MCR);
		if ((reg & 0x3) != 0x3)
			dev_info(qspi->dev, "reset ignored 0x%x.\n", reg);

		udelay(1);
		reg &= ~(QSPI_MCR_SWRSTHD_MASK | QSPI_MCR_SWRSTSD_MASK);
		qspi_writel(qspi, reg, qspi->io_map + QSPI_MCR);
	}
}


static void qspi_enter_mode(struct x1_qspi *qspi, uint32_t mode)
{
	uint32_t mcr;

	mcr = qspi_readl(qspi, qspi->io_map + QSPI_MCR);
	if (mode == QSPI_NORMAL_MODE)
		mcr &= ~QSPI_MCR_MDIS_MASK;
	else if (mode == QSPI_DISABLE_MODE)
		mcr |= QSPI_MCR_MDIS_MASK;
	qspi_writel(qspi, mcr, qspi->io_map + QSPI_MCR);
}

static void qspi_write_sfar(struct x1_qspi *qspi, uint32_t val)
{
	int err;

	/* QSPI_SR[IP_ACC] must be 0 */
	err = x1_qspi_readl_poll_tout(qspi, qspi->io_map + QSPI_SR,
			QSPI_SR_IP_ACC_MASK, QSPI_WAIT_TIMEOUT*1000, QSPI_WAIT_BIT_CLEAR);
	if (err)
		dev_err(qspi->dev, "failed to set QSPI_SFAR.\n");
	else
		qspi_writel(qspi, val, qspi->io_map + QSPI_SFAR);
}

/*
 * IP Command Trigger could not be executed Error Flag may happen for write
 * access to RBCT/SFAR register, need retry for these two register
 */
static void qspi_write_rbct(struct x1_qspi *qspi, uint32_t val)
{
	int err;

	/* QSPI_SR[IP_ACC] must be 0 */
	err = x1_qspi_readl_poll_tout(qspi, qspi->io_map + QSPI_SR,
			QSPI_SR_IP_ACC_MASK, QSPI_WAIT_TIMEOUT*1000, QSPI_WAIT_BIT_CLEAR);
	if (err)
		dev_err(qspi->dev, "failed to set QSPI_RBCT.\n");
	else
		qspi_writel(qspi, val, qspi->io_map + QSPI_RBCT);
}

void qspi_init_ahbread(struct x1_qspi *qspi, int seq_id)
{
	u32 buf_cfg = 0;

	/* Disable BUF0~BUF1, use BUF3 for all masters */
	qspi_writel(qspi, 0, qspi->io_map + QSPI_BUF0IND);
	qspi_writel(qspi, 0, qspi->io_map + QSPI_BUF1IND);
	qspi_writel(qspi, 0, qspi->io_map + QSPI_BUF2IND);

	buf_cfg = QSPI_BUF3CR_ALLMST_MASK |
			QSPI_BUF3CR_ADATSZ((qspi->ahb_buf_size / 8));

	/* AHB Master port */
	qspi_writel(qspi, 0xe, qspi->io_map + QSPI_BUF0CR);
	qspi_writel(qspi, 0xe, qspi->io_map + QSPI_BUF1CR);
	qspi_writel(qspi, 0xe, qspi->io_map + QSPI_BUF2CR);
	qspi_writel(qspi, buf_cfg, qspi->io_map + QSPI_BUF3CR); // other masters

	/* set AHB read sequence id */
	qspi_writel(qspi, QSPI_BFGENCR_SEQID(seq_id), qspi->io_map + QSPI_BFGENCR);
	dev_info(qspi->dev, "AHB buf size: %d\n", qspi->ahb_buf_size);
}

void qspi_dump_reg(struct x1_qspi *qspi)
{
	u32 reg = 0;
	void __iomem *base = qspi->io_map;
	int i;

	dev_notice(qspi->dev, "dump qspi host register:\n");
	for (i = 0; i < ARRAY_SIZE(reg_offset_table); i++) {
		if (i > 0 && (i % 4 == 0))
			dev_notice(qspi->dev, "\n");
		reg = qspi_readl(qspi, base + reg_offset_table[i]);
		dev_notice(qspi->dev, "offset[0x%03x]:0x%08x\t\t",
				reg_offset_table[i], reg);
	}

	dev_notice(qspi->dev, "\ndump AHB read LUT:\n");
	for (i = 0; i < 4; i++) {
		reg = qspi_readl(qspi, base + QSPI_LUT_REG(SEQID_LUT_AHBREAD_ID, i));
		dev_notice(qspi->dev, "lut_reg[0x%03x]:0x%08x\t\t",
				QSPI_LUT_REG(SEQID_LUT_AHBREAD_ID, i), reg);
	}

	dev_notice(qspi->dev, "\ndump shared LUT:\n");
	for (i = 0; i < 4; i++) {
		reg = qspi_readl(qspi, base + QSPI_LUT_REG(SEQID_LUT_SHARED_ID, i));
		dev_notice(qspi->dev, "lut_reg[0x%03x]:0x%08x\t\t",
				QSPI_LUT_REG(SEQID_LUT_SHARED_ID, i), reg);
	}
	dev_notice(qspi->dev, "\n");
}

/*
 * If the slave device content being changed by Write/Erase, need to
 * invalidate the AHB buffer. This can be achieved by doing the reset
 * of controller after setting MCR0[SWRESET] bit.
 */
static inline void x1_qspi_invalid(struct x1_qspi *qspi)
{
	u32 reg;

	reg = qspi_readl(qspi, qspi->io_map + QSPI_MCR);
	reg |= QSPI_MCR_SWRSTHD_MASK | QSPI_MCR_SWRSTSD_MASK;
	qspi_writel(qspi, reg, qspi->io_map + QSPI_MCR);

	/*
	 * The minimum delay : 1 AHB + 2 SFCK clocks.
	 * Delay 1 us is enough.
	 */
	udelay(1);

	reg &= ~(QSPI_MCR_SWRSTHD_MASK | QSPI_MCR_SWRSTSD_MASK);
	qspi_writel(qspi, reg, qspi->io_map + QSPI_MCR);
}

static void x1_qspi_prepare_lut(struct x1_qspi *qspi,
				const struct spi_mem_op *op, u32 seq_id)
{
	u32 lutval[4] = {0,};
	int lutidx = 0;
	int i;

	/* qspi cmd */
	lutval[0] |= LUT_DEF(lutidx, LUT_INSTR_CMD,
			     LUT_PAD(op->cmd.buswidth),
			     op->cmd.opcode);
	lutidx++;

	/* addr bytes */
	if (op->addr.nbytes) {
		lutval[lutidx / 2] |= LUT_DEF(lutidx, LUT_INSTR_ADDR,
					      LUT_PAD(op->addr.buswidth),
					      op->addr.nbytes * 8);
		lutidx++;
	}

	/* dummy bytes, if needed */
	if (op->dummy.nbytes) {
		lutval[lutidx / 2] |= LUT_DEF(lutidx, LUT_INSTR_DUMMY,
					      LUT_PAD(op->dummy.buswidth),
					      op->dummy.nbytes * 8 /
					      op->dummy.buswidth);
		lutidx++;
	}

	/* read/write data bytes */
	if (op->data.nbytes) {
		lutval[lutidx / 2] |= LUT_DEF(lutidx,
					      op->data.dir == SPI_MEM_DATA_IN ?
					      LUT_INSTR_READ : LUT_INSTR_WRITE,
					      LUT_PAD(op->data.buswidth),
					      0);
		lutidx++;
	}

	/* stop condition. */
	lutval[lutidx / 2] |= LUT_DEF(lutidx, LUT_INSTR_STOP, 0, 0);

	/* unlock LUT */
	qspi_writel(qspi, QSPI_LUTKEY_VALUE, qspi->io_map + QSPI_LUTKEY);
	qspi_writel(qspi, QSPI_LCKER_UNLOCK, qspi->io_map + QSPI_LCKCR);

	/* fill LUT register */
	for (i = 0; i < ARRAY_SIZE(lutval); i++)
		qspi_writel(qspi, lutval[i], qspi->io_map + QSPI_LUT_REG(seq_id, i));

	/* lock LUT */
	qspi_writel(qspi, QSPI_LUTKEY_VALUE, qspi->io_map + QSPI_LUTKEY);
	qspi_writel(qspi, QSPI_LCKER_LOCK, qspi->io_map + QSPI_LCKCR);

	dev_dbg(qspi->dev, "opcode:0x%x, lut_reg[0:0x%x, 1:0x%x, 2:0x%x, 3:0x%x]\n",
		op->cmd.opcode, lutval[0], lutval[1], lutval[2], lutval[3]);
}

static void x1_qspi_enable_interrupt(struct x1_qspi *qspi, u32 val)
{
	u32 resr = 0;

	resr = qspi_readl(qspi, qspi->io_map + QSPI_RSER);
	resr |= val;
	qspi_writel(qspi, resr, qspi->io_map + QSPI_RSER);
}

static void x1_qspi_disable_interrupt(struct x1_qspi *qspi, u32 val)
{
	u32 resr = 0;

	resr = qspi_readl(qspi, qspi->io_map + QSPI_RSER);
	resr &= ~val;
	qspi_writel(qspi, resr, qspi->io_map + QSPI_RSER);
}

static void x1_qspi_prepare_dma(struct x1_qspi *qspi)
{
	struct dma_slave_config dma_cfg;
	struct device *dev = qspi->dev;
	dma_cap_mask_t mask;

	if (qspi->rx_dma_enable) {
		/* RX DMA: DMA_MEMCPY type */
		dma_cap_zero(mask);
		dma_cap_set(DMA_MEMCPY, mask);
		qspi->rx_dma = dma_request_chan_by_mask(&mask);
		if (IS_ERR_OR_NULL(qspi->rx_dma)) {
			dev_err(dev, "rx dma request channel failed\n");
			qspi->rx_dma = NULL;
			qspi->rx_dma_enable = 0;
		} else {
			dev_dbg(dev, "rx dma enable, channel:%d\n", qspi->rx_dma->chan_id);
		}
	}

	if (qspi->tx_dma_enable) {
		/* TX DMA: DMA_SLAVE type */
		qspi->tx_dma = dma_request_slave_channel(dev, "tx-dma");
		if (qspi->tx_dma) {
			memset(&dma_cfg, 0, sizeof(struct dma_slave_config));
			dma_cfg.direction = DMA_MEM_TO_DEV;
			dma_cfg.dst_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
			dma_cfg.dst_addr = qspi->io_phys + QSPI_TBDR - 4;
			dma_cfg.dst_maxburst = QSPI_TX_DMA_BURST;
			if (dmaengine_slave_config(qspi->tx_dma, &dma_cfg)) {
				dev_err(dev, "tx dma slave config failed\n");
				dma_release_channel(qspi->tx_dma);
				qspi->tx_dma = NULL;
				qspi->tx_dma_enable = 0;
			} else {
				dev_dbg(dev, "tx dma enable, channel:%d\n", qspi->tx_dma->chan_id);
			}
		} else {
			qspi->tx_dma_enable = 0;
		}
	}

	if (qspi->tx_dma || qspi->rx_dma)
		init_completion(&qspi->dma_completion);
}

static void x1_qspi_dma_callback(void *arg)
{
	struct completion *dma_completion = arg;

	complete(dma_completion);
}

int x1_qspi_tx_dma_exec(struct x1_qspi *qspi,
			const struct spi_mem_op *op)
{
	struct dma_async_tx_descriptor *desc;
	enum dma_transfer_direction dma_dir;
	dma_cookie_t cookie;
	int err = 0;

	if (!virt_addr_valid(op->data.buf.in) ||
	    spi_controller_dma_map_mem_op_data(qspi->ctrl, op, &qspi->sgt)) {
		dev_err(qspi->dev, "tx dma spi_controller_dma_map_mem_op_data error\n");
		return -EIO;
	}

	dma_dir = DMA_MEM_TO_DEV;
	desc = dmaengine_prep_slave_sg(qspi->tx_dma, qspi->sgt.sgl, qspi->sgt.nents,
				       dma_dir, DMA_PREP_INTERRUPT);
	if (!desc) {
		dev_err(qspi->dev, "tx dma dmaengine_prep_slave_sg error\n");
		err = -ENOMEM;
		goto out;
	}

	reinit_completion(&qspi->dma_completion);
	desc->callback = x1_qspi_dma_callback;
	desc->callback_param = &qspi->dma_completion;

	cookie = dmaengine_submit(desc);
	err = dma_submit_error(cookie);
	if (err) {
		dev_err(qspi->dev, "tx dma dmaengine_submit error\n");
		goto out;
	}

	dma_async_issue_pending(qspi->tx_dma);

	return 0;
out:
	spi_controller_dma_unmap_mem_op_data(qspi->ctrl, op, &qspi->sgt);
	return err;
}

int x1_qspi_rx_dma_exec(struct x1_qspi *qspi, dma_addr_t dma_dst,
			dma_addr_t dma_src, size_t len)
{
	dma_cookie_t cookie;
	enum dma_ctrl_flags flags = DMA_CTRL_ACK | DMA_PREP_INTERRUPT;
	struct dma_async_tx_descriptor *desc;
	int ret;

	desc = dmaengine_prep_dma_memcpy(qspi->rx_dma, dma_dst, dma_src, len, flags);
	if (!desc) {
		dev_err(qspi->dev, "dmaengine_prep_dma_memcpy error\n");
		return -EIO;
	}

	reinit_completion(&qspi->dma_completion);
	desc->callback = x1_qspi_dma_callback;
	desc->callback_param = &qspi->dma_completion;
	cookie = dmaengine_submit(desc);
	ret = dma_submit_error(cookie);
	if (ret) {
		dev_err(qspi->dev, "dma_submit_error %d\n", cookie);
		return -EIO;
	}

	dma_async_issue_pending(qspi->rx_dma);
	ret = wait_for_completion_timeout(&qspi->dma_completion,
					  msecs_to_jiffies(len));
	if (ret <= 0) {
		dmaengine_terminate_sync(qspi->rx_dma);
		dev_err(qspi->dev, "DMA wait_for_completion_timeout\n");
		return -ETIMEDOUT;
	}

	return 0;
}

static int x1_qspi_rx_dma_sg(struct x1_qspi *qspi, struct sg_table rx_sg,
			       loff_t from)
{
	struct scatterlist *sg;
	dma_addr_t dma_src = qspi->memmap_base + from;
	dma_addr_t dma_dst;
	int i, len, ret;

	for_each_sg(rx_sg.sgl, sg, rx_sg.nents, i) {
		dma_dst = sg_dma_address(sg);
		len = sg_dma_len(sg);
		dev_dbg(qspi->dev, "rx dma, dst:0x%pad, src:0x%pad, len:%d\n",
			&dma_dst, &dma_src, len);
		ret = x1_qspi_rx_dma_exec(qspi, dma_dst, dma_src, len);
		if (ret)
			return ret;
		dma_src += len;
	}

	return 0;
}

static int x1_qspi_ahb_read(struct x1_qspi *qspi,
				const struct spi_mem_op *op)
{
	int ret = 0;
	u32 len = op->data.nbytes;
	u32 from = op->addr.val;
	struct sg_table sgt;

	/* Read out the data directly from the AHB buffer. */
	dev_dbg(qspi->dev, "ahb read %d bytes from address:0x%llx\n",
				len, (qspi->memmap_base + op->addr.val));
	if (from + len > qspi->memmap_size)
		return -ENOTSUPP;

	/* firstly try the DMA */
	if (qspi->rx_dma_enable) {
		if (virt_addr_valid(op->data.buf.in) &&
		    !spi_controller_dma_map_mem_op_data(qspi->ctrl, op, &sgt)) {
			ret = x1_qspi_rx_dma_sg(qspi, sgt, from);
			spi_controller_dma_unmap_mem_op_data(qspi->ctrl, op, &sgt);
		} else {
			ret = -EIO;
			dev_err(qspi->dev, "spi_controller_dma_map_mem_op_data error\n");
		}

		/* DMA completed */
		if (!ret)
			return 0;
	}

	if (qspi->rx_dma_enable && ret) {
		dev_dbg(qspi->dev, "rx dma read fallback to memcpy read.\n");
	}

	if (!qspi->rx_dma_enable || (qspi->rx_dma_enable && ret)) {
		memcpy(op->data.buf.in, (qspi->ahb_map + op->addr.val), len);
	}

	return 0;
}

static int x1_qspi_fill_txfifo(struct x1_qspi *qspi,
				 const struct spi_mem_op *op)
{
	void __iomem *base = qspi->io_map;
	int i;
	u32 val;
	u32 tbsr;
	u32 wait_cnt;

	if (!qspi->tx_dma_enable || (op->data.nbytes % QSPI_TX_BUFF_POP_MIN)) {
		qspi->tx_wmrk = 0;
		for (i = 0; i < ALIGN_DOWN(op->data.nbytes, 4); i += 4) {
			memcpy(&val, op->data.buf.out + i, 4);
			qspi_writel(qspi, val, base + QSPI_TBDR);
		}

		if (i < op->data.nbytes) {
			memcpy(&val, op->data.buf.out + i, op->data.nbytes - i);
			qspi_writel(qspi, val, base + QSPI_TBDR);
		}

		/*
		 * There must be at least 128bit data available in TX FIFO
		 * for any pop operation otherwise QSPI_FR[TBUF] will be set
		 */
		for (i = op->data.nbytes; i < ALIGN_DOWN(op->data.nbytes + (QSPI_TX_BUFF_POP_MIN - 1), QSPI_TX_BUFF_POP_MIN); i += 4) {
			qspi_writel(qspi, 0, base + QSPI_TBDR);
		}
	} else {
		/*
		 * Note that the number of bytes per DMA loop is determined
		 * by thee size of the QSPI_TBCT[WMRK].
		 * bytes per DMA loop = (QSPI_TBCT[WMRK] + 1) * 4.
		 * set QSPI_TX_WMRK as the TX watermark.
		 */
		qspi->tx_wmrk = QSPI_TX_WMRK;
		qspi_writel(qspi, qspi->tx_wmrk, base + QSPI_TBCT);

		/* config DMA channel and start */
		if (x1_qspi_tx_dma_exec(qspi, op)) {
			qspi->tx_wmrk = 0;
			dev_err(qspi->dev, "failed to start tx dma\n");
			return -EIO;
		}
		/* enable DMA request */
		x1_qspi_enable_interrupt(qspi, QSPI_RSER_TBFDE);

		/*
		 * before trigger qspi to send data to external bus, TX bufer
		 * need to have some data, or underrun error may happen.
		 * DMA need some time to write data to TX buffer, so add
		 * a delay here for this requirement.
		 */
		wait_cnt = 0;
		tbsr = qspi_readl(qspi, base + QSPI_TBSR);
		while (4 * (tbsr >> 16) < min_t(unsigned int, qspi->tx_buf_size, op->data.nbytes)) {
			udelay(1);
			tbsr = qspi_readl(qspi, base + QSPI_TBSR);
			if (wait_cnt++ >= 100) {
				msleep(100);
				tbsr = qspi_readl(qspi, base + QSPI_TBSR);
				if (4 * (tbsr >> 16) < min_t(unsigned int, qspi->tx_buf_size, op->data.nbytes)) {
					dev_err(qspi->dev, "tx dma failed to fill txbuf\n");
					/* disable all interrupts */
					qspi_writel(qspi, 0, qspi->io_map + QSPI_RSER);
					dmaengine_terminate_all(qspi->tx_dma);
					spi_controller_dma_unmap_mem_op_data(qspi->ctrl, op, &qspi->sgt);
					qspi->tx_wmrk = 0;

					return -EIO;
				} else {
					break;
				}
			}
		}
	}

	return 0;
}

static void x1_qspi_read_rxfifo(struct x1_qspi *qspi,
			  const struct spi_mem_op *op)
{
	void __iomem *base = qspi->io_map;
	int i;
	u8 *buf = op->data.buf.in;
	u32 val;

	dev_dbg(qspi->dev, "ip read %d bytes\n", op->data.nbytes);
	for (i = 0; i < ALIGN_DOWN(op->data.nbytes, 4); i += 4) {
		val = qspi_readl(qspi, base + QSPI_RBDR(i / 4));
		memcpy(buf + i, &val, 4);
	}

	if (i < op->data.nbytes) {
		val = qspi_readl(qspi, base + QSPI_RBDR(i / 4));
		memcpy(buf + i, &val, op->data.nbytes - i);
	}
}

static irqreturn_t x1_qspi_irq_handler(int irq, void *dev_id)
{
	struct x1_qspi *qspi = dev_id;
	u32 fr;

	/* disable all interrupts */
	qspi_writel(qspi, 0, qspi->io_map + QSPI_RSER);

	fr = qspi_readl(qspi, qspi->io_map + QSPI_FR);
	dev_dbg(qspi->dev, "QSPI_FR:0x%08x\n", fr);
	/* check QSPI_FR error flag */
	if (fr & (COMMAND_FR_FLAG | BUFFER_FR_FLAG)) {
		qspi->fr_error_flag = fr & (COMMAND_FR_FLAG | BUFFER_FR_FLAG);

		if (fr & QSPI_FR_IPGEF)
			dev_err(qspi->dev, "IP command trigger during AHB grant\n");
		if (fr & QSPI_FR_IPIEF)
			dev_err(qspi->dev, "IP command trigger could not be executed\n");
		if (fr & QSPI_FR_IPAEF)
			dev_err(qspi->dev, "IP command trigger during AHB access\n");
		if (fr & QSPI_FR_IUEF)
			dev_err(qspi->dev, "IP command usage error\n");
		if (fr & QSPI_FR_AIBSEF)
			dev_err(qspi->dev, "AHB illegal burst size error\n");
		if (fr & QSPI_FR_AITEF)
			dev_err(qspi->dev, "AHB illegal trancaction error\n");
		if (fr & QSPI_FR_ABSEF)
			dev_err(qspi->dev, "AHB sequence error\n");

		if (fr & QSPI_FR_TBUF) {
			/* disable TBFDE interrupt */
			x1_qspi_disable_interrupt(qspi, QSPI_RSER_TBFDE);
			dev_err_ratelimited(qspi->dev, "TX buffer underrun\n");
			qspi->tx_underrun_err++;
		}
		if (fr & QSPI_FR_RBOF) {
			dev_err(qspi->dev, "RX buffer overflow\n");
			qspi->rx_overflow_err++;
		}
		if (fr & QSPI_FR_ABOF) {
			dev_err(qspi->dev, "AHB buffer overflow\n");
			qspi->ahb_overflow_err++;
		}
	}

	if (qspi->cmd_interrupt && (fr & (QSPI_FR_TFF_MASK | COMMAND_FR_FLAG | BUFFER_FR_FLAG)))
		complete(&qspi->cmd_completion);

	return IRQ_HANDLED;
}

static int x1_qspi_do_op(struct x1_qspi *qspi, const struct spi_mem_op *op)
{
	void __iomem *base = qspi->io_map;
	int err = 0;
	u32 mcr;

	if (qspi->cmd_interrupt) {
		x1_qspi_enable_interrupt(qspi, QSPI_RSER_TFIE | BUFFER_ERROR_INT | COMMAND_ERROR_INT);
		init_completion(&qspi->cmd_completion);
	}

#ifdef X1_DUMP_QSPI_REG
	/* dump reg if need */
	qspi_dump_reg(qspi);
#endif
	/* trigger LUT */
	qspi_writel(qspi, op->data.nbytes | QSPI_IPCR_SEQID(SEQID_LUT_SHARED_ID),
		    base + QSPI_IPCR);

	/* wait for the transaction complete */
	if (qspi->cmd_interrupt) {
		wait_for_completion(&qspi->cmd_completion);
	} else {
		err = x1_qspi_readl_poll_tout(qspi, base + QSPI_FR, QSPI_FR_TFF_MASK,
						QSPI_WAIT_TIMEOUT*1000, QSPI_WAIT_BIT_SET);
	}
	if (err) {
		dev_err(qspi->dev, "opcode:0x%x transaction abort, ret:%d, error flag:0x%08x\n",
			op->cmd.opcode, err, qspi->fr_error_flag);
		dev_err(qspi->dev, "pmuap[0x%08x]:0x%08x\n", qspi->pmuap_reg, qspi_readl(qspi, qspi->pmuap_addr));
		dev_err(qspi->dev, "mpmu[0x%08x]:0x%08x\n", X1_MPMU_ACGR, qspi_readl(qspi, qspi->mpmu_acgr));
		qspi_dump_reg(qspi);
		goto tx_dma_unmap;
	}

	err = x1_qspi_readl_poll_tout(qspi, base + QSPI_SR, QSPI_SR_BUSY,
					QSPI_WAIT_TIMEOUT*1000, QSPI_WAIT_BIT_CLEAR);
	if (err) {
		dev_err(qspi->dev, "opcode:0x%x busy timeout, ret:%d\n", op->cmd.opcode, err);
		goto tx_dma_unmap;
	}

	/* read RX buffer for IP command read */
	if (op->data.nbytes && op->data.dir == SPI_MEM_DATA_IN) {
#ifdef X1_DUMP_QSPI_REG
		qspi_dump_reg(qspi);
#endif
		x1_qspi_read_rxfifo(qspi, op);
	}

	if (qspi->fr_error_flag & QSPI_FR_TBUF) {
		/* abort current dma transfer */
		if (qspi->tx_dma_enable)
			dmaengine_terminate_all(qspi->tx_dma);

		/* clear TX buf */
		mcr = qspi_readl(qspi, qspi->io_map + QSPI_MCR);
		mcr |= QSPI_MCR_CLR_TXF_MASK ;
		qspi_writel(qspi, mcr, qspi->io_map + QSPI_MCR);

		/* reduce tx unit size and retry */
		if (qspi->tx_dma_enable)
			qspi->tx_unit_size = qspi->tx_buf_size;

		err = -EAGAIN;
	} else {
		if (qspi->tx_dma_enable)
			qspi->tx_unit_size = qspi->tx_buf_size;
	}

tx_dma_unmap:
	if (qspi->tx_wmrk) {
		/* disable TBFDE interrupt and dma unmap */
		x1_qspi_disable_interrupt(qspi, QSPI_RSER_TBFDE);
		spi_controller_dma_unmap_mem_op_data(qspi->ctrl, op, &qspi->sgt);
		qspi->tx_wmrk = 0;
	}

	return err;
}

static void dump_spi_mem_op_info(struct x1_qspi *qspi,
				const struct spi_mem_op *op)
{
	dev_dbg(qspi->dev, "cmd.opcode:0x%x\n", op->cmd.opcode);
	dev_dbg(qspi->dev, "cmd.buswidth:%d\n", op->cmd.buswidth);
	dev_dbg(qspi->dev, "addr.nbytes:%d,\n", op->addr.nbytes);
	dev_dbg(qspi->dev, "addr.buswidth:%d\n", op->addr.buswidth);
	dev_dbg(qspi->dev, "addr.val:0x%llx\n", op->addr.val);
	dev_dbg(qspi->dev, "dummy.nbytes:%d\n", op->dummy.nbytes);
	dev_dbg(qspi->dev, "dummy.buswidth:%d\n", op->dummy.buswidth);
	dev_dbg(qspi->dev, "%s data.nbytes:%d\n",
		(op->data.dir == SPI_MEM_DATA_IN) ? "read" :"write",
		op->data.nbytes);
	dev_dbg(qspi->dev, "data.buswidth:%d\n", op->data.buswidth);
	dev_dbg(qspi->dev, "data.buf:0x%p\n", op->data.buf.in);
}

static int is_read_from_cache_opcode(u8 opcode)
{
	int ret;

	ret = ((opcode == READ_FROM_CACHE_OP) ||
		(opcode == READ_FROM_CACHE_OP_Fast) ||
		(opcode == READ_FROM_CACHE_OP_X2) ||
		(opcode == READ_FROM_CACHE_OP_X4) ||
		(opcode == READ_FROM_CACHE_OP_DUALIO) ||
		(opcode == READ_FROM_CACHE_OP_QUADIO));

	return ret;
}

static int x1_qspi_check_buswidth(struct x1_qspi *qspi, u8 width)
{
	switch (width) {
	case 1:
	case 2:
	case 4:
		return 0;
	}

	return -ENOTSUPP;
}

static bool x1_qspi_supports_op(struct spi_mem *mem,
				 const struct spi_mem_op *op)
{
	struct x1_qspi *qspi = spi_controller_get_devdata(mem->spi->master);
	int ret;

	mutex_lock(&qspi->lock);
	ret = x1_qspi_check_buswidth(qspi, op->cmd.buswidth);

	if (op->addr.nbytes)
		ret |= x1_qspi_check_buswidth(qspi, op->addr.buswidth);

	if (op->dummy.nbytes)
		ret |= x1_qspi_check_buswidth(qspi, op->dummy.buswidth);

	if (op->data.nbytes)
		ret |= x1_qspi_check_buswidth(qspi, op->data.buswidth);

	if (ret) {
		mutex_unlock(&qspi->lock);
		return false;
	}

	/* address bytes should be equal to or less than 4 bytes */
	if (op->addr.nbytes > 4) {
		mutex_unlock(&qspi->lock);
		return false;
	}

	/* check controller TX/RX buffer limits and alignment */
	if (op->data.dir == SPI_MEM_DATA_IN &&
	    (op->data.nbytes > qspi->rx_unit_size ||
	    (op->data.nbytes > qspi->rx_buf_size - 4 && !IS_ALIGNED(op->data.nbytes, 4)))) {
		mutex_unlock(&qspi->lock);
		return false;
	}

	if (op->data.dir == SPI_MEM_DATA_OUT && op->data.nbytes > qspi->tx_unit_size) {
		mutex_unlock(&qspi->lock);
		return false;
	}

	/*
	 * If requested address value is greater than controller assigned
	 * memory mapped space, return error as it didn't fit in the range.
	 */
	if (op->addr.val >= qspi->memmap_size) {
		pr_err("x1_qspi_supports_op: addr.val:%lld greater than the map size\n", op->addr.val);
		mutex_unlock(&qspi->lock);
		return false;
	}

	/* number of dummy clock cycles should be <= 64 cycles */
	if (op->dummy.buswidth &&
	    (op->dummy.nbytes * 8 / op->dummy.buswidth > 64)) {
		mutex_unlock(&qspi->lock);
		return false;
	}

	mutex_unlock(&qspi->lock);
	return true;
}

static const char *x1_qspi_get_name(struct spi_mem *mem)
{

	struct x1_qspi *qspi = spi_master_get_devdata(mem->spi->master);
	struct device *dev = qspi->dev;
	const char *name;

	name = devm_kasprintf(dev, GFP_KERNEL,
			      "%s-%d", dev_name(dev),
			      mem->spi->chip_select);

	if (!name) {
		dev_err(dev, "failed to get memory for custom flash name\n");
		return ERR_PTR(-ENOMEM);
	}

	return name;
}

static int x1_qspi_exec_op(struct spi_mem *mem, const struct spi_mem_op *op)
{
	struct x1_qspi *qspi = spi_controller_get_devdata(mem->spi->master);
	int err = 0;
	u32 mask;
	u32 reg;
	void __iomem *base;

	base = qspi->io_map;

	mutex_lock(&qspi->lock);

	dump_spi_mem_op_info(qspi, op);

	/* wait for controller being ready */
	mask = QSPI_SR_BUSY | QSPI_SR_IP_ACC_MASK | QSPI_SR_AHB_ACC_MASK;
	err = x1_qspi_readl_poll_tout(qspi, base + QSPI_SR, mask, QSPI_WAIT_TIMEOUT*1000, QSPI_WAIT_BIT_CLEAR);
	if (err) {
		dev_err(qspi->dev, "controller not ready!\n");
		dev_err(qspi->dev, "pmuap[0x%08x]:0x%08x\n", qspi->pmuap_reg, qspi_readl(qspi, qspi->pmuap_addr));
		dev_err(qspi->dev, "mpmu[0x%08x]:0x%08x\n", X1_MPMU_ACGR, qspi_readl(qspi, qspi->mpmu_acgr));
		qspi_dump_reg(qspi);
		mutex_unlock(&qspi->lock);
		return err;
	}

	/* clear TX/RX buffer before transaction */
	reg = qspi_readl(qspi, base + QSPI_MCR);
	reg |= QSPI_MCR_CLR_TXF_MASK | QSPI_MCR_CLR_RXF_MASK;
	qspi_writel(qspi, reg, base + QSPI_MCR);

	/*
	 * reset the sequence pointers whenever the sequence ID is changed by
	 * updating the SEDID filed in QSPI_IPCR OR QSPI_BFGENCR.
	 */
	reg = qspi_readl(qspi, base + QSPI_SPTRCLR);
	reg |= (QSPI_SPTRCLR_IPPTRC | QSPI_SPTRCLR_BFPTRC);
	qspi_writel(qspi, reg, base + QSPI_SPTRCLR);

	/* set the flash address into the QSPI_SFAR */
	qspi_write_sfar(qspi, qspi->memmap_base + op->addr.val);

	/* clear QSPI_FR before trigger LUT command */
	reg = qspi_readl(qspi, base + QSPI_FR);
	if (reg)
		qspi_writel(qspi, reg, base + QSPI_FR);
	qspi->fr_error_flag = 0;

	/*
	 * read page command 13h must be done by IP command.
	 * read from cache through the AHB bus by accessing the mapped memory.
	 * In all other cases we use IP commands to access the flash.
	 */
	if (op->data.nbytes > (qspi->rx_buf_size - 4) &&
		op->data.dir == SPI_MEM_DATA_IN &&
		qspi->ahb_read_enable &&
		is_read_from_cache_opcode(op->cmd.opcode)) {
		x1_qspi_prepare_lut(qspi, op, SEQID_LUT_AHBREAD_ID);
		err = x1_qspi_ahb_read(qspi, op);
	} else {
		/* IP command */
		x1_qspi_prepare_lut(qspi, op, SEQID_LUT_SHARED_ID);
		if (op->data.nbytes && op->data.dir == SPI_MEM_DATA_OUT) {
			err = x1_qspi_fill_txfifo(qspi, op);
		}
		if (!err)
			err = x1_qspi_do_op(qspi, op);
	}

	/* invalidate the data in the AHB buffer. */
	x1_qspi_invalid(qspi);

	mutex_unlock(&qspi->lock);

	return err;
}

static int x1_qspi_adjust_op_size(struct spi_mem *mem, struct spi_mem_op *op)
{
	struct x1_qspi *qspi = spi_controller_get_devdata(mem->spi->master);

	mutex_lock(&qspi->lock);
	if (op->data.dir == SPI_MEM_DATA_OUT) {
		if (op->data.nbytes > qspi->tx_unit_size)
			op->data.nbytes = qspi->tx_unit_size;
	} else {
		if (op->data.nbytes > qspi->rx_unit_size) {
			op->data.nbytes = qspi->rx_unit_size;
		} else if (op->data.nbytes > qspi->rx_buf_size - 4 && !IS_ALIGNED(op->data.nbytes, 4)) {
			op->data.nbytes = qspi->rx_buf_size - 4;
		}
	}
	mutex_unlock(&qspi->lock);

	return 0;
}

static int x1_qspi_host_init(struct x1_qspi *qspi)
{
	void __iomem *base = qspi->io_map;
	u32 reg;

	qspi->resets = devm_reset_control_array_get_optional_exclusive(qspi->dev);
	if (IS_ERR(qspi->resets)) {
		dev_err(qspi->dev, "Failed to get qspi's resets\n");
		return PTR_ERR(qspi->resets);
	}

	/* config mfp */
	qspi_config_mfp(qspi);

	reset_control_assert(qspi->resets);
	/* set PMUap */
	qspi_set_func_clk(qspi);
	reset_control_deassert(qspi->resets);

	/* rest qspi */
	qspi_reset(qspi);

	/* clock settings */
	qspi_enter_mode(qspi, QSPI_DISABLE_MODE);

	/* sampled by sfif_clk_b; half cycle delay; */
	if (qspi->max_hz < (QSPI_SMPR_FSPHS_CLK >> 2))
		qspi_writel(qspi, 0x0, base + QSPI_SMPR);
	else
		qspi_writel(qspi, QSPI_SMPR_FSPHS_MASK, base + QSPI_SMPR);

	/* Fix wirte failure issue*/
	qspi_writel(qspi, 0x8, base + QSPI_SOCCR);

	/* set the default source address QSPI_AMBA_BASE*/
	qspi_write_sfar(qspi, qspi->memmap_base);
	qspi_writel(qspi, 0x0, base + QSPI_SFACR);

	 /* config ahb read */
	qspi_init_ahbread(qspi, SEQID_LUT_AHBREAD_ID);

	/* set flash memory map */
	qspi_writel(qspi, qspi->sfa1ad & 0xfffffc00, base + QSPI_SFA1AD);
	qspi_writel(qspi, qspi->sfa2ad & 0xfffffc00, base + QSPI_SFA2AD);
	qspi_writel(qspi, qspi->sfb1ad & 0xfffffc00, base + QSPI_SFB1AD);
	qspi_writel(qspi, qspi->sfb2ad & 0xfffffc00, base + QSPI_SFB2AD);

	/* ISD3FB, ISD2FB, ISD3FA, ISD2FA = 1; END_CFG=0x3 */
	reg = qspi_readl(qspi, base + QSPI_MCR);
	reg |= QSPI_MCR_END_CFG_MASK | QSPI_MCR_ISD_MASK;
	qspi_writel(qspi, reg, base + QSPI_MCR);

	/* Module enabled */
	qspi_enter_mode(qspi, QSPI_NORMAL_MODE);

	/* Read using the IP Bus registers QSPI_RBDR0 to QSPI_RBDR31*/
	qspi_write_rbct(qspi, QSPI_RBCT_RXBRD_MASK);

	/* clear all interrupt status */
	qspi_writel(qspi, 0xffffffff, base + QSPI_FR);

	dev_dbg(qspi->dev, "qspi host init done.\n");
#ifdef X1_DUMP_QSPI_REG
	qspi_dump_reg(qspi);
#endif
	return 0;
}

static const struct spi_controller_mem_ops x1_qspi_mem_ops = {
	.adjust_op_size = x1_qspi_adjust_op_size,
	.supports_op = x1_qspi_supports_op,
	.exec_op = x1_qspi_exec_op,
	.get_name = x1_qspi_get_name,
};

static int x1_qspi_probe(struct platform_device *pdev)
{
	struct spi_controller *ctlr;
	struct device *dev = &pdev->dev;
	struct device_node *np = dev->of_node;
	struct x1_qspi *qspi;
	struct resource *res;

	int ret = 0;
	u32 qspi_bus_num = 0;
	int host_irq = 0;

	ctlr = spi_alloc_master(&pdev->dev, sizeof(struct x1_qspi));
	if (!ctlr)
		return -ENOMEM;

	ctlr->mode_bits = SPI_RX_DUAL | SPI_RX_QUAD | SPI_TX_DUAL | SPI_TX_QUAD ;
	qspi = spi_controller_get_devdata(ctlr);
	qspi->dev = dev;
	qspi->ctrl = ctlr;

	platform_set_drvdata(pdev, qspi);

	/* get qspi frequency */
	if (of_property_read_u32(dev->of_node, "x1,qspi-freq", &qspi->max_hz)) {
		dev_err(dev, "failed to get qspi frequency\n");
		goto err_put_ctrl;
	}

	/* get qspi register base address */
	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "qspi-base");
	qspi->io_map = devm_ioremap_resource(dev, res);
	if (IS_ERR(qspi->io_map)) {
		ret = PTR_ERR(qspi->io_map);
		goto err_put_ctrl;
	}
	qspi->io_phys = res->start;

	/* get qspi memory-map address */
	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "qspi-mmap");
	qspi->ahb_map = devm_ioremap_resource(dev, res);
	if (IS_ERR(qspi->ahb_map)) {
		ret = PTR_ERR(qspi->ahb_map);
		goto err_put_ctrl;
	}

	qspi->memmap_base = res->start;
	qspi->memmap_size = resource_size(res);

	if (of_property_read_u32(dev->of_node, "x1,qspi-sfa1ad", &qspi->sfa1ad))
		qspi->sfa1ad = QSPI_FLASH_A1_TOP;
	else
		qspi->sfa1ad += qspi->memmap_base;
	if (of_property_read_u32(dev->of_node, "x1,qspi-sfa2ad", &qspi->sfa2ad))
		qspi->sfa2ad = QSPI_FLASH_A2_TOP;
	else
		qspi->sfa2ad += qspi->sfa1ad;
	if (of_property_read_u32(dev->of_node, "x1,qspi-sfb1ad", &qspi->sfb1ad))
		qspi->sfb1ad = QSPI_FLASH_B1_TOP;
	else
		qspi->sfb1ad = qspi->sfa2ad;
	if (of_property_read_u32(dev->of_node, "x1,qspi-sfb2ad", &qspi->sfb2ad))
		qspi->sfb2ad = QSPI_FLASH_B2_TOP;
	else
		qspi->sfb2ad += qspi->sfb1ad;

	dev_dbg(dev, "x1_qspi_probe:memmap base:0x%pa, memmap size:0x%x\n",
			&qspi->memmap_base, qspi->memmap_size);

	host_irq = platform_get_irq(pdev, 0);
	if (host_irq < 0) {
		dev_err(dev, "invalid host irq:%d\n", host_irq);
		goto err_put_ctrl;
	}
	ret = devm_request_irq(dev, host_irq, x1_qspi_irq_handler,
				0, pdev->name, qspi);
	if (ret) {
		dev_err(dev, "failed to request irq:%d\n", ret);
		goto err_put_ctrl;
	}
	init_completion(&qspi->cmd_completion);
	dev_dbg(qspi->dev, "x1_qspi_probe: host_irq:%d\n", host_irq);

	/* map QSPI PMUap register address */
	if (of_property_read_u32(dev->of_node, "x1,qspi-pmuap-reg", &qspi->pmuap_reg)) {
		qspi->pmuap_reg = PMUA_QSPI_CLK_RES_CTRL;
	}
	qspi->pmuap_addr = ioremap(qspi->pmuap_reg, 4);

	/* map QSPI MPMU ACGR register address */
	if (of_property_read_u32(dev->of_node, "x1,qspi-mpmu-acgr-reg", &qspi->mpmu_acgr_reg)) {
		qspi->mpmu_acgr_reg = X1_MPMU_ACGR;
	}
	qspi->mpmu_acgr = ioremap(qspi->mpmu_acgr_reg, 4);

	if (of_property_read_u32(dev->of_node, "x1,qspi-rx-buf", &qspi->rx_buf_size)) {
		qspi->rx_buf_size = QSPI_RX_BUFF_MAX;
	}

	if (of_property_read_u32(dev->of_node, "x1,qspi-tx-buf", &qspi->tx_buf_size)) {
		qspi->tx_buf_size = QSPI_TX_BUFF_MAX;
	}

	if (of_property_read_u32(dev->of_node, "x1,qspi-ahb-buf", &qspi->ahb_buf_size)) {
		qspi->ahb_buf_size = QSPI_AHB_BUFF_MAX_SIZE;
	}

	if (of_property_read_u32(dev->of_node, "x1,qspi-ahb-enable", &qspi->ahb_read_enable)) {
		qspi->ahb_read_enable = 1;
	}

	if (of_property_read_u32(dev->of_node, "x1,qspi-interrupt", &qspi->cmd_interrupt)) {
		qspi->cmd_interrupt = 1;
	}

	if (of_property_read_u32(dev->of_node, "x1,qspi-endian-xchg", &qspi->endian_xchg)) {
		qspi->endian_xchg = 0;
	}

	if (of_property_read_u32(dev->of_node, "x1,qspi-cs", &qspi->cs_selected)) {
		qspi->cs_selected = QSPI_DEFAULT_CS;
	}

	if (of_property_read_u32(dev->of_node, "x1,qspi-tx-dma", &qspi->tx_dma_enable)) {
		qspi->tx_dma_enable = 0;
	}

	if (of_property_read_u32(dev->of_node, "x1,qspi-rx-dma", &qspi->rx_dma_enable)) {
		qspi->rx_dma_enable = 0;
	}

	x1_qspi_prepare_dma(qspi);
	mutex_init(&qspi->lock);

	/* set the qspi device default index */
	if (of_property_read_u32(dev->of_node, "x1,qspi-id", &qspi_bus_num))
		ctlr->bus_num = 0;
	else
		ctlr->bus_num = qspi_bus_num;
	ctlr->num_chipselect = 1;
	ctlr->mem_ops = &x1_qspi_mem_ops;

	dev_dbg(dev, "x1_qspi_probe: rx_buf_size:%d, tx_buf_size:%d\n",
			qspi->rx_buf_size, qspi->tx_buf_size);
	dev_dbg(dev, "x1_qspi_probe: ahb_buf_size:%d, ahb_read:%d\n",
			qspi->ahb_buf_size, qspi->ahb_read_enable);

	if (qspi->tx_dma_enable)
		qspi->tx_unit_size = qspi->tx_buf_size;
	else
		qspi->tx_unit_size = qspi->tx_buf_size;

	if (qspi->ahb_read_enable)
		qspi->rx_unit_size = SZ_4K;
	else
		qspi->rx_unit_size = qspi->rx_buf_size;
	x1_qspi_host_init(qspi);

	pm_runtime_use_autosuspend(&pdev->dev);
	pm_runtime_set_autosuspend_delay(&pdev->dev, QSPI_AUTOSUSPEND_TIMEOUT);
	pm_suspend_ignore_children(&pdev->dev, 1);
	pm_runtime_enable(&pdev->dev);
	pm_runtime_set_active(&pdev->dev);

	ctlr->dev.of_node = np;
	ctlr->dev.parent = &pdev->dev;
	ctlr->use_gpio_descriptors = true;
	ctlr->auto_runtime_pm = true;
	ret = spi_register_controller(ctlr);
	if (ret)
		goto err_destroy_mutex;

	pm_runtime_put_autosuspend(&pdev->dev);

#ifdef CONFIG_SYSFS
	ret = sysfs_create_group(&(pdev->dev.kobj),
			(const struct attribute_group *)(&qspi_dev_group));
	if (ret) {
		dev_err(dev,
			"failed to create attr group for qspi dev!\n");
		goto err_destroy_mutex;
	}
#endif

	return 0;

err_destroy_mutex:
	pm_runtime_disable(&pdev->dev);
	pm_runtime_put_noidle(&pdev->dev);

	mutex_destroy(&qspi->lock);
	iounmap(qspi->pmuap_addr);

err_put_ctrl:
	spi_controller_put(ctlr);

	dev_err(dev, "X1 QSPI probe failed\n");
	return ret;
}

static int x1_qspi_remove(struct platform_device *pdev)
{
	struct x1_qspi *qspi = platform_get_drvdata(pdev);

	pm_runtime_get_sync(&pdev->dev);

	/* set disable mode */
	qspi_writel(qspi, QSPI_MCR_MDIS_MASK, qspi->io_map + QSPI_MCR);
	qspi_writel(qspi, 0x0, qspi->io_map + QSPI_RSER);

	pm_runtime_disable(&pdev->dev);
	pm_runtime_put_noidle(&pdev->dev);

	if (qspi->tx_dma)
		dma_release_channel(qspi->tx_dma);
	if (qspi->rx_dma)
		dma_release_channel(qspi->rx_dma);

	mutex_destroy(&qspi->lock);
	iounmap(qspi->pmuap_addr);

	reset_control_assert(qspi->resets);
	clk_disable_unprepare(qspi->clk);
	clk_disable_unprepare(qspi->bus_clk);

#ifdef CONFIG_SYSFS
	sysfs_remove_group(&(pdev->dev.kobj),
			(const struct attribute_group *)(&qspi_dev_group));
#endif
	return 0;
}

#ifdef CONFIG_PM_SLEEP
static int x1_qspi_suspend(struct device *dev)
{
	int ret;
	u32 sr;
	struct x1_qspi *qspi = dev_get_drvdata(dev);

	pm_runtime_get_sync(qspi->dev);

	sr = qspi_readl(qspi, qspi->io_map + QSPI_SR);
	if (sr & QSPI_SR_BUSY) {
		dev_err(dev, "qspi busy with ongoing cmd\n");
		return -EBUSY;
	}

	ret = pm_runtime_force_suspend(dev);
	if (ret) {
		dev_err(dev, "failed to suspend(ret:%d)\n", ret);
		return ret;
	}

	return 0;
}

static int x1_qspi_resume(struct device *dev)
{
	int ret;

	ret = pm_runtime_force_resume(dev);
	if (ret) {
		dev_err(dev, "failed to resume(ret:%d)\n", ret);
		return ret;
	}

	pm_runtime_mark_last_busy(dev);
	pm_runtime_put_autosuspend(dev);

	return 0;
}
#endif

#ifdef CONFIG_PM
static int x1_qspi_runtime_suspend(struct device *dev)
{
	u32 sr;
	struct x1_qspi *qspi = dev_get_drvdata(dev);

	mutex_lock(&qspi->lock);
	sr = qspi_readl(qspi, qspi->io_map + QSPI_SR);
	if (sr & QSPI_SR_BUSY) {
		dev_err(dev, "qspi busy with ongoing cmd\n");
		mutex_unlock(&qspi->lock);
		return -EBUSY;
	}
	qspi_enter_mode(qspi, QSPI_DISABLE_MODE);
	mutex_unlock(&qspi->lock);

	return 0;
}

static int x1_qspi_runtime_resume(struct device *dev)
{
	struct x1_qspi *qspi = dev_get_drvdata(dev);

	qspi_enter_mode(qspi, QSPI_NORMAL_MODE);

	return 0;
}

static const struct dev_pm_ops x1_qspi_pmops = {
	SET_SYSTEM_SLEEP_PM_OPS(x1_qspi_suspend, x1_qspi_resume)
	SET_RUNTIME_PM_OPS(x1_qspi_runtime_suspend,
		x1_qspi_runtime_resume, NULL)
};

#define X1_QSPI_PMOPS (&x1_qspi_pmops)

#else
#define X1_QSPI_PMOPS NULL
#endif

static const struct of_device_id x1_qspi_dt_ids[] = {
	{ .compatible = "ky,x1-qspi", },
	{}
};
MODULE_DEVICE_TABLE(of, x1_qspi_dt_ids);

static struct platform_driver x1_qspi_driver = {
	.driver = {
		.name	= "x1-qspi",
		.of_match_table = x1_qspi_dt_ids,
		.pm = X1_QSPI_PMOPS,
	},
	.probe          = x1_qspi_probe,
	.remove		= x1_qspi_remove,
};
module_platform_driver(x1_qspi_driver);

MODULE_AUTHOR("Ky");
MODULE_DESCRIPTION("Ky x1 qspi controller driver");
MODULE_LICENSE("GPL v2");
