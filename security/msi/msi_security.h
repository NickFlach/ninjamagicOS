/*
 * NinjaMagic MSI Security Module
 *
 * Enforces capability-based security for MSI domains:
 * - Domain sealing: once sealed, grants cannot be modified
 * - Hardware attestation: domain state attested via Titan M2 / Qualcomm SPU
 * - Audit logging: all MSI operations logged for security analysis
 * - Sandboxing: skill domains cannot access resources outside their grants
 *
 * Security model:
 *   Every MSI domain has a set of grants (capabilities).
 *   Once a domain is sealed, it cannot gain new grants.
 *   All cross-domain access is mediated by the kernel module.
 *   Violations are logged and the offending lane is killed.
 */

#ifndef NINJAMAGIC_MSI_SECURITY_H
#define NINJAMAGIC_MSI_SECURITY_H

#include <linux/types.h>
#include <linux/spinlock.h>
#include <linux/list.h>
#include <linux/ktime.h>

/* ===== Audit Event Types ===== */

typedef enum {
    MSI_AUDIT_DOMAIN_CREATE,
    MSI_AUDIT_DOMAIN_GRANT,
    MSI_AUDIT_DOMAIN_SEAL,
    MSI_AUDIT_DOMAIN_DESTROY,
    MSI_AUDIT_LANE_SPAWN,
    MSI_AUDIT_LANE_KILL,
    MSI_AUDIT_EVENT_PUBLISH,
    MSI_AUDIT_EVENT_SUBSCRIBE,
    MSI_AUDIT_STATE_READ,
    MSI_AUDIT_STATE_WRITE,
    MSI_AUDIT_STATE_COMMIT,
    MSI_AUDIT_ASSOC_PUT,
    MSI_AUDIT_ASSOC_GET,
    MSI_AUDIT_ASSOC_QUERY,
    MSI_AUDIT_ACCEL_INVOKE,
    MSI_AUDIT_VIOLATION_GRANT,      /* Attempted access beyond grants */
    MSI_AUDIT_VIOLATION_SEALED,     /* Attempted grant on sealed domain */
    MSI_AUDIT_VIOLATION_CROSS,      /* Cross-domain access attempt */
    MSI_AUDIT_ATTESTATION_REQUEST,
    MSI_AUDIT_ATTESTATION_COMPLETE,
} MsiAuditType;

typedef enum {
    MSI_AUDIT_LEVEL_INFO,
    MSI_AUDIT_LEVEL_WARN,
    MSI_AUDIT_LEVEL_VIOLATION,
    MSI_AUDIT_LEVEL_CRITICAL,
} MsiAuditLevel;

/* ===== Audit Record ===== */

#define MSI_AUDIT_MAX_DETAIL    128
#define MSI_AUDIT_RING_SIZE     4096

typedef struct {
    uint64_t        timestamp_ns;
    uint32_t        domain_id;
    uint32_t        lane_id;
    pid_t           pid;
    uid_t           uid;
    MsiAuditType    type;
    MsiAuditLevel   level;
    int             result;         /* 0 = success, negative = error */
    char            detail[MSI_AUDIT_MAX_DETAIL];
} MsiAuditRecord;

/* ===== Attestation ===== */

#define MSI_ATTEST_NONCE_SIZE   32
#define MSI_ATTEST_SIGNATURE_SIZE 512
#define MSI_ATTEST_CERT_MAX     2048

typedef struct {
    uint32_t        domain_id;
    uint8_t         domain_hash[32];     /* SHA-256 of domain grants */
    uint8_t         nonce[MSI_ATTEST_NONCE_SIZE];
    uint64_t        timestamp_ns;
    uint32_t        grant_count;
    bool            is_sealed;
    /* Hardware-signed attestation blob */
    uint8_t         signature[MSI_ATTEST_SIGNATURE_SIZE];
    uint32_t        signature_len;
    uint8_t         cert_chain[MSI_ATTEST_CERT_MAX];
    uint32_t        cert_chain_len;
} MsiAttestationResult;

/* ===== Grant Enforcement ===== */

typedef enum {
    MSI_GRANT_EVENTS,
    MSI_GRANT_STATE,
    MSI_GRANT_ASSOC,
    MSI_GRANT_CLOCK,
    MSI_GRANT_ACCEL,
} MsiGrantType;

typedef enum {
    MSI_PERM_READ       = 0x01,
    MSI_PERM_WRITE      = 0x02,
    MSI_PERM_READWRITE  = 0x03,
} MsiPermission;

typedef struct {
    MsiGrantType    type;
    MsiPermission   perm;
    char            resource[64];   /* topic prefix, state name, accel type */
} MsiGrant;

#define MSI_MAX_GRANTS_PER_DOMAIN   32

typedef struct {
    uint32_t        domain_id;
    char            name[64];
    MsiGrant        grants[MSI_MAX_GRANTS_PER_DOMAIN];
    uint32_t        grant_count;
    bool            sealed;
    uint64_t        created_ns;
    uint64_t        sealed_ns;
    pid_t           creator_pid;
    uid_t           creator_uid;
    spinlock_t      lock;
} MsiSecureDomain;

/* ===== API ===== */

/**
 * Initialize the MSI security subsystem.
 * Sets up audit ring buffer, attestation interface, grant enforcement.
 */
int msi_security_init(void);

/**
 * Clean up the MSI security subsystem.
 */
void msi_security_exit(void);

/**
 * Check if a domain has a grant for the given operation.
 * Returns 0 if allowed, -EACCES if denied.
 * Logs a violation audit event on denial.
 */
int msi_check_grant(uint32_t domain_id, MsiGrantType type,
                     MsiPermission perm, const char *resource);

/**
 * Seal a domain, preventing further grant modifications.
 * Returns 0 on success.
 */
int msi_seal_domain(uint32_t domain_id);

/**
 * Request hardware attestation of a domain's state.
 * The nonce is provided by the verifier to prevent replay.
 * Returns 0 on success, fills result with signed attestation.
 */
int msi_attest_domain(uint32_t domain_id,
                       const uint8_t nonce[MSI_ATTEST_NONCE_SIZE],
                       MsiAttestationResult *result);

/**
 * Log an audit event.
 */
void msi_audit_log(MsiAuditType type, MsiAuditLevel level,
                    uint32_t domain_id, uint32_t lane_id,
                    int result, const char *fmt, ...);

/**
 * Read audit records from the ring buffer.
 * Returns the number of records copied to `out`.
 */
int msi_audit_read(MsiAuditRecord *out, int max_records,
                    uint64_t after_timestamp_ns);

/**
 * Get audit statistics.
 */
void msi_audit_stats(uint64_t *total_events, uint64_t *total_violations,
                      uint64_t *total_attestations);

#endif /* NINJAMAGIC_MSI_SECURITY_H */
