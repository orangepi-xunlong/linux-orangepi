// SPDX-License-Identifier: GPL-2.0
// Copyright 2024 Cix Technology Group Co., Ltd.
#include "hdac.h"
#include "hdacodec.h"

#include <linux/init.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/module.h>
#include <linux/export.h>
#include <linux/pm.h>
#include <linux/pm_runtime.h>
#include <sound/core.h>
#include <sound/hda_codec.h>
#include <sound/soc.h>
#include <sound/tlv.h>
#include <sound/pcm_params.h>
#include <linux/acpi.h>
#include <linux/of_platform.h>

#include <linux/mod_devicetable.h>

#include "hda_local.h"
#include "hda_jack.h"
#include <sound/info.h>

#define DEV_NAME_LEN 32

const char *snd_hda_pcm_type_name[HDA_PCM_NTYPES] = {
	"Audio", "SPDIF", "HDMI", "Modem"
};
static unsigned int proc_res;

static struct hda_codec *hda_codec_device_init(struct hdac_bus *bus, int addr);

/*
 * read widget caps for each widget and store in cache
 */
static int read_widget_caps(struct hda_codec *codec, hda_nid_t fg_node)
{
	int i;
	hda_nid_t nid;

	codec->wcaps = kmalloc_array(codec->core.num_nodes, 4, GFP_KERNEL);
	if (!codec->wcaps)
		return -ENOMEM;
	nid = codec->core.start_nid;
	for (i = 0; i < codec->core.num_nodes; i++, nid++)
		codec->wcaps[i] = snd_hdac_read_parm_uncached(&codec->core,
					nid, AC_PAR_AUDIO_WIDGET_CAP);
	return 0;
}

/* read all pin default configurations and save codec->init_pins */
static int read_pin_defaults(struct hda_codec *codec)
{
	hda_nid_t nid;

	for_each_hda_codec_node(nid, codec) {
		struct hda_pincfg *pin;
		unsigned int wcaps = get_wcaps(codec, nid);
		unsigned int wid_type = get_wcaps_type(wcaps);

		if (wid_type != AC_WID_PIN)
			continue;
		pin = snd_array_new(&codec->init_pins);
		if (!pin)
			return -ENOMEM;
		pin->nid = nid;
		pin->cfg = snd_hda_codec_read(codec, nid, 0,
					      AC_VERB_GET_CONFIG_DEFAULT, 0);
		/*
		 * all device entries are the same widget control so far
		 * fixme: if any codec is different, need fix here
		 */
		pin->ctrl = snd_hda_codec_read(codec, nid, 0,
					       AC_VERB_GET_PIN_WIDGET_CONTROL,
					       0);
	}
	return 0;
}

/* look up the given pin config list and return the item matching with NID */
static struct hda_pincfg *look_up_pincfg(struct hda_codec *codec,
					 struct snd_array *array,
					 hda_nid_t nid)
{
	struct hda_pincfg *pin;
	int i;

	snd_array_for_each(array, i, pin) {
		if (pin->nid == nid)
			return pin;
	}
	return NULL;
}

/**
 * snd_hda_codec_get_pincfg - Obtain a pin-default configuration
 * @codec: the HDA codec
 * @nid: NID to get the pin config
 *
 * Get the current pin config value of the given pin NID.
 * If the pincfg value is cached or overridden via sysfs or driver,
 * returns the cached value.
 */
unsigned int snd_hda_codec_get_pincfg(struct hda_codec *codec, hda_nid_t nid)
{
	struct hda_pincfg *pin;

#ifdef CONFIG_SND_HDA_RECONFIG
	{
		unsigned int cfg = 0;

		mutex_lock(&codec->user_mutex);
		pin = look_up_pincfg(codec, &codec->user_pins, nid);
		if (pin)
			cfg = pin->cfg;
		mutex_unlock(&codec->user_mutex);
		if (cfg)
			return cfg;
	}
#endif
	pin = look_up_pincfg(codec, &codec->driver_pins, nid);
	if (pin)
		return pin->cfg;
	pin = look_up_pincfg(codec, &codec->init_pins, nid);
	if (pin)
		return pin->cfg;
	return 0;
}
EXPORT_SYMBOL_GPL(snd_hda_codec_get_pincfg);

/* find a mixer control element with the given name */
static struct snd_kcontrol *
find_mixer_ctl(struct hda_codec *codec, const char *name, int dev, int idx)
{
	struct snd_card *card = get_hda_card();
	struct snd_ctl_elem_id id;

	if (!card)
		return NULL;

	memset(&id, 0, sizeof(id));
	id.iface = SNDRV_CTL_ELEM_IFACE_MIXER;
	id.device = dev;
	id.index = idx;
	if (snd_BUG_ON(strlen(name) >= sizeof(id.name)))
		return NULL;
	strcpy(id.name, name);

	return snd_ctl_find_id(codec->card, &id);
}

/**
 * snd_hda_find_mixer_ctl - Find a mixer control element with the given name
 * @codec: HD-audio codec
 * @name: ctl id name string
 *
 * Get the control element with the given id string and IFACE_MIXER.
 */
struct snd_kcontrol *snd_hda_find_mixer_ctl(struct hda_codec *codec,
					    const char *name)
{
	return find_mixer_ctl(codec, name, 0, 0);
}
EXPORT_SYMBOL_GPL(snd_hda_find_mixer_ctl);

static int find_empty_mixer_ctl_idx(struct hda_codec *codec, const char *name,
				    int start_idx)
{
	int i, idx;
	/* 16 ctlrs should be large enough */
	for (i = 0, idx = start_idx; i < 16; i++, idx++) {
		if (!find_mixer_ctl(codec, name, 0, idx))
			return idx;
	}
	return -EBUSY;
}

