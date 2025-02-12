/* SPDX-License-Identifier: GPL-2.0 */

#ifndef _X1_EMAC_H_
#define _X1_EMAC_H_
#include <linux/bitops.h>
#include <linux/ptp_clock_kernel.h>


#define PHY_INTF_RGMII					BIT(2)

/*
 * only valid for rmii mode
 * 0: ref clock from external phy
 * 1: ref clock from soc
 */
#define REF_CLK_SEL					BIT(3)

/*
 * emac function clock select
 * 0: 208M
 * 1: 312M
 */
#define FUNC_CLK_SEL					BIT(4)

/* only valid for rmii, invert tx clk */
#define RMII_TX_CLK_SEL					BIT(6)

/* only valid for rmii, invert rx clk */
#define RMII_RX_CLK_SEL					BIT(7)

/*
 * only valid for rgmiii
 * 0: tx clk from rx clk
 * 1: tx clk from soc
 */
#define RGMII_TX_CLK_SEL				BIT(8)

#define PHY_IRQ_EN					BIT(12)
#define AXI_SINGLE_ID					BIT(13)

#define RMII_TX_PHASE_OFFSET				(16)
#define RMII_TX_PHASE_MASK				GENMASK(18, 16)
#define RMII_RX_PHASE_OFFSET				(20)
#define RMII_RX_PHASE_MASK				GENMASK(22, 20)

#define RGMII_TX_PHASE_OFFSET				(24)
#define RGMII_TX_PHASE_MASK				GENMASK(26, 24)
#define RGMII_RX_PHASE_OFFSET				(28)
#define RGMII_RX_PHASE_MASK				GENMASK(30, 28)

#define EMAC_RX_DLINE_EN				BIT(0)
#define EMAC_RX_DLINE_STEP_OFFSET			(4)
#define EMAC_RX_DLINE_STEP_MASK				GENMASK(5, 4)
#define EMAC_RX_DLINE_CODE_OFFSET			(8)
#define EMAC_RX_DLINE_CODE_MASK				GENMASK(15, 8)

#define EMAC_TX_DLINE_EN				BIT(16)
#define EMAC_TX_DLINE_STEP_OFFSET			(20)
#define EMAC_TX_DLINE_STEP_MASK				GENMASK(21, 20)
#define EMAC_TX_DLINE_CODE_OFFSET			(24)
#define EMAC_TX_DLINE_CODE_MASK				GENMASK(31, 24)

/* DMA register set */
#define DMA_CONFIGURATION				0x0000
#define DMA_CONTROL					0x0004
#define DMA_STATUS_IRQ					0x0008
#define DMA_INTERRUPT_ENABLE				0x000C

#define DMA_TRANSMIT_AUTO_POLL_COUNTER			0x0010
#define DMA_TRANSMIT_POLL_DEMAND			0x0014
#define DMA_RECEIVE_POLL_DEMAND				0x0018

#define DMA_TRANSMIT_BASE_ADDRESS			0x001C
#define DMA_RECEIVE_BASE_ADDRESS			0x0020
#define DMA_MISSED_FRAME_COUNTER			0x0024
#define DMA_STOP_FLUSH_COUNTER				0x0028

#define DMA_RECEIVE_IRQ_MITIGATION_CTRL			0x002C

#define DMA_CURRENT_TRANSMIT_DESCRIPTOR_POINTER		0x0030
#define DMA_CURRENT_TRANSMIT_BUFFER_POINTER		0x0034
#define DMA_CURRENT_RECEIVE_DESCRIPTOR_POINTER		0x0038
#define DMA_CURRENT_RECEIVE_BUFFER_POINTER		0x003C

