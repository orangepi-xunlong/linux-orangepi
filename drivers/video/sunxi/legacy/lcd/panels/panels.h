#ifndef __PANEL_H__
#define __PANEL_H__
#include "../lcd_source_interface.h"
#include "../lcd_panel_cfg.h"

typedef struct
{
	char name[32];
	__lcd_panel_fun_t func;
}__lcd_panel_t;

extern __lcd_panel_t * panel_array[];

#endif