/**
 * snd_hda_ctl_add - Add a control element and assign to the codec
 * @codec: HD-audio codec
 * @nid: corresponding NID (optional)
 * @kctl: the control element to assign
 *
 * Add the given control element to an array inside the codec instance.
 * All control elements belonging to a codec are supposed to be added
 * by this function so that a proper clean-up works at the free or
 * reconfiguration time.
 *
 * If non-zero @nid is passed, the NID is assigned to the control element.
 * The assignment is shown in the codec proc file.
 *
 * snd_hda_ctl_add() checks the control subdev id field whether
 * #HDA_SUBDEV_NID_FLAG bit is set.  If set (and @nid is zero), the lower
 * bits value is taken as the NID to assign. The #HDA_NID_ITEM_AMP bit
 * specifies if kctl->private_value is a HDA amplifier value.
 */
int snd_hda_ctl_add(struct hda_codec *codec, hda_nid_t nid,
		    struct snd_kcontrol *kctl)
{
	int err;
	unsigned short flags = 0;
	struct hda_nid_item *item;

	if (kctl->id.subdevice & HDA_SUBDEV_AMP_FLAG) {
		flags |= HDA_NID_ITEM_AMP;
		if (nid == 0)
			nid = get_amp_nid_(kctl->private_value);
	}
	if ((kctl->id.subdevice & HDA_SUBDEV_NID_FLAG) != 0 && nid == 0)
		nid = kctl->id.subdevice & 0xffff;
	if (kctl->id.subdevice & (HDA_SUBDEV_NID_FLAG|HDA_SUBDEV_AMP_FLAG))
		kctl->id.subdevice = 0;
	err = snd_ctl_add(codec->card, kctl);
	if (err < 0)
		return err;
	item = snd_array_new(&codec->mixers);
	if (!item)
		return -ENOMEM;
	item->kctl = kctl;
	item->nid = nid;
	item->flags = flags;
	return 0;
}
EXPORT_SYMBOL_GPL(snd_hda_ctl_add);

/**
 * snd_hda_ctls_clear - Clear all controls assigned to the given codec
 * @codec: HD-audio codec
 */
void snd_hda_ctls_clear(struct hda_codec *codec)
{
	int i;
	struct hda_nid_item *items = codec->mixers.list;

	down_write(&codec->card->controls_rwsem);
	for (i = 0; i < codec->mixers.used; i++)
		snd_ctl_remove(codec->card, items[i].kctl);
	up_write(&codec->card->controls_rwsem);
	snd_array_free(&codec->mixers);
	snd_array_free(&codec->nids);
}

/**
 * snd_hda_add_new_ctls - create controls from the array
 * @codec: the HDA codec
 * @knew: the array of struct snd_kcontrol_new
 *
 * This helper function creates and add new controls in the given array.
 * The array must be terminated with an empty entry as terminator.
 *
 * Returns 0 if successful, or a negative error code.
 */
int snd_hda_add_new_ctls(struct hda_codec *codec,
			 const struct snd_kcontrol_new *knew)
{
	int err;

	for (; knew->name; knew++) {
		struct snd_kcontrol *kctl;
		int addr = 0, idx = 0;

		if (knew->iface == (__force snd_ctl_elem_iface_t)-1)
			continue; /* skip this codec private value */
		for (;;) {
			kctl = snd_ctl_new1(knew, codec);
			if (!kctl)
				return -ENOMEM;
			/* Do not use the id.device field for MIXER elements.
			 * This field is for real device numbers (like PCM) but codecs
			 * are hidden components from the user space view (unrelated
			 * to the mixer element identification).
			 */
			if (addr > 0 && codec->ctl_dev_id)
				kctl->id.device = addr;
			if (idx > 0)
				kctl->id.index = idx;
			err = snd_hda_ctl_add(codec, 0, kctl);
			if (!err)
				break;
			/* try first with another device index corresponding to
			 * the codec addr; if it still fails (or it's the
			 * primary codec), then try another control index
			 */
			if (!addr && codec->core.addr) {
				addr = codec->core.addr;
				if (!codec->ctl_dev_id)
					idx += 10 * addr;
			} else if (!idx && !knew->index) {
				idx = find_empty_mixer_ctl_idx(codec,
							       knew->name, 0);
				if (idx <= 0)
					return err;
			} else
				return err;
		}
	}
	return 0;
}
EXPORT_SYMBOL_GPL(snd_hda_add_new_ctls);

/*
 * amp access functions
 */

/**
 * query_amp_caps - query AMP capabilities
 * @codec: the HD-auio codec
 * @nid: the NID to query
 * @direction: either #HDA_INPUT or #HDA_OUTPUT
 *
 * Query AMP capabilities for the given widget and direction.
 * Returns the obtained capability bits.
 *
 * When cap bits have been already read, this doesn't read again but
 * returns the cached value.
 */
u32 query_amp_caps(struct hda_codec *codec, hda_nid_t nid, int direction)
{
	if (!(get_wcaps(codec, nid) & AC_WCAP_AMP_OVRD))
		nid = codec->core.afg;
	return snd_hda_param_read(codec, nid,
				  direction == HDA_OUTPUT ?
				  AC_PAR_AMP_OUT_CAP : AC_PAR_AMP_IN_CAP);
}
EXPORT_SYMBOL_GPL(query_amp_caps);

static unsigned int encode_amp(struct hda_codec *codec, hda_nid_t nid,
			       int ch, int dir, int idx)
{
	unsigned int cmd = snd_hdac_regmap_encode_amp(nid, ch, dir, idx);

	/* enable fake mute if no h/w mute but min=mute */
	if ((query_amp_caps(codec, nid, dir) &
	     (AC_AMPCAP_MUTE | AC_AMPCAP_MIN_MUTE)) == AC_AMPCAP_MIN_MUTE)
		cmd |= AC_AMP_FAKE_MUTE;
	return cmd;
}

