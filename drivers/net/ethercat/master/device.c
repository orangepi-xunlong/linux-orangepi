/*****************************************************************************
 *
 *  Copyright (C) 2006-2008  Florian Pose, Ingenieurgemeinschaft IgH
 *
 *  This file is part of the IgH EtherCAT Master.
 *
 *  The IgH EtherCAT Master is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License version 2, as
 *  published by the Free Software Foundation.
 *
 *  The IgH EtherCAT Master is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General
 *  Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with the IgH EtherCAT Master; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 ****************************************************************************/

/**
   \file
   EtherCAT device methods.
*/

/****************************************************************************/

#include <linux/module.h>
#include <linux/skbuff.h>
#include <linux/if_ether.h>
#include <linux/netdevice.h>

#include "device.h"
#include "master.h"

#ifdef EC_DEBUG_RING
#define timersub(a, b, result) \
    do { \
        (result)->tv_sec = (a)->tv_sec - (b)->tv_sec; \
        (result)->tv_usec = (a)->tv_usec - (b)->tv_usec; \
        if ((result)->tv_usec < 0) { \
            --(result)->tv_sec; \
            (result)->tv_usec += 1000000; \
        } \
    } while (0)
#endif

/****************************************************************************/

enum {
    /* genet driver needs extra headroom in skb for status block */
    EXTRA_HEADROOM = 64,
};

/** Constructor.
 *
 * \return 0 in case of success, else < 0
 */
int ec_device_init(
        ec_device_t *device, /**< EtherCAT device */
        ec_master_t *master /**< master owning the device */
        )
{
    int ret;
    unsigned int i;
    struct ethhdr *eth;
#ifdef EC_DEBUG_IF
    char ifname[10];
    char mb = 'x';
#endif

    device->master = master;
    device->dev = NULL;
    device->poll = NULL;
    device->module = NULL;
    device->open = 0;
    device->link_state = 0;
    for (i = 0; i < EC_TX_RING_SIZE; i++) {
        device->tx_skb[i] = NULL;
    }
    device->tx_ring_index = 0;
#ifdef EC_HAVE_CYCLES
    device->cycles_poll = 0;
#endif
#ifdef EC_DEBUG_RING
    device->timeval_poll.tv_sec = 0;
    device->timeval_poll.tv_usec = 0;
#endif
    device->jiffies_poll = 0;

    ec_device_clear_stats(device);

#ifdef EC_DEBUG_RING
    for (i = 0; i < EC_DEBUG_RING_SIZE; i++) {
        ec_debug_frame_t *df = &device->debug_frames[i];
        df->dir = TX;
        df->t.tv_sec = 0;
        df->t.tv_usec = 0;
        memset(df->data, 0, EC_MAX_DATA_SIZE);
        df->data_size = 0;
    }
#endif
#ifdef EC_DEBUG_RING
    device->debug_frame_index = 0;
    device->debug_frame_count = 0;
#endif

#ifdef EC_DEBUG_IF
    if (device == &master->devices[EC_DEVICE_MAIN]) {
        mb = 'm';
    }
    else {
        mb = 'b';
    }

    sprintf(ifname, "ecdbg%c%u", mb, master->index);

    ret = ec_debug_init(&device->dbg, device, ifname);
    if (ret < 0) {
        EC_MASTER_ERR(master, "Failed to init debug device!\n");
        goto out_return;
    }
#endif

    for (i = 0; i < EC_TX_RING_SIZE; i++) {
        if (!(device->tx_skb[i] = dev_alloc_skb(ETH_FRAME_LEN + EXTRA_HEADROOM))) {
            EC_MASTER_ERR(master, "Error allocating device socket buffer!\n");
            ret = -ENOMEM;
            goto out_tx_ring;
        }

        // add Ethernet-II-header
        skb_reserve(device->tx_skb[i], ETH_HLEN + EXTRA_HEADROOM);
        eth = (struct ethhdr *) skb_push(device->tx_skb[i], ETH_HLEN);
        eth->h_proto = htons(0x88A4);
        memset(eth->h_dest, 0xFF, ETH_ALEN);
    }

    return 0;

out_tx_ring:
    for (i = 0; i < EC_TX_RING_SIZE; i++) {
        if (device->tx_skb[i]) {
            dev_kfree_skb(device->tx_skb[i]);
        }
    }
#ifdef EC_DEBUG_IF
    ec_debug_clear(&device->dbg);
out_return:
#endif
    return ret;
}

