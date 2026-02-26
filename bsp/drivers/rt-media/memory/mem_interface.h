/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * =====================================================================================
 *
 * Copyright (c) 2008-2020 Allwinner Technology Co. Ltd.
 * All rights reserved.
 *
 *       Filename:  mem_interface.h
 *
 *    Description:
 *
 *        Version:  1.0
 *        Created:  2020-04-14
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  jilinglin,<jilinglin@allwinnertech.com>
 *        Company:  Allwinner Technology
 *
 * =====================================================================================
 */
#ifndef MEM_INTERFACE_H
#define MEM_INTERFACE_H

#include "cdc_log.h"

enum mem_type {
	MEM_TYPE_ION = 0,
};

struct mem_param {
	void *need; //we won't use strcut device
};

enum mem_flag {
	MEM_IOMMU_FLAG = 0,
	MEM_NORMAL_FLAG,
};

struct mem_dma_buf {
	int dma_fd;
	int buf_size;
	unsigned int phyaddr;
	unsigned char *viraddr;
};

struct mem_interface {
	void *(*palloc_no_cache)(struct mem_interface * /*p*/, int /*size*/);

	void *(*palloc)(struct mem_interface * /*p*/, int /*size*/);

	void (*pfree)(struct mem_interface * /*p*/, void * /*mem*/);

	void (*flush_cache)(struct mem_interface * /*p*/, void * /*start*/, int /*size*/);

	unsigned long (*get_phyaddr)(struct mem_interface * /*p*/, void * /*viraddr*/);

	void *(*get_viraddr)(struct mem_interface * /*p*/, unsigned long /*phyaddr*/);

	int (*get_mem_flag)(struct mem_interface * /*p*/);

	int (*share_fd)(struct mem_interface * /*p*/, void * /*viraddr*/);
};

//*show caller detail info
#if 1
static inline void *cdc_mem_palloc_no_cache(struct mem_interface *p, int size)
{
	return p->palloc_no_cache(p, size);
}

static inline void *cdc_mem_palloc(struct mem_interface *p, int size)
{
	return p->palloc(p, size);
}
#else
#define cdc_mem_palloc(p, size) cdc_mem_palloc_new(p, size, __FUNCTION__, __LINE__)
#define cdc_mem_palloc_no_cache(p, size) cdc_mem_palloc_no_cache_new(p, size, __FUNCTION__, __LINE__)

static inline void *cdc_mem_palloc_no_cache_new(struct mem_interface *p, int size, const char *file, unsigned int line)
{
	logd("no cache palloc: size = %d, func = %s, line = %d",
		size, file, line);
	    return p->palloc_no_cache(p, size);
}

static inline void *cdc_mem_palloc_new(struct mem_interface *p, int size, const char *file, unsigned int line)
{
		logd("palloc: size = %d, func = %s, line = %d",
			size, file, line);
	    return p->palloc(p, size);
}
#endif

static inline void cdc_mem_pfree(struct mem_interface *p, void *addr)
{
	p->pfree(p, addr);
}

static inline void cdc_mem_flush_cache(struct mem_interface *p, void *start, int size)
{
	p->flush_cache(p, start, size);
}

static inline unsigned long cdc_mem_get_phy(struct mem_interface *p, void *vir)
{
	return p->get_phyaddr(p, vir);
}

static inline void *cdc_mem_get_vir(struct mem_interface *p, unsigned long phy)
{
	return p->get_viraddr(p, phy);
}

static inline int cdc_mem_get_flag(struct mem_interface *p)
{
	return p->get_mem_flag(p);
}

static inline int cdc_mem_share_fd(struct mem_interface *p, void *vir)
{
	return p->share_fd(p, vir);
}

struct mem_interface *mem_create(int type, struct mem_param param);

void mem_destory(int type, struct mem_interface *p);

#endif
