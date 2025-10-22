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

#ifndef _SUNXI_NAND_H
#define _SUNXI_NAND_H
#include <asm/types.h>

#ifndef __s8
typedef signed char __s8;
#endif

#ifndef __u8
typedef unsigned char __u8;
#endif

#ifndef __s16
typedef signed short __s16;
#endif

#ifndef __u16
typedef unsigned short __u16;
#endif

#ifndef __s32
typedef signed int __s32;
#endif

#ifndef __u32
typedef unsigned int __u32;
#endif

#ifndef __s64
typedef signed long long __s64;
#endif

#ifndef __u64
typedef unsigned long long __u64;
#endif

#ifndef uchar
typedef unsigned char   uchar;
#endif

#ifndef uint16
typedef unsigned short  uint16;
#endif

#ifndef uint32
typedef unsigned int    uint32;
#endif

#ifndef uint
typedef unsigned int    uint;
#endif


#ifndef sint32
typedef  int            sint32;
#endif

#ifndef uint64
typedef unsigned long long         uint64;
#endif

#ifndef sint16
typedef short           sint16;
#endif

#ifndef UINT8
typedef unsigned char   UINT8;
#endif

#ifndef UINT32
typedef unsigned int    UINT32;
#endif

#ifndef SINT32
typedef  signed int     SINT32;
#endif

#define NULL ((void *)0)

//extern __u32 SPIC_IO_BASE;
/* run time control */
#define TEST_SPI_NO		(0)
#define SPI_DEFAULT_CLK	(40000000)
#define SPI_TX_WL		(32)
#define SPI_RX_WL		(32)
#define SPI_FIFO_SIZE	(64)
#define SPI_CLK_SRC		(1)	//0-24M, 1-PLL6
#define SPI_MCLK		(40000000)

//#define SPIC_BASE_OS	(0x1000)
extern void *NAND_GetIOBaseAddr(u32 no);
#define SPI_BASE		(u8 *)(NAND_GetIOBaseAddr(0))
//#define SPI_BASE		(0xe086f000)
//#define SPI_IRQNO(_n)	(INTC_SRC_SPI0 + (_n))

#define SPI_VAR		(SPI_BASE + 0x00)
#define SPI_GCR		(SPI_BASE + 0x04)
#define SPI_TCR		(SPI_BASE + 0x08)
#define SPI_IER		(SPI_BASE + 0x10)
#define SPI_ISR		(SPI_BASE + 0x14)
#define SPI_FCR		(SPI_BASE + 0x18)
#define SPI_FSR		(SPI_BASE + 0x1c)
#define SPI_WCR		(SPI_BASE + 0x20)
#define SPI_CCR		(SPI_BASE + 0x24)
#define SPI_MBC		(SPI_BASE + 0x30)
#define SPI_MTC		(SPI_BASE + 0x34)
#define SPI_BCC		(SPI_BASE + 0x38)
#define SPI_TXD		(SPI_BASE + 0x200)
#define SPI_RXD		(SPI_BASE + 0x300)

/* bit field of registers */
#define SPI_SOFT_RST	(1U << 31)
#define SPI_TXPAUSE_EN	(1U << 7)
#define SPI_MASTER		(1U << 1)
#define SPI_ENABLE		(1U << 0)

#define SPI_EXCHANGE	(1U << 31)
#define SPI_SAMPLE_MODE	(1U << 13)
#define SPI_LSB_MODE	(1U << 12)
#define SPI_SAMPLE_CTRL	(1U << 11)
#define SPI_RAPIDS_MODE	(1U << 10)
#define SPI_DUMMY_1		(1U << 9)
#define SPI_DHB			(1U << 8)
#define SPI_SET_SS_1	(1U << 7)
#define SPI_SS_MANUAL	(1U << 6)
#define SPI_SEL_SS0		(0U << 4)
#define SPI_SEL_SS1		(1U << 4)
#define SPI_SEL_SS2		(2U << 4)
#define SPI_SEL_SS3		(3U << 4)
#define SPI_SS_N_INBST	(1U << 3)
#define SPI_SS_ACTIVE0	(1U << 2)
#define SPI_MODE0		(0U << 0)
#define SPI_MODE1		(1U << 0)
#define SPI_MODE2		(2U << 0)
#define SPI_MODE3		(3U << 0)

