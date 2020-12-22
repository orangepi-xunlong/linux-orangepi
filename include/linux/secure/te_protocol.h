
#ifndef __TE_PROTOCOL_H__
#define __TE_PROTOCOL_H__

#include <linux/mutex.h>
#include "te_types.h"
#include <linux/smp.h>

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/export.h>
#include <linux/time.h>
#include <linux/platform_device.h>
#include <linux/sched.h>

#define TE_IOCTL_MAGIC_NUMBER ('t')
#define TE_IOCTL_OPEN_CLIENT_SESSION \
	_IOWR(TE_IOCTL_MAGIC_NUMBER, 0x10, union te_cmd)
#define TE_IOCTL_CLOSE_CLIENT_SESSION \
	_IOWR(TE_IOCTL_MAGIC_NUMBER, 0x11, union te_cmd)
#define TE_IOCTL_LAUNCH_OPERATION \
	_IOWR(TE_IOCTL_MAGIC_NUMBER, 0x14, union te_cmd)
#define TE_IOCTL_SHARED_MEM_FREE_REQUEST \
	_IOWR(TE_IOCTL_MAGIC_NUMBER, 0x15, union te_cmd)
	
#define TE_IOCTL_MIN_NR	_IOC_NR(TE_IOCTL_OPEN_CLIENT_SESSION)
#define TE_IOCTL_MAX_NR	_IOC_NR(TE_IOCTL_FILE_REQ_COMPLETE)

/* shared buffer is 2 pages: 1st are requests, 2nd are params */
#define TE_CMD_DESC_MAX	(PAGE_SIZE / sizeof(struct te_request))
#define TE_PARAM_MAX	(PAGE_SIZE / sizeof(struct te_oper_param))

struct te_device {
    struct te_request *req_addr;
    dma_addr_t req_addr_phys;
    struct te_oper_param *param_addr;
    dma_addr_t param_addr_phys;
    char *req_param_buf;
    unsigned long *param_bitmap;

    struct list_head used_cmd_list;
    struct list_head free_cmd_list;
};

struct te_cmd_req_desc {
    struct te_request *req_addr;
    struct list_head list;
};

struct te_shmem_desc{

    struct list_head list;
    void* index;
    void *buffer;
    void* u_addr;
    size_t size;
};



struct te_context {
    struct te_device *dev;
    struct list_head shmem_alloc_list;
};

enum{
    /* TEE SMC command type */
    TEE_SMC_INIT_CALL                   = 0x0FFFFFF1,
    TEE_SMC_PLAFORM_OPERATION           = 0x0FFFFFF2,
    TEE_SMC_OPEN_SESSION                = 0x0FFFFFF3,
    TEE_SMC_CLOSE_SESSION               = 0x0FFFFFF4,
    TEE_SMC_INVOKE_COMMAND              = 0x0FFFFFF5,
    TEE_SMC_NS_IRQ_CALL                 = 0x0FFFFFF6,
    TEE_SMC_NS_IRQ_DONE	                = 0x0FFFFFF7,
    TEE_SMC_NS_KERNEL_CALL              = 0x0FFFFFF8,
    TEE_SMC_SECURE_FIQ_CALL		= 0x0FFFFFF9,
    TEE_SMC_SECURE_FIQ_DONE		= 0x0FFFFFFA,
    TEE_SMC_CONFIG_SHMEM	        = 0x0FFFFFFB,
    TEE_SMC_RPC_CALL                    = 0x0FFFFFFC,
    TEE_SMC_RPC_RETURN                  = 0x0FFFFFFD,
    TEE_SMC_SST_COMMAND                 = 0x0FFFFF10,
    TEE_SMC_PM_SUSPEND			= 0x0FFFFF20,
};

enum {
    TE_PARAM_TYPE_NONE          = 0,
    TE_PARAM_TYPE_INT_RO        = 1,
    TE_PARAM_TYPE_INT_RW        = 2,
    TE_PARAM_TYPE_MEM_RO        = 3,
    TE_PARAM_TYPE_MEM_RW        = 4,
};
struct te_oper_param {
    uint32_t index;
    uint32_t type;
    union {
        struct {
                    uint32_t val_a;
					uint32_t val_b;
                } Int;
        struct {
                    void  *base;
                    void  *phys;
                    uint32_t len;
                } Mem;
    } u;
    void *next_ptr_user;
};

struct te_operation {
    uint32_t command;
    struct te_oper_param *list_head;
    /* Maintain a pointer to tail of list to easily add new param node */
    struct te_oper_param *list_tail;
    uint32_t list_count;
    uint32_t status;
    uint32_t iterface_side;
};

struct te_service_id {
    uint32_t time_low;
    uint16_t time_mid;
    uint16_t time_hi_and_version;
    uint8_t clock_seq_and_node[8];
};

struct te_answer {
    uint32_t    result;
    uint32_t    session_id;
    uint32_t    result_origin;
};

/*
 * OpenSession
 */
struct te_opensession {
    struct te_service_id dest_uuid;
    struct te_operation operation;
    struct te_answer answer;
};

/*
 * CloseSession
 */
struct te_closesession {
    struct te_service_id service_id;
    struct te_answer answer;
    uint32_t	session_id;

};

/*
 * LaunchOperation
 */
struct te_launchop {
    struct te_service_id service_id;
    uint32_t		session_id;
    struct te_operation	operation;
    struct te_answer answer;
};

union te_cmd {
    struct te_opensession	opensession;
    struct te_closesession	closesession;
    struct te_launchop	launchop;
};

struct te_request {
    uint32_t		type;
    uint32_t		session_id;
    uint32_t		command_id;
    struct te_oper_param	*params;
    uint32_t		params_size;
    uint32_t		dest_uuid[4];
    uint32_t		result;
    uint32_t		result_origin;
};

struct tee_load_arisc_param {
	uint32_t  image_phys;
	uint32_t  image_size;
	uint32_t  para_phys;
	uint32_t  para_size;
	uint32_t  para_offset;
};

void te_open_session(struct te_opensession *cmd,
	struct te_request *request,
	struct te_context *context);

void te_close_session(struct te_closesession *cmd,
	struct te_request *request,
	struct te_context *context);

void te_launch_operation(struct te_launchop *cmd,
	struct te_request *request,
	struct te_context *context);

unsigned int te_generic_smc(
				unsigned int arg0, 
				unsigned int arg1, 
				unsigned int arg2, 
				unsigned int  arg3
				);
#endif
