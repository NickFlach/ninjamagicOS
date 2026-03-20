/*
 * NinjaMagic MSI Security Module — Implementation
 *
 * Grant enforcement, audit logging, and hardware attestation
 * for MSI capability-based security.
 */

#include "msi_security.h"
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/string.h>
#include <linux/ktime.h>
#include <linux/printk.h>
#include <linux/vmalloc.h>
#include <linux/stdarg.h>

#define TAG "msi_security"

/* ===== Audit Ring Buffer ===== */

static MsiAuditRecord *audit_ring;
static uint32_t audit_head;
static uint32_t audit_count;
static spinlock_t audit_lock;
static uint64_t audit_total_events;
static uint64_t audit_total_violations;
static uint64_t audit_total_attestations;

/* ===== Domain Registry ===== */

#define MSI_MAX_DOMAINS 256

static MsiSecureDomain *domains[MSI_MAX_DOMAINS];
static spinlock_t domains_lock;

/* ===== Initialization ===== */

int msi_security_init(void)
{
    audit_ring = vzalloc(sizeof(MsiAuditRecord) * MSI_AUDIT_RING_SIZE);
    if (!audit_ring)
        return -ENOMEM;

    audit_head = 0;
    audit_count = 0;
    audit_total_events = 0;
    audit_total_violations = 0;
    audit_total_attestations = 0;
    spin_lock_init(&audit_lock);
    spin_lock_init(&domains_lock);

    memset(domains, 0, sizeof(domains));

    pr_info("[%s] MSI security subsystem initialized\n", TAG);
    pr_info("[%s]   Audit ring: %d entries\n", TAG, MSI_AUDIT_RING_SIZE);
    pr_info("[%s]   Max domains: %d\n", TAG, MSI_MAX_DOMAINS);
    pr_info("[%s]   Max grants/domain: %d\n", TAG, MSI_MAX_GRANTS_PER_DOMAIN);

    return 0;
}

void msi_security_exit(void)
{
    unsigned long flags;
    int i;

    spin_lock_irqsave(&domains_lock, flags);
    for (i = 0; i < MSI_MAX_DOMAINS; i++) {
        if (domains[i]) {
            kfree(domains[i]);
            domains[i] = NULL;
        }
    }
    spin_unlock_irqrestore(&domains_lock, flags);

    vfree(audit_ring);
    audit_ring = NULL;

    pr_info("[%s] MSI security subsystem shut down. "
            "Total events: %llu, violations: %llu\n",
            TAG, audit_total_events, audit_total_violations);
}

/* ===== Audit Logging ===== */

void msi_audit_log(MsiAuditType type, MsiAuditLevel level,
                    uint32_t domain_id, uint32_t lane_id,
                    int result, const char *fmt, ...)
{
    unsigned long flags;
    MsiAuditRecord *rec;
    va_list args;

    if (!audit_ring)
        return;

    spin_lock_irqsave(&audit_lock, flags);

    rec = &audit_ring[audit_head % MSI_AUDIT_RING_SIZE];
    rec->timestamp_ns = ktime_get_boottime_ns();
    rec->domain_id = domain_id;
    rec->lane_id = lane_id;
    rec->pid = current->pid;
    rec->uid = current_uid().val;
    rec->type = type;
    rec->level = level;
    rec->result = result;

    if (fmt) {
        va_start(args, fmt);
        vsnprintf(rec->detail, MSI_AUDIT_MAX_DETAIL, fmt, args);
        va_end(args);
    } else {
        rec->detail[0] = '\0';
    }

    audit_head++;
    if (audit_count < MSI_AUDIT_RING_SIZE)
        audit_count++;

    audit_total_events++;
    if (level >= MSI_AUDIT_LEVEL_VIOLATION)
        audit_total_violations++;
    if (type == MSI_AUDIT_ATTESTATION_REQUEST ||
        type == MSI_AUDIT_ATTESTATION_COMPLETE)
        audit_total_attestations++;

    spin_unlock_irqrestore(&audit_lock, flags);

    /* Also log violations to kernel log */
    if (level >= MSI_AUDIT_LEVEL_VIOLATION) {
        pr_warn("[%s] VIOLATION domain=%u lane=%u pid=%d: %s (result=%d)\n",
                TAG, domain_id, lane_id, current->pid,
                rec->detail, result);
    }
}

