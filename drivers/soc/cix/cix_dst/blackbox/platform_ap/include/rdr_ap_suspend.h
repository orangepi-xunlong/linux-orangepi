/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef __RDR_AP_SUSPEND_H__
#define __RDR_AP_SUSPEND_H__

#include <linux/platform_device.h>
#include <linux/soc/cix/rdr_pub.h>

int suspend_dump_init(struct platform_device *pdev, struct rdr_safemem_pool *pool);
void ap_suspend_dump(u32 modid, u32 etype);
int ap_suspend_cleartext(const char *dir_path, u64 log_addr, u32 log_len);

#endif
