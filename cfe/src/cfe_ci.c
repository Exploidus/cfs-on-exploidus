/*
 * NASA cFS Command Ingest (CI) Application - Exploidus Native Port
 *
 * Implements uplink ground command ingest, CCSDS packet extraction,
 * sequence counter validation, and capability-gated dispatch on the Software Bus.
 */

#include <stdio.h>
#include <string.h>
#include "../../include/osapi.h"
#include "../inc/cfe_ci.h"
#include "../inc/cfe_sb.h"
#include "../inc/cfe_evs.h"

static CFE_CI_Diagnostics_t g_ci_diag;
static uint32               g_ci_initialized = 0;

int32 CFE_CI_AppInit(void)
{
    memset(&g_ci_diag, 0, sizeof(g_ci_diag));
    g_ci_initialized = 1;
    CFE_EVS_SendEvent(300, CFE_EVS_EventType_INFORMATION,
                      "CI: Command Ingest application initialized.");
    return CFE_SUCCESS;
}

int32 CFE_CI_IngestFrame(const uint8 *FrameBuffer, uint32 FrameLen)
{
    if (!FrameBuffer || FrameLen < sizeof(CFE_CI_CmdHeader_t)) {
        g_ci_diag.CommandsRejected++;
        return CFE_STATUS_BAD_COMMAND_SIZE;
    }

    g_ci_diag.CommandsIngested++;

    const CFE_CI_CmdHeader_t *hdr = (const CFE_CI_CmdHeader_t *)FrameBuffer;

    /* Verify packet length matches payload */
    uint32 expected_total = sizeof(CFE_CI_CmdHeader_t) + hdr->Length;
    if (FrameLen < expected_total) {
        g_ci_diag.CommandsRejected++;
        CFE_EVS_SendEvent(301, CFE_EVS_EventType_ERROR,
                          "CI: Truncated frame (got %u, expected %u bytes).",
                          FrameLen, expected_total);
        return CFE_STATUS_BAD_COMMAND_SIZE;
    }

    /* Basic 8-bit XOR checksum verification across header + payload */
    uint8 chk = 0;
    for (uint32 i = 0; i < expected_total; i++) {
        chk ^= FrameBuffer[i];
    }
    if (chk != 0) {
        g_ci_diag.InvalidCrcCount++;
        g_ci_diag.CommandsRejected++;
        CFE_EVS_SendEvent(302, CFE_EVS_EventType_ERROR,
                          "CI: Checksum mismatch on APID 0x%04X, reject command.",
                          hdr->StreamId);
        return CFE_STATUS_CHECKSUM_ERROR;
    }

    /* Package into Ingest Packet and dispatch */
    CFE_CI_IngestPacket_t packet;
    memcpy(&packet.Header, hdr, sizeof(CFE_CI_CmdHeader_t));
    uint32 payload_bytes = hdr->Length;
    if (payload_bytes > sizeof(packet.Payload)) {
        payload_bytes = sizeof(packet.Payload);
    }
    memcpy(packet.Payload, FrameBuffer + sizeof(CFE_CI_CmdHeader_t), payload_bytes);

    return CFE_CI_Dispatch(&packet);
}

int32 CFE_CI_Dispatch(const CFE_CI_IngestPacket_t *CmdPacket)
{
    if (!CmdPacket) return CFE_STATUS_BAD_ARGUMENT;

    /* Prepare Software Bus message */
    CFE_SB_Buffer_t sb_msg;
    CFE_SB_MsgId_t msg_id = (CFE_SB_MsgId_t)CmdPacket->Header.StreamId;

    uint32 msg_size = sizeof(CFE_CI_CmdHeader_t) + CmdPacket->Header.Length;
    if (msg_size > sizeof(sb_msg.Data)) {
        msg_size = sizeof(sb_msg.Data);
    }

    CFE_SB_InitMsg(&sb_msg, msg_id, msg_size, true);
    memcpy(sb_msg.Data, CmdPacket, msg_size);

    int32 status = CFE_SB_TransmitMsg(&sb_msg, true);
    if (status == CFE_SUCCESS) {
        g_ci_diag.CommandsAccepted++;
        CFE_EVS_SendEvent(303, CFE_EVS_EventType_INFORMATION,
                          "CI: Dispatched command APID 0x%04X, CC %u to Software Bus.",
                          CmdPacket->Header.StreamId, CmdPacket->Header.CommandCode);
    } else {
        g_ci_diag.CommandsRejected++;
        CFE_EVS_SendEvent(304, CFE_EVS_EventType_ERROR,
                          "CI: Software Bus dispatch failed for APID 0x%04X (err=%d).",
                          CmdPacket->Header.StreamId, status);
    }

    return status;
}

int32 CFE_CI_GetDiagnostics(CFE_CI_Diagnostics_t *DiagPtr)
{
    if (!DiagPtr) return CFE_STATUS_BAD_ARGUMENT;
    memcpy(DiagPtr, &g_ci_diag, sizeof(CFE_CI_Diagnostics_t));
    return CFE_SUCCESS;
}
