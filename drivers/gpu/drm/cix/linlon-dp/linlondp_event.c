// SPDX-License-Identifier: GPL-2.0
/*
 * (C) COPYRIGHT 2022-2023 Arm Technology (China) Co., Ltd.
 * ALL RIGHTS RESERVED
 *
 */
#include <drm/drm_atomic.h>
#include <drm/drm_print.h>

#include "linlondp_dev.h"

struct linlondp_str {
    char *str;
    u32 sz;
    u32 len;
};

/* return 0 on success,  < 0 on no space.
 */
__printf(2, 3)
static int linlondp_sprintf(struct linlondp_str *str, const char *fmt, ...)
{
    va_list args;
    int num, free_sz;
    int err;

    free_sz = str->sz - str->len - 1;
    if (free_sz <= 0)
        return -ENOSPC;

    va_start(args, fmt);

    num = vsnprintf(str->str + str->len, free_sz, fmt, args);

    va_end(args);

    if (num < free_sz) {
        str->len += num;
        err = 0;
    } else {
        str->len = str->sz - 1;
        err = -ENOSPC;
    }

    return err;
}

static void evt_sprintf(struct linlondp_str *str, u64 evt, const char *msg)
{
    if (evt)
        linlondp_sprintf(str, msg);
}

static void evt_str(struct linlondp_str *str, u64 events)
{
    if (events == 0ULL) {
        linlondp_sprintf(str, "None");
        return;
    }

    evt_sprintf(str, events & LINLONDP_EVENT_VSYNC, "VSYNC|");
    evt_sprintf(str, events & LINLONDP_EVENT_FLIP, "FLIP|");
    evt_sprintf(str, events & LINLONDP_EVENT_EOW, "EOW|");
    evt_sprintf(str, events & LINLONDP_EVENT_MODE, "OP-MODE|");

    evt_sprintf(str, events & LINLONDP_EVENT_URUN, "UNDERRUN|");
    evt_sprintf(str, events & LINLONDP_EVENT_OVR, "OVERRUN|");

    /* GLB error */
    evt_sprintf(str, events & LINLONDP_ERR_MERR, "MERR|");
    evt_sprintf(str, events & LINLONDP_ERR_FRAMETO, "FRAMETO|");

    /* DOU error */
    evt_sprintf(str, events & LINLONDP_ERR_DRIFTTO, "DRIFTTO|");
    evt_sprintf(str, events & LINLONDP_ERR_FRAMETO, "FRAMETO|");
    evt_sprintf(str, events & LINLONDP_ERR_TETO, "TETO|");
    evt_sprintf(str, events & LINLONDP_ERR_CSCE, "CSCE|");

    /* LPU errors or events */
    evt_sprintf(str, events & LINLONDP_EVENT_IBSY, "IBSY|");
    evt_sprintf(str, events & LINLONDP_EVENT_EMPTY, "EMPTY|");
    evt_sprintf(str, events & LINLONDP_EVENT_FULL, "FULL|");
    evt_sprintf(str, events & LINLONDP_ERR_AXIE, "AXIE|");
    evt_sprintf(str, events & LINLONDP_ERR_ACE0, "ACE0|");
    evt_sprintf(str, events & LINLONDP_ERR_ACE1, "ACE1|");
    evt_sprintf(str, events & LINLONDP_ERR_ACE2, "ACE2|");
    evt_sprintf(str, events & LINLONDP_ERR_ACE3, "ACE3|");

    /* LPU TBU errors*/
    evt_sprintf(str, events & LINLONDP_ERR_TCF, "TCF|");
    evt_sprintf(str, events & LINLONDP_ERR_TTNG, "TTNG|");
    evt_sprintf(str, events & LINLONDP_ERR_TITR, "TITR|");
    evt_sprintf(str, events & LINLONDP_ERR_TEMR, "TEMR|");
    evt_sprintf(str, events & LINLONDP_ERR_TTF, "TTF|");

    /* CU errors*/
    evt_sprintf(str, events & LINLONDP_ERR_CPE, "COPROC|");
    evt_sprintf(str, events & LINLONDP_ERR_ZME, "ZME|");
    evt_sprintf(str, events & LINLONDP_ERR_CFGE, "CFGE|");
    evt_sprintf(str, events & LINLONDP_ERR_TEMR, "TEMR|");

    if (str->len > 0 && (str->str[str->len - 1] == '|')) {
        str->str[str->len - 1] = 0;
        str->len--;
    }
}

static bool is_new_frame(struct linlondp_events *a)
{
    return (a->pipes[0] | a->pipes[1]) &
           (LINLONDP_EVENT_FLIP | LINLONDP_EVENT_EOW);
}

void linlondp_print_events(struct linlondp_events *evts, struct drm_device *dev)
{
    u64 print_evts = 0;
    static bool en_print = true;
    struct linlondp_dev *mdev = dev->dev_private;
    u16 const err_verbosity = mdev->err_verbosity;
    u64 evts_mask = evts->global | evts->pipes[0] | evts->pipes[1];

    /* reduce the same msg print, only print the first evt for one frame */
    if (evts->global || is_new_frame(evts))
        en_print = true;
    if (!(err_verbosity & LINLONDP_DEV_PRINT_DISABLE_RATELIMIT) && !en_print)
        return;

    if (err_verbosity & LINLONDP_DEV_PRINT_ERR_EVENTS)
        print_evts |= LINLONDP_ERR_EVENTS;
    if (err_verbosity & LINLONDP_DEV_PRINT_WARN_EVENTS)
        print_evts |= LINLONDP_WARN_EVENTS;
    if (err_verbosity & LINLONDP_DEV_PRINT_INFO_EVENTS)
        print_evts |= LINLONDP_INFO_EVENTS;

    if (evts_mask & print_evts) {
        char msg[256];
        struct linlondp_str str;
        struct drm_printer p = drm_info_printer(dev->dev);

        str.str = msg;
        str.sz  = sizeof(msg);
        str.len = 0;

        linlondp_sprintf(&str, "gcu: ");
        evt_str(&str, evts->global);
        linlondp_sprintf(&str, ", pipes[0]: ");
        evt_str(&str, evts->pipes[0]);
        linlondp_sprintf(&str, ", pipes[1]: ");
        evt_str(&str, evts->pipes[1]);

        dev_err(mdev->dev, "err detect: %s\n", msg);
        if ((err_verbosity & LINLONDP_DEV_PRINT_DUMP_STATE_ON_EVENT) &&
            (evts_mask & (LINLONDP_ERR_EVENTS | LINLONDP_WARN_EVENTS)))
            drm_state_dump(dev, &p);

        en_print = false;
    }
}
