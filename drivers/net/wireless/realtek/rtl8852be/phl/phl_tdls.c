#define _PHL_TDLS_C_
#include "phl_headers.h"

#ifdef CONFIG_PHL_TDLS
enum rtw_phl_status
rtw_phl_tdls_init_ops(
	void *phl,
	struct rtw_phl_tdls_ops *ops)
{
	enum rtw_phl_status status = RTW_PHL_STATUS_FAILURE;
	struct phl_info_t *phl_info = (struct phl_info_t *)phl;
	struct phl_tdls_info_t *tdls_info = &phl_info->tdls_info;

	if(tdls_info == NULL)
		goto exit;

	tdls_info->ops.priv = ops->priv;
	tdls_info->ops.check_tdls_frame = ops->check_tdls_frame;
	status = RTW_PHL_STATUS_SUCCESS;
exit:
	return status;
}
#endif