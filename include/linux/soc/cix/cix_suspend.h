#ifndef __CIX_SUSPEND_H
#define __CIX_SUSPEND_H

#define SUSPEND_MAX_MODULE_NAME_LEN                 16

typedef enum {
    SUSPEND_WARN_TYPE_NONE = 0,
    SUSPEND_WARN_TYPE_TIMEOUT,
    SUSPEND_WARN_TYPE_FAILED,
    SUSPEND_WARN_TYPE_ABORTED,
    SUSPEND_WARN_TYPE_MAX
} suspend_warn_type_t;

struct suspend_info {
    char *name;
    suspend_warn_type_t type;
    bool is_set;
};

extern void suspend_warning_set(const char *name, suspend_warn_type_t type);
extern void suspend_warning_clear(void);
extern bool suspend_warning_check(void);

#endif
