
/*
 * Copyright (c) 2007-2017 Allwinnertech Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

/*********************************************************************************************************
*                                                                NAND FLASH DRIVER
*								(c) Copyright 2008, SoftWinners Co,Ld.
*                                          			    All Right Reserved
*file : nand_simple.c
*description : this file creates some physic basic access function based on single plane for boot .
*history :
*	v0.1  2008-03-26 Richard
* v0.2  2009-9-3 penggang modified for 1615
*
*********************************************************************************************************/
#include "../nand_type_spinand.h"
#include "../nand_physic.h"
#include "../nand_physic_interface_spinand.h"
#include "../spinand_drv_cfg.h"
#include "../spic.h"

extern struct __NandStorageInfo_t  NandStorageInfo;
extern struct __NandPageCachePool_t PageCachePool;

extern __u32 _cal_addr_in_chip(__u32 block, __u32 page);
extern __u32 _cal_first_valid_bit(__u32 secbitmap);
extern __u32 _cal_valid_bits(__u32 secbitmap);


/*  local function  */
__s32 m2_spi_nand_wren(__u32 spi_no)
{
	__u8 sdata = SPI_NAND_WREN;
	__s32 ret = NAND_OP_TRUE;
	__u32 txnum;
	__u32 rxnum;

	txnum = 1;
	rxnum = 0;

	Spic_config_io_mode(spi_no, 0, 0, txnum);
	ret = Spic_rw(spi_no, txnum, (void *)&sdata, rxnum, NULL, 0);

	return ret;
}

__s32 m2_spi_nand_wrdi(__u32 spi_no)
{
	__u8 sdata = SPI_NAND_WRDI;
	__s32 ret = NAND_OP_TRUE;
	__u32 txnum;
	__u32 rxnum;

	txnum = 1;
	rxnum = 0;

	Spic_config_io_mode(spi_no, 0, 0, txnum);
	ret = Spic_rw(spi_no, txnum, (void *)&sdata, rxnum, NULL, 0);

	return ret;
}

__s32 m2_spi_nand_getsr(__u32 spi_no, __u8 *reg)
{
	__s32 ret = NAND_OP_TRUE;
	__u8 sdata[2];
	__u32 txnum;
	__u32 rxnum;

	txnum = 2;
	rxnum = 1;

	sdata[0] = SPI_NAND_GETSR;
	sdata[1] = 0xc0;       //status adr:0xc0;feature adr:0xb0;protection adr:0xa0

	Spic_config_io_mode(spi_no, 0, 0, txnum);
	ret = Spic_rw(spi_no, txnum, (void *)sdata, rxnum, (void *)reg, 0);

	return ret;
}

