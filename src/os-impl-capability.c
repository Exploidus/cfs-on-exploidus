#include <string.h>
#include "../include/exploidus_capability.h"
#include "../include/osapi-error.h"

/*
 * Exploidus Kernel Master Secret Key
 * In production Exploidus OS, this key is pinned exclusively in kernel-isolated memory
 * and is never mapped to userspace address space.
 */
static const uint8 KERNEL_MASTER_SECRET[32] = {
    0xEC, 0x5A, 0x77, 0x19, 0x48, 0x9B, 0x22, 0xAF,
    0xD3, 0x88, 0x01, 0xFE, 0x76, 0x54, 0x32, 0x10,
    0xBA, 0xDC, 0xFE, 0x98, 0x76, 0x54, 0x32, 0x10,
    0x0F, 0x1E, 0x2D, 0x3C, 0x4B, 0x5A, 0x69, 0x78
};

/*
 * Constant-time 32-bit SipHash/BLAKE3-style mixing function
 */
static void compute_token_digest(uint32 resource_id, uint32 pid, uint32 perms, uint32 max_seq, uint8 out[32])
{
    uint32 state[8];
    for (int i = 0; i < 8; ++i) {
        state[i] = ((const uint32 *)KERNEL_MASTER_SECRET)[i];
    }

    state[0] ^= resource_id;
    state[1] ^= pid;
    state[2] ^= perms;
    state[3] ^= max_seq;

    /* 4 rounds of cryptographic diffusion */
    for (int r = 0; r < 4; ++r) {
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

int32 Exploidus_Cap_Create(uint32 resource_id, uint32 pid, uint32 perms, uint32 max_seq, exploidus_cap_token_t *out_token)
{
    if (out_token == NULL) {
        return OS_INVALID_POINTER;
    }

    out_token->resource_id = resource_id;
    out_token->authorized_pid = pid;
    out_token->permissions = perms;
    out_token->valid_until_seq = max_seq;

    compute_token_digest(resource_id, pid, perms, max_seq, out_token->token_digest);
    return OS_SUCCESS;
}

int32 Exploidus_Cap_Verify(const exploidus_cap_token_t *token, uint32 resource_id, uint32 pid, uint32 required_perms, uint32 current_seq)
{
    if (token == NULL) {
        return OS_INVALID_POINTER;
    }

    /* 1. Verify resource binding */
    if (token->resource_id != resource_id) {
        return OS_ERR_EFFECTOR_UNAUTHORIZED;
    }

    /* 2. Verify process ownership */
    if (token->authorized_pid != pid) {
        return OS_ERR_CAPABILITY_DENIED;
    }

    /* 3. Verify permissions */
    if ((token->permissions & required_perms) != required_perms) {
        return OS_ERR_CAPABILITY_DENIED;
    }

    /* 4. Verify freshness / monotonic sequence expiration */
    if (current_seq > token->valid_until_seq) {
        return OS_ERR_CAPABILITY_EXPIRED;
    }

    /* 5. Cryptographic signature check (constant-time comparison) */
    uint8 expected_digest[32];
    compute_token_digest(token->resource_id, token->authorized_pid, token->permissions, token->valid_until_seq, expected_digest);

    uint8 diff = 0;
    for (int i = 0; i < 32; ++i) {
        diff |= (token->token_digest[i] ^ expected_digest[i]);
    }

    if (diff != 0) {
        return OS_ERR_CAPABILITY_DENIED;
    }

    return OS_SUCCESS;
}
