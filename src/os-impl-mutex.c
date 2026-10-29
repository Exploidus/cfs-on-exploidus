#include <string.h>
#include "../include/osapi.h"

typedef struct
{
    bool       in_use;
    char       name[OS_MAX_API_NAME];
    int32      lock_val;  /* 0 = unlocked, 1 = locked uncontended, 2 = locked with waiters */
    osal_id_t  owner_task;
} exploidus_mutex_record_t;

static exploidus_mutex_record_t mutex_table[OS_MAX_MUTEXES];

int32 OS_MutSemCreate(osal_id_t *sem_id, const char *sem_name, uint32 options)
{
    (void)options;
    if (sem_id == NULL || sem_name == NULL) {
        return OS_INVALID_POINTER;
    }

    for (uint32 i = 0; i < OS_MAX_MUTEXES; ++i) {
        if (!mutex_table[i].in_use) {
            mutex_table[i].in_use = true;
            strncpy(mutex_table[i].name, sem_name, OS_MAX_API_NAME - 1);
            mutex_table[i].name[OS_MAX_API_NAME - 1] = '\0';
            mutex_table[i].lock_val = 0;
            mutex_table[i].owner_task = OS_OBJECT_ID_UNDEFINED;
            *sem_id = i + 1;
            return OS_SUCCESS;
        }
    }

    return OS_ERR_NO_FREE_IDS;
}

int32 OS_MutSemTake(osal_id_t sem_id)
{
    if (sem_id == 0 || sem_id > OS_MAX_MUTEXES) {
        return OS_ERR_INVALID_ID;
    }

    exploidus_mutex_record_t *m = &mutex_table[sem_id - 1];
    if (!m->in_use) {
        return OS_ERR_INVALID_ID;
    }

    /* Fast userspace atomic path (simulate atomic swap) */
    while (__sync_lock_test_and_set(&m->lock_val, 1) != 0) {
        /*
         * In pure Exploidus OS, this calls syscall3(SYS_FUTEX, &m->lock_val, FUTEX_WAIT, 1).
         * For cooperative simulation, we yield the execution quantum.
         */
        OS_TaskDelay(1);
    }

    m->owner_task = OS_TaskGetId();
    return OS_SUCCESS;
}

int32 OS_MutSemGive(osal_id_t sem_id)
{
    if (sem_id == 0 || sem_id > OS_MAX_MUTEXES) {
        return OS_ERR_INVALID_ID;
    }

    exploidus_mutex_record_t *m = &mutex_table[sem_id - 1];
    if (!m->in_use) {
        return OS_ERR_INVALID_ID;
    }

    m->owner_task = OS_OBJECT_ID_UNDEFINED;
    __sync_lock_release(&m->lock_val);

    /*
     * In pure Exploidus OS, this triggers syscall3(SYS_FUTEX, &m->lock_val, FUTEX_WAKE, 1)
     * waking up the highest-priority blocked flight task.
     */
    return OS_SUCCESS;
}

int32 OS_MutSemDelete(osal_id_t sem_id)
{
    if (sem_id == 0 || sem_id > OS_MAX_MUTEXES) {
        return OS_ERR_INVALID_ID;
    }

    exploidus_mutex_record_t *m = &mutex_table[sem_id - 1];
    if (!m->in_use) {
        return OS_ERR_INVALID_ID;
    }

    m->in_use = false;
    m->lock_val = 0;
    return OS_SUCCESS;
}
