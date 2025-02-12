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

#ifdef HALBB_AUTO_DBG_SUPPORT
void halbb_query_hang_info(struct bb_info *bb, struct bb_stat_hang_info *rpt)
{
	BB_DBG(bb, DBG_AUTO_DBG, "[%s]\n", __func__);
	halbb_mem_cpy(bb, rpt, &bb->bb_stat_i.bb_stat_hang_i, sizeof(struct bb_stat_hang_info));
}

void halbb_query_pmac_info(struct bb_info *bb, struct bb_bkp_pmac_info *rpt)
{
	BB_DBG(bb, DBG_AUTO_DBG, "[%s]\n", __func__);
	halbb_mem_cpy(bb, rpt, &bb->bb_auto_dbg_i.bb_bkp_pmac_i, sizeof(struct bb_bkp_pmac_info));
}

void halbb_query_phy_utility_info(struct bb_info *bb, struct bb_bkp_phy_utility_info *rpt)
{
	BB_DBG(bb, DBG_AUTO_DBG, "[%s]\n", __func__);
	halbb_mem_cpy(bb, rpt, &bb->bb_auto_dbg_i.bb_bkp_phy_utility_i, sizeof(struct bb_bkp_phy_utility_info));
}

void halbb_store_phy_utility(struct bb_info *bb)
{
	struct bb_auto_dbg_info *a_dbg = &bb->bb_auto_dbg_i;
	struct bb_bkp_phy_utility_info *bkp_phy_utility = &a_dbg->bb_bkp_phy_utility_i;

	BB_DBG(bb, DBG_AUTO_DBG, "[%s]\n", __func__);

	bkp_phy_utility->rx_utility = bb->bb_link_i.rx_utility;
	bkp_phy_utility->avg_phy_rate = bb->bb_link_i.avg_phy_rate;
	bkp_phy_utility->rx_rate_plurality = bb->bb_link_i.rx_rate_plurality;
	bkp_phy_utility->tx_rate = bb->bb_link_i.tx_rate;

	halbb_mem_cpy(bb, &bkp_phy_utility->bb_physts_avg_i, &bb->bb_cmn_rpt_i.bb_physts_avg_i, sizeof(struct bb_physts_avg_info));
#if 0
	BB_DBG(bb, DBG_AUTO_DBG, "tx_rate=0x%x, avg_phy_rate=0x%x, rx_rate_plurality=0x%x, rx_utility=%d\n",
	    bkp_phy_utility->tx_rate, bkp_phy_utility->avg_phy_rate, bkp_phy_utility->rx_rate_plurality,
	    bkp_phy_utility->rx_utility);
	BB_DBG(bb, DBG_AUTO_DBG, "evm_min=%d, evm_max=%d, evm_1ss=%d\n",
	    bkp_phy_utility->bb_physts_avg_i.evm_min, bkp_phy_utility->bb_physts_avg_i.evm_max,
	    bkp_phy_utility->bb_physts_avg_i.evm_1ss);
#endif
}

void halbb_store_pmac_info(struct bb_info *bb)
{
	struct bb_auto_dbg_info *a_dbg = &bb->bb_auto_dbg_i;
	struct bb_bkp_pmac_info *bkp_pmac = &a_dbg->bb_bkp_pmac_i;
	struct bb_stat_info *stat_t = &bb->bb_stat_i;

	BB_DBG(bb, DBG_AUTO_DBG, "[%s]\n", __func__);

	halbb_mem_cpy(bb, &bkp_pmac->bb_tx_cnt_i, &stat_t->bb_tx_cnt_i, sizeof(struct bb_tx_cnt_info));
	halbb_mem_cpy(bb, &bkp_pmac->bb_cca_i, &stat_t->bb_cca_i, sizeof(struct bb_cca_info));
	halbb_mem_cpy(bb, &bkp_pmac->bb_crc_i, &stat_t->bb_crc_i, sizeof(struct bb_crc_info));
	halbb_mem_cpy(bb, &bkp_pmac->bb_crc2_i, &stat_t->bb_crc2_i, sizeof(struct bb_crc2_info));
	halbb_mem_cpy(bb, &bkp_pmac->bb_fa_i, &stat_t->bb_fa_i, sizeof(struct bb_fa_info));
	halbb_mem_cpy(bb, &bkp_pmac->bb_usr_set_i, &stat_t->bb_usr_set_i, sizeof(struct bb_usr_set_info));
}

