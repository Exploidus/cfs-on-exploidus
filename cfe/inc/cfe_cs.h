#ifndef CFE_CS_H
#define CFE_CS_H

#include "../../include/common_types.h"
#include "cfe_error.h"

#define CFE_CS_APP_NAME            "CFE_CS"
#define CFE_CS_MAX_REGIONS          8

typedef enum
{
    CFE_CS_STATE_EMPTY = 0,
    CFE_CS_STATE_ENABLED,
    CFE_CS_STATE_DISABLED
} CFE_CS_RegionState_t;

typedef struct
{
    char                 Name[32];
    const void          *Address;
    uint32               Size;
    uint32               ExpectedCrc;
    uint32               LastCalculatedCrc;
    CFE_CS_RegionState_t State;
} CFE_CS_MemoryRegion_t;

typedef struct
{
    uint32 TotalScansCompleted;
    uint32 TotalIntegrityChecksPassed;
    uint32 BitFlipAnomaliesDetected;
    uint32 ActiveRegionsMonitored;
} CFE_CS_Diagnostics_t;

/* Public CS Application API */
int32 CFE_CS_AppInit(void);
int32 CFE_CS_RegisterRegion(const char *Name, const void *Address, uint32 Size, uint32 *RegionIndex);
int32 CFE_CS_ScanRegion(uint32 RegionIndex);
int32 CFE_CS_ScanAll(void);
int32 CFE_CS_GetDiagnostics(CFE_CS_Diagnostics_t *DiagPtr);

#endif /* CFE_CS_H */
