#include <stdio.h>
#include <stdlib.h>
#include "../inc/cfe_psp.h"

static uint32 g_reset_type = CFE_PSP_RST_TYPE_POWERON;
static uint32 g_reset_subtype = CFE_PSP_RST_SUBTYPE_POWER_CYCLE;

uint32 CFE_PSP_GetResetType(uint32 *ResetSubtype)
{
    if (ResetSubtype != NULL) {
        *ResetSubtype = g_reset_subtype;
    }
    return g_reset_type;
}

uint32 CFE_PSP_GetProcessorId(void)
{
    return EXPLOIDUS_PROCESSOR_ID;
}

uint32 CFE_PSP_GetSpacecraftId(void)
{
    return EXPLOIDUS_SPACECRAFT_ID;
}

void CFE_PSP_Restart(uint32 reset_type)
{
    OS_printf("[PSP-EXPLOIDUS] Flight Computer Reset Requested (Type: %u)\n", reset_type);
    g_reset_type = reset_type;
    g_reset_subtype = CFE_PSP_RST_SUBTYPE_HW_SPECIAL_RESET;
}

void CFE_PSP_Panic(int32 ErrorCode)
{
    OS_printf("[PSP-EXPLOIDUS CRITICAL PANIC] Kernel Halt! ErrorCode: %d\n", ErrorCode);
    exit(1);
}