/* MAC Register set */
#define MAC_GLOBAL_CONTROL				0x0100
#define MAC_TRANSMIT_CONTROL				0x0104
#define MAC_RECEIVE_CONTROL				0x0108
#define MAC_MAXIMUM_FRAME_SIZE				0x010C
#define MAC_TRANSMIT_JABBER_SIZE			0x0110
#define MAC_RECEIVE_JABBER_SIZE				0x0114
#define MAC_ADDRESS_CONTROL				0x0118
#define MAC_MDIO_CLK_DIV				0x011C
#define MAC_ADDRESS1_HIGH				0x0120
#define MAC_ADDRESS1_MED				0x0124
#define MAC_ADDRESS1_LOW				0x0128
#define MAC_ADDRESS2_HIGH				0x012C
#define MAC_ADDRESS2_MED				0x0130
#define MAC_ADDRESS2_LOW				0x0134
#define MAC_ADDRESS3_HIGH				0x0138
#define MAC_ADDRESS3_MED				0x013C
#define MAC_ADDRESS3_LOW				0x0140
#define MAC_ADDRESS4_HIGH				0x0144
#define MAC_ADDRESS4_MED				0x0148
#define MAC_ADDRESS4_LOW				0x014C
#define MAC_MULTICAST_HASH_TABLE1			0x0150
#define MAC_MULTICAST_HASH_TABLE2			0x0154
#define MAC_MULTICAST_HASH_TABLE3			0x0158
#define MAC_MULTICAST_HASH_TABLE4			0x015C
#define MAC_FC_CONTROL					0x0160
#define MAC_FC_PAUSE_FRAME_GENERATE			0x0164
#define MAC_FC_SOURCE_ADDRESS_HIGH			0x0168
#define MAC_FC_SOURCE_ADDRESS_MED			0x016C
#define MAC_FC_SOURCE_ADDRESS_LOW			0x0170
#define MAC_FC_DESTINATION_ADDRESS_HIGH			0x0174
#define MAC_FC_DESTINATION_ADDRESS_MED			0x0178
#define MAC_FC_DESTINATION_ADDRESS_LOW			0x017C
#define MAC_FC_PAUSE_TIME_VALUE				0x0180
#define MAC_MDIO_CONTROL				0x01A0
#define MAC_MDIO_DATA					0x01A4
#define MAC_RX_STATCTR_CONTROL				0x01A8
#define MAC_RX_STATCTR_DATA_HIGH			0x01AC
#define MAC_RX_STATCTR_DATA_LOW				0x01B0
#define MAC_TX_STATCTR_CONTROL				0x01B4
#define MAC_TX_STATCTR_DATA_HIGH			0x01B8
#define MAC_TX_STATCTR_DATA_LOW				0x01BC
#define MAC_TRANSMIT_FIFO_ALMOST_FULL			0x01C0
#define MAC_TRANSMIT_PACKET_START_THRESHOLD		0x01C4
#define MAC_RECEIVE_PACKET_START_THRESHOLD		0x01C8
#define MAC_STATUS_IRQ					0x01E0
#define MAC_INTERRUPT_ENABLE				0x01E4

/* DMA_CONFIGURATION (0x0000) register bit info
 * 0-DMA controller in normal operation mode,
 * 1-DMA controller reset to default state,
 * clearing all internal state information
 */
#define MREGBIT_SOFTWARE_RESET				BIT(0)
#define MREGBIT_BURST_1WORD				BIT(1)
#define MREGBIT_BURST_2WORD				BIT(2)
#define MREGBIT_BURST_4WORD				BIT(3)
#define MREGBIT_BURST_8WORD				BIT(4)
#define MREGBIT_BURST_16WORD				BIT(5)
#define MREGBIT_BURST_32WORD				BIT(6)
#define MREGBIT_BURST_64WORD				BIT(7)
#define MREGBIT_BURST_LENGTH				GENMASK(7, 1)
#define MREGBIT_DESCRIPTOR_SKIP_LENGTH			GENMASK(12, 8)
/* For Receive and Transmit DMA operate in Big-Endian mode for Descriptors. */
#define MREGBIT_DESCRIPTOR_BYTE_ORDERING		BIT(13)
#define MREGBIT_BIG_LITLE_ENDIAN			BIT(14)
#define MREGBIT_TX_RX_ARBITRATION			BIT(15)
#define MREGBIT_WAIT_FOR_DONE				BIT(16)
#define MREGBIT_STRICT_BURST				BIT(17)
#define MREGBIT_DMA_64BIT_MODE				BIT(18)

/* DMA_CONTROL (0x0004) register bit info */
#define MREGBIT_START_STOP_TRANSMIT_DMA			BIT(0)
#define MREGBIT_START_STOP_RECEIVE_DMA			BIT(1)

/* DMA_STATUS_IRQ (0x0008) register bit info */
#define MREGBIT_TRANSMIT_TRANSFER_DONE_IRQ		BIT(0)
#define MREGBIT_TRANSMIT_DES_UNAVAILABLE_IRQ		BIT(1)
#define MREGBIT_TRANSMIT_DMA_STOPPED_IRQ		BIT(2)
#define MREGBIT_RECEIVE_TRANSFER_DONE_IRQ		BIT(4)
#define MREGBIT_RECEIVE_DES_UNAVAILABLE_IRQ		BIT(5)
#define MREGBIT_RECEIVE_DMA_STOPPED_IRQ			BIT(6)
#define MREGBIT_RECEIVE_MISSED_FRAME_IRQ		BIT(7)
#define MREGBIT_MAC_IRQ					BIT(8)
#define MREGBIT_TRANSMIT_DMA_STATE			GENMASK(18, 16)
#define MREGBIT_RECEIVE_DMA_STATE			GENMASK(23, 20)

