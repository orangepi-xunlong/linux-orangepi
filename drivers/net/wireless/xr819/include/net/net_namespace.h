#ifndef _COMPAT_NET_NET_NAMESPACE_H
#define _COMPAT_NET_NET_NAMESPACE_H 1

#include <generated/uapi/linux/version.h>

#if (LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 23))
#include_next <net/net_namespace.h>
#endif /* (LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 23)) */

#endif	/* _COMPAT_NET_NET_NAMESPACE_H */
