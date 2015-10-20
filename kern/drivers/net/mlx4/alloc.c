/*
 * Copyright (c) 2006, 2007 Cisco Systems, Inc.  All rights reserved.
 * Copyright (c) 2007, 2008 Mellanox Technologies. All rights reserved.
 *
 * This software is available to you under a choice of one of two
 * licenses.  You may choose to be licensed under the terms of the GNU
 * General Public License (GPL) Version 2, available from the file
 * COPYING in the main directory of this source tree, or the
 * OpenIB.org BSD license below:
 *
 *     Redistribution and use in source and binary forms, with or
 *     without modification, are permitted provided that the following
 *     conditions are met:
 *
 *      - Redistributions of source code must retain the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer.
 *
 *      - Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials
 *        provided with the distribution.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <linux_compat.h>
#include "mlx4.h"

uint32_t mlx4_bitmap_alloc(struct mlx4_bitmap *bitmap)
{
	uint32_t obj;

	spin_lock(&bitmap->lock);

	obj = find_next_zero_bit(bitmap->table, bitmap->max, bitmap->last);
	if (obj >= bitmap->max) {
		bitmap->top = (bitmap->top + bitmap->max + bitmap->reserved_top)
				& bitmap->mask;
		obj = find_first_zero_bit(bitmap->table, bitmap->max);
	}

	if (obj < bitmap->max) {
		set_bit(obj, bitmap->table);
		bitmap->last = (obj + 1);
		if (bitmap->last == bitmap->max)
			bitmap->last = 0;
		obj |= bitmap->top;
	} else
		obj = -1;

	if (obj != -1)
		--bitmap->avail;

	spin_unlock(&bitmap->lock);

	return obj;
}

void mlx4_bitmap_free(struct mlx4_bitmap *bitmap, uint32_t obj, int use_rr)
{
	mlx4_bitmap_free_range(bitmap, obj, 1, use_rr);
}

static unsigned long find_aligned_range(unsigned long *bitmap,
					uint32_t start, uint32_t nbits,
					int len, int align,
					uint32_t skip_mask)
{
	unsigned long end, i;

again:
	start = ALIGN(start, align);

	while ((start < nbits) && (test_bit(start, bitmap) ||
				   (start & skip_mask)))
		start += align;

	if (start >= nbits)
		return -1;

	end = start+len;
	if (end > nbits)
		return -1;

	for (i = start + 1; i < end; i++) {
		if (test_bit(i, bitmap) || ((uint32_t)i & skip_mask)) {
			start = i + 1;
			goto again;
		}
	}

	return start;
}

uint32_t mlx4_bitmap_alloc_range(struct mlx4_bitmap *bitmap, int cnt,
			    int align, uint32_t skip_mask)
{
	uint32_t obj;

	if (likely(cnt == 1 && align == 1 && !skip_mask))
		return mlx4_bitmap_alloc(bitmap);

	spin_lock(&bitmap->lock);

	obj = find_aligned_range(bitmap->table, bitmap->last,
				 bitmap->max, cnt, align, skip_mask);
	if (obj >= bitmap->max) {
		bitmap->top = (bitmap->top + bitmap->max + bitmap->reserved_top)
				& bitmap->mask;
		obj = find_aligned_range(bitmap->table, 0, bitmap->max,
					 cnt, align, skip_mask);
	}

	if (obj < bitmap->max) {
		bitmap_set(bitmap->table, obj, cnt);
		if (obj == bitmap->last) {
			bitmap->last = (obj + cnt);
			if (bitmap->last >= bitmap->max)
				bitmap->last = 0;
		}
		obj |= bitmap->top;
	} else
		obj = -1;

	if (obj != -1)
		bitmap->avail -= cnt;

	spin_unlock(&bitmap->lock);

	return obj;
}

uint32_t mlx4_bitmap_avail(struct mlx4_bitmap *bitmap)
{
	return bitmap->avail;
}

static uint32_t mlx4_bitmap_masked_value(struct mlx4_bitmap *bitmap,
					 uint32_t obj)
{
	return obj & (bitmap->max + bitmap->reserved_top - 1);
}

void mlx4_bitmap_free_range(struct mlx4_bitmap *bitmap, uint32_t obj, int cnt,
			    int use_rr)
{
	obj &= bitmap->max + bitmap->reserved_top - 1;

	spin_lock(&bitmap->lock);
	if (!use_rr) {
		bitmap->last = MIN(bitmap->last, obj);
		bitmap->top = (bitmap->top + bitmap->max + bitmap->reserved_top)
				& bitmap->mask;
	}
	bitmap_clear(bitmap->table, obj, cnt);
	bitmap->avail += cnt;
	spin_unlock(&bitmap->lock);
}

int mlx4_bitmap_init(struct mlx4_bitmap *bitmap, uint32_t num, uint32_t mask,
		     uint32_t reserved_bot, uint32_t reserved_top)
{
	/* num must be a power of 2 */
	if (num != ROUNDUPPWR2(num))
		return -EINVAL;

	bitmap->last = 0;
	bitmap->top  = 0;
	bitmap->max  = num - reserved_top;
	bitmap->mask = mask;
	bitmap->reserved_top = reserved_top;
	bitmap->avail = num - reserved_top - reserved_bot;
	bitmap->effective_len = bitmap->avail;
	spinlock_init(&bitmap->lock);
	bitmap->table = kzmalloc(BITS_TO_LONGS(bitmap->max) * sizeof(long),
				 KMALLOC_WAIT);
	if (!bitmap->table)
		return -ENOMEM;

	bitmap_set(bitmap->table, 0, reserved_bot);

	return 0;
}

