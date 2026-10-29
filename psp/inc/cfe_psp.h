#ifndef CFE_PSP_H
#define CFE_PSP_H

#include "../../include/common_types.h"
#include "../../include/osapi.h"

/* Reset Types */
#define CFE_PSP_RST_TYPE_POWERON     1
#define CFE_PSP_RST_TYPE_PROCESSOR   2

/* Reset Sub-Types */
#define CFE_PSP_RST_SUBTYPE_POWER_CYCLE      1
#define CFE_PSP_RST_SUBTYPE_PUSH_BUTTON      2
#define CFE_PSP_RST_SUBTYPE_HW_SPECIAL_RESET 3
#define CFE_PSP_RST_SUBTYPE_HW_WATCHDOG      4
#define CFE_PSP_RST_SUBTYPE_EXCEPTION        5

/* Spacecraft and Processor Identification */
#define EXPLOIDUS_SPACECRAFT_ID  0x0042
#define EXPLOIDUS_PROCESSOR_ID   0x0001

/* Function Prototypes */
void   CFE_PSP_Main(int argc, char *argv[]);
uint32 CFE_PSP_GetResetType(uint32 *ResetSubtype);
uint32 CFE_PSP_GetProcessorId(void);
uint32 CFE_PSP_GetSpacecraftId(void);
void   CFE_PSP_Restart(uint32 reset_type);
void   CFE_PSP_Panic(int32 ErrorCode);

/* Memory & Critical Data Store (CDS) services */
int32  CFE_PSP_GetCDSSize(uint32 *SizeOfCDS);
int32  CFE_PSP_WriteToCDS(const void *PtrToDataToWrite, uint32 CDSOffset, uint32 NumBytes);
int32  CFE_PSP_ReadFromCDS(void *PtrToDataToRead, uint32 CDSOffset, uint32 NumBytes);

/* Watchdog Timer Service */
void   CFE_PSP_WatchdogInit(void);
void   CFE_PSP_WatchdogEnable(void);
void   CFE_PSP_WatchdogDisable(void);
void   CFE_PSP_WatchdogService(void);

#endif /* CFE_PSP_H */
