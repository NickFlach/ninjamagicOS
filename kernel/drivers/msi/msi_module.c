// SPDX-License-Identifier: GPL-2.0
/*
 * msi_module.c — MSI v1.0 Kernel Module Entry Point
 *
 * ninjamagicOS Minimal Substrate Interface
 * Cognitive execution primitives at the kernel level.
 *
 * This is the main module file handling init/exit,
 * character device registration, and /dev/msi creation.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

#include "msi_core.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("ninjamagicOS");
MODULE_DESCRIPTION("MSI v1.0 — Minimal Substrate Interface for cognitive workloads");
MODULE_VERSION("1.0.0");

/* Global state */
static dev_t msi_devno;
static struct cdev msi_cdev;
static struct class *msi_class;
static struct device *msi_device;

/* Substrate capabilities — populated at init based on hardware detection */
static struct msi_capabilities substrate_caps;

/* Global lists */
static LIST_HEAD(domain_list);
static LIST_HEAD(lane_list);
static LIST_HEAD(subscription_list);
static LIST_HEAD(state_list);
static DEFINE_SPINLOCK(domain_list_lock);
static DEFINE_SPINLOCK(lane_list_lock);
static DEFINE_SPINLOCK(sub_list_lock);
static DEFINE_SPINLOCK(state_list_lock);

/* ID counters */
static atomic_t next_domain_id = ATOMIC_INIT(1);
static atomic_t next_lane_id = ATOMIC_INIT(1);
static atomic_t next_sub_id = ATOMIC_INIT(1);
static atomic64_t next_event_id = ATOMIC64_INIT(1);
static atomic_t next_state_id = ATOMIC_INIT(1);

/* ===== Accessor functions for other translation units ===== */

struct list_head *msi_get_domain_list(void)
{
	return &domain_list;
}

spinlock_t *msi_get_domain_list_lock(void)
{
	return &domain_list_lock;
}

struct list_head *msi_get_lane_list(void)
{
	return &lane_list;
}

spinlock_t *msi_get_lane_list_lock(void)
{
	return &lane_list_lock;
}

struct list_head *msi_get_sub_list(void)
{
	return &subscription_list;
}

spinlock_t *msi_get_sub_list_lock(void)
{
	return &sub_list_lock;
}

struct list_head *msi_get_state_list(void)
{
	return &state_list;
}

spinlock_t *msi_get_state_list_lock(void)
{
	return &state_list_lock;
}

u32 msi_alloc_domain_id(void)
{
	return (u32)atomic_inc_return(&next_domain_id);
}

u32 msi_alloc_lane_id(void)
{
	return (u32)atomic_inc_return(&next_lane_id);
}

u32 msi_alloc_sub_id(void)
{
	return (u32)atomic_inc_return(&next_sub_id);
}

u64 msi_alloc_event_id(void)
{
	return (u64)atomic64_inc_return(&next_event_id);
}

u32 msi_alloc_state_id(void)
{
	return (u32)atomic_inc_return(&next_state_id);
}

struct msi_capabilities *msi_get_capabilities(void)
{
	return &substrate_caps;
}

/* ===== Discovery ===== */

int msi_get_version(u32 *version)
{
	if (!version)
		return -EINVAL;
	*version = (MSI_VERSION_MAJOR << 16) |
	           (MSI_VERSION_MINOR << 8) |
	           MSI_VERSION_PATCH;
	return 0;
}

/* ===== Hardware capability detection ===== */

static void msi_detect_capabilities(void)
{
	substrate_caps.lanes_min = 1;
	substrate_caps.lanes_max = MSI_MAX_LANES;
	substrate_caps.lanes_realtime = true;
	substrate_caps.events_max_topics = MSI_MAX_TOPICS;
	substrate_caps.state_max_bytes = (u64)totalram_pages() * PAGE_SIZE;
	substrate_caps.accel_cpu = true;

	/*
	 * GPU/NPU/DSP detection is device-specific.
	 * On real hardware this queries the SoC capabilities.
	 * For now, set based on compile-time board config.
	 */
#ifdef CONFIG_MSI_NPU_ACCEL
	substrate_caps.accel_npu = true;
#endif

	/* Security model detection */
#if defined(CONFIG_TRUSTY)
	substrate_caps.security_model = MSI_SECURITY_TEE;
	substrate_caps.security_attest = true;
#elif defined(CONFIG_QCOM_SPU)
	substrate_caps.security_model = MSI_SECURITY_APP_SANDBOX;
	substrate_caps.security_attest = true;
#else
	substrate_caps.security_model = MSI_SECURITY_NONE;
	substrate_caps.security_attest = false;
#endif

	pr_info("msi: capabilities detected — lanes_max=%u topics_max=%u "
	        "npu=%d gpu=%d dsp=%d security=%d\n",
	        substrate_caps.lanes_max,
	        substrate_caps.events_max_topics,
	        substrate_caps.accel_npu,
	        substrate_caps.accel_gpu,
	        substrate_caps.accel_dsp,
	        substrate_caps.security_model);
}

