/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2024 Spaciemit Ltd.
 */
#ifndef __ASM_RWONCE_H
#define __ASM_RWONCE_H

#if defined(CONFIG_KY_ERRATA_LOAD_ATOMIC) && !defined(__ASSEMBLY__)


#define __READ_ONCE(x)							\
({									\
	typeof(&(x)) __x = &(x);					\
	int atomic = 1;							\
	union { __unqual_scalar_typeof(*__x) __val; char __c[1]; } __u;	\
	switch (sizeof(x)) {						\
	case 4:								\
		asm volatile (						\
			"lr.w %0, 0(%1)"				\
			: "=r" (*(__u32 *)__u.__c)			\
			: "r" (__x)					\
			: "memory"					\
		);							\
		break;							\
	case 8:								\
		asm volatile (						\
			"lr.d %0, 0(%1)"				\
			: "=r" (*(__u64 *)__u.__c)			\
			: "r" (__x)					\
			: "memory"					\
		);							\
		break;							\
	default:							\
		asm volatile ("fence	rw, rw" ::: "memory");		\
		atomic = 0;						\
	}								\
	atomic ? (typeof(*__x))__u.__val : (*(volatile typeof(__x))__x);\
})

#endif	/* CONFIG_KY_ERRATA_LOAD_ATOMIC && !__ASSEMBLY__ */

#include <asm-generic/rwonce.h>

#endif	/* __ASM_RWONCE_H */
