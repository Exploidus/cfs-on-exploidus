# NASA cFS on Exploidus OS (cfs-on-exploidus)

Native Operating System Abstraction Layer (OSAL) port and capability-hardened execution environment for the NASA Core Flight System (cFS) running on Exploidus OS.

Developed by **Exploidus Labs** as an independent research reference for next-generation, high-integrity spaceflight platforms.

---

## 1. Architectural Motivation

The NASA Core Flight System (cFS) executes mission-critical spacecraft applications (Guidance, Navigation, and Control, Command Ingest, Telemetry Output, Attitude Determination). Historically, cFS targets POSIX (Linux) or proprietary RTOS kernels (VxWorks, RTEMS).

However, executing cFS on standard Linux platforms introduces fundamental security and qualification challenges:
1. **DAC Leaks & Same-UID Vulnerability**: Under Linux, all cFS applications running under the same UID can inspect, ptrace, or tamper with peer flight tasks via `/proc/pid/mem` or inherited descriptors.
2. **Unauthenticated Effector Actuation**: Standard POSIX syscalls (`write`, `ioctl`) cannot verify whether an actuation command originated from an authorized flight task or a compromised peer.
3. **Observation Discontinuities**: Linux ringbuffer backpressure and scheduling jitter degrade deterministic flight control guarantees.

**Exploidus OS** eliminates these vulnerabilities at the microkernel boundary through **BLAKE3 Cryptographic Capability Tokens**, **Intent-Based Real-Time Scheduling**, and **Kernel-Enforced Effector Authentication**.

---

## 2. Architecture Overview

```
 +---------------------------------------------------------------+
 |                     NASA cFS Flight Apps                     |
 |        [ GNC_App ]       [ Telemetry_App ]      [ Thruster ]  |
 +---------------------------------------------------------------+
                                 |
                                 v  (Core Flight Executive)
 +---------------------------------------------------------------+
 |           Core Flight Executive (cFE) v0.2.0 Layer            |
 |  cfe_sb (Gated Pub/Sub) | cfe_es (Lifecycle) | cfe_evs (Audit)|
 +---------------------------------------------------------------+
                                 |
                                 v  (Standard cFS OSAL API)
 +---------------------------------------------------------------+
 |                   OSAL Exploidus Native Port                  |
 |  os-impl-tasks  |  os-impl-mutex  |  os-impl-queues  |  cap   |
 +---------------------------------------------------------------+
                                 |
                                 v
 +---------------------------------------------------------------+
 |                   PSP Exploidus Native Port                   |
 |  cfe_psp_start  |  cfe_psp_memory |  cfe_psp_watchdog | cds   |
 +---------------------------------------------------------------+
                                 |
                                 v  (Exploidus Native Syscalls)
 +---------------------------------------------------------------+
 |                      Exploidus OS Kernel                      |
 |  - SYS_SPAWN (Intent-Based Priority Classes: INTENT_COMPUTE)  |
 |  - SYS_FUTEX (Fast Userspace Locking with Zero Contention)    |
 |  - BLAKE3 Capability Token Engine (Hardware Effector Gate)    |
 |  - Correlated Network Security Layer (CNSL Telemetry Guard)   |
 +---------------------------------------------------------------+
```

---

## 3. Core Flight Executive (cFE) Architecture (Milestone 1)

1. **cFE Software Bus (SB)**: Implements capability-gated publish-subscribe routing pipes (`CFE_SB_CreatePipe`, `CFE_SB_SubscribeGated`, `CFE_SB_TransmitMsgGated`). Message endpoints marked with `CFE_SB_CAP_RESTRICTED_MASK` require cryptographic capability verification before messages can be routed.
2. **cFE Executive Services (ES)**: Manages application registration, child flight tasks, and static memory pools (`CFE_ES_RegisterApp`, `CFE_ES_CreateChildTask`, `CFE_ES_PoolCreate`).
3. **cFE Event Services (EVS)**: High-integrity event dispatching featuring cryptographic audit chaining (`CFE_EVS_SendEvent`, `CFE_EVS_VerifyAuditChain`). Every event is committed to a rolling hash chain to prevent in-flight tampering or log omission.

---

## 3. Subsystem Implementation Matrix

| OSAL Interface | Exploidus Native Binding | Security & Performance Advantage |
| :--- | :--- | :--- |
| `OS_TaskCreate` | `SYS_SPAWN` + `INTENT_COMPUTE` | Dedicated flight priority class, immune to background starvation |
| `OS_MutSemTake/Give`| `SYS_FUTEX` (Atomic CAS + Futex) | Zero syscall overhead on uncontended lock acquisitions |
| `OS_QueuePut/Get` | Exploidus IPC Circular Ringbuffer | Statically bounded memory, zero dynamic allocation during flight |
| `OS_GetLocalTime` | `SYS_UPTIME` | Monotonic hardware timer, immune to wall-clock manipulation |
| `OS_EffectorActuate`| BLAKE3 Capability Token Verification | Unauthorized or forged physical commands rejected by kernel |

---

## 4. Quick Start & Demonstration

To compile the native OSAL demonstration:

```bash
make clean
make all
./cfs_exploidus_demo
```

### Expected Output

```text
=================================================================
 NASA cFS on Exploidus OS - Flight Executive & Capability Test   
 Exploidus Labs - Native OSAL Port Demonstration                 
=================================================================

[OSAL-EXPLOIDUS] Initializing Exploidus Native Flight Subsystems...
[OSAL-EXPLOIDUS] Kernel Futex Fast Locking Layer : ACTIVE
[OSAL-EXPLOIDUS] Intent-Based Priority Scheduler : ACTIVE
[OSAL-EXPLOIDUS] BLAKE3 Capability Token Engine  : ARMED
[CFE_CORE] Software Bus Telemetry Queue initialized.

[CAPABILITY MGR] Issued unforgeable token for Thruster RCS (Authorized Task: 1, Valid until seq: 100)

[GNC_APP] Initialized on Exploidus OS (Task ID: 1)
[EFFECTOR FIRED] Authorized hardware command executed on Effector 0x01 (Seq: 2, Task: 1)
  [PASS] GNC thruster firing verified via valid capability token.
[TELEMETRY RX] Consumed GNC telemetry packet from Software Bus queue (code: 0xAA01)

=================================================================
 Adversarial Capability Boundary Evaluation                      
=================================================================

[SCENARIO 1] Adversary attempts unauthorized thruster actuation (token=NULL)...
  [PASS] Actuation rejected! NULL token denied access.

[SCENARIO 2] Adversary attempts actuation with forged capability token...
[SECURITY AUDIT] Unauthorized effector actuation attempt blocked! (Effector: 0x01, Task: 1, Error: -100)
  [PASS] Cryptographic tampering detected! Forged token rejected.

[SCENARIO 3] Adversary attempts replay with expired capability token...
[SECURITY AUDIT] Unauthorized effector actuation attempt blocked! (Effector: 0x01, Task: 1, Error: -101)
  [PASS] Freshness check passed! Stale token rejected as expired.

=================================================================
 [PORT QUALIFIED] NASA cFS OSAL on Exploidus OS fully operational.
 Zero Linux bloat, zero DAC vulnerabilities, unforgeable effectors.
=================================================================
```

---

## 5. License

Apache 2.0. Copyright (c) Exploidus Labs.
NASA cFS core definitions licensed under Apache 2.0 by NASA GSFC.
