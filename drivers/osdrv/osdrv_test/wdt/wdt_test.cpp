#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/watchdog.h>
#include <sys/ioctl.h>

#include "gtest/gtest.h"

TEST(Init, check_default_config)
{
	int fd;
	int ret;
	int timeout = 0;
	unsigned int flag = 0;

	fd = open("/dev/watchdog0", O_RDWR);
	ASSERT_GT(fd, 0);

	// Check if default status is active and hw_running
	ret = ioctl(fd, WDIOC_GETSTATUS, &flag);
	EXPECT_EQ((flag & 0xF), 0xD);
	printf("flag : 0x%x\n", flag);

	// Check if default timeout is 30
	ret = ioctl(fd, WDIOC_GETTIMEOUT, &timeout);
	EXPECT_EQ(timeout, 30);

	flag = WDIOS_DISABLECARD;
	ret = ioctl(fd, WDIOC_SETOPTIONS, &flag);
	EXPECT_EQ(ret, 0);

	close(fd);
}

TEST(Start_Stop, start_stop)
{
	int fd;
	int ret;
	unsigned int flag;
	unsigned int time_left, old_time_left = 100;
	int i = 10;

	fd = open("/dev/watchdog0", O_RDWR);
	ASSERT_GT(fd, 0);

	ret = ioctl(fd, WDIOC_GETSTATUS, &flag);
	// Check if status is active after open device
	EXPECT_EQ((flag & 0x1), 0x1);
	printf("flag : 0x%x\n", flag);

	while (i-- > 0) {
		sleep(1);
		ret = ioctl(fd, WDIOC_GETTIMELEFT, &time_left);
		EXPECT_EQ(ret, 0);

		// Check if time left is decrease
		EXPECT_LT(time_left, old_time_left);
		old_time_left = time_left;
		printf("time left : %d\n", time_left);
	}

	flag = WDIOS_DISABLECARD;
	ret = ioctl(fd, WDIOC_SETOPTIONS, &flag);
	EXPECT_EQ(ret, 0);

	ret = ioctl(fd, WDIOC_GETSTATUS, &flag);
	printf("flag : 0x%x\n", flag);
	EXPECT_EQ((flag & 0x1), 0x0);

	close(fd);
}

TEST(Feed, check_time_left)
{
	int fd;
	int ret;
	int timeout = 0;
	unsigned int flag;
	unsigned int time_left;
	int i = 10;

	fd = open("/dev/watchdog0", O_RDWR);
	ASSERT_GT(fd, 0);

	ret = ioctl(fd, WDIOC_GETTIMEOUT, &timeout);
	EXPECT_EQ(timeout, 30);

	while (i-- > 0) {
		sleep(1);
		ret = ioctl(fd, WDIOC_KEEPALIVE, NULL);
		EXPECT_EQ(ret, 0);
		ret = ioctl(fd, WDIOC_GETTIMELEFT, &time_left);
		EXPECT_EQ(ret, 0);

		EXPECT_EQ(time_left, timeout);
		printf("time left : %d\n", time_left);
	}

	flag = WDIOS_DISABLECARD;
	ret = ioctl(fd, WDIOC_SETOPTIONS, &flag);
	EXPECT_EQ(ret, 0);

	close(fd);
}

#define TIMEOUT (60)
TEST(Timeout, set_after_enable)
{
	int fd;
	int ret;
	int timeout = TIMEOUT;
	unsigned int flag;

	fd = open("/dev/watchdog0", O_RDWR);
	ASSERT_GT(fd, 0);

	ret = ioctl(fd, WDIOC_GETSTATUS, &flag);
	// Check if default status is active
	EXPECT_EQ((flag & 0x1), 0x1);
	printf("flag : 0x%x\n", flag);

	ret = ioctl(fd, WDIOC_SETTIMEOUT, &timeout);
	EXPECT_EQ(ret, -1);

	timeout = 0;
	ret = ioctl(fd, WDIOC_GETTIMEOUT, &timeout);
	EXPECT_EQ(timeout, 30);

	flag = WDIOS_DISABLECARD;
	ret = ioctl(fd, WDIOC_SETOPTIONS, &flag);
	EXPECT_EQ(ret, 0);

	close(fd);
}

TEST(Timeout, stop_set)
{
	int fd;
	int ret;
	int timeout;
	unsigned int flag;
	unsigned int time_left, old_time_left = 100;
	int i = 10;

	fd = open("/dev/watchdog0", O_RDWR);
	ASSERT_GT(fd, 0);

	flag = WDIOS_DISABLECARD;
	ret = ioctl(fd, WDIOC_SETOPTIONS, &flag);
	EXPECT_EQ(ret, 0);

	timeout = 0;
	ret = ioctl(fd, WDIOC_GETTIMEOUT, &timeout);
	EXPECT_EQ(timeout, 30);

	timeout = 58;
	ret = ioctl(fd, WDIOC_SETTIMEOUT, &timeout);
	EXPECT_EQ(ret, 0);

	flag = WDIOS_ENABLECARD;
	ret = ioctl(fd, WDIOC_SETOPTIONS, &flag);
	EXPECT_EQ(ret, 0);

	timeout = 0;
	ret = ioctl(fd, WDIOC_GETTIMEOUT, &timeout);
	EXPECT_EQ(timeout, 58);

	while (i-- > 0) {
		sleep(1);
		ret = ioctl(fd, WDIOC_GETTIMELEFT, &time_left);
		EXPECT_EQ(ret, 0);

		// Check if time left is decrease
		EXPECT_LT(time_left, old_time_left);
		old_time_left = time_left;
		printf("time left : %d\n", time_left);
	}

	flag = WDIOS_DISABLECARD;
	ret = ioctl(fd, WDIOC_SETOPTIONS, &flag);
	EXPECT_EQ(ret, 0);

	close(fd);
}

