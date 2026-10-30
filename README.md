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
 |  cfe_time (Mission Clock) | cfe_tbl (Hot-Reload) | cfe_sch    |
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

## 3. Core Flight Executive (cFE) -- Milestone 1

### cFE Services (Milestone 1)

1. **cFE Software Bus (SB)**: Implements capability-gated publish-subscribe routing pipes (`CFE_SB_CreatePipe`, `CFE_SB_SubscribeGated`, `CFE_SB_TransmitMsgGated`). Message endpoints marked with `CFE_SB_CAP_RESTRICTED_MASK` require cryptographic capability verification before messages can be routed.
2. **cFE Executive Services (ES)**: Manages application registration, child flight tasks, and static memory pools (`CFE_ES_RegisterApp`, `CFE_ES_CreateChildTask`, `CFE_ES_PoolCreate`).
3. **cFE Event Services (EVS)**: High-integrity event dispatching featuring cryptographic audit chaining (`CFE_EVS_SendEvent`, `CFE_EVS_VerifyAuditChain`). Every event is committed to a rolling hash chain to prevent in-flight tampering or log omission.

---

## 4. Core Flight Executive (cFE) -- Milestone 2

Milestone 2 extends the cFE layer with three production flight services: mission-accurate Time Services, runtime-hot-reloadable Table Services, and a deterministic 100 Hz Scheduler.

### 4.1 cFE Time Services (TIME)

Provides a dual-clock mission timeline (`MET` -- Mission Elapsed Time, `TAI` -- International Atomic Time, `UTC`) backed by the Exploidus `SYS_UPTIME` monotonic hardware counter.

Key features:
- **Leap-Second Correction**: `CFE_TIME_SetLeapSeconds` / `CFE_TIME_GetLeapSeconds` for UTC drift compensation during long-duration missions.
- **Delta & Subtract**: `CFE_TIME_Subtract` and `CFE_TIME_Add` for sub-microsecond interval arithmetic.
- **MicroSec Utility**: `CFE_TIME_SysTimeToMicroSec` for high-resolution telemetry timestamps.
- **MET/TAI/UTC Getters**: Unified API over the single hardware monotonic source.

### 4.2 cFE Table Services (TBL)

Runtime parameter upload, CRC32 integrity validation, and atomic double-buffered swap without application restart.

Key features:
- **Double-Buffer Architecture**: `Active` and `Pending` buffers with atomic pointer swap on `CFE_TBL_Manage` -- zero downtime for GNC parameter updates.
- **CRC32 Integrity Guard**: Every pending table load is validated via CRC32 before swap. Corrupt uploads are rejected and the active buffer is preserved.
- **Application Validation Callback**: `CFE_TBL_Register` accepts an optional `ValidationFuncPtr` called before each swap -- allows flight software to enforce parameter range limits (e.g., rejecting GNC gains outside the safe flight envelope).
- **Load & Manage API**: `CFE_TBL_Load`, `CFE_TBL_Manage`, `CFE_TBL_GetAddress` implement the full CCSDS Table Services lifecycle.

### 4.3 cFE Scheduler Application (SCH)

Deterministic 100 Hz minor-frame scheduler backed by the Exploidus `SYS_UPTIME` hardware monotonic timer with `INTENT_REALTIME` priority class.

Key features:
- **100 Hz Major Frame / 10ms Minor Frame**: `CFE_SCH_MAJOR_FRAME_HZ = 100`, `CFE_SCH_MINOR_FRAMES = 100` -- 10ms per minor frame.
- **Phase-Offset Activity Table**: Up to 128 activity slots, each with configurable `Frequency`, `Offset`, and wakeup `MessageId`. Phase offsets eliminate thundering-herd packet bursts at frame boundaries.
- **Software Bus Wakeup Dispatch**: Each scheduled activity fires a wakeup message via `CFE_SB_TransmitMsg` -- flight tasks block on their pipe and wake exactly at their scheduled slot.
- **Diagnostics**: `CFE_SCH_GetDiagnostics` exposes `MinorFrameCount`, `MajorFrameCount`, `ScheduledActivities`, `SkippedActivities` for ground-station health monitoring.

---

## 5. Subsystem Implementation Matrix

| OSAL Interface | Exploidus Native Binding | Security & Performance Advantage |
| :--- | :--- | :--- |
| `OS_TaskCreate` | `SYS_SPAWN` + `INTENT_COMPUTE` | Dedicated flight priority class, immune to background starvation |
| `OS_MutSemTake/Give`| `SYS_FUTEX` (Atomic CAS + Futex) | Zero syscall overhead on uncontended lock acquisitions |
| `OS_QueuePut/Get` | Exploidus IPC Circular Ringbuffer | Statically bounded memory, zero dynamic allocation during flight |
| `OS_GetLocalTime` | `SYS_UPTIME` | Monotonic hardware timer, immune to wall-clock manipulation |
| `OS_EffectorActuate`| BLAKE3 Capability Token Verification | Unauthorized or forged physical commands rejected by kernel |

---

## 6. Quick Start & Demonstration

### Milestone 1 Demo (OSAL + cFE Core)

```bash
make clean
make all
./cfs_exploidus_demo
```

### Milestone 2 Demo (TIME + TBL + SCH)

```bash
make clean
make all
./cfs_exploidus_m2
```

### Expected Output (Milestone 2)

```text
=================================================================
 NASA cFS on Exploidus OS -- Milestone 2 Demonstration
 Time Services | Table Services | Scheduler (100Hz)
=================================================================

--- Phase 1: Mission Clock (TIME Services) ---
  [TIME] MET  : 0s 0us
  [TIME] TAI  : 0s 37us
  [TIME] UTC  : 0s 37us  (leap delta = 0us)
  [TIME] Leap seconds updated: 18
  [TIME] UTC after leap correction: 0s 37us

--- Phase 2: Table Services (TBL) -- GNC PID Hot-Reload ---
  [TBL] Registered GNC_PID_Table (handle 0)
  [TBL] Loading GNC PID parameters (v1: conservative gains)...
  [TBL] CRC32=0x... -- validation PASSED. Swapping active buffer.
  [TBL] Active kp_roll=1.200, ki_roll=0.050, max_thrust=120.0N
  [TBL] Loading GNC PID parameters (v2: aggressive gains)...
  [TBL] CRC32=0x... -- validation PASSED. Swapping active buffer.
  [TBL] Active kp_roll=3.500, ki_roll=0.120, max_thrust=350.0N
  [TBL] Loading INVALID parameters (kp_roll=999.0 -- rejected)...
  [TBL REJECT] kp_roll=999.000 out of safe range [0, 50]
  [TBL] Validation callback rejected upload -- active buffer preserved.

--- Phase 3: Scheduler (SCH) -- 100Hz Minor-Frame Dispatch ---
  [SCH] Registered 4 activities: GNC@1Hz, GNC@10Hz, TLM@1Hz, Health@1Hz
  [SCH] Running 3 major frames (300 minor frames)...
  [SCH] GNC_WAKEUP_1HZ  fired on minor frame 0
  [SCH] GNC_WAKEUP_10HZ fired on minor frame 0
  [SCH] Diagnostics: major=3 minor=300 scheduled=... skipped=0

=================================================================
 [M2 QUALIFIED] TIME + TBL + SCH services operational on Exploidus OS.
=================================================================
```

---

## 7. License

Apache 2.0. Copyright (c) Exploidus Labs.
NASA cFS core definitions licensed under Apache 2.0 by NASA GSFC.