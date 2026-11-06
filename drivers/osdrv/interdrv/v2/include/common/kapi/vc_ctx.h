#ifndef __VC_CTX_H__
#define __VC_CTX_H__

#ifdef __cplusplus
	extern "C" {
#endif

#define VCODEC_QUIRK_SUPPORT_CLOCK_CONTROL  (1 << 0)
#define VCODEC_QUIRK_SUPPORT_SWITCH_TO_PLL  (1 << 1)
#define VCODEC_QUIRK_SUPPORT_REMAP_DDR      (1 << 2)
#define VCODEC_QUIRK_SUPPORT_VC_CTRL_REG    (1 << 3)
#define VCODEC_QUIRK_SUPPORT_VC_SBM     (1 << 4)  // slice buffer mode
#define VCODEC_QUIRK_SUPPORT_VC_ADDR_REMAP  (1 << 5)  // address remapping
#define VCDOEC_QUIRK_SUPPORT_FPGA       (1 << 6)  // fpga initialization

#ifdef __cplusplus
}
#endif

#endif /* __VC_CTX_H__ */

