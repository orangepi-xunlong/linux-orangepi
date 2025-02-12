/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2016 SiFive
 */

#ifndef _ASM_RISCV_PCI_H
#define _ASM_RISCV_PCI_H

#include <linux/types.h>
#include <linux/slab.h>
#include <linux/dma-mapping.h>

#include <asm/io.h>

#define PCIBIOS_MIN_IO		4
#define PCIBIOS_MIN_MEM		16

#if defined(CONFIG_PCI) && defined(CONFIG_NUMA)
static inline int pcibus_to_node(struct pci_bus *bus)
{
	return dev_to_node(&bus->dev);
}
#ifndef cpumask_of_pcibus
#define cpumask_of_pcibus(bus)	(pcibus_to_node(bus) == -1 ?		\
				 cpu_all_mask :				\
				 cpumask_of_node(pcibus_to_node(bus)))
#endif
#endif /* defined(CONFIG_PCI) && defined(CONFIG_NUMA) */

#if defined(CONFIG_SOC_KY_X1)
#define arch_can_pci_mmap_wc() 1
#endif

/* Generic PCI */
#include <asm-generic/pci.h>

#endif  /* _ASM_RISCV_PCI_H */