void halbb_auto_debug_en(struct bb_info *bb, enum bb_auto_dbg_t type, bool en)
{
	struct bb_auto_dbg_info *a_dbg = &bb->bb_auto_dbg_i;

	if (en)
		a_dbg->auto_dbg_type |= (u32)BIT(type);
	else
		a_dbg->auto_dbg_type &= (u32)~(BIT(type));

	if (a_dbg->auto_dbg_type)
		bb->support_ability |= BB_AUTO_DBG;
	else
		bb->support_ability &= ~BB_AUTO_DBG;

	BB_DBG(bb, DBG_AUTO_DBG, "[%s] en=%d, type=0x%x, auto_dbg_type=0x%x, dbg_comp=0x%llx\n",
	       __func__, en, type, a_dbg->auto_dbg_type, bb->dbg_component);
}

void halbb_auto_chk_hang_reset(struct bb_info *bb)
{
	struct bb_auto_dbg_info *a_dbg = &bb->bb_auto_dbg_i;
	struct bb_chk_hang_info *chk_hang = &a_dbg->bb_chk_hang_i;
	
	halbb_mem_set(bb, chk_hang->dbg_port_val, 0, chk_hang->table_size);
}

void halbb_auto_chk_hang(struct bb_info *bb)
{
	struct bb_auto_dbg_info *a_dbg = &bb->bb_auto_dbg_i;
	struct bb_chk_hang_info *chk_hang = &a_dbg->bb_chk_hang_i;
	u32 dbg_port = 0;
	u32 dbg_port_value = 0;
	u32 i = 0;

	BB_DBG(bb, DBG_AUTO_DBG, "[%s]\n", __func__);

	/*=== Get check hang Information ===============================*/
	/*Get packet counter Report*/

	/*Get BB Register*/

	/*Get RF Register*/

	/*Get Debug Port*/
	for (i = 0; i < chk_hang->table_size; i++) {
		dbg_port = chk_hang->dbg_port_table[i];
		if (halbb_bb_dbg_port_racing(bb, DBGPORT_PRI_3)) {
			halbb_set_bb_dbg_port_ip(bb, (dbg_port & 0xff0000) >> 16);
			halbb_set_bb_dbg_port(bb, dbg_port & 0xffff);
			dbg_port_value = halbb_get_bb_dbg_port_val(bb);
			halbb_release_bb_dbg_port(bb);

			BB_DBG(bb, DBG_AUTO_DBG, "dbg_port[0x%x]=(0x%x)\n",
			       dbg_port, dbg_port_value);
		}  else {
			BB_DBG(bb, DBG_AUTO_DBG, "Dbg_port Racing Fail!\n");
			return;
		}
	}

	/*=== Make check hang decision ===============================*/
	BB_DBG(bb, DBG_AUTO_DBG, "Check Hang Decision\n");


	halbb_auto_chk_hang_reset(bb);
}

void halbb_auto_chk_hang_init(struct bb_info *bb)
{
	struct bb_auto_dbg_info *a_dbg = &bb->bb_auto_dbg_i;
	struct bb_chk_hang_info *chk_hang = &a_dbg->bb_chk_hang_i;
	u32 dbg_port_table[] = {0x0, 0x803, 0x208, 0xab0};
	u32 table_size = sizeof(dbg_port_table);

	chk_hang->table_size = table_size;
	chk_hang->dbg_port_table = halbb_mem_alloc(bb, table_size);
	halbb_mem_cpy(bb, chk_hang->dbg_port_table, dbg_port_table, table_size);
	
	chk_hang->dbg_port_val= halbb_mem_alloc(bb, table_size);
	a_dbg->auto_dbg_type |= AUTO_DBG_CHECK_HANG;
}

void halbb_auto_debug_watchdog(struct bb_info *bb)
{
	struct bb_auto_dbg_info *a_dbg = &bb->bb_auto_dbg_i;

	if (!(bb->support_ability & BB_AUTO_DBG))
		return;

#ifdef BB_AUTO_CHK_HALNG
	/*check hang*/
	if (a_dbg->auto_dbg_type & AUTO_DBG_CHECK_HANG)
		halbb_auto_chk_hang(bb);
#endif
	if (a_dbg->auto_dbg_type & AUTO_DBG_STORE_PMAC)
		halbb_store_pmac_info(bb);

	if (a_dbg->auto_dbg_type & AUTO_DBG_PHY_UTILITY)
		halbb_store_phy_utility(bb);
}

