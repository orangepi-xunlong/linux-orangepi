/* SPDX-License-Identifier: GPL-2.0 */
#ifndef X1_ISP_PIPE_DEV_H
#define X1_ISP_PIPE_DEV_H

struct file_operations *x1isp_pipe_get_fops(void);
int x1isp_pipe_dev_init(struct platform_device *pdev,
			 struct x1isp_pipe_dev *isp_pipe_dev[]);
int x1isp_pipe_dev_exit(struct platform_device *pdev,
			 struct x1isp_pipe_dev *isp_pipe_dev[]);
void x1isp_pipe_dev_irq_handler(void *irq_data);
void x1isp_pipe_dma_irq_handler(struct x1isp_pipe_dev *pipe_dev, void *irq_data);
#endif
