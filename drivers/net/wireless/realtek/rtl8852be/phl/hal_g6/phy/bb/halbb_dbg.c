/******************************************************************************
 *
 * Copyright(c) 2007 - 2020  Realtek Corporation.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * The full GNU General Public License is included in this distribution in the
 * file called LICENSE.
 *
 * Contact Information:
 * wlanfae <wlanfae@realtek.com>
 * Realtek Corporation, No. 2, Innovation Road II, Hsinchu Science Park,
 * Hsinchu 300, Taiwan.
 *
 * Larry Finger <Larry.Finger@lwfinger.net>
 *
 *****************************************************************************/

#include "halbb_precomp.h"

#define HALBB_DBG_DVLP_FLAG 1

void halbb_dbg_comp_init(struct bb_info *bb)
{
	#if 0
	if (bb->phl_com->bb_dbg_comp_manual_cfg != 0xffffffff) {
		bb->dbg_component = bb->phl_com->bb_dbg_comp_manual_cfg;
		return;
	}
	#endif

	bb->dbg_component =
		/*DBG_RA |		*/
		/*DBG_FA_CNT |		*/
		/*DBG_RSSI_MNTR |	*/
		/*DBG_DFS |		*/
		/*DBG_EDCCA |		*/
		/*DBG_ENV_MNTR |	*/
		/*DBG_CFO_TRK |		*/
		/*DBG_SNIFFER |		*/
		/*DBG_PHY_STATUS |	*/
		/*DBG_CONNECT	|	*/
		/*DBG_FW_INFO	|	*/
		/*DBG_COMMON_FLOW |	*/
		/*DBG_IC_API |		*/
		/*DBG_DBG_API |		*/
		/*DBG_DBCC |		*/
		/*DBG_DM_SUMMARY |	*/
		/*DBG_PHY_CONFIG |	*/
		/*DBG_INIT |		*/
		/*DBG_DIG |		*/
		/*DBG_CH_INFO |		*/
		/*DBG_CMN |		*/
		0;

	BB_DBG(bb, DBG_INIT, "HALBB dbg_comp = 0x%llx\n", bb->dbg_component);

}

void halbb_print_devider(struct bb_info *bb, u8 len, bool with_space, u64 comp) {

	if (comp != FRC_PRINT_LINE && (bb->dbg_component & comp) == 0)
		return;

	if (len == BB_DEVIDER_LEN_32)
		BB_TRACE("--------------------------------\n");
	else
		BB_TRACE("----------------\n");

	if (with_space)
		BB_TRACE("\n");
}

#ifdef HALBB_TDMA_CR_SUPPORT

void halbb_tdma_cr_sel_io_en(struct bb_info *bb)
{
	BB_DBG(bb, DBG_DBG_API, "[%s]===>\n", __func__);

	halbb_tdma_cr_sel_main(bb);
}

void halbb_tdma_cr_sel_callback(void *context)
{
	struct bb_info *bb = (struct bb_info *)context;
	struct halbb_timer_info *timer = &bb->bb_dbg_i.tdma_cr_timer_i;

	BB_DBG(bb, DBG_DBG_API, "[%s]===>\n", __func__);
	timer->timer_state = BB_TIMER_IDLE;

	rtw_hal_cmd_notify(bb->phl_com, MSG_EVT_NOTIFY_BB, (void *)(&timer->event_idx), bb->bb_phy_idx);
}

void halbb_tdma_cr_timer_init(struct bb_info *bb)
{
	struct halbb_timer_info *timer = &bb->bb_dbg_i.tdma_cr_timer_i;
		
	BB_DBG(bb, DBG_INIT, "[%s]\n", __func__);

	timer->event_idx = BB_EVENT_TIMER_TDMA_CR;
	timer->timer_state = BB_TIMER_IDLE;

	halbb_init_timer(bb, &timer->timer_list, halbb_tdma_cr_sel_callback, bb, "halbb_tdma_cr");
}

void halbb_tdma_cr_sel_main(struct bb_info *bb)
{
	struct bb_dbg_info *dbg = &bb->bb_dbg_i;
	u32 period = 0;
	u32 val_new = 0, val_old = 0;

	if (!dbg->tdma_cr_en) {
		BB_DBG(bb, DBG_DBG_API, "[%s] tdma_cr_en = %d\n", __func__, dbg->tdma_cr_en);
		return;
	}

	if (dbg->tdma_cr_state == 0) {
		val_old = dbg->tdma_cr_val_1;
		val_new = dbg->tdma_cr_val_0;
		period = dbg->tdma_cr_period_0;
		dbg->tdma_cr_state = 1; /*CR0*/
	} else { /*PFD_LEGACY*/
		val_old = dbg->tdma_cr_val_0;
		val_new = dbg->tdma_cr_val_1;
		period = dbg->tdma_cr_period_1;
		dbg->tdma_cr_state= 0;  /*CR1*/
	}

	halbb_set_reg(bb, dbg->tdma_cr_idx, dbg->tdma_cr_mask, val_new);

	dbg->tdma_cr_timer_i.cb_time = period;
	halbb_cfg_timers(bb, BB_SET_TIMER, &dbg->tdma_cr_timer_i);

	BB_DBG(bb, DBG_DBG_API, "Reg 0x%x[0x%x] = {0x%x -> 0x%x}, period=%d ms\n",
	       dbg->tdma_cr_idx, dbg->tdma_cr_mask,
	       val_old , val_new, period);
}

void halbb_tdma_cr_sel_init(struct bb_info *bb)
{
	struct bb_dbg_info *dbg = &bb->bb_dbg_i;

	BB_DBG(bb, DBG_DBG_API, "[%s]\n", __func__);
	dbg->tdma_cr_en = false;
	dbg->tdma_cr_period_0 = 50;
	dbg->tdma_cr_period_1 = 50;
	dbg->tdma_cr_idx = 0;
	dbg->tdma_cr_state = 0;
}
#endif

#if 1 /*debug port - relative*/
void halbb_bb_dbg_port_clock_en(struct bb_info *bb, u8 enable)
{
	struct bb_dbg_cr_info *cr = &bb->bb_dbg_i.bb_dbg_cr_i;
	u32 reg_value = 0;

	reg_value = enable ? 1 : 0;
	halbb_set_reg_cmn(bb, cr->clk_en, cr->clk_en_m, reg_value, bb->bb_phy_idx);
	halbb_set_reg_cmn(bb, cr->dbgport_en, cr->dbgport_en_m, reg_value, bb->bb_phy_idx);

}

u32 halbb_get_bb_dbg_port_idx(struct bb_info *bb)
{
	struct bb_dbg_cr_info *cr = &bb->bb_dbg_i.bb_dbg_cr_i;
	u32 val = 0;
	u32 dbg_port, ip;

	ip = halbb_get_reg_cmn(bb, cr->dbgport_ip, cr->dbgport_ip_m, bb->bb_phy_idx);
	dbg_port = halbb_get_reg_cmn(bb, cr->dbgport_idx, cr->dbgport_idx_m, bb->bb_phy_idx);

	val = (ip << 8) | (dbg_port & 0xff);

	return val;
}

void halbb_set_bb_dbg_port_ip(struct bb_info *bb, enum bb_dbg_port_ip_t ip)
{
	struct bb_dbg_cr_info *cr = &bb->bb_dbg_i.bb_dbg_cr_i;

	halbb_set_reg_cmn(bb, cr->dbgport_ip, cr->dbgport_ip_m, ip, bb->bb_phy_idx);
}

void halbb_set_bb_dbg_port(struct bb_info *bb, u32 dbg_port)
{
	struct bb_dbg_cr_info *cr = &bb->bb_dbg_i.bb_dbg_cr_i;

	halbb_set_reg_cmn(bb, cr->dbgport_idx, cr->dbgport_idx_m, dbg_port, bb->bb_phy_idx);
}

bool halbb_bb_dbg_port_racing(struct bb_info *bb, u8 curr_dbg_priority)
{
	
	bool dbg_port_result = false;

	if (curr_dbg_priority > bb->pre_dbg_priority) {

		halbb_bb_dbg_port_clock_en(bb, true);

		BB_DBG(bb, DBG_DBG_API,
		       "DbgPort racing success, Cur_priority=((%d)), Pre_priority=((%d))\n",
		       curr_dbg_priority, bb->pre_dbg_priority);

		bb->pre_dbg_priority = curr_dbg_priority;
		dbg_port_result = true;
	}

	return dbg_port_result;
}

void halbb_release_bb_dbg_port(struct bb_info *bb)
{
	halbb_bb_dbg_port_clock_en(bb, false);
	bb->pre_dbg_priority = DBGPORT_RELEASE;
	BB_DBG(bb, DBG_DBG_API, "[%s] pre_dbg_priority=%d\n", __func__, bb->pre_dbg_priority);
}

u32 halbb_get_bb_dbg_port_val(struct bb_info *bb)
{
	struct bb_dbg_cr_info *cr = &bb->bb_dbg_i.bb_dbg_cr_i;
	u32 dbg_port_value = 0;

	dbg_port_value = halbb_get_reg_cmn(bb, cr->dbgport_val, cr->dbgport_val_m, bb->bb_phy_idx);
	BB_DBG(bb, DBG_DBG_API, "dbg_port_value = 0x%x\n", dbg_port_value);
	
	return dbg_port_value;
}

void halbb_dbgport_dump_all(struct bb_info *bb, u32 *_used, char *output,
			    u32 *_out_len)
{
	switch (bb->ic_type) {

	#ifdef BB_8852A_2_SUPPORT
	case BB_RTL8852A:
		halbb_dbgport_dump_all_8852a_2(bb, _used, output, _out_len);
		break;
	#endif

	#ifdef BB_8852B_SUPPORT
	case BB_RTL8852B:
		halbb_dbgport_dump_all_8852b(bb, _used, output, _out_len);
		break;
	#endif

	#ifdef BB_8852C_SUPPORT
	case BB_RTL8852C:
		halbb_dbgport_dump_all_8852c(bb, _used, output, _out_len);
		break;
	#endif

	#ifdef BB_8192XB_SUPPORT
	case BB_RTL8192XB:
		halbb_dbgport_dump_all_8192xb(bb, _used, output, _out_len);
		break;
	#endif

	#ifdef BB_8851B_SUPPORT
	case BB_RTL8851B:
		halbb_dbgport_dump_all_8851b(bb, _used, output, _out_len);
		break;
	#endif
	#ifdef BB_1115_SUPPORT
	case BB_RLE1115:
		halbb_dbgport_dump_all_1115(bb, _used, output, _out_len);
		break;
	#endif
	#ifdef BB_8922A_SUPPORT
	case BB_RTL8922A:
		halbb_dbgport_dump_all_8922a(bb, _used, output, _out_len);
		break;
	#endif

	default:
		break;
	}
}

void halbb_dbgport_dbg(struct bb_info *bb, char input[][16], u32 *_used,
			 char *output, u32 *_out_len)
{
	u32 val[10] = {0};
	u32 used = *_used;
	u32 out_len = *_out_len;
	u32 dp = 0; /*debug port value*/
	u8 dbg[32];
	u8 tmp = 0;
	u8 i;

	if (_os_strcmp(input[1], "-h") == 0) {
		BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
			 "{dbg_port_ip} {dbg_port_idx}\n");
		BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
			 "{dump_all}\n");
		return;
	} 

	if (!halbb_bb_dbg_port_racing(bb, DBGPORT_PRI_3)) {
		BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
			    "{Racing Fail} pre_dbg_priority=%d\n", bb->pre_dbg_priority);
		return;
	}

	if (_os_strcmp(input[1], "dump_all") == 0) {
		halbb_dbgport_dump_all(bb, _used, output, _out_len);
		return;
	} else {

		HALBB_SCAN(input[1], DCMD_HEX, &val[0]);
		HALBB_SCAN(input[2], DCMD_HEX, &val[1]);

		halbb_set_bb_dbg_port_ip(bb, (enum bb_dbg_port_ip_t)val[0]);
		halbb_set_bb_dbg_port(bb, val[1]);
		dp = halbb_get_bb_dbg_port_val(bb);
		halbb_release_bb_dbg_port(bb);

		for (i = 0; i < 32; i++)
			dbg[i] = (u8)((dp & BIT(i)) >> i);

		BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
			    "Dbg Port[0x%02x, 0x%03x] = 0x%08x\n", val[0], val[1], dp);

		for (i = 4; i != 0; i--) {
			tmp = 8 * (i - 1);
			BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
				 "val[%02d:%02d] = 8b'%d %d %d %d %d %d %d %d\n",
				 tmp + 7, tmp, dbg[tmp + 7], dbg[tmp + 6],
				 dbg[tmp + 5], dbg[tmp + 4], dbg[tmp + 3],
				 dbg[tmp + 2], dbg[tmp + 1], dbg[tmp + 0]);
		}
	}
}

u32 halbb_get_bb_mntr(struct bb_info *bb, u8 mntr_idx, u32 sel)
{
	struct bb_dbg_cr_info *cr = &bb->bb_dbg_i.bb_dbg_cr_i;
	u32 bb_mntr = 0;

	if (mntr_idx == 0) {
		halbb_set_reg(bb, cr->bb_monitor_sel0, cr->bb_monitor_sel0_m, sel);
		bb_mntr = halbb_get_reg(bb, cr->bb_monitor0, cr->bb_monitor0_m);
	} else {
		halbb_set_reg(bb, cr->bb_monitor_sel1, cr->bb_monitor_sel1_m, sel);
		bb_mntr = halbb_get_reg(bb, cr->bb_monitor1, cr->bb_monitor1_m);
	}

	return bb_mntr;
}

u32 halbb_get_dbgport_val_by_racing(struct bb_info *bb, u32 dbg_port)
{	
	u32 dbg_port_value = 0;

	if (!halbb_bb_dbg_port_racing(bb, DBGPORT_PRI_3))
		return 0xdeadbeef;

	halbb_set_bb_dbg_port_ip(bb, (dbg_port & 0xff0000) >> 16);
	halbb_set_bb_dbg_port(bb, dbg_port & 0xffff);
	dbg_port_value = halbb_get_bb_dbg_port_val(bb);
	halbb_release_bb_dbg_port(bb);

	//BB_DBG(bb, DBG_AUTO_DBG, "dbg_port[0x%x]=(0x%x)\n", dbg_port, dbg_port_value);

	return dbg_port_value;
}

#endif

#if HALBB_DBG_DVLP_FLAG /*Common debug message - relative*/
void halbb_crc32_cnt2_cmn_log(struct bb_info *bb)
{
	struct bb_stat_info *stat_t = &bb->bb_stat_i;
	struct bb_crc2_info *crc2 = &stat_t->bb_crc2_i;
	struct bb_usr_set_info *usr_set = &stat_t->bb_usr_set_i;
	char dbg_buf[4][HALBB_SNPRINT_SIZE];

	halbb_mem_set(bb, dbg_buf, 0, sizeof(dbg_buf[0][0]) * 4 * HALBB_SNPRINT_SIZE);

	halbb_print_rate_2_buff(bb, usr_set->ofdm2_rate_idx,
				RTW_GILTF_LGI_4XHE32, dbg_buf[0], HALBB_SNPRINT_SIZE);
	halbb_print_rate_2_buff(bb, usr_set->ht2_rate_idx,
				RTW_GILTF_LGI_4XHE32, dbg_buf[1], HALBB_SNPRINT_SIZE);
	halbb_print_rate_2_buff(bb, usr_set->vht2_rate_idx,
				RTW_GILTF_LGI_4XHE32, dbg_buf[2], HALBB_SNPRINT_SIZE);
	halbb_print_rate_2_buff(bb, usr_set->he2_rate_idx,
				RTW_GILTF_LGI_4XHE32, dbg_buf[3], HALBB_SNPRINT_SIZE);

	BB_DBG(bb, DBG_CMN,
	       "[CRC32 OK Cnt] {%s, %s, %s, %s}= {%d, %d, %d, %d}\n",
	       dbg_buf[0], dbg_buf[1], dbg_buf[2], dbg_buf[3],
	       crc2->cnt_ofdm2_crc32_ok, crc2->cnt_ht2_crc32_ok,
	       crc2->cnt_vht2_crc32_ok, crc2->cnt_he2_crc32_ok);

	BB_DBG(bb, DBG_CMN,
	       "[CRC32 Err Cnt] {%s, %s, %s , %s}= {%d, %d, %d, %d}\n",
	       dbg_buf[0], dbg_buf[1], dbg_buf[2], dbg_buf[3],
	       crc2->cnt_ofdm2_crc32_error, crc2->cnt_ht2_crc32_error,
	       crc2->cnt_vht2_crc32_error, crc2->cnt_he2_crc32_error);
}

