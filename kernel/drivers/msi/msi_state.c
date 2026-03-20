// SPDX-License-Identifier: GPL-2.0
/*
 * msi_state.c — MSI Addressable State (Memory Regions)
 *
 * State regions are mmap-backed byte buffers that cognitive programs
 * use for working memory. Each region is named, sized, and
 * permission-controlled via domain grants.
 *
 * commit() flushes dirty state to persistent storage (if available).
 */

#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/string.h>
#include <linux/uaccess.h>

#include "msi_core.h"

/* External accessors */
extern struct list_head *msi_get_state_list(void);
extern spinlock_t *msi_get_state_list_lock(void);
extern u32 msi_alloc_state_id(void);
extern struct msi_domain *msi_domain_lookup(u32 domain_id);
extern void msi_domain_put(struct msi_domain *d);
extern bool msi_domain_has_state_grant(struct msi_domain *d, const char *name,
                                       enum msi_perms required);

/* ===== Internal helpers ===== */

static struct msi_state_region *msi_find_state_locked(u32 handle_id)
{
	struct msi_state_region *s;

	list_for_each_entry(s, msi_get_state_list(), list) {
		if (s->id == handle_id)
			return s;
	}
	return NULL;
}

static struct msi_state_region *msi_state_lookup(u32 handle_id)
{
	struct msi_state_region *s;
	unsigned long flags;

	spin_lock_irqsave(msi_get_state_list_lock(), flags);
	s = msi_find_state_locked(handle_id);
	spin_unlock_irqrestore(msi_get_state_list_lock(), flags);

	return s;
}

/* ===== Public API ===== */

int msi_state_map(u32 domain_id, const char *name, size_t bytes,
                  enum msi_perms perms, u32 *handle_id)
{
	struct msi_state_region *region;
	struct msi_domain *d = NULL;
	unsigned long flags;

	if (!name || !handle_id || bytes == 0)
		return -EINVAL;

	/* Enforce a sane upper limit per region (256MB) */
	if (bytes > (256UL * 1024 * 1024)) {
		pr_warn("msi: state_map denied — %zu bytes exceeds 256MB limit\n",
		        bytes);
		return -ENOMEM;
	}

	/* Validate domain grant */
	if (domain_id != 0) {
		d = msi_domain_lookup(domain_id);
		if (!d)
			return -ENOENT;
		if (!msi_domain_has_state_grant(d, name, perms)) {
			msi_domain_put(d);
			pr_warn("msi: state_map denied — domain %u lacks grant "
			        "for state '%s' (perms=%d)\n",
			        domain_id, name, perms);
			return -EPERM;
		}
		msi_domain_put(d);
	}

	/* Check for duplicate name */
	spin_lock_irqsave(msi_get_state_list_lock(), flags);
	{
		struct msi_state_region *existing;
		list_for_each_entry(existing, msi_get_state_list(), list) {
			if (strcmp(existing->name, name) == 0) {
				spin_unlock_irqrestore(msi_get_state_list_lock(),
				                       flags);
				/* Return existing handle */
				*handle_id = existing->id;
				return 0;
			}
		}
	}
	spin_unlock_irqrestore(msi_get_state_list_lock(), flags);

	region = kzalloc(sizeof(*region), GFP_KERNEL);
	if (!region)
		return -ENOMEM;

	/* Use vmalloc for large regions, kmalloc for small */
	if (bytes > PAGE_SIZE * 4)
		region->data = vzalloc(bytes);
	else
		region->data = kzalloc(bytes, GFP_KERNEL);

	if (!region->data) {
		kfree(region);
		return -ENOMEM;
	}

	region->id = msi_alloc_state_id();
	strscpy(region->name, name, sizeof(region->name));
	region->size = bytes;
	region->perms = perms;
	region->committed = false;
	spin_lock_init(&region->lock);
	INIT_LIST_HEAD(&region->list);

	spin_lock_irqsave(msi_get_state_list_lock(), flags);
	list_add_tail(&region->list, msi_get_state_list());
	spin_unlock_irqrestore(msi_get_state_list_lock(), flags);

	*handle_id = region->id;

#ifdef MSI_DEBUG
	pr_info("msi: state mapped — id=%u name='%s' size=%zu perms=%d\n",
	        region->id, name, bytes, perms);
#endif

	return 0;
}

int msi_state_read(u32 handle_id, size_t offset, size_t len, void *buf)
{
	struct msi_state_region *region;
	unsigned long flags;

	if (!buf || len == 0)
		return -EINVAL;

	region = msi_state_lookup(handle_id);
	if (!region)
		return -ENOENT;

	/* Bounds check */
	if (offset + len > region->size) {
		pr_warn("msi: state_read out of bounds — handle=%u "
		        "offset=%zu len=%zu size=%zu\n",
		        handle_id, offset, len, region->size);
		return -ERANGE;
	}

	spin_lock_irqsave(&region->lock, flags);
	memcpy(buf, (u8 *)region->data + offset, len);
	spin_unlock_irqrestore(&region->lock, flags);

	return 0;
}

int msi_state_write(u32 handle_id, size_t offset, void *data, size_t len)
{
	struct msi_state_region *region;
	unsigned long flags;

	if (!data || len == 0)
		return -EINVAL;

	region = msi_state_lookup(handle_id);
	if (!region)
		return -ENOENT;

	/* Check write permission */
	if (region->perms != MSI_PERMS_READWRITE) {
		pr_warn("msi: state_write denied — handle=%u is read-only\n",
		        handle_id);
		return -EPERM;
	}

	/* Bounds check */
	if (offset + len > region->size) {
		pr_warn("msi: state_write out of bounds — handle=%u "
		        "offset=%zu len=%zu size=%zu\n",
		        handle_id, offset, len, region->size);
		return -ERANGE;
	}

	spin_lock_irqsave(&region->lock, flags);
	memcpy((u8 *)region->data + offset, data, len);
	region->committed = false;
	spin_unlock_irqrestore(&region->lock, flags);

	return 0;
}

int msi_state_commit(u32 handle_id)
{
	struct msi_state_region *region;
	unsigned long flags;

	region = msi_state_lookup(handle_id);
	if (!region)
		return -ENOENT;

	/*
	 * Commit semantics: mark region as committed.
	 * On real hardware with persistent storage, this would
	 * flush to NVRAM or storage-backed memory.
	 * For now, this is a logical marker for crash recovery.
	 */
	spin_lock_irqsave(&region->lock, flags);
	region->committed = true;
	spin_unlock_irqrestore(&region->lock, flags);

#ifdef MSI_DEBUG
	pr_info("msi: state committed — handle=%u name='%s'\n",
	        handle_id, region->name);
#endif

	return 0;
}

/*
 * Destroy a state region and free its memory.
 * Called during module cleanup or explicit unmap.
 */
void msi_state_destroy(u32 handle_id)
{
	struct msi_state_region *region;
	unsigned long flags;

	spin_lock_irqsave(msi_get_state_list_lock(), flags);
	region = msi_find_state_locked(handle_id);
	if (region)
		list_del(&region->list);
	spin_unlock_irqrestore(msi_get_state_list_lock(), flags);

	if (region) {
		if (region->data) {
			if (region->size > PAGE_SIZE * 4)
				vfree(region->data);
			else
				kfree(region->data);
		}
		kfree(region);
	}
}
