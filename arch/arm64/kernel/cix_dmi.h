/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef __CIX_DMI_H
#define __CIX_DMI_H
#include <linux/dmi.h>
#include <asm/dmi.h>

#ifdef CONFIG_ARCH_CIX
char *get_cpu_name(void);
#else
char *get_cpu_name(void) { return NULL; }
#endif

#endif /* __CIX_DMI_H */
