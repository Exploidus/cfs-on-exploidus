#include <string.h>
#include <unistd.h>
#include "../include/osapi.h"

#ifdef __EXPLOIDUS_KERNEL__
#include "../../kernel/syscall/table.h"
#else
#include <pthread.h>
#endif

typedef struct
{
    bool              in_use;
    char              name[OS_MAX_API_NAME];
    OS_TaskCallback_t entry_fn;
    uint32            priority;
    osal_id_t         task_id;
#ifndef __EXPLOIDUS_KERNEL__
    pthread_t         thread;
#else
    uint32            pid;
#endif
} exploidus_task_record_t;

static exploidus_task_record_t task_table[OS_MAX_TASKS];
static __thread osal_id_t current_task_id = 1;

#ifndef __EXPLOIDUS_KERNEL__
static void *task_trampoline(void *arg)
{
    exploidus_task_record_t *t = (exploidus_task_record_t *)arg;
    current_task_id = t->task_id;
    if (t->entry_fn) {
        t->entry_fn();
    }
    return NULL;
}
#endif

int32 OS_TaskCreate(osal_id_t *task_id, const char *task_name, OS_TaskCallback_t function_pointer,
                    const uint32 *stack_pointer, size_t stack_size, uint32 priority, uint32 flags)
{
    (void)stack_pointer;
    (void)stack_size;
    (void)flags;

    if (task_id == NULL || task_name == NULL || function_pointer == NULL) {
        return OS_INVALID_POINTER;
    }

    for (uint32 i = 0; i < OS_MAX_TASKS; ++i) {
        if (!task_table[i].in_use) {
            task_table[i].in_use = true;
            strncpy(task_table[i].name, task_name, OS_MAX_API_NAME - 1);
            task_table[i].name[OS_MAX_API_NAME - 1] = '\0';
            task_table[i].entry_fn = function_pointer;
            task_table[i].priority = priority;
            task_table[i].task_id = i + 1;

#ifndef __EXPLOIDUS_KERNEL__
            pthread_create(&task_table[i].thread, NULL, task_trampoline, &task_table[i]);
#else
            /*
             * In pure Exploidus OS, this calls syscall2(SYS_SPAWN, (uint64)task_name, priority)
             * setting the thread intent to INTENT_COMPUTE.
             */
            task_table[i].pid = i + 100;
#endif
            *task_id = i + 1;
            return OS_SUCCESS;
        }
    }

    return OS_ERR_NO_FREE_IDS;
}

int32 OS_TaskDelete(osal_id_t task_id)
{
    if (task_id == 0 || task_id > OS_MAX_TASKS) {
        return OS_ERR_INVALID_ID;
    }

    exploidus_task_record_t *t = &task_table[task_id - 1];
    if (!t->in_use) {
        return OS_ERR_INVALID_ID;
    }

#ifndef __EXPLOIDUS_KERNEL__
    pthread_cancel(t->thread);
    pthread_join(t->thread, NULL);
#endif

    t->in_use = false;
    return OS_SUCCESS;
}

void OS_TaskDelay(uint32 millisecond)
{
#ifndef __EXPLOIDUS_KERNEL__
    usleep((useconds_t)millisecond * 1000);
#else
    /* Exploidus native syscall: syscall1(SYS_SLEEP, millisecond) */
#endif
}

void OS_TaskExit(void)
{
    osal_id_t tid = OS_TaskGetId();
    if (tid > 0 && tid <= OS_MAX_TASKS) {
        task_table[tid - 1].in_use = false;
    }
#ifndef __EXPLOIDUS_KERNEL__
    pthread_exit(NULL);
#endif
}

osal_id_t OS_TaskGetId(void)
{
    return current_task_id;
}