__s32 m2_spi_wait_status(__u32 spi_no, __u8 status)
{
	__s32 timeout = 0xffffff;
	__s32 ret = NAND_OP_TRUE;
//	__u32 i;

	while (1) {
		ret = m2_spi_nand_getsr(spi_no, &status);
		if (ret != NAND_OP_TRUE) {
			PHY_ERR("m2_spi_wait_status getsr fail!\n");
			return ret;
		}
		if (!(status & SPI_NAND_READY))
			break;
		if (timeout < 0) {
			PHY_ERR("m2_spi_wait_status timeout!\n");
			return ERR_TIMEOUT;
		}
		timeout--;
		//for(i=0; i<100; i++);
	}
	return NAND_OP_TRUE;
}
__s32 m2_spi_nand_setstatus(__u32 spi_no, __u32 chip, __u8 reg)
{
	__u8 sdata[3] = {0};
	__s32 ret = NAND_OP_TRUE;
	__u8 status = 0;
	__u32 txnum;
	__u32 rxnum;

	ret = m2_spi_nand_wren(spi_no);
	if (ret != NAND_OP_TRUE)
		return ret;

	txnum = 3;
	rxnum = 0;

	sdata[0] = SPI_NAND_SETSR;
	sdata[1] = 0xc0;
	sdata[2] = reg;

	Spic_sel_ss(spi_no, chip);

	Spic_config_io_mode(spi_no, 0, 0, txnum);
	ret = Spic_rw(spi_no, txnum, (void *)sdata, rxnum, NULL, 0);
	if (ret != NAND_OP_TRUE)
		return ret;

	ret = m2_spi_wait_status(spi_no, status);
	if (ret != NAND_OP_TRUE)
		return ret;

	return NAND_OP_TRUE;
}

 __s32 m2_spi_nand_getblocklock(__u32 spi_no, __u32 chip, __u8 *reg)
{
	__s32 ret = NAND_OP_TRUE;
	__u8 sdata[2];
	__u32 txnum;
	__u32 rxnum;

	txnum = 2;
	rxnum = 1;

	sdata[0] = SPI_NAND_GETSR;
	sdata[1] = 0xa0;       //status adr:0xc0;feature adr:0xb0;protection adr:0xa0

	Spic_sel_ss(spi_no, chip);

	Spic_config_io_mode(spi_no, 0, 0, txnum);
	ret = Spic_rw(spi_no, txnum, (void *)sdata, rxnum, (void *)reg, 0);

	return ret;
}

 __s32 m2_spi_nand_setblocklock(__u32 spi_no, __u32 chip, __u8 reg)
{
	__s32 ret = NAND_OP_TRUE;
	__u8 sdata[3];
	__u32 txnum;
	__u32 rxnum;
	__u8 status = 0;

	txnum = 3;
	rxnum = 0;

	sdata[0] = SPI_NAND_SETSR;
	sdata[1] = 0xa0;       //status adr:0xc0;feature adr:0xb0;protection adr:0xa0
	sdata[2] = reg;

	Spic_sel_ss(spi_no, chip);

	Spic_config_io_mode(spi_no, 0, 0, txnum);
	ret = Spic_rw(spi_no, txnum, (void *)sdata, rxnum, NULL, 0);
	if (ret != NAND_OP_TRUE)
		return ret;

	ret = m2_spi_wait_status(spi_no, status);
	if (ret != NAND_OP_TRUE)
		return ret;

	return ret;
}

 __s32 m2_spi_nand_getotp(__u32 spi_no, __u32 chip, __u8 *reg)
{
	__s32 ret = NAND_OP_TRUE;
	__u8 sdata[2];
	__u32 txnum;
	__u32 rxnum;

	txnum = 2;
	rxnum = 1;

	sdata[0] = SPI_NAND_GETSR;
	sdata[1] = 0xb0;       //status adr:0xc0;feature adr:0xb0;protection adr:0xa0

	Spic_sel_ss(spi_no, chip);

	Spic_config_io_mode(spi_no, 0, 0, txnum);
	ret = Spic_rw(spi_no, txnum, (void *)sdata, rxnum, (void *)reg, 0);

	return ret;
}

 __s32 m2_spi_nand_setotp(__u32 spi_no, __u32 chip, __u8 reg)
{
	__s32 ret = NAND_OP_TRUE;
	__u8 sdata[3];
	__u32 txnum;
	__u32 rxnum;
	__u8 status = 0;

	txnum = 3;
	rxnum = 0;

	sdata[0] = SPI_NAND_SETSR;
	sdata[1] = 0xb0;       //status adr:0xc0;feature adr:0xb0;protection adr:0xa0
	sdata[2] = reg;

	Spic_sel_ss(spi_no, chip);

	Spic_config_io_mode(spi_no, 0, 0, txnum);
	ret = Spic_rw(spi_no, txnum, (void *)sdata, rxnum, NULL, 0);
	if (ret != NAND_OP_TRUE)
		return ret;

	ret = m2_spi_wait_status(spi_no, status);
	if (ret != NAND_OP_TRUE)
		return ret;

	return ret;
}

__s32 m2_spi_nand_getoutdriver(__u32 spi_no, __u32 chip, __u8 *reg)
{
	__s32 ret = NAND_OP_TRUE;
	__u8 sdata[2];
	__u32 txnum;
	__u32 rxnum;

	txnum = 2;
	rxnum = 1;

	sdata[0] = SPI_NAND_GETSR;
	sdata[1] = 0xd0;       //status adr:0xc0;feature adr:0xb0;protection adr:0xa0

	Spic_sel_ss(spi_no, chip);

	Spic_config_io_mode(spi_no, 0, 0, txnum);
	ret = Spic_rw(spi_no, txnum, (void *)sdata, rxnum, (void *)reg, 0);

	return ret;
}

