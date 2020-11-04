
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

/*****************************************************************************/
#define _HW_BUILD_C_
/*****************************************************************************/

#include "../nand_osal.h"
#include "../phy/mbr.h"
#include "../phy/phy.h"

/*****************************************************************************/

extern int PHY_VirtualPageRead(unsigned int nDieNum, unsigned int nBlkNum,
			       unsigned int nPage, uint64 SectBitmap,
			       void *pBuf, void *pSpare);
extern int PHY_VirtualPageWrite(unsigned int nDieNum, unsigned int nBlkNum,
				unsigned int nPage, uint64 SectBitmap,
				void *pBuf, void *pSpare);
extern int PHY_VirtualBlockErase(unsigned int nDieNum, unsigned int nBlkNum);
extern int BlockCheck(unsigned short nDieNum, unsigned short nBlkNum);

extern struct _nand_phy_partition *
build_phy_partition_v2(struct _nand_info *nand_info, struct _partition *part);
extern int nand_info_init_v1(struct _nand_info *nand_info, uchar chip,
			     uint16 start_block, uchar *mbr_data);

extern void debug_read_chip(struct _nand_info *nand_info);
extern void print_partition(struct _partition *partition);

int write_mbr_v2(struct _nand_info *nand_info);
int read_mbr_v2(struct _nand_info *nand_info, unsigned int nDieNum,
		unsigned int nBlkNum);
int write_partition_v2(struct _nand_info *nand_info);
int read_partition_v2(struct _nand_info *nand_info, unsigned int nDieNum,
		      unsigned int nBlkNum);
int write_factory_block_table_v2(struct _nand_info *nand_info);
int get_partition_v2(struct _nand_info *nand_info);
void print_nand_info_v2(struct _nand_info *nand_info);
unsigned short read_new_bad_block_table_v2(struct _nand_info *nand_info);
int write_new_block_table_v2(struct _nand_info *nand_info);
int write_no_use_block_v2(struct _nand_info *nand_info);
int print_factory_block_table_v2(struct _nand_info *nand_info);
int check_mbr_v2(struct _nand_info *nand_info, PARTITION_MBR *mbr);
int check_partition_mbr_v2(struct _nand_info *nand_info, unsigned int nDieNum,
			   unsigned int nBlkNum);
int build_all_phy_partition_v2(struct _nand_info *nand_info);
int write_new_block_table_v2_new_first_build(struct _nand_info *nand_info);
void print_boot_info(struct _nand_info *nand_info);
unsigned int get_no_use_block_v3(struct _nand_info *nand_info, uchar chip,
				 uint16 start_block);
int print_mbr_data(uchar *mbr_data);
int get_partition_v3(struct _nand_info *nand_info);
int check_mbr_v3(struct _nand_info *nand_info, PARTITION_MBR *mbr);
extern __u32 nand_get_twoplane_flag(void);
int read_mbr_partition_v3(struct _nand_info *nand_info, unsigned int chip,
			  unsigned int start_block);
int read_factory_block_v3(struct _nand_info *nand_info, unsigned int chip,
			  unsigned int start_block);
/*****************************************************************************
 *Name         :
 *Description  :
 *Parameter    :
 *Return       :
 *Note         :
*****************************************************************************/
int read_partition_v3(struct _nand_info *nand_info, struct _boot_info *boot)
{
	int i;

	MEMCPY(nand_info->partition, boot->partition.ndata,
	       sizeof(nand_info->partition));

	nand_info->partition_nums = 0;
	for (i = 0; i < MAX_PARTITION; i++) {
		if ((nand_info->partition[i].size != 0) &&
		    (nand_info->partition[i].size != 0xffffffff)) {
			nand_info->partition_nums++;
		}
	}
	return 0;
}

/*****************************************************************************
 *Name         :
 *Description  :
 *Parameter    :
 *Return       :
 *Note         :
*****************************************************************************/
int nand_info_init_v3(struct _nand_info *nand_info, uchar chip,
		      uint16 start_block, uchar *mbr_data)
{
	unsigned int nDieNum, nBlkNum, nPage, i, nouse;
	unsigned int ret;
	struct _boot_info *boot;

	boot = nand_info->boot;

	MEMSET(nand_info->mbr_data, 0xff, sizeof(PARTITION_MBR));

	nand_info->mini_free_block_first_reserved = MIN_FREE_BLOCK_NUM_V2;
	nand_info->mini_free_block_reserved = MIN_FREE_BLOCK_REMAIN;
	nand_info->new_bad_page_addr = 0xffff;
	nand_info->FirstBuild = 0;

	if (mbr_data != NULL) {
		PHY_DBG("[ND]factory FirstBuild %d\n", start_block);
		// print_boot_info(nand_info);
		if (nand_info->boot->mbr.data.PartCount == 0) {
			nouse =
			    get_no_use_block_v3(nand_info, chip, start_block);
			if (nouse == 0xffffffff) {
				// nand3.0;  factory burn ; erase
				PHY_DBG("[ND]erase factory FirstBuild\n");
				MEMCPY(nand_info->mbr_data, mbr_data,
				       sizeof(PARTITION_MBR));
				nand_info->FirstBuild = 1;
				if (get_partition_v3(nand_info)) {
					PHY_ERR(
					    "[NE]get partition v3 fail!!\n");
					return PHY_FAILURE;
				}
				MEMCPY(boot->partition.ndata,
				       nand_info->partition,
				       sizeof(nand_info->partition));
				MEMCPY(boot->mbr.ndata, nand_info->mbr_data,
				       sizeof(PARTITION_MBR));
				nand_info->boot->logic_start_block =
				    start_block;
				nand_info->boot->no_use_block =
				    nand_info->boot->logic_start_block;
			} else {
				// from nand2.0 to nand3.0 ; factory  burn;  not
				// erase
				PHY_DBG(
				    "[ND]old not erase factory FirstBuild\n");
				if (read_mbr_partition_v3(nand_info, chip,
							  start_block) != 0) {
					PHY_ERR(
					    "[NE]not erase MP but no mbr!\n");
					return PHY_FAILURE;
				}
				read_factory_block_v3(nand_info, chip,
						      start_block);
				MEMCPY(boot->mbr.ndata, nand_info->mbr_data,
				       sizeof(PARTITION_MBR));
				MEMCPY(boot->partition.ndata,
				       nand_info->partition,
				       sizeof(nand_info->partition));
				MEMCPY(boot->factory_block.ndata,
				       nand_info->factory_bad_block,
				       FACTORY_BAD_BLOCK_SIZE);
				nand_info->boot->logic_start_block = nouse + 1;
				nand_info->boot->no_use_block =
				    nand_info->boot->logic_start_block;
				if (check_mbr_v2(nand_info,
						 (PARTITION_MBR *)mbr_data) !=
				    0) {
					PHY_ERR("[NE]not erase MP but mbr not "
						"the same 1!\n");
					return PHY_FAILURE;
				}
			}
		} else {
			// nand3.0 to nand3.0 ; factory  burn ;   not erase
			PHY_DBG("[ND]new not erase factory FirstBuild\n");
			MEMCPY(nand_info->mbr_data, boot->mbr.ndata,
			       sizeof(PARTITION_MBR));
			read_partition_v3(nand_info, boot);
			MEMCPY(nand_info->factory_bad_block,
			       boot->factory_block.ndata,
			       FACTORY_BAD_BLOCK_SIZE);
			nand_info->boot->no_use_block =
			    nand_info->boot->logic_start_block;
			if (check_mbr_v3(nand_info,
					 (PARTITION_MBR *)mbr_data) != 0) {
				PHY_ERR("[NE]not erase MP but mbr not the same "
					"2!\n");
				return PHY_FAILURE;
			}
		}
	} else {
		PHY_DBG("[ND]boot start\n");
		print_boot_info(nand_info);
		// print_mbr_data(&boot->mbr.data);
		if (boot->mbr.data.PartCount == 0) {
			// first start nand3.0  from  nand2.0    from OTA
			nouse =
			    get_no_use_block_v3(nand_info, chip, start_block);
			if (nouse == 0xffffffff) {
				PHY_ERR("[NE]boot but not data!\n");
				return PHY_FAILURE;
			} else {
				PHY_DBG("[NE]boot rebuild data!\n");
				if (read_mbr_partition_v3(nand_info, chip,
							  start_block) != 0) {
					PHY_ERR("[NE]cannot find mbr!\n");
					return PHY_FAILURE;
				}
				read_factory_block_v3(nand_info, chip,
						      start_block);
				MEMCPY(boot->mbr.ndata, nand_info->mbr_data,
				       sizeof(PARTITION_MBR));
				MEMCPY(boot->partition.ndata,
				       nand_info->partition,
				       sizeof(nand_info->partition));
				MEMCPY(boot->factory_block.ndata,
				       nand_info->factory_bad_block,
				       FACTORY_BAD_BLOCK_SIZE);
				nand_info->boot->logic_start_block = nouse + 1;
				nand_info->boot->no_use_block =
				    nand_info->boot->logic_start_block;
			}
		} else {
			// nand3.0 boot start
			MEMCPY(nand_info->mbr_data, boot->mbr.ndata,
			       sizeof(PARTITION_MBR));
			read_partition_v3(nand_info, boot);
			MEMCPY(nand_info->factory_bad_block,
			       boot->factory_block.ndata,
			       FACTORY_BAD_BLOCK_SIZE);
		}
	}

	nand_info->no_used_block_addr.Chip_NO = chip;
	nand_info->no_used_block_addr.Block_NO = boot->no_use_block;

	if (build_all_phy_partition_v2(nand_info) != 0) {
		PHY_ERR("[NE]build all phy partition fail!\n");
		return PHY_FAILURE;
	}

	if (nand_info->FirstBuild == 1) {
		MEMCPY(boot->partition.ndata, nand_info->partition,
		       sizeof(nand_info->partition));
		MEMCPY(boot->factory_block.ndata, nand_info->factory_bad_block,
		       FACTORY_BAD_BLOCK_SIZE);
		print_factory_block_table_v2(nand_info);

		// print_mbr_data(&boot->mbr.data);
	}

	print_nand_info_v2(nand_info);

	print_boot_info(nand_info);
	// print_physic_info(nand_info->boot);
	// print_physic_info(phyinfo_buf);

	return 0;
}