void halbb_crc32_cnt3_cmn_log(struct bb_info *bb)
{
	struct bb_stat_info *stat_t = &bb->bb_stat_i;
	struct bb_usr_set_info *usr_set = &stat_t->bb_usr_set_i;
	struct bb_crc2_info *crc2 = &stat_t->bb_crc2_i;

	u32 total_cnt = 0;
	u8 pcr = 0;
	total_cnt = crc2->cnt_ofdm3_crc32_ok + crc2->cnt_ofdm3_crc32_error;
	pcr = (u8)HALBB_DIV(crc2->cnt_ofdm3_crc32_ok * 100, total_cnt);

	switch(usr_set->stat_type_sel_i) {
	case STATE_PROBE_RESP:
		BB_DBG(bb, DBG_CMN,
		  "[Probe Response Data CRC32 Cnt(OFDM only)] {error, ok}= {%d, %d} (PCR=%d percent)\n",
		  crc2->cnt_ofdm3_crc32_error,
		  crc2->cnt_ofdm3_crc32_ok, pcr);
		break;
	case STATE_BEACON:
		BB_DBG(bb, DBG_CMN,
		  "[Beacon CRC32 Cnt(OFDM only)] {error, ok}= {%d, %d} (PCR=%d percent)\n",
		  crc2->cnt_ofdm3_crc32_error,
		  crc2->cnt_ofdm3_crc32_ok, pcr);
		break;
	case STATE_ACTION:
		BB_DBG(bb, DBG_CMN,
		  "[Action CRC32 Cnt(OFDM only)] {error, ok}= {%d, %d} (PCR=%d percent)\n",
		  crc2->cnt_ofdm3_crc32_error,
		  crc2->cnt_ofdm3_crc32_ok, pcr);
		break;
	case STATE_BFRP:
		BB_DBG(bb, DBG_CMN,
		  "[BFRP CRC32 Cnt(OFDM only)] {error, ok}= {%d, %d} (PCR=%d percent)\n",
		  crc2->cnt_ofdm3_crc32_error,
		  crc2->cnt_ofdm3_crc32_ok, pcr);
		break;
	case STATE_NDPA:
		BB_DBG(bb, DBG_CMN,
		  "[NDPA CRC32 Cnt(OFDM only)] {error, ok}= {%d, %d} (PCR=%d percent)\n",
		  crc2->cnt_ofdm3_crc32_error,
		  crc2->cnt_ofdm3_crc32_ok, pcr);
		break;
	case STATE_BA:
		BB_DBG(bb, DBG_CMN,
		  "[BA CRC32 Cnt(OFDM only)] {error, ok}= {%d, %d} (PCR=%d percent)\n",
		  crc2->cnt_ofdm3_crc32_error,
		  crc2->cnt_ofdm3_crc32_ok, pcr);
		break;
	case STATE_RTS:
		BB_DBG(bb, DBG_CMN,
		  "[RTS CRC32 Cnt(OFDM only)] {error, ok}= {%d, %d} (PCR=%d percent)\n",
		  crc2->cnt_ofdm3_crc32_error,
		  crc2->cnt_ofdm3_crc32_ok, pcr);
		break;
	case STATE_CTS:
		BB_DBG(bb, DBG_CMN,
		  "[CTS CRC32 Cnt(OFDM only)] {error, ok}= {%d, %d} (PCR=%d percent)\n",
		  crc2->cnt_ofdm3_crc32_error,
		  crc2->cnt_ofdm3_crc32_ok, pcr);
		break;
	case STATE_ACK:
		BB_DBG(bb, DBG_CMN,
		  "[ACK CRC32 Cnt(OFDM only)] {error, ok}= {%d, %d} (PCR=%d percent)\n",
		  crc2->cnt_ofdm3_crc32_error,
		  crc2->cnt_ofdm3_crc32_ok, pcr);
		break;
	case STATE_DATA:
		BB_DBG(bb, DBG_CMN,
		  "[DATA CRC32 Cnt(OFDM only)] {error, ok}= {%d, %d} (PCR=%d percent)\n",
		  crc2->cnt_ofdm3_crc32_error,
		  crc2->cnt_ofdm3_crc32_ok, pcr);
		break;
	case STATE_NULL:
		BB_DBG(bb, DBG_CMN,
		  "[Null CRC32 Cnt(OFDM only)] {error, ok}= {%d, %d} (PCR=%d percent)\n",
		  crc2->cnt_ofdm3_crc32_error,
		  crc2->cnt_ofdm3_crc32_ok, pcr);
		break;
	case STATE_QOS:
		BB_DBG(bb, DBG_CMN,
		  "[QoS CRC32 Cnt(OFDM only)] {error, ok}= {%d, %d} (PCR=%d percent)\n",
		  crc2->cnt_ofdm3_crc32_error,
		  crc2->cnt_ofdm3_crc32_ok, pcr);
		break;
	default:
		break;
	}
}

void halbb_dig_cmn_log(struct bb_info *bb)
{
	struct bb_dig_cr_info *cr = &bb->bb_dig_i.bb_dig_cr_i;
	u8 i = 0;
	u8 lna = 0, tia = 0, rxbb = 0;
	u8 ofdm_pd_th = 0, ofdm_pd_th_en = 0, cck_pd_th_en = 0;
	u8 rx_num_path = bb->phl_com->phy_cap[bb->bb_phy_idx].rx_path_num;
	s8 cck_pd_th = 0;

	if (bb->bb_watchdog_mode != BB_WATCHDOG_NORMAL)
		return;

	for (i = 0; i < rx_num_path; i++) {
		lna = halbb_get_lna_idx(bb, i);
		tia = halbb_get_tia_idx(bb, i);
		rxbb = halbb_get_rxb_idx(bb, i);
		BB_DBG(bb, DBG_CMN, "[DIG][Path-%d] Get(lna,tia,rxb)=(%d,%d,%d)\n",
		       i, lna, tia, rxbb);
	}

	ofdm_pd_th = (u8)halbb_get_reg_cmn(bb, cr->seg0r_pd_lower_bound_a,
					   cr->seg0r_pd_lower_bound_a_m,
					   bb->bb_phy_idx);
	ofdm_pd_th_en = (u8)halbb_get_reg_cmn(bb, cr->seg0r_pd_spatial_reuse_en_a,
					      cr->seg0r_pd_spatial_reuse_en_a_m,
					      bb->bb_phy_idx);
	cck_pd_th = (s8)halbb_get_reg(bb, cr->rssi_nocca_low_th_a,
				      cr->rssi_nocca_low_th_a_m);
	cck_pd_th_en = (u8)halbb_get_reg(bb, cr->cca_rssi_lmt_en_a,
					 cr->cca_rssi_lmt_en_a_m);

	if ((bb->ic_type == BB_RTL8852A && bb->hal_com->cv < CCV) ||
	    (bb->ic_type == BB_RTL8852B && bb->hal_com->cv < CBV))
		BB_DBG(bb, DBG_CMN, "PD_low_bd_en(ofdm) : (%d), PD_low_bd(ofdm) = (-%d) dBm\n",
		       ofdm_pd_th_en, 102 - (ofdm_pd_th << 1));
	else
		BB_DBG(bb, DBG_CMN, "PD_low_bd_en(ofdm, cck) : (%d, %d), PD_low_bd(ofdm, cck) = (-%d, %d) dBm\n",
		       ofdm_pd_th_en, cck_pd_th_en, 102 - (ofdm_pd_th << 1),
		       cck_pd_th);
}

u16 halbb_rx_utility(struct bb_info *bb, u16 avg_phy_rate, u8 rx_max_ss,
		     enum channel_width bw)
{
	struct bb_cmn_rpt_info	*cmn_rpt = &bb->bb_cmn_rpt_i;
	struct bb_pkt_cnt_su_info *pkt_cnt = &cmn_rpt->bb_pkt_cnt_su_i;
	u16 utility_primitive = 0, utility = 0;

	if (pkt_cnt->he_pkt_not_zero) {
	/*@ HE 1SS MCS11[3.2] 20M: tp = 122, 1000/122 = 8.2, 122*8.25 = 1006.5*/
		utility_primitive = avg_phy_rate * 8 + (avg_phy_rate >> 2);
	} else if (pkt_cnt->vht_pkt_not_zero) {
	/*@ VHT 1SS MCS9(fake) 20M: tp = 87, 1000/87 = 11.49, 87*11.5 = 1000.5*/
		utility_primitive = avg_phy_rate * 11 + (avg_phy_rate >> 1);
	} else if (pkt_cnt->ht_pkt_not_zero) {
	/*@ MCS7 20M: tp = 65, 1000/65 = 15.38, 65*15.5 = 1007*/
		utility_primitive = avg_phy_rate * 15 + (avg_phy_rate >> 1);
	} else {
	/*@ 54M, 1000/54 = 18.5, 54*18.5 = 999*/
		utility_primitive = avg_phy_rate * 18 + (avg_phy_rate >> 1);
	}

	utility = (utility_primitive / rx_max_ss) >> bw;

	if (utility > 1000)
		utility = 1000;

	return utility;
}

u16 halbb_rx_avg_phy_rate(struct bb_info *bb)
{	
	struct bb_cmn_rpt_info	*cmn_rpt = &bb->bb_cmn_rpt_i;
	struct bb_pkt_cnt_su_info *pkt_cnt = &cmn_rpt->bb_pkt_cnt_su_i;
	u16 i = 0;
	u8 base = LEGACY_RATE_NUM;
	u16 rate = 0;
	u32 pkt_cnt_tmp = 0, phy_rate_sum = 0;
	enum channel_width bw = bb->hal_com->band[0].cur_chandef.bw;

	//BB_DBG(bb, DBG_CMN, "bw=%d\n", bb->hal_com->band[0].cur_chandef.bw);

	if (pkt_cnt->he_pkt_not_zero) {
	/*HE Mode*/
		for (i = 0; i < HE_RATE_NUM; i++) {
			if (pkt_cnt->pkt_cnt_he[i] == 0)
				continue;
			rate = VHT_2_HE32_RATE(bb_phy_rate_table[i + base] << bw);
			phy_rate_sum += pkt_cnt->pkt_cnt_he[i] * rate;
			pkt_cnt_tmp += pkt_cnt->pkt_cnt_he[i];

			//BB_DBG(bb, DBG_CMN, "HE  sum:%d +={%d * %d} idx=%d cnt=%d\n", phy_rate_sum, pkt_cnt->pkt_cnt_he[i], rate, i + base, pkt_cnt_tmp);
		}
	} else if (pkt_cnt->vht_pkt_not_zero) {
	/*VHT Mode*/
		for (i = 0; i < VHT_RATE_NUM; i++) {
			if (pkt_cnt->pkt_cnt_vht[i] == 0)
				continue;
			rate = bb_phy_rate_table[i + base] << bw;
			phy_rate_sum += pkt_cnt->pkt_cnt_vht[i] * rate;
			pkt_cnt_tmp += pkt_cnt->pkt_cnt_vht[i];

			//BB_DBG(bb, DBG_CMN, "VHT  sum:%d +={%d * %d} idx=%d cnt=%d\n", phy_rate_sum, pkt_cnt->pkt_cnt_vht[i], rate, i + base, pkt_cnt_tmp);
		}
		
	} else if (pkt_cnt->ht_pkt_not_zero) {
	/*HT Mode*/
		for (i = 0; i < HT_RATE_NUM; i++) {
			if (pkt_cnt->pkt_cnt_ht[i] == 0)
				continue;
			rate = bb_phy_rate_table[i + base] << bw;
			phy_rate_sum += pkt_cnt->pkt_cnt_ht[i] * rate;
			pkt_cnt_tmp += pkt_cnt->pkt_cnt_ht[i];

			//BB_DBG(bb, DBG_CMN, "HT  sum:%d +={%d * %d} idx=%d cnt=%d\n", phy_rate_sum, pkt_cnt->pkt_cnt_ht[i], rate, i + base, pkt_cnt_tmp);
		}

	} else {
	/*Legacy mode*/
		for (i = BB_01M; i <= BB_54M; i++) {
			/*SKIP beacon*/
			if (i == cmn_rpt->bb_pkt_cnt_bcn_i.beacon_phy_rate)
				continue;

			if (pkt_cnt->pkt_cnt_legacy[i] == 0)
				continue;

			rate = bb_phy_rate_table[i];
			phy_rate_sum += pkt_cnt->pkt_cnt_legacy[i] * rate;
			pkt_cnt_tmp += pkt_cnt->pkt_cnt_legacy[i];

			//BB_DBG(bb, DBG_CMN, "LAG  sum:%d +={%d * %d} idx=%d cnt=%d\n", phy_rate_sum, pkt_cnt->pkt_cnt_legacy[i], rate, i + base, pkt_cnt_tmp);
		}
	}

	/*SC Data*/
	if (pkt_cnt->sc40_occur) {
		for (i = 0; i < LOW_BW_RATE_NUM; i++) {
			if (pkt_cnt->pkt_cnt_sc40[i] == 0)
				continue;
			rate = bb_phy_rate_table[i + base] << CHANNEL_WIDTH_40;
			phy_rate_sum += pkt_cnt->pkt_cnt_sc40[i] * rate;
			pkt_cnt_tmp += pkt_cnt->pkt_cnt_sc40[i];
		}
	}

	if (pkt_cnt->sc20_occur) {
		for (i = 0; i < LOW_BW_RATE_NUM; i++) {
			if (pkt_cnt->pkt_cnt_sc20[i] == 0)
				continue;
			rate = bb_phy_rate_table[i + base];
			phy_rate_sum += pkt_cnt->pkt_cnt_sc20[i] * rate;
			pkt_cnt_tmp += pkt_cnt->pkt_cnt_sc20[i];
		}
	}
	//BB_DBG(bb, DBG_CMN, "sum=%d, cnt=%d\n", phy_rate_sum, pkt_cnt_tmp);
	return (u16)HALBB_DIV(phy_rate_sum, pkt_cnt_tmp); /*avg_phy_rate*/
}

