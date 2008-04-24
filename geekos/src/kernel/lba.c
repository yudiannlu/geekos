/*
 * GeekOS - LBA (logical block address) type and operations
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

#include <geekos/range.h>
#include <geekos/lba.h>

/*
 * Create an LBA from a number.
 */
lba_t lba_from_num(u32_t num)
{
	lba_t lba = { .val = num };
	return lba;
}

/*
 * Is the range of blocks described by given start address and
 * number of blocks valid, so that each block in the range
 * has an address less than the total number of blocks?
 */
bool lba_is_range_valid(lba_t start, u32_t num_blocks, u32_t total_blocks)
{
	return range_is_valid_u32(start.val, num_blocks, total_blocks);
}

/*
 * Get the offset in bytes of a block from the start of the
 * block device.
 */
size_t lba_block_offset_in_bytes(lba_t lba, unsigned block_size)
{
	return ((size_t) lba.val) * block_size;
}

/*
 * Get total size of a range of blocks in bytes.
 */
size_t lba_range_size_in_bytes(u32_t num_blocks, unsigned block_size)
{
	return ((size_t) num_blocks) * block_size;
}

