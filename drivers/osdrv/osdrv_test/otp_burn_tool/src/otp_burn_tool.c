// SPDX-License-Identifier: GPL-3.0-or-later
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/types.h>
#include <getopt.h>

#include "otp_op.h"

#define TOOL_VERSION "v0.1"

#define pr(...)\
do {\
	if (debug)\
		printf(__VA_ARGS__);\
} while (0)

static int debug;
static int g_lock_uuid;

int init(void)
{
	int fd = open("/dev/otp", O_RDWR);

	if (fd < 0) {
		printf("open otp failed\n");
		return -1;
	}

	return fd;
}

int cv186ah_to_bm1688(int fd)
{
	u32 bonding;
	int ret;

	bonding = read_bonding(fd);
	printf("Bonding : 0x%x\n", bonding);
	if ((bonding & 0x1FF) != 0x11) {
		printf("platform is not cv186ah\n");
		return -1;
	}

	bonding |= 0x3F;
	ret = program_bonding(fd, bonding);
	if (ret) {
		printf("program bonding failed\n");
		return -1;
	}

	printf("success\n");
	printf("need reboot\n");
	return 0;
}

int get_uuid(int fd)
{
	u32 id0, id1;
	int ret;

	ret = read_uuid(fd, &id0, &id1);
	if (ret) {
		printf("read uuid failed\n");
		return -1;
	}

	printf("IC      uuid : 0x%x 0x%x\n", id0, id1);
	return 0;
}

int get_file_lines(FILE *fp)
{
	u32 index = 0;
	char *line = NULL;
	size_t len = 0;
	ssize_t nread;

	while ((nread = getline(&line, &len, fp)) != -1)
		index++;

	free(line);
	return index;
}

int write_uuid(int fd)
{
	FILE *fp = NULL;
	FILE *fout = NULL;
	char *line = NULL;
	size_t len = 0;
	ssize_t nread;
	u32 id0, id1;
	u64 decimal;
	char yn = 0;
	u32 index = 0;
	u32 used;

	fp = fopen("uuid.txt", "r");
	if (!fp) {
		printf("uuid.txt open failed, maybe not exist\n");
		return -1;
	}

	fout = fopen("uuid_used.txt", "a+");
	if (!fout) {
		printf("uuid_used.txt open failed, maybe not exist\n");
		return -1;
	}

	used = get_file_lines(fout);
	pr("uuid_used.txt lines : %d\n", used);
	while ((nread = getline(&line, &len, fp)) != -1) {
		if (index == used)
			break;

		index++;
	}

	pr("read : %s", line);
	decimal = strtol(line, NULL, 16);
	id0 = decimal & 0xFFFFFFFF;
	id1 = (decimal >> 32) & 0xFFFFFFFF;

	printf("uuid : 0x%x 0x%x, OK?\n", id0, id1);
	yn = getchar();
	if ((yn == 'Y') || (yn == 'y')) {
		fwrite(line, 1, nread, fout);
		program_uuid(fd, id0, id1);
		if (g_lock_uuid)
			lock_uuid(fd);

		get_uuid(fd);
	}

	free(line);
	fclose(fp);
	fclose(fout);

	return 0;
}

static struct option long_options[] = {
	{"help", no_argument, 0, 'h'},
	{"verbose", no_argument, 0, 'v'},
	{"info", no_argument, 0, 'i'},
	{"cv186ah_to_bm1688", no_argument, 0, 'a'},
	{"program_uuid", no_argument, 0, 'u'},
	{"uuid_id", required_argument, 0, 0},
	{"lock_uuid", no_argument, 0, 'l'},
	{0, 0, 0, 0}
};

void usage(int argc, char **argv)
{
	printf("%s : %s [option]\n", __func__, argv[0]);
	printf("	-h, --help : print this message\n");
	printf("	-v, --verbose : print debug message\n");
	printf("	-i, --info : dump info\n");
	printf("	--program_uuid : program uuid\n");
	printf("		--uuid_id : uuid id in file\n");
	printf("	-l, --lock_uuid : lock uuid after program uuid\n");
	printf("	--cv186ah_to_bm1688 : switch cv186ah to bm1688\n");
}

int main(int argc, char **argv)
{
	int option_index = 0;
	int c, fd;

	if (argc == 1) {
		usage(argc, argv);
		return 0;
	}

	fd = init();
	if (fd < 0)
		return -1;

	while (-1 != (c = getopt_long(argc, argv, "hiv", long_options, &option_index))) {
		switch (c) {
		case 'h':
			usage(argc, argv);
			break;
		case 'v':
			debug = 1;
			break;
		case 'l':
			g_lock_uuid = 1;
			break;
		case 'i':
			printf("Tool version : %s\n", TOOL_VERSION);
			printf("IP   version : 0x%x\n", get_version(fd));
			get_uuid(fd);
			break;
		case 'a':
			cv186ah_to_bm1688(fd);
			break;
		case 'u':
			write_uuid(fd);
			break;
		}
	}

	close(fd);
	return 0;
}
