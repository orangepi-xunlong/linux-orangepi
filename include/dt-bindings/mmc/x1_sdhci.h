// SPDX-License-Identifier: GPL-2.0
/*
 * include/linux/dt-bindings/mmc/x1_sdhci.h
 *
 * SDH driver for KY X1 SDCHI
 * Copyright (C) 2023 Ky
 */

#ifndef X1_DT_BINDINGS_MMC_SDHCI_H
#define X1_DT_BINDINGS_MMC_SDHCI_H

/* X1 specific flag */

/* MMC Quirks */
/* Controller has an unusable ADMA engine */
#define SDHCI_QUIRK_BROKEN_ADMA					(1<<6)
#define SDHCI_QUIRK2_PRESET_VALUE_BROKEN			(1<<3)
/* Controller does not support HS200 */
#define SDHCI_QUIRK2_BROKEN_HS200				(1<<6)
/* Support SDH controller on FPGA */
#define SDHCI_QUIRK2_SUPPORT_PHY_BYPASS				(1<<25)
/* Disable scan card at probe phase */
#define SDHCI_QUIRK2_DISABLE_PROBE_CDSCAN			(1<<26)
/* Need to set IO capability by SOC part register */
#define SDHCI_QUIRK2_SET_AIB_MMC				(1<<27)
/* Controller not support phy module */
#define SDHCI_QUIRK2_BROKEN_PHY_MODULE				(1<<28)
/* Controller support encrypt module */
#define SDHCI_QUIRK2_SUPPORT_ENCRYPT				(1<<29)

/* Common flag */
/* Controller provides an incorrect timeout value for transfers */
#define SDHCI_QUIRK_BROKEN_TIMEOUT_VAL				(1<<12)
/* Controller has unreliable card detection */
#define SDHCI_QUIRK_BROKEN_CARD_DETECTION			(1<<15)

/* Controller reports inverted write-protect state */
#define SDHCI_QUIRK_INVERTED_WRITE_PROTECT			(1<<16)

/* MMC caps */
#define MMC_CAP2_CRC_SW_RETRY	(1 << 30)

/* for SDIO */
#define MMC_CAP_NEEDS_POLL	(1 << 5)	/* Needs polling for card-detection */

/* for SD card */
#define MMC_CAP_UHS_SDR12	(1 << 16)	/* Host supports UHS SDR12 mode */
#define MMC_CAP_UHS_SDR25	(1 << 17)	/* Host supports UHS SDR25 mode */
#define MMC_CAP_UHS_SDR50	(1 << 18)	/* Host supports UHS SDR50 mode */
#define MMC_CAP_UHS_SDR104	(1 << 19)	/* Host supports UHS SDR104 mode */
#define MMC_CAP_UHS_DDR50	(1 << 20)	/* Host supports UHS DDR50 mode */
#endif /* X1_DT_BINDINGS_MMC_SDHCI_H */

