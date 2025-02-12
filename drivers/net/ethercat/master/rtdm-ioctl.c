/*****************************************************************************
 *
 *  Copyright (C) 2006-2024  Florian Pose, Ingenieurgemeinschaft IgH
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
 */

/**
   \file
   EtherCAT master character device.
*/

/****************************************************************************/

#include <linux/module.h>
#include <linux/vmalloc.h>

#include "master.h"
#include "slave_config.h"
#include "voe_handler.h"
#include "ethernet.h"
#include "ioctl.h"

/** Set to 1 to enable ioctl() latency tracing.
 *
 * Requires CPU timestamp counter!
 */
#define DEBUG_LATENCY 0

/** Optional compiler attributes fo ioctl() functions.
 */
#if 0
#define ATTRIBUTES __attribute__ ((__noinline__))
#else
#define ATTRIBUTES
#endif

#ifdef EC_IOCTL_RTDM
# include "rtdm_details.h"
/* RTDM does not support locking yet,
 * therefore no send/receive callbacks are set too. */
# define ec_ioctl_lock(lock) do {} while(0)
# define ec_ioctl_unlock(lock) do {} while(0)
# define ec_ioctl_lock_interruptible(lock) (0)
# define ec_copy_to_user(to, from, n, ctx) \
    rtdm_safe_copy_to_user(ec_ioctl_to_rtdm(ctx), to, from, n)
# define ec_copy_from_user(to, from, n, ctx) \
    rtdm_safe_copy_from_user(ec_ioctl_to_rtdm(ctx), to, from, n)
#else
# define ec_ioctl_lock(lock)   rt_mutex_lock(lock)
# define ec_ioctl_unlock(lock) rt_mutex_unlock(lock)
#  if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 17, 0) || \
      (defined(CONFIG_PREEMPT_RT_FULL) && LINUX_VERSION_CODE >= KERNEL_VERSION(3, 2, 0))
#   define ec_ioctl_lock_interruptible(lock) \
           rt_mutex_lock_interruptible(lock)
#  else
#   define ec_ioctl_lock_interruptible(lock) \
           rt_mutex_lock_interruptible(lock, 0)
# endif
# define ec_copy_to_user(to, from, n, ctx) copy_to_user(to, from, n)
# define ec_copy_from_user(to, from, n, ctx) copy_from_user(to, from, n)
#endif  // EC_IOCTL_RTDM

/****************************************************************************/

/** Copies a string to an ioctl structure.
 */
static void ec_ioctl_strcpy(
        char *target, /**< Target. */
        const char *source /**< Source. */
        )
{
    if (source) {
        strncpy(target, source, EC_IOCTL_STRING_SIZE);
        target[EC_IOCTL_STRING_SIZE - 1] = 0;
    } else {
        target[0] = 0;
    }
}

/****************************************************************************/

/** Get module information.
 *
 * \return Zero on success, otherwise a negative error code.
 */
static ATTRIBUTES int ec_ioctl_module(
        void *arg, /**< ioctl() argument. */
        ec_ioctl_context_t *ctx /**< Private data structure of file handle. */
        )
{
    ec_ioctl_module_t data;

    data.ioctl_version_magic = EC_IOCTL_VERSION_MAGIC;
    data.master_count = ec_master_count();

    if (ec_copy_to_user((void __user *) arg, &data, sizeof(data), ctx))
        return -EFAULT;

    return 0;
}

/****************************************************************************/

/** Get master information.
 *
 * \return Zero on success, otherwise a negative error code.
 */
static ATTRIBUTES int ec_ioctl_master(
        ec_master_t *master, /**< EtherCAT master. */
        void *arg /**< Userspace address to store the results. */
        )
{
    ec_ioctl_master_t io;
    unsigned int dev_idx, j;

    if (down_interruptible(&master->master_sem)) {
        return -EINTR;
    }

    io.slave_count = master->slave_count;
    io.scan_index = master->scan_index;
    io.config_count = ec_master_config_count(master);
    io.domain_count = ec_master_domain_count(master);
#ifdef EC_EOE
    io.eoe_handler_count = ec_master_eoe_handler_count(master);
#else
    io.eoe_handler_count = 0;
#endif
    io.phase = (uint8_t) master->phase;
    io.active = (uint8_t) master->active;
    io.scan_busy = master->scan_busy;

    up(&master->master_sem);

    if (down_interruptible(&master->device_sem)) {
        return -EINTR;
    }

    for (dev_idx = EC_DEVICE_MAIN;
            dev_idx < ec_master_num_devices(master); dev_idx++) {
        ec_device_t *device = &master->devices[dev_idx];

        if (device->dev) {
            memcpy(io.devices[dev_idx].address, device->dev->dev_addr,
                    ETH_ALEN);
        } else {
            memcpy(io.devices[dev_idx].address, master->macs[dev_idx],
                    ETH_ALEN);
        }
        io.devices[dev_idx].attached = device->dev ? 1 : 0;
        io.devices[dev_idx].link_state = device->link_state ? 1 : 0;
        io.devices[dev_idx].tx_count = device->tx_count;
        io.devices[dev_idx].rx_count = device->rx_count;
        io.devices[dev_idx].tx_bytes = device->tx_bytes;
        io.devices[dev_idx].rx_bytes = device->rx_bytes;
        io.devices[dev_idx].tx_errors = device->tx_errors;
        for (j = 0; j < EC_RATE_COUNT; j++) {
            io.devices[dev_idx].tx_frame_rates[j] =
                device->tx_frame_rates[j];
            io.devices[dev_idx].rx_frame_rates[j] =
                device->rx_frame_rates[j];
            io.devices[dev_idx].tx_byte_rates[j] =
                device->tx_byte_rates[j];
            io.devices[dev_idx].rx_byte_rates[j] =
                device->rx_byte_rates[j];
        }
    }
    io.num_devices = ec_master_num_devices(master);

    io.tx_count = master->device_stats.tx_count;
    io.rx_count = master->device_stats.rx_count;
    io.tx_bytes = master->device_stats.tx_bytes;
    io.rx_bytes = master->device_stats.rx_bytes;
    for (j = 0; j < EC_RATE_COUNT; j++) {
        io.tx_frame_rates[j] =
            master->device_stats.tx_frame_rates[j];
        io.rx_frame_rates[j] =
            master->device_stats.rx_frame_rates[j];
        io.tx_byte_rates[j] =
            master->device_stats.tx_byte_rates[j];
        io.rx_byte_rates[j] =
            master->device_stats.rx_byte_rates[j];
        io.loss_rates[j] =
            master->device_stats.loss_rates[j];
    }

    up(&master->device_sem);

    io.app_time = master->app_time;
    io.dc_ref_time = master->dc_ref_time;
    io.ref_clock =
        master->dc_ref_clock ? master->dc_ref_clock->ring_position : 0xffff;

    if (copy_to_user((void __user *) arg, &io, sizeof(io))) {
        return -EFAULT;
    }

    return 0;
}

/****************************************************************************/

/** Get slave information.
 *
 * \return Zero on success, otherwise a negative error code.
 */
static ATTRIBUTES int ec_ioctl_slave(
        ec_master_t *master, /**< EtherCAT master. */
        void *arg /**< Userspace address to store the results. */
        )
{
    ec_ioctl_slave_t data;
    const ec_slave_t *slave;
    int i;

    if (copy_from_user(&data, (void __user *) arg, sizeof(data))) {
        return -EFAULT;
    }

    if (down_interruptible(&master->master_sem))
        return -EINTR;

    if (!(slave = ec_master_find_slave_const(
                    master, 0, data.position))) {
        up(&master->master_sem);
        EC_MASTER_ERR(master, "Slave %u does not exist!\n", data.position);
        return -EINVAL;
    }

    data.device_index = slave->device_index;
    data.vendor_id = slave->sii.vendor_id;
    data.product_code = slave->sii.product_code;
    data.revision_number = slave->sii.revision_number;
    data.serial_number = slave->sii.serial_number;
    data.alias = slave->effective_alias;
    data.boot_rx_mailbox_offset = slave->sii.boot_rx_mailbox_offset;
    data.boot_rx_mailbox_size = slave->sii.boot_rx_mailbox_size;
    data.boot_tx_mailbox_offset = slave->sii.boot_tx_mailbox_offset;
    data.boot_tx_mailbox_size = slave->sii.boot_tx_mailbox_size;
    data.std_rx_mailbox_offset = slave->sii.std_rx_mailbox_offset;
    data.std_rx_mailbox_size = slave->sii.std_rx_mailbox_size;
    data.std_tx_mailbox_offset = slave->sii.std_tx_mailbox_offset;
    data.std_tx_mailbox_size = slave->sii.std_tx_mailbox_size;
    data.mailbox_protocols = slave->sii.mailbox_protocols;
    data.has_general_category = slave->sii.has_general;
    data.coe_details = slave->sii.coe_details;
    data.general_flags = slave->sii.general_flags;
    data.current_on_ebus = slave->sii.current_on_ebus;
    for (i = 0; i < EC_MAX_PORTS; i++) {
        data.ports[i].desc = slave->ports[i].desc;
        data.ports[i].link.link_up = slave->ports[i].link.link_up;
        data.ports[i].link.loop_closed = slave->ports[i].link.loop_closed;
        data.ports[i].link.signal_detected =
            slave->ports[i].link.signal_detected;
        data.ports[i].receive_time = slave->ports[i].receive_time;
        if (slave->ports[i].next_slave) {
            data.ports[i].next_slave =
                slave->ports[i].next_slave->ring_position;
        } else {
            data.ports[i].next_slave = 0xffff;
        }
        data.ports[i].delay_to_next_dc = slave->ports[i].delay_to_next_dc;
    }
    data.fmmu_bit = slave->base_fmmu_bit_operation;
    data.dc_supported = slave->base_dc_supported;
    data.dc_range = slave->base_dc_range;
    data.has_dc_system_time = slave->has_dc_system_time;
    data.transmission_delay = slave->transmission_delay;
    data.al_state = slave->current_state;
    data.error_flag = slave->error_flag;

    data.sync_count = slave->sii.sync_count;
    data.sdo_count = ec_slave_sdo_count(slave);
    data.sii_nwords = slave->sii_nwords;
    ec_ioctl_strcpy(data.group, slave->sii.group);
    ec_ioctl_strcpy(data.image, slave->sii.image);
    ec_ioctl_strcpy(data.order, slave->sii.order);
    ec_ioctl_strcpy(data.name, slave->sii.name);

    up(&master->master_sem);

    if (copy_to_user((void __user *) arg, &data, sizeof(data)))
        return -EFAULT;

    return 0;
}

/****************************************************************************/

/** Get slave sync manager information.
 *
 * \return Zero on success, otherwise a negative error code.
 */
static ATTRIBUTES int ec_ioctl_slave_sync(
        ec_master_t *master, /**< EtherCAT master. */
        void *arg /**< Userspace address to store the results. */
        )
{
    ec_ioctl_slave_sync_t data;
    const ec_slave_t *slave;
    const ec_sync_t *sync;

    if (copy_from_user(&data, (void __user *) arg, sizeof(data))) {
        return -EFAULT;
    }

    if (down_interruptible(&master->master_sem))
        return -EINTR;

    if (!(slave = ec_master_find_slave_const(
                    master, 0, data.slave_position))) {
        up(&master->master_sem);
        EC_MASTER_ERR(master, "Slave %u does not exist!\n",
                data.slave_position);
        return -EINVAL;
    }

    if (data.sync_index >= slave->sii.sync_count) {
        up(&master->master_sem);
        EC_SLAVE_ERR(slave, "Sync manager %u does not exist!\n",
                data.sync_index);
        return -EINVAL;
    }

    sync = &slave->sii.syncs[data.sync_index];

    data.physical_start_address = sync->physical_start_address;
    data.default_size = sync->default_length;
    data.control_register = sync->control_register;
    data.enable = sync->enable;
    data.pdo_count = ec_pdo_list_count(&sync->pdos);

    up(&master->master_sem);

    if (copy_to_user((void __user *) arg, &data, sizeof(data)))
        return -EFAULT;

    return 0;
}

/****************************************************************************/

/** Get slave sync manager PDO information.
 *
 * \return Zero on success, otherwise a negative error code.
 */
static ATTRIBUTES int ec_ioctl_slave_sync_pdo(
        ec_master_t *master, /**< EtherCAT master. */
        void *arg /**< Userspace address to store the results. */
        )
{
    ec_ioctl_slave_sync_pdo_t data;
    const ec_slave_t *slave;
    const ec_sync_t *sync;
    const ec_pdo_t *pdo;

    if (copy_from_user(&data, (void __user *) arg, sizeof(data))) {
        return -EFAULT;
    }

    if (down_interruptible(&master->master_sem))
        return -EINTR;

    if (!(slave = ec_master_find_slave_const(
                    master, 0, data.slave_position))) {
        up(&master->master_sem);
        EC_MASTER_ERR(master, "Slave %u does not exist!\n",
                data.slave_position);
        return -EINVAL;
    }

    if (data.sync_index >= slave->sii.sync_count) {
        up(&master->master_sem);
        EC_SLAVE_ERR(slave, "Sync manager %u does not exist!\n",
                data.sync_index);
        return -EINVAL;
    }

    sync = &slave->sii.syncs[data.sync_index];
    if (!(pdo = ec_pdo_list_find_pdo_by_pos_const(
                    &sync->pdos, data.pdo_pos))) {
        up(&master->master_sem);
        EC_SLAVE_ERR(slave, "Sync manager %u does not contain a PDO with "
                "position %u!\n", data.sync_index, data.pdo_pos);
        return -EINVAL;
    }

    data.index = pdo->index;
    data.entry_count = ec_pdo_entry_count(pdo);
    ec_ioctl_strcpy(data.name, pdo->name);

    up(&master->master_sem);

    if (copy_to_user((void __user *) arg, &data, sizeof(data)))
        return -EFAULT;

    return 0;
}

/****************************************************************************/

/** Get slave sync manager PDO entry information.
 *
 * \return Zero on success, otherwise a negative error code.
 */
static ATTRIBUTES int ec_ioctl_slave_sync_pdo_entry(
        ec_master_t *master, /**< EtherCAT master. */
        void *arg /**< Userspace address to store the results. */
        )
{
    ec_ioctl_slave_sync_pdo_entry_t data;
    const ec_slave_t *slave;
    const ec_sync_t *sync;
    const ec_pdo_t *pdo;
    const ec_pdo_entry_t *entry;

    if (copy_from_user(&data, (void __user *) arg, sizeof(data))) {
        return -EFAULT;
    }

    if (down_interruptible(&master->master_sem))
        return -EINTR;

    if (!(slave = ec_master_find_slave_const(
                    master, 0, data.slave_position))) {
        up(&master->master_sem);
        EC_MASTER_ERR(master, "Slave %u does not exist!\n",
                data.slave_position);
        return -EINVAL;
    }

    if (data.sync_index >= slave->sii.sync_count) {
        up(&master->master_sem);
        EC_SLAVE_ERR(slave, "Sync manager %u does not exist!\n",
                data.sync_index);
        return -EINVAL;
    }

    sync = &slave->sii.syncs[data.sync_index];
    if (!(pdo = ec_pdo_list_find_pdo_by_pos_const(
                    &sync->pdos, data.pdo_pos))) {
        up(&master->master_sem);
        EC_SLAVE_ERR(slave, "Sync manager %u does not contain a PDO with "
                "position %u!\n", data.sync_index, data.pdo_pos);
        return -EINVAL;
    }

    if (!(entry = ec_pdo_find_entry_by_pos_const(
                    pdo, data.entry_pos))) {
        up(&master->master_sem);
        EC_SLAVE_ERR(slave, "PDO 0x%04X does not contain an entry with "
                "position %u!\n", data.pdo_pos, data.entry_pos);
        return -EINVAL;
    }

    data.index = entry->index;
    data.subindex = entry->subindex;
    data.bit_length = entry->bit_length;
    ec_ioctl_strcpy(data.name, entry->name);

    up(&master->master_sem);

    if (copy_to_user((void __user *) arg, &data, sizeof(data)))
        return -EFAULT;

    return 0;
}

/****************************************************************************/

/** Get domain information.
 *
 * \return Zero on success, otherwise a negative error code.
 */
static ATTRIBUTES int ec_ioctl_domain(
        ec_master_t *master, /**< EtherCAT master. */
        void *arg /**< Userspace address to store the results. */
        )
{
    ec_ioctl_domain_t data;
    const ec_domain_t *domain;
    unsigned int dev_idx;

    if (copy_from_user(&data, (void __user *) arg, sizeof(data))) {
        return -EFAULT;
    }

    if (down_interruptible(&master->master_sem))
        return -EINTR;

    if (!(domain = ec_master_find_domain_const(master, data.index))) {
        up(&master->master_sem);
        EC_MASTER_ERR(master, "Domain %u does not exist!\n", data.index);
        return -EINVAL;
    }

    data.data_size = domain->data_size;
    data.logical_base_address = domain->logical_base_address;
    for (dev_idx = EC_DEVICE_MAIN;
            dev_idx < ec_master_num_devices(domain->master); dev_idx++) {
        data.working_counter[dev_idx] = domain->working_counter[dev_idx];
    }
    data.expected_working_counter = domain->expected_working_counter;
    data.fmmu_count = ec_domain_fmmu_count(domain);

    up(&master->master_sem);

    if (copy_to_user((void __user *) arg, &data, sizeof(data)))
        return -EFAULT;

    return 0;
}

/****************************************************************************/

/** Get domain FMMU information.
 *
 * \return Zero on success, otherwise a negative error code.
 */
static ATTRIBUTES int ec_ioctl_domain_fmmu(
        ec_master_t *master, /**< EtherCAT master. */
        void *arg /**< Userspace address to store the results. */
        )
{
    ec_ioctl_domain_fmmu_t data;
    const ec_domain_t *domain;
    const ec_fmmu_config_t *fmmu;

    if (copy_from_user(&data, (void __user *) arg, sizeof(data))) {
        return -EFAULT;
    }

    if (down_interruptible(&master->master_sem))
        return -EINTR;

    if (!(domain = ec_master_find_domain_const(master, data.domain_index))) {
        up(&master->master_sem);
        EC_MASTER_ERR(master, "Domain %u does not exist!\n",
                data.domain_index);
        return -EINVAL;
    }

    if (!(fmmu = ec_domain_find_fmmu(domain, data.fmmu_index))) {
        up(&master->master_sem);
        EC_MASTER_ERR(master, "Domain %u has less than %u"
                " fmmu configurations.\n",
                data.domain_index, data.fmmu_index + 1);
        return -EINVAL;
    }

    data.slave_config_alias = fmmu->sc->alias;
    data.slave_config_position = fmmu->sc->position;
    data.sync_index = fmmu->sync_index;
    data.dir = fmmu->dir;
    data.logical_address = fmmu->logical_start_address;
    data.data_size = fmmu->data_size;

    up(&master->master_sem);

    if (copy_to_user((void __user *) arg, &data, sizeof(data)))
        return -EFAULT;

    return 0;
}

/****************************************************************************/

/** Get domain data.
 *
 * \return Zero on success, otherwise a negative error code.
 */
static ATTRIBUTES int ec_ioctl_domain_data(
        ec_master_t *master, /**< EtherCAT master. */
        void *arg /**< Userspace address to store the results. */
        )
{
    ec_ioctl_domain_data_t data;
    const ec_domain_t *domain;

    if (copy_from_user(&data, (void __user *) arg, sizeof(data))) {
        return -EFAULT;
    }

    if (down_interruptible(&master->master_sem))
        return -EINTR;

    if (!(domain = ec_master_find_domain_const(master, data.domain_index))) {
        up(&master->master_sem);
        EC_MASTER_ERR(master, "Domain %u does not exist!\n",
                data.domain_index);
        return -EINVAL;
    }

    if (domain->data_size != data.data_size) {
        up(&master->master_sem);
        EC_MASTER_ERR(master, "Data size mismatch %u/%zu!\n",
                data.data_size, domain->data_size);
        return -EFAULT;
    }

    if (copy_to_user((void __user *) data.target, domain->data,
                domain->data_size)) {
        up(&master->master_sem);
        return -EFAULT;
    }

    up(&master->master_sem);
    return 0;
}

/****************************************************************************/

/** Set master debug level.
 *
 * \return Zero on success, otherwise a negative error code.
 */
