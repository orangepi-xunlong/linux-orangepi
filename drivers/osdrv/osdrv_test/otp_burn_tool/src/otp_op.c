// SPDX-License-Identifier: GPL-3.0-or-later
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/sophon-otp.h>
#include "otp_op.h"

u32 get_version(int fd)
{
	u32 version = 0;

	if (ioctl(fd, IOCTL_OTP_VERSION, &version) != 0) {
		printf("get version failed\n");
		return -1;
	}

	return version;
}

int read_uuid(int fd, u32 *id0, u32 *id1)
{
	struct otp_config config = {
		.segment = 0,
		.addr = 0,
		.size = 2,
		.value = {0},
	};

	if (ioctl(fd, IOCTL_OTP2_READ_CONFIG, &config)) {
		printf("get uuid failed\n");
		return -1;
	}

	*id0 = config.value[0];
	*id1 = config.value[1];
	return 0;
}

int program_uuid(int fd, u32 id0, u32 id1)
{
	struct otp_config config = {
		.segment = 0,
		.addr = 0,
		.size = 2,
	};

	config.value[0] = id0;
	config.value[1] = id1;

	if (ioctl(fd, IOCTL_OTP2_WRITE_CONFIG, &config)) {
		printf("program uuid failed\n");
		return -1;
	}

	return 0;
}

int lock_uuid(int fd)
{
	struct otp_config config = {
		.segment = 0,
		.addr = 2,
		.size = 1,
	};

	config.value[0] = 0x3;

	if (ioctl(fd, IOCTL_OTP2_WRITE_CONFIG, &config)) {
		printf("lock uuid failed\n");
		return -1;
	}

	return 0;
}

u32 read_bonding(int fd)
{
	struct otp_config config = {
		.segment = 2,
		.addr = 5,
		.size = 1,
	};

	if (ioctl(fd, IOCTL_OTP3_READ_CONFIG, &config) != 0) {
		printf("read Bonding failed\n");
		return -1;
	}

	return config.value[0];
}

u32 program_bonding(int fd, u32 bonding)
{
	struct otp_config config = {
		.segment = 2,
		.addr = 5,
		.size = 1,
	};

	config.value[0] = bonding;
	if (ioctl(fd, IOCTL_OTP3_WRITE_CONFIG, &config) != 0) {
		printf("write Bonding failed\n");
		return -1;
	}

	return 0;
}
