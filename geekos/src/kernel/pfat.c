/*
 * GeekOS - PFAT filesystem
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

/* PFAT - a simple FAT-like filesystem */

#include <geekos/vfs.h>
#include <geekos/dev.h>
#include <geekos/blockdev.h>
#include <geekos/mem.h>
#include <geekos/errno.h>
#include <geekos/range.h>
#include <geekos/pfat.h>

/*
 * NOTES:
 * - p field of fs_instance object points to pfat_instance object
 */

/* ------------------- data types ------------------- */

/*
 * Data structure representing a mounted PFAT filesystem instance.
 */
struct pfat_instance {
	struct blockdev *dev;
	struct pfat_superblock *super;
};

/*
 * fs_driver_ops functions.
 */
static const char *pfat_get_name(struct fs_driver *driver);
static int pfat_create_instance(
	struct fs_driver *fs, const char *init, const char *opts, struct fs_instance **p_instance);

static struct fs_driver_ops s_pfat_driver_ops = {
	.get_name = &pfat_get_name,
	.create_instance = &pfat_create_instance,
};

static struct fs_driver s_pfat_driver = {
	.ops = &s_pfat_driver_ops,
};

/*
 * fs_instance_ops functions
 */
static int pfat_get_root(struct fs_instance *instance, struct inode **p_dir);
static int pfat_open(struct fs_instance *instance, const char *path, int mode, struct inode **p_inode);
static int pfat_close(struct fs_instance *instance);

static struct fs_instance_ops s_pfat_instance_ops = {
	.get_root = &pfat_get_root,
	.open = &pfat_open,
	.close = &pfat_close,
};

/* ------------------- private implementation ------------------- */

static int pfat_read_super(struct blockdev *dev, struct pfat_superblock **p_super)
{
	int rc;
	struct pfat_superblock *super = 0;
	unsigned dev_block_size;
	unsigned super_bufsize;

	/* device block size */
	dev_block_size = blockdev_get_block_size(dev);

	/* read superblock into a buffer */
	super_bufsize = range_umax(sizeof(struct pfat_superblock), dev_block_size);
	super = mem_alloc(super_bufsize);
	rc = blockdev_read_sync(dev, lba_from_num(0), super_bufsize / dev_block_size, super);
	if (rc != 0) {
		goto fail;
	}

	/* check magic */
	if (super->magic != PFAT_MAGIC) {
		rc = EINVAL; /* FIXME: better error code? */
		goto fail;
	}

	/* success! */
	return 0;

fail:
	mem_free(super);
	return rc;
}

static const char *pfat_get_name(struct fs_driver *driver)
{
	return "pfat";
}

static int pfat_create_instance(
	struct fs_driver *fs, const char *init, const char *opts, struct fs_instance **p_instance)
{
	int rc;
	struct fs_instance *fs_inst;
	struct blockdev *dev = 0;
	struct pfat_superblock *super = 0;
	struct pfat_instance *inst_data = 0;

	KASSERT(fs == &s_pfat_driver);

	/* the init parameter is the name of the block device containing the fs */
	rc = dev_find_blockdev(init, &dev);
	if (rc != 0) {
		goto done;
	}

	/* read superblock */
	rc = pfat_read_super(dev, &super);
	if (rc != 0) {
		goto done;
	}

	/* read FAT */

	/* read root directory? */

	/*
	 * things look good - create the instance
	 */
	inst_data = mem_alloc(sizeof(struct pfat_instance));
	inst_data->dev = dev;
	inst_data->super = super;

	rc = vfs_fs_instance_create(&s_pfat_instance_ops, inst_data, &fs_inst);

done:
	if (rc != 0) {
		blockdev_close(dev);
		mem_free(super);
		mem_free(inst_data);
	}
	return rc;
}

static int pfat_get_root(struct fs_instance *instance, struct inode **p_dir)
{
	KASSERT(false);
	return ENOTSUP;
}

static int pfat_open(struct fs_instance *instance, const char *path, int mode, struct inode **p_inode)
{
	KASSERT(false);
	return ENOTSUP;
}

static int pfat_close(struct fs_instance *instance)
{
	KASSERT(false);
	return ENOTSUP;
}

/* ------------------- public interface ------------------- */

int pfat_init(void)
{
	return vfs_register_fs_driver(&s_pfat_driver);
}



