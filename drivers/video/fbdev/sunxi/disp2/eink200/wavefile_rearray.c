/*
 * Copyright (C) 2019 Allwinnertech, <liuli@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <stdio.h>
#include <malloc.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/stat.h>
#include <sys/mman.h>

#define FILE_NAME "default.awf"

#define    C_HEADER_INFO_OFFSET		0
#define    C_HEADER_TYPE_ID_OFFSET		0		//eink type(eink id)
#define    C_HEADER_VERSION_STR_OFFSET		1
#define    C_HEADER_INFO_SIZE			128		//size of awf file header
#define    C_TEMP_TBL_OFFSET (C_HEADER_INFO_OFFSET+C_HEADER_INFO_SIZE)	//temperature table offset from the beginning of the awf file
#define    C_TEMP_TBL_SIZE			32	// temperature table size

#define    C_MODE_ADDR_TBL_OFFSET	       (C_TEMP_TBL_OFFSET+C_TEMP_TBL_SIZE)//mode address table offset
#define    C_MODE_ADDR_TBL_SIZE		64

#define    C_INIT_MODE_ADDR_OFFSET		C_MODE_ADDR_TBL_OFFSET
#define    C_GC16_MODE_ADDR_OFFSET		(C_MODE_ADDR_TBL_OFFSET+4)
#define    C_GC4_MODE_ADDR_OFFSET		(C_MODE_ADDR_TBL_OFFSET+8)
#define    C_DU_MODE_ADDR_OFFSET		(C_MODE_ADDR_TBL_OFFSET+12)
#define    C_A2_MODE_ADDR_OFFSET		(C_MODE_ADDR_TBL_OFFSET+16)
#define    C_GC16_LOCAL_MODE_ADDR_OFFSET	(C_MODE_ADDR_TBL_OFFSET+20)
#define    C_GC4_LOCAL_MODE_ADDR_OFFSET		(C_MODE_ADDR_TBL_OFFSET+24)
#define    C_A2_IN_MODE_ADDR_OFFSET		(C_MODE_ADDR_TBL_OFFSET+28)
#define    C_A2_OUT_MODE_ADDR_OFFSET		(C_MODE_ADDR_TBL_OFFSET+32)
#define    C_GL16_MODE_ADDR_OFFSET		(C_MODE_ADDR_TBL_OFFSET+36)
#define    C_GLR16_MODE_ADDR_OFFSET             (C_MODE_ADDR_TBL_OFFSET+40)
#define    C_GLD16_MODE_ADDR_OFFSET             (C_MODE_ADDR_TBL_OFFSET+44)

#define    C_INIT_MODE_OFFSET			(C_MODE_ADDR_TBL_OFFSET+C_MODE_ADDR_TBL_SIZE)

#define MAX_MODE_CNT 6

enum upd_mode {
	EINK_INIT_MODE = 0x01,
	EINK_DU_MODE = 0x02,
	EINK_GC16_MODE = 0x04,
	EINK_GC4_MODE = 0x08,
	EINK_A2_MODE = 0x10,
	EINK_GL16_MODE = 0x20,
	EINK_GLR16_MODE = 0x40,
	EINK_GLD16_MODE = 0x80,
	EINK_GU16_MODE	= 0x84,

	/* use self upd win not de*/
	EINK_RECT_MODE  = 0x400,
	/* AUTO MODE: auto select update mode by E-ink driver */
	EINK_AUTO_MODE = 0x8000,

/*	EINK_NO_MERGE = 0x80000000,*/
};

typedef unsigned char u8;
typedef unsigned int u32;
typedef unsigned short u16;

struct wav_file {
	FILE	*fd;
	FILE	*out_fd;
	char	filename[64];
	char	out_name[64];
	char	panelname[128];
	char	temp_tbl[C_TEMP_TBL_SIZE];
	char	*vaddr;
	char	*remap_vaddr;
	u32	file_len;
	u32	bit_num;
};

struct wav_file wav_file;

int parse_cmdline(int argc, char **argv)
{
	int i = 0, err = 0;
	printf("\n");

	while (i < argc) {
		if (!strcmp(argv[i], "-bit_num")) {
			if (argc > i + 1) {
				i += 1;
				wav_file.bit_num = atoi(argv[i]);
				printf("bit_num = %d\n", wav_file.bit_num);
			} else {
				err++;
				printf("input bit num para err\n");
			}
		}
		i++;
	}
	if (err > 0) {
		printf("example:./test -bit_num 4(/5)\n");
		return -1;
	} else
		return 0;
}

