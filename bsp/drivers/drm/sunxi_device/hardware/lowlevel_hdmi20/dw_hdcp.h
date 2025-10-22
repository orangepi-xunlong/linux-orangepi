/* SPDX-License-Identifier: GPL-2.0-or-later */
/*******************************************************************************
 * Allwinner SoCs hdmi2.0 driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 ******************************************************************************/

#ifndef _DW_HDCP_H_
#define _DW_HDCP_H_

#include "dw_dev.h"

/*****************************************************************************
 *                                                                           *
 *                               DW HDCP Function                            *
 *                                                                           *
 *****************************************************************************/
/**
 * @desc: dw hdcp set software type
 * @type: refer to enum dw_hdcp_type_e
 * @return: current setting type
 */
int dw_hdcp_set_enable_type(int type);
/**
 * @desc: set hdcp avmute state
 * @enable: 1 - enable hdcp avmute
 *          0 - disable hdcp avmute
 */
void dw_hdcp1x_set_avmute(int enable);
/**
 * @desc: dw hdcp sync config tmds mode from framecomposer
 */
void dw_hdcp_sync_tmds_mode(void);
/**
 * @desc: dw hdcp sync config data polarity from framecomposer
 */
void dw_hdcp_sync_data_polarity(void);
/**
 * @desc: check hdcp encrypt status and handle the status
 * @return: refer to enum dw_hdcp_state_e
 */
int dw_hdcp_get_state(void);
/**
 * @desc: dw hdcp api for init
 */
void dw_hdcp_initial(void);
/**
 * @desc: dw hdcp api for exit flow
 */
void dw_hdcp_exit(void);
/**
 * @desc: dw hdcp api for info dump
 * @buf: dump info buffer
 * @return: dump info byte count.
 */
ssize_t dw_hdcp_dump(char *buf);

/*****************************************************************************
 *                                                                           *
 *                               DW HDCP1.x Function                         *
 *                                                                           *
 *****************************************************************************/
/**
 * @desc: dw hdcp1x enable
 * @return: 0 - success
 *         -1 - failed
 */
int dw_hdcp1x_enable(void);
/**
 * @desc: dw hdcp1x disable
 * @return: 0 - success
 *         -1 - failed
 */
int dw_hdcp1x_disable(void);

/*****************************************************************************
 *                                                                           *
 *                               HDCP Registers                              *
 *                                                                           *
 *****************************************************************************/

/**
 * @desc: HDCP Enable and Functional Control Configuration Register 0
 * @bits: [0:0] Configures the transmitter to operate with a HDMI capable device or with a DVI device
 *        [1:1] Enable the use of features 1
 *        [2:2] Information that a sink device was detected connected to the HDMI port
 *        [3:3] This register holds the current AVMUTE state of the DWC_hdmi_tx controller,
 *              as expected to be perceived by the connected HDMI/HDCP sink device
 *        [4:4] Configures if the Ri check should be done at every 2s even or synchronously to every 128 encrypted frame
 *        [5:5] Bypasses all the data encryption stages
 *        [6:6] Enable the I2C fast mode option from the transmitter's side
 *        [7:7] Enables the Enhanced Link Verification from the transmitter's side
 */
#define A_HDCPCFG0              0x00005000
#define A_HDCPCFG0_HDMIDVI_MASK                 0x00000001
#define A_HDCPCFG0_EN11FEATURE_MASK             0x00000002
#define A_HDCPCFG0_RXDETECT_MASK                0x00000004
#define A_HDCPCFG0_AVMUTE_MASK                  0x00000008
#define A_HDCPCFG0_SYNCRICHECK_MASK             0x00000010
#define A_HDCPCFG0_BYPENCRYPTION_MASK           0x00000020
#define A_HDCPCFG0_I2CFASTMODE_MASK             0x00000040
#define A_HDCPCFG0_ELVENA_MASK                  0x00000080

