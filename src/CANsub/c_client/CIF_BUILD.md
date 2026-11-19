# CIF plugins for the CANsub: build plan

**Status:** plan written 2026-11-13. M0 done 2026-11-13 (commit `310567f` on `feature/cansub-rw-class`). R1 to R3 done 2026-11-16, R4 and R5 done 2026-11-17, R6 done 2026-11-18. Next: R7. Step list and status: `README.md`, "Where you are", row F.

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

## 2. Decisions (2026-11-13)

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

Done 2026-11-13 with PowerShell commands. Summary:

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
| P7 | Timestamps | **Decided 2026-11-17: Phase 1 ignores `Use OS Timestamp`.** The packets always carry the device time: `t_dev` (UTC, PTP) + 37 s = TAI. `Initialize` stores the option in the private data, so the interface is complete. To add it later is a small change inside `Read` only (no change of an interface, of the C library or of the plugins): when the option is True, add (host time − `t_dev` of the last frame) to the `t_dev` values of the new frames before the conversion, as the XNET class does with its time pair. Estimate 2 to 3 h with a test. |
| P8 | Time of `Update CAN Status` (a REST call from the plugin loop) | **Measured 2026-11-17/18.** A new HTTPS connection for each call: **78.5–80.0 ms** (15 calls). `curl` shows the cause: TCP connect 0.2 ms, TLS handshake done at 80 ms, a second request on the same connection 0.56 ms. With the client kept open (R6 as built): first call 79.2 ms, later calls **0.57–0.69 ms** with 0 s, 1 s and 5 s between the calls. With **30 s** between the calls each call is 80 ms again and gives no error: the connection is closed when idle for a time between 5 s and 30 s, and the HTTP client connects again. **So `Update CAN Status` must be called at least every 5 s to stay fast.** **Closed 2026-11-18:** the plugin loop calls it after each read timeout and at each housekeeping interval, and the housekeeping interval of the CIF plugins on the bench is much less than 5 s. So the connection stays open in operation, and a call takes about 0.6 ms. No more change is necessary. |
| P9 | pcap byte order | **Closed 2026-11-16: the CIF CAN plugins use a LITTLE-endian pcap packet header.** `CAN_RW_CORE` `Init` sets the NI pcap class to `pcap format.Endian` = Little Endian (used by the parse VIs) and `OS Endian` = Little Endian (used by the build VIs); the SocketCAN header (CAN ID) is always big-endian. Measured with `Test_C_CIF.vi` and NI's `Parse Next Socket CAN.vi` on the PXIe: with `little endian` = True the parser gives TAI 1735689637000000000 ns (correct), with False 2776986727000000000 ns (wrong). So the class sets `little endian` = **True** in `CANsub_Pcap_Options` and in W17. (The decision "big-endian" of 2026-11-02 was the default of the NI class alone.) |

## 6. Steps

