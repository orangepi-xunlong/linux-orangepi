/*************************************************************************/ /*!
@Title          Kernel versions compatibility macros
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    Per-version macros to allow code to seamlessly use older kernel
@License        Dual MIT/GPLv2

The contents of this file are subject to the MIT license as set out below.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

Alternatively, the contents of this file may be used under the terms of
the GNU General Public License Version 2 ("GPL") in which case the provisions
of GPL are applicable instead of those above.

If you wish to allow use of your version of this file only under the terms of
GPL, and not to allow others to use your version of this file under the terms
of the MIT license, indicate your decision by deleting the provisions above
and replace them with the notice and other provisions required by GPL as set
out in the file called "GPL-COPYING" included in this distribution. If you do
not delete the provisions above, a recipient may use your version of this file
under the terms of either the MIT license or GPL.

This License is also included in this distribution in the file called
"MIT-COPYING".

EXCEPT AS OTHERWISE STATED IN A NEGOTIATED AGREEMENT: (A) THE SOFTWARE IS
PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
PURPOSE AND NONINFRINGEMENT; AND (B) IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/ /**************************************************************************/

#ifndef __KERNEL_COMPATIBILITY_H__
#define __KERNEL_COMPATIBILITY_H__

#include <linux/version.h>
#include <linux/compiler.h>

/* Explicitly error out if DDK is built against out-of-support Linux kernel */
#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 9, 0))
#error Linux kernels older than 4.9.0 are not supported
#endif

/*
 * Stop supporting an old kernel? Remove the top block.
 * New incompatible kernel?       Append a new block at the bottom.
 *
 * Please write your version test as `VERSION < X.Y`, and use the earliest
 * possible version :)
 *
 * If including this header file in other files, this should always be the
 * last file included, as it can affect definitions/declarations in files
 * included after it.
 */

#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 11, 0))
#define refcount_read(r) atomic_read(r)
#define drm_mm_insert_node(mm, node, size) drm_mm_insert_node(mm, node, size, 0, DRM_MM_SEARCH_DEFAULT)

#define drm_helper_mode_fill_fb_struct(dev, fb, mode_cmd) drm_helper_mode_fill_fb_struct(fb, mode_cmd)

/*
 * In Linux Kernels >= 4.12 for x86 another level of page tables has been
 * added. The added level (p4d) sits between pgd and pud, so when it
 * doesn`t exist, pud_offset function takes pgd as a parameter instead
 * of p4d.
 */
#define p4d_t pgd_t
#define p4d_offset(pgd, address) (pgd)
#define p4d_none(p4d) (0)
#define p4d_bad(p4d) (0)

#endif /* (LINUX_VERSION_CODE < KERNEL_VERSION(4, 11, 0)) */


#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 12, 0))

#define drm_mode_object_get(obj)          drm_mode_object_reference(obj)
#define drm_mode_object_put(obj)          drm_mode_object_unreference(obj)
#define drm_connector_get(obj)            drm_connector_reference(obj)
#define drm_connector_put(obj)            drm_connector_unreference(obj)
#define drm_framebuffer_get(obj)          drm_framebuffer_reference(obj)
#define drm_framebuffer_put(obj)          drm_framebuffer_unreference(obj)
#define drm_gem_object_get(obj)           drm_gem_object_reference(obj)
#define drm_gem_object_put_locked(obj)    drm_gem_object_unreference(obj)
#define __drm_gem_object_put(obj)         __drm_gem_object_unreference(obj)
#define drm_property_blob_get(obj)        drm_property_reference_blob(obj)
#define drm_property_blob_put(obj)        drm_property_unreference_blob(obj)

#endif /* (LINUX_VERSION_CODE < KERNEL_VERSION(4, 12, 0)) */


#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 14, 0))