void halbb_get_tx_dbg_reg(struct bb_info *bb)
{
	struct bb_dbg_cr_info *cr = &bb->bb_dbg_i.bb_dbg_cr_i;
	struct bb_stat_cr_info *cr2 = &bb->bb_stat_i.bb_stat_cr_i;
	struct bb_dbg_info *dbg = &bb->bb_dbg_i;
	struct bb_tx_info *txdbg = &dbg->tx_info_i;
	u32 sig_a[2];
	u32 n_dbps = 0, n_dbps_last_init = 0;
	u16 vht_n_sd[4] = {52, 108, 234, 468};
	u16 he_n_sd[4] = {234, 468, 980, 1960};
	u16 he_n_sd_short[4] = {60, 120, 240, 492};
	u8 r_n_bpscs_12[14] = {6, 12, 18, 24, 36, 48, 54, 60, 72, 80, 90, 100, 108, 120};
	u16 n_sym_init = 0;
	u8 nss = 0, n_tail = 0, m_stbc = 0, a = 0, a_init = 0, i = 0, j = 0;
	bool ldpc_extra = false;
	u8 num_txinfo_dw = 4;

#ifdef HALBB_DBCC_SUPPORT
	if (bb->bb_phy_idx == HW_PHY_0)
		halbb_set_reg(bb, cr->mac_phy_intf_sel_phy1,
			      cr->mac_phy_intf_sel_phy1_m, false);
	else
		halbb_set_reg(bb, cr->mac_phy_intf_sel_phy1,
			      cr->mac_phy_intf_sel_phy1_m, true);
#endif

	if (bb->ic_type & BB_IC_BE_SERIES)
		num_txinfo_dw = 6;

	for (i = 0; i < num_txinfo_dw; i++)
		txdbg->txinfo[i] = halbb_get_reg(bb, cr->mac_phy_txinfo[i],
						 MASKDWORD);
	if (bb->ic_type & BB_IC_BE_SERIES) {
		for (i = 0; i < 2; i++)
			txdbg->txt2rct[i] = halbb_get_reg(bb,cr->mac_phy_txt2rct[i],
							MASKDWORD);
	}
	for (i = 0; i < 2; i++)
		txdbg->txcomct[i] = halbb_get_reg(bb, cr->mac_phy_txcomct[i],
						  MASKDWORD);
	for (i = 0; i < 4; i++) {
		for (j = 0; j < 2; j++)
			txdbg->txusrct[i][j] = halbb_get_reg(bb,
							     cr->mac_phy_txusrct[i][j],
							     MASKDWORD);
	}
	txdbg->txtimct = halbb_get_reg(bb, cr->mac_phy_txtimct, MASKDWORD);
	txdbg->l_sig = halbb_get_reg(bb, cr->mac_phy_lsig, MASKDWORD);
	sig_a[0] = halbb_get_reg(bb, cr->mac_phy_siga_0, MASKDWORD);
	sig_a[1] = halbb_get_reg(bb, cr->mac_phy_siga_1, MASKDWORD);
	txdbg->sig_b = halbb_get_reg(bb, cr->mac_phy_vht_sigb_0, MASKDWORD);
	if (bb->ic_type & BB_IC_BE_SERIES) {
		txdbg->usig_1 = halbb_get_reg(bb, cr->mac_phy_usig_1, MASKDWORD);
		txdbg->usig_2 = halbb_get_reg(bb, cr->mac_phy_usig_2, MASKDWORD);
	}
	txdbg->txpw_path0 = (u16)halbb_get_reg(bb, cr->path_0_txpw,
					       cr->path_0_txpw_m);
	if (bb->num_rf_path >= 2)
		txdbg->txpw_path1 = (u16)halbb_get_reg(bb, cr->path_1_txpw,
						       cr->path_1_txpw_m);
	if ((bb->ic_type == BB_RTL8852A) || (bb->ic_type == BB_RTL8852B) ||
	    (bb->ic_type == BB_RTL8851B)) {
		/* select cck dbg port to tx*/
		halbb_set_reg(bb, cr2->r1b_rr_sel, cr2->r1b_rr_sel_m, 0);
	}
	/*b mode tx*/
	txdbg->bmode = halbb_get_reg(bb, cr->bmode_tx, MASKDWORD);
	txdbg->bmode_length = (u16)(txdbg->bmode & MASKLWORD);
	txdbg->bmode_service = (u8)((txdbg->bmode & 0xff0000) >> 16);
	txdbg->bmode_rate = (u8)((txdbg->bmode & 0xf000000) >> 24);
	txdbg->bmode_type = (bool)((txdbg->bmode & BIT(31)) >> 31);

	/*mac/phy interface*/
	/*txinfo*/
	if (bb->ic_type & BB_IC_AX_SERIES) {
		txdbg->type = (u8)(txdbg->txinfo[0] & 0xf);
		txdbg->tx_path_en = (u8)((txdbg->txinfo[0] & 0xF000) >> 12);
		txdbg->path_map = (u8)((txdbg->txinfo[0] & 0xff0000) >> 16);
		txdbg->txcmd_num = (u8)((txdbg->txinfo[0] & 0x3f000000) >> 24);
		txdbg->txsc = (u8)((txdbg->txinfo[1] & 0xf0) >> 4);
		txdbg->bw = (u8)((txdbg->txinfo[1] & 0x30000) >> 16);
		txdbg->tx_pw = (u16)((txdbg->txinfo[1] & 0x7FC0000) >> 18);
		txdbg->n_usr = (u8)((txdbg->txinfo[2] & 0xff0) >> 4);
		if ((bb->ic_type != BB_RTL8852A) && (bb->ic_type != BB_RTL8852B) &&
		(bb->ic_type != BB_RTL8851B))
			txdbg->max_mcs = (u8)((txdbg->txinfo[3] & 0xf0) >> 4);
	} else {
		txdbg->type = (u8)(txdbg->txinfo[0] & 0xf);
		txdbg->ppdu_var = (u8)((txdbg->txinfo[0] & 0xf0) >> 4);
		txdbg->txcmd_num = (u8)((txdbg->txinfo[0] & 0x3F00) >> 8);
		txdbg->txsc = (u8)((txdbg->txinfo[0] & 0xF000000) >> 24);
		txdbg->bw = (u8)((txdbg->txinfo[1] & 0x70) >> 4);
		txdbg->tx_pw = (u16)((txdbg->txinfo[1] & 0x1FF00) >> 8);
		txdbg->n_usr = (u8)((txdbg->txinfo[1] & 0x3F000000) >> 24);
		txdbg->tx_path_en = (u8)((txdbg->txinfo[2] & 0xF0) >> 4);
		txdbg->path_map = (u8)((txdbg->txinfo[2] & 0xFF00) >> 8);
		txdbg->max_mcs = (u8)(txdbg->txinfo[4] & 0xf);
	}
	/*txcomct*/
	txdbg->stbc = (bool)(txdbg->txcomct[0] & 0x1);
	txdbg->gi = (u8)((txdbg->txcomct[0] & 0x30) >> 4);
	txdbg->ltf = (u8)((txdbg->txcomct[0] & 0xC0) >> 6);
	/*txusrct*/
	for (i = 0; i < txdbg->n_usr; i++) {
		txdbg->u_id[i] = (u8)(txdbg->txusrct[i][0] & 0xff);
		txdbg->n_sts[i] = (u8)((txdbg->txusrct[i][0] & 0x7000000) >> 24);
		txdbg->fec[i] = (bool)((txdbg->txusrct[i][0] & 0x08000000) >> 27);
		txdbg->mcs[i] = (u8)((u8)((txdbg->txusrct[i][0] & 0xf0000000) >> 28) +
				      (u8)((txdbg->txusrct[i][1] & 0x3) << 4));
		txdbg->dcm[i] = (bool)((txdbg->txusrct[i][1] & 0x4) >> 2);
		if ((bb->ic_type == BB_RTL8852A) || (bb->ic_type == BB_RTL8852B) ||
		    (bb->ic_type == BB_RTL8851B))
			txdbg->precoding[i] = (u8)((txdbg->txusrct[i][1] & 0x4000) >> 14);
		else
			txdbg->precoding[i] = (u8)((txdbg->txusrct[i][0] & 0x00180000) >> 19);
	}
	/*txtimct*/
	txdbg->n_sym = (u16)(txdbg->txtimct & 0x7ff); /*excluding disambiguation but including ldpc_extra*/
	ldpc_extra = (bool)((txdbg->txtimct & 0x01000000) >> 24);
	txdbg->pkt_ext = (u8)((txdbg->txtimct & 0x0E000000) >> 25);
	txdbg->pre_fec = (u8)((txdbg->txtimct & 0x30000000) >> 28);
	/*sig/length/t_data*/
	if (txdbg->type > 6) { /* === HE/EHT === */
		txdbg->sig_a1 = sig_a[0] & 0x3ffffff;
		txdbg->sig_a2 = ((sig_a[1] & 0xfffff) << 6) |
				((sig_a[0] & 0xfc000000) >> 26);
		/*SU only*/
		if (((txdbg->type == 7) || ((txdbg->type == 11) && (txdbg->ppdu_var == 0))) &&
		     (txdbg->n_sym > 0)) {
			/*t_data = n_sym * t_sym = 4 * ceil(n_sym * t_syms / 4)*/
			if (txdbg->gi == 1) /*13.6us*/
				txdbg->t_data = (u16)(4 * HALBB_CEIL(txdbg->n_sym * 34, 10));
			else if (txdbg->gi == 2) /*14.4us*/
				txdbg->t_data = (u16)(4 * HALBB_CEIL(txdbg->n_sym * 36, 10));
			else if (txdbg->gi == 3) /*16us*/
				txdbg->t_data = (u16)(16 * txdbg->n_sym);
			else /*no gi = 0.4 us in he mode*/
				txdbg->t_data = 0;

			/*psdu_length = floor(((n_sym - m_stbc) * n_dbps + m_stbc * n_dbps_last - 16 - n_tail) / 8)*/
			if (txdbg->mcs[0] > 13) { /*Avoid n_dbps error*/
				txdbg->psdu_length = 0;
			} else {
				nss = txdbg->stbc ? txdbg->n_sts[0] : txdbg->n_sts[0] + 1;
				n_dbps = (u32)((nss * r_n_bpscs_12[txdbg->mcs[0]] *
					he_n_sd[txdbg->bw] / 12) >> txdbg->dcm[0]);
				n_tail = txdbg->fec[0] ? 0 : 6;
				m_stbc = txdbg->stbc + 1;
				a = (txdbg->pre_fec == 0) ? 4 : txdbg->pre_fec;
				if (txdbg->fec[0] && ldpc_extra && (a == 1)) {
					a_init = 4;
					n_sym_init = txdbg->n_sym - m_stbc;
				} else if (txdbg->fec[0] && ldpc_extra && (a > 1)) {
					a_init = a - 1;
					n_sym_init = txdbg->n_sym;
				} else {
					a_init = a;
					n_sym_init = txdbg->n_sym;
				}
				n_dbps_last_init = (u32)((a_init == 4) ? n_dbps : a_init *
						 (nss * r_n_bpscs_12[txdbg->mcs[0]] *
						 he_n_sd_short[txdbg->bw] / 12) >> txdbg->dcm[0]);
				txdbg->psdu_length = (u32)(((n_sym_init - m_stbc) * n_dbps +
						     m_stbc * n_dbps_last_init -
						     16 - n_tail) / 8);
			}
		} else {
			txdbg->t_data = 0;
			txdbg->psdu_length = 0;
		}
	} else if (txdbg->type > 4) { // === VHT === //
		txdbg->sig_a1 = sig_a[0] & 0xffffff;
		txdbg->sig_a2 = ((sig_a[1] & 0xffff) << 8) |
				((sig_a[0] & 0xff000000) >> 24);
		if (txdbg->type == 5) { /*SU only temporarily*/
			/*t_data = n_sym * t_sym = 4 * ceil(n_sym * t_syms / 4)*/
			if (txdbg->gi == 0) /*3.6us*/
				txdbg->t_data = (u16)(4 * HALBB_CEIL(txdbg->n_sym * 9, 10));
			else if (txdbg->gi == 1) /*4us*/
				txdbg->t_data = (u16)(4 * txdbg->n_sym);
			else
				txdbg->t_data = 0;
			if (txdbg->bw == 0)
				txdbg->psdu_length = (u32)((txdbg->sig_b & 0x1ffff) << 2);
			else if (txdbg->bw == 1)
				txdbg->psdu_length = (u32)((txdbg->sig_b & 0x7ffff) << 2);
			else
				txdbg->psdu_length = (u32)((txdbg->sig_b & 0x1fffff) << 2);
		} else {
			txdbg->t_data = 0;
			txdbg->psdu_length = 0;
		}
	} else if (txdbg->type > 2) { // === HT === //
		txdbg->sig_a1 = sig_a[0] & 0xffffff;
		txdbg->sig_a2 = ((sig_a[1] & 0xffff) << 8) |
				((sig_a[0] & 0xff000000) >> 24);
		/*t_data = n_sym * t_sym = 4 * ceil(n_sym * t_syms / 4)*/
		if (txdbg->gi == 0) /*3.6us*/
			txdbg->t_data = (u16)(4 * HALBB_CEIL(txdbg->n_sym * 9, 10));
		else if (txdbg->gi == 1) /*4us*/
			txdbg->t_data = (u16)(4 * txdbg->n_sym);
		else
			txdbg->t_data = 0;
		txdbg->psdu_length = (u32)((txdbg->sig_a1 & 0x00FFFF00) >> 8);
	} else if (txdbg->type > 1) { // === OFDM === //
		/*t_data = n_sym * t_sym = 4 * n_sym*/
		txdbg->t_data = (u16)(4 * txdbg->n_sym);
		txdbg->psdu_length = (u32)((txdbg->l_sig & 0x0001FFE0) >> 5);
		/*not support*/
		txdbg->sig_a1 = 0;
		txdbg->sig_a2 = 0;
	} else { // === CCK === //
		/*mac/phy intf report do not support b mode*/
		txdbg->sig_a1 = 0;
		txdbg->sig_a2 = 0;
		txdbg->t_data = 0;
		txdbg->psdu_length = 0;
	}
}

void halbb_basic_dbg_msg_ra_dbg_reg(struct bb_info *bb)
{
	struct rtw_phl_com_t *phl = bb->phl_com;
	struct dev_cap_t *dev = &phl->dev_cap;
	struct bb_dbg_info *dbg = &bb->bb_dbg_i;
	struct bb_ra_dbgreg *dbgreg = &dbg->ra_dbgreg_i;

	if (bb->bb_watchdog_mode != BB_WATCHDOG_NORMAL)
		return;

	BB_DBG(bb, DBG_CMN,
	       "[RA dbgreg]CMAC_tbl DWORD0{macid0,macid1}={0x%x,0x%x}\n",
	       dbgreg->cmac_tbl_id0, dbgreg->cmac_tbl_id1);

	if ((bb->ic_type == BB_RTL8852A) || (bb->ic_type == BB_RTL8852B) ||
	    (bb->ic_type == BB_RTL8851B)) {
		BB_DBG(bb, DBG_CMN, "{PER,RDR,R4}={%d,%d,%d}, try:{PER,RDR,R4}={%d,%d,%d}\n",
		       dbgreg->per, dbgreg->rdr, dbgreg->r4,
		       dbgreg->try_per, dbgreg->try_rdr, dbgreg->try_r4);
	} else {
		BB_DBG(bb, DBG_CMN, "{PER,RDR,R4,cls}={%d,%d,%d,%d}, try:{PER,RDR,R4}={%d,%d,%d}\n",
		       dbgreg->per, dbgreg->rdr, dbgreg->r4, dbgreg->cls,
		       dbgreg->try_per, dbgreg->try_rdr, dbgreg->try_r4);
	}
	BB_DBG(bb, DBG_CMN,
	       "{up_lmt_cnt,PER_ma,VAR}={%d,%d,%d},d_o:{n,p}={%d,%d},TH:{RD,RU}={%d,%d}\n",
	       dbgreg->rate_up_lmt_cnt, dbgreg->per_ma, dbgreg->var,
	       dbgreg->d_o_n, dbgreg->d_o_p, dbgreg->rd_th, dbgreg->ru_th);
	BB_DBG(bb, DBG_CMN,
	       "TxRPT.tot=%d,RAtimer=%d, RAreturn:{tot=0/disra/trying, R4}={%d,%d}\n",
	       dbgreg->txrpt_tot, dbgreg->ra_timer,
	       dbgreg->tot_disra_trying_return, dbgreg->r4_return);
	BB_DBG(bb, DBG_CMN,
	       "RA mask[H->L]={0x%x,0x%x}, highest_rate=0x%x, lowest_rate=0x%x\n",
	       dbgreg->ra_mask_h, dbgreg->ra_mask_l,
	       dbgreg->highest_rate, dbgreg->lowest_rate);
	if ((bb->ic_type == BB_RTL8852A) || (bb->ic_type == BB_RTL8852B)) {
		BB_DBG(bb, DBG_CMN,
		       "dyn_stbc:{en,ok,banned,fail_cnt}={%d,%d,%d,%d}\n",
		       (dbgreg->dyn_stbc >> 7) & 0x1,
		       (dbgreg->dyn_stbc >> 6) & 0x1,
		       (dbgreg->dyn_stbc >> 5) & 0x1,
		       dbgreg->dyn_stbc & 0x1f);
	}

	/*MU MIMO RA*/
	if ((dev->rfe_type >= 50) && (bb->ic_type != BB_RTL8852A)) {
		BB_DBG(bb, DBG_CMN, "==== MU MIMO RA ====\n");
		BB_DBG(bb, DBG_CMN,
		       "sta_1 (su macid 1) : mu_mcs=%d, su_mcs=%d, mu_id=%d, lowest_rate=0x%x\n",
		       dbgreg->mu_mcs & MASKBYTE0,
		       (dbgreg->mu_mcs & MASKBYTE1) >> 8,
		       dbgreg->mu_id_lowest_rate & MASKBYTE0,
		       (dbgreg->mu_id_lowest_rate & MASKBYTE1) >> 8);
		BB_DBG(bb, DBG_CMN,
		       "mu_rd_th=%d, mu_ru_th=%d, mu_per=%d, mu_try_per=%d\n",
		       (dbgreg->mu_rd_ru_th & MASKBYTE1) >> 8,
		       dbgreg->mu_rd_ru_th & MASKBYTE0,
		       dbgreg->mu_per & MASKBYTE0,
		       (dbgreg->mu_per & MASKBYTE1) >> 8);
		BB_DBG(bb, DBG_CMN,
		       "sta_2 (non su macid 1) : mu_mcs=%d, su_mcs=%d, mu_id=%d, lowest_rate=0x%x\n",
		       (dbgreg->mu_mcs & MASKBYTE2) >> 16,
		       (dbgreg->mu_mcs & MASKBYTE3) >> 24,
		       (dbgreg->mu_id_lowest_rate & MASKBYTE2) >> 16,
		       (dbgreg->mu_id_lowest_rate & MASKBYTE3) >> 24);
		BB_DBG(bb, DBG_CMN,
		       "mu_rd_th=%d, mu_ru_th=%d, mu_per=%d, mu_try_per=%d\n",
		       (dbgreg->mu_rd_ru_th & MASKBYTE3) >> 24,
		       (dbgreg->mu_rd_ru_th & MASKBYTE2) >> 16,
		       (dbgreg->mu_per & MASKBYTE2) >> 16,
		       (dbgreg->mu_per & MASKBYTE3) >> 24);
	}
}

void halbb_mac_phy_intf_txcmd_txtp_7(struct bb_info *bb, u8 txcmd_num, char **txcmd)
{
	/*Move txcmd to array declaration would cause warning due to larger frame size*/
	switch (txcmd_num) {
	case 0:
		*txcmd = "DATA";
		break;
	case 1:
		*txcmd = "BCN";
		break;
	case 2:
		*txcmd = "HT-NDPA";
		break;
	case 3:
		*txcmd = "VHT-NDPA";
		break;
	case 4:
		*txcmd = "HE-NDPA";
		break;
	case 5:
		*txcmd = "EHT-NDPA";
		break;
	case 6:
		*txcmd = "11MC_FTM";
		break;
	case 7:
		*txcmd = "11MC_FTM_ACK";
		break;
	case 8:
		*txcmd = "RTS";
		break;
	case 9:
		*txcmd = "CTS-2-Self";
		break;
	case 10:
		*txcmd = "CF-END";
		break;
	case 11:
		*txcmd = "CMP-BAR";
		break;
	case 12:
		*txcmd = "BFRP";
		break;
	case 13:
		*txcmd = "NDP";
		break;
	case 14:
		*txcmd = "QoS_Null";
		break;
	case 15:
		*txcmd = "CTS-2-MURTS";
		break;
	case 16:
		*txcmd = "ACK";
		break;
	case 17:
		*txcmd = "CTS";
		break;
	case 18:
		*txcmd = "CMP-BA";
		break;
	case 19:
		*txcmd = "MSTA-BA";
		break;
	case 20:
		*txcmd = "HT-CSI";
		break;
	case 21:
		*txcmd = "VHT-CSI";
		break;
	case 22:
		*txcmd = "HE-CSI";
		break;
	case 23:
		*txcmd = "EHT-CSI";
		break;
	case 24:
		*txcmd = "NTB_I2R_NDPA";
		break;
	case 25:
		*txcmd = "NTB_I2R_NDP";
		break;
	case 26:
		*txcmd = "NTB_I2R_LMR";
		break;
	case 27:
		*txcmd = "NTB_I2R_NDP";
		break;
	case 28:
		*txcmd = "NTB_I2R_LMR";
		break;
	case 29:
		*txcmd = "NTB_R2I_RANG_NDPA";
		break;
	case 30:
		*txcmd = "NTB_R2I_NDP";
		break;
	case 31:
		*txcmd = "NTB_R2I_LMR";
		break;
	case 32:
		*txcmd = "TRIG-BASIC";
		break;
	case 33:
		*txcmd = "TRIG-BFRP";
		break;
	case 34:
		*txcmd = "TRIG-MUBAR";
		break;
	case 35:
		*txcmd = "TRIG-MURTS";
		break;
	case 36:
		*txcmd = "TRIG-BSRP";
		break;
	case 37:
		*txcmd = "TRIG-BQRP";
		break;
	case 38:
		*txcmd = "TRIG-NFRP";
		break;
	case 39:
		*txcmd = "TRIG-BASIC-DATA";
		break;
	case 40:
		*txcmd = "TRIG-RANG_POLL";
		break;
	case 41:
		*txcmd = "TRIG-RANG_SNR";
		break;
	case 42:
		*txcmd = "TRIG-RANG_LMR";
		break;
	case 48:
		*txcmd = "TRIG-TB_CSI";
		break;
	case 49:
		*txcmd = "TRIG-TB_CBA";
		break;
	case 50:
		*txcmd = "TRIG-TB_MBA";
		break;
	case 51:
		*txcmd = "TRIG-TB_BSR";
		break;
	case 52:
		*txcmd = "TRIG-TB_BQR";
		break;
	case 53:
		*txcmd = "TRIG-TB_ACK";
		break;
	case 54:
		*txcmd = "TRIG-TB_PPDU";
		break;
	case 55:
		*txcmd = "TRIG-TB_I2R_CTS2S";
		break;
	case 56:
		*txcmd = "TRIG-TB_I2R_NDP";
		break;
	case 57:
		*txcmd = "TRIG-TB_I2R_LMR";
		break;
	default:
		*txcmd = "RSVD";
		break;
	}
}

void halbb_mac_phy_intf_txcmd_txtp(struct bb_info *bb, u8 txcmd_num, char **txcmd)
{
	/*Move txcmd to array declaration would cause warning due to larger frame size*/
	switch (txcmd_num) {
	case 0:
		*txcmd = "data";
		break;
	case 1:
		*txcmd = "beacon";
		break;
	case 2:
		*txcmd = "HT-NDPA";
		break;
	case 3:
		*txcmd = "VHT-NDPA";
		break;
	case 4:
		*txcmd = "HE-NDPA";
		break;
	case 8:
		*txcmd = "RTS";
		break;
	case 9:
		*txcmd = "CTS2self";
		break;
	case 10:
		*txcmd = "CF_end";
		break;
	case 11:
		*txcmd = "compressed-BAR";
		break;
	case 12:
		*txcmd = "BFRP";
		break;
	case 13:
		*txcmd = "NDP";
		break;
	case 14:
		*txcmd = "QoS_Null";
		break;
	case 16:
		*txcmd = "ACK";
		break;
	case 17:
		*txcmd = "CTS";
		break;
	case 18:
		*txcmd = "compressed-BA";
		break;
	case 19:
		*txcmd = "Multi-STA-BA";
		break;
	case 20:
		*txcmd = "HT-CSI";
		break;
	case 21:
		*txcmd = "VHT-CSI";
		break;
	case 22:
		*txcmd = "HE-CSI";
		break;
	case 31:
		*txcmd = "TB_PPDU";
		break;
	case 32:
		*txcmd = "TRIG-BASIC";
		break;
	case 33:
		*txcmd = "TRIG-BFRP";
		break;
	case 34:
		*txcmd = "TRIG-MUBAR";
		break;
	case 35:
		*txcmd = "TRIG-MU-RTS";
		break;
	case 36:
		*txcmd = "TRIG-BSRP";
		break;
	case 37:
		*txcmd = "TRIG-BQRP";
		break;
	case 38:
		*txcmd = "TRIG-NFRP";
		break;
	case 48:
		*txcmd = "TRIG-BASIC-DATA";
		break;
	default:
		*txcmd = "RSVD";
		break;
	}
}