/**
 * snd_hda_codec_amp_update - update the AMP mono value
 * @codec: HD-audio codec
 * @nid: NID to read the AMP value
 * @ch: channel to update (0 or 1)
 * @dir: #HDA_INPUT or #HDA_OUTPUT
 * @idx: the index value (only for input direction)
 * @mask: bit mask to set
 * @val: the bits value to set
 *
 * Update the AMP values for the given channel, direction and index.
 */
int snd_hda_codec_amp_update(struct hda_codec *codec, hda_nid_t nid,
			     int ch, int dir, int idx, int mask, int val)
{
	unsigned int cmd = encode_amp(codec, nid, ch, dir, idx);

	return snd_hdac_regmap_update_raw(&codec->core, cmd, mask, val);
}
EXPORT_SYMBOL_GPL(snd_hda_codec_amp_update);

static u32 get_amp_max_value(struct hda_codec *codec, hda_nid_t nid, int dir,
			     unsigned int ofs)
{
	u32 caps = query_amp_caps(codec, nid, dir);

	/* get num steps */
	caps = (caps & AC_AMPCAP_NUM_STEPS) >> AC_AMPCAP_NUM_STEPS_SHIFT;
	if (ofs < caps)
		caps -= ofs;
	return caps;
}

/**
 * snd_hda_mixer_amp_volume_info - Info callback for a standard AMP mixer
 * @kcontrol: referred ctl element
 * @uinfo: pointer to get/store the data
 *
 * The control element is supposed to have the private_value field
 * set up via HDA_COMPOSE_AMP_VAL*() or related macros.
 */
int snd_hda_mixer_amp_volume_info(struct snd_kcontrol *kcontrol,
				  struct snd_ctl_elem_info *uinfo)
{
	struct hda_codec *codec = snd_kcontrol_chip(kcontrol);
	u16 nid = get_amp_nid(kcontrol);
	u8 chs = get_amp_channels(kcontrol);
	int dir = get_amp_direction(kcontrol);
	unsigned int ofs = get_amp_offset(kcontrol);

	uinfo->type = SNDRV_CTL_ELEM_TYPE_INTEGER;
	uinfo->count = chs == 3 ? 2 : 1;
	uinfo->value.integer.min = 0;
	uinfo->value.integer.max = get_amp_max_value(codec, nid, dir, ofs);

	if (!uinfo->value.integer.max) {
		codec_warn(codec,
			   "num_steps = 0 for NID=0x%x (ctl = %s)\n",
			   nid, kcontrol->id.name);
		return -EINVAL;
	}
	return 0;
}
EXPORT_SYMBOL_GPL(snd_hda_mixer_amp_volume_info);

static inline unsigned int
read_amp_value(struct hda_codec *codec, hda_nid_t nid,
	       int ch, int dir, int idx, unsigned int ofs)
{
	unsigned int val;

	val = snd_hda_codec_amp_read(codec, nid, ch, dir, idx);
	val &= HDA_AMP_VOLMASK;
	if (val >= ofs)
		val -= ofs;
	else
		val = 0;
	return val;
}

static inline int
update_amp_value(struct hda_codec *codec, hda_nid_t nid,
		 int ch, int dir, int idx, unsigned int ofs,
		 unsigned int val)
{
	unsigned int maxval;

	if (val > 0)
		val += ofs;
	/* ofs = 0: raw max value */
	maxval = get_amp_max_value(codec, nid, dir, 0);
	if (val > maxval)
		val = maxval;
	return snd_hda_codec_amp_update(codec, nid, ch, dir, idx,
					HDA_AMP_VOLMASK, val);
}

/**
 * snd_hda_mixer_amp_volume_get - Get callback for a standard AMP mixer volume
 * @kcontrol: ctl element
 * @ucontrol: pointer to get/store the data
 *
 * The control element is supposed to have the private_value field
 * set up via HDA_COMPOSE_AMP_VAL*() or related macros.
 */
int snd_hda_mixer_amp_volume_get(struct snd_kcontrol *kcontrol,
				 struct snd_ctl_elem_value *ucontrol)
{
	struct hda_codec *codec = snd_kcontrol_chip(kcontrol);
	hda_nid_t nid = get_amp_nid(kcontrol);
	int chs = get_amp_channels(kcontrol);
	int dir = get_amp_direction(kcontrol);
	int idx = get_amp_index(kcontrol);
	unsigned int ofs = get_amp_offset(kcontrol);
	long *valp = ucontrol->value.integer.value;

	if (chs & 1)
		*valp++ = read_amp_value(codec, nid, 0, dir, idx, ofs);
	if (chs & 2)
		*valp = read_amp_value(codec, nid, 1, dir, idx, ofs);
	return 0;
}
EXPORT_SYMBOL_GPL(snd_hda_mixer_amp_volume_get);

/**
 * snd_hda_mixer_amp_volume_put - Put callback for a standard AMP mixer volume
 * @kcontrol: ctl element
 * @ucontrol: pointer to get/store the data
 *
 * The control element is supposed to have the private_value field
 * set up via HDA_COMPOSE_AMP_VAL*() or related macros.
 */
int snd_hda_mixer_amp_volume_put(struct snd_kcontrol *kcontrol,
				 struct snd_ctl_elem_value *ucontrol)
{
	struct hda_codec *codec = snd_kcontrol_chip(kcontrol);
	hda_nid_t nid = get_amp_nid(kcontrol);
	int chs = get_amp_channels(kcontrol);
	int dir = get_amp_direction(kcontrol);
	int idx = get_amp_index(kcontrol);
	unsigned int ofs = get_amp_offset(kcontrol);
	long *valp = ucontrol->value.integer.value;
	int change = 0;

