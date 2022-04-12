/**
 ****************************************************************************************
 *
 * @file bl_utils.c
 *
 * @brief IPC utility function definitions
 *
 * Copyright (C) BouffaloLab 2017-2018
 *
 ****************************************************************************************
 */
#include <linux/device.h>
#include <linux/dmapool.h>
#include <linux/module.h>
#include <linux/types.h>

#include "bl_utils.h"
#include "bl_defs.h"
#include "bl_rx.h"
#include "bl_tx.h"
#include "bl_msg_rx.h"
#include "bl_debugfs.h"

#ifdef CONFIG_BL_MOD_LEV_DBG
unsigned long bl_filter_severity = 0xffffffff;
unsigned long bl_filter_module   = 0x0;
#else
unsigned long bl_filter_severity;
unsigned long bl_filter_module;
#endif

struct device_attribute dev_attr_filter_severity;
EXPORT_SYMBOL_GPL(dev_attr_filter_severity);
struct device_attribute dev_attr_filter_module;
EXPORT_SYMBOL_GPL(dev_attr_filter_module);

module_param(bl_filter_severity, ulong, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(bl_filter_severity, "used to filter severity");
module_param(bl_filter_module, ulong, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(bl_filter_module, "used to filter module");

static ssize_t filter_severity_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%lx\n", bl_filter_severity);
}

static ssize_t filter_severity_store(struct device *dev,
				 struct device_attribute *attr, const char *buf,
				 size_t count)
{
	if (sscanf(buf, "%lx", &bl_filter_severity) != 1)
		return -EINVAL;
	return count;
}
DEVICE_ATTR_RW(filter_severity);

static ssize_t filter_module_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%lx\n", bl_filter_module);
}

static ssize_t filter_module_store(struct device *dev,
				 struct device_attribute *attr, const char *buf,
				 size_t count)
{
	if (sscanf(buf, "%lx", &bl_filter_module) != 1)
		return -EINVAL;
	return count;
}
DEVICE_ATTR_RW(filter_module);

void log_buf_output(struct bl_hw *bl_hw, const char* buf, size_t size) {
    size_t write_size = 0, write_index = 0;
	//char tmp_buf[256] = {0};

    while (true) {
        if (bl_hw->buf_write_size + size > LOG_BUF_OUTPUT_BUF_SIZE) {
            write_size = LOG_BUF_OUTPUT_BUF_SIZE - bl_hw->buf_write_size;
            memcpy(bl_hw->log_buff + bl_hw->buf_write_size, buf + write_index, write_size);
			//snprintf(tmp_buf, write_size, "%s", buf + write_index);
            write_index += write_size;
            size -= write_size;
            /* reset write index */
            bl_hw->buf_write_size = 0;
        } else {
            memcpy(bl_hw->log_buff + bl_hw->buf_write_size, buf + write_index, size);
            bl_hw->buf_write_size += size;
            break;
        }
    }
}

size_t bl_log_strcpy(size_t cur_len, char *dst, const char *src) {
    const char *src_old = src;

    ASSERT_ERR(dst);
    ASSERT_ERR(src);

    while (*src != 0) {
        /* make sure destination has enough space */
        if (cur_len++ < LOG_LINE_BUF_SIZE) {
            *dst++ = *src++;
        } else {
            break;
        }
    }
    return src - src_old;
}

#define LOG_LINE_NUM_MAX_LEN 5
static void bl_do_log(struct bl_hw *bl_hw, const char *func, const long line, const char *fmt_usr, va_list args)
{
	static char buf[LOG_LINE_BUF_SIZE];
	char line_num[LOG_LINE_NUM_MAX_LEN + 1] = { 0 };
	int log_len = 0;

	snprintf(line_num, LOG_LINE_NUM_MAX_LEN, "%ld", line);
	log_len += bl_log_strcpy(log_len, buf + log_len, line_num);
	log_len += bl_log_strcpy(log_len, buf + log_len, ":");
	log_len += bl_log_strcpy(log_len, buf + log_len, func);
	log_len += bl_log_strcpy(log_len, buf + log_len, ":");

	log_len += vsnprintf(buf + log_len, LOG_LINE_BUF_SIZE - log_len, fmt_usr, args);
	log_buf_output(bl_hw, buf, log_len);
	//printk("%s", buf);
}

