#ifndef _RWNX_VERSION_H_
#define _RWNX_VERSION_H_

#include "rwnx_version_gen.h"

static inline void rwnx_print_version(void)
{
	printk(RWNX_VERS_BANNER"\n");
	printk("Driver Release Tag: %s\n", DRV_RELEASE_TAG);
}

#endif /* _RWNX_VERSION_H_ */
