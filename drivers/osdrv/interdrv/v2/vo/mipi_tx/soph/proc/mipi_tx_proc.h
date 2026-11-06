#ifndef _VO_MIPI_TX_PROC_H_
#define _VO_MIPI_TX_PROC_H_

#ifdef __cplusplus
	extern "C" {
#endif

#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <generated/compile.h>
#include <linux/slab.h>
#include <linux/comm_mipi_tx.h>

int mipi_tx_proc_init(int devno);
int mipi_tx_proc_remove(int devno);

#ifdef __cplusplus
}
#endif

#endif // _VO_MIPI_TX_PROC_H_
