#ifndef __REG_USBDRD_ADDR_MAP_H__
#define __REG_USBDRD_ADDR_MAP_H__

#include "drd_regs_macro.h"

struct usbdrd_register_block_type {
	uint32_t OTGCMD;               /*        0x0 - 0x4        */
	uint32_t OTGSTS;               /*        0x4 - 0x8        */
	uint32_t OTGSTATE;             /*        0x8 - 0xc        */
	uint32_t OTGREFCLK;            /*        0xc - 0x10       */
	uint32_t OTGIEN;               /*       0x10 - 0x14       */
	uint32_t OTGIVECT;             /*       0x14 - 0x18       */
	char pad__0[0x8];              /*       0x18 - 0x20       */
	uint32_t PAD2_18;              /*       0x20 - 0x24       */
	uint32_t OTGTMR;               /*       0x24 - 0x28       */
	char pad__1[0x8];              /*       0x28 - 0x30       */
	uint32_t OTGVERSION;           /*       0x30 - 0x34       */
	uint32_t OTGCAPABILITY;        /*       0x34 - 0x38       */
	char pad__2[0x8];              /*       0x38 - 0x40       */
	uint32_t OTGSIMULATE;          /*       0x40 - 0x44       */
	char pad__3[0xc];              /*       0x44 - 0x50       */
	uint32_t PAD2_26;              /*       0x50 - 0x54       */
	uint32_t PAD2_27;              /*       0x54 - 0x58       */
	uint32_t OTGCTRL1;             /*       0x58 - 0x5c       */
	uint32_t PAD2_29;              /*       0x5c - 0x60       */
};

struct USBDRD_addr_map {
	struct usbdrd_register_block_type usbdrd_register_block;
/*        0x0 - 0x60       */
};

#endif /* __REG_USBDRD_ADDR_MAP_H__ */
