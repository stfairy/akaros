/*
 * Copyright (c) 2009 The Regents of the University of California
 * Barret Rhoden <brho@cs.berkeley.edu>
 * See LICENSE for details.
 */

#include <arch/arch.h>
#include <arch/x86.h>
#include <arch/mmu.h>
#include <stdio.h>
#include <assert.h>
#include <ros/memlayout.h>
#include <pmap.h>
#include <kdebug.h>
#include <string.h>

/* Check Intel's SDM 2a for Table 3-17 for the cpuid leaves */
void print_cpuinfo(void)
{
	uint32_t eax, ebx, ecx, edx;
	uint32_t model, family;
	uint64_t msr_val;
	char vendor_id[13];
	int max_std_lvl, max_extd_lvl;
	extern char _start[];
	bool is_intel;

	if (sizeof(long) == 8)
		printk("64 bit Kernel Booting...\n");
	else
		printk("32 bit Kernel Booting...\n");
	asm volatile ("cpuid;"
	          "movl    %%ebx, (%2);"
	          "movl    %%edx, 4(%2);"
	          "movl    %%ecx, 8(%2);"
	         : "=a"(eax)
	         : "a"(0), "D"(vendor_id)
	         : "%ebx", "%ecx", "%edx");

	vendor_id[12] = '\0';
	cprintf("Vendor ID: %s\n", vendor_id);
	/* not a great check - old intel P5s have no vendor id */
	is_intel = !strcmp(vendor_id, "GenuineIntel");
	/* intel supports a way to hide the upper leaves of cpuid, beyond 3.  the
	 * bios might have done this, so we'll make sure it is off. */
	if (is_intel) {
		msr_val = read_msr(IA32_MISC_ENABLE);
		if (msr_val & (1 << 22))
			write_msr(IA32_MISC_ENABLE, msr_val & ~(1 << 22));
	}
	cprintf("Largest Standard Function Number Supported: %d\n", eax);
	max_std_lvl = eax;
	cpuid(0x80000000, 0x0, &eax, 0, 0, 0);
	cprintf("Largest Extended Function Number Supported: 0x%08x\n", eax);
	max_extd_lvl = eax;
	cpuid(1, 0x0, &eax, &ebx, &ecx, &edx);
	family = ((eax & 0x0FF00000) >> 20) + ((eax & 0x00000F00) >> 8);
	model = ((eax & 0x000F0000) >> 12) + ((eax & 0x000000F0) >> 4);
	cprintf("Family: %d\n", family);
	cprintf("Model: %d\n", model);
	cprintf("Stepping: %d\n", eax & 0x0000000F);
	// eventually can fill this out with SDM Vol3B App B info, or
	// better yet with stepping info.  or cpuid 8000_000{2,3,4}
	switch ( family << 8 | model ) {
		case(0x061a):
			cprintf("Processor: Core i7\n");
			break;
		case(0x060f):
			cprintf("Processor: Core 2 Duo or Similar\n");
			break;
		default:
			cprintf("Unknown or non-Intel CPU\n");
	}
	if (!(edx & 0x00000020))
		panic("MSRs not supported!");
	if (!(edx & 0x00001000))
		panic("MTRRs not supported!");
	if (!(edx & 0x00002000))
		panic("Global Pages not supported!");
	if (!(edx & 0x00000200))
		panic("Local APIC Not Detected!");
	if (ecx & 0x00200000)
		cprintf("x2APIC Detected\n");
	else
		cprintf("x2APIC Not Detected\n");
	/* Not sure how to detect AMD HW virt yet. */
	if ((ecx & 0x00000060) && is_intel) {
		msr_val = read_msr(IA32_FEATURE_CONTROL);
		printd("64 Bit Feature Control: 0x%08x\n", msr_val);
		if ((msr_val & 0x5) == 0x5)
			printk("Hardware virtualization supported\n");
		else
			printk("Hardware virtualization not supported\n");
	} else { 
		printk("Hardware virtualization not detected.  (AMD?)\n");
	}
	/* FP and SSE Checks */
	if (edx & 0x00000001)
		printk("FPU Detected\n");
	else
		panic("FPU Not Detected!!\n");
	printk("SSE support: ");
	if (edx & (1 << 25))
		printk("sse ");
	else
		panic("SSE Support Not Detected!!\n");
	if (edx & (1 << 26))
		printk("sse2 ");
	if (ecx & (1 << 0))
		printk("sse3 ");
	if (ecx & (1 << 9))
		printk("ssse3 ");
	if (ecx & (1 << 19))
		printk("sse4.1 ");
	if (ecx & (1 << 20))
		printk("sse4.2 ");
	if (edx & (1 << 23))
		printk("mmx ");
	if ((edx & (1 << 25)) && (!(edx & (1 << 24))))
		panic("SSE support, but no FXSAVE!");
	printk("\n");
	cpuid(0x80000008, 0x0, &eax, &ebx, &ecx, &edx);
	cprintf("Physical Address Bits: %d\n", eax & 0x000000FF);
	msr_val = read_msr(IA32_APIC_BASE);
	if (!(msr_val & MSR_APIC_ENABLE))
		panic("Local APIC Disabled!!");
	cpuid(0x80000007, 0x0, &eax, &ebx, &ecx, &edx);
	if (edx & 0x00000100)
		printk("Invariant TSC present\n");
	else
		printk("Invariant TSC not present\n");
	cpuid(0x07, 0x0, &eax, &ebx, &ecx, &edx);
	if (ebx & 0x00000001) {
		printk("FS/GS Base RD/W supported\n");
		/* Untested, since we don't have a machine that supports this.  Email us
		 * if this fails. */
		printk("Attempting to enable WRFSBASE...\n");
		lcr4(rcr4() | (1 << 16));
	} else {
		printk("FS/GS Base RD/W not supported\n");
		#ifdef CONFIG_NOFASTCALL_FSBASE
		printk("\nGIANT WARNING: Can't write FS Base from userspace, "
		       "and no FASTCALL support!\n\n");
		#endif
	}
	cpuid(0x80000001, 0x0, &eax, &ebx, &ecx, &edx);
	if (edx & (1 << 27)) {
		printk("RDTSCP supported\n");
		/* Set core 0's id, for use during boot (if FAST_COREID) */
		write_msr(MSR_TSC_AUX, 0);
	} else {
		printk("RDTSCP not supported, but emulated for userspace\n");
	}
	/* Regardless, make sure userspace can access rdtsc (and rdtscp) */
	lcr4(rcr4() & ~CR4_TSD);
	printk("1 GB Jumbo pages %ssupported\n", edx & (1 << 26) ? "" : "not ");
	printk("FS/GS MSRs %ssupported\n", edx & (1 << 29) ? "" : "not ");
	if (!(edx & (1 << 29))) {
		printk("Can't handle no FS/GS MSRs!\n");
		while (1)
			asm volatile ("hlt");
	}
	cpuid(0x00000006, 0x0, &eax, 0, 0, 0);
	if (eax & (1 << 2))
		printk("Always running APIC detected\n");
	else
		printk("Always running APIC *not* detected\n");
}

