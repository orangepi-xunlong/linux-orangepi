/* SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note */
/* Copyright (c) 2023-2024 Arm Technology (China) Co. Ltd. */

#ifndef __UAPI_MISC_ARMCHINA_AIPU_H__
#define __UAPI_MISC_ARMCHINA_AIPU_H__

#include <linux/types.h>
#include <linux/ioctl.h>

/*
 * Zhouyi KMD currently supports Zhouyi aipu v1/v2/v3 hardwares.
 *
 * Structures defined in this header are shared by all hardware versions, but some fields
 * of the structs are specific to certain hardware version(s).
 *
 * In the following member descriptions,
 * [must]     mean that the fields must be filled by user mode driver in all cases.
 * [optional] mean that the fields is optional for user mode driver to enable or not.
 * [alloc]    mean that the buffer(s) represented by the fields must be allocated
 *            by user mode driver before calling IOCTL.
 * [kmd]      mean that the fields should be filled by kernel mode driver
 *            if the calls are successful.
 * [only]     mean that the fields are only for the hardware versions specified to use.
 */

/**
 * emum aipu_arch - AIPU architecture number
 * @AIPU_ARCH_ZHOUYI: AIPU architecture is Zhouyi.
 *
 * This enum is used to indicate the architecture of an AIPU core in the system.
 */
enum aipu_arch {
	AIPU_ARCH_ZHOUYI = 0,
};

/**
 * emum aipu_isa_version - AIPU ISA version number
 * @AIPU_ISA_VERSION_ZHOUYI_V1:   AIPU ISA version is Zhouyi V1.
 * @AIPU_ISA_VERSION_ZHOUYI_V2_0: AIPU ISA version is Zhouyi V2 (Z2).
 * @AIPU_ISA_VERSION_ZHOUYI_V2_1: AIPU ISA version is Zhouyi V2 (Z3).
 * @AIPU_ISA_VERSION_ZHOUYI_V2_2: AIPU ISA version is Zhouyi V2 (X1).
 * @AIPU_ISA_VERSION_ZHOUYI_V3:   AIPU ISA version is Zhouyi V3.
 * @AIPU_ISA_VERSION_ZHOUYI_V3_1: AIPU ISA version is Zhouyi V3_1.
 *
 * Zhouyi architecture has multiple ISA versions released.
 * This enum is used to indicate the ISA version of an AIPU core in the system.
 */
enum aipu_isa_version {
	AIPU_ISA_VERSION_ZHOUYI_V1   = 1,
	AIPU_ISA_VERSION_ZHOUYI_V2_0 = 2,
	AIPU_ISA_VERSION_ZHOUYI_V2_1 = 3,
	AIPU_ISA_VERSION_ZHOUYI_V2_2 = 4,
	AIPU_ISA_VERSION_ZHOUYI_V3   = 5,
	AIPU_ISA_VERSION_ZHOUYI_V3_1 = 6,
};

/**
 * What is an AIPU partition?
 *
 *     For aipu v1/v2, a *partition* represents an AIPU core, and the partition count
 *         is the count of AIPU cores in current system.
 *     For aipu v3, a *partition* represents a group of clusters in the same power/computation
 *         domain, and the partition count is the count of such cluster groups. All partitions
 *         share the same TSM and external register base address.
 */

/**
 * struct aipu_config_clusters - struct used to configure the AIPU clusters (v3 only)
 *
 * @clusters: [kmd][aipu v3 only] Cluster config items
 */
struct aipu_config_clusters {
	struct aipu_cluster_en {
		__u32 en_core_cnt; /* the count of enabled cores in this cluster */
	} clusters[8]; /* at maximum 8 clusters */
};

