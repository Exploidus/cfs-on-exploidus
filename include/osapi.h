#ifndef OSAPI_H
#define OSAPI_H

#include "common_types.h"
#include "osapi-error.h"
#include "exploidus_capability.h"

#define OS_MAX_TASKS           16
#define OS_MAX_QUEUES          16
#define OS_MAX_MUTEXES         32
#define OS_MAX_API_NAME        32

#define OS_CHECK               0
#define OS_PEND               (-1)

typedef struct
{
    uint32 seconds;
    uint32 microsecs;
} OS_time_t;

typedef struct
{
    char       name[OS_MAX_API_NAME];
    osal_id_t  creator;
    uint32     stack_size;
    uint32     priority;
} OS_task_prop_t;

/* Core Life-cycle API */
int32 OS_API_Init(void);
void  OS_printf(const char *format, ...);

/* Task API */
typedef void (*OS_TaskCallback_t)(void);
int32 OS_TaskCreate(osal_id_t *task_id, const char *task_name, OS_TaskCallback_t function_pointer,
                    const uint32 *stack_pointer, size_t stack_size, uint32 priority, uint32 flags);
int32 OS_TaskDelete(osal_id_t task_id);
void  OS_TaskDelay(uint32 millisecond);
osal_id_t OS_TaskGetId(void);

/* Mutex API (Futex-backed on Exploidus) */
int32 OS_MutSemCreate(osal_id_t *sem_id, const char *sem_name, uint32 options);
int32 OS_MutSemTake(osal_id_t sem_id);
int32 OS_MutSemGive(osal_id_t sem_id);
int32 OS_MutSemDelete(osal_id_t sem_id);

/* Queue API */
int32 OS_QueueCreate(osal_id_t *queue_id, const char *queue_name, size_t queue_depth,
                     size_t data_size, uint32 flags);
int32 OS_QueuePut(osal_id_t queue_id, const void *data, size_t size, uint32 flags);
int32 OS_QueueGet(osal_id_t queue_id, void *data, size_t size, size_t *size_copied, int32 timeout);
int32 OS_QueueDelete(osal_id_t queue_id);

/* Time API */
int32 OS_GetLocalTime(OS_time_t *time_struct);

/*
 * Exploidus Native Hardware Effector API
 * Flight software must present an unforgeable capability token to command physical thrusters or actuators.
 */
int32 OS_EffectorActuate(uint32 effector_id, const exploidus_cap_token_t *token, const void *payload, size_t len);

#endif /* OSAPI_H */
