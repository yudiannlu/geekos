/*
 * GeekOS - x86 virtual memory support
 * Copyright (C) 2001-2008, David H. Hovemeyer <david.hovemeyer@gmail.com>
 * Copyright (c) 2003 Jeffrey K. Hollingsworth <hollings@cs.umd.edu>
 *
 * This code is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 only, as
 * published by the Free Software Foundation.
 *   
 * This code is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * version 2 for more details (a copy is included in the LICENSE file that
 * accompanied this code).
 *  
 * You should have received a copy of the GNU General Public License version
 * 2 along with this work; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <geekos/mem.h>
#include <geekos/kassert.h>
#include <geekos/string.h>
#include <geekos/vm.h>
#include <arch/cpu.h>

#define USE_4M_PAGES
/*#define ENABLE_PAGING*/

static pde_t *s_kernel_pagedir;

static void vm_set_pte(pte_t *pgtab, unsigned index, unsigned flags, ulong_t addr)
{
	pte_t pte = { 0 };
	pte.present = 1;
	pte.flags = flags;
	pte.base_addr = VM_PAGE_BASE_ADDR(addr);

	pgtab[index] = pte;
}

static void vm_set_pde(pde_t *pgdir, unsigned index, unsigned flags, ulong_t addr)
{
	pde_t pde = { 0 };
	pde.present = 1;
	pde.flags = flags;
	pde.base_addr = VM_PAGE_BASE_ADDR(addr);
}

#ifdef USE_4M_PAGES
/*
 * Set a large (4M) page in the page directory.
 */
static void vm_set_pde_4m(pde_t *pgdir, unsigned index, unsigned flags, ulong_t addr)
{
	pde_t pde = { 0 };
	pde.present = 1;
	pde.flags = flags;
	pde.page_size = 1; /* make it a 4M page */
	pde.base_addr = VM_PAGE_BASE_ADDR(addr);
}
#endif

void vm_init_paging(struct multiboot_info *boot_info)
{
#ifdef USE_4M_PAGES
	struct x86_cpuid_info cpuid_info;
#endif
	struct frame *pgdir_frame;
	struct frame *pgtab_frame;
	pte_t *pgtab;
	ulong_t addr, mem_max;

#ifdef USE_4M_PAGES
	/*
	 * Check CPUID instruction to see if large pages (PSE feature)
	 * is supported.
	 */
	PANIC_IF(!x86_cpuid(&cpuid_info), "GeekOS requires a Pentium-class CPU");
	PANIC_IF(!cpuid_info.feature_info_edx.pse, "Processor does not support PSE");
	cons_printf("CPU supports PSE\n");

	/*
	 * Enable PSE by setting the PSE bit in CR4.
	 */
	x86_set_cr4(x86_get_cr4() | CR4_PSE);
#endif

	/*
	 * Allocate kernel page directory.
	 */
	pgdir_frame = mem_alloc_frame(FRAME_KERN, 1);
	s_kernel_pagedir = mem_frame_to_pa(pgdir_frame);
	memset(s_kernel_pagedir, '\0', PAGE_SIZE);

	/*
	 * We will support at most 2G of physical memory.
	 */
	mem_max = ((ulong_t) boot_info->mem_upper) * 1024;
	if (mem_max > (1 << 31)) {
		mem_max = (ulong_t) (1 << 31);
	}

#ifdef USE_4M_PAGES
	/*
	 * We need a page table for the low 4M of the kernel address space,
	 * since we want to leave the zero page unmapped (to catch null pointer derefs).
	 */
	pgtab_frame = mem_alloc_frame(FRAME_KERN, 1);
	pgtab = mem_frame_to_pa(pgtab_frame);
	memset(pgtab, '\0', PAGE_SIZE);

	/*
	 * Initialize low page table, leaving page 0 unmapped
	 */
	for (addr = PAGE_SIZE; addr < VM_PT_SPAN; addr += PAGE_SIZE) {
		unsigned index = VM_PAGE_TABLE_INDEX(addr);
		vm_set_pte(pgtab, index, VM_WRITE|VM_READ|VM_EXEC, addr);
	}

	/*
	 * Add low page table to the kernel pagedir.
	 */
	vm_set_pde(s_kernel_pagedir, 0, VM_WRITE|VM_READ|VM_EXEC, 0);

	/*
	 * Use 4M pages to map the rest of the low 2G of memory
	 */
	for (addr = VM_PT_SPAN; addr < mem_max; addr += VM_PT_SPAN) {
		unsigned index = VM_PAGE_DIR_INDEX(addr);
		vm_set_pde_4m(s_kernel_pagedir, index, VM_WRITE|VM_READ|VM_EXEC, addr);
	}
#endif

#ifdef ENABLE_PAGING
	/*
	 * Turn on paging!
	 */
	x86_set_cr3((u32_t) s_kernel_pagedir); /* set the kernel page directory */
	x86_set_cr0(x86_get_cr0() | CR0_PG);   /* turn on the paging bit in cr0 */
#endif
}
