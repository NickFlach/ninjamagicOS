// SPDX-License-Identifier: GPL-2.0
/*
 * msi_domain.c — MSI Domain (Capability Container) Management
 *
 * Domains are the security primitive of MSI. Each domain holds
 * a set of grants (capabilities) that control what resources
 * a cognitive program can access. Once sealed, a domain's
 * grants are immutable — no privilege escalation possible.
 */

#include <linux/slab.h>
#include <linux/string.h>

#include "msi_core.h"

/* External accessors from msi_module.c */
extern struct list_head *msi_get_domain_list(void);
extern spinlock_t *msi_get_domain_list_lock(void);
extern u32 msi_alloc_domain_id(void);

/* ===== Internal helpers ===== */

static struct msi_domain *msi_find_domain_locked(u32 domain_id)
{
	struct msi_domain *d;

	list_for_each_entry(d, msi_get_domain_list(), list) {
		if (d->id == domain_id)
			return d;
	}
	return NULL;
}

struct msi_domain *msi_domain_lookup(u32 domain_id)
{
	struct msi_domain *d;
	unsigned long flags;

	spin_lock_irqsave(msi_get_domain_list_lock(), flags);
	d = msi_find_domain_locked(domain_id);
	if (d)
		kref_get(&d->ref);
	spin_unlock_irqrestore(msi_get_domain_list_lock(), flags);

	return d;
}

static void msi_domain_release(struct kref *ref)
{
	struct msi_domain *d = container_of(ref, struct msi_domain, ref);
	struct msi_grant *g, *tmp;

	list_for_each_entry_safe(g, tmp, &d->grants, list) {
		list_del(&g->list);
		kfree(g);
	}
	kfree(d);
}

void msi_domain_put(struct msi_domain *d)
{
	if (d)
		kref_put(&d->ref, msi_domain_release);
}

/* ===== Grant permission checking ===== */

/*
 * Check if a domain has a grant that permits access to a given
 * event topic. Uses prefix matching: grant "sensor/" permits
 * "sensor/accel", "sensor/gyro/x", etc.
 */
bool msi_domain_has_event_grant(struct msi_domain *d, const char *topic)
{
	struct msi_grant *g;
	unsigned long flags;

	if (!d)
		return true; /* NULL domain = root, all access */

	spin_lock_irqsave(&d->lock, flags);
	list_for_each_entry(g, &d->grants, list) {
		if (g->kind == MSI_GRANT_EVENTS) {
			size_t prefix_len = strlen(g->events.topic_prefix);
			if (strncmp(topic, g->events.topic_prefix,
			            prefix_len) == 0) {
				spin_unlock_irqrestore(&d->lock, flags);
				return true;
			}
		}
	}
	spin_unlock_irqrestore(&d->lock, flags);
	return false;
}

/*
 * Check if a domain has a state grant for a named region.
 */
bool msi_domain_has_state_grant(struct msi_domain *d, const char *name,
                                enum msi_perms required)
{
	struct msi_grant *g;
	unsigned long flags;

	if (!d)
		return true;

	spin_lock_irqsave(&d->lock, flags);
	list_for_each_entry(g, &d->grants, list) {
		if (g->kind == MSI_GRANT_STATE &&
		    strcmp(g->state.name, name) == 0 &&
		    g->state.perms >= required) {
			spin_unlock_irqrestore(&d->lock, flags);
			return true;
		}
	}
	spin_unlock_irqrestore(&d->lock, flags);
	return false;
}

/*
 * Check if a domain has an assoc grant for a named space.
 */
bool msi_domain_has_assoc_grant(struct msi_domain *d, const char *space,
                                enum msi_perms required)
{
	struct msi_grant *g;
	unsigned long flags;

	if (!d)
		return true;

	spin_lock_irqsave(&d->lock, flags);
	list_for_each_entry(g, &d->grants, list) {
		if (g->kind == MSI_GRANT_ASSOC &&
		    strcmp(g->assoc.space, space) == 0 &&
		    g->assoc.perms >= required) {
			spin_unlock_irqrestore(&d->lock, flags);
			return true;
		}
	}
	spin_unlock_irqrestore(&d->lock, flags);
	return false;
}

/*
 * Check if a domain has clock access.
 */
bool msi_domain_has_clock_grant(struct msi_domain *d)
{
	struct msi_grant *g;
	unsigned long flags;

	if (!d)
		return true;

	spin_lock_irqsave(&d->lock, flags);
	list_for_each_entry(g, &d->grants, list) {
		if (g->kind == MSI_GRANT_CLOCK) {
			spin_unlock_irqrestore(&d->lock, flags);
			return true;
		}
	}
	spin_unlock_irqrestore(&d->lock, flags);
	return false;
}

