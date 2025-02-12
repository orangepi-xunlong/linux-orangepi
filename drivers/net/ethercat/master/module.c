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

/** \file
 * EtherCAT master driver module.
 */

/****************************************************************************/

#include <linux/module.h>
#include <linux/device.h>
#include <linux/err.h>

#include "globals.h"
#include "master.h"
#include "device.h"

#include <linux/platform_device.h>
#include <linux/of_net.h>
#include <linux/of.h>
#include <linux/io.h>

#define DRIVER_NAME "x1_ec_master"

/****************************************************************************/

static int ec_mac_parse(uint8_t *, const char *, int);

// prototypes for private functions
int ec_mac_equal(const uint8_t *, const uint8_t *);
int ec_mac_is_broadcast(const uint8_t *);

/****************************************************************************/

//static char *main_devices[EC_MAX_MASTERS]; /**< Main devices parameter. */
static unsigned int master_count; /**< Number of masters. */
static unsigned int debug_level;  /**< Debug level parameter. */
static unsigned int run_on_cpu = 0xffffffff; /**< Bind created kernel threads
                                               to a cpu. Default do not bind.
                                              */

static ec_master_t *masters; /**< Array of masters. */
static struct semaphore master_sem; /**< Master semaphore. */

dev_t device_number; /**< Device number for master cdevs. */
struct class *class; /**< Device class. */

static uint8_t macs[EC_MAX_MASTERS][2][ETH_ALEN]; /**< MAC addresses. */

char *ec_master_version_str = EC_MASTER_VERSION; /**< Version string. */

/****************************************************************************/

static int ec_parse_eth_node(struct device_node *eth_node, int master_index, const char *mode)
{
    int ret;

    if (!eth_node || !mode) {
        EC_ERR("Invalid device node or mode\n");
        return -EINVAL;
    }
    if (master_index < 0 || master_index >= master_count) {
        EC_ERR("Invalid master_index (%d). Must be between 0 and %d.\n", master_index, master_count - 1);
	return -EINVAL;
    }

    if (strcmp(mode, "ec_main") == 0) {
        ret = of_get_mac_address(eth_node, macs[master_index][0]);
        if (ret) {
            EC_ERR("Failed to read MAC address from %s.\n", eth_node->name);
            return ret;
        }
    } else if (strcmp(mode, "ec_backup") == 0) {
        ret = of_get_mac_address(eth_node, macs[master_index][1]);
        if (ret) {
            EC_ERR("Failed to read MAC address from %s.\n", eth_node->name);
            return ret;
        }
    } else {
        EC_ERR("Unknown mode '%s' for %s\n", mode, eth_node->name);
        return -EINVAL;
    }
    return 0;
}

static int ec_parse_dt(struct platform_device *pdev)
{
    struct device_node *node = pdev->dev.of_node;
    struct device_node *eth_node;
    const char *mode;
    int master_index;
    int ret, i, count;

    if (!node) {
        EC_ERR("No device tree node found\n");
        return -EINVAL;
    }

    /* Read master_count from the device tree */
    if (of_property_read_u32(node, "master-count", &master_count) || master_count >= EC_MAX_MASTERS) {
        EC_ERR("Failed to read or invalid master-count\n");
	return -EINVAL;
    }

    /* Read run_on_cpu from the device tree */
    if (of_property_read_u32(node, "run-on-cpu", &run_on_cpu) || run_on_cpu > 7) {
        EC_ERR("Failed to read or invalid run-on-cpu\n");
        return -EINVAL;
    }

    /* Read debug_level from the device tree */
    if (of_property_read_u32(node, "debug-level", &debug_level) || debug_level > 2) {
        EC_ERR("Failed to read or invalid debug-level\n");
        return -EINVAL;
    }

    count = of_property_count_elems_of_size(node, "ec-devices", sizeof(phandle));
    if (count < 0) {
	    EC_ERR("Failed to get the number of EtherCAT devices\n");
	    return -EINVAL;
    }

    for (i = 0; i < count; ++i) {
        /* Parse the phandle for ec_devices[i] */
        if (!(eth_node = of_parse_phandle(node, "ec-devices", i))) {
            EC_ERR("Failed to parse ec_devices[%d]\n", i);
            return -EINVAL;
        }

        /* Read master-indexes[i] */
        if (of_property_read_u32_index(node, "master-indexes", i, &master_index)) {
            EC_ERR("Failed to read master-indexes[%d]\n", i);
            of_node_put(eth_node);
            return -EINVAL;
        }

        /* Read modes[i] */
        if (of_property_read_string_index(node, "modes", i, &mode)) {
            EC_ERR("Failed to read modes[%d]\n", i);
            of_node_put(eth_node);
            return -EINVAL;
        }

        /* Parse EtherCAT node */
        ret = ec_parse_eth_node(eth_node, master_index, mode);
        if (ret) {
            of_node_put(eth_node);
            return -EINVAL;
        }

        of_node_put(eth_node);
    }
    return 0;
}

