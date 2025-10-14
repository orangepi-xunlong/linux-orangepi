#ifndef __LINUX_HW_BOOTTIME_H__
#define __LINUX_HW_BOOTTIME_H__

#include <linux/module.h>

#define MAX_BOOT_PHASE_NAME (16)

enum FW_BOOT_PHASE {
	BROM_PHASE = 0,
	SE_PHASE,
	PM_PHASE,
	PBL_PHASE,
	TFA_PHASE,
	TEE_PHASE,
	BLOADER_PHASE, // uefi or uboot
	GRUB_PHASE,
	FW_BOOT_PHASE_MAX,
};

struct fw_boot_phase_point {
	uint64_t start; // 1us unit
	uint64_t end; // 1us unit
	// "BROM", "SE", "PM", "PBL", "TFA", "TEE", "BLOADER", "GRUB"
	char fw_name[MAX_BOOT_PHASE_NAME];
};

#ifndef CONFIG_PLAT_BOOT_TIME
void boot_record(const char *str)
{
}
int __init_or_module do_boottime_initcall(initcall_t fn)
{
}
#else
void boot_record(const char *str);
int __init_or_module do_boottime_initcall(initcall_t fn);
#endif

#define BOOT_STR_SIZE 120

#endif