/*
 * Check if a domain has access to a specific accelerator.
 */
bool msi_domain_has_accel_grant(struct msi_domain *d, const char *which)
{
	struct msi_grant *g;
	unsigned long flags;

	if (!d)
		return true;

	spin_lock_irqsave(&d->lock, flags);
	list_for_each_entry(g, &d->grants, list) {
		if (g->kind == MSI_GRANT_ACCEL &&
		    strcmp(g->accel.which, which) == 0) {
			spin_unlock_irqrestore(&d->lock, flags);
			return true;
		}
	}
	spin_unlock_irqrestore(&d->lock, flags);
	return false;
}

/* ===== Public API ===== */

int msi_domain_create(const char *name, struct msi_grant *grants,
                      int num_grants, bool seal, u32 *domain_id)
{
	struct msi_domain *d;
	unsigned long flags;
	int i;

	if (!name || !domain_id)
		return -EINVAL;

	d = kzalloc(sizeof(*d), GFP_KERNEL);
	if (!d)
		return -ENOMEM;

	d->id = msi_alloc_domain_id();
	strscpy(d->name, name, sizeof(d->name));
	d->sealed = false;
	INIT_LIST_HEAD(&d->grants);
	spin_lock_init(&d->lock);
	kref_init(&d->ref);

	/* Add initial grants */
	for (i = 0; i < num_grants; i++) {
		struct msi_grant *g = kzalloc(sizeof(*g), GFP_KERNEL);
		if (!g) {
			/* Cleanup already-added grants */
			msi_domain_put(d);
			return -ENOMEM;
		}
		memcpy(g, &grants[i], sizeof(*g));
		INIT_LIST_HEAD(&g->list);
		list_add_tail(&g->list, &d->grants);
	}

	if (seal)
		d->sealed = true;

	/* Add to global list */
	spin_lock_irqsave(msi_get_domain_list_lock(), flags);
	list_add_tail(&d->list, msi_get_domain_list());
	spin_unlock_irqrestore(msi_get_domain_list_lock(), flags);

	*domain_id = d->id;

#ifdef MSI_DEBUG
	pr_info("msi: domain created — id=%u name='%s' grants=%d sealed=%d\n",
	        d->id, d->name, num_grants, d->sealed);
#endif

	return 0;
}

int msi_domain_grant(u32 domain_id, struct msi_grant *grant)
{
	struct msi_domain *d;
	struct msi_grant *g;
	unsigned long flags;

	if (!grant)
		return -EINVAL;

	d = msi_domain_lookup(domain_id);
	if (!d)
		return -ENOENT;

	spin_lock_irqsave(&d->lock, flags);

	if (d->sealed) {
		spin_unlock_irqrestore(&d->lock, flags);
		msi_domain_put(d);
		pr_warn("msi: cannot grant to sealed domain '%s' (id=%u)\n",
		        d->name, d->id);
		return -EPERM;
	}

	g = kzalloc(sizeof(*g), GFP_ATOMIC);
	if (!g) {
		spin_unlock_irqrestore(&d->lock, flags);
		msi_domain_put(d);
		return -ENOMEM;
	}

	memcpy(g, grant, sizeof(*g));
	INIT_LIST_HEAD(&g->list);
	list_add_tail(&g->list, &d->grants);

	spin_unlock_irqrestore(&d->lock, flags);
	msi_domain_put(d);

#ifdef MSI_DEBUG
	pr_info("msi: grant added to domain %u — kind=%d\n",
	        domain_id, grant->kind);
#endif

	return 0;
}

int msi_domain_seal(u32 domain_id)
{
	struct msi_domain *d;
	unsigned long flags;

	d = msi_domain_lookup(domain_id);
	if (!d)
		return -ENOENT;

	spin_lock_irqsave(&d->lock, flags);
	d->sealed = true;
	spin_unlock_irqrestore(&d->lock, flags);

	msi_domain_put(d);

#ifdef MSI_DEBUG
	pr_info("msi: domain sealed — id=%u name='%s'\n", d->id, d->name);
#endif

	return 0;
}

void msi_domain_destroy(u32 domain_id)
{
	struct msi_domain *d;
	unsigned long flags;

	spin_lock_irqsave(msi_get_domain_list_lock(), flags);
	d = msi_find_domain_locked(domain_id);
	if (d)
		list_del(&d->list);
	spin_unlock_irqrestore(msi_get_domain_list_lock(), flags);

	if (d)
		msi_domain_put(d);
}
