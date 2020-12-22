/*
 * This confidential and proprietary software may be used only as
 * authorised by a licensing agreement from ARM Limited
 * (C) COPYRIGHT 2011-2012 ARM Limited
 * ALL RIGHTS RESERVED
 * The entire notice above must be reproduced on all authorised
 * copies and copies may only be made to the extent permitted
 * by a licensing agreement from ARM Limited.
 */

#include "gator.h"

#include <linux/module.h>
#include <linux/time.h>
#include <linux/math64.h>
#include <linux/slab.h>
#include <asm/io.h>

/* Mali T6xx DDK includes */
#include "linux/mali_linux_trace.h"
#include "kbase/src/common/mali_kbase.h"
#include "kbase/src/linux/mali_kbase_mem_linux.h"

#include "gator_events_mali_common.h"

/* Blocks for HW counters */
enum
{
    JM_BLOCK = 0,
    TILER_BLOCK,
    SHADER_BLOCK,
    MMU_BLOCK
};

/* Counters for Mali-T6xx:
 *
 *  - HW counters, 4 blocks
 *    For HW counters we need strings to create /dev/gator/events files.
 *    Enums are not needed because the position of the HW name in the array is the same
 *    of the corresponding value in the received block of memory.
 *    HW counters are requested by calculating a bitmask, passed then to the driver.
 *    Every millisecond a HW counters dump is requested, and if the previous has been completed they are read.
 */

