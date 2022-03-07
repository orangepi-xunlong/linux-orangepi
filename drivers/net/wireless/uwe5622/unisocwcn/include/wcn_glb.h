#ifndef __WCN_GLB_H__
#define __WCN_GLB_H__

#include <marlin_platform.h>
#include "wcn_txrx.h"

#ifndef CONFIG_CHECK_DRIVER_BY_CHIPID
#ifdef CONFIG_UWE5621
#include "uwe5621_glb.h"
#endif

#ifdef CONFIG_UWE5622
#include "uwe5622_glb.h"
#endif

#ifdef CONFIG_UWE5623
#include "uwe5623_glb.h"
#endif

#else
#include "uwe562x_glb.h"
#endif

#include "loopcheck.h"

#endif
