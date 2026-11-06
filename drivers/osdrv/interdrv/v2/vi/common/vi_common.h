#ifndef __VI_COMMON_H__
#define __VI_COMMON_H__

#ifdef __cplusplus
	extern "C" {
#endif

#include <linux/clk.h>
#include <linux/debugfs.h>
#include <linux/io.h>


#ifdef PORTING_TEST
extern int vi_dump_reg;
#endif

#define _reg_read(addr) readl((void __iomem *)addr)
//#define _reg_write(addr, data) writel(data, (void __iomem *)addr)
#ifdef PORTING_TEST
#define _reg_write(addr, data) \
	{ \
		writel(data, (void __iomem *)addr); \
		if (vi_dump_reg) \
			pr_info("MWriteS32 %#x %#x\n", (u32)(addr), (u32)(data)); \
	}
#else
#define _reg_write(addr, data) \
	{ \
		writel(data, (void __iomem *)addr); \
	}
#endif

#define MIN(a, b) (((a) < (b))?(a):(b))
#define MAX(a, b) (((a) > (b))?(a):(b))
#define VI_ALIGN(x) (((x) + 0xF) & ~0xF)   // for 16byte alignment
#define VI_64_ALIGN(x) (((x) + 0x3F) & ~0x3F)   // for 64byte alignment
#define VI_256_ALIGN(x) (((x) + 0xFF) & ~0xFF)   // for 256byte alignment
#define VI_4K_ALIGN(x) (((x) + 0xFFF) & ~0xFFF)   // for 4kbyte alignment
#define ISP_ALIGN(x, y) (((x) + (y - 1)) & ~(y - 1))   // for any bytes alignment
#define UPPER(x, y) (((x) + ((1 << (y)) - 1)) >> (y))   // for alignment
#define CEIL(x, y) (((x) + ((1 << (y)))) >> (y))   // for alignment


extern u32 vi_log_lv;

#define vi_pr(level, fmt, arg...) \
	do { \
		if (vi_log_lv & level) { \
			if (level == VI_ERR) \
				pr_err("%s:%d(): " fmt, __func__, __LINE__, ## arg); \
			else if (level == VI_WARN) \
				pr_warn("%s:%d(): " fmt, __func__, __LINE__, ## arg); \
			else if (level == VI_NOTICE) \
				pr_notice("%s:%d(): " fmt, __func__, __LINE__, ## arg); \
			else if (level == VI_INFO) \
				pr_info("%s:%d(): " fmt, __func__, __LINE__, ## arg); \
			else if (level == VI_DBG) \
				pr_debug("%s:%d(): " fmt, __func__, __LINE__, ## arg); \
		} \
	} while (0)

enum vi_msg_pri {
	VI_ERR		= 0x1,
	VI_WARN		= 0x2,
	VI_NOTICE	= 0x4,
	VI_INFO		= 0x8,
	VI_DBG		= 0x10,
};

struct vi_rect {
	u16 x;
	u16 y;
	u16 w;
	u16 h;
};

extern bool __clk_is_enabled(struct clk *clk);

#ifdef __cplusplus
}
#endif

#endif /* __VI_COMMON_H__ */
