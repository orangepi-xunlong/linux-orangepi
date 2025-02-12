// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef __MV_PLATFORM_USB_H
#define __MV_PLATFORM_USB_H

#include <linux/notifier.h>

enum pxa_ehci_type {
	EHCI_UNDEFINED = 0,
	PXA_U2OEHCI,	/* pxa 168, 9xx */
	PXA_SPH,	/* pxa 168, 9xx SPH */
	MMP3_HSIC,	/* mmp3 hsic */
	MMP3_FSIC,	/* mmp3 fsic */
};

/* for usb middle layer support */
enum {
	PXA_USB_DEV_OTG,
	PXA_USB_DEV_SPH1,
	PXA_USB_DEV_SPH2,
	PXA_USB_DEV_SPH3,
	PXA_USB_DEV_MAX,
};

enum {
	EVENT_VBUS,
	EVENT_ID,
};

struct pxa_usb_vbus_ops {
	int (*get_vbus)(unsigned int *level);
	int (*set_vbus)(unsigned int level);
	int (*init)(void);
};

enum {
	VBUS_LOW	= 0,
	VBUS_HIGH	= 1 << 0,
};

struct pxa_usb_idpin_ops {
	int (*get_idpin)(unsigned int *level);
	int (*init)(void);
 };

struct pxa_usb_extern_ops {
	struct pxa_usb_vbus_ops		vbus;
	struct pxa_usb_idpin_ops	idpin;
};

#define pxa_usb_has_extern_call(id, o, f, arg...)	( \
{ \
	struct pxa_usb_extern_ops *ops;			\
	int ret;					\
	ops  = pxa_usb_get_extern_ops(id);		\
	ret = (!ops ? 0 : ((ops->o.f) ?			\
		1 : 0));				\
	ret;						\
} \
)

#define pxa_usb_extern_call(id, o, f, args...)	( \
{ \
	struct pxa_usb_extern_ops *ops;			\
	int ret;					\
	ops  = pxa_usb_get_extern_ops(id);		\
	ret = (!ops ? -ENODEV : ((ops->o.f) ?		\
		ops->o.f(args) : -ENOIOCTLCMD));	\
	ret;						\
} \
)

#define pxa_usb_set_extern_call(id, o, f, p) ( \
{ \
	struct pxa_usb_extern_ops *ops;		\
	int ret;				\
	ops = pxa_usb_get_extern_ops(id);	\
	ret = !ops ? -ENODEV : ((ops->o.f) ?	\
		-EINVAL : ({ops->o.f = p; 0; }));\
	ret;					\
} \
)

extern int mv_udc_register_client(struct notifier_block *nb);
extern int mv_udc_unregister_client(struct notifier_block *nb);

#ifdef CONFIG_MV_USB_CONNECTOR
extern struct pxa_usb_extern_ops *pxa_usb_get_extern_ops(unsigned int id);
extern int pxa_usb_register_notifier(unsigned int id,
					struct notifier_block *nb);
extern int pxa_usb_unregister_notifier(unsigned int id,
					struct notifier_block *nb);
extern int pxa_usb_notify(unsigned int id, unsigned long val, void *v);
#else
static inline struct pxa_usb_extern_ops *pxa_usb_get_extern_ops(unsigned int id) {return NULL;}
static inline int pxa_usb_register_notifier(unsigned int id, struct notifier_block *nb) {return 0;}
static inline int pxa_usb_unregister_notifier(unsigned int id, struct notifier_block *nb) {return 0;}
static inline int pxa_usb_notify(unsigned int id, unsigned long val, void *v) {return 0;}
/* end of usb middle layer support */
#endif

struct mv_usb_platform_data {
	unsigned int		clknum;
	char			**clkname;
	/*
	 * select from PXA_USB_DEV_OTG to PXA_USB_DEV_MAX.
	 * It indicates the index of usb device.
	 */
	unsigned int		id;
	unsigned int		extern_attr;

	/* only valid for HCD. OTG or Host only*/
	unsigned int		mode;

	/* This flag is used for that needs id pin checked by otg */
	unsigned int    disable_otg_clock_gating:1;
	/* Force a_bus_req to be asserted */
	unsigned int    otg_force_a_bus_req:1;
};

enum charger_type {
	NULL_CHARGER    = 0,
	DEFAULT_CHARGER,
	DCP_CHARGER,            /* standard wall charger */
	CDP_CHARGER,            /* Charging Downstream Port */
	SDP_CHARGER,            /* standard PC charger */
	NONE_STANDARD_CHARGER,  /* none-standard charger */
	MAX_CHARGER
};

#endif
