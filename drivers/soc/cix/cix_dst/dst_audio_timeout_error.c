#include "dst_print.h"
#include <linux/soc/cix/dst_audio_timeout_error.h>
#include "blackbox/rdr_utils.h"

#define SKY1_AUDIO_ADDR_BASE    (0x07000000)
#define SKY1_AUDIO_ADDR_END     (0x07FFFFFF)

int sky1_check_audio_timeout_error(unsigned long far)
{
	unsigned long phy_addr;

	phy_addr = dst_get_phy_addr(far);
	DST_PRINT_DBG("%s,%d: far=0x%lx phy=0x%lx \n", __func__, __LINE__, far, phy_addr);

	if (phy_addr >= SKY1_AUDIO_ADDR_BASE && phy_addr < SKY1_AUDIO_ADDR_END) {
		DST_PRINT_ERR("%s,%d: audio timeout, far=0x%lx phy=0x%lx \n", __func__, __LINE__, far, phy_addr);
		return -1;
	}

	return 0;
}