| Step | What | Test |
|---|---|---|
| **M0** | the move (section 4) | project opens from the new place; smoke test + loopback example |
| **R1** | C library 1.3.0: `csb_frames_to_pcap_range` (`first`, `max_packets`, allow and deny lists, `next`); build, unit tests, deploy. **Code done 2026-11-13:** 2221 checks pass (also ASan/UBSan), cross-check 2000 cases 0 mismatches, `dist\` and `wizard\` built. **On the PXIe 2026-11-16:** 2221 checks ALL PASS, CROSSCHECK PASS (pcap and range lines, 0 mismatches), `.so` installed in `/usr/local/lib` (sha256 `dc12221e…101e0575`). Windows: 2221 checks pass, DLL copied; `Test_C_Smoke` on the PXIe returns **66304**. **R1 done 2026-11-16** (commit `54cc6c0`, local only) | C tests on the PC and the target |
| **R2** | **Done 2026-11-16.** Generated VI `csb frames to pcap range.vi`; W16 `api\Pcap\C_Frames_To_CIF.vi` (`CANsub_Frames` → pcap, inputs `first`, `max packets`, `options` = `CANsub_Pcap_Options.ctl`; output `CANsub_Pcap_Results.ctl`); W17 `wrappers\C_CIF_Encode_RT.vi` (pcap → WebSocket bytes, `C_CIF_Encode_Result.ctl`) | `Test_C_Frames_To_CIF.vi`: 11 runs pass (range, limit, lists, errors, byte order), no buffer copies. `Test_C_CIF.vi`: round trip W10 → decode → W16 → W17 identical (241 bytes, 10 packets), buffer-limit and byte-order-mismatch runs as expected, NI parser reads the 10 packets (IDs, payloads, time) |
| **R3** | **Done 2026-11-16.** API method `Write_Raw` (`channel`, `bytes in`/`bytes out`, `length`, `n frames`): parent in `CANsub_Channel` (must override, 5208 body), override in `CANsub_DirectChannel` (`Check_Open` → `Send_TX`). `Send_TX` has a new optional input `frames (1)` (default 1) for the `frames written` count | `Test_API_Write_Raw.vi` on the PXIe: one call with 10 frames on CH1, 10 frames read on CH2 (IDs 0x100–0x109), `frames written` = 10. Regression: loopback example 30 s, missing 0, `frames written` = sent |
| **R4** | **Done 2026-11-17.** Project `src\Tests\Test CAN RW CANsub.lvproj`. Class `CAN RW CANsub.lvclass` (child of the installed `CAN_RW_CORE`; private data `channel`, `frames`, `next frame`, `pcap out` 154528 B, `ws out` 11136 B, `pcap options`, `Use OS Timestamp`, `Transmit`). `Utilities\Parse Interface.vi` (error **5301**), `Utilities\Make Config.vi` (private). `Core\Initialize.vi` (static; `little endian` = True; the output of `CAN_RW_CORE` `Init.vi` keeps the child wire type, no cast), overrides `Start Reader.vi`, `Start Writer.vi`, `Stop.vi`. Needed first: VIPM package CIF CAN RW Core for LabVIEW 2025 (it was installed for 2023 only) | `Test Open Close.vi` on the PXIe: reader and writer open (`error_active`) and close (`stopped`) with no error; 3 wrong interface strings → 5301 at once; host with no device → error 363500 from the HTTP client GET in `CANsub_REST_Setup` |
| **R5** | **Done 2026-11-17.** Override `Read.vi`: socket read only when no frames remain (`next frame` ≥ `count`), then W16 with `first` = `next frame` and `max packets` = `number to read`; `Timeout` = no packet and no error; `Update Stats` (read = packets + filtered). Override `Write.vi`: W17 → API `Write_Raw` → API `Read` with timeout 0 for the TX acknowledgements → `Check Consumed.vi` (error **5302**) → `Update Stats` (rejected frames count as filtered). `ws out` buffer now 305544 B (1756 × 174), so `Write` needs no loop | `Test Streaming.vi` on the PXIe (writer CH1, reader CH2; packets from NI `Build Socket CAN.vi`, result read with NI `Parse Next Socket CAN.vi`): run 1 `number read` 10/0/0, `Timeout` F/T/T, IDs and payloads correct, stats 10/0, packet time 1794941385107754000 ns TAI (= the time of the run, µs resolution); run 2 (`number to read` 4) 4/4/2; run 3 (deny 0x102, 0x105) 8 packets, filtered 2; run 4 (last packet cut by 5 bytes) `number written` 9 and error 5302 "22 of 265 pcap bytes were not converted" (the reads after it are skipped by the error wire of the test) |
| **R6** | **Done 2026-11-18.** `common\Typedefs\CANsub_Device_Status.ctl` (labels = the JSON names of the device, with underscores), `common\CANsub_REST_Open.vi`, `CANsub_REST_Status.vi` (GET `/api/can/{ch}` with an open client), `CANsub_REST_Close.vi`. API method `Device_Status` (parent 5208 + Direct override) with `api\Private\Status_Call.vi`: the HTTPS client stays open in the channel object (`status client`, `status client open`), is opened on the first call, closed after an error and in `Close`; after a close the object holds an empty handle and the flag False. Class: `Utilities\Convert Device Status.vi` (`error_passive` → Error Passive, `bus_off` → Bus Off, all other → Error Active; counters limited to 255; `Last Error` = None, backlog B13), `Utilities\Update CAN Status.vi` (public, the name the plugins call) | `Test CAN Status.vi` on the PXIe: healthy channel = Error Active 0/0; one frame with no acknowledgement = Error Passive, TX counter 128; with an acknowledging node = Error Active, TX counter 127. Call time: see P8 |
| **R7** | **In work (2026-11-18).** Reader plugin: replace the class, configuration, gRPC (`GetDevices` returns an empty list, `SetInterfaceBlink` does nothing; the proto file does not change). **Done first (R7a):** CIF plugins are developed under My Computer (Windows) and the plugin repo has a Windows build project, so the class must load on Windows. The three `libc.so.6` calls (`CANsub_TLS_Connect.vi`: `setsockopt`, `getsockopt`; `CANsub_Set_Keepalive.vi`: `setsockopt`) are now in a Conditional Disable Structure (`OS == Linux`; `Default` = no option set), and the library name is a path constant on the diagram ("Specify path on diagram"). Found by test: with the name in the node configuration, LabVIEW on Windows searches for `libc.so.6` at load time also when the node is in a subdiagram that is not active; with the path on the diagram it does not. Windows socket options: backlog B15. Plugin class private data: element `XNET CAN RW` replaced with `CAN RW` (`CAN RW CANsub.lvclass`); this save removed the save version 2021 from the plugin class (see R10). `Update External Clock.vi`: the XNET call `Get Time Pair.vi` has no CANsub equivalent, so the `True` case is empty in Phase 1 (`Publish Clock` ignored, backlog B16). Plugin VIs that call XNET class VIs (found by a search of the VI files): `Prepare.vi`, `Update CAN Status.vi`, `Update External Clock.vi`, `gRPC Server.vi`; not changed in R7: the two test helpers and `UI\Plugin\SubVIs\U8 Array to CAN Display.vi` | R7a: under My Computer no `libc.so.6` prompt and no broken Run arrow; on the PXIe `Test Open Close.vi` with no error and cable pull → 5203 with the cause (keepalive still active). Plugin: interactive test project |
| **R8** | **Built 2026-11-18, not tested yet.** Writer plugin: private data element `CAN RW` (`CAN RW CANsub.lvclass`); `Prepare.vi` (`Initialize` with `TX Interface` and `Port Configuration`, `Filter` and `Use OS Timestamp` not connected, then `One Shot TX`, `Start Writer`, `Reset Statistics`), `Update CAN Status.vi`, `Logic.vi` and `Read Comm to Main.vi` (`Write`), `Update Configuration.vi` (`One Shot TX`), `Cleanup.vi` (`Stop`), `gRPC Server.vi` (as the reader), default `TX Interface` = `192.168.0.49/1`. No `Update External Clock.vi` in the writer. Phase 1 limits: `One Shot` has no effect (the parent class stores it, our `Write` does not use it); `ACK Errors` not confirmed | interactive test project |
| **R9** | bench test with the CIF Manager, both plugins, different channels | end-to-end frames, latency numbers |
| R10 | packaging: C source into `src\Wrapper` (CMake), library name and install path, installers, VIPM. **Save version (decided 2026-11-18):** the CIF Foundation sources use save version LabVIEW 2021 (`NI.Lib.SourceVersion` 0x21008000). Our four libraries and the class are in 2025 format, and the two plugin classes lose the 2021 setting when they hold our class. In R10: separate compiled code in our libraries, set save version 2021 on them and on the class, then set it again on the plugin classes; one commit, after the plugin tests. Not confirmed: that all functions we use exist in LabVIEW 2021. Rule until then: no function newer than LabVIEW 2021. **Save version done 2026-11-18 (commit `06ed9fc`):** all 171 VIs and controls, 4 libraries, 3 classes and 2 projects of this repo are in 2021 format; LabVIEW showed no compatibility item. Method that worked: (1) save version `21.0` in each library and class (Properties > General Settings) and in each project (Project > Properties); (2) **File > Save for Previous Version** (21.0) of each project into a new folder; (3) copy the files that are still in 2025 format over the originals (LabVIEW closed). Found by test: the setting and **Save All** convert no VI; a mass compile converted 96 of 171; a plain save and a forced compile + save of one VI (`C_CRC32.vi`) did not convert it although the indicator showed 21.0 (cause not known). The execution settings of the copies were compared with the committed files (inline, reentrancy, debugging, priority): no difference. `src\.lvversion` (`21.0`) gives the save version to VIs in no library. Still to do: set save version `21.0` again on the two plugin classes | install on a clean target |
| P2 | Phase 2: shared connection for the same channel. Needs first: do the two plugins run in one LabVIEW process? | – |