/**
 * struct aipu_partition_cap - Capability of an AIPU partition
 *
 * @id:      [kmd] AIPU partition ID
 * @arch:    [kmd] Architecture number
 * @version: [kmd] ISA version number
 * @config:  [kmd][aipu v1/v2 only] Configuration number
 * @info:    [kmd] Debugging information
 * @cluster_cnt: [kmd][aipu v3 only] Cluster count of this partition
 * @clusters:    [kmd][aipu v3 only] Cluster capacity of this partition
 *
 * For example,
 *    aipu z1-0901 (v1):
 *    arch == AIPU_ARCH_ZHOUYI (0)
 *    version == AIPU_ISA_VERSION_ZHOUYI_V1 (1)
 *    config == 901
 *
 *    aipu x1-1204 (v2):
 *    arch == AIPU_ARCH_ZHOUYI (0)
 *    version == AIPU_ISA_VERSION_ZHOUYI_V2_2 (4)
 *    config == 1204
 *
 *    aipu v3:
 *    arch == AIPU_ARCH_ZHOUYI (0)
 *    version == AIPU_ISA_VERSION_ZHOUYI_V3 (5)
 *    config == 0, not applicable
 */
struct aipu_partition_cap {
	__u32 id;
	__u32 arch;
	__u32 version;
	__u32 config;
	struct aipu_debugger_info {
		__u64 reg_base;	/* External register base address (physical) */
	} info;
	__u32 cluster_cnt;
	struct aipu_cluster_cap {
		__u32 core_cnt;    /* total core count in this cluster */
		__u32 en_core_cnt; /* currently enabled core count */
		__u32 tec_cnt;
	} clusters[8]; /* every partition has at maximum 8 clusters */
};

/**
 * struct aipu_cap - Common capability of the AIPU partition(s)
 * @partition_cnt:  [kmd] Count of AIPU partition(s) in the system
 * @is_homogeneous: [kmd] IS homogeneous AIPU system or not (1/0)
 * @asid0_base:     [kmd] ASID 0 base address
 * @asid1_base:     [kmd] ASID 1 base address
 * @asid2_base:     [kmd] ASID 2 base address
 * @asid3_base:     [kmd] ASID 3 base address
 * @dtcm_base:      [kmd][aipu v2(x1)/v3] DTCM base address
 * @dtcm_size:      [kmd][aipu v2(x1)/v3] DTCM size
 * @gm0_size:       [kmd][aipu v3 only] GM region 0 size (in bytes)
 * @gm1_size:       [kmd][aipu v3 only] GM region 1 size (in bytes)
 * @partition_cap:  [kmd] Capability of the single AIPU partition
 *
 * For aipu v1/v2, AIPU driver supports the management of multiple AIPU cores.
 * This struct is used to indicate the common capability of all AIPU core(s).
 * User mode driver should get this capability via AIPU_IOCTL_QUERY_CAP command.
 * If the core count is 1, the per-core capability is in the partition_cap member;
 * otherwise user mode driver should get all the per-core capabilities as the
 * partition_cnt indicates via AIPU_IOCTL_QUERY_PARTITION_CAP command.
 *
 * For aipu v3, user mode driver should get the cluster counts by AIPU_IOCTL_QUERY_PARTITION_CAP
 * command if partition_cnt > 1.
 */
struct aipu_cap {
	__u32 partition_cnt;
	__u32 asid_cnt;
	__u64 asid_base[32];
	__u32 is_homogeneous;
	__u64 dtcm_base;
	__u32 dtcm_size;
	__u32 gm0_size;
	__u32 gm1_size;
	struct aipu_partition_cap partition_cap;
};

/**
 * enum aipu_mm_data_type - Data/Buffer type
 * @AIPU_MM_DATA_TYPE_NONE:   No type
 * @AIPU_MM_DATA_TYPE_TEXT:   Text (instructions)
 * @AIPU_MM_DATA_TYPE_RODATA: Read-only data (parameters)
 * @AIPU_MM_DATA_TYPE_STACK:  Stack
 * @AIPU_MM_DATA_TYPE_STATIC: Static data (weights)
 * @AIPU_MM_DATA_TYPE_REUSE:  Reuse data (feature maps)
 * @AIPU_MM_DATA_TYPE_TCB:    aipu v3 TCB
 */
