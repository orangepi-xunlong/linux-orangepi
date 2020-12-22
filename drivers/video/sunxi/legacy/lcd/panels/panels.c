#include "panels.h"
#include "default_panel.h"
#include "lp079x01.h"

extern __lcd_panel_t lp079x01_panel;
extern __lcd_panel_t default_panel;

__lcd_panel_t* panel_array[] = {
	&default_panel,
	&lp079x01_panel,
	/* add new panel below */

	NULL,
};