/*****************************************************************************
 *Name         :
 *Description  :
 *Parameter    :
 *Return       :
 *Note         :
*****************************************************************************/
void print_boot_info(struct _nand_info *nand_info)
{
	int i;

	PHY_DBG("[ND]boot :0x%x\n", nand_info->boot);
	PHY_DBG("[ND]boot->magic :0x%x\n", nand_info->boot->magic);
	PHY_DBG("[ND]boot->len :0x%x\n", nand_info->boot->len);
	PHY_DBG("[ND]boot->no_use_block :0x%x\n",
		nand_info->boot->no_use_block);
	PHY_DBG("[ND]boot->uboot_start_block :0x%x\n",
		nand_info->boot->uboot_start_block);
	PHY_DBG("[ND]boot->uboot_next_block :0x%x\n",
		nand_info->boot->uboot_next_block);
	PHY_DBG("[ND]boot->logic_start_block :0x%x\n",
		nand_info->boot->logic_start_block);

	PHY_DBG("[ND]mbr len :%d\n", sizeof(_MBR));
	PHY_DBG("[ND]_PARTITION len :%d\n", sizeof(_PARTITION));
	PHY_DBG("[ND]_NAND_STORAGE_INFO len :%d\n", sizeof(_NAND_STORAGE_INFO));
	PHY_DBG("[ND]_FACTORY_BLOCK len :%d\n", sizeof(_FACTORY_BLOCK));

	//    PHY_DBG("============mbr===============\n");
	//    for(i=-0;i<256;i++)
	//    {
	//        PHY_DBG("%08x ",nand_info->boot->mbr.ndata[i]);
	//        if(((i+1) % 16) == 0)
	//        {
	//            PHY_DBG("\n");
	//        }
	//    }
	//    PHY_DBG("============partition===============\n");
	//
	//    for(i=-0;i<128;i++)
	//    {
	//        PHY_DBG("%08x ",nand_info->boot->partition.ndata[i]);
	//        if(((i+1) % 16) == 0)
	//        {
	//            PHY_DBG("\n");
	//        }
	//    }
	//
	//    PHY_DBG("===========storage_info================\n");
	//    for(i=-0;i<128;i++)
	//    {
	//        PHY_DBG("%08x ",nand_info->boot->storage_info.ndata[i]);
	//        if(((i+1) % 16) == 0)
	//        {
	//            PHY_DBG("\n");
	//        }
	//    }
	//
	//    PHY_DBG("==============factory_block=============\n");
	//    for(i=-0;i<256;i++)
	//    {
	//        PHY_DBG("%08x ",nand_info->boot->factory_block.ndata[i]);
	//        if(((i+1) % 16) == 0)
	//        {
	//            PHY_DBG("\n");
	//        }
	//    }
}
/*****************************************************************************
 *Name         :
 *Description  :
 *Parameter    :
 *Return       :
 *Note         :
*****************************************************************************/
unsigned int get_no_use_block_v3(struct _nand_info *nand_info, uchar chip,
				 uint16 start_block)
{
	unsigned int i, nDieNum, nBlkNum, nPage, block, good;
	unsigned char spare[BYTES_OF_USER_PER_PAGE];
	uint64 SectBitmap;
	unsigned int ret;

	good = 0;
	nDieNum = chip;
	nBlkNum = start_block + 3;
	nPage = 0;
	SectBitmap = nand_info->FullBitmap;

	for (i = 0; i < 50; i++) {
		if (!BlockCheck(nDieNum, nBlkNum)) {
			PHY_VirtualPageRead(nDieNum, nBlkNum, nPage, SectBitmap,
					    nand_info->temp_page_buf, spare);
			if ((spare[0] == 0xff) && (spare[1] == 0xaa) &&
			    (spare[2] == 0xdd)) {
				return nBlkNum;
			}
		}
		nBlkNum++;
		if (nBlkNum == nand_info->BlkPerChip) {
			PHY_ERR("[NE]1 can not find no use block !!\n");
			return 0xffffffff;
		}
	}
	return 0xffffffff;
}

