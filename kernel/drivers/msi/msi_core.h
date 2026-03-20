/* SPDX-License-Identifier: GPL-2.0 */
/*
 * msi_core.h — MSI v1.0 Kernel Module for ninjamagicOS
 *
 * Minimal Substrate Interface: cognitive execution primitives
 * at the kernel level. Provides Lanes, Events, Domains,
 * addressable State, and associative memory hooks.
 *
 * Based on SingularisPrime MSI v1.0 specification.
 */

#ifndef _MSI_CORE_H
#define _MSI_CORE_H

#include <linux/types.h>
#include <linux/list.h>
#include <linux/spinlock.h>
#include <linux/wait.h>
#include <linux/kref.h>

#define MSI_VERSION_MAJOR   1
#define MSI_VERSION_MINOR   0
#define MSI_VERSION_PATCH   0

#define MSI_DEVICE_NAME     "msi"
#define MSI_CLASS_NAME      "msi"

/* Limits — overridable via board config */
#ifndef MSI_MAX_LANES
#define MSI_MAX_LANES       1024
#endif

#ifndef MSI_MAX_DOMAINS
#define MSI_MAX_DOMAINS     256
#endif

#ifndef MSI_MAX_TOPICS
#define MSI_MAX_TOPICS      65536
#endif

#ifndef MSI_MAX_STATE_REGIONS
#define MSI_MAX_STATE_REGIONS 4096
#endif

/* ===== Enumerations ===== */

enum msi_priority {
	MSI_PRIORITY_LOW      = 0,
	MSI_PRIORITY_NORMAL   = 1,
	MSI_PRIORITY_HIGH     = 2,
	MSI_PRIORITY_REALTIME = 3,
};

enum msi_energy_budget {
	MSI_ENERGY_LOW       = 0,
	MSI_ENERGY_BALANCED  = 1,
	MSI_ENERGY_UNBOUNDED = 2,
};

enum msi_affinity {
	MSI_AFFINITY_ANY    = 0,
	MSI_AFFINITY_LITTLE = 1,  /* Efficiency cores */
	MSI_AFFINITY_BIG    = 2,  /* Performance cores */
	MSI_AFFINITY_NPU    = 3,  /* Neural processing unit */
	MSI_AFFINITY_GPU    = 4,  /* GPU compute */
	MSI_AFFINITY_DSP    = 5,  /* Digital signal processor */
};

enum msi_perms {
	MSI_PERMS_READ      = 0,
	MSI_PERMS_READWRITE = 1,
};

enum msi_qos {
	MSI_QOS_BEST_EFFORT   = 0,
	MSI_QOS_AT_LEAST_ONCE = 1,
	MSI_QOS_EXACTLY_ONCE  = 2,
};

enum msi_security_model {
	MSI_SECURITY_NONE        = 0,
	MSI_SECURITY_APP_SANDBOX = 1,
	MSI_SECURITY_TEE         = 2,
	MSI_SECURITY_SE          = 3,
};

enum msi_grant_kind {
	MSI_GRANT_EVENTS = 0,
	MSI_GRANT_STATE  = 1,
	MSI_GRANT_ASSOC  = 2,
	MSI_GRANT_CLOCK  = 3,
	MSI_GRANT_ACCEL  = 4,
};

/* ===== Structures ===== */

/* Lane policy — scheduling hints for cognitive execution contexts */
struct msi_lane_policy {
	enum msi_priority      priority;
	enum msi_energy_budget energy;
	enum msi_affinity      affinity;
};

/* Grant — a single capability permission */
struct msi_grant {
	enum msi_grant_kind kind;
	union {
		struct {
			char topic_prefix[256];
		} events;
		struct {
			char name[128];
			enum msi_perms perms;
		} state;
		struct {
			char space[128];
			enum msi_perms perms;
		} assoc;
		struct {
			char which[32]; /* "cpu", "gpu", "npu", "dsp" */
		} accel;
	};
	struct list_head list;
};

/* Domain — capability container */
struct msi_domain {
	u32                 id;
	char                name[128];
	bool                sealed;
	struct list_head    grants;     /* list of msi_grant */
	spinlock_t          lock;
	struct kref         ref;
	struct list_head    list;       /* global domain list */
};

/* Lane — execution context (backed by kthread) */
struct msi_lane {
	u32                    id;
	struct msi_domain      *domain;  /* owning domain (nullable) */
	struct msi_lane_policy policy;
	struct task_struct     *kthread;
	char                   entry[128];
	bool                   alive;
	spinlock_t             lock;
	struct kref            ref;
	struct list_head       list;     /* global lane list */
};

/* Event — published message on the event bus */
struct msi_event {
	u64              id;
	char             topic[256];
	u64              ts_nanos;
	void             *payload;
	size_t           payload_len;
	enum msi_qos     qos;
	bool             acked;
	struct list_head list;
};

