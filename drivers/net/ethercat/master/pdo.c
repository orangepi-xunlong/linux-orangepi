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
   EtherCAT process data object methods.
*/

/****************************************************************************/

#include <linux/slab.h>
#include <linux/err.h>

#include "pdo.h"

/****************************************************************************/

/** PDO constructor.
 */
void ec_pdo_init(
        ec_pdo_t *pdo /**< EtherCAT PDO */
        )
{
    pdo->sync_index = -1; // not assigned
    pdo->name = NULL;
    INIT_LIST_HEAD(&pdo->entries);
}

/****************************************************************************/

/** PDO copy constructor.
 *
 * \retval  0 Success.
 * \retval <0 Error code.
 */
int ec_pdo_init_copy(
        ec_pdo_t *pdo, /**< PDO to create. */
        const ec_pdo_t *other_pdo /**< PDO to copy from. */
        )
{
    int ret = 0;

    pdo->index = other_pdo->index;
    pdo->sync_index = other_pdo->sync_index;
    pdo->name = NULL;
    INIT_LIST_HEAD(&pdo->entries);

    ret = ec_pdo_set_name(pdo, other_pdo->name);
    if (ret < 0)
        goto out_return;

    ret = ec_pdo_copy_entries(pdo, other_pdo);
    if (ret < 0)
        goto out_clear;

    return 0;

out_clear:
    ec_pdo_clear(pdo);
out_return:
    return ret;
}

/****************************************************************************/

/** PDO destructor.
 */
void ec_pdo_clear(ec_pdo_t *pdo /**< EtherCAT PDO. */)
{
    if (pdo->name)
        kfree(pdo->name);

    ec_pdo_clear_entries(pdo);
}

/****************************************************************************/

/** Clear PDO entry list.
 */
void ec_pdo_clear_entries(ec_pdo_t *pdo /**< EtherCAT PDO. */)
{
    ec_pdo_entry_t *entry, *next;

    // free all PDO entries
    list_for_each_entry_safe(entry, next, &pdo->entries, list) {
        list_del(&entry->list);
        ec_pdo_entry_clear(entry);
        kfree(entry);
    }
}

/****************************************************************************/

/** Set PDO name.
 *
 * \retval  0 Success.
 * \retval <0 Error code.
 */
int ec_pdo_set_name(
        ec_pdo_t *pdo, /**< PDO. */
        const char *name /**< New name. */
        )
{
    unsigned int len;

    if (pdo->name && name && !strcmp(pdo->name, name))
        return 0;

    if (pdo->name)
        kfree(pdo->name);

    if (name && (len = strlen(name))) {
        if (!(pdo->name = (char *) kmalloc(len + 1, GFP_KERNEL))) {
            EC_ERR("Failed to allocate PDO name.\n");
            return -ENOMEM;
        }
        memcpy(pdo->name, name, len + 1);
    } else {
        pdo->name = NULL;
    }

    return 0;
}

/****************************************************************************/

/** Add a new PDO entry to the configuration.
 *
 * \retval Pointer to the added entry, otherwise a ERR_PTR() code.
 */
ec_pdo_entry_t *ec_pdo_add_entry(
        ec_pdo_t *pdo, /**< PDO. */
        uint16_t index, /**< New entry's index. */
        uint8_t subindex, /**< New entry's subindex. */
        uint8_t bit_length /**< New entry's bit length. */
        )
{
    ec_pdo_entry_t *entry;

    if (!(entry = kmalloc(sizeof(ec_pdo_entry_t), GFP_KERNEL))) {
        EC_ERR("Failed to allocate memory for PDO entry.\n");
        return ERR_PTR(-ENOMEM);
    }

    ec_pdo_entry_init(entry);
    entry->index = index;
    entry->subindex = subindex;
    entry->bit_length = bit_length;
    list_add_tail(&entry->list, &pdo->entries);
    return entry;
}

/****************************************************************************/

/** Copy PDO entries from another PDO.
 *
 * \retval  0 Success.
 * \retval <0 Error code.
 */
int ec_pdo_copy_entries(
        ec_pdo_t *pdo, /**< PDO whos entries shall be replaced. */
        const ec_pdo_t *other /**< Pdo with entries to copy. */
        )
{
    ec_pdo_entry_t *entry, *other_entry;
    int ret;

    ec_pdo_clear_entries(pdo);

    list_for_each_entry(other_entry, &other->entries, list) {
        if (!(entry = (ec_pdo_entry_t *)
                    kmalloc(sizeof(ec_pdo_entry_t), GFP_KERNEL))) {
            EC_ERR("Failed to allocate memory for PDO entry copy.\n");
            return -ENOMEM;
        }

        ret = ec_pdo_entry_init_copy(entry, other_entry);
        if (ret < 0) {
            kfree(entry);
            return ret;
        }

        list_add_tail(&entry->list, &pdo->entries);
    }

    return 0;
}

/****************************************************************************/

/** Compares the entries of two PDOs.
 *
 * \retval 1 The entries of the given PDOs are equal.
 * \retval 0 The entries of the given PDOs differ.
 */
int ec_pdo_equal_entries(
        const ec_pdo_t *pdo1, /**< First PDO. */
        const ec_pdo_t *pdo2 /**< Second PDO. */
        )
{
    const struct list_head *head1, *head2, *item1, *item2;
    const ec_pdo_entry_t *entry1, *entry2;

    head1 = item1 = &pdo1->entries;
    head2 = item2 = &pdo2->entries;

    while (1) {
        item1 = item1->next;
        item2 = item2->next;

        if ((item1 == head1) ^ (item2 == head2)) // unequal lengths
            return 0;
        if (item1 == head1) // both finished
            break;

        entry1 = list_entry(item1, ec_pdo_entry_t, list);
        entry2 = list_entry(item2, ec_pdo_entry_t, list);
        if (!ec_pdo_entry_equal(entry1, entry2))
            return 0;
    }

    return 1;
}

/****************************************************************************/

/** Get the number of PDO entries.
 *
 * \return Number of PDO entries.
 */
unsigned int ec_pdo_entry_count(
        const ec_pdo_t *pdo /**< PDO. */
        )
{
    const ec_pdo_entry_t *entry;
    unsigned int num = 0;

    list_for_each_entry(entry, &pdo->entries, list) {
        num++;
    }

    return num;
}

/****************************************************************************/

/** Finds a PDO entry via its position in the list.
 *
 * Const version.
 *
 * \return Search result, or NULL.
 */
const ec_pdo_entry_t *ec_pdo_find_entry_by_pos_const(
        const ec_pdo_t *pdo, /**< PDO. */
        unsigned int pos /**< Position in the list. */
        )
{
    const ec_pdo_entry_t *entry;

    list_for_each_entry(entry, &pdo->entries, list) {
        if (pos--)
            continue;
        return entry;
    }

    return NULL;
}

/****************************************************************************/

/** Outputs the PDOs in the list.
 */
void ec_pdo_print_entries(
        const ec_pdo_t *pdo /**< PDO. */
        )
{
    const ec_pdo_entry_t *entry;

    if (list_empty(&pdo->entries)) {
        printk(KERN_CONT "(none)");
    } else {
        list_for_each_entry(entry, &pdo->entries, list) {
            printk(KERN_CONT "0x%04X:%02X/%u",
                    entry->index, entry->subindex, entry->bit_length);
            if (entry->list.next != &pdo->entries)
                printk(KERN_CONT " ");
        }
    }
}

/****************************************************************************/
