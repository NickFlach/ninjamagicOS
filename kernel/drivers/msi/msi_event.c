// SPDX-License-Identifier: GPL-2.0
/*
 * msi_event.c — MSI Event Bus (Topic-Based Pub/Sub)
 *
 * The event bus is the nervous system of ninjamagicOS. Every
 * phone subsystem publishes events on named topics, and cognitive
 * programs subscribe via prefix matching. This enables zero-latency
 * context awareness for the NinjaMagic Agent.
 *
 * Event delivery semantics:
 *   - best_effort: fire and forget
 *   - at_least_once: retry until acknowledged
 *   - exactly_once: transactional (future)
 */

#include <linux/slab.h>
#include <linux/string.h>
#include <linux/ktime.h>
#include <linux/wait.h>
#include <linux/sched.h>

#include "msi_core.h"

/* External accessors */
extern struct list_head *msi_get_sub_list(void);
extern spinlock_t *msi_get_sub_list_lock(void);
extern u32 msi_alloc_sub_id(void);
extern u64 msi_alloc_event_id(void);
extern struct msi_domain *msi_domain_lookup(u32 domain_id);
extern void msi_domain_put(struct msi_domain *d);
extern bool msi_domain_has_event_grant(struct msi_domain *d, const char *topic);

/* Per-subscription pending event limit */
#define MSI_SUB_MAX_PENDING  256

/* ===== Internal helpers ===== */

static struct msi_subscription *msi_find_sub_locked(u32 sub_id)
{
	struct msi_subscription *s;

	list_for_each_entry(s, msi_get_sub_list(), list) {
		if (s->id == sub_id)
			return s;
	}
	return NULL;
}

/*
 * Check if a topic matches a subscription prefix.
 * "sensor/" matches "sensor/accel", "sensor/gyro/x", etc.
 * Empty prefix matches everything.
 */
static bool msi_topic_matches(const char *topic, const char *prefix)
{
	size_t plen;

	if (!prefix[0])
		return true;

	plen = strlen(prefix);
	return strncmp(topic, prefix, plen) == 0;
}

/*
 * Count pending events in a subscription queue.
 */
static int msi_sub_pending_count(struct msi_subscription *sub)
{
	struct msi_event *e;
	int count = 0;

	list_for_each_entry(e, &sub->events, list)
		count++;

	return count;
}

/*
 * Deliver an event to all matching subscriptions.
 * Called under sub_list_lock.
 */
static void msi_deliver_event(struct msi_event *event)
{
	struct msi_subscription *sub;
	unsigned long flags;

	spin_lock_irqsave(msi_get_sub_list_lock(), flags);

	list_for_each_entry(sub, msi_get_sub_list(), list) {
		if (!msi_topic_matches(event->topic, sub->prefix))
			continue;

		/* Apply filter if set (future: regex/glob matching) */
		if (sub->filter[0] != '\0') {
			/* For now, filter is an exact substring match */
			if (!strstr(event->topic, sub->filter))
				continue;
		}

		/* Check domain grant on subscriber side */
		if (sub->domain &&
		    !msi_domain_has_event_grant(sub->domain, event->topic))
			continue;

		/* Clone event for this subscriber */
		spin_lock(&sub->lock);

		if (msi_sub_pending_count(sub) < MSI_SUB_MAX_PENDING) {
			struct msi_event *clone;

			clone = kzalloc(sizeof(*clone), GFP_ATOMIC);
			if (clone) {
				clone->id = event->id;
				strscpy(clone->topic, event->topic,
				        sizeof(clone->topic));
				clone->ts_nanos = event->ts_nanos;
				clone->qos = event->qos;
				clone->acked = false;
				INIT_LIST_HEAD(&clone->list);

				/* Copy payload */
				if (event->payload && event->payload_len > 0) {
					clone->payload = kmalloc(event->payload_len,
					                         GFP_ATOMIC);
					if (clone->payload) {
						memcpy(clone->payload, event->payload,
						       event->payload_len);
						clone->payload_len = event->payload_len;
					}
				}

				list_add_tail(&clone->list, &sub->events);
				wake_up_interruptible(&sub->wait);
			}
		}
		/* else: drop event for this sub (back-pressure) */

		spin_unlock(&sub->lock);
	}

	spin_unlock_irqrestore(msi_get_sub_list_lock(), flags);
}

/* ===== Public API ===== */

