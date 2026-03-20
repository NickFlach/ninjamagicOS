/*
 * NinjaMagic OTA Update Manager — Implementation
 *
 * A/B seamless updates with agent-managed flow and MSI event integration.
 */

#include "ota_manager.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>

#define TAG "ota_manager"

/* ===== State ===== */

static SlotInfo slots[2];
static OtaSlot active_slot;
static UpdateProgress current_update;
static UpdateConfig current_config;
static bool initialized = false;

/* OTA server URL */
static const char *OTA_SERVER_URL = "https://ota.ninjamagicos.dev/v1";

/* ===== Initialization ===== */

int ota_init(void)
{
    if (initialized)
        return 0;

    memset(slots, 0, sizeof(slots));
    memset(&current_update, 0, sizeof(current_update));
    memset(&current_config, 0, sizeof(current_config));

    /* Default config */
    current_config.policy = UPDATE_POLICY_WIFI_ONLY;
    current_config.auto_reboot = false;
    current_config.auto_reboot_hour = 3;  /* 3 AM */
    current_config.check_interval_hours = 24;
    current_config.allow_metered = false;
    current_config.require_charging = true;
    current_config.min_battery_percent = 30;

    /*
     * Read actual slot state from bootcontrol HAL:
     *   android::hardware::boot::V1_2::IBootControl
     *
     * For Pixel 7: /dev/block/by-name/misc (BCB)
     * For Nord N30: /dev/block/by-name/misc
     */

    /* Initialize slot A */
    slots[SLOT_A].slot = SLOT_A;
    slots[SLOT_A].state = SLOT_STATE_ACTIVE;
    slots[SLOT_A].is_bootable = true;
    slots[SLOT_A].is_active = true;
    strncpy(slots[SLOT_A].os_version, "ninjamagicOS 0.1.0",
            sizeof(slots[SLOT_A].os_version) - 1);
    strncpy(slots[SLOT_A].security_patch, "2026-03-01",
            sizeof(slots[SLOT_A].security_patch) - 1);
    slots[SLOT_A].boot_count = 1;

    /* Initialize slot B as empty */
    slots[SLOT_B].slot = SLOT_B;
    slots[SLOT_B].state = SLOT_STATE_EMPTY;
    slots[SLOT_B].is_bootable = false;
    slots[SLOT_B].is_active = false;

    active_slot = SLOT_A;
    current_update.state = UPDATE_STATE_IDLE;

    initialized = true;

    printf("[%s] OTA manager initialized\n", TAG);
    printf("[%s]   Active slot: %c\n", TAG, active_slot == SLOT_A ? 'A' : 'B');
    printf("[%s]   OS: %s\n", TAG, slots[active_slot].os_version);
    printf("[%s]   Policy: %s\n", TAG,
           current_config.policy == UPDATE_POLICY_AUTO      ? "AUTO" :
           current_config.policy == UPDATE_POLICY_WIFI_ONLY ? "WIFI_ONLY" :
           current_config.policy == UPDATE_POLICY_PROMPT    ? "PROMPT" :
           current_config.policy == UPDATE_POLICY_CHARGING  ? "CHARGING" :
                                                               "MANUAL");

    return 0;
}

void ota_shutdown(void)
{
    if (!initialized)
        return;

    if (current_update.state == UPDATE_STATE_DOWNLOADING ||
        current_update.state == UPDATE_STATE_APPLYING) {
        printf("[%s] WARNING: Shutting down with update in progress!\n", TAG);
    }

    initialized = false;
    printf("[%s] OTA manager shut down\n", TAG);
}

/* ===== Slot Management ===== */

int ota_get_slot_info(OtaSlot slot, SlotInfo *out)
{
    if (!initialized || !out)
        return -EINVAL;
    if (slot > SLOT_B)
        return -EINVAL;

    memcpy(out, &slots[slot], sizeof(SlotInfo));
    return 0;
}

OtaSlot ota_get_active_slot(void)
{
    return active_slot;
}

OtaSlot ota_get_inactive_slot(void)
{
    return (active_slot == SLOT_A) ? SLOT_B : SLOT_A;
}

int ota_mark_boot_successful(void)
{
    if (!initialized)
        return -EINVAL;

    SlotInfo *active = &slots[active_slot];

    if (active->state == SLOT_STATE_PENDING) {
        active->state = SLOT_STATE_GOOD;
        printf("[%s] Slot %c marked as GOOD after successful boot\n",
               TAG, active_slot == SLOT_A ? 'A' : 'B');

        /*
         * TODO: publish MSI event system/ota/complete
         * TODO: call bootcontrol HAL markBootSuccessful()
         */
    }

    active->state = SLOT_STATE_ACTIVE;
    active->boot_count++;

    return 0;
}

