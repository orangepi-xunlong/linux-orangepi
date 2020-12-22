/* 
 * 
 * Header for common Data types
 *
 */

#ifndef __TYPES_H__
#define __TYPES_H__

//------------------------------------------------------------------------------
//return value defines
//------------------------------------------------------------------------------
#define	OK	(0)
#define	FAIL	(-1)
#define TRUE	(1)
#define	FALSE	(0)
#define true     1
#define false    0

#ifndef NULL
#define NULL    0
#endif

//------------------------------------------------------------------------------
//general data type defines
//------------------------------------------------------------------------------
typedef	void * 		    HANDLE;
typedef unsigned long long  u64;
typedef unsigned int        u32;
typedef unsigned short      u16;
typedef unsigned char       u8;
typedef signed long long    s64;
typedef signed int          s32;
typedef signed short        s16;
typedef signed char         s8;
typedef unsigned int        size_t;

typedef u32             pa_t;
typedef u32             va_t;
typedef unsigned int    irq_flags_t;

//------------------------------------------------------------------------------
//general function pointer defines
//------------------------------------------------------------------------------
typedef s32  (* __pCBK_t)(void *p_arg);	 //call-back
typedef s32  (* __pISR_t)(void *p_arg);	 //ISR
typedef void (* __pTASK_t)(void *p_arg); //Task
typedef s32  (* __pNotifier_t)(u32 message, void *arg);//notifer call-back
typedef	s32  (* __pExceptionHandler)(void);		 //cpu exception handler pointer

#endif /* __TYPES_H__ */
