/*
 * msi_bridge.cpp — RIL Callback → MSI Event Bridge Implementation
 *
 * Bridges the legacy Android RIL callback interface to the MSI
 * event bus, enabling the NinjaMagic Agent to interact with
 * telephony via pub/sub events instead of direct RIL calls.
 */

#include "msi_bridge.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <errno.h>

#define LOG_TAG "NinjaMagicRIL-MSIBridge"
#ifdef __ANDROID__
#include <android/log.h>
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#else
#define LOGI(...) do { printf("[MSI-Bridge] "); printf(__VA_ARGS__); printf("\n"); } while(0)
#define LOGE(...) do { fprintf(stderr, "[MSI-Bridge ERROR] "); fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n"); } while(0)
#endif

/* ===== MSI ioctl definitions (mirror from kernel module) ===== */

#define MSI_IOC_MAGIC 'M'

struct msi_ioctl_domain_create {
    char     name[128];
    uint32_t num_grants;
    uint32_t seal;
    uint32_t domain_id;
};

struct msi_grant_raw {
    uint32_t kind;
    uint8_t  data[256];
};

struct msi_ioctl_domain_grant_args {
    uint32_t         domain_id;
    struct msi_grant_raw grant;
};

struct msi_ioctl_event_publish_args {
    uint32_t domain_id;
    char     topic[256];
    uint64_t payload_ptr;
    uint32_t payload_len;
    uint32_t qos;
    uint64_t event_id;
};

struct msi_ioctl_event_subscribe_args {
    uint32_t domain_id;
    char     prefix[256];
    char     filter[256];
    uint32_t sub_id;
};

struct msi_ioctl_event_wait_args {
    uint32_t sub_id;
    uint64_t timeout_nanos;
    uint64_t event_id;
    char     topic[256];
    uint64_t ts_nanos;
    uint32_t payload_len;
    uint64_t payload_ptr;
};

#define MSI_IOC_DOMAIN_CREATE   _IOWR(MSI_IOC_MAGIC, 10, struct msi_ioctl_domain_create)
#define MSI_IOC_DOMAIN_GRANT    _IOW(MSI_IOC_MAGIC, 11, struct msi_ioctl_domain_grant_args)
#define MSI_IOC_DOMAIN_SEAL     _IOW(MSI_IOC_MAGIC, 12, uint32_t)
#define MSI_IOC_EVENT_PUBLISH   _IOW(MSI_IOC_MAGIC, 30, struct msi_ioctl_event_publish_args)
#define MSI_IOC_EVENT_SUBSCRIBE _IOWR(MSI_IOC_MAGIC, 31, struct msi_ioctl_event_subscribe_args)
#define MSI_IOC_EVENT_WAIT      _IOWR(MSI_IOC_MAGIC, 32, struct msi_ioctl_event_wait_args)

/* ===== Global state ===== */

int msi_fd = -1;
uint32_t telephony_domain_id = 0;

static pthread_t listener_thread;
static volatile bool listener_running = false;
static uint32_t agent_cmd_sub_id = 0;

/* Agent command callbacks */
static msi_bridge_dial_cb     cb_dial = NULL;
static msi_bridge_answer_cb   cb_answer = NULL;
static msi_bridge_hangup_cb   cb_hangup = NULL;
static msi_bridge_sms_send_cb cb_sms_send = NULL;

/* ===== Helpers ===== */

static void make_event_grant(struct msi_grant_raw *g, const char *prefix) {
    memset(g, 0, sizeof(*g));
    g->kind = 0; /* MSI_GRANT_EVENTS */
    strncpy((char *)g->data, prefix, 255);
}

static int publish_event(const char *topic, const char *json_payload) {
    if (msi_fd < 0 || telephony_domain_id == 0)
        return -ENODEV;

    struct msi_ioctl_event_publish_args args;
    memset(&args, 0, sizeof(args));
    args.domain_id = telephony_domain_id;
    strncpy(args.topic, topic, sizeof(args.topic) - 1);
    args.payload_ptr = (uint64_t)(uintptr_t)json_payload;
    args.payload_len = json_payload ? (uint32_t)strlen(json_payload) : 0;
    args.qos = 0; /* best-effort */

    if (ioctl(msi_fd, MSI_IOC_EVENT_PUBLISH, &args) < 0) {
        LOGE("publish failed for topic '%s': %s", topic, strerror(errno));
        return -errno;
    }

    LOGI("published: %s (id=%llu)", topic, (unsigned long long)args.event_id);
    return 0;
}

/* ===== Init / Shutdown ===== */