enum aipu_mm_data_type {
	AIPU_MM_DATA_TYPE_NONE,
	AIPU_MM_DATA_TYPE_TEXT,
	AIPU_MM_DATA_TYPE_RODATA,
	AIPU_MM_DATA_TYPE_STACK,
	AIPU_MM_DATA_TYPE_STATIC,
	AIPU_MM_DATA_TYPE_REUSE,
	AIPU_MM_DATA_TYPE_TCB,
};

/**
 * enum aipu_buf_region - ASID regions
 * @AIPU_BUF_ASID_0: [aipu v2/v3 only] ASID 0 region
 * @AIPU_BUF_ASID_1: [aipu v2/v3 only] ASID 1 region
 * @AIPU_BUF_ASID_2: [aipu v2/v3 only] ASID 2 region
 * @AIPU_BUF_ASID_3: [aipu v3 only] ASID 3 region
 * @AIPU_BUF_ASID_x: [aipu v3/v3_1 only] ASID 4-31 region
 */
enum aipu_buf_region {
	AIPU_BUF_ASID_0 = 0,
	AIPU_BUF_ASID_1 = 1,
	AIPU_BUF_ASID_2 = 2,
	AIPU_BUF_ASID_3 = 3,
	AIPU_BUF_ASID_4 = 4,
	AIPU_BUF_ASID_5 = 5,
	AIPU_BUF_ASID_6 = 6,
	AIPU_BUF_ASID_7 = 7,
	AIPU_BUF_ASID_8 = 8,
	AIPU_BUF_ASID_9 = 9,
	AIPU_BUF_ASID_10 = 10,
	AIPU_BUF_ASID_11 = 11,
	AIPU_BUF_ASID_12 = 12,
	AIPU_BUF_ASID_13 = 13,
	AIPU_BUF_ASID_14 = 14,
	AIPU_BUF_ASID_15 = 15,
	AIPU_BUF_ASID_16 = 16,
	AIPU_BUF_ASID_17 = 17,
	AIPU_BUF_ASID_18 = 18,
	AIPU_BUF_ASID_19 = 19,
	AIPU_BUF_ASID_20 = 20,
	AIPU_BUF_ASID_21 = 21,
	AIPU_BUF_ASID_22 = 22,
	AIPU_BUF_ASID_23 = 23,
	AIPU_BUF_ASID_24 = 24,
	AIPU_BUF_ASID_25 = 25,
	AIPU_BUF_ASID_26 = 26,
	AIPU_BUF_ASID_27 = 27,
	AIPU_BUF_ASID_28 = 28,
	AIPU_BUF_ASID_29 = 29,
	AIPU_BUF_ASID_30 = 30,
	AIPU_BUF_ASID_31 = 31,
};

/**
 * enum aipu_buf_region - buffer region type
 * @AIPU_BUF_REGION_DEFAULT:     [aipu v1/v2/v3] default DDR region
 * @AIPU_BUF_REGION_SRAM:        [aipu v1/v2/v3] SRAM region
 * @AIPU_BUF_REGION_DTCM:        [aipu v2(x1)] DTCM region
 */
enum aipu_buf_region_type {
	AIPU_BUF_REGION_DEFAULT     = 0,
	AIPU_BUF_REGION_SRAM        = 1,
	AIPU_BUF_REGION_DTCM        = 2,
};

/**
 * struct aipu_buf_desc - Buffer description.
 *                        KMD returns this info. to the requesting user thread in one ioctl
 * @pa:         [kmd] Buffer physical base address
 * @dev_offset: [kmd] Device offset used in mmap
 * @bytes:      [kmd] Buffer size in bytes
 * @region:     [kmd] this allocated buffer is in memory/SRAM/DTCM/GM region?
 * @asid:       [kmd] ASID region of this buffer
 */
struct aipu_buf_desc {
	__u64 pa;
	__u64 dev_offset;
	__u64 bytes;
	__u8  region;
	__u8  asid;
};

