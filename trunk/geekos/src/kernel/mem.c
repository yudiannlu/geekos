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

#include <geekos/mem.h>
#include <geekos/int.h>
#include <geekos/string.h>
#include <geekos/thread.h>

#define HEAP_SIZE (128*1024)

IMPLEMENT_LIST_INIT(frame_list, frame)
IMPLEMENT_LIST_APPEND(frame_list, frame)
IMPLEMENT_LIST_IS_EMPTY(frame_list, frame)
IMPLEMENT_LIST_REMOVE_FIRST(frame_list, frame)

static ulong_t s_numframes;
static struct frame *s_framelist;
static struct frame_list s_freelist;

static struct thread_queue s_heap_waitqueue;
static struct thread_queue s_frame_waitqueue;

struct scan_region_data {
	bool heap_created;
};

static void mem_heap_init(ulong_t start, ulong_t end)
{
	extern char *g_heapstart, *g_heapend;

	cons_printf("Heap from %lx to %lx\n", start, end);
	g_heapstart = (char *) start;
	g_heapend   = (char *) end;
}

static void mem_set_region_state(ulong_t start, ulong_t end, frame_state_t state)
{
	ulong_t addr;
	KASSERT(start < end);
	for (addr = start; addr < end; addr += PAGE_SIZE) {
		struct frame *frame = &s_framelist[addr / PAGE_SIZE];
		frame->state = state;
		if (state == FRAME_AVAIL) {
			frame_list_append(&s_freelist, frame);
		}
	}
}

static ulong_t mem_scan_region(ulong_t start, ulong_t end, frame_state_t state, void *data_)
{
	struct scan_region_data *data = data_;

	/* try to create kernel heap, if appropriate */
	if (!data->heap_created && state == FRAME_AVAIL && (end - start) >= HEAP_SIZE) {
		mem_set_region_state(start, start + HEAP_SIZE, FRAME_HEAP);
		mem_heap_init(start, start + HEAP_SIZE);
		start += HEAP_SIZE;
		data->heap_created = true;
	}

	/* set state of all frames in region, and add to freelist if appropriate */
	mem_set_region_state(start, end, state);

	return end;
}

void mem_clear_bss(void)
{
	extern char __bss_start, end;
	memset(&__bss_start, '\0', &end - &__bss_start);
}

void mem_init(struct multiboot_info *boot_record)
{
	struct scan_region_data data = { .heap_created = false };

	cons_printf("Initializing segments\n");
	mem_init_segments();
	cons_printf("Initialzing framelist\n");
	mem_create_framelist(boot_record, &s_framelist, &s_numframes);
	cons_printf("Scanning memory regions\n");
	mem_scan_regions(boot_record, &mem_scan_region, &data);

	PANIC_IF(!data.heap_created, "Couldn't create kernel heap!");
}

/*
 * Allocate a buffer in the kernel heap.
 * Suspends calling thread until enough memory
 * is available to satisfy the request.
 */
void *mem_alloc(size_t size)
{
	extern void *malloc(size_t);
	void *buf;
	bool iflag;

	iflag = int_begin_atomic();
	while ((buf = malloc(size)) == 0) {
		thread_wait(&s_heap_waitqueue);
	}
	int_end_atomic(iflag);

	return buf;
}

/*
 * Allocate a physical memory frame.
 * Suspends calling thread until a frame is available.
 */
void *mem_alloc_frame(void)
{
	struct frame *frame;
	bool iflag;

	iflag = int_begin_atomic();

	while (frame_list_is_empty(&s_freelist)) {
		thread_wait(&s_frame_waitqueue);
	}

	frame = frame_list_remove_first(&s_freelist);
	frame->state = FRAME_ALLOCATED;

	int_end_atomic(iflag);

	return frame ? mem_frame_to_pa(frame) : 0;
}

/*
 * Free memory allocated with mem_alloc() or mem_alloc_frame().
 */
void mem_free(void *p)
{
	extern void free(void *);
	extern char *g_heapstart, *g_heapend;
	struct frame *frame;
	bool iflag;

	if (!p) {
		return;
	}

	iflag = int_begin_atomic();

	if (((char*)p) >= g_heapstart && ((char*)p) < g_heapend) {
		/* a buffer in the kernel heap */
		cons_printf("freeing heap buffer @%p\n", p);
		free(p);

		/* wake up any threads waiting for memory */
		thread_wakeup(&s_heap_waitqueue);
	} else {
		/* an allocated frame */
		KASSERT(mem_is_page_aligned((ulong_t) p));
		frame = mem_pa_to_frame(p);
		KASSERT(frame->state == FRAME_ALLOCATED);
		frame->state = FRAME_AVAIL;
		frame_list_append(&s_freelist, frame);

		/* wake up any threads waiting for a frame */
		thread_wakeup(&s_frame_waitqueue);
	}

	int_end_atomic(iflag);
}

void *mem_frame_to_pa(struct frame *frame)
{
	ulong_t offset = frame - s_framelist;
	return (void *) (offset * PAGE_SIZE);
}

struct frame *mem_pa_to_frame(void *pa)
{
	ulong_t framenum = ((ulong_t) pa) / PAGE_SIZE;
	return &s_framelist[framenum];
}

ulong_t mem_round_to_page(ulong_t addr)
{
	if ((addr & PAGE_MASK) != addr) {
		addr = (addr & PAGE_MASK) + PAGE_SIZE;
	}
	return addr;
}

bool mem_is_page_aligned(ulong_t addr)
{
	return mem_round_to_page(addr) == addr;
}