	if (chs & 1) {
		change = update_amp_value(codec, nid, 0, dir, idx, ofs, *valp);
		valp++;
	}
	if (chs & 2)
		change |= update_amp_value(codec, nid, 1, dir, idx, ofs, *valp);
	return change;
}
EXPORT_SYMBOL_GPL(snd_hda_mixer_amp_volume_put);

/* inquiry the amp caps and convert to TLV */
static void get_ctl_amp_tlv(struct snd_kcontrol *kcontrol, unsigned int *tlv)
{
	struct hda_codec *codec = snd_kcontrol_chip(kcontrol);
	hda_nid_t nid = get_amp_nid(kcontrol);
	int dir = get_amp_direction(kcontrol);
	unsigned int ofs = get_amp_offset(kcontrol);
	bool min_mute = get_amp_min_mute(kcontrol);
	u32 caps, val1, val2;

	caps = query_amp_caps(codec, nid, dir);
	val2 = (caps & AC_AMPCAP_STEP_SIZE) >> AC_AMPCAP_STEP_SIZE_SHIFT;
	val2 = (val2 + 1) * 25;
	val1 = -((caps & AC_AMPCAP_OFFSET) >> AC_AMPCAP_OFFSET_SHIFT);
	val1 += ofs;
	val1 = ((int)val1) * ((int)val2);
	if (min_mute || (caps & AC_AMPCAP_MIN_MUTE))
		val2 |= TLV_DB_SCALE_MUTE;
	tlv[SNDRV_CTL_TLVO_TYPE] = SNDRV_CTL_TLVT_DB_SCALE;
	tlv[SNDRV_CTL_TLVO_LEN] = 2 * sizeof(unsigned int);
	tlv[SNDRV_CTL_TLVO_DB_SCALE_MIN] = val1;
	tlv[SNDRV_CTL_TLVO_DB_SCALE_MUTE_AND_STEP] = val2;
}

/**
 * snd_hda_mixer_amp_tlv - TLV callback for a standard AMP mixer volume
 * @kcontrol: ctl element
 * @op_flag: operation flag
 * @size: byte size of input TLV
 * @_tlv: TLV data
 *
 * The control element is supposed to have the private_value field
 * set up via HDA_COMPOSE_AMP_VAL*() or related macros.
 */
int snd_hda_mixer_amp_tlv(struct snd_kcontrol *kcontrol, int op_flag,
			  unsigned int size, unsigned int __user *_tlv)
{
	unsigned int tlv[4];

	if (size < 4 * sizeof(unsigned int))
		return -ENOMEM;
	get_ctl_amp_tlv(kcontrol, tlv);
	if (copy_to_user(_tlv, tlv, sizeof(tlv)))
		return -EFAULT;
	return 0;
}
EXPORT_SYMBOL_GPL(snd_hda_mixer_amp_tlv);

/**
 * snd_hda_set_vmaster_tlv - Set TLV for a virtual master control
 * @codec: HD-audio codec
 * @nid: NID of a reference widget
 * @dir: #HDA_INPUT or #HDA_OUTPUT
 * @tlv: TLV data to be stored, at least 4 elements
 *
 * Set (static) TLV data for a virtual master volume using the AMP caps
 * obtained from the reference NID.
 * The volume range is recalculated as if the max volume is 0dB.
 */
void snd_hda_set_vmaster_tlv(struct hda_codec *codec, hda_nid_t nid, int dir,
			     unsigned int *tlv)
{
	u32 caps;
	int nums, step;

	caps = query_amp_caps(codec, nid, dir);
	nums = (caps & AC_AMPCAP_NUM_STEPS) >> AC_AMPCAP_NUM_STEPS_SHIFT;
	step = (caps & AC_AMPCAP_STEP_SIZE) >> AC_AMPCAP_STEP_SIZE_SHIFT;
	step = (step + 1) * 25;
	tlv[SNDRV_CTL_TLVO_TYPE] = SNDRV_CTL_TLVT_DB_SCALE;
	tlv[SNDRV_CTL_TLVO_LEN] = 2 * sizeof(unsigned int);
	tlv[SNDRV_CTL_TLVO_DB_SCALE_MIN] = -nums * step;
	tlv[SNDRV_CTL_TLVO_DB_SCALE_MUTE_AND_STEP] = step;
}
EXPORT_SYMBOL_GPL(snd_hda_set_vmaster_tlv);

/**
 * snd_hda_mixer_amp_switch_info - Info callback for a standard AMP mixer switch
 * @kcontrol: referred ctl element
 * @uinfo: pointer to get/store the data
 *
 * The control element is supposed to have the private_value field
 * set up via HDA_COMPOSE_AMP_VAL*() or related macros.
 */
int snd_hda_mixer_amp_switch_info(struct snd_kcontrol *kcontrol,
				  struct snd_ctl_elem_info *uinfo)
{
	int chs = get_amp_channels(kcontrol);

	uinfo->type = SNDRV_CTL_ELEM_TYPE_BOOLEAN;
	uinfo->count = chs == 3 ? 2 : 1;
	uinfo->value.integer.min = 0;
	uinfo->value.integer.max = 1;
	return 0;
}
EXPORT_SYMBOL_GPL(snd_hda_mixer_amp_switch_info);

/**
 * snd_hda_mixer_amp_switch_get - Get callback for a standard AMP mixer switch
 * @kcontrol: ctl element
 * @ucontrol: pointer to get/store the data
 *
 * The control element is supposed to have the private_value field
 * set up via HDA_COMPOSE_AMP_VAL*() or related macros.
 */
int snd_hda_mixer_amp_switch_get(struct snd_kcontrol *kcontrol,
				 struct snd_ctl_elem_value *ucontrol)
{
	struct hda_codec *codec = snd_kcontrol_chip(kcontrol);
	hda_nid_t nid = get_amp_nid(kcontrol);
	int chs = get_amp_channels(kcontrol);
	int dir = get_amp_direction(kcontrol);
	int idx = get_amp_index(kcontrol);
	long *valp = ucontrol->value.integer.value;

