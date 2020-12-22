#ifndef _LINUX_AXP19_REGU_H_
#define _LINUX_AXP19_REGU_H_

/* AXP19 Regulator Registers */
#define AXP19_LDO1              POWER19_STATUS
#define AXP19_LDO2              POWER19_LDO24OUT_VOL
#define AXP19_LDO3              POWER19_LDO3OUT_VOL
#define AXP19_LDO4              POWER19_LDO24OUT_VOL
#define AXP19_LDOIO0            POWER19_GPIO0_VOL
#define AXP19_DCDC1             POWER19_DC1OUT_VOL
#define AXP19_DCDC2             POWER19_DC2OUT_VOL
#define AXP19_DCDC3             POWER19_DC3OUT_VOL

#define AXP19_LDO1EN            POWER19_STATUS
#define AXP19_LDO2EN            POWER19_LDO24_DC13_CTL
#define AXP19_LDO3EN            POWER19_LDO3_DC2_CTL
#define AXP19_LDO4EN            POWER19_LDO24_DC13_CTL
#define AXP19_LDOIOEN           POWER19_GPIO0_CTL
#define AXP19_DCDC1EN           POWER19_LDO24_DC13_CTL
#define AXP19_DCDC2EN           POWER19_LDO3_DC2_CTL
#define AXP19_DCDC3EN           POWER19_LDO24_DC13_CTL

#define AXP19_DCDC_MODESET      POWER19_DCDC_MODESET
#define AXP19_DCDC_FREQSET      POWER19_DCDC_FREQSET

extern struct axp_funcdev_info (* axp19_regu_init(void))[8];
extern void axp19_regu_exit(void);

#endif

