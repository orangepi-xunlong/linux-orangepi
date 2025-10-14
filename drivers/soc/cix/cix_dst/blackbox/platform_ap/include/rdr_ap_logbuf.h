/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef __RDR_AP_LOGBUF_H__
#define __RDR_AP_LOGBUF_H__

#include <linux/platform_device.h>
#include <linux/soc/cix/rdr_pub.h>

struct pstore_head {
	u8 flag;
	u8 type;
	u32 size;
} __aligned(16);

int pstore_dump_init(struct platform_device *pdev,
		     struct rdr_safemem_pool *pool);
int ap_pstore_cleartext(const char *dir_path, u64 log_addr, u32 log_len);
void pstore_dump_mount(void);

#endif
