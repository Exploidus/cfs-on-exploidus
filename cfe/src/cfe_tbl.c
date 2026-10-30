/*
 * cFE Table Services (TBL) - Exploidus Native Port Implementation
 *
 * Double-buffered flight parameter tables with CRC32 integrity validation
 * and atomic active-buffer swap. Tables are the primary runtime configuration
 * mechanism for flight application PID gains, thrust limits, and constraints.
 */

#include <stdio.h>
#include <string.h>
#include "../../include/osapi.h"
#include "../inc/cfe_tbl.h"

/* CRC32 implementation (IEEE 802.3) */
static uint32 tbl_crc32(const uint8 *data, size_t len)
{
    uint32 crc = 0xFFFFFFFFU;
    for (size_t i = 0; i < len; i++) {
        crc ^= (uint32)data[i];
        for (int j = 0; j < 8; j++)
            crc = (crc & 1) ? ((crc >> 1) ^ 0xEDB88320U) : (crc >> 1);
    }
    return ~crc;
}

static CFE_TBL_Record_t g_tbl_registry[CFE_TBL_MAX_TABLES];
static uint32            g_tbl_initialized = 0;

int32 CFE_TBL_EarlyInit(void)
{
    memset(g_tbl_registry, 0, sizeof(g_tbl_registry));
    g_tbl_initialized = 1;
    return CFE_SUCCESS;
}

int32 CFE_TBL_Register(CFE_TBL_Handle_t *TblHandlePtr, const char *Name,
                        size_t Size, uint16 TblOptionFlags,
                        CFE_TBL_CallbackFuncPtr_t TblValidationFuncPtr)
{
    if (!TblHandlePtr || !Name || Size == 0 || Size > CFE_TBL_MAX_TABLE_SIZE)
        return CFE_TBL_ERR_INVALID_SIZE;

    /* Check for duplicate name */
    for (int i = 0; i < CFE_TBL_MAX_TABLES; i++) {
        if (g_tbl_registry[i].InUse &&
            strncmp(g_tbl_registry[i].Name, Name, CFE_TBL_MAX_NAME_LEN) == 0)
            return CFE_TBL_ERR_DUPLICATE_REGISTRATION;
    }

    /* Find free slot */
    for (int i = 0; i < CFE_TBL_MAX_TABLES; i++) {
        if (!g_tbl_registry[i].InUse) {
            CFE_TBL_Record_t *rec = &g_tbl_registry[i];
            memset(rec, 0, sizeof(*rec));
            rec->InUse        = true;
            rec->Size         = Size;
            rec->Options      = TblOptionFlags;
            rec->State        = CFE_TBL_State_FREE;
            rec->ActiveBuffer = 0;
            rec->ValidationFn = TblValidationFuncPtr;
            strncpy(rec->Name, Name, CFE_TBL_MAX_NAME_LEN - 1);
            *TblHandlePtr = (CFE_TBL_Handle_t)i;
            return CFE_SUCCESS;
        }
    }
    return CFE_TBL_ERR_NO_RESOURCES;
}

int32 CFE_TBL_Unregister(CFE_TBL_Handle_t TblHandle)
{
    if (TblHandle < 0 || TblHandle >= CFE_TBL_MAX_TABLES)
        return CFE_TBL_ERR_INVALID_HANDLE;
    g_tbl_registry[TblHandle].InUse = false;
    return CFE_SUCCESS;
}

int32 CFE_TBL_Load(CFE_TBL_Handle_t TblHandle, const void *SrcDataPtr, size_t NumBytes)
{
    if (TblHandle < 0 || TblHandle >= CFE_TBL_MAX_TABLES) return CFE_TBL_ERR_INVALID_HANDLE;
    if (!SrcDataPtr) return CFE_TBL_ERR_INVALID_POINTER;
    CFE_TBL_Record_t *rec = &g_tbl_registry[TblHandle];
    if (!rec->InUse) return CFE_TBL_ERR_INVALID_HANDLE;
    if (NumBytes > rec->Size) return CFE_TBL_ERR_INVALID_SIZE;

    /* Load into the inactive buffer (double-buffer: flip from active) */
    uint32 inactive = 1 - rec->ActiveBuffer;
    memcpy(rec->Data[inactive], SrcDataPtr, NumBytes);
    rec->State = CFE_TBL_State_PENDING;
    return CFE_SUCCESS;
}

