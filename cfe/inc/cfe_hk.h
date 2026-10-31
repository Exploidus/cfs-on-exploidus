#ifndef CFE_HK_H
#define CFE_HK_H

#include "../../include/common_types.h"
#include "cfe_sb.h"
#include "cfe_error.h"

#define CFE_HK_APP_NAME            "CFE_HK"
#define CFE_HK_MAX_APPS             16

/* System-Wide Housekeeping Packet */
typedef struct
{
    uint32 ActiveAppCount;
    uint32 TotalMemoryPoolAllocated;
    uint32 TotalMemoryPoolFree;
    uint32 TotalEventsSent;
    uint32 TotalMessagesRouted;
    uint32 CpuUsagePercent;
    uint32 WatchdogResets;
} CFE_HK_CombinedTlm_t;

typedef struct
{
    uint32 HkWakeupsProcessed;
    uint32 TlmPacketsGenerated;
} CFE_HK_Diagnostics_t;

/* Public HK Application API */
int32 CFE_HK_AppInit(void);
int32 CFE_HK_CollectAndReport(CFE_HK_CombinedTlm_t *TlmPtr);
int32 CFE_HK_GetDiagnostics(CFE_HK_Diagnostics_t *DiagPtr);

#endif /* CFE_HK_H */
