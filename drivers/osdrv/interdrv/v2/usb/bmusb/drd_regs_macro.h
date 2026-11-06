#ifndef __REG_USBDRD_ADDR_MAP_MACRO_H__
#define __REG_USBDRD_ADDR_MAP_MACRO_H__


/* macros for BlueprintGlobalNameSpace::OTGCMD_TYPE */
#ifndef __OTGCMD_TYPE_MACRO__
#define __OTGCMD_TYPE_MACRO__

/* macros for field DEV_BUS_REQ */
#define OTGCMD_TYPE__DEV_BUS_REQ__MASK                              0x00000001U
#define OTGCMD_TYPE__HOST_BUS_REQ__MASK                             0x00000002U
#define OTGCMD_TYPE__OTG_EN__MASK                                   0x00000004U
#define OTGCMD_TYPE__OTG_DIS__MASK                                  0x00000008U
#define OTGCMD_TYPE__A_DEV_EN__MASK                                 0x00000010U
#define OTGCMD_TYPE__A_DEV_DIS__MASK                                0x00000020U
#define OTGCMD_TYPE__A_SESS_USE_CLR__MASK                           0x00000080U
#define OTGCMD_TYPE__DEV_BUS_DROP__MASK                             0x00000100U
#define OTGCMD_TYPE__HOST_BUS_DROP__MASK                            0x00000200U
#define OTGCMD_TYPE__DIS_VBUS_DROP__MASK                            0x00000400U
#define OTGCMD_TYPE__DEV_POWER_OFF__MASK                            0x00000800U
#define OTGCMD_TYPE__HOST_POWER_OFF__MASK                           0x00001000U
#define OTGCMD_TYPE__DEV_DEVEN_FORCE_SET__MASK                      0x00002000U
#define OTGCMD_TYPE__SS_HOST_DISABLED_SET__MASK                     0x00080000U
#define OTGCMD_TYPE__SS_PERIPH_DISABLED_SET__MASK                   0x00200000U
#define OTGCMD_TYPE__A_SET_B_HNP_EN_SET__MASK                       0x00800000U
#define OTGCMD_TYPE__A_SET_B_HNP_EN_CLR__MASK                       0x01000000U
#define OTGCMD_TYPE__B_HNP_EN_SET__MASK                             0x02000000U
#define OTGCMD_TYPE__OTG2_SWITCH_TO_PERIPH__MASK                    0x08000000U
#define OTGCMD_TYPE__INIT_SRP__MASK                                 0x10000000U
#define OTGCMD_TYPE__DEV_VBUS_DEB_SHORT_SET__MASK                   0x20000000U
#endif /* __OTGCMD_TYPE_MACRO__ */


/* macros for usbdrd_register_block.OTGCMD */
#ifndef __OTGSTS_TYPE_MACRO__
#define __OTGSTS_TYPE_MACRO__

/* macros for field ID_VALUE */
#define OTGSTS_TYPE__ID_VALUE__READ(src) \
			((uint32_t)(src) & 0x00000001U)
#define OTGSTS_TYPE__SESSION_VALID__READ(src) \
			(((uint32_t)(src)\
			& 0x00000004U) >> 2)
#define OTGSTS_TYPE__OTG_NRDY__READ(src) \
			(((uint32_t)(src)\
			& 0x00000800U) >> 11)
#define OTGSTS_TYPE__STRAP__READ(src)   (((uint32_t)(src) & 0x00007000U) >> 12)

/* macros for field H_WRST_FOR_SWAP */
#define OTGSTS_TYPE__SRP_INITIAL_CONDITION_MET__READ(src) \
			(((uint32_t)(src)\
			& 0x00040000U) >> 18)
#define OTGSTS_TYPE__SRP_DET_NOT_COMPLIANT_DEV__READ(src) \
			(((uint32_t)(src)\
			& 0x00080000U) >> 19)
#define OTGSTS_TYPE__XHC_READY__SHIFT                                        26
#define OTGSTS_TYPE__XHC_READY__READ(src) \
			(((uint32_t)(src)\
			& 0x04000000U) >> 26)
#define OTGSTS_TYPE__DEV_READY__SHIFT                                        27
#endif /* __OTGSTS_TYPE_MACRO__ */


/* macros for usbdrd_register_block.OTGSTS */
#ifndef __OTGSTATE_TYPE_MACRO__
#define __OTGSTATE_TYPE_MACRO__

/* macros for field dev_otg_state */
#define OTGSTATE_TYPE__DEV_OTG_STATE__MASK                          0x00000007U
#define OTGSTATE_TYPE__HOST_OTG_STATE__MASK                         0x00000038U
#define OTGSTATE_TYPE__HOST_OTG_STATE__READ(src) \
			(((uint32_t)(src)\
			& 0x00000038U) >> 3)

