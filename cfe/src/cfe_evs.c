#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "../inc/cfe_evs.h"

static CFE_EVS_AuditEntry_t g_evs_audit_log[CFE_EVS_MAX_AUDIT_LOGS];
static uint32               g_evs_audit_count = 0;
static osal_id_t            g_evs_mutex;
static bool                 g_evs_initialized = false;
static uint8                g_evs_last_hash[32];

/* Initial genesis hash for audit log chain */
static const uint8 EVS_GENESIS_SEED[32] = {
    0x45, 0x58, 0x50, 0x4C, 0x4F, 0x49, 0x44, 0x55, /* "EXPLOIDU" */
    0x53, 0x5F, 0x43, 0x46, 0x53, 0x5F, 0x45, 0x56, /* "S_CFS_EV" */
    0x53, 0x5F, 0x41, 0x55, 0x44, 0x49, 0x54, 0x5F, /* "S_AUDIT_" */
    0x47, 0x45, 0x4E, 0x45, 0x53, 0x49, 0x53, 0x31  /* "GENESIS1" */
};

static void compute_chain_hash(const uint8 prev_hash[32], const CFE_EVS_AuditEntry_t *entry, uint8 out[32])
{
    uint32 state[8];
    for (int i = 0; i < 8; ++i) {
        state[i] = ((const uint32 *)prev_hash)[i];
    }

    state[0] ^= entry->Sequence;
    state[1] ^= entry->Timestamp;
    state[2] ^= (entry->AppId << 16) | entry->EventID;
    state[3] ^= (uint32)entry->EventType;

    /* Mix in message payload */
    const uint32 *msg_words = (const uint32 *)entry->Message;
    for (size_t w = 0; w < sizeof(entry->Message) / 4; ++w) {
        state[w % 8] ^= msg_words[w];
    }

    /* 6 rounds of cryptographic diffusion */
    for (int r = 0; r < 6; ++r) {
        state[0] = state[0] + state[1];
        state[3] = (state[3] ^ state[0]);
        state[3] = (state[3] << 16) | (state[3] >> 16);

        state[2] = state[2] + state[3];
        state[1] = (state[1] ^ state[2]);
        state[1] = (state[1] << 12) | (state[1] >> 20);

        state[0] = state[0] + state[1];
        state[3] = (state[3] ^ state[0]);
        state[3] = (state[3] << 8) | (state[3] >> 24);

        state[2] = state[2] + state[3];
        state[1] = (state[1] ^ state[2]);
        state[1] = (state[1] << 7) | (state[1] >> 25);
    }

    memcpy(out, state, 32);
}

int32 CFE_EVS_EarlyInit(void)
{
    if (g_evs_initialized) {
        return CFE_SUCCESS;
    }

    memset(g_evs_audit_log, 0, sizeof(g_evs_audit_log));
    g_evs_audit_count = 0;
    memcpy(g_evs_last_hash, EVS_GENESIS_SEED, 32);

    int32 status = OS_MutSemCreate(&g_evs_mutex, "CFE_EVS_MUTEX", 0);
    if (status != OS_SUCCESS) {
        return status;
    }

    g_evs_initialized = true;
    OS_printf("[CFE_EVS] Event Services (EVS) with Cryptographic Audit Logging initialized.\n");
    return CFE_SUCCESS;
}

int32 CFE_EVS_Register(const void *Filters, uint16 NumEventFilters, uint16 FilterScheme)
{
    (void)Filters;
    (void)NumEventFilters;
    (void)FilterScheme;
    return CFE_SUCCESS;
}

int32 CFE_EVS_Unregister(void)
{
    return CFE_SUCCESS;
}

int32 CFE_EVS_SendEvent(uint16 EventID, uint16 EventType, const char *Spec, ...)
{
    if (!Spec) {
        return CFE_EVS_INVALID_PARAMETER;
    }

    char buffer[CFE_EVS_MAX_MSG_LEN];
    va_list args;
    va_start(args, Spec);
    vsnprintf(buffer, sizeof(buffer), Spec, args);
    va_end(args);

    OS_MutSemTake(g_evs_mutex);

    uint32 idx = g_evs_audit_count;
    if (idx >= CFE_EVS_MAX_AUDIT_LOGS) {
        /* Circular retention overwrite */
        idx = idx % CFE_EVS_MAX_AUDIT_LOGS;
    }

    CFE_EVS_AuditEntry_t *entry = &g_evs_audit_log[idx];
    entry->Sequence = g_evs_audit_count + 1;

    OS_time_t now;
    OS_GetLocalTime(&now);
    entry->Timestamp = (uint32)now.seconds;
    entry->AppId = (uint32)OS_TaskGetId();
    entry->EventID = EventID;
    entry->EventType = EventType;

    strncpy(entry->Message, buffer, sizeof(entry->Message) - 1);
    entry->Message[sizeof(entry->Message) - 1] = '\0';

    compute_chain_hash(g_evs_last_hash, entry, entry->AuditHash);
    memcpy(g_evs_last_hash, entry->AuditHash, 32);

    g_evs_audit_count++;

    const char *type_str = "INFO";
    if (EventType == CFE_EVS_EventType_DEBUG)    type_str = "DEBUG";
    if (EventType == CFE_EVS_EventType_ERROR)    type_str = "ERROR";
    if (EventType == CFE_EVS_EventType_CRITICAL) type_str = "CRIT ";

    OS_printf("[EVS][%s][App:%u][Event:%u][Seq:%u] %s (Hash: %02x%02x%02x%02x..)\n",
              type_str, entry->AppId, entry->EventID, entry->Sequence,
              entry->Message, entry->AuditHash[0], entry->AuditHash[1],
              entry->AuditHash[2], entry->AuditHash[3]);

    OS_MutSemGive(g_evs_mutex);
    return CFE_SUCCESS;
}

int32 CFE_EVS_VerifyAuditChain(void)
{
    OS_MutSemTake(g_evs_mutex);

    uint8 rolling_hash[32];
    memcpy(rolling_hash, EVS_GENESIS_SEED, 32);

    uint32 entries_to_verify = g_evs_audit_count;
    if (entries_to_verify > CFE_EVS_MAX_AUDIT_LOGS) {
        entries_to_verify = CFE_EVS_MAX_AUDIT_LOGS;
    }

    for (uint32 i = 0; i < entries_to_verify; i++) {
        uint8 expected_hash[32];
        compute_chain_hash(rolling_hash, &g_evs_audit_log[i], expected_hash);

        if (memcmp(expected_hash, g_evs_audit_log[i].AuditHash, 32) != 0) {
            OS_printf("[SECURITY AUDIT] Chain corruption or log tampering detected at entry index %u!\n", i);
            OS_MutSemGive(g_evs_mutex);
            return CFE_EVS_ERROR;
        }

        memcpy(rolling_hash, expected_hash, 32);
    }

    OS_MutSemGive(g_evs_mutex);
    return CFE_SUCCESS;
}

uint32 CFE_EVS_GetAuditLogCount(void)
{
    return g_evs_audit_count;
}

int32 CFE_EVS_InjectTamperFault(uint32 index)
{
    OS_MutSemTake(g_evs_mutex);
    if (index < g_evs_audit_count && index < CFE_EVS_MAX_AUDIT_LOGS) {
        g_evs_audit_log[index].Message[0] ^= 0xFF;
        OS_MutSemGive(g_evs_mutex);
        return CFE_SUCCESS;
    }
    OS_MutSemGive(g_evs_mutex);
    return CFE_EVS_INVALID_PARAMETER;
}
