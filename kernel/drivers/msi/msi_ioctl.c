// SPDX-License-Identifier: GPL-2.0
/*
 * msi_ioctl.c — MSI ioctl Dispatch
 *
 * Routes userspace ioctl calls from /dev/msi to the appropriate
 * MSI subsystem (domain, lane, event, state). This is the primary
 * interface between the MSI userspace runtime and the kernel module.
 */

#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>

#include "msi_core.h"

/* External API functions */
extern struct msi_capabilities *msi_get_capabilities(void);

/* ===== Userspace ↔ kernel data transfer helpers ===== */

struct msi_ioctl_domain_create {
	char     name[128];
	u32      num_grants;
	bool     seal;
	u32      domain_id; /* out */
};

struct msi_ioctl_domain_grant_args {
	u32              domain_id;
	struct msi_grant grant;
};

struct msi_ioctl_lane_spawn_args {
	u32                    domain_id;
	char                   entry[128];
	struct msi_lane_policy policy;
	u32                    lane_id; /* out */
};

struct msi_ioctl_event_publish_args {
	u32          domain_id;
	char         topic[256];
	u64          payload_ptr; /* userspace pointer */
	u32          payload_len;
	enum msi_qos qos;
	u64          event_id; /* out */
};

struct msi_ioctl_event_subscribe_args {
	u32  domain_id;
	char prefix[256];
	char filter[256];
	u32  sub_id; /* out */
};

struct msi_ioctl_event_wait_args {
	u32  sub_id;
	u64  timeout_nanos;
	/* Output event fields */
	u64  event_id;
	char topic[256];
	u64  ts_nanos;
	u32  payload_len;
	u64  payload_ptr; /* userspace buffer for payload */
};

struct msi_ioctl_state_map_args {
	u32          domain_id;
	char         name[128];
	u64          bytes;
	enum msi_perms perms;
	u32          handle_id; /* out */
};

struct msi_ioctl_state_rw_args {
	u32 handle_id;
	u64 offset;
	u64 len;
	u64 buf_ptr; /* userspace pointer */
};

/* ===== ioctl dispatch ===== */