/*****************************************************************************
 *Name         :
 *Description  :
 *Parameter    :
 *Return       :
 *Note         :
*****************************************************************************/
int nand_info_init_v2(struct _nand_info *nand_info, uchar chip,
		      uint16 start_block, uchar *mbr_data)
{
	unsigned int nDieNum, nBlkNum, nPage;
	unsigned char spare[BYTES_OF_USER_PER_PAGE];
	uint64 SectBitmap;
	unsigned int ret;
	unsigned int bytes_per_page;

	bytes_per_page = nand_info->SectorNumsPerPage;
	bytes_per_page <<= 9;

	MEMSET(spare, 0xff, BYTES_OF_USER_PER_PAGE);

	MEMSET(nand_info->mbr_data, 0xff, sizeof(PARTITION_MBR));

	nand_info->mini_free_block_first_reserved = MIN_FREE_BLOCK_NUM_V2;
	nand_info->mini_free_block_reserved = MIN_FREE_BLOCK_REMAIN;

	nand_info->new_bad_page_addr = 0xffff;

	nand_info->FirstBuild = 0;

	///////////////////////////////////////////////////////////////////////
	// check mbr
	if (mbr_data != NULL) {
		nDieNum = chip;
		nBlkNum = start_block;
		while (1) {
			nPage = 0;
			SectBitmap = nand_info->FullBitmap;
			if (!BlockCheck(nDieNum, nBlkNum)) {
				ret = read_partition_v2(nand_info, nDieNum,
							nBlkNum);
				if (ret == 0) {
					PHY_DBG("[NE]not erase MP!!!\n");
					if (check_mbr_v2(
						nand_info,
						(PARTITION_MBR *)mbr_data) ==
					    0) {
						break;
					} else {
						PHY_ERR("[NE]not erase MP but "
							"mbr not the same "
							"v2!\n");
						return PHY_FAILURE;
					}
				} else {
					PHY_DBG("[NE]erase MP!!!\n");
					nand_info->FirstBuild = 1;
					break;
				}
			} else {
				nBlkNum++;
				if (nBlkNum == nand_info->BlkPerChip) {
					PHY_ERR("[NE]1 can not find mbr table "
						"!!\n");
					return PHY_FAILURE;
				}
			}
		}
	}

	///////////////////////////////////////////////////////////////////////
	// get mbr data
	nand_info->mbr_block_addr.Chip_NO = chip;
	nand_info->mbr_block_addr.Block_NO = start_block;
	while (1) {
		///////////////////////////////////////////////////////////////
		// get mbr table
		nDieNum = nand_info->mbr_block_addr.Chip_NO;
		nBlkNum = nand_info->mbr_block_addr.Block_NO;
		nPage = 0;
		SectBitmap = nand_info->FullBitmap;
		if (!BlockCheck(nDieNum, nBlkNum)) {
			if (nand_info->FirstBuild == 1) {
				MEMCPY(nand_info->mbr_data, mbr_data,
				       sizeof(PARTITION_MBR));
				write_mbr_v2(nand_info);
				break;
			} else {
				ret = read_mbr_v2(nand_info, nDieNum, nBlkNum);
				if (ret == 0) {
					PHY_DBG("[NE]get mbr_data table\n");
					break;
				} else {
					PHY_ERR("[NE]no mbr_data table %d "
						"!!!!!!!\n",
						nBlkNum);
					return PHY_FAILURE;
				}
			}
		} else {
			nand_info->mbr_block_addr.Block_NO++;
			if (nand_info->mbr_block_addr.Block_NO ==
			    nand_info->BlkPerChip) {
				PHY_ERR("[NE]can not find mbr table !!\n");
				return PHY_FAILURE;
			}
		}
	}

	nand_info->no_used_block_addr.Chip_NO =
	    nand_info->mbr_block_addr.Chip_NO;
	nand_info->no_used_block_addr.Block_NO =
	    nand_info->mbr_block_addr.Block_NO + 1;
	///////////////////////////////////////////////////////////////////////
	// get factory_bad_block table
	nand_info->bad_block_addr.Chip_NO = nand_info->mbr_block_addr.Chip_NO;
	nand_info->bad_block_addr.Block_NO =
	    nand_info->mbr_block_addr.Block_NO + 1;
	while (1) {
		nDieNum = nand_info->bad_block_addr.Chip_NO;
		nBlkNum = nand_info->bad_block_addr.Block_NO;
		nPage = 0;
		SectBitmap = nand_info->FullBitmap;
		if (!BlockCheck(nDieNum, nBlkNum)) {
			if (nand_info->FirstBuild == 1) {
				break;
			} else {
				PHY_VirtualPageRead(
				    nDieNum, nBlkNum, nPage, SectBitmap,
				    nand_info->temp_page_buf, spare);
				if ((spare[1] == 0xaa) && (spare[2] == 0xbb)) {
					PHY_DBG("[ND]ok  get factory_bad_block "
						"table!\n");
					MEMCPY(nand_info->factory_bad_block,
					       nand_info->temp_page_buf,
					       FACTORY_BAD_BLOCK_SIZE);
					print_factory_block_table_v2(nand_info);
					break;
				} else if ((spare[1] == 0xaa) &&
					   (spare[2] == 0xdd)) {
					PHY_ERR("[NE]no factory_bad_block "
						"table!!!!!!!\n");
					return PHY_FAILURE;
				} else {
					PHY_ERR("[NE]read factory_bad_block "
						"table error2\n");
				}
			}
		}
		nand_info->bad_block_addr.Block_NO++;
		if (nand_info->bad_block_addr.Block_NO ==
		    nand_info->BlkPerChip) {
			PHY_ERR(
			    "[NE]can not find factory_bad_block table !!\n");
			return PHY_FAILURE;
		}
	}
	nand_info->no_used_block_addr.Chip_NO =
	    nand_info->bad_block_addr.Chip_NO;
	nand_info->no_used_block_addr.Block_NO =
	    nand_info->bad_block_addr.Block_NO + 1;
	///////////////////////////////////////////////////////////////////////
	// get new_bad_block table
	nand_info->new_bad_block_addr.Chip_NO =
	    nand_info->bad_block_addr.Chip_NO;
	nand_info->new_bad_block_addr.Block_NO =
	    nand_info->bad_block_addr.Block_NO + 1;
	while (1) {
		nDieNum = nand_info->new_bad_block_addr.Chip_NO;
		nBlkNum = nand_info->new_bad_block_addr.Block_NO;
		nPage = 0;
		SectBitmap = nand_info->FullBitmap;
		if (!BlockCheck(nDieNum, nBlkNum)) {
			if (nand_info->FirstBuild == 1) {
				if (PHY_VirtualBlockErase(nDieNum, nBlkNum) !=
				    0) {
					PHY_ERR("[NE]init new_bad_block table "
						"error!!\n");
					return PHY_FAILURE;
				}
				write_new_block_table_v2_new_first_build(
				    nand_info);
				break;
			} else {
				nand_info->new_bad_page_addr =
				    read_new_bad_block_table_v2(nand_info);
				break;
			}
		}
		nand_info->new_bad_block_addr.Block_NO++;
		if (nand_info->new_bad_block_addr.Block_NO ==
		    nand_info->BlkPerChip) {
			PHY_ERR("[NE]can not find new_bad_block table!!\n");
			return PHY_FAILURE;
		}
	}
	nand_info->no_used_block_addr.Chip_NO =
	    nand_info->new_bad_block_addr.Chip_NO;
	nand_info->no_used_block_addr.Block_NO =
	    nand_info->new_bad_block_addr.Block_NO + 1;
	///////////////////////////////////////////////////////////////////////
	// get no use block
	nand_info->no_used_block_addr.Chip_NO =
	    nand_info->new_bad_block_addr.Chip_NO;
	nand_info->no_used_block_addr.Block_NO =
	    nand_info->new_bad_block_addr.Block_NO + 1;
	while (1) {
		nDieNum = nand_info->no_used_block_addr.Chip_NO;
		nBlkNum = nand_info->no_used_block_addr.Block_NO;
		nPage = 0;
		SectBitmap = nand_info->FullBitmap;
		if (!BlockCheck(nDieNum, nBlkNum)) {
			if (nand_info->FirstBuild == 1) {
				write_no_use_block_v2(nand_info);
				break;
			} else {
				PHY_VirtualPageRead(
				    nDieNum, nBlkNum, nPage, SectBitmap,
				    nand_info->temp_page_buf, spare);
				if ((spare[1] == 0xaa) && (spare[2] == 0xdd)) {
					break;
				} else {
					PHY_ERR("[NE]can not find no use block "
						"%d!\n",
						nBlkNum);
					return PHY_FAILURE;
				}
			}
		}
		nand_info->no_used_block_addr.Block_NO++;
		if (nand_info->no_used_block_addr.Block_NO ==
		    nand_info->BlkPerChip) {
			PHY_ERR("[NE]can not find no_used_block!!\n");
			return PHY_FAILURE;
		}
	}

	nand_info->no_used_block_addr.Block_NO =
	    nand_info->no_used_block_addr.Block_NO + 1;

	///////////////////////////////////////////////////////////////////////

	nand_info->phy_partition_head = NULL;

	PHY_DBG("[ND]build all_phy partition start!\n");

	if (nand_info->FirstBuild == 1) {
		get_partition_v2(nand_info);
	} else {
		ret = read_partition_v2(nand_info,
					nand_info->mbr_block_addr.Chip_NO,
					nand_info->mbr_block_addr.Block_NO);
		if (ret != 0) {
			PHY_ERR("[NE]read partition partition fail!\n");
			return PHY_FAILURE;
		}
	}

	if (build_all_phy_partition_v2(nand_info) != 0) {
		PHY_ERR("[NE]build all phy partition fail!\n");
		return PHY_FAILURE;
	}

	if (nand_info->FirstBuild == 1) {
		write_partition_v2(nand_info);
		write_factory_block_table_v2(nand_info);
	}

	print_nand_info_v2(nand_info);

	return 0;
}

/*****************************************************************************
 *Name         :
 *Description  :
 *Parameter    :
 *Return       :
 *Note         :
*****************************************************************************/
extern int nand_secure_storage_first_build(unsigned int start_block);

int nand_info_init(struct _nand_info *nand_info, uchar chip, uint16 start_block,
		   uchar *mbr_data)
{
	unsigned int nDieNum, nBlkNum;
	int ret = -1, flag = 0;
	unsigned int bytes_per_page;
	int block_temp, real_start_block;

	bytes_per_page = nand_info->SectorNumsPerPage;
	bytes_per_page <<= 9;
	//   nand_info->temp_page_buf = MALLOC(bytes_per_page);

	nand_info->temp_page_buf = MALLOC(bytes_per_page);
	if (nand_info->temp_page_buf == NULL)
		PHY_ERR("[NE]%s: malloc fail for temp_page_buf!\n", __func__);

	nand_info->factory_bad_block = MALLOC(FACTORY_BAD_BLOCK_SIZE);
	if (nand_info->factory_bad_block == NULL)
		PHY_ERR("[NE]%s: malloc fail for factory_bad_block!\n",
			__func__);

	nand_info->new_bad_block = MALLOC(PHY_PARTITION_BAD_BLOCK_SIZE);
	if (nand_info->new_bad_block == NULL)
		PHY_ERR("[NE]%s: malloc fail for new_bad_block!\n", __func__);

	nand_info->mbr_data = MALLOC(sizeof(PARTITION_MBR));
	if (nand_info->mbr_data == NULL)
		PHY_ERR("[NE]%s: malloc fail for mbr_data!\n", __func__);

	MEMSET(nand_info->factory_bad_block, 0xff, FACTORY_BAD_BLOCK_SIZE);
	MEMSET(nand_info->new_bad_block, 0xff, PHY_PARTITION_BAD_BLOCK_SIZE);
	MEMSET(nand_info->mbr_data, 0xff, sizeof(PARTITION_MBR));

	block_temp = nand_info->boot->uboot_next_block;

	block_temp = nand_secure_storage_first_build(block_temp);

	block_temp = block_temp + nand_info->boot->physic_block_reserved;

	real_start_block = block_temp;
	if (nand_get_twoplane_flag() == 1) {
		real_start_block = real_start_block / 2;
		if (block_temp % 2)
			real_start_block++;
	}

	ret = nand_info_init_v3(nand_info, chip, real_start_block, mbr_data);
	if (ret == 0)
		return 0;
	else {
		// return PHY_FAILURE;
	}
	PHY_DBG("[ND]use old nand info init!\n");

	nDieNum = chip;
	nBlkNum = real_start_block;

	//    debug_read_chip(nand_info);
	//    while(1);
	/*
	while (1) {
		if (!BlockCheck(nDieNum, nBlkNum)) {
			flag =
			    check_partition_mbr_v2(nand_info, nDieNum, nBlkNum);
			break;
		} else {
			nBlkNum++;
			if (nBlkNum == nand_info->BlkPerChip) {
				PHY_ERR("[NE]not find mbr table!!!!\n");
				return PHY_FAILURE;
			}
		}
	}

	if (flag == 1) {
		PHY_DBG("[ND]old nand info init!!\n");
		return nand_info_init_v1(nand_info, chip, real_start_block,
					 mbr_data);
	} else if (flag == 2) {
		PHY_DBG("[ND]new nand info init!!\n");
		return nand_info_init_v2(nand_info, chip, real_start_block,
					 mbr_data);
	} else {
		if (mbr_data != NULL) {
			return nand_info_init_v2(nand_info, chip,
						 real_start_block, mbr_data);
		}
		PHY_ERR("[NE]boot not find mbr table!!!!\n");
		return PHY_FAILURE;
	}*/

	return 0;
}

/*****************************************************************************
 *Name         :
 *Description  :
 *Parameter    :
 *Return       :
 *Note         :
*****************************************************************************/
int read_mbr_partition_v3(struct _nand_info *nand_info, unsigned int chip,
			  unsigned int start_block)
{
	int ret;
	unsigned int nDieNum, nBlkNum, nPage, i;
	uint64 SectBitmap;

	// get mbr table
	nDieNum = chip;
	nBlkNum = start_block;
	nPage = 0;
	SectBitmap = nand_info->FullBitmap;

	for (i = 0; i < 50; i++) {
		if (!BlockCheck(nDieNum, nBlkNum)) {
			ret =
			    check_partition_mbr_v2(nand_info, nDieNum, nBlkNum);
			if (ret == 2)
				return 0;
		}
		nBlkNum++;
		if (nBlkNum == nand_info->BlkPerChip) {
			PHY_ERR("[NE]can not find mbr table !!\n");
			return PHY_FAILURE;
		}
	}

	return PHY_FAILURE;
}

