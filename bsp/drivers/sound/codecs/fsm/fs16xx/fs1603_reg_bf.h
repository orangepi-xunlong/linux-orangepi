/* SPDX-License-Identifier: GPL-2.0-or-later */
/**
 * Copyright (C) 2016-2019 Fourier Semiconductor Inc. All rights reserved.
 * 2020-01-17 File created.
 */

#ifndef __FS1603_H__
#define __FS1603_H__

#define FS1603_STATUS         0xF000
#define FS1603_BOVDS          0x0000
#define FS1603_PLLS           0x0100
#define FS1603_OTDS           0x0200
#define FS1603_OVDS           0x0300
#define FS1603_UVDS           0x0400
#define FS1603_OCDS           0x0500
#define FS1603_CLKS           0x0600
#define FS1603_SPKS           0x0A00
#define FS1603_SPKT           0x0B00

#define FS1603_BATS           0xF001
#define FS1603_BATV           0x9001

#define FS1603_TEMPS          0xF002
#define FS1603_TEMPV          0x8002

#define FS1603_ID             0xF003
#define FS1603_REV            0x7003
#define FS1603_DEVICE_ID      0x7803

#define FS1603_I2SCTRL        0xF004
#define FS1603_I2SF           0x2004
#define FS1603_CHS12          0x1304
#define FS1603_DISP           0x0A04
#define FS1603_I2SDOE         0x0B04
#define FS1603_I2SSR          0x3C04

#define FS1603_ANASTAT        0xF005
#define FS1603_VBGS           0x0005
#define FS1603_PLLSTAT        0x0105
#define FS1603_BSTS           0x0205
#define FS1603_AMPS           0x0305

#define FS1603_AUDIOCTRL      0xF006
#define FS1603_VOL            0x7806

#define FS1603_TEMPSEL        0xF008
#define FS1603_EXTTS          0x8108

#define FS1603_SYSCTRL        0xF009
#define FS1603_PWDN           0x0009
#define FS1603_I2CR           0x0109
#define FS1603_AMPE           0x0309

#define FS1603_SPKSET         0xF00A
#define FS1603_SPKR           0x190A

#define FS1603_OTPACC         0xF00B
#define FS1603_TRIMKEY        0x700B
#define FS1603_REKEY          0x780B

#define FS1603_SPKMT01        0xF064
#define FS1603_SPKMT02        0xF065
#define FS1603_SPKMT03        0xF066
#define FS1603_SPKMT04        0xF067
#define FS1603_SPKMT05        0xF068
#define FS1603_SPKMT06        0xF069

#define FS1603_STERC1         0xF070

#define FS1603_STERCTRL       0xF07E

#define FS1603_STERGAIN       0xF07F

#define FS1603_SPKSTAUTS      0xF080

#define FS1603_ACSEQWL        0xF082

#define FS1603_ACSEQWH        0xF083

#define FS1603_ACSEQRL        0xF084

#define FS1603_ACSEQRH        0xF085

#define FS1603_ACSEQA         0xF086

#define FS1603_ACSCTRL        0xF089

#define FS1603_ACSDRC         0xF08A

#define FS1603_ACSDRCS        0xF08B

#define FS1603_ACSDRCP        0xF08C

#define FS1603_CHIPINI        0xF090
#define FS1603_INIFINISH      0x0090
#define FS1603_INIOK          0x0190

#define FS1603_I2CADD         0xF091

#define FS1603_BISTSTAT1      0xF09D

#define FS1603_BISTSTAT2      0xF09E

#define FS1603_BISTSTAT3      0xF09F

#define FS1603_I2SSET         0xF0A0
#define FS1603_BCMP           0x13A0
#define FS1603_LRCLKP         0x05A0
#define FS1603_BCLKP          0x06A0
#define FS1603_I2SOSWAP       0x07A0
#define FS1603_AECSELL        0x28A0
#define FS1603_AECSELR        0x2CA0
#define FS1603_BCLKSTA        0x0FA0

#define FS1603_DSPCTRL        0xF0A1
#define FS1603_DCCOEF         0x20A1
#define FS1603_NOFILTEN       0x04A1
#define FS1603_POSTEQBEN      0x0AA1
#define FS1603_POSTEQEN       0x0BA1
#define FS1603_DSPEN          0x0CA1
#define FS1603_EQCOEFSEL      0x0DA1

#define FS1603_DACEQWL        0xF0A2

#define FS1603_DACEQWH        0xF0A3

#define FS1603_DACEQRL        0xF0A4

#define FS1603_DACEQRH        0xF0A5

#define FS1603_DACEQA         0xF0A6

#define FS1603_BFLCTRL        0xF0A7
#define FS1603_BFL_EN         0x01A7
#define FS1603_BFLFUN_EN      0x02A7

#define FS1603_BFLSET         0xF0A8

#define FS1603_SQC            0xF0A9

#define FS1603_AGC            0xF0AA
#define FS1603_AGC_GAIN       0x70AA