/****************************************************************************/

/** Destructor.
 */
void ec_device_clear(
        ec_device_t *device /**< EtherCAT device */
        )
{
    unsigned int i;

    if (device->open) {
        ec_device_close(device);
    }
    for (i = 0; i < EC_TX_RING_SIZE; i++)
        dev_kfree_skb(device->tx_skb[i]);
#ifdef EC_DEBUG_IF
    ec_debug_clear(&device->dbg);
#endif
}

/****************************************************************************/

/** Associate with net_device.
 */
void ec_device_attach(
        ec_device_t *device, /**< EtherCAT device */
        struct net_device *net_dev, /**< net_device structure */
        ec_pollfunc_t poll, /**< pointer to device's poll function */
        struct module *module /**< the device's module */
        )
{
    unsigned int i;
    struct ethhdr *eth;

    ec_device_detach(device); // resets fields

    device->dev = net_dev;
    device->poll = poll;
    device->module = module;

    for (i = 0; i < EC_TX_RING_SIZE; i++) {
        device->tx_skb[i]->dev = net_dev;
        eth = (struct ethhdr *) (device->tx_skb[i]->data);
        memcpy(eth->h_source, net_dev->dev_addr, ETH_ALEN);
    }

#ifdef EC_DEBUG_IF
    ec_debug_register(&device->dbg, net_dev);
#endif
}

/****************************************************************************/

/** Disconnect from net_device.
 */
void ec_device_detach(
        ec_device_t *device /**< EtherCAT device */
        )
{
    unsigned int i;

#ifdef EC_DEBUG_IF
    ec_debug_unregister(&device->dbg);
#endif

    device->dev = NULL;
    device->poll = NULL;
    device->module = NULL;
    device->open = 0;
    device->link_state = 0; // down

    ec_device_clear_stats(device);

    for (i = 0; i < EC_TX_RING_SIZE; i++) {
        device->tx_skb[i]->dev = NULL;
    }
}

/****************************************************************************/

/** Opens the EtherCAT device.
 *
 * \return 0 in case of success, else < 0
 */
int ec_device_open(
        ec_device_t *device /**< EtherCAT device */
        )
{
    int ret;

    if (!device->dev) {
        EC_MASTER_ERR(device->master, "No net_device to open!\n");
        return -ENODEV;
    }

    if (device->open) {
        EC_MASTER_WARN(device->master, "Device already opened!\n");
        return 0;
    }

    device->link_state = 0;

    ec_device_clear_stats(device);

    ret = device->dev->netdev_ops->ndo_open(device->dev);
    if (!ret)
        device->open = 1;

    return ret;
}

/****************************************************************************/

/** Stops the EtherCAT device.
 *
 * \return 0 in case of success, else < 0
 */
int ec_device_close(
        ec_device_t *device /**< EtherCAT device */
        )
{
    int ret;

    if (!device->dev) {
        EC_MASTER_ERR(device->master, "No device to close!\n");
        return -ENODEV;
    }

    if (!device->open) {
        EC_MASTER_WARN(device->master, "Device already closed!\n");
        return 0;
    }

    ret = device->dev->netdev_ops->ndo_stop(device->dev);
    if (!ret)
        device->open = 0;

    return ret;
}

/****************************************************************************/

/** Returns a pointer to the device's transmit memory.
 *
 * \return pointer to the TX socket buffer
 */
uint8_t *ec_device_tx_data(
        ec_device_t *device /**< EtherCAT device */
        )
{
    /* cycle through socket buffers, because otherwise there is a race
     * condition, if multiple frames are sent and the DMA is not scheduled in
     * between. */
    device->tx_ring_index++;
    device->tx_ring_index %= EC_TX_RING_SIZE;
    return device->tx_skb[device->tx_ring_index]->data + ETH_HLEN;
}

