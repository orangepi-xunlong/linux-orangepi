#include <stdio.h>
#include "types.h"
#include "ssv_lib.h"
#include <sys/time.h>

#define _PRINT_CHARRARY_PREFIX      "    "
#define _PRINT_CHARRARY_COLUMN_N_   8

u32 ssv_atoi_base( const s8 *s, u32 base )
{
    u32  index, upbound=base-1;
    u32  value = 0, v;

    while( (v = (u8)*s) != 0 ) {
        index = v - '0';
        if ( index > 10 && base==16 ) {
            index = (v >= 'a') ? (v - 'a') : (v - 'A');
            index += 10;
        }
        if ( index > upbound )
            break;
        value = value * base + index;
        s++;
    }

    return value;
}

s32 ssv_atoi( const s8 *s )
{
    u32 neg=0, value, base=10;

    if ( *s=='0' ) {
        switch (*++s) {
        case 'x':
        case 'X': base = 16; break;
        case 'b':
        case 'B': base = 2; break;
        default: return 0;
        }
        s++;
    }
    else if ( *s=='-' ) {
        neg = 1;
        s++;
    }

    value = ssv_atoi_base(s, base);

    if ( neg==1 )
        return -(s32)value;
    return (s32)value;

}

int print_charray (int length, unsigned char *charray) {

    int idx_array;
    int idx_column;
    int idx_row;

    // print header
    printf(_PRINT_CHARRARY_PREFIX "%9c", ' ');  // 9c: "## xx ## "
    for(idx_column=0; idx_column < _PRINT_CHARRARY_COLUMN_N_; idx_column++) {
        printf("%2d ", idx_column);
    }
    printf("\n");

    // print hex body
    idx_array  = 0;
    idx_row    = 0;
    idx_column = 0;
    while (idx_array < length) {

        idx_column = idx_array % _PRINT_CHARRARY_COLUMN_N_;

        // row index
        if (idx_column == 0) {
            printf(_PRINT_CHARRARY_PREFIX "## %2d ## ", idx_row);
        }

        // parameter content
        printf("%02x ", charray[idx_array++]);

        // next row?
        if (idx_column == (_PRINT_CHARRARY_COLUMN_N_-1)) {
            printf("\n");
            idx_row++;
        }
    }

    // print termination
    if (idx_column != (_PRINT_CHARRARY_COLUMN_N_-1)) {
        printf("\n\n");
    }

    return idx_array;
}

u32 u8_to_u32(const u8* src, u8 size) {

    u32 dst;
    s8  loop_idx;

    dst = 0;
    for(loop_idx=(size-1); loop_idx >= 0; loop_idx--) {
        dst = dst << 8;
        dst += src[loop_idx];
    }

    return dst;
}

void u32_to_u8(u32 src, u8* dst, u8 size) {

    u8 loop_idx;

    for(loop_idx=0; loop_idx<size; loop_idx++) {
        dst[loop_idx] = (u8)src;
        src = src >> 8;
    }
}

// unit: ms
u64	gettime_ms(void)
{
	struct timeval time;

	gettimeofday(&time, NULL);
	return(time.tv_sec * 1000 + time.tv_usec/1000);
}

void terminate_process(pid_t pid) {
    char str[32];
    printf("kill with %d\n" , pid);
    sprintf(str,"kill -9 %d" , pid);
    system(str);
}