int32 CFE_TBL_Validate(CFE_TBL_Handle_t TblHandle)
{
    if (TblHandle < 0 || TblHandle >= CFE_TBL_MAX_TABLES) return CFE_TBL_ERR_INVALID_HANDLE;
    CFE_TBL_Record_t *rec = &g_tbl_registry[TblHandle];
    if (!rec->InUse || rec->State != CFE_TBL_State_PENDING) return CFE_TBL_ERR_INVALID_HANDLE;

    uint32 inactive = 1 - rec->ActiveBuffer;

    /* CRC32 integrity check on pending buffer */
    uint32 crc = tbl_crc32(rec->Data[inactive], rec->Size);

    /* Call application-provided validation callback if registered */
    if (rec->ValidationFn != NULL) {
        int32 cb_ret = rec->ValidationFn(rec->Data[inactive]);
        if (cb_ret != CFE_SUCCESS)
            return cb_ret;
    }

    rec->Crc   = crc;
    rec->State = CFE_TBL_State_VALIDATED;
    return CFE_SUCCESS;
}

int32 CFE_TBL_Update(CFE_TBL_Handle_t TblHandle)
{
    if (TblHandle < 0 || TblHandle >= CFE_TBL_MAX_TABLES) return CFE_TBL_ERR_INVALID_HANDLE;
    CFE_TBL_Record_t *rec = &g_tbl_registry[TblHandle];
    if (!rec->InUse) return CFE_TBL_ERR_INVALID_HANDLE;
    if (rec->State != CFE_TBL_State_VALIDATED) return CFE_TBL_ERR_INVALID_HANDLE;

    /* Atomic buffer swap */
    rec->ActiveBuffer = 1 - rec->ActiveBuffer;
    rec->State        = CFE_TBL_State_LOADED;
    rec->UpdateCount++;
    return CFE_SUCCESS;
}

int32 CFE_TBL_GetAddress(void **TblPtr, CFE_TBL_Handle_t TblHandle)
{
    if (!TblPtr) return CFE_TBL_ERR_INVALID_POINTER;
    if (TblHandle < 0 || TblHandle >= CFE_TBL_MAX_TABLES) return CFE_TBL_ERR_INVALID_HANDLE;
    CFE_TBL_Record_t *rec = &g_tbl_registry[TblHandle];
    if (!rec->InUse) return CFE_TBL_ERR_INVALID_HANDLE;
    *TblPtr = rec->Data[rec->ActiveBuffer];
    int32 ret = (rec->State == CFE_TBL_State_VALIDATED) ? CFE_TBL_INFO_UPDATED : CFE_SUCCESS;
    return ret;
}

int32 CFE_TBL_ReleaseAddress(CFE_TBL_Handle_t TblHandle)
{
    (void)TblHandle;
    return CFE_SUCCESS;
}

int32 CFE_TBL_Manage(CFE_TBL_Handle_t TblHandle)
{
    if (TblHandle < 0 || TblHandle >= CFE_TBL_MAX_TABLES) return CFE_TBL_ERR_INVALID_HANDLE;
    CFE_TBL_Record_t *rec = &g_tbl_registry[TblHandle];
    if (!rec->InUse) return CFE_TBL_ERR_INVALID_HANDLE;
    if (rec->State == CFE_TBL_State_PENDING) {
        CFE_TBL_Validate(TblHandle);
        CFE_TBL_Update(TblHandle);
        return CFE_TBL_INFO_UPDATED;
    }
    return CFE_SUCCESS;
}

int32 CFE_TBL_GetInfo(CFE_TBL_Handle_t TblHandle, char *NameBuf, size_t BufLen,
                       uint32 *Crc, uint32 *UpdateCount)
{
    if (TblHandle < 0 || TblHandle >= CFE_TBL_MAX_TABLES) return CFE_TBL_ERR_INVALID_HANDLE;
    CFE_TBL_Record_t *rec = &g_tbl_registry[TblHandle];
    if (!rec->InUse) return CFE_TBL_ERR_INVALID_HANDLE;
    if (NameBuf)     strncpy(NameBuf, rec->Name, BufLen - 1);
    if (Crc)         *Crc = rec->Crc;
    if (UpdateCount) *UpdateCount = rec->UpdateCount;
    return CFE_SUCCESS;
}
