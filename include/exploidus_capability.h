#ifndef EXPLOIDUS_CAPABILITY_H
#define EXPLOIDUS_CAPABILITY_H

#include "common_types.h"

#define EXPLOIDUS_CAP_TOKEN_LEN 32

/*
 * Exploidus Unforgeable Capability Token
 * Bound to specific process ID, effector ID, and monotonic sequence window.
 */
typedef struct
{
    uint32    resource_id;     /* Target hardware effector (e.g. 0x01 = Thruster, 0x02 = GNC) */
    uint32    authorized_pid;  /* Authorized flight process PID */
    uint32    permissions;     /* EXPLOIDUS_CAP_* bitmask */
    uint32    valid_until_seq; /* Expiration sequence threshold */
    uint8     token_digest[EXPLOIDUS_CAP_TOKEN_LEN]; /* Cryptographic capability signature */
} exploidus_cap_token_t;

/*
 * Generates and validates capability token using BLAKE3-compatible keyed hash
 */
int32 Exploidus_Cap_Create(uint32 resource_id, uint32 pid, uint32 perms, uint32 max_seq, exploidus_cap_token_t *out_token);
int32 Exploidus_Cap_Verify(const exploidus_cap_token_t *token, uint32 resource_id, uint32 pid, uint32 required_perms, uint32 current_seq);

#endif /* EXPLOIDUS_CAPABILITY_H */
