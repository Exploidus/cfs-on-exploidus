#ifndef CFE_TBL_H
#define CFE_TBL_H

#include "../../include/common_types.h"
#include "../../include/osapi.h"
#include "cfe_error.h"

/*
 * cFE Table Services (TBL) - Exploidus Native Port
 *
 * Manages flight parameter tables with atomic double-buffered updates,
 * CRC32 integrity validation, and capability-gated write access.
 * Tables are the primary mechanism for uploading mission parameters
 * (PID gains, thrust limits, attitude constraints) without restarting apps.
 */

#define CFE_TBL_MAX_TABLES          16
#define CFE_TBL_MAX_NAME_LEN        32
#define CFE_TBL_MAX_TABLE_SIZE      4096  /* bytes */
#define CFE_TBL_MAX_ADDRESSES       8

/* Table option flags */
#define CFE_TBL_OPT_DEFAULT         0x0000
#define CFE_TBL_OPT_SNGL_BUFFER     0x0001  /* Single-buffer (no double-buffer) */
#define CFE_TBL_OPT_DBL_BUFFER      0x0002  /* Double-buffered atomic swap */
#define CFE_TBL_OPT_DUMP_ONLY       0x0004  /* Read-only / telemetry dump */

/* Table states */
#define CFE_TBL_State_FREE          0
#define CFE_TBL_State_LOADED        1
#define CFE_TBL_State_PENDING       2   /* Update loaded, awaiting validation */
#define CFE_TBL_State_VALIDATED     3   /* Pending update passed CRC check */

/* Return codes */
#define CFE_TBL_INFO_UPDATED        (CFE_SEVERITY_INFO | 0x0001)
#define CFE_TBL_INFO_DUMP_PENDING   (CFE_SEVERITY_INFO | 0x0002)

typedef int32 CFE_TBL_Handle_t;
#define CFE_TBL_BAD_TABLE_HANDLE    (-1)

typedef int32 (*CFE_TBL_CallbackFuncPtr_t)(void *TblPtr);

typedef struct
{
    bool                    InUse;
    char                    Name[CFE_TBL_MAX_NAME_LEN];
    size_t                  Size;
    uint32                  Options;
    uint32                  State;
    uint32                  Crc;            /* CRC32 of active buffer */
    uint32                  ActiveBuffer;   /* 0 or 1 for double-buffer */
    uint8                   Data[2][CFE_TBL_MAX_TABLE_SIZE]; /* double buffer */
    CFE_TBL_CallbackFuncPtr_t ValidationFn;
    uint32                  UpdateCount;
    uint32                  OwnerAppId;
} CFE_TBL_Record_t;

/* Function Prototypes */
int32 CFE_TBL_EarlyInit(void);
int32 CFE_TBL_Register(CFE_TBL_Handle_t *TblHandlePtr, const char *Name,
                        size_t Size, uint16 TblOptionFlags,
                        CFE_TBL_CallbackFuncPtr_t TblValidationFuncPtr);
int32 CFE_TBL_Unregister(CFE_TBL_Handle_t TblHandle);
int32 CFE_TBL_Load(CFE_TBL_Handle_t TblHandle, const void *SrcDataPtr, size_t NumBytes);
int32 CFE_TBL_Update(CFE_TBL_Handle_t TblHandle);
int32 CFE_TBL_GetAddress(void **TblPtr, CFE_TBL_Handle_t TblHandle);
int32 CFE_TBL_ReleaseAddress(CFE_TBL_Handle_t TblHandle);
int32 CFE_TBL_Validate(CFE_TBL_Handle_t TblHandle);
int32 CFE_TBL_Manage(CFE_TBL_Handle_t TblHandle);
int32 CFE_TBL_GetInfo(CFE_TBL_Handle_t TblHandle, char *NameBuf, size_t BufLen,
                       uint32 *Crc, uint32 *UpdateCount);

#endif /* CFE_TBL_H */
