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

static pde_t *s_kernel_pagedir;

void vm_init_paging(struct multiboot_info *boot_info)
{
	struct x86_cpuid_info cpuid_info;
	struct frame *pgdir_frame;
	struct frame *pgtab_frame;
	ulong_t mem_max;

	/*
	 * Check CPUID instruction to see if large pages (PSE feature)
	 * is supported.
	 */
	PANIC_IF(!x86_cpuid(&cpuid_info), "GeekOS requires a Pentium-class CPU");
	PANIC_IF(!cpuid_info.feature_info_edx.pse, "Processor does not support PSE");
	cons_printf("CPU supports PSE\n");

	/*
	 * Allocate kernel page directory.
	 */
	pgdir_frame = mem_alloc_frame(FRAME_KERN, 1);
	s_kernel_pagedir = mem_frame_to_pa(pgdir_frame);
	memset(s_kernel_pagedir, '\0', PAGE_SIZE);

	/*
	 * We need a page table for the low 4M of the kernel address space,
	 * since we want to leave the zero page unmapped (to catch null pointer derefs).
	 */
	pgtab_frame = mem_alloc_frame(FRAME_KERN, 1);

	/* TODO: initialize low page table */

	/*
	 * We will support at most 2G of physical memory.
	 */
	mem_max = ((ulong_t) boot_info->mem_upper) * 1024;
	if (mem_max > (1 << 31)) {
		mem_max = (ulong_t) (1 << 31);
	}

	/* TODO: use 4M pages to map the rest of the low 2G of memory */
}
