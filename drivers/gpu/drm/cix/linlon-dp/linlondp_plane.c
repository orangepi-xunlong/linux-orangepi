// SPDX-License-Identifier: GPL-2.0
/*
 * (C) COPYRIGHT 2022-2023 Arm Technology (China) Co., Ltd.
 * ALL RIGHTS RESERVED
 *
 */
#include <drm/drm_atomic.h>
#include <drm/drm_atomic_helper.h>
#include <drm/drm_blend.h>
#include <drm/drm_print.h>
#include <drm/drm_file.h>
#include "linlondp_dev.h"
#include "linlondp_drm.h"
#include "linlondp_kms.h"
#include "linlondp_framebuffer.h"

static int
linlondp_plane_init_data_flow(struct drm_plane_state *st,
                struct linlondp_crtc_state *kcrtc_st,
                struct linlondp_data_flow_cfg *dflow)
{
    struct linlondp_plane *kplane = to_kplane(st->plane);
    struct drm_framebuffer *fb = st->fb;
    const struct linlondp_format_caps *caps = to_kfb(fb)->format_caps;
    struct linlondp_pipeline *pipe = kplane->layer->base.pipeline;
    struct linlondp_dev *mdev = st->plane->dev->dev_private;
    struct drm_crtc_state *crtc_st;
    struct drm_display_mode *mode;
    struct drm_display_mode *adjusted_mode;
    u16 hdisplay, vdisplay, crtc_hdisplay, crtc_vdisplay;

    memset(dflow, 0, sizeof(*dflow));

    dflow->blending_zorder = st->normalized_zpos;
    if (pipe == to_kcrtc(st->crtc)->master)
        dflow->blending_zorder -= kcrtc_st->max_slave_zorder;
    if (dflow->blending_zorder < 0) {
        DRM_DEBUG_ATOMIC("%s zorder:%d < max_slave_zorder: %d.\n",
                 st->plane->name, st->normalized_zpos,
                 kcrtc_st->max_slave_zorder);
        return -EINVAL;
    }

    dflow->pixel_blend_mode = st->pixel_blend_mode;
    dflow->layer_alpha = st->alpha >> 8;

    dflow->out_x = st->crtc_x;
    dflow->out_y = st->crtc_y;
    dflow->out_w = st->crtc_w;
    dflow->out_h = st->crtc_h;

    if (!mdev->side_by_side) {
        crtc_st = drm_atomic_get_new_crtc_state(st->state, st->crtc);
        mode = &crtc_st->mode;
        adjusted_mode = &crtc_st->adjusted_mode;
        hdisplay = mode->hdisplay;
        vdisplay = mode->vdisplay;
        crtc_hdisplay = adjusted_mode->crtc_hdisplay;
        crtc_vdisplay = adjusted_mode->crtc_vdisplay;

        if (crtc_hdisplay != hdisplay) {
            dflow->out_x = dflow->out_x * crtc_hdisplay / hdisplay;
            dflow->out_w = dflow->out_w * crtc_hdisplay / hdisplay;
        }

        if (crtc_vdisplay != vdisplay) {
            dflow->out_y = dflow->out_y * crtc_vdisplay / vdisplay;
            dflow->out_h = dflow->out_h * crtc_vdisplay / vdisplay;
        }
    }

    dflow->in_x = st->src_x >> 16;
    dflow->in_y = st->src_y >> 16;
    dflow->in_w = st->src_w >> 16;
    dflow->in_h = st->src_h >> 16;

    dflow->rot = drm_rotation_simplify(st->rotation, caps->supported_rots);
    if (!has_bits(dflow->rot, caps->supported_rots)) {
        DRM_DEBUG_ATOMIC("rotation(0x%x) isn't supported by %p4cc with modifier: 0x%llx.\n",
                 dflow->rot, &caps->fourcc, fb->modifier);
        return -EINVAL;
    }

    linlondp_complete_data_flow_cfg(kplane->layer, dflow, fb);