/**
 * @desc: HDCP Software Reset and Functional Control Configuration Register 1
 * @bits: [0:0] Software reset signal, active by writing a zero and auto cleared to 1 in the following cycle
 *        [1:1] Disable encryption without losing authentication
 *        [2:2] Enables the encoding of packet header in the tmdsch0 bit[0] with cipher[2] instead of the tmdsch0 bit[2]
 *              Note: This bit must always be set to 1 for all PHYs (PHY GEN1, PHY GEN2, and non-Synopsys PHY)
 *        [3:3] Disables the request to the API processor to verify the SHA1 message digest of a received KSV List
 *        [4:4] Lock the HDCP bypass and encryption disable mechanisms
 *          - 0: The default
 *          - 1: value enables you to bypass HDCP through bit 5 (bypencryption)
 *              of the A_HDCPCFG0 register or to disable the encryption through bit 1 (encryptiondisable) of A_HDCPCFG1
 *        [7:5] This is a spare register with no associated functionality
 */
#define A_HDCPCFG1              0x00005001
#define A_HDCPCFG1_SWRESET_MASK                 0x00000001
#define A_HDCPCFG1_ENCRYPTIONDISABLE_MASK       0x00000002
#define A_HDCPCFG1_PH2UPSHFTENC_MASK            0x00000004
#define A_HDCPCFG1_DISSHA1CHECK_MASK            0x00000008
#define A_HDCPCFG1_HDCP_LOCK_MASK               0x00000010
#define A_HDCPCFG1_SPARE_MASK                   0x000000E0

/**
 * @desc: HDCP Observation Register 0
 * @bits: [0:0] Informs that the current HDMI link has the HDCP protocol fully engaged
 *        [3:1] Observability register informs in which sub-state the authentication is on
 *        [7:4] Observability register informs in which state the authentication machine is on
 */
#define A_HDCPOBS0              0x00005002
#define A_HDCPOBS0_HDCPENGAGED_MASK             0x00000001
#define A_HDCPOBS0_SUBSTATEA_MASK               0x0000000E
#define A_HDCPOBS0_STATEA_MASK                  0x000000F0

/**
 * @desc: HDCP Observation Register 1
 * @bits: [3:0] Observability register informs in which state the revocation machine is on
 *        [6:4] Observability register informs in which state the OESS machine is on
 */
#define A_HDCPOBS1              0x00005003
#define A_HDCPOBS1_STATER_MASK                  0x0000000F
#define A_HDCPOBS1_STATEOEG_MASK                0x00000070

/**
 * @desc: HDCP Observation Register 2
 * @bits: [2:0] Observability register informs in which state the EESS machine is on
 *        [5:3] Observability register informs in which state the cipher machine is on
 */
#define A_HDCPOBS2              0x00005004
#define A_HDCPOBS2_STATEEEG_MASK                0x00000007
#define A_HDCPOBS2_STATEE_MASK                  0x00000038

/**
 * @desc: HDCP Observation Register 3
 * @bits: [0:0] Register read from attached sink device: Bcap(0x40) bit 0
 *        [1:1] Register read from attached sink device: Bcap(0x40) bit 1
 *        [2:2] Register read from attached sink device: Bstatus(0x41) bit 12
 *        [3:3] Register read from attached sink device: Bstatus(0x41) bit 13
 *        [4:4] Register read from attached sink device: Bcap(0x40) bit 4
 *        [5:5] Register read from attached sink device: Bcap(0x40) bit 5
 *        [6:6] Register read from attached sink device: Bcap(0x40) bit 6
 *        [7:7] Register read from attached sink device: Bcap(0x40) bit 7
 */
#define A_HDCPOBS3              0x00005005
#define A_HDCPOBS3_FAST_REAUTHENTICATION_MASK   0x00000001
#define A_HDCPOBS3_FEATURES_1_1_MASK            0x00000002
#define A_HDCPOBS3_HDMI_MODE_MASK               0x00000004
#define A_HDCPOBS3_HDMI_REVERVED_2_MASK         0x00000008
#define A_HDCPOBS3_FAST_I2C_MASK                0x00000010
#define A_HDCPOBS3_KSV_FIFO_READY_MASK          0x00000020
#define A_HDCPOBS3_REPEATER_MASK                0x00000040
#define A_HDCPOBS3_HDMI_REVERVED_1_MASK         0x00000008

