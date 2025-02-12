#if !defined(__KY_INIT_H__)
#define __KY_INIT_H__

#include "img_types.h"
#include "pvrsrv_error.h"
#include "pvrsrv_device.h"
#include "servicesext.h"
#include <linux/version.h>

struct st_context {
	PVRSRV_DEVICE_CONFIG    *dev_config;
	struct clk          *gpu_clk;
	struct reset_control *gpu_reset;
	/* mutex protect for set power state */
	struct mutex set_power_state;
	IMG_BOOL            gpu_active;
	IMG_BOOL            bEnablePd;
};

struct st_context * RgxStInit(PVRSRV_DEVICE_CONFIG* psDevConfig);
void RgxStUnInit(struct st_context *platform);
void RgxResume(struct st_context *platform);
void RgxSuspend(struct st_context *platform);
PVRSRV_ERROR STPrePowerState(IMG_HANDLE hSysData,
							 PVRSRV_SYS_POWER_STATE eNewPowerState,
							 PVRSRV_SYS_POWER_STATE eCurrentPowerState,
							 PVRSRV_POWER_FLAGS ePwrFlags);
PVRSRV_ERROR STPostPowerState(IMG_HANDLE hSysData,
							  PVRSRV_SYS_POWER_STATE eNewPowerState,
							  PVRSRV_SYS_POWER_STATE eCurrentPowerState,
							  PVRSRV_POWER_FLAGS ePwrFlags);
void stSetFrequency(IMG_HANDLE hSysData, IMG_UINT32 ui32Frequency);
void stSetVoltage(IMG_HANDLE hSysData, IMG_UINT32 ui32Voltage);
#endif