    /* debug flag, force en_split after comlete data flow cfg */
    if (kplane->force_layer_split)
        dflow->en_split = true;

    return 0;
}

/**
 * linlondp_plane_atomic_check - build input data flow
 * @plane: DRM plane
 * @state: the plane state object
 *
 * RETURNS:
 * Zero for success or -errno
 */
static int
linlondp_plane_atomic_check(struct drm_plane *plane,
                struct drm_atomic_state *state)
{
    struct drm_plane_state *new_plane_state = drm_atomic_get_new_plane_state(state, plane);
    struct linlondp_plane *kplane = to_kplane(plane);
    struct linlondp_plane_state *kplane_st = to_kplane_st(new_plane_state);
    struct linlondp_layer *layer = kplane->layer;
    struct drm_crtc_state *crtc_st;
    struct linlondp_crtc *kcrtc;
    struct linlondp_crtc_state *kcrtc_st;
    struct linlondp_data_flow_cfg dflow;
    int err;

    if (!new_plane_state->crtc || !new_plane_state->fb)
        return 0;

    crtc_st = drm_atomic_get_crtc_state(state, new_plane_state->crtc);
    if (IS_ERR(crtc_st) || !crtc_st->enable) {
        DRM_DEBUG_ATOMIC("Cannot update plane on a disabled CRTC.\n");
        return -EINVAL;
    }

    /* crtc is inactive, skip the resource assignment */
    if (!crtc_st->active)
        return 0;

    kcrtc = to_kcrtc(crtc_st->crtc);
    kcrtc_st = to_kcrtc_st(crtc_st);

    err = linlondp_plane_init_data_flow(new_plane_state, kcrtc_st, &dflow);
    if (err)
        return err;

    if (kcrtc->side_by_side)
        err = linlondp_build_layer_sbs_data_flow(layer,
                kplane_st, kcrtc_st, &dflow);
    else if (dflow.en_split)
        err = linlondp_build_layer_split_data_flow(layer,
                kplane_st, kcrtc_st, &dflow);
    else
        err = linlondp_build_layer_data_flow(layer,
                kplane_st, kcrtc_st, &dflow);

    return err;
}

/* plane doesn't represent a real HW, so there is no HW update for plane.
 * linlondp handles all the HW update in crtc->atomic_flush
 */
static void
linlondp_plane_atomic_update(struct drm_plane *plane,
                struct drm_atomic_state *state)
{
}

static const struct drm_plane_helper_funcs linlondp_plane_helper_funcs = {
    .atomic_check    = linlondp_plane_atomic_check,
    .atomic_update    = linlondp_plane_atomic_update,
};

static void linlondp_plane_destroy(struct drm_plane *plane)
{
    drm_plane_cleanup(plane);

    kfree(to_kplane(plane));
}

static void linlondp_plane_reset(struct drm_plane *plane)
{
    struct linlondp_plane_state *state;

    if (plane->state)
        __drm_atomic_helper_plane_destroy_state(plane->state);

    kfree(plane->state);
    plane->state = NULL;

    state = kzalloc(sizeof(*state), GFP_KERNEL);
    if (state)
        __drm_atomic_helper_plane_reset(plane, &state->base);
}

static inline struct drm_property_blob *
linlondp_drm_blob_get(struct drm_property_blob *blob)
{
    if (!blob)
        return NULL;

    return drm_property_blob_get(blob);
}

static struct drm_plane_state *
linlondp_plane_atomic_duplicate_state(struct drm_plane *plane)
{
    struct linlondp_plane_state *new;
    struct linlondp_plane_state *old;

    if (WARN_ON(!plane->state))
        return NULL;

    new = kzalloc(sizeof(*new), GFP_KERNEL);
    if (!new)
        return NULL;

    old = to_kplane_st(plane->state);
    new->layer_split = old->layer_split;
    new->degamma_lut = linlondp_drm_blob_get(old->degamma_lut);
    new->gamma_lut = linlondp_drm_blob_get(old->gamma_lut);
    new->ctm = linlondp_drm_blob_get(old->ctm);
    INIT_LIST_HEAD(&new->zlist_node);

    __drm_atomic_helper_plane_duplicate_state(plane, &new->base);

    return &new->base;
}

