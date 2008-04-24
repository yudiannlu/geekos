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

/*
 * Create a vm_obj using the given pager
 * as its underlying data store.
 */
int vm_create_vm_obj(struct vm_pager *pager, struct vm_obj **p_obj)
{
	struct vm_obj *obj;

	obj = mem_alloc(sizeof(struct vm_obj));

	mutex_init(&obj->lock);
	obj->n_readers = 0;
	obj->n_writers = 0;
	frame_list_clear(&obj->pagelist);
	obj->pager = pager;

	*p_obj = obj;
	return 0;
}

#if 0

/*
 * Lock a page in a vm_obj.
 *
 * Parameters:
 *   obj - the vm_obj
 *   page_num - which page to lock
 *   access - the type of access
 *   p_frame - where to return the pointer to the frame containing the page data
 */
int vm_lock_page(struct vm_obj *obj, u32_t page_num, vm_obj_access_t access, struct frame **p_frame)
{
	int rc;
	struct frame *frame;

	mutex_lock(&obj->lock);

	/*
	 * See if page is already available.
	 */
	for (frame = frame_list_get_first(&obj->pagelist);
	     frame != 0;
	     frame = frame_list_next(frame)) {
		if (frame->vm_obj_page_num == page_num) {
			frame->refcount++;
			break;
		}
	}

	if (frame == 0) {
		/*
		 * Page is not currently present; page it in.
		 * Future optimization; if entire page will
		 * be written by caller, avoid pagein.
		 */
		rc = vm_pagein(obj, page_num, p_frame);
		if (rc != 0) {
			goto done;
		}
	}

done:
	mutex_unlock(&obj->lock);

	return rc;
}

/*
 * Unlock a page in a vm_obj.
 *
 * Parameters:
 *   obj - the vm_obj
 *   access - the type of access
 *   frame - the frame containing the page data
 */
int vm_unlock_page(struct vm_obj *obj, vm_obj_access_t access, struct frame *frame)
{
	mutex_lock(&obj->lock);



	mutex_unlock(&obj->lock);
}

#endif
