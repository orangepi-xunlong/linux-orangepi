#ifndef __DBG_INI_UTIL_H__
#define __DBG_INI_UTIL_H__

struct dbg_ini_cfg {
	int softap_channel;
	int debug_log_level;
};

enum SEC_TYPE {
	SEC_INVALID,
	SEC_SOFTAP,
	SEC_DEBUG,
	MAX_SEC_NUM,
};

int dbg_util_init(struct dbg_ini_cfg *cfg);
#endif