static void
linlondp_plane_atomic_destroy_state(struct drm_plane *plane,
                  struct drm_plane_state *state)
{
    __drm_atomic_helper_plane_destroy_state(state);
    kfree(to_kplane_st(state));
}

static int
linlondp_plane_atomic_get_property(struct drm_plane *plane,
                 const struct drm_plane_state *state,
                 struct drm_property *property,
                 uint64_t *val)
{
    struct linlondp_plane_state *st = to_kplane_st(state);
    struct linlondp_plane *kplane = to_kplane(plane);

    if (property == kplane->degamma_lut_property)
        *val = (st->degamma_lut) ? st->degamma_lut->base.id : 0;
    else if (property == kplane->ctm_property)
        *val = (st->ctm) ? st->ctm->base.id : 0;
    else if (property == kplane->ctm_ext_property)
        *val = (st->ctm) ? st->ctm->base.id : 0;
    else if (property == kplane->gamma_lut_property)
        *val = (st->gamma_lut) ? st->gamma_lut->base.id : 0;
    else if (property == kplane->layer_split_property)
        *val = st->layer_split;

    return 0;
}

int
linlondp_atomic_replace_property_blob_from_id(struct drm_device *dev,
                     struct drm_property_blob **blob,
                     uint64_t blob_id,
                     ssize_t expected_size,
                     ssize_t expected_elem_size,
                     bool *replaced)
{
    struct drm_property_blob *new_blob = NULL;

    if (blob_id != 0) {
        new_blob = drm_property_lookup_blob(dev, blob_id);
        if (new_blob == NULL)
            return -EINVAL;

        if (expected_size > 0 &&
            new_blob->length != expected_size) {
            drm_property_blob_put(new_blob);
            return -EINVAL;
        }
        if (expected_elem_size > 0 &&
            new_blob->length % expected_elem_size != 0) {
            drm_property_blob_put(new_blob);
            return -EINVAL;
        }
    }

    *replaced |= drm_property_replace_blob(blob, new_blob);
    drm_property_blob_put(new_blob);

    return 0;
}

static int
linlondp_plane_atomic_set_property(struct drm_plane *plane,
                 struct drm_plane_state *state,
                 struct drm_property *property,
                 uint64_t val)
{
    struct drm_device *drm = plane->dev;
    struct linlondp_plane *kplane = to_kplane(plane);
    struct linlondp_plane_state *kplane_st = to_kplane_st(state);
    bool replaced = false;
    int ret = 0;

    if (property == kplane->degamma_lut_property) {
        ret = linlondp_atomic_replace_property_blob_from_id(drm,
                        &kplane_st->degamma_lut,
                        val, -1, sizeof(struct drm_color_lut),
                        &replaced);
        kplane_st->color_mgmt_changed |= replaced;
    } else if (property == kplane->ctm_property) {
        ret = linlondp_atomic_replace_property_blob_from_id(drm,
                        &kplane_st->ctm,
                        val,
                        sizeof(struct drm_color_ctm), -1,
                        &replaced);
        kplane_st->color_mgmt_changed |= replaced;
    } else if (property == kplane->ctm_ext_property) {
        ret = linlondp_atomic_replace_property_blob_from_id(drm,
                        &kplane_st->ctm,
                        val,
                        sizeof(struct color_ctm_ext), -1,
                        &replaced);
        kplane_st->color_mgmt_changed |= replaced;
    } else if (property == kplane->gamma_lut_property) {
        ret = linlondp_atomic_replace_property_blob_from_id(drm,
                        &kplane_st->gamma_lut,
                        val, -1, sizeof(struct drm_color_lut),
                        &replaced);
        kplane_st->color_mgmt_changed |= replaced;
    } else if (property == kplane->layer_split_property) {
        kplane_st->layer_split = !!val;
        kplane->force_layer_split = !!val;
    }

    return ret;
}


