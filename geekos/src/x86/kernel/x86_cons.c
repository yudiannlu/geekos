/*
 * GeekOS - x86 VGA text console
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

#include <geekos/string.h>
#include <geekos/cons.h>
#include <arch/ioport.h>

/* prototypes */
void x86cons_movecurs(struct console *cons_, int row, int col);
void x86cons_cleartoeol(struct console *cons_);

/* ----------------------------------------------------------------------
 * Implementation
 * ---------------------------------------------------------------------- */

#define VGA_VIDMEM 0xB8000
#define VGA_NUMROWS 25
#define VGA_NUMCOLS 80
#define VGA_BYTES_PER_ROW (VGA_NUMCOLS * 2)

#define VALID_ROW(row) ((row) >= 0 && (row) < VGA_NUMROWS)
#define VALID_COL(col) ((col) >= 0 && (col) < VGA_NUMCOLS)

/* Address of character at current cursor position in the video memory */
#define CUR_ADDR(cons) \
(((u8_t *)VGA_VIDMEM) + ((cons)->y * VGA_BYTES_PER_ROW) + ((cons)->x * 2))

#define ROW_ADDR(row) \
(((u8_t *) VGA_VIDMEM) + ((row) * VGA_BYTES_PER_ROW))

#define VGA_CRT_ADDR_REG 0x3D4
#define VGA_CRT_DATA_REG 0x3D5
#define VGA_CRT_CURSOR_LOC_HIGH_REG 0x0E
#define VGA_CRT_CURSOR_LOC_LOW_REG 0x0F

#define DEFAULT_ATTR 7

struct x86cons {
	struct console_ops *ops;
	int y, x;
	u8_t attr;
};

static void x86cons_scroll(struct x86cons *cons)
{
	int j;

	/* move rows 1..VGA_NUMROWS-1 up one row */
	for (j = 1; j < VGA_NUMROWS; j++) {
		memcpy(ROW_ADDR(j-1), ROW_ADDR(j), VGA_BYTES_PER_ROW);
	}

	/* clear last row */
	memset(ROW_ADDR(VGA_NUMROWS-1), '\0', VGA_BYTES_PER_ROW);
}

static void x86cons_newline(struct x86cons *cons)
{
	cons->x = 0;

	/* scroll? */
	if (cons->y == VGA_NUMROWS - 1) {
		x86cons_scroll(cons);
	} else {
		cons->y++;
	}

#ifndef NDEBUG
	ioport_outb(0xE9, '\n'); /* bochs port E9 hack: goes to tty */
#endif
}

static void x86cons_putgraphic(struct x86cons *cons, int ch)
{
	/* set the character in video memory */
	u8_t *ptr = CUR_ADDR(cons);
	*ptr++ = (u8_t) ch;
	*ptr++ = (u8_t) cons->attr;
	cons->x++;
#ifndef NDEBUG
	ioport_outb(0xE9, ch); /* bochs port E9 hack: goes to tty */
#endif

	/* reached end of line? */
	if (cons->x >= VGA_NUMCOLS) {
		x86cons_newline(cons);
	}
}

static void x86cons_updatecurs(struct x86cons *cons)
{
	unsigned charoff;
	u8_t origaddr;

	/* Compute new cursor location (character offset) */
	charoff = (cons->y * VGA_NUMCOLS) + cons->x;

	/* get original contents of VGA address register */
	origaddr = ioport_inb(VGA_CRT_ADDR_REG);

	/* move the cursor */
	ioport_outb(VGA_CRT_ADDR_REG, VGA_CRT_CURSOR_LOC_HIGH_REG);
	ioport_outb(VGA_CRT_DATA_REG, (charoff >> 8) & 0xFF);
	ioport_outb(VGA_CRT_ADDR_REG, VGA_CRT_CURSOR_LOC_LOW_REG);
	ioport_outb(VGA_CRT_DATA_REG, charoff & 0xFF);

	/* restore contents of VGA address register */
	ioport_outb(VGA_CRT_ADDR_REG, origaddr);
}

/* ----------------------------------------------------------------------
 * Interface
 * ---------------------------------------------------------------------- */

void x86cons_clear(struct console *cons_)
{
	memset((void *) VGA_VIDMEM, '\0', VGA_NUMROWS * VGA_BYTES_PER_ROW);
	x86cons_movecurs(cons_, 0, 0);
}

int x86cons_numrows(struct console *cons_)
{
	return VGA_NUMROWS;
}

int x86cons_numcols(struct console *cons_)
{
	return VGA_NUMCOLS;
}

int x86cons_getx(struct console *cons_)
{
	struct x86cons *cons = (struct x86cons *) cons_;
	return cons->x;
}

int x86cons_gety(struct console *cons_)
{
	struct x86cons *cons = (struct x86cons *) cons_;
	return cons->y;
}

void x86cons_movecurs(struct console *cons_, int row, int col)
{
	struct x86cons *cons = (struct x86cons *) cons_;

	if (VALID_ROW(row) && VALID_COL(col)) {
		/* update location and update cursor */
		cons->y = row;
		cons->x = col;
		x86cons_updatecurs(cons);
	}
}

void x86cons_putchar(struct console *cons_, int ch)
{
	struct x86cons *cons = (struct x86cons *) cons_;
	int nspace;

	switch (ch) {
	case '\t':
		nspace = 8 - (cons->x % CONS_TABSIZE);
		while (nspace-- > 0) {
			x86cons_putgraphic(cons, ' ');
		}
		break;

	case '\n':
		x86cons_cleartoeol(cons_);
		x86cons_newline(cons);
		break;

	default:
		x86cons_putgraphic(cons, ch);
		break;
	}

	x86cons_updatecurs(cons);
}

void x86cons_write(struct console *cons_, const char *str)
{
	while (*str != '\0') {
		x86cons_putchar(cons_, *str++);
	}
}

void x86cons_cleartoeol(struct console *cons_)
{
	struct x86cons *cons = (struct x86cons *) cons_;

	int toclear = VGA_NUMCOLS - cons->x;
	memset(CUR_ADDR(cons), '\0', toclear * 2);
}

static struct console_ops s_x86cons_ops = {
	.clear = &x86cons_clear,
	.numrows = &x86cons_numrows,
	.numcols = &x86cons_numcols,
	.getx = &x86cons_getx,
	.gety = &x86cons_gety,
	.movecurs = &x86cons_movecurs,
	.putchar = &x86cons_putchar,
	.write = &x86cons_write,
	.cleartoeol = &x86cons_cleartoeol,
};

static struct x86cons s_instance = {
	.ops = &s_x86cons_ops,
	.y = 0,
	.x = 0,
	.attr = DEFAULT_ATTR,
};

struct console *cons_getdefault(void)
{
	return (struct console *) &s_instance;
}