/*****************************************************************************
 *Name         :
 *Description  :
 *Parameter    :
 *Return       :
 *Note         :
*****************************************************************************/
int read_factory_block_v3(struct _nand_info *nand_info, unsigned int chip,
			  unsigned int start_block)
{
	unsigned int nDieNum, nBlkNum, nPage, ret;
	uint64 SectBitmap;
	unsigned char spare[BYTES_OF_USER_PER_PAGE];

	// get mbr table
	ret = 0;
	nDieNum = chip;
	nBlkNum = start_block + 1;
	nPage = 0;
	SectBitmap = nand_info->FullBitmap;
	while (1) {
		MEMSET(spare, 0xff, BYTES_OF_USER_PER_PAGE);
		if (!BlockCheck(nDieNum, nBlkNum)) {
			PHY_VirtualPageRead(nDieNum, nBlkNum, nPage, SectBitmap,
					    nand_info->temp_page_buf, spare);
			if ((spare[1] == 0xaa) && (spare[2] == 0xbb)) {
				PHY_DBG("[ND]ok get factory_bad_block table!  "
					"%d\n",
					nBlkNum);
				MEMCPY(nand_info->factory_bad_block,
				       nand_info->temp_page_buf,
				       FACTORY_BAD_BLOCK_SIZE);
				print_factory_block_table_v2(nand_info);
				ret = 0;
				break;
			} else if ((spare[1] == 0xaa) && (spare[2] == 0xdd)) {
				PHY_ERR(
				    "[NE]no factory_bad_block table!!!!!!!\n");
				return PHY_FAILURE;
			} else {
				PHY_ERR("[NE]read factory_bad_block table "
					"error2 %d\n",
					nBlkNum);
			}
		}
		nBlkNum++;
		if (nBlkNum == nand_info->BlkPerChip) {
			PHY_ERR("[NE]can not find mbr table !!\n");
			return PHY_FAILURE;
		}
	}

	return ret;
}

/*****************************************************************************
 *Name         :
 *Description  :
 *Parameter    :
 *Return       :
 *Note         :
*****************************************************************************/
int write_mbr_v2(struct _nand_info *nand_info)
{
	int ret, i;
	unsigned int nDieNum, nBlkNum, nPage;
	unsigned char spare[BYTES_OF_USER_PER_PAGE];
	uint64 SectBitmap;
	unsigned char *buf;

	MEMSET(spare, 0xff, BYTES_OF_USER_PER_PAGE);

	nDieNum = nand_info->mbr_block_addr.Chip_NO;
	nBlkNum = nand_info->mbr_block_addr.Block_NO;
	nPage = 0;
	SectBitmap = nand_info->FullBitmap;
	buf = nand_info->temp_page_buf;
	spare[1] = 0xaa;
	spare[2] = 0xaa;

	ret = PHY_VirtualBlockErase(nDieNum, nBlkNum);
	if (ret != 0)
		PHY_ERR("[NE]mbr_block_addr erase error?!\n");
	MEMCPY(buf, nand_info->mbr_data, sizeof(PARTITION_MBR));

	PHY_DBG("[ND]new write mbr %d!\n", nBlkNum);

	for (i = 0; i < 10; i++) {
		ret = PHY_VirtualPageWrite(nDieNum, nBlkNum, i, SectBitmap, buf,
					   spare);
		if (ret != 0) {
			PHY_ERR("[NE]mbr_block_addr write error %d %d!\n",
				nBlkNum, i);
		}
	}

	PHY_DBG("[ND]new write mbr end!\n");
	/*
	 *   nDieNum = nand_info->mbr_block_addr.Chip_NO;
	 *   nBlkNum = nand_info->mbr_block_addr.Block_NO;
	 *   nPage = 0;
	 *   SectBitmap = nand_info->FullBitmap;
	 *   buf = nand_info->temp_page_buf;
	 *   spare[1] = 0xff;
	 *   spare[2] = 0xff;
	 *   for(i=0;i<nand_info->PageNumsPerBlk;i++)
	 *   {
	 *	PHY_VirtualPageRead(nDieNum,nBlkNum,i,
	 *			    SectBitmap,nand_info->temp_page_buf,spare);
	 *	if((spare[1] != 0xaa) || (spare[2] != 0xaa))
	 *	{
	 *	    PHY_ERR("[NE]mbr_block_addr write error %d %d %d
	 *  !\n",nDieNum,nBlkNum,i);
	 *	}
	 *   }
	 */
	return 0;
}

/*****************************************************************************
 *Name         :
 *Description  :
 *Parameter    :
 *Return       :
 *Note         :
*****************************************************************************/
int read_mbr_v2(struct _nand_info *nand_info, unsigned int nDieNum,
		unsigned int nBlkNum)
{
	int ret = 1, i;
	unsigned int nPage;
	unsigned char spare[BYTES_OF_USER_PER_PAGE];
	uint64 SectBitmap;

	MEMSET(spare, 0xff, BYTES_OF_USER_PER_PAGE);
	SectBitmap = nand_info->FullBitmap;

	PHY_DBG("[NE]mbr read %d\n", nBlkNum);
	for (i = 0; i < 10; i++) {
		nPage = i;
		ret = PHY_VirtualPageRead(nDieNum, nBlkNum, nPage, SectBitmap,
					  nand_info->temp_page_buf, spare);
		if ((spare[1] == 0xaa) && (spare[2] == 0xaa) && (ret >= 0)) {
			PHY_DBG("[NE]mbr read ok!\n");
			MEMCPY(nand_info->mbr_data, nand_info->temp_page_buf,
			       sizeof(PARTITION_MBR));
			ret = 0;
			break;
		} else if ((spare[1] == 0xaa) && (spare[2] == 0xdd)) {
			ret = -1;
			break;
		} else if (ret > 0) {
			ret = 1;
			break;
		} else {
			ret = 1;
		}
	}
	PHY_DBG("[NE]mbr read end!\n");
	return ret;
}
/*****************************************************************************
 *Name         :
 *Description  :
 *Parameter    :
 *Return       :
 *Note         :
*****************************************************************************/
int check_mbr_v3(struct _nand_info *nand_info, PARTITION_MBR *mbr)
{
	int i, m, j, k, ret;
	unsigned int com_len[ND_MAX_PARTITION_COUNT];

	MEMSET(com_len, 0xff, ND_MAX_PARTITION_COUNT << 2);

	// PHY_DBG("[NE]=================================mbr1!\n");
	for (i = 0, j = 0; i < ND_MAX_PARTITION_COUNT; i++) {
		if ((mbr->array[i].len == 0xffffffff) ||
		    (mbr->array[i].len == 0)) {
			break;
		}
		// PHY_DBG("len: 0x%x!\n",mbr->array[i].len);
		com_len[j] = mbr->array[i].len;
		j++;
	}

	// PHY_DBG("[NE]=================================mbr2!\n");
	for (i = 0, k = 0, ret = 0; i < nand_info->partition_nums; i++) {
		for (m = 0; m < MAX_PART_COUNT_PER_FTL; m++) {
			if ((nand_info->partition[i].nand_disk[m].size ==
			     0xffffffff) ||
			    (nand_info->partition[i].nand_disk[m].size == 0)) {
				break;
			}
			// PHY_DBG("[NE]len:
			// 0x%x!\n",nand_info->partition[i].nand_disk[m].size);
			if ((com_len[k] !=
			     nand_info->partition[i].nand_disk[m].size) &&
			    (com_len[k] != 0xffffffff)) {
				PHY_DBG(
				    "[NE]len1: 0x%x len2: 0x%x!\n", com_len[j],
				    nand_info->partition[i].nand_disk[m].size);
				return 1;
			}
			k++;
		}
	}

	if ((j + 1) != k) {
		PHY_DBG("j 0x%x k 0x%x!\n", j, k);
		return 1;
	}

	// PHY_DBG("[NE]=================================!\n");

	return ret;
}

/*****************************************************************************
 *Name         :
 *Description  :
 *Parameter    :
 *Return       :
 *Note         :
*****************************************************************************/
int check_mbr_v2(struct _nand_info *nand_info, PARTITION_MBR *mbr)
{
	int i, m, j, k, ret;
	unsigned int com_len[ND_MAX_PARTITION_COUNT];

	MEMSET(com_len, 0xff, ND_MAX_PARTITION_COUNT << 2);

	// PHY_DBG("[NE]=================================mbr1!\n");
	for (i = 0, j = 0; i < ND_MAX_PARTITION_COUNT; i++) {
		if ((mbr->array[i].len == 0xffffffff) ||
		    (mbr->array[i].len == 0)) {
			break;
		}
		// PHY_DBG("len: 0x%x!\n",mbr->array[i].len);
		com_len[j] = mbr->array[i].len;
		j++;
	}

	// PHY_DBG("[NE]=================================mbr2!\n");
	for (i = 0, k = 0, ret = 0; i < nand_info->partition_nums; i++) {
		for (m = 0; m < MAX_PART_COUNT_PER_FTL; m++) {
			if ((nand_info->partition[i].nand_disk[m].size ==
			     0xffffffff) ||
			    (nand_info->partition[i].nand_disk[m].size == 0)) {
				break;
			}
			// PHY_DBG("[NE]len:
			// 0x%x!\n",nand_info->partition[i].nand_disk[m].size);
			if (com_len[k] !=
			    nand_info->partition[i].nand_disk[m].size) {
				PHY_DBG(
				    "[NE]len1: 0x%x len2: 0x%x!\n", com_len[j],
				    nand_info->partition[i].nand_disk[m].size);
				return 1;
			}
			k++;
		}
	}

	if (j != k)
		return 1;

	// PHY_DBG("[NE]=================================!\n");

	return ret;
}