/**
 * struct aipu_buf_request - Buffer allocation request structure.
 * @bytes:         [must] Buffer size to allocate (in bytes)
 * @align_in_page: [must] Buffer address alignment (must be a power of 2)
 * @data_type:     [must] Type of data in this buffer/Type of this buffer
 * @region:        [kmd] set to request a buffer in a default DDR region or a GM region
 * @asid:          [aipu v2/v3 only, optional] from which region (ASID 0/1/2/3) to request
 * @desc:          [kmd]  Descriptor of the successfully allocated buffer
 */
struct aipu_buf_request {
	__u64 bytes;
	__u32 align_in_page;
	__u32 data_type;
	__u8  region;
	__u8  asid;
	struct aipu_buf_desc desc;
};

/**
 * struct aipu_dma_buf_request - Dma-buf request structure.
 * @fd:    [kmd] A dma-buf file descriptor
 * @bytes: [must] Buffer size to request (in bytes):
 */
struct aipu_dma_buf_request {
	int fd;
	__u64 bytes;
};

/**
 * struct aipu_dma_buf - Dma-buf descriptor structure.
 * @fd:    [must] A dma-buf file descriptor
 * @pa:    [kmd] Buffer address
 * @bytes: [kmd] Buffer size allocated (in bytes)
 */
struct aipu_dma_buf {
	int fd;
	__u64 pa;
	__u64 bytes;
};

/**
 * enum aipu_job_execution_flag - Flags for AIPU's executions
 * @AIPU_JOB_EXEC_FLAG_NONE:         No flag
 * @AIPU_JOB_EXEC_FLAG_SRAM_MUTEX:   The job uses SoC SRAM exclusively.
 * @AIPU_JOB_EXEC_FLAG_QOS_SLOW:     [aipu v3 only] Quality of Service (QoS) slow
 * @AIPU_JOB_EXEC_FLAG_QOS_FAST:     [aipu v3 only] QoS fast
 * @AIPU_JOB_EXEC_FLAG_SINGLE_GROUP: [aipu v3 only] the scheduled job is a single group task
 * @AIPU_JOB_EXEC_FLAG_MULTI_GROUP:  [aipu v3 only] the scheduled job is a multi-groups task
 * @AIPU_JOB_EXEC_FLAG_DBG_DISPATCH: [aipu v3 only] the job should be scheduled with debug-dispatch
 * @AIPU_JOB_EXEC_FLAG_SEG_MMU:      [aipu v3 only] the job has configured segment mmu
 */
enum aipu_job_execution_flag {
	AIPU_JOB_EXEC_FLAG_NONE         = 0,
	AIPU_JOB_EXEC_FLAG_SRAM_MUTEX   = 1 << 0,
	AIPU_JOB_EXEC_FLAG_QOS_SLOW     = 1 << 1,
	AIPU_JOB_EXEC_FLAG_QOS_FAST     = 1 << 2,
	AIPU_JOB_EXEC_FLAG_SINGLE_GROUP = 1 << 3,
	AIPU_JOB_EXEC_FLAG_MULTI_GROUP  = 1 << 4,
	AIPU_JOB_EXEC_FLAG_DBG_DISPATCH  = 1 << 5,
	AIPU_JOB_EXEC_FLAG_SEG_MMU       = 1 << 6,
};

