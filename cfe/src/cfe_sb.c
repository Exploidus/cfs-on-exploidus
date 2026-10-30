#include <stdio.h>
#include <string.h>
#include "../inc/cfe_sb.h"
#include "../inc/cfe_evs.h"

typedef struct
{
    bool       InUse;
    char       Name[32];
    osal_id_t  QueueId;
    uint32     OwnerTask;
    uint16     Depth;
} CFE_SB_PipeEntry_t;

typedef struct
{
    bool            InUse;
    CFE_SB_MsgId_t  MsgId;
    CFE_SB_PipeId_t PipeId;
    bool            IsRestricted;
} CFE_SB_RouteEntry_t;

static CFE_SB_PipeEntry_t  g_sb_pipes[CFE_SB_MAX_PIPES];
static CFE_SB_RouteEntry_t g_sb_routes[CFE_SB_MAX_SUBSCRIPTIONS];
static osal_id_t           g_sb_mutex;
static bool                g_sb_initialized = false;
static uint16              g_sb_global_seq = 0;

/* Static receive buffers per pipe to avoid dynamic heap allocation */
static CFE_SB_Buffer_t     g_sb_rx_buffers[CFE_SB_MAX_PIPES];

int32 CFE_SB_EarlyInit(void)
{
    if (g_sb_initialized) {
        return CFE_SUCCESS;
    }

    memset(g_sb_pipes, 0, sizeof(g_sb_pipes));
    memset(g_sb_routes, 0, sizeof(g_sb_routes));
    memset(g_sb_rx_buffers, 0, sizeof(g_sb_rx_buffers));

    int32 status = OS_MutSemCreate(&g_sb_mutex, "CFE_SB_MUTEX", 0);
    if (status != OS_SUCCESS) {
        return status;
    }

    g_sb_initialized = true;
    OS_printf("[CFE_SB] Software Bus (SB) core service initialized.\n");
    return CFE_SUCCESS;
}

int32 CFE_SB_CreatePipe(CFE_SB_PipeId_t *PipeIdPtr, uint16 Depth, const char *PipeName)
{
    if (!PipeIdPtr || !PipeName || Depth == 0 || Depth > CFE_SB_PIPE_DEPTH_MAX) {
        return CFE_SB_BAD_ARGUMENT;
    }

    OS_MutSemTake(g_sb_mutex);

    for (uint32 i = 0; i < CFE_SB_MAX_PIPES; i++) {
        if (!g_sb_pipes[i].InUse) {
            osal_id_t queue_id;
            int32 status = OS_QueueCreate(&queue_id, PipeName, Depth, sizeof(CFE_SB_Buffer_t), 0);
            if (status != OS_SUCCESS) {
                OS_MutSemGive(g_sb_mutex);
                return status;
            }

            g_sb_pipes[i].InUse = true;
            g_sb_pipes[i].QueueId = queue_id;
            g_sb_pipes[i].OwnerTask = (uint32)OS_TaskGetId();
            g_sb_pipes[i].Depth = Depth;
            strncpy(g_sb_pipes[i].Name, PipeName, sizeof(g_sb_pipes[i].Name) - 1);
            g_sb_pipes[i].Name[sizeof(g_sb_pipes[i].Name) - 1] = '\0';

            *PipeIdPtr = (CFE_SB_PipeId_t)i;
            OS_MutSemGive(g_sb_mutex);
            return CFE_SUCCESS;
        }
    }

    OS_MutSemGive(g_sb_mutex);
    return CFE_SB_MAX_PIPES_MET;
}

int32 CFE_SB_DeletePipe(CFE_SB_PipeId_t PipeId)
{
    if (PipeId >= CFE_SB_MAX_PIPES) {
        return CFE_SB_BAD_ARGUMENT;
    }

    OS_MutSemTake(g_sb_mutex);

    if (!g_sb_pipes[PipeId].InUse) {
        OS_MutSemGive(g_sb_mutex);
        return CFE_SB_BAD_ARGUMENT;
    }

    /* Unsubscribe all matching routes */
    for (uint32 i = 0; i < CFE_SB_MAX_SUBSCRIPTIONS; i++) {
        if (g_sb_routes[i].InUse && g_sb_routes[i].PipeId == PipeId) {
            g_sb_routes[i].InUse = false;
        }
    }

    OS_QueueDelete(g_sb_pipes[PipeId].QueueId);
    g_sb_pipes[PipeId].InUse = false;

    OS_MutSemGive(g_sb_mutex);
    return CFE_SUCCESS;
}

