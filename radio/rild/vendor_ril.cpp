/*
 * vendor_ril.cpp — Dynamic Vendor RIL Library Loader Implementation
 *
 * Probes the device, loads the correct vendor RIL .so via dlopen(),
 * resolves RIL_Init, and provides a unified request interface.
 */

#include "vendor_ril.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include <unistd.h>
#include <sys/system_properties.h>
#include <errno.h>

#define LOG_TAG "NinjaMagicRIL-VendorLoader"
#ifdef __ANDROID__
#include <android/log.h>
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#else
#define LOGI(...) do { printf("[VendorRIL] "); printf(__VA_ARGS__); printf("\n"); } while(0)
#define LOGE(...) do { fprintf(stderr, "[VendorRIL ERROR] "); fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n"); } while(0)
#define LOGW(...) do { fprintf(stderr, "[VendorRIL WARN] "); fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n"); } while(0)
#endif

/* ===== Internal state ===== */

static void *s_vendor_handle = NULL;
static const RIL_RadioFunctions *s_vendor_funcs = NULL;
static char s_vendor_lib_path[256] = {0};

/* ===== Device detection ===== */

typedef enum {
    DEVICE_UNKNOWN = 0,
    DEVICE_PIXEL7,       /* Google Pixel 7 (panther) — Tensor GS201, Exynos 5300 modem */
    DEVICE_NORDN30,      /* OnePlus Nord N30 (larry) — Snapdragon 695, X51 modem */
} device_type_t;

static device_type_t detect_device(void) {
    char prop_value[256] = {0};

    /* Check ro.ninjamagic.device first (set by our product .mk) */
#ifdef __ANDROID__
    __system_property_get("ro.ninjamagic.device", prop_value);
#else
    /* Fallback for host testing */
    const char *env = getenv("NINJAMAGIC_DEVICE");
    if (env) strncpy(prop_value, env, sizeof(prop_value) - 1);
#endif

    if (strcmp(prop_value, "panther") == 0) {
        LOGI("Device detected: Pixel 7 (panther)");
        return DEVICE_PIXEL7;
    }
    if (strcmp(prop_value, "larry") == 0) {
        LOGI("Device detected: Nord N30 (larry)");
        return DEVICE_NORDN30;
    }

    /* Fallback: check ro.hardware.chipset */
    memset(prop_value, 0, sizeof(prop_value));
#ifdef __ANDROID__
    __system_property_get("ro.hardware.chipset", prop_value);
#endif

    if (strstr(prop_value, "gs201") || strstr(prop_value, "tensor")) {
        LOGI("Device detected via chipset: Tensor GS201 (Pixel 7)");
        return DEVICE_PIXEL7;
    }
    if (strstr(prop_value, "sm6375") || strstr(prop_value, "695")) {
        LOGI("Device detected via chipset: Snapdragon 695 (Nord N30)");
        return DEVICE_NORDN30;
    }

    LOGW("Unknown device — will try generic vendor RIL");
    return DEVICE_UNKNOWN;
}

/* ===== Library loading ===== */

static const char *get_candidate_paths(device_type_t device, int index) {
    switch (device) {
    case DEVICE_PIXEL7:
        switch (index) {
        case 0: return VENDOR_RIL_PATH_PIXEL7;
        case 1: return VENDOR_RIL_PATH_PIXEL7_ALT;
        case 2: return VENDOR_RIL_PATH_GENERIC;
        default: return NULL;
        }
    case DEVICE_NORDN30:
        switch (index) {
        case 0: return VENDOR_RIL_PATH_NORDN30;
        case 1: return VENDOR_RIL_PATH_NORDN30_ALT;
        case 2: return VENDOR_RIL_PATH_GENERIC;
        default: return NULL;
        }
    default:
        switch (index) {
        case 0: return VENDOR_RIL_PATH_GENERIC;
        default: return NULL;
        }
    }
}