static int ec_probe(struct platform_device *pdev)
{
    int ret, i;

    sema_init(&master_sem, 1);

    ret = ec_parse_dt(pdev);
    if (ret) {
	EC_ERR(DRIVER_NAME ": Failed to parse dts, error %d\n", ret);
	master_count = 0;
    }

    if (master_count) {
        if (alloc_chrdev_region(&device_number,
                    0, master_count, "EtherCAT")) {
            EC_ERR("Failed to obtain device number(s)!\n");
            ret = -EBUSY;
            goto out_return;
        }
    }

#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 4, 0)
    class = class_create(THIS_MODULE, "EtherCAT");
#else
    class = class_create("EtherCAT");
#endif
    if (IS_ERR(class)) {
        EC_ERR("Failed to create device class.\n");
        ret = PTR_ERR(class);
        goto out_cdev;
    }

    // initialize static master variables
    ec_master_init_static();

    if (master_count) {
        if (!(masters = kmalloc(sizeof(ec_master_t) * master_count,
                        GFP_KERNEL))) {
            EC_ERR("Failed to allocate memory"
                    " for EtherCAT masters.\n");
            ret = -ENOMEM;
            goto out_class;
        }
    }

    for (i = 0; i < master_count; i++) {
        ret = ec_master_init(&masters[i], i, macs[i][0], macs[i][1],
                    device_number, class, debug_level, run_on_cpu);
        if (ret)
            goto out_free_masters;
    }

    EC_INFO("%u master%s waiting for devices.\n",
            master_count, (master_count == 1 ? "" : "s"));
    return ret;

out_free_masters:
    for (i--; i >= 0; i--)
        ec_master_clear(&masters[i]);
    kfree(masters);
out_class:
    class_destroy(class);
out_cdev:
    if (master_count)
        unregister_chrdev_region(device_number, master_count);
out_return:
    return ret;
}

static int ec_remove(struct platform_device *pdev)
{
    unsigned int i;

    for (i = 0; i < master_count; i++) {
        ec_master_clear(&masters[i]);
    }

    if (master_count)
        kfree(masters);

    class_destroy(class);

    if (master_count)
        unregister_chrdev_region(device_number, master_count);

    EC_INFO("Master module cleaned up.\n");
    return 0;
}

/****************************************************************************/

/** Get the number of masters.
 */
unsigned int ec_master_count(void)
{
    return master_count;
}

/*****************************************************************************
 * MAC address functions
 ****************************************************************************/

/**
 * \return true, if two MAC addresses are equal.
 */
int ec_mac_equal(
        const uint8_t *mac1, /**< First MAC address. */
        const uint8_t *mac2 /**< Second MAC address. */
        )
{
    unsigned int i;

    for (i = 0; i < ETH_ALEN; i++)
        if (mac1[i] != mac2[i])
            return 0;

    return 1;
}

/****************************************************************************/

/** Maximum MAC string size.
 */
#define EC_MAX_MAC_STRING_SIZE (3 * ETH_ALEN)

/** Print a MAC address to a buffer.
 *
 * The buffer size must be at least EC_MAX_MAC_STRING_SIZE.
 *
 * \return number of bytes written.
 */
ssize_t ec_mac_print(
        const uint8_t *mac, /**< MAC address */
        char *buffer /**< Target buffer. */
        )
{
    off_t off = 0;
    unsigned int i;

    for (i = 0; i < ETH_ALEN; i++) {
        off += sprintf(buffer + off, "%02X", mac[i]);
        if (i < ETH_ALEN - 1) off += sprintf(buffer + off, ":");
    }

    return off;
}