	if (pm_runtime_status_suspended(&codec->core.dev))
		return 0;

	if (chs & 1)
		*valp++ = (snd_hda_codec_amp_read(codec, nid, 0, dir, idx) &
			   HDA_AMP_MUTE) ? 1 : 0;
	if (chs & 2)
		*valp = (snd_hda_codec_amp_read(codec, nid, 1, dir, idx) &
			 HDA_AMP_MUTE) ? 1 : 0;
	return 0;
}
EXPORT_SYMBOL_GPL(snd_hda_mixer_amp_switch_get);

/**
 * snd_hda_mixer_amp_switch_put - Put callback for a standard AMP mixer switch
 * @kcontrol: ctl element
 * @ucontrol: pointer to get/store the data
 *
 * The control element is supposed to have the private_value field
 * set up via HDA_COMPOSE_AMP_VAL*() or related macros.
 */
int snd_hda_mixer_amp_switch_put(struct snd_kcontrol *kcontrol,
				 struct snd_ctl_elem_value *ucontrol)
{
	struct hda_codec *codec = snd_kcontrol_chip(kcontrol);

	hda_nid_t nid = get_amp_nid(kcontrol);
	int chs = get_amp_channels(kcontrol);
	int dir = get_amp_direction(kcontrol);
	int idx = get_amp_index(kcontrol);
	long *valp = ucontrol->value.integer.value;
	int change = 0;

	if (pm_runtime_status_suspended(&codec->core.dev))
		return 0;

	if (chs & 1) {
		change = snd_hda_codec_amp_update(codec, nid, 0, dir, idx,
						  HDA_AMP_MUTE,
						  *valp ? HDA_AMP_MUTE : 0);
		valp++;
	}
	if (chs & 2)
		change |= snd_hda_codec_amp_update(codec, nid, 1, dir, idx,
						   HDA_AMP_MUTE,
						   *valp ? HDA_AMP_MUTE : 0);
	hda_call_check_power_status(codec, nid);
	return change;
}
EXPORT_SYMBOL_GPL(snd_hda_mixer_amp_switch_put);

int snd_hda_codec_set_name(struct hda_codec *codec, const char *name)
{
	int err;

	if (!name)
		return 0;
	err = snd_hdac_device_set_chip_name(&codec->core, name);
	if (err < 0)
		return err;

	return 0;
}

static int hda_codec_config(struct hda_codec_priv *hcp, int stream,
			    struct snd_pcm_hw_params *params)
{
	struct hda_codec *codec = hcp->codec;
	struct hda_codec_pdata *codec_pdata = hcp->codec_pdata;
	struct hdac_device *hdev = &codec->core;
	unsigned int format_val, maxbps, i;
	hda_nid_t nid;

	if (codec_pdata && codec_pdata->ops->hw_params)
		codec_pdata->ops->hw_params(codec, stream);

	switch (params_format(params)) {
	case SNDRV_PCM_FORMAT_S16_LE:
		maxbps = 16;
		break;
	case SNDRV_PCM_FORMAT_S24_LE:
		maxbps = 24;
		break;
	default:
		maxbps = 32;
		break;
	}

	format_val = snd_hdac_calc_stream_format(params_rate(params),
						 params_channels(params),
						 params_format(params),
						 maxbps,
						 0);

	nid = hdev->start_nid;
	for (i = 0; i < hdev->num_nodes; i++, nid++) {
		unsigned int type;

		type = get_wcaps_type(get_wcaps(codec, nid));
		if (stream == SNDRV_PCM_STREAM_PLAYBACK) {
			if (type == AC_WID_AUD_OUT) {
				snd_hda_codec_write(codec, nid, 0,
						    AC_VERB_SET_STREAM_FORMAT,
						    format_val);
				continue;
			}
		} else {
			if (type == AC_WID_AUD_IN) {
				snd_hda_codec_write(codec, nid, 0,
						    AC_VERB_SET_STREAM_FORMAT,
						    format_val);
				continue;
			}
		}
	}

	return 0;
}

static int hda_codec_hw_params(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params, struct snd_soc_dai *dai)
{
	struct snd_soc_component *component = dai->component;
	struct hda_codec_priv *hcp = snd_soc_component_get_drvdata(component);
	struct hda_codec *codec = hcp->codec;

	if (!codec) {
		dev_err(component->dev, "no codec setups yet");
		return -EINVAL;
	}

	dev_info(component->dev, "bitdepth:%d, channels:%d, rate:%d\n",
		 params_width(params),
		 params_channels(params),
		 params_rate(params));

	hcp->codec_params[substream->stream] =
		devm_kzalloc(hcp->dev, sizeof(struct snd_pcm_hw_params),
			     GFP_KERNEL);
	if (!hcp->codec_params[substream->stream])
		return -ENOMEM;

	memcpy(hcp->codec_params[substream->stream],
	       params, sizeof(struct snd_pcm_hw_params));

	return hda_codec_config(hcp, substream->stream, params);
}

static int hda_codec_hw_free(struct snd_pcm_substream *substream,
			     struct snd_soc_dai *dai)
{
	struct snd_soc_component *component = dai->component;
	struct hda_codec_priv *hcp = snd_soc_component_get_drvdata(component);

	if (hcp->codec_params[substream->stream]) {
		struct hda_codec *codec = hcp->codec;
		struct hda_codec_pdata *codec_pdata = hcp->codec_pdata;

		if (codec_pdata && codec_pdata->ops->hw_free)
			codec_pdata->ops->hw_free(codec, substream->stream);

		devm_kfree(hcp->dev, hcp->codec_params[substream->stream]);
		hcp->codec_params[substream->stream] = NULL;
	}

	return 0;
}

static const struct snd_soc_dai_ops hda_codec_dai_ops = {
	.hw_params = hda_codec_hw_params,
	.hw_free   = hda_codec_hw_free,
};

