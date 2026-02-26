/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */

/*
 * code defined in kernel but not accessable for modules in GKI
 *
 * we borrowed them to here for our own usage borrowed code may
 * have namespace conflict with original in kernel so we should
 * make all symbol here static, expected those already declared
 * non static, and include this .c file instead of adding it in
 * compile source list
 */

#include <linux/utsname.h>
#include <linux/kexec.h>
#include <linux/kprobes.h>
#define KEXEC_CORE_NOTE_NAME CRASH_CORE_NOTE_NAME

#if IS_ENABLED(CONFIG_ARM64)
#define BORROWED_FROM_arm64_crash_core_c
#ifdef BORROWED_FROM_arm64_crash_core_c
static inline u64 get_tcr_el1_t1sz(void)
{
	return (read_sysreg(tcr_el1) & TCR_T1SZ_MASK) >> TCR_T1SZ_OFFSET;
}

/* kernel declared non static, isolate this is we encounter compile error later */
void __arch_crash_save_vmcoreinfo(void)
{
	VMCOREINFO_NUMBER(VA_BITS);
	/* Please note VMCOREINFO_NUMBER() uses "%d", not "%x" */
	vmcoreinfo_append_str("NUMBER(MODULES_VADDR)=0x%lx\n", MODULES_VADDR);
	vmcoreinfo_append_str("NUMBER(MODULES_END)=0x%lx\n", MODULES_END);
	vmcoreinfo_append_str("NUMBER(VMALLOC_START)=0x%lx\n", VMALLOC_START);
	vmcoreinfo_append_str("NUMBER(VMALLOC_END)=0x%lx\n", VMALLOC_END);
	vmcoreinfo_append_str("NUMBER(VMEMMAP_START)=0x%lx\n", VMEMMAP_START);
	vmcoreinfo_append_str("NUMBER(VMEMMAP_END)=0x%lx\n", VMEMMAP_END);
	vmcoreinfo_append_str("NUMBER(kimage_voffset)=0x%llx\n",
			      kimage_voffset);
	vmcoreinfo_append_str("NUMBER(PHYS_OFFSET)=0x%llx\n", PHYS_OFFSET);
	vmcoreinfo_append_str("NUMBER(TCR_EL1_T1SZ)=0x%llx\n",
			      get_tcr_el1_t1sz());
	vmcoreinfo_append_str("KERNELOFFSET=%lx\n", kaslr_offset());
	vmcoreinfo_append_str(
		"NUMBER(KERNELPACMASK)=0x%llx\n",
		system_supports_address_auth() ? ptrauth_kernel_pac_mask() : 0);
}
#endif
#else
void __arch_crash_save_vmcoreinfo(void)
{
	;
}
#endif

#define BORROWED_FROM_crash_core_c
#ifdef BORROWED_FROM_crash_core_c
/* vmcoreinfo stuff */
unsigned char *vmcoreinfo_data;
size_t vmcoreinfo_size;
u32 *vmcoreinfo_note;

/* Per cpu memory for storing cpu states in case of system crash. */
note_buf_t __percpu *sunxi_crash_notes;
static int crash_notes_memory_init(void)
{
	/* Allocate memory for saving cpu registers. */
	size_t size, align;

	/*
	 * crash_notes could be allocated across 2 vmalloc pages when percpu
	 * is vmalloc based . vmalloc doesn't guarantee 2 continuous vmalloc
	 * pages are also on 2 continuous physical pages. In this case the
	 * 2nd part of crash_notes in 2nd page could be lost since only the
	 * starting address and size of crash_notes are exported through sysfs.
	 * Here round up the size of crash_notes to the nearest power of two
	 * and pass it to __alloc_percpu as align value. This can make sure
	 * crash_notes is allocated inside one physical page.
	 */
	size = sizeof(note_buf_t);
	align = min(roundup_pow_of_two(sizeof(note_buf_t)), PAGE_SIZE);

	/*
	 * Break compile if size is bigger than PAGE_SIZE since crash_notes
	 * definitely will be in 2 pages with that.
	 */
	BUILD_BUG_ON(size > PAGE_SIZE);

	sunxi_crash_notes = __alloc_percpu(size, align);
	if (!sunxi_crash_notes) {
		pr_warn("Memory allocation for saving cpu register states failed\n");
		return -ENOMEM;
	}
	return 0;
}

Elf_Word *append_elf_note(Elf_Word *buf, char *name, unsigned int type,
			  void *data, size_t data_len)
{
	struct elf_note *note = (struct elf_note *)buf;

	note->n_namesz = strlen(name) + 1;
	note->n_descsz = data_len;
	note->n_type = type;
	buf += DIV_ROUND_UP(sizeof(*note), sizeof(Elf_Word));
	memcpy(buf, name, note->n_namesz);
	buf += DIV_ROUND_UP(note->n_namesz, sizeof(Elf_Word));
	memcpy(buf, data, data_len);
	buf += DIV_ROUND_UP(data_len, sizeof(Elf_Word));

	return buf;
}

void final_note(Elf_Word *buf)
{
	memset(buf, 0, sizeof(struct elf_note));
}

static void update_vmcoreinfo_note(void)
{
	u32 *buf = vmcoreinfo_note;

	if (!vmcoreinfo_size)
		return;
	buf = append_elf_note(buf, VMCOREINFO_NOTE_NAME, 0, vmcoreinfo_data,
			      vmcoreinfo_size);
	final_note(buf);
}

void vmcoreinfo_append_str(const char *fmt, ...)
{
	va_list args;
	char buf[0x50];
	size_t r;

	va_start(args, fmt);
	r = vscnprintf(buf, sizeof(buf), fmt, args);
	va_end(args);

	r = min(r, (size_t)VMCOREINFO_BYTES - vmcoreinfo_size);

	memcpy(&vmcoreinfo_data[vmcoreinfo_size], buf, r);

	vmcoreinfo_size += r;

	WARN_ONCE(vmcoreinfo_size == VMCOREINFO_BYTES,
		  "vmcoreinfo data exceeds allocated size, truncating");
}

static int crash_save_vmcoreinfo_init(void)
{
	vmcoreinfo_data = (unsigned char *)get_zeroed_page(GFP_KERNEL);
	if (!vmcoreinfo_data) {
		pr_warn("Memory allocation for vmcoreinfo_data failed\n");
		return -ENOMEM;
	}

	if (vmcoreinfo_note)
		kfree(vmcoreinfo_note);
	vmcoreinfo_note =
		kmalloc(VMCOREINFO_NOTE_SIZE, GFP_KERNEL | __GFP_ZERO);
	if (!vmcoreinfo_note) {
		free_page((unsigned long)vmcoreinfo_data);
		vmcoreinfo_data = NULL;
		pr_warn("Memory allocation for vmcoreinfo_note failed\n");
		return -ENOMEM;
	}

#ifdef ABLE_TO_GET_STEXT_ADDR
	/*
	 * no stable ways to get _stext in a module for now
	 * may try to find one later -- 2024.12
	 */
	VMCOREINFO_SYMBOL(_stext);
#endif

	__arch_crash_save_vmcoreinfo();
	update_vmcoreinfo_note();

	return 0;
}
#endif //BORROWED_FROM_crash_core_c