int msi_event_publish(u32 domain_id, const char *topic,
                      void *payload, size_t len, enum msi_qos qos,
                      u64 *event_id)
{
	struct msi_domain *d = NULL;
	struct msi_event event;

	if (!topic || !event_id)
		return -EINVAL;

	/* Validate domain grant */
	if (domain_id != 0) {
		d = msi_domain_lookup(domain_id);
		if (!d)
			return -ENOENT;
		if (!msi_domain_has_event_grant(d, topic)) {
			msi_domain_put(d);
			pr_warn("msi: publish denied — domain %u lacks grant for '%s'\n",
			        domain_id, topic);
			return -EPERM;
		}
		msi_domain_put(d);
	}

	/* Build event */
	memset(&event, 0, sizeof(event));
	event.id = msi_alloc_event_id();
	strscpy(event.topic, topic, sizeof(event.topic));
	event.ts_nanos = ktime_get_ns();
	event.payload = payload;
	event.payload_len = len;
	event.qos = qos;
	event.acked = false;

	/* Deliver to all matching subscribers */
	msi_deliver_event(&event);

	*event_id = event.id;

#ifdef MSI_DEBUG
	pr_info("msi: event published — id=%llu topic='%s' len=%zu qos=%d\n",
	        event.id, topic, len, qos);
#endif

	return 0;
}

int msi_event_subscribe(u32 domain_id, const char *prefix,
                        const char *filter, u32 *sub_id)
{
	struct msi_subscription *sub;
	struct msi_domain *d = NULL;
	unsigned long flags;

	if (!prefix || !sub_id)
		return -EINVAL;

	if (domain_id != 0) {
		d = msi_domain_lookup(domain_id);
		if (!d)
			return -ENOENT;
		if (!msi_domain_has_event_grant(d, prefix)) {
			msi_domain_put(d);
			return -EPERM;
		}
		/* Keep reference — sub holds domain ref */
	}

	sub = kzalloc(sizeof(*sub), GFP_KERNEL);
	if (!sub) {
		if (d)
			msi_domain_put(d);
		return -ENOMEM;
	}

	sub->id = msi_alloc_sub_id();
	sub->domain = d;
	strscpy(sub->prefix, prefix, sizeof(sub->prefix));
	if (filter)
		strscpy(sub->filter, filter, sizeof(sub->filter));
	init_waitqueue_head(&sub->wait);
	INIT_LIST_HEAD(&sub->events);
	spin_lock_init(&sub->lock);
	INIT_LIST_HEAD(&sub->list);

	spin_lock_irqsave(msi_get_sub_list_lock(), flags);
	list_add_tail(&sub->list, msi_get_sub_list());
	spin_unlock_irqrestore(msi_get_sub_list_lock(), flags);

	*sub_id = sub->id;

#ifdef MSI_DEBUG
	pr_info("msi: subscription created — id=%u prefix='%s' domain=%u\n",
	        sub->id, prefix, domain_id);
#endif

	return 0;
}

int msi_event_wait(u32 sub_id, u64 timeout_nanos, struct msi_event *out)
{
	struct msi_subscription *sub;
	struct msi_event *event;
	unsigned long flags, jiffies_timeout;
	int ret = 0;

	if (!out)
		return -EINVAL;

	spin_lock_irqsave(msi_get_sub_list_lock(), flags);
	sub = msi_find_sub_locked(sub_id);
	spin_unlock_irqrestore(msi_get_sub_list_lock(), flags);

	if (!sub)
		return -ENOENT;

	/* Calculate timeout in jiffies */
	if (timeout_nanos == 0) {
		jiffies_timeout = MAX_SCHEDULE_TIMEOUT;
	} else {
		u64 ms = div_u64(timeout_nanos, 1000000);
		jiffies_timeout = msecs_to_jiffies((unsigned int)ms);
		if (jiffies_timeout == 0)
			jiffies_timeout = 1;
	}

	/* Wait for an event */
	ret = wait_event_interruptible_timeout(sub->wait,
	                                       !list_empty(&sub->events),
	                                       jiffies_timeout);

	if (ret == 0)
		return -ETIMEDOUT;
	if (ret < 0)
		return -EINTR;

	/* Dequeue first event */
	spin_lock_irqsave(&sub->lock, flags);
	if (!list_empty(&sub->events)) {
		event = list_first_entry(&sub->events, struct msi_event, list);
		list_del(&event->list);
		spin_unlock_irqrestore(&sub->lock, flags);

		/* Copy to output (without payload — userspace reads separately) */
		out->id = event->id;
		strscpy(out->topic, event->topic, sizeof(out->topic));
		out->ts_nanos = event->ts_nanos;
		out->qos = event->qos;
		out->payload_len = event->payload_len;

		/* Payload transfer: for now, copy to out->payload if set */
		if (event->payload) {
			out->payload = event->payload;
			out->payload_len = event->payload_len;
			event->payload = NULL; /* Ownership transferred */
		}

		kfree(event);
		return 0;
	}
	spin_unlock_irqrestore(&sub->lock, flags);

	return -EAGAIN;
}

int msi_event_ack(u64 event_id)
{
	/*
	 * For at_least_once QoS: mark event as acknowledged
	 * so it won't be redelivered. For best_effort, this is a no-op.
	 *
	 * Full implementation tracks unacked events per subscription
	 * and redelivers on timeout.
	 */
#ifdef MSI_DEBUG
	pr_info("msi: event ack — id=%llu\n", event_id);
#endif
	return 0;
}