void mlx4_bitmap_cleanup(struct mlx4_bitmap *bitmap)
{
	kfree(bitmap->table);
}

struct mlx4_zone_allocator {
	struct list_head		entries;
	struct list_head		prios;
	uint32_t				last_uid;
	uint32_t				mask;
	/* protect the zone_allocator from concurrent accesses */
	spinlock_t			lock;
	enum mlx4_zone_alloc_flags	flags;
};

struct mlx4_zone_entry {
	struct list_head		list;
	struct list_head		prio_list;
	uint32_t				uid;
	struct mlx4_zone_allocator	*allocator;
	struct mlx4_bitmap		*bitmap;
	int				use_rr;
	int				priority;
	int				offset;
	enum mlx4_zone_flags		flags;
};

struct mlx4_zone_allocator *mlx4_zone_allocator_create(enum mlx4_zone_alloc_flags flags)
{
	struct mlx4_zone_allocator *zones = kmalloc(sizeof(*zones),
						    KMALLOC_WAIT);

	if (NULL == zones)
		return NULL;

	INIT_LIST_HEAD(&zones->entries);
	INIT_LIST_HEAD(&zones->prios);
	spinlock_init(&zones->lock);
	zones->last_uid = 0;
	zones->mask = 0;
	zones->flags = flags;

	return zones;
}

int mlx4_zone_add_one(struct mlx4_zone_allocator *zone_alloc,
		      struct mlx4_bitmap *bitmap,
		      uint32_t flags,
		      int priority,
		      int offset,
		      uint32_t *puid)
{
	uint32_t mask = mlx4_bitmap_masked_value(bitmap, (uint32_t)-1);
	struct mlx4_zone_entry *it;
	struct mlx4_zone_entry *zone = kmalloc(sizeof(*zone), KMALLOC_WAIT);

	if (NULL == zone)
		return -ENOMEM;

	zone->flags = flags;
	zone->bitmap = bitmap;
	zone->use_rr = (flags & MLX4_ZONE_USE_RR) ? MLX4_USE_RR : 0;
	zone->priority = priority;
	zone->offset = offset;

	spin_lock(&zone_alloc->lock);

	zone->uid = zone_alloc->last_uid++;
	zone->allocator = zone_alloc;

	if (zone_alloc->mask < mask)
		zone_alloc->mask = mask;

	list_for_each_entry(it, &zone_alloc->prios, prio_list)
		if (it->priority >= priority)
			break;

	if (&it->prio_list == &zone_alloc->prios || it->priority > priority)
		list_add_tail(&zone->prio_list, &it->prio_list);
	list_add_tail(&zone->list, &it->list);

	spin_unlock(&zone_alloc->lock);

	*puid = zone->uid;

	return 0;
}

