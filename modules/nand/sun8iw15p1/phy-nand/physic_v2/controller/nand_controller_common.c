
/*
 * Copyright (c) 2007-2017 Allwinnertech Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#define _NCTRIC_C_

#include "../nand_physic_inc.h"

extern s32 fill_nctri(struct _nand_controller_info *nctri);

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:ok  -1:fail
*Note         :
*****************************************************************************/
int add_to_nctri(struct _nand_controller_info *node)
{
	struct _nand_controller_info *head = g_nctri;

	if (head == NULL) {
		node->channel_id = 0;
		g_nctri = node;
	} else {
		node->channel_id = 1;
		while (head->next != 0) {
			node->channel_id++;
			head = head->next;
		}
		head->next = node;
	}

	fill_nctri(node);

	return 0;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:ok  -1:fail
*Note         :
*****************************************************************************/
struct _nand_controller_info *nctri_get(struct _nand_controller_info *head,
					unsigned int num)
{
	int i;
	struct _nand_controller_info *nctri;

	for (i = 0, nctri = head; i < num; i++)
		nctri = nctri->next;

	return nctri;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:ok  -1:fail
*Note         :
*****************************************************************************/
int before_update_nctri_interface(struct _nand_controller_info *nctri)
{
	struct _nand_chip_info *nci = nctri->nci;
	s32 ret = 0;
	u32 ddr_type, sdr_edo, ddr_edo, ddr_delay;

	if (nctri->nci->support_toggle_only) {
		/* init toggle ddr interface with classic clk cfg(20MHz) */
		PHY_DBG("%s: ch %d, init toggle interface!\n", __func__,
			nctri->channel_id);
		if ((nctri->nci->interface_type == TOG_DDR) ||
		    (nctri->nci->interface_type == TOG_DDR2)) {
			ret = NAND_SetClk(nctri->channel_id, 20, 20 * 2);
			if (ret) {
				PHY_ERR("%s: init toggle interface with "
					"classic clk cfg(%d, %d) failed!\n",
					__func__, 20, 20 * 2);
				return ERR_NO_48;
			}

			ddr_type = TOG_DDR;
			sdr_edo = 0;
			ddr_edo = 0x2;
			ddr_delay = 0x1f;
			ndfc_change_nand_interface(nctri, ddr_type, sdr_edo,
						   ddr_edo, ddr_delay);
		}

		while (nci != NULL) {
			nctri->ddr_timing_ctl[nci->nctri_chip_no] =
			    (ddr_edo << 8) | ddr_delay;
			nci = nci->nctri_next;
		}
	}

	return 0;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:ok  -1:fail
*Note         :
*****************************************************************************/
int start_update_nctri_interface(struct _nand_controller_info *nctri,
				 u32 input_timing_para, u32 ddr_type,
				 u32 ddr_type_old, u32 sclk0, u32 sclk1,
				 u32 sdr_edo_i, u32 ddr_edo_i, u32 ddr_delay_i)
{
	s32 ret;
	struct _nand_chip_info *nci;
	u32 sdr_edo, ddr_edo, ddr_delay, tmp, i;
	u32 ddr_type_bak, ddr_type_old_bak;

	ddr_type_bak = ddr_type;
	ddr_type_old_bak = ddr_type_old;

	ret = _change_all_nand_parameter(nctri, ddr_type_bak, ddr_type_old_bak,
					 sclk0);
	if (ret) {
		PHY_ERR("_change nand parameter error when use clk %dMhz!\n",
			sclk0);
		return ret;
	}

	ret = NAND_SetClk(nctri->channel_id, sclk0, sclk1);
	if (ret) {
		PHY_ERR("change clk to cfg(%d, %d) failed!\n", sclk0,
			sclk0 * 2);
		return ERR_NO_47;
	}

	if (input_timing_para) {
		sdr_edo = sdr_edo_i;
		ddr_edo = ddr_edo_i;
		ddr_delay = ddr_delay_i;
		ndfc_change_nand_interface(nctri, ddr_type_bak, sdr_edo,
					   ddr_edo, ddr_delay);
	} else {
		/* get right timing parameter. only get timing parameter for
		 * first chip */
		ret = _get_right_timing_para(nctri, ddr_type_bak, &sdr_edo,
					     &ddr_edo, &ddr_delay);
		if (ret < 0) {
			if ((nctri->nci->support_toggle_only == 0) &&
			    (nctri->nci->interface_type != SDR)) {
				nctri->nci->interface_type = SDR;
				ddr_type_bak = nctri->nci->interface_type;
				ddr_type_old_bak = ddr_type;

				for (i = 0; i < 3; i++) {
					// nctri->nci->frequency = 40;
					nctri->nci->frequency =
					    15; // for debug A50 IC by zzm
					PHY_DBG("try timing %d clk %dMHz!\n", i,
						nctri->nci->frequency);
					ret = _change_all_nand_parameter(
					    nctri, ddr_type_bak,
					    ddr_type_old_bak,
					    nctri->nci->frequency);
					if (ret) {
						PHY_ERR("1 _change nand "
							"parameter error when "
							"use clk %dMhz!\n",
							nctri->nci->frequency);
						return ret;
					}

					ret = NAND_SetClk(
					    nctri->channel_id,
					    nctri->nci->frequency,
					    2 * nctri->nci->frequency);
					if (ret) {
						PHY_ERR("1 change clk to "
							"cfg(%d, %d) failed!\n",
							nctri->nci->frequency,
							nctri->nci->frequency *
							    2);
						return ERR_NO_47;
					}
					ret = _get_right_timing_para(
					    nctri, ddr_type_bak, &sdr_edo,
					    &ddr_edo, &ddr_delay);
					if (ret >= 0) {
						break;
					}

					nctri->nci->frequency -= 10;
				}
				if (ret < 0) {
					/*failed, */
					PHY_ERR("get timing para failed 0!\n");
					return ret;
				} else {
					/* successful, set parameter*/
					ndfc_change_nand_interface(
					    nctri, ddr_type_bak, sdr_edo,
					    ddr_edo, ddr_delay);
				}
			} else {
				/*failed, */
				PHY_ERR("get timing para failed! 1\n");
				return ret;
			}
		} else {
			/* successful, set parameter*/
			ndfc_change_nand_interface(nctri, ddr_type_bak, sdr_edo,
						   ddr_edo, ddr_delay);
		}
	}

	if (ddr_type_bak == SDR) {
		tmp = sdr_edo << 8;
	} else {
		tmp = (ddr_edo << 8) | ddr_delay;
	}

	nci = nctri->nci;
	while (nci) {
		nctri->ddr_timing_ctl[nci->nctri_chip_no] = tmp;
		nci = nci->nctri_next;
	}

	if ((ddr_type_bak == ONFI_DDR2) || (ddr_type_bak == TOG_DDR2)) {
#if 0
		/* get nand flash timing mode feature and show*/
		feature_addr = 0x01; //timing mode
		nand_get_nand_feature(nctri->nci, feature_addr, feature);
		PHY_DBG("current nand flash's timing mode is: %d\n", feature[0]);

		if (SUPPORT_DDR2_SPECIFIC_CFG) {
			/* set ndfc's ddr2 para */
			_setup_ndfc_ddr2_para(nctri);
		}

		/* set ndfc's other timing para */
		reg_val = NDFC_READ_REG_CTL(); //cfg for dqs and re
		reg_val |= (0x3U << 29);
		NDFC_WRITE_REG_CTL(reg_val);

		reg_val = NDFC_READ_REG_INT_DEBUG(); //debug reg(addr 0x184)
		reg_val |= (0x1U << 8);
		NDFC_WRITE_REG_INT_DEBUG(reg_val);

		reg_val = 0;             //set timing cfg
		reg_val = 0xffffffff;
		NDFC_WRITE_REG_TIMING_CFG(reg_val);
#endif
	} else if (((ddr_type_old_bak == ONFI_DDR2) ||
		    (ddr_type_old_bak == TOG_DDR2)) &&
		   (ddr_type_bak == SDR)) {
#if 0
		if (SUPPORT_DDR2_SPECIFIC_CFG) {
			NDFC_WRITE_REG_SPEC_CTL(0x0);
		}

		//config for dqs and re
		reg_val = NDFC_READ_REG_CTL();
		reg_val &= (~(0x3U << 29));
		NDFC_WRITE_REG_CTL(reg_val);

		//debug reg
		reg_val = NDFC_READ_REG_INT_DEBUG(); //debug reg(addr 0x184)
		reg_val &= (~(0x1U << 8));
		NDFC_WRITE_REG_INT_DEBUG(reg_val);

		/* configure default timing parameter */
		_set_ndfc_def_timing_param();
#endif
	}

	return 0;
}
/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:ok  -1:fail
*Note         :
*****************************************************************************/
int ddr_mode_to_sdr(struct _nand_controller_info *nctri)
{
	struct _nand_chip_info *nci;
	nci = nctri->nci;

	if (!nci->support_vendor_specific_cfg) {
		return 0;
	}

	while (nci != NULL) {
		_setup_ddr_nand_force_to_sdr_para(nci);
		nci = nci->nctri_next;
	}

	return 0;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:ok  -1:fail
*Note         :
*****************************************************************************/
int update_1st_nctri_interface(struct _nand_controller_info *nctri)
{
	int ret = 0;
	// struct _nand_chip_info *nci;
	unsigned int ddr_type_bak, sdr_edo_bak, ddr_edo_bak, ddr_delay_bak;
	unsigned int ddr_type, sdr_edo, ddr_edo, ddr_delay;
	unsigned int sclk0, sclk1, sclk0_bak, sclk1_bak;

	if (nctri->channel_id != 0) {
		PHY_ERR("wrong nctri %d\n", nctri->channel_id);
		return ERR_NO_47;
	}

	ddr_mode_to_sdr(nctri);

	/* get current interface & access frequency */
	ndfc_get_nand_interface(nctri, &ddr_type_bak, &sdr_edo_bak,
				&ddr_edo_bak, &ddr_delay_bak);
	NAND_GetClk(nctri->channel_id, &sclk0_bak, &sclk1_bak);

	/* new interface type & access frequency */
	ddr_type = nctri->nci->interface_type;
	sclk0 = nctri->nci->frequency;
	sclk1 = 2 * sclk0;

	ret = _check_scan_data(1, 0, nctri->ddr_scan_blk_no, NULL);
	if ((ret < 0) || (ret == 1)) {
		if ((ddr_type == SDR) && (sclk0 < 50)) {
			PHY_DBG("interface change ddr_type:%x sclk0:%d "
				"sclk1:%d sdr_edo_bak:%x ddr_edo_bak:%x "
				"ddr_delay_bak:%x !\n",
				ddr_type, sclk0, sclk1, sdr_edo_bak,
				ddr_edo_bak, ddr_delay_bak);
			ret = start_update_nctri_interface(
			    nctri, 0, ddr_type, ddr_type_bak, sclk0, sclk1,
			    sdr_edo_bak, ddr_edo_bak, ddr_delay_bak);
		} else {
			PHY_DBG("interface  not change ddr_type:%x sclk0:%d "
				"sclk1:%d sdr_edo:%x ddr_edo:%x ddr_delay:%x "
				"!\n",
				ddr_type_bak, sclk0_bak, sclk1_bak, sdr_edo_bak,
				ddr_edo_bak, ddr_delay_bak);
			ret = start_update_nctri_interface(
			    nctri, 1, ddr_type_bak, ddr_type_bak, sclk0_bak,
			    sclk1_bak, sdr_edo_bak, ddr_edo_bak, ddr_delay_bak);
		}
	} else {
		PHY_DBG("interface change ddr_type:%x sclk0:%d sclk1:%d "
			"sdr_edo_bak:%x ddr_edo_bak:%x ddr_delay_bak:%x !\n",
			ddr_type, sclk0, sclk1, sdr_edo_bak, ddr_edo_bak,
			ddr_delay_bak);
		ret = start_update_nctri_interface(
		    nctri, 0, ddr_type, ddr_type_bak, sclk0, sclk1, sdr_edo_bak,
		    ddr_edo_bak, ddr_delay_bak);
	}

	if (ret) {
		PHY_ERR("start update nctri interface  fail --1!\n");
	}

	return ret;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:ok  -1:fail
*Note         :
*****************************************************************************/
int update_other_nctri_interface(struct _nand_controller_info *nctri)
{
	int ret = 0;
	unsigned int timing_change = 0;
	unsigned int ddr_type_bak, sdr_edo_bak, ddr_edo_bak, ddr_delay_bak;
	unsigned int ddr_type, sdr_edo, ddr_edo, ddr_delay;
	unsigned int sclk0, sclk1, sclk0_bak, sclk1_bak;

	if (nctri->channel_id == 0) {
		PHY_ERR("%s: wrong nctri %d\n", nctri->channel_id);
		return ERR_NO_46;
	}

	ddr_mode_to_sdr(nctri);

	// get current configuration
	ndfc_get_nand_interface(nctri, &ddr_type_bak, &sdr_edo_bak,
				&ddr_edo_bak, &ddr_delay_bak);
	NAND_GetClk(nctri->channel_id, &sclk0_bak, &sclk1_bak);

	// get new configuration from channel 0
	ndfc_get_nand_interface(g_nctri, &ddr_type, &sdr_edo, &ddr_edo,
				&ddr_delay);
	NAND_GetClk(g_nctri->channel_id, &sclk0, &sclk1);

	/*  change ? */
	if (ddr_type_bak != ddr_type) {
		timing_change = 1;
		PHY_DBG("%s: nand interface will be changed! %d -> %d\n",
			__func__, ddr_type_bak, ddr_type);
	}

	if (sclk0_bak != sclk0) {
		timing_change = 1;
		PHY_DBG("%s: sclk0 will be changed! %d -> %d\n", __func__,
			sclk0_bak, sclk0);
	}

	if ((nctri->type == NDFC_VERSION_V2) && (sclk1_bak != sclk1)) {
		timing_change = 1;
		PHY_DBG("%s: sclk1 will be changed! %d -> %d\n", __func__,
			sclk1_bak, sclk1);
	}

	PHY_DBG("%s: timing_change %d, sclk0_bak %d, sclk0 %d!\n", __func__,
		timing_change, sclk0_bak, sclk0);

	if (timing_change) {
		ret = start_update_nctri_interface(nctri, 1, ddr_type,
						   ddr_type_bak, sclk0, sclk1,
						   sdr_edo, ddr_edo, ddr_delay);
		if (ret) {
			PHY_ERR("start update nctri_interface fail --0!\n");
		}
	}
	return ret;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:ok  -1:fail
*Note         :
*****************************************************************************/
s32 _update_ext_accessfreq_para(struct _nand_controller_info *nctri)
{
	u32 id_number_ctl;
	u32 script_frequence, para;
	struct _nand_chip_info *nci;

	id_number_ctl = NAND_GetNandIDNumCtrl();

	if ((id_number_ctl & 0x01) != 0) {// bit 0, set freq para
		para = NAND_GetNandExtPara(0);
		if (para == 0xffffffff) {// get script fail
			PHY_ERR("_update_ext accessfreq_para: wrong freq para, "
				"0x%x\n",
				para);
			return ERR_NO_43;
		}

		if (((para & 0xffffff) == nctri->nci->npi->id_number) ||
		    ((para & 0xffffff) == 0xeeeeee)) {
			script_frequence = (para >> 24) & 0xff;
			if ((script_frequence > 10) &&
			    (script_frequence < 100)) {
				nci = nctri->nci;
				while (nci) {
					nci->frequency = script_frequence;
					nci = nci->nctri_next;
				}
				PHY_DBG("_update_ext accessfreq_para: update "
					"freq from script, %d\n",
					script_frequence);
			} else {
				PHY_ERR("_update_ext accessfreq_para: wrong "
					"freq, %d\n",
					script_frequence);
				return ERR_NO_42;
			}
		} else {
			PHY_ERR("_update_ext accessfreq_para: wrong id number, "
				"0x%x/0x%x\n",
				(para & 0xffffff), nctri->nci->npi->id_number);
			return ERR_NO_41;
		}
	} else if ((id_number_ctl & 0x10) != 0) {
		script_frequence = nctri->nci->npi->access_high_freq;
		if ((script_frequence > 10) && (script_frequence < 100)) {
			nci = nctri->nci;
			while (nci) {
				nci->frequency = script_frequence;
				nci = nci->nctri_next;
			}
			PHY_DBG("_update_ext accessfreq_para: update freq "
				"high, %d\n",
				script_frequence);
		} else {
			PHY_ERR("_update_ext accessfreq_para: wrong freq, %d\n",
				script_frequence);
			return ERR_NO_42;
		}
	} else {
		return 0;
	}
	return 0;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:ok  -1:fail
*Note         :
*****************************************************************************/
int set_nand_script_frequence(void)
{
	int ret;
	struct _nand_controller_info *nctri = g_nctri;

	if (SUPPORT_UPDATE_EXTERNAL_ACCESS_FREQ) {
		while (nctri != NULL) {
			ret = _update_ext_accessfreq_para(nctri);
			if (ret != 0) {
				PHY_ERR("update ext accessfreq para fail, "
					"using internal frequency!\n");
			}
			nctri = nctri->next;
		}
	}
	return 0;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:ok  -1:fail
*Note         :
*****************************************************************************/
int update_nctri(struct _nand_controller_info *nctri)
{
	s32 i, ret;
	u32 rb_connect = 0xff;
	u32 chip_cnt = nctri->chip_cnt;
	u32 chip_connect = nctri->chip_connect_info;

	while (nctri != NULL) {
		// change ndfc configuration
		ndfc_change_page_size(nctri);
		ndfc_set_ecc_mode(nctri, nctri->nci->ecc_mode);

		// build physical connection
		MEMSET(nctri->ce, 0xff, MAX_CHIP_PER_CHANNEL);
		MEMSET(nctri->rb, 0xff, MAX_CHIP_PER_CHANNEL);

		if ((chip_cnt == 1) && (chip_connect & (1 << 0))) {
			rb_connect = 1;

			nctri->ce[0] = 0;
			nctri->rb[0] = 0;
		} else if (chip_cnt == 2) {
			if ((chip_connect & (1 << 0)) &&
			    (chip_connect & (1 << 1))) {
				rb_connect = 2;

				nctri->ce[0] = 0;
				nctri->rb[0] = 0;
				nctri->ce[1] = 1;
				nctri->rb[1] = 1;
			} else if ((chip_connect & (1 << 0)) &&
				   (chip_connect & (1 << 2))) {
				rb_connect = 3;

				nctri->ce[0] = 0;
				nctri->rb[0] = 0;
				nctri->ce[1] = 2;
				nctri->rb[1] = 1;
			} else if ((chip_connect & (1 << 0)) &&
				   (chip_connect & (1 << 7))) {
				// special use, only one rb
				rb_connect = 0;

				nctri->ce[0] = 0;
				nctri->rb[0] = 0;
				nctri->ce[1] = 7;
				nctri->rb[1] = 0;
			}
		} else if (chip_cnt == 4) {
			if ((chip_connect & (1 << 0)) &&
			    (chip_connect & (1 << 1)) &&
			    (chip_connect & (1 << 2)) &&
			    (chip_connect & (1 << 3))) {
				rb_connect = 4;

				nctri->ce[0] = 0;
				nctri->rb[0] = 0;
				nctri->ce[1] = 1;
				nctri->rb[1] = 0;
				nctri->ce[2] = 2;
				nctri->rb[2] = 1;
				nctri->ce[3] = 3;
				nctri->rb[3] = 1;
			} else if ((chip_connect & (1 << 0)) &&
				   (chip_connect & (1 << 2)) &&
				   (chip_connect & (1 << 4)) &&
				   (chip_connect & (1 << 6))) {
				rb_connect = 5;

				nctri->ce[0] = 0;
				nctri->rb[0] = 0;
				nctri->ce[1] = 2;
				nctri->rb[1] = 1;
				nctri->ce[2] = 4;
				nctri->rb[2] = 0;
				nctri->ce[3] = 6;
				nctri->rb[3] = 1;
			}
		} else if (chip_cnt == 8) {
			rb_connect = 8;

			nctri->ce[0] = 0;
			nctri->rb[0] = 0;
			nctri->ce[1] = 1;
			nctri->rb[1] = 0;
			nctri->ce[2] = 2;
			nctri->rb[2] = 1;
			nctri->ce[3] = 3;
			nctri->rb[3] = 1;

			nctri->ce[4] = 4;
			nctri->rb[4] = 0;
			nctri->ce[5] = 5;
			nctri->rb[5] = 0;
			nctri->ce[6] = 6;
			nctri->rb[6] = 1;
			nctri->ce[7] = 7;
			nctri->rb[7] = 1;
		} else {
			return ERR_NO_40;
		}

		if (rb_connect == 0xff) {
			PHY_ERR("update_nctri: build nand rb connect fail, "
				"chip_cnt=%d, chip_connect=0x%x\n",
				chip_cnt, chip_connect);
			return ERR_NO_39;
		} else {
			nctri->rb_connect_info = rb_connect;
			PHY_DBG("build nand physical connection ok...\n");
			PHY_DBG("CE: ");
			for (i = 0; i < chip_cnt; i++)
				PHY_DBG("%d ", nctri->ce[i]);

			PHY_DBG("\nRB: ");
			for (i = 0; i < chip_cnt; i++)
				PHY_DBG("%d ", nctri->rb[i]);
			PHY_DBG("\n");
		}

		ret = before_update_nctri_interface(nctri);
		if (ret) {
			PHY_ERR("before_update nctri nand interface fail!\n");
			return ret;
		}
		if (nctri->channel_id == 0) {
			ret = update_1st_nctri_interface(nctri);
			if (ret) {
				PHY_ERR("update 1st nctri interface fail!\n");
				return ret;
			}
		} else {
			ret = update_other_nctri_interface(nctri);
			if (ret) {
				PHY_ERR("update other nctri interface fail!\n");
				return ret;
			}
		}
		nctri = nctri->next;
	}

	return 0;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:ok  -1:fail
*Note         :
*****************************************************************************/
int check_nctri(struct _nand_controller_info *nctri)
{
	int ch;
	unsigned int id_number, chip_connect;
	struct _nand_controller_info *nctri_tmp;
	struct _nand_chip_info *nci;

	for (ch = MAX_CHANNEL - 1; ch >= 0; ch--) {
		nctri_tmp = nctri_get(nctri, ch);
		if ((nctri_tmp != NULL) && (nctri_tmp->chip_cnt == 0)) {
			FREE(nctri_tmp, sizeof(struct _nand_controller_info));
			if (ch >= 1) {
				nctri_tmp = nctri_get(nctri, ch - 1);
				nctri_tmp->next = NULL;
				PHY_DBG("null channel %d\n", ch);
			} else {
				PHY_ERR("error, channel 0 is null!!!!\n");
				return ERR_NO_38;
			}
		}
	}

	while (nctri != NULL) {
		if (nctri->channel_id == 0) {
			id_number = nctri->nci->npi->id_number;
			chip_connect = nctri->chip_connect_info;
		} else {
			id_number = g_nctri->nci->npi->id_number;
			chip_connect = g_nctri->chip_connect_info;
		}

		if ((nctri->chip_connect_info != chip_connect) &&
		    (nctri->chip_cnt != 0)) {
			PHY_ERR("check_nctri, different chip_connect %d %d\n",
				chip_connect, nctri->chip_connect_info);
			return ERR_NO_37;
		}

		// all chips are same ?
		nci = nctri->nci;
		while (nci != NULL) {
			if (nci->npi->id_number != id_number) {
				PHY_ERR("check_nctri, different id number\n");
				return ERR_NO_36;
			}
			nci = nci->nctri_next;
		}

		nctri = nctri->next;
	}

	return 0;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:ok  -1:fail
*Note         :
*****************************************************************************/
int update_boot0_nctri_interface(struct _nand_controller_info *nctri)
{
	int ret = 0;
	// struct _nand_chip_info *nci;
	unsigned int ddr_type_bak, sdr_edo_bak, ddr_edo_bak, ddr_delay_bak;
	unsigned int ddr_type, sdr_edo, ddr_edo, ddr_delay;
	unsigned int sclk0, sclk1, sclk0_bak, sclk1_bak;

	if (nctri->channel_id != 0) {
		PHY_ERR("wrong nctri %d\n", nctri->channel_id);
		return ERR_NO_47;
	}

	/* get current interface & access frequency */
	ndfc_get_nand_interface(nctri, &ddr_type_bak, &sdr_edo_bak,
				&ddr_edo_bak, &ddr_delay_bak);
	NAND_GetClk(nctri->channel_id, &sclk0_bak, &sclk1_bak);

	/* new interface type & access frequency */
	ddr_type = 0x0; // SDR interface
	sclk0 = 15;     // 30MHz
	sclk1 = 2 * sclk0;

	ret = start_update_nctri_interface(nctri, 0, ddr_type, ddr_type_bak,
					   sclk0, sclk1, sdr_edo_bak,
					   ddr_edo_bak, ddr_delay_bak);
	if (ret) {
		PHY_ERR("update write boot0 nctri interface  fail --1!\n");
	}

	return ret;
}