void halbb_mac_phy_intf_ppdu_type(struct bb_info *bb, u8 type, char **ppdu)
{
	switch (type) {
	case 0:
		*ppdu = "L-CCK";
		break;
	case 1:
		*ppdu = "S-CCK";
		break;
	case 2:
		*ppdu = "Legacy";
		break;
	case 3:
		*ppdu = "HT";
		break;
	case 4:
		*ppdu = "HT GF";
		break;
	case 5:
		*ppdu = "VHT SU";
		break;
	case 6:
		*ppdu = "VHT MU";
		break;
	case 7:
		*ppdu = "HE SU";
		break;
	case 8:
		*ppdu = "HE ER SU";
		break;
	case 9:
		*ppdu = "HE MU";
		break;
	case 10:
		*ppdu = "HE TB";
		break;
	default:
		*ppdu = "RSVD";
		break;
	}
}

void halbb_mac_phy_intf_ppdu_var_type_7(struct bb_info *bb, u8 ppdu_type, u8 ppdu_var, char **ppdu)
{
	switch (ppdu_type) {
	case 0:
		*ppdu = "L-CCK";
		break;
	case 1:
		*ppdu = "S-CCK";
		break;
	case 2:
		if (ppdu_var == 0)
			*ppdu = "Legacy";
		else if (ppdu_var == 1)
			*ppdu = "Legacy dup";
		else if (ppdu_var == 2)
			*ppdu = "Legacy dup punc";
		break;
	case 3:
		if (ppdu_var == 0)
			*ppdu = "HT MF";
		else if (ppdu_var == 1)
			*ppdu = "HT SND NDP";
		break;
	case 4:
		*ppdu = "HT GF";
		break;
	case 5:
		if (ppdu_var == 0)
			*ppdu = "VHT SU";
		else if (ppdu_var == 1)
			*ppdu = "VHT SND NDP";
		break;
	case 6:
		*ppdu = "VHT MU";
		break;
	case 7:
		if (ppdu_var == 0)
			*ppdu = "HE SU";
		else if (ppdu_var == 1)
			*ppdu = "HE SND NDP";
		else if (ppdu_var == 2)
			*ppdu = "HE SND NDP punc";
		else if (ppdu_var == 3)
			*ppdu = "HE RANG NDP";
		break;
	case 8:
		*ppdu = "HE ER SU";
		break;
	case 9:
		if (ppdu_var == 0)
			*ppdu = "HE MU RU";
		else if (ppdu_var == 1)
			*ppdu = "HE MU MU";
		else if (ppdu_var == 2)
			*ppdu = "HE MU RU punc";
		break;
	case 10:
		if (ppdu_var == 0)
			*ppdu = "HE TB";
		else if (ppdu_var == 1)
			*ppdu = "HE TB FB NDP";
		else if (ppdu_var == 2)
			*ppdu = "HE MU RANG NDP";
		break;
	case 11:
		if (ppdu_var == 0)
			*ppdu = "EHT MU SU";
		else if (ppdu_var == 1)
			*ppdu = "EHT MU ER";
		else if (ppdu_var == 2)
			*ppdu = "EHT MU RU";
		else if (ppdu_var == 3)
			*ppdu = "EHT MU MU";
		else if (ppdu_var == 4)
			*ppdu = "EHT MU SND NDP";
		else if (ppdu_var == 5)
			*ppdu = "EHT MU SU punc";
		else if (ppdu_var == 6)
			*ppdu = "EHT MU RU punc";
		else if (ppdu_var == 7)
			*ppdu = "EHT SND NDP punc";
		else if (ppdu_var == 8)
			*ppdu = "EHT MU MU punc";
		break;
	case 12:
		*ppdu = "EHT TB";
		break;
	default:
		*ppdu = "RSVD";
		break;
	}
}

void halbb_basic_dbg_msg_tx_dbg_reg(struct bb_info *bb)
{
	struct bb_dbg_info *dbg = &bb->bb_dbg_i;
	struct bb_tx_info *txdbg = &dbg->tx_info_i;
	s32 pw = 0;
	u8 i = 0;
	char gi_type[][4] = {{"0.4"}, {"0.8"}, {"1.6"}, {"3.2"}};
	char fec_type[][5] = {{"BCC"}, {"LDPC"}};
	char precoding_type[][8] = {{"normal"}, {"TxBF"}, {"MU-MIMO"}};
	char b_mode_type[][7] = {{"long"}, {"short"}};
	char *b_mode_rate = NULL;
	char *txcmd[20] = {0};
	char *ppdu[20] = {0};
	char tx_pw0[HALBB_SNPRINT_SIZE] = {0};
	char tx_pw1[HALBB_SNPRINT_SIZE] = {0};
	u8 n_usr = 0;

	if (bb->bb_watchdog_mode != BB_WATCHDOG_NORMAL)
		return;

	/*Store the whole tx_dbg register to avoid timing issue*/
	halbb_get_tx_dbg_reg(bb);

	pw = halbb_cnvrt_2_sign((u32)txdbg->txpw_path0, 9);
	halbb_print_sign_frac_digit(bb, (u32)pw, 9, 2, tx_pw0,
				    HALBB_SNPRINT_SIZE);
	if (bb->num_rf_path >= 2) {
		pw = halbb_cnvrt_2_sign((u32)txdbg->txpw_path1, 9);
		halbb_print_sign_frac_digit(bb, (u32)pw, 9, 2, tx_pw1,
					    HALBB_SNPRINT_SIZE);
	}

	if (bb->ic_type == BB_RTL8852C) {
		BB_DBG(bb, DBG_CMN, "txpw_bb={%s,%s}dBm\n", tx_pw0, tx_pw1);
		return;
	}

	if ((txdbg->tx_path_en == 0) && (txdbg->tx_pw == 0) &&
	    (txdbg->l_sig == 0)) {
		BB_DBG(bb, DBG_CMN,
		       "Txinfo is empty!BB reset has been probably toggled.\n");
		return;
	}

	pw = halbb_cnvrt_2_sign((u32)txdbg->tx_pw, 9);
	halbb_print_sign_frac_digit(bb, (u32)pw, 9, 2, bb->dbg_buf, HALBB_SNPRINT_SIZE);
	if (bb->ic_type & BB_IC_AX_SERIES) {
		halbb_mac_phy_intf_ppdu_type(bb, txdbg->type, ppdu);
		halbb_mac_phy_intf_txcmd_txtp(bb, txdbg->txcmd_num, txcmd);
	} else {
		halbb_mac_phy_intf_ppdu_var_type_7(bb, txdbg->type, txdbg->ppdu_var, ppdu);
		halbb_mac_phy_intf_txcmd_txtp_7(bb, txdbg->txcmd_num, txcmd);
	}

	if (bb->ic_type & BB_IC_AX_SERIES)
		BB_DBG(bb, DBG_CMN,
		       "[%s][%s] BW=%dM/%dM, TxSC=%d, TxPathEn=%d, PathMap=0x%x\n",
		       *ppdu, *txcmd, 20 << txdbg->bw, bb->bb_link_i.bb_bw,
		       txdbg->txsc, txdbg->tx_path_en, txdbg->path_map);
	else
		BB_DBG(bb, DBG_CMN,
		       "[%s][%s] BW=%dM/%dM, TxSB=%d, TxPathEn=%d, PathMap=0x%x\n",
		       *ppdu, *txcmd, 20 << txdbg->bw, bb->bb_link_i.bb_bw,
		       txdbg->txsc, txdbg->tx_path_en, txdbg->path_map);

	if ((bb->ic_type == BB_RTL8852A) || (bb->ic_type == BB_RTL8852B))
		BB_DBG(bb, DBG_CMN,
		       "User_num=%d, TxPw:tmac=%sdBm/bb=(%s,%s)dBm\n",
		       txdbg->n_usr, bb->dbg_buf, tx_pw0, tx_pw1);
	else if (bb->ic_type == BB_RTL8851B)
		BB_DBG(bb, DBG_CMN,
		       "User_num=%d, TxPw:tmac=%sdBm/bb=%sdBm\n",
		       txdbg->n_usr, bb->dbg_buf, tx_pw0);
	else
		BB_DBG(bb, DBG_CMN,
		       "User_num=%d, TxPw:tmac=%sdBm/bb=(%s,%s)dBm, max_mcs=%d\n",
		       txdbg->n_usr, bb->dbg_buf, tx_pw0, tx_pw1,
		       txdbg->max_mcs);

	if (txdbg->type > 6) { /* === HE/EHT === */
		BB_DBG(bb, DBG_CMN, "STBC=%d, GILTF=%dx%s\n",
		       txdbg->stbc, 1 << txdbg->ltf, gi_type[txdbg->gi]);
		for (i = 0; i < txdbg->n_usr; i++) {
			BB_DBG(bb, DBG_CMN,
			       "U_id=%d, FEC=%s, N_sts=%d, MCS=%d, precoding=%s, DCM=%d\n",
			       txdbg->u_id[i], fec_type[txdbg->fec[i]],
			       txdbg->n_sts[i], txdbg->mcs[i],
			       precoding_type[txdbg->precoding[i]],
			       txdbg->dcm[i]);
		}
		/*SU only*/
		if ((txdbg->type == 7) || ((txdbg->type == 11) && (txdbg->ppdu_var == 0)))
			BB_DBG(bb, DBG_CMN,
			       "n_sym=%d, t_data=%d us, PSDU_length=%d Bytes, pre_fec=%d, pkt_ext=%d us\n",
			       txdbg->n_sym, txdbg->t_data, txdbg->psdu_length,
			       txdbg->pre_fec, txdbg->pkt_ext << 2);
		if (txdbg->type > 10) { /* === EHT === */
			BB_DBG(bb, DBG_CMN,
			       "L-SIG/USIG-1/USIG-2={0x%08x, 0x%08x, 0x%08x}\n",
			       txdbg->l_sig, txdbg->usig_1, txdbg->usig_2);
		} else {/* === HE === */
			BB_DBG(bb, DBG_CMN,
			       "L-SIG/HE-SIG-A1/HE-SIG-A2={0x%08x, 0x%08x, 0x%08x}\n",
			       txdbg->l_sig, txdbg->sig_a1, txdbg->sig_a2);
		}
	} else if (txdbg->type > 4) { /* === VHT === */
		BB_DBG(bb, DBG_CMN, "STBC=%d, GI=%s\n",
		       txdbg->stbc, gi_type[txdbg->gi]);
		for (i = 0; i < txdbg->n_usr; i++) {
			BB_DBG(bb, DBG_CMN,
			       "U_id=%d, FEC=%s, N_sts=%d, MCS=%d, precoding=%s\n",
			       txdbg->u_id[i], fec_type[txdbg->fec[i]],
			       txdbg->n_sts[i], txdbg->mcs[i],
			       precoding_type[txdbg->precoding[i]]);
		}
		if (txdbg->type == 5) /*SU only temporarily*/
			BB_DBG(bb, DBG_CMN,
			       "n_sym=%d, t_data=%d us, VHT_length=%d Bytes\n",
			       txdbg->n_sym, txdbg->t_data, txdbg->psdu_length);
		BB_DBG(bb, DBG_CMN,
		       "L-SIG/VHT-SIG-A1/VHT-SIG-A2/VHT-SIG-B={0x%08x, 0x%08x, 0x%08x, 0x%08x}\n",
		       txdbg->l_sig, txdbg->sig_a1, txdbg->sig_a2, txdbg->sig_b);
	} else if (txdbg->type > 2) { /* === HT === */
		BB_DBG(bb, DBG_CMN,
		       "STBC=%d, FEC=%s, GI=%s, N_sts=%d, MCS=%d\n",
		       txdbg->stbc, fec_type[txdbg->fec[0]],
		       gi_type[txdbg->gi], txdbg->n_sts[0], txdbg->mcs[0]);
		BB_DBG(bb, DBG_CMN,
		       "n_sym=%d, t_data=%d us, HT_length=%d Bytes\n",
		       txdbg->n_sym, txdbg->t_data, txdbg->psdu_length);
		BB_DBG(bb, DBG_CMN,
		       "L-SIG/HT-SIG1/HT-SIG2={0x%08x, 0x%08x, 0x%08x}\n",
		       txdbg->l_sig, txdbg->sig_a1, txdbg->sig_a2);
	} else if (txdbg->type > 1) { /* === OFDM === */
		BB_DBG(bb, DBG_CMN, "rate=%dM\n",
		       bb_phy_rate_table[4 + txdbg->mcs[0]]);
		BB_DBG(bb, DBG_CMN,
		       "n_sym=%d, t_data=%d us, L_length=%d Bytes\n",
		       txdbg->n_sym, txdbg->t_data, txdbg->psdu_length);
		BB_DBG(bb, DBG_CMN, "L-SIG={0x%08x}\n", txdbg->l_sig);
	} else { /* === CCK === */
		if (txdbg->bmode_rate == 1)
			b_mode_rate = "1M";
		else if (txdbg->bmode_rate == 2)
			b_mode_rate = "2M";
		else if (txdbg->bmode_rate == 4)
			b_mode_rate = "5.5M";
		else if (txdbg->bmode_rate == 8)
			b_mode_rate = "11M";
		else
			b_mode_rate = "RSVD";

		BB_DBG(bb, DBG_CMN,
		       "Rate=%s, length=%d(us), service=0x%x, preamble=%s\n",
		       b_mode_rate, txdbg->bmode_length,
		       txdbg->bmode_service, b_mode_type[txdbg->bmode_type]);
		BB_DBG(bb, DBG_CMN, "plcp hdr info=0x%08x\n", txdbg->bmode);
	}

	/*raw data*/
	BB_DBG(bb, DBG_CMN, "TxInfo={0x%08x, 0x%08x, 0x%08x, 0x%08x}\n",
	       txdbg->txinfo[0], txdbg->txinfo[1], txdbg->txinfo[2],
	       txdbg->txinfo[3]);
	if (bb->ic_type & BB_IC_BE_SERIES)
		BB_DBG(bb, DBG_CMN,
		       "TxInfo2={0x%08x, 0x%08x}, TxT2rCt={0x%08x, 0x%08x}\n",
		       txdbg->txinfo[4], txdbg->txinfo[5], txdbg->txt2rct[0],
		       txdbg->txt2rct[1]);

	if (txdbg->type > 1) { /* === OFDM === */
		BB_DBG(bb, DBG_CMN,
		       "TxComCt={0x%08x, 0x%08x}, TxTimCt=0x%08x\n",
		       txdbg->txcomct[0], txdbg->txcomct[1],
		       txdbg->txtimct);

		if (txdbg->type > 4) /* VHT/HE/EHT*/
			n_usr = txdbg->n_usr;
		else
			n_usr = 1;

		for (i = 0; i < n_usr; i++) {
			BB_DBG(bb, DBG_CMN,
			       "U_id=%d, TxUsrCt={0x%08x, 0x%08x}\n",
			       txdbg->u_id[i], txdbg->txusrct[i][0],
			       txdbg->txusrct[i][1]);
		}
	}
}

void halbb_basic_dbg_03_msg_pmac(struct bb_info *bb)
{

#ifdef HALBB_STATISTICS_SUPPORT
	struct bb_stat_info *stat = &bb->bb_stat_i;
	struct bb_fa_info *fa = &stat->bb_fa_i;
	struct bb_cck_fa_info *cck_fa = &fa->bb_cck_fa_i;
	struct bb_legacy_fa_info *legacy_fa = &fa->bb_legacy_fa_i;
	struct bb_ht_fa_info *ht_fa = &fa->bb_ht_fa_i;
	struct bb_vht_fa_info *vht_fa = &fa->bb_vht_fa_i;
	struct bb_he_fa_info *he_fa = &fa->bb_he_fa_i;
	struct bb_cca_info *cca = &stat->bb_cca_i;
	struct bb_crc_info *crc = &stat->bb_crc_i;

	if (!(bb->cmn_dbg_msg_component & BB_BASIC_DBG_03_PMAC)) {
		BB_DBG(bb, DBG_CMN, "Disabled\n");
		return;
	}

	if (bb->bb_watchdog_mode != BB_WATCHDOG_NORMAL)
		return;

	BB_DBG(bb, DBG_CMN,
	       "[Tx]{CCK_TxEN, CCK_TxON, OFDM_TxEN, OFDM_TxON}: {%d, %d, %d, %d}\n",
	       stat->bb_tx_cnt_i.cck_mac_txen, stat->bb_tx_cnt_i.cck_phy_txon,
	       stat->bb_tx_cnt_i.ofdm_mac_txen,
	       stat->bb_tx_cnt_i.ofdm_phy_txon);
	BB_DBG(bb, DBG_CMN,
	       "[CRC]{B/G/N/AC/AX/All/MPDU} OK:{%d, %d, %d, %d, %d, %d, %d} Err:{%d, %d, %d, %d, %d, %d, %d}\n",
	       crc->cnt_cck_crc32_ok, crc->cnt_ofdm_crc32_ok,
	       crc->cnt_ht_crc32_ok, crc->cnt_vht_crc32_ok,
	       crc->cnt_he_crc32_ok, crc->cnt_crc32_ok_all,
	       crc->cnt_ampdu_crc_ok, crc->cnt_cck_crc32_error,
	       crc->cnt_ofdm_crc32_error, crc->cnt_ht_crc32_error,
	       crc->cnt_vht_crc32_error, crc->cnt_he_crc32_error,
	       crc->cnt_crc32_error_all, crc->cnt_ampdu_crc_error);
	BB_DBG(bb, DBG_CMN,
	       "[CCA]{CCK, OFDM, All}: %d, %d, %d\n",
	       cca->cnt_cck_cca, cca->cnt_ofdm_cca, cca->cnt_cca_all);
	BB_DBG(bb, DBG_CMN,
	       "[FA]{CCK, OFDM, All}: %d, %d, %d\n",
	       fa->cnt_cck_fail, fa->cnt_ofdm_fail, fa->cnt_fail_all);
	BB_DBG(bb, DBG_CMN,
	       " *[CCK]sfd/sig_GG=%d/%d, *[OFDM]Prty=%d, Rate=%d, LSIG_brk_s/l=%d/%d, SBD=%d\n",
	       cck_fa->sfd_gg_cnt, cck_fa->sig_gg_cnt,
	       legacy_fa->cnt_parity_fail, legacy_fa->cnt_rate_illegal,
	       legacy_fa->cnt_lsig_brk_s_th, legacy_fa->cnt_lsig_brk_l_th,
	       legacy_fa->cnt_sb_search_fail);
	BB_DBG(bb, DBG_CMN,
	       " *[HT]CRC8=%d, MCS=%d, *[VHT]SIGA_CRC8=%d, MCS=%d\n",
	       ht_fa->cnt_crc8_fail, ht_fa->cnt_mcs_fail,
	       vht_fa->cnt_crc8_fail_vhta, vht_fa->cnt_mcs_fail_vht);
	BB_DBG(bb, DBG_CMN,
	       " *[HE]SIGA_CRC4{SU/ERSU/MU}=%d/%d/%d, SIGB_CRC4{ch1/ch2}=%d/%d, MCS{nrml/bcc/dcm}=%d/%d/%d\n",
	       he_fa->cnt_crc4_fail_hea_su, he_fa->cnt_crc4_fail_hea_ersu,
	       he_fa->cnt_crc4_fail_hea_mu, he_fa->cnt_crc4_fail_heb_ch1_mu,
	       he_fa->cnt_crc4_fail_heb_ch2_mu, he_fa->cnt_mcs_fail_he,
	       he_fa->cnt_mcs_fail_he_bcc, he_fa->cnt_mcs_fail_he_dcm);

	halbb_crc32_cnt2_cmn_log(bb);
	halbb_crc32_cnt3_cmn_log(bb);
#endif
}