static bool
linlondp_plane_format_mod_supported(struct drm_plane *plane,
                  u32 format, u64 modifier)
{
    struct linlondp_dev *mdev = plane->dev->dev_private;
    struct linlondp_plane *kplane = to_kplane(plane);
    u32 layer_type = kplane->layer->layer_type;

    return linlondp_format_mod_supported(&mdev->fmt_tbl, layer_type,
                       format, modifier, 0);
}

#ifdef CONFIG_DEBUG_FS
static int
linlondp_plane_debugfs_init(struct drm_plane *plane)
{
    struct linlondp_plane *kplane = to_kplane(plane);
    struct dentry *dir;
    char name[32];

    if (!kplane->layer || !kplane->layer->right)
        return 0;

    snprintf(name, sizeof(name), "plane-%d", drm_plane_index(plane));

    dir = debugfs_create_dir(name, plane->dev->primary->debugfs_root);
    if (!dir) {
        DRM_DEBUG_ATOMIC("Cannot create debugfs dir for %s\n", name);
        return 0;
    }

    debugfs_create_bool("force_layer_split", 0644, dir,
                &kplane->force_layer_split);

    return 0;
}
#endif /*CONFIG_DEBUG_FS*/

static const struct drm_plane_funcs linlondp_plane_funcs = {
    .update_plane        = drm_atomic_helper_update_plane,
    .disable_plane        = drm_atomic_helper_disable_plane,
    .destroy        = linlondp_plane_destroy,
    .reset            = linlondp_plane_reset,
    .atomic_duplicate_state    = linlondp_plane_atomic_duplicate_state,
    .atomic_destroy_state    = linlondp_plane_atomic_destroy_state,
    .atomic_get_property    = linlondp_plane_atomic_get_property,
    .atomic_set_property    = linlondp_plane_atomic_set_property,
    .format_mod_supported    = linlondp_plane_format_mod_supported,

#ifdef CONFIG_DEBUG_FS
        .late_register      = linlondp_plane_debugfs_init,
#endif /*CONFIG_DEBUG_FS*/
};

/* for linlondp, which is pipeline can be share between crtcs */
static u32 get_possible_crtcs(struct linlondp_kms_dev *kms,
                  struct linlondp_pipeline *pipe)
{
    struct linlondp_crtc *crtc;
    u32 possible_crtcs = 0;
    int i;

    for (i = 0; i < kms->n_crtcs; i++) {
        crtc = &kms->crtcs[i];

        if ((pipe == crtc->master) || (pipe == crtc->slave))
            possible_crtcs |= BIT(i);
    }

    return possible_crtcs;
}

static void
linlondp_set_crtc_plane_mask(struct linlondp_kms_dev *kms,
               struct linlondp_pipeline *pipe,
               struct drm_plane *plane)
{
    struct linlondp_crtc *kcrtc;
    int i;

    for (i = 0; i < kms->n_crtcs; i++) {
        kcrtc = &kms->crtcs[i];

        if (pipe == kcrtc->slave)
            kcrtc->slave_planes |= BIT(drm_plane_index(plane));
    }
}

/* use Layer0 as primary */
static u32 get_plane_type(struct linlondp_kms_dev *kms,
              struct linlondp_component *c)
{
    bool is_primary = (c->id == LINLONDP_COMPONENT_LAYER0);

    return is_primary ? DRM_PLANE_TYPE_PRIMARY : DRM_PLANE_TYPE_OVERLAY;
}

