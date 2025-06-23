// SPDX-License-Identifier: GPL-2.0
/*
 * (C) COPYRIGHT 2022-2023 Arm Technology (China) Co., Ltd.
 * ALL RIGHTS RESERVED
 *
 */
#include <drm/drm_framebuffer.h>
#include "linlondp_dev.h"
#include "linlondp_kms.h"

static int
linlondp_wb_init_data_flow(struct linlondp_layer *wb_layer,
             struct drm_connector_state *conn_st,
             struct linlondp_crtc_state *kcrtc_st,
             struct linlondp_data_flow_cfg *dflow)
{
    struct drm_framebuffer *fb = conn_st->writeback_job->fb;
    struct linlondp_wb_connector *wb_conn;

    memset(dflow, 0, sizeof(*dflow));

    dflow->out_w = fb->width;
    dflow->out_h = fb->height;

    /* the write back data comes from the compiz */
    pipeline_composition_size(kcrtc_st, false, &dflow->in_w, &dflow->in_h,
                  false);
    dflow->input.component = &wb_layer->base.pipeline->compiz->base;
    /* compiz doesn't output alpha */
    dflow->pixel_blend_mode = DRM_MODE_BLEND_PIXEL_NONE;
    dflow->rot = DRM_MODE_ROTATE_0;

    linlondp_complete_data_flow_cfg(wb_layer, dflow, fb);
        wb_conn = _drm_conn_to_kconn(conn_st->connector);
        if (wb_conn->force_scaling_split && dflow->en_scaling)
            dflow->en_split = true;

    return 0;
}

static int
linlondp_wb_encoder_atomic_check(struct drm_encoder *encoder,
                   struct drm_crtc_state *crtc_st,
                   struct drm_connector_state *conn_st)
{
    struct linlondp_crtc *kcrtc = to_kcrtc(crtc_st->crtc);
    struct linlondp_crtc_state *kcrtc_st = to_kcrtc_st(crtc_st);
    struct drm_writeback_job *writeback_job = conn_st->writeback_job;
    struct linlondp_layer *wb_layer;
    struct linlondp_data_flow_cfg dflow;
    int err;

    if (!writeback_job)
        return 0;

    if (!crtc_st->active) {
        DRM_DEBUG_ATOMIC("Cannot write the composition result out on a inactive CRTC.\n");
        return -EINVAL;
    }

    wb_layer = to_kconn(to_wb_conn(conn_st->connector))->wb_layer;

    /*
     * No need for a full modested when the only connector changed is the
     * writeback connector.
     */
    if (crtc_st->connectors_changed &&
        is_only_changed_connector(crtc_st, conn_st->connector))
        crtc_st->connectors_changed = false;

    err = linlondp_wb_init_data_flow(wb_layer, conn_st, kcrtc_st, &dflow);
    if (err)
        return err;

    if (kcrtc->side_by_side)
        err = linlondp_build_wb_sbs_data_flow(kcrtc,
                conn_st, kcrtc_st, &dflow);
    else if (dflow.en_split)
        err = linlondp_build_wb_split_data_flow(wb_layer,
                conn_st, kcrtc_st, &dflow);
    else
        err = linlondp_build_wb_data_flow(wb_layer,
                conn_st, kcrtc_st, &dflow);

    return err;
}

static const struct drm_encoder_helper_funcs linlondp_wb_encoder_helper_funcs = {
    .atomic_check = linlondp_wb_encoder_atomic_check,
};

static int
linlondp_wb_connector_get_modes(struct drm_connector *connector)
{
    return 0;
}

static enum drm_mode_status
linlondp_wb_connector_mode_valid(struct drm_connector *connector,
                   struct drm_display_mode *mode)
{
    struct drm_device *dev = connector->dev;
    struct drm_mode_config *mode_config = &dev->mode_config;
    int w = mode->hdisplay, h = mode->vdisplay;

    if ((w < mode_config->min_width) || (w > mode_config->max_width))
        return MODE_BAD_HVALUE;

    if ((h < mode_config->min_height) || (h > mode_config->max_height))
        return MODE_BAD_VVALUE;

    return MODE_OK;
}

static const struct drm_connector_helper_funcs linlondp_wb_conn_helper_funcs = {
    .get_modes    = linlondp_wb_connector_get_modes,
    .mode_valid    = linlondp_wb_connector_mode_valid,
};

static enum drm_connector_status
linlondp_wb_connector_detect(struct drm_connector *connector, bool force)
{
    return connector_status_connected;
}

static int
linlondp_wb_connector_fill_modes(struct drm_connector *connector,
                   uint32_t maxX, uint32_t maxY)
{
    return 0;
}