/**
 * struct aipu_job_desc - Description of a job to be scheduled.
 * @is_defer_run:      [aipu v1/v2 only, optional] Reserve a core for this job and defer to run
 * @version_compatible:[aipu v1/v2 only, optional] Is this job compatible on different ISA versions
 * @core_id:           [aipu v1/v2/v3 optional] ID of the core to reserve/debug dispatch (v3)
 * @partition_id:      [aipu v3 must] ID of the partition requested to schedule a job onto
 * @do_trigger:        [aipu v1/v2 only, optional] Trigger the deferred job to run
 * @aipu_arch:         [must] Target device architecture
 * @aipu_version:      [must] Target device ISA version
 * @aipu_config:       [aipu v1/v2 only, must] Target device configuration
 * @start_pc_addr:     [aipu v1/v2 only, must] Address of the start PC (buf_pa - asid_base)
 * @intr_handler_addr: [aipu v1/v2 only, must] Address of the interrupt handler (pa - asid_base)
 * @data_0_addr:       [aipu v1/v2 only, must] Address of the 0th data buffer (buf_pa - asid_base)
 * @data_1_addr:       [aipu v1/v2 only, must] Address of the 1th data buffer (buf_pa - asid_base)
 * @job_id:            [aipu v1/v2 only, must] ID of this job
 * @enable_prof:       [aipu v1/v2 only, optional] Enable performance profiling counters in SoC
 * @profile_pa:        [optional] Physical address of the profiler buffer
 * @profile_sz:        [optional] Size of the profiler buffer (should be 0 if no such a buffer)
 * @profile_fd:        [aipu v3 only] Profile data file fd
 * @enable_poll_opt:   [aipu v1/v2 only, optional] Enable optimizations for job status polling
 * @exec_flag:         [optional] Combinations of execution flags
 * @dtcm_size_kb:      [aipu v2(x1)only, optional] DTCM size in KB
 * @head_tcb_pa:       [aipu v3 only, must] base address of the first init TCB of this job
 * @first_task_tcb_pa: [aipu v3 only, must] base address of the first task TCB of this job
 * @last_task_tcb_pa:  [aipu v3 only, must] base address of the last task TCB of this job
 * @tail_tcb_pa:       [aipu v3 only, must] base address of the tail TCB of this job
 * @is_coredump_en:    [aipu v3 and above only, optional] Coredump is enable or not
 *
 * For fields is_defer_run/do_trigger/enable_prof/enable_asid/enable_poll_opt,
 * set them to be 1/0 to enable/disable the corresponding operations.
 */
struct aipu_job_desc {
	__u32 is_defer_run;
	__u32 version_compatible;
	__u32 core_id;
	__u32 partition_id;
	__u32 do_trigger;
	__u32 aipu_arch;
	__u32 aipu_version;
	__u32 aipu_config;
	__u32 start_pc_addr;
	__u32 intr_handler_addr;
	__u32 data_0_addr;
	__u32 data_1_addr;
	__u64 job_id;
	__u32 enable_prof;
	__s64 profile_fd;
	__u64 profile_pa;
	__u32 profile_sz;
	__u32 enable_poll_opt;
	__u32 exec_flag;
	__u32 dtcm_size_kb;
	__u64 head_tcb_pa;
	__u64 first_task_tcb_pa;
	__u64 last_task_tcb_pa;
	__u64 tail_tcb_pa;
	__u32 is_coredump_en;
};

/**
 * struct aipu_job_status_desc - Jod execution status.
 * @job_id:    [kmd] Job ID
 * @thread_id: [kmd] ID of the thread scheduled this job
 * @state:     [kmd] Execution state: done or exception
 * @pdata:     [kmd] External profiling results
 */
struct aipu_job_status_desc {
	__u64 job_id;
	__u32 thread_id;
#define AIPU_JOB_STATE_DONE      0x1
#define AIPU_JOB_STATE_EXCEPTION 0x2
#define AIPU_JOB_STATE_COREDUMP  0x3
	__u32 state;
	struct aipu_ext_profiling_data {
		__u64 tick_counter;      /* [kmd][aipu v3 only] Value of the tick counter */
		__s64 execution_time_ns; /* [kmd] Execution time */
		__u32 rdata_tot_msb;     /* [kmd] Total read transactions (MSB) */
		__u32 rdata_tot_lsb;     /* [kmd] Total read transactions (LSB) */
		__u32 wdata_tot_msb;     /* [kmd] Total write transactions (MSB) */
		__u32 wdata_tot_lsb;     /* [kmd] Total write transactions (LSB) */
		__u32 tot_cycle_msb;     /* [kmd] Total cycle counts (MSB) */
		__u32 tot_cycle_lsb;     /* [kmd] Total cycle counts (LSB) */
	} pdata;
};

