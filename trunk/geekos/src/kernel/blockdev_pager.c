/*
 * GeekOS - block device pager
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
#include <geekos/blockdev.h>
#include <geekos/vm.h>
#include <geekos/blockdev_pager.h>

/*
 * Create a vm_pager that pages to/from a (range of) a block device.
 */
int blockdev_pager_create(struct blockdev *dev, lba_t start, u32_t num_blocks, struct vm_pager **p_pager)
{
	/* TODO: implement */
	return -1;
}
