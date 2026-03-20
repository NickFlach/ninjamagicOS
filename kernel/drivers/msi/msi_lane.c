// SPDX-License-Identifier: GPL-2.0
/*
 * msi_lane.c — MSI Lane (Execution Context) Management
 *
 * Lanes are the execution primitive of MSI. Each lane maps to
 * a kernel thread with cognitive scheduling policies:
 * - priority: low/normal/high/realtime
 * - energy: low/balanced/unbounded
 * - affinity: any/little/big/npu/gpu/dsp
 *
 * Lane failures must NEVER crash the substrate.
 */

#include <linux/slab.h>
#include <linux/kthread.h>
#include <linux/sched.h>
#include <linux/sched/types.h>
#include <linux/cpumask.h>
#include <linux/delay.h>
#include <linux/string.h>

#include "msi_core.h"

/* External accessors */
extern struct list_head *msi_get_lane_list(void);
extern spinlock_t *msi_get_lane_list_lock(void);
extern u32 msi_alloc_lane_id(void);
extern struct msi_domain *msi_domain_lookup(u32 domain_id);
extern void msi_domain_put(struct msi_domain *d);

/* ===== CPU topology helpers ===== */

/*
 * Map MSI affinity to a cpumask. On ARM big.LITTLE SoCs:
 *
 * Pixel 7 (Tensor GS201):
 *   LITTLE (A55): CPUs 0-3
 *   big (A78):    CPUs 4-5
 *   BIG (X1):     CPUs 6-7
 *
 * Nord N30 (SD695):
 *   LITTLE (Silver): CPUs 0-5
 *   big (Gold):      CPUs 6-7
 *
 * For NPU/GPU/DSP affinity, we pin to big cores since
 * the actual accelerator dispatch happens in userspace.
 * The lane thread handles orchestration.
 */
static void msi_set_lane_affinity(struct task_struct *task,
                                  enum msi_affinity affinity)
{
	struct cpumask mask;

	cpumask_clear(&mask);

	switch (affinity) {
	case MSI_AFFINITY_LITTLE:
		/* Efficiency cores — first N cores */
		cpumask_set_cpu(0, &mask);
		cpumask_set_cpu(1, &mask);
		cpumask_set_cpu(2, &mask);
		cpumask_set_cpu(3, &mask);
		break;

	case MSI_AFFINITY_BIG:
	case MSI_AFFINITY_NPU:
	case MSI_AFFINITY_GPU:
	case MSI_AFFINITY_DSP:
		/* Performance cores — last N cores */
		if (num_online_cpus() > 4) {
			int i;
			for (i = num_online_cpus() - 2; i < num_online_cpus(); i++)
				cpumask_set_cpu(i, &mask);
		} else {
			/* Fallback: use all CPUs */
			cpumask_copy(&mask, cpu_online_mask);
		}
		break;

	case MSI_AFFINITY_ANY:
	default:
		cpumask_copy(&mask, cpu_online_mask);
		break;
	}

	set_cpus_allowed_ptr(task, &mask);
}

/*
 * Map MSI priority to Linux scheduler policy and priority.
 */
static void msi_set_lane_priority(struct task_struct *task,
                                  enum msi_priority priority)
{
	struct sched_param param;

	switch (priority) {
	case MSI_PRIORITY_REALTIME:
		param.sched_priority = 50;
		sched_setscheduler_nocheck(task, SCHED_FIFO, &param);
		break;

	case MSI_PRIORITY_HIGH:
		param.sched_priority = 0;
		sched_setscheduler_nocheck(task, SCHED_NORMAL, &param);
		set_user_nice(task, -10);
		break;

	case MSI_PRIORITY_NORMAL:
		param.sched_priority = 0;
		sched_setscheduler_nocheck(task, SCHED_NORMAL, &param);
		set_user_nice(task, 0);
		break;

	case MSI_PRIORITY_LOW:
		param.sched_priority = 0;
		sched_setscheduler_nocheck(task, SCHED_NORMAL, &param);
		set_user_nice(task, 10);
		break;
	}
}

/* ===== Lane lifecycle ===== */

static struct msi_lane *msi_find_lane_locked(u32 lane_id)
{
	struct msi_lane *l;

	list_for_each_entry(l, msi_get_lane_list(), list) {
		if (l->id == lane_id)
			return l;
	}
	return NULL;
}

struct msi_lane *msi_lane_lookup(u32 lane_id)
{
	struct msi_lane *l;
	unsigned long flags;

	spin_lock_irqsave(msi_get_lane_list_lock(), flags);
	l = msi_find_lane_locked(lane_id);
	if (l)
		kref_get(&l->ref);
	spin_unlock_irqrestore(msi_get_lane_list_lock(), flags);

	return l;
}

static void msi_lane_release(struct kref *ref)
{
	struct msi_lane *l = container_of(ref, struct msi_lane, ref);

	if (l->domain)
		msi_domain_put(l->domain);
	kfree(l);
}

void msi_lane_put(struct msi_lane *l)
{
	if (l)
		kref_put(&l->ref, msi_lane_release);
}

/*
 * Lane thread function. In the kernel module, lanes are placeholders
 * for userspace cognitive programs. The actual cognitive loop runs in
 * userspace via the MSI runtime; the kernel thread provides scheduling
 * guarantees (priority, affinity, energy budget).
 *
 * For kernel-internal lanes (e.g., event bus relay), the entry point
 * name is resolved to a kernel function.
 */
