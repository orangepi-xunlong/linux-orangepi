// SPDX-License-Identifier: GPL-2.0
/*
 * (C) COPYRIGHT 2022-2023 Arm Technology (China) Co., Ltd.
 * ALL RIGHTS RESERVED
 *
 */
#include "linlondp_dev.h"
#include "linlondp_kms.h"

static void
linlondp_component_state_reset(struct linlondp_component_state *st)
{
    st->binding_user = NULL;
    st->affected_inputs = st->active_inputs;
    st->active_inputs = 0;
    st->changed_active_inputs = 0;
}

static struct drm_private_state *
linlondp_layer_atomic_duplicate_state(struct drm_private_obj *obj)
{
    struct linlondp_layer_state *old = to_layer_st(priv_to_comp_st(obj->state));
    struct linlondp_layer_state *st;

    st = kmemdup(obj->state, sizeof(*st), GFP_KERNEL);
    if (!st)
        return NULL;

    linlondp_color_duplicate_state(&st->color_st, &old->color_st);

    linlondp_component_state_reset(&st->base);
    __drm_atomic_helper_private_obj_duplicate_state(obj, &st->base.obj);

    return &st->base.obj;
}

static void
linlondp_layer_atomic_destroy_state(struct drm_private_obj *obj,
                  struct drm_private_state *state)
{
    struct linlondp_layer_state *st = to_layer_st(priv_to_comp_st(state));

    kfree(st);
}

static const struct drm_private_state_funcs linlondp_layer_obj_funcs = {
    .atomic_duplicate_state    = linlondp_layer_atomic_duplicate_state,
    .atomic_destroy_state    = linlondp_layer_atomic_destroy_state,
};

static int linlondp_layer_obj_add(struct linlondp_kms_dev *kms,
                struct linlondp_layer *layer)
{
    struct linlondp_layer_state *st;

    st = kzalloc(sizeof(*st), GFP_KERNEL);
    if (!st)
        return -ENOMEM;

    st->base.component = &layer->base;
    drm_atomic_private_obj_init(&kms->base, &layer->base.obj, &st->base.obj,
                    &linlondp_layer_obj_funcs);
    return 0;
}

static struct drm_private_state *
linlondp_scaler_atomic_duplicate_state(struct drm_private_obj *obj)
{
    struct linlondp_scaler_state *st;

    st = kmemdup(obj->state, sizeof(*st), GFP_KERNEL);
    if (!st)
        return NULL;

    linlondp_component_state_reset(&st->base);
    __drm_atomic_helper_private_obj_duplicate_state(obj, &st->base.obj);

    return &st->base.obj;
}

static void
linlondp_scaler_atomic_destroy_state(struct drm_private_obj *obj,
                   struct drm_private_state *state)
{
    kfree(to_scaler_st(priv_to_comp_st(state)));
}

static const struct drm_private_state_funcs linlondp_scaler_obj_funcs = {
    .atomic_duplicate_state    = linlondp_scaler_atomic_duplicate_state,
    .atomic_destroy_state    = linlondp_scaler_atomic_destroy_state,
};

static int linlondp_scaler_obj_add(struct linlondp_kms_dev *kms,
                 struct linlondp_scaler *scaler)
{
    struct linlondp_scaler_state *st;

    st = kzalloc(sizeof(*st), GFP_KERNEL);
    if (!st)
        return -ENOMEM;

    st->base.component = &scaler->base;
    drm_atomic_private_obj_init(&kms->base,
                    &scaler->base.obj, &st->base.obj,
                    &linlondp_scaler_obj_funcs);
    return 0;
}

static struct drm_private_state *
linlondp_compiz_atomic_duplicate_state(struct drm_private_obj *obj)
{
    struct linlondp_compiz_state *st;

    st = kmemdup(obj->state, sizeof(*st), GFP_KERNEL);
    if (!st)
        return NULL;

    linlondp_component_state_reset(&st->base);
    __drm_atomic_helper_private_obj_duplicate_state(obj, &st->base.obj);

    return &st->base.obj;
}

static void
linlondp_compiz_atomic_destroy_state(struct drm_private_obj *obj,
                   struct drm_private_state *state)
{
    kfree(to_compiz_st(priv_to_comp_st(state)));
}

static const struct drm_private_state_funcs linlondp_compiz_obj_funcs = {
    .atomic_duplicate_state    = linlondp_compiz_atomic_duplicate_state,
    .atomic_destroy_state    = linlondp_compiz_atomic_destroy_state,
};

static int linlondp_compiz_obj_add(struct linlondp_kms_dev *kms,
                 struct linlondp_compiz *compiz)
{
    struct linlondp_compiz_state *st;

