#ifndef	_LINUX_AXP19_FILENODE_H_
#define	_LINUX_AXP19_FILENODE_H_

#define AXP_CHARGE_CONTROL1             AXP19_CHARGE_CONTROL1
#define AXP_CHARGE_CONTROL2             AXP19_CHARGE_CONTROL2
#define AXP_ADC_CONTROL3                AXP19_ADC_CONTROL3
#define AXP_CHARGE_VBUS                 AXP19_CHARGE_VBUS

#define AXP_IRQ_USBLO			AXP19_IRQ_USBLO
#define	AXP_IRQ_USBRE                   AXP19_IRQ_USBRE
#define	AXP_IRQ_USBIN                   AXP19_IRQ_USBIN
#define	AXP_IRQ_USBOV                   AXP19_IRQ_USBOV
#define	AXP_IRQ_ACRE                    AXP19_IRQ_ACRE
#define	AXP_IRQ_ACIN                    AXP19_IRQ_ACIN
#define	AXP_IRQ_ACOV                    AXP19_IRQ_ACOV
#define	AXP_IRQ_CHAOV                   AXP19_IRQ_CHAOV
#define	AXP_IRQ_CHAST                   AXP19_IRQ_CHAST
#define	AXP_IRQ_BATATOU                 AXP19_IRQ_BATATOU
#define	AXP_IRQ_BATATIN                 AXP19_IRQ_BATATIN
#define AXP_IRQ_BATRE                   AXP19_IRQ_BATRE
#define AXP_IRQ_BATIN                   AXP19_IRQ_BATIN
#define	AXP_IRQ_TEMLO                   AXP19_IRQ_TEMLO
#define	AXP_IRQ_TEMOV                   AXP19_IRQ_TEMOV
#define AXP_IRQ_PEKLO                   AXP19_IRQ_PEKLO
#define AXP_IRQ_PEKSH                   AXP19_IRQ_PEKSH

#define AXP_STATUS_SOURCE               AXP19_STATUS_SOURCE
#define AXP_STATUS_ACUSBSH              AXP19_STATUS_ACUSBSH
#define AXP_STATUS_BATCURDIR            AXP19_STATUS_BATCURDIR
#define AXP_STATUS_USBLAVHO             AXP19_STATUS_USBLAVHO
#define AXP_STATUS_USBVA                AXP19_STATUS_USBVA
#define AXP_STATUS_USBEN                AXP19_STATUS_USBEN
#define AXP_STATUS_ACVA                 AXP19_STATUS_ACVA
#define AXP_STATUS_ACEN                 AXP19_STATUS_ACEN
#define AXP_STATUS_BATINACT             AXP19_STATUS_BATINACT
#define AXP_STATUS_BATEN                AXP19_STATUS_BATEN
#define AXP_STATUS_INCHAR               AXP19_STATUS_INCHAR
#define AXP_STATUS_ICTEMOV              AXP19_STATUS_ICTEMOV

#define AXP_VBATH_RES                   AXP19_VBATH_RES
#define AXP_VACH_RES                    AXP19_VACH_RES
#define AXP_CHARGE_STATUS               AXP19_CHARGE_STATUS
#define AXP_IN_CHARGE                   AXP19_IN_CHARGE
#define AXP_INTTEMP                     AXP19_INTTEMP

#define AXP_INTEN1                      POWER19_INTEN1
#define AXP_INTEN2                      POWER19_INTEN2
#define AXP_INTEN3                      POWER19_INTEN3
#define AXP_INTEN4                      POWER19_INTEN4
#define AXP_INTSTS1                     POWER19_INTSTS1
#define AXP_INTSTS2                     POWER19_INTSTS2
#define AXP_INTSTS3                     POWER19_INTSTS3
#define AXP_INTSTS4                     POWER19_INTSTS4

#define AXP_VOL_MAX                     AXP19_VOL_MAX
#define AXP_CAP                         AXP19_CAP

#define AXP19_NOTIFIER_ON		(AXP_IRQ_USBOV | \
					AXP_IRQ_USBIN | \
					AXP_IRQ_USBRE | \
					AXP_IRQ_USBLO | \
					AXP_IRQ_ACOV | \
					AXP_IRQ_ACIN | \
					AXP_IRQ_ACRE | \
					AXP_IRQ_TEMOV | \
					AXP_IRQ_TEMLO | \
					AXP_IRQ_BATIN | \
					AXP_IRQ_BATRE | \
					AXP_IRQ_PEKLO | \
					AXP_IRQ_PEKSH)

#endif

