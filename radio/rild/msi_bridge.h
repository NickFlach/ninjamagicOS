/*
 * msi_bridge.h — RIL Callback → MSI Event Bridge
 *
 * Translates vendor RIL unsolicited responses and solicited
 * callbacks into MSI event bus publications. Also listens for
 * MSI events from the agent (e.g., phone/call/dial) and
 * dispatches them as RIL requests to the vendor library.
 *
 * This is the integration seam between legacy Android telephony
 * and ninjamagicOS's cognitive event architecture.
 */

#ifndef NINJAMAGIC_MSI_BRIDGE_H
#define NINJAMAGIC_MSI_BRIDGE_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* MSI device file descriptor (opened by RILD main) */
extern int msi_fd;

/* Domain ID for the telephony domain */
extern uint32_t telephony_domain_id;

/*
 * Initialize the MSI bridge.
 * Opens /dev/msi, creates the telephony domain with appropriate
 * grants, and starts the agent-command listener lane.
 *
 * Returns 0 on success, negative errno on failure.
 */
int msi_bridge_init(void);

/*
 * Shutdown the MSI bridge.
 * Kills listener lanes and closes /dev/msi.
 */
void msi_bridge_shutdown(void);

/* ===== Outbound: RIL → MSI Events ===== */

/* Call state changes */
void msi_bridge_on_incoming_call(const char *number, const char *name,
                                  int sim_slot);
void msi_bridge_on_call_active(int call_id, const char *number);
void msi_bridge_on_call_ended(int call_id, int reason, int duration_secs);

/* SMS */
void msi_bridge_on_sms_received(const char *from, const char *body,
                                 uint64_t timestamp);
void msi_bridge_on_sms_sent(const char *to, int status);

/* Data connectivity */
void msi_bridge_on_data_connected(const char *type, const char *apn,
                                   const char *ip_addr);
void msi_bridge_on_data_disconnected(int reason);

/* Signal & network */
void msi_bridge_on_signal_strength(int rssi, int rsrp, int rsrq, int snr);
void msi_bridge_on_network_registered(const char *operator_name,
                                       const char *network_type,
                                       bool roaming);

/* SIM state */
void msi_bridge_on_sim_state(int slot, const char *state, const char *iccid);

/* ===== Inbound: MSI Events → RIL Requests ===== */

/*
 * Callback type for agent commands received via MSI events.
 * The bridge calls these when it receives events on phone/call/dial,
 * phone/call/answer, phone/call/hangup, phone/sms/send, etc.
 */
typedef void (*msi_bridge_dial_cb)(const char *number);
typedef void (*msi_bridge_answer_cb)(int call_id);
typedef void (*msi_bridge_hangup_cb)(int call_id);
typedef void (*msi_bridge_sms_send_cb)(const char *to, const char *body);

/*
 * Register callbacks for agent-initiated telephony actions.
 */
void msi_bridge_register_dial(msi_bridge_dial_cb cb);
void msi_bridge_register_answer(msi_bridge_answer_cb cb);
void msi_bridge_register_hangup(msi_bridge_hangup_cb cb);
void msi_bridge_register_sms_send(msi_bridge_sms_send_cb cb);

/*
 * Start the agent command listener thread.
 * Subscribes to phone/call/dial, phone/call/answer, phone/call/hangup,
 * phone/sms/send topics and dispatches to registered callbacks.
 */
int msi_bridge_start_listener(void);

#ifdef __cplusplus
}
#endif

#endif /* NINJAMAGIC_MSI_BRIDGE_H */
