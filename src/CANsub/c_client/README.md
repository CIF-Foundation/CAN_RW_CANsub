# CANsub C client (LabVIEW + libcansub_codec.so): build steps

**The active client**, since 2026-10-23. LabVIEW does the networking: REST, TLS and the WebSocket
handshake and close, from the shared `CANsub_Common.lvlib`. The C library `libcansub_codec.so`
does all byte work: the WebSocket frames, HDLC, CRC32, the CAN frame codec and the bit timing.

The native LabVIEW client (`labview\native_client\`, build doc
`hil_test\LABVIEW_NATIVE_CLIENT_BUILD.md`) is **parked**. Its test tables stay the reference data
for the codec tests here.

## Where you are (start here)

**This file is the only list of steps.** Each step points to exactly one working document.

| Step | What | Work from | Status |
|---|---|---|---|
| S0 | project, common library, typedef move | this file, S0 | **done** 2026-10-28 |
| S1 | C library on the PXIe (P0) | this file, S1 (`c_functions\README.md`, "Deploy") | **done** 2026-10-28 (927 checks, bench numbers in `c_functions\README.md`) |
| S2a | 18 generated CLFN VIs (`CANsub_Codec.lvlib`) | `c_functions\WIZARD.md` | **done** 2026-10-29 (reviewed) |
| S2b | wrapper VIs W1–W15 + tests P1–P3-RT | `c_functions\LABVIEW_C_TEST.md` | **done** 2026-11-04 (rows 1–5; row 6, the CIF wrappers, belongs to the CIF adapter ticket) |
| **S3** | **shared VIs C1–C7 (REST, TLS, handshake, close)** | **`..\common\COMMON_BUILD.md`** | **done** 2026-11-06 (C1–C7; C3, C5, C6, C7 tests passed on the PXIe) |
| V1 | duplex test (one TLS connection, two loops) | `tests\Test_V1_Duplex_design.md` | to do (needs S2b + S3) |
| S4 | `Test_C_Receive.vi` | this file, S4 | **passed** 2026-11-06 (10/10 frames 0x123, payload OK, 0 CRC/decode errors, closed by device, stopped; first `skew` 299.869 s = PXIe clock not synced; with `ptp4l` client on the PXIe (same master as the device): `skew` = 0.000486 s. So `t_dev` is **UTC**, not TAI) |
| S5 | `CANsub_Echo_C.vi` (`echo\`) | this file, S5 | **test b passed** 2026-11-09: 10 s, 56 % bus load, 12275 echoes, **0 lost**, loop_latency p50 1.8 / 2.1 / 2.7 ms (5 / 10 / 20 ms IDs), max 3.9 ms, none > 5 ms; VI: 24552 frames = 12276 tests + 12276 acks, 0 unmatched, 0 CRC/decode errors. **Test a passed** (10 × 0x123 received, 0 echoes). Report datagrams: 5 × 36 B, seq 0 / idx 0–4; `t_echo_txack2_dev` − `t_rx2_dev` = 0.71–0.96 ms; `t_echo_send_lv` − `t_rx2_lv` (W14 + TCP Write) = 2.4–15.3 µs |
| S6 | harness bring-up | this file, S6 | **in progress.** Run B (10 s, Inline on, debugging off) 2026-11-09: 12275 echoes, 0 lost, `ch_consistency` 0.0 µs, `sw_echo_dev` p50 774 / p99 1067 / max 1749 µs (Python ref 930 / 1270 / 3000 µs over 1 h), `echo_node_host` p50 3 / p99 6 / max 15.8 µs, loop p50 1.85 / 2.35 / 2.76 ms, max 3.67 ms, none > 5 ms. **Run C (60 s): passed** — 74925 echoes, 0 lost; VI 149852 frames = device `frame_count`, 74926 matched, 0 unmatched; `ch_consistency` max 1.2 µs; `sw_echo_dev` p50 715 / p99 958 / p99.9 1393 / max 2057 µs; `echo_node_host` p50 3 / p99 5 / max 18 µs; loop p50 1.54 / 2.33 / 2.63 ms, max 4.22 ms, none > 5 ms. **Run D (1 h): passed → S6 done** — 4 499 864 echoes, **0 lost**; VI 8 999 730 frames = device `frame_count`, 4 499 865 sent = matched; `ch_consistency` max 2.1 µs; `sw_echo_dev` p50 869 / p99 1047 / p99.9 1392 / max 2356 µs (Python ref 930 / 1270 / max 3000); `echo_node_host` p50 3 / p99 5 / p99.9 8 / max 244 µs; loop p50 1.76 / 2.24 / 2.86 ms, max 5.57 ms, **5 of 4.5 M > 5 ms** (Python ref 3), **0 > 10 ms**; `lvrt` RES unchanged over the hour (108680 / 133060 / 212176 KiB); PTP tracking before and after |
| **A** | **the public API (ticket 3): Direct ownership, Poll mode** | **`API_BUILD.md`** (design: `ARCHITECTURE.md`) | **A0–A6 done** 2026-11-12: Init/Frames_Create/Open/Read/Frame_Get/Write_Frame/Write_Echo/Status/Close; A6 A/B proof passed (1 h: 0 lost, API +1 µs p50 / +2 µs p99 in the VI, no memory growth). **A7 done** 2026-11-13: TCP keepalive (X5 in C4) + `Lost_Error` (5203 keeps the TCP cause); cable pull → 5203 / Lost in ≈ 3–5.5 s, no `DELETE /ws` needed after re-plug. **Next: CIF ticket.** Backlog: `API_BUILD.md` section 6 |
| **E** | **API example: `examples\CANsub_API_Loopback_Example.vi`** (send on CH1, receive on CH2, round-trip time) | build steps in the conversation of 2026-11-12; lvkit review | **done** 2026-11-12: 40 s run, 3999 sent = received = TX acks, 0 missing; host round trip min 384 / mean 529 / max 10 726 µs (one outlier, cause not measured) |
| **F** | **CIF plugins: `CAN RW CANsub.lvclass` (child of `CAN_RW_CORE`) + reader and writer plugins** | **`CIF_BUILD.md`** | plan written 2026-11-13. **Next: M0** (move this code to `C:\Dev\CIF\CAN_RW_CANsub\src\CANsub\`, branch `feature/cansub-rw-class`). Hand-over checkpoint: `HANDOFF.md` |

**Reference only (do not work from these):**

| Document | Use it to look up |
|---|---|
| `c_functions\README.md` | the C library: functions, build, deploy, bench numbers |
| `c_functions\WIZARD.md` | the generated VIs: parameters by function (section 3), the library path (section 5) |
| `ARCHITECTURE.md` | how plugins will use the client (layers, API class, RT rules) |
| `hil_test\LABVIEW_NATIVE_CLIENT.md` | protocol reference (bytes, JSON, report datagram) |
| `hil_test\LABVIEW_NATIVE_CLIENT_BUILD.md` | the parked native client; its test tables are used by P2 |

**[verify]** = not confirmed on this target. Check it on the first run and tell me the result.

## Using the API

**Start with the example** `examples\CANsub_API_Loopback_Example.vi` (sends on CH1, receives on
CH2 through the CH1–CH2 cable, checks the sequence, shows the round trip). Design and rules:
`ARCHITECTURE.md` (sections 2, 4, 5); what is built and measured: `API_BUILD.md`; what is not
built yet: `API_BUILD.md` section 6 (backlog).

**1. Prerequisites on the RT target**

| Item | Where | Check |
|---|---|---|
| `libcansub_codec.so` 1.2.0 or newer | `/usr/local/lib/` (S1, "Deploy" in `c_functions\README.md`) | `Open` gives **5204** if it is missing or too old |
| `cansub_root_cert.crt` | `/home/lvuser/cansub_root_cert.crt` (copy in `..\common\`) | `config.connection.cert path` |
| PTP client (only if you compare `t_dev` with the PXIe clock) | "Bench settings" below | S4 `skew` ≈ 0.5 ms |

**2. Add to your LabVIEW project** (under the RT target): `api\CANsub_API.lvlib`,
`CANsub_C.lvlib`, `codec_vis\CANsub_Codec.lvlib`, `..\common\CANsub_Common.lvlib`. They are used
as source libraries (no packed library). A library can be open in only one project at a time.

**3. The call sequence** (all public items are in `CANsub_API.lvlib`)

```
setup      Init (CANsub_Config) ─► Frames_Create (1756) ─► Open
loop       Read (channel, frames, timeout ms) ─► for i < frames.count: Frame_Get (frames, i)
           Write_Frame (id, data, ide, fdf, brs)   or   Write_Echo (frames, i, new id)
           Status (any time)
teardown   Status ─► Close                      (Close also runs with an error in)
```

Wire `CANsub_Channel` and `CANsub_Frames` from shift register → method → shift register, with no
branch. Use the parent nodes (`CANsub_Channel:…`); the calls dispatch to `CANsub_DirectChannel`.

**4. Rules**

| Rule | Why |
|---|---|
| Only `Read`, `Frame_Get`, `Write_*`, `Status` in a periodic loop (**loop-safe**); `Init`, `Frames_Create`, `Open`, `Close` before/after it (**setup/teardown only**) | the setup methods allocate and do REST/TLS calls |
| One `CANsub_Channel` object per channel; two channels = two objects | the device allows one client per channel |
| **A channel that sends must also be read** (`Read` with timeout 0 is enough) | the device returns one TX ack per sent frame on that channel's connection |
| `Read` waits at most `timeout ms`; 0 = no wait | use the timeout to pace the loop instead of a Wait |
| Payload bytes: `Index Array (frames.data, data offset + k)` | `Frame_Get` returns no data array (no copy) |
| On error **5203** (`state` = Lost): call `Close`, then `Init`/`Open` again outside the time-critical part | `Reopen` is not built yet |

**5. Error codes** (full list: `ARCHITECTURE.md` 2.3)

| Code | Meaning |
|---|---|
| 5000 | REST call returned an unexpected HTTP status (the text names the call) |
| 5001, 5002 | from `Open`: API version not 05.00 / channel not stopped; from `Init` (W9): invalid bit-rate input / no bit timing with one prescaler. The error text tells which. |
| 5003 | TCP_NODELAY could not be set |
| 5004 | WebSocket handshake failed |
| 5005 | TCP keepalive could not be set |
| 5202 / 5203 | channel not open / connection lost (the 5203 text gives the cause, e.g. "cause: error 66") |
| 5204 | C library missing or older than 1.2.0 |
| 5206 | `Frames_Create` capacity below 1756 |
| 5207 | config asks for a feature not built yet (listen only, filter lists, Reader mode, Shared) |
| 5208 | method not implemented by this class (parent body) |
| 5209 | `Frame_Get` index outside 0 … count − 1 |

**6. Measured cost** (A6, 1 h, 2500 frames/s, 56 % bus load): the API adds about 1 µs (p50) /
2 µs (p99) per echo compared with direct wrapper calls, +0.2 % CPU, no memory growth; 0 lost
frames in 4.5 M. **Dead connection** (cable pull, device power off): `Read` gives 5203 and
`state` = Lost about 3–5.5 s later (A7, TCP keepalive), also while the channel only receives.

## Bench settings (the only place for addresses)

| Setting | Where it is used | How |
|---|---|---|
| Device address | `host` control of every VI; shell commands | VIs: type it on the front panel (make it the default with **Data Operations → Make Current Value Default**). Shell: `export CANSUB_HOST=<device IP>` once per shell. |
| RT target address | `scp` from the PC | `<rt-ip>` in the commands |
| Channels | CH1 = harness sender, CH2 = this client | fixed in the test design |
| PXIe clock | every run that uses `t_dev` (S4 `skew`, S5, S6, CIF) | **Manual, not a boot service (decided 2026-11-06).** Stop all VIs, then in two root shells on the PXIe: `ptp4l -i eno0 -s -m` and `phc2sys -s eno0 -c CLOCK_REALTIME -w -m` (hardware time stamps; same master as the device). Without it the PXIe clock drifts (measured 299.9 s off). Check: S4 `skew` ≈ 0.5 ms. For a run in the background: `nohup ptp4l -i eno0 -s > /tmp/ptp4l.log 2>&1 &` and `nohup phc2sys -s eno0 -c CLOCK_REALTIME -w > /tmp/phc2sys.log 2>&1 &`. **Is it running?** `ps aux \| grep -E "ptp4l\|phc2sys" \| grep -v grep` (plain `ps` on NI Linux RT shows only the processes of the current shell session). |

The address is not fixed in any document, because it has changed before. To find it: webCAN, or
`python ex5_loopback.py <ip>` in the repo root. Keep a DHCP reservation for the device: this is
one of the bench design rules.

## Folder layout

```
labview\
  common\                    CANsub_Common.lvlib, Typedefs\, cansub_root_cert.crt, COMMON_BUILD.md
  c_client\                  CANsub_C_Client.lvproj  (this project)
    CANsub_C.lvlib           wrappers + C_State.ctl, C_Info.ctl, C_Decode_Result.ctl, C_RX_Buffers.ctl
    codec_vis\               CANsub_Codec.lvlib (S2a, generated; under the PXIe target)
      VIs\                   csb <name>.vi (18), CANsub_Codec_LibPath.vi, libcansub_codec.dll
    wrappers\                C_*.vi  (W1–W14, S2b)
    tests\                   Test_C_*.vi, Test_CLFN_Path_Cost.vi, Test_TLS.vi, Test_C_Receive.vi
    echo\                    S5: CANsub_Echo_C.vi (main VI), Echo_* subVIs and typedefs (not in an lvlib)
    c_functions\             the C library sources, Makefile, tests, docs
  native_client\             parked
  CANsub_Loopback.lvproj     the native project (also has udp_echo)
```

## Overview

| Step | What | Test / pass criterion | Effort |
|---|---|---|---|
| S0 | Project, common library, prerequisites (**done**) | project deploys an empty VI | 1 h |
| S1 | C library: build on the desktop, copy to the target (**done**) | P0: static test ALL PASS, `crosscheck.py` PASS, bench `--fifo` numbers | 0.5 h |
| S2a | Generated CLFN VIs, `WIZARD.md` (**done**) | `csb version.vi` = 65792 on the PXIe; lvkit review | 0.5 day |
| S2b | Wrapper VIs W1–W14, `LABVIEW_C_TEST.md` | P1 smoke, P2 tables, P3 helpers, P3-RT real-time path | 1 day |
| S3 | Shared VIs C1–C7 | C3 (curl `phy`/`filter`), C5 (`GET /api/version` over TLS, `nodelay` 1) | 1 day |
| S4 | `Test_C_Receive.vi` | decoded IDs, `t_dev` vs `date +%s`, close leaves the channel `stopped` | 0.5 day |
| S5 | `CANsub_Echo_C.vi` | negative check + report datagrams | 0.5 day |
| S6 | Harness bring-up | 10 s / 60 s / 1 h, the same table as every run | 0.5–1 day |

---

## S0: project and prerequisites (done 2026-10-28)

1. **The common library first.** Follow `..\common\COMMON_BUILD.md`, "Move the existing typedefs
   into the common library". Do it in the native project, so that LabVIEW relinks the native
   subVIs. Then `labview\common\CANsub_Common.lvlib` exists with the shared typedefs, including
   `Bit_Timing.ctl`.
2. **New project:** File → Create Project → Blank Project, save it as
   `labview\c_client\CANsub_C_Client.lvproj`. Add the RT target: right-click the project →
   New → Targets and Devices → the PXIe-8862, as in the native project.
3. Under the RT target:
   - **Add → File** `labview\common\CANsub_Common.lvlib`;
   - **New → Library** `labview\c_client\CANsub_C.lvlib`;
   - in it, **New → Type Definition** `C_State.ctl` = U8 array (save it in `labview\c_client\`).
4. **Prerequisites.** These are already confirmed on this target (native build doc, step 0.7). Only
   check that nothing changed:
   - **HTTP Client with SSL Support** installed on the target (NI MAX); without it you get error 7.
   - `cansub_root_cert.crt` at `/home/lvuser/cansub_root_cert.crt`.
   - The CLFN to `libc.so.6` (no leading `/`) works: you use it again in C4.
5. Deploy an empty VI from the new project and run it.

## S1: the C library (build on the desktop, run on the target) (done 2026-10-28)

`c_functions\README.md`, section "Deploy". The `.so` built on desktop Linux runs on NI Linux RT:
it needs only glibc ≥ 2.14 and baseline x86-64, and imports only `memcpy`, `memset`,
`nearbyint`. In short:
1. Desktop Linux: `make dist`.
2. Copy `dist\` to the target.
3. On the target:
   - `sha256sum -c SHA256SUMS`;
   - `./test_cansub_codec_static` (ALL PASS);
   - `python3 crosscheck.py ./libcansub_codec.so` (CROSSCHECK PASS);
   - `./bench_cansub_codec_static --fifo 80` (the RT timing);
   - `cp libcansub_codec.so /usr/local/lib/ && ldconfig`.

This is test **P0**. After any library rebuild: copy the `.so` again, `ldconfig`, restart the
LabVIEW application on the target.

## S2a: the generated CLFN VIs (done 2026-10-29)

`c_functions\WIZARD.md`. Result: `codec_vis\CANsub_Codec.lvlib`, one VI per C function, each
with one CLFN (path on diagram from `CANsub_Codec_LibPath.vi`). These are the only CLFNs in the
project. Keep `CANsub_Codec.lvlib` under the PXIe target.

## S2b: the wrapper VIs (next)

**Work from `c_functions\LABVIEW_C_TEST.md`**, its "Build order" table at the top: typedefs →
W1/W2/W9 + P1 → W3–W8 + P2 → W10/W11 + P3 → W12–W15 + P3-RT. Stop at each test and send me the
values. Only the real-time layer W12–W15 is used inside a loop (S5, the API Read/Write): they work on buffers allocated once, in
place.

## S3: the shared VIs

`..\common\COMMON_BUILD.md`, C1–C7. They go into `CANsub_Common.lvlib`:
- C2 gets its `bit timing` from W9 `C_Bit_Timing`.
- C7 `CANsub_WS_Close` uses no codec VI.

Tests: C3 (REST, with curl on the target) and C5 (`Test_TLS.vi`, in `c_client\tests\`).

---

## S4: `Test_C_Receive.vi` (receive only)

**Settings:** group **T**: Non-reentrant, Normal, Inline No, Standard, automatic error handling off.

Front panel controls: `host`, `channel` (U8, `2`), `cert path`, `nominal bit rate` (U32,
`500000`), `nominal sample point` (DBL, `80`), `data bit rate` (U32, `2000000`), `data sample
point` (DBL, `80`).
Indicators: `ids` (U32 array, hex), `last t_dev` (DBL, 6 digits), `crc errors` (U32), `decode
errors` (I32), `frames` (I32), `error out`.

**Sequence** (error wire in series):
`C_Bit_Timing` → `CANsub_REST_Setup (before_ws)` → `CANsub_TLS_Connect` → `WS_Handshake` →
`CANsub_REST_Setup (after_ws)` → receive loop (10 s) → `CANsub_WS_Close`.

**Redesigned 2026-11-06 (before the build): S4 uses the RT layer** (W12 `C_RX_Buffers_Init`,
W13 `C_Decode_RT`), the same path the API will use, not W11. Extra controls/indicators: `run time s`
(U32, 30), `last data` (U8, hex), `skew s` (host UTC − `t_dev` at arrival), `pings`, `closed by
device`. Note: `nominal/data sample point` are in **percent** (80), not 0.8.

**Receive loop** (**While Loop**). Shift registers: `error`, `buffers` (from W12), `pending` (init
= C6 `leftover`, then empty), `ids`, `frames`, `decode errors`, `pings`, `last t_dev`, `last data`,
`skew`.
1. **TCP Read**, Immediate, `4096`, `100` ms → Clear Errors (56) → **String To Byte Array**.
2. `bytes` = **Build Array** (Concatenate Inputs): `pending`, new bytes. `pending` ← empty array.
3. W13 `C_Decode_RT` (`bytes`, `buffers`) → `result`, `buffers out`.
4. `n frames` > 0: Array Subset of `id` (0, n) onto `ids`; frame n−1: `t_dev`, `data` slot
   ((n−1)·64, `data len`), `skew` = host UTC (Get Date/Time In Seconds − 2082844800) − `t_dev`.
   `frames` += n; `decode errors` += per call. `crc errors` is cumulative in the state: show the
   last value.
5. `ping len` ≥ 0: Array Subset (`ping`, 0, `ping len`) → W7 `C_WS_BuildFrame` (opcode 10) →
   TCP Write (the pong), `pings` + 1.
6. Stop on: error, `peer close`, or `run time s` passed. After the loop: C7.

**Traffic** (on the RT target, `cansub-venv` active): 10 frames with ID `0x123` on CH1 at 500 kbit/s.
```
python -c "import can,os,time; b=can.Bus(interface='cansub',channel=os.environ['CANSUB_HOST']+'@1',bitrate=500000,error_frames=True,max_device_time_skew=None); [(b.send(can.Message(arbitration_id=0x123,is_extended_id=False,data=[i,0x11,0x22,0x33,0x44,0x55,0x66,0x77])),time.sleep(0.1)) for i in range(10)]; b.shutdown()"
```
Do not use `ex3_send.py`: `cansub_config.py` sets 125 kbit/s.

**Expect:**
- `ids` = 10 × `0x123`, `frames` = 10;
- `crc errors` 0, `decode errors` 0;
- `last t_dev` within about 1 s of `date +%s` on the target (both UTC);
- after the VI stops: `curl -sk https://$CANSUB_HOST/api/can/2` shows `"state":"stopped"` (C7
  worked). Note whether C7 saw `closed by device`.

