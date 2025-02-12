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
   EtherCAT PDO list methods.
*/

/****************************************************************************/

#include <linux/module.h>

#include "globals.h"
#include "pdo.h"
#include "slave_config.h"
#include "master.h"

#include "pdo_list.h"

/****************************************************************************/

/** PDO list constructor.
 */
void ec_pdo_list_init(
        ec_pdo_list_t *pl /**< PDO list. */
        )
{
    INIT_LIST_HEAD(&pl->list);
}

/****************************************************************************/

/** PDO list destructor.
 */
void ec_pdo_list_clear(ec_pdo_list_t *pl /**< PDO list. */)
{
    ec_pdo_list_clear_pdos(pl);
}

/****************************************************************************/

/** Clears the list of mapped PDOs.
 */
void ec_pdo_list_clear_pdos(ec_pdo_list_t *pl /**< PDO list. */)
{
    ec_pdo_t *pdo, *next;

    list_for_each_entry_safe(pdo, next, &pl->list, list) {
        list_del_init(&pdo->list);
        ec_pdo_clear(pdo);
        kfree(pdo);
    }
}

/****************************************************************************/

/** Calculates the total size of the mapped PDO entries.
 *
 * \retval Data size in byte.
 */
uint16_t ec_pdo_list_total_size(
        const ec_pdo_list_t *pl /**< PDO list. */
        )
{
    unsigned int bit_size;
    const ec_pdo_t *pdo;
    const ec_pdo_entry_t *pdo_entry;
    uint16_t byte_size;

    bit_size = 0;
    list_for_each_entry(pdo, &pl->list, list) {
        list_for_each_entry(pdo_entry, &pdo->entries, list) {
            bit_size += pdo_entry->bit_length;
        }
    }

    if (bit_size % 8) // round up to full bytes
        byte_size = bit_size / 8 + 1;
    else
        byte_size = bit_size / 8;

    return byte_size;
}

/****************************************************************************/

/** Add a new PDO to the list.
 *
 * \return Pointer to new PDO, otherwise an ERR_PTR() code.
 */
ec_pdo_t *ec_pdo_list_add_pdo(
        ec_pdo_list_t *pl, /**< PDO list. */
        uint16_t index /**< PDO index. */
        )
{
    ec_pdo_t *pdo;

    if (!(pdo = (ec_pdo_t *) kmalloc(sizeof(ec_pdo_t), GFP_KERNEL))) {
        EC_ERR("Failed to allocate memory for PDO.\n");
        return ERR_PTR(-ENOMEM);
    }

    ec_pdo_init(pdo);
    pdo->index = index;
    list_add_tail(&pdo->list, &pl->list);
    return pdo;
}

/****************************************************************************/

/** Add the copy of an existing PDO to the list.
 *
 * \return 0 on success, else < 0
 */
int ec_pdo_list_add_pdo_copy(
        ec_pdo_list_t *pl, /**< PDO list. */
        const ec_pdo_t *pdo /**< PDO to add. */
        )
{
    ec_pdo_t *mapped_pdo;
    int ret;

    // PDO already mapped?
    list_for_each_entry(mapped_pdo, &pl->list, list) {
        if (mapped_pdo->index != pdo->index) continue;
        EC_ERR("PDO 0x%04X is already mapped!\n", pdo->index);
        return -EEXIST;
    }

    if (!(mapped_pdo = kmalloc(sizeof(ec_pdo_t), GFP_KERNEL))) {
        EC_ERR("Failed to allocate PDO memory.\n");
        return -ENOMEM;
    }

    ret = ec_pdo_init_copy(mapped_pdo, pdo);
    if (ret < 0) {
        kfree(mapped_pdo);
        return ret;
    }

    list_add_tail(&mapped_pdo->list, &pl->list);
    return 0;
}

/****************************************************************************/

/** Makes a deep copy of another PDO list.
 *
 * \return 0 on success, else < 0
 */