/* Should be called under a lock */
static int __mlx4_zone_remove_one_entry(struct mlx4_zone_entry *entry)
{
	struct mlx4_zone_allocator *zone_alloc = entry->allocator;

	if (!list_empty(&entry->prio_list)) {
		/* Check if we need to add an alternative node to the prio list */
		if (!list_is_last(&entry->list, &zone_alloc->entries)) {
			struct mlx4_zone_entry *next = list_first_entry(&entry->list,
									typeof(*next),
									list);

			if (next->priority == entry->priority)
				list_add_tail(&next->prio_list, &entry->prio_list);
		}

		list_del(&entry->prio_list);
	}

	list_del(&entry->list);

	if (zone_alloc->flags & MLX4_ZONE_ALLOC_FLAGS_NO_OVERLAP) {
		uint32_t mask = 0;
		struct mlx4_zone_entry *it;

		list_for_each_entry(it, &zone_alloc->prios, prio_list) {
			uint32_t cur_mask = mlx4_bitmap_masked_value(it->bitmap,
								     (uint32_t)-1);

			if (mask < cur_mask)
				mask = cur_mask;
		}
		zone_alloc->mask = mask;
	}

	return 0;
}

void mlx4_zone_allocator_destroy(struct mlx4_zone_allocator *zone_alloc)
{
	struct mlx4_zone_entry *zone, *tmp;

	spin_lock(&zone_alloc->lock);

	list_for_each_entry_safe(zone, tmp, &zone_alloc->entries, list) {
		list_del(&zone->list);
		list_del(&zone->prio_list);
		kfree(zone);
	}

	spin_unlock(&zone_alloc->lock);
	kfree(zone_alloc);
}

/* Should be called under a lock */
static uint32_t __mlx4_alloc_from_zone(struct mlx4_zone_entry *zone, int count,
				  int align, uint32_t skip_mask,
				  uint32_t *puid)
{
	uint32_t uid;
	uint32_t res;
	struct mlx4_zone_allocator *zone_alloc = zone->allocator;
	struct mlx4_zone_entry *curr_node;

	res = mlx4_bitmap_alloc_range(zone->bitmap, count,
				      align, skip_mask);

	if (res != (uint32_t)-1) {
		res += zone->offset;
		uid = zone->uid;
		goto out;
	}

	list_for_each_entry(curr_node, &zone_alloc->prios, prio_list) {
		if (unlikely(curr_node->priority == zone->priority))
			break;
	}

	if (zone->flags & MLX4_ZONE_ALLOW_ALLOC_FROM_LOWER_PRIO) {
		struct mlx4_zone_entry *it = curr_node;

		list_for_each_entry_continue_reverse(it, &zone_alloc->entries, list) {
			res = mlx4_bitmap_alloc_range(it->bitmap, count,
						      align, skip_mask);
			if (res != (uint32_t)-1) {
				res += it->offset;
				uid = it->uid;
				goto out;
			}
		}
	}

	if (zone->flags & MLX4_ZONE_ALLOW_ALLOC_FROM_EQ_PRIO) {
		struct mlx4_zone_entry *it = curr_node;

		list_for_each_entry_from(it, &zone_alloc->entries, list) {
			if (unlikely(it == zone))
				continue;

			if (unlikely(it->priority != curr_node->priority))
				break;

			res = mlx4_bitmap_alloc_range(it->bitmap, count,
						      align, skip_mask);
			if (res != (uint32_t)-1) {
				res += it->offset;
				uid = it->uid;
				goto out;
			}
		}
	}

	if (zone->flags & MLX4_ZONE_FALLBACK_TO_HIGHER_PRIO) {
		if (list_is_last(&curr_node->prio_list, &zone_alloc->prios))
			goto out;

		curr_node = list_first_entry(&curr_node->prio_list,
					     typeof(*curr_node),
					     prio_list);

		list_for_each_entry_from(curr_node, &zone_alloc->entries, list) {
			res = mlx4_bitmap_alloc_range(curr_node->bitmap, count,
						      align, skip_mask);
			if (res != (uint32_t)-1) {
				res += curr_node->offset;
				uid = curr_node->uid;
				goto out;
			}
		}
	}

out:
	if (NULL != puid && res != (uint32_t)-1)
		*puid = uid;
	return res;
}

/* Should be called under a lock */
static void __mlx4_free_from_zone(struct mlx4_zone_entry *zone, uint32_t obj,
				  uint32_t count)
{
	mlx4_bitmap_free_range(zone->bitmap, obj - zone->offset, count, zone->use_rr);
}

