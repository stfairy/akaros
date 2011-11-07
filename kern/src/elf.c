#include <mm.h>
#include <frontend.h>
#include <string.h>
#include <kmalloc.h>
#include <syscall.h>
#include <elf.h>
#include <pmap.h>
#include <smp.h>
#include <arch/arch.h>

#ifdef KERN64
# define elf_field(obj, field) (elf64 ? (obj##64)->field : (obj##32)->field)
#else
# define elf_field(obj, field) ((obj##32)->field)
#endif

static int load_one_elf(struct proc *p, struct file *f, int pgoffset,
                        elf_info_t *ei)
{
	int ret = -1;
	ei->phdr = -1;
	ei->dynamic = 0;
	ei->highest_addr = 0;
	off_t f_off = 0;

	/* assume program headers fit in a page.  if this isn't true, change the
	 * code below that maps in program headers */
	char* elf = (char*)kmalloc(PGSIZE, 0);
	
	/* When reading on behalf of the kernel, we need to make sure no proc is
	 * "current".  This is a bit ghetto (TODO: KFOP) */
	struct proc *cur_proc = current;
	current = 0;
	if (!elf || f->f_op->read(f, elf, PGSIZE, &f_off) == -1)
		goto fail;
	current = cur_proc;

	elf32_t* elfhdr32 = (elf32_t*)elf;
	elf64_t* elfhdr64 = (elf64_t*)elf;
	bool elf32 = elfhdr32->e_ident[ELF_IDENT_CLASS] == ELFCLASS32;
	bool elf64 = elfhdr32->e_ident[ELF_IDENT_CLASS] == ELFCLASS64;
	if (!elf64 && !elf32)
		goto fail;
	#ifndef KERN64
	if(elf64)
		goto fail;
	#endif
	
	proghdr32_t* proghdrs32 = (proghdr32_t*)(elf + elfhdr32->e_phoff);
	proghdr64_t* proghdrs64 = (proghdr64_t*)(elf + elfhdr64->e_phoff);
	uintptr_t e_phoff = elf_field(elfhdr, e_phoff);
	size_t phsz = elf64 ? sizeof(proghdr64_t) : sizeof(proghdr32_t);
	uint16_t e_phnum = elf_field(elfhdr, e_phnum);
	// we don't support prog hdrs extending past the first elf page
	if (e_phoff + e_phnum * phsz > PGSIZE)
		goto fail;

	for (int i = 0; i < e_phnum; i++) {
		proghdr32_t* ph32 = proghdrs32+i;
		proghdr64_t* ph64 = proghdrs64+i;
		uint16_t p_type = elf_field(ph, p_type);
		uintptr_t p_va = elf_field(ph, p_va);
		uintptr_t p_offset = elf_field(ph, p_offset);
		uintptr_t p_align = elf_field(ph, p_align);
		uintptr_t p_memsz = elf_field(ph, p_memsz);
		uintptr_t p_filesz = elf_field(ph, p_filesz);

		if (p_type == ELF_PROG_PHDR)
			ei->phdr = elf_field(ph, p_va);
		if (p_type == ELF_PROG_INTERP) {
			int maxlen = MIN(PGSIZE - p_offset, sizeof(ei->interp));
			int len = strnlen(elf + p_offset, maxlen);
			if (len < maxlen) {
				memcpy(ei->interp, elf + p_offset, maxlen + 1);
				ei->dynamic = 1;
			}
			else
				goto fail;
		}

		if (p_type == ELF_PROG_LOAD && p_memsz) {
			if (p_align % PGSIZE)
				goto fail;
			if (p_offset % PGSIZE != p_va % PGSIZE)
				goto fail;

			uintptr_t filestart = ROUNDDOWN(p_offset, PGSIZE);
			uintptr_t fileend = p_offset + p_filesz;
			uintptr_t filesz = fileend - filestart;

			uintptr_t memstart = ROUNDDOWN(p_va, PGSIZE);
			uintptr_t memend = ROUNDUP(p_va + p_memsz, PGSIZE);
			uintptr_t memsz = memend - memstart;
			if (memend > ei->highest_addr)
				ei->highest_addr = memend;
			/* This needs to be a PRIVATE mapping, and the stuff after the file
			 * needs to be zeroed. */
			if (filesz) {
				/* TODO: figure out proper permissions from the elf */
				if (do_mmap(p, memstart + pgoffset * PGSIZE, filesz,
				           PROT_READ|PROT_WRITE|PROT_EXEC, MAP_FIXED|MAP_PRIVATE,
				           f, filestart) == MAP_FAILED)
					goto fail;
				/* Due to elf-ghetto-ness, we need to zero the first part of the
				 * BSS from the last page of the data segment.  We translate to
				 * the KVA so we don't need to worry about using the proc's
				 * mapping */
				uintptr_t z_s = memstart + pgoffset * PGSIZE + filesz;
				pte_t *pte = pgdir_walk(p->env_pgdir, (void*)z_s, 0);
				assert(pte);
				uintptr_t kva_z_s = (uintptr_t)ppn2kva(PTE2PPN(*pte)) + PGOFF(z_s);
				uintptr_t kva_z_e = ROUNDUP(kva_z_s, PGSIZE);
				memset((void*)kva_z_s, 0, kva_z_e - kva_z_s);
				filesz = ROUNDUP(filesz, PGSIZE);
			}
			/* Any extra pages are mapped anonymously... (a bit weird) */
			if (filesz < memsz)
				if (do_mmap(p, memstart + filesz + pgoffset*PGSIZE, memsz-filesz,
				           PROT_READ|PROT_WRITE|PROT_EXEC, MAP_FIXED|MAP_ANON,
				           NULL, 0) == MAP_FAILED)
					goto fail;
		}
	}
	/* map in program headers anyway if not present in binary.
	 * useful for TLS in static programs. */
	if (ei->phdr == -1) {
		void *phdr_addr = do_mmap(p, MMAP_LOWEST_VA, PGSIZE, PROT_READ, 0, f,
		                          0);
		if (phdr_addr == MAP_FAILED)
			goto fail;
		ei->phdr = (long)phdr_addr + e_phoff;
	}
	ei->entry = elf_field(elfhdr, e_entry) + pgoffset*PGSIZE;
	ei->phnum = e_phnum;
	ei->elf64 = elf64;
	ret = 0;
fail:
	kfree(elf);
	return ret;
}

