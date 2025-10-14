#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/init.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/soc/cix/cix_ap2se_ipc.h>

#define KS			     32
#define SE_CONFIG_NODE		     "auto_clock_gating"

static struct proc_dir_entry *se_config_proc;
static struct proc_dir_entry *cix_sky1_root;


static char proc_gating_string[KS];
static char proc_gating_string_param[KS];

static ssize_t
hw_auto_clk_gating_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
	int nbytes = sprintf(proc_gating_string_param, "%s\n", proc_gating_string);
        return simple_read_from_buffer(buf, count, f_pos, proc_gating_string_param, nbytes);
}

static ssize_t hw_auto_clk_gating_write(struct file *filp, const char __user *buf, size_t count,
                        loff_t *f_pos)
{
	ssize_t rc;
	int err = 0;
        rc = simple_write_to_buffer(proc_gating_string_param, count, f_pos, buf, count);
        sscanf(proc_gating_string_param, "%s", proc_gating_string);

	if (!strcmp(proc_gating_string, "enable")) {
		// Send clk auto enable msg
		err = cix_ap2se_ipc_send(FFA_CLK_AUTO_GATING_ENABLE, NULL, 0, 0);
		if (err < 0) {
			pr_err("auto clk_gating enbale mbox_send_message failed: %d\n", err);
		}

	} else if (!strcmp(proc_gating_string, "disable")) {
		// Send clk auto enable msg
		err = cix_ap2se_ipc_send(FFA_CLK_AUTO_GATING_DISABLE, NULL, 0, 0);
		if (err < 0) {
			pr_err("auto clk_gating disable mbox_send_message failed: %d\n", err);
		}
	} else {
		pr_err("proc_gating_string = %s error msg, please input enable/disable!\n", proc_gating_string);
	}

        return rc;
}

static const struct proc_ops hw_auto_clk_gating_fops = {
        .proc_read = hw_auto_clk_gating_read,
        .proc_write = hw_auto_clk_gating_write,
};

static int cix_sky1_se_config_init(void)
{

        cix_sky1_root = proc_mkdir("cix_sky1", NULL);
        if (IS_ERR(cix_sky1_root)){
                pr_err("failed to make cix_sky1 dir\n");
                return -1;
        }

        se_config_proc = proc_create(SE_CONFIG_NODE, 0, cix_sky1_root, &hw_auto_clk_gating_fops);
        if (IS_ERR(se_config_proc)){
                pr_err("failed to make %s", SE_CONFIG_NODE);
                return -1;
        }
        pr_info("%s Created %s\n", __func__, SE_CONFIG_NODE);
        return 0;
}

static void cix_sky1_se_config_exit(void)
{
        if (cix_sky1_root) {
                proc_remove(se_config_proc);
                proc_remove(cix_sky1_root);
                pr_info("Removed auto_clk_gating\n");
        }
}

module_init(cix_sky1_se_config_init);
module_exit(cix_sky1_se_config_exit);
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Jerry Zhu <jerry.zhu@cixtech.com>");
MODULE_DESCRIPTION("Cix sky1 se config driver");