#define BIT_SPACING "        "
#define BIT_DASHES "----------------"

void show_mapping(pgdir_t pgdir, uintptr_t start, size_t size)
{
	pte_t pte;
	int perm;
	page_t *page;
	uintptr_t i;

	printk("   %sVirtual    %sPhysical  Ps Dr Ac CD WT U W P EPTE\n", BIT_SPACING,
	       BIT_SPACING);
	printk("--------------------------------------------%s\n", BIT_DASHES);
	for(i = 0; i < size; i += PGSIZE, start += PGSIZE) {
		pte = pgdir_walk(pgdir, (void*)start, 0);
		printk("%p  ", start);
		if (pte_walk_okay(pte)) {
			/* A note on PTE perms.  If you look at just the PTE, you don't get
			 * the full picture for W and U.  Those are the intersection of all
			 * bits.  In Akaros, we do U or not at the earliest point (PML4
			 * entries).  All other PTEs have U set.  For W, it's the opposite.
			 * The PTE for the actual page has W or not, and all others has W
			 * set.  W needs to be more fine-grained, but U doesn't.  Plus the
			 * UVPT mapping requires the U to see interior pages (but have W
			 * off). */
			perm = get_va_perms(pgdir, (void*)start);
			printk("%p  %1d  %1d  %1d  %1d  %1d  %1d %1d %1d 0x%llx\n",
			       pte_get_paddr(pte),
			       pte_is_jumbo(pte),
			       pte_is_dirty(pte),
			       pte_is_accessed(pte),
			       (pte_print(pte) & PTE_PCD) / PTE_PCD,
			       (pte_print(pte) & PTE_PWT) / PTE_PWT,
			       (perm & PTE_U) / PTE_U,
			       (perm & PTE_W) / PTE_W,
			       pte_is_present(pte),
				*(uint64_t *)(&pte[512]));
		} else {
			printk("%p\n", 0);
		}
	}
}

/* return 0 if ok, -1 if it failed for some reason.
 * Be sensible and call it with 16 bytes.
 */
int vendor_id(char *vid)
{
	uint32_t eax, ebx, ecx, edx;

	asm volatile ("cpuid;"
	          "movl    %%ebx, (%2);"
	          "movl    %%edx, 4(%2);"
	          "movl    %%ecx, 8(%2);"
	         : "=a"(eax)
	         : "a"(0), "D"(vid)
	         : "%ebx", "%ecx", "%edx");

	vid[12] = '\0';
	return 0;
}