static void *try_load_library(const char *path) {
    if (access(path, R_OK) != 0) {
        LOGI("Candidate %s — not found", path);
        return NULL;
    }

    LOGI("Attempting to load: %s", path);
    void *handle = dlopen(path, RTLD_NOW);
    if (!handle) {
        LOGW("dlopen failed for %s: %s", path, dlerror());
        return NULL;
    }

    /* Verify RIL_Init symbol exists */
    void *init_sym = dlsym(handle, "RIL_Init");
    if (!init_sym) {
        LOGW("RIL_Init not found in %s", path);
        dlclose(handle);
        return NULL;
    }

    LOGI("Successfully loaded: %s", path);
    return handle;
}

/* ===== Public API ===== */

int vendor_ril_load(const RIL_RadioFunctions_Callbacks *callbacks) {
    if (s_vendor_handle) {
        LOGW("Vendor RIL already loaded");
        return 0;
    }

    if (!callbacks) {
        LOGE("callbacks is NULL");
        return -EINVAL;
    }

    device_type_t device = detect_device();

    /* Try candidate library paths in order */
    for (int i = 0; ; i++) {
        const char *path = get_candidate_paths(device, i);
        if (!path) break;

        s_vendor_handle = try_load_library(path);
        if (s_vendor_handle) {
            strncpy(s_vendor_lib_path, path, sizeof(s_vendor_lib_path) - 1);
            break;
        }
    }

    if (!s_vendor_handle) {
        LOGE("Failed to load any vendor RIL library");
        return -ENOENT;
    }

    /* Resolve RIL_Init */
    RIL_InitFunc ril_init = (RIL_InitFunc)dlsym(s_vendor_handle, "RIL_Init");
    if (!ril_init) {
        LOGE("Failed to resolve RIL_Init: %s", dlerror());
        dlclose(s_vendor_handle);
        s_vendor_handle = NULL;
        return -ENOSYS;
    }

    /* Prepare vendor RIL arguments */
    char *argv[] = {
        (char *)"ninjamagic-rild",
        (char *)"-d",
        (char *)"/dev/ttyS0", /* modem device — adjusted per device */
        NULL
    };
    int argc = 3;

    /* Call RIL_Init — this starts the vendor modem communication */
    LOGI("Calling RIL_Init...");
    s_vendor_funcs = ril_init(callbacks, argc, argv);
    if (!s_vendor_funcs) {
        LOGE("RIL_Init returned NULL");
        dlclose(s_vendor_handle);
        s_vendor_handle = NULL;
        return -EIO;
    }

    LOGI("Vendor RIL initialized — version=%d lib=%s",
         s_vendor_funcs->version, s_vendor_lib_path);

    if (s_vendor_funcs->getVersion) {
        LOGI("Vendor RIL version string: %s", s_vendor_funcs->getVersion());
    }

    return 0;
}

void vendor_ril_unload(void) {
    s_vendor_funcs = NULL;
    if (s_vendor_handle) {
        dlclose(s_vendor_handle);
        s_vendor_handle = NULL;
    }
    memset(s_vendor_lib_path, 0, sizeof(s_vendor_lib_path));
    LOGI("Vendor RIL unloaded");
}

const RIL_RadioFunctions *vendor_ril_get_functions(void) {
    return s_vendor_funcs;
}

int vendor_ril_request(int request, const void *data, size_t data_len,
                       RIL_Token token) {
    if (!s_vendor_funcs || !s_vendor_funcs->onRequest) {
        LOGE("Vendor RIL not loaded or onRequest is NULL");
        return -ENODEV;
    }

    /* Check if vendor supports this request */
    if (s_vendor_funcs->supports && !s_vendor_funcs->supports(request)) {
        LOGW("Vendor RIL does not support request %d", request);
        return -ENOTSUP;
    }

    s_vendor_funcs->onRequest(request, data, data_len, token);
    return 0;
}

int vendor_ril_get_state(void) {
    if (!s_vendor_funcs || !s_vendor_funcs->onStateRequest) {
        return -1;
    }
    return s_vendor_funcs->onStateRequest();
}

const char *vendor_ril_get_version(void) {
    if (!s_vendor_funcs || !s_vendor_funcs->getVersion) {
        return "unknown";
    }
    return s_vendor_funcs->getVersion();
}
