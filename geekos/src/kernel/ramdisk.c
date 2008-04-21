/*
 * GeekOS - ramdisk block device
 *
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

#include <geekos/ramdisk.h>
#include <geekos/blockdev.h>
#include <geekos/mem.h>
#include <geekos/kassert.h>
#include <geekos/workqueue.h>
#include <geekos/string.h>

/*
 * NOTES:
 * - dev->data points to the ramdisk_data object for the device
 */

#define RAMDISK_BLOCK_SIZE 512
#define RAMDISK_NUM_BLOCKS(dev) ((dev)->size / (dev)->block_size)

struct ramdisk_data {
	char *buf;
	size_t size;
	unsigned block_size;
};

/*
 * Ramdisk workqueue callback function.
 * Performs block read and write requests by copying data
 * to/from the ramdisk buffer.
 */
void ramdisk_handle_request(void *data)
{
	struct blockdev_req *req = data;
	struct ramdisk_data *rd = req->dev->data;
	char *ramdisk_buf;
	size_t copy_size;

	/* Range check (TODO: handle boundary/overflow conditions) */
	KASSERT(req->lba + req->num_blocks <= RAMDISK_NUM_BLOCKS(rd));

	ramdisk_buf = rd->buf + (req->lba * rd->block_size);
	copy_size = req->num_blocks * rd->block_size;

	if (req->type == BLOCKDEV_REQ_READ) {
		/* block read */
		memcpy(req->buf, ramdisk_buf, copy_size);
	} else {
		/* block write */
		memcpy(ramdisk_buf, req->buf, copy_size);
	}

	/* notify that the I/O is complete */
	blockdev_notify_complete(req, BLOCKDEV_REQ_FINISHED);
}

void ramdisk_post_request(struct blockdev *dev, struct blockdev_req *req)
{
	/* schedule the request for later handling by the workqueue thread */
	workqueue_schedule_work(&ramdisk_handle_request, req);
}

ulong_t ramdisk_get_num_blocks(struct blockdev *dev)
{
	return RAMDISK_NUM_BLOCKS((struct ramdisk_data *) dev->data);
}

unsigned ramdisk_get_block_size(struct blockdev *dev)
{
	struct ramdisk_data *rd = dev->data;
	return rd->block_size;
}

static struct blockdev_ops s_ramdisk_blockdev_ops = {
	.post_request = &ramdisk_post_request,
	.get_num_blocks = &ramdisk_get_num_blocks,
	.get_block_size = &ramdisk_get_block_size,
};

struct blockdev *ramdisk_create(void *buf, size_t size)
{
	struct blockdev *dev;
	struct ramdisk_data *rd;

	dev = mem_alloc(sizeof(struct blockdev));
	dev->ops = &s_ramdisk_blockdev_ops;
	
	rd = mem_alloc(sizeof(struct ramdisk_data));
	rd->buf = buf;
	rd->size = size;
	rd->block_size = RAMDISK_BLOCK_SIZE;

	dev->data = rd;

	return dev;
}
