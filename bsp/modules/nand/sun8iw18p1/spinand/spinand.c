/*
 * spinand.c for  SUNXI NAND .
 *
 * Copyright (C) 2016 Allwinner.
 *
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

/**
 * spinand2 feature:
 *
 * 1. Complete and continuous ECC protected area
 * 2. suitable for 2 or 3 bit ecc status. (see function spi_nand_read_status)
 * 3. driver register:
 *	0: 50% (default)
 *	1: 25%
 *	2: 75%
 *	3: 100%
 * 4. if use quad mode, must enable QE
 */
#if 0//#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 0, 0))
#include <mach/sunxi_spinand.h>
#endif
#include "spinand_ecc_op.h"

#define STAT_MODE_READ 0
/* Just for compatibility of function PHY_Wait_Status() */
#define STAT_MODE_ERASE_WRITE 1
#define STAT_MODE_WRITE 3
#define STAT_MODE_ERASE 4

extern struct __NandStorageInfo_t  NandStorageInfo;
extern struct __NandPageCachePool_t PageCachePool;
extern int NAND_Print(const char *fmt, ...);
extern void NAND_Memcpy(void *pAddr_dst, void *pAddr_src, unsigned int len);
extern void NAND_Memset(void *pAddr, unsigned char value, unsigned int len);


#define PHY_ERR(...) NAND_Print(__VA_ARGS__)

///////////////

unsigned int cal_addr_in_chip(unsigned int block, unsigned int page)
{
	return block * NandStorageInfo.PageCntPerPhyBlk + page;
}

unsigned int cal_first_valid_bit(unsigned int secbitmap)
{
	unsigned int firstbit = 0;
	u8 i = 0;

	while (1) {
		if (secbitmap & (0x1 << i)) {
			firstbit = i;
			break;
		}
		i++;
		if (i == 32)
			break;
	}

	return firstbit;
}

unsigned int cal_valid_bits(unsigned int secbitmap)
{
	unsigned int validbit = 0;

	while (secbitmap) {
		if (secbitmap & 0x1)
			validbit++;
		secbitmap >>= 1;
	}

	return validbit;
}
void spic_select_ss(unsigned int spi_no, unsigned int ssx)
{
	unsigned int rval = readw(SPI_TCR) & (~(3 << 4));
	rval |= ssx << 4;
	writew(rval, SPI_TCR);
}

void spic_config_dual_mode(unsigned int spi_no, unsigned int rxdual, unsigned int dbc, unsigned int stc)
{
	writew((rxdual<<28)|(dbc<<24)|(stc), SPI_BCC);
}


__s32 wait_sfer_complete(void)
{
	unsigned int timeout = 0xffffff;

	while (!(readw(SPI_ISR)&(0x1<<12))) {//wait transfer complete
		timeout--;
		if (!timeout)
			break;
	}
	if (timeout == 0) {
		NAND_Print("TC Complete wait status timeout!\n");
		return -ERR_TIMEOUT;
	}

	return 0;
}

////