int get_upd_mode_from_index(u32 mode)
{
	enum upd_mode upd_mode;

	switch (mode) {
	case 0:
		upd_mode = EINK_INIT_MODE;
		break;
	case 1:
		upd_mode = EINK_DU_MODE;
		break;
	case 2:
		upd_mode = EINK_GC16_MODE;
		break;
	case 3:
		upd_mode = EINK_GC4_MODE;
		break;
	case 4:
		upd_mode = EINK_A2_MODE;
		break;
	case 5:
		upd_mode = EINK_GU16_MODE;
		break;
	case 6:
		upd_mode = EINK_GLR16_MODE;
		break;
	case 7:
		upd_mode = EINK_GLD16_MODE;
		break;
	case 8:
		upd_mode = EINK_GL16_MODE;
		break;
	default:
		printf("%s:mode is err\n", __func__);
		break;
	}
	return upd_mode;
}

static u8 *get_mode_virt_address(enum upd_mode mode, char *vaddr)
{
	u8 *p_wf_file = NULL;
	u32 offset = 0;

	switch (mode & 0xff) {
	case EINK_INIT_MODE:
		{
			offset = *((unsigned int *)(vaddr + C_INIT_MODE_ADDR_OFFSET));
			p_wf_file = (u8 *)vaddr + offset;
			break;
		}

	case EINK_DU_MODE:
		{
			offset = *((unsigned int *)(vaddr + C_DU_MODE_ADDR_OFFSET));
			p_wf_file = (u8 *)vaddr + offset;
			break;
		}

	case EINK_GC16_MODE:
		{
			offset = *((unsigned int *)(vaddr + C_GC16_MODE_ADDR_OFFSET));
			p_wf_file = (u8 *)vaddr + offset;
			break;
		}

	case EINK_GC4_MODE:
		{
			offset = *((unsigned int *)(vaddr + C_GC4_MODE_ADDR_OFFSET));
			p_wf_file = (u8 *)vaddr + offset;
			break;
		}

	case EINK_A2_MODE:
		{
			offset = *((unsigned int *)(vaddr + C_A2_MODE_ADDR_OFFSET));
			p_wf_file = (u8 *)vaddr + offset;
			break;
		}


	case EINK_GU16_MODE:
		{
			offset = *((unsigned int *)(vaddr + C_GC16_LOCAL_MODE_ADDR_OFFSET));
			p_wf_file = (u8 *)vaddr + offset;
			break;
		}
/*
	case EINK_GL16_MODE:
		{
			offset = (u32)g_waveform_file.p_gl16_wf - g_waveform_file.p_wf_paddr;
			p_wf_file = (u8 *)g_waveform_file.p_wf_vaddr + offset;
			break;
		}

	case EINK_GLR16_MODE:
		{
			offset = (u32)g_waveform_file.p_glr16_wf - g_waveform_file.p_wf_paddr;
			pr_info("glr16 offset = 0x%x\n", offset);
			p_wf_file = (u8 *)g_waveform_file.p_wf_vaddr + offset;
			break;
		}

	case EINK_GLD16_MODE:
		{
			offset = (u32)g_waveform_file.p_gld16_wf - g_waveform_file.p_wf_paddr;
			p_wf_file = (u8 *)g_waveform_file.p_wf_vaddr + offset;
			break;
		}
*/
	default:
		{
			printf("unkown mode(0x%x)\n", mode);
			p_wf_file = NULL;
			break;
		}
	}

	return p_wf_file;
}

static int get_temp_range_index(int temperature)
{
	int index = -1;

	for (index = 0; index < C_TEMP_TBL_SIZE; index++) {
		if (((wav_file.temp_tbl[index] == 0)) && (index > 0)) {
			break;
		}

		if (temperature < wav_file.temp_tbl[index]) {
			if (index > 0) {
				index -= 1;
			}
			break;
		}
		if ((temperature == wav_file.temp_tbl[index]) && (temperature > 0)) {
			return index;
		}
	}

	return index;
}

int get_original_wavedata(enum upd_mode mode, unsigned int temp,
			unsigned int *total_frames, unsigned long *wf_vaddr, unsigned long *wf_remap_vaddr)
{
	u8 *p_mode_virt_addr = NULL;
	u8 *remap_virt_addr = NULL;
	u32 mode_temp_offset = 0;
	u8 *p_mode_temp_addr = NULL;
	u32 temp_range_id = 0;

	if ((total_frames == NULL) || (wf_vaddr == NULL)) {
		printf("input param is null\n");
		return -1;
	}

	p_mode_virt_addr = get_mode_virt_address(mode, wav_file.vaddr);
	if (p_mode_virt_addr == NULL) {
		printf("get mode virturl address fail, mode=0x%x\n", mode);
		return -1;
	}

	remap_virt_addr = get_mode_virt_address(mode, wav_file.remap_vaddr);
	if (remap_virt_addr == NULL) {
		printf("get mode virturl address fail, mode=0x%x\n", mode);
		return -1;
	}

	temp_range_id = get_temp_range_index(temp);
	if (temp_range_id < 0) {
		printf("get temp range index fail, temp=0x%x\n", temp);
		return -1;
	}

	mode_temp_offset =  *((u32 *)p_mode_virt_addr + temp_range_id);

	if (mode_temp_offset == 0) {
		*wf_vaddr = 0;
		*wf_remap_vaddr = 0;
		return 0;
	}

	p_mode_temp_addr = (u8 *)p_mode_virt_addr + mode_temp_offset;

	*total_frames = *((u16 *)p_mode_temp_addr);
	mode_temp_offset = mode_temp_offset + 4;		//skip total frame(2 Byte) and dividor(2 Byte)

#if 0
	printf("----------org vir=%p, org remap=%p, off =0x%x\n", wav_file.vaddr, wav_file.remap_vaddr,
			mode_temp_offset);
	printf("E4 0x%x, r 0x%x\n", *(wav_file.vaddr + 0xe4), *(wav_file.remap_vaddr + 0xe4));
#endif
	*wf_vaddr = (unsigned long)(p_mode_virt_addr + mode_temp_offset);
	*wf_remap_vaddr = (unsigned long)(remap_virt_addr + mode_temp_offset);

	return 0;
}

