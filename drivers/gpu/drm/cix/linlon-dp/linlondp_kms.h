/* SPDX-License-Identifier: GPL-2.0 */
/*
 * (C) COPYRIGHT 2022-2023 Arm Technology (China) Co., Ltd.
 * ALL RIGHTS RESERVED
 *
 */
#ifndef _LINLONDP_KMS_H_
#define _LINLONDP_KMS_H_

#include <linux/list.h>
#include <drm/drm_atomic.h>
#include <drm/drm_atomic_helper.h>
#include <drm/drm_blend.h>
#include <drm/drm_crtc_helper.h>
#include <drm/drm_device.h>
#include <drm/drm_writeback.h>
#include <drm/drm_print.h>

struct linlondp_events;
/**
 * struct linlondp_plane - linlondp instance of drm_plane
 */
struct linlondp_plane {
    /** @base: &drm_plane */
    struct drm_plane base;

    /**
     * @force_layer_split:
     *
     * debug flag, if true, force enable layer_split when this plane
     * has been enabled.
     */
    bool force_layer_split;
    /**
     * @layer:
     *
     * represents available layer input pipelines for this plane.
     *
     * NOTE:
     * the layer is not for a specific Layer, but indicate a group of
     * Layers with same capabilities.
     */
    struct linlondp_layer *layer;

    /**
    * @layer_split_property: Optional Plane property to enable
    * software layer split.
    */
    struct drm_property *layer_split_property;

    /**
    * @degamma_lut_property: Optional Plane property to set the LUT
    * used to convert the framebuffer's colors to linear gamma.
    */
    struct drm_property *degamma_lut_property;

    /**
    * @degamma_lut_size_property: Optional Plane property for the
    * size of the degamma LUT as supported by the driver (read-only).
    */
    struct drm_property *degamma_lut_size_property;

    /**
    * @plane_ctm_property: Optional Plane property to set the
    * matrix used to convert colors after the lookup in the
    * degamma LUT.
    */
    struct drm_property *ctm_property;
    /* ctm ext properties */
    struct drm_property *ctm_ext_property;

    /**
    * @plane_gamma_lut_property: Optional Plane property to set the LUT
    * used to convert the colors, after the CTM matrix, to the common
    * gamma space chosen for blending.
    */
    struct drm_property *gamma_lut_property;

    /**
    * @plane_gamma_lut_size_property: Optional Plane property for the size
    * of the gamma LUT as supported by the driver (read-only).
    */
    struct drm_property *gamma_lut_size_property;
};

/**
 * struct linlondp_plane_state
 *
 * The plane_state can be split into two data flow (left/right) and handled
 * by two layers &linlondp_plane.layer and &linlondp_plane.layer.right
 */
struct linlondp_plane_state {
    /** @base: &drm_plane_state */
    struct drm_plane_state base;
    /** @zlist_node: zorder list node */
    struct list_head zlist_node;

    /** @layer_split: on/off layer_split */
    u8 layer_split : 1;

    /* @degamma_lut:
    *
    * Lookup table for converting framebuffer pixel data before apply the
    * color conversion matrix @ctm. See drm_plane_enable_color_mgmt(). The
    * blob (if not NULL) is an array of &struct drm_color_lut_ext.
    */
    struct drm_property_blob *degamma_lut;

    /**
    * @ctm:
    *
    * Color transformation matrix. See drm_plane_enable_color_mgmt(). The
    * blob (if not NULL) is a &struct drm_color_ctm.
    */
    struct drm_property_blob *ctm;

    /**
    * @gamma_lut:
    *
    * Lookup table for converting pixel data after the color conversion
    * matrix @ctm.  See drm_plane_enable_color_mgmt(). The blob (if not
    * NULL) is an array of &struct drm_color_lut_ext.
    */
    struct drm_property_blob *gamma_lut;

    u8 color_mgmt_changed : 1;
};

/**
 * struct linlondp_wb_connector
 */
struct linlondp_wb_connector {
    /** @base: &drm_writeback_connector */
    struct drm_writeback_connector base;

    /**
     * @force_scaling_split:
     *
     * Debug flag, if true, force enable split for the scaling enabled
     * writeback job.
     */
    bool force_scaling_split;

    /** @wb_layer: represents associated writeback pipeline of linlondp */
    struct linlondp_layer *wb_layer;

    /*
     * Bitmask that tells for which pipeline we should expect a writeback
     * interrupt to come.
     */
    int expected_eow;
    /*
     * Bitmask that tells for which pipeline we already received a writeback
     * interrupt. Every time expected_eow == received_eow we signal
     * the writeback_job and set received_eow to 0.
     */
    atomic_t received_eow;

    /**
     * @color_encoding_property: enum property for specifying color encoding
     * for non RGB formats for writeback layer.
     */
    struct drm_property *color_encoding_property;
    /**
     * @color_range_property: enum property for specifying color range for
     * non RGB formats for writeback layer.
     */
    struct drm_property *color_range_property;

};

/**
 * struct linlondp_wb_connector_state
 */
struct linlondp_wb_connector_state {
    struct drm_connector_state base;

    enum drm_color_encoding color_encoding;
    enum drm_color_range color_range;
};

/**
 * struct linlondp_crtc
 */