__s32 spic0_rw(__u32 spi_no, __u32 tcnt, __u8 *txbuf, __u32 rcnt, __u8 *rxbuf,
		__u32 dummy_cnt)
{
	__u32 xcnt = 0, fcr;
	__u32 tx_dma_flag = 0;
	__u32 rx_dma_flag = 0;
	__s32 timeout = 0xffff;
	__s32 ret_dma_config = 0;
	__s32 ret = NAND_OP_TRUE;

	writew(0, SPI_IER);
	writew(0xffffffff, SPI_ISR);//clear status register

	writew(tcnt, SPI_MTC);
	writew(tcnt+rcnt+dummy_cnt, SPI_MBC);

	/* start transmit */
	writew(readw(SPI_TCR) | SPI_EXCHANGE, SPI_TCR);
	if (tcnt) {
		if (tcnt <= 64)
			tx_dma_flag = 0;
		else
			tx_dma_flag = 1;


		if (tx_dma_flag == 1) {
			writew((readw(SPI_FCR)|SPI_TXDMAREQ_EN), SPI_FCR);
			ret_dma_config = nand_dma_config_start(1, (__u32) txbuf, tcnt);
			if (ret_dma_config != 0) {
				PHY_ERR("Spic_rw: tx nand_dma_config_start fail!\n");
				writew((readw(SPI_FCR)&~SPI_TXDMAREQ_EN), SPI_FCR);
				tx_dma_flag = 0;  /* clear dma flag, to try use cpu */
			}
		}

		if (tx_dma_flag == 0) {
			xcnt = 0;
			timeout = 0xfffff;
			while (xcnt < tcnt) {
				while (((readw(SPI_FSR) >> 16) & 0x7f) >= SPI_FIFO_SIZE) {
					if (--timeout < 0) {
						PHY_ERR("Spic_rw: cpu send data timeout!\n");
						ret = ERR_TIMEOUT;
						goto out;
					}
				}
				writeb(*(txbuf + xcnt), SPI_TXD);
				xcnt++;
			}
		}
	}

	if (rcnt) {
		if (rcnt <= 64)
			rx_dma_flag = 0;
		else
			rx_dma_flag = 1;

		if (rx_dma_flag == 1) {
			writew((readw(SPI_FCR)|SPI_RXDMAREQ_EN), SPI_FCR);
			ret_dma_config = nand_dma_config_start(0, (__u32) rxbuf, rcnt);
			if (ret_dma_config != 0) {
				PHY_ERR("Spic_rw: rx nand_dma_config_start fail!\n");
				writew((readw(SPI_FCR)&~SPI_RXDMAREQ_EN), SPI_FCR);
				rx_dma_flag = 0;  /* clear dma flag, to try use cpu */
			}
		}

		if (rx_dma_flag == 0) {
			xcnt = 0;
			timeout = 0xfffff;
			while (xcnt < rcnt) {
				if (((readw(SPI_FSR)) & 0x7f) && (--timeout > 0)) {
					*(rxbuf + xcnt) = readb(SPI_RXD);
					xcnt++;
				}
			}
			if (timeout <= 0) {
				PHY_ERR("Spic_rw: cpu receive data timeout!\n");
				ret = ERR_TIMEOUT;
				goto out;
			}
		}
	}

	if (NAND_WaitDmaFinish(tx_dma_flag, rx_dma_flag)) {
		PHY_ERR("Spic_rw: DMA wait status timeout!\n");
		ret = ERR_TIMEOUT;
		goto out;
	}

	if (wait_sfer_complete()) {
		PHY_ERR("Spic_rw: wait tc complete timeout!\n");
		ret = ERR_TIMEOUT;
		goto out;
	}

	if (readw(SPI_ISR) & (0xf << 8)) {
		PHY_ERR("Spic_rw: FIFO status error: 0x%x!\n", readw(SPI_ISR));
		ret = NAND_OP_FALSE;
		goto out;
	}

	if (readw(SPI_TCR) & SPI_EXCHANGE)
		PHY_ERR("Spic_rw: XCH Control Error!!\n");

out:
	/* clear spi reg */
	fcr = readw(SPI_FCR);
	fcr &= ~(SPI_TXDMAREQ_EN|SPI_RXDMAREQ_EN);
	writew(fcr, SPI_FCR);

	writew(0xffffffff, SPI_ISR); /* clear flag */

	/* release dma resource */
	if (tx_dma_flag)
		Nand_Dma_End(1, (__u32) txbuf, tcnt);

	if (rx_dma_flag)
		Nand_Dma_End(0, (__u32) rxbuf, rcnt);

	return ret;
}




////
#if 0
__s32 spic0_rw(unsigned int spi_no, unsigned int tcnt, __u8 *txbuf, unsigned int rcnt, __u8 *rxbuf, unsigned int dummy_cnt)
{
	unsigned int i = 0, fcr;
	unsigned int tx_dma_flag = 0;
	unsigned int rx_dma_flag = 0;
	__s32 timeout = 0xffff;

	writew(0, SPI_IER);
	writew(0xffffffff, SPI_ISR);//clear status register

	writew(tcnt, SPI_MTC);
	writew(tcnt+rcnt+dummy_cnt, SPI_MBC);

	//read and write by cpu operation
	if (tcnt) {
		if (tcnt <= 64) {
			i = 0;
			while (i < tcnt) {
				//send data
				if (((readw(SPI_FSR)>>16) & 0x7f) == SPI_FIFO_SIZE)
					NAND_Print("TX FIFO size error!\n");
				writeb(*(txbuf+i), SPI_TXD);
				i++;
			}
		} else {
			tx_dma_flag = 1;

			writew((readw(SPI_FCR)|SPI_TXDMAREQ_EN), SPI_FCR);

			nand_dma_config_start(1, (unsigned int) txbuf, tcnt);
		}
	}
	/* start transmit */
	writew(readw(SPI_TCR)|SPI_EXCHANGE, SPI_TCR);
	if (rcnt) {
		if (rcnt <= 64) {
			i = 0;
			timeout = 0xfffff;
			while (1) {
				if (((readw(SPI_FSR))&0x7f) == rcnt)
					break;
				if (timeout < 0) {
					NAND_Print("RX FIFO size error,timeout!\n");
					break;
				}
				timeout--;
			}
			while (i < rcnt) {
				//receive valid data
				//while(((readw(SPI_FSR))&0x7f)==0);
				//while((((readw(SPI_FSR))&0x7f)!=rcnt)||(timeout < 0))
				//	NAND_Print("RX FIFO size error!\n");
				*(rxbuf+i) = readb(SPI_RXD);
				i++;
			}
		} else {
			rx_dma_flag = 1;

			writew((readw(SPI_FCR)|SPI_RXDMAREQ_EN), SPI_FCR);

			nand_dma_config_start(0, (unsigned int) rxbuf, rcnt);
		}
	}

	if (NAND_WaitDmaFinish(tx_dma_flag, rx_dma_flag)) {
		NAND_Print("DMA wait status timeout!\n");
		return -ERR_TIMEOUT;
	}

	if (wait_sfer_complete()) {
		NAND_Print("wait tc complete timeout!\n");
		return -ERR_TIMEOUT;
	}

	if (tx_dma_flag)
		Nand_Dma_End(1, (unsigned int) txbuf, tcnt);

	if (rx_dma_flag)
		Nand_Dma_End(0, (unsigned int) rxbuf, rcnt);

	fcr = readw(SPI_FCR);
	fcr &= ~(SPI_TXDMAREQ_EN|SPI_RXDMAREQ_EN);
	writew(fcr, SPI_FCR);
	if (readw(SPI_ISR) & (0xf << 8)) {	/* (1U << 11) | (1U << 10) | (1U << 9) | (1U << 8)) */
		NAND_Print("FIFO status error: 0x%x!\n", readw(SPI_ISR));
		return NAND_OP_FALSE;
	}

	if (readw(SPI_TCR) & SPI_EXCHANGE) {
		NAND_Print("XCH Control Error!!\n");
	}

	writew(0xffffffff, SPI_ISR);  /* clear  flag */
	return NAND_OP_TRUE;
}
#endif
////////////////
__s32 spinand_get(unsigned int spi_no, unsigned int chip, __u8 addr, __u8 *reg)
{
	__u8 sdata[2];
	unsigned int txnum;
	unsigned int rxnum;

	txnum = 2;
	rxnum = 1;

	sdata[0] = SPI_NAND_GETSR;
	sdata[1] = addr;

	spic_select_ss(spi_no, chip);

	spic_config_dual_mode(spi_no, 0, 0, txnum);
	return spic0_rw(spi_no, txnum, (void *)sdata, rxnum, (void *)reg, 0);
}

