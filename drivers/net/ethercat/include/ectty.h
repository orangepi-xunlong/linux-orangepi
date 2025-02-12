/*****************************************************************************
 *
 *  Copyright (C) 2006-2008  Florian Pose, Ingenieurgemeinschaft IgH
 *
 *  This file is part of the IgH EtherCAT master userspace library.
 *
 *  The IgH EtherCAT master userspace library is free software; you can
 *  redistribute it and/or modify it under the terms of the GNU Lesser General
 *  Public License as published by the Free Software Foundation; version 2.1
 *  of the License.
 *
 *  The IgH EtherCAT master userspace library is distributed in the hope that
 *  it will be useful, but WITHOUT ANY WARRANTY; without even the implied
 *  warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with the IgH EtherCAT master userspace library. If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 ****************************************************************************/

/** \file
 *
 * EtherCAT virtual TTY interface.
 *
 * \defgroup TTYInterface EtherCAT Virtual TTY Interface
 *
 * @{
 */

/****************************************************************************/

#ifndef __ECTTY_H__
#define __ECTTY_H__

#include <linux/termios.h>

/*****************************************************************************
 * Data types
 ****************************************************************************/

struct ec_tty;
typedef struct ec_tty ec_tty_t; /**< \see ec_tty */

/** Operations on the virtual TTY interface.
 */
typedef struct {
    int (*cflag_changed)(void *, tcflag_t); /**< Called when the serial
                                              * settings shall be changed. The
                                              * \a cflag argument contains the
                                              * new settings. */
} ec_tty_operations_t;

/*****************************************************************************
 * Global functions
 ****************************************************************************/

/** Create a virtual TTY interface.
 *
 * \param ops Set of callbacks.
 * \param cb_data Arbitrary data, that is passed to any callback.
 *
 * \return Pointer to the interface object, otherwise an ERR_PTR value.
 */
ec_tty_t *ectty_create(
        const ec_tty_operations_t *ops,
        void *cb_data
        );

/*****************************************************************************
 * TTY interface methods
 ****************************************************************************/

/** Releases a virtual TTY interface.
 */
void ectty_free(
        ec_tty_t *tty /**< TTY interface. */
        );

/** Reads data to send from the TTY interface.
 *
 * If there are data to send, they are copied into the \a buffer. At maximum,
 * \a size bytes are copied. The actual number of bytes copied is returned.
 *
 * \return Number of bytes copied.
 */
unsigned int ectty_tx_data(
        ec_tty_t *tty, /**< TTY interface. */
        uint8_t *buffer, /**< Buffer for data to transmit. */
        size_t size /**< Available space in \a buffer. */
        );

/** Pushes received data to the TTY interface.
 */
void ectty_rx_data(
        ec_tty_t *tty, /**< TTY interface. */
        const uint8_t *buffer, /**< Buffer with received data. */
        size_t size /**< Number of bytes in \a buffer. */
        );

/****************************************************************************/

/** @} */

#endif