int ota_set_active_slot(OtaSlot slot)
{
    if (!initialized || slot > SLOT_B)
        return -EINVAL;

    if (!slots[slot].is_bootable) {
        printf("[%s] ERROR: Slot %c is not bootable\n",
               TAG, slot == SLOT_A ? 'A' : 'B');
        return -EINVAL;
    }

    /*
     * TODO: call bootcontrol HAL setActiveBootSlot(slot)
     * This writes to the BCB (boot control block) in /dev/block/by-name/misc
     */

    slots[active_slot].is_active = false;
    slots[active_slot].state = SLOT_STATE_GOOD;
    slots[slot].is_active = true;
    slots[slot].state = SLOT_STATE_PENDING;
    active_slot = slot;

    printf("[%s] Active slot set to %c (pending verification)\n",
           TAG, slot == SLOT_A ? 'A' : 'B');

    return 0;
}

/* ===== Update Flow ===== */

int ota_check_for_update(UpdateInfo *out)
{
    if (!initialized)
        return -EINVAL;

    current_update.state = UPDATE_STATE_CHECKING;
    printf("[%s] Checking for updates at %s\n", TAG, OTA_SERVER_URL);

    /*
     * TODO: HTTP GET to OTA server
     *   GET /v1/check?device=panther&version=0.1.0&slot=a&patch=2026-03-01
     *
     * Server responds with UpdateInfo JSON if update available,
     * or 204 No Content if up to date.
     *
     * For now, simulate no update available.
     */

    /* TODO: publish MSI event system/ota/check */

    current_update.state = UPDATE_STATE_IDLE;
    printf("[%s] No update available (current: %s)\n",
           TAG, slots[active_slot].os_version);

    return -ENOENT; /* No update available */
}

int ota_start_download(const UpdateInfo *info)
{
    if (!initialized || !info)
        return -EINVAL;

    if (current_update.state != UPDATE_STATE_IDLE &&
        current_update.state != UPDATE_STATE_AVAILABLE) {
        printf("[%s] ERROR: Cannot start download in state %d\n",
               TAG, current_update.state);
        return -EBUSY;
    }

    /* Check policy */
    if (current_config.policy == UPDATE_POLICY_MANUAL) {
        printf("[%s] Policy is MANUAL — download not started\n", TAG);
        return -EPERM;
    }

    memcpy(&current_update.info, info, sizeof(UpdateInfo));
    current_update.state = UPDATE_STATE_DOWNLOADING;
    current_update.target_slot = ota_get_inactive_slot();
    current_update.bytes_downloaded = 0;
    current_update.bytes_total = info->download_size;
    current_update.progress = 0.0f;
    current_update.paused = false;
    current_update.started_timestamp = (uint64_t)time(NULL);
    current_update.error[0] = '\0';

    printf("[%s] Downloading update %s (%llu bytes) to slot %c\n",
           TAG, info->version,
           (unsigned long long)info->download_size,
           current_update.target_slot == SLOT_A ? 'A' : 'B');

    /*
     * TODO: start async download
     *   - Resume support: check /data/ota/payload.bin for partial download
     *   - Verify download hash against info->payload_hash
     *   - Publish progress to MSI event system/ota/progress
     */

    return 0;
}

int ota_pause_download(void)
{
    if (current_update.state != UPDATE_STATE_DOWNLOADING)
        return -EINVAL;

    current_update.paused = true;
    printf("[%s] Download paused at %.1f%%\n",
           TAG, current_update.progress * 100.0f);
    return 0;
}

int ota_resume_download(void)
{
    if (current_update.state != UPDATE_STATE_DOWNLOADING || !current_update.paused)
        return -EINVAL;

    current_update.paused = false;
    printf("[%s] Download resumed\n", TAG);
    return 0;
}

int ota_cancel(void)
{
    if (current_update.state == UPDATE_STATE_IDLE)
        return 0;

    printf("[%s] Update cancelled (was in state %d)\n",
           TAG, current_update.state);

    current_update.state = UPDATE_STATE_IDLE;
    current_update.progress = 0.0f;
    current_update.bytes_downloaded = 0;

    /* TODO: delete /data/ota/payload.bin */
    /* TODO: publish MSI event system/ota/failed with "cancelled" */

    return 0;
}

