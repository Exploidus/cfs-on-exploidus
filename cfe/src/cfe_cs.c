/*
 * NASA cFS Checksum (CS) Application - Exploidus Native Port
 *
 * Continuously computes CRC32 checksums over registered flight code
 * and static configuration memory tables to detect radiation-induced
 * Single Event Upset (SEU) bit-flips in orbit.
 */

#include <stdio.h>
#include <string.h>
#include "../../include/osapi.h"
#include "../inc/cfe_cs.h"
#include "../inc/cfe_evs.h"

static CFE_CS_MemoryRegion_t g_cs_regions[CFE_CS_MAX_REGIONS];
static CFE_CS_Diagnostics_t  g_cs_diag;
static uint32                g_cs_initialized = 0;

/* Standard IEEE 802.3 CRC32 algorithm */
static uint32 CFE_CS_ComputeCrc32(const uint8 *data, uint32 length)
{
    uint32 crc = 0xFFFFFFFF;
    for (uint32 i = 0; i < length; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++) {
            crc = (crc >> 1) ^ (0xEDB88320 & -(crc & 1));
        }
    }
    return ~crc;
}

int32 CFE_CS_AppInit(void)
{
    memset(g_cs_regions, 0, sizeof(g_cs_regions));
    memset(&g_cs_diag, 0, sizeof(g_cs_diag));
    g_cs_initialized = 1;
    CFE_EVS_SendEvent(330, CFE_EVS_EventType_INFORMATION,
                      "CS: Checksum Integrity audit application initialized.");
    return CFE_SUCCESS;
}

int32 CFE_CS_RegisterRegion(const char *Name, const void *Address, uint32 Size, uint32 *RegionIndex)
{
    if (!g_cs_initialized || !Name || !Address || Size == 0 || !RegionIndex) {
        return CFE_STATUS_BAD_ARGUMENT;
    }

    for (uint32 i = 0; i < CFE_CS_MAX_REGIONS; i++) {
        if (g_cs_regions[i].State == CFE_CS_STATE_EMPTY) {
            strncpy(g_cs_regions[i].Name, Name, sizeof(g_cs_regions[i].Name) - 1);
            g_cs_regions[i].Name[sizeof(g_cs_regions[i].Name) - 1] = '\0';
            g_cs_regions[i].Address           = Address;
            g_cs_regions[i].Size              = Size;
            g_cs_regions[i].ExpectedCrc       = CFE_CS_ComputeCrc32((const uint8 *)Address, Size);
            g_cs_regions[i].LastCalculatedCrc = g_cs_regions[i].ExpectedCrc;
            g_cs_regions[i].State             = CFE_CS_STATE_ENABLED;

            g_cs_diag.ActiveRegionsMonitored++;
            *RegionIndex = i;

            CFE_EVS_SendEvent(331, CFE_EVS_EventType_INFORMATION,
                              "CS: Memory region '%s' (%p, %u B) registered, initial CRC=0x%08X.",
                              Name, Address, Size, g_cs_regions[i].ExpectedCrc);
            return CFE_SUCCESS;
        }
    }

    return CFE_STATUS_NO_RESOURCE;
}

int32 CFE_CS_ScanRegion(uint32 RegionIndex)
{
    if (!g_cs_initialized || RegionIndex >= CFE_CS_MAX_REGIONS) {
        return CFE_STATUS_BAD_ARGUMENT;
    }

    CFE_CS_MemoryRegion_t *reg = &g_cs_regions[RegionIndex];
    if (reg->State != CFE_CS_STATE_ENABLED) {
        return CFE_SUCCESS;
    }

    const uint8 *mem = (const uint8 *)reg->Address;
    uint32 current_crc = CFE_CS_ComputeCrc32(mem, reg->Size);
    reg->LastCalculatedCrc = current_crc;

    if (current_crc != reg->ExpectedCrc) {
        g_cs_diag.BitFlipAnomaliesDetected++;
        CFE_EVS_SendEvent(332, CFE_EVS_EventType_CRITICAL,
                          "CS [ANOMALY DETECTED]: Bit-flip/corruption in region '%s'! Expected 0x%08X, Computed 0x%08X.",
                          reg->Name, reg->ExpectedCrc, current_crc);
        return CFE_STATUS_CHECKSUM_ERROR;
    }

    g_cs_diag.TotalIntegrityChecksPassed++;
    return CFE_SUCCESS;
}

int32 CFE_CS_ScanAll(void)
{
    if (!g_cs_initialized) return CFE_STATUS_BAD_ARGUMENT;

    int32 result = CFE_SUCCESS;
    for (uint32 i = 0; i < CFE_CS_MAX_REGIONS; i++) {
        if (g_cs_regions[i].State == CFE_CS_STATE_ENABLED) {
            int32 status = CFE_CS_ScanRegion(i);
            if (status != CFE_SUCCESS) {
                result = status;
            }
        }
    }

    g_cs_diag.TotalScansCompleted++;
    return result;
}

int32 CFE_CS_GetDiagnostics(CFE_CS_Diagnostics_t *DiagPtr)
{
    if (!DiagPtr) return CFE_STATUS_BAD_ARGUMENT;
    memcpy(DiagPtr, &g_cs_diag, sizeof(CFE_CS_Diagnostics_t));
    return CFE_SUCCESS;
}