/****************************************************************************/

/**
 * \return true, if the MAC address is all-zero.
 */
int ec_mac_is_zero(
        const uint8_t *mac /**< MAC address. */
        )
{
    unsigned int i;

    for (i = 0; i < ETH_ALEN; i++)
        if (mac[i])
            return 0;

    return 1;
}

/****************************************************************************/

/**
 * \return true, if the given MAC address is the broadcast address.
 */
int ec_mac_is_broadcast(
        const uint8_t *mac /**< MAC address. */
        )
{
    unsigned int i;

    for (i = 0; i < ETH_ALEN; i++)
        if (mac[i] != 0xff)
            return 0;

    return 1;
}

/****************************************************************************/

/** Parse a MAC address from a string.
 *
 * The MAC address must match the regular expression
 * "([0-9a-fA-F]{2}:){5}[0-9a-fA-F]{2}".
 *
 * \return 0 on success, else < 0
 */
static int ec_mac_parse(uint8_t *mac, const char *src, int allow_empty)
{
    unsigned int i, value;
    const char *orig = src;
    char *rem;

    if (!strlen(src)) {
        if (allow_empty){
            return 0;
        } else {
            EC_ERR("MAC address may not be empty.\n");
            return -EINVAL;
        }
    }

    for (i = 0; i < ETH_ALEN; i++) {
        value = simple_strtoul(src, &rem, 16);
        if (rem != src + 2
                || value > 0xFF
                || (i < ETH_ALEN - 1 && *rem != ':')) {
            EC_ERR("Invalid MAC address \"%s\".\n", orig);
            return -EINVAL;
        }
        mac[i] = value;
        if (i < ETH_ALEN - 1) {
            src = rem + 1; // skip colon
        }
    }

    return 0;
}

/****************************************************************************/

/** Outputs frame contents for debugging purposes.
 * If the data block is larger than 256 bytes, only the first 128
 * and the last 128 bytes will be shown
 */
void ec_print_data(const uint8_t *data, /**< pointer to data */
                   size_t size /**< number of bytes to output */
                   )
{
    unsigned int i;

    EC_DBG("");
    for (i = 0; i < size; i++) {
        printk(KERN_CONT "%02X ", data[i]);

        if ((i + 1) % 16 == 0 && i < size - 1) {
            printk(KERN_CONT "\n");
            EC_DBG("");
        }

        if (i + 1 == 128 && size > 256) {
            printk(KERN_CONT "dropped %zu bytes\n", size - 128 - i);
            i = size - 128;
            EC_DBG("");
        }
    }
    printk(KERN_CONT "\n");
}

/****************************************************************************/

/** Outputs frame contents and differences for debugging purposes.
 */
void ec_print_data_diff(const uint8_t *d1, /**< first data */
                        const uint8_t *d2, /**< second data */
                        size_t size /** number of bytes to output */
                        )
{
    unsigned int i;

    EC_DBG("");
    for (i = 0; i < size; i++) {
        if (d1[i] == d2[i]) {
            printk(KERN_CONT ".. ");
        }
        else {
            printk(KERN_CONT "%02X ", d2[i]);
        }
        if ((i + 1) % 16 == 0) {
            printk(KERN_CONT "\n");
            EC_DBG("");
        }
    }
    printk(KERN_CONT "\n");
}

/****************************************************************************/

/** Prints slave states in clear text.
 *
 * \return Size of the created string.
 */