/*****************************************************************************
 *Name         :
 *Description  :
 *Parameter    :
 *Return       :
 *Note         :
*****************************************************************************/
int write_partition_v2(struct _nand_info *nand_info)
{
	int ret, i;
	unsigned int nDieNum, nBlkNum, nPage;
	unsigned char spare[BYTES_OF_USER_PER_PAGE];
	uint64 SectBitmap;
	unsigned char *buf;

	MEMSET(spare, 0xff, BYTES_OF_USER_PER_PAGE);

	nDieNum = nand_info->mbr_block_addr.Chip_NO;
	nBlkNum = nand_info->mbr_block_addr.Block_NO;
	nPage = 0;
	SectBitmap = nand_info->FullBitmap;
	buf = nand_info->temp_page_buf;
	spare[1] = 0xaa;
	spare[2] = 0xee;

	MEMCPY(buf, nand_info->partition, sizeof(nand_info->partition));

	PHY_DBG("[ND]write partition %d!\n", nBlkNum);

	for (i = 10; i < nand_info->PageNumsPerBlk; i++) {
		ret = PHY_VirtualPageWrite(nDieNum, nBlkNum, i, SectBitmap, buf,
					   spare);
		if (ret != 0) {
			PHY_ERR("[NE]mbr_block_addr write error %d %d!\n",
				nBlkNum, i);
		}
	}

	PHY_DBG("[ND]write partition end!\n");

	return 0;
}

/*****************************************************************************
 *Name         :
 *Description  :
 *Parameter    :
 *Return       :
 *Note         :
*****************************************************************************/
int read_partition_v2(struct _nand_info *nand_info, unsigned int nDieNum,
		      unsigned int nBlkNum)
{
	int ret = 1, i, j;
	unsigned int nPage;
	unsigned char spare[BYTES_OF_USER_PER_PAGE];
	uint64 SectBitmap;

	MEMSET(spare, 0xff, BYTES_OF_USER_PER_PAGE);
	SectBitmap = nand_info->FullBitmap;

	PHY_DBG("[NE]mbr partition start!\n");
	for (i = 10; i < 20; i++) {
		nPage = i;
		ret = PHY_VirtualPageRead(nDieNum, nBlkNum, nPage, SectBitmap,
					  nand_info->temp_page_buf, spare);
		if ((spare[1] == 0xaa) && (spare[2] == 0xee) && (ret >= 0)) {
			PHY_DBG("[NE]mbr partition ok!\n");
			MEMCPY(nand_info->partition, nand_info->temp_page_buf,
			       sizeof(nand_info->partition));

			nand_info->partition_nums = 0;

			for (j = 0; j < MAX_PARTITION; j++) {
				if (nand_info->partition[j].size != 0)
					nand_info->partition_nums++;
			}
			ret = 0;
			break;
		} else if ((spare[1] == 0xaa) && (spare[2] == 0xdd)) {
			ret = -1;
			break;
		} else {
			ret = 1;
		}
	}
	PHY_DBG("[NE]mbr partition end!\n");
	return ret;
}

/*****************************************************************************
 *Name         :
 *Description  :
 *Parameter    :
 *Return       :
 *Note         :
*****************************************************************************/
int check_partition_mbr_v2(struct _nand_info *nand_info, unsigned int nDieNum,
			   unsigned int nBlkNum)
{
	int ret;

	ret = read_mbr_v2(nand_info, nDieNum, nBlkNum);
	if (ret != 0)
		return -1; // not find mbr

	ret = read_partition_v2(nand_info, nDieNum, nBlkNum);
	if (ret == 0)
		ret = 2; // get new mbr
	else
		ret = 1; // get old mbr

	return ret;
}

/*****************************************************************************
 *Name         :
 *Description  :
 *Parameter    :
 *Return       :
 *Note         :
*****************************************************************************/
int write_factory_block_table_v2(struct _nand_info *nand_info)
{
	int ret, i;
	unsigned int nDieNum, nBlkNum, nPage, bad_num;
	unsigned char spare[BYTES_OF_USER_PER_PAGE];
	uint64 SectBitmap;
	unsigned char *buf;

	MEMSET(spare, 0xff, BYTES_OF_USER_PER_PAGE);
	spare[1] = 0xaa;
	spare[2] = 0xbb;
	spare[3] = 0x01; // version flag
	nDieNum = nand_info->bad_block_addr.Chip_NO;
	nBlkNum = nand_info->bad_block_addr.Block_NO;
	nPage = 0;
	SectBitmap = nand_info->FullBitmap;
	buf = nand_info->temp_page_buf;

	ret = PHY_VirtualBlockErase(nDieNum, nBlkNum);
	if (ret != 0)
		PHY_ERR("[NE]bad_block_addr erase error?!\n");

	MEMCPY(buf, nand_info->factory_bad_block, FACTORY_BAD_BLOCK_SIZE);

	PHY_DBG("[NE]write_factory_block_table_v2!\n");
	for (i = 0; i < nand_info->PageNumsPerBlk; i++) {
		ret = PHY_VirtualPageWrite(nDieNum, nBlkNum, i, SectBitmap, buf,
					   spare);
		if (ret != 0) {
			PHY_ERR("[NE]bad_block_addr write error %d %d?!\n",
				nBlkNum, i);
		}
	}

	bad_num = 0;
	nDieNum = FACTORY_BAD_BLOCK_SIZE >> 2;
	for (i = 0; i < nDieNum; i++) {
		if (nand_info->factory_bad_block[i].Chip_NO != 0xffff) {
			bad_num++;
			PHY_DBG("[ND]factory bad block:%d %d!\n",
				nand_info->factory_bad_block[i].Chip_NO,
				nand_info->factory_bad_block[i].Block_NO);
		} else {
			break;
		}
	}
	PHY_DBG("[ND]factory bad block num:%d!\n", bad_num);

	return 0;
}

/*****************************************************************************
 *Name         :
 *Description  :
 *Parameter    :
 *Return       :
 *Note         :
*****************************************************************************/
int print_factory_block_table_v2(struct _nand_info *nand_info)
{
	int ret, i, nDieNum;

	nDieNum = FACTORY_BAD_BLOCK_SIZE >> 2;
	for (i = 0; i < nDieNum; i++) {
		if (nand_info->factory_bad_block[i].Chip_NO != 0xffff) {
			PHY_DBG("[ND]factory bad block:%d %d!\n",
				nand_info->factory_bad_block[i].Chip_NO,
				nand_info->factory_bad_block[i].Block_NO);
		} else {
			break;
		}
	}
}

/*****************************************************************************
 *Name         :
 *Description  :
 *Parameter    :
 *Return       :
 *Note         :
*****************************************************************************/
#if 0
int write_new_block_table_v2(struct _nand_info *nand_info)
{
	int ret;
	unsigned int nDieNum, nBlkNum, nPage;
	unsigned char spare[BYTES_OF_USER_PER_PAGE];
	uint64 SectBitmap;
	unsigned char *buf;

	MEMSET(spare, 0xff, BYTES_OF_USER_PER_PAGE);
	spare[1] = 0xaa;
	spare[2] = 0xcc;
	nDieNum = nand_info->new_bad_block_addr.Chip_NO;
	nBlkNum = nand_info->new_bad_block_addr.Block_NO;
	SectBitmap = nand_info->FullBitmap;
	buf = nand_info->temp_page_buf;

	nand_info->new_bad_page_addr++;
	if (nand_info->new_bad_page_addr == nand_info->PageNumsPerBlk) {
		ret = PHY_VirtualBlockErase(nDieNum, nBlkNum);
		if (ret != 0)
			PHY_ERR("[NE]new_bad_block_addr erase error?!\n");
		nand_info->new_bad_page_addr = 0;
	}

	nPage = nand_info->new_bad_page_addr;

	MEMCPY(buf, nand_info->new_bad_block, PHY_PARTITION_BAD_BLOCK_SIZE);

	PHY_DBG("[NE]write_new_bad_block_table %d,%d!\n", nBlkNum, nPage);
	ret = PHY_VirtualPageWrite(nDieNum, nBlkNum, nPage, SectBitmap, buf,
				    spare);
	if (ret != 0)
		PHY_ERR("[NE]bad_block_addr write error?!\n");

	return 0;
}
#else
int write_new_block_table_v2_old(struct _nand_info *nand_info)
{
	int ret;
	unsigned int nDieNum, nBlkNum, nPage;
	unsigned char spare[BYTES_OF_USER_PER_PAGE];
	uint64 SectBitmap;
	unsigned char *buf;

	MEMSET(spare, 0xff, BYTES_OF_USER_PER_PAGE);
	spare[1] = 0xaa;
	spare[2] = 0xcc;
	nDieNum = nand_info->new_bad_block_addr.Chip_NO;
	nBlkNum = nand_info->new_bad_block_addr.Block_NO;
	SectBitmap = nand_info->FullBitmap;
	buf = nand_info->temp_page_buf;

	nand_info->new_bad_page_addr++;
	if (nand_info->new_bad_page_addr == nand_info->PageNumsPerBlk) {
		ret = PHY_VirtualBlockErase(nDieNum, nBlkNum);
		if (ret != 0)
			PHY_ERR("[NE]new_bad_block_addr erase error?!\n");
		nand_info->new_bad_page_addr = 0;
	}

	nPage = nand_info->new_bad_page_addr;

	MEMCPY(buf, nand_info->new_bad_block, PHY_PARTITION_BAD_BLOCK_SIZE);

	PHY_DBG("[NE]write_new_bad_block_table %d,%d!\n", nBlkNum, nPage);
	ret = PHY_VirtualPageWrite(nDieNum, nBlkNum, nPage, SectBitmap, buf,
				   spare);
	if (ret != 0)
		PHY_ERR("[NE]bad_block_addr write error?!\n");

	return 0;
}