/* DMA_INTERRUPT_ENABLE ( 0x000C) register bit info */
#define MREGBIT_TRANSMIT_TRANSFER_DONE_INTR_ENABLE	BIT(0)
#define MREGBIT_TRANSMIT_DES_UNAVAILABLE_INTR_ENABLE	BIT(1)
#define MREGBIT_TRANSMIT_DMA_STOPPED_INTR_ENABLE	BIT(2)
#define MREGBIT_RECEIVE_TRANSFER_DONE_INTR_ENABLE	BIT(4)
#define MREGBIT_RECEIVE_DES_UNAVAILABLE_INTR_ENABLE	BIT(5)
#define MREGBIT_RECEIVE_DMA_STOPPED_INTR_ENABLE		BIT(6)
#define MREGBIT_RECEIVE_MISSED_FRAME_INTR_ENABLE	BIT(7)
#define MREGBIT_MAC_INTR_ENABLE				BIT(8)

/* DMA RECEIVE IRQ MITIGATION CONTROL */
#define MREGBIT_RECEIVE_IRQ_FRAME_COUNTER_MSK		GENMASK(7, 0)
#define MREGBIT_RECEIVE_IRQ_TIMEOUT_COUNTER_OFST	(8)
#define MREGBIT_RECEIVE_IRQ_TIMEOUT_COUNTER_MSK		GENMASK(27, 8)
#define MRGEBIT_RECEIVE_IRQ_FRAME_COUNTER_MODE		BIT(30)
#define MRGEBIT_RECEIVE_IRQ_MITIGATION_ENABLE		BIT(31)

/* MAC_GLOBAL_CONTROL (0x0100) register bit info */
#define MREGBIT_SPEED					GENMASK(1, 0)
#define MREGBIT_SPEED_10M				0x0
#define MREGBIT_SPEED_100M				BIT(0)
#define MREGBIT_SPEED_1000M				BIT(1)
#define MREGBIT_FULL_DUPLEX_MODE			BIT(2)
#define MREGBIT_RESET_RX_STAT_COUNTERS			BIT(3)
#define MREGBIT_RESET_TX_STAT_COUNTERS			BIT(4)
#define MREGBIT_UNICAST_WAKEUP_MODE			BIT(8)
#define MREGBIT_MAGIC_PACKET_WAKEUP_MODE		BIT(9)

/* MAC_TRANSMIT_CONTROL (0x0104) register bit info */
#define MREGBIT_TRANSMIT_ENABLE				BIT(0)
#define MREGBIT_INVERT_FCS				BIT(1)
#define MREGBIT_DISABLE_FCS_INSERT			BIT(2)
#define MREGBIT_TRANSMIT_AUTO_RETRY			BIT(3)
#define MREGBIT_IFG_LEN					GENMASK(6, 4)
#define MREGBIT_PREAMBLE_LENGTH				GENMASK(9, 7)

/* MAC_RECEIVE_CONTROL (0x0108) register bit info */
#define MREGBIT_RECEIVE_ENABLE				BIT(0)
#define MREGBIT_DISABLE_FCS_CHECK			BIT(1)
#define MREGBIT_STRIP_FCS				BIT(2)
#define MREGBIT_STORE_FORWARD				BIT(3)
#define MREGBIT_STATUS_FIRST				BIT(4)
#define MREGBIT_PASS_BAD_FRAMES				BIT(5)
#define MREGBIT_ACOOUNT_VLAN				BIT(6)

/* MAC_MAXIMUM_FRAME_SIZE (0x010C) register bit info */
#define MREGBIT_MAX_FRAME_SIZE				GENMASK(13, 0)

/* MAC_TRANSMIT_JABBER_SIZE (0x0110) register bit info */
#define MREGBIT_TRANSMIT_JABBER_SIZE			GENMASK(15, 0)

/* MAC_RECEIVE_JABBER_SIZE (0x0114) register bit info */
#define MREGBIT_RECEIVE_JABBER_SIZE			GENMASK(15, 0)