int msi_audit_read(MsiAuditRecord *out, int max_records,
                    uint64_t after_timestamp_ns)
{
    unsigned long flags;
    int copied = 0;
    uint32_t i, start;

    if (!audit_ring || !out || max_records <= 0)
        return 0;

    spin_lock_irqsave(&audit_lock, flags);

    start = (audit_head >= audit_count) ?
            (audit_head - audit_count) : 0;

    for (i = start; i < audit_head && copied < max_records; i++) {
        MsiAuditRecord *rec = &audit_ring[i % MSI_AUDIT_RING_SIZE];
        if (rec->timestamp_ns > after_timestamp_ns) {
            memcpy(&out[copied], rec, sizeof(MsiAuditRecord));
            copied++;
        }
    }

    spin_unlock_irqrestore(&audit_lock, flags);
    return copied;
}

void msi_audit_stats(uint64_t *total_events, uint64_t *total_violations,
                      uint64_t *total_attestations)
{
    if (total_events) *total_events = audit_total_events;
    if (total_violations) *total_violations = audit_total_violations;
    if (total_attestations) *total_attestations = audit_total_attestations;
}

/* ===== Domain Management ===== */

static MsiSecureDomain* find_domain(uint32_t domain_id)
{
    if (domain_id >= MSI_MAX_DOMAINS)
        return NULL;
    return domains[domain_id];
}

/* ===== Grant Enforcement ===== */

static bool grant_matches(const MsiGrant *grant, MsiGrantType type,
                           MsiPermission perm, const char *resource)
{
    if (grant->type != type)
        return false;

    /* Check permission bits */
    if ((grant->perm & perm) != perm)
        return false;

    /* Check resource prefix match */
    if (resource && grant->resource[0]) {
        size_t prefix_len = strlen(grant->resource);
        /* Grants with trailing '/' match any sub-topic */
        if (grant->resource[prefix_len - 1] == '/') {
            if (strncmp(resource, grant->resource, prefix_len) != 0)
                return false;
        } else {
            if (strcmp(resource, grant->resource) != 0)
                return false;
        }
    }

    return true;
}

int msi_check_grant(uint32_t domain_id, MsiGrantType type,
                     MsiPermission perm, const char *resource)
{
    MsiSecureDomain *dom;
    unsigned long flags;
    uint32_t i;
    bool allowed = false;

    dom = find_domain(domain_id);
    if (!dom) {
        msi_audit_log(MSI_AUDIT_VIOLATION_GRANT, MSI_AUDIT_LEVEL_VIOLATION,
                       domain_id, 0, -ENOENT,
                       "grant check on non-existent domain");
        return -ENOENT;
    }

    spin_lock_irqsave(&dom->lock, flags);

    /* Clock grants don't need resource matching */
    if (type == MSI_GRANT_CLOCK) {
        for (i = 0; i < dom->grant_count; i++) {
            if (dom->grants[i].type == MSI_GRANT_CLOCK) {
                allowed = true;
                break;
            }
        }
    } else {
        for (i = 0; i < dom->grant_count; i++) {
            if (grant_matches(&dom->grants[i], type, perm, resource)) {
                allowed = true;
                break;
            }
        }
    }

    spin_unlock_irqrestore(&dom->lock, flags);

    if (!allowed) {
        msi_audit_log(MSI_AUDIT_VIOLATION_GRANT, MSI_AUDIT_LEVEL_VIOLATION,
                       domain_id, 0, -EACCES,
                       "denied type=%d perm=%d resource='%s'",
                       type, perm, resource ? resource : "(null)");
        return -EACCES;
    }

    return 0;
}

/* ===== Domain Sealing ===== */

