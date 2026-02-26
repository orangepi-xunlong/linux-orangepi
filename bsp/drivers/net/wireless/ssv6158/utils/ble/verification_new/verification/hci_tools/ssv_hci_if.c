#include "ssv_hci_if.h"
#include <stdlib.h>
#include <assert.h>
#include "../log/log.h"
#include "../hci_tools/lib_bluez/bluetooth.h"
#include "../hci_tools/lib_bluez/hci.h"
#include "../hci_tools/lib_bluez/hci_lib.h"

typedef struct {
	int	dd;		// device desc
	int	dev_id;
	bdaddr_t	ba;
	char	dev_str[8];
	char	vendor_str[8];
	struct hci_filter flt;
} ssv_hci_st;

ssv_hci_st	s_hci;

void	ssv_hci_dump(int handle)
{
	ssv_hci_st	*p = (ssv_hci_st *)handle;
	char	str[32];

	if (p != (&s_hci)) {
		printf(COLOR_ERR "%s(): invalid 'handle'\n" COLOR_NONE, __FUNCTION__);
		return;
	}
	printf("< ssv_hci_dev: 0x%08x >\n", handle);
	printf("%-12s: %d\n", "fd", p->dd);
	printf("%-12s: %d\n", "dev_id", p->dev_id);
	ba2str(&(p->ba), str);
	printf("%-12s: %s\n", "ba", str);
	printf("%-12s: %s\n", "dev_str", p->dev_str);
	printf("%-12s: %s\n", "vendor_str", p->vendor_str);
	return;
}

int ssv_hci_open(const char *dev_str, const char *vendor_str)
{
	ssv_hci_st	*p = &s_hci;
	bdaddr_t ba;
	int dev_id, dd;

	// check 'dev_str' format
	if (strncmp(dev_str, "hci", 3) != 0)
	{
		printf(COLOR_ERR "%s(): invalid 'dev_str'\n" COLOR_NONE, __FUNCTION__);
		return 0;
	}
    if ((dev_id = hci_devid(dev_str)) < 0) {
        printf(COLOR_ERR "%s(): Invalid device\n" COLOR_NONE, __FUNCTION__);
        return 0;
    }
	if (hci_devba(dev_id, &ba) < 0) {
		printf(COLOR_ERR "%s(): Device is not available\n" COLOR_NONE, __FUNCTION__);
		return 0;
	}
	if ((dd = hci_open_dev(dev_id)) < 0) {
		printf(COLOR_ERR "%s(): Device open failed\n" COLOR_NONE, __FUNCTION__);
		return 0;
	}
	// init s_hci
	memset(p, 0, sizeof(ssv_hci_st));
	p->dd		= dd;
	p->dev_id	= dev_id;
	memcpy(&(p->ba), &ba, sizeof(bdaddr_t));
	strcpy(p->dev_str, dev_str);
	if (vendor_str != 0)
		strcpy(p->vendor_str, vendor_str);

	return (int)p;
}

int ssv_hci_close(int handle)
{
	ssv_hci_st	*p = (ssv_hci_st *)handle;

	if (p != (&s_hci)) {
		printf(COLOR_ERR "%s(): invalid 'handle'\n" COLOR_NONE, __FUNCTION__);
		return -1;
	}

	hci_close_dev(p->dd);
	memset(p, 0, sizeof(ssv_hci_st));
	return 0;
}

int ssv_hci_init_filter(int handle)
{
	ssv_hci_st	*p = (ssv_hci_st *)handle;
	struct hci_filter	*p_flt;
	if (p != (&s_hci)) {
		printf(COLOR_ERR "%s(): invalid 'handle'\n" COLOR_NONE, __FUNCTION__);
		return -1;
	}

	p_flt = &(p->flt);
	hci_filter_clear(p_flt);
	hci_filter_all_ptypes(p_flt);
	hci_filter_all_events(p_flt);
	if (setsockopt(p->dd, SOL_HCI, HCI_FILTER, p_flt, sizeof(struct hci_filter)) < 0) {
		printf(COLOR_ERR "%s(): HCI filter setup failed\n" COLOR_NONE, __FUNCTION__);
		// exit(EXIT_FAILURE);
		return -1;
	}
	return 0;
}

int	ssv_hci_get_dd(int handle)
{
	assert(handle == (int)&s_hci);

	return s_hci.dd;
}
