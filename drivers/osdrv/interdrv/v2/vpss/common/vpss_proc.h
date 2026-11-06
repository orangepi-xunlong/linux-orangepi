#ifndef _VIP_VPSS_PROC_H_
#define _VIP_VPSS_PROC_H_

#include <linux/seq_file.h>
#include <generated/compile.h>

#include "vpss_ctx.h"
#include <base_ctx.h>

#include "vpss_core.h"

int vpss_proc_init(struct vpss_device *dev);
int vpss_proc_remove(struct vpss_device *dev);
int vpp_proc_init(struct vpss_device *dev);
int vpp_proc_remove(struct vpss_device *dev);

#endif // _VIP_VPSS_PROC_H_