void halbb_basic_dbg_05_rx(struct bb_info *bb)
{
	struct bb_ch_info *ch = &bb->bb_ch_i;
	struct bb_cmn_rpt_info	*cmn_rpt = &bb->bb_cmn_rpt_i;
	struct bb_pkt_cnt_cap_info *pkt_cnt_cap = &cmn_rpt->bb_pkt_cnt_all_i;
	struct bb_physts_pop_info *pop_info = &cmn_rpt->bb_physts_pop_i;
	struct bb_dbg_cr_info *cr = &bb->bb_dbg_i.bb_dbg_cr_i;
	struct bb_physts_info *physts = &bb->bb_physts_i;
	u8 tmp = 0;
	u32 bb_monitor1 = 0;

	if (!(bb->cmn_dbg_msg_component & BB_BASIC_DBG_05_RX)) {
		BB_DBG(bb, DBG_CMN, "Disabled\n");
		return;
	}

	if (bb->bb_watchdog_mode != BB_WATCHDOG_NORMAL)
		return;	

	BB_DBG(bb, DBG_CMN, "rxsc_idx {Lgcy, 20, 40, 80} = {%d, %d, %d, %d}\n",
	       ch->rxsc_l, ch->rxsc_20, ch->rxsc_40, ch->rxsc_80);
	BB_DBG(bb, DBG_CMN, "RX Pkt Cnt: LDPC=(%d), BCC=(%d), STBC=(%d), SU_non_BF=(%d), SU_BF=(%d), MU=(%d), \n",
	       pkt_cnt_cap->pkt_cnt_ldpc, pkt_cnt_cap->pkt_cnt_bcc,
	       pkt_cnt_cap->pkt_cnt_stbc, pkt_cnt_cap->pkt_cnt_su_non_bf,
	       pkt_cnt_cap->pkt_cnt_subf, pkt_cnt_cap->pkt_cnt_mu);
	BB_DBG(bb, DBG_CMN, "Dly_sprd=(%d)\n", tmp);
	BB_DBG(bb, DBG_CMN,
	       "[POP] cnt=%d, hist_cck/ofdm[0:3]={%d | %d, %d, %d}/{%d | %d, %d, %d}\n",
	       bb->bb_stat_i.bb_cca_i.pop_cnt,
	       pop_info->pop_hist_cck[0], pop_info->pop_hist_cck[1],
	       pop_info->pop_hist_cck[2], pop_info->pop_hist_cck[3],
	       pop_info->pop_hist_ofdm[0], pop_info->pop_hist_ofdm[1],
	       pop_info->pop_hist_ofdm[2], pop_info->pop_hist_ofdm[3]);

	halbb_set_reg(bb, cr->bb_monitor_sel1, cr->bb_monitor_sel1_m, 1);
	bb_monitor1 = halbb_get_reg(bb, cr->bb_monitor1, cr->bb_monitor1_m);
	BB_DBG(bb, DBG_CMN,
	       "BB monitor1 = (0x%x), bt_rx_during_cca=(%d), bt_tx_during_cca=(%d), bt_polluted_bcn_cnt=%d\n",
	       bb_monitor1, physts->bb_physts_cnt_i.bt_rx_during_cca_cnt,
	       physts->bb_physts_cnt_i.bt_tx_during_cca_cnt,
	       physts->bb_physts_cnt_i.bt_polluted_bcn_cnt);
}

void halbb_basic_dbg_msg_tx_info(struct bb_info *bb)
{
	struct bb_ch_info *ch = &bb->bb_ch_i;
	struct rtw_phl_stainfo_t *sta;
	struct rtw_ra_sta_info *ra;
	struct rtw_rate_info *rate_info;
	//char dbg_buf[HALBB_SNPRINT_SIZE] = {0};
	u16 sta_cnt = 0;
	u16 i = 0;
	u8 tmp = 0;
	u16 curr_tx_rt = 0;
	enum rtw_gi_ltf curr_gi_ltf = RTW_GILTF_LGI_4XHE32;
	enum hal_rate_bw curr_bw = HAL_RATE_BW_20;
	#if 0 /*wait for phl mu ra declaration*/
	struct rtw_phl_com_t *phl = bb->phl_com;
	struct dev_cap_t *dev = &phl->dev_cap;
	u8 j = 0;
	u8 u1_muidx = 0;
	#endif

	for (i = 0; i < PHL_MAX_STA_NUM; i++) {
		if (!bb->sta_exist[i])
			continue;
		sta = bb->phl_sta_info[i];
		if (!is_sta_active(sta))
			continue;

		ra = &sta->hal_sta->ra_info;

		if (bb->bb_cmn_hooker->bb_ra_drv_i.is_fw_fix_rate[i])
			rate_info = &ra->fixed_rt_i;
		else
			rate_info = &ra->rpt_rt_i;

		if (bb->ic_type & BB_IC_AX_SERIES)
			curr_tx_rt = (u16)(rate_info->mcs_ss_idx & 0x7f) | ((u16)(rate_info->mode & 0x3) << 7);
		else
			curr_tx_rt = (u16)(rate_info->mcs_ss_idx & 0xff) | ((u16)(rate_info->mode & 0xf) << 8);

		curr_gi_ltf = rate_info->gi_ltf;
		curr_bw = rate_info->bw;
		halbb_print_rate_2_buff(bb, curr_tx_rt, curr_gi_ltf, bb->dbg_buf, HALBB_SNPRINT_SIZE);
		BB_DBG(bb, DBG_CMN,
		       "TxRate[%d]=%s (0x%x-%d), PER=(%d), TXBW=(%d/%d), is_fixed(%d)\n",
		       i, bb->dbg_buf, curr_tx_rt, curr_gi_ltf,
		       ra->curr_retry_ratio, (20 << curr_bw),
		       bb->bb_link_i.bb_bw,
		       bb->bb_cmn_hooker->bb_ra_drv_i.is_fw_fix_rate[i]);

		#if 0 /*wait for phl mu ra declaration*/
		if (dev->rfe_type >= 50) {
			for (j = 0; j < MAX_MU_STA_NUM - 1; j++) {
				if (ra->mu_active[j] == false)
					continue;

				curr_tx_rt = (u16)(ra->rpt_rt_i.mu_mcs_ss_idx[j]) | ((u16)(ra->rpt_rt_i.mu_mode[j]) << 7);
				curr_gi_ltf = ra->rpt_rt_i.mu_gi_ltf[j];
				u1_muidx = (j >= ra->mu_idx) ? j + 1 : j;

				halbb_print_rate_2_buff(bb, curr_tx_rt, curr_gi_ltf, bb->dbg_buf, HALBB_SNPRINT_SIZE);
				BB_DBG(bb, DBG_CMN,
				       "MU : muidx{u0,u1} = {%d,%d}, Tx_Rate=%s (0x%x-%d), PER=(%d), TXBW=(%d)\n",
				       ra->mu_idx, u1_muidx, bb->dbg_buf,
				       curr_tx_rt, curr_gi_ltf,
				       ra->mu_curr_retry_ratio[j],
				       (20 << ra->rpt_rt_i.mu_bw[j]));
			}
		}
		#endif

		sta_cnt++;

		if (sta_cnt == 1)
			bb->bb_link_i.tx_rate = curr_tx_rt; /*store for debug*/

		if (sta_cnt >= bb->hal_com->assoc_sta_cnt)
			break;
	}
	//BB_DBG(bb, DBG_CMN, "TSSI val=(%d)\n", tmp);
	//BB_DBG(bb, DBG_CMN, "EDCA val=(%d)\n", tmp);
	halbb_get_ra_dbgreg(bb);
	halbb_basic_dbg_msg_ra_dbg_reg(bb);
}

void halbb_basic_dbg_08_rssi_rate_mu(struct bb_info *bb)
{
	struct bb_ch_info *ch = &bb->bb_ch_i;
	struct bb_link_info *link = &bb->bb_link_i;

	if (!(bb->cmn_dbg_msg_component & BB_BASIC_DBG_08_RSSI_RATE_MU)) {
		BB_DBG(bb, DBG_CMN, "Disabled\n");
		return;
	}

	if (bb->bb_cmn_rpt_i.bb_pkt_cnt_mu_i.pkt_cnt_all == 0) {
	    BB_DBG(bb, DBG_CMN, "NO MU pkt\n");
	    return;
	}

	/*RX Rate*/
	halbb_print_rate_2_buff(bb, link->rx_rate_plurality_mu,
				RTW_GILTF_LGI_4XHE32, bb->dbg_buf, 32);

	BB_DBG(bb, DBG_CMN, "Plurality_RxRate:%s (0x%x)\n",
	       bb->dbg_buf, link->rx_rate_plurality);

	/*RX Rate Distribution & RSSI*/
	halbb_show_rssi_and_rate_distribution_mu(bb);
}

void halbb_basic_dbg_06_rssi_rate(struct bb_info *bb)
{
	struct bb_ch_info *ch = &bb->bb_ch_i;
	struct bb_link_info *link = &bb->bb_link_i;
	struct bb_cmn_rpt_info	*cmn_rpt = &bb->bb_cmn_rpt_i;
	//struct rtw_phl_stainfo_t *sta;
	//char dbg_buf[HALBB_SNPRINT_SIZE] = {0};
	char dbg_buf2[32] = {0};

	if (!(bb->cmn_dbg_msg_component & BB_BASIC_DBG_06_RSSI_RATE)) {
		BB_DBG(bb, DBG_CMN, "Disabled\n");
		return;
	}

	/*RX Rate*/
	halbb_print_rate_2_buff(bb, link->rx_rate_plurality,
				RTW_GILTF_LGI_4XHE32, dbg_buf2, 32);

	halbb_print_rate_2_buff(bb, cmn_rpt->bb_pkt_cnt_bcn_i.beacon_phy_rate,
				RTW_GILTF_LGI_4XHE32, bb->dbg_buf, HALBB_SNPRINT_SIZE);
	BB_DBG(bb, DBG_CMN, "Plurality_RxRate:%s (0x%x), Bcn_Rate=%s (0x%x), Bcn_cnt=%d\n",
	       dbg_buf2, link->rx_rate_plurality,
	       bb->dbg_buf ,cmn_rpt->bb_pkt_cnt_bcn_i.beacon_phy_rate,
	       cmn_rpt->bb_pkt_cnt_bcn_i.pkt_cnt_beacon);

	/*RX Rate Distribution & RSSI*/
	halbb_show_rssi_and_rate_distribution_su(bb);

	/*RX Utility*/
	bb->bb_link_i.avg_phy_rate = halbb_rx_avg_phy_rate(bb);
	bb->bb_link_i.rx_utility = halbb_rx_utility(bb, bb->bb_link_i.avg_phy_rate, bb->num_rf_path, bb->hal_com->band[0].cur_chandef.bw);

	BB_DBG(bb, DBG_CMN, "Avg_rx_rate = %d, rx_utility=( %d / 1000 )\n",
	       bb->bb_link_i.avg_phy_rate, bb->bb_link_i.rx_utility);
}

void halbb_basic_dbg_01_system(struct bb_info *bb)
{
	struct bb_link_info	*link = &bb->bb_link_i;
	struct bb_ch_info	*ch = &bb->bb_ch_i;
	struct bb_dbg_info	*dbg = &bb->bb_dbg_i;
	struct bb_physts_info	*physts = &bb->bb_physts_i;
	struct bb_cmn_dbg_info *cmn_dbg = &bb->bb_cmn_hooker->bb_cmn_dbg_i;
	enum channel_width bw = bb->hal_com->band[bb->bb_phy_idx].cur_chandef.bw;
	u8 fc = bb->hal_com->band[bb->bb_phy_idx].cur_chandef.center_ch;
	u8 sta_cnt = 0;
	u16 i = 0;
	char ext_loss[HALBB_MAX_PATH][HALBB_SNPRINT_SIZE];
	if (!(bb->cmn_dbg_msg_component & BB_BASIC_DBG_01_SYSTEM)) {
		BB_DBG(bb, DBG_CMN, "Disabled\n");
		return;
	}

	BB_DBG(bb, DBG_CMN, "[%s mode], TP{T,R,ALL}={%d, %d, %d}, BW:%d/%d, CH_fc:%d, ch_switch_cnt=%d\n",
	       ((bb->bb_watchdog_mode == BB_WATCHDOG_NORMAL) ? "Normal" :
	       ((bb->bb_watchdog_mode == BB_WATCHDOG_LOW_IO) ? "LowIO" : "NonIO")),
	       link->tx_tp, link->rx_tp, link->total_tp, 20 << bw,
	       bb->bb_link_i.bb_bw, fc, bb->bb_api_i.ch_switch_cnt);
	BB_DBG(bb, DBG_CMN,
	       "Phy:%d, linked: %d, Num_sta: %d, rssi_max/min= {%02d.%d, %02d.%d}, Noisy:%d\n",
	       bb->bb_phy_idx,
	       link->is_linked, bb->hal_com->assoc_sta_cnt,
	       ch->rssi_max >> 1, (ch->rssi_max & 1) * 5,
	       ch->rssi_min >> 1, (ch->rssi_min & 1) * 5,
	       ch->is_noisy);

	for (i = 0; i < HALBB_MAX_PATH; i++)
		halbb_print_sign_frac_digit(bb, ch->ext_loss[i], 8, 2,
					    ext_loss[i], HALBB_SNPRINT_SIZE);

	halbb_print_sign_frac_digit(bb, ch->ext_loss_avg, 8, 2, bb->dbg_buf,
				    HALBB_SNPRINT_SIZE);

	if (bb->num_rf_path >= 4) {
#if (defined(HALBB_COMPILE_ABOVE_4SS))
		BB_DBG(bb, DBG_CMN,
		       "ext_loss{avg, a, b, c, d} = {%s, %s, %s, %s, %s} dB, int rssi_max/min = {%02d.%d, %02d.%d}\n",
		       bb->dbg_buf, ext_loss[0], ext_loss[1], ext_loss[2],
		       ext_loss[3],
		       ch->int_rssi_max >> 1, (ch->int_rssi_max & 1) * 5,
		       ch->int_rssi_min >> 1, (ch->int_rssi_min & 1) * 5);
#endif
	} else if (bb->num_rf_path >= 2) {
#if (defined(HALBB_COMPILE_ABOVE_2SS))
		BB_DBG(bb, DBG_CMN,
		       "ext_loss{avg, a, b} = {%s, %s, %s} dB, int rssi_max/min = {%02d.%d, %02d.%d}\n",
		       bb->dbg_buf, ext_loss[0], ext_loss[1],
		       ch->int_rssi_max >> 1, (ch->int_rssi_max & 1) * 5,
		       ch->int_rssi_min >> 1, (ch->int_rssi_min & 1) * 5);
#endif
	} else {
		BB_DBG(bb, DBG_CMN,
		       "ext_loss=%s dB, int rssi_max/min = {%02d.%d, %02d.%d}\n",
		       ext_loss[0], ch->int_rssi_max >> 1,
		       (ch->int_rssi_max & 1) * 5, ch->int_rssi_min >> 1,
		       (ch->int_rssi_min & 1) * 5);
	}

	BB_DBG(bb, DBG_CMN,
	       "Mode=%s %s %s %s %s %s\n",
	       (link->wlan_mode_bitmap & WLAN_MD_11B) ? "B" : " ",
	       (link->wlan_mode_bitmap & (WLAN_MD_11G | WLAN_MD_11A)) ? "G" : " ",
	       (link->wlan_mode_bitmap & WLAN_MD_11N) ? "N" : " ",
	       (link->wlan_mode_bitmap & WLAN_MD_11AC) ? "AC" : " ",
	       (link->wlan_mode_bitmap & WLAN_MD_11AX) ? "AX" : " ",
	       (link->wlan_mode_bitmap & BIT6) ? "BE" : " ");

	BB_DBG(bb, DBG_CMN, "physts_cnt{all, 2_self, ok_ie, err_ie, err_len}={%d,%d,%d,%d,%d}, invalid_he=%d\n",
	       physts->bb_physts_cnt_i.all_cnt, physts->bb_physts_cnt_i.is_2_self_cnt,
	       physts->bb_physts_cnt_i.ok_ie_cnt, physts->bb_physts_cnt_i.err_ie_cnt,
	       physts->bb_physts_cnt_i.err_len_cnt,
	       physts->bb_physts_cnt_i.invalid_he_cnt);

	for (i = 0; i< PHL_MAX_STA_NUM; i++) {
		BB_DBG(bb, DBG_CMN, "[%d] Linked macid=%d\n",
		       i, bb->sta_exist[i]);

		sta_cnt++;
		if (sta_cnt >= bb->hal_com->assoc_sta_cnt)
			break;
	}
}