/**
 ****************************************************************************************
 * @brief Function formatting a string and sending it to the defined output
 *
 * @param[in] fmt Format string
 *
 ****************************************************************************************
 */
void dbg_test_print(struct bl_hw *bl_hw, const char *func, const long line, const char *fmt, ...)
{
    const char *fmt_usr = (const char*) fmt;
    uint32_t severity = 0;
    va_list args;
    va_start(args, fmt);

    //printk("%d %d %d %d\n", fmt_usr[0], fmt_usr[1], DBG_MOD_MAX, DBG_SEV_MIN);

    // permit all the debug message is permited
    bl_filter_severity = DBG_SEV_MAX;

    if (bl_filter_severity == 0) return;

    do
    {
        // Get the prefix
        unsigned char prefix = ((unsigned char)*fmt_usr) & 0xFF;

        // ASCII char, start of the user string
        if (prefix < DBG_MOD_MIN) break;

        if (prefix < DBG_MOD_MAX)
        {
            // test module, if filtered returns
            if (~bl_filter_module & CO_BIT(prefix - DBG_MOD_MIN)) return;
        }
        else
        {
            // must be severity code
            ASSERT_ERR(DBG_SEV_MIN <= prefix && prefix < DBG_SEV_MAX);
            severity = (uint32_t)(prefix - DBG_SEV_MIN);

            // test severity, if filtered returns
            if (bl_filter_severity <= severity) return;
        }

        // Check first and second char
        fmt_usr++;
    }
    while (fmt_usr != fmt + 2);

    // print
	bl_do_log(bl_hw, func, line, fmt_usr, args);

    va_end(args);
}


/**
 *
 */
static int dbginfo_allocs(struct bl_hw *bl_hw)
{
    struct dbg_debug_dump_tag *buf;
    u32 len = sizeof(*buf);

    /* Allocate the debug information buffer */
    buf = kmalloc(len, GFP_KERNEL);

    if (!buf) {
        printk(KERN_CRIT "%s:%d: buffer alloc of size %u failed\n\n",
               __func__, __LINE__, len);
        return -ENOMEM;
    }
    bl_hw->dbginfo.buf = buf;
	
    return 0;
}

/**
 *
 */
static void bl_dbginfo_deallocs(struct bl_hw *bl_hw)
{
    if (bl_hw->dbginfo.buf) {
        kfree(bl_hw->dbginfo.buf);
        bl_hw->dbginfo.buf = NULL;
    }
}


/**
 * @brief Deallocate storage elements.
 *
 *  This function deallocates all the elements required for communications with LMAC,
 *  such as Rx Data elements, MSGs elements, ...
 *
 * This function should be called in correspondence with the allocation function.
 *
 * @param[in]   bl_hw   Pointer to main structure storing all the relevant information
 */
static void bl_elems_deallocs(struct bl_hw *bl_hw)
{
    BL_DBG(BL_FN_ENTRY_STR);

    bl_dbginfo_deallocs(bl_hw);
}

/**
 * @brief Allocate storage elements.
 *
 *  This function allocates all the elements required for communications with LMAC,
 *  such as Rx Data elements, MSGs elements, ...
 *
 * This function should be called in correspondence with the deallocation function.
 *
 * @param[in]   bl_hw   Pointer to main structure storing all the relevant information
 */
static int bl_elems_allocs(struct bl_hw *bl_hw)
{
	BL_DBG(BL_FN_ENTRY_STR);
    /* Initialize the debug information buffer */
    if (dbginfo_allocs(bl_hw)) {
        printk(KERN_CRIT "%s:%d: ALLOCATIONS FAILED !\n", __func__,
                 __LINE__);
        goto err_alloc;
    }

    return 0;

err_alloc:
    bl_elems_deallocs(bl_hw);
    return -ENOMEM;
}

/**
 * WLAN driver call-back function for message reception indication
 */
u8 bl_msgind(void *pthis, void *hostid)
{
    u8 ret = 0;
    struct bl_hw *bl_hw = (struct bl_hw *)pthis;
    struct ipc_e2a_msg *msg = (struct ipc_e2a_msg *)hostid;

	BL_DBG(BL_FN_ENTRY_STR);

    /* Relay further actions to the msg parser */
    bl_rx_handle_msg(bl_hw, msg);

    return ret;
}

/**
 * FIXME
 *
 */
