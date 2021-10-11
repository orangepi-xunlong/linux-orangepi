/*
 * Kernel based bootsplash.
 *
 * (Loading and freeing functions)
 *
 * Authors:
 * Max Staudt <mstaudt@suse.de>
 *
 * SPDX-License-Identifier: GPL-2.0
 */

#define pr_fmt(fmt) "bootsplash: " fmt


#include <linux/bootsplash.h>
#include <linux/fb.h>
#include <linux/firmware.h>
#include <linux/kernel.h>
#include <linux/mutex.h>
#include <linux/printk.h>
#include <linux/types.h>
#include <linux/vmalloc.h>

#include "bootsplash_internal.h"
#include "uapi/linux/bootsplash_file.h"




/*
 * Free all vmalloc()'d resources describing a splash file.
 */
void bootsplash_free_file(struct splash_file_priv *fp)
{
	if (!fp)
		return;

	if (fp->pics) {
		unsigned int i;

		for (i = 0; i < fp->header->num_pics; i++) {
			struct splash_pic_priv *pp = &fp->pics[i];

			if (pp->blobs)
				vfree(pp->blobs);
		}

		vfree(fp->pics);
	}

	release_firmware(fp->fw);
	vfree(fp);
}




/*
 * Load a splash screen from a "firmware" file.
 *
 * Parsing, and sanity checks.
 */
#ifdef __BIG_ENDIAN
	#define BOOTSPLASH_MAGIC BOOTSPLASH_MAGIC_BE
#else
	#define BOOTSPLASH_MAGIC BOOTSPLASH_MAGIC_LE
#endif

struct splash_file_priv *bootsplash_load_firmware(struct device *device,
						  const char *path)
{
	const struct firmware *fw;
	struct splash_file_priv *fp;
	bool have_anim = false;
	unsigned int i;
	const u8 *walker;

	if (request_firmware(&fw, path, device))
		return NULL;

	if (fw->size < sizeof(struct splash_file_header)
	    || memcmp(fw->data, BOOTSPLASH_MAGIC, sizeof(fp->header->id))) {
		pr_err("Not a bootsplash file.\n");

		release_firmware(fw);
		return NULL;
	}

	fp = vzalloc(sizeof(struct splash_file_priv));
	if (!fp) {
		release_firmware(fw);
		return NULL;
	}

	pr_info("Loading splash file (%li bytes)\n", fw->size);

	fp->fw = fw;
	fp->header = (struct splash_file_header *)fw->data;

	/* Sanity checks */
	if (fp->header->version != BOOTSPLASH_VERSION) {
		pr_err("Loaded v%d file, but we only support version %d\n",
			fp->header->version,
			BOOTSPLASH_VERSION);

		goto err;
	}

	if (fw->size < sizeof(struct splash_file_header)
		+ fp->header->num_pics
			* sizeof(struct splash_pic_header)
		+ fp->header->num_blobs
			* sizeof(struct splash_blob_header)) {
		pr_err("File incomplete.\n");

		goto err;
	}

	/* Read picture headers */
	if (fp->header->num_pics) {
		fp->pics = vzalloc(fp->header->num_pics
				   * sizeof(struct splash_pic_priv));
		if (!fp->pics)
			goto err;
	}

	walker = fw->data + sizeof(struct splash_file_header);
	for (i = 0; i < fp->header->num_pics; i++) {
		struct splash_pic_priv *pp = &fp->pics[i];
		struct splash_pic_header *ph = (void *)walker;

		pr_debug("Picture %u: Size %ux%u\n", i, ph->width, ph->height);

		if (ph->num_blobs < 1) {
			pr_err("Picture %u: Zero blobs? Aborting load.\n", i);
			goto err;
		}

		if (ph->anim_type > SPLASH_ANIM_LOOP_FORWARD) {
			pr_warn("Picture %u: Unsupported animation type %u.\n",
				i, ph->anim_type);

			ph->anim_type = SPLASH_ANIM_NONE;
		}

		pp->pic_header = ph;
		pp->blobs = vzalloc(ph->num_blobs
					* sizeof(struct splash_blob_priv));
		if (!pp->blobs)
			goto err;

		walker += sizeof(struct splash_pic_header);
	}

	/* Read blob headers */
	for (i = 0; i < fp->header->num_blobs; i++) {
		struct splash_blob_header *bh = (void *)walker;
		struct splash_pic_priv *pp;

		if (walker + sizeof(struct splash_blob_header)
		    > fw->data + fw->size)
			goto err;

		walker += sizeof(struct splash_blob_header);

		if (walker + bh->length > fw->data + fw->size)
			goto err;

		if (bh->picture_id >= fp->header->num_pics)
			goto nextblob;

		pp = &fp->pics[bh->picture_id];

		pr_debug("Blob %u, pic %u, blobs_loaded %u, num_blobs %u.\n",
			 i, bh->picture_id,
			 pp->blobs_loaded, pp->pic_header->num_blobs);

		if (pp->blobs_loaded >= pp->pic_header->num_blobs)
			goto nextblob;

		switch (bh->type) {
		case 0:
			/* Raw 24-bit packed pixels */
			if (bh->length != pp->pic_header->width
					* pp->pic_header->height * 3) {
				pr_err("Blob %u, type 1: Length doesn't match picture.\n",
				       i);

				goto err;
			}
			break;
		default:
			pr_warn("Blob %u, unknown type %u.\n", i, bh->type);
			goto nextblob;
		}

		pp->blobs[pp->blobs_loaded].blob_header = bh;
		pp->blobs[pp->blobs_loaded].data = walker;
		pp->blobs_loaded++;

nextblob:
		walker += bh->length;
		if (bh->length % 16)
			walker += 16 - (bh->length % 16);
	}

	if (walker != fw->data + fw->size)
		pr_warn("Trailing data in splash file.\n");

	/* Walk over pictures and ensure all blob slots are filled */
	for (i = 0; i < fp->header->num_pics; i++) {
		struct splash_pic_priv *pp = &fp->pics[i];
		const struct splash_pic_header *ph = pp->pic_header;

		if (pp->blobs_loaded != pp->pic_header->num_blobs) {
			pr_err("Picture %u doesn't have all blob slots filled.\n",
			       i);

			goto err;
		}

		if (ph->anim_type
		    && ph->num_blobs > 1
		    && ph->anim_loop < pp->blobs_loaded)
			have_anim = true;
	}

	if (!have_anim)
		/* Disable animation timer if there is nothing to animate */
		fp->frame_ms = 0;
	else
		/* Enforce minimum delay between frames */
		fp->frame_ms = max((u16)20, fp->header->frame_ms);

	pr_info("Loaded (%ld bytes, %u pics, %u blobs).\n",
		fw->size,
		fp->header->num_pics,
		fp->header->num_blobs);

	return fp;


err:
	bootsplash_free_file(fp);
	return NULL;
}
