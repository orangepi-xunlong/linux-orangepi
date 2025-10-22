#include "rwnx_main.h"
#include "rwnx_msg_tx.h"
#include "reg_access.h"
#include "aicwf_8800d80.h"

int aicwf_set_rf_config_8800d80(struct rwnx_hw *rwnx_hw, struct mm_set_rf_calib_cfm *cfm)
{
	int ret = 0;

	ret = rwnx_send_txpwr_lvl_v3_req(rwnx_hw);
	if (ret)
		return ret;

	ret = rwnx_send_txpwr_lvl_adj_req(rwnx_hw);
	if (ret)
		return ret;

	ret = rwnx_send_txpwr_ofst2x_req(rwnx_hw);
	if (ret)
		return ret;

	ret = rwnx_send_rf_calib_req(rwnx_hw, cfm);
	if (ret)
		return ret;

	return ret;
}