/* Hardware Counters */
static const char* const hardware_counter_names [] =
{
    /* Job Manager */
    "",
    "",
    "",
    "",
    "MESSAGES_SENT",
    "MESSAGES_RECEIVED",
    "GPU_ACTIVE",            /* 6 */
    "IRQ_ACTIVE",
    "JS0_JOBS",
    "JS0_TASKS",
    "JS0_ACTIVE",
    "",
    "JS0_WAIT_READ",
    "JS0_WAIT_ISSUE",
    "JS0_WAIT_DEPEND",
    "JS0_WAIT_FINISH",
    "JS1_JOBS",
    "JS1_TASKS",
    "JS1_ACTIVE",
    "",
    "JS1_WAIT_READ",
    "JS1_WAIT_ISSUE",
    "JS1_WAIT_DEPEND",
    "JS1_WAIT_FINISH",
    "JS2_JOBS",
    "JS2_TASKS",
    "JS2_ACTIVE",
    "",
    "JS2_WAIT_READ",
    "JS2_WAIT_ISSUE",
    "JS2_WAIT_DEPEND",
    "JS2_WAIT_FINISH",
    "JS3_JOBS",
    "JS3_TASKS",
    "JS3_ACTIVE",
    "",
    "JS3_WAIT_READ",
    "JS3_WAIT_ISSUE",
    "JS3_WAIT_DEPEND",
    "JS3_WAIT_FINISH",
    "JS4_JOBS",
    "JS4_TASKS",
    "JS4_ACTIVE",
    "",
    "JS4_WAIT_READ",
    "JS4_WAIT_ISSUE",
    "JS4_WAIT_DEPEND",
    "JS4_WAIT_FINISH",
    "JS5_JOBS",
    "JS5_TASKS",
    "JS5_ACTIVE",
    "",
    "JS5_WAIT_READ",
    "JS5_WAIT_ISSUE",
    "JS5_WAIT_DEPEND",
    "JS5_WAIT_FINISH",
    "JS6_JOBS",
    "JS6_TASKS",
    "JS6_ACTIVE",
    "",
    "JS6_WAIT_READ",
    "JS6_WAIT_ISSUE",
    "JS6_WAIT_DEPEND",
    "JS6_WAIT_FINISH",

    /*Tiler*/
    "",
    "",
    "",
    "JOBS_PROCESSED",
    "TRIANGLES",
    "QUADS",
    "POLYGONS",
    "POINTS",
    "LINES",
    "VCACHE_HIT",
    "VCACHE_MISS",
    "FRONT_FACING",
    "BACK_FACING",
    "PRIM_VISIBLE",
    "PRIM_CULLED",
    "PRIM_CLIPPED",
    "LEVEL0",
    "LEVEL1",
    "LEVEL2",
    "LEVEL3",
    "LEVEL4",
    "LEVEL5",
    "LEVEL6",
    "LEVEL7",
    "COMMAND_1",
    "COMMAND_2",
    "COMMAND_3",
    "COMMAND_4",
    "COMMAND_4_7",
    "COMMAND_8_15",
    "COMMAND_16_63",
    "COMMAND_64",
    "COMPRESS_IN",
    "COMPRESS_OUT",
    "COMPRESS_FLUSH",
    "TIMESTAMPS",
    "PCACHE_HIT",
    "PCACHE_MISS",
    "PCACHE_LINE",
    "PCACHE_STALL",
    "WRBUF_HIT",
    "WRBUF_MISS",
    "WRBUF_LINE",
    "WRBUF_PARTIAL",
    "WRBUF_STALL",
    "ACTIVE",
    "LOADING_DESC",
    "INDEX_WAIT",
    "INDEX_RANGE_WAIT",
    "VERTEX_WAIT",
    "PCACHE_WAIT",
    "WRBUF_WAIT",
    "BUS_READ",
    "BUS_WRITE",
    "",
    "",
    "",
    "",
    "",
    "UTLB_STALL",
    "UTLB_REPLAY_MISS",
    "UTLB_REPLAY_FULL",
    "UTLB_NEW_MISS",
    "UTLB_HIT",

    /* Shader Core */
    "",
    "",
    "",
    "SHADER_CORE_ACTIVE",
    "FRAG_ACTIVE",
    "FRAG_PRIMATIVES",
    "FRAG_PRIMATIVES_DROPPED",
    "FRAG_CYCLE_DESC",
    "FRAG_CYCLES_PLR",
    "FRAG_CYCLES_VERT",
    "FRAG_CYCLES_TRISETUP",
    "FRAG_CYCLES_RAST",
    "FRAG_THREADS",
    "FRAG_DUMMY_THREADS",
    "FRAG_QUADS_RAST",
    "FRAG_QUADS_EZS_TEST",
    "FRAG_QUADS_EZS_KILLED",
    "FRAG_QUADS_LZS_TEST",
    "FRAG_QUADS_LZS_KILLED",
    "FRAG_CYCLE_NO_TILE",
    "FRAG_NUM_TILES",
    "FRAG_TRANS_ELIM",
    "COMPUTE_ACTIVE",
    "COMPUTE_TASKS",
    "COMPUTE_THREADS",
    "COMPUTE_CYCLES_DESC",
    "TRIPIPE_ACTIVE",
    "ARITH_WORDS",
    "ARITH_CYCLES_REG",
    "ARITH_CYCLES_L0",
    "ARITH_FRAG_DEPEND",
    "LS_WORDS",
    "LS_ISSUES",
    "LS_RESTARTS",
    "LS_REISSUES_MISS",
    "LS_REISSUES_VD",
    "LS_REISSUE_ATTRIB_MISS",
    "LS_NO_WB",
    "TEX_WORDS",
    "TEX_BUBBLES",
    "TEX_WORDS_L0",
    "TEX_WORDS_DESC",
    "TEX_THREADS",
    "TEX_RECIRC_FMISS",
    "TEX_RECIRC_DESC",
    "TEX_RECIRC_MULTI",
    "TEX_RECIRC_PMISS",
    "TEX_RECIRC_CONF",
    "LSC_READ_HITS",
    "LSC_READ_MISSES",
    "LSC_WRITE_HITS",
    "LSC_WRITE_MISSES",
    "LSC_ATOMIC_HITS",
    "LSC_ATOMIC_MISSES",
    "LSC_LINE_FETCHES",
    "LSC_DIRTY_LINE",
    "LSC_SNOOPS",
    "AXI_TLB_STALL",
    "AXI_TLB_MIESS",
    "AXI_TLB_TRANSACTION",
    "LS_TLB_MISS",
    "LS_TLB_HIT",
    "AXI_BEATS_READ",
    "AXI_BEATS_WRITTEN",

    /*L2 and MMU */
    "",
    "",
    "",
    "",
    "MMU_TABLE_WALK",
    "MMU_REPLAY_MISS",
    "MMU_REPLAY_FULL",
    "MMU_NEW_MISS",
    "MMU_HIT",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "UTLB_STALL",
    "UTLB_REPLAY_MISS",
    "UTLB_REPLAY_FULL",
    "UTLB_NEW_MISS",
    "UTLB_HIT",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "L2_WRITE_BEATS",
    "L2_READ_BEATS",
    "L2_ANY_LOOKUP",
    "L2_READ_LOOKUP",
    "L2_SREAD_LOOKUP",
    "L2_READ_REPLAY",
    "L2_READ_SNOOP",
    "L2_READ_HIT",
    "L2_CLEAN_MISS",
    "L2_WRITE_LOOKUP",
    "L2_SWRITE_LOOKUP",
    "L2_WRITE_REPLAY",
    "L2_WRITE_SNOOP",
    "L2_WRITE_HIT",
    "L2_EXT_READ_FULL",
    "L2_EXT_READ_HALF",
    "L2_EXT_WRITE_FULL",
    "L2_EXT_WRITE_HALF",
    "L2_EXT_READ",
    "L2_EXT_READ_LINE",
    "L2_EXT_WRITE",
    "L2_EXT_WRITE_LINE",
    "L2_EXT_WRITE_SMALL",
    "L2_EXT_BARRIER",
    "L2_EXT_AR_STALL",
    "L2_EXT_R_BUF_FULL",
    "L2_EXT_RD_BUF_FULL",
    "L2_EXT_R_RAW",
    "L2_EXT_W_STALL",
    "L2_EXT_W_BUF_FULL",
    "L2_EXT_R_W_HAZARD",
    "L2_TAG_HAZARD",
    "L2_SNOOP_FULL",
    "L2_REPLAY_FULL"
};