static int msi_lane_thread_fn(void *data)
{
	struct msi_lane *lane = (struct msi_lane *)data;

	pr_info("msi: lane %u '%s' started on cpu %d\n",
	        lane->id, lane->entry, smp_processor_id());

	/*
	 * The lane thread sleeps until signaled by userspace via ioctl.
	 * Userspace MSI runtime manages the actual cognitive loop.
	 * This thread exists to hold scheduler state (priority, affinity).
	 */
	while (!kthread_should_stop() && lane->alive) {
		set_current_state(TASK_INTERRUPTIBLE);
		schedule_timeout(msecs_to_jiffies(1000));

		if (signal_pending(current))
			break;
	}

	lane->alive = false;
	pr_info("msi: lane %u '%s' exited\n", lane->id, lane->entry);

	return 0;
}

/* ===== Public API ===== */

int msi_lane_spawn(u32 domain_id, const char *entry,
                   struct msi_lane_policy *policy, u32 *lane_id)
{
	struct msi_lane *l;
	struct msi_domain *d = NULL;
	unsigned long flags;

	if (!entry || !policy || !lane_id)
		return -EINVAL;

	/* Validate domain if specified */
	if (domain_id != 0) {
		d = msi_domain_lookup(domain_id);
		if (!d) {
			pr_warn("msi: lane_spawn failed — unknown domain %u\n",
			        domain_id);
			return -ENOENT;
		}
	}

	l = kzalloc(sizeof(*l), GFP_KERNEL);
	if (!l) {
		if (d)
			msi_domain_put(d);
		return -ENOMEM;
	}

	l->id = msi_alloc_lane_id();
	l->domain = d;
	memcpy(&l->policy, policy, sizeof(*policy));
	strscpy(l->entry, entry, sizeof(l->entry));
	l->alive = true;
	spin_lock_init(&l->lock);
	kref_init(&l->ref);

	/* Create kernel thread */
	l->kthread = kthread_create(msi_lane_thread_fn, l,
	                            "msi-lane-%u", l->id);
	if (IS_ERR(l->kthread)) {
		int ret = PTR_ERR(l->kthread);
		pr_err("msi: failed to create kthread for lane %u: %d\n",
		       l->id, ret);
		if (d)
			msi_domain_put(d);
		kfree(l);
		return ret;
	}

	/* Apply scheduling policy */
	msi_set_lane_priority(l->kthread, policy->priority);
	msi_set_lane_affinity(l->kthread, policy->affinity);

	/* Add to global list */
	spin_lock_irqsave(msi_get_lane_list_lock(), flags);
	list_add_tail(&l->list, msi_get_lane_list());
	spin_unlock_irqrestore(msi_get_lane_list_lock(), flags);

	/* Start the thread */
	wake_up_process(l->kthread);

	*lane_id = l->id;

#ifdef MSI_DEBUG
	pr_info("msi: lane spawned — id=%u entry='%s' domain=%u "
	        "priority=%d affinity=%d\n",
	        l->id, l->entry, domain_id,
	        policy->priority, policy->affinity);
#endif

	return 0;
}

int msi_lane_yield(u32 lane_id)
{
	struct msi_lane *l;

	l = msi_lane_lookup(lane_id);
	if (!l)
		return -ENOENT;

	if (l->kthread && l->alive)
		wake_up_process(l->kthread);

	msi_lane_put(l);
	return 0;
}

int msi_lane_sleep_nanos(u32 lane_id, u64 nanos)
{
	struct msi_lane *l;
	unsigned long usecs;

	l = msi_lane_lookup(lane_id);
	if (!l)
		return -ENOENT;

	usecs = div_u64(nanos, 1000);
	if (usecs > 0 && l->alive)
		usleep_range(usecs, usecs + (usecs >> 4));

	msi_lane_put(l);
	return 0;
}

int msi_lane_kill(u32 lane_id)
{
	struct msi_lane *l;
	unsigned long flags;

	spin_lock_irqsave(msi_get_lane_list_lock(), flags);
	l = msi_find_lane_locked(lane_id);
	if (l) {
		list_del(&l->list);
		kref_get(&l->ref);
	}
	spin_unlock_irqrestore(msi_get_lane_list_lock(), flags);

	if (!l)
		return -ENOENT;

	l->alive = false;
	if (l->kthread) {
		kthread_stop(l->kthread);
		l->kthread = NULL;
	}

	pr_info("msi: lane killed — id=%u entry='%s'\n", l->id, l->entry);

	msi_lane_put(l); /* list ref */
	msi_lane_put(l); /* lookup ref */
	return 0;
}

int msi_lane_set_policy(u32 lane_id, struct msi_lane_policy *policy)
{
	struct msi_lane *l;
	unsigned long flags;

	if (!policy)
		return -EINVAL;

	l = msi_lane_lookup(lane_id);
	if (!l)
		return -ENOENT;

	spin_lock_irqsave(&l->lock, flags);
	memcpy(&l->policy, policy, sizeof(*policy));
	spin_unlock_irqrestore(&l->lock, flags);

	if (l->kthread && l->alive) {
		msi_set_lane_priority(l->kthread, policy->priority);
		msi_set_lane_affinity(l->kthread, policy->affinity);
	}

	msi_lane_put(l);

#ifdef MSI_DEBUG
	pr_info("msi: lane %u policy updated — priority=%d affinity=%d\n",
	        lane_id, policy->priority, policy->affinity);
#endif

	return 0;
}