/* macros for field apb_axi_ctrl */
#define OTGSTATE_TYPE__APB_AXI_CTRL__READ(src) \
			(((uint32_t)(src)\
			& 0x00000300U) >> 8)
#define OTGSTATE_TYPE__PHY_REFCLK_VALID__READ(src) \
			(((uint32_t)(src)\
			& 0x04000000U) >> 26)
#endif /* __OTGSTATE_TYPE_MACRO__ */


/* macros for usbdrd_register_block.OTGSTATE */
#ifndef __OTGREFCLK_TYPE_MACRO__
#define __OTGREFCLK_TYPE_MACRO__

/* macros for field P3_TO_REFCLK_REQ */
#define OTGREFCLK_TYPE__OTG_STB_CLK_SWITCH_EN__READ(src) \
			(((uint32_t)(src)\
			& 0x80000000U) >> 31)
#define OTGREFCLK_TYPE__OTG_STB_CLK_SWITCH_EN__SET(dst) \
			((dst) = ((dst) &\
			~0x80000000U) | ((uint32_t)(1) << 31))
#define OTGREFCLK_TYPE__OTG_STB_CLK_SWITCH_EN__CLR(dst) \
			((dst) = ((dst) &\
			~0x80000000U) | ((uint32_t)(0) << 31))
#endif /* __OTGREFCLK_TYPE_MACRO__ */


/* macros for usbdrd_register_block.OTGREFCLK */
#ifndef __OTGIEN_TYPE_MACRO__
#define __OTGIEN_TYPE_MACRO__

/* macros for field ID_CHANGE_INT_EN */
#define OTGIEN_TYPE__ID_CHANGE_INT_EN__MASK                         0x00000001U
#define OTGIEN_TYPE__OTGSESSVALID_RISE_INT_EN__MASK                 0x00000004U
#define OTGIEN_TYPE__OTGSESSVALID_FALL_INT_EN__MASK                 0x00000008U
#define OTGIEN_TYPE__TA_AIDL_BDIS_TMOUT_INT_EN__MASK                0x00000200U
#define OTGIEN_TYPE__TA_BIDL_ADIS_TMOUT_INT_EN__MASK                0x00000400U
#define OTGIEN_TYPE__SRP_DET_INT_EN__MASK                           0x00000800U
#define OTGIEN_TYPE__SRP_NOT_COMP_DEV_REMOVED_INT_EN__MASK          0x00001000U
#define OTGIEN_TYPE__SRP_NOT_COMP_DEV_REMOVED_INT_EN__READ(src) \
			(((uint32_t)(src)\
			& 0x00001000U) >> 12)
#define OTGIEN_TYPE__OVERCURRENT_INT_EN__MASK                       0x00002000U
#define OTGIEN_TYPE__OVERCURRENT_INT_EN__READ(src) \
			(((uint32_t)(src)\
			& 0x00002000U) >> 13)
#define OTGIEN_TYPE__SRP_FAIL_INT_EN__MASK                          0x00004000U
#define OTGIEN_TYPE__SRP_CMPL_INT_EN__MASK                          0x00008000U
#define OTGIEN_TYPE__TB_ASE0_BRST_TMOUT_INT_EN__MASK                0x00010000U
#define OTGIEN_TYPE__TB_AIDL_BDIS_MIN_TMOUT_INT_EN__MASK            0x00020000U
#define OTGIEN_TYPE__TB_AIDL_BDIS_MIN_TMOUT_INT_EN__READ(src) \
			(((uint32_t)(src)\
			& 0x00020000U) >> 17)
#define OTGIEN_TYPE__TIMER_TMOUT_INT_EN__MASK                       0x00040000U
#endif /* __OTGIEN_TYPE_MACRO__ */


/* macros for usbdrd_register_block.OTGIEN */
#ifndef __OTGIVECT_TYPE_MACRO__
#define __OTGIVECT_TYPE_MACRO__

/* macros for field ID_CHANGE_INT */
#define OTGIVECT_TYPE__TA_BIDL_ADIS_TMOUT_INT__READ(src) \
			(((uint32_t)(src)\
			& 0x00000400U) >> 10)
#define OTGIVECT_TYPE__SRP_DET_INT__READ(src) \
			(((uint32_t)(src)\
			& 0x00000800U) >> 11)
#define OTGIVECT_TYPE__SRP_CMPL_INT__READ(src) \
			(((uint32_t)(src)\
			& 0x00008000U) >> 15)
#define OTGIVECT_TYPE__TB_ASE0_BRST_TMOUT_INT__READ(src) \
			(((uint32_t)(src)\
			& 0x00010000U) >> 16)
#define OTGIVECT_TYPE__TB_AIDL_BDIS_MIN_TMOUT_INT__READ(src) \
			(((uint32_t)(src)\
			& 0x00020000U) >> 17)
