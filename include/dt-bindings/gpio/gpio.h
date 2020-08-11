/*
 * This header provides constants for most GPIO bindings.
 *
 * Most GPIO bindings include a flags cell as part of the GPIO specifier.
 * In most cases, the format of the flags cell uses the standard values
 * defined in this header.
 */

#ifndef _DT_BINDINGS_GPIO_GPIO_H
#define _DT_BINDINGS_GPIO_GPIO_H

/* Bit 0 express polarity */
#define GPIO_ACTIVE_HIGH 0
#define GPIO_ACTIVE_LOW 1

/* Bit 1 express single-endedness */
#define GPIO_PUSH_PULL 0
#define GPIO_SINGLE_ENDED 2

/*
 * Open Drain/Collector is the combination of single-ended active low,
 * Open Source/Emitter is the combination of single-ended active high.
 */
#define GPIO_OPEN_DRAIN (GPIO_SINGLE_ENDED | GPIO_ACTIVE_LOW)
#define GPIO_OPEN_SOURCE (GPIO_SINGLE_ENDED | GPIO_ACTIVE_HIGH)

/* sunxi gpio arg */
#define  PA  0
#define  PB  1
#define  PC  2
#define  PD  3
#define  PE  4
#define  PF  5
#define  PG  6
#define  PH  7
#define  PI  8
#define  PJ  9
#define  PK  10
#define  PL  11
#define  PM  12
#define  PN  13
#define  PO  14
#define  PP  15
#define  default 0xffffffff

#endif
