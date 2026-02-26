#ifndef _SSV_LIB_H_
#define _SSV_LIB_H_
#include "types.h"
#include <sys/types.h>
#include <unistd.h>

#define PRINTF_FX(...) printf("[fx:%s]", __FUNCTION__); printf(__VA_ARGS__);

int print_charray (int length, unsigned char *charray);
s32 ssv_atoi(const s8 *s );

/**
 * from u8
 *
 */
u32 u8_to_u32(const u8* src, u8 size);

#define U8_TO_U16(_SRC_)    ((((u16)((_SRC_)[1])) << 8) | ((u16)((_SRC_)[0])))
#define U8_TO_S16(_SRC_)    ((s16)(U8_TO_U16(_SRC_)))

/**
 * to u8
 *
 */
void u32_to_u8(u32 src, u8* dst, u8 size);
#define U16_TO_U8(_SRC_, _DST_) \
    (_DST_)[1] = (u8)((_SRC_) >> 8); \
    (_DST_)[0] = (u8)(_SRC_)

// get current time since the process start execution in ms.
u64		gettime_ms(void);

void	terminate_process(pid_t pid);

#define NEVER_RUN_HERE() \
do { \
	printf("%s#%d: %s()\nNEVER_RUN_HERE !!!\n", __FILE__, __LINE__, __FUNCTION__); \
	exit(1); \
} while (0)

#endif		// _SSV_LIB_H_