/* MAC_ADDRESS_CONTROL	 (0x0118) register bit info */
#define MREGBIT_MAC_ADDRESS1_ENABLE			BIT(0)
#define MREGBIT_MAC_ADDRESS2_ENABLE			BIT(1)
#define MREGBIT_MAC_ADDRESS3_ENABLE			BIT(2)
#define MREGBIT_MAC_ADDRESS4_ENABLE			BIT(3)
#define MREGBIT_INVERSE_MAC_ADDRESS1_ENABLE		BIT(4)
#define MREGBIT_INVERSE_MAC_ADDRESS2_ENABLE		BIT(5)
#define MREGBIT_INVERSE_MAC_ADDRESS3_ENABLE		BIT(6)
#define MREGBIT_INVERSE_MAC_ADDRESS4_ENABLE		BIT(7)
#define MREGBIT_PROMISCUOUS_MODE			BIT(8)

/* MAC_ADDRESSx_HIGH (0x0120) register bit info */
#define MREGBIT_MAC_ADDRESS1_01_BYTE			GENMASK(7, 0)
#define MREGBIT_MAC_ADDRESS1_02_BYTE			GENMASK(15, 8)
/* MAC_ADDRESSx_MED (0x0124) register bit info */
#define MREGBIT_MAC_ADDRESS1_03_BYTE			GENMASK(7, 0)
#define MREGBIT_MAC_ADDRESS1_04_BYTE			GENMASK(15, 8)
/* MAC_ADDRESSx_LOW (0x0128) register bit info */
#define MREGBIT_MAC_ADDRESS1_05_BYTE			GENMASK(7, 0)
#define MREGBIT_MAC_ADDRESS1_06_BYTE			GENMASK(15, 8)

/* MAC_FC_CONTROL (0x0160) register bit info */
#define MREGBIT_FC_DECODE_ENABLE			BIT(0)
#define MREGBIT_FC_GENERATION_ENABLE			BIT(1)
#define MREGBIT_AUTO_FC_GENERATION_ENABLE		BIT(2)
#define MREGBIT_MULTICAST_MODE				BIT(3)
#define MREGBIT_BLOCK_PAUSE_FRAMES			BIT(4)

/* MAC_FC_PAUSE_FRAME_GENERATE (0x0164) register bit info */
#define MREGBIT_GENERATE_PAUSE_FRAME			BIT(0)

/* MAC_FC_SRC/DST_ADDRESS_HIGH (0x0168) register bit info */
#define MREGBIT_MAC_ADDRESS_01_BYTE			GENMASK(7, 0)
#define MREGBIT_MAC_ADDRESS_02_BYTE			GENMASK(15, 8)
/* MAC_FC_SRC/DST_ADDRESS_MED (0x016C) register bit info */
#define MREGBIT_MAC_ADDRESS_03_BYTE			GENMASK(7, 0)
#define MREGBIT_MAC_ADDRESS_04_BYTE			GENMASK(15, 8)
/* MAC_FC_SRC/DSTD_ADDRESS_LOW (0x0170) register bit info */
#define MREGBIT_MAC_ADDRESS_05_BYTE			GENMASK(7, 0)
#define MREGBIT_MAC_ADDRESS_06_BYTE			GENMASK(15, 8)

/* MAC_FC_PAUSE_TIME_VALUE (0x0180) register bit info */
#define MREGBIT_MAC_FC_PAUSE_TIME			GENMASK(15, 0)

/* MAC_MDIO_CONTROL (0x01A0) register bit info */
#define MREGBIT_PHY_ADDRESS				GENMASK(4, 0)
#define MREGBIT_REGISTER_ADDRESS			GENMASK(9, 5)
#define MREGBIT_MDIO_READ_WRITE				BIT(10)
#define MREGBIT_START_MDIO_TRANS			BIT(15)

/* MAC_MDIO_DATA (0x01A4) register bit info */
#define MREGBIT_MDIO_DATA				GENMASK(15, 0)

/* MAC_RX_STATCTR_CONTROL (0x01A8) register bit info */
#define MREGBIT_RX_COUNTER_NUMBER			GENMASK(4, 0)
#define MREGBIT_START_RX_COUNTER_READ			BIT(15)

/* MAC_RX_STATCTR_DATA_HIGH (0x01AC) register bit info */
#define MREGBIT_RX_STATCTR_DATA_HIGH			GENMASK(15, 0)
/* MAC_RX_STATCTR_DATA_LOW (0x01B0) register bit info */
#define MREGBIT_RX_STATCTR_DATA_LOW			GENMASK(15, 0)

/* MAC_TX_STATCTR_CONTROL (0x01B4) register bit info */
#define MREGBIT_TX_COUNTER_NUMBER			GENMASK(4, 0)
#define MREGBIT_START_TX_COUNTER_READ			BIT(15)

