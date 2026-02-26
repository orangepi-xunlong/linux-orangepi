/* SPDX-License-Identifier: GPL-2.0-or-later */


#ifndef _RT_COMPONENT_H_
#define _RT_COMPONENT_H_

#include "rt_common.h"

error_type comp_get_handle(PARAM_OUT comp_handle *pHandle,
								PARAM_IN  char *cComponentName,
								PARAM_IN  void *pAppData,
								PARAM_IN  const rt_media_config_s *pmedia_config,
								PARAM_IN  comp_callback_type * pCallBacks);

error_type comp_free_handle(PARAM_IN       comp_handle component);

#endif

