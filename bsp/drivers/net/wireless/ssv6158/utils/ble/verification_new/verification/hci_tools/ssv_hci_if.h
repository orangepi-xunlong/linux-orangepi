//
// Abstract:
//		ssv proprietary bluez hci control i/f.
// below this i/f is the bluez's hciconfig, hcitool, hcidump...,etc.
//
// NOTE:
//		except ssv_hci_open(), all other functions will return 0 to indicate SUCCESS as default.
//
#ifndef __SSV_HCI_IF_H__
#define __SSV_HCI_IF_H__

void	ssv_hci_dump(int handle);

// NOTE:
//	(1) dev reset operation will be done inside this function.
//	(2) the 'dev_str' should be in format of "hciX".
//
// return
// > 0: success, a handle value.
// = 0: fail.
//
int	ssv_hci_open(const char *dev_str, const char *vendor_str);
int	ssv_hci_close(int handle);
// NOTE: please directly modify this function if you want different filter settings.
int	ssv_hci_init_filter(int handle);

int	ssv_hci_get_dd(int handle);

#endif		// __SSV_HCI_IF_H__