void halbb_basic_dbg_04_tx(struct bb_info *bb)
{
	if (!(bb->cmn_dbg_msg_component & BB_BASIC_DBG_04_TX)) {
		BB_DBG(bb, DBG_CMN, "Disabled\n");
		return;
	}

	halbb_basic_dbg_msg_tx_dbg_reg(bb);

	if (bb->bb_link_i.is_linked)
		halbb_basic_dbg_msg_tx_info(bb);
}

void halbb_basic_dbg_09_dm_summary(struct bb_info *bb)
{
#ifdef HALBB_CFO_TRK_SUPPORT
	struct bb_cfo_trk_info *cfo_trk = &bb->bb_cfo_trk_i;
#endif
#ifdef HALBB_ANT_DIV_SUPPORT
	struct bb_link_info *bb_link = &bb->bb_link_i;
	struct bb_antdiv_info *bb_ant_div = &bb->bb_ant_div_i;
	struct rtw_phl_com_t *phl = bb->phl_com;
	struct dev_cap_t *dev = &phl->dev_cap;
#endif

	if (!(bb->cmn_dbg_msg_component & BB_BASIC_DBG_09_DM_SUMMARY)) {
		BB_DBG(bb, DBG_CMN, "Disabled\n");
		return;
	}

	/*DIG*/
	halbb_dig_cmn_log(bb);

#ifdef HALBB_CFO_TRK_SUPPORT
	halbb_print_sign_frac_digit(bb, cfo_trk->cfo_avg_pre, 16, 2, bb->dbg_buf, HALBB_SNPRINT_SIZE);

	BB_DBG(bb, DBG_CMN, "CFO[T-1]=(%s kHz), cryst_cap=(%s%d), cfo_ofst=%d\n",
	       bb->dbg_buf,
	       ((cfo_trk->crystal_cap > cfo_trk->def_x_cap) ? "+" : "-"),
	       DIFF_2(cfo_trk->crystal_cap, cfo_trk->def_x_cap),
	       cfo_trk->x_cap_ofst);
#endif

#ifdef HALBB_ANT_DIV_SUPPORT
	if (bb->support_ability & BB_ANT_DIV) {
		BB_DBG(bb, DBG_CMN,
		       "[ANTDIV]rfe_type=%d, ANT=%d, at_least_one_bfee=%d\n",
		       dev->rfe_type, bb_ant_div->target_ant,
		       bb_link->at_least_one_bfee);
	}
#endif

#ifdef HALBB_EDCCA_SUPPORT
	if (bb->support_ability & BB_EDCCA) {
		halbb_edcca_cmn_log(bb);
	}
#endif
}

void halbb_basic_dbg_message(struct bb_info *bb)
{
	struct bb_cmn_dbg_info *cmn_dbg = &bb->bb_cmn_hooker->bb_cmn_dbg_i;

	if (bb->bb_cmn_hooker->bb_cmn_dbg_i.cmn_log_2_cnsl_en)
		return;
	if (bb->bb_cmn_hooker->bb_cmn_dbg_i.cmn_log_2_drv_statistic_en)
		return;

#ifdef HALBB_DBG_TRACE_SUPPORT
	if (!(bb->dbg_component & DBG_CMN) && !(bb->bb_auto_dbg_i.auto_dbg_type & AUTO_DBG_PHY_UTILITY))
		return;

	if (bb->cmn_dbg_msg_cnt >= bb->cmn_dbg_msg_period) { /*unit: Sec*/
		bb->cmn_dbg_msg_cnt = HALBB_WATCHDOG_PERIOD;
	} else {
		bb->cmn_dbg_msg_cnt += HALBB_WATCHDOG_PERIOD;
		return;
	}
#endif

	BB_DBG(bb, DBG_CMN, "[%s]%s\n", __func__, HLABB_CODE_BASE);
	BB_DBG(bb, DBG_CMN, "====[1. System] (%08d sec) (Ability=0x%08llx)\n",
	       bb->bb_sys_up_time, bb->support_ability);
	halbb_basic_dbg_01_system(bb);
	BB_DBG(bb, DBG_CMN, "\n");

	BB_DBG(bb, DBG_CMN, "====[2. ENV Mntr]\n");
	halbb_env_mntr_log(bb, DBG_CMN);
	BB_DBG(bb, DBG_CMN, "\n");

	BB_DBG(bb, DBG_CMN, "====[3. PMAC]\n");
	halbb_basic_dbg_03_msg_pmac(bb);
	BB_DBG(bb, DBG_CMN, "\n");

	BB_DBG(bb, DBG_CMN, "====[4. TX General]\n");
	halbb_basic_dbg_04_tx(bb);
	BB_DBG(bb, DBG_CMN, "\n");

	if (bb->bb_link_i.is_linked) {
		BB_DBG(bb, DBG_CMN, "====[5. RX General]\n");
		halbb_basic_dbg_05_rx(bb);
		BB_DBG(bb, DBG_CMN, "\n");

		BB_DBG(bb, DBG_CMN, "====[6. AVG RSSI/RxRate]\n");
		halbb_basic_dbg_06_rssi_rate(bb);
		BB_DBG(bb, DBG_CMN, "\n");

		BB_DBG(bb, DBG_CMN, "====[7. BB Hist]\n");
		halbb_basic_dbg_07_hist_su(bb);
		BB_DBG(bb, DBG_CMN, "\n");

		BB_DBG(bb, DBG_CMN, "====[8. [MU] AVG RSSI/RxRate]\n");
		halbb_basic_dbg_08_rssi_rate_mu(bb);
		BB_DBG(bb, DBG_CMN, "\n");
	}

	BB_DBG(bb, DBG_CMN, "====[9. DM Summary]\n");
	halbb_basic_dbg_09_dm_summary(bb);

	BB_DBG(bb, DBG_CMN, "============================================\n");
	BB_DBG(bb, DBG_CMN, "\n");
}


#endif

void halbb_get_bb_para_pkg_ver(struct bb_info *bb, u32 *date, u32 *release_ver)

{
	switch (bb->ic_type) {
#ifdef BB_8852A_2_SUPPORT
	case BB_RTL8852A:
		*date = BB_REG_RELEASE_DATE_8852A_2;
		*release_ver = BB_REG_RELEASE_VERSION_8852A_2;
		break;
#endif
#ifdef BB_8852B_SUPPORT
	case BB_RTL8852B:
		*date = BB_REG_RELEASE_DATE_8852B;
		*release_ver = BB_REG_RELEASE_VERSION_8852B;
		break;
#endif
#ifdef BB_8852C_SUPPORT
	case BB_RTL8852C:
		if (bb->ic_sub_type == BB_IC_SUB_TYPE_8852C_8852C) {
			*date = BB_REG_RELEASE_DATE_8852C;
			*release_ver = BB_REG_RELEASE_VERSION_8852C;
		} else if (bb->ic_sub_type == BB_IC_SUB_TYPE_8852C_8852D) {
			*date = BB_REG_RELEASE_DATE_8852D;
			*release_ver = BB_REG_RELEASE_VERSION_8852D;
		}
		break;
#endif
#ifdef BB_8192XB_SUPPORT
	case BB_RTL8192XB:
		*date = BB_REG_RELEASE_DATE_8192XB;
		*release_ver = BB_REG_RELEASE_VERSION_8192XB;
		break;
#endif
#ifdef BB_8851B_SUPPORT
	case BB_RTL8851B:
		*date = BB_REG_RELEASE_DATE_8851B;
		*release_ver = BB_REG_RELEASE_VERSION_8851B;
		break;
#endif
#ifdef BB_1115_SUPPORT
	case BB_RLE1115:
		*date = BB_REG_RELEASE_DATE_1115;
		*release_ver = BB_REG_RELEASE_VERSION_1115;
		break;
#endif
	default:
		BB_WARNING("[%s] ic=%d\n", __func__, bb->ic_type);
		break;
	}

}

void halbb_basic_profile_dbg(struct bb_info *bb, u32 *_used, char *output, u32 *_out_len)
{
	char *cv = NULL;
	char *name = NULL;
	char *support = NULL;
	u32 used = *_used;
	u32 out_len = *_out_len;
	u32 date = 0;
	u32 release_ver = 0;

	BB_DBG_CNSL(out_len, used, output + used, out_len - used, "%s\n",
		 "% [Basic Info] %");

	switch (bb->ic_type) {
	#ifdef BB_8852A_2_SUPPORT
	case BB_RTL8852A:
		name = "8852A(>Bcut)";
		break;
	#endif
	#ifdef BB_8852B_SUPPORT
	case BB_RTL8852B:
		name = "8852B";
		break;
	#endif
	#ifdef BB_8852C_SUPPORT
	case BB_RTL8852C:
		name = "8852C";
		break;
	#endif
	#ifdef BB_8192XB_SUPPORT
	case BB_RTL8192XB:
		name = "8192XB";
		break;
	#endif
	#ifdef BB_8851B_SUPPORT
	case BB_RTL8851B:
		name = "8851B";
		break;
	#endif
	#ifdef BB_1115_SUPPORT
	case BB_RLE1115:
		name = "_1115";
		break;
	#endif
	#ifdef BB_8922A_SUPPORT
	case BB_RTL8922A:
		name = "8922A";
		break;
	#endif

	default:
		BB_WARNING("[%s]\n", __func__);
		break;
	}
	BB_DBG_CNSL(out_len, used, output + used, out_len - used, "  %-25s: RTL%s\n",
		 "IC", name);
	BB_DBG_CNSL(out_len, used, output + used, out_len - used, "  %-25s: %d\n",
		 "sub_type", bb->ic_sub_type);
	BB_DBG_CNSL(out_len, used, output + used, out_len - used, "  %-25s: %d\n",
		 "CR_type", bb->cr_type);
	BB_DBG_CNSL(out_len, used, output + used, out_len - used, "  %-25s: %s\n",
		 "80211 SPEC", ((bb->bb_80211spec == BB_AX_IC) ? "AX" : "BE"));
	BB_DBG_CNSL(out_len, used, output + used, out_len - used,
		 "  %-25s: %s \n", "Normal Mode",
		 (bb->phl_com->drv_mode == RTW_DRV_MODE_NORMAL)? "Y" : "N");

	if (bb->hal_com->cv == CAV)
		cv = "CAV";
	else if (bb->hal_com->cv == CBV)
		cv = "CBV";
	else if (bb->hal_com->cv == CCV)
		cv = "CCV";
	else if (bb->hal_com->cv == CDV)
		cv = "CDV";
	else if (bb->hal_com->cv == CEV)
		cv = "CEV";
	else
		cv = "NA";

	BB_DBG_CNSL(out_len, used, output + used, out_len - used, "  %-25s: %d\n",
		    "RFE", bb->phl_com->dev_cap.rfe_type);
	BB_DBG_CNSL(out_len, used, output + used, out_len - used, "  %-25s: %d\n",
		    "PKG", bb->phl_com->dev_cap.pkg_type);
	BB_DBG_CNSL(out_len, used, output + used, out_len - used, "  %-25s: %s\n",
		    "CV", cv);
	BB_DBG_CNSL(out_len, used, output + used, out_len - used,
		    "  %-25s: %d.%d\n", "FW Ver", bb->u8_dummy,
		    bb->u8_dummy); /*TBD*/


	 BB_DBG_CNSL(out_len, used, output + used, out_len - used,
		 "  %-25s: num_rf_path=%d, HALBB_MAX_PATH = %d\n", "Path_Info",
		 bb->num_rf_path, HALBB_MAX_PATH);

	#ifdef HALBB_COMPILE_ABOVE_1SS
		BB_DBG_CNSL(out_len, used, output + used, out_len - used, "  %-25s\n",
			    "HALBB_COMPILE_ABOVE_1SS");
	#endif
	#ifdef HALBB_COMPILE_ABOVE_2SS
		BB_DBG_CNSL(out_len, used, output + used, out_len - used, "  %-25s\n",
			    "HALBB_COMPILE_ABOVE_2SS");
	#endif
	#ifdef HALBB_COMPILE_ABOVE_3SS
		BB_DBG_CNSL(out_len, used, output + used, out_len - used, "  %-25s\n",
			    "HALBB_COMPILE_ABOVE_3SS");
	#endif
	#ifdef HALBB_COMPILE_ABOVE_4SS
		BB_DBG_CNSL(out_len, used, output + used, out_len - used, "  %-25s\n",
			    "HALBB_COMPILE_ABOVE_4SS");
	#endif

	/*[HALBB Info]*/
	BB_DBG_CNSL(out_len, used, output + used, out_len - used, "%s\n",
		 "% [HALBB Info] %");

	BB_DBG_CNSL(out_len, used, output + used, out_len - used, "  %-25s: %s (idx: %d)\n",
		 "Branch", HLABB_CODE_BASE, HLABB_CODE_BASE_NUM);

	halbb_get_bb_para_pkg_ver(bb, &date, &release_ver);
	BB_DBG_CNSL(out_len, used, output + used, out_len - used, "  %-25s: %02d (%d)\n",
		 "BB CR Ver", release_ver, date);
#ifdef PHL_FEATURE_AP
	BB_DBG_CNSL(out_len, used, output + used, out_len - used, "  %-25s, bb_drv_type=%d\n",
		    "AP drv", bb->bb_cmn_hooker->bb_drv_type);
#endif

#ifdef PHL_FEATURE_NIC
	BB_DBG_CNSL(out_len, used, output + used, out_len - used, "  %-25s, bb_drv_type=%d\n",
		    "NIC drv", bb->bb_cmn_hooker->bb_drv_type);
#endif

	/*IC Support List*/
	BB_DBG_CNSL(out_len, used, output + used, out_len - used, "%s\n",
		 "% [IC Support List] %");

#ifdef BB_8852A_2_SUPPORT
	BB_DBG_CNSL(out_len, used, output + used, out_len - used, "  %-25s\n",
		 "8852A");
#endif
#ifdef BB_8852B_SUPPORT
	BB_DBG_CNSL(out_len, used, output + used, out_len - used, "  %-25s\n",
		 "8852B");
#endif
#ifdef BB_8852C_SUPPORT
	BB_DBG_CNSL(out_len, used, output + used, out_len - used, "  %-25s\n",
		 "8852C");
#endif
#ifdef BB_8192XB_SUPPORT
	BB_DBG_CNSL(out_len, used, output + used, out_len - used, "  %-25s\n",
		 "8192XB");
#endif
#ifdef BB_8851B_SUPPORT
	BB_DBG_CNSL(out_len, used, output + used, out_len - used, "  %-25s\n",
		 "8851B");
#endif
#ifdef BB_1115_SUPPORT
	BB_DBG_CNSL(out_len, used, output + used, out_len - used, "  %-25s\n",
		 "1115");
	#if (HLABB_CODE_BASE_NUM == 32) //will be removed when (022+032) branch phase out
	BB_DBG_CNSL(out_len, used, output + used, out_len - used, "  %-25s\n",
		 "   *HALBB_032_Rename");
	#endif
#endif
#ifdef BB_8922A_SUPPORT
	BB_DBG_CNSL(out_len, used, output + used, out_len - used, "  %-25s\n",
		 "8922A");
#endif
		 
	/*Feature Compile List*/
	BB_DBG_CNSL(out_len, used, output + used, out_len - used, "%s\n",
		 "% [Feature Support List] %");

	#ifdef HALBB_FW_OFLD_SUPPORT
	support = "Y";
	#else
	support = ".";
	#endif
	BB_DBG_CNSL(out_len, used, output + used, out_len - used, "  %-25s: %s\n",
		    "FW_OFLD", support);
	#ifdef HALBB_DBG_TRACE_SUPPORT
	support = "Y";
	#else
	support = ".";
	#endif
	BB_DBG_CNSL(out_len, used, output + used, out_len - used, "  %-25s: %s\n",
		    "DBG_TRACE", support);

	#ifdef HALBB_TIMER_SUPPORT
	support = "Y";
	#else
	support = ".";
	#endif
	BB_DBG_CNSL(out_len, used, output + used, out_len - used, "  %-25s: %s\n",
		    "TIMER", support);

	#ifdef HALBB_PHYSTS_PARSING_SUPPORT
	support = "Y";
	#else
	support = ".";
	#endif
	BB_DBG_CNSL(out_len, used, output + used, out_len - used, "  %-25s: %s\n",
		    "PHYSTS", support);
	#ifdef HALBB_ENV_MNTR_SUPPORT
	support = "Y";
	#else
	support = ".";
	#endif
	BB_DBG_CNSL(out_len, used, output + used, out_len - used, "  %-25s: %s\n",
		    "ENV_MNTR", support);
	#ifdef HALBB_STATISTICS_SUPPORT
	support = "Y";
	#else
	support = ".";
	#endif
	BB_DBG_CNSL(out_len, used, output + used, out_len - used, "  %-25s: %s\n",
		    "STATISTICS", support);
	#ifdef HALBB_RA_SUPPORT
	support = "Y";
	#else
	support = ".";
	#endif
	BB_DBG_CNSL(out_len, used, output + used, out_len - used, "  %-25s: %s\n",
		    "RA", support);
	#ifdef HALBB_EDCCA_SUPPORT
	support = "Y";
	#else
	support = ".";
	#endif
	BB_DBG_CNSL(out_len, used, output + used, out_len - used, "  %-25s: %s\n",
		    "EDCCA", support);
	#ifdef HALBB_CFO_TRK_SUPPORT
	support = "Y";
	#else
	support = ".";
	#endif
	BB_DBG_CNSL(out_len, used, output + used, out_len - used, "  %-25s: %s\n",
		    "CFO_TRK", support);
	#ifdef HALBB_LA_MODE_SUPPORT
	support = "Y";
	#else
	support = ".";
	#endif
	BB_DBG_CNSL(out_len, used, output + used, out_len - used, "  %-25s: %s\n",
		    "LA_MODE", support);
	#ifdef HALBB_PSD_SUPPORT
	support = "Y";
	#else
	support = ".";
	#endif
	BB_DBG_CNSL(out_len, used, output + used, out_len - used, "  %-25s: %s\n",
		    "PSD", support);
	#ifdef HALBB_PWR_CTRL_SUPPORT
	support = "Y";
	#else
	support = ".";
	#endif
	BB_DBG_CNSL(out_len, used, output + used, out_len - used, "  %-25s: %s\n",
		    "PWR_CTRL", support);
	#ifdef HALBB_RUA_SUPPORT
	support = "Y";
	#else
	support = ".";
	#endif
	BB_DBG_CNSL(out_len, used, output + used, out_len - used, "  %-25s: %s\n",
		    "RUA", support);
	#ifdef HALBB_PMAC_TX_SUPPORT
	support = "Y";
	#else
	support = ".";
	#endif
	BB_DBG_CNSL(out_len, used, output + used, out_len - used, "  %-25s: %s\n",
		    "PMAC_TX", support);
	#ifdef HALBB_CH_INFO_SUPPORT
	support = "Y";
	#else
	support = ".";
	#endif
	BB_DBG_CNSL(out_len, used, output + used, out_len - used, "  %-25s: %s\n",
		    "CH_INFO", support);
	#ifdef HALBB_AUTO_DBG_SUPPORT
	support = "Y";
	#else
	support = ".";
	#endif
	BB_DBG_CNSL(out_len, used, output + used, out_len - used, "  %-25s: %s\n",
		    "AUTO_DBG", support);
	#ifdef HALBB_ANT_DIV_SUPPORT
	support = "Y";
	#else
	support = ".";
	#endif
	BB_DBG_CNSL(out_len, used, output + used, out_len - used, "  %-25s: %s\n",
		    "ANT_DIV", support);
	#ifdef HALBB_DBCC_SUPPORT
	support = "Y";
	#else
	support = ".";
	#endif
	BB_DBG_CNSL(out_len, used, output + used, out_len - used, "  %-25s: %s\n",
		    "DBCC", support);
	#ifdef HALBB_PATH_DIV_SUPPORT
	support = "Y";
	#else
	support = ".";
	#endif
	BB_DBG_CNSL(out_len, used, output + used, out_len - used, "  %-25s: %s\n",
		    "PATH_DIV", support);
	#ifdef HALBB_SNIF_SUPPORT
	support = "Y";
	#else
	support = ".";
	#endif
	BB_DBG_CNSL(out_len, used, output + used, out_len - used, "  %-25s: %s\n",
		    "Sniffer", support);

	*_used = used;
	*_out_len = out_len;
}

