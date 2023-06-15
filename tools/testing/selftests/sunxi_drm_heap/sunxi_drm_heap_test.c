// SPDX-License-Identifier: GPL-2.0

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/types.h>

#include "../../../../drivers/sunxi_drm_heap/third-party/uapi-heap.h"

#define DEVPATH "/dev"
#define ONE_MEG (1024 * 1024)
static int dmabuf_heap_open(char *name)
{
	int ret, fd;
	char buf[256];

	ret = snprintf(buf, 256, "%s/%s", DEVPATH, name);
	if (ret < 0) {
		printf("snprintf failed\n");
		return ret;
	}

	fd = open(buf, O_RDWR);
	if (fd < 0)
		printf("open %s failed\n", buf);
	return fd;
}

static int dmabuf_heap_alloc_fdflags(int fd, size_t len, unsigned int fd_flags,
				     unsigned int heap_flags, int *dmabuf_fd)
{
	struct dma_heap_allocation_data data = {
		.len = len,
		.fd = 0,
		.fd_flags = fd_flags,
		.heap_flags = heap_flags,
	};
	int ret;

	if (!dmabuf_fd)
		return -EINVAL;

	ret = ioctl(fd, DMA_HEAP_IOC_ALLOC, &data);
	if (ret < 0)
		return ret;
	*dmabuf_fd = (int)data.fd;
	return ret;
}

static int dmabuf_heap_alloc(int fd, size_t len, unsigned int flags,
			     int *dmabuf_fd)
{
	return dmabuf_heap_alloc_fdflags(fd, len, O_RDWR | O_CLOEXEC, flags,
					 dmabuf_fd);
}

static int dmabuf_heap_phys(int fd, int dmabuf_fd)
{
	struct sunxi_drm_phys_data buffer_data;
	int ret;
	buffer_data.handle = (int)dmabuf_fd;
	ret = ioctl(fd, DMA_HEAP_GET_ADDR, &buffer_data);
	if (ret < 0) {
		printf("%s %d %d\n", __FILE__, __LINE__, ret);
	}
	printf("%x have buffer at 0x%x, size 0x%x\n", dmabuf_fd,
	       buffer_data.phys_addr, buffer_data.size);
	return ret;
}

static int test_simple_alloc(char *heap_name)
{
	int heap_fd = -1, dmabuf_fd = -1;
	int dmabuf_fd2 = -1;
	int ret;

	printf("\n\n%s\n\n", __func__);
	heap_fd = dmabuf_heap_open(heap_name);
	if (heap_fd < 0)
		return -1;

	ret = dmabuf_heap_alloc(heap_fd, ONE_MEG, 0, &dmabuf_fd);
	if (ret) {
		printf("Allocation Failed with %d\n", ret);
		goto out;
	}

	ret = dmabuf_heap_phys(heap_fd, dmabuf_fd);
	if (ret) {
		printf("get phys Failed\n");
		ret = -1;
		goto out;
	}

	ret = dmabuf_heap_alloc(heap_fd, 2 * ONE_MEG, 0, &dmabuf_fd2);
	if (ret) {
		printf("Allocation Failed with %d\n", ret);
		goto out;
	}

	ret = dmabuf_heap_phys(heap_fd, dmabuf_fd2);
	if (ret) {
		printf("get phys Failed\n");
		ret = -1;
		goto out;
	}

	ret = 0;

out:
	if (dmabuf_fd2 >= 0)
		close(dmabuf_fd2);
	if (dmabuf_fd >= 0)
		close(dmabuf_fd);
	if (heap_fd >= 0)
		close(heap_fd);

	if (ret)
		printf("%s failed with:%d\n", __func__, ret);
	return ret;
}

int heap_size;
int test_full_alloc(char *heap_name)
{
	int ret = 0;
	int dmabuf_fds[1024] = { 0 };
	int heap_fd;
	int i, j;
	printf("\n\n%s\n\n", __func__);
	memset(dmabuf_fds, 0, sizeof(dmabuf_fds));
	heap_fd = dmabuf_heap_open(heap_name);
	if (heap_fd < 0)
		return -1;

	for (i = 0; i < 1024; i++) {
		ret = dmabuf_heap_alloc(heap_fd, ONE_MEG, 0, &dmabuf_fds[i]);
		if (ret) {
			if (heap_size != 0 && i != heap_size) {
				printf("detected heap %d MB not equal previous detected %d MB\n",
				       i, heap_size);
				ret = -1;
				goto out;
			}
			if (heap_size == 0) {
				printf("heap have size: %d MB\n", i);
				heap_size = i;
			}
			break;
		} else if (ret) {
			printf("Allocation Failed with %d\n", ret);
			ret = -1;
			goto out;
		} else {
			ret = dmabuf_heap_phys(heap_fd, dmabuf_fds[i]);
			if (ret) {
				printf("get phys Failed\n");
				ret = -1;
				goto out;
			}
		}
	}
	for (j = 0; j < heap_size; j++) {
		if (dmabuf_fds[j] >= 0)
			close(dmabuf_fds[j]);
	}
	for (j = 0; j < i; j++) {
		ret = dmabuf_heap_alloc(heap_fd, ONE_MEG, 0, &dmabuf_fds[i]);
		if (ret) {
			printf("memory leaked, %d of %d MB alloced\n", j, i);
			goto out;
		} else if (ret) {
			printf("Allocation Failed with %d,  %d of %d MB alloced\n",
			       ret, j, i);
			goto out;
		}
	}

	ret = 0;
out:
	for (i = 0; i < 1024; i++) {
		if (dmabuf_fds[i] >= 0)
			close(dmabuf_fds[i]);
	}
	if (heap_fd >= 0)
		close(heap_fd);
	if (ret)
		printf("%s failed with:%d\n", __func__, ret);
	return ret;
}

