#ifndef __ASM_ARM_SUSPEND_H
#define __ASM_ARM_SUSPEND_H
extern int __cpu_suspend(unsigned long, int (*)(unsigned long));
extern void cpu_resume(void);
extern int cpu_suspend(unsigned long, int (*)(unsigned long));

#endif