__s32 m2_spi_nand_setoutdriver(__u32 spi_no, __u32 chip, __u8 reg)
{
	__s32 ret = NAND_OP_TRUE;
	__u8 sdata[3] ;
	__u32 txnum;
	__u32 rxnum;
	__u8 status = 0;

	txnum = 3;
	rxnum = 0;

	sdata[0] = SPI_NAND_SETSR;
	sdata[1] = 0xd0;       //status adr:0xc0;feature adr:0xb0;protection adr:0xa0
	sdata[2] = reg;

	Spic_sel_ss(spi_no, chip);

	Spic_config_io_mode(spi_no, 0, 0, txnum);
	ret = Spic_rw(spi_no, txnum, (void *)sdata, rxnum, NULL, 0);
	if (ret != NAND_OP_TRUE)
		return ret;

	ret = m2_spi_wait_status(spi_no, status);
	if (ret != NAND_OP_TRUE)
		return ret;

	return ret;
}

__s32 m2_spi_nand_block_erase(__u32 spi_no, __u32 row_addr)
{
	__u8 sdata[4] = {0};
	__s32 ret = NAND_OP_TRUE;
	__u8  status = 0;
	__u32 txnum;
	__u32 rxnum;

	ret = m2_spi_nand_wren(spi_no);
	if (ret != NAND_OP_TRUE)
		return ret;

	txnum = 4;
	rxnum = 0;

	sdata[0] = SPI_NAND_BE;
	sdata[1] = (row_addr>>16)&0xff;
	sdata[2] = (row_addr>>8)&0xff;
	sdata[3] = row_addr&0xff;

	Spic_config_io_mode(spi_no, 0, 0, txnum);
	ret = Spic_rw(spi_no, txnum, (void *)sdata, rxnum, NULL, 0);
	if (ret != NAND_OP_TRUE)
		return ret;

	ret = m2_spi_wait_status(spi_no, status);
	if (ret != NAND_OP_TRUE)
		return ret;

	if (status & SPI_NAND_ERASE_FAIL) {
		ret = NAND_OP_FALSE;
		return ret;
	}
	ret = NAND_OP_TRUE;

	return ret;
}

//mode=0:check ecc status  mode=1:check operation status
__s32 m2_spi_nand_read_status(__u32 spi_no, __u32 chip, __u8 status, __u32 mode)
{
	__s32 ret = NAND_OP_TRUE;

	Spic_sel_ss(spi_no, chip);

	if (mode) {
		ret = m2_spi_wait_status(spi_no, status);
		if (ret != NAND_OP_TRUE)
			return ret;

		if (status & SPI_NAND_ERASE_FAIL) {
			PHY_ERR("spi_nand_read_status : erase fail, status = %c\n", status);
			ret = NAND_OP_FALSE;
		}
		if (status & SPI_NAND_WRITE_FAIL) {
			PHY_ERR("spi_nand_read_status : write fail, status = %c\n", status);
			ret = NAND_OP_FALSE;
		}
	} else {
		ret = m2_spi_wait_status(spi_no, status);
		if (ret != NAND_OP_TRUE)
			return ret;

		if (((status >> SPI_NAND_ECC_FIRST_BIT) & 0x7) == 0x0) {
//			PHY_DBG("no error\n", status);
			ret = NAND_OP_TRUE;
		} else if ((((status >> SPI_NAND_ECC_FIRST_BIT) & 0x7) >= 0x1) && (((status >> SPI_NAND_ECC_FIRST_BIT) & 0x7) <= 0x3)) {//ecc < 6,limit
			PHY_DBG("ecc limit %c\n", status);
			ret = ECC_LIMIT;
		} else if (((status >> SPI_NAND_ECC_FIRST_BIT) & 0x7) == 0x7) {
			PHY_ERR("ecc error %c\n", status);
			ret = ERR_ECC;
		}
	}

	return ret;
}

