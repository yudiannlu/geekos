/*
 * GeekOS - virtual memory
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

#ifndef GEEKOS_VM_H
#define GEEKOS_VM_H

#include <geekos/types.h>
#include <geekos/mem.h>

struct vm_pager_ops;

/*
 * A data store from which
 * pages of data can be read and
 * to which pages of data can be written.
 */
struct vm_pager {
	struct vm_pager_ops *ops;
	void *data; /* for use by underlying pager implementation */	
};

/*
 * Operations supported by vm_pager objects.
 */
struct vm_pager_ops {
	int (*read_page)(struct vm_pager *pager, void *buf, u32_t page_num);
	int (*write_page)(struct vm_pager *pager, void *buf, u32_t page_num);
	/* Other ops? */
};

/*
 * A vm_obj is a data store that can be mapped into
 * a process address space.
 * It is a cache of pages containing data from
 * an underlying data store (the pager).
 */
struct vm_obj {
	struct frame_list pagelist; /* list of pages containing data from underlying data store */
	struct vm_pager *pager;    /* the underlying data store */
};

/*
 * Functions
 */
int vm_create_vm_obj(struct vm_pager *pager, struct vm_obj **p_obj);

#endif /* GEEKOS_VM_H */