#define NUMBER_OF_HARDWARE_COUNTERS (sizeof(hardware_counter_names) / sizeof(hardware_counter_names[0]))

#define GET_HW_BLOCK(c) (((c) >> 6) & 0x3)
#define GET_COUNTER_OFFSET(c) ((c) & 0x3f)

/* Memory to dump hardware counters into */
static void *kernel_dump_buffer;

/* kbase context and device */
static kbase_context *kbcontext = NULL;
static struct kbase_device *kbdevice = NULL;

extern struct kbase_device *kbase_find_device(int minor);
static volatile bool kbase_device_busy = false;
static unsigned int num_hardware_counters_enabled;

/*
 * gatorfs variables for counter enable state
 */
static mali_counter counters[NUMBER_OF_HARDWARE_COUNTERS];

/* An array used to return the data we recorded
 * as key,value pairs hence the *2
 */
static unsigned long counter_dump[NUMBER_OF_HARDWARE_COUNTERS * 2];

static int start(void)
{
    kbase_uk_hwcnt_setup setup;
    mali_error err;
    int cnt;
    u16 bitmask[] = {0, 0, 0, 0};

    /* Setup HW counters */
    num_hardware_counters_enabled = 0;

    if(NUMBER_OF_HARDWARE_COUNTERS != 256)
    {
        pr_debug("Unexpected number of hardware counters defined: expecting 256, got %d\n", NUMBER_OF_HARDWARE_COUNTERS);
    }

    /* Calculate enable bitmasks based on counters_enabled array */
    for (cnt = 0; cnt < NUMBER_OF_HARDWARE_COUNTERS; cnt++)
    {
        const mali_counter *counter = &counters[cnt];
        if (counter->enabled)
        {
            int block = GET_HW_BLOCK(cnt);
            int enable_bit = GET_COUNTER_OFFSET(cnt) / 4;
            bitmask[block] |= (1 << enable_bit);
            pr_debug("gator: Mali-T6xx: hardware counter %s selected [%d]\n", hardware_counter_names[cnt], cnt);
            num_hardware_counters_enabled++;
        }
    }

    /* Create a kbase context for HW counters */
    if (num_hardware_counters_enabled > 0)
    {
        kbdevice = kbase_find_device(-1);

        if (kbcontext)
            return -1;

        kbcontext = kbase_create_context(kbdevice);
        if (!kbcontext)
        {
            pr_debug("gator: Mali-T6xx: error creating kbase context\n");
            goto out;
        }

        /*
         * The amount of memory needed to store the dump (bytes)
         * DUMP_SIZE = number of core groups
         *             * number of blocks (always 8 for midgard)
         *             * number of counters per block (always 64 for midgard)
         *             * number of bytes per counter (always 4 in midgard)
         * For a Mali-T6xx with a single core group = 1 * 8 * 64 * 4
         */
        kernel_dump_buffer = kbase_va_alloc(kbcontext, 2048);
        if (!kernel_dump_buffer)
        {
            pr_debug("gator: Mali-T6xx: error trying to allocate va\n");
            goto destroy_context;
        }

        setup.dump_buffer = (uintptr_t)kernel_dump_buffer;
        setup.jm_bm = bitmask[JM_BLOCK];
        setup.tiler_bm = bitmask[TILER_BLOCK];
        setup.shader_bm = bitmask[SHADER_BLOCK];
        setup.mmu_l2_bm = bitmask[MMU_BLOCK];
        /* These counters do not exist on Mali-T60x */
        setup.l3_cache_bm = 0;

        /* Use kbase API to enable hardware counters and provide dump buffer */
        err = kbase_instr_hwcnt_enable(kbcontext, &setup);
        if (err != MALI_ERROR_NONE)
        {
            pr_debug("gator: Mali-T6xx: can't setup hardware counters\n");
            goto free_buffer;
        }
        pr_debug("gator: Mali-T6xx: hardware counters enabled\n");
        kbase_instr_hwcnt_clear(kbcontext);
        pr_debug("gator: Mali-T6xx: hardware counters cleared \n");

        kbase_device_busy = false;
    }

    return 0;

    free_buffer:
        kbase_va_free(kbcontext, kernel_dump_buffer);
    destroy_context:
        kbase_destroy_context(kbcontext);
    out:
        return -1;
}