/****************************************************************************/

/** Sends the content of the transmit socket buffer.
 *
 * Cuts the socket buffer content to the (now known) size, and calls the
 * start_xmit() function of the assigned net_device.
 */
void ec_device_send(
        ec_device_t *device, /**< EtherCAT device */
        size_t size /**< number of bytes to send */
        )
{
    struct sk_buff *skb = device->tx_skb[device->tx_ring_index];

    // set the right length for the data
    skb->len = ETH_HLEN + size;

    if (unlikely(device->master->debug_level > 1)) {
        EC_MASTER_DBG(device->master, 2, "Sending frame:\n");
        ec_print_data(skb->data, ETH_HLEN + size);
    }

    // start sending
    if (device->dev->netdev_ops->ndo_start_xmit(skb, device->dev) ==
            NETDEV_TX_OK)
    {
        device->tx_count++;
        device->master->device_stats.tx_count++;
        device->tx_bytes += ETH_HLEN + size;
        device->master->device_stats.tx_bytes += ETH_HLEN + size;
#ifdef EC_DEBUG_IF
        ec_debug_send(&device->dbg, skb->data, ETH_HLEN + size);
#endif
#ifdef EC_DEBUG_RING
        ec_device_debug_ring_append(
                device, TX, skb->data + ETH_HLEN, size);
#endif
    } else {
        device->tx_errors++;
    }
}

/****************************************************************************/

/** Clears the frame statistics.
 */
void ec_device_clear_stats(
        ec_device_t *device /**< EtherCAT device */
        )
{
    unsigned int i;

    // zero frame statistics
    device->tx_count = 0;
    device->last_tx_count = 0;
    device->rx_count = 0;
    device->last_rx_count = 0;
    device->tx_bytes = 0;
    device->last_tx_bytes = 0;
    device->rx_bytes = 0;
    device->last_rx_bytes = 0;
    device->tx_errors = 0;

    for (i = 0; i < EC_RATE_COUNT; i++) {
        device->tx_frame_rates[i] = 0;
        device->rx_frame_rates[i] = 0;
        device->tx_byte_rates[i] = 0;
        device->rx_byte_rates[i] = 0;
    }
}

/****************************************************************************/

#ifdef EC_DEBUG_RING
/** Appends frame data to the debug ring.
 */
void ec_device_debug_ring_append(
        ec_device_t *device, /**< EtherCAT device */
        ec_debug_frame_dir_t dir, /**< direction */
        const void *data, /**< frame data */
        size_t size /**< data size */
        )
{
    ec_debug_frame_t *df = &device->debug_frames[device->debug_frame_index];

    df->dir = dir;
    if (dir == TX) {
        do_gettimeofday(&df->t);
    }
    else {
        df->t = device->timeval_poll;
    }
    memcpy(df->data, data, size);
    df->data_size = size;

    device->debug_frame_index++;
    device->debug_frame_index %= EC_DEBUG_RING_SIZE;
    if (unlikely(device->debug_frame_count < EC_DEBUG_RING_SIZE))
        device->debug_frame_count++;
}

/****************************************************************************/

/** Outputs the debug ring.
 */
void ec_device_debug_ring_print(
        const ec_device_t *device /**< EtherCAT device */
        )
{
    int i;
    unsigned int ring_index;
    const ec_debug_frame_t *df;
    struct timeval t0, diff;

    // calculate index of the newest frame in the ring to get its time
    ring_index = (device->debug_frame_index + EC_DEBUG_RING_SIZE - 1)
        % EC_DEBUG_RING_SIZE;
    t0 = device->debug_frames[ring_index].t;

    EC_MASTER_DBG(device->master, 1, "Debug ring %u:\n", ring_index);

    // calculate index of the oldest frame in the ring
    ring_index = (device->debug_frame_index + EC_DEBUG_RING_SIZE
            - device->debug_frame_count) % EC_DEBUG_RING_SIZE;

    for (i = 0; i < device->debug_frame_count; i++) {
        df = &device->debug_frames[ring_index];
        timersub(&t0, &df->t, &diff);

        EC_MASTER_DBG(device->master, 1, "Frame %u, dt=%u.%06u s, %s:\n",
                i + 1 - device->debug_frame_count,
                (unsigned int) diff.tv_sec,
                (unsigned int) diff.tv_usec,
                (df->dir == TX) ? "TX" : "RX");
        ec_print_data(df->data, df->data_size);

        ring_index++;
        ring_index %= EC_DEBUG_RING_SIZE;
    }
}
#endif

