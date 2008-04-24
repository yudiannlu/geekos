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

#ifndef GEEKOS_LBA_H
#define GEEKOS_LBA_H

#include <stddef.h>
#include <geekos/types.h>

/* logical block address type */
/*typedef u32_t lba_t;*/
typedef struct { u32_t val; } lba_t;

lba_t lba_from_num(u32_t num);
bool lba_is_range_valid(lba_t start, u32_t num_blocks, u32_t total_blocks);
size_t lba_block_offset_in_bytes(lba_t lba, unsigned block_size);
size_t lba_range_size_in_bytes(u32_t num_blocks, unsigned block_size);

#endif /* GEEKOS_LBA_H */
