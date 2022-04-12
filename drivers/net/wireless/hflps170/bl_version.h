#ifndef _BL_VERSION_H_
#define _BL_VERSION_H_

#include "bl_version_gen.h"

static inline void bl_print_version(void)
{
    printk(BL_VERS_BANNER"\n");
}

#endif /* _BL_VERSION_H_ */