static ATTRIBUTES int ec_ioctl_master_debug(
        ec_master_t *master, /**< EtherCAT master. */
        void *arg /**< ioctl() argument. */
        )
{
    return ec_master_debug_level(master, (unsigned long) arg);
}

/****************************************************************************/

/** Issue a bus scan.
 *
 * \return Always zero (success).
 */
static ATTRIBUTES int ec_ioctl_master_rescan(
        ec_master_t *master, /**< EtherCAT master. */
        void *arg /**< ioctl() argument. */
        )
{
    master->fsm.rescan_required = 1;
    return 0;
}

/****************************************************************************/

/** Set slave state.
 *
 * \return Zero on success, otherwise a negative error code.
 */
static ATTRIBUTES int ec_ioctl_slave_state(
        ec_master_t *master, /**< EtherCAT master. */
        void *arg /**< ioctl() argument. */
        )
{
    ec_ioctl_slave_state_t data;
    ec_slave_t *slave;

    if (copy_from_user(&data, (void __user *) arg, sizeof(data))) {
        return -EFAULT;
    }

    if (down_interruptible(&master->master_sem))
        return -EINTR;

    if (!(slave = ec_master_find_slave(
                    master, 0, data.slave_position))) {
        up(&master->master_sem);
        EC_MASTER_ERR(master, "Slave %u does not exist!\n",
                data.slave_position);
        return -EINVAL;
    }

    ec_slave_request_state(slave, data.al_state);

    up(&master->master_sem);
    return 0;
}

/****************************************************************************/

/** Get slave SDO information.
 *
 * \return Zero on success, otherwise a negative error code.
 */
static ATTRIBUTES int ec_ioctl_slave_sdo(
        ec_master_t *master, /**< EtherCAT master. */
        void *arg /**< ioctl() argument. */
        )
{
    ec_ioctl_slave_sdo_t data;
    const ec_slave_t *slave;
    const ec_sdo_t *sdo;

    if (copy_from_user(&data, (void __user *) arg, sizeof(data))) {
        return -EFAULT;
    }

    if (down_interruptible(&master->master_sem))
        return -EINTR;

    if (!(slave = ec_master_find_slave_const(
                    master, 0, data.slave_position))) {
        up(&master->master_sem);
        EC_MASTER_ERR(master, "Slave %u does not exist!\n",
                data.slave_position);
        return -EINVAL;
    }

    if (!(sdo = ec_slave_get_sdo_by_pos_const(
                    slave, data.sdo_position))) {
        up(&master->master_sem);
        EC_SLAVE_ERR(slave, "SDO %u does not exist!\n", data.sdo_position);
        return -EINVAL;
    }

    data.sdo_index = sdo->index;
    data.max_subindex = sdo->max_subindex;
    ec_ioctl_strcpy(data.name, sdo->name);

    up(&master->master_sem);

    if (copy_to_user((void __user *) arg, &data, sizeof(data)))
        return -EFAULT;

    return 0;
}

/****************************************************************************/

/** Get slave SDO entry information.
 *
 * \return Zero on success, otherwise a negative error code.
 */
static ATTRIBUTES int ec_ioctl_slave_sdo_entry(
        ec_master_t *master, /**< EtherCAT master. */
        void *arg /**< ioctl() argument. */
        )
{
    ec_ioctl_slave_sdo_entry_t data;
    const ec_slave_t *slave;
    const ec_sdo_t *sdo;
    const ec_sdo_entry_t *entry;

    if (copy_from_user(&data, (void __user *) arg, sizeof(data))) {
        return -EFAULT;
    }

    if (down_interruptible(&master->master_sem))
        return -EINTR;

    if (!(slave = ec_master_find_slave_const(
                    master, 0, data.slave_position))) {
        up(&master->master_sem);
        EC_MASTER_ERR(master, "Slave %u does not exist!\n",
                data.slave_position);
        return -EINVAL;
    }

    if (data.sdo_spec <= 0) {
        if (!(sdo = ec_slave_get_sdo_by_pos_const(
                        slave, -data.sdo_spec))) {
            up(&master->master_sem);
            EC_SLAVE_ERR(slave, "SDO %u does not exist!\n", -data.sdo_spec);
            return -EINVAL;
        }
    } else {
        if (!(sdo = ec_slave_get_sdo_const(
                        slave, data.sdo_spec))) {
            up(&master->master_sem);
            EC_SLAVE_ERR(slave, "SDO 0x%04X does not exist!\n",
                    data.sdo_spec);
            return -EINVAL;
        }
    }

    if (!(entry = ec_sdo_get_entry_const(
                    sdo, data.sdo_entry_subindex))) {
        up(&master->master_sem);
        EC_SLAVE_ERR(slave, "SDO entry 0x%04X:%02X does not exist!\n",
                sdo->index, data.sdo_entry_subindex);
        return -EINVAL;
    }

    data.data_type = entry->data_type;
    data.bit_length = entry->bit_length;
    data.read_access[EC_SDO_ENTRY_ACCESS_PREOP] =
        entry->read_access[EC_SDO_ENTRY_ACCESS_PREOP];
    data.read_access[EC_SDO_ENTRY_ACCESS_SAFEOP] =
        entry->read_access[EC_SDO_ENTRY_ACCESS_SAFEOP];
    data.read_access[EC_SDO_ENTRY_ACCESS_OP] =
        entry->read_access[EC_SDO_ENTRY_ACCESS_OP];
    data.write_access[EC_SDO_ENTRY_ACCESS_PREOP] =
        entry->write_access[EC_SDO_ENTRY_ACCESS_PREOP];
    data.write_access[EC_SDO_ENTRY_ACCESS_SAFEOP] =
        entry->write_access[EC_SDO_ENTRY_ACCESS_SAFEOP];
    data.write_access[EC_SDO_ENTRY_ACCESS_OP] =
        entry->write_access[EC_SDO_ENTRY_ACCESS_OP];
    ec_ioctl_strcpy(data.description, entry->description);

    up(&master->master_sem);

    if (copy_to_user((void __user *) arg, &data, sizeof(data)))
        return -EFAULT;

    return 0;
}

/****************************************************************************/

/** Upload SDO.
 *
 * \return Zero on success, otherwise a negative error code.
 */
static ATTRIBUTES int ec_ioctl_slave_sdo_upload(
        ec_master_t *master, /**< EtherCAT master. */
        void *arg /**< ioctl() argument. */
        )
{
    ec_ioctl_slave_sdo_upload_t data;
    uint8_t *target;
    int ret;

    if (copy_from_user(&data, (void __user *) arg, sizeof(data))) {
        return -EFAULT;
    }

    if (!(target = kmalloc(data.target_size, GFP_KERNEL))) {
        EC_MASTER_ERR(master, "Failed to allocate %zu bytes"
                " for SDO upload.\n", data.target_size);
        return -ENOMEM;
    }

    ret = ecrt_master_sdo_upload(master, data.slave_position,
            data.sdo_index, data.sdo_entry_subindex, target,
            data.target_size, &data.data_size, &data.abort_code);

    if (!ret) {
        if (copy_to_user((void __user *) data.target,
                    target, data.data_size)) {
            kfree(target);
            return -EFAULT;
        }
    }

    kfree(target);

    if (__copy_to_user((void __user *) arg, &data, sizeof(data))) {
        return -EFAULT;
    }

    return ret;
}

/****************************************************************************/

/** Download SDO.
 *
 * \return Zero on success, otherwise a negative error code.
 */
static ATTRIBUTES int ec_ioctl_slave_sdo_download(
        ec_master_t *master, /**< EtherCAT master. */
        void *arg /**< ioctl() argument. */
        )
{
    ec_ioctl_slave_sdo_download_t data;
    uint8_t *sdo_data;
    int retval;

    if (copy_from_user(&data, (void __user *) arg, sizeof(data))) {
        return -EFAULT;
    }

    if (!(sdo_data = kmalloc(data.data_size, GFP_KERNEL))) {
        EC_MASTER_ERR(master, "Failed to allocate %zu bytes"
                " for SDO download.\n", data.data_size);
        return -ENOMEM;
    }

    if (copy_from_user(sdo_data, (void __user *) data.data, data.data_size)) {
        kfree(sdo_data);
        return -EFAULT;
    }

    if (data.complete_access) {
        retval = ecrt_master_sdo_download_complete(master, data.slave_position,
                data.sdo_index, sdo_data, data.data_size, &data.abort_code);
    } else {
        retval = ecrt_master_sdo_download(master, data.slave_position,
                data.sdo_index, data.sdo_entry_subindex, sdo_data,
                data.data_size, &data.abort_code);
    }

    kfree(sdo_data);

    if (__copy_to_user((void __user *) arg, &data, sizeof(data))) {
        retval = -EFAULT;
    }

    return retval;
}

/****************************************************************************/

/** Read a slave's SII.
 *
 * \return Zero on success, otherwise a negative error code.
 */
static ATTRIBUTES int ec_ioctl_slave_sii_read(
        ec_master_t *master, /**< EtherCAT master. */
        void *arg /**< ioctl() argument. */
        )
{
    ec_ioctl_slave_sii_t data;
    const ec_slave_t *slave;
    int retval;

    if (copy_from_user(&data, (void __user *) arg, sizeof(data))) {
        return -EFAULT;
    }

    if (down_interruptible(&master->master_sem))
        return -EINTR;

    if (!(slave = ec_master_find_slave_const(
                    master, 0, data.slave_position))) {
        up(&master->master_sem);
        EC_MASTER_ERR(master, "Slave %u does not exist!\n",
                data.slave_position);
        return -EINVAL;
    }

    if (!data.nwords
            || data.offset + data.nwords > slave->sii_nwords) {
        up(&master->master_sem);
        EC_SLAVE_ERR(slave, "Invalid SII read offset/size %u/%u for slave SII"
                " size %zu!\n", data.offset, data.nwords, slave->sii_nwords);
        return -EINVAL;
    }

    if (copy_to_user((void __user *) data.words,
                slave->sii_words + data.offset, data.nwords * 2))
        retval = -EFAULT;
    else
        retval = 0;

    up(&master->master_sem);
    return retval;
}

/****************************************************************************/

/** Write a slave's SII.
 *
 * \return Zero on success, otherwise a negative error code.
 */
static ATTRIBUTES int ec_ioctl_slave_sii_write(
        ec_master_t *master, /**< EtherCAT master. */
        void *arg /**< ioctl() argument. */
        )
{
    ec_ioctl_slave_sii_t data;
    ec_slave_t *slave;
    unsigned int byte_size;
    uint16_t *words;
    ec_sii_write_request_t request;

    if (copy_from_user(&data, (void __user *) arg, sizeof(data))) {
        return -EFAULT;
    }

    if (!data.nwords) {
        return 0;
    }

    byte_size = sizeof(uint16_t) * data.nwords;
    if (!(words = kmalloc(byte_size, GFP_KERNEL))) {
        EC_MASTER_ERR(master, "Failed to allocate %u bytes"
                " for SII contents.\n", byte_size);
        return -ENOMEM;
    }

    if (copy_from_user(words,
                (void __user *) data.words, byte_size)) {
        kfree(words);
        return -EFAULT;
    }

    if (down_interruptible(&master->master_sem)) {
        kfree(words);
        return -EINTR;
    }

    if (!(slave = ec_master_find_slave(
                    master, 0, data.slave_position))) {
        up(&master->master_sem);
        EC_MASTER_ERR(master, "Slave %u does not exist!\n",
                data.slave_position);
        kfree(words);
        return -EINVAL;
    }

    // init SII write request
    INIT_LIST_HEAD(&request.list);
    request.slave = slave;
    request.words = words;
    request.offset = data.offset;
    request.nwords = data.nwords;
    request.state = EC_INT_REQUEST_QUEUED;

    // schedule SII write request.
    list_add_tail(&request.list, &master->sii_requests);

    up(&master->master_sem);

    // wait for processing through FSM
    if (wait_event_interruptible(master->request_queue,
                request.state != EC_INT_REQUEST_QUEUED)) {
        // interrupted by signal
        down(&master->master_sem);
        if (request.state == EC_INT_REQUEST_QUEUED) {
            // abort request
            list_del(&request.list);
            up(&master->master_sem);
            kfree(words);
            return -EINTR;
        }
        up(&master->master_sem);
    }

    // wait until master FSM has finished processing
    wait_event(master->request_queue, request.state != EC_INT_REQUEST_BUSY);

    kfree(words);

    return request.state == EC_INT_REQUEST_SUCCESS ? 0 : -EIO;
}

/****************************************************************************/

/** Read a slave's registers.
 *
 * \return Zero on success, otherwise a negative error code.
 */
static ATTRIBUTES int ec_ioctl_slave_reg_read(
        ec_master_t *master, /**< EtherCAT master. */
        void *arg /**< ioctl() argument. */
        )
{
    ec_ioctl_slave_reg_t io;
    ec_slave_t *slave;
    ec_reg_request_t request;
    int ret;

    if (copy_from_user(&io, (void __user *) arg, sizeof(io))) {
        return -EFAULT;
    }

    if (!io.size) {
        return 0;
    }

    // init register request
    ret = ec_reg_request_init(&request, io.size);
    if (ret) {
        return ret;
    }

    ret = ecrt_reg_request_read(&request, io.address, io.size);
    if (ret) {
        return ret;
    }

    if (down_interruptible(&master->master_sem)) {
        ec_reg_request_clear(&request);
        return -EINTR;
    }

    if (!(slave = ec_master_find_slave(
                    master, 0, io.slave_position))) {
        up(&master->master_sem);
        ec_reg_request_clear(&request);
        EC_MASTER_ERR(master, "Slave %u does not exist!\n",
                io.slave_position);
        return -EINVAL;
    }

    // schedule request.
    list_add_tail(&request.list, &slave->reg_requests);

    up(&master->master_sem);

    // wait for processing through FSM
    if (wait_event_interruptible(master->request_queue,
                request.state != EC_INT_REQUEST_QUEUED)) {
        // interrupted by signal
        down(&master->master_sem);
        if (request.state == EC_INT_REQUEST_QUEUED) {
            // abort request
            list_del(&request.list);
            up(&master->master_sem);
            ec_reg_request_clear(&request);
            return -EINTR;
        }
        up(&master->master_sem);
    }

    // wait until master FSM has finished processing
    wait_event(master->request_queue, request.state != EC_INT_REQUEST_BUSY);

    if (request.state == EC_INT_REQUEST_SUCCESS) {
        if (copy_to_user((void __user *) io.data, request.data, io.size)) {
            return -EFAULT;
        }
    }
    ec_reg_request_clear(&request);

    return request.state == EC_INT_REQUEST_SUCCESS ? 0 : -EIO;
}

/****************************************************************************/

/** Write a slave's registers.
 *
 * \return Zero on success, otherwise a negative error code.
 */
static ATTRIBUTES int ec_ioctl_slave_reg_write(
        ec_master_t *master, /**< EtherCAT master. */
        void *arg /**< ioctl() argument. */
        )
{
    ec_ioctl_slave_reg_t io;
    ec_slave_t *slave;
    ec_reg_request_t request;
    int ret;

    if (copy_from_user(&io, (void __user *) arg, sizeof(io))) {
        return -EFAULT;
    }

    if (!io.size) {
        return 0;
    }

    // init register request
    ret = ec_reg_request_init(&request, io.size);
    if (ret) {
        return ret;
    }

    if (copy_from_user(request.data, (void __user *) io.data, io.size)) {
        ec_reg_request_clear(&request);
        return -EFAULT;
    }

    ret = ecrt_reg_request_write(&request, io.address, io.size);
    if (ret) {
        return ret;
    }

    if (down_interruptible(&master->master_sem)) {
        ec_reg_request_clear(&request);
        return -EINTR;
    }

    if (io.emergency) {
        request.ring_position = io.slave_position;
        // schedule request.
        list_add_tail(&request.list, &master->emerg_reg_requests);
    }
    else {
        if (!(slave = ec_master_find_slave(master, 0, io.slave_position))) {
            up(&master->master_sem);
            ec_reg_request_clear(&request);
            EC_MASTER_ERR(master, "Slave %u does not exist!\n",
                    io.slave_position);
            return -EINVAL;
        }

        // schedule request.
        list_add_tail(&request.list, &slave->reg_requests);
    }

    up(&master->master_sem);

    // wait for processing through FSM
    if (wait_event_interruptible(master->request_queue,
                request.state != EC_INT_REQUEST_QUEUED)) {
        // interrupted by signal
        down(&master->master_sem);
        if (request.state == EC_INT_REQUEST_QUEUED) {
            // abort request
            list_del(&request.list);
            up(&master->master_sem);
            ec_reg_request_clear(&request);
            return -EINTR;
        }
        up(&master->master_sem);
    }

    // wait until master FSM has finished processing
    wait_event(master->request_queue, request.state != EC_INT_REQUEST_BUSY);

    ec_reg_request_clear(&request);

    return request.state == EC_INT_REQUEST_SUCCESS ? 0 : -EIO;
}

/****************************************************************************/

/** Get slave configuration information.
 *
 * \return Zero on success, otherwise a negative error code.
 */
static ATTRIBUTES int ec_ioctl_config(
        ec_master_t *master, /**< EtherCAT master. */
        void *arg /**< ioctl() argument. */
        )
{
    ec_ioctl_config_t data;
    const ec_slave_config_t *sc;
    uint8_t i;

    if (copy_from_user(&data, (void __user *) arg, sizeof(data))) {
        return -EFAULT;
    }

    if (down_interruptible(&master->master_sem))
        return -EINTR;

    if (!(sc = ec_master_get_config_const(
                    master, data.config_index))) {
        up(&master->master_sem);
        EC_MASTER_ERR(master, "Slave config %u does not exist!\n",
                data.config_index);
        return -EINVAL;
    }

    data.alias = sc->alias;
    data.position = sc->position;
    data.vendor_id = sc->vendor_id;
    data.product_code = sc->product_code;
    for (i = 0; i < EC_MAX_SYNC_MANAGERS; i++) {
        data.syncs[i].dir = sc->sync_configs[i].dir;
        data.syncs[i].watchdog_mode = sc->sync_configs[i].watchdog_mode;
        data.syncs[i].pdo_count =
            ec_pdo_list_count(&sc->sync_configs[i].pdos);
    }
    data.watchdog_divider = sc->watchdog_divider;
    data.watchdog_intervals = sc->watchdog_intervals;
    data.sdo_count = ec_slave_config_sdo_count(sc);
    data.idn_count = ec_slave_config_idn_count(sc);
    data.flag_count = ec_slave_config_flag_count(sc);
    data.slave_position = sc->slave ? sc->slave->ring_position : -1;
    data.dc_assign_activate = sc->dc_assign_activate;
    for (i = 0; i < EC_SYNC_SIGNAL_COUNT; i++) {
        data.dc_sync[i] = sc->dc_sync[i];
    }

    up(&master->master_sem);

    if (copy_to_user((void __user *) arg, &data, sizeof(data)))
        return -EFAULT;

    return 0;
}

/****************************************************************************/

/** Get slave configuration PDO information.
 *
 * \return Zero on success, otherwise a negative error code.
 */
static ATTRIBUTES int ec_ioctl_config_pdo(
        ec_master_t *master, /**< EtherCAT master. */
        void *arg /**< ioctl() argument. */
        )
{
    ec_ioctl_config_pdo_t data;
    const ec_slave_config_t *sc;
    const ec_pdo_t *pdo;

    if (copy_from_user(&data, (void __user *) arg, sizeof(data))) {
        return -EFAULT;
    }

    if (data.sync_index >= EC_MAX_SYNC_MANAGERS) {
        EC_MASTER_ERR(master, "Invalid sync manager index %u!\n",
                data.sync_index);
        return -EINVAL;
    }

    if (down_interruptible(&master->master_sem))
        return -EINTR;

    if (!(sc = ec_master_get_config_const(
                    master, data.config_index))) {
        up(&master->master_sem);
        EC_MASTER_ERR(master, "Slave config %u does not exist!\n",
                data.config_index);
        return -EINVAL;
    }

    if (!(pdo = ec_pdo_list_find_pdo_by_pos_const(
                    &sc->sync_configs[data.sync_index].pdos,
                    data.pdo_pos))) {
        up(&master->master_sem);
        EC_MASTER_ERR(master, "Invalid PDO position!\n");
        return -EINVAL;
    }

    data.index = pdo->index;
    data.entry_count = ec_pdo_entry_count(pdo);
    ec_ioctl_strcpy(data.name, pdo->name);

    up(&master->master_sem);

    if (copy_to_user((void __user *) arg, &data, sizeof(data)))
        return -EFAULT;

    return 0;
}

