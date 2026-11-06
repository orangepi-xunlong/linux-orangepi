#ifndef __RGN_DRV_H__
#define __RGN_DRV_H__

#ifdef __cplusplus
	extern "C" {
#endif

#include <linux/delay.h>
#include <linux/types.h>
#include <stdbool.h>

#include <rgn_common.h>

/****************************************************************************
 * Interfaces
 ****************************************************************************/
void rgn_set_base_addr(void *base);

//void rgn_ip_test_cases(struct rgn_ctx *ctx);

#ifdef __cplusplus
	}
#endif

#endif /* __RGN_DRV_H__ */
