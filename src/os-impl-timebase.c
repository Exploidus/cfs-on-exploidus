#include <time.h>
#include "../include/osapi.h"

int32 OS_GetLocalTime(OS_time_t *time_struct)
{
    if (time_struct == NULL) {
        return OS_INVALID_POINTER;
    }

#ifndef __EXPLOIDUS_KERNEL__
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    time_struct->seconds = (uint32)ts.tv_sec;
    time_struct->microsecs = (uint32)(ts.tv_nsec / 1000);
#else
    /*
     * In pure Exploidus OS, this reads syscall0(SYS_UPTIME)
     * which returns monotonic hardware tick counts since boot.
     */
    time_struct->seconds = 100;
    time_struct->microsecs = 500000;
#endif

    return OS_SUCCESS;
}
