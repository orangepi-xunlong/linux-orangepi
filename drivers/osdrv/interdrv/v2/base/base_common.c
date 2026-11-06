#include <linux/types.h>
#include <linux/module.h>
#include <linux/time.h>
#include "base_common.h"
#include "base_debug.h"

#define GENERATE_STRING(STRING) (#STRING),
static const char *const MOD_STRING[] = FOREACH_MOD(GENERATE_STRING);

u32 pm_buf_mode = 1; //0: clean buf  1: reserve

module_param(pm_buf_mode, int, 0644);

const uint8_t *sys_get_modname(mod_id_e id)
{
	return (id < ID_BUTT) ? MOD_STRING[id] : "UNDEF";
}
EXPORT_SYMBOL_GPL(sys_get_modname);

u32 get_diff_in_us(struct timespec64 t1, struct timespec64 t2)
{
	struct timespec64 ts_delta = timespec64_sub(t2, t1);
	u64 ts_ns;

	ts_ns = timespec64_to_ns(&ts_delta);
	do_div(ts_ns, 1000);
	return ts_ns;
}
EXPORT_SYMBOL_GPL(get_diff_in_us);

u32 get_pm_buf_mode(void)
{
	return pm_buf_mode;
}
EXPORT_SYMBOL_GPL(get_pm_buf_mode);