/* Should be called under a lock */
static struct mlx4_zone_entry *__mlx4_find_zone_by_uid(
		struct mlx4_zone_allocator *zones, uint32_t uid)
{
	struct mlx4_zone_entry *zone;

	list_for_each_entry(zone, &zones->entries, list) {
		if (zone->uid == uid)
			return zone;
	}

	return NULL;
}

struct mlx4_bitmap *mlx4_zone_get_bitmap(struct mlx4_zone_allocator *zones,
					 uint32_t uid)
{
	struct mlx4_zone_entry *zone;
	struct mlx4_bitmap *bitmap;

	spin_lock(&zones->lock);

	zone = __mlx4_find_zone_by_uid(zones, uid);

	bitmap = zone == NULL ? NULL : zone->bitmap;

	spin_unlock(&zones->lock);

	return bitmap;
}

int mlx4_zone_remove_one(struct mlx4_zone_allocator *zones, uint32_t uid)
{
	struct mlx4_zone_entry *zone;
	int res;

	spin_lock(&zones->lock);

	zone = __mlx4_find_zone_by_uid(zones, uid);

	if (NULL == zone) {
		res = -1;
		goto out;
	}

	res = __mlx4_zone_remove_one_entry(zone);

out:
	spin_unlock(&zones->lock);
	kfree(zone);

	return res;
}

/* Should be called under a lock */
static struct mlx4_zone_entry *__mlx4_find_zone_by_uid_unique(
		struct mlx4_zone_allocator *zones, uint32_t obj)
{
	struct mlx4_zone_entry *zone, *zone_candidate = NULL;
	uint32_t dist = (uint32_t)-1;

	/* Search for the smallest zone that this obj could be
	 * allocated from. This is done in order to handle
	 * situations when small bitmaps are allocated from bigger
	 * bitmaps (and the allocated space is marked as reserved in
	 * the bigger bitmap.
	 */
	list_for_each_entry(zone, &zones->entries, list) {
		if (obj >= zone->offset) {
			uint32_t mobj = (obj - zone->offset) & zones->mask;

			if (mobj < zone->bitmap->max) {
				uint32_t curr_dist = zone->bitmap->effective_len;

				if (curr_dist < dist) {
					dist = curr_dist;
					zone_candidate = zone;
				}
			}
		}
	}

	return zone_candidate;
}

uint32_t mlx4_zone_alloc_entries(struct mlx4_zone_allocator *zones,
				 uint32_t uid, int count,
				 int align, uint32_t skip_mask,
				 uint32_t *puid)
{
	struct mlx4_zone_entry *zone;
	int res = -1;

	spin_lock(&zones->lock);

	zone = __mlx4_find_zone_by_uid(zones, uid);

	if (NULL == zone)
		goto out;

	res = __mlx4_alloc_from_zone(zone, count, align, skip_mask, puid);

out:
	spin_unlock(&zones->lock);

	return res;
}

uint32_t mlx4_zone_free_entries(struct mlx4_zone_allocator *zones,
				uint32_t uid, uint32_t obj, uint32_t count)
{
	struct mlx4_zone_entry *zone;
	int res = 0;

	spin_lock(&zones->lock);

	zone = __mlx4_find_zone_by_uid(zones, uid);

	if (NULL == zone) {
		res = -1;
		goto out;
	}

	__mlx4_free_from_zone(zone, obj, count);

out:
	spin_unlock(&zones->lock);

	return res;
}

uint32_t mlx4_zone_free_entries_unique(struct mlx4_zone_allocator *zones,
				       uint32_t obj, uint32_t count)
{
	struct mlx4_zone_entry *zone;
	int res;

	if (!(zones->flags & MLX4_ZONE_ALLOC_FLAGS_NO_OVERLAP))
		return -EFAULT;

	spin_lock(&zones->lock);

	zone = __mlx4_find_zone_by_uid_unique(zones, obj);

	if (NULL == zone) {
		res = -1;
		goto out;
	}

	__mlx4_free_from_zone(zone, obj, count);
	res = 0;

out:
	spin_unlock(&zones->lock);

	return res;
}
/*
 * Handling for queue buffers -- we allocate a bunch of memory and
 * register it in a memory region at HCA virtual address 0.  If the
 * requested size is > max_direct, we split the allocation into
 * multiple pages, so we don't require too much contiguous memory.
 */