int ec_pdo_list_copy(
        ec_pdo_list_t *pl, /**< PDO list. */
        const ec_pdo_list_t *other /**< PDO list to copy from. */
        )
{
    ec_pdo_t *other_pdo;
    int ret;

    ec_pdo_list_clear_pdos(pl);

    // PDO already mapped?
    list_for_each_entry(other_pdo, &other->list, list) {
        ret = ec_pdo_list_add_pdo_copy(pl, other_pdo);
        if (ret)
            return ret;
    }

    return 0;
}

/****************************************************************************/

/** Compares two PDO lists.
 *
 * Only the list is compared, not the PDO entries (i. e. the PDO
 * mapping).
 *
 * \retval 1 The given PDO lists are equal.
 * \retval 0 The given PDO lists differ.
 */
int ec_pdo_list_equal(
        const ec_pdo_list_t *pl1, /**< First list. */
        const ec_pdo_list_t *pl2 /**< Second list. */
        )
{
    const struct list_head *h1, *h2, *l1, *l2;
    const ec_pdo_t *p1, *p2;

    h1 = l1 = &pl1->list;
    h2 = l2 = &pl2->list;

    while (1) {
        l1 = l1->next;
        l2 = l2->next;

        if ((l1 == h1) ^ (l2 == h2)) // unequal lengths
            return 0;
        if (l1 == h1) // both finished
            break;

        p1 = list_entry(l1, ec_pdo_t, list);
        p2 = list_entry(l2, ec_pdo_t, list);

        if (p1->index != p2->index)
            return 0;
    }

    return 1;
}

/****************************************************************************/

/** Finds a PDO with the given index.
 *
 * \return Search result, or NULL.
 */
ec_pdo_t *ec_pdo_list_find_pdo(
        const ec_pdo_list_t *pl, /**< PDO list. */
        uint16_t index /**< PDO index. */
        )
{
    ec_pdo_t *pdo;

    list_for_each_entry(pdo, &pl->list, list) {
        if (pdo->index != index)
            continue;
        return pdo;
    }

    return NULL;
}

/****************************************************************************/

/** Finds a PDO with the given index and returns a const pointer.
 *
 * \return Search result, or NULL.
 */
const ec_pdo_t *ec_pdo_list_find_pdo_const(
        const ec_pdo_list_t *pl, /**< PDO list. */
        uint16_t index /**< PDO index. */
        )
{
    const ec_pdo_t *pdo;

    list_for_each_entry(pdo, &pl->list, list) {
        if (pdo->index != index)
            continue;
        return pdo;
    }

    return NULL;
}

/****************************************************************************/

/** Finds a PDO via its position in the list.
 *
 * Const version.
 *
 * \return Zero on success, otherwise a negative error code.
 */
const ec_pdo_t *ec_pdo_list_find_pdo_by_pos_const(
        const ec_pdo_list_t *pl, /**< PDO list. */
        unsigned int pos /**< Position in the list. */
        )
{
    const ec_pdo_t *pdo;

    list_for_each_entry(pdo, &pl->list, list) {
        if (pos--)
            continue;
        return pdo;
    }

    return NULL;
}

/****************************************************************************/

/** Get the number of PDOs in the list.
 *
 * \return Number of PDOs.
 */
unsigned int ec_pdo_list_count(
        const ec_pdo_list_t *pl /**< PDO list. */
        )
{
    const ec_pdo_t *pdo;
    unsigned int num = 0;

    list_for_each_entry(pdo, &pl->list, list) {
        num++;
    }

    return num;
}

/****************************************************************************/

/** Outputs the PDOs in the list.
 */
void ec_pdo_list_print(
        const ec_pdo_list_t *pl /**< PDO list. */
        )
{
    const ec_pdo_t *pdo;

    if (list_empty(&pl->list)) {
        printk(KERN_CONT "(none)");
    } else {
        list_for_each_entry(pdo, &pl->list, list) {
            printk(KERN_CONT "0x%04X", pdo->index);
            if (pdo->list.next != &pl->list)
                printk(KERN_CONT " ");
        }
    }
}

/****************************************************************************/