int write_new_block_table_v2_new(struct _nand_info *nand_info)
{
	int ret;
	unsigned int nDieNum, nBlkNum, nPage, i;
	unsigned char spare[BYTES_OF_USER_PER_PAGE];
	uint64 SectBitmap;
	unsigned char *buf;

	MEMSET(spare, 0xff, BYTES_OF_USER_PER_PAGE);
	spare[1] = 0xaa;
	spare[2] = 0xcc;
	spare[3] = 0x01;
	nDieNum = nand_info->new_bad_block_addr.Chip_NO;
	nBlkNum = nand_info->new_bad_block_addr.Block_NO;
	SectBitmap = nand_info->FullBitmap;
	buf = nand_info->temp_page_buf;

	PHY_DBG("[NE]write_new_bad_block_table new format %d!\n", nBlkNum);

	ret = PHY_VirtualBlockErase(nDieNum, nBlkNum);
	if (ret != 0)
		PHY_ERR("[NE]new_bad_block_addr erase error?!\n");

	MEMCPY(buf, nand_info->new_bad_block, PHY_PARTITION_BAD_BLOCK_SIZE);

	for (i = 0; i < nand_info->PageNumsPerBlk; i++) {
		ret = PHY_VirtualPageWrite(nDieNum, nBlkNum, i, SectBitmap, buf,
					   spare);
		if (ret != 0)
			PHY_ERR("[NE]bad_block_addr write error,page:%d!\n", i);
	}
	return 0;
}

int write_new_block_table_v2(struct _nand_info *nand_info)
{
	int ret;
	unsigned int nDieNum, nBlkNum, nPage;
	unsigned char spare[BYTES_OF_USER_PER_PAGE];
	uint64 SectBitmap;
	unsigned char *buf;

	nDieNum = nand_info->new_bad_block_addr.Chip_NO;
	nBlkNum = nand_info->new_bad_block_addr.Block_NO;
	SectBitmap = nand_info->FullBitmap;
	buf = nand_info->temp_page_buf;

	MEMSET(spare, 0xff, BYTES_OF_USER_PER_PAGE);
	PHY_VirtualPageRead(nDieNum, nBlkNum, 0, SectBitmap,
			    nand_info->temp_page_buf, spare);
	if ((spare[1] == 0xaa) && (spare[2] == 0xcc) && (spare[3] == 0xff) &&
	    (spare[0] == 0xff)) {
		PHY_DBG("[ND] new bad block table old format!\n");
		write_new_block_table_v2_old(nand_info);
	} else if ((spare[1] == 0xff) && (spare[2] == 0xff) &&
		   (spare[0] == 0xff)) {
		PHY_DBG("[ND]new bad block table old format(free block)\n");
		write_new_block_table_v2_old(nand_info);
	} else if ((spare[1] == 0xaa) && (spare[2] == 0xcc) &&
		   (spare[3] == 0x01) && (spare[0] == 0xff)) {
		PHY_DBG("[NE]new_bad_block table new format!\n");
		write_new_block_table_v2_new(nand_info);
	}

	return 0;
}

int write_new_block_table_v2_new_first_build(struct _nand_info *nand_info)
{
	int ret;
	unsigned int nDieNum, nBlkNum, nPage, i;
	unsigned char spare[BYTES_OF_USER_PER_PAGE];
	uint64 SectBitmap;
	unsigned char *buf;

	MEMSET(spare, 0xff, BYTES_OF_USER_PER_PAGE);
	spare[1] = 0xaa;
	spare[2] = 0xcc;
	spare[3] = 0x01;
	nDieNum = nand_info->new_bad_block_addr.Chip_NO;
	nBlkNum = nand_info->new_bad_block_addr.Block_NO;
	SectBitmap = nand_info->FullBitmap;
	buf = nand_info->temp_page_buf;

	PHY_DBG("[NE]write_new_bad_block_table new format first build %d!\n",
		nBlkNum);

	ret = PHY_VirtualBlockErase(nDieNum, nBlkNum);
	if (ret != 0)
		PHY_ERR("[NE]new_bad_block_addr erase error?!\n");

	MEMSET(buf, 0xff, PHY_PARTITION_BAD_BLOCK_SIZE);

	for (i = 0; i < nand_info->PageNumsPerBlk; i++) {
		ret = PHY_VirtualPageWrite(nDieNum, nBlkNum, i, SectBitmap, buf,
					   spare);
		if (ret != 0)
			PHY_ERR("[NE]bad_block_addr write error,page:%d!\n", i);
	}
	return 0;
}

#endif
/*****************************************************************************
 *Name         :
 *Description  :
 *Parameter    :
 *Return       :
 *Note         :
*****************************************************************************/
int write_no_use_block_v2(struct _nand_info *nand_info)
{
	int ret, i;
	unsigned int nDieNum, nBlkNum, nPage;
	unsigned char spare[BYTES_OF_USER_PER_PAGE];
	uint64 SectBitmap;
	unsigned char *buf;

	MEMSET(spare, 0xff, BYTES_OF_USER_PER_PAGE);
	spare[1] = 0xaa;
	spare[2] = 0xdd;
	nDieNum = nand_info->no_used_block_addr.Chip_NO;
	nBlkNum = nand_info->no_used_block_addr.Block_NO;
	nPage = 0;
	SectBitmap = nand_info->FullBitmap;
	buf = nand_info->temp_page_buf;

	ret = PHY_VirtualBlockErase(nDieNum, nBlkNum);
	if (ret != 0)
		PHY_ERR("[NE]new_write no_use_block erase error?!\n");

	PHY_DBG("[ND]new_write_no use_block!\n");
	for (i = 0; i < nand_info->PageNumsPerBlk; i++) {
		ret = PHY_VirtualPageWrite(nDieNum, nBlkNum, i, SectBitmap, buf,
					   spare);
		if (ret != 0) {
			PHY_ERR("[NE]new_write no_use_block write error?!\n");
			return PHY_FAILURE;
		}
	}

	return 0;
}

/*****************************************************************************
 *Name         :
 *Description  :
 *Parameter    :
 *Return       :
 *Note         :
*****************************************************************************/
#if 0
unsigned short read_new_bad_block_table_v2(struct _nand_info *nand_info)
{
	unsigned short num, i;
	unsigned int nDieNum, nBlkNum, nPage;
	unsigned char spare[BYTES_OF_USER_PER_PAGE];
	uint64 SectBitmap;
	unsigned char *buf;

	nDieNum = nand_info->new_bad_block_addr.Chip_NO;
	nBlkNum = nand_info->new_bad_block_addr.Block_NO;
	SectBitmap = nand_info->FullBitmap;
	buf = nand_info->temp_page_buf;

	num = 0xffff;

	for (i = 0; i < nand_info->PageNumsPerBlk; i++) {
		MEMSET(spare, 0xff, BYTES_OF_USER_PER_PAGE);
		PHY_VirtualPageRead(nDieNum, nBlkNum, i, SectBitmap,
				    nand_info->temp_page_buf, spare);
		if ((spare[1] == 0xaa) && (spare[2] == 0xcc) &&
		   (spare[0] == 0xff)) {
			PHY_DBG("[ND]ok get a new bad table!\n");
			MEMCPY(nand_info->new_bad_block,
			       nand_info->temp_page_buf,
			       PHY_PARTITION_BAD_BLOCK_SIZE);
			num = i;
		} else if ((spare[1] == 0xff) && (spare[2] == 0xff) &&
			 (spare[0] == 0xff)) {
			PHY_DBG("[ND]new bad block last first use page:%d\n",
				i);
			break;
		} else {
			PHY_ERR("[NE]read new_bad_block table error:%d!\n", i);
		}
	}

	nDieNum = PHY_PARTITION_BAD_BLOCK_SIZE >> 2;

	for (i = 0; i < nDieNum; i++) {
		if (nand_info->new_bad_block[i].Chip_NO != 0xffff) {
			PHY_DBG("[ND]new bad block:%d %d!\n",
				nand_info->new_bad_block[i].Chip_NO,
				nand_info->new_bad_block[i].Block_NO);
		} else {
			break;
		}
	}

	return num;
}
#else
unsigned short read_new_bad_block_table_v2_old(struct _nand_info *nand_info)
{
	unsigned short num, i;
	unsigned int nDieNum, nBlkNum, nPage;
	unsigned char spare[BYTES_OF_USER_PER_PAGE];
	uint64 SectBitmap;
	unsigned char *buf;

	nDieNum = nand_info->new_bad_block_addr.Chip_NO;
	nBlkNum = nand_info->new_bad_block_addr.Block_NO;
	SectBitmap = nand_info->FullBitmap;
	buf = nand_info->temp_page_buf;

	num = 0xffff;

	for (i = 0; i < nand_info->PageNumsPerBlk; i++) {
		MEMSET(spare, 0xff, BYTES_OF_USER_PER_PAGE);
		PHY_VirtualPageRead(nDieNum, nBlkNum, i, SectBitmap,
				    nand_info->temp_page_buf, spare);
		if ((spare[1] == 0xaa) && (spare[2] == 0xcc) &&
		    (spare[0] == 0xff)) {
			PHY_DBG("[ND]ok get a new bad table!\n");
			MEMCPY(nand_info->new_bad_block,
			       nand_info->temp_page_buf,
			       PHY_PARTITION_BAD_BLOCK_SIZE);
			num = i;
		} else if ((spare[1] == 0xff) && (spare[2] == 0xff) &&
			   (spare[0] == 0xff)) {
			PHY_DBG("[ND]new bad block last first use page:%d\n",
				i);
			break;
		} else {
			PHY_ERR("[NE]read new_bad_block table error:%d!\n", i);
		}
	}

	nDieNum = PHY_PARTITION_BAD_BLOCK_SIZE >> 2;

	for (i = 0; i < nDieNum; i++) {
		if (nand_info->new_bad_block[i].Chip_NO != 0xffff) {
			PHY_DBG("[ND]new bad block:%d %d!\n",
				nand_info->new_bad_block[i].Chip_NO,
				nand_info->new_bad_block[i].Block_NO);
		} else {
			break;
		}
	}

	return num;
}