#if HALBB_DBG_DVLP_FLAG /*Dump register - relative*/

void halbb_cr_table_dump(struct bb_info *bb, u32 *cr_table, u32 cr_len)
{
	u32 i = 0;
	u32 cr_tmp, val;

	for (i = 0; i < cr_len; i++) {
		cr_tmp = cr_table[i];
		if (cr_tmp == 0)
			continue;

		val = halbb_get_reg(bb, cr_tmp, MASKDWORD);
		BB_TRACE("[%03d]Reg[0x%04x] = 0x%08x\n", i, cr_tmp, val);
	}
}

void halbb_cr_hook_fake_init(struct bb_info *bb, u32 *str_table, u32 len)
{
	u32 i = 0;

	BB_TRACE("[%s]\n", __func__);

	for (i = 0; i < len; i++)
		str_table[i] = bb->bb_dbg_i.cr_fake_init_hook_val;
}

void halbb_cr_hook_init_dump(struct bb_info *bb, u32 *str_table, u32 len)
{
	u32 i = 0;
	u32 val_tmp = 0;

	BB_TRACE("[%s]\n", __func__);
	for (i = 0; i < len; i++) {
		val_tmp = str_table[i];
		if (val_tmp > 100000) {
			BB_TRACE("[%03d] Mask             [0x%08x]\n", i, val_tmp);
			continue;
		}

		if ((val_tmp & 0xf) == 0 || (val_tmp & 0xf) == 4 ||
		    (val_tmp & 0xf) == 8 || (val_tmp & 0xf) == 0xc) {
			BB_TRACE("[%03d] Maybe Reg 0x%04x\n", i, val_tmp);
		} else {
			BB_TRACE("[%03d] Maybe Mask       [0x%08x]\n", i, val_tmp);
		}
	}
	halbb_print_devider(bb, BB_DEVIDER_LEN_32, true, FRC_PRINT_LINE);
}

void halbb_dump_bb_reg(struct bb_info *bb, u32 *_used, char *output,
		       u32 *_out_len, bool dump_2_buff,
		       enum bb_frc_phy_dump_reg frc_phy_dump)
{
	u32 release_ver = 0, date = 0;

	if (dump_2_buff) {
		if (*_out_len < 100) {
			BB_WARNING("[%s] out_len=%d", __func__, _out_len[0]);
			return;
		}
	}

	BB_TRACE1(bb, "%-15s: %s\n", "Branch", HLABB_CODE_BASE);

	halbb_get_bb_para_pkg_ver(bb, &date, &release_ver);
	BB_TRACE1(bb, "%-15s: %02d (%d)\n", "BB CR Ver", release_ver, date);

	switch (bb->ic_type) {

	#ifdef BB_8852A_2_SUPPORT
	case BB_RTL8852A:
		halbb_dump_bb_reg_8852a_2(bb, _used, output, _out_len, dump_2_buff, frc_phy_dump);
		break;
	#endif

	#ifdef BB_8852B_SUPPORT
	case BB_RTL8852B:
		halbb_dump_bb_reg_8852b(bb, _used, output, _out_len, dump_2_buff);
		break;
	#endif

	#ifdef BB_8852C_SUPPORT
	case BB_RTL8852C:
		halbb_dump_bb_reg_8852c(bb, _used, output, _out_len, dump_2_buff, frc_phy_dump);
		break;
	#endif

	#ifdef BB_8192XB_SUPPORT
	case BB_RTL8192XB:
		halbb_dump_bb_reg_8192xb(bb, _used, output, _out_len, dump_2_buff);
		break;
	#endif

	#ifdef BB_8851B_SUPPORT
	case BB_RTL8851B:
		halbb_dump_bb_reg_8851b(bb, _used, output, _out_len, dump_2_buff);
		break;
	#endif
	#ifdef BB_1115_SUPPORT
	case BB_RLE1115:
		halbb_dump_bb_reg_1115(bb, _used, output, _out_len, dump_2_buff, frc_phy_dump);
		break;
	#endif
	#ifdef BB_8922A_SUPPORT
	case BB_RTL8922A:
		halbb_dump_bb_reg_8922a(bb, _used, output, _out_len, dump_2_buff, frc_phy_dump);
		break;
	#endif

	default:
		break;
	}
}

void halbb_dump_reg_dbg(struct bb_info *bb, char input[][16], u32 *_used, char *output,
		    u32 *_out_len)
{
	u32 val[10] = {0};
	u32 addr = 0;
	enum bb_frc_phy_dump_reg frc_phy_dump = FRC_DUMP_ALL;

	if (_os_strcmp(input[1], "-h") == 0) {
		BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
			"dumpreg all\n");
		BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
			"dumpreg frc_phy {val}\n");
		return;
	}

	if (_os_strcmp(input[1], "frc_phy") == 0) {
		HALBB_SCAN(input[2], DCMD_DECIMAL, &val[0]);
		frc_phy_dump = (enum bb_frc_phy_dump_reg)val[0];
		BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
			    "frc_phy_dump = phy:%d\n", frc_phy_dump);
	}

	halbb_dump_bb_reg(bb, _used, output, _out_len, true, frc_phy_dump);
}

void halbb_dd_dump_dbg(struct bb_info *bb, char input[][16], u32 *_used,
		       char *output, u32 *_out_len)
{
	char help[] = "-h";
	u32 val[10] = {0};
	u32 used = *_used;
	u32 out_len = *_out_len;

	HALBB_SCAN(input[1], DCMD_DECIMAL, &val[0]);

	if (_os_strcmp(input[1], help) == 0) {
		BB_DBG_CNSL(out_len, *_used, output + *_used, out_len - *_used,
			    "{dd_dbg}\n");
		return;
	}
	/*[Reg]*/
	halbb_dump_bb_reg(bb, &used, output, &out_len, true, FRC_DUMP_ALL);
	/*[Dbg Port]*/
	halbb_dbgport_dump_all(bb, _used, output, _out_len);
	/*[Analog Parameters]*/
	//halbb_get_anapar_table(bb, &used, output, &out_len);

}
#endif

void halbb_cmn_dbg(struct bb_info *bb, char input[][16], u32 *_used,
			char *output, u32 *_out_len)
{
	u32 val[10] = {0};
	u32 cr = 0;
	bool rpt = true;
	enum phl_phy_idx phy_idx;
	struct rtw_para_info_t *reg = NULL;
	enum bb_cr_t cr_type_bkp;
	u16 i = 0, j = 0, k = 0;

	if (_os_strcmp(input[1], "-h") == 0) {
		BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
			 "{cr_rec, cr_rec_rf} {en}\n");
		BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
			 "cr_mp_rec {en}\n");
		BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
			 "cr_hook_list\n");
		BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
			 "fake_hook {en} {hex_val}\n");
		BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
			 "wd {0:Normal/1:LowIo/2:NonIO}\n");
		BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
			 "event {phl_evt_id}\n");
		BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
			 "period {val}\n");
		BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
			 "init {cr, gain} {phy_idx}\n");
		BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
			 "init dbg_mode {en} {rfe} {cv}\n");
		BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
			 "ch_swh_history {1}\n");
	} else if (_os_strcmp(input[1], "ch_swh_history") == 0) {
		BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
			 "CH Switch History\n");
		BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
			 "[Past]================\n");

		j = bb->bb_api_i.ch_switch_ptr;

		while (i < BB_CH_SWH_HISTORY_SIZE) {
			if (i == (BB_CH_SWH_HISTORY_SIZE - 1)) {
				BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
					    "[%02d] Fc_CH_idx=%03d (Current CH)\n",
					    i, bb->bb_api_i.ch_switch_history[j]);
			} else {
				BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
					    "[%02d] Fc_CH_idx=%03d\n",
					    i, bb->bb_api_i.ch_switch_history[j]);
			}
			i++;
			j++;
			if (j == BB_CH_SWH_HISTORY_SIZE)
				j = 0;
		}
		BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
			 "[Now]==================\n");
	} else if (_os_strcmp(input[1], "event") == 0) {
		HALBB_SCAN(input[2], DCMD_DECIMAL, &val[0]);
		halbb_wifi_event_notify(bb, (enum phl_msg_evt_id)val[0], bb->bb_phy_idx);
		BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
			    "phl_evt_id=%d\n", val[0]);
	} else if (_os_strcmp(input[1], "period") == 0) {
		HALBB_SCAN(input[2], DCMD_DECIMAL, &val[0]);
		bb->bb_watchdog_period = (u8)(val[0] & 0xfe);
		if (bb->bb_watchdog_period < 2)
			bb->bb_watchdog_period = 2;
		BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
			    "wd_period=%d\n", bb->bb_watchdog_period);
	} else if (_os_strcmp(input[1], "cr_rec") == 0) {
		HALBB_SCAN(input[2], DCMD_DECIMAL, &val[0]);
		bb->bb_dbg_i.cr_recorder_en = (bool)val[0];
	} else if (_os_strcmp(input[1], "cr_mp_rec") == 0) {
		HALBB_SCAN(input[2], DCMD_DECIMAL, &val[0]);
		bb->bb_dbg_i.cr_mp_recorder_en = (bool)val[0];
	} else if (_os_strcmp(input[1], "cr_hook_list") == 0) {
		bb->bb_dbg_i.cr_init_hook_recorder_en = true;
		halbb_cr_cfg_init(bb);	
		bb->bb_dbg_i.cr_init_hook_recorder_en = false;
	} else if (_os_strcmp(input[1], "fake_hook") == 0) {
		HALBB_SCAN(input[2], DCMD_DECIMAL, &val[0]);
		HALBB_SCAN(input[3], DCMD_HEX, &val[1]);
		bb->bb_dbg_i.cr_fake_init_hook_en = (bool)val[0];
		bb->bb_dbg_i.cr_fake_init_hook_val = (bool)val[1];

		if (bb->bb_dbg_i.cr_fake_init_hook_en) {
			cr_type_bkp = bb->cr_type;
			bb->cr_type = BB_BE0;
			BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
			 	    "Force cr_type=%d\n", bb->cr_type);
			halbb_cr_cfg_init(bb);
			bb->cr_type = cr_type_bkp;
			BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
			 	    "Resume cr_type_bkp=%d\n", bb->cr_type);
		} else {
			halbb_cr_cfg_init(bb);
		}

	} else if (_os_strcmp(input[1], "cr_rec_rf") == 0) {
		HALBB_SCAN(input[2], DCMD_DECIMAL, &val[0]);
		bb->bb_dbg_i.cr_recorder_rf_en = (bool)val[0];
	} else if (_os_strcmp(input[1], "wd") == 0) {
		HALBB_SCAN(input[2], DCMD_DECIMAL, &val[0]);
		halbb_watchdog(bb, (enum bb_watchdog_mode_t)val[0], bb->bb_phy_idx);
		BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
			    "Watchdog trigger, mode=%d\n", val[0]);
	} else if (_os_strcmp(input[1], "init") == 0) {
		HALBB_SCAN(input[3], DCMD_DECIMAL, &val[0]);
		HALBB_SCAN(input[4], DCMD_DECIMAL, &val[1]);
		HALBB_SCAN(input[5], DCMD_DECIMAL, &val[2]);

		if (_os_strcmp(input[2], "cr") == 0) {
			phy_idx = (enum phl_phy_idx)val[0];
			reg = &bb->phl_com->phy_sw_cap[phy_idx].bb_phy_reg_info;
			rpt = halbb_init_cr_default(bb, false, 0, &val[0], phy_idx);
			BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
				    "CR init Success=%d\n", rpt);
		} else if (_os_strcmp(input[2], "gain") == 0) {
			phy_idx = (enum phl_phy_idx)val[0];
			reg = &bb->phl_com->phy_sw_cap[phy_idx].bb_phy_reg_gain_info;
			rpt = halbb_init_gain_table(bb, false, 0, &val[0], phy_idx);
			BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
				    "Gain init Success=%d\n", rpt);
		} else if (_os_strcmp(input[2], "dbg_mode") == 0) {
			bb->bb_dbg_i.cr_dbg_mode_en = (bool)val[0];
			bb->bb_dbg_i.rfe_type_curr_dbg = val[1];
			bb->bb_dbg_i.cut_curr_dbg = val[2];
		} else {
			BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
				    "Err\n");
			return;
		}
	} else if (_os_strcmp(input[1], "1") == 0) {
		HALBB_SCAN(input[2], DCMD_DECIMAL, &val[0]);
		bb->hal_com->assoc_sta_cnt = (u8)val[0];
		BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
			 "assoc_sta_cnt=%d\n", bb->hal_com->assoc_sta_cnt);
	} else if (_os_strcmp(input[1], "cr_demo") == 0) {	
		cr = halbb_get_reg(bb, LA_CLK_EN, LA_CLK_EN_M);
		BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
			    "[Old] cr = %d\n", cr);

		HALBB_SET_CR(bb, LA_CLK_EN, ~cr);
		cr = HALBB_GET_CR(bb, LA_CLK_EN);
		BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
			    "[New] ~cr = %d\n", cr);

		HALBB_SET_CR(bb, LA_CLK_EN, ~cr);
		cr = HALBB_GET_CR(bb, LA_CLK_EN);
		BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
			    "[New] cr = %d\n", cr);
	}
}

void halbb_dbg_setting_init(struct bb_info *bb)
{
	halbb_cmd_parser_init(bb);
	bb->fw_dbg_component = 0;
	bb->cmn_dbg_msg_cnt = HALBB_WATCHDOG_PERIOD;
	bb->cmn_dbg_msg_period = HALBB_WATCHDOG_PERIOD;
	bb->cmn_dbg_msg_component = 0xffff;
	halbb_release_bb_dbg_port(bb);

	bb->bb_dbg_i.cr_recorder_en = false;
	bb->bb_dbg_i.cr_mp_recorder_en = false;
	bb->bb_dbg_i.cr_init_hook_recorder_en = false;
	bb->bb_dbg_i.cr_dbg_mode_en = false;

	bb->bb_cmn_hooker->bb_cmn_dbg_i.cmn_log_2_cnsl_en = false;
	bb->bb_cmn_hooker->bb_cmn_dbg_i.cmn_log_2_drv_statistic_en = false;
}

u32 halbb_c2h_fw_dbg(struct bb_info *bb, u16 len, u8 *c2h)
{
	u16 dbg_num = (u16)(c2h[0] | (c2h[1] << 8));
	u16 content[5] = {0};
	u8 i = 0;

	for (i = 0; i < 5; i++)
		content[i] = (u16)(c2h[2 + (i << 1)] | (c2h[3 + (i << 1)] << 8));

	if (dbg_num == BBDBG_NUM_RA_WRITE_CMAC_0) {
		BB_DBG(bb, DBG_FW_DBG,
		       "[FWBB][RA] macid:%d, mode:%d, ss:%d, mcs:%d, bw:%d\n",
		       content[0], content[1], content[2], content[3],
		       content[4]);
	} else if (dbg_num == BBDBG_NUM_RA_RATE_CHANGED_1) {
		BB_DBG(bb, DBG_FW_DBG,
		       "[FWBB][RA] macid:%d, rate_changed:%d\n",
		       content[0], content[1]);
	} else if (dbg_num == BBDBG_NUM_RA_CAL_PARA_2) {
		BB_DBG(bb, DBG_FW_DBG,
		       "[FWBB][RA] macid:%d, PER:%d, RDR;%d, R4:%d\n",
		       content[0], content[1], content[2], content[3]);
	} else if (dbg_num == BBDBG_NUM_RA_D_O_PARA_3) {
		BB_DBG(bb, DBG_FW_DBG,
		       "[FWBB][RA] macid:%d, rate_idx:%d, bw_idx;%d, PER_MA:%d, VAR:%d\n",
		       content[0], content[1], content[2], content[3],
		       content[4]);
	} else if (dbg_num == BBDBG_NUM_RA_UP_DOWN_TH_4) {
		BB_DBG(bb, DBG_FW_DBG,
		       "[FWBB][RA] macid:%d, d_o_n:%d, d_o_p;%d, RD_TH:%d, RU_TH:%d\n",
		       content[0], content[1], content[2], content[3],
		       content[4]);
	} else if (dbg_num == BBDBG_NUM_RA_TRY_RESULT_5) {
		BB_DBG(bb, DBG_FW_DBG,
		       "[FWBB][RA] macid:%d, try_result=%d, try_bw:%d, try_rate_idx;%d, try_PER:%d, try_RDR:%d\n",
		       content[0], c2h[5], c2h[4], content[2], content[3],
		       content[4]);
	} else if (dbg_num == BBDBG_NUM_RA_RATE_DOWN_6) {
		BB_DBG(bb, DBG_FW_DBG,
		       "[FWBB][RA] macid:%d, rate down, rate_down_cnt:%d\n",
		       content[0], content[1]);
	} else if (dbg_num == BBDBG_NUM_RA_RATE_FORCE_DOWN_7) {
		BB_DBG(bb, DBG_FW_DBG,
		       "[FWBB][RA] macid:%d, force rate down\n",
		       content[0]);
	} else if (dbg_num == BBDBG_NUM_RA_RATE_UP_8) {
		BB_DBG(bb, DBG_FW_DBG,
		       "[FWBB][RA] macid:%d, rate up, rate_up_cnt:%d\n",
		       content[0], content[1]);
	} else if (dbg_num == BBDBG_NUM_RA_RATE_STAY_9) {
		BB_DBG(bb, DBG_FW_DBG,
		       "[FWBB][RA] macid:%d, rate stay, rate_up_cnt:%d, rate_down_cnt:%d\n",
		       content[0], content[1], content[2]);
	} else {
		BB_DBG(bb, DBG_FW_DBG,
		       "[FWBB][others] dbg_num=%d, content0~4 = [0x%x,0x%x,0x%x,0x%x,0x%x]\n",
		       dbg_num, content[0], content[1], content[2], content[3],
		       content[4]);
	}

	return 0;
}