int remap_eink_wavefile(void)
{
	int mode = 0, index = 0, temp = 0;
	enum upd_mode upd_mode;
	u32 total_frames = 0, frame_size = 0, per_size = 0, i = 0;
	unsigned long org_vaddr = 0;
	unsigned long org_remap_vaddr = 0;

	char *remap_vaddr = NULL;

	if (wav_file.bit_num == 5)
		per_size = 1024;
	else
		per_size = 256;

	for (mode = 0; mode < MAX_MODE_CNT; mode++) {
		for (index = 0; index < C_TEMP_TBL_SIZE; index++) {
			temp = wav_file.temp_tbl[index];
			upd_mode = get_upd_mode_from_index(mode);

			get_original_wavedata(upd_mode, temp, &total_frames, &org_vaddr, &org_remap_vaddr);
#if 1
			printf("org vaddr=0x%lx, remap=0x%lx, total = %d, per=%d, remap_vaddr =%p\n",
					org_vaddr, org_remap_vaddr, total_frames, per_size, remap_vaddr);
#endif
			if (org_vaddr == 0)
				continue;
			remap_vaddr = (char *)org_remap_vaddr;

			frame_size = total_frames * per_size;
			memset(remap_vaddr, 0, frame_size);

			for (i = 0; i < (frame_size / 4); i++) {
				*remap_vaddr = (*((char *)org_vaddr) & 0x3) |
					((*((char *)org_vaddr + 1) & 0x3) << 2) |
					((*((char *)org_vaddr + 2) & 0x3) << 4) |
					((*((char *)org_vaddr + 3) & 0x3) << 6);
				remap_vaddr++;
				org_vaddr += 4;
			}


		}
	}
	return 0;
}


int main(int argc, char **argv)
{
	int ret = 0;
	u32 *mode_addr = NULL;
	struct stat statbuf;

	ret = parse_cmdline(argc, argv);
	if (ret < 0) {
		return -1;
	}

	sprintf(wav_file.filename, "%s", FILE_NAME);
	wav_file.fd = fopen(wav_file.filename, "rb+");
	if (wav_file.fd == NULL) {
		printf("open %s failed!\n", wav_file.filename);
		return -1;
	}
	stat(wav_file.filename, &statbuf);
	wav_file.file_len = statbuf.st_size;

	printf("input file size is %d\n", wav_file.file_len);

	wav_file.vaddr = (char *)malloc(wav_file.file_len);
	if (wav_file.vaddr == NULL) {
		printf("failed malloc wav addr\n");
		ret = -1;
		goto err_wav;
	}
	memset(wav_file.vaddr, 0, wav_file.file_len);

	wav_file.remap_vaddr = (char *)malloc(wav_file.file_len);
	if (wav_file.remap_vaddr == NULL) {
		printf("failed malloc remap addr\n");
		ret = -1;
		goto err_remap;
	}

	fread(wav_file.vaddr, wav_file.file_len, 1, wav_file.fd);

	memcpy(wav_file.remap_vaddr, wav_file.vaddr, wav_file.file_len);

	memset(wav_file.panelname, 0, sizeof(wav_file.panelname));
	memcpy(wav_file.panelname, (wav_file.vaddr + 1), (sizeof(wav_file.panelname) - 1));
	printf("wavform name is %s\n", wav_file.panelname);
	memcpy(wav_file.temp_tbl, (wav_file.vaddr + C_TEMP_TBL_OFFSET), C_TEMP_TBL_SIZE);

	remap_eink_wavefile();

	sprintf(wav_file.out_name, "%s", "remap.awf");
	wav_file.out_fd = fopen(wav_file.out_name, "wb+");
	if (wav_file.out_fd == NULL) {
		printf("failed to open file %s\n", wav_file.out_name);
		ret = -1;
		goto err_file;
	}

	ret = fwrite(wav_file.remap_vaddr, wav_file.file_len, 1, wav_file.out_fd);

	ret = 0;

	fclose(wav_file.out_fd);
err_file:
	free(wav_file.remap_vaddr);
err_remap:
	free(wav_file.vaddr);
err_wav:
	fclose(wav_file.fd);
	return ret;
}