static void linlondp_wb_connector_destroy(struct drm_connector *connector)
{
    drm_connector_cleanup(connector);
    kfree(to_kconn(to_wb_conn(connector)));
}

static int
linlondp_wb_connector_get_property(struct drm_connector *connector,
                 const struct drm_connector_state *state,
                 struct drm_property *property,
                 uint64_t *val)
{
    struct linlondp_wb_connector *wb_conn = _drm_conn_to_kconn(connector);
    struct linlondp_wb_connector_state *conn_st = to_kconn_st(state);

    if (property == wb_conn->color_encoding_property) {
        *val = conn_st->color_encoding;
    } else if (property == wb_conn->color_range_property) {
        *val = conn_st->color_range;
    } else {
        DRM_DEBUG_ATOMIC("Unknown property %s\n", property->name);
        return -EINVAL;
    }
    return 0;
}

static int
linlondp_wb_connector_set_property(struct drm_connector *connector,
                 struct drm_connector_state *state,
                 struct drm_property *property,
                 uint64_t val)
{
    struct linlondp_wb_connector *wb_conn = _drm_conn_to_kconn(connector);
    struct linlondp_wb_connector_state *conn_st = to_kconn_st(state);

    if (property == wb_conn->color_encoding_property) {
        conn_st->color_encoding = val;
    } else if (property == wb_conn->color_range_property) {
        conn_st->color_range = val;
    } else {
        DRM_DEBUG_ATOMIC("Unknown property %s\n", property->name);
        return -EINVAL;
    }
    return 0;
}

static void
linlondp_wb_connector_destroy_state(struct drm_connector *connector,
                  struct drm_connector_state *state)
{
    __drm_atomic_helper_connector_destroy_state(state);
    kfree(to_kconn_st(state));
}

#ifdef CONFIG_DEBUG_FS
static int
linlondp_wb_connector_debugfs_init(struct drm_connector *connector)
{
    struct linlondp_wb_connector *wb_conn = _drm_conn_to_kconn(connector);

    debugfs_create_bool("force_scaling_split", 0644,
                connector->debugfs_entry,
                &wb_conn->force_scaling_split);

    return 0;
}
#endif /*CONFIG_DEBUG_FS*/

static int
linlondp_wb_connector_late_register(struct drm_connector *connector)
{
#ifdef CONFIG_DEBUG_FS
    linlondp_wb_connector_debugfs_init(connector);
#endif /*CONFIG_DEBUG_FS*/

    return 0;
}

static void linlondp_wb_connector_reset(struct drm_connector *connector)
{
    struct linlondp_wb_connector_state *kc_state =
        kzalloc(sizeof(*kc_state), GFP_KERNEL);

    if (connector->state) {
        __drm_atomic_helper_connector_destroy_state(connector->state);
        kfree(to_kconn_st(connector->state));
    }

    kc_state->color_encoding = DRM_COLOR_YCBCR_BT601;
    kc_state->color_range = DRM_COLOR_YCBCR_LIMITED_RANGE;
    __drm_atomic_helper_connector_reset(connector, &kc_state->base);
}

static struct drm_connector_state *
linlondp_wb_connector_duplicate_state(struct drm_connector *connector)
{
    struct linlondp_wb_connector_state *state;

    if (WARN_ON(!connector->state))
        return NULL;

    state = kzalloc(sizeof(*state), GFP_KERNEL);
    if (state) {
        struct linlondp_wb_connector_state *old_state =
                to_kconn_st(connector->state);

        __drm_atomic_helper_connector_duplicate_state(connector, &state->base);
        state->color_encoding = old_state->color_encoding;
        state->color_range = old_state->color_range;
    }

    return &state->base;
}

static const struct drm_connector_funcs linlondp_wb_connector_funcs = {
    .reset            = linlondp_wb_connector_reset,
    .detect            = linlondp_wb_connector_detect,
    .fill_modes        = linlondp_wb_connector_fill_modes,
    .destroy        = linlondp_wb_connector_destroy,
    .atomic_duplicate_state    = linlondp_wb_connector_duplicate_state,
    .atomic_destroy_state    = linlondp_wb_connector_destroy_state,
    .atomic_set_property    = linlondp_wb_connector_set_property,
    .atomic_get_property    = linlondp_wb_connector_get_property,
    .late_register      = linlondp_wb_connector_late_register,
};

static const char * const color_encoding_name[] = {
    [DRM_COLOR_YCBCR_BT601] = "ITU-R BT.601 YCbCr",
    [DRM_COLOR_YCBCR_BT709] = "ITU-R BT.709 YCbCr",
    [DRM_COLOR_YCBCR_BT2020] = "ITU-R BT.2020 YCbCr",
};