/****************************************************************************/

/** Calls the poll function of the assigned net_device.
 *
 * The master itself works without using interrupts. Therefore the processing
 * of received data and status changes of the network device has to be
 * done by the master calling the ISR "manually".
 */
void ec_device_poll(
        ec_device_t *device /**< EtherCAT device */
        )
{
#ifdef EC_HAVE_CYCLES
    device->cycles_poll = get_cycles();
#endif
    device->jiffies_poll = jiffies;
#ifdef EC_DEBUG_RING
    do_gettimeofday(&device->timeval_poll);
#endif
    device->poll(device->dev);
}

/****************************************************************************/

/** Update device statistics.
 */
void ec_device_update_stats(
        ec_device_t *device /**< EtherCAT device */
        )
{
    unsigned int i;

    s32 tx_frame_rate = (device->tx_count - device->last_tx_count) * 1000;
    s32 rx_frame_rate = (device->rx_count - device->last_rx_count) * 1000;
    s32 tx_byte_rate = (device->tx_bytes - device->last_tx_bytes);
    s32 rx_byte_rate = (device->rx_bytes - device->last_rx_bytes);

    /* Low-pass filter:
     *      Y_n = y_(n - 1) + T / tau * (x - y_(n - 1))   | T = 1
     *   -> Y_n += (x - y_(n - 1)) / tau
     */
    for (i = 0; i < EC_RATE_COUNT; i++) {
        s32 n = rate_intervals[i];
        device->tx_frame_rates[i] +=
            (tx_frame_rate - device->tx_frame_rates[i]) / n;
        device->rx_frame_rates[i] +=
            (rx_frame_rate - device->rx_frame_rates[i]) / n;
        device->tx_byte_rates[i] +=
            (tx_byte_rate - device->tx_byte_rates[i]) / n;
        device->rx_byte_rates[i] +=
            (rx_byte_rate - device->rx_byte_rates[i]) / n;
    }

    device->last_tx_count = device->tx_count;
    device->last_rx_count = device->rx_count;
    device->last_tx_bytes = device->tx_bytes;
    device->last_rx_bytes = device->rx_bytes;
}

/*****************************************************************************
 *  Device interface
 ****************************************************************************/

/** Withdraws an EtherCAT device from the master.
 *
 * The device is disconnected from the master and all device ressources
 * are freed.
 *
 * \attention Before calling this function, the ecdev_stop() function has
 *            to be called, to be sure that the master does not use the device
 *            any more.
 * \ingroup DeviceInterface
 */
void ecdev_withdraw(ec_device_t *device /**< EtherCAT device */)
{
    ec_master_t *master = device->master;
    char dev_str[20], mac_str[20];

    ec_mac_print(device->dev->dev_addr, mac_str);

    if (device == &master->devices[EC_DEVICE_MAIN]) {
        sprintf(dev_str, "main");
    } else if (device == &master->devices[EC_DEVICE_BACKUP]) {
        sprintf(dev_str, "backup");
    } else {
        EC_MASTER_WARN(master, "%s() called with unknown device %s!\n",
                __func__, mac_str);
        sprintf(dev_str, "UNKNOWN");
    }

    EC_MASTER_INFO(master, "Releasing %s device %s.\n", dev_str, mac_str);

    down(&master->device_sem);
    ec_device_detach(device);
    up(&master->device_sem);
}

/****************************************************************************/

/** Opens the network device and makes the master enter IDLE phase.
 *
 * \return 0 on success, else < 0
 * \ingroup DeviceInterface
 */