/**
 * @desc: HDCP Interrupt Clear Register Write only register,active high and auto cleared,
 * cleans the respective interruption in the interrupt status register
 * @bits: [0:0] Clears the interruption related to KSV memory access grant for Read-Write access
 *        [1:1] Clears the interruption related to KSV list update in memory that needs to be SHA1 verified
 *        [2:2] Clears the interruption related to keep out window error
 *        [3:3] Clears the interruption related to I2C arbitration lost
 *        [4:4] Clears the interruption related to I2C NACK reception
 *        [5:5] Clears the interruption related to SHAQ verification has been done
 *        [6:6] Clears the interruption related to HDCP authentication process failed
 *        [7:7] Clears the interruption related to HDCP authentication process successful
 */
#define A_APIINTCLR             0x00005006
#define A_APIINTCLR_KSVACCESSINT_MASK           0x00000001
#define A_APIINTCLR_KSVSHA1CALCINT_MASK         0x00000002
#define A_APIINTCLR_KEEPOUTERRORINT_MASK        0x00000004
#define A_APIINTCLR_LOSTARBITRATION_MASK        0x00000008
#define A_APIINTCLR_I2CNACK_MASK                0x00000010
#define A_APIINTCLR_KSVSHA1CALDONE_MASK         0x00000020
#define A_APIINTCLR_HDCP_FAILED_MASK            0x00000040
#define A_APIINTCLR_HDCP_ENGAGED_MASK           0x00000080

/**
 * @desc: HDCP Interrupt Status Register Read only register, reports the interruption
 * which caused the activation of the interruption output pin
 * @bits: [0:0] Notifies that the KSV memory access as been guaranteed for Read-Write access
 *        [1:1] Notifies that the HDCP13TCTRL core as updated a KSV list in memory that needs to be SHA1 verified
 *        [2:2] Notifies that during the keep out window, the ctlout[3:0] bus was used besides control period
 *        [3:3] Notifies that the I2C lost the arbitration to communicate
 *        [4:4] Notifies that the I2C received a NACK from slave device
 *        [5:5] Notifies that the HDCP13TCTRL block SHA1 verifition has been done
 *        [6:6] Notifies that the HDCP authentication process was failed
 *        [7:7] Notifies that the HDCP authentication process was successful
 */
#define A_APIINTSTAT            0x00005007
#define A_APIINTSTAT_KSVACCESSINT_MASK          0x00000001
#define A_APIINTSTAT_KSVSHA1CALCINT_MASK        0x00000002
#define A_APIINTSTAT_KEEPOUTERRORINT_MASK       0x00000004
#define A_APIINTSTAT_LOSTARBITRATION_MASK       0x00000008
#define A_APIINTSTAT_I2CNACK_MASK               0x00000010
#define A_APIINTSTAT_KSVSHA1CALCDONEINT_MASK    0x00000020
#define A_APIINTSTAT_HDCP_FAILED_MASK           0x00000040
#define A_APIINTSTAT_HDCP_ENGAGED_MASK          0x00000080

/**
 * @desc: HDCP Interrupt Mask Register
 * The configuration of this register mask a given setup of interruption,
 * disabling them from generating interruption pulses in the interruption output pin
 * @bits: [0:0] Masks the interruption related to KSV memory access grant for Read-Write access
 *        [1:1] Masks the interruption related to KSV list update in memory that needs to be SHA1 verified
 *        [2:2] Masks the interruption related to keep out window error
 *        [3:3] Masks the interruption related to I2C arbitration lost
 *        [4:4] Masks the interruption related to I2C NACK reception
 *        [5:5] This is a spare bit and has no associated functionality
 *        [6:6] Masks the interruption related to HDCP authentication process failed
 *        [7:7] Masks the interruption related to HDCP authentication process successful
 */
#define A_APIINTMSK             0x00005008
#define A_APIINTMSK_KSVACCESSINT_MASK           0x00000001
#define A_APIINTMSK_KSVSHA1CALCINT_MASK         0x00000002
#define A_APIINTMSK_KEEPOUTERRORINT_MASK        0x00000004
#define A_APIINTMSK_LOSTARBITRATION_MASK        0x00000008
#define A_APIINTMSK_I2CNACK_MASK                0x00000010
#define A_APIINTMSK_SPARE_MASK                  0x00000020
#define A_APIINTMSK_HDCP_FAILED_MASK            0x00000040
#define A_APIINTMSK_HDCP_ENGAGED_MASK           0x00000080

