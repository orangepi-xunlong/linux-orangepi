/******************************************************************************
 *
 * Copyright(c) 2012 - 2020 Realtek Corporation.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 ******************************************************************************/
#ifdef PHL_FEATURE_AP

#if defined(MAC_8852C_SUPPORT)
#ifdef MAC_FW_8852C_U1
#ifdef MAC_FW_CATEGORY_AP
#define FW_CONFIG_WRITE_OFFLOAD 1
#define FW_CONFIG_RATE_ADAPTIVE 1
#endif /* MAC_FW_CATEGORY_AP */

#endif /* MAC_FW_8852C_U1 */
#ifdef MAC_FW_8852C_U2
#ifdef MAC_FW_CATEGORY_AP
#define FW_CONFIG_WRITE_OFFLOAD 1
#define FW_CONFIG_RATE_ADAPTIVE 1
#endif /* MAC_FW_CATEGORY_AP */

#endif /* MAC_FW_8852C_U2 */
#endif /* #if MAC_XXXX_SUPPORT */
#endif /* PHL_FEATURE_AP */
#ifdef PHL_FEATURE_NIC

#if defined(MAC_8852C_SUPPORT)
#ifdef MAC_FW_8852C_U1
#ifdef MAC_FW_CATEGORY_NIC
#define FW_CONFIG_WRITE_OFFLOAD 1
#define FW_CONFIG_SWITCH_CHANNEL_OFFLOAD 1
#define FW_CONFIG_TWT_NIC 1
#define FW_CONFIG_SCAN_OFFLOAD 1
#define FW_CONFIG_WIFI_SENSING_CSI 1
#define FW_CONFIG_RATE_ADAPTIVE 1
#endif /* MAC_FW_CATEGORY_NIC */

#ifdef MAC_FW_CATEGORY_WOWLAN
#define FW_CONFIG_WRITE_OFFLOAD 1
#define FW_CONFIG_SCAN_OFFLOAD 1
#endif /* MAC_FW_CATEGORY_WOWLAN */

#endif /* MAC_FW_8852C_U1 */
#ifdef MAC_FW_8852C_U2
#ifdef MAC_FW_CATEGORY_NIC
#define FW_CONFIG_WRITE_OFFLOAD 1
#define FW_CONFIG_SWITCH_CHANNEL_OFFLOAD 1
#define FW_CONFIG_TWT_NIC 1
#define FW_CONFIG_SCAN_OFFLOAD 1
#define FW_CONFIG_WIFI_SENSING_CSI 1
#define FW_CONFIG_RATE_ADAPTIVE 1
#endif /* MAC_FW_CATEGORY_NIC */

#ifdef MAC_FW_CATEGORY_WOWLAN
#define FW_CONFIG_WRITE_OFFLOAD 1
#define FW_CONFIG_SCAN_OFFLOAD 1
#endif /* MAC_FW_CATEGORY_WOWLAN */

#endif /* MAC_FW_8852C_U2 */
#endif /* #if MAC_XXXX_SUPPORT */
#endif /* PHL_FEATURE_NIC */
