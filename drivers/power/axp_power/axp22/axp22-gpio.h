#ifndef AXP22_GPIO_H
#define AXP22_GPIO_H
#include "axp22.h"

#define AXP_GPIO_IRQ_EDGE_RISING   (0x1<<7)
#define AXP_GPIO_IRQ_EDGE_FALLING  (0x1<<6)
#define AXP_GPIO_INPUT_TRIG_MASK   (0x7<<0)
#define AXP_GPIO_EDGE_TRIG_MASK    (AXP_GPIO_IRQ_EDGE_RISING | \
				AXP_GPIO_IRQ_EDGE_FALLING)

#define AXP_GPIO0_CFG        (AXP22_GPIO0_CTL)     /* 0x90 */
#define AXP_GPIO1_CFG        (AXP22_GPIO1_CTL)     /* 0x92 */
#define AXP_GPIO01_STATE     (AXP22_GPIO01_SIGNAL) /* 0x94 */
#define AXP_GPIO01_INTEN     (AXP22_INTEN5)        /* 0x44 */
#define AXP_GPIO01_INTSTA    (AXP22_INTSTS5)       /* 0x4C */

#endif