int32 CFE_SB_Subscribe(CFE_SB_MsgId_t MsgId, CFE_SB_PipeId_t PipeId)
{
    /* If the message ID requires security capabilities, reject standard unauthenticated subscribe */
    if (MsgId & CFE_SB_CAP_RESTRICTED_MASK) {
        OS_printf("[CFE_SB] Security rejection: Restricted MsgId 0x%08X requires capability token.\n", MsgId);
        return CFE_SB_ACCESS_DENIED;
    }

    return CFE_SB_SubscribeGated(MsgId, PipeId, NULL);
}

int32 CFE_SB_SubscribeGated(CFE_SB_MsgId_t MsgId, CFE_SB_PipeId_t PipeId, const exploidus_cap_token_t *CapToken)
{
    if (PipeId >= CFE_SB_MAX_PIPES || !g_sb_pipes[PipeId].InUse) {
        return CFE_SB_BAD_ARGUMENT;
    }

    osal_id_t caller_task = OS_TaskGetId();

    if (MsgId & CFE_SB_CAP_RESTRICTED_MASK) {
        if (!CapToken) {
            return CFE_SB_ACCESS_DENIED;
        }
        int32 v_stat = Exploidus_Cap_Verify(CapToken, MsgId, (uint32)caller_task, EXPLOIDUS_CAP_READ, 1);
        if (v_stat != OS_SUCCESS) {
            OS_printf("[CFE_SB] BLAKE3 capability check failed for task %u on MsgId 0x%08X (Error: %d)\n",
                      caller_task, MsgId, v_stat);
            return CFE_SB_ACCESS_DENIED;
        }
    }

    OS_MutSemTake(g_sb_mutex);

    /* Check for duplicate route */
    for (uint32 i = 0; i < CFE_SB_MAX_SUBSCRIPTIONS; i++) {
        if (g_sb_routes[i].InUse && g_sb_routes[i].MsgId == MsgId && g_sb_routes[i].PipeId == PipeId) {
            OS_MutSemGive(g_sb_mutex);
            return CFE_SUCCESS;
        }
    }

    /* Add route */
    for (uint32 i = 0; i < CFE_SB_MAX_SUBSCRIPTIONS; i++) {
        if (!g_sb_routes[i].InUse) {
            g_sb_routes[i].InUse = true;
            g_sb_routes[i].MsgId = MsgId;
            g_sb_routes[i].PipeId = PipeId;
            g_sb_routes[i].IsRestricted = (MsgId & CFE_SB_CAP_RESTRICTED_MASK) != 0;
            OS_MutSemGive(g_sb_mutex);
            return CFE_SUCCESS;
        }
    }

    OS_MutSemGive(g_sb_mutex);
    return CFE_SB_ERROR;
}

int32 CFE_SB_Unsubscribe(CFE_SB_MsgId_t MsgId, CFE_SB_PipeId_t PipeId)
{
    if (PipeId >= CFE_SB_MAX_PIPES) {
        return CFE_SB_BAD_ARGUMENT;
    }

    OS_MutSemTake(g_sb_mutex);
    for (uint32 i = 0; i < CFE_SB_MAX_SUBSCRIPTIONS; i++) {
        if (g_sb_routes[i].InUse && g_sb_routes[i].MsgId == MsgId && g_sb_routes[i].PipeId == PipeId) {
            g_sb_routes[i].InUse = false;
            OS_MutSemGive(g_sb_mutex);
            return CFE_SUCCESS;
        }
    }
    OS_MutSemGive(g_sb_mutex);
    return CFE_SUCCESS;
}

