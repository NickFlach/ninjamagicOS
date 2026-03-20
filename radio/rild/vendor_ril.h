/*
 * vendor_ril.h — Dynamic Vendor RIL Library Loader
 *
 * Loads the device-specific vendor RIL shared library at runtime.
 * The vendor RIL is the proprietary binary that talks directly to
 * the modem firmware (Exynos 5300 on Pixel 7, Snapdragon X51 on Nord N30).
 *
 * We dynamically load it via dlopen() so that ninjamagicOS RILD can
 * work with either vendor's library without compile-time linking.
 */

#ifndef NINJAMAGIC_VENDOR_RIL_H
#define NINJAMAGIC_VENDOR_RIL_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* RIL request IDs (subset of AOSP ril.h) */
#define RIL_REQUEST_DIAL                10
#define RIL_REQUEST_HANGUP              12
#define RIL_REQUEST_ANSWER              40
#define RIL_REQUEST_SEND_SMS            25
#define RIL_REQUEST_SETUP_DATA_CALL     27
#define RIL_REQUEST_DEACTIVATE_DATA     41
#define RIL_REQUEST_SIGNAL_STRENGTH     19
#define RIL_REQUEST_REGISTRATION_STATE  20
#define RIL_REQUEST_OPERATOR            22
#define RIL_REQUEST_SIM_STATUS          1
#define RIL_REQUEST_SET_RADIO_POWER     23

/* RIL unsolicited response IDs */
#define RIL_UNSOL_RESPONSE_CALL_STATE_CHANGED    1001
#define RIL_UNSOL_RESPONSE_NEW_SMS               1003
#define RIL_UNSOL_SIGNAL_STRENGTH                1009
#define RIL_UNSOL_DATA_CALL_LIST_CHANGED         1010
#define RIL_UNSOL_NITZ_TIME_RECEIVED             1008
#define RIL_UNSOL_RESPONSE_SIM_STATUS_CHANGED    1019
#define RIL_UNSOL_RESPONSE_NETWORK_STATE_CHANGED 1002

/* RIL error codes */
#define RIL_E_SUCCESS           0
#define RIL_E_RADIO_NOT_AVAIL   1
#define RIL_E_GENERIC_FAILURE   2
#define RIL_E_REQUEST_NOT_SUPPORTED 6

/* Token for tracking solicited responses */
typedef uint32_t RIL_Token;

/*
 * Vendor RIL callback structure — the vendor library calls these
 * when it has a response or unsolicited event.
 */
typedef struct {
    /* Solicited response to a request */
    void (*OnRequestComplete)(RIL_Token token, int error,
                              const void *response, size_t response_len);

    /* Unsolicited event from the modem */
    void (*OnUnsolicitedResponse)(int unsolResponse,
                                  const void *data, size_t data_len);

    /* Request to wake up the application processor */
    void (*RequestTimedCallback)(void (*callback)(void *param),
                                 void *param, uint64_t delay_ms);
} RIL_RadioFunctions_Callbacks;

/*
 * Vendor RIL function table — populated by dlsym() after loading
 * the vendor .so library.
 */
typedef struct {
    /* RIL version reported by vendor library */
    int version;

    /* Called by RILD to send a request to the modem */
    void (*onRequest)(int request, const void *data, size_t data_len,
                      RIL_Token token);

    /* Called by RILD to query current radio state */
    int (*onStateRequest)(void);

    /* Called by RILD to check if vendor supports a request */
    int (*supports)(int requestCode);

    /* Called by RILD to cancel a pending request */
    void (*onCancel)(RIL_Token token);

    /* Get vendor RIL version string */
    const char *(*getVersion)(void);
} RIL_RadioFunctions;

/*
 * Vendor RIL init function signature.
 * The vendor .so exports this as "RIL_Init".
 */
typedef const RIL_RadioFunctions *(*RIL_InitFunc)(
    const RIL_RadioFunctions_Callbacks *callbacks,
    int argc,
    char **argv
);

/* ===== Vendor RIL Loader API ===== */

/*
 * Known vendor RIL library paths per device.
 * The loader tries these in order.
 */
#define VENDOR_RIL_PATH_PIXEL7    "/vendor/lib64/libril-samsung-e5300.so"
#define VENDOR_RIL_PATH_PIXEL7_ALT "/vendor/lib64/libril-exynos.so"
#define VENDOR_RIL_PATH_NORDN30   "/vendor/lib64/libril-qc-qmi-1.so"
#define VENDOR_RIL_PATH_NORDN30_ALT "/vendor/lib64/libril-qc-hal-qmi.so"
#define VENDOR_RIL_PATH_GENERIC   "/vendor/lib64/libril-vendor.so"

/*
 * Load the vendor RIL library.
 *
 * Probes the device to determine which vendor .so to load,
 * opens it via dlopen(), resolves RIL_Init, and calls it
 * with our callbacks.
 *
 * Returns 0 on success, negative errno on failure.
 */
int vendor_ril_load(const RIL_RadioFunctions_Callbacks *callbacks);

/*
 * Unload the vendor RIL library.
 */
void vendor_ril_unload(void);

/*
 * Get the loaded vendor RIL function table.
 * Returns NULL if not yet loaded.
 */
const RIL_RadioFunctions *vendor_ril_get_functions(void);

/*
 * Send a request to the vendor RIL.
 */
int vendor_ril_request(int request, const void *data, size_t data_len,
                       RIL_Token token);

/*
 * Query vendor RIL radio state.
 */
int vendor_ril_get_state(void);

/*
 * Get the vendor RIL version string.
 */
const char *vendor_ril_get_version(void);

#ifdef __cplusplus
}
#endif

#endif /* NINJAMAGIC_VENDOR_RIL_H */
