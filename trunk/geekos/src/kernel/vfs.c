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

#include <geekos/vfs.h>
#include <geekos/errno.h>
#include <geekos/string.h>
#include <geekos/mem.h>

/*
 * VFS locking and refcounting rules:
 *
 * - s_fs_mutex must be held while navigating the tree.
 *
 * - A directory inode is marked as busy while performing
 *   a lookup or other operation that depends upon or changes
 *   the tree structure.
 *
 * - s_fs_mutex is unlocked when performing a potentially long-running
 *   I/O operation on a directory inode; re-acquired when finished.
 *   This is safe because the inode is marked as busy, so any
 *   concurrent vfs calls will be blocked if they reach the inode
 *   and try to do something.
 *
 * - An inode's refcount is the number of threads holding an
 *   active reference to the inode, or any tree descendent of the inode.
 *   In other words, if a thread holds a reference to an inode,
 *   it has incremented the refcount of the inode and all of
 *   the inode's tree ancestors back to the root directory.
 */

/* ---------- Private Implementation ---------- */

IMPLEMENT_LIST_GET_FIRST(inode_list, inode)
IMPLEMENT_LIST_NEXT(inode_list, inode)
IMPLEMENT_LIST_APPEND(inode_list, inode)

/* filesystem driver list */
struct mutex s_driver_list_mutex;    /* protects changes/access to fs driver list */
struct fs_driver *s_driver_list;     /* list of filesystem drivers */

/* filesystem data structures */
struct mutex s_fs_mutex;             /* protects changes/access to the tree structure */
struct fs_instance *s_root_instance; /* root filesystem instance */
struct inode *s_root_dir;             /* root directory */

/*
 * Adjust the refcount of given inode and all of its tree ancestors
 * by given delta.
 */
static void vfs_adjust_refcounts(struct inode *inode, int delta)
{
	KASSERT(MUTEX_IS_HELD(&s_fs_mutex));

	for (; inode != 0; inode = inode->parent) {
		KASSERT(inode->refcount >= 0);
		KASSERT(delta > 0 || inode->refcount > 0);
		inode->refcount += delta;
	}
}

/*
 * Determine whether or not given path has more
 * path elements.
 */
static bool vfs_has_more_path_elements(const char *path)
{
	return *path != '\0';
}

/*
 * Copy the next path element in given path buffer
 * into given name buffer.
 * If successful, *p_path is updated to point to the beginning
 * of the remaining path components, and 0 is returned.
 * Otherwise, an error code is returned.
 */
static int vfs_get_next_path_element(const char **p_path, char *namebuf)
{
	size_t namelen = 0;
	const char *p = *p_path;

	KASSERT(*p != '/');

	/* search for next separator (or end of string) */
	while (*p != '/' && *p != '\0') {
		p++;
	}

	/* make sure length of this component is legal */
	namelen = p - *p_path;
	if (namelen > VFS_NAMELEN_MAX)
		return EINVAL;

	/* copy name into buffer */
	memcpy(namebuf, *p_path, namelen);
	namebuf[namelen] = '\0';
	KASSERT(strnlen(namebuf, VFS_NAMELEN_MAX + 1) <= VFS_NAMELEN_MAX);

	/* skip over any path separators */
	while (*p == '/') {
		p++;
	}

	/* update the path pointer to beginning of next path component
	 * (or end of string) */
	*p_path = p;

	return 0;
}

/*
 * Search for named child in given directory.
 * The dir inode must be locked.
 * If sucessful, stores pointer to named child in p_inode and
 * returns 0.  Otherwise, returns error code.
 */
int vfs_lookup_child(struct inode *dir, const char *name, struct inode **p_inode)
{
	int rc = 0;
	struct inode *child;

	KASSERT(MUTEX_IS_HELD(&s_fs_mutex));
	KASSERT(dir->type == VFS_DIR);
	KASSERT(dir->busy);

	/* first, see if the child is already part of the dir's child list */
	for (child = inode_list_get_first(&dir->child_list);
	     child != 0;
	     child = inode_list_next(child)) {
		if (strncmp(name, dir->name, VFS_NAMELEN_MAX) == 0) {
			*p_inode = child;
			goto done;
		}
	}

	/* release fs mutex while search is in progress */
	mutex_unlock(&s_fs_mutex);

	/* look up child from filesystem */
	rc = dir->ops->lookup(dir, name, p_inode);

	/* if lookup succeeded, add to dir's child list */
	if (rc == 0) {
		inode_list_append(&dir->child_list, *p_inode);
	}

	/* re-acquire the fs mutex */
	mutex_lock(&s_fs_mutex);

done:
	return rc;
}

/*
 * Lock a directory in preparation for a lookup.
 * fs mutex must be held.
 */
static void vfs_lock_dir(struct inode *dir)
{
	KASSERT(MUTEX_IS_HELD(&s_fs_mutex));
	while (dir->busy) {
		cond_wait(&dir->inode_cond, &s_fs_mutex);
	}
	dir->busy = true;
}