/* MAC_TX_STATCTR_DATA_HIGH (0x01B8) register bit info */
#define MREGBIT_TX_STATCTR_DATA_HIGH			GENMASK(15, 0)
/* MAC_TX_STATCTR_DATA_LOW (0x01BC) register bit info */
#define MREGBIT_TX_STATCTR_DATA_LOW			GENMASK(15, 0)

/* MAC_TRANSMIT_FIFO_ALMOST_FULL (0x01C0) register bit info */
#define MREGBIT_TX_FIFO_AF				GENMASK(13, 0)

/* MAC_TRANSMIT_PACKET_START_THRESHOLD (0x01C4) register bit info */
#define MREGBIT_TX_PACKET_START_THRESHOLD		GENMASK(13, 0)

/* MAC_RECEIVE_PACKET_START_THRESHOLD (0x01C8) register bit info */
#define MREGBIT_RX_PACKET_START_THRESHOLD		GENMASK(13, 0)

/* MAC_STATUS_IRQ  (0x01E0) register bit info */
#define MREGBIT_MAC_UNDERRUN_IRQ			BIT(0)
#define MREGBIT_MAC_JABBER_IRQ				BIT(1)

/* MAC_INTERRUPT_ENABLE (0x01E4) register bit info */
#define MREGBIT_MAC_UNDERRUN_INTERRUPT_ENABLE		BIT(0)
#define MREGBIT_JABBER_INTERRUPT_ENABLE			BIT(1)

/* Receive Descriptors */
/* MAC_RECEIVE_DESCRIPTOR0 () register bit info */
#define MREGBIT_FRAME_LENGTH				GENMASK(13, 0)
#define MREGBIT_APPLICATION_STATUS			GENMASK(28, 14)
#define MREGBIT_LAST_DESCRIPTOR				BIT(29)
#define MREGBIT_FIRST_DESCRIPTOR			BIT(30)
#define MREGBIT_OWN_BIT					BIT(31)

/* MAC_RECEIVE_DESCRIPTOR1 () register bit info */
#define MREGBIT_BUFFER1_SIZE				GENMASK(11, 0)
#define MREGBIT_BUFFER2_SIZE				GENMASK(23, 12)
#define MREGBIT_SECOND_ADDRESS_CHAINED			BIT(25)
#define MREGBIT_END_OF_RING				BIT(26)

/* MAC_RECEIVE_DESCRIPTOR2 () register bit info */
#define MREGBIT_BUFFER_ADDRESS1				GENMASK(31, 0)

/* MAC_RECEIVE_DESCRIPTOR3 () register bit info */
#define MREGBIT_BUFFER_ADDRESS1				GENMASK(31, 0)

/* Transmit Descriptors */
/* TD_TRANSMIT_DESCRIPTOR0 () register bit info */
#define MREGBIT_TX_PACKET_STATUS			GENMASK(29, 0)
#define MREGBIT_OWN_BIT					BIT(31)

/* TD_TRANSMIT_DESCRIPTOR1 () register bit info */
#define MREGBIT_BUFFER1_SIZE				GENMASK(11, 0)
#define MREGBIT_BUFFER2_SIZE				GENMASK(23, 12)
#define MREGBIT_FORCE_EOP_ERROR				BIT(24)
#define MREGBIT_SECOND_ADDRESS_CHAINED			BIT(25)
#define MREGBIT_END_OF_RING				BIT(26)
#define MREGBIT_DISABLE_PADDING				BIT(27)
#define MREGBIT_ADD_CRC_DISABLE				BIT(28)
#define MREGBIT_FIRST_SEGMENT				BIT(29)
#define MREGBIT_LAST_SEGMENT				BIT(30)
#define MREGBIT_INTERRUPT_ON_COMPLETION			BIT(31)

/* TD_TRANSMIT_DESCRIPTOR2 () register bit info */
#define MREGBIT_BUFFER_ADDRESS1				GENMASK(31, 0)

/* TD_TRANSMIT_DESCRIPTOR3 () register bit info */
#define MREGBIT_BUFFER_ADDRESS1				GENMASK(31, 0)

/* RX frame status */
#define EMAC_RX_FRAME_ALIGN_ERR				BIT(0)
#define EMAC_RX_FRAME_RUNT				BIT(1)
#define EMAC_RX_FRAME_ETHERNET_TYPE			BIT(2)
#define EMAC_RX_FRAME_VLAN				BIT(3)
#define EMAC_RX_FRAME_MULTICAST				BIT(4)
#define EMAC_RX_FRAME_BROADCAST				BIT(5)
#define EMAC_RX_FRAME_CRC_ERR				BIT(6)
#define EMAC_RX_FRAME_MAX_LEN_ERR			BIT(7)
#define EMAC_RX_FRAME_JABBER_ERR			BIT(8)
#define EMAC_RX_FRAME_LENGTH_ERR			BIT(9)
#define EMAC_RX_FRAME_MAC_ADDR1_MATCH			BIT(10)
#define EMAC_RX_FRAME_MAC_ADDR2_MATCH			BIT(11)
#define EMAC_RX_FRAME_MAC_ADDR3_MATCH			BIT(12)
#define EMAC_RX_FRAME_MAC_ADDR4_MATCH			BIT(13)
#define EMAC_RX_FRAME_PAUSE_CTRL			BIT(14)

