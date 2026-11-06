#ifndef _STITCH_PROC_H_
#define _STITCH_PROC_H_

#include <linux/seq_file.h>
#include <generated/compile.h>

#include "stitch_ctx.h"
#include <base_ctx.h>

#include "stitch_core.h"

void stitch_proc_set_timer(bool set);
int stitch_proc_init(struct stitch_dev *dev);
int stitch_proc_remove(struct stitch_dev *dev);

#endif // _STITCH_PROC_H_