__s32 m2_spi_nand_reset(__u32 spi_no, __u32 chip)
{
	__u8 sdata = SPI_NAND_RESET;
	__s32 ret = NAND_OP_TRUE;
	__u32 txnum;
	__u32 rxnum;
	__u8  status = 0;

	txnum = 1;
	rxnum = 0;

	Spic_sel_ss(spi_no, chip);

	Spic_config_io_mode(spi_no, 0, 0, txnum);
	ret = Spic_rw(spi_no, txnum, (void *)&sdata, rxnum, NULL, 0);
	if (ret != NAND_OP_TRUE)
		return ret;

	ret = m2_spi_wait_status(spi_no, status);
	if (ret != NAND_OP_TRUE)
		return ret;

	ret = NAND_OP_TRUE;

	return ret;
}

__s32 m2_spi_nand_read_x1(__u32 spi_no, __u32 page_num, __u32 mbyte_cnt, __u32 sbyte_cnt, void *mbuf, void *sbuf, __u32 column)
{
	__u32 txnum;
	__u32 rxnum;
	__u32 page_addr = page_num;
	__u8  sdata[8] = {0};
	__u8 spare_buf[32] = {0};
	__s32 ret = NAND_OP_TRUE;
	__u8 status = 0;
	__s32 ecc_status = 0;

	txnum = 4;
	rxnum = 0;

	sdata[0] = SPI_NAND_PAGE_READ;
	sdata[1] = (page_addr>>16)&0xff; //9dummy+15bit row adr
	sdata[2] = (page_addr>>8)&0xff;
	sdata[3] = page_addr&0xff;

	Spic_config_io_mode(spi_no, 0, 0, txnum);
	ret = Spic_rw(spi_no, txnum, (void *)sdata, rxnum, NULL, 0);
	if (ret != NAND_OP_TRUE)
		return ret;

	ret = m2_spi_wait_status(spi_no, status);
	if (ret != NAND_OP_TRUE)
		return ret;

	ecc_status = m2_spi_nand_read_status(spi_no, 0, status, 0);

	if (mbuf) {
		//read main data
		txnum = 5;
		rxnum = mbyte_cnt;

		sdata[0] = SPI_NAND_FAST_READ_X1;
		sdata[1] = 0x0; //1byte dummy
		sdata[2] = ((column>>8)&0xff);//4bit dummy,12bit column adr
		sdata[3] = column&0xff;
		sdata[4] = 0x0; //1byte dummy

		Spic_config_io_mode(spi_no, 0, 0, txnum); //signal read, dummy:2byte, signal tx:3
		ret = Spic_rw(spi_no, txnum, (void *)sdata, rxnum, mbuf, 0);
		if (ret != NAND_OP_TRUE)
			return ret;
	}

	if (sbuf) {
		//read spare data
		txnum = 5;
		rxnum = sbyte_cnt;

		sdata[0] = SPI_NAND_FAST_READ_X1;
		sdata[1] = 0x0; //1byte dummy
		sdata[2] = (((512 * NandStorageInfo.SectorCntPerPage)>>8)&0xff);//4bit dummy,12bit column adr
		sdata[3] = (__u8)((512 * NandStorageInfo.SectorCntPerPage)&0xff);
		sdata[4] = 0x0; //1byte dummy

		Spic_config_io_mode(spi_no, 0, 0, txnum); //signal read, dummy:2byte, signal tx:3
		ret = Spic_rw(spi_no, txnum, (void *)sdata, rxnum, spare_buf, 0);
		if (ret != NAND_OP_TRUE)
			return ret;

		MEMCPY((__u8 *)sbuf, (__u8 *)spare_buf, sbyte_cnt);

		if (*((__u8 *)spare_buf) != 0xff)
			*((__u8 *)sbuf) = 0x0;

	}

//	ret = m2_spi_nand_read_status(spi_no, 0, status, 0);

	return ecc_status;
}

