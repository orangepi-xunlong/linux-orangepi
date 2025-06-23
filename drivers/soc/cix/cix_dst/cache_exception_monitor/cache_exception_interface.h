// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright 2024 Cix Technology Group Co., Ltd.
 */
#ifndef __CACHE_EXCEP_INF__
#define __CACHE_EXCEP_INF__

enum cache_excep_type {
	CACHE_EXCEP_DSU = 0,
	CACHE_EXCEP_CORE = 1,
	CACHE_EXCEP_L1 = CACHE_EXCEP_CORE,
   	CACHE_EXCEP_L2,
};

enum core_type {
	CORE_HAYES = 0,
	CORE_HUNTER,
	CORE_DSU,
	CORE_MAX,
};

struct cache_excep_drvdata {
	int cpu;
	int state;
	int complex_errirq;
	int complex_faultirq;
	int errirq;
	int faultirq;
	int core_type;
};

void cache_excep_init(void *info);
void cache_excep_uninit(void *info);
void cache_excep_record(enum cache_excep_type type);

#endif