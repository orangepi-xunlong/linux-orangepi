#ifndef __RGN_DEFINES_H__
#define __RGN_DEFINES_H__

#ifdef __cplusplus
	extern "C" {
#endif

#include <linux/miscdevice.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

#include <linux/defines.h>
#include <vip/rgn_drv.h>

enum sclr_gop {
	SCL_GOP_SCD,
	SCL_GOP_SCV1,
	SCL_GOP_SCV2,
	SCL_GOP_SCV3,
	SCL_GOP_DISP,
	SCL_GOP_MAX,
};


/**
 * struct rgn - RGN IP abstraction
 */
struct rgn_dev {
	struct miscdevice	miscdev;

	struct device		*dev;
	spinlock_t		lock;
	spinlock_t		rdy_lock;
	struct mutex		mutex;
	bool			bind_fb;
};

#ifdef __cplusplus
}
#endif

#endif /* __RGN_DEFINES_H__ */