/**
 * struct aipu_job_status_query - Query status of (a) job(s) scheduled before.
 * @max_cnt:        [must] Maximum number of job status to query
 * @of_this_thread: [must] Get status of jobs scheduled by this thread/all threads share fd (1/0)
 * @status:         [alloc] Pointer to an array (length is max_cnt) to store the status
 * @poll_cnt:       [kmd] Count of the successfully polled job(s)
 */
struct aipu_job_status_query {
	__u32 max_cnt;
	__u32 of_this_thread;
	struct aipu_job_status_desc *status;
	__u32 poll_cnt;
};

/**
 * struct aipu_io_req - AIPU core IO operations request.
 * @partition_id: 	[must] partition ID, 0 in default.
 * @offset:  		[must] Register offset
 * @rw:      		[must] Read or write operation
 * @value:   		[must]/[kmd] Value to be written/value readback
 */
struct aipu_io_req {
	__u32 partition_id;
	__u32 offset;
	enum aipu_rw_attr {
		AIPU_IO_READ,
		AIPU_IO_WRITE
	} rw;
	__u32 value;
};

/**
 * struct aipu_hw_status - AIPU working status.
 * @status: [kmd] current working status
 */
struct aipu_hw_status {
	enum {
		AIPU_STATUS_IDLE,
		AIPU_STATUS_BUSY,
		AIPU_STATUS_EXCEPTION,
	} status;
};

/**
 * struct aipu_group_id_desc - Group ID descriptor.
 * @group_size: [umd] Size of the group (i.e. number of group IDs)
 * @first_id:   [umd/kmd] The first group ID allocated by KMD or to free
 */
struct aipu_group_id_desc {
	__u16 group_size;
	__u16 first_id;
};

/*
 * AIPU IOCTL List
 */
#define AIPU_IOCTL_MAGIC 'A'
/**
 * DOC: AIPU_IOCTL_QUERY_CAP
 *
 * @Description
 *
 * ioctl to query the common capability of AIPUs
 *
 * User mode driver should call this before calling AIPU_IOCTL_QUERYCORECAP.
 */
#define AIPU_IOCTL_QUERY_CAP _IOR(AIPU_IOCTL_MAGIC, 0, struct aipu_cap)
/**
 * DOC: AIPU_IOCTL_QUERY_PARTITION_CAP
 *
 * @Description
 *
 * ioctl to query the capability of an AIPU partition
 *
 * UMD only need to call this when the partition count returned by AIPU_IOCTL_QUERYCAP > 1.
 */
#define AIPU_IOCTL_QUERY_PARTITION_CAP _IOR(AIPU_IOCTL_MAGIC, 1, struct aipu_partition_cap)
/**
 * DOC: AIPU_IOCTL_REQ_BUF
 *
 * @Description
 *
 * ioctl to request to allocate a coherent buffer
 *
 * This fails if kernel driver cannot find a free buffer meets the size/alignment request.
 */
#define AIPU_IOCTL_REQ_BUF _IOWR(AIPU_IOCTL_MAGIC, 2, struct aipu_buf_request)
/**
 * DOC: AIPU_IOCTL_FREE_BUF
 *
 * @Description
 *
 * ioctl to request to free a coherent buffer allocated by AIPU_IOCTL_REQBUF
 *
 */
#define AIPU_IOCTL_FREE_BUF _IOW(AIPU_IOCTL_MAGIC, 3, struct aipu_buf_desc)
/**
 * DOC: AIPU_IOCTL_DISABLE_SRAM
 *
 * @Description
 *
 * ioctl to disable the management of SoC SRAM in kernel driver
 *
 * This fails if the there is no SRAM in the system or the SRAM has already been allocated.
 */
#define AIPU_IOCTL_DISABLE_SRAM _IO(AIPU_IOCTL_MAGIC, 4)
/**
 * DOC: AIPU_IOCTL_ENABLE_SRAM
 *
 * @Description
 *
 * ioctl to enable the management of SoC SRAM in kernel driver disabled by AIPU_IOCTL_DISABLE_SRAM
 */
