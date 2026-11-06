/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2022. All rights reserved.
 *
 * File Name:reg_blocks_csi_mac.h
 * Description:HW register description
 */

#ifndef _REG_BLOCKS_CSI_MAC_H_
#define _REG_BLOCKS_CSI_MAC_H_

#ifdef __cplusplus
extern "C" {
#endif

/******************************************/
/*           module definition            */
/******************************************/
struct reg_sensor_mac_t {
	union reg_sensor_mac_00                 reg_00;
	uint32_t                                _resv_0x4[3];
	union reg_sensor_mac_10                 reg_10;
	union reg_sensor_mac_14                 reg_14;
	union reg_sensor_mac_18                 reg_18;
	union reg_sensor_mac_1c                 reg_1c;
	union reg_sensor_mac_20                 reg_20;
	union reg_sensor_mac_24                 reg_24;
	union reg_sensor_mac_28                 reg_28;
	uint32_t                                _resv_0x2c[1];
	union reg_sensor_mac_30                 reg_30;
	union reg_sensor_mac_34                 reg_34;
	uint32_t                                _resv_0x38[2];
	union reg_sensor_mac_40                 reg_40;
	union reg_sensor_mac_44                 reg_44;
	union reg_sensor_mac_48                 reg_48;
	union reg_sensor_mac_4c                 reg_4c;
	union reg_sensor_mac_50                 reg_50;
	union reg_sensor_mac_54                 reg_54;
	union reg_sensor_mac_58                 reg_58;
	uint32_t                                _resv_0x5c[1];
	union reg_sensor_mac_60                 reg_60;
	union reg_sensor_mac_64                 reg_64;
	union reg_sensor_mac_68                 reg_68;
	union reg_sensor_mac_6c                 reg_6c;
	union reg_sensor_mac_70                 reg_70;
	union reg_sensor_mac_74                 reg_74;
	uint32_t                                _resv_0x78[2];
	union reg_sensor_mac_80                 reg_80;
	union reg_sensor_mac_84                 reg_84;
	union reg_sensor_mac_88                 reg_88;
	union reg_sensor_mac_8c                 reg_8c;
	union reg_sensor_mac_90                 reg_90;
	union reg_sensor_mac_94                 reg_94;
	union reg_sensor_mac_98                 reg_98;
	union reg_sensor_mac_9c                 reg_9c;
	union reg_sensor_mac_a0                 reg_a0;
	union reg_sensor_mac_a4                 reg_a4;
	union reg_sensor_mac_a8                 reg_a8;
	uint32_t                                _resv_0xac[1];
	union reg_sensor_mac_b0                 reg_b0;
	union reg_sensor_mac_b4                 reg_b4;
	union reg_sensor_mac_b8                 reg_b8;
	union reg_sensor_mac_bc                 reg_bc;
	union reg_sensor_mac_c0                 reg_c0;
	union reg_sensor_mac_c4                 reg_c4;
	union reg_sensor_mac_c8                 reg_c8;
	uint32_t                                _resv_0xcc[1];
	union reg_sensor_mac_d0                 reg_d0;
	union reg_sensor_mac_d4                 reg_d4;
	union reg_sensor_mac_d8                 reg_d8;
	union reg_sensor_mac_dc                 reg_dc;
	union reg_sensor_mac_e0                 reg_e0;
	union reg_sensor_mac_e4                 reg_e4;
	union reg_sensor_mac_e8                 reg_e8;
	union reg_sensor_mac_ec                 reg_ec;
	union reg_sensor_mac_f0                 reg_f0;
	union reg_sensor_mac_f4                 reg_f4;
	union reg_sensor_mac_f8                 reg_f8;
	union reg_sensor_mac_fc                 reg_fc;
	union reg_sensor_mac_100                reg_100;
	union reg_sensor_mac_104                reg_104;
	union reg_sensor_mac_108                reg_108;
	uint32_t                                _resv_0x10c[1];
	union reg_sensor_mac_110                reg_110;
	union reg_sensor_mac_114                reg_114;
	union reg_sensor_mac_118                reg_118;
	union reg_sensor_mac_11c                reg_11c;
	union reg_sensor_mac_120                reg_120;
	union reg_sensor_mac_124                reg_124;
};

/******************************************/
/*           module definition            */
/******************************************/
struct reg_sublvds_ctrl_top_t {
	union reg_sublvds_ctrl_top_00           reg_00;
	union reg_sublvds_ctrl_top_04           reg_04;
	union reg_sublvds_ctrl_top_08           reg_08;
	union reg_sublvds_ctrl_top_0c           reg_0c;
	union reg_sublvds_ctrl_top_10           reg_10;
	union reg_sublvds_ctrl_top_14           reg_14;
	union reg_sublvds_ctrl_top_18           reg_18;
	union reg_sublvds_ctrl_top_1c           reg_1c;
	union reg_sublvds_ctrl_top_20           reg_20;
	union reg_sublvds_ctrl_top_24           reg_24;
	union reg_sublvds_ctrl_top_28           reg_28;
	union reg_sublvds_ctrl_top_2c           reg_2c;
	union reg_sublvds_ctrl_top_30           reg_30;
	uint32_t                                _resv_0x34[3];
	union reg_sublvds_ctrl_top_40           reg_40;
	uint32_t                                _resv_0x44[3];
	union reg_sublvds_ctrl_top_50           reg_50;
	union reg_sublvds_ctrl_top_54           reg_54;
	union reg_sublvds_ctrl_top_58           reg_58;
	uint32_t                                _resv_0x5c[1];
	union reg_sublvds_ctrl_top_60           reg_60;
	union reg_sublvds_ctrl_top_64           reg_64;
	union reg_sublvds_ctrl_top_68           reg_68;
	union reg_sublvds_ctrl_top_6c           reg_6c;
	union reg_sublvds_ctrl_top_70           reg_70;
	union reg_sublvds_ctrl_top_74           reg_74;
	uint32_t                                _resv_0x78[2];
	union reg_sublvds_ctrl_top_80           reg_80;
};

/******************************************/
/*           module definition            */
/******************************************/
struct reg_csi_ctrl_top_t {
	union reg_csi_ctrl_top_00               reg_00;
	union reg_csi_ctrl_top_04               reg_04;
	union reg_csi_ctrl_top_08               reg_08;
	union reg_csi_ctrl_top_0c               reg_0c;
	union reg_csi_ctrl_top_10               reg_10;
	union reg_csi_ctrl_top_14               reg_14;
	union reg_csi_ctrl_top_18               reg_18;
	union reg_csi_ctrl_top_1c               reg_1c;
	union reg_csi_ctrl_top_20               reg_20;
	union reg_csi_ctrl_top_24               reg_24;
	uint32_t                                _resv_0x28[2];
	union reg_csi_ctrl_top_30               reg_30;
	union reg_csi_ctrl_top_34               reg_34;
	uint32_t                                _resv_0x38[2];
	union reg_csi_ctrl_top_40               reg_40;
	uint32_t                                _resv_0x44[1];
	union reg_csi_ctrl_top_48               reg_48;
	union reg_csi_ctrl_top_4c               reg_4c;
	union reg_csi_ctrl_top_50               reg_50;
	union reg_csi_ctrl_top_54               reg_54;
	union reg_csi_ctrl_top_58               reg_58;
	union reg_csi_ctrl_top_5c               reg_5c;
	union reg_csi_ctrl_top_60               reg_60;
	union reg_csi_ctrl_top_64               reg_64;
	uint32_t                                _resv_0x68[2];
	union reg_csi_ctrl_top_70               reg_70;
	union reg_csi_ctrl_top_74               reg_74;
};

/******************************************/
/*           module definition            */
/******************************************/
struct reg_sensor_mac_vi_t {
	union reg_sensor_mac_vi_00              reg_00;
	uint32_t                                _resv_0x4[3];
	union reg_sensor_mac_vi_10              reg_10;
	union reg_sensor_mac_vi_14              reg_14;
	uint32_t                                _resv_0x18[2];
	union reg_sensor_mac_vi_20              reg_20;
	union reg_sensor_mac_vi_24              reg_24;
	union reg_sensor_mac_vi_28              reg_28;
	union reg_sensor_mac_vi_2c              reg_2c;
	union reg_sensor_mac_vi_30              reg_30;
	union reg_sensor_mac_vi_34              reg_34;
	union reg_sensor_mac_vi_38              reg_38;
	union reg_sensor_mac_vi_3c              reg_3c;
	union reg_sensor_mac_vi_40              reg_40;
	union reg_sensor_mac_vi_44              reg_44;
	union reg_sensor_mac_vi_48              reg_48;
};

#ifdef __cplusplus
}
#endif

#endif /* _REG_BLOCKS_CSI_MAC_H_ */
