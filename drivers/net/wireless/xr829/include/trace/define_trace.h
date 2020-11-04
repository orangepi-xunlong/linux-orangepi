#include <generated/uapi/linux/version.h>

#if (KERNEL_VERSION(2, 6, 30) < LINUX_VERSION_CODE)
#include_next <trace/define_trace.h>
#endif /* (LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 30)) */
