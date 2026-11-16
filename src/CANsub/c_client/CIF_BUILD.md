# CIF plugins for the CANsub: build plan

**Status:** plan written 2026-11-13. M0 done 2026-11-13 (commit `310567f` on `feature/cansub-rw-class`). R1 code done 2026-11-13, deploy on the PXIe open. Step list and status: `README.md`, "Where you are", row F.

**[verify]** = not confirmed. Check it at the step that names it.

## 1. Result of the repo review (2026-11-13)

| Repo (`C:\Dev\CIF\`) | State | Role |
|---|---|---|
| `CAN_RW_Core` | released (VIPM, `vi.lib\CIF Foundation\CIF CAN RW Core`) | parent class `CAN_RW_CORE.lvclass`: the interface the plugins call |
| `CAN_RW_XNET` | released | child class `XNET CAN RW.lvclass`: the model to copy |
| `CAN_RW_CANsub` | skeleton (2026-10-28): empty `src` folders, placeholder C++ `cansub_lv_wrapper`, installers, VIPM spec | **home of our code and of the new class** |
| `Plugin_CAN_Reader_CANsub`, `Plugin_CAN_Writer_CANsub` | unchanged copies of the XNET plugins (names changed only) | change last: replace the class |

The plugins do not call XNET. They hold one RW object and call:

| Plugin VI | Calls on the RW object |
|---|---|
| `Prepare` (reader) | `Initialize` (Interface, Port Configuration, Use OS Timestamp, Filter) → `Start Reader` → `Reset Statistics` |
| `Prepare` (writer) | `Initialize` → accessor `One Shot TX` → `Start Writer` → `Reset Statistics` |
| `Logic` (reader) | `Read` (number to read, timeout ms) → pcap `[U8]`, `Timeout`, `number read` → CIF `Write CAN FIFO` |
| `Logic` (writer) | CIF `Read CAN FIFO` → `Sleep for T_Out` → `Write` (pcap `[U8]`, timeout ms) → `number written` |
| `Cleanup` | `Stop` |
| `Update CAN Status` (after the loop, at each housekeep) | `Update CAN Status` + accessor `CAN Status` |

Core typedefs: `CAN Port Configuration` {Mode (CAN 2.0 / CAN FD), Baud Rate, Temination On,
Subordinate, Echo TX, FD Config {FD Baud Rate, ISO Mode, BRS Enabled}}; `CAN Filter`
{Allow List (ID), Deny List (ID)}; `CAN Status` {Comm State (Error Active / Error Passive / Bus
Off), Last Error, RX Error Counter, TX Error Counter}; `Statistics` (Frames Filtered, Total Frames
RW, ACK Errors).

## 2. Decisions (Marcus, 2026-11-13)

| No. | Decision |
|---|---|
| 1 | **Option A:** our libraries, C source, tests, examples and docs move to `CAN_RW_CANsub`. One copy only. Backlog work (API_BUILD section 6) is done there. |
| 2 | Branch `feature/cansub-rw-class` in `CAN_RW_CANsub`. One branch in each plugin repo later. |
| 3 | `Interface` string = `<host>/<channel>`, for example `192.168.0.49/2`. Certificate at a fixed path on the target (`/home/lvuser/cansub_root_cert.crt`). |
| 4 | **Phase 1:** Direct ownership, one channel per plugin. **Phase 2:** reader and writer on the same channel (the CANsub permits one WebSocket per channel; XNET permits two sessions per port). Phase 2 is built inside the class; the plugins do not change. |

Open before a push: (a) is `CAN_RW_CANsub` public (Apache 2.0)? Is this code permitted there?
(b) agree with the skeleton author on the native library name and path (section 6).

## 3. Layout after the move

```
CAN_RW_CANsub\
  src\
    CAN RW CANsub.lvclass          new (R-steps)
    Core\ Accessors\ Utilities\ typedef\ Tests\     the class's VIs (skeleton folders)
    CANsub\
      common\                      was CANSub\labview\common      (CANsub_Common.lvlib)
      c_client\                    was CANSub\labview\c_client    (API, C wrappers, Codec,
                                   c_functions, tests, echo, examples, docs, the .lvproj)
    Wrapper\                       placeholder C++ (replaced at the packaging step)
```

`common` and `c_client` keep their relative position, so every link between the files stays
valid and no VI must be relinked. Stays in `C:\Dev\CANSub`: Python harness and scripts,
`hil_test`, the CAN database tools, `labview\native_client` (parked; it loses
`CANsub_Common.lvlib` and will show missing items until it is pointed to the new place),
`labview\udp_echo`.

## 4. Step M0: the move

Commands: in the conversation of 2026-11-13 (PowerShell). Summary:

1. Commit this file and the README change in `C:\Dev\CANSub`.
2. `CAN_RW_CANsub`: new branch `feature/cansub-rw-class`.
3. Export the tracked files of `labview/common` and `labview/c_client` with `git archive` and put
   them in `src\CANsub\`. Copy `libcansub_codec.dll` by hand (git ignores it in both repos).
4. Check: rename `C:\Dev\CANSub\labview` to `labview_old`, open
   `src\CANsub\c_client\CANsub_C_Client.lvproj`, no missing items and no conflicts, run
   `Test_C_Smoke` and the loopback example on the PXIe.
5. Commit in `CAN_RW_CANsub`. Rename `labview_old` back, `git rm` the two folders in CANSub, commit.

## 5. Class design: `CAN RW CANsub.lvclass` (child of `CAN_RW_CORE`)

Private data (one cluster, by-value object as in XNET):

| Field | Type | Use |
|---|---|---|
| `channel` | `CANsub_Channel` | the API object (`CANsub_DirectChannel` at run time) |
| `frames` | `CANsub_Frames` | decode buffers, made once (`Frames_Create`, 1756) |
| `next frame` | I32 | first frame of `frames` that `Read` has not returned yet |
| `pcap out` | [U8] | W16 output, made once |
| `ws out` | [U8] | W17 output, made once |
| `Use OS Timestamp` | Boolean | same meaning as XNET |
| `Transmit` | Boolean | set by `Start Writer` |

| Method (same connector as the XNET class) | Body |
|---|---|
| `Initialize` | parent `Init` → parse `Interface` (`host/channel`) → `CANsub_Config` (Baud Rate, FD Baud Rate, sample points 80 %) → API `Init` → `Frames_Create` → make `pcap out`, `ws out`. No network. |
| `Start Reader`, `Start Writer` | API `Open` |
| `Read` | if `next frame` = `frames.count`: API `Read` (timeout) → convert at most `number to read` frames from `next frame` to pcap (W16) with the allow and deny lists → `Update Stats` → `Timeout` = no packet |
| `Write` | W17 (pcap → WebSocket bytes) → API `Write_Raw` (new) → API `Read` with timeout 0 to take the TX acks → `Update Stats` |
| `Stop` | API `Close` |
| `Update CAN Status` | REST `GET /api/can/{ch}` → `CAN Status` (backlog B1, moved into this ticket) |

Design points that need new work:

| No. | Point | Proposal |
|---|---|---|
| P1 | `Read` must not return more than `number to read` packets: the plugin sizes the CIF FIFO message as `Frames to read` × 88 bytes. One TCP Read can decode up to 1756 frames. | New C function (library 1.3.0) that converts frames `first` … `first + max − 1` and applies the allow and deny ID lists. `Read` keeps `next frame` and returns the remaining frames on the next calls before it reads the socket again. |
| P2 | `CAN Filter` is an allow list and a deny list of IDs, applied in software by XNET. | In the same C function (P1). Device hardware filters stay "accept all" in Phase 1. |
| P3 | The API has `Write_Frame` and `Write_Echo` only. `Write` needs to send a W17 buffer. | New API method `Write_Raw` (dynamic, Direct override = `Check_Open` → `Send_TX`). |
| P4 | A channel that sends must be read (TX acks). The writer plugin never calls `Read`. | `Write` calls API `Read` with timeout 0. `Echo TX` (reader) = keep the TX acks in W16 (`CSB_PCAP_KEEP_TX_ACK`). |
| P5 | `One Shot TX`, `ACK Errors` | **[verify]** device support (TX ack flags, REST option). Phase 1: `One Shot TX` ignored, `ACK Errors` = 0, stated in the VI description. |
| P6 | `Temination On`, `Subordinate`, `ISO Mode` | Phase 1: ignored (termination = backlog B3; non-ISO FD is not supported by the device **[verify]**). `Subordinate` is the natural switch for Phase 2 (attach to a connection that another plugin owns). |
| P7 | Timestamps | `Use OS Timestamp` False: `t_dev` (UTC, PTP) + 37 s → TAI (W16 parameter). True: PXIe time at `Read`. |
| P8 | `Update CAN Status` does a REST call (TLS) from the plugin loop at each housekeep | **[verify]** the time of one call; if it is too long for the loop, do it at a lower rate. |
| P9 | pcap byte order | Core `Init` sets the pcap class format; W16/W17 option must agree. The P6-CIF test checks it against NI's `Parse Next Socket CAN.vi`. |

## 6. Steps

| Step | What | Test |
|---|---|---|
| **M0** | the move (section 4) | project opens from the new place; smoke test + loopback example |
| **R1** | C library 1.3.0: `csb_frames_to_pcap_range` (`first`, `max_packets`, allow and deny lists, `next`); build, unit tests, deploy. **Code done 2026-11-13:** 2221 checks pass (also ASan/UBSan), cross-check 2000 cases 0 mismatches, `dist\` and `wizard\` built. **On the PXIe 2026-11-16:** 2221 checks ALL PASS, CROSSCHECK PASS (pcap and range lines, 0 mismatches), `.so` installed in `/usr/local/lib` (sha256 `dc12221e…101e0575`). **Open:** Windows DLL copy and the version check in LabVIEW (66304) | C tests on the PC and the target |
| **R2** | W16 `C_To_CIF_RT`, W17 `C_CIF_Encode_RT` (`LABVIEW_C_TEST.md` section 9, updated for R1) | P6-CIF `Test_C_CIF.vi` |
| **R3** | API: `Write_Raw` | `Test_API_Write` variant |
| **R4** | class skeleton: `CAN RW CANsub.lvclass`, private data, `Initialize`, `Start Reader`, `Start Writer`, `Stop` | open and close on the PXIe |
| **R5** | `Read`, `Write` | `Test Streaming` copy: two RW objects, CH1 ↔ CH2 cable, pcap bytes equal |
| **R6** | `Update CAN Status` (B1) | status with the bus connected and disconnected |
| **R7** | reader plugin: replace the class, configuration, gRPC (`GetDevices`, `Blink` removed or replaced) | interactive test project |
| **R8** | writer plugin: the same | interactive test project |
| **R9** | bench test with the CIF Manager, both plugins, different channels | end-to-end frames, latency numbers |
| R10 | packaging: C source into `src\Wrapper` (CMake), library name and install path, installers, VIPM | install on a clean target |
| P2 | Phase 2: shared connection for the same channel. Needs first: do the two plugins run in one LabVIEW process? | – |