/**
 * @desc: HDCP Video Polarity Configuration Register
 * @bits: [0:0] This is a spare bit and has no associated functionality
 *        [1:1] Configuration of the video Horizontal synchronism polarity
 *        [2:2] This is a spare bit and has no associated functionality
 *        [3:3] Configuration of the video Vertical synchronism polarity
 *        [4:4] Configuration of the video data enable polarity
 *        [6:5] Configuration of the color sent when sending unencrypted video data
 *              For a complete table showing the color results (RGB),
 *              refer to the "Color Configuration When Sending Unencrypted Video Data"
 */
#define A_VIDPOLCFG             0x00005009
#define A_VIDPOLCFG_SPARE_1_MASK                0x00000001
#define A_VIDPOLCFG_HSYNCPOL_MASK               0x00000002
#define A_VIDPOLCFG_SPARE_2_MASK                0x00000004
#define A_VIDPOLCFG_VSYNCPOL_MASK               0x00000008
#define A_VIDPOLCFG_DATAENPOL_MASK              0x00000010
#define A_VIDPOLCFG_UNENCRYPTCONF_MASK          0x00000060

/**
 * @desc: HDCP OESS WOO Configuration Register Pulse width of the encryption enable (CTL3) signal in the HDCP OESS mode
 * @bits: [7:0] HDCP OESS WOO Configuration Register
 */
#define A_OESSWCFG              0x0000500A
#define A_OESSWCFG_A_OESSWCFG_MASK              0x000000FF

/**
 * @desc: HDCP Core Version Register LSB Design ID number
 * @bits: [7:0] HDCP Core Version Register LSB
 */
#define A_COREVERLSB            0x00005014
#define A_COREVERLSB_A_COREVERLSB_MASK          0x000000FF

/**
 * @desc: HDCP Core Version Register MSB Revision ID number
 * @bits: [7:0] HDCP Core Version Register MSB
 */
#define A_COREVERMSB            0x00005015
#define A_COREVERMSB_A_COREVERMSB_MASK          0x000000FF

/**
 * @desc: HDCP KSV Memory Control Register The KSVCTRLupd bit is a notification flag
 * @bits: [0:0] Request access to the KSV memory; must be de-asserted after the access is completed by the system
 *        [1:1] Notification that the KSV memory access as been guaranteed
 *        [2:2] Set to inform that the KSV list in memory has been analyzed and the response to the Message Digest as been updated
 *        [3:3] Notification whether the KSV list message digest is correct
 *        [4:4] Notification whether ther KSV list message digest is correct from the controller
 *          - 1: if digest message verification failed
 *          - 0: if digest message verification successed
 */
#define A_KSVMEMCTRL            0x00005016
#define A_KSVMEMCTRL_KSVMEMREQUEST_MASK         0x00000001
#define A_KSVMEMCTRL_KSVMEMACCESS_MASK          0x00000002
#define A_KSVMEMCTRL_KSVCTRLUPD_MASK            0x00000004
#define A_KSVMEMCTRL_SHA1FAIL_MASK              0x00000008
#define A_KSVMEMCTRL_KSVSHA1STATUS_MASK         0x00000010

/**
 * @desc: HDCP BStatus Register 0
 * @bits: [7:0] HDCP BSTATUS[7:0]
 */
#define HDCP_BSTATUS            0x00005020
#define HDCP_BSTATUS_MASK                       0x000000FF

/**
 * @desc: HDCP BStatus Register 1
 * @bits: [7:0] HDCP BSTATUS[15:8]
 */
#define HDCP_BSTATUS1           0x00005021
#define HDCP_BSTATUS1_MASK                      0x000000FF

/**
 * @desc: HDCP M0 Register Array, size 8
 */
#define HDCP_M0                 0x00005022

/**
 * @desc: HDCP KSV Registers
 */
#define HDCP_KSV                0x0000502A

/**
 * @desc: HDCP SHA-1 VH Registers
 */
#define HDCP_VH                 0x000052A5

/**
 * @desc: HDCP Revocation KSV List Size Register 0
 * @bits: [7:0] Register containing the LSB of KSV list size (ksv_list_size[7:0])
 */
#define HDCP_REVOC_SIZE_0       0x000052B9
#define HDCP_REVOC_SIZE_0_HDCP_REVOC_SIZE_0_MASK    0x000000FF

/**
 * @desc: HDCP Revocation KSV List Size Register 1
 * @bits: [7:0] Register containing the MSB of KSV list size (ksv_list_size[15:8])
 */
