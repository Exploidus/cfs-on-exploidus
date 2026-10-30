#ifndef CFE_TIME_H
#define CFE_TIME_H

#include "../../include/common_types.h"
#include "../../include/osapi.h"
#include "cfe_error.h"

/*
 * cFE Time Services (TIME) - Exploidus Native Port
 *
 * Provides mission-monotonic spacecraft clock synchronized to hardware
 * uptime counter (SYS_UPTIME). All timestamps are leap-second corrected
 * and expressed in CCSDS Unsegmented Time Code (CUC) format.
 */

/* Time format constants */
#define CFE_TIME_FS_FACTOR          4295  /* Subsecond to microsecond conversion */
#define CFE_TIME_EPOCH_DIFF         315532800U /* GPS to Unix epoch delta (sec) */
#define CFE_TIME_LEAP_SECONDS       18          /* Current GPS-UTC leap seconds */

/* Clock states */
#define CFE_TIME_ClockState_INVALID     0   /* Clock not yet synchronized */
#define CFE_TIME_ClockState_VALID       1   /* Clock synchronized to mission reference */
#define CFE_TIME_ClockState_FLYWHEEL    2   /* Flying on internal oscillator */

/* Time comparison results */
#define CFE_TIME_EQUAL              0
#define CFE_TIME_A_GT_B             1
#define CFE_TIME_A_LT_B             (-1)

/*
 * CCSDS Unsegmented Time Code (CUC) structure.
 * Seconds since GPS epoch + subseconds (1/2^32 second resolution).
 */
typedef struct
{
    uint32 Seconds;     /* Whole seconds since GPS epoch */
    uint32 Subseconds;  /* Fractional seconds (units of 2^-32 s) */
} CFE_TIME_SysTime_t;

typedef struct
{
    uint32              ClockState;
    CFE_TIME_SysTime_t  AtToneDelay;     /* Tone signal propagation delay */
    CFE_TIME_SysTime_t  MetOffset;       /* Mission Elapsed Time offset */
    uint32              LeapSeconds;     /* Current GPS-UTC leap second count */
    uint32              ClockSignal;     /* Primary / Redundant tone source */
} CFE_TIME_Reference_t;

/* Function Prototypes */
int32              CFE_TIME_EarlyInit(void);
CFE_TIME_SysTime_t CFE_TIME_GetTime(void);
CFE_TIME_SysTime_t CFE_TIME_GetMET(void);
CFE_TIME_SysTime_t CFE_TIME_GetTAI(void);
CFE_TIME_SysTime_t CFE_TIME_GetUTC(void);
uint32             CFE_TIME_GetClockState(void);
uint32             CFE_TIME_GetLeapSeconds(void);
CFE_TIME_SysTime_t CFE_TIME_Add(CFE_TIME_SysTime_t Time1, CFE_TIME_SysTime_t Time2);
CFE_TIME_SysTime_t CFE_TIME_Subtract(CFE_TIME_SysTime_t Time1, CFE_TIME_SysTime_t Time2);
int32              CFE_TIME_Compare(CFE_TIME_SysTime_t Time1, CFE_TIME_SysTime_t Time2);
CFE_TIME_SysTime_t CFE_TIME_MicroSecToSysTime(uint32 MicroSeconds);
uint32             CFE_TIME_SysTimeToMicroSec(CFE_TIME_SysTime_t SysTime);
int32              CFE_TIME_SetLeapSeconds(int16 LeapSeconds);
int32              CFE_TIME_SetMETOffset(CFE_TIME_SysTime_t NewOffset);
void               CFE_TIME_Print(char *PrintBuffer, CFE_TIME_SysTime_t TimeToPrint);

#endif /* CFE_TIME_H */