/****************************************************************************/

/** Get slave configuration PDO entry information.
 *
 * \return Zero on success, otherwise a negative error code.
 */
static ATTRIBUTES int ec_ioctl_config_pdo_entry(
        ec_master_t *master, /**< EtherCAT master. */
        void *arg /**< ioctl() argument. */
        )
{
    ec_ioctl_config_pdo_entry_t data;
    const ec_slave_config_t *sc;
    const ec_pdo_t *pdo;
    const ec_pdo_entry_t *entry;

    if (copy_from_user(&data, (void __user *) arg, sizeof(data))) {
        return -EFAULT;
    }

    if (data.sync_index >= EC_MAX_SYNC_MANAGERS) {
        EC_MASTER_ERR(master, "Invalid sync manager index %u!\n",
                data.sync_index);
        return -EINVAL;
    }

    if (down_interruptible(&master->master_sem))
        return -EINTR;

    if (!(sc = ec_master_get_config_const(
                    master, data.config_index))) {
        up(&master->master_sem);
        EC_MASTER_ERR(master, "Slave config %u does not exist!\n",
                data.config_index);
        return -EINVAL;
    }

    if (!(pdo = ec_pdo_list_find_pdo_by_pos_const(
                    &sc->sync_configs[data.sync_index].pdos,
                    data.pdo_pos))) {
        up(&master->master_sem);
        EC_MASTER_ERR(master, "Invalid PDO position!\n");
        return -EINVAL;
    }

    if (!(entry = ec_pdo_find_entry_by_pos_const(
                    pdo, data.entry_pos))) {
        up(&master->master_sem);
        EC_MASTER_ERR(master, "Entry not found!\n");
        return -EINVAL;
    }

    data.index = entry->index;
    data.subindex = entry->subindex;
    data.bit_length = entry->bit_length;
    ec_ioctl_strcpy(data.name, entry->name);

    up(&master->master_sem);

    if (copy_to_user((void __user *) arg, &data, sizeof(data)))
        return -EFAULT;

    return 0;
}

/****************************************************************************/

/** Get slave configuration SDO information.
 *
 * \return Zero on success, otherwise a negative error code.
 */
static ATTRIBUTES int ec_ioctl_config_sdo(
        ec_master_t *master, /**< EtherCAT master. */
        void *arg /**< ioctl() argument. */
        )
{
    ec_ioctl_config_sdo_t *ioctl;
    const ec_slave_config_t *sc;
    const ec_sdo_request_t *req;

    if (!(ioctl = kmalloc(sizeof(*ioctl), GFP_KERNEL))) {
        return -ENOMEM;
    }

    if (copy_from_user(ioctl, (void __user *) arg, sizeof(*ioctl))) {
        kfree(ioctl);
        return -EFAULT;
    }

    if (down_interruptible(&master->master_sem)) {
        kfree(ioctl);
        return -EINTR;
    }

    if (!(sc = ec_master_get_config_const(
                    master, ioctl->config_index))) {
        up(&master->master_sem);
        EC_MASTER_ERR(master, "Slave config %u does not exist!\n",
                ioctl->config_index);
        kfree(ioctl);
        return -EINVAL;
    }

    if (!(req = ec_slave_config_get_sdo_by_pos_const(
                    sc, ioctl->sdo_pos))) {
        up(&master->master_sem);
        EC_MASTER_ERR(master, "Invalid SDO position!\n");
        kfree(ioctl);
        return -EINVAL;
    }

    ioctl->index = req->index;
    ioctl->subindex = req->subindex;
    ioctl->size = req->data_size;
    memcpy(ioctl->data, req->data,
            min((u32) ioctl->size, (u32) EC_MAX_SDO_DATA_SIZE));
    ioctl->complete_access = req->complete_access;

    up(&master->master_sem);

    if (copy_to_user((void __user *) arg, ioctl, sizeof(*ioctl))) {
        kfree(ioctl);
        return -EFAULT;
    }

    kfree(ioctl);
    return 0;
}

/****************************************************************************/

/** Get slave configuration IDN information.
 *
 * \return Zero on success, otherwise a negative error code.
 */
static ATTRIBUTES int ec_ioctl_config_idn(
        ec_master_t *master, /**< EtherCAT master. */
        void *arg /**< ioctl() argument. */
        )
{
    ec_ioctl_config_idn_t *ioctl;
    const ec_slave_config_t *sc;
    const ec_soe_request_t *req;

    if (!(ioctl = kmalloc(sizeof(*ioctl), GFP_KERNEL))) {
        return -ENOMEM;
    }

    if (copy_from_user(ioctl, (void __user *) arg, sizeof(*ioctl))) {
        kfree(ioctl);
        return -EFAULT;
    }

    if (down_interruptible(&master->master_sem)) {
        kfree(ioctl);
        return -EINTR;
    }

    if (!(sc = ec_master_get_config_const(
                    master, ioctl->config_index))) {
        up(&master->master_sem);
        EC_MASTER_ERR(master, "Slave config %u does not exist!\n",
                ioctl->config_index);
        kfree(ioctl);
        return -EINVAL;
    }

    if (!(req = ec_slave_config_get_idn_by_pos_const(
                    sc, ioctl->idn_pos))) {
        up(&master->master_sem);
        EC_MASTER_ERR(master, "Invalid IDN position!\n");
        kfree(ioctl);
        return -EINVAL;
    }

    ioctl->drive_no = req->drive_no;
    ioctl->idn = req->idn;
    ioctl->state = req->al_state;
    ioctl->size = req->data_size;
    memcpy(ioctl->data, req->data,
            min((u32) ioctl->size, (u32) EC_MAX_IDN_DATA_SIZE));

    up(&master->master_sem);

    if (copy_to_user((void __user *) arg, ioctl, sizeof(*ioctl))) {
        kfree(ioctl);
        return -EFAULT;
    }

    kfree(ioctl);
    return 0;
}

/****************************************************************************/

/** Get slave configuration feature flag information.
 *
 * \return Zero on success, otherwise a negative error code.
 */
static ATTRIBUTES int ec_ioctl_config_flag(
        ec_master_t *master, /**< EtherCAT master. */
        void *arg /**< ioctl() argument. */
        )
{
    ec_ioctl_config_flag_t *ioctl;
    const ec_slave_config_t *sc;
    const ec_flag_t *flag;
    size_t size;

    if (!(ioctl = kmalloc(sizeof(*ioctl), GFP_KERNEL))) {
        return -ENOMEM;
    }

    if (copy_from_user(ioctl, (void __user *) arg, sizeof(*ioctl))) {
        kfree(ioctl);
        return -EFAULT;
    }

    if (down_interruptible(&master->master_sem)) {
        kfree(ioctl);
        return -EINTR;
    }

    if (!(sc = ec_master_get_config_const(
                    master, ioctl->config_index))) {
        up(&master->master_sem);
        EC_MASTER_ERR(master, "Slave config %u does not exist!\n",
                ioctl->config_index);
        kfree(ioctl);
        return -EINVAL;
    }

    if (!(flag = ec_slave_config_get_flag_by_pos_const(
                    sc, ioctl->flag_pos))) {
        up(&master->master_sem);
        EC_MASTER_ERR(master, "Invalid flag position!\n");
        kfree(ioctl);
        return -EINVAL;
    }

    size = min((u32) strlen(flag->key), (u32) EC_MAX_FLAG_KEY_SIZE - 1);
    memcpy(ioctl->key, flag->key, size);
    ioctl->key[size] = 0x00;
    ioctl->value = flag->value;

    up(&master->master_sem);

    if (copy_to_user((void __user *) arg, ioctl, sizeof(*ioctl))) {
        kfree(ioctl);
        return -EFAULT;
    }

    kfree(ioctl);
    return 0;
}

/****************************************************************************/

#ifdef EC_EOE

/** Get configured EoE IP parameters for a given slave configuration.
 *
 * \return Zero on success, otherwise a negative error code.
 */
static ATTRIBUTES int ec_ioctl_config_ip(
        ec_master_t *master, /**< EtherCAT master. */
        void *arg /**< ioctl() argument. */
        )
{
    ec_ioctl_eoe_ip_t *ioctl;
    const ec_slave_config_t *sc;
    const ec_eoe_request_t *req;

    if (!(ioctl = kmalloc(sizeof(*ioctl), GFP_KERNEL))) {
        return -ENOMEM;
    }

    if (copy_from_user(ioctl, (void __user *) arg, sizeof(*ioctl))) {
        kfree(ioctl);
        return -EFAULT;
    }

    if (down_interruptible(&master->master_sem)) {
        kfree(ioctl);
        return -EINTR;
    }

    if (!(sc = ec_master_get_config_const(master, ioctl->config_index))) {
        up(&master->master_sem);
        EC_MASTER_ERR(master, "Slave config %u does not exist!\n",
                ioctl->config_index);
        kfree(ioctl);
        return -EINVAL;
    }

    req = &sc->eoe_ip_param_request;

    ioctl->mac_address_included = req->mac_address_included;
    ioctl->ip_address_included = req->ip_address_included;
    ioctl->subnet_mask_included = req->subnet_mask_included;
    ioctl->gateway_included = req->gateway_included;
    ioctl->dns_included = req->dns_included;
    ioctl->name_included = req->name_included;

    memcpy(ioctl->mac_address, req->mac_address, EC_ETH_ALEN);
    ioctl->ip_address = req->ip_address;
    ioctl->subnet_mask = req->subnet_mask;
    ioctl->gateway = req->gateway;
    ioctl->dns = req->dns;
    strncpy(ioctl->name, req->name, EC_MAX_HOSTNAME_SIZE);

    up(&master->master_sem);

    if (copy_to_user((void __user *) arg, ioctl, sizeof(*ioctl))) {
        kfree(ioctl);
        return -EFAULT;
    }

    kfree(ioctl);
    return 0;
}

#endif

/****************************************************************************/

#ifdef EC_EOE

/** Get EoE handler information.
 *
 * \return Zero on success, otherwise a negative error code.
 */
static ATTRIBUTES int ec_ioctl_eoe_handler(
        ec_master_t *master, /**< EtherCAT master. */
        void *arg /**< ioctl() argument. */
        )
{
    ec_ioctl_eoe_handler_t data;
    const ec_eoe_t *eoe;

    if (copy_from_user(&data, (void __user *) arg, sizeof(data))) {
        return -EFAULT;
    }

    if (down_interruptible(&master->master_sem))
        return -EINTR;

    if (!(eoe = ec_master_get_eoe_handler_const(master, data.eoe_index))) {
        up(&master->master_sem);
        EC_MASTER_ERR(master, "EoE handler %u does not exist!\n",
                data.eoe_index);
        return -EINVAL;
    }

    if (eoe->slave) {
        data.slave_position = eoe->slave->ring_position;
    } else {
        data.slave_position = 0xffff;
    }
    snprintf(data.name, EC_DATAGRAM_NAME_SIZE, eoe->dev->name);
    data.open = eoe->opened;
    data.rx_bytes = eoe->stats.tx_bytes;
    data.rx_rate = eoe->tx_rate;
    data.tx_bytes = eoe->stats.rx_bytes;
    data.tx_rate = eoe->tx_rate;
    data.tx_queued_frames = eoe->tx_queued_frames;
    data.tx_queue_size = eoe->tx_queue_size;

    up(&master->master_sem);

    if (copy_to_user((void __user *) arg, &data, sizeof(data)))
        return -EFAULT;

    return 0;
}

#endif

/****************************************************************************/

#ifdef EC_EOE
/** Request EoE IP parameter setting.
 *
 * \return Zero on success, otherwise a negative error code.
 */
static ATTRIBUTES int ec_ioctl_slave_eoe_ip_param(
        ec_master_t *master, /**< EtherCAT master. */
        void *arg /**< ioctl() argument. */
        )
{
    ec_ioctl_eoe_ip_t io;
    ec_eoe_request_t req;
    ec_slave_t *slave;

    if (copy_from_user(&io, (void __user *) arg, sizeof(io))) {
        return -EFAULT;
    }

    // init EoE request
    ec_eoe_request_init(&req);

    req.mac_address_included = io.mac_address_included;
    req.ip_address_included = io.ip_address_included;
    req.subnet_mask_included = io.subnet_mask_included;
    req.gateway_included = io.gateway_included;
    req.dns_included = io.dns_included;
    req.name_included = io.name_included;

    memcpy(req.mac_address, io.mac_address, EC_ETH_ALEN);
    req.ip_address = io.ip_address;
    req.subnet_mask = io.subnet_mask;
    req.gateway = io.gateway;
    req.dns = io.dns;
    memcpy(req.name, io.name, EC_MAX_HOSTNAME_SIZE);

    req.state = EC_INT_REQUEST_QUEUED;

    if (down_interruptible(&master->master_sem)) {
        return -EINTR;
    }

    if (!(slave = ec_master_find_slave(
                    master, 0, io.slave_position))) {
        up(&master->master_sem);
        EC_MASTER_ERR(master, "Slave %u does not exist!\n",
                io.slave_position);
        return -EINVAL;
    }

    EC_MASTER_DBG(master, 1, "Scheduling EoE request.\n");

    // schedule request.
    list_add_tail(&req.list, &slave->eoe_requests);

    up(&master->master_sem);

    // wait for processing through FSM
    if (wait_event_interruptible(master->request_queue,
                req.state != EC_INT_REQUEST_QUEUED)) {
        // interrupted by signal
        down(&master->master_sem);
        if (req.state == EC_INT_REQUEST_QUEUED) {
            // abort request
            list_del(&req.list);
            up(&master->master_sem);
            return -EINTR;
        }
        up(&master->master_sem);
    }

    // wait until master FSM has finished processing
    wait_event(master->request_queue, req.state != EC_INT_REQUEST_BUSY);

    io.result = req.result;

    if (copy_to_user((void __user *) arg, &io, sizeof(io))) {
        return -EFAULT;
    }

    return req.state == EC_INT_REQUEST_SUCCESS ? 0 : -EIO;
}
#endif

/*****************************************************************************/

/** Request the master from userspace.
 *
 * \return Zero on success, otherwise a negative error code.
 */
static ATTRIBUTES int ec_ioctl_request(
        ec_master_t *master, /**< EtherCAT master. */
        void *arg, /**< ioctl() argument. */
        ec_ioctl_context_t *ctx /**< Private data structure of file handle. */
        )
{
    ec_master_t *m;
    int ret = 0;

    m = ecrt_request_master_err(master->index);
    if (IS_ERR(m)) {
        ret = PTR_ERR(m);
    } else {
        ctx->requested = 1;
    }

    return ret;
}

/****************************************************************************/

/** Create a domain.
 *
 * \return Domain index on success, otherwise a negative error code.
 */
static ATTRIBUTES int ec_ioctl_create_domain(
        ec_master_t *master, /**< EtherCAT master. */
        void *arg, /**< ioctl() argument. */
        ec_ioctl_context_t *ctx /**< Private data structure of file handle. */
        )
{
    ec_domain_t *domain;

    if (unlikely(!ctx->requested))
        return -EPERM;

    domain = ecrt_master_create_domain_err(master);
    if (IS_ERR(domain))
        return PTR_ERR(domain);

    return domain->index;
}

/****************************************************************************/

/** Create a slave configuration.
 *
 * \return Zero on success, otherwise a negative error code.
 */
static ATTRIBUTES int ec_ioctl_create_slave_config(
        ec_master_t *master, /**< EtherCAT master. */
        void *arg, /**< ioctl() argument. */
        ec_ioctl_context_t *ctx /**< Private data structure of file handle. */
        )
{
    ec_ioctl_config_t data;
    ec_slave_config_t *sc, *entry;

    if (unlikely(!ctx->requested))
        return -EPERM;

    if (copy_from_user(&data, (void __user *) arg, sizeof(data))) {
        return -EFAULT;
    }

    sc = ecrt_master_slave_config_err(master, data.alias, data.position,
            data.vendor_id, data.product_code);
    if (IS_ERR(sc))
        return PTR_ERR(sc);

    data.config_index = 0;

    if (down_interruptible(&master->master_sem))
        return -EINTR;

    list_for_each_entry(entry, &master->configs, list) {
        if (entry == sc)
            break;
        data.config_index++;
    }

    up(&master->master_sem);

    if (copy_to_user((void __user *) arg, &data, sizeof(data)))
        return -EFAULT;

    return 0;
}

/****************************************************************************/

/** Select the DC reference clock.
 *
 * \return Zero on success, otherwise a negative error code.
 */
static ATTRIBUTES int ec_ioctl_select_ref_clock(
        ec_master_t *master, /**< EtherCAT master. */
        void *arg, /**< ioctl() argument. */
        ec_ioctl_context_t *ctx /**< Private data structure of file handle. */
        )
{
    unsigned long config_index = (unsigned long) arg;
    ec_slave_config_t *sc = NULL;
    int ret = 0;

    if (unlikely(!ctx->requested)) {
        ret = -EPERM;
        goto out_return;
    }

    if (down_interruptible(&master->master_sem)) {
        ret = -EINTR;
        goto out_return;
    }

    if (config_index != 0xFFFFFFFF) {
        if (!(sc = ec_master_get_config(master, config_index))) {
            ret = -ENOENT;
            goto out_up;
        }
    }

    ecrt_master_select_reference_clock(master, sc);

out_up:
    up(&master->master_sem);
out_return:
    return ret;
}

/****************************************************************************/

/** Activates the master.
 *
 * \return Zero on success, otherwise a negative error code.
 */
static ATTRIBUTES int ec_ioctl_activate(
        ec_master_t *master, /**< EtherCAT master. */
        void *arg, /**< ioctl() argument. */
        ec_ioctl_context_t *ctx /**< Private data structure of file handle. */
        )
{
    ec_ioctl_master_activate_t io;
    ec_domain_t *domain;
    off_t offset;
    int ret;

    if (unlikely(!ctx->requested))
        return -EPERM;

    io.process_data = NULL;

    /* Get the sum of the domains' process data sizes. */

    ctx->process_data_size = 0;

    if (down_interruptible(&master->master_sem))
        return -EINTR;

    list_for_each_entry(domain, &master->domains, list) {
        ctx->process_data_size += ecrt_domain_size(domain);
    }

    up(&master->master_sem);

    if (ctx->process_data_size) {
        ctx->process_data = vmalloc(ctx->process_data_size);
        if (!ctx->process_data) {
            ctx->process_data_size = 0;
            return -ENOMEM;
        }

        /* Set the memory as external process data memory for the
         * domains.
         */
        offset = 0;
        list_for_each_entry(domain, &master->domains, list) {
            ecrt_domain_external_memory(domain,
                    ctx->process_data + offset);
            offset += ecrt_domain_size(domain);
        }

#if defined(EC_IOCTL_RTDM) && !defined(EC_RTDM_XENOMAI_V3)
        /* RTDM uses a different approach for memory-mapping, which has to be
         * initiated by the kernel.
         */
        ret = ec_rtdm_mmap(ctx, &io.process_data);
        if (ret < 0) {
            EC_MASTER_ERR(master, "Failed to map process data"
                    " memory to user space (code %i).\n", ret);
            return ret;
        }
#endif
    }

    io.process_data_size = ctx->process_data_size;

#ifndef EC_IOCTL_RTDM
    /* RTDM does not support locking yet. */
    ecrt_master_callbacks(master, ec_master_internal_send_cb,
            ec_master_internal_receive_cb, master);
#endif

    ret = ecrt_master_activate(master);
    if (ret < 0)
        return ret;

    if (copy_to_user((void __user *) arg, &io,
                sizeof(ec_ioctl_master_activate_t)))
        return -EFAULT;

    return 0;
}

/****************************************************************************/

/** Deactivates the master.
 *
 * \return Zero on success, otherwise a negative error code.
 */
static ATTRIBUTES int ec_ioctl_deactivate(
        ec_master_t *master, /**< EtherCAT master. */
        void *arg, /**< ioctl() argument. */
        ec_ioctl_context_t *ctx /**< Private data structure of file handle. */
        )
{
    if (unlikely(!ctx->requested))
        return -EPERM;

    return ecrt_master_deactivate(master);
}

/****************************************************************************/

/** Set max. number of databytes in a cycle
 *
 * \return Zero on success, otherwise a negative error code.
 */