    st = kzalloc(sizeof(*st), GFP_KERNEL);
    if (!st)
        return -ENOMEM;

    st->base.component = &compiz->base;
    drm_atomic_private_obj_init(&kms->base, &compiz->base.obj, &st->base.obj,
                    &linlondp_compiz_obj_funcs);

    return 0;
}

static struct drm_private_state *
linlondp_splitter_atomic_duplicate_state(struct drm_private_obj *obj)
{
    struct linlondp_splitter_state *st;

    st = kmemdup(obj->state, sizeof(*st), GFP_KERNEL);
    if (!st)
        return NULL;

    linlondp_component_state_reset(&st->base);
    __drm_atomic_helper_private_obj_duplicate_state(obj, &st->base.obj);

    return &st->base.obj;
}

static void
linlondp_splitter_atomic_destroy_state(struct drm_private_obj *obj,
                     struct drm_private_state *state)
{
    kfree(to_splitter_st(priv_to_comp_st(state)));
}

static const struct drm_private_state_funcs linlondp_splitter_obj_funcs = {
    .atomic_duplicate_state    = linlondp_splitter_atomic_duplicate_state,
    .atomic_destroy_state    = linlondp_splitter_atomic_destroy_state,
};

static int linlondp_splitter_obj_add(struct linlondp_kms_dev *kms,
                   struct linlondp_splitter *splitter)
{
    struct linlondp_splitter_state *st;

    st = kzalloc(sizeof(*st), GFP_KERNEL);
    if (!st)
        return -ENOMEM;

    st->base.component = &splitter->base;
    drm_atomic_private_obj_init(&kms->base,
                    &splitter->base.obj, &st->base.obj,
                    &linlondp_splitter_obj_funcs);

    return 0;
}

static struct drm_private_state *
linlondp_merger_atomic_duplicate_state(struct drm_private_obj *obj)
{
    struct linlondp_merger_state *st;

    st = kmemdup(obj->state, sizeof(*st), GFP_KERNEL);
    if (!st)
        return NULL;

    linlondp_component_state_reset(&st->base);
    __drm_atomic_helper_private_obj_duplicate_state(obj, &st->base.obj);

    return &st->base.obj;
}

static void linlondp_merger_atomic_destroy_state(struct drm_private_obj *obj,
                           struct drm_private_state *state)
{
    kfree(to_merger_st(priv_to_comp_st(state)));
}

static const struct drm_private_state_funcs linlondp_merger_obj_funcs = {
    .atomic_duplicate_state    = linlondp_merger_atomic_duplicate_state,
    .atomic_destroy_state    = linlondp_merger_atomic_destroy_state,
};

static int linlondp_merger_obj_add(struct linlondp_kms_dev *kms,
                 struct linlondp_merger *merger)
{
    struct linlondp_merger_state *st;

    st = kzalloc(sizeof(*st), GFP_KERNEL);
    if (!st)
        return -ENOMEM;

    st->base.component = &merger->base;
    drm_atomic_private_obj_init(&kms->base,
                    &merger->base.obj, &st->base.obj,
                    &linlondp_merger_obj_funcs);

    return 0;
}

static struct drm_private_state *
linlondp_improc_atomic_duplicate_state(struct drm_private_obj *obj)
{
    struct linlondp_improc_state *st;

    st = kmemdup(obj->state, sizeof(*st), GFP_KERNEL);
    if (!st)
        return NULL;

    linlondp_component_state_reset(&st->base);
    __drm_atomic_helper_private_obj_duplicate_state(obj, &st->base.obj);

    return &st->base.obj;
}

static void
linlondp_improc_atomic_destroy_state(struct drm_private_obj *obj,
                   struct drm_private_state *state)
{
    kfree(to_improc_st(priv_to_comp_st(state)));
}

static const struct drm_private_state_funcs linlondp_improc_obj_funcs = {
    .atomic_duplicate_state    = linlondp_improc_atomic_duplicate_state,
    .atomic_destroy_state    = linlondp_improc_atomic_destroy_state,
};

static int linlondp_improc_obj_add(struct linlondp_kms_dev *kms,
                 struct linlondp_improc *improc)
{
    struct linlondp_improc_state *st;

    st = kzalloc(sizeof(*st), GFP_KERNEL);
    if (!st)
        return -ENOMEM;

    st->base.component = &improc->base;
    drm_atomic_private_obj_init(&kms->base, &improc->base.obj, &st->base.obj,
                    &linlondp_improc_obj_funcs);

    return 0;
}