__s32 spinand_set(unsigned int spi_no, unsigned int chip, __u8 addr, __u8 reg)
{
	__u8 sdata[3];
	unsigned int txnum;
	unsigned int rxnum;

	txnum = 3;
	rxnum = 0;

	sdata[0] = SPI_NAND_SETSR;
	sdata[1] = addr;
	sdata[2] = reg;

	spic_select_ss(spi_no, chip);

	spic_config_dual_mode(spi_no, 0, 0, txnum);
	return spic0_rw(spi_no, txnum, (void *)sdata, rxnum, NULL, 0);
}

/* Write Enable */
__s32 spinand_wren(unsigned int spi_no)
{
	__u8 sdata = SPI_NAND_WREN;
	unsigned int txnum;
	unsigned int rxnum;

	txnum = 1;
	rxnum = 0;

	spic_config_dual_mode(spi_no, 0, 0, txnum);
	return spic0_rw(spi_no, txnum, (void *)&sdata, rxnum, NULL, 0);
}

/* Write Disable */
__s32 spinand_wrdi(unsigned int spi_no)
{
	__u8 sdata = SPI_NAND_WRDI;
	unsigned int txnum;
	unsigned int rxnum;

	txnum = 1;
	rxnum = 0;

	spic_config_dual_mode(spi_no, 0, 0, txnum);
	return spic0_rw(spi_no, txnum, (void *)&sdata, rxnum, NULL, 0);
}

/* get status register */
__s32 spinand_getsr(unsigned int spi_no, __u8 *reg)
{
	__u8 sdata[2];
	unsigned int txnum;
	unsigned int rxnum;

	txnum = 2;
	rxnum = 1;

	sdata[0] = SPI_NAND_GETSR;
	/*
	 * status adr:0xc0
	 * feature adr:0xb0
	 * protection adr:0xa0
	 */
	sdata[1] = 0xc0;

	spic_config_dual_mode(spi_no, 0, 0, txnum);
	return spic0_rw(spi_no, txnum, (void *)sdata, rxnum, (void *)reg, 0);
}

/**
 * wait nand finish operation
 *
 * operation: Program Execute, Page Read, Block Erase, Reset
 */
__s32 spinand_wait_status(unsigned int spi_no, __u8 *status)
{
	__s32 timeout = 0xfffff;
	__s32 ret;

	while (timeout--) {
		ret = spinand_getsr(spi_no, status);
		if (ret != NAND_OP_TRUE) {
			NAND_Print("%s getsr fail!\n", __func__);
			return ret;
		}

		if (!(*(__u8 *)status & SPI_NAND_READY))
			break;

		if (timeout < 0) {
			NAND_Print("%s timeout!\n", __func__);
			return ERR_TIMEOUT;
		}
	}
	return NAND_OP_TRUE;
}

