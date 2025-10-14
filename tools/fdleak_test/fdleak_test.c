#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <linux/fdleak.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

#define DEVICE_FDLEAK "/dev/log_exception"

void test_socket(void)
{
	int cfd = socket(AF_INET, SOCK_STREAM, 0);
	if (cfd<0) {
		printf("socket error ... \n");
		return;
	}

	close(cfd);
}

int main(int argc, char const *argv[])
{
	int ret = 0;
	int fd;
	struct fdleak_op fdop;

	fd = open(DEVICE_FDLEAK, O_RDWR);
	if (fd < 0) {
		printf("open device failed ... \n");
		return -1;
	}

	fdop.magic = FDLEAK_MAGIC;
	fdop.pid = getpid();
	fdop.wp_mask = MASK_FDLEAK_WP_SOCKET;
	printf("pid=0x%x, mask=0x%x \n", fdop.pid, fdop.wp_mask);

	ret = ioctl(fd, FDLEAK_ENABLE_WATCH, &fdop);
	if (ret) {
		printf("ioctl failed(%d) ... \n", ret);
		close(fd);
		return -1;
	}

	test_socket();

	fdop.magic = FDLEAK_MAGIC;
	fdop.wp_mask = MASK_FDLEAK_WP_SOCKET;
	printf("pid=0x%x, mask=0x%x \n", fdop.pid, fdop.wp_mask);
	ret = ioctl(fd, FDLEAK_DISABLE_WATCH, &fdop);

	close(fd);
	return 0;
}
