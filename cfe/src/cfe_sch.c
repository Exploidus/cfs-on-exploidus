/*
 * cFS Scheduler Application (SCH) - Exploidus Native Port Implementation
 *
 * Deterministic 100 Hz minor-frame scheduler backed by SYS_UPTIME hardware
 * monotonic timer with INTENT_REALTIME priority class.
 *
 * Each minor frame (10ms), the scheduler iterates the activity table and
 * transmits wakeup messages to registered flight tasks via the Software Bus.
 * Activities are phase-offset to avoid thundering herd at frame boundaries.
 */

#include <stdio.h>
#include <string.h>
#include "../../include/osapi.h"
#include "../inc/cfe_es.h"
#include "../inc/cfe_sch.h"
#include "../inc/cfe_sb.h"
#include "../inc/cfe_evs.h"

#define SCH_APP_NAME "CFE_SCH"

static CFE_SCH_ScheduleTable_t g_sch_table;
static CFE_SCH_DiagPacket_t    g_sch_diag;
static uint32                  g_sch_initialized = 0;

int32 CFE_SCH_AppInit(void)
{
    memset(&g_sch_table, 0, sizeof(g_sch_table));
    memset(&g_sch_diag,  0, sizeof(g_sch_diag));
    g_sch_initialized = 1;
    CFE_EVS_SendEvent(200, CFE_EVS_EventType_INFORMATION,
                      "SCH: Scheduler App initialized (100Hz, %d activity slots)",
                      CFE_SCH_MAX_ACTIVITIES);
    return CFE_SUCCESS;
}

int32 CFE_SCH_LoadTable(const CFE_SCH_ScheduleTable_t *TablePtr)
{
    if (!TablePtr) return CFE_SCH_ERR_INVALID_TABLE;
    memcpy(&g_sch_table, TablePtr, sizeof(CFE_SCH_ScheduleTable_t));
    CFE_EVS_SendEvent(201, CFE_EVS_EventType_INFORMATION,
                      "SCH: Schedule table loaded successfully.");
    return CFE_SUCCESS;
}

int32 CFE_SCH_EnableActivity(uint32 SlotIndex)
{
    if (SlotIndex >= CFE_SCH_MAX_ACTIVITIES) return CFE_SCH_ERR_INVALID_SLOT;
    g_sch_table.Activity[SlotIndex].EnableState = CFE_SCH_ENABLED;
    return CFE_SUCCESS;
}

int32 CFE_SCH_DisableActivity(uint32 SlotIndex)
{
    if (SlotIndex >= CFE_SCH_MAX_ACTIVITIES) return CFE_SCH_ERR_INVALID_SLOT;
    g_sch_table.Activity[SlotIndex].EnableState = CFE_SCH_DISABLED;
    return CFE_SUCCESS;
}

int32 CFE_SCH_GetDiagnostics(CFE_SCH_DiagPacket_t *DiagPtr)
{
    if (!DiagPtr) return CFE_SCH_ERR_INVALID_TABLE;
    memcpy(DiagPtr, &g_sch_diag, sizeof(CFE_SCH_DiagPacket_t));
    return CFE_SUCCESS;
}

/*
 * CFE_SCH_ProcessMinorFrame -- called once per 10ms minor frame.
 * Iterates all activity slots and fires wakeup messages on schedule.
 */
static void CFE_SCH_ProcessMinorFrame(uint32 minor_frame)
{
    for (uint32 i = 0; i < CFE_SCH_MAX_ACTIVITIES; i++) {
        CFE_SCH_ActivityEntry_t *act = &g_sch_table.Activity[i];

        if (act->EnableState != CFE_SCH_ENABLED) continue;
        if (act->MessageId == CFE_SCH_UNUSED_MID) continue;
        if (act->Frequency == 0) continue;

        /* Fire when (minor_frame - offset) is divisible by frequency */
        uint32 adjusted = (minor_frame + CFE_SCH_MINOR_FRAMES - act->Offset)
                          % CFE_SCH_MINOR_FRAMES;
        if ((adjusted % act->Frequency) != 0) continue;

        /* Transmit wakeup message on Software Bus */
        CFE_SB_Buffer_t wakeup_msg;
        CFE_SB_InitMsg(&wakeup_msg, act->MessageId,
                       sizeof(uint32) * CFE_SCH_MAX_MSG_WORDS, true);
        memcpy(wakeup_msg.Data, act->MessageBuffer,
               sizeof(uint32) * CFE_SCH_MAX_MSG_WORDS);

        if (CFE_SB_TransmitMsg(&wakeup_msg, true) == CFE_SUCCESS)
            g_sch_diag.ScheduledActivities++;
        else
            g_sch_diag.SkippedActivities++;
    }
}

/*
 * CFE_SCH_AppMain -- Scheduler main loop.
 * Runs at INTENT_REALTIME priority on Exploidus OS.
 * Each iteration sleeps for one minor frame period (10ms) then processes.
 */
void CFE_SCH_AppMain(void)
{
    CFE_SCH_AppInit();
    uint32 run_status = CFE_ES_RunStatus_APP_RUN;

    while (CFE_ES_RunLoop(&run_status)) {
        /* Sleep for one minor frame (10ms) */
        OS_TaskDelay(CFE_SCH_MINOR_FRAME_MS);

        uint32 minor_frame = g_sch_diag.MinorFrameCount % CFE_SCH_MINOR_FRAMES;
        CFE_SCH_ProcessMinorFrame(minor_frame);

        g_sch_diag.MinorFrameCount++;
        if (minor_frame == CFE_SCH_MINOR_FRAMES - 1)
            g_sch_diag.MajorFrameCount++;
    }
}