/*
 * GeekOS - memory allocation
 *
 * Copyright (C) 2001-2008, David H. Hovemeyer <david.hovemeyer@gmail.com>
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

#ifndef GEEKOS_MEM_H
#define GEEKOS_MEM_H

#include <stddef.h>
#include <geekos/types.h>
#include <geekos/boot.h>
#include <geekos/list.h>
#include <arch/mem.h>

/*
 * Frame states
 */
typedef enum {
	FRAME_AVAIL,     /* frame is on the freelist */
	FRAME_KERN,      /* frame used by kernel code or data */
	FRAME_HW,        /* frame used by hardware (e.g., ISA hole) */
	FRAME_ALLOCATED, /* frame is allocated */
	FRAME_UNUSED,    /* frame is unused */
	FRAME_HEAP,      /* frame is in kernel heap */
} frame_state_t;

DECLARE_LIST(frame_list, frame);

/*
 * Frame metadata structure
 */
struct frame {
	frame_state_t state;
	DEFINE_LINK(frame_list, frame);
};

void mem_clear_bss(void);
void mem_init(struct multiboot_info *boot_record);
void *mem_alloc(size_t size);
void *mem_alloc_frame(void);
void mem_free(void *p);

void *mem_frame_to_pa(struct frame *frm);
struct frame *mem_pa_to_frame(void *pa);
ulong_t mem_round_to_page(ulong_t addr);
bool mem_is_page_aligned(ulong_t addr);

/* architecture-dependent initialization */

typedef ulong_t (scan_reg_func_t)(ulong_t start_addr, ulong_t end_addr,
	frame_state_t state, void *data);

void mem_init_segments(void);
void mem_create_framelist(struct multiboot_info *boot_record, struct frame **framelist,
	ulong_t *numframes);
void mem_scan_regions(struct multiboot_info *boot_record,
	scan_reg_func_t *scan_reg_func, void *data);

#endif /* GEEKOS_MEM_H */