static const struct snd_soc_component_driver hda_codec_dev = {
	.use_pmdown_time        = 1,
	.endianness             = 1,
};

static struct snd_soc_dai_driver hda_codec_dai[] = {
	{
		.name = HDA_CODEC_DRV_NAME,
		.playback = {
			.stream_name = "Playback",
			.channels_min = 2,
			.channels_max = 8,
			.rates = SNDRV_PCM_RATE_8000_192000,
			.formats =  HDA_CODEC_FMT,
		},
		.capture = {
			.stream_name = "Capture",
			.channels_min = 2,
			.channels_max = 8,
			.rates = SNDRV_PCM_RATE_8000_192000,
			.formats =  HDA_CODEC_FMT,
		},
		.ops = &hda_codec_dai_ops,
	}
};

static void snd_hda_codec_dev_release(struct device *dev)
{
	struct hda_codec *codec = dev_to_hda_codec(dev);

	snd_hdac_device_exit(&codec->core);
	kfree(codec->modelname);
	kfree(codec->wcaps);
	kfree(codec);
}

struct hda_codec *snd_hda_codec_device_init(struct hda_bus *bus,
	unsigned int codec_addr,
	const char *fmt, ...)
{
	va_list vargs;
	char name[DEV_NAME_LEN];
	struct hda_codec *codec;
	hda_nid_t fg;
	int err;

	if (snd_BUG_ON(!bus))
		return ERR_PTR(-EINVAL);
	if (snd_BUG_ON(codec_addr > HDA_MAX_CODEC_ADDRESS))
		return ERR_PTR(-EINVAL);

	codec = devm_kzalloc(bus->core.dev, sizeof(*codec), GFP_KERNEL);
	if (!codec)
		return ERR_PTR(-ENOMEM);

	va_start(vargs, fmt);
	vsprintf(name, fmt, vargs);
	va_end(vargs);

	err = snd_hdac_device_init(&codec->core, &bus->core, name, codec_addr);
	if (err < 0) {
		devm_kfree(bus->core.dev, codec);
		return ERR_PTR(err);
	}

	codec->bus = bus;
	codec->card = get_hda_card();
	codec->depop_delay = -1;
	codec->core.dev.release = snd_hda_codec_dev_release;
	codec->core.type = HDA_DEV_LEGACY;

	snd_array_init(&codec->mixers, sizeof(struct hda_nid_item), 32);
	snd_array_init(&codec->nids, sizeof(struct hda_nid_item), 32);
	snd_array_init(&codec->init_pins, sizeof(struct hda_pincfg), 16);
	snd_array_init(&codec->jacktbl, sizeof(struct hda_jack_tbl), 16);

	fg = codec->core.afg ? codec->core.afg : codec->core.mfg;
	err = read_widget_caps(codec, fg);
	if (err < 0)
		dev_warn(bus->core.dev,
			 "read pin widget caps error:%d\n", err);

	err = read_pin_defaults(codec);
	if (err < 0)
		dev_warn(bus->core.dev,
			 "read pin defaults error:%d\n", err);

	codec->core.registered = 1;

	return codec;
}

static struct hda_codec *hda_codec_device_init(struct hdac_bus *bus, int addr)
{
	struct hda_codec *codec;
	int ret;

	codec = snd_hda_codec_device_init(to_hda_bus(bus), addr,
					  "hdaudio%dD%d", bus->idx, addr);
	if (IS_ERR(codec)) {
		dev_err(bus->dev,
			"device init failed for hdac device\n");
		return codec;
	}

	codec->core.type = HDA_DEV_ASOC;

	ret = snd_hdac_device_register(&codec->core);
	if (ret) {
		dev_err(bus->dev,
			"failed to register hdac device\n");
		put_device(&codec->core.dev);
		return ERR_PTR(ret);
	}

	return codec;
}

int hda_codec_match(struct hdac_device *dev, struct hdac_driver *drv)
{
	return 1;
}

void hda_codec_unsol_event(struct hdac_device *dev, unsigned int event)
{
	struct hda_codec *codec = container_of(dev, struct hda_codec, core);

	snd_hda_jack_unsol_event(codec, event);
}

static struct hdac_driver hdacodec_driver = {
	.driver = {
		.name	= "HDA Codec",
	},
	.match		= hda_codec_match,
	.unsol_event	= hda_codec_unsol_event,
};

static void codecreg_read(struct snd_info_entry *entry,
			     struct snd_info_buffer *buffer)
{
	snd_iprintf(buffer, "%x\n", proc_res);
}

static void codecreg_set(struct snd_info_entry *entry,
			     struct snd_info_buffer *buffer)
{
	char line[64];
	unsigned int cmd, ret;
	struct 	hda_codec *codec = entry->private_data;
	struct hdac_bus *bus = codec->core.bus;
	unsigned int *res;
	res = &proc_res;

	if (!snd_info_get_line(buffer, line, sizeof(line))) {
		if (sscanf(line, "%x", &cmd) != 1)
			dev_err(codec->card->dev,"Invilid date input for codec set");
		ret = snd_hdac_bus_exec_verb_unlocked(bus, codec->addr, cmd, res);
		if (ret) {
			dev_err(codec->card->dev,"codec set failed by codecset node");
		}
	}
}

