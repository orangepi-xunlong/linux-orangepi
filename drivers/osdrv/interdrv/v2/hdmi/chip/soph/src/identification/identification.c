#include "identification.h"
#include "core/hdmi_reg.h"
#include "util/util.h"
#include "bsp/access.h"

u8 id_design(hdmi_tx_dev_t *dev)
{
	return dev_read(DESIGN_ID);
}

u8 id_revision(hdmi_tx_dev_t *dev)
{
	return dev_read(REVISION_ID);
}

u8 id_product_line(hdmi_tx_dev_t *dev)
{
	return dev_read(PRODUCT_ID0);
}

u8 id_product_type(hdmi_tx_dev_t *dev)
{
	return dev_read(PRODUCT_ID1);
}

int id_hdcp_support(hdmi_tx_dev_t *dev)
{
	if (dev_read_mask(PRODUCT_ID1, PRODUCT_ID1_PRODUCT_ID1_HDCP_MASK) == 3)
		return TRUE;
	else
		return FALSE;
}

int id_hdcp14_support(hdmi_tx_dev_t *dev)
{
	if (dev_read_mask(CONFIG0_ID, CONFIG0_ID_HDCP_MASK))
		return TRUE;
	else
		return FALSE;
}

int id_hdcp22_support(hdmi_tx_dev_t *dev)
{
	if (dev_read_mask(CONFIG1_ID, CONFIG1_ID_HDCP22_EXT_MASK))
		return HDCP_22_EXT;
	else if (dev_read_mask(CONFIG1_ID, CONFIG1_ID_HDCP22_SNPS_MASK))
		return HDCP_22_SNPS;
	else
		return FALSE;
}

int id_phy(hdmi_tx_dev_t *dev)
{
	return dev_read(CONFIG2_ID);
}