int ecdev_open(ec_device_t *device /**< EtherCAT device */)
{
    int ret;
    ec_master_t *master = device->master;
    unsigned int all_open = 1, dev_idx;

    ret = ec_device_open(device);
    if (ret) {
        EC_MASTER_ERR(master, "Failed to open device: error %d!\n", ret);
        return ret;
    }

    for (dev_idx = EC_DEVICE_MAIN;
            dev_idx < ec_master_num_devices(device->master); dev_idx++) {
        if (!master->devices[dev_idx].open) {
            all_open = 0;
            break;
        }
    }

    if (all_open) {
        ret = ec_master_enter_idle_phase(device->master);
        if (ret) {
            EC_MASTER_ERR(device->master, "Failed to enter IDLE phase!\n");
            return ret;
        }
    }

    return 0;
}

/****************************************************************************/

/** Makes the master leave IDLE phase and closes the network device.
 *
 * \return 0 on success, else < 0
 * \ingroup DeviceInterface
 */
void ecdev_close(ec_device_t *device /**< EtherCAT device */)
{
    ec_master_t *master = device->master;

    if (master->phase == EC_IDLE) {
        ec_master_leave_idle_phase(master);
    }

    if (ec_device_close(device)) {
        EC_MASTER_WARN(master, "Failed to close device!\n");
    }
}

/****************************************************************************/

/** Accepts a received frame.
 *
 * Forwards the received data to the master. The master will analyze the frame
 * and dispatch the received commands to the sending instances.
 *
 * The data have to begin with the Ethernet header (target MAC address).
 *
 * \ingroup DeviceInterface
 */
void ecdev_receive(
        ec_device_t *device, /**< EtherCAT device */
        const void *data, /**< pointer to received data */
        size_t size /**< number of bytes received */
        )
{
    const void *ec_data = data + ETH_HLEN;
    size_t ec_size = size - ETH_HLEN;

    if (unlikely(!data)) {
        EC_MASTER_WARN(device->master, "%s() called with NULL data.\n",
                __func__);
        return;
    }

    device->rx_count++;
    device->master->device_stats.rx_count++;
    device->rx_bytes += size;
    device->master->device_stats.rx_bytes += size;

    if (unlikely(device->master->debug_level > 1)) {
        EC_MASTER_DBG(device->master, 2, "Received frame:\n");
        ec_print_data(data, size);
    }

#ifdef EC_DEBUG_IF
    ec_debug_send(&device->dbg, data, size);
#endif
#ifdef EC_DEBUG_RING
    ec_device_debug_ring_append(device, RX, ec_data, ec_size);
#endif

    ec_master_receive_datagrams(device->master, device, ec_data, ec_size);
}

/****************************************************************************/

/** Sets a new link state.
 *
 * If the device notifies the master about the link being down, the master
 * will not try to send frames using this device.
 *
 * \ingroup DeviceInterface
 */
void ecdev_set_link(
        ec_device_t *device, /**< EtherCAT device */
        uint8_t state /**< new link state */
        )
{
    if (unlikely(!device)) {
        EC_WARN("ecdev_set_link() called with null device!\n");
        return;
    }

    if (likely(state != device->link_state)) {
        device->link_state = state;
        EC_MASTER_INFO(device->master,
                "Link state of %s changed to %s.\n",
                device->dev->name, (state ? "UP" : "DOWN"));
    }
}

/****************************************************************************/

/** Reads the link state.
 *
 * \ingroup DeviceInterface
 *
 * \return Link state.
 */
uint8_t ecdev_get_link(
        const ec_device_t *device /**< EtherCAT device */
        )
{
    if (unlikely(!device)) {
        EC_WARN("ecdev_get_link() called with null device!\n");
        return 0;
    }

    return device->link_state;
}

/****************************************************************************/

/** \cond */

EXPORT_SYMBOL(ecdev_withdraw);
EXPORT_SYMBOL(ecdev_open);
EXPORT_SYMBOL(ecdev_close);
EXPORT_SYMBOL(ecdev_receive);
EXPORT_SYMBOL(ecdev_get_link);
EXPORT_SYMBOL(ecdev_set_link);

/** \endcond */

/****************************************************************************/
