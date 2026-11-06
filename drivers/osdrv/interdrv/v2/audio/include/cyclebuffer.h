#ifndef __CYCLE_BUFFER_H__
#define __CYCLE_BUFFER_H__
//#include <stdio.h>
//#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_SHAREBUFFER_COUNT  (64)
typedef struct _cycleBuffer {
	int size;
	unsigned int wroffset;
	unsigned int rdoffser;
	unsigned int realWR;
	unsigned int realRD;
	char *buf;
	int buflen;
	int isInit;
	int mutex_index;
	//pthread_mutex_t lock;
} cycleBuffer;

#if 0
int cycle_buf_init(void **ppstCb, int bufLen);
void cycle_buf_destroy(void *pstCb);
int cycle_buf_read(void *pstCb, char *outbuf, int readLen);
int cycle_buf_write(void *pstCb, char *inbuf, int wrireLen);
int cycle_buf_dataLen(void *pstCb);
int cycle_buf_size(void *pstCb);
void cycle_buf_reset(void *pstCb);
int cycle_buf_freeSize(void *pstCb);
int cycle_buf_write_wait(void *pstCb, char *inbuf, int wrireLen, int timeoutMs);
#endif
#ifdef __cplusplus
} /* extern "C" */
#endif

#endif //__CYCLE_BUFFER_H__



