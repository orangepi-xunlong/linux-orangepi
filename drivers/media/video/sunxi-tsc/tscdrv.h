
#ifndef __TSC_DRV_H__
#define __TSC_DRV_H__


#define DRV_VERSION                 "0.01alpha"      //version
 
#define TS_IRQ_NO                   (SUNXI_IRQ_TS)             //interrupt number, 

#ifndef TSCDEV_MAJOR
#define TSCDEV_MAJOR                (225)
#endif

#ifndef TSCDEV_MINOR
#define TSCDEV_MINOR                (0)
#endif

struct tsc_dev {
    struct cdev cdev;               /* char device struct */
    struct device *dev;             /* ptr to class device struct */
    struct class *class;            /* class for auto create device node */
    struct semaphore sem;           /* mutual exclusion semaphore */
    spinlock_t lock;                /* spinlock to protect ioclt access */
    wait_queue_head_t  wq;          /* wait queue for poll ops */

    struct resource *regs;          /* registers resource */
    char *regsaddr ;                /* registers address */

    unsigned int irq;               /* tsc driver irq number */
    unsigned int irq_flag;          /* flag of tsc driver irq generated */

    int     ts_dev_major;
    int     ts_dev_minor;

    struct clk *parent;
    struct clk *tsc_clk;            /* ts  clock */

    struct intrstatus intstatus;    /* save interrupt status */
    int    is_opened;
    struct pinctrl *pinctrl;
};

#endif