/* ===== Character device file operations ===== */

static int msi_open(struct inode *inode, struct file *filp)
{
	/* Each open gets a fresh context (future: per-process domain tracking) */
	return 0;
}

static int msi_release(struct inode *inode, struct file *filp)
{
	/* Cleanup per-process resources (future: auto-destroy orphaned lanes) */
	return 0;
}

/* ioctl dispatch — implemented in msi_ioctl.c */
extern long msi_ioctl_dispatch(struct file *filp, unsigned int cmd,
                               unsigned long arg);

static long msi_unlocked_ioctl(struct file *filp, unsigned int cmd,
                               unsigned long arg)
{
	return msi_ioctl_dispatch(filp, cmd, arg);
}

static const struct file_operations msi_fops = {
	.owner          = THIS_MODULE,
	.open           = msi_open,
	.release        = msi_release,
	.unlocked_ioctl = msi_unlocked_ioctl,
};

/* ===== Boot sequence (per MSI contract.yaml) ===== */

static int msi_boot_sequence(void)
{
	pr_info("msi: === Phase 0: Substrate Probe ===\n");
	msi_detect_capabilities();

	pr_info("msi: === Phase 1: State Initialization ===\n");
	/* Core state regions will be mapped on demand */

	pr_info("msi: === Phase 2: Event System ===\n");
	/* Event bus is initialized via list heads above */
	pr_info("msi: event bus ready (max topics: %u)\n",
	        substrate_caps.events_max_topics);

	pr_info("msi: === Phase 3: Cognitive Bootstrap ===\n");
	/* Kernel lanes will be spawned by userspace MSI runtime */
	pr_info("msi: substrate ready — awaiting cognitive programs\n");

	return 0;
}

/* ===== Module init/exit ===== */

static int __init msi_module_init(void)
{
	int ret;

	pr_info("msi: ninjamagicOS MSI v%d.%d.%d loading\n",
	        MSI_VERSION_MAJOR, MSI_VERSION_MINOR, MSI_VERSION_PATCH);

	/* Allocate character device number */
	ret = alloc_chrdev_region(&msi_devno, 0, 1, MSI_DEVICE_NAME);
	if (ret < 0) {
		pr_err("msi: failed to allocate chrdev region: %d\n", ret);
		return ret;
	}

	/* Create device class */
	msi_class = class_create(MSI_CLASS_NAME);
	if (IS_ERR(msi_class)) {
		ret = PTR_ERR(msi_class);
		pr_err("msi: failed to create class: %d\n", ret);
		goto err_class;
	}

	/* Initialize and add cdev */
	cdev_init(&msi_cdev, &msi_fops);
	msi_cdev.owner = THIS_MODULE;
	ret = cdev_add(&msi_cdev, msi_devno, 1);
	if (ret < 0) {
		pr_err("msi: failed to add cdev: %d\n", ret);
		goto err_cdev;
	}

	/* Create /dev/msi device node */
	msi_device = device_create(msi_class, NULL, msi_devno, NULL,
	                           MSI_DEVICE_NAME);
	if (IS_ERR(msi_device)) {
		ret = PTR_ERR(msi_device);
		pr_err("msi: failed to create device: %d\n", ret);
		goto err_device;
	}

	/* Run MSI boot sequence */
	ret = msi_boot_sequence();
	if (ret < 0) {
		pr_err("msi: boot sequence failed: %d\n", ret);
		goto err_boot;
	}

	pr_info("msi: module loaded — /dev/%s ready\n", MSI_DEVICE_NAME);
	return 0;

err_boot:
	device_destroy(msi_class, msi_devno);
err_device:
	cdev_del(&msi_cdev);
err_cdev:
	class_destroy(msi_class);
err_class:
	unregister_chrdev_region(msi_devno, 1);
	return ret;
}

static void __exit msi_module_exit(void)
{
	pr_info("msi: unloading — destroying all domains and lanes\n");

	/* TODO: Kill all active lanes */
	/* TODO: Destroy all domains */
	/* TODO: Free all state regions */
	/* TODO: Drain event queues */

	device_destroy(msi_class, msi_devno);
	cdev_del(&msi_cdev);
	class_destroy(msi_class);
	unregister_chrdev_region(msi_devno, 1);

	pr_info("msi: module unloaded\n");
}

module_init(msi_module_init);
module_exit(msi_module_exit);
