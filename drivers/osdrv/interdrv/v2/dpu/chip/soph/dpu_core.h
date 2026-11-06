#ifndef _DPU_CORE_H_
#define _DPU_CORE_H_
#include <linux/module.h>
#include "linux/kernel.h"
#include <linux/kthread.h>
#include <linux/miscdevice.h>
#include <linux/spinlock.h>
#include <linux/wait.h>
#include <linux/mutex.h>
#include <linux/types.h>
#include <linux/timex.h>

#include <linux/comm_dpu.h>
#include "dpu.h"

#define DPU_SGBM_LD1_BASE_ADDR                 (0x680ba000)
#define DPU_SGBM_LD2_BASE_ADDR                 (0x680ba100)
#define DPU_FGS_CHFH_LD_BASE_ADDR              (0x680ba200)
#define DPU_FGS_GX_LD_BASE_ADDR                (0x680ba300)
#define DPU_SGBM_BTCOST_ST_BASE_ADDR           (0x680bb000)
#define DPU_SGBM_MEDIAN_ST_BASE_ADDR           (0x680bb100)
#define DPU_FGS_CHFH_ST_BASE_ADDR              (0x680bb200)
#define DPU_FGS_UX_ST_BASE_ADDR                (0x680bb300)

#define DMA_REG_SIZE                           (0X1C)

#endif /* _DPU_CORE_H_*/