__s32 spinand_setstatus(unsigned int spi_no, unsigned int chip, __u8 reg)
{
	__u8 sdata[3] = {0};
	__s32 ret = NAND_OP_TRUE;
	unsigned int txnum;
	unsigned int rxnum;

	ret = spinand_wren(spi_no);
	if (ret != NAND_OP_TRUE)
		return ret;

	txnum = 3;
	rxnum = 0;

	sdata[0] = SPI_NAND_SETSR;
	sdata[1] = 0xc0;
	sdata[2] = reg;

	spic_select_ss(spi_no, chip);

	spic_config_dual_mode(spi_no, 0, 0, txnum);
	return spic0_rw(spi_no, txnum, (void *)sdata, rxnum, NULL, 0);
}

__s32 spinand_getblocklock(unsigned int spi_no, unsigned int chip, __u8 *reg)
{
	__u8 sdata[2];
	unsigned int txnum;
	unsigned int rxnum;

	txnum = 2;
	rxnum = 1;

	sdata[0] = SPI_NAND_GETSR;
	/*
	 * status addr: 0xC0
	 * feature addr: 0xB0(control others) and 0xD0 (control driver)
	 * protection addr: 0xA0
	 */
	sdata[1] = 0xA0;

	spic_select_ss(spi_no, chip);

	spic_config_dual_mode(spi_no, 0, 0, txnum);
	return spic0_rw(spi_no, txnum, (void *)sdata, rxnum, (void *)reg, 0);
}

__s32 spinand_setblocklock(unsigned int spi_no, unsigned int chip, __u8 reg)
{
	__u8 sdata[3];
	unsigned int txnum;
	unsigned int rxnum;

	txnum = 3;
	rxnum = 0;

	sdata[0] = SPI_NAND_SETSR;
	/*
	 * status addr: 0xC0
	 * feature addr: 0xB0(control others) and 0xD0 (control driver)
	 * protection addr: 0xA0
	 */
	sdata[1] = 0xA0;
	sdata[2] = reg;

	spic_select_ss(spi_no, chip);

	spic_config_dual_mode(spi_no, 0, 0, txnum);
	return spic0_rw(spi_no, txnum, (void *)sdata, rxnum, NULL, 0);
}

__s32 spinand_getotp(unsigned int spi_no, unsigned int chip, __u8 *reg)
{
	__u8 sdata[2];
	unsigned int txnum;
	unsigned int rxnum;

	txnum = 2;
	rxnum = 1;

	sdata[0] = SPI_NAND_GETSR;
	/*
	 * status addr: 0xC0
	 * feature addr: 0xB0(control others) and 0xD0 (control driver)
	 * protection addr: 0xA0
	 */
	sdata[1] = 0xB0;

	spic_select_ss(spi_no, chip);

	spic_config_dual_mode(spi_no, 0, 0, txnum);
	return spic0_rw(spi_no, txnum, (void *)sdata, rxnum, (void *)reg, 0);
}

__s32 spinand_setotp(unsigned int spi_no, unsigned int chip, __u8 reg)
{
	__u8 sdata[3];
	unsigned int txnum;
	unsigned int rxnum;

	txnum = 3;
	rxnum = 0;

	sdata[0] = SPI_NAND_SETSR;
	/*
	 * status addr: 0xC0
	 * feature addr: 0xB0(control others) and 0xD0 (control driver)
	 * protection addr: 0xA0
	 */
	sdata[1] = 0xB0;
	sdata[2] = reg;

	spic_select_ss(spi_no, chip);

	spic_config_dual_mode(spi_no, 0, 0, txnum);
	return spic0_rw(spi_no, txnum, (void *)sdata, rxnum, NULL, 0);
}

__s32 spinand_getoutdriver(unsigned int spi_no, unsigned int chip, __u8 *reg)
{
	__u8 sdata[2] ;
	unsigned int txnum;
	unsigned int rxnum;

	txnum = 2;
	rxnum = 1;

	sdata[0] = SPI_NAND_GETSR;
	/*
	 * status addr: 0xC0
	 * feature addr: 0xB0(control others) and 0xD0 (control driver)
	 * protection addr: 0xA0
	 */
	sdata[1] = 0xD0;

	spic_select_ss(spi_no, chip);

	spic_config_dual_mode(spi_no, 0, 0, txnum);
	return spic0_rw(spi_no, txnum, (void *)sdata, rxnum, (void *)reg, 0);
}

__s32 spinand_setoutdriver(unsigned int spi_no, unsigned int chip, __u8 reg)
{
	__u8 sdata[3] ;
	unsigned int txnum;
	unsigned int rxnum;

	txnum = 3;
	rxnum = 0;

	sdata[0] = SPI_NAND_SETSR;
	/*
	 * status addr: 0xC0
	 * feature addr: 0xB0 and 0xD0
	 * protection addr: 0xA0
	 */
	sdata[1] = 0xD0;
	sdata[2] = reg;

	spic_select_ss(spi_no, chip);

	spic_config_dual_mode(spi_no, 0, 0, txnum);
	return spic0_rw(spi_no, txnum, (void *)sdata, rxnum, NULL, 0);
}