size_t ec_state_string(uint8_t states, /**< slave states */
                       char *buffer, /**< target buffer
                                       (min. EC_STATE_STRING_SIZE bytes) */
                       uint8_t multi /**< Show multi-state mask. */
                       )
{
    off_t off = 0;
    unsigned int first = 1;

    if (!states) {
        off += sprintf(buffer + off, "(unknown)");
        return off;
    }

    if (multi) { // multiple slaves
        if (states & EC_SLAVE_STATE_INIT) {
            off += sprintf(buffer + off, "INIT");
            first = 0;
        }
        if (states & EC_SLAVE_STATE_PREOP) {
            if (!first) off += sprintf(buffer + off, ", ");
            off += sprintf(buffer + off, "PREOP");
            first = 0;
        }
        if (states & EC_SLAVE_STATE_SAFEOP) {
            if (!first) off += sprintf(buffer + off, ", ");
            off += sprintf(buffer + off, "SAFEOP");
            first = 0;
        }
        if (states & EC_SLAVE_STATE_OP) {
            if (!first) off += sprintf(buffer + off, ", ");
            off += sprintf(buffer + off, "OP");
        }
    } else { // single slave
        if ((states & EC_SLAVE_STATE_MASK) == EC_SLAVE_STATE_INIT) {
            off += sprintf(buffer + off, "INIT");
        } else if ((states & EC_SLAVE_STATE_MASK) == EC_SLAVE_STATE_PREOP) {
            off += sprintf(buffer + off, "PREOP");
        } else if ((states & EC_SLAVE_STATE_MASK) == EC_SLAVE_STATE_BOOT) {
            off += sprintf(buffer + off, "BOOT");
        } else if ((states & EC_SLAVE_STATE_MASK) == EC_SLAVE_STATE_SAFEOP) {
            off += sprintf(buffer + off, "SAFEOP");
        } else if ((states & EC_SLAVE_STATE_MASK) == EC_SLAVE_STATE_OP) {
            off += sprintf(buffer + off, "OP");
        } else {
            off += sprintf(buffer + off, "(invalid)");
        }
        first = 0;
    }

    if (states & EC_SLAVE_STATE_ACK_ERR) {
        if (!first) off += sprintf(buffer + off, " + ");
        off += sprintf(buffer + off, "ERROR");
    }

    return off;
}

/*****************************************************************************
 *  Device interface
 ****************************************************************************/

/** Device names.
 */
const char *ec_device_names[2] = {
    "main",
    "backup"
};

/** Offers an EtherCAT device to a certain master.
 *
 * The master decides, if it wants to use the device for EtherCAT operation,
 * or not. It is important, that the offered net_device is not used by the
 * kernel IP stack. If the master, accepted the offer, the address of the
 * newly created EtherCAT device is returned, else \a NULL is returned.
 *
 * \return Pointer to device, if accepted, or NULL if declined.
 * \ingroup DeviceInterface
 */
ec_device_t *ecdev_offer(
        struct net_device *net_dev, /**< net_device to offer */
        ec_pollfunc_t poll, /**< device poll function */
        struct module *module /**< pointer to the module */
        )
{
    ec_master_t *master;
    char str[EC_MAX_MAC_STRING_SIZE];
    unsigned int i, dev_idx;

    for (i = 0; i < master_count; i++) {
        master = &masters[i];
        ec_mac_print(net_dev->dev_addr, str);

        if (down_interruptible(&master->device_sem)) {
            EC_MASTER_WARN(master, "%s() interrupted!\n", __func__);
            return NULL;
        }

        for (dev_idx = EC_DEVICE_MAIN;
                dev_idx < ec_master_num_devices(master); dev_idx++) {
            if (!master->devices[dev_idx].dev
                && (ec_mac_equal(master->macs[dev_idx], net_dev->dev_addr)
                    || ec_mac_is_broadcast(master->macs[dev_idx]))) {

                EC_INFO("Accepting %s as %s device for master %u.\n",
                        str, ec_device_names[dev_idx != 0], master->index);

                ec_device_attach(&master->devices[dev_idx],
                        net_dev, poll, module);
                up(&master->device_sem);

                snprintf(net_dev->name, IFNAMSIZ, "ec%c%u",
                        ec_device_names[dev_idx != 0][0], master->index);

                return &master->devices[dev_idx]; // offer accepted
            }
        }

        up(&master->device_sem);

        EC_MASTER_DBG(master, 1, "Master declined device %s.\n", str);
    }

    return NULL; // offer declined
}

/*****************************************************************************
 * Application interface
 ****************************************************************************/

/** Request a master.
 *
 * Same as ecrt_request_master(), but with ERR_PTR() return value.
 *
 * \return Requested master.
 */