__s32 m2_spi_nand_read_x2(__u32 spi_no, __u32 page_num, __u32 mbyte_cnt, __u32 sbyte_cnt, void *mbuf, void *sbuf, __u32 column)
{
	__u32 txnum;
	__u32 rxnum;
	__u32 page_addr = page_num;
	__u8  sdata[8] = {0};
	__s32 ret = NAND_OP_TRUE;
	__u8  status = 0;
	__s32 ecc_status = 0;
	__u8 spare_buf[32] = {0};

	txnum = 4;
	rxnum = 0;

	sdata[0] = SPI_NAND_PAGE_READ;
	sdata[1] = (page_addr>>16)&0xff;
	sdata[2] = (page_addr>>8)&0xff;
	sdata[3] = page_addr&0xff;

	Spic_config_io_mode(spi_no, 0, 0, txnum);
	ret = Spic_rw(spi_no, txnum, (void *)sdata, rxnum, NULL, 0);
	if (ret != NAND_OP_TRUE)
		return ret;

	ret = m2_spi_wait_status(spi_no, status);
	if (ret != NAND_OP_TRUE)
		return ret;

	ecc_status = m2_spi_nand_read_status(spi_no, 0, status, 0);

	if (mbuf) {
		//read main data
		txnum = 5;
		rxnum = mbyte_cnt;

		sdata[0] = SPI_NAND_READ_X2;
		sdata[1] = 0x0; //1byte dummy
		sdata[2] = ((column>>8)&0xff);//4bit dummy,12bit column adr
		sdata[3] = column&0xff;
		sdata[4] = 0x0; //1byte dummy

		Spic_config_io_mode(spi_no, 1, 0, txnum); //dual read, dummy:2byte, signal tx:3
		ret = Spic_rw(spi_no, txnum, (void *)sdata, rxnum, mbuf, 0);
		if (ret != NAND_OP_TRUE)
			return ret;
	}

	if (sbuf) {
		//read spare data
		txnum = 5;
		rxnum = sbyte_cnt;

		sdata[0] = SPI_NAND_READ_X2;
		sdata[1] = 0x0; //1byte dummy
		sdata[2] = (((512 * NandStorageInfo.SectorCntPerPage)>>8)&0xff);//4bit dummy,12bit column adr
		sdata[3] = (__u8)((512 * NandStorageInfo.SectorCntPerPage)&0xff);
		sdata[4] = 0x0; //1byte dummy

		Spic_config_io_mode(spi_no, 1, 0, txnum); //dual read, dummy:2byte, signal tx:3
		ret = Spic_rw(spi_no, txnum, (void *)sdata, rxnum, spare_buf, 0);
		if (ret != NAND_OP_TRUE)
			return ret;

		MEMCPY((__u8 *)sbuf, (__u8 *)spare_buf, sbyte_cnt);

		if (*((__u8 *)spare_buf) != 0xff)
			*((__u8 *)sbuf) = 0x0;
	}

//	ret = m2_spi_nand_read_status(spi_no, 0, status, 0);

	return ecc_status;
}

__s32 m2_spi_nand_write(__u32 spi_no, __u32 page_addr, __u32 mbyte_cnt, __u32 sbyte_cnt, void *mbuf, void *sbuf, __u32 column)
{
	__u32 txnum;
	__u32 rxnum;
	__u8 *sdata = NULL;
	__u8  status = 0;
//	__u32 i = 0;
	__s32 ret = NAND_OP_TRUE;

	sdata = (__u8 *)PageCachePool.SpiPageCache;

	ret = m2_spi_nand_wren(spi_no);
	if (ret != NAND_OP_TRUE)
	   goto __err_out_;

	txnum = sbyte_cnt + 512 * NandStorageInfo.SectorCntPerPage + 3;
	rxnum = 0;

	MEMSET((void *)sdata, 0xff, (512 * NandStorageInfo.SectorCntPerPage + 3 + sbyte_cnt));

	sdata[0] = SPI_NAND_PP;
	sdata[1] = (column>>8)&0xff; //4bit dummy,12bit column adr
	sdata[2] = column&0xff;      //A7:A0
						   //column address

	MEMCPY((void *)((__u8 *)sdata+3), mbuf, mbyte_cnt);
	MEMCPY((void *)((__u8 *)sdata+3+512 * NandStorageInfo.SectorCntPerPage), sbuf, sbyte_cnt);

	Spic_config_io_mode(spi_no, 0, 0, txnum);
	ret = Spic_rw(spi_no, txnum, (void *)sdata, rxnum, NULL, 0);

	if (ret != NAND_OP_TRUE)
		goto __err_out_;

	txnum = 4;
	rxnum = 0;

	sdata[0] = SPI_NAND_PE;
	sdata[1] = (page_addr>>16)&0xff;
	sdata[2] = (page_addr>>8)&0xff;
	sdata[3] = page_addr&0xff;

	Spic_config_io_mode(spi_no, 0, 0, txnum);
	ret = Spic_rw(spi_no, txnum, (void *)sdata, rxnum, NULL, 0);
	if (ret != NAND_OP_TRUE)
		goto __err_out_;

	ret = m2_spi_wait_status(spi_no, status);
	if (ret != NAND_OP_TRUE)
		goto __err_out_;


	ret = NAND_OP_TRUE;

__err_out_:

//	FREE(sdata, (512 * NandStorageInfo.SectorCntPerPage + 10));
	return ret;
}

