/*
 * Generic Broadcom Home Networking Division (HND) PIO engine HW interface
 * This supports the following chips: BCM42xx, 44xx, 47xx .
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

#ifndef	_sbhndpio_h_
#define	_sbhndpio_h_

/* PIO structure,
 *  support two PIO format: 2 bytes access and 4 bytes access
 *  basic FIFO register set is per channel(transmit or receive)
 *  a pair of channels is defined for convenience
 */

/* 2byte-wide pio register set per channel(xmt or rcv) */
typedef volatile struct {
	uint16	fifocontrol;
	uint16	fifodata;
	uint16	fifofree;	/* only valid in xmt channel, not in rcv channel */
	uint16	PAD;
} pio2regs_t;

/* a pair of pio channels(tx and rx) */
typedef volatile struct {
	pio2regs_t	tx;
	pio2regs_t	rx;
} pio2regp_t;

/* 4byte-wide pio register set per channel(xmt or rcv) */
typedef volatile struct {
	uint32	fifocontrol;
	uint32	fifodata;
} pio4regs_t;

/* a pair of pio channels(tx and rx) */
typedef volatile struct {
	pio4regs_t	tx;
	pio4regs_t	rx;
} pio4regp_t;

#endif /* _sbhndpio_h_ */