#define SPI_CPHA        (1U << 0)

#define SPI_SS_INT		(1U << 13)
#define SPI_TC_INT		(1U << 12)
#define SPI_TXUR_INT	(1U << 11)
#define SPI_TXOF_INT	(1U << 10)
#define SPI_RXUR_INT	(1U << 9)
#define SPI_RXOF_INT	(1U << 8)
#define SPI_TXFULL_INT	(1U << 6)
#define SPI_TXEMPT_INT	(1U << 5)
#define SPI_TXREQ_INT	(1U << 4)
#define SPI_RXFULL_INT	(1U << 2)
#define SPI_RXEMPT_INT	(1U << 1)
#define SPI_RXREQ_INT	(1U << 0)
#define SPI_ERROR_INT	(SPI_TXUR_INT|SPI_TXOF_INT|SPI_RXUR_INT|SPI_RXOF_INT)

#define SPI_TXFIFO_RST	(1U << 31)
#define SPI_TXFIFO_TST	(1U << 30)
#define SPI_TXDMAREQ_EN	(1U << 24)
#define SPI_RXFIFO_RST	(1U << 15)
#define SPI_RXFIFO_TST	(1U << 14)
#define SPI_RXDMAREQ_EN	(1U << 8)

#define SPI_MASTER_DUAL	(1U << 28)

#define SPI_NAND_READY		  (1U << 0)
#define SPI_NAND_ERASE_FAIL   (1U << 2)
#define SPI_NAND_WRITE_FAIL   (1U << 3)
#define SPI_NAND_ECC_FIRST_BIT   (4)
#define SPI_NAND_ECC_BITMAP		 (0x3)
#define SPI_NAND_INT_ECCSR_BITMAP	(0xf)

#define SPI_NAND_WREN  		        0x06
#define SPI_NAND_WRDI  		        0x04
#define SPI_NAND_GETSR              0x0f   //get status/features
#define SPI_NAND_SETSR              0x1f   //set status/features
#define SPI_NAND_PAGE_READ          0x13
#define SPI_NAND_FAST_READ_X1		0x0b
#define SPI_NAND_READ_X1		    0x03
#define SPI_NAND_READ_X2            0x3b
#define SPI_NAND_READ_X4            0x6b
#define SPI_NAND_READ_DUAL_IO 	    0xbb
#define SPI_NAND_READ_QUAD_IO 	    0xeb
#define SPI_NAND_RDID  		        0x9f
#define SPI_NAND_PP    		        0x02
#define SPI_NAND_PP_X4    	        0x32
#define SPI_NAND_RANDOM_PP    		0x84
#define SPI_NAND_RANDOM_PP_X4           0x34
#define SPI_NAND_PE                 0x10   //program execute
#define SPI_NAND_BE    		        0xd8   //block erase
#define SPI_NAND_RESET 		        0xff
#define SPI_NAND_READ_INT_ECCSTATUS 0x7c

#undef readb
#undef readw
#undef writeb
#undef writew
#define readb(addr)		(*((volatile unsigned char  *)(addr)))
#define readw(addr)		(*((volatile unsigned int *)(addr)))
#define writeb(v, addr)	(*((volatile unsigned char  *)(addr)) = (unsigned char)(v))
#define writew(v, addr)	(*((volatile unsigned int *)(addr)) = (unsigned int)(v))

#define NAND_OP_TRUE            (0)                     //define the successful return value
#define NAND_OP_FALSE           (-1)                    //define the failed return value


//define the return value
//#define ECC_CORRECT              9                  //error can be corrected
#define ECC_LIMIT               10                  //reach the limit of the ability of ECC

#define ERR_ECC                 12                  //too much ecc error

#define ERR_NANDFAIL            13                  //nand flash program or erase fail
#define ERR_TIMEOUT             14                  //hardware timeout


#define SECTOR_SIZE             512                 //the size of a sector, based on byte


/* define the mask for the nand flash optional operation */