unsigned short read_new_bad_block_table_v2_new(struct _nand_info *nand_info)
{
	unsigned short num, i;
	unsigned int nDieNum, nBlkNum, nPage;
	unsigned char spare[BYTES_OF_USER_PER_PAGE];
	uint64 SectBitmap;
	unsigned char *buf;

	nDieNum = nand_info->new_bad_block_addr.Chip_NO;
	nBlkNum = nand_info->new_bad_block_addr.Block_NO;
	SectBitmap = nand_info->FullBitmap;
	buf = nand_info->temp_page_buf;

	num = 0xffff;

	for (i = 0; i < nand_info->PageNumsPerBlk; i = i + 64) {
		MEMSET(spare, 0xff, BYTES_OF_USER_PER_PAGE);
		PHY_VirtualPageRead(nDieNum, nBlkNum, i, SectBitmap,
				    nand_info->temp_page_buf, spare);
		if ((spare[1] == 0xaa) && (spare[2] == 0xcc) &&
		    (spare[3] == 0x01) && (spare[0] == 0xff)) {
			PHY_DBG("[ND]ok get a new bad table!\n");
			MEMCPY(nand_info->new_bad_block,
			       nand_info->temp_page_buf,
			       PHY_PARTITION_BAD_BLOCK_SIZE);
			break;
		}
	}

	nDieNum = PHY_PARTITION_BAD_BLOCK_SIZE >> 2;

	for (i = 0; i < nDieNum; i++) {
		if (nand_info->new_bad_block[i].Chip_NO != 0xffff) {
			PHY_DBG("[ND]new bad block:%d %d!\n",
				nand_info->new_bad_block[i].Chip_NO,
				nand_info->new_bad_block[i].Block_NO);
		} else {
			break;
		}
	}

	return num;
}

unsigned short read_new_bad_block_table_v2(struct _nand_info *nand_info)
{
	unsigned short page_num, i;
	unsigned int nDieNum, nBlkNum, nPage;
	unsigned char spare[BYTES_OF_USER_PER_PAGE];
	uint64 SectBitmap;
	unsigned char *buf;

	nDieNum = nand_info->new_bad_block_addr.Chip_NO;
	nBlkNum = nand_info->new_bad_block_addr.Block_NO;
	SectBitmap = nand_info->FullBitmap;
	buf = nand_info->temp_page_buf;

	page_num = 0xffff;

	MEMSET(spare, 0xff, BYTES_OF_USER_PER_PAGE);
	PHY_VirtualPageRead(nDieNum, nBlkNum, 0, SectBitmap,
			    nand_info->temp_page_buf, spare);
	if ((spare[1] == 0xaa) && (spare[2] == 0xcc) && (spare[3] == 0xff) &&
	    (spare[0] == 0xff)) {
		PHY_DBG("[ND] new bad block table old format!\n");
		page_num = read_new_bad_block_table_v2_old(nand_info);
	} else if ((spare[1] == 0xff) && (spare[2] == 0xff) &&
		   (spare[0] == 0xff)) {
		PHY_DBG("[ND]new bad block table old format(free block)\n");
		page_num = 0;
	} else if ((spare[1] == 0xaa) && (spare[2] == 0xcc) &&
		   (spare[3] == 0x01) && (spare[0] == 0xff)) {
		PHY_DBG("[NE]new_bad_block table new format!\n");
		read_new_bad_block_table_v2_new(nand_info);
	}

	return page_num;
}

#endif
/*****************************************************************************
 *Name         :
 *Description  :
 *Parameter    :
 *Return       :
 *Note         :
*****************************************************************************/
void print_nand_info_v2(struct _nand_info *nand_info)
{
	int i;

	PHY_DBG("[ND]nand_info->type :%d\n", nand_info->type);
	PHY_DBG("[ND]nand_info->SectorNumsPerPage :%d\n",
		nand_info->SectorNumsPerPage);
	PHY_DBG("[ND]nand_info->BytesUserData :%d\n", nand_info->BytesUserData);
	PHY_DBG("[ND]nand_info->PageNumsPerBlk :%d\n",
		nand_info->PageNumsPerBlk);
	PHY_DBG("[ND]nand_info->BlkPerChip :%d\n", nand_info->BlkPerChip);
	PHY_DBG("[ND]nand_info->FirstBuild :%d\n", nand_info->FirstBuild);
	PHY_DBG("[ND]nand_info->FullBitmap :%d\n", nand_info->FullBitmap);
	PHY_DBG("[ND]nand_info->bad_block_addr.Chip_NO :%d\n",
		nand_info->bad_block_addr.Chip_NO);
	PHY_DBG("[ND]nand_info->bad_block_addr.Block_NO :%d\n",
		nand_info->bad_block_addr.Block_NO);
	PHY_DBG("[ND]nand_info->mbr_block_addr.Chip_NO :%d\n",
		nand_info->mbr_block_addr.Chip_NO);
	PHY_DBG("[ND]nand_info->mbr_block_addr.Block_NO :%d\n",
		nand_info->mbr_block_addr.Block_NO);
	PHY_DBG("[ND]nand_info->no_used_block_addr.Chip_NO :%d\n",
		nand_info->no_used_block_addr.Chip_NO);
	PHY_DBG("[ND]nand_info->no_used_block_addr.Block_NO :%d\n",
		nand_info->no_used_block_addr.Block_NO);
	PHY_DBG("[ND]nand_info->new_bad_block_addr.Chip_NO :%d\n",
		nand_info->new_bad_block_addr.Chip_NO);
	PHY_DBG("[ND]nand_info->new_bad_block_addr.Block_NO :%d\n",
		nand_info->new_bad_block_addr.Block_NO);
	PHY_DBG("[ND]nand_info->new_bad_page_addr :%d\n",
		nand_info->new_bad_page_addr);
	PHY_DBG("[ND]nand_info->partition_nums :%d\n",
		nand_info->partition_nums);

	PHY_DBG("[ND]sizeof partition:%d\n", sizeof(nand_info->partition));

	for (i = 0; i < nand_info->partition_nums; i++) {
		PHY_DBG("[ND]nand_info->partition:%d:\n", i);
		PHY_DBG("[ND]size:0x%x\n", nand_info->partition[i].size);
		PHY_DBG("[ND]cross_talk:0x%x\n",
			nand_info->partition[i].cross_talk);
		PHY_DBG("[ND]attribute:0x%x\n",
			nand_info->partition[i].attribute);
		PHY_DBG("[ND]start: chip:%d block:%d\n",
			nand_info->partition[i].start.Chip_NO,
			nand_info->partition[i].start.Block_NO);
		PHY_DBG("[ND]end  : chip:%d block:%d\n",
			nand_info->partition[i].end.Chip_NO,
			nand_info->partition[i].end.Block_NO);
	}
}