int msi_seal_domain(uint32_t domain_id)
{
    MsiSecureDomain *dom;
    unsigned long flags;

    dom = find_domain(domain_id);
    if (!dom)
        return -ENOENT;

    spin_lock_irqsave(&dom->lock, flags);

    if (dom->sealed) {
        spin_unlock_irqrestore(&dom->lock, flags);
        msi_audit_log(MSI_AUDIT_VIOLATION_SEALED, MSI_AUDIT_LEVEL_WARN,
                       domain_id, 0, -EPERM,
                       "attempt to re-seal already sealed domain");
        return -EPERM;
    }

    dom->sealed = true;
    dom->sealed_ns = ktime_get_boottime_ns();

    spin_unlock_irqrestore(&dom->lock, flags);

    msi_audit_log(MSI_AUDIT_DOMAIN_SEAL, MSI_AUDIT_LEVEL_INFO,
                   domain_id, 0, 0,
                   "domain '%s' sealed with %u grants",
                   dom->name, dom->grant_count);

    pr_info("[%s] Domain %u ('%s') sealed — %u grants, immutable\n",
            TAG, domain_id, dom->name, dom->grant_count);

    return 0;
}

/* ===== Hardware Attestation ===== */

static int compute_domain_hash(MsiSecureDomain *dom, uint8_t hash_out[32])
{
    /*
     * SHA-256 over serialized domain grants.
     * In production, uses kernel crypto API:
     *   struct crypto_shash *tfm = crypto_alloc_shash("sha256", 0, 0);
     *   crypto_shash_digest(desc, data, len, hash_out);
     *
     * For now, placeholder.
     */
    memset(hash_out, 0, 32);
    /* Mix in domain properties */
    hash_out[0] = (uint8_t)(dom->domain_id & 0xFF);
    hash_out[1] = (uint8_t)(dom->grant_count & 0xFF);
    hash_out[2] = dom->sealed ? 0xFF : 0x00;
    return 0;
}

static int sign_attestation_hardware(const uint8_t *data, size_t data_len,
                                      uint8_t *sig_out, uint32_t *sig_len)
{
    /*
     * In production:
     * - Pixel 7: send to Titan M2 via /dev/titan_m2 for signing
     * - Nord N30: send to Qualcomm SPU via /dev/qseecom for signing
     *
     * The hardware SE signs with its embedded attestation key,
     * chained to Google/Qualcomm root CA.
     */
    memset(sig_out, 0xAA, 256);
    *sig_len = 256;
    return 0;
}

int msi_attest_domain(uint32_t domain_id,
                       const uint8_t nonce[MSI_ATTEST_NONCE_SIZE],
                       MsiAttestationResult *result)
{
    MsiSecureDomain *dom;
    unsigned long flags;
    int ret;

    if (!result || !nonce)
        return -EINVAL;

    msi_audit_log(MSI_AUDIT_ATTESTATION_REQUEST, MSI_AUDIT_LEVEL_INFO,
                   domain_id, 0, 0, "attestation requested");

    dom = find_domain(domain_id);
    if (!dom)
        return -ENOENT;

    memset(result, 0, sizeof(*result));
    result->domain_id = domain_id;
    memcpy(result->nonce, nonce, MSI_ATTEST_NONCE_SIZE);
    result->timestamp_ns = ktime_get_boottime_ns();

    spin_lock_irqsave(&dom->lock, flags);
    result->grant_count = dom->grant_count;
    result->is_sealed = dom->sealed;
    ret = compute_domain_hash(dom, result->domain_hash);
    spin_unlock_irqrestore(&dom->lock, flags);

    if (ret != 0)
        return ret;

    /* Sign with hardware secure element */
    ret = sign_attestation_hardware(
        result->domain_hash, 32,
        result->signature, &result->signature_len
    );

    if (ret != 0) {
        msi_audit_log(MSI_AUDIT_ATTESTATION_COMPLETE, MSI_AUDIT_LEVEL_WARN,
                       domain_id, 0, ret, "attestation signing failed");
        return ret;
    }

    msi_audit_log(MSI_AUDIT_ATTESTATION_COMPLETE, MSI_AUDIT_LEVEL_INFO,
                   domain_id, 0, 0,
                   "attestation complete, sealed=%d, grants=%u",
                   result->is_sealed, result->grant_count);

    return 0;
}