static ATTRIBUTES int ec_ioctl_set_send_interval(
        ec_master_t *master, /**< EtherCAT master. */
        void *arg, /**< ioctl() argument. */
        ec_ioctl_context_t *ctx /**< Private data structure of file handle. */
        )
{
    size_t send_interval;

    if (unlikely(!ctx->requested)) {
        return -EPERM;
    }

    if (copy_from_user(&send_interval, (void __user *) arg,
                sizeof(send_interval))) {
        return -EFAULT;
    }

    if (down_interruptible(&master->master_sem))
        return -EINTR;

    ec_master_set_send_interval(master, send_interval);

    up(&master->master_sem);
    return 0;
}

/****************************************************************************/

/** Send frames.
 *
 * \return Zero on success, otherwise a negative error code.
 */
static ATTRIBUTES int ec_ioctl_send(
        ec_master_t *master, /**< EtherCAT master. */
        void *arg, /**< ioctl() argument. */
        ec_ioctl_context_t *ctx /**< Private data structure of file handle. */
        )
{
    int ret;

    if (unlikely(!ctx->requested)) {
        return -EPERM;
    }

    if (ec_ioctl_lock_interruptible(&master->io_mutex))
        return -EINTR;

    ret = ecrt_master_send(master);
    ec_ioctl_unlock(&master->io_mutex);
    return ret;
}

/****************************************************************************/

/** Receive frames.
 *
 * \return Zero on success, otherwise a negative error code.
 */
static ATTRIBUTES int ec_ioctl_receive(
        ec_master_t *master, /**< EtherCAT master. */
        void *arg, /**< ioctl() argument. */
        ec_ioctl_context_t *ctx /**< Private data structure of file handle. */
        )
{
    int ret;

    if (unlikely(!ctx->requested)) {
        return -EPERM;
    }

    if (ec_ioctl_lock_interruptible(&master->io_mutex))
        return -EINTR;

    ret = ecrt_master_receive(master);
    ec_ioctl_unlock(&master->io_mutex);
    return ret;
}

/****************************************************************************/

/** Get the master state.
 *
 * \return Zero on success, otherwise a negative error code.
 */
static ATTRIBUTES int ec_ioctl_master_state(
        ec_master_t *master, /**< EtherCAT master. */
        void *arg, /**< ioctl() argument. */
        ec_ioctl_context_t *ctx /**< Private data structure of file handle. */
        )
{
    ec_master_state_t data;
    int ret;

    ret = ecrt_master_state(master, &data);
    if (ret)
        return ret;

    if (ec_copy_to_user((void __user *) arg, &data, sizeof(data), ctx))
        return -EFAULT;

    return 0;
}

/****************************************************************************/

/** Get the link state.
 *
 * \return Zero on success, otherwise a negative error code.
 */
static ATTRIBUTES int ec_ioctl_master_link_state(
        ec_master_t *master, /**< EtherCAT master. */
        void *arg, /**< ioctl() argument. */
        ec_ioctl_context_t *ctx /**< Private data structure of file handle. */
        )
{
    ec_ioctl_link_state_t ioctl;
    ec_master_link_state_t state;
    int ret;

    if (ec_copy_from_user(&ioctl, (void __user *) arg, sizeof(ioctl), ctx)) {
        return -EFAULT;
    }

    ret = ecrt_master_link_state(master, ioctl.dev_idx, &state);
    if (ret < 0) {
        return ret;
    }

    if (ec_copy_to_user((void __user *) ioctl.state,
                        &state, sizeof(state), ctx)) {
        return -EFAULT;
    }

    return 0;
}

/****************************************************************************/

/** Set the master DC application time.
 *
 * \return Zero on success, otherwise a negative error code.
 */
static ATTRIBUTES int ec_ioctl_app_time(
        ec_master_t *master, /**< EtherCAT master. */
        void *arg, /**< ioctl() argument. */
        ec_ioctl_context_t *ctx /**< Private data structure of file handle. */
        )
{
    uint64_t time;

    if (unlikely(!ctx->requested))
        return -EPERM;

    if (ec_copy_from_user(&time, (void __user *) arg, sizeof(time), ctx)) {
        return -EFAULT;
    }

    return ecrt_master_application_time(master, time);
}

/****************************************************************************/

/** Sync the reference clock.
 *
 * \return Zero on success, otherwise a negative error code.
 */
static ATTRIBUTES int ec_ioctl_sync_ref(
        ec_master_t *master, /**< EtherCAT master. */
        void *arg, /**< ioctl() argument. */
        ec_ioctl_context_t *ctx /**< Private data structure of file handle. */
        )
{
    int ret;

    if (unlikely(!ctx->requested)) {
        return -EPERM;
    }

    if (ec_ioctl_lock_interruptible(&master->io_mutex))
        return -EINTR;

    ret = ecrt_master_sync_reference_clock(master);
    ec_ioctl_unlock(&master->io_mutex);
    return ret;
}

/****************************************************************************/

/** Sync the reference clock.
 *
 * \return Zero on success, otherwise a negative error code.
 */
static ATTRIBUTES int ec_ioctl_sync_ref_to(
        ec_master_t *master, /**< EtherCAT master. */
        void *arg, /**< ioctl() argument. */
        ec_ioctl_context_t *ctx /**< Private data structure of file handle. */
        )
{
    int ret;
    uint64_t time;

    if (unlikely(!ctx->requested))
        return -EPERM;

    if (ec_copy_from_user(&time, (void __user *) arg, sizeof(time), ctx)) {
        return -EFAULT;
    }

    if (ec_ioctl_lock_interruptible(&master->io_mutex))
        return -EINTR;

    ret = ecrt_master_sync_reference_clock_to(master, time);
    ec_ioctl_unlock(&master->io_mutex);
    return ret;
}

/****************************************************************************/

/** Sync the slave clocks.
 *
 * \return Zero on success, otherwise a negative error code.
 */
static ATTRIBUTES int ec_ioctl_sync_slaves(
        ec_master_t *master, /**< EtherCAT master. */
        void *arg, /**< ioctl() argument. */
        ec_ioctl_context_t *ctx /**< Private data structure of file handle. */
        )
{
    int ret;

    if (unlikely(!ctx->requested)) {
        return -EPERM;
    }

    if (ec_ioctl_lock_interruptible(&master->io_mutex))
        return -EINTR;

    ret = ecrt_master_sync_slave_clocks(master);
    ec_ioctl_unlock(&master->io_mutex);
    return ret;
}

/****************************************************************************/

/** Get the system time of the reference clock.
 *
 * \return Zero on success, otherwise a negative error code.
 */
static ATTRIBUTES int ec_ioctl_ref_clock_time(
        ec_master_t *master, /**< EtherCAT master. */
        void *arg, /**< ioctl() argument. */
        ec_ioctl_context_t *ctx /**< Private data structure of file handle. */
        )
{
    uint32_t time;
    int ret;

    if (unlikely(!ctx->requested)) {
        return -EPERM;
    }

    ret = ecrt_master_reference_clock_time(master, &time);
    if (ret) {
        return ret;
    }

    if (ec_copy_to_user((void __user *) arg, &time, sizeof(time), ctx)) {
        return -EFAULT;
    }

    return 0;
}

/****************************************************************************/

/** Queue the sync monitoring datagram.
 *
 * \return Zero on success, otherwise a negative error code.
 */
static ATTRIBUTES int ec_ioctl_sync_mon_queue(
        ec_master_t *master, /**< EtherCAT master. */
        void *arg, /**< ioctl() argument. */
        ec_ioctl_context_t *ctx /**< Private data structure of file handle. */
        )
{
    int ret;

    if (unlikely(!ctx->requested)) {
        return -EPERM;
    }

    if (ec_ioctl_lock_interruptible(&master->io_mutex))
        return -EINTR;

    ret = ecrt_master_sync_monitor_queue(master);
    ec_ioctl_unlock(&master->io_mutex);
    return ret;
}

/****************************************************************************/

/** Processes the sync monitoring datagram.
 *
 * \return Zero on success, otherwise a negative error code.
 */
static ATTRIBUTES int ec_ioctl_sync_mon_process(
        ec_master_t *master, /**< EtherCAT master. */
        void *arg, /**< ioctl() argument. */
        ec_ioctl_context_t *ctx /**< Private data structure of file handle. */
        )
{
    uint32_t time_diff;

    if (unlikely(!ctx->requested))
        return -EPERM;

    time_diff = ecrt_master_sync_monitor_process(master);

    if (ec_copy_to_user((void __user *) arg, &time_diff,
                        sizeof(time_diff), ctx))
        return -EFAULT;

    return 0;
}

/****************************************************************************/

/** Reset configuration.
 *
 * \return Always zero (success).
 */
static ATTRIBUTES int ec_ioctl_reset(
        ec_master_t *master, /**< EtherCAT master. */
        void *arg, /**< ioctl() argument. */
        ec_ioctl_context_t *ctx /**< Private data structure of file handle. */
        )
{
#ifdef EC_IOCTL_RTDM
    /* Xenomai/LXRT is like NMI context, so we do a two-stage schedule. */
    irq_work_queue(&master->sc_reset_work_kicker);
#else
    schedule_work(&master->sc_reset_work);
#endif
    return 0;
}

/****************************************************************************/

/** Configure a sync manager.
 *
 * \return Zero on success, otherwise a negative error code.
 */
static ATTRIBUTES int ec_ioctl_sc_sync(
        ec_master_t *master, /**< EtherCAT master. */
        void *arg, /**< ioctl() argument. */
        ec_ioctl_context_t *ctx /**< Private data structure of file handle. */
        )
{
    ec_ioctl_config_t data;
    ec_slave_config_t *sc;
    unsigned int i;
    int ret = 0;

    if (unlikely(!ctx->requested)) {
        ret = -EPERM;
        goto out_return;
    }

    if (copy_from_user(&data, (void __user *) arg, sizeof(data))) {
        ret = -EFAULT;
        goto out_return;
    }

    if (down_interruptible(&master->master_sem)) {
        ret = -EINTR;
        goto out_return;
    }

    if (!(sc = ec_master_get_config(master, data.config_index))) {
        ret = -ENOENT;
        goto out_up;
    }

    for (i = 0; i < EC_MAX_SYNC_MANAGERS; i++) {
        if (data.syncs[i].config_this) {
            ret = ecrt_slave_config_sync_manager(sc, i, data.syncs[i].dir,
                        data.syncs[i].watchdog_mode);
            if (ret) {
                goto out_up;
            }
        }
    }

out_up:
    up(&master->master_sem);
out_return:
    return ret;
}

/****************************************************************************/

/** Configure a slave's watchdogs.
 *
 * \return Zero on success, otherwise a negative error code.
 */
static ATTRIBUTES int ec_ioctl_sc_watchdog(
        ec_master_t *master, /**< EtherCAT master. */
        void *arg, /**< ioctl() argument. */
        ec_ioctl_context_t *ctx /**< Private data structure of file handle. */
        )
{
    ec_ioctl_config_t data;
    ec_slave_config_t *sc;
    int ret = 0;

    if (unlikely(!ctx->requested)) {
        ret = -EPERM;
        goto out_return;
    }

    if (copy_from_user(&data, (void __user *) arg, sizeof(data))) {
        ret = -EFAULT;
        goto out_return;
    }

    if (down_interruptible(&master->master_sem)) {
        ret = -EINTR;
        goto out_return;
    }

    if (!(sc = ec_master_get_config(master, data.config_index))) {
        ret = -ENOENT;
        goto out_up;
    }

    ret = ecrt_slave_config_watchdog(sc,
            data.watchdog_divider, data.watchdog_intervals);

out_up:
    up(&master->master_sem);
out_return:
    return ret;
}

/****************************************************************************/

/** Add a PDO to the assignment.
 *
 * \return Zero on success, otherwise a negative error code.
 */
static ATTRIBUTES int ec_ioctl_sc_add_pdo(
        ec_master_t *master, /**< EtherCAT master. */
        void *arg, /**< ioctl() argument. */
        ec_ioctl_context_t *ctx /**< Private data structure of file handle. */
        )
{
    ec_ioctl_config_pdo_t data;
    ec_slave_config_t *sc;

    if (unlikely(!ctx->requested))
        return -EPERM;

    if (copy_from_user(&data, (void __user *) arg, sizeof(data)))
        return -EFAULT;

    if (down_interruptible(&master->master_sem))
        return -EINTR;

    if (!(sc = ec_master_get_config(master, data.config_index))) {
        up(&master->master_sem);
        return -ENOENT;
    }

    up(&master->master_sem); /** \todo sc could be invalidated */

    return ecrt_slave_config_pdo_assign_add(sc, data.sync_index, data.index);
}

/****************************************************************************/

/** Clears the PDO assignment.
 *
 * \return Zero on success, otherwise a negative error code.
 */
static ATTRIBUTES int ec_ioctl_sc_clear_pdos(
        ec_master_t *master, /**< EtherCAT master. */
        void *arg, /**< ioctl() argument. */
        ec_ioctl_context_t *ctx /**< Private data structure of file handle. */
        )
{
    ec_ioctl_config_pdo_t data;
    ec_slave_config_t *sc;

    if (unlikely(!ctx->requested))
        return -EPERM;

    if (copy_from_user(&data, (void __user *) arg, sizeof(data)))
        return -EFAULT;

    if (down_interruptible(&master->master_sem))
        return -EINTR;

    if (!(sc = ec_master_get_config(master, data.config_index))) {
        up(&master->master_sem);
        return -ENOENT;
    }

    up(&master->master_sem); /** \todo sc could be invalidated */

    return ecrt_slave_config_pdo_assign_clear(sc, data.sync_index);
}

/****************************************************************************/

/** Add an entry to a PDO's mapping.
 *
 * \return Zero on success, otherwise a negative error code.
 */
static ATTRIBUTES int ec_ioctl_sc_add_entry(
        ec_master_t *master, /**< EtherCAT master. */
        void *arg, /**< ioctl() argument. */
        ec_ioctl_context_t *ctx /**< Private data structure of file handle. */
        )
{
    ec_ioctl_add_pdo_entry_t data;
    ec_slave_config_t *sc;

    if (unlikely(!ctx->requested))
        return -EPERM;

    if (copy_from_user(&data, (void __user *) arg, sizeof(data)))
        return -EFAULT;

    if (down_interruptible(&master->master_sem))
        return -EINTR;

    if (!(sc = ec_master_get_config(master, data.config_index))) {
        up(&master->master_sem);
        return -ENOENT;
    }

    up(&master->master_sem); /** \todo sc could be invalidated */

    return ecrt_slave_config_pdo_mapping_add(sc, data.pdo_index,
            data.entry_index, data.entry_subindex, data.entry_bit_length);
}

/****************************************************************************/

/** Clears the mapping of a PDO.
 *
 * \return Zero on success, otherwise a negative error code.
 */
static ATTRIBUTES int ec_ioctl_sc_clear_entries(
        ec_master_t *master, /**< EtherCAT master. */
        void *arg, /**< ioctl() argument. */
        ec_ioctl_context_t *ctx /**< Private data structure of file handle. */
        )
{
    ec_ioctl_config_pdo_t data;
    ec_slave_config_t *sc;

    if (unlikely(!ctx->requested))
        return -EPERM;

    if (copy_from_user(&data, (void __user *) arg, sizeof(data)))
        return -EFAULT;

    if (down_interruptible(&master->master_sem))
        return -EINTR;

    if (!(sc = ec_master_get_config(master, data.config_index))) {
        up(&master->master_sem);
        return -ENOENT;
    }

    up(&master->master_sem); /** \todo sc could be invalidated */

    return ecrt_slave_config_pdo_mapping_clear(sc, data.index);
}

/****************************************************************************/

/** Registers a PDO entry.
 *
 * \return Process data offset on success, otherwise a negative error code.
 */
static ATTRIBUTES int ec_ioctl_sc_reg_pdo_entry(
        ec_master_t *master, /**< EtherCAT master. */
        void *arg, /**< ioctl() argument. */
        ec_ioctl_context_t *ctx /**< Private data structure of file handle. */
        )
{
    ec_ioctl_reg_pdo_entry_t data;
    ec_slave_config_t *sc;
    ec_domain_t *domain;
    int ret;

    if (unlikely(!ctx->requested))
        return -EPERM;

    if (copy_from_user(&data, (void __user *) arg, sizeof(data)))
        return -EFAULT;

    if (down_interruptible(&master->master_sem))
        return -EINTR;

    if (!(sc = ec_master_get_config(master, data.config_index))) {
        up(&master->master_sem);
        return -ENOENT;
    }

    if (!(domain = ec_master_find_domain(master, data.domain_index))) {
        up(&master->master_sem);
        return -ENOENT;
    }

    up(&master->master_sem); /** \todo sc or domain could be invalidated */

    ret = ecrt_slave_config_reg_pdo_entry(sc, data.entry_index,
            data.entry_subindex, domain, &data.bit_position);

    if (copy_to_user((void __user *) arg, &data, sizeof(data)))
        return -EFAULT;

    return ret;
}

/****************************************************************************/

/** Registers a PDO entry by its position.
 *
 * \return Process data offset on success, otherwise a negative error code.
 */
static ATTRIBUTES int ec_ioctl_sc_reg_pdo_pos(
        ec_master_t *master, /**< EtherCAT master. */
        void *arg, /**< ioctl() argument. */
        ec_ioctl_context_t *ctx /**< Private data structure of file handle. */
        )
{
    ec_ioctl_reg_pdo_pos_t io;
    ec_slave_config_t *sc;
    ec_domain_t *domain;
    int ret;

    if (unlikely(!ctx->requested)) {
        return -EPERM;
    }

    if (copy_from_user(&io, (void __user *) arg, sizeof(io))) {
        return -EFAULT;
    }

    if (down_interruptible(&master->master_sem)) {
        return -EINTR;
    }

    if (!(sc = ec_master_get_config(master, io.config_index))) {
        up(&master->master_sem);
        return -ENOENT;
    }

    if (!(domain = ec_master_find_domain(master, io.domain_index))) {
        up(&master->master_sem);
        return -ENOENT;
    }

    up(&master->master_sem); /** \todo sc or domain could be invalidated */

    ret = ecrt_slave_config_reg_pdo_entry_pos(sc, io.sync_index,
            io.pdo_pos, io.entry_pos, domain, &io.bit_position);

    if (copy_to_user((void __user *) arg, &io, sizeof(io)))
        return -EFAULT;

    return ret;
}

/****************************************************************************/

/** Sets the DC AssignActivate word and the sync signal times.
 *
 * \return Zero on success, otherwise a negative error code.
 */
static ATTRIBUTES int ec_ioctl_sc_dc(
        ec_master_t *master, /**< EtherCAT master. */
        void *arg, /**< ioctl() argument. */
        ec_ioctl_context_t *ctx /**< Private data structure of file handle. */
        )
{
    ec_ioctl_config_t data;
    ec_slave_config_t *sc;
    int ret;

    if (unlikely(!ctx->requested))
        return -EPERM;

    if (copy_from_user(&data, (void __user *) arg, sizeof(data)))
        return -EFAULT;

    if (down_interruptible(&master->master_sem))
        return -EINTR;

    if (!(sc = ec_master_get_config(master, data.config_index))) {
        up(&master->master_sem);
        return -ENOENT;
    }

    ret = ecrt_slave_config_dc(sc, data.dc_assign_activate,
            data.dc_sync[0].cycle_time,
            data.dc_sync[0].shift_time,
            data.dc_sync[1].cycle_time,
            data.dc_sync[1].shift_time);

    up(&master->master_sem);

    return ret;
}

/****************************************************************************/

/** Configures an SDO.
 *
 * \return Zero on success, otherwise a negative error code.
 */