int mlx4_buf_alloc(struct mlx4_dev *dev, int size, int max_direct,
		   struct mlx4_buf *buf, gfp_t gfp)
{
	dma_addr_t t;

#if 0 // AKAROS_PORT
	if (size <= max_direct) {
#else
	if (size > max_direct)
		printd("size %p max_direct %p\n", size, max_direct);
	if (1) {
#endif
		buf->nbufs        = 1;
		buf->npages       = 1;
		buf->page_shift   = get_order(size) + PAGE_SHIFT;
		buf->direct.buf   = dma_alloc_coherent(&dev->persist->pdev->dev,
						       size, &t, gfp);
		if (!buf->direct.buf)
			return -ENOMEM;

		buf->direct.map = t;

		while (t & ((1 << buf->page_shift) - 1)) {
			--buf->page_shift;
			buf->npages *= 2;
		}

		memset(buf->direct.buf, 0, size);
	} else {
		panic("Disabled");
#if 0 // AKAROS_PORT
		int i;

		buf->direct.buf  = NULL;
		buf->nbufs       = (size + PAGE_SIZE - 1) / PAGE_SIZE;
		buf->npages      = buf->nbufs;
		buf->page_shift  = PAGE_SHIFT;
		buf->page_list   = kzmalloc((buf->nbufs) * (sizeof(*buf->page_list)),
					    gfp);
		if (!buf->page_list)
			return -ENOMEM;

		for (i = 0; i < buf->nbufs; ++i) {
			buf->page_list[i].buf =
				dma_alloc_coherent(&dev->persist->pdev->dev,
						   PAGE_SIZE,
						   &t, gfp);
			if (!buf->page_list[i].buf)
				goto err_free;

			buf->page_list[i].map = t;

			memset(buf->page_list[i].buf, 0, PAGE_SIZE);
		}

		if (BITS_PER_LONG == 64) {
			struct page **pages;
			pages = kmalloc(sizeof *pages * buf->nbufs, gfp);
			if (!pages)
				goto err_free;
			for (i = 0; i < buf->nbufs; ++i)
				pages[i] = kva2page(buf->page_list[i].buf);
			buf->direct.buf = vmap(pages, buf->nbufs, VM_MAP, PAGE_KERNEL);
			kfree(pages);
			if (!buf->direct.buf)
				goto err_free;
		}
#endif
	}

	return 0;

err_free:
	mlx4_buf_free(dev, size, buf);

	return -ENOMEM;
}
EXPORT_SYMBOL_GPL(mlx4_buf_alloc);

void mlx4_buf_free(struct mlx4_dev *dev, int size, struct mlx4_buf *buf)
{
	int i;

	if (buf->nbufs == 1)
		dma_free_coherent(&dev->persist->pdev->dev, size,
				  buf->direct.buf,
				  buf->direct.map);
	else {
		panic("Disabled");
#if 0 // AKAROS_PORT
		if (BITS_PER_LONG == 64)
			vunmap(buf->direct.buf);

		for (i = 0; i < buf->nbufs; ++i)
			if (buf->page_list[i].buf)
				dma_free_coherent(&dev->persist->pdev->dev,
						  PAGE_SIZE,
						  buf->page_list[i].buf,
						  buf->page_list[i].map);
		kfree(buf->page_list);
#endif
	}
}
EXPORT_SYMBOL_GPL(mlx4_buf_free);

static struct mlx4_db_pgdir *mlx4_alloc_db_pgdir(struct device *dma_device,
						 gfp_t gfp)
{
	struct mlx4_db_pgdir *pgdir;

	pgdir = kzmalloc(sizeof *pgdir, gfp);
	if (!pgdir)
		return NULL;

	bitmap_fill(pgdir->order1, MLX4_DB_PER_PAGE / 2);
	pgdir->bits[0] = pgdir->order0;
	pgdir->bits[1] = pgdir->order1;
	pgdir->db_page = dma_alloc_coherent(dma_device, PAGE_SIZE,
					    &pgdir->db_dma, gfp);
	if (!pgdir->db_page) {
		kfree(pgdir);
		return NULL;
	}

	return pgdir;
}

static int mlx4_alloc_db_from_pgdir(struct mlx4_db_pgdir *pgdir,
				    struct mlx4_db *db, int order)
{
	int o;
	int i;

