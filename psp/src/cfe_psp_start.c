#include <stdio.h>
#include "../inc/cfe_psp.h"

/* Forward declaration of Core Flight Executive entry point */
extern void CFE_ES_Main(uint32 start_type, uint32 start_subtype, uint32 clean_code);

void CFE_PSP_Main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    uint32 reset_subtype = 0;
    uint32 reset_type = CFE_PSP_GetResetType(&reset_subtype);

    OS_printf("\n=================================================================\n");
    OS_printf(" NASA Core Flight Executive (cFE) - Exploidus Platform Support  \n");
    OS_printf(" Spacecraft ID: 0x%04X | Processor ID: 0x%04X                   \n",
              CFE_PSP_GetSpacecraftId(), CFE_PSP_GetProcessorId());
    OS_printf(" Boot Type    : %s (Subtype: %u)                             \n",
              reset_type == CFE_PSP_RST_TYPE_POWERON ? "POWER-ON RESET" : "PROCESSOR RESET",
              reset_subtype);
    OS_printf("=================================================================\n\n");

    /* Initialize OSAL */
    if (OS_API_Init() != OS_SUCCESS) {
        CFE_PSP_Panic(-1);
    }

    /* Initialize Platform Subsystems */
    CFE_PSP_WatchdogInit();
    CFE_PSP_WatchdogEnable();

    OS_printf("[PSP-EXPLOIDUS] Critical Data Store (CDS) Size: %u KB\n",
              128);
    OS_printf("[PSP-EXPLOIDUS] Handing over execution quantum to cFE Core Executive...\n\n");

    /* Jump into Core Flight Executive */
    CFE_ES_Main(reset_type, reset_subtype, 0);
}
