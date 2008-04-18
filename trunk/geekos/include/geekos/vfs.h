/*
 * GeekOS - virtual filesystem (VFS)
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

#ifndef GEEKOS_VFS_H
#define GEEKOS_VFS_H

#ifdef KERNEL

#include <stddef.h>
#include <geekos/list.h>
#include <geekos/synch.h>

/* maximum length for a path name */
#define VFS_PATHLEN_MAX 1023

/* maximum length for a file name */
#define VFS_NAMELEN_MAX 255

#ifndef ASM

struct fs_driver;
struct fs_instance;
struct inode;
struct pagecache;

DECLARE_LIST(inode_list, inode);

/*
 * Operations to be defined by filesystem drivers.
 */
struct fs_driver_ops {
	const char *(*get_name)(struct fs_driver *fs);
	int (*create_instance)(struct fs_driver *fs, const char *init, struct fs_instance **p_instance);
};

/*
 * Filesystem driver.
 */
struct fs_driver {
	struct fs_driver_ops *ops;    /* operations */
	struct fs_driver *next;       /* link to next fs_driver */
	void *fs_data;                /* for use by filesystem driver */
};

/*
 * Operations to be defined by filesystem instances.
 */
struct fs_instance_ops {
	int (*get_root)(struct fs_instance *instance, struct inode **p_dir);
	int (*open)(struct fs_instance *instance, const char *path, int mode, struct inode **p_inode);
	int (*close)(struct fs_instance *instance);
};

/*
 * An instance of a filesystem on a particular block device.
 */
struct fs_instance {
	struct fs_instance_ops *ops;  /* operations */
	int refcount;                 /* reference count */
	void *fs_data;                /* for use by filesystem driver */
};

/*
 * Operations to be defined by inodes.
 */
struct inode_ops {
#if 0
	int (*read)(struct inode *inode, void *buf, size_t len);  /* files only */
	int (*write)(struct inode *inode, void *buf, size_t len); /* files only */
#endif
	int (*close)(struct inode *inode);
	int (*lookup)(struct inode *inode, const char *name, struct inode **p_inode); /* dirs only */
};

typedef enum { VFS_FILE, VFS_DIR } vfs_inode_type_t;

/*
 * A inode: the object used to access a particular file or directory on-disk.
 * The in-memory tree of files and directories is constructed
 * out of inodes.
 */
struct inode {
	struct inode_ops *ops;        /* operations */
	struct inode *parent;         /* parent directory */
	vfs_inode_type_t type;        /* type: file or directory */
	char *name;                   /* filename string */
	struct inode_list child_list; /* list of child files and directories */
	DEFINE_LINK(inode_list, inode);/* link fields for inode_list */
	int refcount;                 /* reference count */
	bool busy;                    /* true if a lookup is in progress */
	struct condition inode_cond;  /* condition to serialize lookups; must hold fs mutex */
#ifdef notyet
	struct pagecache *pagecache;  /* pagecache for file/dir data */
#endif
	void *fs_data;                /* for use by filesystem driver */
};

int vfs_register_fs_driver(struct fs_driver *fs);
int vfs_mount_root(struct fs_instance *instance);

int vfs_get_root_dir(struct inode **p_dir);
int vfs_lookup_inode(struct inode *start_dir, const char *path, struct inode **p_inode);
void vfs_release_ref(struct inode *inode);

int vfs_read(struct inode *inode, void *buf, size_t len);
int vfs_write(struct inode *inode, void *buf, size_t len);
int vfs_close(struct inode *inode);

#endif /* ifndef ASM */

#endif /* ifdef KERNEL */

#endif /* ifndef GEEKOS_VFS_H */
