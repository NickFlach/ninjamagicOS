/*
 * main.cpp — NinjaMagic RILD Entry Point
 *
 * Custom Radio Interface Layer Daemon for ninjamagicOS.
 * Replaces AOSP rild with a thin daemon that:
 * 1. Loads the vendor RIL .so (Exynos or Qualcomm)
 * 2. Initializes the MSI event bridge
 * 3. Routes modem callbacks → MSI events
 * 4. Routes MSI agent commands → modem requests
 *
 * Started by init.ninjamagic.rc as service "ninjamagic-rild".
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>

#include "vendor_ril.h"
#include "msi_bridge.h"

#define LOG_TAG "NinjaMagicRILD"
#ifdef __ANDROID__
#include <android/log.h>
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#else
#define LOGI(...) do { printf("[RILD] "); printf(__VA_ARGS__); printf("\n"); } while(0)
#define LOGE(...) do { fprintf(stderr, "[RILD ERROR] "); fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n"); } while(0)
#define LOGW(...) do { fprintf(stderr, "[RILD WARN] "); fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n"); } while(0)
#endif

/* ===== Global state ===== */

static volatile bool s_running = true;
static pthread_mutex_t s_token_mutex = PTHREAD_MUTEX_INITIALIZER;
static RIL_Token s_next_token = 1;

/* ===== Token management ===== */

static RIL_Token alloc_token(void) {
    pthread_mutex_lock(&s_token_mutex);
    RIL_Token t = s_next_token++;
    pthread_mutex_unlock(&s_token_mutex);
    return t;
}

/* ===== Vendor RIL callbacks ===== */

/*
 * Called by vendor RIL when a solicited request completes.
 * We translate the response and publish it as an MSI event.
 */
static void on_request_complete(RIL_Token token, int error,
                                const void *response, size_t response_len) {
    if (error != RIL_E_SUCCESS) {
        LOGW("Request token=%u completed with error=%d", token, error);
        return;
    }

    LOGI("Request token=%u completed successfully (response_len=%zu)",
         token, response_len);

    /* Response routing is handled per-request-type in the command
     * dispatch functions below. Solicited responses that need to
     * reach the agent are published there after interpreting the
     * vendor-specific response format. */
}

/*
 * Called by vendor RIL for unsolicited modem events.
 * This is the main pathway for modem → agent communication.
 */
static void on_unsolicited_response(int unsolResponse,
                                     const void *data, size_t data_len) {
    LOGI("Unsolicited response: %d (data_len=%zu)", unsolResponse, data_len);

    switch (unsolResponse) {

    case RIL_UNSOL_RESPONSE_CALL_STATE_CHANGED: {
        /* Query current call list to get full state */
        LOGI("Call state changed — querying call list");
        /* In production: issue RIL_REQUEST_GET_CURRENT_CALLS and
         * parse the response to determine incoming/active/ended,
         * then call the appropriate msi_bridge_on_* function. */

        /* Simplified: treat as incoming call notification */
        msi_bridge_on_incoming_call("Unknown", "", 0);
        break;
    }

    case RIL_UNSOL_RESPONSE_NEW_SMS: {
        /* data is a C string containing the SMS PDU */
        if (data && data_len > 0) {
            /* In production: decode SMS PDU to extract sender and body */
            msi_bridge_on_sms_received("Unknown", (const char *)data, 0);
        }
        break;
    }

    case RIL_UNSOL_SIGNAL_STRENGTH: {
        /* data contains signal strength values */
        /* Simplified: publish with placeholder values */
        msi_bridge_on_signal_strength(-80, -100, -10, 15);
        break;
    }

    case RIL_UNSOL_DATA_CALL_LIST_CHANGED: {
        /* Data connection state changed */
        msi_bridge_on_data_connected("LTE", "internet", "0.0.0.0");
        break;
    }

    case RIL_UNSOL_RESPONSE_NETWORK_STATE_CHANGED: {
        /* Network registration changed — query for details */
        msi_bridge_on_network_registered("", "LTE", false);
        break;
    }

    case RIL_UNSOL_RESPONSE_SIM_STATUS_CHANGED: {
        msi_bridge_on_sim_state(0, "READY", "");
        break;
    }

    default:
        LOGI("Unhandled unsolicited response: %d", unsolResponse);
        break;
    }
}

/*
 * Called by vendor RIL to schedule a timed callback.
 */
static void request_timed_callback(void (*callback)(void *param),
                                    void *param, uint64_t delay_ms) {
    /* Simple implementation: spawn a thread that sleeps then calls */
    struct timed_cb_data {
        void (*callback)(void *);
        void *param;
        uint64_t delay_ms;
    };

    auto *cbd = (struct timed_cb_data *)malloc(sizeof(struct timed_cb_data));
    if (!cbd) return;
    cbd->callback = callback;
    cbd->param = param;
    cbd->delay_ms = delay_ms;

    pthread_t thread;
    pthread_create(&thread, NULL, [](void *arg) -> void * {
        auto *cbd = (struct timed_cb_data *)arg;
        usleep(cbd->delay_ms * 1000);
        if (cbd->callback) cbd->callback(cbd->param);
        free(cbd);
        return NULL;
    }, cbd);
    pthread_detach(thread);
}

