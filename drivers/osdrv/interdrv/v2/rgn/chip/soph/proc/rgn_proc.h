#ifndef _RGN_PROC_H_
#define _RGN_PROC_H_

#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/version.h>
#include <generated/compile.h>

#include <linux/comm_video.h>
#include <linux/comm_region.h>
#include <linux/comm_vo.h>
#include "rgn_ctx.h"

#define GDC_PROC_JOB_INFO_NUM      (500)
#define RGN_PROC_INFO_OFFSET       (sizeof(struct vpss_proc_ctx) * VPSS_MAX_GRP_NUM)

int rgn_proc_init(void);
int rgn_proc_remove(void);

#endif // _RGN_PROC_H_
