#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "../inc/cfe_es.h"
#include "../inc/cfe_evs.h"
#include "../inc/cfe_sb.h"
#include "../../psp/inc/cfe_psp.h"

typedef struct
{
    bool   InUse;
    uint8  *Buffer;
    size_t TotalSize;
    size_t Allocated;
} CFE_ES_MemPoolEntry_t;

static CFE_ES_AppRecord_t   g_es_apps[CFE_ES_MAX_APPLICATIONS];
static CFE_ES_MemPoolEntry_t g_es_mempools[CFE_ES_MAX_MEMPOOLS];
static osal_id_t            g_es_mutex;
static bool                 g_es_initialized = false;
static uint32               g_es_app_counter = 0;

int32 CFE_ES_EarlyInit(void)
{
    if (g_es_initialized) {
        return CFE_SUCCESS;
    }

    memset(g_es_apps, 0, sizeof(g_es_apps));
    memset(g_es_mempools, 0, sizeof(g_es_mempools));
    g_es_app_counter = 0;

    int32 status = OS_MutSemCreate(&g_es_mutex, "CFE_ES_MUTEX", 0);
    if (status != OS_SUCCESS) {
        return status;
    }

    g_es_initialized = true;
    OS_printf("[CFE_ES] Executive Services (ES) core service initialized.\n");
    return CFE_SUCCESS;
}

int32 CFE_ES_RegisterApp(void)
{
    osal_id_t tid = OS_TaskGetId();

    OS_MutSemTake(g_es_mutex);

    /* Check if already registered */
    for (uint32 i = 0; i < CFE_ES_MAX_APPLICATIONS; i++) {
        if (g_es_apps[i].InUse && g_es_apps[i].TaskId == tid) {
            OS_MutSemGive(g_es_mutex);
            return CFE_SUCCESS;
        }
    }

    /* Allocate slot */
    for (uint32 i = 0; i < CFE_ES_MAX_APPLICATIONS; i++) {
        if (!g_es_apps[i].InUse) {
            g_es_app_counter++;
            g_es_apps[i].InUse = true;
            g_es_apps[i].AppId = g_es_app_counter;
            g_es_apps[i].TaskId = tid;
            g_es_apps[i].State = CFE_ES_AppState_RUNNING;
            g_es_apps[i].ExecutionCounter = 0;
            snprintf(g_es_apps[i].AppName, sizeof(g_es_apps[i].AppName), "APP_%u", g_es_apps[i].AppId);

            OS_MutSemGive(g_es_mutex);
            return CFE_SUCCESS;
        }
    }

    OS_MutSemGive(g_es_mutex);
    return CFE_ES_MAX_APPLICATIONS_MET;
}

int32 CFE_ES_GetAppID(uint32 *AppIdPtr)
{
    if (!AppIdPtr) {
        return CFE_ES_BAD_ARGUMENT;
    }

    osal_id_t tid = OS_TaskGetId();

    OS_MutSemTake(g_es_mutex);
    for (uint32 i = 0; i < CFE_ES_MAX_APPLICATIONS; i++) {
        if (g_es_apps[i].InUse && g_es_apps[i].TaskId == tid) {
            *AppIdPtr = g_es_apps[i].AppId;
            OS_MutSemGive(g_es_mutex);
            return CFE_SUCCESS;
        }
    }
    OS_MutSemGive(g_es_mutex);

    /* Default fallback */
    *AppIdPtr = (uint32)tid;
    return CFE_SUCCESS;
}

int32 CFE_ES_GetAppName(char *AppNamePtr, uint32 AppId, size_t BufferLength)
{
    if (!AppNamePtr || BufferLength == 0) {
        return CFE_ES_BAD_ARGUMENT;
    }

    OS_MutSemTake(g_es_mutex);
    for (uint32 i = 0; i < CFE_ES_MAX_APPLICATIONS; i++) {
        if (g_es_apps[i].InUse && g_es_apps[i].AppId == AppId) {
            strncpy(AppNamePtr, g_es_apps[i].AppName, BufferLength - 1);
            AppNamePtr[BufferLength - 1] = '\0';
            OS_MutSemGive(g_es_mutex);
            return CFE_SUCCESS;
        }
    }
    OS_MutSemGive(g_es_mutex);
    return CFE_ES_ERR_APP_NOT_FOUND;
}

int32 CFE_ES_CreateChildTask(uint32 *TaskIdPtr, const char *TaskName, void (*FunctionPtr)(void), void *StackPtr, size_t StackSize, uint32 Priority, uint32 Flags)
{
    (void)StackPtr;
    (void)Flags;

    osal_id_t osal_task;
    int32 status = OS_TaskCreate(&osal_task, TaskName, FunctionPtr, NULL, StackSize, Priority, 0);
    if (status == OS_SUCCESS && TaskIdPtr) {
        *TaskIdPtr = (uint32)osal_task;
    }
    return status;
}