**If nothing arrives:** check, in this order:
1. the `after_ws` filter POSTs;
2. `curl -sk https://$CANSUB_HOST/api/can/2` (`state`);
3. the `pending`/`leftover` wiring;
4. the return value of W11.

---

## S5: `CANsub_Echo_C.vi` (the main VI)

**Settings:** group **T**: Non-reentrant, **Normal** (not Time-critical, not Subroutine), Inline No, Standard, automatic error handling off. Allow debugging off in S6.

It is the native main VI (build doc 5.1) with the codec part replaced by the two helpers. The
classification, the time store and the UDP report are unchanged.

**Restructured 2026-11-06 (agreed with Marcus, LabVIEW style rules):** folder `c_client\echo\`,
not in `CANsub_C.lvlib`. S5a typedefs `Echo_Config.ctl`, `Echo_Counters.ctl`,
`Echo_Time_Store.ctl` (3 DBL arrays, changed with an In Place Element Structure), `Echo_Context.ctl`
(values fixed during a run). S5b subVI `Echo_Handle_Frames.vi` (the For Loop of B.5: classify,
echo with W14, report with UDP; Preallocated clone + Inline). S5c `CANsub_Echo_C.vi` with 7 shift
registers (`error`, `buffers`, `tx`, `pending`, `time store`, `counters`; the indicator update
uses `i`). **Refined 2026-11-06:** typedefs `Echo_Frame_Kind.ctl` (enum Other/Test/Ack) and
`Echo_Frame.ctl`; `Echo_Handle_Frames` calls `Echo_Classify.vi` (testable on the target without
the device: `Test_Echo_Classify.vi`), then `Echo_Send_Echo.vi` or `Echo_Send_Report.vi`. The
text below is the original design; the build steps follow the structure above.

**Front panel controls:**
- `host`, `channel` (`2`), `cert path`;
- `nominal bit rate` (U32, `500000`), `nominal sample point` (DBL, `80`), `data bit rate` (U32,
  `2000000`), `data sample point` (DBL, `80`);
- `echo offset` (I32, `-256`), `base id` (U32 hex, `100`), `report port` (U16, `5302`), `Stop`.

The bit rates must be the same as the harness's (`--bitrate`, `--data-bitrate`).

**Indicators:** `frames received`, `echoes sent`, `TX acks matched`, `unmatched acks`, `crc
errors`, `decode errors`, `nodelay`, `error out`.

**A. Before the loop** (error wire in series)
1. `C_Bit_Timing` → `CANsub_REST_Setup (before_ws)` → `CANsub_TLS_Connect` → `WS_Handshake` →
   `CANsub_REST_Setup (after_ws)`.
2. **UDP Open** (`port` = `5303`) → `udp`. **String To IP** (`127.0.0.1`) → `lv addr` (U32).
3. **Time store:** three DBL arrays, `t_rx2_dev`, `t_rx2_lv` and `t_echo_send_lv`. Make each with
   **Initialize Array** (element `0`, size `720896` = 11 × 65536). Key = `idx` × 65536 + `seq`.
4. **Buffers, states and counters:**
   - W12 `C_RX_Buffers_Init` → `buffers` (both states, frame arrays for 1756 frames, ping) and
     `tx` (174 B). It also runs `csb_warmup`, so the first loop iteration has no page faults.
   - `pending` = `leftover`.
   - counters (U32, `0`): `frames received`, `echoes sent`, `TX acks matched`, `unmatched acks`.

   **Nothing is allocated after this point**, except the strings inside LabVIEW's TCP Read and
   TCP Write (see W14).
5. **Ranges:** `test lo` = `base id`, `test hi` = `base id` + `10`; `ack lo`/`ack hi` = the same +
   `echo offset` (in I32, then **To Unsigned Long Integer**). With the defaults: tests
   `0x100`–`0x10A`, acks `0x000`–`0x00A`.

**B. The While Loop.** Shift registers: `buffers`, `tx`, `pending`, the three time arrays, the
four counters and the error.
1. **TCP Read**, Immediate, `4096`, `100` ms. Clear error 56; any other error → stop.
2. `bytes`: a **Case Structure** on **Empty Array?** (`pending`). True: `bytes` = the new bytes.
   False: `bytes` = **Build Array** (Concatenate) `pending` + the new bytes, and `pending` =
   empty. The concatenation runs only on the first iteration.
3. W13 `C_Decode_RT` (`bytes`, `buffers`) → `buffers` (to the shift register), `result`
   (`C_Decode_Result.ctl`) → **Unbundle By Name**: `n frames`, `peer close`, `ping len`, `crc
   errors`, `decode errors`. `frames received` += `n frames`.
4. `ping len` ≥ `0` → pong: W7 (**Array Subset** (`buffers.ping`, `0`, `ping len`), opcode `10`) →
   **Byte Array To String** → **TCP Write**. (Pings are rare, so W7's allocation is acceptable.)
5. **Unbundle By Name** (`buffers`): `t_dev`, `id`, `flags`, `data_len`, `data`, once, before the
   For Loop. Wire them in with **non-indexing** tunnels (read only, no copy). **For Loop**, N =
   `n frames`. Per frame `i`: `id[i]`, `flags[i]` and `t_dev[i]` with **Index Array**; the Booleans
   from `flags[i]` with **And** + **Not Equal To 0?** (`ide` 0001, `fdf` 0002, `brs` 0008, `tx_ack`
   0020, `error` 0040); `seq` = `data[i·64]` × 256 + `data[i·64 + 1]`, `idx` = `data[i·64 + 2]`
   (**Index Array**; no Array Subset needed); `data_len[i]` ≥ 3 for the test-frame check. Then,
   **as build doc 5.1 B.5**:
   - unbundle; `seq`, `idx` and `key`;
   - `is test` / `is ack`; three cases selected with the two Select nodes;
   - **Case 1** (test frame) in a three-frame **Flat Sequence**:
     - frame 1: **High Resolution Relative Seconds** → `t_rx_lv`;
     - frame 2: **W14 `C_Echo_RT`** (`new id` = `id[i]` + `echo offset`, `index` = `i`, `n frames`,
       and the `flags`, `data_len`, `data` arrays; `tx` from its shift register) → **Array Subset**
       (`tx`, `0`, `length`) → **Byte Array To String** → **TCP Write**. W14 keeps IDE, FDF and BRS
       of the received frame;
     - frame 3: **High Resolution Relative Seconds** → `t_send_lv`;
     - then **Replace Array Subset** in the three arrays, and increment `echoes sent`.
   - **Case 2** (TX ack of an echo): the 36-byte `Echo_Report.ctl` → **Flatten To String**
     (big-endian) → **UDP Write**; clear `t_rx2_dev` at `key`; increment `TX acks matched` (or
     `unmatched acks`).
   - **Case 0**: everything passes through.
6. Indicators every 100 iterations (**Quotient & Remainder**), as in 5.1.
7. Stop if `Stop` OR `peer close` OR an error (not 56). Also stop if W13's error out has an
   error: a negative return value of the library is a bug, not a bus event.

**C. After the loop:** `CANsub_WS_Close` (C7) → **UDP Close** → `error out`; also wire the final
counter values to the indicators.

**Checks on the diagram before the first run:**
- There is no **Wait** in the loop.
- **Flatten To String** gets the cluster (36 bytes, no prefix).
- `echo offset` = −256.
- In the loop only **W13, W14** (and W7 for a pong) call the library: no W1–W11 test wrapper,
  no Initialize Array, no Build Array (except the first-iteration concatenation in B.2), no
  `CAN_Frame` cluster arrays.
- `buffers` and `tx` go from shift register to wrapper and back without a branch. A branch to an
  indicator or a probe makes LabVIEW copy the whole buffer on every iteration.

### S5 test: negative check, then report datagrams

**a. Negative check.** Run the VI. Send ID `0x123` with the command in S4. After 10 s, press Stop.
Expect: `frames received` = 10, `echoes sent` = 0, `TX acks matched` = 0.

**b. Report datagrams.** Run the VI again. On the target, start the listener (build doc 5.2b,
unchanged) in one shell:
```
python3 - <<'EOF'
import socket, struct
s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM); s.bind(("127.0.0.1", 5302)); s.settimeout(30)
R = struct.Struct(">HBxdddd")
for _ in range(5):
    d, _ = s.recvfrom(256)
    print(len(d), R.unpack(d) if len(d) == R.size else "WRONG SIZE")