static ATTRIBUTES int ec_ioctl_sc_sdo(
        ec_master_t *master, /**< EtherCAT master. */
        void *arg, /**< ioctl() argument. */
        ec_ioctl_context_t *ctx /**< Private data structure of file handle. */
        )
{
    ec_ioctl_sc_sdo_t data;
    ec_slave_config_t *sc;
    uint8_t *sdo_data = NULL;
    int ret;

    if (unlikely(!ctx->requested))
        return -EPERM;

    if (copy_from_user(&data, (void __user *) arg, sizeof(data)))
        return -EFAULT;

    if (!data.size)
        return -EINVAL;

    if (!(sdo_data = kmalloc(data.size, GFP_KERNEL))) {
        return -ENOMEM;
    }

    if (copy_from_user(sdo_data, (void __user *) data.data, data.size)) {
        kfree(sdo_data);
        return -EFAULT;
    }

    if (down_interruptible(&master->master_sem)) {
        kfree(sdo_data);
        return -EINTR;
    }

    if (!(sc = ec_master_get_config(master, data.config_index))) {
        up(&master->master_sem);
        kfree(sdo_data);
        return -ENOENT;
    }

    up(&master->master_sem); /** \todo sc could be invalidated */

    if (data.complete_access) {
        ret = ecrt_slave_config_complete_sdo(sc,
                data.index, sdo_data, data.size);
    } else {
        ret = ecrt_slave_config_sdo(sc, data.index, data.subindex, sdo_data,
                data.size);
    }
    kfree(sdo_data);
    return ret;
}

/****************************************************************************/

/** Set the emergency ring buffer size.
 *
 * \return Zero on success, otherwise a negative error code.
 */
static ATTRIBUTES int ec_ioctl_sc_emerg_size(
        ec_master_t *master, /**< EtherCAT master. */
        void *arg, /**< ioctl() argument. */
        ec_ioctl_context_t *ctx /**< Private data structure of file handle. */
        )
{
    ec_ioctl_sc_emerg_t io;
    ec_slave_config_t *sc;
    int ret;

    if (unlikely(!ctx->requested))
        return -EPERM;

    if (copy_from_user(&io, (void __user *) arg, sizeof(io)))
        return -EFAULT;

    if (down_interruptible(&master->master_sem)) {
        return -EINTR;
    }

    if (!(sc = ec_master_get_config(master, io.config_index))) {
        up(&master->master_sem);
        return -ENOENT;
    }

    ret = ecrt_slave_config_emerg_size(sc, io.size);

    up(&master->master_sem);

    return ret;
}

/****************************************************************************/

/** Get an emergency message from the ring.
 *
 * \return Zero on success, otherwise a negative error code.
 */
static ATTRIBUTES int ec_ioctl_sc_emerg_pop(
        ec_master_t *master, /**< EtherCAT master. */
        void *arg, /**< ioctl() argument. */
        ec_ioctl_context_t *ctx /**< Private data structure of file handle. */
        )
{
    ec_ioctl_sc_emerg_t io;
    ec_slave_config_t *sc;
    u8 msg[EC_COE_EMERGENCY_MSG_SIZE];
    int ret;

    if (unlikely(!ctx->requested)) {
        return -EPERM;
    }

    if (ec_copy_from_user(&io, (void __user *) arg, sizeof(io), ctx)) {
        return -EFAULT;
    }

    /* no locking of master_sem needed, because configuration will not be
     * deleted in the meantime. */

    if (!(sc = ec_master_get_config(master, io.config_index))) {
        return -ENOENT;
    }

    ret = ecrt_slave_config_emerg_pop(sc, msg);
    if (ret < 0) {
        return ret;
    }

    if (ec_copy_to_user((void __user *) io.target, msg, sizeof(msg), ctx)) {
        return -EFAULT;
    }

    return ret;
}

/****************************************************************************/

/** Clear the emergency ring.
 *
 * \return Zero on success, otherwise a negative error code.
 */
static ATTRIBUTES int ec_ioctl_sc_emerg_clear(
        ec_master_t *master, /**< EtherCAT master. */
        void *arg, /**< ioctl() argument. */
        ec_ioctl_context_t *ctx /**< Private data structure of file handle. */
        )
{
    ec_ioctl_sc_emerg_t io;
    ec_slave_config_t *sc;

    if (unlikely(!ctx->requested)) {
        return -EPERM;
    }

    if (ec_copy_from_user(&io, (void __user *) arg, sizeof(io), ctx)) {
        return -EFAULT;
    }

    /* no locking of master_sem needed, because configuration will not be
     * deleted in the meantime. */

    if (!(sc = ec_master_get_config(master, io.config_index))) {
        return -ENOENT;
    }

    return ecrt_slave_config_emerg_clear(sc);
}

/****************************************************************************/

/** Get the number of emergency overruns.
 *
 * \return Zero on success, otherwise a negative error code.
 */
static ATTRIBUTES int ec_ioctl_sc_emerg_overruns(
        ec_master_t *master, /**< EtherCAT master. */
        void *arg, /**< ioctl() argument. */
        ec_ioctl_context_t *ctx /**< Private data structure of file handle. */
        )
{
    ec_ioctl_sc_emerg_t io;
    ec_slave_config_t *sc;
    int ret;

    if (unlikely(!ctx->requested)) {
        return -EPERM;
    }

    if (ec_copy_from_user(&io, (void __user *) arg, sizeof(io), ctx)) {
        return -EFAULT;
    }

    /* no locking of master_sem needed, because configuration will not be
     * deleted in the meantime. */

    if (!(sc = ec_master_get_config(master, io.config_index))) {
        return -ENOENT;
    }

    ret = ecrt_slave_config_emerg_overruns(sc);
    if (ret < 0) {
        return ret;
    }

    io.overruns = ret;

    if (ec_copy_to_user((void __user *) arg, &io, sizeof(io), ctx)) {
        return -EFAULT;
    }

    return 0;
}

/****************************************************************************/

/** Create an SDO request.
 *
 * \return Zero on success, otherwise a negative error code.
 */
static ATTRIBUTES int ec_ioctl_sc_create_sdo_request(
        ec_master_t *master, /**< EtherCAT master. */
        void *arg, /**< ioctl() argument. */
        ec_ioctl_context_t *ctx /**< Private data structure of file handle. */
        )
{
    ec_ioctl_sdo_request_t data;
    ec_slave_config_t *sc;
    ec_sdo_request_t *req;

    if (unlikely(!ctx->requested))
        return -EPERM;

    if (copy_from_user(&data, (void __user *) arg, sizeof(data))) {
        return -EFAULT;
    }

    data.request_index = 0;

    if (down_interruptible(&master->master_sem))
        return -EINTR;

    sc = ec_master_get_config(master, data.config_index);
    if (!sc) {
        up(&master->master_sem);
        return -ENOENT;
    }

    list_for_each_entry(req, &sc->sdo_requests, list) {
        data.request_index++;
    }

    up(&master->master_sem); /** \todo sc could be invalidated */

    req = ecrt_slave_config_create_sdo_request_err(sc, data.sdo_index,
            data.sdo_subindex, data.size);
    if (IS_ERR(req))
        return PTR_ERR(req);

    if (copy_to_user((void __user *) arg, &data, sizeof(data)))
        return -EFAULT;

    return 0;
}

/****************************************************************************/

/** Create an SoE request.
 *
 * \return Zero on success, otherwise a negative error code.
 */
static ATTRIBUTES int ec_ioctl_sc_create_soe_request(
        ec_master_t *master, /**< EtherCAT master. */
        void *arg, /**< ioctl() argument. */
        ec_ioctl_context_t *ctx /**< Private data structure of file handle. */
        )
{
    ec_ioctl_soe_request_t data;
    ec_slave_config_t *sc;
    ec_soe_request_t *req;

    if (unlikely(!ctx->requested))
        return -EPERM;

    if (copy_from_user(&data, (void __user *) arg, sizeof(data))) {
        return -EFAULT;
    }

    data.request_index = 0;

    if (down_interruptible(&master->master_sem))
        return -EINTR;

    sc = ec_master_get_config(master, data.config_index);
    if (!sc) {
        up(&master->master_sem);
        return -ENOENT;
    }

    list_for_each_entry(req, &sc->soe_requests, list) {
        data.request_index++;
    }

    up(&master->master_sem); /** \todo sc could be invalidated */

    req = ecrt_slave_config_create_soe_request_err(sc, data.drive_no,
            data.idn, data.size);
    if (IS_ERR(req)) {
        return PTR_ERR(req);
    }

    if (copy_to_user((void __user *) arg, &data, sizeof(data))) {
        return -EFAULT;
    }

    return 0;
}

/****************************************************************************/

/** Create a register request.
 *
 * \return Zero on success, otherwise a negative error code.
 */
static ATTRIBUTES int ec_ioctl_sc_create_reg_request(
        ec_master_t *master, /**< EtherCAT master. */
        void *arg, /**< ioctl() argument. */
        ec_ioctl_context_t *ctx /**< Private data structure of file handle. */
        )
{
    ec_ioctl_reg_request_t io;
    ec_slave_config_t *sc;
    ec_reg_request_t *reg;

    if (unlikely(!ctx->requested)) {
        return -EPERM;
    }

    if (copy_from_user(&io, (void __user *) arg, sizeof(io))) {
        return -EFAULT;
    }

    io.request_index = 0;

    if (down_interruptible(&master->master_sem)) {
        return -EINTR;
    }

    sc = ec_master_get_config(master, io.config_index);
    if (!sc) {
        up(&master->master_sem);
        return -ENOENT;
    }

    list_for_each_entry(reg, &sc->reg_requests, list) {
        io.request_index++;
    }

    up(&master->master_sem); /** \todo sc could be invalidated */

    reg = ecrt_slave_config_create_reg_request_err(sc, io.mem_size);
    if (IS_ERR(reg)) {
        return PTR_ERR(reg);
    }

    if (copy_to_user((void __user *) arg, &io, sizeof(io))) {
        return -EFAULT;
    }

    return 0;
}

/****************************************************************************/

/** Create a VoE handler.
 *
 * \return Zero on success, otherwise a negative error code.
 */
static ATTRIBUTES int ec_ioctl_sc_create_voe_handler(
        ec_master_t *master, /**< EtherCAT master. */
        void *arg, /**< ioctl() argument. */
        ec_ioctl_context_t *ctx /**< Private data structure of file handle. */
        )
{
    ec_ioctl_voe_t data;
    ec_slave_config_t *sc;
    ec_voe_handler_t *voe;

    if (unlikely(!ctx->requested))
        return -EPERM;

    if (copy_from_user(&data, (void __user *) arg, sizeof(data))) {
        return -EFAULT;
    }

    data.voe_index = 0;

    if (down_interruptible(&master->master_sem))
        return -EINTR;

    sc = ec_master_get_config(master, data.config_index);
    if (!sc) {
        up(&master->master_sem);
        return -ENOENT;
    }

    list_for_each_entry(voe, &sc->voe_handlers, list) {
        data.voe_index++;
    }

    up(&master->master_sem); /** \todo sc could be invalidated */

    voe = ecrt_slave_config_create_voe_handler_err(sc, data.size);
    if (IS_ERR(voe))
        return PTR_ERR(voe);

    if (copy_to_user((void __user *) arg, &data, sizeof(data)))
        return -EFAULT;

    return 0;
}

/****************************************************************************/

/** Get the slave configuration's state.
 *
 * \return Zero on success, otherwise a negative error code.
 */
static ATTRIBUTES int ec_ioctl_sc_state(
        ec_master_t *master, /**< EtherCAT master. */
        void *arg, /**< ioctl() argument. */
        ec_ioctl_context_t *ctx /**< Private data structure of file handle. */
        )
{
    ec_ioctl_sc_state_t data;
    const ec_slave_config_t *sc;
    ec_slave_config_state_t state;
    int ret;

    if (unlikely(!ctx->requested))
        return -EPERM;

    if (ec_copy_from_user(&data, (void __user *) arg, sizeof(data), ctx)) {
        return -EFAULT;
    }

    /* no locking of master_sem needed, because sc will not be deleted in the
     * meantime. */

    if (!(sc = ec_master_get_config_const(master, data.config_index))) {
        return -ENOENT;
    }

    ret = ecrt_slave_config_state(sc, &state);
    if (ret)
        return ret;

    if (ec_copy_to_user((void __user *) data.state,
                        &state, sizeof(state), ctx))
        return -EFAULT;

    return 0;
}

/****************************************************************************/

/** Configures an IDN.
 *
 * \return Zero on success, otherwise a negative error code.
 */
static ATTRIBUTES int ec_ioctl_sc_idn(
        ec_master_t *master, /**< EtherCAT master. */
        void *arg, /**< ioctl() argument. */
        ec_ioctl_context_t *ctx /**< Private data structure of file handle. */
        )
{
    ec_ioctl_sc_idn_t ioctl;
    ec_slave_config_t *sc;
    uint8_t *data = NULL;
    int ret;

    if (unlikely(!ctx->requested))
        return -EPERM;

    if (copy_from_user(&ioctl, (void __user *) arg, sizeof(ioctl)))
        return -EFAULT;

    if (!ioctl.size)
        return -EINVAL;

    if (!(data = kmalloc(ioctl.size, GFP_KERNEL))) {
        return -ENOMEM;
    }

    if (copy_from_user(data, (void __user *) ioctl.data, ioctl.size)) {
        kfree(data);
        return -EFAULT;
    }

    if (down_interruptible(&master->master_sem)) {
        kfree(data);
        return -EINTR;
    }

    if (!(sc = ec_master_get_config(master, ioctl.config_index))) {
        up(&master->master_sem);
        kfree(data);
        return -ENOENT;
    }

    up(&master->master_sem); /** \todo sc could be invalidated */

    ret = ecrt_slave_config_idn(
            sc, ioctl.drive_no, ioctl.idn, ioctl.al_state, data, ioctl.size);
    kfree(data);
    return ret;
}

/****************************************************************************/

/** Configures a feature flag.
 *
 * \return Zero on success, otherwise a negative error code.
 */
static ATTRIBUTES int ec_ioctl_sc_flag(
        ec_master_t *master, /**< EtherCAT master. */
        void *arg, /**< ioctl() argument. */
        ec_ioctl_context_t *ctx /**< Private data structure of file handle. */
        )
{
    ec_ioctl_sc_flag_t ioctl;
    ec_slave_config_t *sc;
    uint8_t *key;
    int ret;

    if (unlikely(!ctx->requested)) {
        return -EPERM;
    }

    if (copy_from_user(&ioctl, (void __user *) arg, sizeof(ioctl))) {
        return -EFAULT;
    }

    if (!ioctl.key_size) {
        return -EINVAL;
    }

    if (!(key = kmalloc(ioctl.key_size + 1, GFP_KERNEL))) {
        return -ENOMEM;
    }

    if (copy_from_user(key, (void __user *) ioctl.key, ioctl.key_size)) {
        kfree(key);
        return -EFAULT;
    }
    key[ioctl.key_size] = '\0';

    if (down_interruptible(&master->master_sem)) {
        kfree(key);
        return -EINTR;
    }

    if (!(sc = ec_master_get_config(master, ioctl.config_index))) {
        up(&master->master_sem);
        kfree(key);
        return -ENOENT;
    }

    up(&master->master_sem); /** \todo sc could be invalidated */

    ret = ecrt_slave_config_flag(sc, key, ioctl.value);
    kfree(key);
    return ret;
}

/****************************************************************************/

/** Sets an AL state transition timeout.
 *
 * \return Zero on success, otherwise a negative error code.
 */
static ATTRIBUTES int ec_ioctl_sc_state_timeout(
        ec_master_t *master, /**< EtherCAT master. */
        void *arg, /**< ioctl() argument. */
        ec_ioctl_context_t *ctx /**< Private data structure of file handle. */
        )
{
    ec_ioctl_sc_state_timeout_t ioctl;
    ec_slave_config_t *sc;
    int ret;

    if (unlikely(!ctx->requested)) {
        return -EPERM;
    }

    if (copy_from_user(&ioctl, (void __user *) arg, sizeof(ioctl))) {
        return -EFAULT;
    }

    if (down_interruptible(&master->master_sem)) {
        return -EINTR;
    }

    if (!(sc = ec_master_get_config(master, ioctl.config_index))) {
        up(&master->master_sem);
        return -ENOENT;
    }

    up(&master->master_sem); /** \todo sc could be invalidated */

    ret = ecrt_slave_config_state_timeout(sc, ioctl.from_state,
            ioctl.to_state, ioctl.timeout_ms);
    return ret;
}

/****************************************************************************/

#ifdef EC_EOE

/** Configures EoE IP parameters.
 *
 * \return Zero on success, otherwise a negative error code.
 */
static ATTRIBUTES int ec_ioctl_sc_ip(
        ec_master_t *master, /**< EtherCAT master. */
        void *arg, /**< ioctl() argument. */
        ec_ioctl_context_t *ctx /**< Private data structure of file handle. */
        )
{
    ec_ioctl_eoe_ip_t io;
    ec_slave_config_t *sc;

    if (unlikely(!ctx->requested)) {
        return -EPERM;
    }

    if (copy_from_user(&io, (void __user *) arg, sizeof(io))) {
        return -EFAULT;
    }

    if (down_interruptible(&master->master_sem)) {
        return -EINTR;
    }

    if (!(sc = ec_master_get_config(master, io.config_index))) {
        up(&master->master_sem);
        return -ENOENT;
    }

    up(&master->master_sem); /** \todo sc could be invalidated */

    /* the kernel versions of the EoE set IP methods never fail. */
    if (io.mac_address_included) {
        ecrt_slave_config_eoe_mac_address(sc, io.mac_address);
    }
    if (io.ip_address_included) {
        ecrt_slave_config_eoe_ip_address(sc, io.ip_address);
    }
    if (io.subnet_mask_included) {
        ecrt_slave_config_eoe_subnet_mask(sc, io.subnet_mask);
    }
    if (io.gateway_included) {
        ecrt_slave_config_eoe_default_gateway(sc, io.gateway);
    }
    if (io.dns_included) {
        ecrt_slave_config_eoe_dns_address(sc, io.dns);
    }
    if (io.name_included) {
        ecrt_slave_config_eoe_hostname(sc, io.name);
    }

    return 0;
}

#endif

/****************************************************************************/

/** Gets the domain's data size.
 *
 * \return Domain size, or a negative error code.
 */
static ATTRIBUTES int ec_ioctl_domain_size(
        ec_master_t *master, /**< EtherCAT master. */
        void *arg, /**< ioctl() argument. */
        ec_ioctl_context_t *ctx /**< Private data structure of file handle. */
        )
{
    const ec_domain_t *domain;

    if (unlikely(!ctx->requested)) {
        return -EPERM;
    }

    if (down_interruptible(&master->master_sem)) {
        return -EINTR;
    }

    list_for_each_entry(domain, &master->domains, list) {
        if (domain->index == (unsigned long) arg) {
            size_t size = ecrt_domain_size(domain);
            up(&master->master_sem);
            return size;
        }
    }

    up(&master->master_sem);
    return -ENOENT;
}

/****************************************************************************/

/** Gets the domain's offset in the total process data.
 *
 * \return Domain offset, or a negative error code.
 */
static ATTRIBUTES int ec_ioctl_domain_offset(
        ec_master_t *master, /**< EtherCAT master. */
        void *arg, /**< ioctl() argument. */
        ec_ioctl_context_t *ctx /**< Private data structure of file handle. */
        )
{
    int offset = 0;
    const ec_domain_t *domain;

    if (unlikely(!ctx->requested))
        return -EPERM;

    if (down_interruptible(&master->master_sem)) {
        return -EINTR;
    }

    list_for_each_entry(domain, &master->domains, list) {
        if (domain->index == (unsigned long) arg) {
            up(&master->master_sem);
            return offset;
        }
        offset += ecrt_domain_size(domain);
    }

    up(&master->master_sem);
    return -ENOENT;
}

/****************************************************************************/

/** Process the domain.
 *
 * \return Zero on success, otherwise a negative error code.
 */
static ATTRIBUTES int ec_ioctl_domain_process(
        ec_master_t *master, /**< EtherCAT master. */
        void *arg, /**< ioctl() argument. */
        ec_ioctl_context_t *ctx /**< Private data structure of file handle. */
        )
{
    ec_domain_t *domain;

    if (unlikely(!ctx->requested))
        return -EPERM;

    /* no locking of master_sem needed, because domain will not be deleted in
     * the meantime. */

    if (!(domain = ec_master_find_domain(master, (unsigned long) arg))) {
        return -ENOENT;
    }

    return ecrt_domain_process(domain);
}

/****************************************************************************/

/** Queue the domain.
 *
 * \return Zero on success, otherwise a negative error code.
 */
