/*
 * NinjaMagic Privacy Guard
 *
 * Per-domain network traffic monitoring, encrypted MSI state regions,
 * and TEE-backed key storage. Ensures no unintended data exfiltration
 * and all sensitive state is encrypted at rest.
 *
 * Components:
 * 1. Network Monitor — per-domain traffic accounting and firewall
 * 2. Encrypted State — AES-256-GCM encryption for MSI state regions
 * 3. Key Storage — keys stored in hardware TEE (Titan M2 / Qualcomm SPU)
 * 4. Data Audit — track what data each domain has accessed
 */

#ifndef NINJAMAGIC_PRIVACY_GUARD_H
#define NINJAMAGIC_PRIVACY_GUARD_H

#include <linux/types.h>
#include <linux/spinlock.h>

/* ===== Network Monitor ===== */

typedef enum {
    NET_POLICY_ALLOW,          /* Full network access */
    NET_POLICY_LOCAL_ONLY,     /* localhost / LAN only */
    NET_POLICY_BLOCKED,        /* No network access at all */
    NET_POLICY_ALLOWLIST,      /* Only allowed domains/IPs */
} NetPolicy;

typedef struct {
    uint32_t    domain_id;
    NetPolicy   policy;
    uint64_t    bytes_sent;
    uint64_t    bytes_received;
    uint64_t    packets_sent;
    uint64_t    packets_received;
    uint64_t    packets_blocked;
    uint64_t    last_activity_ns;
    /* Allowlist for ALLOWLIST policy */
    char        allowed_hosts[16][256];
    uint32_t    allowed_host_count;
} DomainNetStats;

#define PRIVACY_MAX_DOMAINS     256

/* ===== Encrypted State ===== */

typedef enum {
    CRYPTO_ALG_AES_256_GCM,
    CRYPTO_ALG_CHACHA20_POLY1305,
} CryptoAlgorithm;

typedef struct {
    uint32_t            domain_id;
    uint32_t            state_id;
    CryptoAlgorithm     algorithm;
    uint8_t             key_handle[32];  /* Opaque handle to TEE-stored key */
    uint8_t             iv[12];          /* Initialization vector */
    uint8_t             tag[16];         /* Authentication tag */
    bool                is_encrypted;
    uint64_t            last_encrypt_ns;
    uint64_t            last_decrypt_ns;
} EncryptedStateInfo;

/* ===== TEE Key Storage ===== */

typedef enum {
    KEY_PURPOSE_STATE_ENCRYPTION,
    KEY_PURPOSE_ASSOC_ENCRYPTION,
    KEY_PURPOSE_ATTESTATION,
    KEY_PURPOSE_USER_AUTH,
    KEY_PURPOSE_AGENT_MEMORY,
} KeyPurpose;

typedef struct {
    uint8_t         handle[32];
    KeyPurpose      purpose;
    uint32_t        domain_id;
    uint32_t        key_size_bits;
    CryptoAlgorithm algorithm;
    bool            hardware_bound;      /* Key cannot leave TEE */
    bool            auth_required;       /* Requires user auth to use */
    uint64_t        created_ns;
    uint64_t        last_used_ns;
    uint32_t        use_count;
} TeeKeyInfo;

/* ===== Data Access Audit ===== */

typedef enum {
    DATA_TYPE_LOCATION,
    DATA_TYPE_CONTACTS,
    DATA_TYPE_CALL_LOG,
    DATA_TYPE_SMS,
    DATA_TYPE_CAMERA,
    DATA_TYPE_MICROPHONE,
    DATA_TYPE_SENSORS,
    DATA_TYPE_STORAGE,
    DATA_TYPE_NETWORK,
    DATA_TYPE_CLIPBOARD,
} SensitiveDataType;

typedef struct {
    uint32_t            domain_id;
    SensitiveDataType   data_type;
    uint64_t            access_count;
    uint64_t            last_access_ns;
    bool                user_consented;
} DataAccessRecord;

/* ===== API ===== */

/** Initialize privacy guard subsystem. */
int privacy_guard_init(void);

/** Shutdown privacy guard. */
void privacy_guard_exit(void);

/* --- Network Monitor --- */

/** Set network policy for a domain. */
int privacy_net_set_policy(uint32_t domain_id, NetPolicy policy);

/** Add a host to domain's allowlist (for ALLOWLIST policy). */
int privacy_net_add_allowed_host(uint32_t domain_id, const char *host);

/** Get network stats for a domain. */
int privacy_net_get_stats(uint32_t domain_id, DomainNetStats *out);

/** Account outgoing packet for a domain. Returns 0 if allowed, -EPERM if blocked. */
int privacy_net_account_tx(uint32_t domain_id, const char *dest_host,
                            uint32_t bytes);

/** Account incoming packet for a domain. */
int privacy_net_account_rx(uint32_t domain_id, uint32_t bytes);

/** Get total network stats across all domains. */
void privacy_net_get_totals(uint64_t *total_sent, uint64_t *total_recv,
                             uint64_t *total_blocked);

/* --- Encrypted State --- */

/** Encrypt an MSI state region in-place. Key is stored in TEE. */
int privacy_encrypt_state(uint32_t domain_id, uint32_t state_id,
                           void *data, size_t data_len);

/** Decrypt an MSI state region in-place. */
int privacy_decrypt_state(uint32_t domain_id, uint32_t state_id,
                           void *data, size_t data_len);

/** Check if a state region is encrypted. */
bool privacy_state_is_encrypted(uint32_t domain_id, uint32_t state_id);

/* --- TEE Key Storage --- */

/** Generate a new key in the TEE. Returns key handle. */
int privacy_tee_generate_key(KeyPurpose purpose, uint32_t domain_id,
                              CryptoAlgorithm alg, uint32_t key_size_bits,
                              bool auth_required, uint8_t handle_out[32]);

/** Delete a key from the TEE. */
int privacy_tee_delete_key(const uint8_t handle[32]);

/** Get info about a TEE-stored key. */
int privacy_tee_key_info(const uint8_t handle[32], TeeKeyInfo *out);

/** Encrypt data using a TEE key (key never leaves hardware). */
int privacy_tee_encrypt(const uint8_t key_handle[32],
                         const void *plaintext, size_t plain_len,
                         void *ciphertext, size_t *cipher_len,
                         uint8_t iv_out[12], uint8_t tag_out[16]);

/** Decrypt data using a TEE key. */
int privacy_tee_decrypt(const uint8_t key_handle[32],
                         const void *ciphertext, size_t cipher_len,
                         const uint8_t iv[12], const uint8_t tag[16],
                         void *plaintext, size_t *plain_len);

/* --- Data Access Audit --- */

/** Record a sensitive data access by a domain. */
int privacy_record_data_access(uint32_t domain_id, SensitiveDataType type);

/** Get data access records for a domain. */
int privacy_get_data_access(uint32_t domain_id, DataAccessRecord *out,
                             int max_records);

/** Check if user has consented to a domain accessing a data type. */
bool privacy_check_consent(uint32_t domain_id, SensitiveDataType type);

/** Set user consent for a domain/data type pair. */
int privacy_set_consent(uint32_t domain_id, SensitiveDataType type,
                         bool consented);

#endif /* NINJAMAGIC_PRIVACY_GUARD_H */