int ota_apply_update(void)
{
    if (!initialized)
        return -EINVAL;

    if (current_update.state != UPDATE_STATE_DOWNLOADING) {
        /* Allow apply if we have a fully downloaded payload */
        printf("[%s] ERROR: No downloaded update to apply\n", TAG);
        return -EINVAL;
    }

    /* Check battery */
    if (current_config.min_battery_percent > 0) {
        /* TODO: check actual battery level */
        printf("[%s] Battery check: require >= %u%%\n",
               TAG, current_config.min_battery_percent);
    }

    current_update.state = UPDATE_STATE_VERIFYING;
    printf("[%s] Verifying update payload...\n", TAG);

    /*
     * TODO: verify payload
     * 1. Check SHA-256 hash matches UpdateInfo.payload_hash
     * 2. Verify AVB signature on the payload
     * 3. Check rollback index >= current
     */

    current_update.state = UPDATE_STATE_APPLYING;
    printf("[%s] Applying update to slot %c...\n",
           TAG, current_update.target_slot == SLOT_A ? 'A' : 'B');

    /*
     * TODO: apply via update_engine
     *
     * For full payload:
     *   Write each partition image to the inactive slot block devices
     *   /dev/block/by-name/boot_b, system_b, vendor_b, etc.
     *
     * For delta payload:
     *   Apply binary diffs (bsdiff) to each partition
     *   Source: active slot block devices
     *   Target: inactive slot block devices
     *
     * After writing:
     *   avb_sign_image() for each partition
     *   Write vbmeta to inactive slot
     */

    /* Mark target slot as bootable */
    OtaSlot target = current_update.target_slot;
    slots[target].is_bootable = true;
    strncpy(slots[target].os_version, current_update.info.version,
            sizeof(slots[target].os_version) - 1);
    strncpy(slots[target].security_patch, current_update.info.security_patch,
            sizeof(slots[target].security_patch) - 1);
    slots[target].rollback_index = current_update.info.rollback_index;
    slots[target].installed_timestamp = (uint64_t)time(NULL);
    slots[target].boot_count = 0;
    slots[target].boot_fail_count = 0;

    /* Set inactive slot as next boot target */
    ota_set_active_slot(target);

    current_update.state = UPDATE_STATE_PENDING_REBOOT;
    current_update.progress = 1.0f;

    printf("[%s] Update applied successfully. Reboot required.\n", TAG);

    /* TODO: publish MSI event system/ota/ready */

    return 0;
}

int ota_get_progress(UpdateProgress *out)
{
    if (!initialized || !out)
        return -EINVAL;

    memcpy(out, &current_update, sizeof(UpdateProgress));
    return 0;
}

/* ===== Configuration ===== */

int ota_set_config(const UpdateConfig *config)
{
    if (!initialized || !config)
        return -EINVAL;

    memcpy(&current_config, config, sizeof(UpdateConfig));

    printf("[%s] Config updated: policy=%d auto_reboot=%d interval=%uh\n",
           TAG, config->policy, config->auto_reboot,
           config->check_interval_hours);

    return 0;
}

int ota_get_config(UpdateConfig *out)
{
    if (!initialized || !out)
        return -EINVAL;

    memcpy(out, &current_config, sizeof(UpdateConfig));
    return 0;
}

/* ===== Agent Integration ===== */

int ota_schedule_reboot(uint64_t reboot_at_timestamp)
{
    if (!initialized)
        return -EINVAL;

    if (current_update.state != UPDATE_STATE_PENDING_REBOOT) {
        printf("[%s] Cannot schedule reboot — no pending update\n", TAG);
        return -EINVAL;
    }

    printf("[%s] Reboot scheduled by agent for timestamp %llu\n",
           TAG, (unsigned long long)reboot_at_timestamp);

    /*
     * TODO: schedule reboot via AlarmManager or kernel timer
     * Agent chooses optimal time:
     *   - Phone is idle (screen off, no calls)
     *   - Plugged in / charging
     *   - Preferred hour from config (default 3 AM)
     *   - Not during active navigation or media playback
     */

    return 0;
}

int ota_agent_approve(bool approved)
{
    if (!initialized)
        return -EINVAL;

    if (current_update.state != UPDATE_STATE_AVAILABLE) {
        return -EINVAL;
    }

    if (approved) {
        printf("[%s] Agent approved update — starting download\n", TAG);
        return ota_start_download(&current_update.info);
    } else {
        printf("[%s] Agent/user declined update\n", TAG);
        if (!current_update.info.is_mandatory) {
            current_update.state = UPDATE_STATE_IDLE;
            return 0;
        } else {
            printf("[%s] WARNING: Mandatory security update cannot be skipped\n", TAG);
            return -EPERM;
        }
    }
}
