#include <stdio.h>
#include <string.h>
#include "../include/osapi.h"

static uint32 g_effector_sequence = 1;

int32 OS_EffectorActuate(uint32 effector_id, const exploidus_cap_token_t *token, const void *payload, size_t len)
{
    (void)payload;
    (void)len;

    osal_id_t caller_task = OS_TaskGetId();

    /*
     * Enforce Exploidus BLAKE3 Capability Validation
     * The caller task MUST possess EXPLOIDUS_CAP_EFFECTOR permission for this effector_id.
     */
    int32 status = Exploidus_Cap_Verify(token, effector_id, (uint32)caller_task, EXPLOIDUS_CAP_EFFECTOR, g_effector_sequence);
    if (status != OS_SUCCESS) {
        OS_printf("[SECURITY AUDIT] Unauthorized effector actuation attempt blocked! (Effector: 0x%02X, Task: %u, Error: %d)\n",
                  effector_id, caller_task, status);
        return status;
    }

    g_effector_sequence++;
    OS_printf("[EFFECTOR FIRED] Authorized hardware command executed on Effector 0x%02X (Seq: %u, Task: %u)\n",
              effector_id, g_effector_sequence, caller_task);
    return OS_SUCCESS;
}
