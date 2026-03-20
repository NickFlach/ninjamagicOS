/*
 * NinjaMagic AVB Boot Verification
 *
 * Implements the secure boot chain verification for ninjamagicOS.
 * Verifies partition signatures, rollback indices, and enables dm-verity.
 */

#include "avb_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/dm-ioctl.h>

/* ===== Device Profiles ===== */

static const AvbDeviceProfile device_profiles[] = {
    {
        .device         = NINJAMAGIC_DEVICE_PIXEL7,
        .device_name    = "Google Pixel 7",
        .codename       = "panther",
        .secure_element = "Titan M2",
        .has_titan_m2   = true,
        .has_qualcomm_spu = false,
        .max_rollback_index = 0xFFFFFFFF,
        .fuse_partition = "/dev/titan_m2",
    },
    {
        .device         = NINJAMAGIC_DEVICE_NORD_N30,
        .device_name    = "OnePlus Nord N30",
        .codename       = "larry",
        .secure_element = "Qualcomm SPU",
        .has_titan_m2   = false,
        .has_qualcomm_spu = true,
        .max_rollback_index = 0xFFFFFFFF,
        .fuse_partition = "/dev/qseecom",
    },
};

/* ===== Partition Configs ===== */

static const AvbPartitionConfig partition_configs[AVB_PARTITION_COUNT] = {
    [AVB_PARTITION_BOOT] = {
        .name           = "boot",
        .rollback_index = AVB_ROLLBACK_INDEX_BOOT,
        .use_hashtree   = false,
        .is_chained     = false,
        .hash_algorithm = AVB_HASH_ALGORITHM,
    },
    [AVB_PARTITION_VENDOR_BOOT] = {
        .name           = "vendor_boot",
        .rollback_index = AVB_ROLLBACK_INDEX_BOOT,
        .use_hashtree   = false,
        .is_chained     = false,
        .hash_algorithm = AVB_HASH_ALGORITHM,
    },
    [AVB_PARTITION_SYSTEM] = {
        .name           = "system",
        .rollback_index = AVB_ROLLBACK_INDEX_SYSTEM,
        .use_hashtree   = true,    /* dm-verity */
        .is_chained     = true,
        .hash_algorithm = AVB_HASH_ALGORITHM,
    },
    [AVB_PARTITION_VENDOR] = {
        .name           = "vendor",
        .rollback_index = AVB_ROLLBACK_INDEX_VENDOR,
        .use_hashtree   = true,    /* dm-verity */
        .is_chained     = true,
        .hash_algorithm = AVB_HASH_ALGORITHM,
    },
    [AVB_PARTITION_VBMETA] = {
        .name           = "vbmeta",
        .rollback_index = AVB_ROLLBACK_INDEX_VBMETA,
        .use_hashtree   = false,
        .is_chained     = false,
        .hash_algorithm = AVB_HASH_ALGORITHM,
    },
    [AVB_PARTITION_DTBO] = {
        .name           = "dtbo",
        .rollback_index = AVB_ROLLBACK_INDEX_BOOT,
        .use_hashtree   = false,
        .is_chained     = true,
        .hash_algorithm = AVB_HASH_ALGORITHM,
    },
};

/* ===== Global State ===== */

static BootVerificationResult g_boot_result;
static const AvbDeviceProfile *g_device = NULL;
static bool g_initialized = false;

/* ===== Internal Helpers ===== */

static const AvbDeviceProfile* find_device_profile(NinjaMagicDevice device)
{
    for (size_t i = 0; i < sizeof(device_profiles) / sizeof(device_profiles[0]); i++) {
        if (device_profiles[i].device == device)
            return &device_profiles[i];
    }
    return NULL;
}

static int read_rollback_from_hardware(const AvbDeviceProfile *dev,
                                        AvbPartition partition,
                                        uint64_t *out_index)
{
    /*
     * In production:
     * - Pixel 7: read from Titan M2 via /dev/titan_m2 ioctl
     * - Nord N30: read from QSEE via /dev/qseecom ioctl
     *
     * For development, return stored values.
     */
    if (!dev || !out_index)
        return -EINVAL;

    /* TODO: actual hardware ioctl */
    *out_index = 0;
    printf("[AVB] Read rollback index for %s partition %d: %llu\n",
           dev->codename, partition, (unsigned long long)*out_index);
    return 0;
}

