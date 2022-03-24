/*
 * include/linux/motorcomm_phy.h
 *
 * Motorcomm PHY IDs
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 */

#ifndef _MOTORCOMM_PHY_H
#define _MOTORCOMM_PHY_H

#define MOTORCOMM_PHY_ID_MASK	0x00000fff
#define MOTORCOMM_PHY_ID_8531_MASK	0xffffffff
#define MOTORCOMM_MPHY_ID_MASK	0x0000ffff

#define PHY_ID_YT8010		0x00000309
#define PHY_ID_YT8510		0x00000109
#define PHY_ID_YT8511		0x0000010a
#define PHY_ID_YT8512		0x00000118
#define PHY_ID_YT8512B		0x00000128
#define PHY_ID_YT8521		0x0000011a
#define PHY_ID_YT8531S		0x4f51e91a
#define PHY_ID_YT8531		0x4f51e91b
//#define PHY_ID_YT8614		0x0000e899
#define PHY_ID_YT8618		0x0000e889

#define REG_PHY_SPEC_STATUS		0x11
#define REG_DEBUG_ADDR_OFFSET		0x1e
#define REG_DEBUG_DATA			0x1f

#define YT8512_EXTREG_AFE_PLL		0x50
#define YT8512_EXTREG_EXTEND_COMBO	0x4000
#define YT8512_EXTREG_LED0		0x40c0
#define YT8512_EXTREG_LED1		0x40c3

#define YT8512_EXTREG_SLEEP_CONTROL1	0x2027

#define YT_SOFTWARE_RESET		0x8000

#define YT8512_CONFIG_PLL_REFCLK_SEL_EN	0x0040
#define YT8512_CONTROL1_RMII_EN		0x0001
#define YT8512_LED0_ACT_BLK_IND		0x1000
#define YT8512_LED0_DIS_LED_AN_TRY	0x0001
#define YT8512_LED0_BT_BLK_EN		0x0002
#define YT8512_LED0_HT_BLK_EN		0x0004
#define YT8512_LED0_COL_BLK_EN		0x0008
#define YT8512_LED0_BT_ON_EN		0x0010
#define YT8512_LED1_BT_ON_EN		0x0010
#define YT8512_LED1_TXACT_BLK_EN	0x0100
#define YT8512_LED1_RXACT_BLK_EN	0x0200
#define YT8512_SPEED_MODE		0xc000
#define YT8512_DUPLEX			0x2000

#define YT8512_SPEED_MODE_BIT		14
#define YT8512_DUPLEX_BIT		13
#define YT8512_EN_SLEEP_SW_BIT		15

#define YT8521_EXTREG_SLEEP_CONTROL1	0x27
#define YT8521_EN_SLEEP_SW_BIT		15

#define YT8521_SPEED_MODE		0xc000
#define YT8521_DUPLEX			0x2000
#define YT8521_SPEED_MODE_BIT		14
#define YT8521_DUPLEX_BIT		13
#define YT8521_LINK_STATUS_BIT		10

/* based on yt8521 wol config register */
#define YTPHY_UTP_INTR_REG             0x12
/* WOL Event Interrupt Enable */
#define YTPHY_WOL_INTR            BIT(6)

/* Magic Packet MAC address registers */
#define YTPHY_MAGIC_PACKET_MAC_ADDR2                 0xa007
#define YTPHY_MAGIC_PACKET_MAC_ADDR1                 0xa008
#define YTPHY_MAGIC_PACKET_MAC_ADDR0                 0xa009

#define YTPHY_WOL_CFG_REG		0xa00a
#define YTPHY_WOL_CFG_TYPE		BIT(0)	/* WOL TYPE */
#define YTPHY_WOL_CFG_EN		BIT(3)	/* WOL Enable */
#define YTPHY_WOL_CFG_INTR_SEL	BIT(6)	/* WOL Event Interrupt Enable */
#define YTPHY_WOL_CFG_WIDTH1	BIT(1)	/* WOL Pulse Width */
#define YTPHY_WOL_CFG_WIDTH2	BIT(2)

#define YTPHY_REG_SPACE_UTP             0
#define YTPHY_REG_SPACE_FIBER           2

enum ytphy_wol_type_e
{
    YTPHY_WOL_TYPE_LEVEL,
    YTPHY_WOL_TYPE_PULSE,
    YTPHY_WOL_TYPE_MAX
};
typedef enum ytphy_wol_type_e ytphy_wol_type_t;

enum ytphy_wol_width_e
{
    YTPHY_WOL_WIDTH_84MS,
    YTPHY_WOL_WIDTH_168MS,
    YTPHY_WOL_WIDTH_336MS,
    YTPHY_WOL_WIDTH_672MS,
    YTPHY_WOL_WIDTH_MAX
};
typedef enum ytphy_wol_width_e ytphy_wol_width_t;

struct ytphy_wol_cfg_s
{
    int enable;
    int type;
    int width;
};
typedef struct ytphy_wol_cfg_s ytphy_wol_cfg_t;

#endif /* _MOTORCOMM_PHY_H */


