#ifndef AXP_CORE_H_
#define AXP_CORE_H_
#include <linux/interrupt.h>
#include <linux/pm_wakeup.h>
#include <linux/power/aw_pm.h>
#include "axp-charger.h"

#define AXP_REG_WIDTH     (8)
#define AXP_ADD_WIDTH     (8)
#define ABS(x)		((x) > 0 ? (x) : -(x))
#ifdef CONFIG_DUAL_AXP_USED
#define AXP_ONLINE_SUM    (2)
#else
#define AXP_ONLINE_SUM    (1)
#endif

enum AXP_REGMAP_TYPE {
	AXP_REGMAP_I2C,
	AXP_REGMAP_ARISC_RSB,
	AXP_REGMAP_ARISC_TWI,
};

struct axp_regmap {
	enum AXP_REGMAP_TYPE type;
	struct i2c_client    *client;
	struct mutex         lock;
#ifndef CONFIG_AXP_TWI_USED
	spinlock_t           spinlock;
#endif
	u8                   rsbaddr;
};

struct axp_regmap_irq {
	irq_handler_t handler;
	void *data;
};

struct axp_regmap_irq_chip {
	const char *name;
	unsigned int status_base;
	unsigned int enable_base;
	int num_regs;
};

struct axp_irq_chip_data {
	struct mutex lock;
	struct axp_regmap *map;
	struct axp_regmap_irq_chip *chip;
	struct axp_regmap_irq *irqs;
	int num_irqs;
	u64 irqs_enabled;
	void (*wakeup_event)(void);
};

struct axp_dev {
	struct device            *dev;
	struct axp_regmap        *regmap;
	int                      nr_cells;
	struct mfd_cell          *cells;
	struct axp_irq_chip_data *irq_data;
	int                      irq;
	bool                     is_dummy;
	bool                     is_slave;
	struct list_head         list;
	int                      pmu_num;
};

struct axp_platform_ops {
	s32 (*usb_det)(void);
	s32 (*usb_vbus_output)(int);
	int (*cfg_pmux_para)(int, struct aw_pm_info *, int *);
	const char * (*get_pmu_name)(void);
	struct axp_dev * (*get_pmu_dev)(void);
	int (*pmu_regulator_save)(void);
	void (*pmu_regulator_restore)(void);
	char *powerkey_name[8];
	char *regulator_name[8];
	char *charger_name[8];
	char *gpio_name[8];
};

struct axp_interrupts {
	char *name;
	irq_handler_t isr;
};

#define AXP_DUMP_ATTR(_name)				\
{							\
	.attr = { .name = #_name, .mode = 0644 },	\
	.show =  _name##_show,				\
	.store = _name##_store,				\
}

enum {
	AXP_SPLY = 1U << 0,
	AXP_REGU = 1U << 1,
	AXP_INT  = 1U << 2,
	AXP_CHG  = 1U << 3,
};

enum {
	AXP_NOT_SUSPEND = 1U << 0,
	AXP_WAS_SUSPEND = 1U << 1,
	AXP_SUSPEND_WITH_IRQ = 1U << 2,
};

#define AXP_DEBUG(level_mask, pmu_num, fmt, arg...)   \
	{ if (unlikely(axp_debug & level_mask)) {     \
		printk("[%s] ", get_pmu_cur_name(pmu_num));  \
		printk(fmt, ##arg);                   \
	 }                                            \
	}

struct axp_regmap *axp_regmap_init_i2c(struct device *dev);
struct axp_regmap *axp_regmap_init_arisc_rsb(struct device *dev, u8 addr);
struct axp_regmap *axp_regmap_init_arisc_twi(struct device *dev);
struct axp_irq_chip_data *axp_irq_chip_register(struct axp_regmap *map,
	int irq, int irq_flags, struct axp_regmap_irq_chip *irq_chip,
	void (*wakeup_event)(void));
void axp_irq_chip_unregister(int irq, struct axp_irq_chip_data *irq_data);

int axp_regmap_write(struct axp_regmap *map, s32 reg, u8 val);
int axp_regmap_writes(struct axp_regmap *map, s32 reg, s32 len, u8 *val);
int axp_regmap_read(struct axp_regmap *map, s32 reg, u8 *val);
int axp_regmap_reads(struct axp_regmap *map, s32 reg, s32 len, u8 *val);
int axp_regmap_update(struct axp_regmap *map, s32 reg, u8 val, u8 mask);
int axp_regmap_set_bits(struct axp_regmap *map, s32 reg, u8 bit_mask);
int axp_regmap_clr_bits(struct axp_regmap *map, s32 reg, u8 bit_mask);
int axp_regmap_update_sync(struct axp_regmap *map, s32 reg, u8 val, u8 mask);
int axp_regmap_set_bits_sync(struct axp_regmap *map, s32 reg, u8 bit_mask);
int axp_regmap_clr_bits_sync(struct axp_regmap *map, s32 reg, u8 bit_mask);
int axp_mfd_add_devices(struct axp_dev *axp_dev);
int axp_mfd_remove_devices(struct axp_dev *axp_dev);
int axp_request_irq(struct axp_dev *adev, int irq_no,
				irq_handler_t handler, void *data);
int axp_free_irq(struct axp_dev *adev, int irq_no);
int axp_dt_parse(struct device_node *node, int pmu_num,
				struct axp_config_info *axp_config);
void axp_platform_ops_set(int pmu_num, struct axp_platform_ops *ops);
void axp_dev_set(struct axp_dev *axpdev);
int axp_gpio_irq_register(struct axp_dev *adev, int irq_no,
				irq_handler_t handler, void *data);
struct axp_dev *get_pmu_cur_dev(int pmu_num);
int axp_mfd_cell_name_init(struct axp_platform_ops *ops, int count,  int pmu_num,
				int size, struct mfd_cell *cells);
int axp_get_pmu_num(const struct of_device_id *ids, int size);

#ifdef CONFIG_AXP_NMI_USED
extern void clear_nmi_status(void);
extern void disable_nmi(void);
extern void enable_nmi(void);
extern void set_nmi_trigger(u32 trigger);
#endif

extern int axp_suspend_flag;
extern int axp_num;
extern const char *axp_name[AXP_ONLINE_SUM];
extern const char *get_pmu_cur_name(int pmu_num);

#endif /* AXP_CORE_H_ */
