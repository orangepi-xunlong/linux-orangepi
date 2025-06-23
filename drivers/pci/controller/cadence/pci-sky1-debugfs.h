/* SPDX-License-Identifier: GPL-2.0 */
/*
 * pci-sky1 - PCIe debug for CIX's sky1 SoCs
 *
 * Author: Hans Zhang <Hans.Zhang@cixtech.com>
 */

#ifndef _PCI_SKY1_DEBUGFS_H
#define _PCI_SKY1_DEBUGFS_H

void sky1_pcie_debugfs_init(struct sky1_pcie *pcie);
void sky1_pcie_debugfs_exit(struct sky1_pcie *pcie);
const char *sky1_ltssm_sts_name(u32 code);
const char *sky1_local_err_msg(enum sky1_local_err err, u32 code);
const char *sky1_ltssttran_msg(u32 code);

#endif // _PCI_SKY1_DEBUGFS_H
