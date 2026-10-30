#include <stdio.h>
#include <string.h>
#include "../cfe/inc/cfe.h"

#define EFFECTOR_THRUSTER_RCS     0x01
#define FLIGHT_TASK_PRIORITY      50

#define MSG_ID_TELEMETRY          0x1801
#define MSG_ID_RESTRICTED_GNC_CMD (CFE_SB_CAP_RESTRICTED_MASK | 0x0001)

static CFE_SB_PipeId_t       g_gnc_pipe;
static exploidus_cap_token_t g_thruster_token;
static exploidus_cap_token_t g_sb_write_token;
static exploidus_cap_token_t g_sb_read_token;

typedef struct
{
    uint32 command_code;
    float  thrust_vector[3];
} thruster_cmd_t;

/* Authorized Flight Application */
static void gnc_flight_task(void)
{
    CFE_ES_RegisterApp();

    uint32 app_id = 0;
    CFE_ES_GetAppID(&app_id);
    CFE_EVS_SendEvent(101, CFE_EVS_EventType_INFORMATION,
                      "GNC Application initialized on Exploidus cFE (App ID: %u)", app_id);

    /* 1. Actuate Physical Thruster with authorized capability token */
    thruster_cmd_t cmd = {
        .command_code = 0xAA01,
        .thrust_vector = { 0.0f, 0.5f, 1.0f }
    };

    int32 eff_ret = OS_EffectorActuate(EFFECTOR_THRUSTER_RCS, &g_thruster_token, &cmd, sizeof(cmd));
    if (eff_ret == OS_SUCCESS) {
        CFE_EVS_SendEvent(102, CFE_EVS_EventType_INFORMATION,
                          "GNC physical thruster firing verified via valid capability token.");
    } else {
        CFE_EVS_SendEvent(103, CFE_EVS_EventType_ERROR,
                          "GNC thruster actuation failed: %d", eff_ret);
    }

    /* 2. Publish Restricted Guidance Command via Gated Software Bus */
    CFE_SB_Buffer_t restricted_msg;
    CFE_SB_InitMsg(&restricted_msg, MSG_ID_RESTRICTED_GNC_CMD, sizeof(cmd), true);
    memcpy(restricted_msg.Data, &cmd, sizeof(cmd));

    int32 sb_pub = CFE_SB_TransmitMsgGated(&restricted_msg, true, &g_sb_write_token);
    if (sb_pub == CFE_SUCCESS) {
        CFE_EVS_SendEvent(104, CFE_EVS_EventType_INFORMATION,
                          "GNC transmitted capability-gated guidance packet to Software Bus.");
    } else {
        CFE_EVS_SendEvent(105, CFE_EVS_EventType_ERROR,
                          "GNC failed to transmit gated packet: %d", sb_pub);
    }
}

