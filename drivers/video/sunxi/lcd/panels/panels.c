#include "panels.h"
#include "default_panel.h"
#include "tft720x1280.h"
#include "B116XAN03.h"
#include "gm7121_cvbs.h"

extern __lcd_panel_t tft720x1280_panel;
extern __lcd_panel_t vvx10f004b00_panel;
extern __lcd_panel_t lp907qx_panel;
extern __lcd_panel_t starry768x1024_panel;
extern __lcd_panel_t sl698ph_720p_panel;
extern __lcd_panel_t B116XAN03_panel;
extern __lcd_panel_t gm7121_cvbs;

__lcd_panel_t* panel_array[] = {
	&gm7121_cvbs,
	&default_panel,
	&tft720x1280_panel,
	&vvx10f004b00_panel,
	&lp907qx_panel,
	&starry768x1024_panel,
	&sl698ph_720p_panel,
	&B116XAN03_panel,
	/* add new panel below */

	NULL,
};