/* emac ptp 1588 register */
#define PTP_1588_CTRL					(0x300)
#define TX_TIMESTAMP_EN					BIT(1)
#define RX_TIMESTAMP_EN					BIT(2)
#define RX_PTP_PKT_TYPE_OFST				3
#define RX_PTP_PKT_TYPE_MSK				GENMASK(5, 3)

#define PTP_INRC_ATTR					(0x304)
#define INRC_VAL_MSK					GENMASK(23, 0)
#define INCR_PERIOD_OFST				24
#define INCR_PERIOD_MSK					GENMASK(31, 24)

#define PTP_ETH_TYPE					(0x308)
#define PTP_ETH_TYPE_MSK				GENMASK(15, 0)

#define PTP_MSG_ID					(0x30c)

#define PTP_UDP_PORT					(0x310)
#define PTP_UDP_PORT_MSK				GENMASK(15, 0)

/* read current system time from controller */
#define SYS_TIME_GET_LOW				(0x320)
#define SYS_TIME_GET_HI					(0x324)

#define SYS_TIME_ADJ_LOW				(0x328)
#define SYS_TIME_LOW_MSK				GENMASK(31, 0)
#define SYS_TIME_ADJ_HI					(0x32c)
#define SYS_TIME_IS_NEG					BIT(31)

#define TX_TIMESTAMP_LOW				(0x330)
#define TX_TIMESTAMP_HI					(0x334)

#define RX_TIMESTAMP_LOW				(0x340)
#define RX_TIMESTAMP_HI					(0x344)

#define RX_PTP_PKT_ATTR_LOW				(0x348)
#define PTP_SEQ_ID_MSK					GENMASK(15, 0)
#define PTP_SRC_ID_LOW_OFST				16
#define PTP_SRC_ID_LOW_MSK				GENMASK(31, 16)

#define RX_PTP_PKT_ATTR_MID				(0x34c)
#define PTP_SRC_ID_MID_MSK				GENMASK(31, 0)

#define RX_PTP_PKT_ATTR_HI				(0x350)
#define PTP_SRC_ID_HI_MSK				GENMASK(31, 0)

#define PTP_1588_IRQ_STS				(0x360)
#define PTP_1588_IRQ_EN					(0x364)
#define PTP_TX_TIMESTAMP				BIT(0)
#define PTP_RX_TIMESTAMP				BIT(1)

/* emac ptp register */

#define EMAC_DEFAULT_BUFSIZE				1536
#define EMAC_RX_BUF_2K					2048
#define EMAC_RX_BUF_4K					4096

#define MAX_DATA_PWR_TX_DES				11
#define MAX_DATA_LEN_TX_DES				2048 //2048=1<<11

#define MAX_TX_STATS_NUM				12
#define MAX_RX_STATS_NUM				25

/* The sizes (in bytes) of a ethernet packet */
#define ETHERNET_HEADER_SIZE				14
#define MAXIMUM_ETHERNET_FRAME_SIZE			1518  //With FCS
#define MINIMUM_ETHERNET_FRAME_SIZE			64  //With FCS
#define ETHERNET_FCS_SIZE				4
#define MAXIMUM_ETHERNET_PACKET_SIZE \
		(MAXIMUM_ETHERNET_FRAME_SIZE - ETHERNET_FCS_SIZE)

#define MINIMUM_ETHERNET_PACKET_SIZE \
		(MINIMUM_ETHERNET_FRAME_SIZE - ETHERNET_FCS_SIZE)

#define CRC_LENGTH					ETHERNET_FCS_SIZE
#define MAX_JUMBO_FRAME_SIZE				0x3F00

#define TX_STORE_FORWARD_MODE				0x5EE

#define EMAC_TX_FRAMES					64
/* 40ms */
#define EMAC_TX_COAL_TIMEOUT				40000

#define EMAC_RX_FRAMES					64

/* axi clk 312M, 1us = 312 cycle,
 * every packet almost take 120us when operate at 100Mbps
 * so we set 5 packet delay time which 600us as rx coal timeout
 */
