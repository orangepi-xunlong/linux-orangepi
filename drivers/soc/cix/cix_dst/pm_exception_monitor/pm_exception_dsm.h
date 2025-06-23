/* SPDX-License-Identifier: GPL-2.0 */

#ifndef __DSM_PM_H__
#define __DSM_PM_H__

#include <linux/soc/cix/dsm_pub.h>

#define DSM_PMIC_BUF_SIZE   1024 // byte
#define DSM_PMIC_NAME       "dsm_pmic_info"

enum pm_device_type {
	PM_PMIC = 0,
	PM_DEVICE_MAX
};

int pm_calc_error_id(uint8_t src, uint8_t type, uint16_t id);

#ifdef CONFIG_PM_EXCP_DSM_DRIVER
int pm_dsm_report_num(enum pm_device_type device_type, int error_no,
		unsigned int mesg_no);
int pm_dsm_report_info(enum pm_device_type device_type, int error_no,
		char *fmt, ...);
#else
static inline int pm_dsm_report_num(enum pm_device_type device_type,
		int error_no, unsigned int mesg_no)
{
	return 0;
}

static inline int pm_dsm_report_info(enum pm_device_type device_type,
		int error_no, char *fmt, ...)
{
	return 0;
}
#endif // !CONFIG_PM_EXCP_DSM_DRIVER

#endif // __DSM_PM_H__