static ATTRIBUTES int ec_ioctl_domain_queue(
        ec_master_t *master, /**< EtherCAT master. */
        void *arg, /**< ioctl() argument. */
        ec_ioctl_context_t *ctx /**< Private data structure of file handle. */
        )
{
    ec_domain_t *domain;
    int ret;

    if (unlikely(!ctx->requested))
        return -EPERM;

    /* no locking of master_sem needed, because domain will not be deleted in
     * the meantime. */

    if (!(domain = ec_master_find_domain(master, (unsigned long) arg))) {
        return -ENOENT;
    }

    if (ec_ioctl_lock_interruptible(&master->io_mutex))
        return -EINTR;

    ret = ecrt_domain_queue(domain);
    ec_ioctl_unlock(&master->io_mutex);
    return ret;
}

/****************************************************************************/

/** Get the domain state.
 *
 * \return Zero on success, otherwise a negative error code.
 */
static ATTRIBUTES int ec_ioctl_domain_state(
        ec_master_t *master, /**< EtherCAT master. */
        void *arg, /**< ioctl() argument. */
        ec_ioctl_context_t *ctx /**< Private data structure of file handle. */
        )
{
    ec_ioctl_domain_state_t data;
    const ec_domain_t *domain;
    ec_domain_state_t state;
    int ret;

    if (unlikely(!ctx->requested))
        return -EPERM;

    if (ec_copy_from_user(&data, (void __user *) arg, sizeof(data), ctx)) {
        return -EFAULT;
    }

    /* no locking of master_sem needed, because domain will not be deleted in
     * the meantime. */

    if (!(domain = ec_master_find_domain_const(master, data.domain_index))) {
        return -ENOENT;
    }

    ret = ecrt_domain_state(domain, &state);
    if (ret)
        return ret;

    if (ec_copy_to_user((void __user *) data.state, &state, sizeof(state),
                ctx))
        return -EFAULT;

    return 0;
}

/****************************************************************************/

/** Sets an SDO request's SDO index and subindex.
 *
 * \return Zero on success, otherwise a negative error code.
 */
static ATTRIBUTES int ec_ioctl_sdo_request_index(
        ec_master_t *master, /**< EtherCAT master. */
        void *arg, /**< ioctl() argument. */
        ec_ioctl_context_t *ctx /**< Private data structure of file handle. */
        )
{
    ec_ioctl_sdo_request_t data;
    ec_slave_config_t *sc;
    ec_sdo_request_t *req;

    if (unlikely(!ctx->requested))
        return -EPERM;

    if (ec_copy_from_user(&data, (void __user *) arg, sizeof(data), ctx))
        return -EFAULT;

    /* no locking of master_sem needed, because neither sc nor req will not be
     * deleted in the meantime. */

    if (!(sc = ec_master_get_config(master, data.config_index))) {
        return -ENOENT;
    }

    if (!(req = ec_slave_config_find_sdo_request(sc, data.request_index))) {
        return -ENOENT;
    }

    return ecrt_sdo_request_index(req, data.sdo_index, data.sdo_subindex);
}

/****************************************************************************/

/** Sets an SDO request's timeout.
 *
 * \return Zero on success, otherwise a negative error code.
 */
static ATTRIBUTES int ec_ioctl_sdo_request_timeout(
        ec_master_t *master, /**< EtherCAT master. */
        void *arg, /**< ioctl() argument. */
        ec_ioctl_context_t *ctx /**< Private data structure of file handle. */
        )
{
    ec_ioctl_sdo_request_t data;
    ec_slave_config_t *sc;
    ec_sdo_request_t *req;

    if (unlikely(!ctx->requested))
        return -EPERM;

    if (ec_copy_from_user(&data, (void __user *) arg, sizeof(data), ctx))
        return -EFAULT;

    /* no locking of master_sem needed, because neither sc nor req will not be
     * deleted in the meantime. */

    if (!(sc = ec_master_get_config(master, data.config_index))) {
        return -ENOENT;
    }

    if (!(req = ec_slave_config_find_sdo_request(sc, data.request_index))) {
        return -ENOENT;
    }

    return ecrt_sdo_request_timeout(req, data.timeout);
}

/****************************************************************************/

/** Gets an SDO request's state.
 *
 * Also pre-fetches the size of incoming data.
 *
 * \return Zero on success, otherwise a negative error code.
 */
static ATTRIBUTES int ec_ioctl_sdo_request_state(
        ec_master_t *master, /**< EtherCAT master. */
        void *arg, /**< ioctl() argument. */
        ec_ioctl_context_t *ctx /**< Private data structure of file handle. */
        )
{
    ec_ioctl_sdo_request_t data;
    ec_slave_config_t *sc;
    ec_sdo_request_t *req;

    if (unlikely(!ctx->requested))
        return -EPERM;

    if (ec_copy_from_user(&data, (void __user *) arg, sizeof(data), ctx))
        return -EFAULT;

    /* no locking of master_sem needed, because neither sc nor req will not be
     * deleted in the meantime. */

    if (!(sc = ec_master_get_config(master, data.config_index))) {
        return -ENOENT;
    }

    if (!(req = ec_slave_config_find_sdo_request(sc, data.request_index))) {
        return -ENOENT;
    }

    data.state = ecrt_sdo_request_state(req);
    if (data.state == EC_REQUEST_SUCCESS && req->dir == EC_DIR_INPUT)
        data.size = ecrt_sdo_request_data_size(req);
    else
        data.size = 0;

    if (ec_copy_to_user((void __user *) arg, &data, sizeof(data), ctx))
        return -EFAULT;

    return 0;
}

/****************************************************************************/

/** Starts an SDO read operation.
 *
 * \return Zero on success, otherwise a negative error code.
 */
static ATTRIBUTES int ec_ioctl_sdo_request_read(
        ec_master_t *master, /**< EtherCAT master. */
        void *arg, /**< ioctl() argument. */
        ec_ioctl_context_t *ctx /**< Private data structure of file handle. */
        )
{
    ec_ioctl_sdo_request_t data;
    ec_slave_config_t *sc;
    ec_sdo_request_t *req;

    if (unlikely(!ctx->requested))
        return -EPERM;

    if (ec_copy_from_user(&data, (void __user *) arg, sizeof(data), ctx))
        return -EFAULT;

    /* no locking of master_sem needed, because neither sc nor req will not be
     * deleted in the meantime. */

    if (!(sc = ec_master_get_config(master, data.config_index))) {
        return -ENOENT;
    }

    if (!(req = ec_slave_config_find_sdo_request(sc, data.request_index))) {
        return -ENOENT;
    }

    return ecrt_sdo_request_read(req);
}

/****************************************************************************/

/** Starts an SDO write operation.
 *
 * \return Zero on success, otherwise a negative error code.
 */
static ATTRIBUTES int ec_ioctl_sdo_request_write(
        ec_master_t *master, /**< EtherCAT master. */
        void *arg, /**< ioctl() argument. */
        ec_ioctl_context_t *ctx /**< Private data structure of file handle. */
        )
{
    ec_ioctl_sdo_request_t data;
    ec_slave_config_t *sc;
    ec_sdo_request_t *req;

    if (unlikely(!ctx->requested))
        return -EPERM;

    if (ec_copy_from_user(&data, (void __user *) arg, sizeof(data), ctx))
        return -EFAULT;

    if (!data.size) {
        EC_MASTER_ERR(master, "SDO download: Data size may not be zero!\n");
        return -EINVAL;
    }

    /* no locking of master_sem needed, because neither sc nor req will not be
     * deleted in the meantime. */

    if (!(sc = ec_master_get_config(master, data.config_index))) {
        return -ENOENT;
    }

    if (!(req = ec_slave_config_find_sdo_request(sc, data.request_index))) {
        return -ENOENT;
    }

    if (data.size > req->mem_size)
        return -ENOBUFS;

    if (ec_copy_from_user(req->data, (void __user *) data.data,
                          data.size, ctx))
        return -EFAULT;

    req->data_size = data.size;
    return ecrt_sdo_request_write(req);
}

/****************************************************************************/

/** Read SDO data.
 *
 * \return Zero on success, otherwise a negative error code.
 */
static ATTRIBUTES int ec_ioctl_sdo_request_data(
        ec_master_t *master, /**< EtherCAT master. */
        void *arg, /**< ioctl() argument. */
        ec_ioctl_context_t *ctx /**< Private data structure of file handle. */
        )
{
    ec_ioctl_sdo_request_t data;
    ec_slave_config_t *sc;
    ec_sdo_request_t *req;

    if (unlikely(!ctx->requested))
        return -EPERM;

    if (ec_copy_from_user(&data, (void __user *) arg, sizeof(data), ctx))
        return -EFAULT;

    /* no locking of master_sem needed, because neither sc nor req will not be
     * deleted in the meantime. */

    if (!(sc = ec_master_get_config(master, data.config_index))) {
        return -ENOENT;
    }

    if (!(req = ec_slave_config_find_sdo_request(sc, data.request_index))) {
        return -ENOENT;
    }

    if (ec_copy_to_user((void __user *) data.data, ecrt_sdo_request_data(req),
                ecrt_sdo_request_data_size(req), ctx))
        return -EFAULT;

    return 0;
}

/****************************************************************************/

/** Sets an SoE request's drive number and IDN.
 *
 * \return Zero on success, otherwise a negative error code.
 */
static ATTRIBUTES int ec_ioctl_soe_request_index(
        ec_master_t *master, /**< EtherCAT master. */
        void *arg, /**< ioctl() argument. */
        ec_ioctl_context_t *ctx /**< Private data structure of file handle. */
        )
{
    ec_ioctl_soe_request_t data;
    ec_slave_config_t *sc;
    ec_soe_request_t *req;

    if (unlikely(!ctx->requested))
        return -EPERM;

    if (ec_copy_from_user(&data, (void __user *) arg, sizeof(data), ctx))
        return -EFAULT;

    /* no locking of master_sem needed, because neither sc nor req will not be
     * deleted in the meantime. */

    if (!(sc = ec_master_get_config(master, data.config_index))) {
        return -ENOENT;
    }

    if (!(req = ec_slave_config_find_soe_request(sc, data.request_index))) {
        return -ENOENT;
    }

    return ecrt_soe_request_idn(req, data.drive_no, data.idn);
}

/****************************************************************************/

/** Sets an CoE request's timeout.
 *
 * \return Zero on success, otherwise a negative error code.
 */
static ATTRIBUTES int ec_ioctl_soe_request_timeout(
        ec_master_t *master, /**< EtherCAT master. */
        void *arg, /**< ioctl() argument. */
        ec_ioctl_context_t *ctx /**< Private data structure of file handle. */
        )
{
    ec_ioctl_soe_request_t data;
    ec_slave_config_t *sc;
    ec_soe_request_t *req;

    if (unlikely(!ctx->requested))
        return -EPERM;

    if (ec_copy_from_user(&data, (void __user *) arg, sizeof(data), ctx))
        return -EFAULT;

    /* no locking of master_sem needed, because neither sc nor req will not be
     * deleted in the meantime. */

    if (!(sc = ec_master_get_config(master, data.config_index))) {
        return -ENOENT;
    }

    if (!(req = ec_slave_config_find_soe_request(sc, data.request_index))) {
        return -ENOENT;
    }

    return ecrt_soe_request_timeout(req, data.timeout);
}

/****************************************************************************/

/** Gets an SoE request's state.
 *
 * \return Zero on success, otherwise a negative error code.
 */
static ATTRIBUTES int ec_ioctl_soe_request_state(
        ec_master_t *master, /**< EtherCAT master. */
        void *arg, /**< ioctl() argument. */
        ec_ioctl_context_t *ctx /**< Private data structure of file handle. */
        )
{
    ec_ioctl_soe_request_t data;
    ec_slave_config_t *sc;
    ec_soe_request_t *req;

    if (unlikely(!ctx->requested))
        return -EPERM;

    if (ec_copy_from_user(&data, (void __user *) arg, sizeof(data), ctx))
        return -EFAULT;

    /* no locking of master_sem needed, because neither sc nor req will not be
     * deleted in the meantime. */

    if (!(sc = ec_master_get_config(master, data.config_index))) {
        return -ENOENT;
    }

    if (!(req = ec_slave_config_find_soe_request(sc, data.request_index))) {
        return -ENOENT;
    }

    data.state = ecrt_soe_request_state(req);
    if (data.state == EC_REQUEST_SUCCESS && req->dir == EC_DIR_INPUT) {
        data.size = ecrt_soe_request_data_size(req);
    }
    else {
        data.size = 0;
    }

    if (ec_copy_to_user((void __user *) arg, &data, sizeof(data), ctx))
        return -EFAULT;

    return 0;
}

/****************************************************************************/

/** Starts an SoE IDN read operation.
 *
 * \return Zero on success, otherwise a negative error code.
 */
static ATTRIBUTES int ec_ioctl_soe_request_read(
        ec_master_t *master, /**< EtherCAT master. */
        void *arg, /**< ioctl() argument. */
        ec_ioctl_context_t *ctx /**< Private data structure of file handle. */
        )
{
    ec_ioctl_soe_request_t data;
    ec_slave_config_t *sc;
    ec_soe_request_t *req;

    if (unlikely(!ctx->requested))
        return -EPERM;

    if (ec_copy_from_user(&data, (void __user *) arg, sizeof(data), ctx))
        return -EFAULT;

    /* no locking of master_sem needed, because neither sc nor req will not be
     * deleted in the meantime. */

    if (!(sc = ec_master_get_config(master, data.config_index))) {
        return -ENOENT;
    }

    if (!(req = ec_slave_config_find_soe_request(sc, data.request_index))) {
        return -ENOENT;
    }

    return ecrt_soe_request_read(req);
}

/****************************************************************************/

/** Starts an SoE IDN write operation.
 *
 * \return Zero on success, otherwise a negative error code.
 */
static ATTRIBUTES int ec_ioctl_soe_request_write(
        ec_master_t *master, /**< EtherCAT master. */
        void *arg, /**< ioctl() argument. */
        ec_ioctl_context_t *ctx /**< Private data structure of file handle. */
        )
{
    ec_ioctl_soe_request_t data;
    ec_slave_config_t *sc;
    ec_soe_request_t *req;

    if (unlikely(!ctx->requested))
        return -EPERM;

    if (ec_copy_from_user(&data, (void __user *) arg, sizeof(data), ctx))
        return -EFAULT;

    if (!data.size) {
        EC_MASTER_ERR(master, "IDN write: Data size may not be zero!\n");
        return -EINVAL;
    }

    /* no locking of master_sem needed, because neither sc nor req will not be
     * deleted in the meantime. */

    if (!(sc = ec_master_get_config(master, data.config_index))) {
        return -ENOENT;
    }

    if (!(req = ec_slave_config_find_soe_request(sc, data.request_index))) {
        return -ENOENT;
    }

    if (data.size > req->mem_size)
        return -ENOBUFS;

    if (ec_copy_from_user(req->data, (void __user *) data.data,
                          data.size, ctx))
        return -EFAULT;

    req->data_size = data.size;
    return ecrt_soe_request_write(req);
}

/****************************************************************************/

/** Read SoE IDN data.
 *
 * \return Zero on success, otherwise a negative error code.
 */
static ATTRIBUTES int ec_ioctl_soe_request_data(
        ec_master_t *master, /**< EtherCAT master. */
        void *arg, /**< ioctl() argument. */
        ec_ioctl_context_t *ctx /**< Private data structure of file handle. */
        )
{
    ec_ioctl_soe_request_t data;
    ec_slave_config_t *sc;
    ec_soe_request_t *req;

    if (unlikely(!ctx->requested))
        return -EPERM;

    if (ec_copy_from_user(&data, (void __user *) arg, sizeof(data), ctx))
        return -EFAULT;

    /* no locking of master_sem needed, because neither sc nor req will not be
     * deleted in the meantime. */

    if (!(sc = ec_master_get_config(master, data.config_index))) {
        return -ENOENT;
    }

    if (!(req = ec_slave_config_find_soe_request(sc, data.request_index))) {
        return -ENOENT;
    }

    if (ec_copy_to_user((void __user *) data.data, ecrt_soe_request_data(req),
                ecrt_soe_request_data_size(req), ctx))
        return -EFAULT;

    return 0;
}

/****************************************************************************/

/** Read register data.
 *
 * \return Zero on success, otherwise a negative error code.
 */
static ATTRIBUTES int ec_ioctl_reg_request_data(
        ec_master_t *master, /**< EtherCAT master. */
        void *arg, /**< ioctl() argument. */
        ec_ioctl_context_t *ctx /**< Private data structure of file handle. */
        )
{
    ec_ioctl_reg_request_t io;
    ec_slave_config_t *sc;
    ec_reg_request_t *reg;

    if (unlikely(!ctx->requested)) {
        return -EPERM;
    }

    if (ec_copy_from_user(&io, (void __user *) arg, sizeof(io), ctx)) {
        return -EFAULT;
    }

    if (io.mem_size <= 0) {
        return 0;
    }

    /* no locking of master_sem needed, because neither sc nor reg will not be
     * deleted in the meantime. */

    if (!(sc = ec_master_get_config(master, io.config_index))) {
        return -ENOENT;
    }

    if (!(reg = ec_slave_config_find_reg_request(sc, io.request_index))) {
        return -ENOENT;
    }

    if (ec_copy_to_user((void __user *) io.data, ecrt_reg_request_data(reg),
                min(reg->mem_size, io.mem_size), ctx)) {
        return -EFAULT;
    }

    return 0;
}

/****************************************************************************/

/** Gets an register request's state.
 *
 * \return Zero on success, otherwise a negative error code.
 */
static ATTRIBUTES int ec_ioctl_reg_request_state(
        ec_master_t *master, /**< EtherCAT master. */
        void *arg, /**< ioctl() argument. */
        ec_ioctl_context_t *ctx /**< Private data structure of file handle. */
        )
{
    ec_ioctl_reg_request_t io;
    ec_slave_config_t *sc;
    ec_reg_request_t *reg;

    if (unlikely(!ctx->requested)) {
        return -EPERM;
    }

    if (ec_copy_from_user(&io, (void __user *) arg, sizeof(io), ctx)) {
        return -EFAULT;
    }

    /* no locking of master_sem needed, because neither sc nor reg will not be
     * deleted in the meantime. */

    if (!(sc = ec_master_get_config(master, io.config_index))) {
        return -ENOENT;
    }

    if (!(reg = ec_slave_config_find_reg_request(sc, io.request_index))) {
        return -ENOENT;
    }

    io.state = ecrt_reg_request_state(reg);
    io.new_data = io.state == EC_REQUEST_SUCCESS && reg->dir == EC_DIR_INPUT;

    if (ec_copy_to_user((void __user *) arg, &io, sizeof(io), ctx)) {
        return -EFAULT;
    }

    return 0;
}

/****************************************************************************/

/** Starts an register write operation.
 *
 * \return Zero on success, otherwise a negative error code.
 */
static ATTRIBUTES int ec_ioctl_reg_request_write(
        ec_master_t *master, /**< EtherCAT master. */
        void *arg, /**< ioctl() argument. */
        ec_ioctl_context_t *ctx /**< Private data structure of file handle. */
        )
{
    ec_ioctl_reg_request_t io;
    ec_slave_config_t *sc;
    ec_reg_request_t *reg;

    if (unlikely(!ctx->requested)) {
        return -EPERM;
    }

    if (ec_copy_from_user(&io, (void __user *) arg, sizeof(io), ctx)) {
        return -EFAULT;
    }

    /* no locking of master_sem needed, because neither sc nor reg will not be
     * deleted in the meantime. */

    if (!(sc = ec_master_get_config(master, io.config_index))) {
        return -ENOENT;
    }

    if (!(reg = ec_slave_config_find_reg_request(sc, io.request_index))) {
        return -ENOENT;
    }

    if (io.transfer_size > reg->mem_size) {
        return -ENOBUFS;
    }

    if (ec_copy_from_user(reg->data, (void __user *) io.data,
                io.transfer_size, ctx)) {
        return -EFAULT;
    }

    return ecrt_reg_request_write(reg, io.address, io.transfer_size);
}

/****************************************************************************/

/** Starts an register read operation.
 *
 * \return Zero on success, otherwise a negative error code.
 */