#define AIPU_IOCTL_ENABLE_SRAM _IO(AIPU_IOCTL_MAGIC, 5)
/**
 * DOC: AIPU_IOCTL_SCHEDULE_JOB
 *
 * @Description
 *
 * ioctl to schedule a user job to kernel mode driver for execution
 *
 * This is a non-blocking operation therefore user mode driver should check the job status
 * via AIPU_IOCTL_QUERY_STATUS.
 */
#define AIPU_IOCTL_SCHEDULE_JOB _IOW(AIPU_IOCTL_MAGIC, 6, struct aipu_job_desc)
/**
 * DOC: AIPU_IOCTL_QUERY_STATUS
 *
 * @Description
 *
 * ioctl to query the execution status of one or multiple scheduled job(s)
 */
#define AIPU_IOCTL_QUERY_STATUS _IOWR(AIPU_IOCTL_MAGIC, 7, struct aipu_job_status_query)
/**
 * DOC: AIPU_IOCTL_KILL_TIMEOUT_JOB
 *
 * @Description
 *
 * ioctl to kill a timeout job and clean it from kernel mode driver; works for aipu v1/v2 only.
 */
#define AIPU_IOCTL_KILL_TIMEOUT_JOB _IOW(AIPU_IOCTL_MAGIC, 8, __u32)
/**
 * DOC: AIPU_IOCTL_REQ_IO
 *
 * @Description
 *
 * ioctl to read/write an external register of an AIPU core; works for aipu v1/v2 only.
 */
#define AIPU_IOCTL_REQ_IO _IOWR(AIPU_IOCTL_MAGIC, 9, struct aipu_io_req)
/**
 * DOC: AIPU_IOCTL_GET_HW_STATUS
 *
 * @Description
 *
 * ioctl to the hardware status: idle or busy.
 */
#define AIPU_IOCTL_GET_HW_STATUS _IOR(AIPU_IOCTL_MAGIC, 10, struct aipu_hw_status)
/**
 * DOC: AIPU_IOCTL_ABORT_CMD_POOL
 *
 * @Description
 *
 * ioctl to issue a command pool abortion command from userspace.
 * this ioctl shall only be applied in a NPU debugger application.
 */
#define AIPU_IOCTL_ABORT_CMD_POOL _IO(AIPU_IOCTL_MAGIC, 11)
/**
 * DOC: AIPU_IOCTL_DISABLE_TICK_COUNTER
 *
 * @Description
 *
 * ioctl to disable aipu v3 tick counter
 *
 */
#define AIPU_IOCTL_DISABLE_TICK_COUNTER _IO(AIPU_IOCTL_MAGIC, 12)
/**
 * DOC: AIPU_IOCTL_ENABLE_TICK_COUNTER
 *
 * @Description
 *
 * ioctl to enable aipu v3 tick counter
 *
 * by default, tick counter is disabled.
 */
#define AIPU_IOCTL_ENABLE_TICK_COUNTER _IO(AIPU_IOCTL_MAGIC, 13)
/**
 * DOC: AIPU_IOCTL_CONFIG_CLUSTERS
 *
 * @Description
 *
 * ioctl to config the v3 clusters
 *
 * please configure the clusters when they are idle.
 */
#define AIPU_IOCTL_CONFIG_CLUSTERS _IOW(AIPU_IOCTL_MAGIC, 14, struct aipu_config_clusters)
/**
 * DOC: AIPU_IOCTL_ALLOC_DMA_BUF
 *
 * @Description
 *
 * ioctl to allocate a buffer and return the corresponding dma-buf fd
 *   aipu_dma_buf_request->bytes: filled by UMD
 *   aipu_dma_buf_request->fd:    filled by KMD
 *
 */
#define AIPU_IOCTL_ALLOC_DMA_BUF _IOR(AIPU_IOCTL_MAGIC, 15, struct aipu_dma_buf_request)
/**
 * DOC: AIPU_IOCTL_FREE_DMA_BUF
 *
 * @Description
 *
 * ioctl to free a buffer related to a dma-buf fd
 */