#endif /* __OTGIVECT_TYPE_MACRO__ */


/* macros for usbdrd_register_block.OTGIVECT */
#ifndef __CLK_FREQ_TYPE_MACRO__
#define __CLK_FREQ_TYPE_MACRO__

/* macros for field CLK_FREQ_MHZ */
#endif /* __CLK_FREQ_TYPE_MACRO__ */


/* macros for usbdrd_register_block.CLK_FREQ */
#ifndef __OTGTMR_TYPE_MACRO__
#define __OTGTMR_TYPE_MACRO__

/* macros for field TIMEOUT_VALUE */
#define OTGTMR_TYPE__TIMEOUT_VALUE__WRITE(src)  ((uint32_t)(src) & 0x0000ffffU)
#define OTGTMR_TYPE__TIMEOUT_UNITS__WRITE(src) \
			(((uint32_t)(src)\
			<< 16) & 0x00030000U)
#define OTGTMR_TYPE__TIMER_WRITE__MASK                              0x00040000U
#define OTGTMR_TYPE__TIMER_START__MASK                              0x00080000U
#define OTGTMR_TYPE__TIMER_STOP__MASK                               0x00100000U
#endif /* __OTGTMR_TYPE_MACRO__ */


/* macros for usbdrd_register_block.OTGTMR */
#ifndef __OTGVERSION_TYPE_MACRO__
#define __OTGVERSION_TYPE_MACRO__

/* macros for field OTGVERSION */
#define OTGVERSION_TYPE__OTGVERSION__READ(src) \
			((uint32_t)(src) & 0x0000ffffU)

/* macros for field Rsvd1 */
#endif /* __OTGVERSION_TYPE_MACRO__ */


/* macros for usbdrd_register_block.OTGVERSION */
#ifndef __OTGCAPABILITY_TYPE_MACRO__
#define __OTGCAPABILITY_TYPE_MACRO__

/* macros for field SRP_SUPPORT */
#define OTGCAPABILITY_TYPE__SRP_SUPPORT__READ(src) \
			((uint32_t)(src)\
			& 0x00000001U)
#define OTGCAPABILITY_TYPE__HNP_SUPPORT__READ(src) \
			(((uint32_t)(src)\
			& 0x00000002U) >> 1)
#define OTGCAPABILITY_TYPE__ADP_SUPPORT__READ(src) \
			(((uint32_t)(src)\
			& 0x00000004U) >> 2)
#define OTGCAPABILITY_TYPE__OTG2REVISION__READ(src) \
			(((uint32_t)(src)\
			& 0x000fff00U) >> 8)

/* macros for field OTG3REVISION */
#endif /* __OTGCAPABILITY_TYPE_MACRO__ */


/* macros for usbdrd_register_block.OTGCAPABILITY */
#ifndef __OTGSIMULATE_TYPE_MACRO__
#define __OTGSIMULATE_TYPE_MACRO__

/* macros for field OTG_CFG_FAST_SIMS */
#define OTGSIMULATE_TYPE__OTG_CFG_FAST_SIMS__MASK                   0x00000001U
#endif /* __OTGSIMULATE_TYPE_MACRO__ */


/* macros for usbdrd_register_block.OTGSIMULATE */
#ifndef __OTGANASTS_TYPE_MACRO__
#define __OTGANASTS_TYPE_MACRO__

/* macros for field dp_vdat_ref_comp_sts */
#endif /* __OTGANASTS_TYPE_MACRO__ */


/* macros for usbdrd_register_block.OTGANASTS */
#ifndef __ADP_RAMP_TIME_TYPE_MACRO__
#define __ADP_RAMP_TIME_TYPE_MACRO__

/* macros for field ADP_RAMP_TIME */
#endif /* __ADP_RAMP_TIME_TYPE_MACRO__ */


/* macros for usbdrd_register_block.ADP_RAMP_TIME */
#ifndef __OTGCTRL1_TYPE_MACRO__
#define __OTGCTRL1_TYPE_MACRO__

/* macros for field adp_en */
#define OTGCTRL1_TYPE__IDPULLUP__SET(dst) \
			((dst) = ((dst) &\
			~0x01000000U) | ((uint32_t)(1) << 24))
#endif /* __OTGCTRL1_TYPE_MACRO__ */


/* macros for usbdrd_register_block.OTGCTRL1 */
#ifndef __OTGCTRL2_TYPE_MACRO__
#define __OTGCTRL2_TYPE_MACRO__

/* macros for field TA_ADP_PRB */
#endif /* __OTGCTRL2_TYPE_MACRO__ */


/* macros for usbdrd_register_block.OTGCTRL2 */
#endif /* __REG_USBDRD_ADDR_MAP_MACRO_H__ */