int msi_bridge_init(void) {
    LOGI("initializing MSI bridge");

    /* Open /dev/msi */
    msi_fd = open("/dev/msi", O_RDWR);
    if (msi_fd < 0) {
        LOGE("cannot open /dev/msi: %s", strerror(errno));
        return -errno;
    }

    /* Create telephony domain */
    struct msi_ioctl_domain_create dc;
    memset(&dc, 0, sizeof(dc));
    strncpy(dc.name, "Telephony", sizeof(dc.name) - 1);
    dc.seal = 0; /* Add grants first */

    if (ioctl(msi_fd, MSI_IOC_DOMAIN_CREATE, &dc) < 0) {
        LOGE("domain_create failed: %s", strerror(errno));
        close(msi_fd);
        msi_fd = -1;
        return -errno;
    }
    telephony_domain_id = dc.domain_id;
    LOGI("telephony domain created: id=%u", telephony_domain_id);

    /* Add grants for phone/* events */
    const char *prefixes[] = {
        "phone/call/", "phone/sms/", "phone/data/",
        "phone/signal/", "phone/network/", "phone/sim/"
    };
    for (int i = 0; i < 6; i++) {
        struct msi_ioctl_domain_grant_args ga;
        memset(&ga, 0, sizeof(ga));
        ga.domain_id = telephony_domain_id;
        make_event_grant(&ga.grant, prefixes[i]);
        if (ioctl(msi_fd, MSI_IOC_DOMAIN_GRANT, &ga) < 0) {
            LOGE("grant failed for '%s': %s", prefixes[i], strerror(errno));
        }
    }

    /* Seal the domain */
    if (ioctl(msi_fd, MSI_IOC_DOMAIN_SEAL, &telephony_domain_id) < 0) {
        LOGE("domain_seal failed: %s", strerror(errno));
    }

    LOGI("MSI bridge initialized — domain=%u sealed", telephony_domain_id);
    return 0;
}

void msi_bridge_shutdown(void) {
    listener_running = false;
    if (listener_thread) {
        pthread_join(listener_thread, NULL);
        listener_thread = 0;
    }
    if (msi_fd >= 0) {
        close(msi_fd);
        msi_fd = -1;
    }
    LOGI("MSI bridge shutdown");
}

/* ===== Outbound: RIL → MSI Events ===== */

void msi_bridge_on_incoming_call(const char *number, const char *name,
                                  int sim_slot) {
    char buf[512];
    snprintf(buf, sizeof(buf),
             "{\"number\":\"%s\",\"name\":\"%s\",\"slot\":%d}",
             number ? number : "", name ? name : "", sim_slot);
    publish_event("phone/call/incoming", buf);
}

void msi_bridge_on_call_active(int call_id, const char *number) {
    char buf[256];
    snprintf(buf, sizeof(buf),
             "{\"call_id\":%d,\"number\":\"%s\"}",
             call_id, number ? number : "");
    publish_event("phone/call/active", buf);
}

void msi_bridge_on_call_ended(int call_id, int reason, int duration_secs) {
    char buf[256];
    snprintf(buf, sizeof(buf),
             "{\"call_id\":%d,\"reason\":%d,\"duration\":%d}",
             call_id, reason, duration_secs);
    publish_event("phone/call/ended", buf);
}

void msi_bridge_on_sms_received(const char *from, const char *body,
                                 uint64_t timestamp) {
    char buf[2048];
    snprintf(buf, sizeof(buf),
             "{\"from\":\"%s\",\"body\":\"%s\",\"timestamp\":%llu}",
             from ? from : "", body ? body : "",
             (unsigned long long)timestamp);
    publish_event("phone/sms/received", buf);
}

void msi_bridge_on_sms_sent(const char *to, int status) {
    char buf[256];
    snprintf(buf, sizeof(buf),
             "{\"to\":\"%s\",\"status\":%d}",
             to ? to : "", status);
    publish_event("phone/sms/sent", buf);
}

void msi_bridge_on_data_connected(const char *type, const char *apn,
                                   const char *ip_addr) {
    char buf[512];
    snprintf(buf, sizeof(buf),
             "{\"type\":\"%s\",\"apn\":\"%s\",\"ip\":\"%s\"}",
             type ? type : "", apn ? apn : "", ip_addr ? ip_addr : "");
    publish_event("phone/data/connected", buf);
}

void msi_bridge_on_data_disconnected(int reason) {
    char buf[64];
    snprintf(buf, sizeof(buf), "{\"reason\":%d}", reason);
    publish_event("phone/data/disconnected", buf);
}

void msi_bridge_on_signal_strength(int rssi, int rsrp, int rsrq, int snr) {
    char buf[128];
    snprintf(buf, sizeof(buf),
             "{\"rssi\":%d,\"rsrp\":%d,\"rsrq\":%d,\"snr\":%d}",
             rssi, rsrp, rsrq, snr);
    publish_event("phone/signal/strength", buf);
}

