/*
 * GeekOS - x86 CPU support
 *
 * Copyright (c) 2001-2007, David H. Hovemeyer <david.hovemeyer@gmail.com>
 */

/*
 * Information sources:
 * - Protected Mode Software Architecture by Tom Shanley, ISBN 020155447X.
 * - Intel IA-32 manual, Vol. 3
 */

#ifndef ARCH_CPU_H
#define ARCH_CPU_H

#ifndef ASM

#include <geekos/types.h>

struct x86_tss {
	/*
	 * Link to nested task.  For example, if an interrupt is handled
	 * by a task gate, the link field will contain the selector for
	 * the TSS of the interrupted task.
	 */
	u16_t link;
	u16_t reserved1;

	/* Stacks for privilege levels.  esp0/ss0 specifies the kernel stack. */
	u32_t esp0;
	u16_t ss0;
	u16_t reserved2;
	u32_t esp1;
	u16_t ss1;
	u16_t reserved3;
	u32_t esp2;
	u16_t ss2;
	u16_t reserved4;

	/* Page directory register. */
	u32_t cr3;

	/* General processor registers. */
	u32_t eip;
	u32_t eflags;
	u32_t eax;
	u32_t ecx;
	u32_t edx;
	u32_t ebx;
	u32_t esp;
	u32_t ebp;
	u32_t esi;
	u32_t edi;

	/* Segment registers and padding. */
	u16_t es;
	u16_t reserved5;
	u16_t cs;
	u16_t reserved6;
	u16_t ss;
	u16_t reserved7;
	u16_t ds;
	u16_t reserved8;
	u16_t fs;
	u16_t reserved9;
	u16_t gs;
	u16_t reserved10;

	/* GDT selector for the LDT descriptor. */
	u16_t ldt;
	u16_t reserved11;

	/*
	 * The debug trap bit causes a debug exception upon a switch
	 * to the task specified by this TSS.
	 */
	uint_t debug_trap : 1;
	uint_t reserved12 : 15;

	/* Offset in the TSS specifying where the io map is located. */
	u16_t io_map_base;
};

/*
 * A segment descriptor is an entry in the GDT or an LDT.
 */
struct x86_segment_descriptor {
	u32_t words[2];
};

/*
 * An interrupt gate is an entry in the IDT.
 */
struct x86_interrupt_gate {
	u16_t offset_low          ;
	u16_t segment_selector    ;
	uint_t reserved        : 5;
	uint_t signature       : 8;
	uint_t dpl             : 2;
	uint_t present         : 1;
	u16_t offset_high         ;
};

/* initialize segment descriptors */
void x86_seg_init_null(struct x86_segment_descriptor *desc,
	u32_t base, u32_t num_pages, int priv);
void x86_eg_init_code(struct x86_segment_descriptor *desc,
	u32_t base, u32_t num_pages, int priv);
void x86_seg_init_data(struct x86_segment_descriptor *desc,
	u32_t base, u32_t num_pages, int priv);
void x86_seg_init_tss(struct x86_segment_descriptor *desc, struct x86_tss *tss);

/* initialize interrupt gate */
void x86_init_int_gate(struct x86_interrupt_gate *gate, ulong_t addr, int dpl);

/* initialize GDT */
void x86_seg_init_gdt(void);
void x86_load_gdtr(u16_t *limit_and_base);

/* initialize IDT */
void x86_load_idtr(u16_t *limit_and_base);

/* load TSS */
void x86_load_tr(struct x86_segment_descriptor *tss_desc);
#endif

#define PRIV_KERN 0
#define PRIV_USER 3

#define SEL_GDT 0
#define SEL_LDT (1 << 2)

#define SELECTOR(index, table, rpl) \
((((index) & 0x1FFF) << 3) | (table) | ((rpl) & 3))

/* selectors for kernel code/data */
#define KERN_CS SELECTOR(1, SEL_GDT, 0)
#define KERN_DS SELECTOR(2, SEL_GDT, 0)

/*
 * bits in eflags register
 */
#define EFLAGS_IF (1 << 9)

#endif /* ARCH_CPU_H */
