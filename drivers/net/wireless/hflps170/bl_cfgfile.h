/**
 ****************************************************************************************
 *
 * @file bl_cfgfile.h
 *
 * Copyright (C) BouffaloLab 2017-2018
 *
 ****************************************************************************************
 */

#ifndef _BL_CFGFILE_H_
#define _BL_CFGFILE_H_

/*
 * Structure used to retrieve information from the Config file used at Initialization time
 */
struct bl_conf_file {
    u8 mac_addr[ETH_ALEN];
};

/*
 * Structure used to retrieve information from the PHY Config file used at Initialization time
 */
struct bl_phy_conf_file {
    struct phy_trd_cfg_tag trd;
    struct phy_karst_cfg_tag karst;
};

int bl_parse_configfile(struct bl_hw *bl_hw, const char *filename,
                          struct bl_conf_file *config);

int bl_parse_phy_configfile(struct bl_hw *bl_hw, const char *filename,
                              struct bl_phy_conf_file *config);

#endif /* _BL_CFGFILE_H_ */