static void hda_codec_probe_work(struct work_struct *work)
{
	struct hda_codec_priv *hcp = container_of(work, struct hda_codec_priv, probe_work.work);
	struct snd_card *card = get_hda_card();
	int ret;

	if (!card) {
		mod_delayed_work(system_power_efficient_wq,
				 &hcp->probe_work, msecs_to_jiffies(250));
	} else {
		struct hda_codec *codec = hcp->codec;
		struct hdac_bus *bus = hcp->bus;

		codec->card = card;

		pm_runtime_get_sync(bus->dev);
		if (codec->patch_ops.build_controls)
			codec->patch_ops.build_controls(codec);

		ret = snd_card_rw_proc_new(card, "codecset", codec, codecreg_read,codecreg_set);
		if (ret)
			dev_err(hcp->dev,"codecset init failed, ret: %d",ret);

		ret = snd_hda_codec_proc_new(codec);
		if (ret)
			dev_err(hcp->dev,"codec proc init failed, ret: %d",ret);
		snd_info_card_register(card);

		pm_runtime_put(bus->dev);
	}
}

extern const struct of_device_id snd_hda_id_realtek_of_match[];

int hda_codec_probe(struct platform_device *pdev)
{
	struct hda_codec_bus_pdata *hcbd = pdev->dev.platform_data;
	struct device *dev = &pdev->dev;
	hda_codec_patch_t patch;
	struct hdac_bus *bus;
	struct hda_codec_priv *hcp;
	struct hda_codec_pdata *codec_pdata;
	struct hda_codec *codec;
	int ret;

	codec_pdata = (struct hda_codec_pdata *)device_get_match_data(dev);
	if (codec_pdata)
		patch = codec_pdata->data;

	hcp = devm_kzalloc(dev, sizeof(*hcp), GFP_KERNEL);
	if (!hcp)
		return -ENOMEM;

	if (hcbd) {
		hcp->hcbdata = *hcbd;
		bus = (struct hdac_bus *)(hcbd->bus);
	} else {
		dev_warn(dev,
			 "no platform data, get bus from controller data\n");

		bus = get_ipb_hda_bus();
		if (!bus)
			return -EPROBE_DEFER;
		hcp->codec_pdata = codec_pdata;
	}
	hcp->dev = dev;
	hcp->bus = bus;

	pm_runtime_get_sync(bus->dev);
	codec = hda_codec_device_init(bus, 0);
	if (IS_ERR(codec)) {
		dev_err(dev,
			"failed to create hdac codec, error:0x%x\n",
			PTR_ERR_OR_ZERO(codec));
		pm_runtime_put(bus->dev);
		return -EINVAL;
	}
	hcp->codec = codec;

	snd_hda_codec_set_name(codec, dev_name(dev));

	if (codec && patch) {
		ret = patch(codec);
		if (ret < 0) {
			dev_err(dev, "failed to patch codec\n");
			goto error_exit;
		}
	}
	pm_runtime_put(bus->dev);

	hcp->hdac_driver = &hdacodec_driver;
	snd_hda_ext_driver_register(hcp->hdac_driver);

	INIT_DELAYED_WORK(&hcp->probe_work, hda_codec_probe_work);

	pm_runtime_set_active(dev);
	pm_runtime_enable(dev);

	platform_set_drvdata(pdev, hcp);

	/* ASoC specific initialization */
	ret = devm_snd_soc_register_component(dev, &hda_codec_dev,
					      hda_codec_dai,
					      ARRAY_SIZE(hda_codec_dai));

	if (ret < 0) {
		dev_err(dev, "failed to register hda codec component");
		pm_runtime_disable(dev);
		return ret;
	}

	/* card not yet ready, try later */
	mod_delayed_work(system_power_efficient_wq,
			 &hcp->probe_work, msecs_to_jiffies(250));

	dev_info(dev, "%s, %s probed\n", __func__, HDA_CODEC_DRV_NAME);

	return ret;

error_exit:
	snd_hdac_device_unregister(&codec->core);
	if (codec->patch_ops.free)
		codec->patch_ops.free(codec);
	pm_runtime_put(bus->dev);

	return ret;
}

int snd_hda_get_devices(struct hda_codec *codec, hda_nid_t nid,
			u8 *dev_list, int max_devices)
{
	unsigned int parm;
	int i, dev_len, devices;

	parm = snd_hda_get_num_devices(codec, nid);
	if (!parm)	/* not multi-stream capable */
		return 0;

	dev_len = parm + 1;
	dev_len = dev_len < max_devices ? dev_len : max_devices;

	devices = 0;
	while (devices < dev_len) {
		if (snd_hdac_read(&codec->core, nid,
				  AC_VERB_GET_DEVICE_LIST, devices, &parm))
			break; /* error */

		for (i = 0; i < 8; i++) {
			dev_list[devices] = (u8)parm;
			parm >>= 4;
			devices++;
			if (devices >= dev_len)
				break;
		}
	}
	return devices;
}

/**
 * snd_hda_get_dev_select - get device entry select on the pin
 * @codec: the HDA codec
 * @nid: NID of the pin to get device entry select
 *
 * Get the devcie entry select on the pin. Return the device entry
 * id selected on the pin. Return 0 means the first device entry
 * is selected or MST is not supported.
 */
int snd_hda_get_dev_select(struct hda_codec *codec, hda_nid_t nid)
{
	 /* not support dp_mst will always return 0, using first dev_entry */
	 if (!codec->dp_mst)
		 return 0;

	 return snd_hda_codec_read(codec, nid, 0, AC_VERB_GET_DEVICE_SEL, 0);
}
EXPORT_SYMBOL_GPL(snd_hda_get_dev_select);

 /**
  * snd_hda_set_dev_select - set device entry select on the pin
  * @codec: the HDA codec
  * @nid: NID of the pin to set device entry select
  * @dev_id: device entry id to be set
  *
  * Set the device entry select on the pin nid.
  */