__s32 spinand_read_int_eccsr(unsigned int spi_no, __u8 *reg)
{
	__s32 ret = NAND_OP_TRUE;
	__u8 sdata[2] ;
	unsigned int txnum;
	unsigned int rxnum;

	txnum = 2;
	rxnum = 1;

	sdata[0] = SPI_NAND_READ_INT_ECCSTATUS;
	sdata[1] = 0x0;

	spic_config_dual_mode(spi_no, 0, 0, txnum);
	ret = spic0_rw(spi_no, txnum, (void *)sdata, rxnum, (void *)reg, 0);

	return ret;
}

static inline __s32 spinand_check_fail_status(__u8 status, __u8 mask)
{
	if (status & mask) {
		if (mask == SPI_NAND_ERASE_FAIL)
			NAND_Print("%s: erase fail, status = 0x%x\n",
			       __func__, status);
		else
			NAND_Print("%s: write fail, status = 0x%x\n",
			       __func__, status);
		return NAND_OP_FALSE;
	}
	return NAND_OP_TRUE;
}

/**
 * check status
 *
 * @mode: 0-check ecc status  1-check operation status
 *
 * All of modes will wait status, it means waiting operation end.
 */
__s32 spinand_read_status(unsigned int spi_no, unsigned int chip, __u8 status, unsigned int mode)
{
	__s32 ret;
	__u8 ext_ecc = 0;

	struct __NandStorageInfo_t  *info = &NandStorageInfo;

	spic_select_ss(spi_no, chip);

	/* write and erase */
	if (mode) {
		ret = spinand_wait_status(spi_no, &status);
		if (ret != NAND_OP_TRUE)
			return ret;

		if (mode == STAT_MODE_ERASE) {
			return spinand_check_fail_status(status,
							 (__u8)SPI_NAND_ERASE_FAIL);
		} else if (mode == STAT_MODE_WRITE) {
			return spinand_check_fail_status(status,
							 (__u8)SPI_NAND_WRITE_FAIL);
		} else {
			ret |= spinand_check_fail_status(status,
							 (__u8)SPI_NAND_ERASE_FAIL);
			ret |= spinand_check_fail_status(status,
							 (__u8)SPI_NAND_WRITE_FAIL);
			return ret;
		}
		/* read (ecc status) */
	} else {
		unsigned int ecc_type;

		if (info->EccType == ECC_TYPE_ERR) {
			NAND_Print("invalid EccType on nand info\n");
			return ERR_ECC;
		}

		ret = spinand_wait_status(spi_no, &status);
		if (ret != NAND_OP_TRUE)
			return ret;

		if (info->EccType & HAS_EXT_ECC_STATUS) {
			/* extern ecc status should not shift */
			ret = spinand_read_int_eccsr(spi_no, &status);
			if (ret != NAND_OP_TRUE)
				return ret;
		} else {
			if (info->ecc_status_shift)
				status = status >> info->ecc_status_shift;
			else
				status = status >> SPI_NAND_ECC_FIRST_BIT;
		}

		if (status && (info->EccType & HAS_EXT_ECC_SE01)) {
			ret = spinand_get(spi_no, 0, 0xF0, &ext_ecc);
			if (ret != NAND_OP_TRUE)
				return ret;
			ext_ecc = ((ext_ecc >> 4) & 0x3);
			status = ((status << 2) | ext_ecc);
		}

		ecc_type = info->EccType & ECC_TYPE_BITMAP;
		return spinand_check_ecc(ecc_type, status);
	}
}

__s32 spinand_block_erase(unsigned int spi_no, unsigned int row_addr)
{
	__u8 sdata[4] = {0};
	__s32 ret = NAND_OP_TRUE;
	__u8  status = 0;
	unsigned int txnum;
	unsigned int rxnum;

	//N_mdelay(1);
	ret = spinand_wren(spi_no);
	if (ret != NAND_OP_TRUE)
		return ret;

	txnum = 4;
	rxnum = 0;

	sdata[0] = SPI_NAND_BE;
	sdata[1] = (row_addr >> 16) & 0xff;
	sdata[2] = (row_addr >> 8) & 0xff;
	sdata[3] = row_addr & 0xff;

	spic_config_dual_mode(spi_no, 0, 0, txnum);
	ret = spic0_rw(spi_no, txnum, (void *)sdata, rxnum, NULL, 0);
	if (ret != NAND_OP_TRUE)
		return ret;

	return spinand_read_status(spi_no, 0, status, STAT_MODE_ERASE);
}