int32 CFE_SB_TransmitMsg(CFE_SB_Buffer_t *BufPtr, bool IncrementSequence)
{
    if (!BufPtr) {
        return CFE_SB_BAD_ARGUMENT;
    }

    if (BufPtr->MsgId & CFE_SB_CAP_RESTRICTED_MASK) {
        OS_printf("[CFE_SB] Security rejection: Transmit to restricted MsgId 0x%08X without capability token blocked.\n",
                  BufPtr->MsgId);
        return CFE_SB_ACCESS_DENIED;
    }

    return CFE_SB_TransmitMsgGated(BufPtr, IncrementSequence, NULL);
}

int32 CFE_SB_TransmitMsgGated(CFE_SB_Buffer_t *BufPtr, bool IncrementSequence, const exploidus_cap_token_t *CapToken)
{
    if (!BufPtr) {
        return CFE_SB_BAD_ARGUMENT;
    }

    osal_id_t caller_task = OS_TaskGetId();

    if (BufPtr->MsgId & CFE_SB_CAP_RESTRICTED_MASK) {
        if (!CapToken) {
            return CFE_SB_ACCESS_DENIED;
        }
        int32 v_stat = Exploidus_Cap_Verify(CapToken, BufPtr->MsgId, (uint32)caller_task, EXPLOIDUS_CAP_WRITE, 1);
        if (v_stat != OS_SUCCESS) {
            OS_printf("[CFE_SB] Capability verification failed for transmit on MsgId 0x%08X (Error: %d)\n",
                      BufPtr->MsgId, v_stat);
            return CFE_SB_ACCESS_DENIED;
        }
    }

    OS_MutSemTake(g_sb_mutex);

    if (IncrementSequence) {
        g_sb_global_seq++;
        BufPtr->Sequence = g_sb_global_seq;
    }
    BufPtr->SenderApp = (uint32)caller_task;

    uint32 routed_count = 0;
    for (uint32 i = 0; i < CFE_SB_MAX_SUBSCRIPTIONS; i++) {
        if (g_sb_routes[i].InUse && g_sb_routes[i].MsgId == BufPtr->MsgId) {
            CFE_SB_PipeId_t pipe = g_sb_routes[i].PipeId;
            if (g_sb_pipes[pipe].InUse) {
                OS_QueuePut(g_sb_pipes[pipe].QueueId, BufPtr, sizeof(CFE_SB_Buffer_t), 0);
                routed_count++;
            }
        }
    }

    OS_MutSemGive(g_sb_mutex);
    return CFE_SUCCESS;
}

int32 CFE_SB_ReceiveBuffer(CFE_SB_Buffer_t **BufPtr, CFE_SB_PipeId_t PipeId, int32 TimeOut)
{
    if (!BufPtr || PipeId >= CFE_SB_MAX_PIPES) {
        return CFE_SB_BAD_ARGUMENT;
    }

    if (!g_sb_pipes[PipeId].InUse) {
        return CFE_SB_PIPE_RD_ERR;
    }

    size_t actual_size = 0;
    int32 status = OS_QueueGet(g_sb_pipes[PipeId].QueueId, &g_sb_rx_buffers[PipeId], sizeof(CFE_SB_Buffer_t), &actual_size, TimeOut);
    if (status != OS_SUCCESS) {
        *BufPtr = NULL;
        return (status == OS_QUEUE_EMPTY) ? CFE_SB_NO_MESSAGE : CFE_SB_PIPE_RD_ERR;
    }

    *BufPtr = &g_sb_rx_buffers[PipeId];
    return CFE_SUCCESS;
}

int32 CFE_SB_InitMsg(CFE_SB_Buffer_t *BufPtr, CFE_SB_MsgId_t MsgId, size_t Size, bool Clear)
{
    if (!BufPtr || Size > CFE_SB_MAX_MSG_SIZE) {
        return CFE_SB_BAD_ARGUMENT;
    }

    if (Clear) {
        memset(BufPtr, 0, sizeof(CFE_SB_Buffer_t));
    }

    BufPtr->MsgId = MsgId;
    BufPtr->Size = (uint16)Size;
    BufPtr->Sequence = 0;
    BufPtr->SenderApp = (uint32)OS_TaskGetId();

    return CFE_SUCCESS;
}