int snd_hda_set_dev_select(struct hda_codec *codec, hda_nid_t nid, int dev_id)
{
	 int ret, num_devices;

	 /* not support dp_mst will always return 0, using first dev_entry */
	 if (!codec->dp_mst)
		 return 0;

	 /* AC_PAR_DEVLIST_LEN is 0 based. */
	 num_devices = snd_hda_get_num_devices(codec, nid) + 1;
	 /* If Device List Length is 0 (num_device = 1),
	  * the pin is not multi stream capable.
	  * Do nothing in this case.
	  */
	 if (num_devices == 1)
		 return 0;

	 /* Behavior of setting index being equal to or greater than
	  * Device List Length is not predictable
	  */
	 if (num_devices <= dev_id)
		 return -EINVAL;

	 ret = snd_hda_codec_write(codec, nid, 0,
			 AC_VERB_SET_DEVICE_SEL, dev_id);

	 return ret;
}
EXPORT_SYMBOL_GPL(snd_hda_set_dev_select);

/**
 * snd_print_pcm_bits - Print the supported PCM fmt bits to the string buffer
 * @pcm: PCM caps bits
 * @buf: the string buffer to write
 * @buflen: the max buffer length
 *
 * used by hda_proc.c and hda_eld.c
 */
void snd_print_pcm_bits(int pcm, char *buf, int buflen)
{
	 static const unsigned int bits[] = { 8, 16, 20, 24, 32 };
	 int i, j;

	 for (i = 0, j = 0; i < ARRAY_SIZE(bits); i++)
		 if (pcm & (AC_SUPPCM_BITS_8 << i))
			 j += scnprintf(buf + j, buflen - j,  " %d", bits[i]);

	 buf[j] = '\0'; /* necessary when j == 0 */
}
EXPORT_SYMBOL_GPL(snd_print_pcm_bits);

/**
 * snd_hda_get_num_devices - get DEVLIST_LEN parameter of the given widget
 *  @codec: the HDA codec
 *  @nid: NID of the pin to parse
 *
 * Get the device entry number on the given widget. This is a feature of
 * DP MST audio. Each pin can have several device entries in it.
 */
unsigned int snd_hda_get_num_devices(struct hda_codec *codec, hda_nid_t nid)
{
	 unsigned int wcaps = get_wcaps(codec, nid);
	 unsigned int parm;

	 if (!codec->dp_mst || !(wcaps & AC_WCAP_DIGITAL) ||
		 get_wcaps_type(wcaps) != AC_WID_PIN)
		 return 0;

	 parm = snd_hdac_read_parm_uncached(&codec->core, nid, AC_PAR_DEVLIST_LEN);
	 if (parm == -1)
		 parm = 0;
	 return parm & AC_DEV_LIST_LEN_MASK;
}
EXPORT_SYMBOL_GPL(snd_hda_get_num_devices);

int hda_codec_remove(struct platform_device *pdev)
{
	struct hda_codec_priv *hcp = dev_get_drvdata(&pdev->dev);
	struct hda_codec *codec = hcp->codec;

	if (codec->patch_ops.free)
		codec->patch_ops.free(codec);

	pm_runtime_disable(hcp->dev);

	snd_hda_ext_driver_unregister(hcp->hdac_driver);
	cancel_delayed_work_sync(&hcp->probe_work);

	return 0;
}

int hda_codec_suspend(struct device *dev)
{
	struct hda_codec_priv *hcp = dev_get_drvdata(dev);
	struct hda_codec *codec = hcp->codec;
	struct hda_codec_pdata *codec_pdata = hcp->codec_pdata;
	struct hdac_bus *bus = hcp->bus;

	/* force codec to suspend, to save codec info. */
	if (codec && codec->patch_ops.resume) {
		pm_runtime_get_sync(bus->dev);

		if (codec_pdata && codec_pdata->ops->check_pm)
			codec_pdata->ops->check_pm(codec, 1);

		codec->patch_ops.suspend(codec);

		pm_runtime_put(bus->dev);
	}

	pm_runtime_force_suspend(dev);

	return 0;
}

int hda_codec_resume(struct device *dev)
{
	struct hda_codec_priv *hcp = dev_get_drvdata(dev);
	struct hda_codec *codec = hcp->codec;
	struct hda_codec_pdata *codec_pdata = hcp->codec_pdata;
	struct hdac_bus *bus = hcp->bus;
	int i;

	/* force codec to resume, then to init condec */
	if (codec && codec->patch_ops.resume) {
		pm_runtime_get_sync(bus->dev);

		for (i = 0; i < codec->init_verbs_size; i++)
			bus->ops->command(bus, codec->init_verbs[i]);

		codec->patch_ops.resume(codec);

		if (codec_pdata && codec_pdata->ops->check_pm)
			codec_pdata->ops->check_pm(codec, 0);

		pm_runtime_put(bus->dev);
	}

	pm_runtime_force_resume(dev);

	return 0;
}

int __maybe_unused hda_codec_runtime_suspend(struct device *dev)
{
	struct hda_codec_priv *hcp = dev_get_drvdata(dev);
	struct hda_codec *codec = hcp->codec;

	if (codec) {
		if (codec->patch_ops.suspend) {
			struct hdac_bus *bus = hcp->bus;

			pm_runtime_get_sync(bus->dev);
			codec->patch_ops.suspend(codec);
			pm_runtime_put(bus->dev);
		}
	}

	return 0;
}

int __maybe_unused hda_codec_runtime_resume(struct device *dev)
{
	struct hda_codec_priv *hcp = dev_get_drvdata(dev);
	struct hda_codec *codec = hcp->codec;
	struct hdac_bus *bus = hcp->bus;
	int i;

	if (codec) {
		pm_runtime_get_sync(bus->dev);

		if (codec->patch_ops.resume)
			codec->patch_ops.resume(codec);

		for (i = 0; i < SNDRV_PCM_STREAM_LAST + 1; i++) {
			if (hcp->codec_params[i])
				hda_codec_config(hcp, i, hcp->codec_params[i]);
		}

		pm_runtime_put(bus->dev);
	}

	return 0;
}

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("HDA codec Driver");
MODULE_AUTHOR("Xing Wang<xing.wang@cixtech.com>");
