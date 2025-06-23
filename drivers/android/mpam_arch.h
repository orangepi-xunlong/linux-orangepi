// SPDX-License-Identifier: GPL-2.0+
#ifndef MPAM_ARCH_H
#define MPAM_ARCH_H

#define MPAM_PARTID_DEFAULT 0

extern void mpam_write_partid(unsigned int partid);
extern unsigned int mpam_get_partid_count(void);

#endif