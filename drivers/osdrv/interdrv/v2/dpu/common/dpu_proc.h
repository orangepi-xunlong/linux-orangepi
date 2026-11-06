#ifndef _DPU_PROC_H_
#define _DPU_PROC_H_

#include <linux/seq_file.h>
#include <generated/compile.h>
#include <base_ctx.h>

#include "dpu_core.h"


int dpu_proc_init(struct dpu_dev_s *dev);
int dpu_proc_remove(struct dpu_dev_s *dev);

#endif // _VIP_DPU_PROC_H_
