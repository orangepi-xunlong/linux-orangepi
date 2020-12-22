/*
 * (C) Copyright 2012
 *     wangflord@allwinnertech.com
 *
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program;
 *
 */
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/module.h>
#include "boot0_v2.h"
#include "boot1_v2.h"

extern int NAND_Print(const char *fmt, ...);
#define debug NAND_Print

extern int NAND_GetParam(boot_nand_para_t0 * nand_param);
extern int NAND_ReadBoot0(unsigned int length, void *buf);
extern void test_dram_para(void *buffer);
extern unsigned int NAND_GetValidBlkRatio(void);
/*
************************************************************************************************************
*
*                                             function
*
*    name          :
*
*    parmeters     :
*
*    return        :
*
*    note          :
*
*
************************************************************************************************************
*/
int gen_check_sum( void *boot_buf )
{
	standard_boot_file_head_t  *head_p;
	unsigned int           length;
	unsigned int           *buf;
	unsigned int            loop;
	unsigned int            i;
	unsigned int            sum;

	head_p = (standard_boot_file_head_t *)boot_buf;
	length = head_p->length;
	if( ( length & 0x3 ) != 0 )                   // must 4-byte-aligned
		return -1;
	buf = (unsigned int *)boot_buf;
	head_p->check_sum = STAMP_VALUE;              // fill stamp
	loop = length >> 2;
    /* ���㵱ǰ�ļ����ݵġ�У��͡�*/
    for( i = 0, sum = 0;  i < loop;  i++ )
    	sum += buf[i];

    /* write back check sum */
    head_p->check_sum = sum;

    return 0;
}
/*
************************************************************************************************************
*
*                                             function
*
*    name          :
*
*    parmeters     :
*
*    return        :
*
*    note          :
*
*
************************************************************************************************************
*/
int gen_uboot_check_sum( void *boot_buf )
{
	boot_file_head_t  *head_p;
	unsigned int           length;
	unsigned int           *buf;
	unsigned int            loop;
	unsigned int            i;
	unsigned int            sum;

	head_p = (boot_file_head_t *)boot_buf;
	length = head_p->length;
	if( ( length & 0x3 ) != 0 )                   // must 4-byte-aligned
		return -1;
	buf = (unsigned int *)boot_buf;
	head_p->check_sum = STAMP_VALUE;              // fill stamp
	loop = length >> 2;
	/* ���㵱ǰ�ļ����ݵġ�У��͡�*/
	for( i = 0, sum = 0;  i < loop;  i++ )
		sum += buf[i];

	/* write back check sum */
	head_p->check_sum = sum;

	return 0;
}
/*
************************************************************************************************************
*
*                                             function
*
*    name          :
*
*    parmeters     :
*
*    return        :
*
*    note          :
*
*
************************************************************************************************************
*/
int get_nand_para(void *boot_buf)
{
	boot0_file_head_t  *boot0_buf;
	char               *data_buf;
	boot_nand_para_t0   *nand_para;

	boot0_buf = (boot0_file_head_t *)boot_buf;
	data_buf  = boot0_buf->prvt_head.storage_data;
	nand_para = (boot_nand_para_t0 *)data_buf;

	NAND_GetParam(nand_para);

	return 0;
}
/*
************************************************************************************************************
*
*                                             function
*
*    name          :
*
*    parmeters     :
*
*    return        :
*
*    note          :
*
*
************************************************************************************************************
*/
int get_dram_para(void *boot_buf)
{
	boot0_file_head_t  *src_boot0;
	boot0_file_head_t  *dst_boot0;
	char *buffer = NULL;

	buffer = (char *)kmalloc(32 * 1024, GFP_KERNEL);
	if (buffer == NULL) {
		debug("get_dram_para, kmalloc failed!\n");	
		return -1;
	}
	memset(buffer, 0, 32*1024);
	if ( NAND_ReadBoot0(32 * 1024, buffer) ) {
		debug("get_dram_para, NAND_ReadBoot0() error!\n");	
		goto error;
	}

	test_dram_para(buffer);
	src_boot0 = (boot0_file_head_t *)buffer;
	dst_boot0 = (boot0_file_head_t *)boot_buf;

	//memcpy(dst_boot0->prvt_head.dram_para, src_boot0->prvt_head.dram_para, 32 * 4);
	memcpy(&dst_boot0->prvt_head, &src_boot0->prvt_head, 40 * 4);
	test_dram_para(boot_buf);
	kfree(buffer);
	buffer = NULL;
	return 0;
	
error:
	kfree(buffer);
	buffer = NULL;
	return -1;
}
/*
************************************************************************************************************
*
*                                             function
*
*    name          :
*
*    parmeters     :
*
*    return        :
*
*    note          :
*
*
************************************************************************************************************
*/
int get_nand_para_for_boot1(void *boot_buf)
{
	boot1_file_head_t  *boot1_buf;
	boot_nand_para_t0   *nand_para;

	boot1_buf = (boot1_file_head_t *)boot_buf;
	nand_para = (boot_nand_para_t0 *)boot1_buf->prvt_head.nand_spare_data;

	nand_para->good_block_ratio = NAND_GetValidBlkRatio();

	return 0;
}
