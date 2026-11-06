#ifndef __SOPHGO_PT_H__
#define __SOPHGO_PT_H__

#include <linux/kernel.h>
#include "sophgo_common.h"

/* must multi of 4 */
#define PT_ALIGN 8

#define SHM_HOST_PHY_OFFSET 0x00
#define SHM_HOST_LEN_OFFSET 0x08
#define SHM_HOST_HEAD_OFFSET 0x10
#define SHM_HOST_TAIL_OFFSET 0x14
/* reserved 8 bytes */
#define SHM_HOST_RX_OFFSET 0x00
#define SHM_HOST_TX_OFFSET 0x20

#define SHM_SOC_TX_OFFSET SHM_HOST_RX_OFFSET
#define SHM_SOC_RX_OFFSET SHM_HOST_TX_OFFSET

struct host_queue {
	u64 __iomem *phy;
	u64 __iomem *len;
	u32 __iomem *head;
	u32 __iomem *tail;
	u64 vphy;
};

struct local_queue {
	u32 len;
	u32 head;
	u32 tail;
	dma_addr_t phy;
	u8 *cpu;
};

// ring queue
struct pt_queue {
	struct local_queue lq;
	struct host_queue hq;
};

struct veth_dev;
// package transmitter
struct pt {
	struct device *dev;
	struct veth_dev *vdev;
	struct pt_queue tx, rx;
	void __iomem *shm;
};

/* load configuration from share memory */
struct pt *pt_init(struct device *dev, void __iomem *shm, struct veth_dev *vdev);
int pt_send(struct pt *pt, void *data, int len);
int pt_recv(struct pt *pt, void *data, int len);
void pt_load_tx(struct pt *pt);
bool pt_load_rx(struct pt *pt);
int pt_store_tx(struct pt *pt);
void pt_store_rx(struct pt *pt);
int pt_pkg_len(struct pt *pt);
void pt_uninit(struct pt *pt);
void pt_info(struct pt *pt);
void pt_store_rx(struct pt *pt);

#endif