__s32 spinand_reset(unsigned int spi_no, unsigned int chip)
{
	__u8 sdata = SPI_NAND_RESET;
	__s32 ret = NAND_OP_TRUE;
	unsigned int txnum;
	unsigned int rxnum;
	__u8  status = 0;

	txnum = 1;
	rxnum = 0;

	spic_select_ss(spi_no, chip);

	spic_config_dual_mode(spi_no, 0, 0, txnum);
	ret = spic0_rw(spi_no, txnum, (void *)&sdata, rxnum, NULL, 0);
	if (ret != NAND_OP_TRUE)
		return ret;

	return spinand_wait_status(spi_no, &status);
}

/**
 * read data from nand
 *
 * return ecc status.
 */
__s32 spinand_read(unsigned int spi_no, unsigned int page_num, unsigned int mbyte_cnt,
		   unsigned int sbyte_cnt, void *mbuf, void *sbuf, unsigned int column)
{
	unsigned int txnum, rxnum, page_addr = page_num;
	__u8  sdata[8] = {0}, status = 0;
	__s32 ret, ecc_status = 0;
	__u8 *spare_buf = PageCachePool.SpareCache;
	struct __NandStorageInfo_t  *info = &NandStorageInfo;
	unsigned int op_opt = info->OperationOpt;
	unsigned int data_area_size = SECTOR_SIZE * info->SectorCntPerPage;
	unsigned int spare_area_size;
	__u8 op_width, op_dummy = 0;

	if (op_opt & NAND_ONEDUMMY_AFTER_RANDOMREAD)
		op_dummy = 1;
	if (op_opt & NAND_QUAD_READ)
		op_width = 4;
	else if (op_opt & NAND_DUAL_READ)
		op_width = 2;
	else
		op_width = 1;
	if (info->EccProtectedType == ECC_PROTECTED_TYPE) {
		NAND_Print("invalid EccProtectedType on nand info\n");
		return NAND_OP_FALSE;
	}

	spare_area_size = spinand_get_spare_size(info->EccProtectedType);

	/* read page to nand cache */
	txnum = 4;
	rxnum = 0;
	sdata[0] = SPI_NAND_PAGE_READ;
	sdata[1] = (page_addr >> 16) & 0xff;
	sdata[2] = (page_addr >> 8) & 0xff;
	sdata[3] = page_addr & 0xff;
	spic_config_dual_mode(spi_no, 0, 0, txnum);
	ret = spic0_rw(spi_no, txnum, (void *)sdata, rxnum, NULL, 0);
	if (ret != NAND_OP_TRUE)
		return ret;

	/* wait operation and get ecc status */
	ecc_status = spinand_read_status(spi_no, 0, status, STAT_MODE_READ);

	if (mbuf) {
		/* read main data and spare data from nand cache */
		if (op_dummy) {
			txnum = 5;
			/* 1byte dummy */
			sdata[1] = 0x0;
			sdata[2] = ((column >> 8) & 0xff);
			sdata[3] = column & 0xff;
			sdata[4] = 0x0;
		} else {
			txnum = 4;
			sdata[1] = ((column >> 8) & 0xff);
			if (op_opt & NAND_TWO_PLANE_SELECT) {
				if (page_num & info->PageCntPerPhyBlk)
					sdata[1] = ((column >> 8) & 0x0f) | 0x10;
				else
					sdata[1] = ((column >> 8) & 0x0f);
			}
			sdata[2] = column & 0xff;
			sdata[3] = 0x0;
		}
		rxnum = sbuf ? mbyte_cnt + spare_area_size : mbyte_cnt;
		if (op_width == 4) {
			sdata[0] = SPI_NAND_READ_X4;
			spic_config_dual_mode(spi_no, 2, 0, txnum);
		} else if (op_width == 2) {
			sdata[0] = SPI_NAND_READ_X2;
			spic_config_dual_mode(spi_no, 1, 0, txnum);
		} else {
			sdata[0] = SPI_NAND_FAST_READ_X1;
			spic_config_dual_mode(spi_no, 0, 0, txnum);
		}

		ret = spic0_rw(spi_no, txnum, (void *)sdata, rxnum, mbuf, 0);
		if (ret != NAND_OP_TRUE)
			goto err;

		if (sbuf) {
			ret = spinand_copy_from_spare(info->EccProtectedType,
						      sbuf, mbuf + data_area_size, sbyte_cnt);
			if (ret != NAND_OP_TRUE)
				goto err;
			/* invalid data */
			if (((__u8 *)mbuf)[data_area_size] != 0xff ||
			   (((__u8 *)sbuf)[0] != 0xff))
				((__u8 *)sbuf)[0] = 0x0;
		}
	} else if (sbuf) {
		/* read spare data only */
		if (op_dummy) {
			txnum = 5;
			/* 1byte dummy */
			sdata[1] = 0x0;
			sdata[2] = ((data_area_size >> 8) & 0xff);
			sdata[3] = data_area_size & 0xff;
			sdata[4] = 0x0;
		} else {
			txnum = 4;
			sdata[1] = ((data_area_size >> 8) & 0xff);
			sdata[2] = data_area_size & 0xff;
			sdata[3] = 0x0;
		}
		rxnum = spare_area_size;
		if (op_width == 4) {
			sdata[0] = SPI_NAND_READ_X4;
			spic_config_dual_mode(spi_no, 2, 0, txnum);
		} else if (op_width == 2) {
			sdata[0] = SPI_NAND_READ_X2;
			spic_config_dual_mode(spi_no, 1, 0, txnum);
		} else {
			sdata[0] = SPI_NAND_FAST_READ_X1;
			spic_config_dual_mode(spi_no, 0, 0, txnum);
		}

		ret = spic0_rw(spi_no, txnum, (void *)sdata, rxnum, spare_buf, 0);
		if (ret != NAND_OP_TRUE)
			goto err;

		ret = spinand_copy_from_spare(info->EccProtectedType, sbuf,
					      spare_buf, sbyte_cnt);
		if (ret != NAND_OP_TRUE)
			goto err;
		/* invalid data */
		if ((spare_buf[0] != 0xff) || (((__u8 *)sbuf)[0] != 0xff))
			((__u8 *)sbuf)[0] = 0x0;
	}

	ret = ecc_status;
err:
	return ret;
}