/*****************************************************************************
 *Name         :
 *Description  :
 *Parameter    :
 *Return       :
 *Note         :
*****************************************************************************/
int get_partition_v2(struct _nand_info *nand_info)
{
	PARTITION_MBR *mbr;
	unsigned int part_cnt, part_type, i, m, udisk_part_found;

	mbr = (PARTITION_MBR *)nand_info->mbr_data;

	MEMSET(nand_info->partition, 0xff, sizeof(nand_info->partition));

	PHY_DBG("[ND]nand_info->SectorNumsPerPage :0x%x\n",
		nand_info->SectorNumsPerPage);
	PHY_DBG("[ND]nand_info->PageNumsPerBlk :0x%x\n",
		nand_info->PageNumsPerBlk);
	PHY_DBG("[ND]nand_info->BlkPerChip :0x%x\n", nand_info->BlkPerChip);
	PHY_DBG("[ND]nand_info->ChipNum :0x%x\n", nand_info->ChipNum);

	udisk_part_found = 0;
	nand_info->partition_nums = 0;
	for (i = 0; i < MAX_PARTITION; i++) {
		nand_info->partition[i].size = 0;
		nand_info->partition[i].cross_talk = 0;
		nand_info->partition[i].attribute = 0;
		m = 0;
		for (part_cnt = 0; part_cnt < mbr->PartCount &&
				   part_cnt < ND_MAX_PARTITION_COUNT;
		     part_cnt++) {
			part_type = mbr->array[part_cnt].user_type & 0x0000ff00;

			if ((part_type & FTL_PARTITION_TYPE) != 0) {
				part_type >>= 8;
				if ((part_type & 0x0f) == i) {
					if ((part_type & 0x40) != 0) {
						nand_info->partition[i]
						    .cross_talk = 1;
					}

					if ((mbr->array[part_cnt].user_type &
					     0x01) != 0) {
						nand_info->partition[i]
						    .attribute = 1;
					}

					// PHY_DBG("[ND]get part partition:%d
					// part:%d size :0x%x
					// user_type:0x%x\n",i,part_cnt,mbr->array[part_cnt].len,mbr->array[part_cnt].user_type);
					nand_info->partition[i]
					    .nand_disk[m]
					    .size = mbr->array[part_cnt].len;
					nand_info->partition[i]
					    .nand_disk[m]
					    .type = 0;
					MEMCPY(nand_info->partition[i]
						   .nand_disk[m]
						   .name,
					       mbr->array[part_cnt].classname,
					       PARTITION_NAME_SIZE);
					if (nand_info->partition[i].size !=
					    0xffffffff) {
						if (mbr->array[part_cnt].len ==
						    0) {
							udisk_part_found = 1;
							nand_info->partition[i]
							    .size = 0xffffffff;
						} else {
							nand_info->partition[i]
							    .size +=
							    nand_info
								->partition[i]
								.nand_disk[m]
								.size;
						}
					}
					// nand_info->partition[i].size =
					// 0xffffffff;
					m++;
				}
			}
		}
		if (m != 0)
			nand_info->partition_nums++;
	}

// PHY_DBG("[ND]partition_nums:%x,udisk_part_found:%d\n",
//          nand_info->partition_nums, udisk_part_found);
#if 1
	for (part_cnt = 0;
	     part_cnt < mbr->PartCount && part_cnt < ND_MAX_PARTITION_COUNT;
	     part_cnt++) {
		// PHY_DBG("[ND]mbr->PartCount:%d,part_cnt:%d,len:%x\n",
		//          mbr->PartCount,part_cnt,mbr->array[part_cnt].len);
		if (mbr->array[part_cnt].len == 0) {
			if (udisk_part_found == 0)
				nand_info->partition_nums++;

			part_type = mbr->array[part_cnt].user_type & 0x0000ff00;
			// PHY_DBG("[ND]user_type:%x\n",part_type);
			part_type >>= 8;
			if ((part_type & 0x40) != 0) {
				nand_info
				    ->partition[nand_info->partition_nums - 1]
				    .cross_talk = 0;
			}
			if ((mbr->array[part_cnt].user_type & 0x01) != 0) {
				nand_info
				    ->partition[nand_info->partition_nums - 1]
				    .attribute = 1;
			}

			// PHY_DBG("[ND]get last phy_partition\n");
			nand_info->partition[nand_info->partition_nums - 1]
			    .size = 0xffffffff;
			nand_info->partition[nand_info->partition_nums - 1]
			    .nand_disk[0]
			    .size = 0xffffffff;
			nand_info->partition[nand_info->partition_nums - 1]
			    .nand_disk[0]
			    .type = 0;
			nand_info->partition[nand_info->partition_nums - 1]
			    .nand_disk[1]
			    .type = 0xffffffff;
			MEMCPY(
			    nand_info->partition[nand_info->partition_nums - 1]
				.nand_disk[0]
				.name,
			    mbr->array[part_cnt].classname,
			    PARTITION_NAME_SIZE);
			break;
		}
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
int get_partition_v3(struct _nand_info *nand_info)
{
	int last_num = 0, nums = 0, new_partition;
	PARTITION_MBR *mbr;
	unsigned int part_cnt, part_type, i, m, udisk_part_found;

	mbr = (PARTITION_MBR *)nand_info->mbr_data;

	MEMSET(nand_info->partition, 0xff, sizeof(nand_info->partition));

	PHY_DBG("[ND]nand_info->SectorNumsPerPage :0x%x\n",
		nand_info->SectorNumsPerPage);
	PHY_DBG("[ND]nand_info->PageNumsPerBlk :0x%x\n",
		nand_info->PageNumsPerBlk);
	PHY_DBG("[ND]nand_info->BlkPerChip :0x%x\n", nand_info->BlkPerChip);
	PHY_DBG("[ND]nand_info->ChipNum :0x%x\n", nand_info->ChipNum);

	udisk_part_found = 0;
	nand_info->partition[0].cross_talk = 0;
	nand_info->partition[0].attribute = 0;
	nand_info->partition_nums = 1;

	if ((mbr->array[0].user_type & 0xf000) != 0xf000) {
		new_partition = 0;
		nand_info->partition[0].size = 0xffffffff;
	} else {
		new_partition = 1;
	}

	// print_mbr_data(mbr);

	for (part_cnt = 0, m = 0;
	     part_cnt < mbr->PartCount && part_cnt < ND_MAX_PARTITION_COUNT;
	     part_cnt++) {
		if (new_partition == 0) {
			nand_info->partition[0].nand_disk[m].size =
			    mbr->array[part_cnt].len;
			nand_info->partition[0].nand_disk[m].type = 0;
			MEMCPY(nand_info->partition[0].nand_disk[m].name,
			       mbr->array[part_cnt].classname,
			       PARTITION_NAME_SIZE);
			m++;
		} else {
			nums = mbr->array[part_cnt].user_type & 0x0f00;
			nums >>= 8;
			if ((last_num != nums) && ((last_num + 1) != nums))
				return -1;
			if (last_num != nums) {
				m = 0;
				nand_info->partition[nums].size = 0;
				nand_info->partition_nums = nums + 1;
			}
			last_num = nums;
			nand_info->partition[nums].nand_disk[m].size =
			    mbr->array[part_cnt].len;
			nand_info->partition[nums].nand_disk[m].type = 0;
			MEMCPY(nand_info->partition[nums].nand_disk[m].name,
			       mbr->array[part_cnt].classname,
			       PARTITION_NAME_SIZE);
			nand_info->partition[nums].size +=
			    nand_info->partition[nums].nand_disk[m].size;
			nand_info->partition[nums].cross_talk = 0;
			nand_info->partition[nums].attribute = 0;
			if (mbr->array[part_cnt].len == 0)
				nand_info->partition[nums].size = 0xffffffff;
			m++;
		}
	}

	for (i = 0; i < nand_info->partition_nums; i++) {
		PHY_DBG("[NE]print partition %d !\n", i);
		print_partition(&nand_info->partition[i]);
	}

	return 0;
}

/*****************************************************************************
 *Name         :
 *Description  :
 *Parameter    :
 *Return       :
 *Note         :
*****************************************************************************/
void new_print_all_partition(struct _nand_info *nand_info)
{
	int i, m;

	for (i = 0; i < nand_info->partition_nums; i++) {
		PHY_DBG("[NE]partition_num: %x,size :0x%x,cross_talk %x\n", i,
			nand_info->partition[i].size,
			nand_info->partition[i].cross_talk);

		for (m = 0; m < MAX_PART_COUNT_PER_FTL; m++) {
			if (nand_info->partition[i].nand_disk[m].type ==
			    0xffffffff) {
				break;
			}
			PHY_DBG("[NE]part %s size: 0x%x type: %x\n",
				nand_info->partition[i].nand_disk[m].name,
				nand_info->partition[i].nand_disk[m].size,
				nand_info->partition[i].nand_disk[m].type);
		}
	}
}

/*****************************************************************************
 *Name         :
 *Description  :
 *Parameter    :
 *Return       :
 *Note         :
*****************************************************************************/
int build_all_phy_partition_v2(struct _nand_info *nand_info)
{
	int i;

	new_print_all_partition(nand_info);

	nand_info->phy_partition_head = NULL;

	for (i = 0; i < nand_info->partition_nums; i++) {
		nand_info->partition[i].cross_talk = 0;
		if (build_phy_partition_v2(nand_info,
					   &nand_info->partition[i]) == NULL) {
			PHY_ERR("[NE]build phy partition %d error!\n", i);
			return PHY_FAILURE;
		}
	}

	PHY_DBG("[ND]build %d phy_partition !\n", nand_info->partition_nums);
	return 0;
}

/*****************************************************************************
 *Name         :
 *Description  :
 *Parameter    :
 *Return       :
 *Note         :
*****************************************************************************/
int print_mbr_data(uchar *mbr_data)
{
	int i;

	PARTITION_MBR *mbr = (PARTITION_MBR *)mbr_data;
	NAND_PARTITION *part;
	PHY_DBG("[NE]mbr->PartCount: %d!\n", mbr->PartCount);

	for (i = 0; i < ND_MAX_PARTITION_COUNT; i++) {
		part = (NAND_PARTITION *)(&mbr->array[i]);

		PHY_DBG("part: %d!\n", i);
		PHY_DBG("[NE]part->name: %s !\n", part->classname);
		PHY_DBG("[NE]part->addr: 0x%x !\n", part->addr);
		PHY_DBG("[NE]part->len: 0x%x !\n", part->len);
		PHY_DBG("[NE]part->user_type: 0x%x !\n", part->user_type);
		PHY_DBG("[NE]part->keydata: %d !\n", part->keydata);
		PHY_DBG("[NE]part->ro: %d !\n", part->ro);
	}
}
