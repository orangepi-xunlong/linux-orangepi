/*
 * Chip related low power flags
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
#ifndef _lpflags_h_
#define _lpflags_h_

/* Chip related low power flags (lpflags) */
#define LPFLAGS_SI_GLOBAL_DISABLE		(1 << 0)
#define LPFLAGS_SI_MEM_STDBY_DISABLE		(1 << 1)
#define LPFLAGS_SI_SFLASH_DISABLE		(1 << 2)
#define LPFLAGS_SI_BTLDO3P3_DISABLE		(1 << 3)
#define LPFLAGS_SI_GCI_FORCE_REGCLK_DISABLE	(1 << 4)
#define LPFLAGS_SI_FORCE_PWM_WHEN_RADIO_ON	(1 << 5)
#define LPFLAGS_SI_DS0_SLEEP_PDA_DISABLE	(1 << 6)
#define LPFLAGS_SI_DS1_SLEEP_PDA_DISABLE	(1 << 7)
#define LPFLAGS_PHY_GLOBAL_DISABLE		(1 << 16)
#define LPFLAGS_PHY_LP_DISABLE			(1 << 17)
#define LPFLAGS_PSM_PHY_CTL			(1 << 18)

#endif /* _lpflags_h_ */