__s32 spinand_write(unsigned int spi_no, unsigned int page_addr, unsigned int mbyte_cnt,
		    unsigned int sbyte_cnt, void *mbuf, void *sbuf, unsigned int column)
{
	unsigned int txnum, rxnum;
	__u8 *sdata, status = 0;
	__s32 ret;
	struct __NandStorageInfo_t *info = &NandStorageInfo;
	unsigned int op_opt = info->OperationOpt;
	unsigned int data_area_size = SECTOR_SIZE * info->SectorCntPerPage;
	unsigned int spare_area_size;
	__u8 op_width;

	sdata = PageCachePool.SpiPageCache;
	spare_area_size = spinand_get_spare_size(info->EccProtectedType);
	NAND_Memset((void *)sdata, 0xFF, data_area_size + spare_area_size + 64);

	if (op_opt & NAND_QUAD_PROGRAM)
		op_width = 4;
	else
		op_width = 1;
	if (info->EccProtectedType == ECC_PROTECTED_TYPE) {
		NAND_Print("invalid EccProtectedType on nand info\n");
		return NAND_OP_FALSE;
	}

	/* write enable */
	ret = spinand_wren(spi_no);
	if (ret != NAND_OP_TRUE)
		return ret;

	if (0xef != info->NandChipId[0]) {
		/* cmd(1B) + addr(2B) + data(2048B) + spare */
		txnum = 3 + data_area_size + spare_area_size;
		rxnum = 0;
		sdata[0] = op_width == 4 ? SPI_NAND_PP_X4 : SPI_NAND_PP;
		sdata[1] = (column >> 8) & 0xff;
		if (op_opt & NAND_TWO_PLANE_SELECT) {
			if (page_addr & info->PageCntPerPhyBlk)
				sdata[1] = ((column >> 8) & 0x0f) | 0x10;
			else
				sdata[1] = ((column >> 8) & 0x0f);
		}
		sdata[2] = column & 0xff;
		/* copy data */
		NAND_Memcpy((void *)(sdata + 3), mbuf, mbyte_cnt);
		/* copy spare data */
		ret = spinand_copy_to_spare(info->EccProtectedType,
					    sdata + 3 + data_area_size, sbuf, sbyte_cnt);
		if (ret != NAND_OP_TRUE)
			return ret;
		/* Program Load */
		if (op_width == 4)
			spic_config_dual_mode(spi_no, 2, 0, 3);
		else
			spic_config_dual_mode(spi_no, 0, 0, txnum);
		ret = spic0_rw(spi_no, txnum, (void *)sdata, rxnum, NULL, 0);
		if (ret != NAND_OP_TRUE)
			return ret;
	} else {
		/* Special case for Winbond 4x100 bug: there may be an
		 * unexpected 0xFF @ 0x400 or 0x401 offset address of
		 * physical page*/
		/* step 1, program half page */
		/* cmd(1B) + addr(2B) + data((data_area_size>>1)) */
		txnum = 3 + (data_area_size >> 1);
		rxnum = 0;
		sdata[0] = op_width == 4 ? SPI_NAND_PP_X4 : SPI_NAND_PP;
		sdata[1] = (column >> 8) & 0xff;
		if (op_opt & NAND_TWO_PLANE_SELECT) {
			if (page_addr & info->PageCntPerPhyBlk)
				sdata[1] = ((column >> 8) & 0x0f) | 0x10;
			else
				sdata[1] = ((column >> 8) & 0x0f);
		}
		sdata[2] = column & 0xff;
		/* copy data */
		NAND_Memcpy((void *)(sdata + 3), mbuf, (data_area_size >> 1));
		/* Program Load */
		if (op_width == 4)
			spic_config_dual_mode(spi_no, 2, 0, 3);
		else
			spic_config_dual_mode(spi_no, 0, 0, txnum);
		ret = spic0_rw(spi_no, txnum, (void *)sdata, rxnum, NULL, 0);
		if (ret != NAND_OP_TRUE)
			return ret;

		/* step 2, random_program another half page */
		/* cmd(1B) + addr(2B) + data((data_area_size>>1)) + spare(64B) */
		txnum = 3 + (data_area_size >> 1) + spare_area_size;
		rxnum = 0;

		NAND_Memset((void *)sdata, 0xff, txnum);

		sdata[0] = op_width == 4 ? SPI_NAND_RANDOM_PP_X4 : SPI_NAND_RANDOM_PP;
		// column address
		sdata[1] = ((data_area_size >> 1) >> 8) & 0xff;  // 4bit dummy,12bit column adr
		sdata[2] = (__u8)((data_area_size >> 1) & 0xff); // A7:A0

		NAND_Memcpy((void *)(sdata + 3), mbuf + (data_area_size >> 1), (data_area_size >> 1));
		/* copy spare data */
		ret = spinand_copy_to_spare(info->EccProtectedType,
					    sdata + 3 + (data_area_size >> 1), sbuf, sbyte_cnt);
		if (ret != NAND_OP_TRUE)
			return ret;
		if (op_width == 4)
			spic_config_dual_mode(spi_no, 2, 0, 3);
		else
			spic_config_dual_mode(spi_no, 0, 0, txnum);
		ret = spic0_rw(spi_no, txnum, (void *)sdata, rxnum, NULL, 0);
		if (ret != NAND_OP_TRUE)
			return ret;
	}

	txnum = 4;
	rxnum = 0;
	sdata[0] = SPI_NAND_PE;
	sdata[1] = (page_addr >> 16) & 0xff;
	sdata[2] = (page_addr >> 8) & 0xff;
	sdata[3] = page_addr & 0xff;
	/* Program Execute */
	spic_config_dual_mode(spi_no, 0, 0, txnum);
	ret = spic0_rw(spi_no, txnum, (void *)sdata, rxnum, NULL, 0);
	if (ret != NAND_OP_TRUE)
		return ret;

	/* Wait End and return result */
	return spinand_read_status(spi_no, 0, status, STAT_MODE_WRITE);
}

