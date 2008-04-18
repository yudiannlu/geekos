/*
 * GeekOS - PFAT filesystem
 *
 * Copyright (C) 2001-2007, David H. Hovemeyer <david.hovemeyer@gmail.com>
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

/* PFAT - a simple FAT-like filesystem */

#ifndef GEEKOS_PFAT_H
#define GEEKOS_PFAT_H

#include <geekos/types.h>

#define PFAT_MAGIC 0x77e2ef5aU

#define PFAT_ENTRY_SIZE 64

/*
 * Data stored in the first block of the filesystem.
 */
struct pfat_superblock {
	u32_t magic;               /* must contain PFAT_MAGIC */
	u32_t fat_lba;             /* lba of FAT */
	u32_t fat_num_entries;     /* number of entries in FAT */
	u32_t block_size;          /* block size in bytes (typically, 512) */
	u32_t blocks_per_cluster;  /* number of blocks in one allocation cluster (typically, 8) */
	u32_t num_reserved_blocks; /* number of reserved disk blocks after superblock */

	/* other metainfo could go here... */
	char reserved[512 - 24];
};

/*
 * Each FAT entry corresponds to one allocation cluster.
 */
struct pfat_entry {
	uint_t used     : 1;     /* true if entry is used */
	uint_t is_dir   : 1;     /* true if entry is a directory */
	uint_t linked   : 1;     /* true if entry is linked from another entry */
	uint_t reserved : 13;    /* reserved for future use */
	uint_t namelen  : 16;    /* length of file/directory name */
	uint_t next_entry;       /* index of next entry in chain */
	char name[PFAT_ENTRY_SIZE - 8];
};

#endif /* GEEKOS_PFAT_H */