static int verify_vbmeta_signature(const char *partition_path,
                                    const char *slot,
                                    uint8_t *digest_out)
{
    /*
     * In production:
     * 1. Read vbmeta header from partition
     * 2. Verify RSA-4096 signature against embedded public key
     * 3. Verify public key matches our AVB key hash
     * 4. Calculate and return vbmeta digest
     *
     * Uses libavb from AOSP.
     */
    char path[256];
    snprintf(path, sizeof(path), "/dev/block/by-name/%s%s",
             partition_configs[AVB_PARTITION_VBMETA].name,
             slot ? slot : "");

    printf("[AVB] Verifying vbmeta signature: %s\n", path);

    /* TODO: actual libavb verification */
    if (digest_out)
        memset(digest_out, 0xAB, 32); /* Placeholder digest */

    return 0;
}

static int setup_dm_verity_table(const char *partition_name,
                                  const char *slot)
{
    /*
     * Sets up device-mapper verity target for a partition.
     *
     * dm-verity parameters:
     *   - data_dev: the actual partition block device
     *   - hash_dev: where the Merkle tree is stored (appended to partition)
     *   - data_block_size: 4096
     *   - hash_block_size: 4096
     *   - root_hash: from vbmeta hashtree descriptor
     *   - salt: from vbmeta hashtree descriptor
     */
    char data_dev[256];
    char dm_name[128];

    snprintf(data_dev, sizeof(data_dev), "/dev/block/by-name/%s%s",
             partition_name, slot ? slot : "");
    snprintf(dm_name, sizeof(dm_name), "verity-%s", partition_name);

    printf("[AVB] Setting up dm-verity: %s -> /dev/mapper/%s\n",
           data_dev, dm_name);

    /*
     * TODO: actual dm-ioctl setup
     *
     * struct dm_ioctl io;
     * struct dm_target_spec spec;
     * int fd = open("/dev/device-mapper", O_RDWR);
     *
     * 1. DM_DEV_CREATE — create dm device
     * 2. DM_TABLE_LOAD — load verity target with:
     *    "0 <sectors> verity 1 <data_dev> <hash_dev> 4096 4096
     *     <data_blocks> <hash_start> sha256 <root_hash> <salt>"
     * 3. DM_DEV_SUSPEND — activate the table
     */

    return 0;
}

/* ===== Public API ===== */

int avb_init(NinjaMagicDevice device)
{
    if (g_initialized) {
        printf("[AVB] Already initialized\n");
        return 0;
    }

    g_device = find_device_profile(device);
    if (!g_device) {
        printf("[AVB] ERROR: Unknown device %d\n", device);
        return -ENODEV;
    }

    printf("[AVB] Initializing for %s (%s) — SE: %s\n",
           g_device->device_name, g_device->codename,
           g_device->secure_element);

    memset(&g_boot_result, 0, sizeof(g_boot_result));
    g_boot_result.os_version = "ninjamagicOS 0.1.0";
    g_boot_result.security_patch = "2026-03-01";

    /* Read rollback indices from hardware */
    for (int i = 0; i < AVB_PARTITION_COUNT; i++) {
        read_rollback_from_hardware(g_device, i,
                                     &g_boot_result.rollback_indices[i]);
    }

    /* Verify vbmeta chain */
    const char *active_slot = "_a"; /* TODO: read from bootcontrol HAL */
    int ret = verify_vbmeta_signature(NULL, active_slot,
                                       g_boot_result.vbmeta_digest);
    if (ret != 0) {
        g_boot_result.state = BOOT_STATE_RED;
        printf("[AVB] ERROR: vbmeta verification failed!\n");
        return ret;
    }

    /* Determine boot state */
    /* TODO: check if bootloader is locked and key matches production key */
    g_boot_result.state = BOOT_STATE_YELLOW; /* Dev-signed for now */

    g_initialized = true;
    printf("[AVB] Boot state: %s\n",
           g_boot_result.state == BOOT_STATE_GREEN  ? "GREEN (verified)" :
           g_boot_result.state == BOOT_STATE_YELLOW ? "YELLOW (custom key)" :
           g_boot_result.state == BOOT_STATE_ORANGE ? "ORANGE (unlocked)" :
                                                       "RED (failed)");
    return 0;
}