long msi_ioctl_dispatch(struct file *filp, unsigned int cmd,
                        unsigned long arg)
{
	void __user *uarg = (void __user *)arg;
	int ret;

	switch (cmd) {

	/* --- Discovery --- */

	case MSI_IOC_VERSION: {
		u32 version;
		ret = msi_get_version(&version);
		if (ret)
			return ret;
		if (copy_to_user(uarg, &version, sizeof(version)))
			return -EFAULT;
		return 0;
	}

	case MSI_IOC_CAPABILITIES: {
		struct msi_capabilities *caps = msi_get_capabilities();
		if (copy_to_user(uarg, caps, sizeof(*caps)))
			return -EFAULT;
		return 0;
	}

	/* --- Domains --- */

	case MSI_IOC_DOMAIN_CREATE: {
		struct msi_ioctl_domain_create args;

		if (copy_from_user(&args, uarg, sizeof(args)))
			return -EFAULT;

		/*
		 * For simplicity, grants are added via separate
		 * MSI_IOC_DOMAIN_GRANT calls after creation.
		 * Create with zero grants, then add, then seal.
		 */
		ret = msi_domain_create(args.name, NULL, 0,
		                        args.seal, &args.domain_id);
		if (ret)
			return ret;

		if (copy_to_user(uarg, &args, sizeof(args)))
			return -EFAULT;
		return 0;
	}

	case MSI_IOC_DOMAIN_GRANT: {
		struct msi_ioctl_domain_grant_args args;

		if (copy_from_user(&args, uarg, sizeof(args)))
			return -EFAULT;

		return msi_domain_grant(args.domain_id, &args.grant);
	}

	case MSI_IOC_DOMAIN_SEAL: {
		u32 domain_id;

		if (copy_from_user(&domain_id, uarg, sizeof(domain_id)))
			return -EFAULT;

		return msi_domain_seal(domain_id);
	}

	/* --- Lanes --- */

	case MSI_IOC_LANE_SPAWN: {
		struct msi_ioctl_lane_spawn_args args;

		if (copy_from_user(&args, uarg, sizeof(args)))
			return -EFAULT;

		ret = msi_lane_spawn(args.domain_id, args.entry,
		                     &args.policy, &args.lane_id);
		if (ret)
			return ret;

		if (copy_to_user(uarg, &args, sizeof(args)))
			return -EFAULT;
		return 0;
	}

	case MSI_IOC_LANE_YIELD: {
		u32 lane_id;

		if (copy_from_user(&lane_id, uarg, sizeof(lane_id)))
			return -EFAULT;

		return msi_lane_yield(lane_id);
	}

	case MSI_IOC_LANE_SLEEP: {
		u64 nanos;

		if (copy_from_user(&nanos, uarg, sizeof(nanos)))
			return -EFAULT;

		/* Lane ID is implicit (current thread) for sleep */
		usleep_range(div_u64(nanos, 1000),
		             div_u64(nanos, 1000) + 100);
		return 0;
	}

	case MSI_IOC_LANE_KILL: {
		u32 lane_id;

		if (copy_from_user(&lane_id, uarg, sizeof(lane_id)))
			return -EFAULT;

		return msi_lane_kill(lane_id);
	}

	/* --- Events --- */

	case MSI_IOC_EVENT_PUBLISH: {
		struct msi_ioctl_event_publish_args args;
		void *payload = NULL;

		if (copy_from_user(&args, uarg, sizeof(args)))
			return -EFAULT;

		/* Copy payload from userspace if present */
		if (args.payload_ptr && args.payload_len > 0) {
			if (args.payload_len > (1024 * 1024)) /* 1MB max */
				return -E2BIG;

			payload = kmalloc(args.payload_len, GFP_KERNEL);
			if (!payload)
				return -ENOMEM;

			if (copy_from_user(payload,
			                   (void __user *)args.payload_ptr,
			                   args.payload_len)) {
				kfree(payload);
				return -EFAULT;
			}
		}

		ret = msi_event_publish(args.domain_id, args.topic,
		                        payload, args.payload_len,
		                        args.qos, &args.event_id);

		kfree(payload);

		if (ret)
			return ret;

		if (copy_to_user(uarg, &args, sizeof(args)))
			return -EFAULT;
		return 0;
	}

	case MSI_IOC_EVENT_SUBSCRIBE: {
		struct msi_ioctl_event_subscribe_args args;

		if (copy_from_user(&args, uarg, sizeof(args)))
			return -EFAULT;

		ret = msi_event_subscribe(args.domain_id, args.prefix,
		                          args.filter[0] ? args.filter : NULL,
		                          &args.sub_id);
		if (ret)
			return ret;

		if (copy_to_user(uarg, &args, sizeof(args)))
			return -EFAULT;
		return 0;
	}

	case MSI_IOC_EVENT_WAIT: {
		struct msi_ioctl_event_wait_args args;
		struct msi_event event;

		if (copy_from_user(&args, uarg, sizeof(args)))
			return -EFAULT;

		memset(&event, 0, sizeof(event));
		ret = msi_event_wait(args.sub_id, args.timeout_nanos, &event);
		if (ret)
			return ret;

		/* Copy event metadata back to userspace */
		args.event_id = event.id;
		strscpy(args.topic, event.topic, sizeof(args.topic));
		args.ts_nanos = event.ts_nanos;
		args.payload_len = event.payload_len;

		/* Copy payload to userspace buffer if provided */
		if (event.payload && event.payload_len > 0 &&
		    args.payload_ptr) {
			u32 copy_len = min_t(u32, event.payload_len,
			                     args.payload_len);
			if (copy_to_user((void __user *)args.payload_ptr,
			                 event.payload, copy_len)) {
				kfree(event.payload);
				return -EFAULT;
			}
			args.payload_len = copy_len;
		}

		kfree(event.payload);

		if (copy_to_user(uarg, &args, sizeof(args)))
			return -EFAULT;
		return 0;
	}

	case MSI_IOC_EVENT_ACK: {
		u64 event_id;

		if (copy_from_user(&event_id, uarg, sizeof(event_id)))
			return -EFAULT;

		return msi_event_ack(event_id);
	}

	/* --- State --- */

	case MSI_IOC_STATE_MAP: {
		struct msi_ioctl_state_map_args args;

		if (copy_from_user(&args, uarg, sizeof(args)))
			return -EFAULT;

		ret = msi_state_map(args.domain_id, args.name,
		                    (size_t)args.bytes, args.perms,
		                    &args.handle_id);
		if (ret)
			return ret;

		if (copy_to_user(uarg, &args, sizeof(args)))
			return -EFAULT;
		return 0;
	}

	case MSI_IOC_STATE_READ: {
		struct msi_ioctl_state_rw_args args;
		void *buf;

		if (copy_from_user(&args, uarg, sizeof(args)))
			return -EFAULT;

		if (args.len > (1024 * 1024)) /* 1MB max per read */
			return -E2BIG;

		buf = kmalloc(args.len, GFP_KERNEL);
		if (!buf)
			return -ENOMEM;

		ret = msi_state_read(args.handle_id, (size_t)args.offset,
		                     (size_t)args.len, buf);
		if (ret) {
			kfree(buf);
			return ret;
		}

		if (copy_to_user((void __user *)args.buf_ptr, buf, args.len)) {
			kfree(buf);
			return -EFAULT;
		}

		kfree(buf);
		return 0;
	}

	case MSI_IOC_STATE_WRITE: {
		struct msi_ioctl_state_rw_args args;
		void *data;

		if (copy_from_user(&args, uarg, sizeof(args)))
			return -EFAULT;

		if (args.len > (1024 * 1024))
			return -E2BIG;

		data = kmalloc(args.len, GFP_KERNEL);
		if (!data)
			return -ENOMEM;

		if (copy_from_user(data, (void __user *)args.buf_ptr,
		                   args.len)) {
			kfree(data);
			return -EFAULT;
		}

		ret = msi_state_write(args.handle_id, (size_t)args.offset,
		                      data, (size_t)args.len);
		kfree(data);
		return ret;
	}

	case MSI_IOC_STATE_COMMIT: {
		u32 handle_id;

		if (copy_from_user(&handle_id, uarg, sizeof(handle_id)))
			return -EFAULT;

		return msi_state_commit(handle_id);
	}

	default:
		return -ENOTTY;
	}
}
