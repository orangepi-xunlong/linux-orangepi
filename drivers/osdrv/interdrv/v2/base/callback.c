#include <linux/types.h>
#include <linux/module.h>
#include <base_cb.h>
#include "base_debug.h"


static struct base_m_cb_info base_m_cb[E_MODULE_BUTT];

const char * const CB_MOD_STR[] = CB_FOREACH_MOD(CB_GENERATE_STRING);

#define IDTOSTR(X) (X < E_MODULE_BUTT) ? CB_MOD_STR[X] : "UNDEF"



int base_rm_module_cb(enum enum_modules_id module_id)
{
	if (module_id < 0 || module_id >= E_MODULE_BUTT) {
		TRACE_BASE(DBG_ERR, "base rm cb error: wrong module_id\n");
		return -1;
	}

	base_m_cb[module_id].dev = NULL;
	base_m_cb[module_id].cb  = NULL;

	return 0;
}
EXPORT_SYMBOL_GPL(base_rm_module_cb);

int base_reg_module_cb(struct base_m_cb_info *cb_info)
{
	if (!cb_info || !cb_info->dev || !cb_info->cb) {
		TRACE_BASE(DBG_ERR, "base reg cb error: no data\n");
		return -1;
	}

	if (cb_info->module_id < 0 || cb_info->module_id >= E_MODULE_BUTT) {
		TRACE_BASE(DBG_ERR, "base reg cb error: wrong module_id\n");
		return -1;
	}

	base_m_cb[cb_info->module_id] = *cb_info;

	return 0;
}
EXPORT_SYMBOL_GPL(base_reg_module_cb);

int base_exe_module_cb(struct base_exe_m_cb *exe_cb)
{
	struct base_m_cb_info *cb_info;

	if (exe_cb->caller < 0 || exe_cb->caller >= E_MODULE_BUTT) {
		TRACE_BASE(DBG_ERR, "base exe cb error: wrong caller\n");
		return -1;
	}

	if (exe_cb->callee < 0 || exe_cb->callee >= E_MODULE_BUTT) {
		TRACE_BASE(DBG_ERR, "base exe cb error: wrong callee\n");
		return -1;
	}

	cb_info = &base_m_cb[exe_cb->callee];

	if (!cb_info->cb) {
		TRACE_BASE(DBG_ERR, "base exe cb error: cb of callee(%s) is null, caller(%s)\n",
			IDTOSTR(exe_cb->callee), IDTOSTR(exe_cb->caller));
		return -1;
	}

	return cb_info->cb(cb_info->dev, exe_cb->caller, exe_cb->cmd_id, exe_cb->data);
}
EXPORT_SYMBOL_GPL(base_exe_module_cb);