/* Subscription — event topic listener */
struct msi_subscription {
	u32              id;
	struct msi_domain *domain;
	char             prefix[256];
	char             filter[256];
	wait_queue_head_t wait;
	struct list_head  events;   /* pending events for this sub */
	spinlock_t        lock;
	struct list_head  list;     /* global subscription list */
};

/* State region — mmap-backed addressable byte buffer */
struct msi_state_region {
	u32              id;
	struct msi_domain *domain;
	char             name[128];
	void             *data;
	size_t           size;
	enum msi_perms   perms;
	bool             committed;
	spinlock_t       lock;
	struct list_head list;
};

/* Hardware capabilities — discovered at boot */
struct msi_capabilities {
	u32  lanes_min;
	u32  lanes_max;
	bool lanes_realtime;
	u32  events_max_topics;
	u64  state_max_bytes;
	bool security_attest;
	enum msi_security_model security_model;
	bool accel_cpu;
	bool accel_gpu;
	bool accel_npu;
	bool accel_dsp;
};

/* ===== IOCTL definitions ===== */

#define MSI_IOC_MAGIC 'M'

#define MSI_IOC_VERSION        _IOR(MSI_IOC_MAGIC, 0, u32)
#define MSI_IOC_CAPABILITIES   _IOR(MSI_IOC_MAGIC, 1, struct msi_capabilities)
#define MSI_IOC_DOMAIN_CREATE  _IOWR(MSI_IOC_MAGIC, 10, struct msi_domain)
#define MSI_IOC_DOMAIN_GRANT   _IOW(MSI_IOC_MAGIC, 11, struct msi_grant)
#define MSI_IOC_DOMAIN_SEAL    _IOW(MSI_IOC_MAGIC, 12, u32)
#define MSI_IOC_LANE_SPAWN     _IOWR(MSI_IOC_MAGIC, 20, struct msi_lane)
#define MSI_IOC_LANE_YIELD     _IOW(MSI_IOC_MAGIC, 21, u32)
#define MSI_IOC_LANE_SLEEP     _IOW(MSI_IOC_MAGIC, 22, u64)
#define MSI_IOC_LANE_KILL      _IOW(MSI_IOC_MAGIC, 23, u32)
#define MSI_IOC_EVENT_PUBLISH  _IOW(MSI_IOC_MAGIC, 30, struct msi_event)
#define MSI_IOC_EVENT_SUBSCRIBE _IOWR(MSI_IOC_MAGIC, 31, struct msi_subscription)
#define MSI_IOC_EVENT_WAIT     _IOWR(MSI_IOC_MAGIC, 32, struct msi_event)
#define MSI_IOC_EVENT_ACK      _IOW(MSI_IOC_MAGIC, 33, u64)
#define MSI_IOC_STATE_MAP      _IOWR(MSI_IOC_MAGIC, 40, struct msi_state_region)
#define MSI_IOC_STATE_READ     _IOWR(MSI_IOC_MAGIC, 41, struct msi_state_region)
#define MSI_IOC_STATE_WRITE    _IOW(MSI_IOC_MAGIC, 42, struct msi_state_region)
#define MSI_IOC_STATE_COMMIT   _IOW(MSI_IOC_MAGIC, 43, u32)

/* ===== Function prototypes ===== */

/* Module lifecycle */
int msi_init(void);
void msi_exit(void);

/* Discovery */
int msi_get_version(u32 *version);
int msi_get_capabilities(struct msi_capabilities *caps);

/* Domains */
int msi_domain_create(const char *name, struct msi_grant *grants,
                      int num_grants, bool seal, u32 *domain_id);
int msi_domain_grant(u32 domain_id, struct msi_grant *grant);
int msi_domain_seal(u32 domain_id);
void msi_domain_destroy(u32 domain_id);

/* Lanes */
int msi_lane_spawn(u32 domain_id, const char *entry,
                   struct msi_lane_policy *policy, u32 *lane_id);
int msi_lane_yield(u32 lane_id);
int msi_lane_sleep_nanos(u32 lane_id, u64 nanos);
int msi_lane_kill(u32 lane_id);
int msi_lane_set_policy(u32 lane_id, struct msi_lane_policy *policy);

/* Events */
int msi_event_publish(u32 domain_id, const char *topic,
                      void *payload, size_t len, enum msi_qos qos,
                      u64 *event_id);
int msi_event_subscribe(u32 domain_id, const char *prefix,
                        const char *filter, u32 *sub_id);
int msi_event_wait(u32 sub_id, u64 timeout_nanos, struct msi_event *event);
int msi_event_ack(u64 event_id);

/* Addressable State */
int msi_state_map(u32 domain_id, const char *name, size_t bytes,
                  enum msi_perms perms, u32 *handle_id);
int msi_state_read(u32 handle_id, size_t offset, size_t len, void *buf);
int msi_state_write(u32 handle_id, size_t offset, void *data, size_t len);
int msi_state_commit(u32 handle_id);

#endif /* _MSI_CORE_H */
