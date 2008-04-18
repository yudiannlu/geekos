/*
 * GeekOS - x86 CPU support
 *
 * Copyright (c) 2001-2007, David H. Hovemeyer <david.hovemeyer@gmail.com>
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

#include <stdbool.h>
#include <geekos/string.h>
#include <geekos/kassert.h>
#include <arch/cpu.h>

/* segment descriptor constants (upper word) */
#define SEG_PAGE_GRANULARITY (1 << 23)
#define SEG_PRESENT          (1 << 15)
#define SEG_CODE_OR_DATA     (1 << 12) /* system bit = 1 */
#define SEG_DB               (1 << 22)
#define SEG_TYPE_CODE        (0xA << 8) /* code, !conforming, readable, !accessed */
#define SEG_TYPE_DATA        (0x2 << 8) /* data, expand-up, writable, !accessed */
#define SEG_TYPE_TSS         (0x9 << 8) /* 32 bit, !busy */

/* -------------------- Private -------------------- */

#define GDT_LEN 4

static struct x86_segment_descriptor s_gdt[GDT_LEN];
static struct x86_tss s_tss;

#ifndef NDEBUG
static void dump_gdt(void)
{
	int i;
	for (i = 0; i < GDT_LEN; i++) {
		cons_printf("gdt %d: %lx:%lx\n", i, s_gdt[i].words[0], s_gdt[i].words[1]);
	}
}
#endif

/*
 * Initialize base and limit of a segment descriptor.
 */
static void x86_seg_init_base_and_limit(
	struct x86_segment_descriptor *desc, ulong_t base, ulong_t limit, bool is_pages)
{
	/* set limit */
	desc->words[0] |= (limit & 0xFFFF);   /* bits 0..15 */
	desc->words[1] |= (limit & 0xF0000);  /* bits 16..19 */

	/* set base */
	desc->words[0] |= ((base & 0xFFFF) << 16); /* bits 0..15 */
	desc->words[1] |= ((base >> 16) & 0xFF);   /* bits 16..23 */
	desc->words[1] |= (base & 0xFF000000);     /* bits 24..31 */

	/* set granularity */
	desc->words[1] |= (is_pages ? SEG_PAGE_GRANULARITY : 0);
}

/* -------------------- Public -------------------- */

void x86_seg_init_code(struct x86_segment_descriptor *desc, u32_t base, u32_t num_pages, int priv)
{
	KASSERT(priv >= 0 && priv <= 3);

	x86_seg_init_base_and_limit(desc, base, num_pages - 1, true);
	desc->words[1] |= (SEG_TYPE_CODE | SEG_CODE_OR_DATA | SEG_PRESENT | SEG_DB);
	desc->words[1] |= ((priv & 0x3) << 13);
}

void x86_seg_init_data(struct x86_segment_descriptor *desc, u32_t base, u32_t num_pages, int priv)
{
	KASSERT(priv >= 0 && priv <= 3);

	x86_seg_init_base_and_limit(desc, base, num_pages - 1, true);
	desc->words[1] |= (SEG_TYPE_DATA | SEG_CODE_OR_DATA | SEG_PRESENT | SEG_DB);
	desc->words[1] |= ((priv & 0x3) << 13);
}

void x86_seg_init_tss(struct x86_segment_descriptor *desc, struct x86_tss *tss)
{
	x86_seg_init_base_and_limit(desc, (ulong_t) tss, sizeof(*tss), false);
	desc->words[1] |= (SEG_TYPE_TSS | SEG_PRESENT);
}

void x86_init_int_gate(struct x86_interrupt_gate *gate, ulong_t addr, int dpl)
{
	gate->offset_low = addr & 0xffff;
	gate->segment_selector = KERN_CS;
	gate->reserved = 0;
	gate->signature = 0x70;  /* == 01110000b */
	gate->dpl = dpl;
	gate->present = 1;
	gate->offset_high = addr >> 16;
}

/*
 * Create the GeekOS GDT.
 */
void x86_seg_init_gdt(void)
{
	u16_t limit_and_base[3];

	KASSERT(sizeof(struct x86_segment_descriptor) == 8);
	cons_printf("&s_gdt = %lx, sizeof(s_gdt) = %lu\n", (ulong_t)s_gdt, (ulong_t)sizeof(s_gdt));

	memset(&s_gdt, '\0', sizeof(s_gdt));
	cons_printf("gdt is cleared\n");
	x86_seg_init_code(&s_gdt[1], 0, 1048576, PRIV_KERN);
	x86_seg_init_data(&s_gdt[2], 0, 1048576, PRIV_KERN);
	x86_seg_init_tss(&s_gdt[3], &s_tss);
	/* TODO: user code/data */

#ifndef NDEBUG
	dump_gdt();
#endif

	limit_and_base[0] = sizeof(s_gdt);            /* size of GDT */
	limit_and_base[1] = ((u32_t) s_gdt) & 0xFFFF; /* low 16 bits of base addr */
	limit_and_base[2] = ((u32_t) s_gdt) >> 16;    /* high 16 bits of base addr */
	x86_load_gdtr(limit_and_base);
}