#define drm_universal_plane_init(dev, plane, possible_crtcs, funcs, formats, format_count, format_modifiers, type, name, ...) \
	({ (void) format_modifiers; drm_universal_plane_init(dev, plane, possible_crtcs, funcs, formats, format_count, type, name, ##__VA_ARGS__); })

#endif /* (LINUX_VERSION_CODE < KERNEL_VERSION(4, 14, 0)) */


#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 15, 0))

#define drm_dev_put(dev) drm_dev_unref(dev)

#define drm_mode_object_find(dev, file_priv, id, type) drm_mode_object_find(dev, id, type)
#define drm_encoder_find(dev, file_priv, id) drm_encoder_find(dev, id)

#endif /* (LINUX_VERSION_CODE < KERNEL_VERSION(4, 15, 0)) */

#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 16, 0))

#define drm_atomic_helper_check_plane_state(plane_state, crtc_state, \
											min_scale, max_scale, \
											can_position, can_update_disabled) \
	({ \
		const struct drm_rect __clip = { \
			.x2 = crtc_state->crtc->mode.hdisplay, \
			.y2 = crtc_state->crtc->mode.vdisplay, \
		}; \
		int __ret = drm_plane_helper_check_state(plane_state, \
												 &__clip, \
												 min_scale, max_scale, \
												 can_position, \
												 can_update_disabled); \
		__ret; \
	})

#elif (LINUX_VERSION_CODE < KERNEL_VERSION(4, 17, 0))

#define drm_atomic_helper_check_plane_state(plane_state, crtc_state, \
											min_scale, max_scale, \
											can_position, can_update_disabled) \
	({ \
		const struct drm_rect __clip = { \
			.x2 = crtc_state->crtc->mode.hdisplay, \
			.y2 = crtc_state->crtc->mode.vdisplay, \
		}; \
		int __ret = drm_atomic_helper_check_plane_state(plane_state, \
														crtc_state, \
														&__clip, \
														min_scale, max_scale, \
														can_position, \
														can_update_disabled); \
		__ret; \
	})

#endif /* (LINUX_VERSION_CODE < KERNEL_VERSION(4, 17, 0)) */

#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 19, 0))

#define drm_connector_attach_encoder(connector, encoder) \
	drm_mode_connector_attach_encoder(connector, encoder)

#define drm_connector_update_edid_property(connector, edid) \
	drm_mode_connector_update_edid_property(connector, edid)

#endif /* (LINUX_VERSION_CODE < KERNEL_VERSION(4, 19, 0)) */


#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 0, 0))

/*
 * Work around architectures, e.g. MIPS, that define copy_from_user and
 * copy_to_user as macros that call access_ok, as this gets redefined below.
 * As of kernel 4.12, these functions are no longer defined per-architecture
 * so this work around isn't needed.
 */
#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 12, 0))
#if defined(copy_from_user)
 /*
  * NOTE: This function should not be called directly as it exists simply to
  * work around copy_from_user being defined as a macro that calls access_ok.
  */
static inline int
__pvr_copy_from_user(void *to, const void __user *from, unsigned long n)
{
	return copy_from_user(to, from, n);
}

#undef copy_from_user
#define copy_from_user(to, from, n) __copy_from_user(to, from, n)
#endif

#if defined(copy_to_user)
 /*
  * NOTE: This function should not be called directly as it exists simply to
  * work around copy_to_user being defined as a macro that calls access_ok.
  */
static inline int
__pvr_copy_to_user(void __user *to, const void *from, unsigned long n)
{
	return copy_to_user(to, from, n);
}

#undef copy_to_user
#define copy_to_user(to, from, n) __copy_to_user(to, from, n)
#endif
#endif

/*
 * Linux 5.0 dropped the type argument.
 *
 * This is unused in at least Linux 3.4 and above for all architectures other
 * than 'um' (User Mode Linux), which stopped using it in 4.2.
 */
#if defined(access_ok)
 /*
  * NOTE: This function should not be called directly as it exists simply to
  * work around access_ok being defined as a macro.
  */
static inline int
__pvr_access_ok_compat(int type, const void __user * addr, unsigned long size)
{
	return access_ok(type, addr, size);
}

#undef access_ok
#define access_ok(addr, size) __pvr_access_ok_compat(0, addr, size)
#else
#define access_ok(addr, size) access_ok(0, addr, size)
#endif

#endif

#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 3, 0))
#define MODULE_IMPORT_NS(ns)
#endif

/*
 * Before v5.8, the "struct mm" has a semaphore named "mmap_sem" which is
 * renamed to "mmap_lock" in v5.8. Moreover, new APIs are provided to
 * access this lock starting from v5.8.
 */
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 8, 0))

#define mmap_write_lock(mm)   down_write(&mm->mmap_sem)
#define mmap_write_unlock(mm) up_write(&mm->mmap_sem)

#define mmap_read_lock(mm)    down_read(&mm->mmap_sem)
#define mmap_read_unlock(mm)  up_read(&mm->mmap_sem)

#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(5, 8, 0) */

#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 12, 0))
#define drm_gem_object_put(obj) drm_gem_object_unreference_unlocked(obj)
#elif (LINUX_VERSION_CODE < KERNEL_VERSION(5, 9, 0))
#define drm_gem_object_put(obj) drm_gem_object_put_unlocked(obj)
#endif /* (LINUX_VERSION_CODE < KERNEL_VERSION(4, 12, 0)) */

#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0))

#define drm_prime_pages_to_sg(dev, pages, nr_pages) \
	drm_prime_pages_to_sg(pages, nr_pages)

#endif /* (LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0)) */

#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 11, 0))

struct dma_buf_map {
	void *vaddr;
};

#define dma_buf_vmap(dmabuf, map) \
	({ \
		(map)->vaddr = dma_buf_vmap(dmabuf); \
		(map)->vaddr ? 0 : ((dmabuf) && (dmabuf)->ops->vmap) ? -ENOMEM : -EINVAL; \
	})

#define dma_buf_vunmap(dmabuf, map) \
	({ \
		dma_buf_vunmap(dmabuf, (map)->vaddr); \
		(map)->vaddr = NULL; \
	})

#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(5, 11, 0) */

#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 12, 0))

#define drm_prime_sg_to_page_array(sgt, pages, npages) \
	drm_prime_sg_to_page_addr_arrays(sgt, pages, NULL, npages)

#endif /* (LINUX_VERSION_CODE < KERNEL_VERSION(5, 12, 0)) */

