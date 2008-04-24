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

#include <geekos/vm.h>

int vm_create_vm_obj(struct vm_pager *pager, struct vm_obj **p_obj)
{
	struct vm_obj *obj;

	obj = mem_alloc(sizeof(struct vm_obj));
	frame_list_clear(&obj->pagelist);
	obj->pager = pager;

	*p_obj = obj;
	return 0;
}