EOF
```
In a second shell, run the harness, with its own report receiver moved to another port:
```
python cansub_loopback_test.py --channel-tx $CANSUB_HOST@1 --channel-echo $CANSUB_HOST@2 --echo-mode lv --echo-offset=-0x100 --lv-report-port 5399 --duration 10 --csv none
```
Expect:
- 5 lines `36 (…)`, with `idx` 0–9;
- `t_echo_txack2_dev` − `t_rx2_dev` ≈ 1 ms, and `t_echo_send_lv` > `t_rx2_lv`;
- the harness prints `lost echoes` 0.

---

## S6: bring-up with the harness

**Order used (2026-11-09):** S6.0 setup (clock sync with `nohup` for all runs, no listener on
5302) → S6.1 run A 10 s (debugging on; correctness first) → S6.2 Inline on + Allow debugging off
(`Echo_Handle_Frames`, `Echo_Classify`, `Echo_Send_Echo`, `Echo_Send_Report`, `C_Send_Pong`;
debugging off also on `CANsub_Echo_C`) → S6.3 run B 10 s → S6.4 run C 60 s → S6.5 run D 1 h
(`nohup`, `top` at start and end) → S6.6 optional CAN FD 60 s. Reference (Python process echo):
`sw_echo_dev` p50 0.93 / p99 1.27 / max 3.0 ms; loop p50 1.8–2.9 / p99 2.4–3.6 / max 5.1 ms;
`ch_consistency` ≤ 2.1 µs. The original steps below are kept for reference.

1. Start `CANsub_Echo_C.vi` first: it must own CH2 before the harness starts.
2. On the target (`cansub-venv` active, `CANSUB_HOST` set):
   ```
   python cansub_loopback_test.py --channel-tx $CANSUB_HOST@1 --channel-echo $CANSUB_HOST@2 --echo-mode lv --echo-offset=-0x100 --duration 10 --csv none
   ```
   The harness defaults to `192.168.0.40` for both channels; the two `--channel-…` options
   replace that.
3. Read the result table of build doc step 6, in the same order: wrong-size datagrams, report
   count, `ch_consistency`, `lost echoes`, `sw_echo_dev`, `echo_node_host`. In the "If not"
   column, read "`CAN_Deserialize`" and "`CAN_Serialize`" as the C library (W13, W14).
4. Before the long runs: **Allow debugging** off on all VIs of this project and on
   `CANsub_Common.lvlib`. (The generated VIs and W13/W14 are already inline with debugging off.)
   Compare `echo_node_host` with the bench numbers of P0: on the PXIe, decode of a 70-frame
   message takes 8.6 µs and one echo encode 0.07 µs in C, plus about 96 ns per CLFN call from
   LabVIEW (`Test_CLFN_Path_Cost.vi`). A much larger `echo_node_host` points to LabVIEW-side copies or allocations.
5. Then 60 s, then 1 h:
   ```
   python cansub_loopback_test.py --channel-tx $CANSUB_HOST@1 --channel-echo $CANSUB_HOST@2 --echo-mode lv --echo-offset=-0x100 --duration 60 --csv none
   nohup python cansub_loopback_test.py --channel-tx $CANSUB_HOST@1 --channel-echo $CANSUB_HOST@2 --echo-mode lv --echo-offset=-0x100 --duration 3600 --csv none > loopback_1h_c.log 2>&1 &
   top -b -n 1 -u lvuser        # at start and at end of the hour
   ```
6. **Optional CAN FD run:** VI `data bit rate` = `2000000`, then add `--fd --data-bitrate 2000000`
   to the 60 s command.

The comparison numbers: Python echo 0.94 ms, Candidate A (bridge + VI) 1.03 ms. `sw_echo_dev` of
this client is the Candidate B result with C codecs.

## Checkpoints to report back

| After | Send me |
|---|---|
| S0 | the project deploys; the native subVIs still load after the typedef move |
| S1 | the last lines of `make test` and `crosscheck.py` on the target |
| S2a | done |
| S2b | P1 values; failed P2 rows (if any); P3 values; P3-RT values (one message per test) |
| S3 | `nodelay`, the TLS validation mode, the two curl outputs |
| S4 | `ids`, `last t_dev` vs `date +%s`, `state` after close, `closed by device` |
| S5 | negative-check counters; the listener lines |
| S6 | the three result blocks (10 s, 60 s, 1 h) |
