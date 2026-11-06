#include "rwnx_main.h"
#include "rwnx_msg_tx.h"
#include "reg_access.h"

#define FW_USERCONFIG_NAME_8800D80         "aic_userconfig_8800d80.txt"
#define FW_POWERLIMIT_NAME_8800D80         "aic_powerlimit_8800d80.txt"

extern char aic_fw_path[200];

int rwnx_request_firmware_common(struct rwnx_hw *rwnx_hw,
	u32** buffer, const char *filename);
void rwnx_plat_userconfig_parsing(char *buffer, int size);
void rwnx_release_firmware_common(u32** buffer);


int aicwf_set_rf_config_8800d80(struct rwnx_hw *rwnx_hw, struct mm_set_rf_calib_cfm *cfm)
{
	int ret = 0;

	if ((ret = rwnx_send_txpwr_lvl_v3_req(rwnx_hw))) {
		return -1;
	}
	if ((ret = rwnx_send_txpwr_lvl_adj_req(rwnx_hw))) {
		return -1;
	}
	if ((ret = rwnx_send_txpwr_ofst2x_req(rwnx_hw))) {
		return -1;
	}
	if ((ret = rwnx_send_rf_calib_req(rwnx_hw, cfm))) {
		return -1;
	}
	return 0 ;
}


int	rwnx_plat_userconfig_load_8800d80(struct rwnx_hw *rwnx_hw){
    int size;
    u32 *dst=NULL;
    char *filename = FW_USERCONFIG_NAME_8800D80;

#ifndef ANDROID_PLATFORM
            sprintf(aic_fw_path, "%s/%s", aic_fw_path, "aic8800D80");
#endif

    AICWFDBG(LOGINFO, "userconfig file path:%s \r\n", filename);

    /* load file */
    size = rwnx_request_firmware_common(rwnx_hw, &dst, filename);
    if (size <= 0) {
            AICWFDBG(LOGERROR, "wrong size of firmware file\n");
            dst = NULL;
            return 0;
    }

    /* Copy the file on the Embedded side */
    AICWFDBG(LOGINFO, "### Load file done: %s, size=%d\n", filename, size);

    rwnx_plat_userconfig_parsing((char *)dst, size);

    rwnx_release_firmware_common(&dst);

    AICWFDBG(LOGINFO, "userconfig download complete\n\n");
    return 0;

}

#ifdef CONFIG_POWER_LIMIT
extern char country_code[];
int rwnx_plat_powerlimit_load_8800d80(struct rwnx_hw *rwnx_hw)
{
    int size;
    u32 *dst=NULL;
    char *filename = FW_POWERLIMIT_NAME_8800D80;

    AICWFDBG(LOGINFO, "powerlimit file path:%s \r\n", filename);

    /* load file */
    size = rwnx_request_firmware_common(rwnx_hw, &dst, filename);
    if (size <= 0) {
        AICWFDBG(LOGERROR, "wrong size of cfg file\n");
        dst = NULL;
        return 0;
    }

    AICWFDBG(LOGINFO, "### Load file done: %s, size=%d\n", filename, size);

    /* parsing the file */
    rwnx_plat_powerlimit_parsing((char *)dst, size, country_code);

    rwnx_release_firmware_common(&dst);

    AICWFDBG(LOGINFO, "powerlimit download complete\n\n");
    return 0;
}
#endif

