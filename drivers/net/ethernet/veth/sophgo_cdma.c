#include <linux/types.h>
#include <linux/spinlock.h>
#include <linux/errno.h>

#include "sophgo_common.h"
#include "sophgo_cdma.h"

void sg_construct_cdma_arg(struct sg_cdma_arg *p_arg, u64 src, u64 dst, u64 size, enum sg_memcpy_dir dir)
{
	p_arg->src = src;
	p_arg->dst = dst;
	p_arg->size = size;
	p_arg->dir = dir;
}
