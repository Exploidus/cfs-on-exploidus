#ifndef CFE_SCH_H
#define CFE_SCH_H

#include "../../include/common_types.h"
#include "../../include/osapi.h"
#include "cfe_error.h"
#include "cfe_sb.h"

/*
 * cFS Scheduler Application (SCH) - Exploidus Native Port
 *
 * Provides deterministic minor-frame based scheduling for flight applications.
 * The scheduler fires at a configurable tick rate (default: 100 Hz / 10ms frames)
 * and dispatches wakeup messages to registered flight tasks.
 *
 * On Exploidus OS, the minor frame timer is backed by SYS_UPTIME hardware
 * counter with INTENT_REALTIME priority class -- immune to background jitter.
 */

#define CFE_SCH_MAJOR_FRAME_HZ      1     /* Major frames per second */
#define CFE_SCH_MINOR_FRAMES        100   /* Minor frames per major frame (100Hz) */
#define CFE_SCH_MINOR_FRAME_MS      10    /* Minor frame period in milliseconds */
#define CFE_SCH_MAX_ACTIVITIES      32    /* Maximum scheduled activities */
#define CFE_SCH_MAX_MSG_WORDS       4     /* Max message size in 32-bit words */
#define CFE_SCH_UNUSED_MID          0x0000

/* Activity enable/disable states */
#define CFE_SCH_ENABLED             1
#define CFE_SCH_DISABLED            0

/* Activity frequency (every N minor frames) */
#define CFE_SCH_ACTIVITY_1HZ        100
#define CFE_SCH_ACTIVITY_2HZ        50
#define CFE_SCH_ACTIVITY_4HZ        25
#define CFE_SCH_ACTIVITY_5HZ        20
#define CFE_SCH_ACTIVITY_10HZ       10
#define CFE_SCH_ACTIVITY_25HZ       4
#define CFE_SCH_ACTIVITY_50HZ       2
#define CFE_SCH_ACTIVITY_100HZ      1

/*
 * Scheduler table entry -- one row per scheduled activity.
 * Loaded via TBL services at startup; supports in-flight parameter update.
 */
typedef struct
{
    uint32          EnableState;        /* CFE_SCH_ENABLED or CFE_SCH_DISABLED */
    uint32          Frequency;          /* Fire every N minor frames */
    uint32          Offset;             /* Initial minor frame phase offset */
    CFE_SB_MsgId_t  MessageId;          /* Wakeup message MsgId to transmit */
    uint32          MessageBuffer[CFE_SCH_MAX_MSG_WORDS]; /* Wakeup message payload */
} CFE_SCH_ActivityEntry_t;

typedef struct
{
    CFE_SCH_ActivityEntry_t Activity[CFE_SCH_MAX_ACTIVITIES];
} CFE_SCH_ScheduleTable_t;

typedef struct
{
    uint32  MajorFrameCount;
    uint32  MinorFrameCount;
    uint32  ScheduledActivities;
    uint32  MissedMajorFrames;
    uint32  SkippedActivities;
} CFE_SCH_DiagPacket_t;

/* Function Prototypes */
int32 CFE_SCH_AppInit(void);
int32 CFE_SCH_LoadTable(const CFE_SCH_ScheduleTable_t *TablePtr);
int32 CFE_SCH_EnableActivity(uint32 SlotIndex);
int32 CFE_SCH_DisableActivity(uint32 SlotIndex);
int32 CFE_SCH_GetDiagnostics(CFE_SCH_DiagPacket_t *DiagPtr);
void  CFE_SCH_AppMain(void);

#endif /* CFE_SCH_H */
