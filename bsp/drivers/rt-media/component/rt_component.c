/* SPDX-License-Identifier: GPL-2.0-or-later */

#define LOG_TAG "rt_component"

#include "rt_common.h"
#include "rt_venc_component.h"
#include "rt_vi_component.h"
#include "rt_component.h"

typedef struct rt_comp_registry {
	char *name;
	component_init   comp_init_fn;
} rt_comp_registry;

rt_comp_registry rt_comp_table[] = {
	{
		"video.encoder",
		venc_comp_component_init
	},

	{
		"video.vipp",
		vi_comp_component_init
	},

};

static int get_cmp_index(char *cmp_name)
{
	int rc = -1;
	unsigned int i;
	unsigned int size_of_comp = sizeof(rt_comp_table) / sizeof(rt_comp_registry);
	for (i = 0; i < size_of_comp; i++) {
		RT_LOGI("get_cmp_index: cmp_name = %s , cdx_comp_table[%d].name = %s", cmp_name, i, rt_comp_table[i].name);
		if (!strcmp(cmp_name, rt_comp_table[i].name)) {
			rc = i;
			break;
		}
	}
	return rc;
}


error_type comp_get_handle(PARAM_OUT comp_handle *pHandle,
								PARAM_IN  char *cComponentName,
								PARAM_IN  void *pAppData,
								PARAM_IN  const rt_media_config_s *pmedia_config,
								PARAM_IN  comp_callback_type * pCallBacks)
{
	error_type eRet = ERROR_TYPE_OK;
	int comp_index = get_cmp_index(cComponentName);
	RT_LOGD("component name[%s]", cComponentName);
	RT_LOGI("COMP CORE API - COMP_GetHandle");

	if (comp_index >= 0) {
		comp_handle compHandle;
		compHandle = kmalloc(sizeof(rt_component_type), GFP_KERNEL);
		if (!compHandle) {
			RT_LOGE("Component Creation failed");
			return ERROR_TYPE_NOMEM;
		}
		memset(compHandle, 0, sizeof(rt_component_type));
		rt_comp_table[comp_index].comp_init_fn(compHandle, pmedia_config);
		comp_set_callbacks(compHandle, pCallBacks, pAppData);
		*pHandle = compHandle;
	} else {
		RT_LOGE("component name[%s] is not found", cComponentName);
		eRet = ERROR_TYPE_UNEXIST;
	}
	return eRet;
}

error_type comp_free_handle(
	PARAM_IN  comp_handle component)
{
	error_type eRet = ERROR_TYPE_OK;
	rt_component_type *pComp = (rt_component_type *)component;
	comp_state_type state;

	RT_LOGI("COMP CORE API - Free Handle %px", component);
	if (NULL == component) {
		return ERROR_TYPE_OK;
	}
	pComp->get_state(component, &state);
	if (state != COMP_STATE_IDLE) {
		RT_LOGE("fatal error! Calling FreeHandle in state %d", state);
	}
	pComp->destroy(component);
	kfree(component);
	return eRet;
}