static void stop(void) {
    unsigned int cnt;
    kbase_context *temp_kbcontext;

    pr_debug("gator: Mali-T6xx: stop\n");

    /* Set all counters as disabled */
    for (cnt = 0; cnt < NUMBER_OF_HARDWARE_COUNTERS; cnt++) {
        counters[cnt].enabled = 0;
    }

    /* Destroy the context for HW counters */
    if (num_hardware_counters_enabled > 0 && kbcontext != NULL)
    {
        /*
         * Set the global variable to NULL before destroying it, because
         * other function will check this before using it.
         */
        temp_kbcontext = kbcontext;
        kbcontext = NULL;

        kbase_instr_hwcnt_disable(temp_kbcontext);
        kbase_va_free(temp_kbcontext, kernel_dump_buffer);
        kbase_destroy_context(temp_kbcontext);
        pr_debug("gator: Mali-T6xx: hardware counters stopped\n");
    }
}

static int read(int **buffer) {
    int cnt;
    int len = 0;
    u32 value = 0;
    mali_bool success;

    if (smp_processor_id()!=0)
    {
        return 0;
    }

    /*
     * Report the HW counters
     * Only process hardware counters if at least one of the hardware counters is enabled.
     */
    if (num_hardware_counters_enabled > 0)
    {
        const unsigned int vithar_blocks[] = {
            0x700,    /* VITHAR_JOB_MANAGER,     Block 0 */
            0x400,    /* VITHAR_TILER,           Block 1 */
            0x000,    /* VITHAR_SHADER_CORE,     Block 2 */
            0x500     /* VITHAR_MEMORY_SYSTEM,   Block 3 */
        };

        if (!kbcontext)
        {
            return -1;
        }

        // TODO: SYMBOL_GET (all kbase functions)
        if (kbase_instr_hwcnt_dump_complete(kbcontext, &success) == MALI_TRUE)
        {
            kbase_device_busy = false;

            if (success == MALI_TRUE)
            {
                for (cnt = 0; cnt < NUMBER_OF_HARDWARE_COUNTERS; cnt++)
                {
                    const mali_counter *counter = &counters[cnt];
                    if (counter->enabled)
                    {
                        const int block = GET_HW_BLOCK(cnt);
                        const int counter_offset = GET_COUNTER_OFFSET(cnt);
                        const u32 *counter_block = (u32 *)((uintptr_t)kernel_dump_buffer + vithar_blocks[block]);
                        const u32 *counter_address = counter_block + counter_offset;

                        value = *counter_address;

                        if(block == SHADER_BLOCK){
                            /* (counter_address + 0x000) has already been accounted-for above. */
                            value += *(counter_address + 0x100);
                            value += *(counter_address + 0x200);
                            value += *(counter_address + 0x300);
                        }

                        counter_dump[len++] = counter->key;
                        counter_dump[len++] = value;
                    }
                }
            }
        }

        if (! kbase_device_busy)
        {
            kbase_device_busy = true;
            kbase_instr_hwcnt_dump_irq(kbcontext);
        }
    }

    /* Update the buffer */
    if (buffer) {
        *buffer = (int*) counter_dump;
    }

    return len;
}

static int create_files(struct super_block *sb, struct dentry *root)
{
    unsigned int event;
    /*
     * Create the filesystem for all events
     */
    int counter_index = 0;
    const char* mali_name = gator_mali_get_mali_name();

    for (event = 0; event < NUMBER_OF_HARDWARE_COUNTERS; event++)
    {
        if (gator_mali_create_file_system(mali_name, hardware_counter_names[counter_index], sb, root, &counters[event]) != 0)
            return -1;
        counter_index++;
    }

    return 0;
}


static struct gator_interface gator_events_mali_t6xx_interface = {
    .create_files    = create_files,
    .start        = start,
    .stop        = stop,
    .read        = read
};

int gator_events_mali_t6xx_hw_init(void)
{
    pr_debug("gator: Mali-T6xx: sw_counters init\n");

    gator_mali_initialise_counters(counters, NUMBER_OF_HARDWARE_COUNTERS);

    return gator_events_install(&gator_events_mali_t6xx_interface);
}

gator_events_init(gator_events_mali_t6xx_hw_init);
