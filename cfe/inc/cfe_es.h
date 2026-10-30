#ifndef CFE_ES_H
#define CFE_ES_H

#include "../../include/common_types.h"
#include "../../include/osapi.h"
#include "cfe_error.h"

#define CFE_ES_MAX_APPLICATIONS     16
#define CFE_ES_APP_NAME_LEN         32
#define CFE_ES_MAX_MEMPOOLS         8

/* Run Status */
#define CFE_ES_RunStatus_APP_RUN         1
#define CFE_ES_RunStatus_APP_EXIT        2
#define CFE_ES_RunStatus_APP_ERROR       3
#define CFE_ES_RunStatus_SYS_RESTART     4

/* Application States */
#define CFE_ES_AppState_UNDEFINED        0
#define CFE_ES_AppState_RUNNING          1
#define CFE_ES_AppState_WAITING          2
#define CFE_ES_AppState_STOPPED          3

typedef uint32 CFE_ES_MemHandle_t;

typedef struct
{
    bool          InUse;
    uint32        AppId;
    osal_id_t     TaskId;
    char          AppName[CFE_ES_APP_NAME_LEN];
    uint32        State;
    uint32        ExecutionCounter;
} CFE_ES_AppRecord_t;

/* Function Prototypes */
int32 CFE_ES_EarlyInit(void);
int32 CFE_ES_RegisterApp(void);
int32 CFE_ES_GetAppID(uint32 *AppIdPtr);
int32 CFE_ES_GetAppName(char *AppNamePtr, uint32 AppId, size_t BufferLength);
int32 CFE_ES_CreateChildTask(uint32 *TaskIdPtr, const char *TaskName, void (*FunctionPtr)(void), void *StackPtr, size_t StackSize, uint32 Priority, uint32 Flags);
int32 CFE_ES_DeleteChildTask(uint32 TaskId);
void  CFE_ES_ExitApp(uint32 ExitStatus);
bool  CFE_ES_RunLoop(uint32 *RunStatus);
int32 CFE_ES_WaitForStartupSync(uint32 TimeOutMilliseconds);

/* Memory Management */
int32 CFE_ES_PoolCreate(CFE_ES_MemHandle_t *PoolHandlePtr, uint8 *MemPtr, size_t Size);
int32 CFE_ES_GetPoolBuf(CFE_ES_MemHandle_t PoolHandle, void **BufPtr, size_t Size);
int32 CFE_ES_PutPoolBuf(CFE_ES_MemHandle_t PoolHandle, void *BufPtr);

/* System Logging & LifeCycle */
void  CFE_ES_WriteToSysLog(const char *SpecString, ...);
void  CFE_ES_Main(uint32 StartType, uint32 StartSubtype, uint32 CleanCode);

#endif /* CFE_ES_H */
