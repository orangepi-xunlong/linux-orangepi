/* SPDX-License-Identifier: GPL-2.0+*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#define RAWNAND_BLKBURNBOOT0 _IO('v', 125)
#define RAWNAND_BLKBURNBOOT1 _IO('v', 126)

typedef struct {
	void *buffer;
	long len;
} burn_param_t;


static void usage(void)
{
	printf("Usage:\n");
	printf("BootLoader_update -boot0 boot0.fex /dev/mtd0!\n");
	printf("BootLoader_update -boot1 boot1.fex /dev/mtd1!\n");
}

int main(int argc, char *argv[])
{
	int ret, fd, fd1, size, boot_type;
	struct stat st;
	char *buf, *bin, *part, name;
	burn_param_t cookie;

	ret = -1;
	if (argc < 4) {
		printf("too few argument\n");
		usage();
		return ret;
	}

	boot_type = -1;
	if (!strncmp("-boot0", argv[1], strlen("-boot0")))
		boot_type = 0;
	else if (!strncmp("-boot1", argv[1], strlen("-boot1")) ||
			!strncmp("-uboot", argv[1], strlen("-uboot")))
		boot_type = 1;

	if (-1 == boot_type) {
		printf("boot type: %s invalid!\n", argv[1]);
		return ret;
	}

	bin = argv[2];
	part = argv[3];

	if ((!bin || !bin[1]) || (!part || !part[1])) {
		printf("invalid bin or part name!\n");
		return ret;
	}

	fd = open(bin, O_RDONLY);
	if (fd < 0) {
		perror("Error: ");
		return ret;
	}

	fstat(fd, &st);
	buf = malloc(st.st_size);
	if (!buf) {
		perror("Error: ");
		return ret;
	}

	if (read(fd, buf, st.st_size) != -1) {
		printf("read %s success\n", boot_type == 0 ? "boot0" : "boot1");
	}

	cookie.buffer = buf;
	cookie.len = st.st_size;

	fd1 = open(part, O_RDWR);
	if (fd1 > 0) {
		if (boot_type == 0)
			ret = ioctl(fd1, RAWNAND_BLKBURNBOOT0, (unsigned long)&cookie);
		else if (boot_type == 1)
			ret = ioctl(fd1, RAWNAND_BLKBURNBOOT1, (unsigned long)&cookie);
	}
	if (ret) {
		printf("burnNand %s failed ! errno is %d : %s\n", boot_type == 0 ? "boot0" : "boot1",
				errno, strerror(errno));
	} else {
		printf("burnNand %s succeed!\n", boot_type == 0 ? "boot0" : "boot1");
	}

	free(cookie.buffer);

	close(fd1);
	close(fd);

	return ret;
}
