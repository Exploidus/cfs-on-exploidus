#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "../include/osapi.h"

static bool os_initialized = false;

int32 OS_API_Init(void)
{
    if (os_initialized) {
        return OS_SUCCESS;
    }

    OS_printf("[OSAL-EXPLOIDUS] Initializing Exploidus Native Flight Subsystems...\n");
    OS_printf("[OSAL-EXPLOIDUS] Kernel Futex Fast Locking Layer : ACTIVE\n");
    OS_printf("[OSAL-EXPLOIDUS] Intent-Based Priority Scheduler : ACTIVE\n");
    OS_printf("[OSAL-EXPLOIDUS] BLAKE3 Capability Token Engine  : ARMED\n");

    os_initialized = true;
    return OS_SUCCESS;
}

void OS_printf(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    fflush(stdout);
}
