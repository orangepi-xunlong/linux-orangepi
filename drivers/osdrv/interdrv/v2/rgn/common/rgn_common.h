#ifndef __RGN_COMMON_H__
#define __RGN_COMMON_H__

#ifdef __cplusplus
	extern "C" {
#endif

#include <linux/clk.h>
#include <linux/debugfs.h>
#include <linux/io.h>


#define MIN(a, b) (((a) < (b))?(a):(b))
#define MAX(a, b) (((a) > (b))?(a):(b))
#define RGN_64_ALIGN(x) (((x) + 0x3F) & ~0x3F)   // for 64byte alignment
#define RGN_256_ALIGN(x) (((x) + 0xFF) & ~0xFF)   // for 256byte alignment
#define RGN_ALIGN(x) (((x) + 0xF) & ~0xF)   // for 16byte alignment
#define RGN_256_ALIGN(x) (((x) + 0xFF) & ~0xFF)   // for 256byte alignment
#define ISP_ALIGN(x, y) (((x) + (y - 1)) & ~(y - 1))   // for any bytes alignment
#define UPPER(x, y) (((x) + ((1 << (y)) - 1)) >> (y))   // for alignment
#define CEIL(x, y) (((x) + ((1 << (y)))) >> (y))   // for alignment


extern unsigned int rgn_log_lv;

#define RGN_ERR        1   /* error conditions                     */
#define RGN_WARN       2   /* warning conditions                   */
#define RGN_NOTICE     3   /* normal but significant condition     */
#define RGN_INFO       4   /* informational                        */
#define RGN_DEBUG      5   /* debug-level messages                 */

#define CVI_TRACE_RGN(level, fmt, ...) \
	do { \
		if (level <= rgn_log_lv) { \
			if (level == RGN_ERR) \
				pr_err("%s:(%d): " fmt, __func__, __LINE__, ##__VA_ARGS__); \
			else if (level == RGN_WARN) \
				pr_warn("%s:(%d): " fmt, __func__, __LINE__, ##__VA_ARGS__); \
			else if (level == RGN_NOTICE) \
				pr_notice("%s:(%d): " fmt, __func__, __LINE__, ##__VA_ARGS__); \
			else if (level == RGN_INFO) \
				pr_info("%s:(%d): " fmt, __func__, __LINE__, ##__VA_ARGS__); \
			else if (level == RGN_DEBUG) \
				pr_debug("%s:(%d): " fmt, __func__, __LINE__, ##__VA_ARGS__); \
		} \
	} while (0)

#define RGB888_2_ARGB1555(rgb888) \
	(BIT(15) | ((rgb888 & 0x00f80000) >> 9) | ((rgb888 & 0x0000f800) >> 6) | ((rgb888 & 0x000000f8) >> 3))

#define RGN_COLOR_DARK		0x8000
#define RGN_COLOR_BRIGHT	0xffff

enum rgn_op {
	RGN_OP_UPDATE = 0,
	RGN_OP_INSERT,
	RGN_OP_REMOVE,
};

/********************************************************************
 *   APIs to replace bmtest's standard APIs
 ********************************************************************/
extern bool __clk_is_enabled(struct clk *clk);

#ifdef __cplusplus
}
#endif

#endif /* __RGN_COMMON_H__ */