/* ===== Agent command handlers ===== */

/*
 * Agent requests a phone call via MSI event.
 * Translates to RIL_REQUEST_DIAL.
 */
static void handle_agent_dial(const char *number) {
    LOGI("Agent dial: %s", number);

    const RIL_RadioFunctions *funcs = vendor_ril_get_functions();
    if (!funcs) {
        LOGE("Cannot dial — vendor RIL not loaded");
        return;
    }

    /* Construct RIL dial data.
     * In production: use proper RIL_Dial struct from ril.h.
     * Simplified here for the skeleton. */
    struct {
        const char *address;
        int clir;      /* 0 = default, 1 = invocation, 2 = suppression */
    } dial_data = { number, 0 };

    vendor_ril_request(RIL_REQUEST_DIAL, &dial_data, sizeof(dial_data),
                       alloc_token());
}

/*
 * Agent answers an incoming call.
 */
static void handle_agent_answer(int call_id) {
    LOGI("Agent answer: call_id=%d", call_id);
    vendor_ril_request(RIL_REQUEST_ANSWER, NULL, 0, alloc_token());
}

/*
 * Agent hangs up a call.
 */
static void handle_agent_hangup(int call_id) {
    LOGI("Agent hangup: call_id=%d", call_id);
    int hangup_data = call_id;
    vendor_ril_request(RIL_REQUEST_HANGUP, &hangup_data, sizeof(hangup_data),
                       alloc_token());
}

/*
 * Agent sends an SMS.
 */
static void handle_agent_sms_send(const char *to, const char *body) {
    LOGI("Agent SMS: to=%s body_len=%zu", to, strlen(body));

    /* In production: encode body as SMS PDU, use RIL_REQUEST_SEND_SMS.
     * Simplified here. */
    const char *sms_data[2] = { NULL, body }; /* [SMSC address, PDU] */
    vendor_ril_request(RIL_REQUEST_SEND_SMS, sms_data, sizeof(sms_data),
                       alloc_token());
}

/* ===== Signal handling ===== */

static void signal_handler(int sig) {
    LOGI("Received signal %d — shutting down", sig);
    s_running = false;
}

/* ===== Main ===== */

int main(int argc, char **argv) {
    LOGI("=== NinjaMagic RILD v0.1.0 starting ===");

    /* Install signal handlers */
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    /* Phase 1: Initialize MSI event bridge */
    LOGI("Phase 1: Initializing MSI event bridge");
    int ret = msi_bridge_init();
    if (ret < 0) {
        LOGE("MSI bridge init failed: %d — continuing without MSI", ret);
        /* Don't abort — RIL can still function without MSI for basic testing */
    } else {
        /* Register agent command handlers */
        msi_bridge_register_dial(handle_agent_dial);
        msi_bridge_register_answer(handle_agent_answer);
        msi_bridge_register_hangup(handle_agent_hangup);
        msi_bridge_register_sms_send(handle_agent_sms_send);
    }

    /* Phase 2: Load vendor RIL library */
    LOGI("Phase 2: Loading vendor RIL");
    RIL_RadioFunctions_Callbacks callbacks = {
        .OnRequestComplete = on_request_complete,
        .OnUnsolicitedResponse = on_unsolicited_response,
        .RequestTimedCallback = request_timed_callback,
    };

    ret = vendor_ril_load(&callbacks);
    if (ret < 0) {
        LOGE("Failed to load vendor RIL: %d", ret);
        LOGE("Telephony will not be available");
        /* Don't exit — allow agent to function without telephony */
    } else {
        LOGI("Vendor RIL loaded: %s (state=%d)",
             vendor_ril_get_version(), vendor_ril_get_state());
    }

    /* Phase 3: Start MSI agent command listener */
    if (msi_fd >= 0) {
        LOGI("Phase 3: Starting agent command listener");
        ret = msi_bridge_start_listener();
        if (ret < 0) {
            LOGW("Agent command listener failed to start: %d", ret);
        }
    }

    /* Phase 4: Publish RILD ready event */
    LOGI("Phase 4: RILD ready");
    /* The MSI bridge will publish phone/sim/state when SIM is detected */

    /* Main loop — keep the daemon alive */
    LOGI("=== NinjaMagic RILD running ===");
    while (s_running) {
        sleep(1);
    }

    /* Cleanup */
    LOGI("Shutting down...");
    msi_bridge_shutdown();
    vendor_ril_unload();
    LOGI("=== NinjaMagic RILD stopped ===");

    return 0;
}