/*
 * Unlock a directory after completing a lookup.
 * fs mutex must be held.
 */
static void vfs_unlock_dir(struct inode *dir)
{
	KASSERT(MUTEX_IS_HELD(&s_fs_mutex));
	KASSERT(dir->busy);
	dir->busy = false;
	cond_broadcast(&dir->inode_cond);
}

/* ---------- Public Interface ---------- */

/*
 * Register a filesystem driver.
 */
int vfs_register_fs_driver(struct fs_driver *fs)
{
	mutex_lock(&s_driver_list_mutex);

	fs->next = s_driver_list;
	s_driver_list = fs;

	mutex_unlock(&s_driver_list_mutex);

	return 0;
}

/*
 * Mount given instance as the root filesystem.
 */
int vfs_mount_root(struct fs_instance *instance)
{
	int rc;

	KASSERT(instance->refcount == 0);

	mutex_lock(&s_fs_mutex);

	/* make sure root filesystem hasn't already been mounted */
	if (s_root_instance) {
		rc = EEXIST;
		goto done;
	}

	/* get the root directory */
	if ((rc = instance->ops->get_root(instance, &s_root_dir)) != 0) {
		goto done;
	}
	KASSERT(s_root_dir->refcount == 0);

	/* set root instance and add reference */
	s_root_instance = instance;
	instance->refcount++;

done:
	mutex_unlock(&s_fs_mutex);

	return rc;
}

/*
 * Get a pointer to the root directory.
 * Stores a pointer to the root directory in *p_dir and adds a reference if successful.
 * Returns EEXIST if the root filesystem hasn't been mounted.
 */
int vfs_get_root_dir(struct inode **p_dir)
{
	int rc = 0;

	mutex_lock(&s_fs_mutex);

	/* return EEXIST if root filesystem hasn't been mounted yet */
	if (!s_root_dir) {
		rc = EEXIST;
		goto done;
	}

	/* return ptr to root dir in p_dir and add a reference */
	*p_dir = s_root_dir;
	(*p_dir)->refcount++;

done:
	mutex_lock(&s_fs_mutex);

	return rc;
}

/*
 * Starting from the given start directory, look up the inode
 * named by given relative path.  If successful, a pointer
 * to the named inode (with an incremented refcount) is stored in p_inode,
 * and 0 is returned.  Otherwise, an error code is returned.
 */
int vfs_lookup_inode(struct inode *start_dir, const char *path, struct inode **p_inode)
{
	int rc = 0;
	struct inode *inode = start_dir, *child;
	char *name = 0;

	KASSERT(*path != '/'); /* must be a relative path! */
	KASSERT(start_dir->refcount > 0);

	/* check length of path */
	if (strnlen(path, VFS_PATHLEN_MAX + 1) > VFS_PATHLEN_MAX) {
		return EINVAL;
	}

	/* allocate a name buffer */
	name = mem_alloc(VFS_NAMELEN_MAX + 1);

	mutex_lock(&s_fs_mutex);

	/* increment the refcount of start inode and each tree ancestor */
	vfs_adjust_refcounts(start_dir, 1);

	while (vfs_has_more_path_elements(path)) {
		/* extract one path element */
		if ((rc = vfs_get_next_path_element(&path, name)) != 0) {
			goto done;
		}

		/* current inode needs to be a directory */
		if (inode->type != VFS_DIR) {
			rc = ENOTDIR;
			goto done;
		}

		/* lock dir */
		vfs_lock_dir(inode);

		/* look up child */
		rc = vfs_lookup_child(inode, name, &child);

		/* unlock dir */
		vfs_unlock_dir(inode);

		if (rc != 0) {
			/* child not found */
			goto done;
		}

		/* continue search in child */
		inode = child;
		inode->refcount++;
	}

	/* success: the path is empty and we have located the named inode */
	KASSERT(!vfs_has_more_path_elements(path));
	KASSERT(inode != 0);
	KASSERT(inode->refcount > 0);
	*p_inode = inode;

done:
	if (rc != 0) {
		/* failed search: decrement refcounts of current inode
		 * and all tree ancestors */
		vfs_adjust_refcounts(inode, -1);
	}

	mutex_unlock(&s_fs_mutex);

	mem_free(name);

	return rc;
}

/*
 * Release the reference to given inode.
 */
void vfs_release_ref(struct inode *inode)
{
	mutex_lock(&s_fs_mutex);

	KASSERT(inode->refcount > 0);

	/* decremenent refcounts of inode and all tree ancestors */
	vfs_adjust_refcounts(inode, -1);

	/*
	 * Note: we allow the refcount of a inode to reach 0.
	 * It will be removed from the tree only if the
	 * underlying filesystem file is deleted.
	 */

	mutex_unlock(&s_fs_mutex);
}

int vfs_read(struct inode *inode, void *buf, size_t len)
{
	/* TODO */
	return -1;
}

int vfs_write(struct inode *inode, void *buf, size_t len)
{
	/* TODO */
	return -1;
}

int vfs_close(struct inode *inode)
{
	/* TODO */
	return -1;
}
