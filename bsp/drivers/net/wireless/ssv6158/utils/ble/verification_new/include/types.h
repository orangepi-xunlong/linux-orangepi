#ifndef _TYPES_H_
#define _TYPES_H_


typedef char                        s8;
typedef unsigned char               u8;
typedef short                       s16;
typedef unsigned short              u16;
typedef int                         s32;
typedef unsigned int                u32;
typedef long long                   u64;



#ifndef bool
#define bool                        u8
#endif

//typedef unsigned int                size_t;

typedef volatile u32                SSV_REG;

#if defined(_WIN32) && !defined(__cplusplus)
#define inline __inline
#endif


#define OS_APIs
#define LIB_APIs
#define DRV_APIs

#define IN
#define OUT
#define INOUT

#ifndef NULL
#define NULL                        (void *)0
#endif

#ifndef true
#define true                        1
#endif

#ifndef false
#define false                       0
#endif

#ifndef TRUE
#define TRUE                        1
#endif

#ifndef FALSE
#define FALSE                       0
#endif

#define FX_SUCCESS                  0
#define FX_FAIL                     -1

#ifndef SUCCESS
#define SUCCESS						0
#endif

#ifndef ERROR
#define	ERROR						-1
#endif

#define ASSERT(x) \
{ \
    if (!(x)) \
    { \
        printf("Assert!! file: %s, function: %s, line: %d\n\t" #x, __FILE__, \
                __FUNCTION__, __LINE__); \
	while(1); \
    } \
}

#define WHOISRUNNING() \
    printf("%s() running\n", __FUNCTION__);

#endif /* _TYPES_H_ */
