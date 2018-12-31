/* ------------------------------------------------------------------------------------------------------------------------------
    vs10xx queue functions.
    Copyright (C) 2010-2013 Richard van Paasen <rvp-nl@t3i.nl>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
   ----------------------------------------------------------------------------------------------------------------------------- */

#include "vs10xx_common.h"
#include "vs10xx_queue.h"

#include <linux/slab.h>

/* queue size*/
static int queuelen = 2048;
module_param(queuelen, int, 0644);

struct vs10xx_queue_entry_t {
	struct list_head list;
	struct vs10xx_queue_buf_t buf;
};

struct vs10xx_queue_t {
	int id;
	int free;
	int valid;
	struct list_head buf_pool;
	struct list_head buf_data;
};

static struct vs10xx_queue_t vs10xx_queue[VS10XX_MAX_DEVICES];

/* ----------------------------------------------------------------------------------------------------------------------------- */
/* VS10XX QUEUE BUFFERS                                                                                                          */
/* ----------------------------------------------------------------------------------------------------------------------------- */

static int vs10xx_queue_alloc(int id) {

	struct vs10xx_queue_t *queue = &vs10xx_queue[id];
	struct vs10xx_queue_entry_t *entry;
	int i, status = 0;

	/* create buf_pool list */
	INIT_LIST_HEAD(&queue->buf_pool);

	/* create buf_data list */
	INIT_LIST_HEAD(&queue->buf_data);

	/* allocate buffers in pool */
	for(i = 0; (status == 0) && (i < queuelen); i++) {
		entry = kzalloc(sizeof *entry, GFP_KERNEL);
		if (!entry) {
			vs10xx_err("kzalloc queue entry");
			status = -1;
		} else {
			list_add(&entry->list, &queue->buf_pool);
		}
	}

	queue->free = ((status == 0) ? queuelen : 0);
	queue->valid = ((status == 0) ? 1 : 0);

	vs10xx_inf("id:%d allocated %d/%d buffers (%d bytes)", queue->id, i, queuelen, (i * sizeof(entry->buf)));

	return status;
}

void vs10xx_queue_flush(int id) {

	struct vs10xx_queue_t *queue = &vs10xx_queue[id];
	struct vs10xx_queue_entry_t *entry, *n;

	/* move all buffers to pool */
	if (!list_empty(&queue->buf_data)) {
		vs10xx_dbg("id:%d flush queue", id);
		list_for_each_entry_safe(entry, n, &queue->buf_data, list) {
			list_move_tail(&entry->list, &queue->buf_pool);
		}
	}
}

static void vs10xx_queue_free(int id) {

	struct vs10xx_queue_t *queue = &vs10xx_queue[id];
	struct vs10xx_queue_entry_t *entry, *n;

	if (queue->valid) {

		vs10xx_queue_flush(id);

		/* free buffers */
		list_for_each_entry_safe(entry, n, &queue->buf_pool, list) {
			list_del(&entry->list);
			kfree(entry);
		}

		queue->free = 0;
		queue->valid = 0;
	}
}

int vs10xx_queue_isfull(int id) {

	struct vs10xx_queue_t *queue = &vs10xx_queue[id];

	return list_empty(&queue->buf_pool);

}

int vs10xx_queue_isempty(int id) {

	struct vs10xx_queue_t *queue = &vs10xx_queue[id];

	return list_empty(&queue->buf_data);

}

int vs10xx_queue_getfree(int id) {

	struct vs10xx_queue_t *queue = &vs10xx_queue[id];

	return queue->free;
}

struct vs10xx_queue_buf_t* vs10xx_queue_getslot(int id) {

	struct vs10xx_queue_t *queue = &vs10xx_queue[id];
	/* get the first (empty) data entry from the data queue */
	struct vs10xx_queue_entry_t *entry = list_first_entry(&queue->buf_pool, struct vs10xx_queue_entry_t, list);

	return &entry->buf;
}

struct vs10xx_queue_buf_t* vs10xx_queue_gethead(int id) {

	struct vs10xx_queue_t *queue = &vs10xx_queue[id];
	/* get the first (filled) data entry from the data queue */
	struct vs10xx_queue_entry_t *entry = list_first_entry(&queue->buf_data, struct vs10xx_queue_entry_t, list);

	return &entry->buf;
}

void vs10xx_queue_enqueue(int id) {

	struct vs10xx_queue_t *queue = &vs10xx_queue[id];
	/* could ask the caller for the entry, but let's do not to prevent queue corruption */
	struct vs10xx_queue_entry_t *entry = list_first_entry(&queue->buf_pool, struct vs10xx_queue_entry_t, list);
	if (entry->buf.len > 0) {
		list_move_tail(&entry->list, &queue->buf_data);
		queue->free -= 1;
	}
}

void vs10xx_queue_dequeue(int id) {

	struct vs10xx_queue_t *queue = &vs10xx_queue[id];
	/* could ask the caller for the entry, but let's do not to prevent queue corruption */
	struct vs10xx_queue_entry_t *entry = list_first_entry(&queue->buf_data, struct vs10xx_queue_entry_t, list);
	list_move_tail(&entry->list, &queue->buf_pool);
	entry->buf.len = 0;
	queue->free += 1;
}

/* ----------------------------------------------------------------------------------------------------------------------------- */
/* VS10XX QUEUE INIT/EXIT                                                                                                        */
/* ----------------------------------------------------------------------------------------------------------------------------- */

int vs10xx_queue_init(int id) {

	int status;

	vs10xx_queue[id].id = id;
	status = vs10xx_queue_alloc(id);

	return status;
}

void vs10xx_queue_exit(int id) {

	vs10xx_queue_free(id);

}