void msi_bridge_on_network_registered(const char *operator_name,
                                       const char *network_type,
                                       bool roaming) {
    char buf[256];
    snprintf(buf, sizeof(buf),
             "{\"operator\":\"%s\",\"type\":\"%s\",\"roaming\":%s}",
             operator_name ? operator_name : "",
             network_type ? network_type : "",
             roaming ? "true" : "false");
    publish_event("phone/network/registered", buf);
}

void msi_bridge_on_sim_state(int slot, const char *state, const char *iccid) {
    char buf[256];
    snprintf(buf, sizeof(buf),
             "{\"slot\":%d,\"state\":\"%s\",\"iccid\":\"%s\"}",
             slot, state ? state : "", iccid ? iccid : "");
    publish_event("phone/sim/state", buf);
}

/* ===== Inbound: MSI Events → RIL Requests ===== */

void msi_bridge_register_dial(msi_bridge_dial_cb cb) { cb_dial = cb; }
void msi_bridge_register_answer(msi_bridge_answer_cb cb) { cb_answer = cb; }
void msi_bridge_register_hangup(msi_bridge_hangup_cb cb) { cb_hangup = cb; }
void msi_bridge_register_sms_send(msi_bridge_sms_send_cb cb) { cb_sms_send = cb; }

static void *listener_thread_fn(void *arg) {
    (void)arg;
    char payload_buf[4096];

    LOGI("agent command listener started (sub_id=%u)", agent_cmd_sub_id);

    while (listener_running) {
        struct msi_ioctl_event_wait_args wa;
        memset(&wa, 0, sizeof(wa));
        wa.sub_id = agent_cmd_sub_id;
        wa.timeout_nanos = 1000000000ULL; /* 1 second timeout */
        wa.payload_ptr = (uint64_t)(uintptr_t)payload_buf;
        wa.payload_len = sizeof(payload_buf) - 1;

        int ret = ioctl(msi_fd, MSI_IOC_EVENT_WAIT, &wa);
        if (ret < 0) {
            if (errno == ETIMEDOUT || errno == EAGAIN || errno == EINTR)
                continue;
            LOGE("event_wait failed: %s", strerror(errno));
            break;
        }

        payload_buf[wa.payload_len] = '\0';

        LOGI("agent command: topic='%s' payload='%s'", wa.topic, payload_buf);

        /* Dispatch based on topic */
        if (strncmp(wa.topic, "phone/call/dial", 15) == 0 && cb_dial) {
            /* Parse number from JSON payload */
            /* Simple extraction — production would use proper JSON parser */
            char *num_start = strstr(payload_buf, "\"number\":\"");
            if (num_start) {
                num_start += 10;
                char *num_end = strchr(num_start, '"');
                if (num_end) {
                    *num_end = '\0';
                    cb_dial(num_start);
                }
            }
        } else if (strncmp(wa.topic, "phone/call/answer", 17) == 0 && cb_answer) {
            char *id_start = strstr(payload_buf, "\"call_id\":");
            if (id_start) {
                cb_answer(atoi(id_start + 10));
            }
        } else if (strncmp(wa.topic, "phone/call/hangup", 17) == 0 && cb_hangup) {
            char *id_start = strstr(payload_buf, "\"call_id\":");
            if (id_start) {
                cb_hangup(atoi(id_start + 10));
            }
        } else if (strncmp(wa.topic, "phone/sms/send", 14) == 0 && cb_sms_send) {
            char *to_start = strstr(payload_buf, "\"to\":\"");
            char *body_start = strstr(payload_buf, "\"body\":\"");
            if (to_start && body_start) {
                to_start += 6;
                char *to_end = strchr(to_start, '"');
                body_start += 8;
                char *body_end = strchr(body_start, '"');
                if (to_end && body_end) {
                    *to_end = '\0';
                    *body_end = '\0';
                    cb_sms_send(to_start, body_start);
                }
            }
        }
    }

    LOGI("agent command listener stopped");
    return NULL;
}

int msi_bridge_start_listener(void) {
    /* Subscribe to agent command topics */
    struct msi_ioctl_event_subscribe_args sa;
    memset(&sa, 0, sizeof(sa));
    sa.domain_id = telephony_domain_id;
    strncpy(sa.prefix, "phone/", sizeof(sa.prefix) - 1);

    if (ioctl(msi_fd, MSI_IOC_EVENT_SUBSCRIBE, &sa) < 0) {
        LOGE("subscribe failed for 'phone/': %s", strerror(errno));
        return -errno;
    }
    agent_cmd_sub_id = sa.sub_id;

    listener_running = true;
    if (pthread_create(&listener_thread, NULL, listener_thread_fn, NULL) != 0) {
        LOGE("failed to create listener thread: %s", strerror(errno));
        listener_running = false;
        return -errno;
    }

    return 0;
}
