#include <string.h>
#include "../include/osapi.h"

#define MAX_QUEUE_DEPTH 32
#define MAX_QUEUE_DATA_SIZE 256

typedef struct
{
    bool       in_use;
    char       name[OS_MAX_API_NAME];
    size_t     max_depth;
    size_t     msg_size;
    size_t     head;
    size_t     tail;
    size_t     count;
    uint8      buffer[MAX_QUEUE_DEPTH][MAX_QUEUE_DATA_SIZE];
    osal_id_t  mutex_id;
} exploidus_queue_record_t;

static exploidus_queue_record_t queue_table[OS_MAX_QUEUES];

int32 OS_QueueCreate(osal_id_t *queue_id, const char *queue_name, size_t queue_depth,
                     size_t data_size, uint32 flags)
{
    (void)flags;
    if (queue_id == NULL || queue_name == NULL) {
        return OS_INVALID_POINTER;
    }

    if (queue_depth > MAX_QUEUE_DEPTH || data_size > MAX_QUEUE_DATA_SIZE) {
        return OS_QUEUE_INVALID_SIZE;
    }

    for (uint32 i = 0; i < OS_MAX_QUEUES; ++i) {
        if (!queue_table[i].in_use) {
            queue_table[i].in_use = true;
            strncpy(queue_table[i].name, queue_name, OS_MAX_API_NAME - 1);
            queue_table[i].name[OS_MAX_API_NAME - 1] = '\0';
            queue_table[i].max_depth = queue_depth;
            queue_table[i].msg_size = data_size;
            queue_table[i].head = 0;
            queue_table[i].tail = 0;
            queue_table[i].count = 0;

            char mut_name[OS_MAX_API_NAME];
            snprintf(mut_name, sizeof(mut_name), "q_mut_%u", i);
            OS_MutSemCreate(&queue_table[i].mutex_id, mut_name, 0);

            *queue_id = i + 1;
            return OS_SUCCESS;
        }
    }

    return OS_ERR_NO_FREE_IDS;
}

int32 OS_QueuePut(osal_id_t queue_id, const void *data, size_t size, uint32 flags)
{
    (void)flags;
    if (queue_id == 0 || queue_id > OS_MAX_QUEUES || data == NULL) {
        return OS_ERR_INVALID_ID;
    }

    exploidus_queue_record_t *q = &queue_table[queue_id - 1];
    if (!q->in_use) {
        return OS_ERR_INVALID_ID;
    }

    OS_MutSemTake(q->mutex_id);

    if (q->count >= q->max_depth) {
        OS_MutSemGive(q->mutex_id);
        return OS_QUEUE_FULL;
    }

    size_t copy_len = size < q->msg_size ? size : q->msg_size;
    memcpy(q->buffer[q->tail], data, copy_len);
    q->tail = (q->tail + 1) % q->max_depth;
    q->count++;

    OS_MutSemGive(q->mutex_id);
    return OS_SUCCESS;
}

int32 OS_QueueGet(osal_id_t queue_id, void *data, size_t size, size_t *size_copied, int32 timeout)
{
    (void)timeout;
    if (queue_id == 0 || queue_id > OS_MAX_QUEUES || data == NULL || size_copied == NULL) {
        return OS_ERR_INVALID_ID;
    }

    exploidus_queue_record_t *q = &queue_table[queue_id - 1];
    if (!q->in_use) {
        return OS_ERR_INVALID_ID;
    }

    OS_MutSemTake(q->mutex_id);

    if (q->count == 0) {
        OS_MutSemGive(q->mutex_id);
        return OS_QUEUE_EMPTY;
    }

    size_t copy_len = size < q->msg_size ? size : q->msg_size;
    memcpy(data, q->buffer[q->head], copy_len);
    *size_copied = copy_len;
    q->head = (q->head + 1) % q->max_depth;
    q->count--;

    OS_MutSemGive(q->mutex_id);
    return OS_SUCCESS;
}

int32 OS_QueueDelete(osal_id_t queue_id)
{
    if (queue_id == 0 || queue_id > OS_MAX_QUEUES) {
        return OS_ERR_INVALID_ID;
    }

    exploidus_queue_record_t *q = &queue_table[queue_id - 1];
    if (!q->in_use) {
        return OS_ERR_INVALID_ID;
    }

    OS_MutSemDelete(q->mutex_id);
    q->in_use = false;
    return OS_SUCCESS;
}