#define FS1603_DRPARA         0xF0AD
#define FS1603_DRC            0xC0AD

#define FS1603_DACCTRL        0xF0AE
#define FS1603_SDMSTLBYP      0x04AE
#define FS1603_MUTE           0x08AE
#define FS1603_FADE           0x09AE

#define FS1603_TSCTRL         0xF0AF
#define FS1603_TSGAIN         0x20AF
#define FS1603_TSEN           0x03AF
#define FS1603_OFF_THD        0x24AF
#define FS1603_OFF_DELAY      0x28AF
#define FS1603_OFF_ZEROCRS    0x0CAF
#define FS1603_OFF_AUTOEN     0x0DAF
#define FS1603_OFFSTA         0x0EAF

#define FS1603_ADCCTRL        0xF0B3
#define FS1603_ADCLEN         0x04B3
#define FS1603_EQB1EN_R       0x08B3
#define FS1603_EQB0EN_R       0x09B3
#define FS1603_ADCRGAIN       0x1AB3
#define FS1603_ADCREN         0x0CB3
#define FS1603_ADCRSEL        0x0DB3

#define FS1603_ADCEQWL        0xF0B4

#define FS1603_ADCEQWH        0xF0B5

#define FS1603_ADCEQRL        0xF0B6

#define FS1603_ADCEQRH        0xF0B7

#define FS1603_ADCEQA         0xF0B8

#define FS1603_ADCENV         0xF0B9

#define FS1603_ADCTIME        0xF0BA

#define FS1603_ZMDATA         0xF0BB

#define FS1603_DACENV         0xF0BC

#define FS1603_DIGSTAT        0xF0BD
#define FS1603_ADCRUN         0x00BD
#define FS1603_DACRUN         0x01BD
#define FS1603_DSPFLAG        0x03BD
#define FS1603_STAM24         0x04BD
#define FS1603_STAM6          0x05BD
#define FS1603_STARE          0x06BD
#define FS1603_SPKFSM         0x3CBD

#define FS1603_BSTCTRL        0xF0C0
#define FS1603_DISCHARGE      0x00C0
#define FS1603_DAC_GAIN       0x11C0
#define FS1603_BSTEN          0x03C0
#define FS1603_MODE_CTRL      0x14C0
#define FS1603_ILIM_SEL       0x36C0
#define FS1603_VOUT_SEL       0x3AC0
#define FS1603_SSEND          0x0FC0

#define FS1603_PLLCTRL1       0xF0C1

#define FS1603_PLLCTRL2       0xF0C2

#define FS1603_PLLCTRL3       0xF0C3

#define FS1603_PLLCTRL4       0xF0C4
#define FS1603_PLLEN          0x00C4
#define FS1603_OSCEN          0x01C4
#define FS1603_ZMEN           0x02C4
#define FS1603_VBGEN          0x03C4

#define FS1603_OTCTRL         0xF0C6

#define FS1603_UVCTRL         0xF0C7

#define FS1603_OVCTRL         0xF0C8

#define FS1603_SPKERR         0xF0C9

#define FS1603_SPKM24         0xF0CA

#define FS1603_SPKM6          0xF0CB

#define FS1603_SPKRE          0xF0CC

#define FS1603_SPKMDB         0xF0CD
#define FS1603_DBVSPKM6       0x70CD
#define FS1603_DBVSPKM24      0x78CD

#define FS1603_ADPBST         0xF0CF
#define FS1603_TFSEL          0x12CF
#define FS1603_TRSEL          0x14CF
#define FS1603_ENV            0x1ACF
#define FS1603_ENVSEL         0x0CCF
#define FS1603_DISCHARGESEL   0x1ECF

#define FS1603_ANACTRL        0xF0D0

#define FS1603_CLDCTRL        0xF0D3

#define FS1603_ZMCONFIG       0xF0D5
#define FS1603_CALIBEN        0x04D5

#define FS1603_AUXCFG         0xF0D7

#define FS1603_OTPCMD         0xF0DC
#define FS1603_OTPR           0x00DC
#define FS1603_OTPW           0x01DC
#define FS1603_OTPBUSY        0x02DC
#define FS1603_EPROM_LD       0x08DC
#define FS1603_PW             0x0CDC

#define FS1603_OTPADDR        0xF0DD

#define FS1603_OTPWDATA       0xF0DE

#define FS1603_OTPRDATA       0xF0DF

#define FS1603_OTPPG0W0       0xF0E0

#define FS1603_OTPPG0W1       0xF0E1

#define FS1603_OTPPG0W2       0xF0E2

#define FS1603_OTPPG0W3       0xF0E3

#define FS1603_OTPPG1W0       0xF0E4

#define FS1603_OTPPG1W1       0xF0E5

#define FS1603_OTPPG1W2       0xF0E6

#define FS1603_OTPPG1W3       0xF0E7

#define FS1603_OTPPG2         0xF0E8

#endif /* Generated at: 2020-01-17.13:28:54 */
