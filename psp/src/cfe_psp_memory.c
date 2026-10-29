#include <string.h>
#include "../inc/cfe_psp.h"

#define EXPLOIDUS_CDS_SIZE (128 * 1024) /* 128 KB Critical Data Store */

static uint8 g_exploidus_cds_memory[EXPLOIDUS_CDS_SIZE];

int32 CFE_PSP_GetCDSSize(uint32 *SizeOfCDS)
{
    if (SizeOfCDS == NULL) {
        return OS_INVALID_POINTER;
    }
    *SizeOfCDS = EXPLOIDUS_CDS_SIZE;
    return OS_SUCCESS;
}

int32 CFE_PSP_WriteToCDS(const void *PtrToDataToWrite, uint32 CDSOffset, uint32 NumBytes)
{
    if (PtrToDataToWrite == NULL) {
        return OS_INVALID_POINTER;
    }

    if ((CDSOffset + NumBytes) > EXPLOIDUS_CDS_SIZE) {
        return OS_ERROR;
    }

    memcpy(&g_exploidus_cds_memory[CDSOffset], PtrToDataToWrite, NumBytes);
    return OS_SUCCESS;
}

int32 CFE_PSP_ReadFromCDS(void *PtrToDataToRead, uint32 CDSOffset, uint32 NumBytes)
{
    if (PtrToDataToRead == NULL) {
        return OS_INVALID_POINTER;
    }

    if ((CDSOffset + NumBytes) > EXPLOIDUS_CDS_SIZE) {
        return OS_ERROR;
    }

    memcpy(PtrToDataToRead, &g_exploidus_cds_memory[CDSOffset], NumBytes);
    return OS_SUCCESS;
}
