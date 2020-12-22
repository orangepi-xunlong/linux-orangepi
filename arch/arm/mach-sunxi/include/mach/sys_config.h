/*
 * arch/arch/mach-sunxi/include/mach/sys_config.h
 * (C) Copyright 2010-2015
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * Kevin <Kevin@allwinnertech.com>
 *
 * sys_config utils (porting from 2.6.36)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#ifndef __SW_SYS_CONFIG_H
#define __SW_SYS_CONFIG_H

#include <mach/gpio.h>

/* pio end, invalid macro */
#define GPIO_INDEX_INVALID	(0xFFFFFFF0      )
#define GPIO_CFG_INVALID	(0xEEEEEEEE      )
#define GPIO_PULL_INVALID	(0xDDDDDDDD      )
#define GPIO_DRVLVL_INVALID	(0xCCCCCCCC      )
#define IRQ_NUM_INVALID		(0xFFFFFFFF      )
#define AXP_PORT_VAL		(0x0000FFFF      ) /* port val for axp pin in sys_config.fex */

/* pio default macro */
#define GPIO_PULL_DEFAULT	((u32)-1         )
#define GPIO_DRVLVL_DEFAULT	((u32)-1         )
#define GPIO_DATA_DEFAULT	((u32)-1         )

/* gpio config info */
struct gpio_config {
	u32	gpio;		/* gpio global index, must be unique */
	u32 	mul_sel;	/* multi sel val: 0 - input, 1 - output... */
	u32 	pull;		/* pull val: 0 - pull up/down disable, 1 - pull up... */
	u32 	drv_level;	/* driver level val: 0 - level 0, 1 - level 1... */
	u32	data;		/* data val: 0 - low, 1 - high, only vaild when mul_sel is input/output */
};

/*
 * define types of script item
 * @SCIRPT_ITEM_VALUE_TYPE_INVALID:  invalid item type
 * @SCIRPT_ITEM_VALUE_TYPE_INT: integer item type
 * @SCIRPT_ITEM_VALUE_TYPE_STR: strint item type
 * @SCIRPT_ITEM_VALUE_TYPE_PIO: gpio item type
 */
typedef enum {
	SCIRPT_ITEM_VALUE_TYPE_INVALID = 0,
	SCIRPT_ITEM_VALUE_TYPE_INT,
	SCIRPT_ITEM_VALUE_TYPE_STR,
	SCIRPT_ITEM_VALUE_TYPE_PIO,
} script_item_value_type_e;


/*
 * define data structure script item
 * @val: integer value for integer type item
 * @str: string pointer for sting type item
 * @gpio: gpio config for gpio type item
 */
typedef union {
    int                 val;
    char                *str;
    struct gpio_config  gpio;
} script_item_u;

int __init script_init(void);

/*
 * script_get_item
 *      get an item from script based on main_key & sub_key
 * @main_key    main key value in script which is marked by '[]'
 * @sub_key     sub key value in script which is left of '='
 * @item        item pointer for return value
 * @return      type of the item
 */
script_item_value_type_e script_get_item(char *main_key, char *sub_key, script_item_u *item);


/*
 * script_get_pio_list
 *      get gpio list from script baseed on main_key
 * @main_key    main key value in script which is marked by '[]'
 * @list        list pointer for return gpio list
 * @return      count of the gpios
 */
int script_get_pio_list(char *main_key, script_item_u **list);

/*
 * script_dump_mainkey
 *      dump main_key info
 * @main_key    main key value in script which is marked by '[]',
 *              if NULL, dump all main key info in script
 * @return      0
 */
int script_dump_mainkey(char *main_key);

/*
 * script_get_main_key_count
 *      get the count of main_key
 *
 * @return     the count of main_key
 */
unsigned int script_get_main_key_count(void);


/*
 * script_get_main_key_name
 *      get the name of main_key by index
 *
 * @main_key_index   the index of main_key
 * @main_key_name    the buffer to store target main_key_name
 * @return     the pointer of target mainkey name
 */
char *script_get_main_key_name(unsigned int main_key_index);

/*
 * script_is_main_key_exist
 *      check if the name of main_key exist
 *
 * @main_key    the buffer to store target main key name
 * @return      true if exist, false if not exist or script not initialized
 */
bool script_is_main_key_exist(char *main_key);

#endif