void halbb_cr_cfg_dbg_init(struct bb_info *bb)
{
	struct bb_dbg_cr_info *cr = &bb->bb_dbg_i.bb_dbg_cr_i;

	switch (bb->cr_type) {

	#ifdef HALBB_COMPILE_AP_SERIES
	case BB_AP:
		cr->dbgport_ip = DBG_PORT_IP_SEL_A;
		cr->dbgport_ip_m = DBG_PORT_IP_SEL_A_M;
		cr->dbgport_idx = DBG_PORT_SEL_A;
		cr->dbgport_idx_m = DBG_PORT_SEL_A_M;
		cr->dbgport_val = DBG32_D_A;
		cr->dbgport_val_m = DBG32_D_A_M;
		cr->clk_en = DBG_PORT_REF_CLK_EN_A;
		cr->clk_en_m = DBG_PORT_REF_CLK_EN_A_M;
		cr->dbgport_en = DBG_PORT_EN_A;
		cr->dbgport_en_m = DBG_PORT_EN_A_M;
		cr->bb_monitor_sel0 = MONITOR_SEL0_A;
		cr->bb_monitor_sel0_m = MONITOR_SEL0_A_M;
		cr->bb_monitor0 = 0x1b10;
		cr->bb_monitor0_m = MASKDWORD;
		cr->bb_monitor_sel1 = MONITOR_SEL1_A;
		cr->bb_monitor_sel1_m = MONITOR_SEL1_A_M;
		cr->bb_monitor1 = 0x1b14;
		cr->bb_monitor1_m = MASKDWORD;
		cr->path_0_txpw = 0x1C78;
		cr->path_0_txpw_m = 0x1FF;
		cr->path_1_txpw = 0x3C78;
		cr->path_1_txpw_m = 0x1FF;
		cr->bmode_tx = 0x23e0;
		/*mac_phy_intf*/
		cr->mac_phy_intf_sel_phy1 = INTF_R_INTF_RPT_SEL_P1_A;
		cr->mac_phy_intf_sel_phy1_m = INTF_R_INTF_RPT_SEL_P1_A_M;
		cr->mac_phy_txinfo[0] = 0x1800;
		cr->mac_phy_txinfo[1] = 0x1804;
		cr->mac_phy_txinfo[2] = 0x1808;
		cr->mac_phy_txinfo[3] = 0x180c;
		cr->mac_phy_txcomct[0] = 0x1810;
		cr->mac_phy_txcomct[1] = 0x1814;
		cr->mac_phy_txusrct[0][0] = 0x1818;
		cr->mac_phy_txusrct[0][1] = 0x181c;
		cr->mac_phy_txusrct[1][0] = 0x1820;
		cr->mac_phy_txusrct[1][1] = 0x1824;
		cr->mac_phy_txusrct[2][0] = 0x1828;
		cr->mac_phy_txusrct[2][1] = 0x182c;
		cr->mac_phy_txusrct[3][0] = 0x1830;
		cr->mac_phy_txusrct[3][1] = 0x1834;
		cr->mac_phy_txtimct = 0x1838;
		cr->mac_phy_lsig = 0x1840;
		cr->mac_phy_siga_0 = 0x1848;
		cr->mac_phy_siga_1 = 0x184c;
		cr->mac_phy_vht_sigb_0 = 0x1850;
		break;
	#endif
	#ifdef HALBB_COMPILE_AP2_SERIES
	case BB_AP2:
		cr->dbgport_ip = DBG_PORT_IP_SEL_A2;
		cr->dbgport_ip_m = DBG_PORT_IP_SEL_A2_M;
		cr->dbgport_idx = DBG_PORT_SEL_A2;
		cr->dbgport_idx_m = DBG_PORT_SEL_A2_M;
		cr->dbgport_val = DBG32_D_A2;
		cr->dbgport_val_m = DBG32_D_A2_M;
		cr->clk_en = DBG_PORT_REF_CLK_EN_A2;
		cr->clk_en_m = DBG_PORT_REF_CLK_EN_A2_M;
		cr->dbgport_en = DBG_PORT_EN_A2;
		cr->dbgport_en_m = DBG_PORT_EN_A2_M;
		cr->bb_monitor_sel0 = MONITOR_SEL0_A2;
		cr->bb_monitor_sel0_m = MONITOR_SEL0_A2_M;
		cr->bb_monitor0 = 0x1b10;
		cr->bb_monitor0_m = MASKDWORD;
		cr->bb_monitor_sel1 = MONITOR_SEL1_A2;
		cr->bb_monitor_sel1_m = MONITOR_SEL1_A2_M;
		cr->bb_monitor1 = 0x1b14;
		cr->bb_monitor1_m = MASKDWORD;
		cr->path_0_txpw = PATH0_TXPW_A2;
		cr->path_0_txpw_m = PATH0_TXPW_A2_M;
		cr->path_1_txpw = PATH1_TXPW_A2;
		cr->path_1_txpw_m = PATH1_TXPW_A2_M;
		cr->bmode_tx = 0x2380;
		/*mac_phy_intf*/
		cr->mac_phy_intf_sel_phy1 = INTF_R_INTF_RPT_SEL_P1_A2;
		cr->mac_phy_intf_sel_phy1_m = INTF_R_INTF_RPT_SEL_P1_A2_M;
		cr->mac_phy_txinfo[0] = 0x1800;
		cr->mac_phy_txinfo[1] = 0x1804;
		cr->mac_phy_txinfo[2] = 0x1808;
		cr->mac_phy_txinfo[3] = 0x180c;
		cr->mac_phy_txcomct[0] = 0x1810;
		cr->mac_phy_txcomct[1] = 0x1814;
		cr->mac_phy_txusrct[0][0] = 0x1818;
		cr->mac_phy_txusrct[0][1] = 0x181c;
		cr->mac_phy_txusrct[1][0] = 0x1820;
		cr->mac_phy_txusrct[1][1] = 0x1824;
		cr->mac_phy_txusrct[2][0] = 0x1828;
		cr->mac_phy_txusrct[2][1] = 0x182c;
		cr->mac_phy_txusrct[3][0] = 0x1830;
		cr->mac_phy_txusrct[3][1] = 0x1834;
		cr->mac_phy_txtimct = 0x1838;
		cr->mac_phy_lsig = 0x1840;
		cr->mac_phy_siga_0 = 0x1848;
		cr->mac_phy_siga_1 = 0x184c;
		cr->mac_phy_vht_sigb_0 = 0x1850;
		break;
	#endif
	#ifdef HALBB_COMPILE_CLIENT_SERIES
	case BB_CLIENT:
		cr->dbgport_ip = DBG_PORT_IP_SEL_C;
		cr->dbgport_ip_m = DBG_PORT_IP_SEL_C_M;
		cr->dbgport_idx = DBG_PORT_SEL_C;
		cr->dbgport_idx_m = DBG_PORT_SEL_C_M;
		cr->dbgport_val = DBG32_D_C;
		cr->dbgport_val_m = DBG32_D_C_M;
		cr->clk_en = DBG_PORT_REF_CLK_EN_C;
		cr->clk_en_m = DBG_PORT_REF_CLK_EN_C_M;
		cr->dbgport_en = DBG_PORT_EN_C;
		cr->dbgport_en_m = DBG_PORT_EN_C_M;
		cr->bb_monitor_sel0 = MONITOR_SEL0_C;
		cr->bb_monitor_sel0_m = MONITOR_SEL0_C_M;
		cr->bb_monitor0 = 0x1b10;
		cr->bb_monitor0_m = MASKDWORD;
		cr->bb_monitor_sel1 = MONITOR_SEL1_C;
		cr->bb_monitor_sel1_m = MONITOR_SEL1_C_M;
		cr->bb_monitor1 = 0x1b14;
		cr->bb_monitor1_m = MASKDWORD;
		cr->path_0_txpw = PATH0_TXPW_C;
		cr->path_0_txpw_m = PATH0_TXPW_C_M;
#ifdef HALBB_COMPILE_ABOVE_2SS /*prevent compiler errors in 1SS ICs*/
		cr->path_1_txpw = PATH1_TXPW_C;
		cr->path_1_txpw_m = PATH1_TXPW_C_M;
#endif
		cr->bmode_tx = 0x23e0;
		/*mac_phy_intf*/
		cr->mac_phy_intf_sel_phy1 = INTF_R_INTF_RPT_SEL_P1_C;
		cr->mac_phy_intf_sel_phy1_m = INTF_R_INTF_RPT_SEL_P1_C_M;
		cr->mac_phy_txinfo[0] = 0x1800;
		cr->mac_phy_txinfo[1] = 0x1804;
		cr->mac_phy_txinfo[2] = 0x1808;
		cr->mac_phy_txinfo[3] = 0x180c;
		cr->mac_phy_txcomct[0] = 0x1810;
		cr->mac_phy_txcomct[1] = 0x1814;
		cr->mac_phy_txusrct[0][0] = 0x1818;
		cr->mac_phy_txusrct[0][1] = 0x181c;
		cr->mac_phy_txusrct[1][0] = 0x1820;
		cr->mac_phy_txusrct[1][1] = 0x1824;
		cr->mac_phy_txusrct[2][0] = 0x1828;
		cr->mac_phy_txusrct[2][1] = 0x182c;
		cr->mac_phy_txusrct[3][0] = 0x1830;
		cr->mac_phy_txusrct[3][1] = 0x1834;
		cr->mac_phy_txtimct = 0x1838;
		cr->mac_phy_lsig = 0x1840;
		cr->mac_phy_siga_0 = 0x1848;
		cr->mac_phy_siga_1 = 0x184c;
		cr->mac_phy_vht_sigb_0 = 0x1850;
		break;
	#endif
	
	#ifdef HALBB_COMPILE_BE0_SERIES
	case BB_BE0:
		cr->dbgport_ip = DBG_PORT_IP_SEL_BE0;
		cr->dbgport_ip_m = DBG_PORT_IP_SEL_BE0_M;
		cr->dbgport_idx = DBG_PORT_SEL_BE0;
		cr->dbgport_idx_m = DBG_PORT_SEL_BE0_M;
		cr->dbgport_val = DBG32_D_BE0;
		cr->dbgport_val_m = DBG32_D_BE0_M;
		cr->clk_en = DBG_PORT_REF_CLK_EN_BE0;
		cr->clk_en_m = DBG_PORT_REF_CLK_EN_BE0_M;
		cr->dbgport_en = DBG_PORT_EN_BE0;
		cr->dbgport_en_m = DBG_PORT_EN_BE0_M;
		cr->bb_monitor_sel0 = MONITOR_SEL0_BE0;
		cr->bb_monitor_sel0_m = MONITOR_SEL0_BE0_M;
		cr->bb_monitor0 = 0x1b10;
		cr->bb_monitor0_m = MASKDWORD;
		cr->bb_monitor_sel1 = MONITOR_SEL1_BE0;
		cr->bb_monitor_sel1_m = MONITOR_SEL1_BE0_M;
		cr->bb_monitor1 = MONITOR1_BE0;
		cr->bb_monitor1_m = MONITOR1_BE0_M;
		cr->path_0_txpw = TXPW_PATH0_TXPW_BE0;
		cr->path_0_txpw_m = TXPW_PATH0_TXPW_BE0_M;
		cr->path_1_txpw = TXPW_PATH1_TXPW_BE0;
		cr->path_1_txpw_m = TXPW_PATH1_TXPW_BE0_M;
		cr->bmode_tx = 0x2380;
		/*mac_phy_intf*/
		cr->mac_phy_intf_sel_phy1 = INTF_R_INTF_RPT_SEL_P1_BE0;
		cr->mac_phy_intf_sel_phy1_m = INTF_R_INTF_RPT_SEL_P1_BE0_M;
		cr->mac_phy_txinfo[0] = 0x1800;
		cr->mac_phy_txinfo[1] = 0x1804;
		cr->mac_phy_txinfo[2] = 0x1808;
		cr->mac_phy_txinfo[3] = 0x180c;
		cr->mac_phy_txinfo[4] = 0x1810;
		cr->mac_phy_txinfo[5] = 0x1814;
		cr->mac_phy_txt2rct[0] = 0x1818;
		cr->mac_phy_txt2rct[1] = 0x181c;
		cr->mac_phy_txcomct[0] = 0x1820;
		cr->mac_phy_txcomct[1] = 0x1824;
		cr->mac_phy_txusrct[0][0] = 0x1828;
		cr->mac_phy_txusrct[0][1] = 0x182c;
		cr->mac_phy_txusrct[1][0] = 0x1830;
		cr->mac_phy_txusrct[1][1] = 0x1834;
		cr->mac_phy_txusrct[2][0] = 0x1838;
		cr->mac_phy_txusrct[2][1] = 0x183c;
		cr->mac_phy_txusrct[3][0] = 0x1840;
		cr->mac_phy_txusrct[3][1] = 0x1844;
		cr->mac_phy_txtimct = 0x1848;
		cr->mac_phy_lsig = 0x1850;
		cr->mac_phy_siga_0 = 0x1858;
		cr->mac_phy_siga_1 = 0x185c;
		cr->mac_phy_vht_sigb_0 = 0x1860;
		cr->mac_phy_usig_1 = 0x1870;
		cr->mac_phy_usig_2 = 0x1874;
		break;
	#endif

	#ifdef HALBB_COMPILE_BE1_SERIES
	case BB_BE1:
		cr->dbgport_ip = DBG_PORT_IP_SEL_BE1;
		cr->dbgport_ip_m = DBG_PORT_IP_SEL_BE1_M;
		cr->dbgport_idx = DBG_PORT_SEL_BE1;
		cr->dbgport_idx_m = DBG_PORT_SEL_BE1_M;
		cr->dbgport_val = DBG32_D_BE1;
		cr->dbgport_val_m = DBG32_D_BE1_M;
		cr->clk_en = DBG_PORT_REF_CLK_EN_BE1;
		cr->clk_en_m = DBG_PORT_REF_CLK_EN_BE1_M;
		cr->dbgport_en = DBG_PORT_EN_BE1;
		cr->dbgport_en_m = DBG_PORT_EN_BE1_M;
		cr->bb_monitor_sel0 = TOP_CTRL0_P0_R_MONITOR_SEL0_BE1;
		cr->bb_monitor_sel0_m = TOP_CTRL0_P0_R_MONITOR_SEL0_BE1_M;
		cr->bb_monitor0 = MONITOR0_BE1;
		cr->bb_monitor0_m = MONITOR0_BE1_M;
		cr->bb_monitor_sel1 = TOP_CTRL0_P0_R_MONITOR_SEL1_BE1;
		cr->bb_monitor_sel1_m = TOP_CTRL0_P0_R_MONITOR_SEL1_BE1_M;
		cr->bb_monitor1 = MONITOR1_BE1;
		cr->bb_monitor1_m = MONITOR1_BE1_M;
		cr->path_0_txpw = 0xee0c;
		cr->path_0_txpw_m = 0x1ff;
		cr->path_1_txpw = 0xef0c;
		cr->path_1_txpw_m = 0x1ff;
		cr->bmode_tx = 0x0580;
		/*mac_phy_intf*/
		cr->mac_phy_intf_sel_phy1 = INTF_R_INTF_RPT_SEL_P1_BE1;
		cr->mac_phy_intf_sel_phy1_m = INTF_R_INTF_RPT_SEL_P1_BE1_M;
		cr->mac_phy_txinfo[0] = 0x3e00;
		cr->mac_phy_txinfo[1] = 0x3e04;
		cr->mac_phy_txinfo[2] = 0x3e08;
		cr->mac_phy_txinfo[3] = 0x3e0c;
		cr->mac_phy_txinfo[4] = 0x3e10;
		cr->mac_phy_txinfo[5] = 0x3e14;
		cr->mac_phy_txt2rct[0] = 0x3e18;
		cr->mac_phy_txt2rct[1] = 0x3e1c;
		cr->mac_phy_txcomct[0] = 0x3e20;
		cr->mac_phy_txcomct[1] = 0x3e24;
		cr->mac_phy_txusrct[0][0] = 0x3e28;
		cr->mac_phy_txusrct[0][1] = 0x3e2c;
		cr->mac_phy_txusrct[1][0] = 0x3e30;
		cr->mac_phy_txusrct[1][1] = 0x3e34;
		cr->mac_phy_txusrct[2][0] = 0x3e38;
		cr->mac_phy_txusrct[2][1] = 0x3e3c;
		cr->mac_phy_txusrct[3][0] = 0x3e40;
		cr->mac_phy_txusrct[3][1] = 0x3e44;
		cr->mac_phy_txtimct = 0x3e48;
		cr->mac_phy_lsig = 0x3e50;
		cr->mac_phy_siga_0 = 0x3e58;
		cr->mac_phy_siga_1 = 0x3e5c;
		cr->mac_phy_vht_sigb_0 = 0x3e60;
		cr->mac_phy_usig_1 = 0x3e70;
		cr->mac_phy_usig_2 = 0x3e74;
		break;
	#endif

	default:
		BB_WARNING("[%s] BBCR Hook FAIL!\n", __func__);
	
		if (bb->bb_dbg_i.cr_fake_init_hook_en) {
			BB_TRACE("[%s] BBCR fake init\n", __func__);
			halbb_cr_hook_fake_init(bb, (u32 *)cr, (sizeof(struct bb_dbg_cr_info) >> 2));
		}
		
		break;
	}

	if (bb->bb_dbg_i.cr_init_hook_recorder_en) {
		BB_TRACE("[%s] BBCR Hook dump\n", __func__);
		halbb_cr_hook_init_dump(bb, (u32 *)cr, (sizeof(struct bb_dbg_cr_info) >> 2));
	}
}

