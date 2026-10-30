#ifndef CFE_EVS_H
#define CFE_EVS_H

#include "../../include/common_types.h"
#include "../../include/osapi.h"
#include "cfe_error.h"

/* Event Types */
#define CFE_EVS_EventType_DEBUG         1
#define CFE_EVS_EventType_INFORMATION   2
#define CFE_EVS_EventType_ERROR         3
#define CFE_EVS_EventType_CRITICAL      4

/* Filter Schemes */
#define CFE_EVS_NO_FILTER               0
#define CFE_EVS_FIRST_ONE_STOP          1
#define CFE_EVS_FIRST_TWO_STOP          2
#define CFE_EVS_BINARY_FILTER_MASK      3

#define CFE_EVS_MAX_AUDIT_LOGS          64
#define CFE_EVS_MAX_MSG_LEN             128

typedef struct
{
    uint32 Sequence;
    uint32 Timestamp;
    uint32 AppId;
    uint16 EventID;
    uint16 EventType;
    char   Message[CFE_EVS_MAX_MSG_LEN];
    uint8  AuditHash[32];
} CFE_EVS_AuditEntry_t;

/* Function Prototypes */
int32  CFE_EVS_EarlyInit(void);
int32  CFE_EVS_Register(const void *Filters, uint16 NumEventFilters, uint16 FilterScheme);
int32  CFE_EVS_Unregister(void);
int32  CFE_EVS_SendEvent(uint16 EventID, uint16 EventType, const char *Spec, ...);
int32  CFE_EVS_VerifyAuditChain(void);
uint32 CFE_EVS_GetAuditLogCount(void);
int32  CFE_EVS_InjectTamperFault(uint32 index);

#endif /* CFE_EVS_H */
