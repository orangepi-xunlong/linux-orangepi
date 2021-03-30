
/*
 ******************************************************************************
 *
 * vin_3a.h
 *
 * Hawkview ISP - vin_3a.h module
 *
 * Copyright (c) 2016 by Allwinnertech Co., Ltd.  http://www.allwinnertech.com
 *
 * Version		  Author         Date		    Description
 *
 *   3.0		  Yang Feng   	2016/03/11	VIDEO INPUT
 *
 *****************************************************************************
 */

#ifndef _VIN_3A_H_
#define _VIN_3A_H_

#include "../vin-isp/sunxi_isp.h"

/* Init and cleanup functions */
int vin_isp_h3a_init(struct isp_dev *isp);
void vin_isp_h3a_cleanup(struct isp_dev *isp);

#endif /*_VIN_3A_H_*/