#define HDCP_REVOC_SIZE_1       0x000052BA
#define HDCP_REVOC_SIZE_1_HDCP_REVOC_SIZE_1_MASK    0x000000FF

/**
 * @desc: HDCP Revocation KSV Registers, size 5060
 */
#define HDCP_REVOC_LIST         0x000052BB

/**
 * @desc: HDCP KSV Status Register 0
 * @bits: [7:0] Contains the value of BKSV[7:0]
 */
#define HDCPREG_BKSV0           0x00007800
#define HDCPREG_BKSV0_HDCPREG_BKSV0_MASK        0x000000FF

/**
 * @desc: HDCP KSV Status Register 1
 * @bits: [7:0] Contains the value of BKSV[15:8]
 */
#define HDCPREG_BKSV1           0x00007801
#define HDCPREG_BKSV1_HDCPREG_BKSV1_MASK        0x000000FF

/**
 * @desc: HDCP KSV Status Register 2
 * @bits: [7:0] Contains the value of BKSV[23:16]
 */
#define HDCPREG_BKSV2           0x00007802
#define HDCPREG_BKSV2_HDCPREG_BKSV2_MASK        0x000000FF

/**
 * @desc: HDCP KSV Status Register 3
 * @bits: [7:0] Contains the value of BKSV[31:24]
 */
#define HDCPREG_BKSV3           0x00007803
#define HDCPREG_BKSV3_HDCPREG_BKSV3_MASK        0x000000FF

/**
 * @desc: HDCP KSV Status Register 4
 * @bits: [7:0] Contains the value of BKSV[39:32]
 */
#define HDCPREG_BKSV4           0x00007804
#define HDCPREG_BKSV4_HDCPREG_BKSV4_MASK        0x000000FF

/**
 * @desc: HDCP AN Bypass Control Register
 * @bits: [0:0] When oanbypass=1, the value of AN used in the HDCP engine comes
 *              from the hdcpreg_an0 to hdcpreg_an7 registers
 */
#define HDCPREG_ANCONF          0x00007805
#define HDCPREG_ANCONF_OANBYPASS_MASK           0x00000001

/**
 * @desc: HDCP Forced AN Register 0
 * @bits: [7:0] Contains the value of AN[7:0]
 */
#define HDCPREG_AN0             0x00007806
#define HDCPREG_AN0_HDCPREG_AN0_MASK            0x000000FF

/**
 * @desc: HDCP Forced AN Register 1
 * @bits: [7:0] Contains the value of AN[15:8]
 */
#define HDCPREG_AN1             0x00007807
#define HDCPREG_AN1_HDCPREG_AN1_MASK            0x000000FF

/**
 * @desc: HDCP Forced AN Register 2
 * @bits: [7:0] Contains the value of AN[23:16]
 */
#define HDCPREG_AN2             0x00007808
#define HDCPREG_AN2_HDCPREG_AN2_MASK            0x000000FF

/**
 * @desc: HDCP Forced AN Register 3
 * @bits: [7:0] Contains the value of AN[31:24]
 */
#define HDCPREG_AN3             0x00007809
#define HDCPREG_AN3_HDCPREG_AN3_MASK            0x000000FF

/**
 * @desc: HDCP Forced AN Register 4
 * @bits: [7:0] Contains the value of AN[39:32]
 */
#define HDCPREG_AN4             0x0000780A
#define HDCPREG_AN4_HDCPREG_AN4_MASK            0x000000FF

/**
 * @desc: HDCP Forced AN Register 5
 * @bits: [7:0] Contains the value of AN[47:40]
 */
#define HDCPREG_AN5             0x0000780B
#define HDCPREG_AN5_HDCPREG_AN5_MASK            0x000000FF

/**
 * @desc: HDCP Forced AN Register 6
 * @bits: [7:0] Contains the value of AN[55:48]
 */
#define HDCPREG_AN6             0x0000780C
#define HDCPREG_AN6_HDCPREG_AN6_MASK            0x000000FF

/**
 * @desc: HDCP Forced AN Register 7
 * @bits: [7:0] Contains the value of AN[63:56]
 */
#define HDCPREG_AN7             0x0000780D
#define HDCPREG_AN7_HDCPREG_AN7_MASK            0x000000FF

#endif  /* _DW_HDCP_H_ */
