#ifndef __CVI_OTG_FSM_H
#define __CVI_OTG_FSM_H

#include <linux/workqueue.h>
#include <linux/version.h>

void cviusb_usb_otg_work(struct work_struct *work);
void otg_hnp_polling_work(struct work_struct *work);
void cviusb_set_state(struct otg_fsm *fsm, enum usb_otg_state new_state);

#endif /* __CVI_OTG_FSM_H */