static const char * const color_range_name[] = {
    [DRM_COLOR_YCBCR_FULL_RANGE] = "YCbCr full range",
    [DRM_COLOR_YCBCR_LIMITED_RANGE] = "YCbCr limited range",
};

static void
linlondp_wb_connector_attch_property(struct linlondp_wb_connector *wb_conn,
                   struct drm_property *property,
                   uint64_t init_val)
{
    struct drm_mode_object *obj = &wb_conn->base.base.base;

    drm_object_attach_property(obj, property, init_val);
}

static int
linlondp_wb_connector_create_color_prop(struct linlondp_wb_connector *wb_conn)
{
    struct drm_device *dev= wb_conn->base.base.dev;
    struct drm_property *prop;
    struct drm_prop_enum_list enum_list[max_t(int, DRM_COLOR_ENCODING_MAX,
                               DRM_COLOR_RANGE_MAX)];
    u32 supported_encoding = BIT(DRM_COLOR_YCBCR_BT601) |
                 BIT(DRM_COLOR_YCBCR_BT709);
    u32 supported_range = BIT(DRM_COLOR_YCBCR_LIMITED_RANGE) |
                  BIT(DRM_COLOR_YCBCR_FULL_RANGE);
    int i, len;

    len = 0;
    for (i = 0; i < DRM_COLOR_ENCODING_MAX; i++) {
        if ((supported_encoding & BIT(i)) == 0)
            continue;
        enum_list[len].type = i;
        enum_list[len].name = color_encoding_name[i];
        len++;
    }
    prop = drm_property_create_enum(dev, 0, "COLOR_ENCODING",
                    enum_list, len);
    if (!prop)
        return -ENOMEM;
    wb_conn->color_encoding_property = prop;
    linlondp_wb_connector_attch_property(wb_conn, prop,
                       DRM_COLOR_YCBCR_BT601);

    len = 0;
    for (i = 0; i < DRM_COLOR_RANGE_MAX; i++) {
        if ((supported_range & BIT(i)) == 0)
            continue;
        enum_list[len].type = i;
        enum_list[len].name = color_range_name[i];
        len++;
    }
    prop = drm_property_create_enum(dev, 0, "COLOR_RANGE",
                    enum_list, len);
    if (!prop)
        return -ENOMEM;
    wb_conn->color_range_property = prop;
    linlondp_wb_connector_attch_property(wb_conn, prop,
                       DRM_COLOR_YCBCR_LIMITED_RANGE);

    return 0;
}

static int linlondp_wb_connector_add(struct linlondp_kms_dev *kms,
                   struct linlondp_crtc *kcrtc)
{
    struct linlondp_dev *mdev = kms->base.dev_private;
    struct linlondp_wb_connector *kwb_conn;
    struct drm_writeback_connector *wb_conn;
    struct drm_display_info *info;
    u32 *formats, n_formats = 0;
    int err;

    if (!kcrtc->master->wb_layer)
        return 0;

    kwb_conn = kzalloc(sizeof(*kwb_conn), GFP_KERNEL);
    if (!kwb_conn)
        return -ENOMEM;

    kwb_conn->wb_layer = kcrtc->master->wb_layer;
        kwb_conn->force_scaling_split = false;

        kwb_conn->expected_eow = BIT(kcrtc->master->id);
        if (kcrtc->side_by_side && kcrtc->slave)
            kwb_conn->expected_eow |= BIT(kcrtc->slave->id);

    wb_conn = &kwb_conn->base;

    formats = linlondp_get_layer_fourcc_list(&mdev->fmt_tbl,
                           kwb_conn->wb_layer->layer_type,
                           &n_formats);

    err = drm_writeback_connector_init(&kms->base, wb_conn,
                       &linlondp_wb_connector_funcs,
                       &linlondp_wb_encoder_helper_funcs,
                       formats, n_formats,
                       BIT(drm_crtc_index(&kcrtc->base)));
    linlondp_put_fourcc_list(formats);
    if (err) {
        kfree(kwb_conn);
        return err;
    }

    drm_connector_helper_add(&wb_conn->base, &linlondp_wb_conn_helper_funcs);

    info = &kwb_conn->base.base.display_info;
    info->bpc = __fls(kcrtc->master->improc->supported_color_depths);
    info->color_formats = kcrtc->master->improc->supported_color_formats;

    kcrtc->wb_conn = kwb_conn;

    err = linlondp_wb_connector_create_color_prop(kwb_conn);
    if (err)
        return err;

    return 0;
}

int linlondp_kms_add_wb_connectors(struct linlondp_kms_dev *kms,
                 struct linlondp_dev *mdev)
{
    int i, err;

    for (i = 0; i < kms->n_crtcs; i++) {
        err = linlondp_wb_connector_add(kms, &kms->crtcs[i]);
        if (err)
            return err;
    }

    return 0;
}