/**
* drm_plane_enable_color_mgmt - enable color management properties
* @plane: DRM Plane
* @plane_degamma_lut_size: the size of the degamma lut (before CSC)
* @plane_has_ctm: whether to attach ctm_property for CSC matrix
* @plane_gamma_lut_size: the size of the gamma lut (after CSC)
*
* This function lets the driver enable the color correction
* properties on a plane. This includes 3 degamma, csc and gamma
* properties that userspace can set and 2 size properties to inform
* the userspace of the lut sizes. Each of the properties are
* optional. The gamma and degamma properties are only attached if
* their size is not 0 and ctm_property is only attached if has_ctm is
* true.
*/
static void drm_plane_enable_color_mgmt(struct linlondp_plane *kplane,
        u32 plane_degamma_lut_size,
        bool plane_has_ctm,
        u32 plane_gamma_lut_size)
{
    struct drm_plane *plane = &kplane->base;

    if (plane_degamma_lut_size) {
        drm_object_attach_property(&plane->base,
            kplane->degamma_lut_property, 0);
        drm_object_attach_property(&plane->base,
            kplane->degamma_lut_size_property,
            plane_degamma_lut_size);
    }

    if (plane_has_ctm) {
        drm_object_attach_property(&plane->base,
            kplane->ctm_property, 0);
        drm_object_attach_property(&plane->base,
            kplane->ctm_ext_property, 0);
    }

    if (plane_gamma_lut_size) {
        drm_object_attach_property(&plane->base,
            kplane->gamma_lut_property, 0);
        drm_object_attach_property(&plane->base,
            kplane->gamma_lut_size_property,
            plane_gamma_lut_size);
    }
}

/**
* DOC: Plane Color Properties
*
* Plane Color management or color space adjustments is supported
* through a set of 5 properties on the &drm_plane object.
*
* degamma_lut_property:
*     Blob property which allows a userspace to provide LUT values
*     to apply degamma curve using the h/w plane degamma processing
*     engine, thereby making the content as linear for further color
*     processing.
*
* degamma_lut_size_property:
*     Range Property to indicate size of the plane degamma LUT.
*
* ctm_property:
*    Blob property which allows a userspace to provide CTM coefficients
*    to do color space conversion or any other enhancement by doing a
*    matrix multiplication using the h/w CTM processing engine
*
* gamma_lut_property:
*    Blob property which allows a userspace to provide LUT values
*    to apply gamma/tone-mapping curve using the h/w plane gamma
*    processing engine, thereby making the content as non-linear
*    or to perform any tone mapping operation for HDR usecases.
*
* gamma_lut_size_property:
*    Range Property to indicate size of the plane gamma LUT.
*/
static int drm_plane_color_create_prop(struct drm_device *dev,
                        struct linlondp_plane *plane)
{
    struct drm_property *prop;

    prop = drm_property_create(dev, DRM_MODE_PROP_BLOB,
            "PLANE_DEGAMMA_LUT", 0);
    if (!prop)
        return -ENOMEM;

    plane->degamma_lut_property = prop;

    prop = drm_property_create_range(dev, DRM_MODE_PROP_IMMUTABLE,
            "PLANE_DEGAMMA_LUT_SIZE", 0,
            UINT_MAX);
    if (!prop)
        return -ENOMEM;
    plane->degamma_lut_size_property = prop;

    prop = drm_property_create(dev, DRM_MODE_PROP_BLOB,
            "PLANE_CTM", 0);
    if (!prop)
        return -ENOMEM;

    plane->ctm_property = prop;

    prop = drm_property_create(dev, DRM_MODE_PROP_BLOB,
            "PLANE_CTM_EXT", 0);
    if (!prop)
        return -ENOMEM;

    plane->ctm_ext_property = prop;

    prop = drm_property_create(dev, DRM_MODE_PROP_BLOB,
            "PLANE_GAMMA_LUT", 0);
    if (!prop)
        return -ENOMEM;

    plane->gamma_lut_property = prop;

    prop = drm_property_create_range(dev, DRM_MODE_PROP_IMMUTABLE,
            "PLANE_GAMMA_LUT_SIZE", 0,
            UINT_MAX);
    if (!prop)
        return -ENOMEM;

    plane->gamma_lut_size_property = prop;

    return 0;
}

