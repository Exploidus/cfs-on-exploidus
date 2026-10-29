#include "../inc/cfe_psp.h"

static bool g_watchdog_enabled = false;
static uint32 g_watchdog_counter = 0;

void CFE_PSP_WatchdogInit(void)
{
    g_watchdog_enabled = false;
    g_watchdog_counter = 0;
    OS_printf("[PSP-EXPLOIDUS] Flight Watchdog Subsystem Initialized.\n");
}

void CFE_PSP_WatchdogEnable(void)
{
    g_watchdog_enabled = true;
    OS_printf("[PSP-EXPLOIDUS] Hardware Watchdog ENABLED.\n");
}

void CFE_PSP_WatchdogDisable(void)
{
    g_watchdog_enabled = false;
    OS_printf("[PSP-EXPLOIDUS] Hardware Watchdog DISABLED.\n");
}

void CFE_PSP_WatchdogService(void)
{
    if (g_watchdog_enabled) {
        g_watchdog_counter++;
    }
}
