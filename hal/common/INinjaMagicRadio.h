/*
 * INinjaMagicRadio.h — Unified Radio HAL Interface
 *
 * Abstract interface for telephony/modem operations.
 * Implemented per-SoC in hal/tensor/radio/ and hal/snapdragon/radio/.
 * Used by the NinjaMagic RILD to abstract vendor differences.
 */

#ifndef ININJAMAGIC_RADIO_H
#define ININJAMAGIC_RADIO_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Call state enumeration */
typedef enum {
    CALL_STATE_IDLE = 0,
    CALL_STATE_RINGING,
    CALL_STATE_DIALING,
    CALL_STATE_ACTIVE,
    CALL_STATE_HOLDING,
    CALL_STATE_ENDED,
} ninjamagic_call_state_t;

/* Network type enumeration */
typedef enum {
    NETWORK_TYPE_UNKNOWN = 0,
    NETWORK_TYPE_GSM,
    NETWORK_TYPE_UMTS,
    NETWORK_TYPE_LTE,
    NETWORK_TYPE_NR5G,
} ninjamagic_network_type_t;

/* Signal strength */
typedef struct {
    int rssi;    /* Received Signal Strength Indicator (dBm) */
    int rsrp;    /* Reference Signal Received Power (dBm) */
    int rsrq;    /* Reference Signal Received Quality (dB) */
    int snr;     /* Signal-to-Noise Ratio (dB) */
} ninjamagic_signal_t;

/* SIM state */
typedef enum {
    SIM_STATE_ABSENT = 0,
    SIM_STATE_PRESENT,
    SIM_STATE_READY,
    SIM_STATE_LOCKED,
    SIM_STATE_ERROR,
} ninjamagic_sim_state_t;

/* Callback types for unsolicited events */
typedef void (*on_call_state_cb)(int call_id, ninjamagic_call_state_t state,
                                  const char *number, const char *name);
typedef void (*on_sms_received_cb)(const char *from, const char *body,
                                    uint64_t timestamp);
typedef void (*on_signal_cb)(const ninjamagic_signal_t *signal);
typedef void (*on_network_cb)(const char *operator_name,
                               ninjamagic_network_type_t type, bool roaming);
typedef void (*on_data_cb)(bool connected, const char *apn, const char *ip);
typedef void (*on_sim_state_cb)(int slot, ninjamagic_sim_state_t state);

/* Callbacks structure */
typedef struct {
    on_call_state_cb   on_call_state;
    on_sms_received_cb on_sms_received;
    on_signal_cb       on_signal;
    on_network_cb      on_network;
    on_data_cb         on_data;
    on_sim_state_cb    on_sim_state;
} ninjamagic_radio_callbacks_t;

/*
 * Radio HAL interface — implemented per-SoC.
 *
 * Pixel 7:  hal/tensor/radio/tensor_radio.cpp
 * Nord N30: hal/snapdragon/radio/snapdragon_radio.cpp
 */

/* Initialize the radio HAL and load vendor RIL library */
int ninjamagic_radio_init(const ninjamagic_radio_callbacks_t *callbacks);

/* Shutdown the radio HAL */
void ninjamagic_radio_shutdown(void);

/* Telephony operations */
int ninjamagic_radio_dial(const char *number, int sim_slot);
int ninjamagic_radio_answer(int call_id);
int ninjamagic_radio_hangup(int call_id);
int ninjamagic_radio_hold(int call_id);
int ninjamagic_radio_resume(int call_id);

/* SMS operations */
int ninjamagic_radio_send_sms(const char *to, const char *body, int sim_slot);

/* Data operations */
int ninjamagic_radio_setup_data(const char *apn);
int ninjamagic_radio_deactivate_data(void);

/* Network operations */
int ninjamagic_radio_get_signal(ninjamagic_signal_t *out);
int ninjamagic_radio_get_operator(char *out, size_t out_len);
ninjamagic_network_type_t ninjamagic_radio_get_network_type(void);

/* SIM operations */
ninjamagic_sim_state_t ninjamagic_radio_get_sim_state(int slot);

/* Modem power */
int ninjamagic_radio_set_modem_power(bool on);

#ifdef __cplusplus
}
#endif

#endif /* ININJAMAGIC_RADIO_H */
