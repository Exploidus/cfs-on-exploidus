#ifndef CFE_TO_H
#define CFE_TO_H

#include "../../include/common_types.h"
#include "cfe_sb.h"
#include "cfe_error.h"

#define CFE_TO_APP_NAME            "CFE_TO"
#define CFE_TO_MAX_SUBSCRIPTIONS    16
#define CFE_TO_DOWNLINK_BUFFER_SIZE 1024

/* CCSDS Telemetry Packet Secondary Header (Time Tagged) */
typedef struct
{
    uint16 StreamId;       /* Telemetry Stream APID */
    uint16 Sequence;       /* Sequence Count */
    uint16 Length;         /* Packet Data Length minus 1 */
    uint32 Seconds;        /* Mission Elapsed Seconds */
    uint16 Subseconds;     /* 1/65536 fractions of second */
} CFE_TO_TlmHeader_t;

typedef struct
{
    uint32 PacketsProcessed;
    uint32 PacketsDownlinked;
    uint32 DownlinkBytesSent;
    uint32 DroppedPackets;
} CFE_TO_Diagnostics_t;

/* Public TO Application API */
int32 CFE_TO_AppInit(void);
int32 CFE_TO_SubscribeStream(CFE_SB_MsgId_t MsgId);
int32 CFE_TO_ProcessDownlink(void);
int32 CFE_TO_GetDiagnostics(CFE_TO_Diagnostics_t *DiagPtr);

#endif /* CFE_TO_H */
