#ifndef _PHL_TDLS_EN_H_
#define _PHL_TDLS_EN_H_

#ifdef CONFIG_PHL_TDLS
struct rtw_phl_tdls_ops{
	void *priv;
	bool (*check_tdls_frame)(void *priv,
	                      struct rtw_phl_stainfo_t *phl_sta,
	                      struct rtw_recv_pkt *phlrx);
};
struct phl_tdls_info_t{
	struct rtw_phl_tdls_ops ops;
};
enum rtw_phl_status
rtw_phl_tdls_init_ops(
	void *phl,
	struct rtw_phl_tdls_ops *ops);
#endif
#endif