static int linlondp_plane_create_layer_split_property(struct linlondp_plane *kplane)
{
    struct drm_plane *plane = &kplane->base;
    struct drm_property *prop;

    prop = drm_property_create_bool(plane->dev, 0,
                    "LAYER_SPLIT");
    if (!prop)
        return -ENOMEM;

    drm_object_attach_property(&plane->base, prop, 0);
    kplane->layer_split_property = prop;

    return 0;
}

static int linlondp_plane_add(struct linlondp_kms_dev *kms,
                struct linlondp_layer *layer)
{
    struct linlondp_dev *mdev = kms->base.dev_private;
    struct linlondp_component *c = &layer->base;
    struct linlondp_color_manager *color_mgr;
    struct linlondp_plane *kplane;
    struct drm_plane *plane;
    u32 *formats, n_formats = 0;
    int err;

    kplane = kzalloc(sizeof(*kplane), GFP_KERNEL);
    if (!kplane)
        return -ENOMEM;

    plane = &kplane->base;
    kplane->layer = layer;
        kplane->force_layer_split = false;

    formats = linlondp_get_layer_fourcc_list(&mdev->fmt_tbl,
                           layer->layer_type, &n_formats);

    err = drm_universal_plane_init(&kms->base, plane,
            get_possible_crtcs(kms, c->pipeline),
            &linlondp_plane_funcs,
            formats, n_formats, linlondp_supported_modifiers,
            get_plane_type(kms, c),
            "%s", c->name);

    linlondp_put_fourcc_list(formats);

    if (err)
        goto cleanup;

    drm_plane_helper_add(plane, &linlondp_plane_helper_funcs);

    err = drm_plane_create_rotation_property(plane, DRM_MODE_ROTATE_0,
                         layer->supported_rots);
    if (err)
        goto cleanup;

    err = drm_plane_create_alpha_property(plane);
    if (err)
        goto cleanup;

    err = drm_plane_create_blend_mode_property(plane,
            BIT(DRM_MODE_BLEND_PIXEL_NONE) |
            BIT(DRM_MODE_BLEND_PREMULTI)   |
            BIT(DRM_MODE_BLEND_COVERAGE));
    if (err)
        goto cleanup;

    err = drm_plane_create_color_properties(plane,
            BIT(DRM_COLOR_YCBCR_BT601) |
            BIT(DRM_COLOR_YCBCR_BT709) |
            BIT(DRM_COLOR_YCBCR_BT2020),
            BIT(DRM_COLOR_YCBCR_LIMITED_RANGE) |
            BIT(DRM_COLOR_YCBCR_FULL_RANGE),
            DRM_COLOR_YCBCR_BT601,
            DRM_COLOR_YCBCR_LIMITED_RANGE);
    if (err)
        goto cleanup;

    err = drm_plane_color_create_prop(plane->dev, kplane);
    if (err)
        goto cleanup;

    color_mgr = &layer->color_mgr;

    drm_plane_enable_color_mgmt(kplane,
            color_mgr->igamma_mgr ? LINLONDP_COLOR_LUT_SIZE : 0,
            color_mgr->has_ctm,
            color_mgr->fgamma_mgr ? LINLONDP_COLOR_LUT_SIZE : 0);

    err = drm_plane_create_zpos_property(plane, layer->base.id, 0, 8);
    if (err)
        goto cleanup;

    err = linlondp_plane_create_layer_split_property(kplane);
    if (err)
        goto cleanup;

    linlondp_set_crtc_plane_mask(kms, c->pipeline, plane);

    return 0;
cleanup:
    linlondp_plane_destroy(plane);
    return err;
}

int linlondp_kms_add_planes(struct linlondp_kms_dev *kms, struct linlondp_dev *mdev)
{
    struct linlondp_pipeline *pipe;
    int i, j, err;

    for (i = 0; i < mdev->n_pipelines; i++) {
        pipe = mdev->pipelines[i];

        for (j = 0; j < pipe->n_layers; j++) {
            err = linlondp_plane_add(kms, pipe->layers[j]);
            if (err)
                return err;
        }
    }

    return 0;
}
