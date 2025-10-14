// SPDX-License-Identifier: GPL-2.0
/* Copyright 2024 Cix Technology Group Co., Ltd.*/
/**
 * Cix Energy Model driver
 */

#ifndef __CIX_SCMI_EM_H
#define __CIX_SCMI_EM_H

#ifdef CONFIG_CIX_SCMI_ENERGY_MODEL
/**
 * cix_scmi_register_em() - Register an energy model using
 *              SCMI Message Protocol
 *
 * @dev: Device for which the EM is to register
 *
 * Return: 0 for success; else the error code is returned
 */
int cix_scmi_register_em(struct device *dev);

#else /* !CONFIG_CIX_SCMI_ENERGY_MODEL */

static inline int cix_scmi_register_em(struct device *dev)
{
	return -EINVAL;
}

#endif /* CONFIG_CIX_SCMI_ENERGY_MODEL */

#endif /* __CIX_SCMI_EM_H */