__s32 spinand_read_single_page(struct boot_physical_param *readop,
			       unsigned int spare_only_flag)
{
	__s32 ret = NAND_OP_TRUE;
	unsigned int addr;
	unsigned int first_sector;
	unsigned int sector_num;
	__u8 *data_buf;

	data_buf = PageCachePool.SpiPageCache;

	addr = cal_addr_in_chip(readop->block, readop->page);

	spic_select_ss(0, readop->chip);

	first_sector = cal_first_valid_bit(readop->sectorbitmap);
	sector_num = cal_valid_bits(readop->sectorbitmap);
	/*NAND_Print("read page@%d, sector_num@%d\n", addr, sector_num);*/

	if (spare_only_flag)
		ret = spinand_read(0, addr, 0, 16, NULL, readop->oobbuf, 0);
	else
		ret = spinand_read(0, addr, 512 * sector_num, 16, data_buf,
				   readop->oobbuf, 512 * first_sector);

	if (spare_only_flag == 0)
		NAND_Memcpy((__u8 *)readop->mainbuf + 512 * first_sector, data_buf,
		       512 * sector_num);

	return ret;
}

__s32 spinand_write_single_page(struct boot_physical_param *writeop)
{
	__u8  sparebuf[32];
	unsigned int addr;

	/* select chip */
	spic_select_ss(0, writeop->chip);

	NAND_Memset(sparebuf, 0xff, 32);
	/* we just use 16 bytes spare area */
	if (writeop->oobbuf)
		NAND_Memcpy(sparebuf, writeop->oobbuf, 16);

	addr = cal_addr_in_chip(writeop->block, writeop->page);

	return spinand_write(0, addr,
			     512 * NandStorageInfo.SectorCntPerPage, 16,
			     writeop->mainbuf, sparebuf, 0);
}

__s32 spinand_erase_single_block(struct boot_physical_param *eraseop)
{
	unsigned int addr;

	addr = cal_addr_in_chip(eraseop->block, 0);
	spic_select_ss(0, eraseop->chip);
	return spinand_block_erase(0, addr);
}

struct spi_nand_function spinand_function = {
	spinand_reset,
	spinand_read_status,
	spinand_setstatus,
	spinand_getblocklock,
	spinand_setblocklock,
	spinand_getotp,
	spinand_setotp,
	spinand_getoutdriver,
	spinand_setoutdriver,
	spinand_erase_single_block,
	spinand_write_single_page,
	spinand_read_single_page,
};