/* nand flash support dual read operation */
#define NAND_DUAL_READ (1 << 0)
/* nand flash support page dual program operation */
#define NAND_DUAL_PROGRAM (1 << 1)
/* nand flash support multi-plane page read operation */
#define NAND_MULTI_READ (1 << 2)
/* nand flash support multi-plane page program operation */
#define NAND_MULTI_PROGRAM (1 << 3)
/* nand flash support external inter-leave operation, it based multi-chip */
#define NAND_EXT_INTERLEAVE (1 << 4)
/* nand flash support the maximum block erase cnt */
#define NAND_MAX_BLK_ERASE_CNT (1 << 5)
/* nand flash support to read reclaim Operation */
#define NAND_READ_RECLAIM (1 << 6)
/* nand flash need plane select for addr */
#define NAND_TWO_PLANE_SELECT (1 << 7)
#define SPINAND_TWO_PLANE_SELECT NAND_TWO_PLANE_SELECT
/* nand flash need a dummy Byte after random fast read */
#define NAND_ONEDUMMY_AFTER_RANDOMREAD (1 << 8)
/* nand flash only support 8B user meta data under ecc protected */
#define NAND_8B_METADATA_ECC_PROTECTED (1 << 9)
/* nand flash support quad read operation */
#define NAND_QUAD_READ (1<<10)
/* nand flash support page quad program operation */
#define NAND_QUAD_PROGRAM (1<<11)

/* nand flash should not enable QE register bit manually */
#define SPINAND_QUAD_NO_NEED_ENABLE		(1 << 12)

enum ecc_status_shift {
	ECC_STATUS_SHIFT_0 = 0,
	ECC_STATUS_SHIFT_1,
	ECC_STATUS_SHIFT_2,
	ECC_STATUS_SHIFT_3,
	ECC_STATUS_SHIFT_4,
	ECC_STATUS_SHIFT_5,
	ECC_STATUS_SHIFT_6,
	ECC_STATUS_SHIFT_7,
};

//define the nand flash physical information parameter type, for id table
struct __NandPhyInfoPar_t {
	__u8        NandID[8];                          //the ID number of the nand flash chip
	__u8        DieCntPerChip;                      //the count of the Die in one nand flash chip
	__u8        SectCntPerPage;                     //the count of the sectors in one single physical page
	__u16       PageCntPerBlk;                      //the count of the pages in one single physical block
	__u16       BlkCntPerDie;                       //the count fo the physical blocks in one nand flash Die
	__u32       OperationOpt;                       //the bitmap that marks which optional operation that the nand flash can support
	__u16       AccessFreq;                         //the highest access frequence of the nand flash chip, based on MHz
	__u32       SpiMode;                            //spi nand mode, 0:mode 0, 3:mode 3
	__u32		pagewithbadflag;					//bad block flag was written at the first byte of spare area of this page
	struct spi_nand_function *spi_nand_function;    //erase,write,read function for spi nand
	__u32       MultiPlaneBlockOffset;              //the value of the block number offset between the two plane block
	__u32       MaxEraseTimes;              		//the max erase times of a physic block
	__u32		MaxEccBits;							//the max ecc bits that nand support
	__u32		EccLimitBits;						//the ecc limit flag for tne nand
	__u32		Idnumber;
	__u32 EccType; // Just use in spinand2, select different ecc status type.
	__u32 EccProtectedType; // just use in spinand2, select different ecc protected type.
	enum ecc_status_shift ecc_status_shift;
	__u8        reserved[4];						//reserved for 32bit align
};

//define the nand flash storage system information
struct __NandStorageInfo_t {
	__u8        ChipCnt;                            //the count of the total nand flash chips are currently connecting on the CE pin
	__u16       ChipConnectInfo;                    //chip connect information, bit == 1 means there is a chip connecting on the CE pin
	__u8        ConnectMode;						//the rb connect  mode
	__u8        BankCntPerChip;                     //the count of the banks in one nand chip, multiple banks can support Inter-Leave
	__u8        DieCntPerChip;                      //the count of the dies in one nand chip, block management is based on Die
	__u8        PlaneCntPerDie;                     //the count of planes in one die, multiple planes can support multi-plane operation
	__u8        SectorCntPerPage;                   //the count of sectors in one single physic page, one sector is 0.5k
	__u16       PageCntPerPhyBlk;                   //the count of physic pages in one physic block
	__u32       BlkCntPerDie;                       //the count of the physic blocks in one die, include valid block and invalid block
	__u32       OperationOpt;                       //the mask of the operation types which current nand flash can support support
	__u16       FrequencePar;                       //the parameter of the hardware access clock, based on 'MHz'
	__u32       SpiMode;                            //spi nand mode, 0:mode 0, 3:mode 3
	__u8        NandChipId[8];                      //the nand chip id of current connecting nand chip
	__u32		pagewithbadflag;					//bad block flag was written at the first byte of spare area of this page
	__u32       MultiPlaneBlockOffset;              //the value of the block number offset between the two plane block
	__u32       MaxEraseTimes;              		//the max erase times of a physic block
	__u32		MaxEccBits;							//the max ecc bits that nand support
	__u32		EccLimitBits;						//the ecc limit flag for tne nand
	__u32           Idnumber;
	__u32 EccType; // Just use in spinand2, select different ecc status type.
	__u32 EccProtectedType; // just use in spinand2, select different ecc protected type.
	struct spi_nand_function *spi_nand_function;    //erase,write,read function for spi nand
	enum ecc_status_shift ecc_status_shift;
};