	for (o = order; o <= 1; ++o) {
		i = find_first_bit(pgdir->bits[o], MLX4_DB_PER_PAGE >> o);
		if (i < MLX4_DB_PER_PAGE >> o)
			goto found;
	}

	return -ENOMEM;

found:
	clear_bit(i, pgdir->bits[o]);

	i <<= o;

	if (o > order)
		set_bit(i ^ 1, pgdir->bits[order]);

	db->u.pgdir = pgdir;
	db->index   = i;
	db->db      = pgdir->db_page + db->index;
	db->dma     = pgdir->db_dma  + db->index * 4;
	db->order   = order;

	return 0;
}

int mlx4_db_alloc(struct mlx4_dev *dev, struct mlx4_db *db, int order, gfp_t gfp)
{
	struct mlx4_priv *priv = mlx4_priv(dev);
	struct mlx4_db_pgdir *pgdir;
	int ret = 0;

	qlock(&priv->pgdir_mutex);

	list_for_each_entry(pgdir, &priv->pgdir_list, list)
		if (!mlx4_alloc_db_from_pgdir(pgdir, db, order))
			goto out;

#if 0 // AKAROS_PORT
	pgdir = mlx4_alloc_db_pgdir(&dev->persist->pdev->dev, gfp);
#else
	pgdir = mlx4_alloc_db_pgdir(0, gfp);
#endif
	if (!pgdir) {
		ret = -ENOMEM;
		goto out;
	}

	list_add(&pgdir->list, &priv->pgdir_list);

	/* This should never fail -- we just allocated an empty page: */
	warn_on(mlx4_alloc_db_from_pgdir(pgdir, db, order));

out:
	qunlock(&priv->pgdir_mutex);

	return ret;
}
EXPORT_SYMBOL_GPL(mlx4_db_alloc);

void mlx4_db_free(struct mlx4_dev *dev, struct mlx4_db *db)
{
	struct mlx4_priv *priv = mlx4_priv(dev);
	int o;
	int i;

	qlock(&priv->pgdir_mutex);

	o = db->order;
	i = db->index;

	if (db->order == 0 && test_bit(i ^ 1, db->u.pgdir->order0)) {
		clear_bit(i ^ 1, db->u.pgdir->order0);
		++o;
	}
	i >>= o;
	set_bit(i, db->u.pgdir->bits[o]);

	if (bitmap_full(db->u.pgdir->order1, MLX4_DB_PER_PAGE / 2)) {
		dma_free_coherent(&dev->persist->pdev->dev, PAGE_SIZE,
				  db->u.pgdir->db_page, db->u.pgdir->db_dma);
		list_del(&db->u.pgdir->list);
		kfree(db->u.pgdir);
	}

	qunlock(&priv->pgdir_mutex);
}
EXPORT_SYMBOL_GPL(mlx4_db_free);

int mlx4_alloc_hwq_res(struct mlx4_dev *dev, struct mlx4_hwq_resources *wqres,
		       int size, int max_direct)
{
	int err;

	err = mlx4_db_alloc(dev, &wqres->db, 1, KMALLOC_WAIT);
	if (err)
		return err;

	*wqres->db.db = 0;

	err = mlx4_buf_alloc(dev, size, max_direct, &wqres->buf, KMALLOC_WAIT);
	if (err)
		goto err_db;

	err = mlx4_mtt_init(dev, wqres->buf.npages, wqres->buf.page_shift,
			    &wqres->mtt);
	if (err)
		goto err_buf;

	err = mlx4_buf_write_mtt(dev, &wqres->mtt, &wqres->buf, KMALLOC_WAIT);
	if (err)
		goto err_mtt;

	return 0;

err_mtt:
	mlx4_mtt_cleanup(dev, &wqres->mtt);
err_buf:
	mlx4_buf_free(dev, size, &wqres->buf);
err_db:
	mlx4_db_free(dev, &wqres->db);

	return err;
}
EXPORT_SYMBOL_GPL(mlx4_alloc_hwq_res);

void mlx4_free_hwq_res(struct mlx4_dev *dev, struct mlx4_hwq_resources *wqres,
		       int size)
{
	mlx4_mtt_cleanup(dev, &wqres->mtt);
	mlx4_buf_free(dev, size, &wqres->buf);
	mlx4_db_free(dev, &wqres->db);
}
EXPORT_SYMBOL_GPL(mlx4_free_hwq_res);
