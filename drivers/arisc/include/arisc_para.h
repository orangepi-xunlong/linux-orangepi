#ifndef __ARISC_PARA_H__
#define __ARISC_PARA_H__

#include <linux/power/axp_depend.h>

#define ARISC_MACHINE_PAD    0
#define ARISC_MACHINE_HOMLET 1

/* arisc parameter size: 128byte */
/*
 * machine: machine id, pad = 0, homlet = 1;
 * message_pool_phys: message pool physical address;
 * message_pool_size: message pool size;
 */
#define SERVICES_DVFS_USED (1<<0)

typedef struct arisc_para
{
	unsigned int machine;
	unsigned int oz_scale_delay;
	unsigned int oz_onoff_delay;
	unsigned int message_pool_phys;
	unsigned int message_pool_size;
	unsigned int uart_pin_used;
	unsigned int services_used;
	unsigned int power_regu_tree[VCC_MAX_INDEX];
	unsigned int reseved[10];
} arisc_para_t;

#define ARISC_PARA_SIZE (sizeof(struct arisc_para))

#endif /* __ARISC_PARA_H__ */