void halbb_auto_debug_init(struct bb_info *bb)
{
	struct bb_auto_dbg_info *a_dbg = &bb->bb_auto_dbg_i;
	/*a_dbg->auto_dbg_type = AUTO_DBG_STORE_PMAC | AUTO_DBG_PHY_UTILITY;*/

	/*check hang*/
	#ifdef BB_AUTO_CHK_HALNG
	halbb_auto_chk_hang_init(bb);
	#endif
	/*check RX Part*/
	/*check TX Part*/
}

void halbb_auto_debug_pmac_print(struct bb_info *bb, u32 *_used,
				   char *output, u32 *_out_len, struct bb_bkp_pmac_info *pmac_rpt)
{
#ifdef HALBB_STATISTICS_SUPPORT
	//struct bb_stat_info *pmac_rpt = &bb->bb_stat_i;
	struct bb_fa_info *fa = &pmac_rpt->bb_fa_i;
	struct bb_cck_fa_info *cck_fa = &fa->bb_cck_fa_i;
	struct bb_legacy_fa_info *legacy_fa = &fa->bb_legacy_fa_i;
	struct bb_ht_fa_info *ht_fa = &fa->bb_ht_fa_i;
	struct bb_vht_fa_info *vht_fa = &fa->bb_vht_fa_i;
	struct bb_he_fa_info *he_fa = &fa->bb_he_fa_i;
	struct bb_cca_info *cca = &pmac_rpt->bb_cca_i;
	struct bb_crc_info *crc = &pmac_rpt->bb_crc_i;
	//struct bb_crc2_info *crc2 = &stat_t->bb_crc2_i;

	BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
		    "[Tx]{CCK_TxEN, CCK_TxON, OFDM_TxEN, OFDM_TxON}: {%d, %d, %d, %d}\n",
		    pmac_rpt->bb_tx_cnt_i.cck_mac_txen, pmac_rpt->bb_tx_cnt_i.cck_phy_txon,
		    pmac_rpt->bb_tx_cnt_i.ofdm_mac_txen,
		    pmac_rpt->bb_tx_cnt_i.ofdm_phy_txon);
	BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
		    "[CRC]{B/G/N/AC/AX/All/MPDU} OK:{%d, %d, %d, %d, %d, %d, %d} Err:{%d, %d, %d, %d, %d, %d, %d}\n",
		    crc->cnt_cck_crc32_ok, crc->cnt_ofdm_crc32_ok,
		    crc->cnt_ht_crc32_ok, crc->cnt_vht_crc32_ok,
		    crc->cnt_he_crc32_ok, crc->cnt_crc32_ok_all,
		    crc->cnt_ampdu_crc_ok, crc->cnt_cck_crc32_error,
		    crc->cnt_ofdm_crc32_error, crc->cnt_ht_crc32_error,
		    crc->cnt_vht_crc32_error, crc->cnt_he_crc32_error,
		    crc->cnt_crc32_error_all, crc->cnt_ampdu_crc_error);
	BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
		    "[CCA]{CCK, OFDM, All}: %d, %d, %d\n",
		    cca->cnt_cck_cca, cca->cnt_ofdm_cca, cca->cnt_cca_all);
	BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
		    "[FA]{CCK, OFDM, All}: %d, %d, %d\n",
		    fa->cnt_cck_fail, fa->cnt_ofdm_fail, fa->cnt_fail_all);
	BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
		    " *[CCK]sfd/sig_GG=%d/%d, *[OFDM]Prty=%d, Rate=%d, LSIG_brk_s/l=%d/%d, SBD=%d\n",
		    cck_fa->sfd_gg_cnt, cck_fa->sig_gg_cnt,
		    legacy_fa->cnt_parity_fail, legacy_fa->cnt_rate_illegal,
		    legacy_fa->cnt_lsig_brk_s_th, legacy_fa->cnt_lsig_brk_l_th,
		    legacy_fa->cnt_sb_search_fail);
	BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
		    " *[HT]CRC8=%d, MCS=%d, *[VHT]SIGA_CRC8=%d, MCS=%d\n",
		    ht_fa->cnt_crc8_fail, ht_fa->cnt_mcs_fail,
		    vht_fa->cnt_crc8_fail_vhta, vht_fa->cnt_mcs_fail_vht);
	BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
		    " *[HE]SIGA_CRC4{SU/ERSU/MU}=%d/%d/%d, SIGB_CRC4{ch1/ch2}=%d/%d, MCS{nrml/bcc/dcm}=%d/%d/%d\n",
		    he_fa->cnt_crc4_fail_hea_su, he_fa->cnt_crc4_fail_hea_ersu,
		    he_fa->cnt_crc4_fail_hea_mu, he_fa->cnt_crc4_fail_heb_ch1_mu,
		    he_fa->cnt_crc4_fail_heb_ch2_mu, he_fa->cnt_mcs_fail_he,
		    he_fa->cnt_mcs_fail_he_bcc, he_fa->cnt_mcs_fail_he_dcm);
