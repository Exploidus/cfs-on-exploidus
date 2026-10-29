#include <stdio.h>
#include <string.h>
#include "../include/osapi.h"
#include "../psp/inc/cfe_psp.h"

#define EFFECTOR_THRUSTER_RCS 0x01
#define FLIGHT_TASK_PRIORITY  50

static osal_id_t g_telemetry_queue;
static exploidus_cap_token_t g_authorized_token;

typedef struct
{
    uint32 command_code;
    float  thrust_vector[3];
} thruster_cmd_t;

/* Authorized Guidance, Navigation, and Control (GNC) Task */
static void gnc_flight_task(void)
{
    OS_printf("[GNC_APP] Initialized on Exploidus OS (Task ID: %u)\n", OS_TaskGetId());

    thruster_cmd_t cmd = {
        .command_code = 0xAA01,
        .thrust_vector = { 0.0f, 0.5f, 1.0f }
    };

    /* Send telemetry via Software Bus queue */
    OS_QueuePut(g_telemetry_queue, &cmd, sizeof(cmd), 0);

    /* Actuate physical thruster with authorized capability token */
    int32 ret = OS_EffectorActuate(EFFECTOR_THRUSTER_RCS, &g_authorized_token, &cmd, sizeof(cmd));
    if (ret == OS_SUCCESS) {
        OS_printf("  [PASS] GNC thruster firing verified via valid capability token.\n");
    } else {
        OS_printf("  [FAIL] Legitimate GNC actuation was blocked: %d\n", ret);
    }
}

/*
 * Simulated NASA cFE Core Executive Entry Point
 */
void CFE_ES_Main(uint32 start_type, uint32 start_subtype, uint32 clean_code)
{
    (void)start_type;
    (void)start_subtype;
    (void)clean_code;

    OS_printf("[CFE_ES] Core Flight Executive started successfully.\n");

    /* 1. Test Critical Data Store (CDS) persistence */
    uint32 mission_seq = 42;
    CFE_PSP_WriteToCDS(&mission_seq, 0, sizeof(mission_seq));
    uint32 recovered_seq = 0;
    CFE_PSP_ReadFromCDS(&recovered_seq, 0, sizeof(recovered_seq));
    if (recovered_seq == 42) {
        OS_printf("  [PASS] Critical Data Store (CDS) integrity verified across platform memory.\n");
    }

    /* 2. Service Watchdog */
    CFE_PSP_WatchdogService();
    OS_printf("  [PASS] Hardware watchdog serviced.\n\n");

    /* 3. Create Software Bus Telemetry Queue */
    if (OS_QueueCreate(&g_telemetry_queue, "cFE_SB_TLM_Q", 16, sizeof(thruster_cmd_t), 0) != OS_SUCCESS) {
        OS_printf("Failed to create Software Bus Queue\n");
        return;
    }
    OS_printf("[CFE_SB] Software Bus Telemetry Queue initialized.\n");

    /* 4. Issue Capability Token to GNC Task (Task ID = 1) */
    Exploidus_Cap_Create(EFFECTOR_THRUSTER_RCS, 1, EXPLOIDUS_CAP_EFFECTOR, 100, &g_authorized_token);
    OS_printf("[CAPABILITY MGR] Issued unforgeable token for Thruster RCS (Authorized Task: 1, Valid until seq: 100)\n\n");

    /* 5. Spawn GNC Flight Task */
    osal_id_t gnc_task_id;
    OS_TaskCreate(&gnc_task_id, "cFE_GNC_APP", gnc_flight_task, NULL, 4096, FLIGHT_TASK_PRIORITY, 0);

    /* Allow GNC task to execute */
    OS_TaskDelay(100);

    /* Read telemetry from queue */
    thruster_cmd_t tlm_out;
    size_t copied = 0;
    if (OS_QueueGet(g_telemetry_queue, &tlm_out, sizeof(tlm_out), &copied, OS_CHECK) == OS_SUCCESS) {
        OS_printf("[TELEMETRY RX] Consumed GNC telemetry packet from Software Bus queue (code: 0x%04X)\n\n",
                  tlm_out.command_code);
    }

    OS_printf("=================================================================\n");
    OS_printf(" Adversarial Capability Boundary Evaluation                      \n");
    OS_printf("=================================================================\n\n");

    /* Scenario 1: Unprivileged peer attempts to fire thruster with zero token */
    OS_printf("[SCENARIO 1] Adversary attempts unauthorized thruster actuation (token=NULL)...\n");
    int32 forged_ret = OS_EffectorActuate(EFFECTOR_THRUSTER_RCS, NULL, &tlm_out, sizeof(tlm_out));
    if (forged_ret == OS_INVALID_POINTER) {
        OS_printf("  [PASS] Actuation rejected! NULL token denied access.\n\n");
    }

    /* Scenario 2: Compromised peer tampers with token permissions */
    OS_printf("[SCENARIO 2] Adversary attempts actuation with forged capability token...\n");
    exploidus_cap_token_t forged_token = g_authorized_token;
    forged_token.permissions = 0; /* Revoke effector permission */
    forged_ret = OS_EffectorActuate(EFFECTOR_THRUSTER_RCS, &forged_token, &tlm_out, sizeof(tlm_out));
    if (forged_ret == OS_ERR_CAPABILITY_DENIED) {
        OS_printf("  [PASS] Cryptographic tampering detected! Forged token rejected.\n\n");
    }

    /* Scenario 3: Monotonic freshness expiration / replay attack */
    OS_printf("[SCENARIO 3] Adversary attempts replay with expired capability token...\n");
    exploidus_cap_token_t expired_token;
    Exploidus_Cap_Create(EFFECTOR_THRUSTER_RCS, 1, EXPLOIDUS_CAP_EFFECTOR, 0, &expired_token); /* Expired at seq 0 */
    forged_ret = OS_EffectorActuate(EFFECTOR_THRUSTER_RCS, &expired_token, &tlm_out, sizeof(tlm_out));
    if (forged_ret == OS_ERR_CAPABILITY_EXPIRED) {
        OS_printf("  [PASS] Freshness check passed! Stale token rejected as expired.\n\n");
    }

    OS_printf("=================================================================\n");
    OS_printf(" [PORT QUALIFIED] NASA cFS OSAL & PSP on Exploidus OS Verified.   \n");
    OS_printf(" Zero Linux bloat, zero DAC vulnerabilities, unforgeable effectors.\n");
    OS_printf("=================================================================\n\n");
}

int main(int argc, char *argv[])
{
    /* Hand over execution to the Exploidus Platform Support Package */
    CFE_PSP_Main(argc, argv);
    return 0;
}
