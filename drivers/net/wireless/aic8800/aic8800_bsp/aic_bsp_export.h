#ifndef __AIC_BSP_EXPORT_H
#define __AIC_BSP_EXPORT_H

enum aicbsp_subsys {
	AIC_BLUETOOTH,
	AIC_WIFI,
};

enum aicbsp_pwr_state {
	AIC_PWR_OFF,
	AIC_PWR_ON,
};

struct aicbsp_feature_t {
	int      hwinfo;
	uint32_t sdio_clock;
	uint8_t  sdio_phase;
	bool     fwlog_en;
};

int aicbsp_set_subsys(int, int);
int aicbsp_get_feature(struct aicbsp_feature_t *feature);

#endif
