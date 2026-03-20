/*
 * NinjaMagic OTA Update Manager
 *
 * Handles over-the-air updates for ninjamagicOS using Android's A/B
 * partition scheme. Updates are downloaded, verified, applied to the
 * inactive slot, and activated on next reboot.
 *
 * Features:
 * - A/B seamless updates (no downtime during install)
 * - Delta/incremental updates for bandwidth efficiency
 * - AVB signature verification before applying
 * - Rollback on boot failure (automatic)
 * - Agent-managed update flow (agent notifies user, schedules install)
 * - MSI event integration for update progress reporting
 *
 * Update flow:
 *   1. Agent checks for updates (periodic or user-triggered)
 *   2. Download OTA payload to /data/ota/ (resume supported)
 *   3. Verify payload signature against AVB key
 *   4. Apply to inactive slot via update_engine
 *   5. Mark inactive slot as bootable
 *   6. Agent prompts user to reboot (or schedules for charging)
 *   7. On boot: bootloader tries new slot
 *   8. If boot succeeds: mark slot as good
 *   9. If boot fails: bootloader rolls back to previous slot
 */

#ifndef NINJAMAGIC_OTA_MANAGER_H
#define NINJAMAGIC_OTA_MANAGER_H

#include <stdint.h>
#include <stdbool.h>

/* ===== Slot Management ===== */

typedef enum {
    SLOT_A = 0,
    SLOT_B = 1,
} OtaSlot;

typedef enum {
    SLOT_STATE_GOOD,        /* Verified and booting fine */
    SLOT_STATE_ACTIVE,      /* Currently booted slot */
    SLOT_STATE_PENDING,     /* Updated, waiting for reboot verification */
    SLOT_STATE_FAILED,      /* Boot failed, rolled back */
    SLOT_STATE_EMPTY,       /* No valid OS image */
} SlotState;

typedef struct {
    OtaSlot         slot;
    SlotState       state;
    char            os_version[32];
    char            build_id[64];
    char            security_patch[16];
    uint64_t        rollback_index;
    uint64_t        installed_timestamp;
    uint32_t        boot_count;         /* Boots on this slot */
    uint32_t        boot_fail_count;    /* Failed boots */
    bool            is_bootable;
    bool            is_active;
} SlotInfo;

/* ===== Update Payload ===== */

typedef enum {
    PAYLOAD_FULL,           /* Full image for all partitions */
    PAYLOAD_DELTA,          /* Binary diff against current slot */
} PayloadType;

typedef enum {
    UPDATE_STATE_IDLE,
    UPDATE_STATE_CHECKING,
    UPDATE_STATE_AVAILABLE,
    UPDATE_STATE_DOWNLOADING,
    UPDATE_STATE_VERIFYING,
    UPDATE_STATE_APPLYING,
    UPDATE_STATE_PENDING_REBOOT,
    UPDATE_STATE_REBOOTING,
    UPDATE_STATE_FAILED,
} UpdateState;

typedef struct {
    char            version[32];
    char            build_id[64];
    char            security_patch[16];
    char            changelog[1024];
    PayloadType     type;
    uint64_t        payload_size;
    uint64_t        download_size;      /* Compressed/delta size */
    uint8_t         payload_hash[32];   /* SHA-256 of payload */
    char            download_url[512];
    uint64_t        rollback_index;
    bool            is_mandatory;       /* Security update, cannot skip */
} UpdateInfo;

typedef struct {
    UpdateState     state;
    UpdateInfo      info;
    OtaSlot         target_slot;
    uint64_t        bytes_downloaded;
    uint64_t        bytes_total;
    float           progress;           /* 0.0 - 1.0 */
    char            error[256];
    uint64_t        started_timestamp;
    uint64_t        eta_seconds;
    bool            paused;
} UpdateProgress;

/* ===== Update Policy ===== */

typedef enum {
    UPDATE_POLICY_AUTO,         /* Download + install automatically */
    UPDATE_POLICY_WIFI_ONLY,    /* Only download on WiFi */
    UPDATE_POLICY_PROMPT,       /* Always ask user before downloading */
    UPDATE_POLICY_CHARGING,     /* Only install while charging */
    UPDATE_POLICY_MANUAL,       /* Only check when user requests */
} UpdatePolicy;

typedef struct {
    UpdatePolicy    policy;
    bool            auto_reboot;            /* Reboot automatically after install */
    uint32_t        auto_reboot_hour;       /* Preferred reboot hour (0-23) */
    uint32_t        check_interval_hours;   /* How often to check (default: 24) */
    bool            allow_metered;          /* Download on metered connections */
    bool            require_charging;       /* Only install while charging */
    uint32_t        min_battery_percent;    /* Min battery to start install */
} UpdateConfig;

/* ===== MSI Event Topics ===== */

/*
 * OTA events published to MSI event bus:
 *   system/ota/check       — checking for update
 *   system/ota/available   — update available (payload: UpdateInfo JSON)
 *   system/ota/progress    — download/install progress (payload: UpdateProgress JSON)
 *   system/ota/ready       — ready to reboot
 *   system/ota/failed      — update failed (payload: error message)
 *   system/ota/complete    — update installed successfully after reboot
 */

/* ===== API ===== */

/** Initialize OTA manager, read slot info from bootcontrol HAL. */
int ota_init(void);

/** Shutdown OTA manager. */
void ota_shutdown(void);

/* --- Slot Management --- */

/** Get info for a slot. */
int ota_get_slot_info(OtaSlot slot, SlotInfo *out);

/** Get the currently active slot. */
OtaSlot ota_get_active_slot(void);

/** Get the inactive slot (target for updates). */
OtaSlot ota_get_inactive_slot(void);

/** Mark the active slot as good (called after successful boot). */
int ota_mark_boot_successful(void);

/** Set the next boot slot (used after applying update). */
int ota_set_active_slot(OtaSlot slot);

/* --- Update Flow --- */

/** Check for available updates from the OTA server. */
int ota_check_for_update(UpdateInfo *out);

/** Start downloading an update. Non-blocking, progress via events. */
int ota_start_download(const UpdateInfo *info);

/** Pause/resume an ongoing download. */
int ota_pause_download(void);
int ota_resume_download(void);

/** Cancel an ongoing download or installation. */
int ota_cancel(void);

/** Apply a downloaded update to the inactive slot. */
int ota_apply_update(void);

/** Get current update progress. */
int ota_get_progress(UpdateProgress *out);

/* --- Configuration --- */

/** Set update policy/config. */
int ota_set_config(const UpdateConfig *config);

/** Get current update policy/config. */
int ota_get_config(UpdateConfig *out);

/* --- Agent Integration --- */

/**
 * Called by the NinjaMagic Agent to schedule a reboot.
 * Agent decides the optimal time based on user activity and charging state.
 */
int ota_schedule_reboot(uint64_t reboot_at_timestamp);

/**
 * Called by the NinjaMagic Agent to approve an update.
 * For PROMPT policy, agent asks user and calls this with the decision.
 */
int ota_agent_approve(bool approved);

#endif /* NINJAMAGIC_OTA_MANAGER_H */