#define AIPU_IOCTL_FREE_DMA_BUF _IOW(AIPU_IOCTL_MAGIC, 16, int)
/**
 * DOC: AIPU_IOCTL_GET_DMA_BUF_INFO
 *
 * @Description
 *
 * ioctl to get the buffer addr and size from a dma-buf fd
 *   aipu_dma_buf->fd:    filled by UMD
 *   aipu_dma_buf->pa:    filled by KMD
 *   aipu_dma_buf->bytes: filled by KMD
 */
#define AIPU_IOCTL_GET_DMA_BUF_INFO _IOWR(AIPU_IOCTL_MAGIC, 17, struct aipu_dma_buf)
/**
 * DOC: AIPU_IOCTL_GET_DRIVER_VERSION
 *
 * @Description
 *
 * ioctl to get the kmd version: major.minor.patch
 * the char buffer size from UMD should be at least 16 characters.
 */
#define AIPU_IOCTL_GET_DRIVER_VERSION _IOR(AIPU_IOCTL_MAGIC, 18, char*)
/**
 * DOC: AIPU_IOCTL_ATTACH_DMA_BUF
 *
 * @Description
 *
 * ioctl to get the buffer addr and size from a dma-buf importer's fd
 *   aipu_dma_buf->fd:    filled by UMD
 *   aipu_dma_buf->pa:    filled by KMD
 *   aipu_dma_buf->bytes: filled by KMD
 */
#define AIPU_IOCTL_ATTACH_DMA_BUF _IOWR(AIPU_IOCTL_MAGIC, 19, struct aipu_dma_buf)
/**
 * DOC: AIPU_IOCTL_DETACH_DMA_BUF
 *
 * @Description
 *
 * ioctl to detach a dma-buf attached before
 *   int fd: filled by UMD
 */
#define AIPU_IOCTL_DETACH_DMA_BUF _IOW(AIPU_IOCTL_MAGIC, 20, int)
/**
 * DOC: AIPU_IOCTL_ALLOC_GRID_ID
 *
 * @Description
 *
 * ioctl to get a unique grid ID (do not need to free)
 */
#define AIPU_IOCTL_ALLOC_GRID_ID _IOR(AIPU_IOCTL_MAGIC, 21, int)
/**
 * DOC: AIPU_IOCTL_ALLOC_GROUP_ID
 *
 * @Description
 *
 * ioctl to get a group of continuous unique group IDs
 *   aipu_group_id_desc->group_size: filled by UMD
 *   aipu_group_id_desc->first_id:   filled by KMD
 */
#define AIPU_IOCTL_ALLOC_GROUP_ID _IOWR(AIPU_IOCTL_MAGIC, 22, struct aipu_group_id_desc)
/**
 * DOC: AIPU_IOCTL_FREE_GROUP_ID
 *
 * @Description
 *
 * ioctl to free a group of allocated group IDs
 *   aipu_group_id_desc->group_size: filled by UMD
 *   aipu_group_id_desc->first_id:   filled by UMD
 */
#define AIPU_IOCTL_FREE_GROUP_ID _IOW(AIPU_IOCTL_MAGIC, 23, struct aipu_group_id_desc)

/**
 * DOC: AIPU_IOCTL_BUF_CACHE_INVALID
 *
 * @Description
 *
 * ioctl to request to invalid a coherent buffer allocated by AIPU_IOCTL_REQBUF
 *
 */
#define AIPU_IOCTL_BUF_CACHE_INVALID _IOW(AIPU_IOCTL_MAGIC, 24, struct aipu_buf_desc)

/**
 * DOC: AIPU_IOCTL_BUF_CACHE_FLUSH
 *
 * @Description
 *
 * ioctl to request to invalid a coherent buffer allocated by AIPU_IOCTL_REQBUF
 *
 */
#define AIPU_IOCTL_BUF_CACHE_FLUSH _IOW(AIPU_IOCTL_MAGIC, 25, struct aipu_buf_desc)

#endif /* __UAPI_MISC_ARMCHINA_AIPU_H__ */
