#ifndef __RDR_HISI_PLATFORM_H__
#define __RDR_HISI_PLATFORM_H__

#include <linux/pstore.h>
#include <linux/soc/cix/rdr_types.h>
#include <linux/soc/cix/rdr_pub.h>
#include <linux/soc/cix/rdr_platform_ap_hook.h>
#include <mntn_public_interface.h>
#include <mntn_subtype_exception.h>

/*record log length*/
#define LOG_PATH_LEN 96
#define DEST_LOG_PATH_LEN (LOG_PATH_LEN + 10)
#define NEXT_LOG_PATH_LEN (LOG_PATH_LEN + 30)

#define MODID_START_DEF(name, start)                  \
	MNTN_DEF_ARGS(MODID_##name##_START = (start), \
		      MODID_##name##_END = (start) + 0xff)

#undef SUBTYPE_DEF
#define SUBTYPE_DEF(typename, subname, value)                           \
	MNTN_DEF_ARGS(MODID_##typename##_##subname = (value) + MODID_## \
						     typename##_START)

#define MODID_DEF(name, start) MODID_START_DEF(name, (start)), name##_SUB_LIST

typedef enum {
	MODID_AP_START = PLAT_BB_MOD_AP_START,

	MODID_DEF(AP_PANIC, MODID_AP_START),
	MODID_DEF(AP_MAILBOX, MODID_AP_PANIC_END + 1),
	MODID_DEF(AP_RESUME, MODID_AP_MAILBOX_END + 1),
	MODID_DEF(AP_AWDT, MODID_AP_RESUME_END + 1),
	MODID_DEF(BL31_PANIC, MODID_AP_AWDT_END + 1),

	/* IDM subtype reason */
	MODID_DEF(NI700_EXCEPTION, MODID_BL31_PANIC_END + 1),

	/* TZC400 subtype reason */
	MODID_DEF(TZC400_EXCEPTION, MODID_NI700_EXCEPTION_END + 1),

	/* TEE subtype reason */
	MODID_DEF(TEE_EXCEPTION, MODID_TZC400_EXCEPTION_END + 1),

	/* DDRC Mod id */
	MODID_DEF(DDR_EXCEPTION, MODID_TEE_EXCEPTION_END + 1),

	MODID_DEF(RCSU_EXCEPTION, MODID_DDR_EXCEPTION_END + 1),

	MODID_DEF(AP_SUSPEND, MODID_RCSU_EXCEPTION_END + 1),

	MODID_AP_END = PLAT_BB_MOD_AP_END
} modid_ap;

typedef enum {
#ifdef CONFIG_PLAT_BBOX_TEST
	MODU_TEST,
#endif
	MODU_NOC,
	MODU_DDR,
	MODU_TZC400,
	MODU_IDM,
	MODU_SMMU,
	MODU_TFA,
	MODU_GAP, /*256 byte space as the gap, adding modules need before this */
	MODU_MAX
} dump_mem_module;

typedef int (*ap_dump_func)(void *dump_addr, unsigned int size);

#ifdef CONFIG_PLAT_BBOX
int register_module_dump_mem_func(ap_dump_func func, const char *module_name,
				  dump_mem_module modu);
int get_module_dump_mem_addr(dump_mem_module modu, unsigned char **dump_addr,
			     u32 *size);
bool rdr_get_ap_init_done(void);
void logmem_add(enum pstore_type_id id, void *buf, u32 size);

#else
static inline void save_module_dump_mem(void)
{
}
static inline void regs_dump(void)
{
}
static inline void hisiap_nmi_notify_lpm3(void)
{
}
static inline int register_module_dump_mem_func(rdr_hisiap_dump_func_ptr func,
						const char *module_name,
						dump_mem_module modu)
{
	return -1;
}
static inline int get_module_dump_mem_addr(dump_mem_module modu,
					   unsigned char **dump_addr, u32 *size)
{
	return -1;
}
static inline bool rdr_get_ap_init_done(void)
{
	return 0;
}
#endif

#endif
