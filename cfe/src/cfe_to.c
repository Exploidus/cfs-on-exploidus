/*
 * NASA cFS Telemetry Output (TO) Application - Exploidus Native Port
 *
 * Subscribes to telemetry topics on the Software Bus, formats CCSDS
 * downlink packets with mission time tags, and manages downlink queues.
 */

#include <stdio.h>
#include <string.h>
#include "../../include/osapi.h"
#include "../inc/cfe_to.h"
#include "../inc/cfe_sb.h"
#include "../inc/cfe_evs.h"
#include "../inc/cfe_time.h"

static CFE_SB_PipeId_t      g_to_pipe;
static CFE_TO_Diagnostics_t g_to_diag;
static uint32               g_to_initialized = 0;

int32 CFE_TO_AppInit(void)
{
    memset(&g_to_diag, 0, sizeof(g_to_diag));

    int32 status = CFE_SB_CreatePipe(&g_to_pipe, 16, "CFE_TO_PIPE");
    if (status != CFE_SUCCESS) {
        return status;
    }

    g_to_initialized = 1;
    CFE_EVS_SendEvent(310, CFE_EVS_EventType_INFORMATION,
                      "TO: Telemetry Output app initialized, pipe ID %u created.",
                      g_to_pipe);
    return CFE_SUCCESS;
}

int32 CFE_TO_SubscribeStream(CFE_SB_MsgId_t MsgId)
{
    if (!g_to_initialized) return CFE_STATUS_BAD_ARGUMENT;

    int32 status = CFE_SB_Subscribe(MsgId, g_to_pipe);
    if (status == CFE_SUCCESS) {
        CFE_EVS_SendEvent(311, CFE_EVS_EventType_INFORMATION,
                          "TO: Subscribed to telemetry stream 0x%04X.", MsgId);
    }
    return status;
}

int32 CFE_TO_ProcessDownlink(void)
{
    if (!g_to_initialized) return CFE_STATUS_BAD_ARGUMENT;

    CFE_SB_Buffer_t *msg_buf = NULL;
    /* Receive buffer from SB pipe using non-blocking CFE_SB_POLL */
    int32 status = CFE_SB_ReceiveBuffer(&msg_buf, g_to_pipe, CFE_SB_POLL);
    if (status == CFE_SUCCESS && msg_buf != NULL) {
        g_to_diag.PacketsProcessed++;

        /* Downlink packet framing */
        CFE_TIME_SysTime_t met = CFE_TIME_GetMET();
        CFE_TO_TlmHeader_t tlm_hdr;
        tlm_hdr.StreamId   = (uint16)msg_buf->MsgId;
        tlm_hdr.Sequence   = (uint16)(g_to_diag.PacketsDownlinked & 0xFFFF);
        tlm_hdr.Length     = msg_buf->Size;
        tlm_hdr.Seconds    = met.Seconds;
        tlm_hdr.Subseconds = (uint16)(met.Subseconds >> 16);

        g_to_diag.PacketsDownlinked++;
        g_to_diag.DownlinkBytesSent += (sizeof(tlm_hdr) + msg_buf->Size);

        return CFE_SUCCESS;
    }

    return status;
}

int32 CFE_TO_GetDiagnostics(CFE_TO_Diagnostics_t *DiagPtr)
{
    if (!DiagPtr) return CFE_STATUS_BAD_ARGUMENT;
    memcpy(DiagPtr, &g_to_diag, sizeof(CFE_TO_Diagnostics_t));
    return CFE_SUCCESS;
}