#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 13, 0))

#define drm_gem_plane_helper_prepare_fb drm_gem_fb_prepare_fb

#endif /* (LINUX_VERSION_CODE < KERNEL_VERSION(5, 13, 0)) */

/*
 * Linux 5.11 renames the privileged uaccess routines for arm64 and Android
 * kernel v5.10 merges the change as well. These routines are only used for
 * arm64 so CONFIG_ARM64 testing can be ignored.
 */
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0)) || \
	((LINUX_VERSION_CODE < KERNEL_VERSION(5, 11, 0)) && !defined(ANDROID))
#define uaccess_enable_privileged() uaccess_enable()
#define uaccess_disable_privileged() uaccess_disable()
#endif

#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 16, 0))
#define pde_data PDE_DATA
#endif /* (LINUX_VERSION_CODE < KERNEL_VERSION(5, 16, 0)) */

#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 17, 0))
#define kthread_complete_and_exit(comp, ret) complete_and_exit(comp, ret);
#endif /* (LINUX_VERSION_CODE < KERNEL_VERSION(5, 17, 0)) */

#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 18, 0))
#define iosys_map dma_buf_map
#define iosys_map_set_vaddr dma_buf_map_set_vaddr
#define iosys_map_set_vaddr_iomem dma_buf_map_set_vaddr_iomem
#define iosys_map_clear dma_buf_map_clear
#endif /* (LINUX_VERSION_CODE < KERNEL_VERSION(5, 18, 0)) */

#if (LINUX_VERSION_CODE < KERNEL_VERSION(6, 0, 0))

#define register_shrinker(shrinker, name) \
	register_shrinker(shrinker)

#endif /* (LINUX_VERSION_CODE < KERNEL_VERSION(6, 0, 0)) */

#if (LINUX_VERSION_CODE < KERNEL_VERSION(6, 1, 0))
#define DRM_PLANE_NO_SCALING DRM_PLANE_HELPER_NO_SCALING
#define drm_plane_helper_destroy drm_primary_helper_destroy
#endif /* (LINUX_VERSION_CODE < KERNEL_VERSION(6, 1, 0)) */

#if (LINUX_VERSION_CODE < KERNEL_VERSION(6, 2, 0))
#define genl_split_ops genl_ops
#define COMPAT_FB_INFO fbdev
#define drm_fb_helper_alloc_info drm_fb_helper_alloc_fbi
#define drm_fb_helper_unregister_info drm_fb_helper_unregister_fbi
#else
#define COMPAT_FB_INFO info
#endif /* (LINUX_VERSION_CODE < KERNEL_VERSION(6, 2, 0)) */

#if (LINUX_VERSION_CODE < KERNEL_VERSION(6, 1, 0)) || \
        ((LINUX_VERSION_CODE < KERNEL_VERSION(6, 3, 0)) && !defined(ANDROID))
static inline void pvr_vm_flags_set(struct vm_area_struct *vma,
				vm_flags_t flags)
{
	vma->vm_flags |= flags;
}
static inline void pvr_vm_flags_init(struct vm_area_struct *vma,
				vm_flags_t flags)
{
	vma->vm_flags = flags;
}
static inline void pvr_vm_flags_clear(struct vm_area_struct *vma,
				vm_flags_t flags)
{
	vma->vm_flags &= ~flags;
}
#else
#define pvr_vm_flags_set  vm_flags_set
#define pvr_vm_flags_init vm_flags_init
#define pvr_vm_flags_clear vm_flags_clear
#endif /* (LINUX_VERSION_CODE < KERNEL_VERSION(6, 3, 0)) */

#if (LINUX_VERSION_CODE < KERNEL_VERSION(6, 4, 0))
#define pvr_class_create(name) class_create(THIS_MODULE, name)
#else
#define pvr_class_create(name) class_create(name)
#endif /* (LINUX_VERSION_CODE < KERNEL_VERSION(6, 4, 0)) */

#if (LINUX_VERSION_CODE < KERNEL_VERSION(6, 6, 0))
#define thermal_tripless_zone_device_register(type, devdata, ops, tzp) \
	thermal_zone_device_register((type), 0, 0, (devdata), (ops), (tzp), 0, 0)
#endif /* (LINUX_VERSION_CODE < KERNEL_VERSION(6, 6, 0)) */

#if defined(__GNUC__)
#define GCC_VERSION_AT_LEAST(major, minor) \
	(__GNUC__ > (major) || \
	(__GNUC__ == (major) && __GNUC_MINOR__ >= (minor)))
#else
#define GCC_VERSION_AT_LEAST(major, minor) 0
#endif

#if defined(__clang__)
#define CLANG_VERSION_AT_LEAST(major) \
	(__clang_major__ >= (major))
#else
#define CLANG_VERSION_AT_LEAST(major) 0
#endif

#if !defined(__fallthrough)
	#if GCC_VERSION_AT_LEAST(7, 0) || CLANG_VERSION_AT_LEAST(10)
		#define __fallthrough __attribute__((__fallthrough__))
	#else
		#define __fallthrough
	#endif
#endif

#endif /* __KERNEL_COMPATIBILITY_H__ */
