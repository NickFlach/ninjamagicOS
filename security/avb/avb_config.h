/*
 * NinjaMagic AVB (Android Verified Boot) Configuration
 *
 * Implements AVB 2.0 for secure boot chain verification on both
 * Pixel 7 (Tensor GS201 / Titan M2) and Nord N30 (Snapdragon 695 / SPU).
 *
 * Boot chain:
 *   Bootloader (ROM) → ABL (signed) → Kernel (AVB) → MSI module (dm-verity) → Userspace
 *
 * Partition layout (both devices use A/B):
 *   boot_a/boot_b     — kernel + ramdisk (AVB signed)
 *   vendor_boot_a/b   — vendor ramdisk + dtb (AVB signed)
 *   system_a/b        — main OS (dm-verity protected)
 *   vendor_a/b        — vendor HALs (dm-verity protected)
 *   vbmeta_a/b        — AVB metadata chain
 *   userdata           — FBE encrypted
 */

#ifndef NINJAMAGIC_AVB_CONFIG_H
#define NINJAMAGIC_AVB_CONFIG_H

#include <stdint.h>
#include <stdbool.h>

/* ===== Key Configuration ===== */

/* AVB key sizes */
#define AVB_RSA_KEY_SIZE        4096
#define AVB_HASH_ALGORITHM      "sha256"
#define AVB_SIGNING_ALGORITHM   "SHA256_RSA4096"

/* Key paths (relative to build root) */
#define AVB_KEY_DIR                     "security/avb/keys"
#define AVB_SIGNING_KEY_PATH            AVB_KEY_DIR "/ninjamagic_avb.pem"
#define AVB_SIGNING_PUBKEY_PATH         AVB_KEY_DIR "/ninjamagic_avb_pub.bin"
#define AVB_VERITY_KEY_PATH             AVB_KEY_DIR "/ninjamagic_verity.pem"

/* Rollback protection indices */
#define AVB_ROLLBACK_INDEX_BOOT         0
#define AVB_ROLLBACK_INDEX_SYSTEM       1
#define AVB_ROLLBACK_INDEX_VENDOR       2
#define AVB_ROLLBACK_INDEX_VBMETA       3

/* ===== Partition Configuration ===== */

typedef enum {
    AVB_PARTITION_BOOT,
    AVB_PARTITION_VENDOR_BOOT,
    AVB_PARTITION_SYSTEM,
    AVB_PARTITION_VENDOR,
    AVB_PARTITION_VBMETA,
    AVB_PARTITION_DTBO,
    AVB_PARTITION_COUNT,
} AvbPartition;

typedef struct {
    const char*     name;
    const char*     slot_suffix;  /* "_a" or "_b" */
    uint64_t        rollback_index;
    bool            use_hashtree;  /* dm-verity hashtree vs hash descriptor */
    bool            is_chained;    /* chained vbmeta partition */
    const char*     hash_algorithm;
} AvbPartitionConfig;

/* ===== Device-Specific Configuration ===== */

typedef enum {
    NINJAMAGIC_DEVICE_PIXEL7,      /* Google Pixel 7 (panther) — Titan M2 */
    NINJAMAGIC_DEVICE_NORD_N30,    /* OnePlus Nord N30 (larry) — Qualcomm SPU */
} NinjaMagicDevice;

typedef struct {
    NinjaMagicDevice device;
    const char*      device_name;
    const char*      codename;
    const char*      secure_element;    /* Hardware security module */
    bool             has_titan_m2;
    bool             has_qualcomm_spu;
    uint32_t         max_rollback_index;
    const char*      fuse_partition;    /* Where rollback index is stored */
} AvbDeviceProfile;

/* ===== Boot State ===== */

typedef enum {
    BOOT_STATE_GREEN,      /* Fully verified, locked bootloader */
    BOOT_STATE_YELLOW,     /* Custom key, locked bootloader (dev signed) */
    BOOT_STATE_ORANGE,     /* Unlocked bootloader */
    BOOT_STATE_RED,        /* Verification failed */
} BootState;

typedef struct {
    BootState       state;
    uint64_t        rollback_indices[AVB_PARTITION_COUNT];
    bool            dm_verity_enabled;
    bool            fbe_enabled;       /* File-based encryption */
    const char*     os_version;
    const char*     security_patch;
    uint8_t         vbmeta_digest[32]; /* SHA-256 of vbmeta chain */
} BootVerificationResult;

/* ===== API ===== */

/**
 * Initialize AVB subsystem for the given device.
 * Reads fuses, verifies boot chain, sets up dm-verity.
 */
int avb_init(NinjaMagicDevice device);

/**
 * Verify a partition's AVB signature.
 * Returns 0 on success, negative on failure.
 */
int avb_verify_partition(AvbPartition partition, const char* slot);

/**
 * Get the current boot verification result.
 */
const BootVerificationResult* avb_get_boot_state(void);

/**
 * Check if rollback protection allows the given index.
 * Compares against hardware fuse values.
 */
bool avb_check_rollback(AvbPartition partition, uint64_t index);

/**
 * Update rollback index in hardware fuses (irreversible on locked devices).
 */
int avb_update_rollback(AvbPartition partition, uint64_t new_index);

/**
 * Enable dm-verity on a partition.
 * Uses hashtree descriptor from vbmeta.
 */
int avb_enable_dm_verity(AvbPartition partition, const char* slot);

/**
 * Generate AVB signing keys (development use only).
 * In production, keys are generated offline on HSM.
 */
int avb_generate_test_keys(const char* output_dir);

/**
 * Sign a partition image with the AVB signing key.
 */
int avb_sign_image(const char* image_path, AvbPartition partition,
                   uint64_t rollback_index, const char* key_path);

#endif /* NINJAMAGIC_AVB_CONFIG_H */