#define EMAC_RX_COAL_TIMEOUT				(36 * 312)

/* only works for sizes that are powers of 2 */
#define EMAC_ROUNDUP(i, size) ((i) = (((i) + (size) - 1) & ~((size) - 1)))

/* number of descriptors are required for len */
#define EMAC_TXD_COUNT(S, X) (((S) >> (X)) + 1)

/* calculate the number of descriptors unused */
#define EMAC_DESC_UNUSED(R) \
	((((R)->nxt_clean > (R)->nxt_use) ? 0 : (R)->total_cnt) + \
	(R)->nxt_clean - (R)->nxt_use - 1)

typedef struct ifreq  st_ifreq, *pst_ifreq;

enum rx_frame_status {
	frame_ok = 0,
	frame_discard,
	frame_max,
};

enum rx_ptp_type {
	PTP_V2_L2_ONLY = 0x0,
	PTP_V1_L4_ONLY = 0x1,
	PTP_V2_L2_L4  = 0x2,
};

enum ptp_event_msg_id {
	MSG_SYNC = 0x00,
	MSG_DELAY_REQ = 0x01,
	MSG_PDELAY_REQ = 0x02,
	MSG_PDELAY_RESP = 0x03,
	ALL_EVENTS = 0x03020100,
};

enum emac_state {
	EMAC_DOWN,
	EMAC_RESET_REQUESTED,
	EMAC_RESETING,
	EMAC_TASK_SCHED,
	EMAC_STATE_MAX,
};

/* Receive Descriptor structure */
struct emac_rx_desc {
	u32 FramePacketLength:14;
	u32 ApplicationStatus:15;
	u32 LastDescriptor:1;
	u32 FirstDescriptor:1;
	u32 OWN:1;

	u32 BufferSize1:12;
	u32 BufferSize2:12;
	u32 Reserved1:1;
	u32 SecondAddressChained:1;
	u32 EndRing:1;
	u32 Reserved2:3;
	u32 rx_timestamp:1;
	u32 ptp_pkt:1;

	u32 BufferAddr1;
	u32 BufferAddr2;
};

/* Transmit Descriptor */
struct emac_tx_desc {
	u32 FramePacketStatus:30;
	u32 tx_timestamp:1;
	u32 OWN:1;

	u32 BufferSize1:12;
	u32 BufferSize2:12;
	u32 ForceEOPError:1;
	u32 SecondAddressChained:1;
	u32 EndRing:1;
	u32 DisablePadding:1;
	u32 AddCRCDisable:1;
	u32 FirstSegment:1;
	u32 LastSegment:1;
	u32 InterruptOnCompletion:1;

	u32 BufferAddr1;
	u32 BufferAddr2;
};

struct desc_buf {
	u64 dma_addr;
	void *buff_addr;
	u16 dma_len;
	u8 map_as_page;
};

/* Descriptor buffer structure */
struct emac_tx_desc_buffer {
	struct sk_buff *skb;
	struct desc_buf buf[2];
	u8 timestamped;
};

/* Descriptor buffer structure */
struct emac_desc_buffer {
	struct sk_buff *skb;
	u64 dma_addr;
	void *buff_addr;
	unsigned long ulTimeStamp;
	u16 dma_len;
	u8 map_as_page;
	u8 timestamped;
};

/* Descriptor ring structure */
struct emac_desc_ring {
	/* virtual memory address to the descriptor ring memory */
	void *desc_addr;
	/* physical address of the descriptor ring */
	dma_addr_t desc_dma_addr;
	/* length of descriptor ring in bytes */
	u32 total_size;
	/* number of descriptors in the ring */
	u32 total_cnt;
	/* next descriptor to associate a buffer with */
	u32 head;
	/* next descriptor to check for DD status bit */
	u32 tail;
	/* array of buffer information structs */
	union {
		struct emac_desc_buffer *desc_buf;
		struct emac_tx_desc_buffer *tx_desc_buf;
	};
};

