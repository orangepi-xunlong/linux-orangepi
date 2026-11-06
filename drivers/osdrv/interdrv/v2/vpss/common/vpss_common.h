#ifndef _VPSS_COMMON_H_
#define _VPSS_COMMON_H_

#define VIP_ALIGNMENT 0x40
#define GOP_ALIGNMENT 0x10

#define MIN(a, b) (((a) < (b))?(a):(b))
#define MAX(a, b) (((a) > (b))?(a):(b))
#define UPPER(x, y) (((x) + ((1 << (y)) - 1)) >> (y))   // for alignment

#define VIP_64_ALIGN(x) (((x) + 0x3F) & ~0x3F)   // for 64byte alignment
#define VIP_ALIGN(x) (((x) + 0xF) & ~0xF)   // for 16byte alignment

#define R_IDX 0
#define G_IDX 1
#define B_IDX 2

#endif /* _VPSS_COMMON_H_ */