ec_master_t *ecrt_request_master_err(
        unsigned int master_index /**< Master index. */
        )
{
    ec_master_t *master, *errptr = NULL;
    unsigned int dev_idx = EC_DEVICE_MAIN;

    EC_INFO("Requesting master %u...\n", master_index);

    if (master_index >= master_count) {
        EC_ERR("Invalid master index %u.\n", master_index);
        errptr = ERR_PTR(-EINVAL);
        goto out_return;
    }
    master = &masters[master_index];

    if (down_interruptible(&master_sem)) {
        errptr = ERR_PTR(-EINTR);
        goto out_return;
    }

    if (master->reserved) {
        up(&master_sem);
        EC_MASTER_ERR(master, "Master already in use!\n");
        errptr = ERR_PTR(-EBUSY);
        goto out_return;
    }
    master->reserved = 1;
    up(&master_sem);

    if (down_interruptible(&master->device_sem)) {
        errptr = ERR_PTR(-EINTR);
        goto out_release;
    }

    if (master->phase != EC_IDLE) {
        up(&master->device_sem);
        EC_MASTER_ERR(master, "Master still waiting for devices!\n");
        errptr = ERR_PTR(-ENODEV);
        goto out_release;
    }

    for (; dev_idx < ec_master_num_devices(master); dev_idx++) {
        ec_device_t *device = &master->devices[dev_idx];
        if (!try_module_get(device->module)) {
            up(&master->device_sem);
            EC_MASTER_ERR(master, "Device module is unloading!\n");
            errptr = ERR_PTR(-ENODEV);
            goto out_module_put;
        }
    }

    up(&master->device_sem);

    if (ec_master_enter_operation_phase(master)) {
        EC_MASTER_ERR(master, "Failed to enter OPERATION phase!\n");
        errptr = ERR_PTR(-EIO);
        goto out_module_put;
    }

    EC_INFO("Successfully requested master %u.\n", master_index);
    return master;

 out_module_put:
    for (; dev_idx > 0; dev_idx--) {
        ec_device_t *device = &master->devices[dev_idx - 1];
        module_put(device->module);
    }
 out_release:
    master->reserved = 0;
 out_return:
    return errptr;
}

/****************************************************************************/

ec_master_t *ecrt_request_master(unsigned int master_index)
{
    ec_master_t *master = ecrt_request_master_err(master_index);
    return IS_ERR(master) ? NULL : master;
}

/****************************************************************************/

void ecrt_release_master(ec_master_t *master)
{
    unsigned int dev_idx;

    EC_MASTER_INFO(master, "Releasing master...\n");

    if (!master->reserved) {
        EC_MASTER_WARN(master, "%s(): Master was was not requested!\n",
                __func__);
        return;
    }

    ec_master_leave_operation_phase(master);

    for (dev_idx = EC_DEVICE_MAIN; dev_idx < ec_master_num_devices(master);
            dev_idx++) {
        module_put(master->devices[dev_idx].module);
    }

    master->reserved = 0;

    EC_MASTER_INFO(master, "Released.\n");
}

/****************************************************************************/

unsigned int ecrt_version_magic(void)
{
    return ECRT_VERSION_MAGIC;
}

/****************************************************************************/

/** Global request state type translation table.
 *
 * Translates an internal request state to an external one.
 */
const ec_request_state_t ec_request_state_translation_table[] = {
    EC_REQUEST_UNUSED,  // EC_INT_REQUEST_INIT,
    EC_REQUEST_BUSY,    // EC_INT_REQUEST_QUEUED,
    EC_REQUEST_BUSY,    // EC_INT_REQUEST_BUSY,
    EC_REQUEST_SUCCESS, // EC_INT_REQUEST_SUCCESS,
    EC_REQUEST_ERROR    // EC_INT_REQUEST_FAILURE
};

/****************************************************************************/

static const struct of_device_id ec_of_match[] = {
    { .compatible = "igh,x1-ec-master" },
    { },
};
MODULE_DEVICE_TABLE(of, ec_of_match);

static struct platform_driver ec_master_driver = {
    .probe = ec_probe,
    .remove = ec_remove,
    .driver = {
        .name = DRIVER_NAME,
        .of_match_table = of_match_ptr(ec_of_match),
    },
};

module_platform_driver(ec_master_driver);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Ethercat master driver");

