/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef __RDR_AP_MEMID_H__
#define __RDR_AP_MEMID_H__

#include <linux/soc/cix/rdr_platform_ap_hook.h>

#define HOOK_MAX_NUMBERS 12

enum ap_memid {
	MEMID_REGDUMP = 1,
	MEMID_MODULE_DUMP = 100,
	MEMID_PSTORE = 200,
	MEMID_STACK = 210,
	MEMID_SUSPEND_INFO = 220,
	MEMID_HOOK = 1000,
	MEMID_RES = 2000,
};

#endif