struct linlondp_crtc {
    /** @base: &drm_crtc */
    struct drm_crtc base;
    /** @master: only master has display output */
    struct linlondp_pipeline *master;
    /**
     * @slave: optional
     *
     * Doesn't have its own display output, the handled data flow will
     * merge into the master.
     */
    struct linlondp_pipeline *slave;

    /** @side_by_side: if the master and slave works on side by side mode */
    bool side_by_side;

    /** @sbs_overlap: amount of overlap in side by side mode */
    u32 sbs_overlap;
    /** @slave_planes: linlondp slave planes mask */
    u32 slave_planes;

    /** @wb_conn: linlondp write back connector */
    struct linlondp_wb_connector *wb_conn;

    /** @disable_done: this flip_done is for tracing the disable */
    struct completion *disable_done;
    /* protected mode property */
    struct drm_property *protected_mode_property;
    /* ctm ext properties */
    struct drm_property *ctm_ext_property;
};

/**
 * struct linlondp_crtc_state
 */
struct linlondp_crtc_state {
    /** @base: &drm_crtc_state */
    struct drm_crtc_state base;

    /* private properties */

    /* computed state which are used by validate/check */
    /**
     * @affected_pipes:
     * the affected pipelines in once display instance
     */
    u32 affected_pipes;
    /**
     * @active_pipes:
     * the active pipelines in once display instance
     */
    u32 active_pipes;

    /** @clock_ratio: ratio of (aclk << 32)/pxlclk */
    u64 clock_ratio;

    /** @max_slave_zorder: the maximum of slave zorder */
    u32 max_slave_zorder;
    bool en_protected_mode;
};

/** struct linlondp_kms_dev - for gather KMS related things */
struct linlondp_kms_dev {
    /** @base: &drm_device */
    struct drm_device base;

    /** @n_crtcs: valid numbers of crtcs in &linlondp_kms_dev.crtcs */
    int n_crtcs;
    /** @crtcs: crtcs list */
    struct linlondp_crtc crtcs[LINLONDP_MAX_PIPELINES];
};

#define to_kplane(p)    container_of(p, struct linlondp_plane, base)
#define to_kplane_st(p)    container_of(p, struct linlondp_plane_state, base)
#define to_kconn(p)    container_of(p, struct linlondp_wb_connector, base)
#define to_kcrtc(p)    container_of(p, struct linlondp_crtc, base)
#define to_kcrtc_st(p)    container_of(p, struct linlondp_crtc_state, base)
#define to_kdev(p)    container_of(p, struct linlondp_kms_dev, base)
#define to_wb_conn(x)    container_of(x, struct drm_writeback_connector, base)
#define to_kconn_st(p)    container_of(p, struct linlondp_wb_connector_state, base)

#define _drm_conn_to_kconn(c)   to_kconn(to_wb_conn((c)))

static inline bool is_writeback_only(struct drm_crtc_state *st)
{
    struct linlondp_wb_connector *wb_conn = to_kcrtc(st->crtc)->wb_conn;
    struct drm_connector *conn = wb_conn ? &wb_conn->base.base : NULL;

    return conn && (st->connector_mask == BIT(drm_connector_index(conn)));
}

static inline bool
is_only_changed_connector(struct drm_crtc_state *st, struct drm_connector *conn)
{
    struct drm_crtc_state *old_st;
    u32 changed_connectors;

    old_st = drm_atomic_get_old_crtc_state(st->state, st->crtc);
    changed_connectors = st->connector_mask ^ old_st->connector_mask;

    return BIT(drm_connector_index(conn)) == changed_connectors;
}

static inline bool has_flip_h(u32 rot)
{
    u32 rotation = drm_rotation_simplify(rot,
                         DRM_MODE_ROTATE_0 |
                         DRM_MODE_ROTATE_90 |
                         DRM_MODE_REFLECT_MASK);

    if (rotation & DRM_MODE_ROTATE_90)
        return !!(rotation & DRM_MODE_REFLECT_Y);
    else
        return !!(rotation & DRM_MODE_REFLECT_X);
}

void linlondp_crtc_get_color_config(struct drm_crtc_state *crtc_st,
                  u32 *color_depths, u32 *color_formats);
unsigned long linlondp_crtc_get_aclk(struct linlondp_crtc_state *kcrtc_st);

int linlondp_kms_setup_crtcs(struct linlondp_kms_dev *kms, struct linlondp_dev *mdev);

int linlondp_kms_add_crtcs(struct linlondp_kms_dev *kms, struct linlondp_dev *mdev);
int linlondp_kms_add_planes(struct linlondp_kms_dev *kms, struct linlondp_dev *mdev);
int linlondp_kms_add_private_objs(struct linlondp_kms_dev *kms,
                struct linlondp_dev *mdev);
int linlondp_kms_add_wb_connectors(struct linlondp_kms_dev *kms,
                 struct linlondp_dev *mdev);
void linlondp_kms_cleanup_private_objs(struct linlondp_kms_dev *kms);

void linlondp_crtc_handle_event(struct linlondp_crtc   *kcrtc,
                  struct linlondp_events *evts);
void linlondp_crtc_flush_and_wait_for_flip_done(struct linlondp_crtc *kcrtc,
                  struct completion *input_flip_done);

struct linlondp_kms_dev *linlondp_kms_attach(struct linlondp_dev *mdev);
void linlondp_kms_detach(struct linlondp_kms_dev *kms);

#endif /*_LINLONDP_KMS_H_*/
