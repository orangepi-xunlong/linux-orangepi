#ifndef _HDMI_PROC_H_
#define _HDMI_PROC_H_
#include "core/hdmi_core.h"
#include <linux/seq_file.h>
#include <generated/compile.h>
#include <base_ctx.h>

int hdmi_proc_init(struct hdmitx_dev *dev);

int hdmi_video_proc_init(struct hdmitx_dev *dev);

int hdmi_audio_proc_init(struct hdmitx_dev *dev);

int hdmi_sink_proc_init(struct hdmitx_dev *dev);

int hdmi_proc_remove(struct hdmitx_dev *dev);

int hdmi_video_proc_remove(struct hdmitx_dev *dev);

int hdmi_audio_proc_remove(struct hdmitx_dev *dev);

int hdmi_sink_proc_remove(struct hdmitx_dev *dev);

#endif // _HDMI_PROC_H_
