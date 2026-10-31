#ifndef CFE_CI_H
#define CFE_CI_H

#include "../../include/common_types.h"
#include "cfe_sb.h"
#include "cfe_error.h"

#define CFE_CI_APP_NAME           "CFE_CI"
#define CFE_CI_MAX_INGEST_WORDS    64
#define CFE_CI_MAX_COMMANDS_QUEUED 16

/* CCSDS Telecommand Packet Header */
typedef struct
{
    uint16 StreamId;       /* Application Process Identifier (APID) */
    uint16 Sequence;       /* Packet sequence count / segmentation flags */
    uint16 Length;         /* Packet data length minus 1 */
    uint8  CommandCode;    /* Function/Command Code */
    uint8  Checksum;       /* Verification checksum */
} CFE_CI_CmdHeader_t;

typedef struct
{
    CFE_CI_CmdHeader_t Header;
    uint8              Payload[CFE_CI_MAX_INGEST_WORDS * 4];
} CFE_CI_IngestPacket_t;

typedef struct
{
    uint32 CommandsIngested;
    uint32 CommandsAccepted;
    uint32 CommandsRejected;
    uint32 InvalidCrcCount;
    uint32 StreamIdErrors;
} CFE_CI_Diagnostics_t;

/* Public CI Application API */
int32 CFE_CI_AppInit(void);
int32 CFE_CI_IngestFrame(const uint8 *FrameBuffer, uint32 FrameLen);
int32 CFE_CI_Dispatch(const CFE_CI_IngestPacket_t *CmdPacket);
int32 CFE_CI_GetDiagnostics(CFE_CI_Diagnostics_t *DiagPtr);

#endif /* CFE_CI_H */
