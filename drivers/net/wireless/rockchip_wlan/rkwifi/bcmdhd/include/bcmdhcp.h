/*
 * Fundamental constants relating to DHCP Protocol
 *
 * Copyright (C) 2024 Synaptics Incorporated. All rights reserved.
 *
 * This software is licensed to you under the terms of the
 * GNU General Public License version 2 (the "GPL") with Broadcom special exception.
 *
 * INFORMATION CONTAINED IN THIS DOCUMENT IS PROVIDED "AS-IS," AND SYNAPTICS
 * EXPRESSLY DISCLAIMS ALL EXPRESS AND IMPLIED WARRANTIES, INCLUDING ANY
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE,
 * AND ANY WARRANTIES OF NON-INFRINGEMENT OF ANY INTELLECTUAL PROPERTY RIGHTS.
 * IN NO EVENT SHALL SYNAPTICS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, PUNITIVE, OR CONSEQUENTIAL DAMAGES ARISING OUT OF OR IN CONNECTION
 * WITH THE USE OF THE INFORMATION CONTAINED IN THIS DOCUMENT, HOWEVER CAUSED
 * AND BASED ON ANY THEORY OF LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, AND EVEN IF SYNAPTICS WAS ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE. IF A TRIBUNAL OF COMPETENT JURISDICTION
 * DOES NOT PERMIT THE DISCLAIMER OF DIRECT DAMAGES OR ANY OTHER DAMAGES,
 * SYNAPTICS' TOTAL CUMULATIVE LIABILITY TO ANY PARTY SHALL NOT
 * EXCEED ONE HUNDRED U.S. DOLLARS
 *
 * Copyright (C) 2024, Broadcom.
 *
 *      Unless you and Broadcom execute a separate written software license
 * agreement governing use of this software, this software is licensed to you
 * under the terms of the GNU General Public License version 2 (the "GPL"),
 * available at http://www.broadcom.com/licenses/GPLv2.php, with the
 * following added to such license:
 *
 *      As a special exception, the copyright holders of this software give you
 * permission to link this software with independent modules, and to copy and
 * distribute the resulting executable under terms of your choice, provided that
 * you also meet, for each linked independent module, the terms and conditions of
 * the license of that module.  An independent module is a module which is not
 * derived from this software.  The special exception does not apply to any
 * modifications of the software.
 *
 *
 * <<Broadcom-WL-IPTag/Dual:>>
 */

#ifndef _bcmdhcp_h_
#define _bcmdhcp_h_

/* DHCP params */
#define DHCP_TYPE_OFFSET	0	/* DHCP type (request|reply) offset */
#define DHCP_TID_OFFSET		4	/* DHCP transition id offset */
#define DHCP_FLAGS_OFFSET	10	/* DHCP flags offset */
#define DHCP_CIADDR_OFFSET	12	/* DHCP client IP address offset */
#define DHCP_YIADDR_OFFSET	16	/* DHCP your IP address offset */
#define DHCP_GIADDR_OFFSET	24	/* DHCP relay agent IP address offset */
#define DHCP_CHADDR_OFFSET	28	/* DHCP client h/w address offset */
#define DHCP_OPT_OFFSET		236	/* DHCP options offset */

#define DHCP_OPT_MSGTYPE	53	/* DHCP message type */
#define DHCP_OPT_MSGTYPE_REQ	3
#define DHCP_OPT_MSGTYPE_ACK	5	/* DHCP message type - ACK */

#define DHCP_OPT_CODE_OFFSET	0	/* Option identifier */
#define DHCP_OPT_LEN_OFFSET	1	/* Option data length */
#define DHCP_OPT_DATA_OFFSET	2	/* Option data */

#define DHCP_OPT_CODE_CLIENTID	61	/* Option identifier */

#define DHCP_TYPE_REQUEST	1	/* DHCP request (discover|request) */
#define DHCP_TYPE_REPLY		2	/* DHCP reply (offset|ack) */

#define DHCP_PORT_SERVER	67	/* DHCP server UDP port */
#define DHCP_PORT_CLIENT	68	/* DHCP client UDP port */

#define DHCP_FLAG_BCAST	0x8000	/* DHCP broadcast flag */

#define DHCP_FLAGS_LEN	2	/* DHCP flags field length */

#define DHCP6_TYPE_SOLICIT	1	/* DHCP6 solicit */
#define DHCP6_TYPE_ADVERTISE	2	/* DHCP6 advertise */
#define DHCP6_TYPE_REQUEST	3	/* DHCP6 request */
#define DHCP6_TYPE_CONFIRM	4	/* DHCP6 confirm */
#define DHCP6_TYPE_RENEW	5	/* DHCP6 renew */
#define DHCP6_TYPE_REBIND	6	/* DHCP6 rebind */
#define DHCP6_TYPE_REPLY	7	/* DHCP6 reply */
#define DHCP6_TYPE_RELEASE	8	/* DHCP6 release */
#define DHCP6_TYPE_DECLINE	9	/* DHCP6 decline */
#define DHCP6_TYPE_RECONFIGURE	10	/* DHCP6 reconfigure */
#define DHCP6_TYPE_INFOREQ	11	/* DHCP6 information request */
#define DHCP6_TYPE_RELAYFWD	12	/* DHCP6 relay forward */
#define DHCP6_TYPE_RELAYREPLY	13	/* DHCP6 relay reply */

#define DHCP6_TYPE_OFFSET	0	/* DHCP6 type offset */

#define	DHCP6_MSG_OPT_OFFSET	4	/* Offset of options in client server messages */
#define	DHCP6_RELAY_OPT_OFFSET	34	/* Offset of options in relay messages */

#define	DHCP6_OPT_CODE_OFFSET	0	/* Option identifier */
#define	DHCP6_OPT_LEN_OFFSET	2	/* Option data length */
#define	DHCP6_OPT_DATA_OFFSET	4	/* Option data */

#define	DHCP6_OPT_CODE_CLIENTID	1	/* DHCP6 CLIENTID option */
#define	DHCP6_OPT_CODE_SERVERID	2	/* DHCP6 SERVERID option */

#define DHCP6_PORT_SERVER	547	/* DHCP6 server UDP port */
#define DHCP6_PORT_CLIENT	546	/* DHCP6 client UDP port */

#define UDP_PORT_DNS		53	/* UDP DNS port */

#endif	/* #ifndef _bcmdhcp_h_ */