int test_random_alloc(char *heap_name)
{
	int ret = 0;
	int dmabuf_fds[1024];
	int heap_fd = -1;
	int urandom_fd = -1;
	char random_size;
	int total_alloced;
	int i;
	printf("\n\n%s\n\n", __func__);
	memset(dmabuf_fds, 0, sizeof(dmabuf_fds));
	heap_fd = dmabuf_heap_open(heap_name);
	if (heap_fd < 0) {
		ret = -1;
		goto out;
	}
	urandom_fd = open("/dev/urandom", O_RDONLY);
	if (urandom_fd < 0) {
		ret = -1;
		goto out;
	}

	for (i = 0; i < 1024; i++) {
		dmabuf_fds[i] = -1;
	}
	total_alloced = 0;
	for (i = 0; i < 1024; i++) {
		if (read(urandom_fd, &random_size, 1) < 0) {
			ret = -2;
			goto out;
		}
		random_size = random_size % 16 + 1;
		ret = dmabuf_heap_alloc(
			heap_fd, (unsigned long long)random_size * ONE_MEG, 0,
			&dmabuf_fds[i]);
		if (ret) {
			if (random_size + total_alloced >= heap_size) {
				ret = 0;
			} else {
				printf("out of memory when allocing %d after %d allocated\n",
				       random_size, total_alloced);
			}
			break;
		} else if (ret) {
			printf("Allocation Failed with %d\n", ret);
			ret = -1;
			goto out;
		} else {
			ret = dmabuf_heap_phys(heap_fd, dmabuf_fds[i]);
			if (ret) {
				printf("get phys Failed\n");
				ret = -1;
				goto out;
			}
		}

		total_alloced += random_size;
	}
	for (i = 0; i < 1024; i++) {
		if (dmabuf_fds[i] >= 0) {
			if (ret) {
				dmabuf_heap_phys(heap_fd, dmabuf_fds[i]);
			}
		}
		close(dmabuf_fds[i]);
		dmabuf_fds[i] = -1;
	}
	total_alloced = 0;
	for (i = 0; i < 1024; i++) {
		if (read(urandom_fd, &random_size, 1) < 0) {
			ret = -2;
			goto out;
		}
		random_size = random_size % 16 + 1;
		ret = dmabuf_heap_alloc(heap_fd, random_size * ONE_MEG, 0,
					&dmabuf_fds[i]);
		if (ret) {
			if (random_size + total_alloced >= heap_size) {
				ret = 0;
			} else {
				printf("out of memory when allocing %d after %d allocated\n",
				       random_size, total_alloced);
			}
			break;
		} else if (ret) {
			printf("Allocation Failed with %d\n", ret);
			ret = -1;
			goto out;
		} else {
			ret = dmabuf_heap_phys(heap_fd, dmabuf_fds[i]);
			if (ret) {
				printf("get phys Failed\n");
				ret = -1;
				goto out;
			}
		}
		total_alloced += random_size;
	}

out:
	for (i = 0; i < 1024; i++) {
		if (dmabuf_fds[i] >= 0) {
			if (ret) {
				dmabuf_heap_phys(heap_fd, dmabuf_fds[i]);
			}
			close(dmabuf_fds[i]);
		}
	}
	if (urandom_fd >= 0)
		close(urandom_fd);
	if (heap_fd >= 0)
		close(heap_fd);
	if (ret)
		printf("%s failed with:%d\n", __func__, ret);
	return ret;
}

int main(void)
{
	int ret = -1;
	printf("Testing heap: %s\n", "sunxi_drm_heaps");

	ret = test_simple_alloc("sunxi_drm_heap");
	if (ret)
		return ret;

	ret = test_full_alloc("sunxi_drm_heap");
	if (ret)
		return ret;

	ret = test_random_alloc("sunxi_drm_heap");
	if (ret)
		return ret;

	ret = test_full_alloc("sunxi_drm_heap");
	if (ret)
		return ret;

	return 0;
}