static ATTRIBUTES int ec_ioctl_reg_request_read(
        ec_master_t *master, /**< EtherCAT master. */
        void *arg, /**< ioctl() argument. */
        ec_ioctl_context_t *ctx /**< Private data structure of file handle. */
        )
{
    ec_ioctl_reg_request_t io;
    ec_slave_config_t *sc;
    ec_reg_request_t *reg;

    if (unlikely(!ctx->requested)) {
        return -EPERM;
    }

    if (ec_copy_from_user(&io, (void __user *) arg, sizeof(io), ctx)) {
        return -EFAULT;
    }

    /* no locking of master_sem needed, because neither sc nor reg will not be
     * deleted in the meantime. */

    if (!(sc = ec_master_get_config(master, io.config_index))) {
        return -ENOENT;
    }

    if (!(reg = ec_slave_config_find_reg_request(sc, io.request_index))) {
        return -ENOENT;
    }

    if (io.transfer_size > reg->mem_size) {
        return -ENOBUFS;
    }

    return ecrt_reg_request_read(reg, io.address, io.transfer_size);
}

/****************************************************************************/

/** Sets the VoE send header.
 *
 * \return Zero on success, otherwise a negative error code.
 */
static ATTRIBUTES int ec_ioctl_voe_send_header(
        ec_master_t *master, /**< EtherCAT master. */
        void *arg, /**< ioctl() argument. */
        ec_ioctl_context_t *ctx /**< Private data structure of file handle. */
        )
{
    ec_ioctl_voe_t data;
    ec_slave_config_t *sc;
    ec_voe_handler_t *voe;
    uint32_t vendor_id;
    uint16_t vendor_type;

    if (unlikely(!ctx->requested))
        return -EPERM;

    if (ec_copy_from_user(&data, (void __user *) arg, sizeof(data), ctx))
        return -EFAULT;

    if (ec_copy_from_user(&vendor_id, data.vendor_id, sizeof(vendor_id), ctx))
        return -EFAULT;

    if (ec_copy_from_user(&vendor_type, data.vendor_type,
                          sizeof(vendor_type), ctx))
        return -EFAULT;

    /* no locking of master_sem needed, because neither sc nor voe will not be
     * deleted in the meantime. */

    if (!(sc = ec_master_get_config(master, data.config_index))) {
        return -ENOENT;
    }

    if (!(voe = ec_slave_config_find_voe_handler(sc, data.voe_index))) {
        return -ENOENT;
    }

    return ecrt_voe_handler_send_header(voe, vendor_id, vendor_type);
}

/****************************************************************************/

/** Gets the received VoE header.
 *
 * \return Zero on success, otherwise a negative error code.
 */
static ATTRIBUTES int ec_ioctl_voe_rec_header(
        ec_master_t *master, /**< EtherCAT master. */
        void *arg, /**< ioctl() argument. */
        ec_ioctl_context_t *ctx /**< Private data structure of file handle. */
        )
{
    ec_ioctl_voe_t data;
    ec_slave_config_t *sc;
    ec_voe_handler_t *voe;
    uint32_t vendor_id;
    uint16_t vendor_type;
    int ret;

    if (unlikely(!ctx->requested))
        return -EPERM;

    if (ec_copy_from_user(&data, (void __user *) arg, sizeof(data), ctx))
        return -EFAULT;

    /* no locking of master_sem needed, because neither sc nor voe will not be
     * deleted in the meantime. */

    if (!(sc = ec_master_get_config(master, data.config_index))) {
        return -ENOENT;
    }

    if (!(voe = ec_slave_config_find_voe_handler(sc, data.voe_index))) {
        return -ENOENT;
    }

    ret = ecrt_voe_handler_received_header(voe, &vendor_id, &vendor_type);
    if (ret)
        return ret;

    if (likely(data.vendor_id))
        if (ec_copy_to_user(data.vendor_id, &vendor_id,
                            sizeof(vendor_id), ctx))
            return -EFAULT;

    if (likely(data.vendor_type))
        if (ec_copy_to_user(data.vendor_type, &vendor_type,
            sizeof(vendor_type), ctx))
            return -EFAULT;

    return 0;
}

/****************************************************************************/

/** Starts a VoE read operation.
 *
 * \return Zero on success, otherwise a negative error code.
 */
static ATTRIBUTES int ec_ioctl_voe_read(
        ec_master_t *master, /**< EtherCAT master. */
        void *arg, /**< ioctl() argument. */
        ec_ioctl_context_t *ctx /**< Private data structure of file handle. */
        )
{
    ec_ioctl_voe_t data;
    ec_slave_config_t *sc;
    ec_voe_handler_t *voe;

    if (unlikely(!ctx->requested))
        return -EPERM;

    if (ec_copy_from_user(&data, (void __user *) arg, sizeof(data), ctx))
        return -EFAULT;

    /* no locking of master_sem needed, because neither sc nor voe will not be
     * deleted in the meantime. */

    if (!(sc = ec_master_get_config(master, data.config_index))) {
        return -ENOENT;
    }

    if (!(voe = ec_slave_config_find_voe_handler(sc, data.voe_index))) {
        return -ENOENT;
    }

    return ecrt_voe_handler_read(voe);
}

/****************************************************************************/

/** Starts a VoE read operation without sending a sync message first.
 *
 * \return Zero on success, otherwise a negative error code.
 */
static ATTRIBUTES int ec_ioctl_voe_read_nosync(
        ec_master_t *master, /**< EtherCAT master. */
        void *arg, /**< ioctl() argument. */
        ec_ioctl_context_t *ctx /**< Private data structure of file handle. */
        )
{
    ec_ioctl_voe_t data;
    ec_slave_config_t *sc;
    ec_voe_handler_t *voe;

    if (unlikely(!ctx->requested))
        return -EPERM;

    if (ec_copy_from_user(&data, (void __user *) arg, sizeof(data), ctx))
        return -EFAULT;

    /* no locking of master_sem needed, because neither sc nor voe will not be
     * deleted in the meantime. */

    if (!(sc = ec_master_get_config(master, data.config_index))) {
        return -ENOENT;
    }

    if (!(voe = ec_slave_config_find_voe_handler(sc, data.voe_index))) {
        return -ENOENT;
    }

    return ecrt_voe_handler_read_nosync(voe);
}

/****************************************************************************/

/** Starts a VoE write operation.
 *
 * \return Zero on success, otherwise a negative error code.
 */
static ATTRIBUTES int ec_ioctl_voe_write(
        ec_master_t *master, /**< EtherCAT master. */
        void *arg, /**< ioctl() argument. */
        ec_ioctl_context_t *ctx /**< Private data structure of file handle. */
        )
{
    ec_ioctl_voe_t data;
    ec_slave_config_t *sc;
    ec_voe_handler_t *voe;

    if (unlikely(!ctx->requested))
        return -EPERM;

    if (ec_copy_from_user(&data, (void __user *) arg, sizeof(data), ctx))
        return -EFAULT;

    /* no locking of master_sem needed, because neither sc nor voe will not be
     * deleted in the meantime. */

    if (!(sc = ec_master_get_config(master, data.config_index))) {
        return -ENOENT;
    }

    if (!(voe = ec_slave_config_find_voe_handler(sc, data.voe_index))) {
        return -ENOENT;
    }

    if (data.size) {
        if (data.size > ec_voe_handler_mem_size(voe))
            return -ENOBUFS;

        if (ec_copy_from_user(ecrt_voe_handler_data(voe),
                    (void __user *) data.data, data.size, ctx))
            return -EFAULT;
    }

    return ecrt_voe_handler_write(voe, data.size);
}

/****************************************************************************/

/** Executes the VoE state machine.
 *
 * \return Zero on success, otherwise a negative error code.
 */
static ATTRIBUTES int ec_ioctl_voe_exec(
        ec_master_t *master, /**< EtherCAT master. */
        void *arg, /**< ioctl() argument. */
        ec_ioctl_context_t *ctx /**< Private data structure of file handle. */
        )
{
    ec_ioctl_voe_t data;
    ec_slave_config_t *sc;
    ec_voe_handler_t *voe;

    if (unlikely(!ctx->requested))
        return -EPERM;

    if (ec_copy_from_user(&data, (void __user *) arg, sizeof(data), ctx))
        return -EFAULT;

    /* no locking of master_sem needed, because neither sc nor voe will not be
     * deleted in the meantime. */

    if (!(sc = ec_master_get_config(master, data.config_index))) {
        return -ENOENT;
    }

    if (!(voe = ec_slave_config_find_voe_handler(sc, data.voe_index))) {
        return -ENOENT;
    }

    if (ec_ioctl_lock_interruptible(&master->io_mutex))
        return -EINTR;

    data.state = ecrt_voe_handler_execute(voe);
    ec_ioctl_unlock(&master->io_mutex);
    if (data.state == EC_REQUEST_SUCCESS && voe->dir == EC_DIR_INPUT)
        data.size = ecrt_voe_handler_data_size(voe);
    else
        data.size = 0;

    if (ec_copy_to_user((void __user *) arg, &data, sizeof(data), ctx))
        return -EFAULT;

    return 0;
}

/****************************************************************************/

/** Reads the received VoE data.
 *
 * \return Zero on success, otherwise a negative error code.
 */
static ATTRIBUTES int ec_ioctl_voe_data(
        ec_master_t *master, /**< EtherCAT master. */
        void *arg, /**< ioctl() argument. */
        ec_ioctl_context_t *ctx /**< Private data structure of file handle. */
        )
{
    ec_ioctl_voe_t data;
    ec_slave_config_t *sc;
    ec_voe_handler_t *voe;

    if (unlikely(!ctx->requested))
        return -EPERM;

    if (ec_copy_from_user(&data, (void __user *) arg, sizeof(data), ctx))
        return -EFAULT;

    /* no locking of master_sem needed, because neither sc nor voe will not be
     * deleted in the meantime. */

    if (!(sc = ec_master_get_config(master, data.config_index))) {
        return -ENOENT;
    }

    if (!(voe = ec_slave_config_find_voe_handler(sc, data.voe_index))) {
        return -ENOENT;
    }

    if (ec_copy_to_user((void __user *) data.data, ecrt_voe_handler_data(voe),
                ecrt_voe_handler_data_size(voe), ctx))
        return -EFAULT;

    return 0;
}

/****************************************************************************/

/** Read a file from a slave via FoE.
 *
 * \return Zero on success, otherwise a negative error code.
 */
static ATTRIBUTES int ec_ioctl_slave_foe_read(
        ec_master_t *master, /**< EtherCAT master. */
        void *arg /**< ioctl() argument. */
        )
{
    ec_ioctl_slave_foe_t io;
    ec_foe_request_t request;
    ec_slave_t *slave;
    int ret;

    if (copy_from_user(&io, (void __user *) arg, sizeof(io))) {
        return -EFAULT;
    }

    ec_foe_request_init(&request, io.file_name);
    ret = ec_foe_request_alloc(&request, 10000); // FIXME
    if (ret) {
        ec_foe_request_clear(&request);
        return ret;
    }

    ec_foe_request_read(&request);

    if (down_interruptible(&master->master_sem)) {
        ec_foe_request_clear(&request);
        return -EINTR;
    }

    if (!(slave = ec_master_find_slave(master, 0, io.slave_position))) {
        up(&master->master_sem);
        ec_foe_request_clear(&request);
        EC_MASTER_ERR(master, "Slave %u does not exist!\n",
                io.slave_position);
        return -EINVAL;
    }

    EC_SLAVE_DBG(slave, 1, "Scheduling FoE read request.\n");

    // schedule request.
    list_add_tail(&request.list, &slave->foe_requests);

    up(&master->master_sem);

    // wait for processing through FSM
    if (wait_event_interruptible(master->request_queue,
                request.state != EC_INT_REQUEST_QUEUED)) {
        // interrupted by signal
        down(&master->master_sem);
        if (request.state == EC_INT_REQUEST_QUEUED) {
            list_del(&request.list);
            up(&master->master_sem);
            ec_foe_request_clear(&request);
            return -EINTR;
        }
        // request already processing: interrupt not possible.
        up(&master->master_sem);
    }

    // wait until master FSM has finished processing
    wait_event(master->request_queue, request.state != EC_INT_REQUEST_BUSY);

    io.result = request.result;
    io.error_code = request.error_code;

    if (request.state != EC_INT_REQUEST_SUCCESS) {
        io.data_size = 0;
        ret = -EIO;
    } else {
        if (request.data_size > io.buffer_size) {
            EC_SLAVE_ERR(slave, "%s(): Buffer too small.\n", __func__);
            ec_foe_request_clear(&request);
            return -ENOBUFS;
        }
        io.data_size = request.data_size;
        if (copy_to_user((void __user *) io.buffer,
                    request.buffer, io.data_size)) {
            ec_foe_request_clear(&request);
            return -EFAULT;
        }
        ret = 0;
    }

    if (__copy_to_user((void __user *) arg, &io, sizeof(io))) {
        ret = -EFAULT;
    }

    ec_foe_request_clear(&request);
    return ret;
}

/****************************************************************************/

/** Write a file to a slave via FoE
 *
 * \return Zero on success, otherwise a negative error code.
 */
static ATTRIBUTES int ec_ioctl_slave_foe_write(
        ec_master_t *master, /**< EtherCAT master. */
        void *arg /**< ioctl() argument. */
        )
{
    ec_ioctl_slave_foe_t io;
    ec_foe_request_t request;
    ec_slave_t *slave;
    int ret;

    if (copy_from_user(&io, (void __user *) arg, sizeof(io))) {
        return -EFAULT;
    }

    ec_foe_request_init(&request, io.file_name);

    ret = ec_foe_request_alloc(&request, io.buffer_size);
    if (ret) {
        ec_foe_request_clear(&request);
        return ret;
    }

    if (copy_from_user(request.buffer,
                (void __user *) io.buffer, io.buffer_size)) {
        ec_foe_request_clear(&request);
        return -EFAULT;
    }

    request.data_size = io.buffer_size;
    ec_foe_request_write(&request);

    if (down_interruptible(&master->master_sem)) {
        ec_foe_request_clear(&request);
        return -EINTR;
    }

    if (!(slave = ec_master_find_slave(master, 0, io.slave_position))) {
        up(&master->master_sem);
        EC_MASTER_ERR(master, "Slave %u does not exist!\n",
                io.slave_position);
        ec_foe_request_clear(&request);
        return -EINVAL;
    }

    EC_SLAVE_DBG(slave, 1, "Scheduling FoE write request.\n");

    // schedule FoE write request.
    list_add_tail(&request.list, &slave->foe_requests);

    up(&master->master_sem);

    // wait for processing through FSM
    if (wait_event_interruptible(master->request_queue,
                request.state != EC_INT_REQUEST_QUEUED)) {
        // interrupted by signal
        down(&master->master_sem);
        if (request.state == EC_INT_REQUEST_QUEUED) {
            // abort request
            list_del(&request.list);
            up(&master->master_sem);
            ec_foe_request_clear(&request);
            return -EINTR;
        }
        up(&master->master_sem);
    }

    // wait until master FSM has finished processing
    wait_event(master->request_queue, request.state != EC_INT_REQUEST_BUSY);

    io.result = request.result;
    io.error_code = request.error_code;

    ret = request.state == EC_INT_REQUEST_SUCCESS ? 0 : -EIO;

    if (__copy_to_user((void __user *) arg, &io, sizeof(io))) {
        ret = -EFAULT;
    }

    ec_foe_request_clear(&request);
    return ret;
}

/****************************************************************************/

/** Read an SoE IDN.
 *
 * \return Zero on success, otherwise a negative error code.
 */
static ATTRIBUTES int ec_ioctl_slave_soe_read(
        ec_master_t *master, /**< EtherCAT master. */
        void *arg /**< ioctl() argument. */
        )
{
    ec_ioctl_slave_soe_read_t ioctl;
    u8 *data;
    int retval;

    if (copy_from_user(&ioctl, (void __user *) arg, sizeof(ioctl))) {
        return -EFAULT;
    }

    data = kmalloc(ioctl.mem_size, GFP_KERNEL);
    if (!data) {
        EC_MASTER_ERR(master, "Failed to allocate %zu bytes of IDN data.\n",
                ioctl.mem_size);
        return -ENOMEM;
    }

    retval = ecrt_master_read_idn(master, ioctl.slave_position,
            ioctl.drive_no, ioctl.idn, data, ioctl.mem_size, &ioctl.data_size,
            &ioctl.error_code);
    if (retval) {
        kfree(data);
        return retval;
    }

    if (copy_to_user((void __user *) ioctl.data,
                data, ioctl.data_size)) {
        kfree(data);
        return -EFAULT;
    }
    kfree(data);

    if (__copy_to_user((void __user *) arg, &ioctl, sizeof(ioctl))) {
        retval = -EFAULT;
    }

    EC_MASTER_DBG(master, 1, "Finished SoE read request.\n");
    return retval;
}

/****************************************************************************/

/** Write an IDN to a slave via SoE.
 *
 * \return Zero on success, otherwise a negative error code.
 */
static ATTRIBUTES int ec_ioctl_slave_soe_write(
        ec_master_t *master, /**< EtherCAT master. */
        void *arg /**< ioctl() argument. */
        )
{
    ec_ioctl_slave_soe_write_t ioctl;
    u8 *data;
    int retval;

    if (copy_from_user(&ioctl, (void __user *) arg, sizeof(ioctl))) {
        return -EFAULT;
    }

    data = kmalloc(ioctl.data_size, GFP_KERNEL);
    if (!data) {
        EC_MASTER_ERR(master, "Failed to allocate %zu bytes of IDN data.\n",
                ioctl.data_size);
        return -ENOMEM;
    }
    if (copy_from_user(data, (void __user *) ioctl.data, ioctl.data_size)) {
        kfree(data);
        return -EFAULT;
    }

    retval = ecrt_master_write_idn(master, ioctl.slave_position,
            ioctl.drive_no, ioctl.idn, data, ioctl.data_size,
            &ioctl.error_code);
    kfree(data);
    if (retval) {
        return retval;
    }

    if (__copy_to_user((void __user *) arg, &ioctl, sizeof(ioctl))) {
        retval = -EFAULT;
    }

    EC_MASTER_DBG(master, 1, "Finished SoE write request.\n");
    return retval;
}

/*****************************************************************************
 * ioctl() file operation functions
 ****************************************************************************/

/** ioctl() function to use.
 *
 * For RTDM, there will be ec_ioctl_rtdm_rt and ec_ioctl_rtdm_nrt.
 * For "normal" cdev, there will be ec_ioctl_rt only.
 */
#ifndef EC_IOCTL_RTDM
static long ec_ioctl_nrt(
        ec_master_t *master, /**< EtherCAT master. */
        ec_ioctl_context_t *ctx, /**< Device context. */
        unsigned int cmd, /**< ioctl() command identifier. */
        void *arg /**< ioctl() argument. */);
#endif

/****************************************************************************/

/** Called when an ioctl() command is issued.
 * Both RT and nRT context.
 *
 * \return ioctl() return code.
 */
static long ec_ioctl_both(
        ec_master_t *master, /**< EtherCAT master. */
        ec_ioctl_context_t *ctx, /**< Device context. */
        unsigned int cmd, /**< ioctl() command identifier. */
        void *arg /**< ioctl() argument. */
        )
{
    int ret;

    switch (cmd) {
        case EC_IOCTL_MODULE:
            ret = ec_ioctl_module(arg, ctx);
            break;
        case EC_IOCTL_MASTER_RESCAN:
            if (!ctx->writable) {
                ret = -EPERM;
                break;
            }
            ret = ec_ioctl_master_rescan(master, arg);
            break;
        case EC_IOCTL_MASTER_STATE:
            ret = ec_ioctl_master_state(master, arg, ctx);
            break;
        case EC_IOCTL_MASTER_LINK_STATE:
            ret = ec_ioctl_master_link_state(master, arg, ctx);
            break;
        case EC_IOCTL_SDO_REQUEST_TIMEOUT:
            if (!ctx->writable) {
                ret = -EPERM;
                break;
            }
            ret = ec_ioctl_sdo_request_timeout(master, arg, ctx);
            break;
        case EC_IOCTL_SDO_REQUEST_DATA:
            ret = ec_ioctl_sdo_request_data(master, arg, ctx);
            break;
        case EC_IOCTL_SOE_REQUEST_TIMEOUT:
            if (!ctx->writable) {
                ret = -EPERM;
                break;
            }
            ret = ec_ioctl_soe_request_timeout(master, arg, ctx);
            break;
        case EC_IOCTL_SOE_REQUEST_DATA:
            ret = ec_ioctl_soe_request_data(master, arg, ctx);
            break;
        case EC_IOCTL_REG_REQUEST_DATA:
            ret = ec_ioctl_reg_request_data(master, arg, ctx);
            break;
        case EC_IOCTL_VOE_SEND_HEADER:
            if (!ctx->writable) {
                ret = -EPERM;
                break;
            }
            ret = ec_ioctl_voe_send_header(master, arg, ctx);
            break;
        case EC_IOCTL_VOE_DATA:
            ret = ec_ioctl_voe_data(master, arg, ctx);
            break;
        default:
#ifdef EC_IOCTL_RTDM
            ret = -ENOTTY;
#else
            /* chain non-rt commands for normal cdev */
            ret = ec_ioctl_nrt(master, ctx, cmd, arg);
#endif
            break;
    }

    return ret;
}