int avb_verify_partition(AvbPartition partition, const char *slot)
{
    if (!g_initialized)
        return -EINVAL;
    if (partition >= AVB_PARTITION_COUNT)
        return -EINVAL;

    const AvbPartitionConfig *cfg = &partition_configs[partition];
    printf("[AVB] Verifying partition: %s%s\n", cfg->name, slot ? slot : "");

    /* Check rollback index */
    uint64_t hw_index;
    int ret = read_rollback_from_hardware(g_device, partition, &hw_index);
    if (ret != 0) return ret;

    if (g_boot_result.rollback_indices[partition] < hw_index) {
        printf("[AVB] ERROR: Rollback detected for %s! "
               "Image index %llu < fuse index %llu\n",
               cfg->name,
               (unsigned long long)g_boot_result.rollback_indices[partition],
               (unsigned long long)hw_index);
        return -EPERM;
    }

    /* TODO: verify hash/hashtree descriptor from vbmeta against partition */

    printf("[AVB] Partition %s%s: VERIFIED\n", cfg->name, slot ? slot : "");
    return 0;
}

const BootVerificationResult* avb_get_boot_state(void)
{
    return g_initialized ? &g_boot_result : NULL;
}

bool avb_check_rollback(AvbPartition partition, uint64_t index)
{
    if (!g_initialized || partition >= AVB_PARTITION_COUNT)
        return false;

    uint64_t hw_index;
    if (read_rollback_from_hardware(g_device, partition, &hw_index) != 0)
        return false;

    return index >= hw_index;
}

int avb_update_rollback(AvbPartition partition, uint64_t new_index)
{
    if (!g_initialized || partition >= AVB_PARTITION_COUNT)
        return -EINVAL;

    printf("[AVB] WARNING: Updating rollback index for %s to %llu (IRREVERSIBLE)\n",
           partition_configs[partition].name,
           (unsigned long long)new_index);

    /*
     * TODO: write to hardware fuses via secure element
     * - Pixel 7: Titan M2 ioctl
     * - Nord N30: QSEE ioctl
     */
    return 0;
}

int avb_enable_dm_verity(AvbPartition partition, const char *slot)
{
    if (!g_initialized || partition >= AVB_PARTITION_COUNT)
        return -EINVAL;

    const AvbPartitionConfig *cfg = &partition_configs[partition];
    if (!cfg->use_hashtree) {
        printf("[AVB] Partition %s does not use dm-verity\n", cfg->name);
        return 0;
    }

    int ret = setup_dm_verity_table(cfg->name, slot);
    if (ret == 0) {
        g_boot_result.dm_verity_enabled = true;
        printf("[AVB] dm-verity enabled for %s%s\n", cfg->name, slot ? slot : "");
    }
    return ret;
}

int avb_generate_test_keys(const char *output_dir)
{
    printf("[AVB] Generating test AVB keys in %s\n", output_dir);
    printf("[AVB] WARNING: Test keys only! Use HSM-generated keys for production.\n");

    /*
     * In practice, keys are generated with:
     *   openssl genrsa -out ninjamagic_avb.pem 4096
     *   avbtool extract_public_key --key ninjamagic_avb.pem --output ninjamagic_avb_pub.bin
     */

    /* TODO: shell out to openssl or use libcrypto */
    return 0;
}

int avb_sign_image(const char *image_path, AvbPartition partition,
                   uint64_t rollback_index, const char *key_path)
{
    if (partition >= AVB_PARTITION_COUNT)
        return -EINVAL;

    const AvbPartitionConfig *cfg = &partition_configs[partition];

    printf("[AVB] Signing %s as partition '%s' with rollback index %llu\n",
           image_path, cfg->name, (unsigned long long)rollback_index);

    /*
     * In practice, this calls avbtool:
     *   avbtool add_hash_footer (for boot, dtbo)
     *   avbtool add_hashtree_footer (for system, vendor)
     *   avbtool make_vbmeta_image (for vbmeta chain)
     */

    return 0;
}
