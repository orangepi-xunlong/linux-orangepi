/* SPDX-License-Identifier: GPL-3.0-or-later */

#define u32 uint32_t
#define u64 uint64_t

u32 get_version(int fd);
u32 read_bonding(int fd);
u32 program_bonding(int fd, u32 bonding);

int read_uuid(int fd, u32 *id0, u32 *id1);
int program_uuid(int fd, u32 id0, u32 id1);
int lock_uuid(int fd);