int load_elf(struct proc* p, struct file* f)
{
	elf_info_t ei, interp_ei;
	if (load_one_elf(p, f, 0,& ei))
		return -1;

	if (ei.dynamic) {
		struct file *interp = do_file_open(ei.interp, 0, 0);
		if (!interp)
			return -1;
		/* careful, this could conflict with the mmap from the TLS up above */
 		int error = load_one_elf(p, interp, 2, &interp_ei);
		kref_put(&interp->f_kref);
		if (error)
			return -1;
	}

	// fill in auxiliary info for dynamic linker/runtime
	elf_aux_t auxp[] = {{ELF_AUX_PHDR, ei.phdr},
	                    {ELF_AUX_PHENT, sizeof(proghdr32_t)},
	                    {ELF_AUX_PHNUM, ei.phnum},
	                    {ELF_AUX_ENTRY, ei.entry},
	                    #ifdef __sparc_v8__
	                    {ELF_AUX_HWCAP, ELF_HWCAP_SPARC_FLUSH},
	                    #endif
	                    {0, 0}};

	// put auxp after argv, envp in procinfo
	int auxp_pos = -1;
	for (int i = 0, zeros = 0; i < PROCINFO_MAX_ARGP; i++)
		if (p->procinfo->argp[i] == NULL)
			if (++zeros == 2)
				auxp_pos = i + 1;
	if (auxp_pos == -1 ||
	    auxp_pos + sizeof(auxp) / sizeof(char*) >= PROCINFO_MAX_ARGP)
		return -1;
	memcpy(p->procinfo->argp+auxp_pos,auxp,sizeof(auxp));

	uintptr_t core0_entry = ei.dynamic ? interp_ei.entry : ei.entry;
	proc_init_trapframe(&p->env_tf,0,core0_entry,USTACKTOP);
	p->env_entry = ei.entry;

	// map in stack using POPULATE (because SPARC requires it)
	int flags = MAP_FIXED | MAP_ANONYMOUS;
	#ifdef __sparc_v8__
	flags |= MAP_POPULATE; // SPARC stacks must be mapped in
	#endif
	uintptr_t stacksz = USTACK_NUM_PAGES*PGSIZE;
	if (do_mmap(p, USTACKTOP-stacksz, stacksz, PROT_READ | PROT_WRITE,
	            flags, NULL, 0) == MAP_FAILED)
		return -1;

	// Set the heap bottom and top to just past where the text 
	// region has been loaded
	p->heap_top = (void*)ei.highest_addr;
	p->procinfo->heap_bottom = p->heap_top;

	return 0;
}

