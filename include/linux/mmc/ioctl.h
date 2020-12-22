#ifndef LINUX_MMC_IOCTL_H
#define LINUX_MMC_IOCTL_H

#include <linux/types.h>

#define eMMC_Magician_SDK_Changes

#ifdef eMMC_Magician_SDK_Changes
struct mmc_ioc_mag_sdk_cmd {
	/* Implies direction of data.  true = write, false = read */
	int write_flag;

	/* Application-specific command.  true = precede with CMD55 */
	int is_acmd;
    /* If it needs to reset, set this value to true */
    int is_need_to_reset;
    int is_need_to_delay;
	int is_not_need_to_cmd13;

	__u32 opcode;
	__u32 arg;
	__u32 response[5];  /* CMD response */
	unsigned int flags;
	unsigned int blksz;
	unsigned int blocks;

	/*
	 * Sleep at least postsleep_min_us useconds, and at most
	 * postsleep_max_us useconds *after* issuing command.  Needed for
	 * some read commands for which cards have no other way of indicating
	 * they're ready for the next command (i.e. there is no equivalent of
	 * a "busy" indicator for read operations).
	 */
	unsigned int postsleep_min_us;
	unsigned int postsleep_max_us;

	/*
	 * Override driver-computed timeouts.  Note the difference in units!
	 */
	unsigned int data_timeout_ns;
	unsigned int cmd_timeout_ms;

	/*
	 * For 64-bit machines, the next member, ``__u64 data_ptr``, wants to
	 * be 8-byte aligned.  Make sure this struct is the same size when
	 * built for 32-bit.
	 */
	__u32 __pad;

	/* DAT buffer */
	__u64 data_ptr;
};

#define MAX_MMC_IOC_MAG_SDK_CMD   25

struct mmc_ioc_mag_sdk_cmd_array
{
	struct mmc_ioc_mag_sdk_cmd array_ic[MAX_MMC_IOC_MAG_SDK_CMD];
};

#define MMC_IOC_MAG_SDK_CMD _IOWR(MMC_BLOCK_MAJOR, 1, struct mmc_ioc_mag_sdk_cmd_array)

#define MMC_IOC_MAG_SDK_MAX_BYTES  (512L * 512)

#define PRINTLEVEL  1
#if PRINTLEVEL > 0
#define MagDbgPr(format, args...)       \
            printk(KERN_ALERT " " format , ##args);
#else
#define MagDbgPr(format, args...)
#endif

#define MagRelPr(format, args...)       \
            printk(KERN_ALERT " " format , ##args);
#endif


struct mmc_ioc_cmd {
	/* Implies direction of data.  true = write, false = read */
	int write_flag;

	/* Application-specific command.  true = precede with CMD55 */
	int is_acmd;

	__u32 opcode;
	__u32 arg;
	__u32 response[4];  /* CMD response */
	unsigned int flags;
	unsigned int blksz;
	unsigned int blocks;

	/*
	 * Sleep at least postsleep_min_us useconds, and at most
	 * postsleep_max_us useconds *after* issuing command.  Needed for
	 * some read commands for which cards have no other way of indicating
	 * they're ready for the next command (i.e. there is no equivalent of
	 * a "busy" indicator for read operations).
	 */
	unsigned int postsleep_min_us;
	unsigned int postsleep_max_us;

	/*
	 * Override driver-computed timeouts.  Note the difference in units!
	 */
	unsigned int data_timeout_ns;
	unsigned int cmd_timeout_ms;

	/*
	 * For 64-bit machines, the next member, ``__u64 data_ptr``, wants to
	 * be 8-byte aligned.  Make sure this struct is the same size when
	 * built for 32-bit.
	 */
	__u32 __pad;

	/* DAT buffer */
	__u64 data_ptr;
};
#define mmc_ioc_cmd_set_data(ic, ptr) ic.data_ptr = (__u64)(unsigned long) ptr

#define MMC_IOC_CMD _IOWR(MMC_BLOCK_MAJOR, 0, struct mmc_ioc_cmd)


struct mmc_ioc_erase_cmd {
	unsigned int flags;
	unsigned int start_sec;
	unsigned int size_sec;

	/*
	 * For 64-bit machines, the next member, ``__u64 data_ptr``, wants to
	 * be 8-byte aligned.  Make sure this struct is the same size when
	 * built for 32-bit.
	 */
	__u32 __pad;

	/* DAT buffer */
	__u64 data_ptr;
};

#define MMC_IOC_ERASE_CMD _IOWR(MMC_BLOCK_MAJOR, 1, struct mmc_ioc_erase_cmd)
#define MMC_IOC_SECURE_WIPE_CMD _IOWR(MMC_BLOCK_MAJOR, 2, struct mmc_ioc_erase_cmd)


/*
 * Since this ioctl is only meant to enhance (and not replace) normal access
 * to the mmc bus device, an upper data transfer limit of MMC_IOC_MAX_BYTES
 * is enforced per ioctl call.  For larger data transfers, use the normal
 * block device operations.
 */
#define MMC_IOC_MAX_BYTES  (512L * 256)
#endif /* LINUX_MMC_IOCTL_H */
