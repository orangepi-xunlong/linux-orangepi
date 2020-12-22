#ifndef DRIVER_INTERFACE_H
#define DRIVER_INTERFACE_H

#include<linux/ioctl.h>
/*****************************************************************************/
//define iocltl command
#define TSCDEV_IOC_MAGIC            't'

#define TSCDEV_ENABLE_INT           _IO(TSCDEV_IOC_MAGIC,   0)
#define TSCDEV_DISABLE_INT          _IO(TSCDEV_IOC_MAGIC,   1)
#define TSCDEV_GET_PHYSICS          _IO(TSCDEV_IOC_MAGIC,   2)
#define TSCDEV_RELEASE_SEM          _IO(TSCDEV_IOC_MAGIC,   3)
#define TSCDEV_WAIT_INT             _IOR(TSCDEV_IOC_MAGIC,  4,  unsigned long)
#define TSCDEV_ENABLE_CLK           _IOW(TSCDEV_IOC_MAGIC,  5,  unsigned long)
#define TSCDEV_DISABLE_CLK          _IOW(TSCDEV_IOC_MAGIC,  6,  unsigned long)
#define TSCDEV_PUT_CLK              _IOW(TSCDEV_IOC_MAGIC,  7,  unsigned long)
#define TSCDEV_SET_CLK_FREQ         _IOW(TSCDEV_IOC_MAGIC,  8,  unsigned long)
#define TSCDEV_GET_CLK              _IOWR(TSCDEV_IOC_MAGIC, 9,  unsigned long)
#define TSCDEV_GET_CLK_FREQ         _IOWR(TSCDEV_IOC_MAGIC, 10, unsigned long)
#define TSCDEV_SET_SRC_CLK_FREQ     _IOWR(TSCDEV_IOC_MAGIC, 11, unsigned long)
#define TSCDEV_FOR_TEST             _IO(TSCDEV_IOC_MAGIC,   12)


#define TSCDEV_IOC_MAXNR            (12)

/*****************************************************************************/
struct intrstatus {
    unsigned int port0chan;
    unsigned int port0pcr;
    unsigned int port1chan;
    unsigned int port1pcr;
};

/*
 * define struct for clock parameters
 */
#define CLK_NAME_LEN (32)
struct clk_para {
    char            clk_name[CLK_NAME_LEN];
    unsigned int    handle;
    int             clk_rate;
};
//define address of Registers
#define REGS_BASE   (0x01C06000)
#define REGS_SIZE   (0x1000)
#endif
