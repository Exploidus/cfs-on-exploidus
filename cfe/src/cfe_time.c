/*
 * cFE Time Services (TIME) - Exploidus Native Port Implementation
 *
 * Mission clock backed by OS_GetLocalTime() (SYS_UPTIME hardware counter).
 * Provides CCSDS CUC timestamps, MET, TAI, and UTC with leap-second correction.
 */

#include <stdio.h>
#include <string.h>
#include "../../include/osapi.h"
#include "../inc/cfe_time.h"

/* Internal clock state */
static struct {
    uint32             ClockState;
    CFE_TIME_SysTime_t MetOffset;
    int16              LeapSeconds;
    uint32             Initialized;
} g_time_state;

int32 CFE_TIME_EarlyInit(void)
{
    memset(&g_time_state, 0, sizeof(g_time_state));
    g_time_state.LeapSeconds  = CFE_TIME_LEAP_SECONDS;
    g_time_state.ClockState   = CFE_TIME_ClockState_FLYWHEEL;
    g_time_state.MetOffset.Seconds    = 0;
    g_time_state.MetOffset.Subseconds = 0;
    g_time_state.Initialized  = 1;
    return CFE_SUCCESS;
}

/*
 * CFE_TIME_GetMET -- Mission Elapsed Time.
 * Reads hardware monotonic uptime and applies MetOffset.
 */
CFE_TIME_SysTime_t CFE_TIME_GetMET(void)
{
    OS_time_t hw;
    OS_GetLocalTime(&hw);

    CFE_TIME_SysTime_t met;
    met.Seconds    = hw.seconds    + g_time_state.MetOffset.Seconds;
    /* Convert microseconds to 2^-32 subseconds: subsec = usec * (2^32 / 1e6) */
    uint64 subsec  = (uint64)hw.microsecs * 4295ULL;
    met.Subseconds = (uint32)(subsec & 0xFFFFFFFFULL)
                   + g_time_state.MetOffset.Subseconds;
    if (met.Subseconds < g_time_state.MetOffset.Subseconds)
        met.Seconds++; /* subsecond carry */
    return met;
}

/*
 * CFE_TIME_GetTAI -- International Atomic Time.
 * TAI = MET + GPS Epoch offset (no leap second subtraction).
 */
CFE_TIME_SysTime_t CFE_TIME_GetTAI(void)
{
    CFE_TIME_SysTime_t tai = CFE_TIME_GetMET();
    tai.Seconds += CFE_TIME_EPOCH_DIFF;
    return tai;
}

/*
 * CFE_TIME_GetUTC -- Coordinated Universal Time.
 * UTC = TAI - LeapSeconds
 */
CFE_TIME_SysTime_t CFE_TIME_GetUTC(void)
{
    CFE_TIME_SysTime_t utc = CFE_TIME_GetTAI();
    utc.Seconds -= (uint32)g_time_state.LeapSeconds;
    return utc;
}

CFE_TIME_SysTime_t CFE_TIME_GetTime(void)
{
    return CFE_TIME_GetMET();
}

uint32 CFE_TIME_GetClockState(void)
{
    return g_time_state.ClockState;
}

uint32 CFE_TIME_GetLeapSeconds(void)
{
    return (uint32)g_time_state.LeapSeconds;
}

CFE_TIME_SysTime_t CFE_TIME_Add(CFE_TIME_SysTime_t Time1, CFE_TIME_SysTime_t Time2)
{
    CFE_TIME_SysTime_t result;
    result.Subseconds = Time1.Subseconds + Time2.Subseconds;
    result.Seconds    = Time1.Seconds    + Time2.Seconds;
    if (result.Subseconds < Time1.Subseconds)
        result.Seconds++; /* overflow carry */
    return result;
}

CFE_TIME_SysTime_t CFE_TIME_Subtract(CFE_TIME_SysTime_t Time1, CFE_TIME_SysTime_t Time2)
{
    CFE_TIME_SysTime_t result;
    result.Seconds    = Time1.Seconds - Time2.Seconds;
    result.Subseconds = Time1.Subseconds - Time2.Subseconds;
    if (Time1.Subseconds < Time2.Subseconds)
        result.Seconds--; /* borrow */
    return result;
}

int32 CFE_TIME_Compare(CFE_TIME_SysTime_t Time1, CFE_TIME_SysTime_t Time2)
{
    if (Time1.Seconds > Time2.Seconds) return CFE_TIME_A_GT_B;
    if (Time1.Seconds < Time2.Seconds) return CFE_TIME_A_LT_B;
    if (Time1.Subseconds > Time2.Subseconds) return CFE_TIME_A_GT_B;
    if (Time1.Subseconds < Time2.Subseconds) return CFE_TIME_A_LT_B;
    return CFE_TIME_EQUAL;
}

CFE_TIME_SysTime_t CFE_TIME_MicroSecToSysTime(uint32 MicroSeconds)
{
    CFE_TIME_SysTime_t t;
    t.Seconds    = MicroSeconds / 1000000U;
    t.Subseconds = (uint32)(((uint64)(MicroSeconds % 1000000U)) * 4295ULL);
    return t;
}

uint32 CFE_TIME_SysTimeToMicroSec(CFE_TIME_SysTime_t SysTime)
{
    return SysTime.Seconds * 1000000U + (uint32)(SysTime.Subseconds / 4295ULL);
}

int32 CFE_TIME_SetLeapSeconds(int16 LeapSeconds)
{
    g_time_state.LeapSeconds = LeapSeconds;
    return CFE_SUCCESS;
}

int32 CFE_TIME_SetMETOffset(CFE_TIME_SysTime_t NewOffset)
{
    g_time_state.MetOffset = NewOffset;
    g_time_state.ClockState = CFE_TIME_ClockState_VALID;
    return CFE_SUCCESS;
}

void CFE_TIME_Print(char *PrintBuffer, CFE_TIME_SysTime_t TimeToPrint)
{
    uint32 usec = (uint32)(TimeToPrint.Subseconds / 4295ULL);
    snprintf(PrintBuffer, 32, "%010u.%06u", TimeToPrint.Seconds, usec);
}