#endif
}

void halbb_auto_debug_pmac_print_2(struct bb_info *bb, u32 *_used,
				   char *output, u32 *_out_len, struct bb_bkp_pmac_info *pmac_rpt)
{
	//struct bb_stat_info *stat_t = &bb->bb_stat_i;
	struct bb_crc2_info *crc2 = &pmac_rpt->bb_crc2_i;
	struct bb_usr_set_info *usr_set = &pmac_rpt->bb_usr_set_i;
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

	BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
		    "[CRC32 OK Cnt] {%s, %s, %s, %s}= {%d, %d, %d, %d}\n",
		    dbg_buf[0], dbg_buf[1], dbg_buf[2], dbg_buf[3],
		    crc2->cnt_ofdm2_crc32_ok, crc2->cnt_ht2_crc32_ok,
		    crc2->cnt_vht2_crc32_ok, crc2->cnt_he2_crc32_ok);

	BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
		    "[CRC32 Err Cnt] {%s, %s, %s , %s}= {%d, %d, %d, %d}\n",
		    dbg_buf[0], dbg_buf[1], dbg_buf[2], dbg_buf[3],
		    crc2->cnt_ofdm2_crc32_error, crc2->cnt_ht2_crc32_error,
		    crc2->cnt_vht2_crc32_error, crc2->cnt_he2_crc32_error);
}

void halbb_auto_debug_pmac_print_3(struct bb_info *bb, u32 *_used,
				   char *output, u32 *_out_len, struct bb_bkp_pmac_info *pmac_rpt)
{
	//struct bb_stat_info *stat_t = &bb->bb_stat_i;
	struct bb_usr_set_info *usr_set = &pmac_rpt->bb_usr_set_i;
	struct bb_crc2_info *crc2 = &pmac_rpt->bb_crc2_i;

	u32 total_cnt = 0;
	u8 pcr = 0;
	total_cnt = crc2->cnt_ofdm3_crc32_ok + crc2->cnt_ofdm3_crc32_error;
	pcr = (u8)HALBB_DIV(crc2->cnt_ofdm3_crc32_ok * 100, total_cnt);

	switch(usr_set->stat_type_sel_i) {
	case STATE_PROBE_RESP:
		BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
			    "[Probe Response Data CRC32 Cnt(OFDM only)] {error, ok}= {%d, %d} (PCR=%d percent)\n",
			    crc2->cnt_ofdm3_crc32_error,
			    crc2->cnt_ofdm3_crc32_ok, pcr);
		break;
	case STATE_BEACON:
		BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
			    "[Beacon CRC32 Cnt(OFDM only)] {error, ok}= {%d, %d} (PCR=%d percent)\n",
			    crc2->cnt_ofdm3_crc32_error,
			    crc2->cnt_ofdm3_crc32_ok, pcr);
		break;
	case STATE_ACTION:
		BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
			    "[Action CRC32 Cnt(OFDM only)] {error, ok}= {%d, %d} (PCR=%d percent)\n",
			    crc2->cnt_ofdm3_crc32_error,
			    crc2->cnt_ofdm3_crc32_ok, pcr);
		break;
	case STATE_BFRP:
		BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
			    "[BFRP CRC32 Cnt(OFDM only)] {error, ok}= {%d, %d} (PCR=%d percent)\n",
			    crc2->cnt_ofdm3_crc32_error,
			    crc2->cnt_ofdm3_crc32_ok, pcr);
		break;
	case STATE_NDPA:
		BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
			    "[NDPA CRC32 Cnt(OFDM only)] {error, ok}= {%d, %d} (PCR=%d percent)\n",
			    crc2->cnt_ofdm3_crc32_error,
			    crc2->cnt_ofdm3_crc32_ok, pcr);
		break;
	case STATE_BA:
		BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
			    "[BA CRC32 Cnt(OFDM only)] {error, ok}= {%d, %d} (PCR=%d percent)\n",
			    crc2->cnt_ofdm3_crc32_error,
			    crc2->cnt_ofdm3_crc32_ok, pcr);
		break;
	case STATE_RTS:
		BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
			    "[RTS CRC32 Cnt(OFDM only)] {error, ok}= {%d, %d} (PCR=%d percent)\n",
			    crc2->cnt_ofdm3_crc32_error,
			    crc2->cnt_ofdm3_crc32_ok, pcr);
		break;
	case STATE_CTS:
		BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
			    "[CTS CRC32 Cnt(OFDM only)] {error, ok}= {%d, %d} (PCR=%d percent)\n",
			    crc2->cnt_ofdm3_crc32_error,
			    crc2->cnt_ofdm3_crc32_ok, pcr);
		break;
	case STATE_ACK:
		BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
			    "[ACK CRC32 Cnt(OFDM only)] {error, ok}= {%d, %d} (PCR=%d percent)\n",
			    crc2->cnt_ofdm3_crc32_error,
			    crc2->cnt_ofdm3_crc32_ok, pcr);
		break;
	case STATE_DATA:
		BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
			    "[DATA CRC32 Cnt(OFDM only)] {error, ok}= {%d, %d} (PCR=%d percent)\n",
			    crc2->cnt_ofdm3_crc32_error,
			    crc2->cnt_ofdm3_crc32_ok, pcr);
		break;
	case STATE_NULL:
		BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
			    "[Null CRC32 Cnt(OFDM only)] {error, ok}= {%d, %d} (PCR=%d percent)\n",
			    crc2->cnt_ofdm3_crc32_error,
			    crc2->cnt_ofdm3_crc32_ok, pcr);
		break;
	case STATE_QOS:
		BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
			    "[QoS CRC32 Cnt(OFDM only)] {error, ok}= {%d, %d} (PCR=%d percent)\n",
			    crc2->cnt_ofdm3_crc32_error,
			    crc2->cnt_ofdm3_crc32_ok, pcr);
		break;
	default:
		break;
	}
}