static struct drm_private_state *
linlondp_timing_ctrlr_atomic_duplicate_state(struct drm_private_obj *obj)
{
    struct linlondp_timing_ctrlr_state *st;

    st = kmemdup(obj->state, sizeof(*st), GFP_KERNEL);
    if (!st)
        return NULL;

    linlondp_component_state_reset(&st->base);
    __drm_atomic_helper_private_obj_duplicate_state(obj, &st->base.obj);

    return &st->base.obj;
}

static void
linlondp_timing_ctrlr_atomic_destroy_state(struct drm_private_obj *obj,
                     struct drm_private_state *state)
{
    kfree(to_ctrlr_st(priv_to_comp_st(state)));
}

static const struct drm_private_state_funcs linlondp_timing_ctrlr_obj_funcs = {
    .atomic_duplicate_state    = linlondp_timing_ctrlr_atomic_duplicate_state,
    .atomic_destroy_state    = linlondp_timing_ctrlr_atomic_destroy_state,
};

static int linlondp_timing_ctrlr_obj_add(struct linlondp_kms_dev *kms,
                       struct linlondp_timing_ctrlr *ctrlr)
{
    struct linlondp_compiz_state *st;

    st = kzalloc(sizeof(*st), GFP_KERNEL);
    if (!st)
        return -ENOMEM;

    st->base.component = &ctrlr->base;
    drm_atomic_private_obj_init(&kms->base, &ctrlr->base.obj, &st->base.obj,
                    &linlondp_timing_ctrlr_obj_funcs);

    return 0;
}

static struct drm_private_state *
linlondp_pipeline_atomic_duplicate_state(struct drm_private_obj *obj)
{
    struct linlondp_pipeline_state *st;

    st = kmemdup(obj->state, sizeof(*st), GFP_KERNEL);
    if (!st)
        return NULL;

    st->active_comps = 0;

    __drm_atomic_helper_private_obj_duplicate_state(obj, &st->obj);

    return &st->obj;
}

static void
linlondp_pipeline_atomic_destroy_state(struct drm_private_obj *obj,
                     struct drm_private_state *state)
{
    kfree(priv_to_pipe_st(state));
}

static const struct drm_private_state_funcs linlondp_pipeline_obj_funcs = {
    .atomic_duplicate_state    = linlondp_pipeline_atomic_duplicate_state,
    .atomic_destroy_state    = linlondp_pipeline_atomic_destroy_state,
};

static int linlondp_pipeline_obj_add(struct linlondp_kms_dev *kms,
                   struct linlondp_pipeline *pipe)
{
    struct linlondp_pipeline_state *st;

    st = kzalloc(sizeof(*st), GFP_KERNEL);
    if (!st)
        return -ENOMEM;

    st->pipe = pipe;
    drm_atomic_private_obj_init(&kms->base, &pipe->obj, &st->obj,
                    &linlondp_pipeline_obj_funcs);

    return 0;
}

int linlondp_kms_add_private_objs(struct linlondp_kms_dev *kms,
                struct linlondp_dev *mdev)
{
    struct linlondp_pipeline *pipe;
    int i, j, err;

    for (i = 0; i < mdev->n_pipelines; i++) {
        pipe = mdev->pipelines[i];

        err = linlondp_pipeline_obj_add(kms, pipe);
        if (err)
            return err;

        for (j = 0; j < pipe->n_layers; j++) {
            err = linlondp_layer_obj_add(kms, pipe->layers[j]);
            if (err)
                return err;
        }

        if (pipe->wb_layer) {
            err = linlondp_layer_obj_add(kms, pipe->wb_layer);
            if (err)
                return err;
        }

        for (j = 0; j < pipe->n_scalers; j++) {
            err = linlondp_scaler_obj_add(kms, pipe->scalers[j]);
            if (err)
                return err;
        }

        err = linlondp_compiz_obj_add(kms, pipe->compiz);
        if (err)
            return err;

        if (pipe->splitter) {
            err = linlondp_splitter_obj_add(kms, pipe->splitter);
            if (err)
                return err;
        }

        if (pipe->merger) {
            err = linlondp_merger_obj_add(kms, pipe->merger);
            if (err)
                return err;
        }

        err = linlondp_improc_obj_add(kms, pipe->improc);
        if (err)
            return err;

        err = linlondp_timing_ctrlr_obj_add(kms, pipe->ctrlr);
        if (err)
            return err;
    }

    return 0;
}

void linlondp_kms_cleanup_private_objs(struct linlondp_kms_dev *kms)
{
    struct drm_mode_config *config = &kms->base.mode_config;
    struct drm_private_obj *obj, *next;

    list_for_each_entry_safe(obj, next, &config->privobj_list, head)
        drm_atomic_private_obj_fini(obj);
}