struct emac_hw_stats {
	u32 tx_ok_pkts;
	u32 tx_total_pkts;
	u32 tx_ok_bytes;
	u32 tx_err_pkts;
	u32 tx_singleclsn_pkts;
	u32 tx_multiclsn_pkts;
	u32 tx_lateclsn_pkts;
	u32 tx_excessclsn_pkts;
	u32 tx_unicast_pkts;
	u32 tx_multicast_pkts;
	u32 tx_broadcast_pkts;
	u32 tx_pause_pkts;
	u32 rx_ok_pkts;
	u32 rx_total_pkts;
	u32 rx_crc_err_pkts;
	u32 rx_align_err_pkts;
	u32 rx_err_total_pkts;
	u32 rx_ok_bytes;
	u32 rx_total_bytes;
	u32 rx_unicast_pkts;
	u32 rx_multicast_pkts;
	u32 rx_broadcast_pkts;
	u32 rx_pause_pkts;
	u32 rx_len_err_pkts;
	u32 rx_len_undersize_pkts;
	u32 rx_len_oversize_pkts;
	u32 rx_len_fragment_pkts;
	u32 rx_len_jabber_pkts;
	u32 rx_64_pkts;
	u32 rx_65_127_pkts;
	u32 rx_128_255_pkts;
	u32 rx_256_511_pkts;
	u32 rx_512_1023_pkts;
	u32 rx_1024_1518_pkts;
	u32 rx_1519_plus_pkts;
	u32 rx_drp_fifo_full_pkts;
	u32 rx_truncate_fifo_full_pkts;

	spinlock_t	stats_lock;
};

struct emac_priv;
struct emac_hw_ptp {
	void (*config_hw_tstamping) (struct emac_priv *priv, u32 enable,
					u8 rx_ptp_type, u32 ptp_msg_id);
	u32 (*config_systime_increment)(struct emac_priv *priv, u32 ptp_clock,
					u32 adj_clock);
	int (*init_systime) (struct emac_priv *priv, u64 set_ns);
	u64 (*get_phc_time)(struct emac_priv *priv);
	u64 (*get_tx_timestamp)(struct emac_priv *priv);
	u64 (*get_rx_timestamp)(struct emac_priv *priv);
};

struct emac_priv {
	u32 dma_buf_sz;
	u32 wol;
	spinlock_t spStatsLock;
	struct work_struct tx_timeout_task;
	struct emac_desc_ring tx_ring;
	struct emac_desc_ring rx_ring;
	spinlock_t spTxLock;
	struct net_device *ndev;
	struct napi_struct napi;
	struct platform_device *pdev;
	struct clk *mac_clk;
	struct clk *phy_clk;
	struct clk *ptp_clk;
	struct reset_control *reset;
	void __iomem *iobase;
	u32 apmu_base;
	int irq;
	int link;
	int duplex;
	int speed;
	phy_interface_t phy_interface;
	struct mii_bus *mii;
	struct phy_device *phy;
	struct emac_hw_stats *hw_stats;
	u8 tx_clk_phase;
	u8 rx_clk_phase;
	u8 clk_tuning_way;
	bool clk_tuning_enable;
	unsigned long state;
	u32 tx_threshold;
	u32 rx_threshold;
	u32 tx_ring_num;
	u32 rx_ring_num;
	u32 dma_burst_len;
	u32 ref_clk_frm_soc;
	void __iomem *ctrl_reg;
	void __iomem *dline_reg;
	s32 lpm_qos;
	u32 tx_count_frames;
	u32 tx_coal_frames;
	u32 tx_coal_timeout;
	struct timer_list txtimer;
	struct ptp_clock *ptp_clock;
	struct ptp_clock_info ptp_clock_ops;
	spinlock_t ptp_lock;
	int ptp_support;
	u32 ptp_clk_rate;
	int hwts_tx_en;
	int hwts_rx_en;
	struct emac_hw_ptp *hwptp;
	struct delayed_work systim_overflow_work;
	struct cyclecounter cc;
	struct timecounter tc;
};


static inline void emac_wr(struct emac_priv *priv, u32 reg, u32 val)
{
	writel(val, (priv->iobase + reg));
}

static inline int emac_rd(struct emac_priv *priv, u32 reg)
{
	return readl(priv->iobase + reg);
}

int emac_init_hw(struct emac_priv *priv);
int emac_reset_hw(struct emac_priv *priv);
int emac_set_mac_addr(struct emac_priv *priv, const unsigned char *addr);
int emac_down(struct emac_priv *priv);
void emac_command_options(struct emac_priv *priv);
int emac_alloc_tx_resources(struct emac_priv *priv);
int emac_alloc_rx_resources(struct emac_priv *priv);
void emac_free_tx_resources(struct emac_priv *priv);
void emac_free_rx_resources(struct emac_priv *priv);
u32 ReadRxStatCounters(struct emac_priv *priv, u8 cnt);
u32 ReadTxStatCounters(struct emac_priv *priv, u8 cnt);

extern void emac_ptp_register(struct emac_priv *priv);
extern void emac_ptp_unregister(struct emac_priv *priv);
void emac_ptp_init(struct emac_priv *priv);
void emac_ptp_deinit(struct emac_priv *priv);
#endif /* _X1_EMAC_H_ */