u8 bl_msgackind(void *pthis, void *hostid)
{
    struct bl_hw *bl_hw = (struct bl_hw *)pthis;

    bl_hw->cmd_mgr.llind(&bl_hw->cmd_mgr, (struct bl_cmd *)hostid);

    return -1;
}

/**
 * WLAN driver call-back function for primary TBTT indication
 */
void bl_prim_tbtt_ind(void *pthis)
{
}

/**
 * WLAN driver call-back function for secondary TBTT indication
 */
void bl_sec_tbtt_ind(void *pthis)
{
    /* TODO */
}

/**
 * WLAN driver call-back function for Debug message reception indication
 */
u8 bl_dbgind(void *pthis, void *hostid)
{
    struct bl_dbg_elem *dbg_elem = hostid;
    struct ipc_dbg_msg *dbg_msg;
    u8 ret = 0;

    /* Retrieve the message structure */
    dbg_msg = (struct ipc_dbg_msg *)dbg_elem->dbgbuf_ptr;

    /* Look for pattern which means that this hostbuf has been used for a MSG */
    if (dbg_msg->pattern != IPC_DBG_VALID_PATTERN) {
        ret = -1;
        goto dbg_no_push;
    }

    /* Display the LMAC string */
    printk("lmac %s", (char *)dbg_msg->string);

    /* Reset the msg element and re-use it */
    dbg_msg->pattern = 0;
    wmb();

dbg_no_push:
    return ret;
}

int bl_ipc_init(struct bl_hw *bl_hw)
{
    struct ipc_host_cb_tag cb;

    BL_DBG(BL_FN_ENTRY_STR);

    /* initialize the API interface */
    cb.recv_data_ind   = bl_rxdataind;
    cb.recv_msg_ind    = bl_msgind;
    cb.recv_msgack_ind = bl_msgackind;
    cb.recv_dbg_ind    = bl_dbgind;
    cb.send_data_cfm   = bl_txdatacfm;
    cb.prim_tbtt_ind   = bl_prim_tbtt_ind;
    cb.sec_tbtt_ind    = bl_sec_tbtt_ind;

    /* set the IPC environment */
    bl_hw->ipc_env = (struct ipc_host_env_tag *)
                       kzalloc(sizeof(struct ipc_host_env_tag), GFP_KERNEL);

    /* call the initialization of the IPC */
    ipc_host_init(bl_hw->ipc_env, &cb, bl_hw);

    bl_cmd_mgr_init(&bl_hw->cmd_mgr);

    return bl_elems_allocs(bl_hw);
}

/**
 *
 */
void bl_ipc_deinit(struct bl_hw *bl_hw)
{
    BL_DBG(BL_FN_ENTRY_STR);

    bl_ipc_tx_drain(bl_hw);
    bl_cmd_mgr_deinit(&bl_hw->cmd_mgr);
    bl_elems_deallocs(bl_hw);
    kfree(bl_hw->ipc_env);
    bl_hw->ipc_env = NULL;
}

/**
 * This assumes LMAC is still (tx wise) and there's no TX race until LMAC is up
 * tx wise.
 * This also lets both IPC sides remain in sync before resetting the LMAC,
 * e.g with bl_send_reset.
 */
void bl_ipc_tx_drain(struct bl_hw *bl_hw)
{
    int i, j;

    BL_DBG(BL_FN_ENTRY_STR);

    if (!bl_hw->ipc_env) {
        printk("%s: bypassing (restart must have failed)\n", __func__);
        return;
    }

    for (i = 0; i < BL_HWQ_NB; i++) {
        for (j = 0; j < nx_txuser_cnt[i]; j++) {
            struct sk_buff *skb;
            while ((skb = (struct sk_buff *)ipc_host_tx_flush(bl_hw->ipc_env, i, j))) {
                struct bl_sw_txhdr *sw_txhdr =
                    ((struct bl_txhdr *)skb->data)->sw_hdr;
                skb_pull(skb, sw_txhdr->headroom);
                dev_kfree_skb_any(skb);
            }
        }
    }
}

void bl_error_ind(struct bl_hw *bl_hw)
{
    printk(KERN_CRIT "%s (type %d): dump received\n", __func__,
           bl_hw->dbginfo.buf->dbg_info.error_type);
    bl_hw->debugfs.trace_prst = true;
}