void halbb_auto_debug_dbg(struct bb_info *bb, char input[][16], 
			  u32 *_used, char *output, u32 *_out_len)
{
	struct bb_bkp_phy_utility_info phy_rpt;
	struct bb_bkp_pmac_info pmac_rpt;
	struct bb_stat_hang_info hang_rpt;
	u32 val[5] = {0};

	if (_os_strcmp(input[1], "-h") == 0) {
		BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
			 "type {0:chk_hang, 1:chk_tx, 2:PMAC, 3:phy_utility} {en}\n");
		BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
			 "query {0:phy_utility, 1: PMAC}\n");

		return;
	}

	if (_os_strcmp(input[1], "type") == 0) {
		HALBB_SCAN(input[2], DCMD_DECIMAL, &val[0]);
		HALBB_SCAN(input[3], DCMD_DECIMAL, &val[1]);
	
		halbb_auto_debug_en(bb, (enum bb_auto_dbg_t)val[0], (bool)val[1]);
		BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
			    "type_idx=0x%x en=%d\n", val[0], val[1]);
	} else if (_os_strcmp(input[1], "query") == 0) {
		HALBB_SCAN(input[2], DCMD_DECIMAL, &val[0]);

		if (val[0] == 3) {
			halbb_query_phy_utility_info(bb, &phy_rpt);
			BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
			    "tx_rate=0x%x, avg_phy_rate=0x%x, rx_rate_plurality=0x%x, rx_utility=%d\n",
			    phy_rpt.tx_rate, phy_rpt.avg_phy_rate, phy_rpt.rx_rate_plurality,
			    phy_rpt.rx_utility);
			BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
			    "evm_min=%d, evm_max=%d, evm_1ss=%d\n",
			    phy_rpt.bb_physts_avg_i.evm_min, phy_rpt.bb_physts_avg_i.evm_max,
			    phy_rpt.bb_physts_avg_i.evm_1ss);
		} else if (val[0] == 2){
			halbb_query_pmac_info(bb, &pmac_rpt);
			halbb_auto_debug_pmac_print(bb, _used, output, _out_len, &pmac_rpt);
			halbb_auto_debug_pmac_print_2(bb, _used, output, _out_len, &pmac_rpt);
			halbb_auto_debug_pmac_print_3(bb, _used, output, _out_len, &pmac_rpt);
		} else if (val[0] == 0) {
			halbb_query_hang_info(bb, &hang_rpt);
			BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
			    "consecutive_no_tx_cnt=%d, consecutive_no_rx_cnt=%d, hang_occur=%d\n",
			    hang_rpt.consecutive_no_tx_cnt,
			    hang_rpt.consecutive_no_rx_cnt,
			    hang_rpt.hang_occur);
		} else {
			BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
			    "Err\n");
		}
	}
}
#endif
