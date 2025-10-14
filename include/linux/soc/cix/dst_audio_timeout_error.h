#ifndef __DST_AUDIO_TIMEOUT_H__
#define __DST_AUDIO_TIMEOUT_H__

#include <linux/ptrace.h>

void sky1_check_audio_timeout_error(unsigned long far, struct pt_regs *regs);

#endif