/****************************************************************************/

/** Called when an ioctl() command is issued.
 * RTDM: RT only.
 *
 * \return ioctl() return code.
 */
#ifdef EC_IOCTL_RTDM
long ec_ioctl_rtdm_rt
#else
long ec_ioctl
#endif
        (
        ec_master_t *master, /**< EtherCAT master. */
        ec_ioctl_context_t *ctx, /**< Device context. */
        unsigned int cmd, /**< ioctl() command identifier. */
        void *arg /**< ioctl() argument. */
        )
{
#if DEBUG_LATENCY
    cycles_t a = get_cycles(), b;
    unsigned int t;
#endif
    long ret;

    switch (cmd) {
        case EC_IOCTL_SEND:
            if (!ctx->writable) {
                ret = -EPERM;
                break;
            }
            ret = ec_ioctl_send(master, arg, ctx);
            break;
        case EC_IOCTL_RECEIVE:
            if (!ctx->writable) {
                ret = -EPERM;
                break;
            }
            ret = ec_ioctl_receive(master, arg, ctx);
            break;
        case EC_IOCTL_APP_TIME:
            if (!ctx->writable) {
                ret = -EPERM;
                break;
            }
            ret = ec_ioctl_app_time(master, arg, ctx);
            break;
        case EC_IOCTL_SYNC_REF:
            if (!ctx->writable) {
                ret = -EPERM;
                break;
            }
            ret = ec_ioctl_sync_ref(master, arg, ctx);
            break;
        case EC_IOCTL_SYNC_REF_TO:
            if (!ctx->writable) {
                ret = -EPERM;
                break;
            }
            ret = ec_ioctl_sync_ref_to(master, arg, ctx);
            break;
        case EC_IOCTL_SYNC_SLAVES:
            if (!ctx->writable) {
                ret = -EPERM;
                break;
            }
            ret = ec_ioctl_sync_slaves(master, arg, ctx);
            break;
        case EC_IOCTL_REF_CLOCK_TIME:
            if (!ctx->writable) {
                ret = -EPERM;
                break;
            }
            ret = ec_ioctl_ref_clock_time(master, arg, ctx);
            break;
        case EC_IOCTL_SYNC_MON_QUEUE:
            if (!ctx->writable) {
                ret = -EPERM;
                break;
            }
            ret = ec_ioctl_sync_mon_queue(master, arg, ctx);
            break;
        case EC_IOCTL_SYNC_MON_PROCESS:
            if (!ctx->writable) {
                ret = -EPERM;
                break;
            }
            ret = ec_ioctl_sync_mon_process(master, arg, ctx);
            break;
        case EC_IOCTL_RESET:
            if (!ctx->writable) {
                ret = -EPERM;
                break;
            }
            ret = ec_ioctl_reset(master, arg, ctx);
            break;
        case EC_IOCTL_SC_EMERG_POP:
            if (!ctx->writable) {
                ret = -EPERM;
                break;
            }
            ret = ec_ioctl_sc_emerg_pop(master, arg, ctx);
            break;
        case EC_IOCTL_SC_EMERG_CLEAR:
            if (!ctx->writable) {
                ret = -EPERM;
                break;
            }
            ret = ec_ioctl_sc_emerg_clear(master, arg, ctx);
            break;
        case EC_IOCTL_SC_EMERG_OVERRUNS:
            ret = ec_ioctl_sc_emerg_overruns(master, arg, ctx);
            break;
        case EC_IOCTL_SC_STATE:
            ret = ec_ioctl_sc_state(master, arg, ctx);
            break;
        case EC_IOCTL_DOMAIN_PROCESS:
            if (!ctx->writable) {
                ret = -EPERM;
                break;
            }
            ret = ec_ioctl_domain_process(master, arg, ctx);
            break;
        case EC_IOCTL_DOMAIN_QUEUE:
            if (!ctx->writable) {
                ret = -EPERM;
                break;
            }
            ret = ec_ioctl_domain_queue(master, arg, ctx);
            break;
        case EC_IOCTL_DOMAIN_STATE:
            ret = ec_ioctl_domain_state(master, arg, ctx);
            break;
        case EC_IOCTL_SDO_REQUEST_INDEX:
            if (!ctx->writable) {
                ret = -EPERM;
                break;
            }
            ret = ec_ioctl_sdo_request_index(master, arg, ctx);
            break;
        case EC_IOCTL_SDO_REQUEST_STATE:
            ret = ec_ioctl_sdo_request_state(master, arg, ctx);
            break;
        case EC_IOCTL_SDO_REQUEST_READ:
            if (!ctx->writable) {
                ret = -EPERM;
                break;
            }
            ret = ec_ioctl_sdo_request_read(master, arg, ctx);
            break;
        case EC_IOCTL_SDO_REQUEST_WRITE:
            if (!ctx->writable) {
                ret = -EPERM;
                break;
            }
            ret = ec_ioctl_sdo_request_write(master, arg, ctx);
            break;
        case EC_IOCTL_SOE_REQUEST_IDN:
            if (!ctx->writable) {
                ret = -EPERM;
                break;
            }
            ret = ec_ioctl_soe_request_index(master, arg, ctx);
            break;
        case EC_IOCTL_SOE_REQUEST_STATE:
            ret = ec_ioctl_soe_request_state(master, arg, ctx);
            break;
        case EC_IOCTL_SOE_REQUEST_READ:
            if (!ctx->writable) {
                ret = -EPERM;
                break;
            }
            ret = ec_ioctl_soe_request_read(master, arg, ctx);
            break;
        case EC_IOCTL_SOE_REQUEST_WRITE:
            if (!ctx->writable) {
                ret = -EPERM;
                break;
            }
            ret = ec_ioctl_soe_request_write(master, arg, ctx);
            break;
        case EC_IOCTL_REG_REQUEST_STATE:
            ret = ec_ioctl_reg_request_state(master, arg, ctx);
            break;
        case EC_IOCTL_REG_REQUEST_WRITE:
            if (!ctx->writable) {
                ret = -EPERM;
                break;
            }
            ret = ec_ioctl_reg_request_write(master, arg, ctx);
            break;
        case EC_IOCTL_REG_REQUEST_READ:
            if (!ctx->writable) {
                ret = -EPERM;
                break;
            }
            ret = ec_ioctl_reg_request_read(master, arg, ctx);
            break;
        case EC_IOCTL_VOE_REC_HEADER:
            ret = ec_ioctl_voe_rec_header(master, arg, ctx);
            break;
        case EC_IOCTL_VOE_READ:
            if (!ctx->writable) {
                ret = -EPERM;
                break;
            }
            ret = ec_ioctl_voe_read(master, arg, ctx);
            break;
        case EC_IOCTL_VOE_READ_NOSYNC:
            if (!ctx->writable) {
                ret = -EPERM;
                break;
            }
            ret = ec_ioctl_voe_read_nosync(master, arg, ctx);
            break;
        case EC_IOCTL_VOE_WRITE:
            if (!ctx->writable) {
                ret = -EPERM;
                break;
            }
            ret = ec_ioctl_voe_write(master, arg, ctx);
            break;
        case EC_IOCTL_VOE_EXEC:
            if (!ctx->writable) {
                ret = -EPERM;
                break;
            }
            ret = ec_ioctl_voe_exec(master, arg, ctx);
            break;
        default:
            ret = ec_ioctl_both(master, ctx, cmd, arg);
            break;
    }

#if DEBUG_LATENCY
    b = get_cycles();
    t = (unsigned int) ((b - a) * 1000LL) / cpu_khz;
    if (t > 50) {
        EC_MASTER_WARN(master, "ioctl(0x%02x) took %u us.\n",
                _IOC_NR(cmd), t);
    }
#endif

    return ret;
}

/****************************************************************************/

/** Called when an ioctl() command is issued.
 * nRT context only.
 *
 * \return ioctl() return code.
 */
#ifdef EC_IOCTL_RTDM
long ec_ioctl_rtdm_nrt
#else
static long ec_ioctl_nrt
#endif
        (
        ec_master_t *master, /**< EtherCAT master. */
        ec_ioctl_context_t *ctx, /**< Device context. */
        unsigned int cmd, /**< ioctl() command identifier. */
        void *arg /**< ioctl() argument. */
        )
{
#if DEBUG_LATENCY && !defined(EC_IOCTL_RTDM)
    cycles_t a = get_cycles(), b;
    unsigned int t;
#endif
    int ret;

    switch (cmd) {
        case EC_IOCTL_MASTER:
            ret = ec_ioctl_master(master, arg);
            break;
        case EC_IOCTL_SLAVE:
            ret = ec_ioctl_slave(master, arg);
            break;
        case EC_IOCTL_SLAVE_SYNC:
            ret = ec_ioctl_slave_sync(master, arg);
            break;
        case EC_IOCTL_SLAVE_SYNC_PDO:
            ret = ec_ioctl_slave_sync_pdo(master, arg);
            break;
        case EC_IOCTL_SLAVE_SYNC_PDO_ENTRY:
            ret = ec_ioctl_slave_sync_pdo_entry(master, arg);
            break;
        case EC_IOCTL_DOMAIN:
            ret = ec_ioctl_domain(master, arg);
            break;
        case EC_IOCTL_DOMAIN_FMMU:
            ret = ec_ioctl_domain_fmmu(master, arg);
            break;
        case EC_IOCTL_DOMAIN_DATA:
            ret = ec_ioctl_domain_data(master, arg);
            break;
        case EC_IOCTL_MASTER_DEBUG:
            if (!ctx->writable) {
                ret = -EPERM;
                break;
            }
            ret = ec_ioctl_master_debug(master, arg);
            break;
        case EC_IOCTL_SLAVE_STATE:
            if (!ctx->writable) {
                ret = -EPERM;
                break;
            }
            ret = ec_ioctl_slave_state(master, arg);
            break;
        case EC_IOCTL_SLAVE_SDO:
            ret = ec_ioctl_slave_sdo(master, arg);
            break;
        case EC_IOCTL_SLAVE_SDO_ENTRY:
            ret = ec_ioctl_slave_sdo_entry(master, arg);
            break;
        case EC_IOCTL_SLAVE_SDO_UPLOAD:
            if (!ctx->writable) {
                ret = -EPERM;
                break;
            }
            ret = ec_ioctl_slave_sdo_upload(master, arg);
            break;
        case EC_IOCTL_SLAVE_SDO_DOWNLOAD:
            if (!ctx->writable) {
                ret = -EPERM;
                break;
            }
            ret = ec_ioctl_slave_sdo_download(master, arg);
            break;
        case EC_IOCTL_SLAVE_SII_READ:
            ret = ec_ioctl_slave_sii_read(master, arg);
            break;
        case EC_IOCTL_SLAVE_SII_WRITE:
            if (!ctx->writable) {
                ret = -EPERM;
                break;
            }
            ret = ec_ioctl_slave_sii_write(master, arg);
            break;
        case EC_IOCTL_SLAVE_REG_READ:
            if (!ctx->writable) {
                ret = -EPERM;
                break;
            }
            ret = ec_ioctl_slave_reg_read(master, arg);
            break;
        case EC_IOCTL_SLAVE_REG_WRITE:
            if (!ctx->writable) {
                ret = -EPERM;
                break;
            }
            ret = ec_ioctl_slave_reg_write(master, arg);
            break;
        case EC_IOCTL_SLAVE_FOE_READ:
            if (!ctx->writable) {
                ret = -EPERM;
                break;
            }
            ret = ec_ioctl_slave_foe_read(master, arg);
            break;
        case EC_IOCTL_SLAVE_FOE_WRITE:
            if (!ctx->writable) {
                ret = -EPERM;
                break;
            }
            ret = ec_ioctl_slave_foe_write(master, arg);
            break;
        case EC_IOCTL_SLAVE_SOE_READ:
            if (!ctx->writable) {
                ret = -EPERM;
                break;
            }
            ret = ec_ioctl_slave_soe_read(master, arg);
            break;
        case EC_IOCTL_SLAVE_SOE_WRITE:
            if (!ctx->writable) {
                ret = -EPERM;
                break;
            }
            ret = ec_ioctl_slave_soe_write(master, arg);
            break;
#ifdef EC_EOE
        case EC_IOCTL_SLAVE_EOE_IP_PARAM:
            if (!ctx->writable) {
                ret = -EPERM;
                break;
            }
            ret = ec_ioctl_slave_eoe_ip_param(master, arg);
            break;
#endif
        case EC_IOCTL_CONFIG:
            ret = ec_ioctl_config(master, arg);
            break;
        case EC_IOCTL_CONFIG_PDO:
            ret = ec_ioctl_config_pdo(master, arg);
            break;
        case EC_IOCTL_CONFIG_PDO_ENTRY:
            ret = ec_ioctl_config_pdo_entry(master, arg);
            break;
        case EC_IOCTL_CONFIG_SDO:
            ret = ec_ioctl_config_sdo(master, arg);
            break;
        case EC_IOCTL_CONFIG_IDN:
            ret = ec_ioctl_config_idn(master, arg);
            break;
        case EC_IOCTL_CONFIG_FLAG:
            ret = ec_ioctl_config_flag(master, arg);
            break;
#ifdef EC_EOE
        case EC_IOCTL_CONFIG_EOE_IP_PARAM:
            ret = ec_ioctl_config_ip(master, arg);
            break;
        case EC_IOCTL_EOE_HANDLER:
            ret = ec_ioctl_eoe_handler(master, arg);
            break;
#endif

        /* Application interface */

        case EC_IOCTL_REQUEST:
            if (!ctx->writable) {
                ret = -EPERM;
                break;
            }
            ret = ec_ioctl_request(master, arg, ctx);
            break;
        case EC_IOCTL_CREATE_DOMAIN:
            if (!ctx->writable) {
                ret = -EPERM;
                break;
            }
            ret = ec_ioctl_create_domain(master, arg, ctx);
            break;
        case EC_IOCTL_CREATE_SLAVE_CONFIG:
            if (!ctx->writable) {
                ret = -EPERM;
                break;
            }
            ret = ec_ioctl_create_slave_config(master, arg, ctx);
            break;
        case EC_IOCTL_SELECT_REF_CLOCK:
            if (!ctx->writable) {
                ret = -EPERM;
                break;
            }
            ret = ec_ioctl_select_ref_clock(master, arg, ctx);
            break;
        case EC_IOCTL_ACTIVATE:
            if (!ctx->writable) {
                ret = -EPERM;
                break;
            }
            ret = ec_ioctl_activate(master, arg, ctx);
            break;
        case EC_IOCTL_DEACTIVATE:
            if (!ctx->writable) {
                ret = -EPERM;
                break;
            }
            ret = ec_ioctl_deactivate(master, arg, ctx);
            break;
        case EC_IOCTL_SC_SYNC:
            if (!ctx->writable) {
                ret = -EPERM;
                break;
            }
            ret = ec_ioctl_sc_sync(master, arg, ctx);
            break;
        case EC_IOCTL_SC_WATCHDOG:
            if (!ctx->writable) {
                ret = -EPERM;
                break;
            }
            ret = ec_ioctl_sc_watchdog(master, arg, ctx);
            break;
        case EC_IOCTL_SC_ADD_PDO:
            if (!ctx->writable) {
                ret = -EPERM;
                break;
            }
            ret = ec_ioctl_sc_add_pdo(master, arg, ctx);
            break;
        case EC_IOCTL_SC_CLEAR_PDOS:
            if (!ctx->writable) {
                ret = -EPERM;
                break;
            }
            ret = ec_ioctl_sc_clear_pdos(master, arg, ctx);
            break;
        case EC_IOCTL_SC_ADD_ENTRY:
            if (!ctx->writable) {
                ret = -EPERM;
                break;
            }
            ret = ec_ioctl_sc_add_entry(master, arg, ctx);
            break;
        case EC_IOCTL_SC_CLEAR_ENTRIES:
            if (!ctx->writable) {
                ret = -EPERM;
                break;
            }
            ret = ec_ioctl_sc_clear_entries(master, arg, ctx);
            break;
        case EC_IOCTL_SC_REG_PDO_ENTRY:
            if (!ctx->writable) {
                ret = -EPERM;
                break;
            }
            ret = ec_ioctl_sc_reg_pdo_entry(master, arg, ctx);
            break;
        case EC_IOCTL_SC_REG_PDO_POS:
            if (!ctx->writable) {
                ret = -EPERM;
                break;
            }
            ret = ec_ioctl_sc_reg_pdo_pos(master, arg, ctx);
            break;
        case EC_IOCTL_SC_DC:
            if (!ctx->writable) {
                ret = -EPERM;
                break;
            }
            ret = ec_ioctl_sc_dc(master, arg, ctx);
            break;
        case EC_IOCTL_SC_SDO:
            if (!ctx->writable) {
                ret = -EPERM;
                break;
            }
            ret = ec_ioctl_sc_sdo(master, arg, ctx);
            break;
        case EC_IOCTL_SC_EMERG_SIZE:
            if (!ctx->writable) {
                ret = -EPERM;
                break;
            }
            ret = ec_ioctl_sc_emerg_size(master, arg, ctx);
            break;
        case EC_IOCTL_SC_SDO_REQUEST:
            if (!ctx->writable) {
                ret = -EPERM;
                break;
            }
            ret = ec_ioctl_sc_create_sdo_request(master, arg, ctx);
            break;
        case EC_IOCTL_SC_SOE_REQUEST:
            if (!ctx->writable) {
                ret = -EPERM;
                break;
            }
            ret = ec_ioctl_sc_create_soe_request(master, arg, ctx);
            break;
        case EC_IOCTL_SC_REG_REQUEST:
            if (!ctx->writable) {
                ret = -EPERM;
                break;
            }
            ret = ec_ioctl_sc_create_reg_request(master, arg, ctx);
            break;
        case EC_IOCTL_SC_VOE:
            if (!ctx->writable) {
                ret = -EPERM;
                break;
            }
            ret = ec_ioctl_sc_create_voe_handler(master, arg, ctx);
            break;
        case EC_IOCTL_SC_IDN:
            if (!ctx->writable) {
                ret = -EPERM;
                break;
            }
            ret = ec_ioctl_sc_idn(master, arg, ctx);
            break;
        case EC_IOCTL_SC_FLAG:
            if (!ctx->writable) {
                ret = -EPERM;
                break;
            }
            ret = ec_ioctl_sc_flag(master, arg, ctx);
            break;
        case EC_IOCTL_SC_STATE_TIMEOUT:
            if (!ctx->writable) {
                ret = -EPERM;
                break;
            }
            ret = ec_ioctl_sc_state_timeout(master, arg, ctx);
            break;
#ifdef EC_EOE
        case EC_IOCTL_SC_EOE_IP_PARAM:
            if (!ctx->writable) {
                ret = -EPERM;
                break;
            }
            ret = ec_ioctl_sc_ip(master, arg, ctx);
            break;
#endif
        case EC_IOCTL_DOMAIN_SIZE:
            ret = ec_ioctl_domain_size(master, arg, ctx);
            break;
        case EC_IOCTL_DOMAIN_OFFSET:
            ret = ec_ioctl_domain_offset(master, arg, ctx);
            break;
        case EC_IOCTL_SET_SEND_INTERVAL:
            if (!ctx->writable) {
                ret = -EPERM;
                break;
            }
            ret = ec_ioctl_set_send_interval(master, arg, ctx);
            break;
        default:
#ifdef EC_IOCTL_RTDM
            ret = ec_ioctl_both(master, ctx, cmd, arg);
#else
            ret = -ENOTTY;
#endif
            break;
    }

#if DEBUG_LATENCY && !defined(EC_IOCTL_RTDM)
    b = get_cycles();
    t = (unsigned int) ((b - a) * 1000LL) / cpu_khz;
    if (t > 50) {
        EC_MASTER_WARN(master, "ioctl(0x%02x) took %u us.\n",
                _IOC_NR(cmd), t);
    }
#endif

    return ret;
}

/****************************************************************************/