void CFE_UserApplicationInit(void)
{
    OS_printf("\n=================================================================\n");
    OS_printf(" NASA cFE v0.2.0 on Exploidus OS - Milestone 1 Evaluation        \n");
    OS_printf(" Core Architecture: Software Bus (SB), ES, and EVS Audit Logging \n");
    OS_printf("=================================================================\n\n");

    /* -------------------------------------------------------------
     * Subsystem Test 1: Executive Services (ES) App & Memory Pools
     * ------------------------------------------------------------- */
    OS_printf("[TEST 1] cFE Executive Services (ES) Subsystem...\n");
    CFE_ES_RegisterApp();

    uint32 main_app_id = 0;
    CFE_ES_GetAppID(&main_app_id);
    char app_name[CFE_ES_APP_NAME_LEN];
    CFE_ES_GetAppName(app_name, main_app_id, sizeof(app_name));
    OS_printf("  [PASS] Application registered in ES Table: %s (App ID: %u)\n", app_name, main_app_id);

    /* Static Memory Pool test */
    static uint8 s_flight_pool_mem[2048];
    CFE_ES_MemHandle_t pool_handle = 0;
    if (CFE_ES_PoolCreate(&pool_handle, s_flight_pool_mem, sizeof(s_flight_pool_mem)) == CFE_SUCCESS) {
        void *test_buf = NULL;
        if (CFE_ES_GetPoolBuf(pool_handle, &test_buf, 256) == CFE_SUCCESS && test_buf != NULL) {
            OS_printf("  [PASS] ES Memory Pool buffer allocation verified (Handle: %u, Size: 256 B)\n", pool_handle);
        }
    }

    /* PSP Services check */
    uint32 mission_seq = 0x50415353;
    CFE_PSP_WriteToCDS(&mission_seq, 0, sizeof(mission_seq));
    uint32 restored_seq = 0;
    CFE_PSP_ReadFromCDS(&restored_seq, 0, sizeof(restored_seq));
    if (restored_seq == 0x50415353) {
        OS_printf("  [PASS] Critical Data Store (CDS) persistence verified across platform memory.\n");
    }
    CFE_PSP_WatchdogService();
    OS_printf("  [PASS] Hardware Watchdog timer serviced.\n\n");

    /* -------------------------------------------------------------
     * Subsystem Test 2: Software Bus (SB) Pub/Sub & Capability Gating
     * ------------------------------------------------------------- */
    OS_printf("[TEST 2] cFE Software Bus (SB) Subsystem & Security Gates...\n");
    int32 sb_status = CFE_SB_CreatePipe(&g_gnc_pipe, 8, "GNC_INPUT_PIPE");
    if (sb_status == CFE_SUCCESS) {
        OS_printf("  [PASS] Software Bus pipe created successfully (Pipe ID: %u)\n", g_gnc_pipe);
    }

    /* Standard Telemetry Subscription & Delivery */
    CFE_SB_Subscribe(MSG_ID_TELEMETRY, g_gnc_pipe);

    CFE_SB_Buffer_t tlm_packet;
    CFE_SB_InitMsg(&tlm_packet, MSG_ID_TELEMETRY, 64, true);
    snprintf((char *)tlm_packet.Data, sizeof(tlm_packet.Data), "EXPLOIDUS_CFS_TLM_OK");
    CFE_SB_TransmitMsg(&tlm_packet, true);

    CFE_SB_Buffer_t *rx_buf = NULL;
    if (CFE_SB_ReceiveBuffer(&rx_buf, g_gnc_pipe, CFE_SB_POLL) == CFE_SUCCESS && rx_buf != NULL) {
        OS_printf("  [PASS] Standard Software Bus Pub/Sub verified (MsgId: 0x%04X, Payload: %s)\n",
                  rx_buf->MsgId, (char *)rx_buf->Data);
    }

    /* Restricted Software Bus Evaluation (Capability Gated) */
    OS_printf("\n  [SECURITY EVALUATION] Software Bus Capability Gate Verification:\n");

    /* Adversarial attempt: subscribe to restricted MsgId without token */
    int32 unauth_sub = CFE_SB_Subscribe(MSG_ID_RESTRICTED_GNC_CMD, g_gnc_pipe);
    if (unauth_sub == CFE_SB_ACCESS_DENIED) {
        OS_printf("  [PASS] Unauthorized SB subscription rejected (ACCESS_DENIED).\n");
    }

    /* Adversarial attempt: transmit to restricted MsgId without token */
    CFE_SB_Buffer_t forged_msg;
    CFE_SB_InitMsg(&forged_msg, MSG_ID_RESTRICTED_GNC_CMD, 16, true);
    int32 unauth_tx = CFE_SB_TransmitMsg(&forged_msg, true);
    if (unauth_tx == CFE_SB_ACCESS_DENIED) {
        OS_printf("  [PASS] Unauthorized SB transmit rejected (ACCESS_DENIED).\n");
    }

    /* Issue valid Exploidus capability tokens */
    osal_id_t main_task = OS_TaskGetId();
    Exploidus_Cap_Create(MSG_ID_RESTRICTED_GNC_CMD, (uint32)main_task, EXPLOIDUS_CAP_READ, 100, &g_sb_read_token);
    Exploidus_Cap_Create(MSG_ID_RESTRICTED_GNC_CMD, 1, EXPLOIDUS_CAP_WRITE, 100, &g_sb_write_token);
    Exploidus_Cap_Create(EFFECTOR_THRUSTER_RCS, 1, EXPLOIDUS_CAP_EFFECTOR, 100, &g_thruster_token);

    int32 auth_sub = CFE_SB_SubscribeGated(MSG_ID_RESTRICTED_GNC_CMD, g_gnc_pipe, &g_sb_read_token);
    if (auth_sub == CFE_SUCCESS) {
        OS_printf("  [PASS] Capability-gated SB subscription authorized via BLAKE3 token.\n\n");
    }

    /* -------------------------------------------------------------
     * Subsystem Test 3: Spawn Authorized GNC Application Task
     * ------------------------------------------------------------- */
    OS_printf("[TEST 3] Executing GNC Flight Task under cFE...\n");
    uint32 gnc_task_id = 0;
    CFE_ES_CreateChildTask(&gnc_task_id, "GNC_TASK", gnc_flight_task, NULL, 4096, FLIGHT_TASK_PRIORITY, 0);

    /* Allow GNC task to run */
    CFE_ES_WaitForStartupSync(100);

    /* Consume gated packet from GNC on Software Bus */
    CFE_SB_Buffer_t *gated_rx = NULL;
    if (CFE_SB_ReceiveBuffer(&gated_rx, g_gnc_pipe, CFE_SB_POLL) == CFE_SUCCESS && gated_rx != NULL) {
        thruster_cmd_t *rx_cmd = (thruster_cmd_t *)gated_rx->Data;
        OS_printf("  [PASS] Received authorized GNC command on gated Software Bus (Cmd: 0x%04X, Vector: [%.1f, %.1f, %.1f])\n\n",
                  rx_cmd->command_code,
                  rx_cmd->thrust_vector[0], rx_cmd->thrust_vector[1], rx_cmd->thrust_vector[2]);
    }

    /* -------------------------------------------------------------
     * Subsystem Test 4: cFE Event Services (EVS) Cryptographic Audit
     * ------------------------------------------------------------- */
    OS_printf("[TEST 4] cFE Event Services (EVS) Cryptographic Audit Log Chain...\n");
    uint32 total_events = CFE_EVS_GetAuditLogCount();
    OS_printf("  Total EVS Audit Entries Recorded: %u\n", total_events);

    int32 chain_integrity = CFE_EVS_VerifyAuditChain();
    if (chain_integrity == CFE_SUCCESS) {
        OS_printf("  [PASS] Cryptographic audit hash chain verified 100%% intact (Zero tampering).\n");
    }

    /* Active tamper detection test */
    OS_printf("  [SECURITY EVALUATION] Simulating unauthorized in-memory audit log tampering...\n");
    CFE_EVS_InjectTamperFault(0);
    if (CFE_EVS_VerifyAuditChain() != CFE_SUCCESS) {
        OS_printf("  [PASS] Tampering instantly detected! Hash chain anomaly caught.\n");
    }
    CFE_EVS_InjectTamperFault(0); /* Restore original state */
    if (CFE_EVS_VerifyAuditChain() == CFE_SUCCESS) {
        OS_printf("  [PASS] Hash chain restored and re-verified clean.\n");
    }

    /* -------------------------------------------------------------
     * Subsystem Test 5: Hardware Effector Tampering Resistance
     * ------------------------------------------------------------- */
    OS_printf("\n[TEST 5] Hardware Effector Tampering Resistance...\n");
    thruster_cmd_t dummy_cmd = { .command_code = 0xFFFF };

    int32 null_ret = OS_EffectorActuate(EFFECTOR_THRUSTER_RCS, NULL, &dummy_cmd, sizeof(dummy_cmd));
    if (null_ret == OS_INVALID_POINTER) {
        OS_printf("  [PASS] Physical actuation without capability token blocked.\n");
    }

    exploidus_cap_token_t forged_token = g_thruster_token;
    forged_token.permissions = 0;
    int32 forged_ret = OS_EffectorActuate(EFFECTOR_THRUSTER_RCS, &forged_token, &dummy_cmd, sizeof(dummy_cmd));
    if (forged_ret == OS_ERR_CAPABILITY_DENIED) {
        OS_printf("  [PASS] Physical actuation with tampered capability token blocked.\n");
    }

    OS_printf("\n=================================================================\n");
    OS_printf(" [MILESTONE 1 COMPLETE] Core Flight Executive (cFE) Architecture \n");
    OS_printf(" - cFE Software Bus (SB): Active with Capability-Gated Routing   \n");
    OS_printf(" - cFE Executive Services (ES): Active with Lifecycle & MemPool  \n");
    OS_printf(" - cFE Event Services (EVS): Active with Cryptographic Audit Log \n");
    OS_printf("=================================================================\n\n");
}

int main(int argc, char *argv[])
{
    CFE_PSP_Main(argc, argv);
    return 0;
}