//define the page buffer pool for nand flash driver
struct __NandPageCachePool_t {
	__u8        *PageCache0;                        //the pointer to the first page size ram buffer
	//    __u8        *PageCache1;                        //the pointer to the second page size ram buffer
	//    __u8        *PageCache2;                        //the pointer to the third page size ram buffer
	__u8		*SpareCache;
	__u8		*TmpPageCache;
	__u8        *SpiPageCache;
	__u8		*SpareCache1;
};

struct boot_physical_param {
	__u32   chip; //chip no
	__u32  block; // block no within chip
	__u32  page; // apge no within block
	__u32  sectorbitmap;
	void   *mainbuf; //data buf
	void   *oobbuf; //oob buf
};
struct spi_nand_function {
	__s32 (*spi_nand_reset)(__u32 spi_no, __u32 chip);
	__s32 (*spi_nand_read_status)(__u32 spi_no, __u32 chip, __u8 status, __u32 mode);
	__s32 (*spi_nand_setstatus)(__u32 spi_no, __u32 chip, __u8 reg);
	__s32 (*spi_nand_getblocklock)(__u32 spi_no, __u32 chip, __u8 *reg);
	__s32 (*spi_nand_setblocklock)(__u32 spi_no, __u32 chip, __u8 reg);
	__s32 (*spi_nand_getotp)(__u32 spi_no, __u32 chip, __u8 *reg);
	__s32 (*spi_nand_setotp)(__u32 spi_no, __u32 chip, __u8 reg);
	__s32 (*spi_nand_getoutdriver)(__u32 spi_no, __u32 chip, __u8 *reg);
	__s32 (*spi_nand_setoutdriver)(__u32 spi_no, __u32 chip, __u8 reg);
	__s32 (*erase_single_block)(struct boot_physical_param *eraseop);
	__s32 (*write_single_page)(struct boot_physical_param *writeop);
	__s32 (*read_single_page)(struct boot_physical_param *readop, __u32 spare_only_flag);
};

extern struct spi_nand_function spinand_function;

extern int nand_dma_config_start(__u32 tx_mode, __u32 addr, __u32 length);
extern int NAND_WaitDmaFinish(__u32 tx_flag, __u32 rx_flag);
extern int Nand_Dma_End(__u32 rw, __u32 addr, __u32 length);

extern int spinand_get_mbr(char *buffer, uint len);

extern int spinand_uboot_init(int boot_mode);
extern int spinand_uboot_exit(int force);

extern int spinand_uboot_probe(void);

extern uint spinand_uboot_read(uint start, uint sectors, void *buffer);

extern uint spinand_uboot_write(uint start, uint sectors, void *buffer);


extern int spinand_download_boot0(uint length, void *buffer);
extern int spinand_download_uboot(uint length, void *buffer);

extern int spinand_force_download_uboot(uint length, void *buffer);
extern int spinand_uboot_erase(int user_erase);

extern uint spinand_uboot_get_flash_info(void *buffer, uint length);

extern uint spinand_uboot_set_flash_info(void *buffer, uint length);

extern uint spinand_uboot_get_flash_size(void);

extern int spinand_uboot_flush(void);

extern int SPINAND_Uboot_Force_Erase(void);

uint spinand_upload_boot0(uint length, void *buf);

int spinand_download_boot0_simple(uint length, void *buffer);

#endif