int32 CFE_ES_DeleteChildTask(uint32 TaskId)
{
    return OS_TaskDelete((osal_id_t)TaskId);
}

void CFE_ES_ExitApp(uint32 ExitStatus)
{
    (void)ExitStatus;
    osal_id_t tid = OS_TaskGetId();

    OS_MutSemTake(g_es_mutex);
    for (uint32 i = 0; i < CFE_ES_MAX_APPLICATIONS; i++) {
        if (g_es_apps[i].InUse && g_es_apps[i].TaskId == tid) {
            g_es_apps[i].State = CFE_ES_AppState_STOPPED;
            break;
        }
    }
    OS_MutSemGive(g_es_mutex);

    OS_TaskExit();
}

bool CFE_ES_RunLoop(uint32 *RunStatus)
{
    if (!RunStatus) {
        return false;
    }
    return (*RunStatus == CFE_ES_RunStatus_APP_RUN);
}

int32 CFE_ES_WaitForStartupSync(uint32 TimeOutMilliseconds)
{
    OS_TaskDelay(TimeOutMilliseconds);
    return CFE_SUCCESS;
}

int32 CFE_ES_PoolCreate(CFE_ES_MemHandle_t *PoolHandlePtr, uint8 *MemPtr, size_t Size)
{
    if (!PoolHandlePtr || !MemPtr || Size == 0) {
        return CFE_ES_BAD_ARGUMENT;
    }

    OS_MutSemTake(g_es_mutex);
    for (uint32 i = 0; i < CFE_ES_MAX_MEMPOOLS; i++) {
        if (!g_es_mempools[i].InUse) {
            g_es_mempools[i].InUse = true;
            g_es_mempools[i].Buffer = MemPtr;
            g_es_mempools[i].TotalSize = Size;
            g_es_mempools[i].Allocated = 0;
            *PoolHandlePtr = i;
            OS_MutSemGive(g_es_mutex);
            return CFE_SUCCESS;
        }
    }
    OS_MutSemGive(g_es_mutex);
    return CFE_ES_RESOURCE_ERROR;
}

int32 CFE_ES_GetPoolBuf(CFE_ES_MemHandle_t PoolHandle, void **BufPtr, size_t Size)
{
    if (!BufPtr || PoolHandle >= CFE_ES_MAX_MEMPOOLS) {
        return CFE_ES_BAD_ARGUMENT;
    }

    OS_MutSemTake(g_es_mutex);
    CFE_ES_MemPoolEntry_t *pool = &g_es_mempools[PoolHandle];
    if (!pool->InUse || (pool->Allocated + Size > pool->TotalSize)) {
        OS_MutSemGive(g_es_mutex);
        return CFE_ES_RESOURCE_ERROR;
    }

    *BufPtr = (void *)(pool->Buffer + pool->Allocated);
    pool->Allocated += Size;
    OS_MutSemGive(g_es_mutex);
    return CFE_SUCCESS;
}

int32 CFE_ES_PutPoolBuf(CFE_ES_MemHandle_t PoolHandle, void *BufPtr)
{
    (void)PoolHandle;
    (void)BufPtr;
    /* Static flight arena pool, no fragmentation reclamation required during active flight mode */
    return CFE_SUCCESS;
}

void CFE_ES_WriteToSysLog(const char *SpecString, ...)
{
    char buffer[256];
    va_list args;
    va_start(args, SpecString);
    vsnprintf(buffer, sizeof(buffer), SpecString, args);
    va_end(args);

    OS_printf("[ES_SYSLOG] %s\n", buffer);
}

/* User application initialization hook */
extern void CFE_UserApplicationInit(void);
__attribute__((weak)) void CFE_UserApplicationInit(void)
{
    OS_printf("[CFE_ES] No custom user application registered, standing by.\n");
}

void CFE_ES_Main(uint32 StartType, uint32 StartSubtype, uint32 CleanCode)
{
    (void)StartType;
    (void)StartSubtype;
    (void)CleanCode;

    OS_printf("[CFE_ES] Starting Core Flight Executive (cFE) Architecture v0.2.0...\n");

    if (CFE_ES_EarlyInit() != CFE_SUCCESS) {
        OS_printf("[CFE_ES] FATAL: EarlyInit failed!\n");
        return;
    }

    if (CFE_EVS_EarlyInit() != CFE_SUCCESS) {
        OS_printf("[CFE_ES] FATAL: EVS EarlyInit failed!\n");
        return;
    }

    if (CFE_SB_EarlyInit() != CFE_SUCCESS) {
        OS_printf("[CFE_ES] FATAL: SB EarlyInit failed!\n");
        return;
    }

    CFE_EVS_SendEvent(1, CFE_EVS_EventType_INFORMATION,
                      "cFE Core Executive boot sequence completed successfully.");

    /* Launch application layer */
    CFE_UserApplicationInit();
}