__s32 m2_read_single_page(struct boot_physical_param *readop, __u32 spare_only_flag)
{
	__s32 ret = NAND_OP_TRUE;
	__u32 addr;
	__u32 first_sector;
	__u32 sector_num;
	__u8 *data_buf;

	data_buf = PageCachePool.TmpPageCache;

	addr = _cal_addr_in_chip(readop->block, readop->page);

	Spic_sel_ss(0, readop->chip);

	first_sector = _cal_first_valid_bit(readop->sectorbitmap);
	sector_num = _cal_valid_bits(readop->sectorbitmap);

	if (CFG_SUPPORT_DUAL_READ && (NandStorageInfo.OperationOpt & SPINAND_DUAL_READ)) {
		if (spare_only_flag)
			ret = m2_spi_nand_read_x2(0, addr, 0, 16, NULL, readop->oobbuf, 0);
		else
			ret = m2_spi_nand_read_x2(0, addr, 512 * sector_num, 16, data_buf, readop->oobbuf, 512 * first_sector);
	} else {
		if (spare_only_flag)
			ret = m2_spi_nand_read_x1(0, addr, 0, 16, NULL, readop->oobbuf, 0);
		else
			ret = m2_spi_nand_read_x1(0, addr, 512 * sector_num, 16, data_buf, readop->oobbuf, 512 * first_sector);
	}

	if (spare_only_flag == 0)
		MEMCPY((__u8 *)readop->mainbuf + 512 * first_sector, data_buf, 512 * sector_num);

	return ret;
}

__s32 m2_write_single_page(struct boot_physical_param *writeop)
{
	__s32 ret = NAND_OP_TRUE;
	__u8  sparebuf[32];
	__u32 addr;

	Spic_sel_ss(0, writeop->chip);

	MEMSET(sparebuf, 0xff, 32);
	if (writeop->oobbuf) {
		MEMCPY(sparebuf, writeop->oobbuf, 16);
	}

	addr = _cal_addr_in_chip(writeop->block, writeop->page);
//    ret = m2_spi_nand_write(0,addr,512 * NandStorageInfo.SectorCntPerPage,writeop->mainbuf,0);
//    ret = m2_spi_nand_write(0,addr,8,sparebuf,512 * NandStorageInfo.SectorCntPerPage);
	ret = m2_spi_nand_write(0, addr, 512 * NandStorageInfo.SectorCntPerPage, 16, writeop->mainbuf, sparebuf, 0);

	return ret;

}

__s32 m2_erase_single_block(struct boot_physical_param *eraseop)
{
	__s32 ret = NAND_OP_TRUE;
	__u32 addr;

//	addr = _cal_addr_in_chip(eraseop->block,eraseop->page);
	addr = _cal_addr_in_chip(eraseop->block, 0);

	Spic_sel_ss(0, eraseop->chip);

	ret = m2_spi_nand_block_erase(0, addr);

	return ret;
}
