// SPDX-License-Identifier: GPL-2.0
/*
 * System Control and Power Interface (SCMI) based PM exception debug driver
 *
 */

#ifndef __PM_EXCEPTION_DRV_H__
#define __PM_EXCEPTION_DRV_H__
#include <linux/types.h>

#define MAX_PMIC_NUM (4)

enum pm_error_type {
	PM_PMIC_ERROR = 1,
    PM_CHARGER_ERROR,
    PM_EC_ERROR,
	PM_ERROR_MAX
};

#pragma pack(push, 1)
union pmic_status {
	uint8_t pmic_status8;
	struct {
		uint8_t vldo_fault_0 : 1;
		uint8_t vr_fault_0 : 1;
		uint8_t uv_0 : 1;
		uint8_t oc_0 : 1;
		uint8_t ov_0 : 1;
		uint8_t ot_0 : 1;
		uint8_t rsvd_0 : 2;
	} pmic_status_bits;
};
#pragma pack(pop)

#pragma pack(push, 4)
typedef struct {
	union {
		uint32_t pmic_events_flag;
		struct {
			/* pmic flag */
			union pmic_status pmic_flag[MAX_PMIC_NUM];
		} events_flag;
	};
	union {
		uint32_t pmic_support_mask;
		struct {
			/* pmic mask */
			union pmic_status pmic_mask[MAX_PMIC_NUM];
		} support_mask;
	};
	uint32_t pmic_status_word[3];
} csu_pm_state_report_t;
#pragma pack(pop)

struct pm_exception_info {
	uint8_t excp_src;		// 0: no error; 1: pmic; 2: charger; 3: EC
	uint8_t excp_type;		// excp type
	uint32_t excp_id;		// excp id in the spcified type
	char excp_desc[20];		// excp description(optional)
};

void get_pm_exception_data(void);
int pm_pmic_report_exception(csu_pm_state_report_t *info);
void print_pm_exception_data(struct pm_exception_info *info);
#endif