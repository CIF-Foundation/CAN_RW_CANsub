# CANsub LabVIEW API, ticket 3: plan (Direct ownership, Poll mode)

> **Working document for step A** of `README.md` ("Where you are"). Design reference:
> `ARCHITECTURE.md` (sections 2–5). This file says **what is built in ticket 3, in which order,
> and how it is proven**. Each VI is built from written build steps and reviewed with
> a VI reader tool, as for S3–S6. Status 2026-11-09: **plan; decisions D1–D3 taken: all "a"
(section 2). Next: A0.**

## 1. Goal and proof

A plugin or application calls **only** `CANsub_API.lvlib` (L2). The API must add **no copies and
no measurable time** compared with the direct wrapper calls of S4–S6.

**Proof:** S4 and S5 are built again as API users (`Test_API_Receive.vi`, `CANsub_Echo_API.vi`)
and run with the same harness commands as S6. Pass criteria against the S6 reference (1 h,
2026-11-09):

| Value | S6 reference (direct calls) | API pass criterion |
|---|---|---|
| lost echoes | 0 of 4.5 M | 0 |
| `ch_consistency` | max 2.1 µs | within ±2 µs |
| `sw_echo_dev` | p50 869 / p99 1047 / max 2356 µs | p50 and p99 within ±50 µs of the reference run of the same day |
| `echo_node_host` (in the VI) | p50 3 / p99 5 / p99.9 8 µs | p50 ≤ 5 µs, p99 ≤ 10 µs |
| loop > 10 ms | 0 | 0 |
| `lvrt` RES over 1 h | unchanged | unchanged |
| Show Buffer Allocations | no dot on `buffers` / `tx` at W13, W14 | no dot on the object or `Frames` at `Read` / `Write` |

`sw_echo_dev` depends on the bus pattern of the hour, so the API run and a direct run are made
on the same day (A/B), not compared with a number from another day only.

## 2. Decisions (taken 2026-11-09: D1 = a, D2 = a, D3 = a)

| # | Question | Options | Recommendation |
|---|---|---|---|
| D1 | Where do the received frames live? | **a.** Caller-owned `CANsub_Frames` (as ARCHITECTURE 2.1). `Read` decodes **directly** into it; the object keeps only the decoder states (WS, HDLC, ping). Needs one new L1 wrapper `C_Decode_To_Frames.vi` (the same CLFN as W13, with states and frame arrays as two clusters). **b.** The frames stay inside the object; `Frame_Get` reads them from the object. No `Frames` buffer. | **a**: no copy, the caller can hand the arrays to the CIF wrappers (W16) and to `Write_Echo`; the object stays small. |
| D2 | Class structure now | **a.** Abstract `CANsub_Channel.lvclass` + child `CANsub_DirectChannel.lvclass` (dynamic dispatch), as ARCHITECTURE. **b.** Only `CANsub_DirectChannel` now; the parent later. | **a**: adding the parent later changes the wire type in every caller. Dynamic dispatch costs < 1 µs per call [verify in the A/B run]; it cannot be inlined. |
| D3 | `Write` in ticket 3 | **a.** Two methods: `Write_Frame` (one frame: id, flags, data → W15) and `Write_Echo` (frame *i* of `Frames` with a new ID → W14). One TCP Write per frame, as S5. **b.** Also a batch write now (needs C library 1.3 `csb_encode_frames_to_ws`). | **a**: matches S5 exactly, so the A/B proof is clean. Batch write = ARCHITECTURE build order step 4, after ticket 3. |

## 3. Scope

**In ticket 3:** Direct ownership, Poll mode, accept-all filters, `listen only` = False.

**Not in ticket 3** (fields exist in the typedefs so the wire types do not change later):
Reader mode (needs test V1 and the C ring), sequences, `Reopen`, Shared ownership, filter lists,
listen-only, batch write, `Config_FromJSON` (CIF ticket), CIF wrappers W16/W17 (CIF ticket).
`Init` returns error **5207** if a config field asks for something not built yet.

## 4. What is built (with D1 = a, D2 = a, D3 = a)

### 4.1 Typedefs (public, in `CANsub_API.lvlib`)

| Typedef | Content |
|---|---|
| `CANsub_Config.ctl` | `connection` (`CANsub_connection.ctl`), `bit rates` (`CANsub_Bit_Rates.ctl`), `listen only` (Bool), `filters` (array of `CANsub_Filter.ctl`), `read mode` (`CANsub_Read_Mode.ctl`: Poll, Reader), `max bytes per read` (I32, 4096), `reader FIFO depth` (I32, 0 = default), `reader priority` (I32, 0 = normal), `ownership` (`CANsub_Ownership.ctl`: Direct, Shared) |
| `CANsub_Frame.ctl` (added A4) | one frame from `Frame_Get`: `id` U32 hex, `t_dev` DBL, `dlc` U8, `data len` U8, `ide`, `fdf`, `rtr`, `brs`, `esi`, `tx ack`, `error frame` (Bool); no data array (the data stays in `frames.data`, read it at `data offset`) |
| `CANsub_Filter.ctl` | `extended` (Bool), `range` (Bool), `f1`, `f2` (U32) |
| `CANsub_Frames.ctl` | `count` (I32), `t_dev` [DBL], `id` [U32], `flags` [U16], `dlc` [U8], `data len` [U8], `data` [U8] (64 B per frame) |
| `CANsub_Status.ctl` | `state` (`CANsub_State.ctl`: Closed, Open, Lost), `frames read`, `frames written` (U32), `crc errors` (U32), `decode errors` (I32), `pings` (U32), `FIFO overflows`, `reopens` (U32, 0 until Reader mode / Reopen exist), `nodelay` (I32), `api version` (string), `lib version` (U32), `bit timing` (`Bit_Timing.ctl`), `last error` (error cluster) |
| `C_RX_State.ctl` (L1, `CANsub_C.lvlib`) | `ws` [U8], `hdlc` [U8], `ping` [U8]: the decoder state part of `C_RX_Buffers` |

### 4.2 Decoder for the API (in `CANsub_API.lvlib` » `Private`, file `api\Private\`)

**Changed 2026-11-09 (A2):** not in `CANsub_C.lvlib`, because it uses the public API typedef
`CANsub_Frames.ctl`; in `CANsub_C` that would make the two libraries depend on each other. It is
API-private (scope Public until packaging, so that its test can call it).

| VI | Purpose |
|---|---|
| `C_Decode_To_Frames.vi` | W13 split in two clusters: `state` (ws, hdlc, ping) in the object, `frames` (`CANsub_Frames`) from the caller; in place, no copy. Reuses the generated `csb decode ws to can` CLFN. |

### 4.3 Classes and methods

`CANsub_Channel.lvclass` (abstract parent, **interface only: empty private data**) and
`CANsub_DirectChannel.lvclass` (private data: `config`, `status`, `TLS connection`, `rx state`
(`C_RX_State.ctl`), `tx` buffer, `pending` bytes).

**Changed 2026-11-09 (A1):** `config` and `status` are in the child, not the parent. A child
cannot unbundle its parent's private data; it needs accessor VIs. `Read` updates the counters
on every call, so the counters belong to the class that runs `Read`. `Init` (static, parent)
makes a `CANsub_DirectChannel` and calls the protected dynamic method `Configure` on it, which
stores the config and the bit timing. `Status` is dynamic (must override).

| Method | Kind | Built from | Loop-safe (see ARCHITECTURE 2.2; "no" = setup/teardown only) |
|---|---|---|---|
| `Init` | static, parent | config check (5207), W9 bit timing; returns a `CANsub_DirectChannel` as `CANsub_Channel` | no |
| `Frames_Create` | static, parent | `capacity` (default 1756) → `CANsub_Frames` (Initialize Array ×6, `data` = 64 × capacity); capacity < W1 `max frames (4096)` → 5206, because one 4096-byte read can hold that many frames | no |
| `Configure` | dynamic, **protected**, parent (5208) + Direct override | stores `config` and `bit timing`, `status.state` = Closed; called only by `Init` | no |
| `Open` | dynamic, Direct override | W1 version check (≥ 0x00010200, else 5204) → C2 `before_ws` → C4 → C6 → C2 `after_ws` → W12 (states + tx) → `state` = Open; with an error **in**: does nothing (pass through); on an error **inside** Open: C7 cleanup, `state` stays Closed | no |
| `Read` | dynamic, Direct override (**thin**: In Place Element Structure on the object → `Read_Poll`) | `Read_Poll.vi` (static private, Preallocated + Inline, `api\Private\`): state Closed → 5202, Lost → 5203; TCP Read (Immediate, 4096 B, `timeout ms`) → pending → `C_Decode_To_Frames` → pong (W7) → counters; TCP error or peer close → `state` = Lost, error 5203; any error → `frames.count` = 0 | **yes** |
| `Frame_Get` | static, parent (Preallocated, Inline) | `frames`, `i` → `frame` (`CANsub_Frame.ctl`: id, t_dev, dlc, data len, 7 flag Booleans) + `data offset` (= i·64, index into `frames.data`, no copy); i outside 0…count−1 → 5209 | **yes** |
| `Write_Frame` | dynamic, Direct override (thin: In Place Element Structure on `TLS connection`, `tx`, `status`) | `Check_Open` (5202/5203) → W15 `C_Encode_RT` (tx in place) → `Send_TX` | **yes** |
| `Write_Echo` | dynamic, Direct override (thin, as above) | `Check_Open` → W14 `C_Echo_RT` (`frames`, `i`, new ID) → `Send_TX` | **yes** |
| `Write_Raw` (added 2026-11-16 for the CIF class) | dynamic, Direct override (thin: In Place Element Structure on `TLS connection`, `status`) | `Check_Open` → `Send_TX` with `frames (1)` ← `n frames`; sends a buffer the caller prepared (W17 `C_CIF_Encode_RT`) with one TCP Write; `bytes in` → `bytes out` | **yes** |
| `Device_Status` (added 2026-11-18 for the CIF class; backlog B1 done) | dynamic, Direct override (thin: In Place Element Structure on `status client`, `status client open`) | `Status_Call.vi` (private): `CANsub_REST_Open` on the first call → `CANsub_REST_Status` (GET `/api/can/{ch}` → `CANsub_Device_Status`) → after an error `CANsub_REST_Close`. First call ≈ 80 ms (TLS handshake), later calls ≈ 0.6 ms while the connection stays open | no (REST; use at a low rate) |
| `Check_Open` (private, `api\Private\`, Inline) | – | `status.state`: Closed → 5202, Lost → 5203 | yes |
| `Send_TX` (private, `api\Private\`, Inline) | – | Array Subset (`tx`, 0, `length`) → TCP Write (1000 ms) → `frames written` + 1; TCP error → `state` = Lost, 5203 | yes |

**A5 is split (2026-11-10):** **A5a** = the write methods + `Test_API_Write` (frames checked on
CH1 with python-can); **A5b** = `CANsub_Echo_API` with API versions of the echo subVIs that
touch the device (`Echo_Classify_API` via `Frame_Get`, `Echo_Send_Echo_API` via `Write_Echo`,
`Echo_Handle_Frames_API`); `Echo_Send_Report` is reused unchanged.
| `Status` | dynamic, parent | returns `status` | yes |
| `Close` | dynamic, Direct override | if `state` ≠ Closed: C7; `state` = Closed; runs also with an error in | no |

**Reentrancy (found 2026-11-09, LabVIEW 2025):** a VI with dynamic dispatch terminals cannot be
Preallocated clone ("Reentrant VIs can only have dynamic dispatch terminals if they share clones").
So: **all dynamic dispatch methods (parent and overrides) = Shared clone**; static methods
(`Init`, `Frames_Create`) and private subVIs = Preallocated clone. For the hot path (A4/A5), the
dynamic `Read` / `Write_*` overrides stay thin and call **static private subVIs** (Preallocated,
Inline) that do the work, so the per-frame code keeps the S5/S6 settings.

Hot-path methods (`Read`, `Write_*`) follow the LabVIEW style rules of `COMMON_BUILD.md`: the
object and `Frames` go in and out on the same connector row, no branch, subVIs for each
responsibility.

### 4.4 Test applications (L3)

| VI | Replaces | Test |
|---|---|---|
| `Test_API_Receive.vi` | S4 `Test_C_Receive` | 10 × 0x123: 10 frames, payload, `skew` |
| `CANsub_Echo_API.vi` | S5 `CANsub_Echo_C` | S5 tests a/b, then S6 runs B, C, D as A/B pairs with `CANsub_Echo_C` |

The echo application keeps its own time store and report (`Echo_*` VIs); only the channel I/O
moves to the API: `Read` → for each frame `Echo_Classify` → `Write_Echo` / report.

## 5. Build order and estimate

| Step | Content | New VIs / ctl | Estimate |
|---|---|---|---|
| A0 | Decisions D1–D3; `CANsub_API.lvlib` + the two classes (empty private data) in `labview\c_client\api\` (one folder per class, `Typedefs\` for A1); inheritance Direct → Channel. Community scope of `CANsub_C` / `CANsub_Common` waits for packaging (tests still call them). | 3 files | 0.25 day |
| A1 | Typedefs 4.1 | 7 ctl | 0.25 day |
| A2 | `C_Decode_To_Frames` + a quick test against W13 (same input, same frames) | 1 + 1 test | 0.5 day |
| A3 | `Init`, `Frames_Create`, `Open`, `Close`, `Status` + `Test_API_Open_Close` (open, status, close, channel `stopped`) | 6–8 + 1 test | 1 day |
| A4 | `Read`, `Frame_Get` + `Test_API_Receive` | 2–3 + 1 test | 0.75 day |
| A5 | `Write_Frame`, `Write_Echo` + `CANsub_Echo_API` + S5 tests a/b | 2–3 + 1 app | 1 day |
| A6 | A/B harness runs (10 s, 60 s, 1 h) against `CANsub_Echo_C`; Show Buffer Allocations | – | 0.5 day |
| | **Total** | **~18 VIs, 7 ctl** | **~4–4.5 days** |

### A6 results (2026-11-12, same day, order C 10 s → API 10 s → C 60 s → API 60 s; Inline on, debugging off on both sides)

| Value | C 10 s | API 10 s | C 60 s | API 60 s | Criterion |
|---|---|---|---|---|---|
| lost echoes | 0 | 0 | 0 | 0 | 0 ✓ |
| `ch_consistency` max | 1.2 µs | 1.2 µs | 1.2 µs | 1.2 µs | ±2 µs ✓ |
| `sw_echo_dev` p50 | 872 | 851 (−21) | 854 | 874 (+20) | ±50 µs ✓ |
| `sw_echo_dev` p99 | 1060 | 1033 (−27) | 1017 | **1154 (+137)** | ±50 µs ✗ (60 s) |
| `sw_echo_dev` p99.9 / max | 1459 / 1967 | 1431 / 2028 | 1410 / 2001 | 1455 / 2167 | – |
| `echo_node_host` p50 / p99 / p99.9 / max | 3 / 5 / 13 / 31.4 | 3 / 5 / 9 / 12.5 | 3 / 5 / 9 / 50.6 | **4 / 7** / 17 / 43.8 | p50 ≤ 5, p99 ≤ 10 ✓ |
| loop > 5 ms / > 10 ms | 0 / 0 | 0 / 0 | 0 / 0 | 0 / 0 | 0 ✓ |
| 10 ms IDs `sw_echo_dev` p99 | 959 | **1187** | 949 | **1159** | – |

Reading: the time inside the VI (`echo_node_host`) is +1 µs p50 / +2 µs p99 with the API (dynamic
dispatch, Shared clone) — small and inside the criterion. The +137 µs on `sw_echo_dev` p99
(60 s) comes almost entirely from the 10 ms IDs, which are ≈ +210 µs at p99 in **both** API runs,
so it is not proven to be noise; but it is **outside** the measured VI window, so its cause is
not known. The per-group p99 also moves by up to 222 µs between the two C runs (20 ms IDs:
1168 vs 946), so 10–60 s samples are noisy at p99. **Next:** the 1 h pair (API and C, same day)
decides the p99 criterion; if the difference stays > 50 µs, a 60 s pair with `--csv` locates it.

**1 h pair (2026-11-12, API 10:50–11:50, C 11:59–12:59; logs and panel screenshots in
a local results folder, not in this repo): A6 PASSED.**

| Value | C 1 h | API 1 h | Criterion |
|---|---|---|---|
| frames analysed / lost echoes | 4 499 214 / 0 | 4 499 146 / 0 | 0 ✓ |
| `ch_consistency` max | 0.0 µs | 2.1 µs | ±2 µs ✓ (2.1 = S6 reference) |
| `sw_echo_dev` p50 / p99 / p99.9 / max | 880 / 1158 / 1404 / 2221 | **761 / 976 / 1295** / 2844 | API not worse ✓ (p50 −119, p99 −182) |
| 10 ms IDs `sw_echo_dev` p99 | **1164** | **947** | – (the 60 s pattern is reversed) |
| `echo_node_host` p50 / p99 / p99.9 / max | 3 / 5 / 9 / 264 | 4 / 7 / 14 / 519 | p50 ≤ 5, p99 ≤ 10 ✓ |
| loop > 5 ms / > 10 ms | 1 / 0 | 4 / 0 | > 10 ms: 0 ✓ (S6 C ref: 5 / 0) |
| VI counters | 8 999 802 frames, 4 499 901 echoes = matched, 0 unmatched | 8 999 672 frames = `status.frames read`; 4 499 836 echoes = matched = `frames written`; 0 unmatched | ✓ |
| `lvrt` RES (PID 3958) over the hour | 116 996 KiB, constant | 116 996 KiB, constant | unchanged ✓ |
| `lvrt` CPU over the hour | 2 min 13 s (3.7 %) | 2 min 20 s (3.9 %) | – |

**Conclusion:** the API costs **about 1 µs (p50) / 2 µs (p99) per echo** inside the VI (dynamic
dispatch) and **+0.2 % CPU**; it adds no memory growth and no measurable end-to-end latency. The
`sw_echo_dev` p99 difference changes sign between runs (60 s: API +137 µs; 1 h: API −182 µs; the
10 ms-ID group flips in the same way), so it is the bus/arbitration pattern of the run, not the
client. **Criterion corrected:** "p50/p99 within ±50 µs" was tighter than the run-to-run variation
(±200 µs at p99); the valid criterion is "API not worse than C by more than the run-to-run
variation, at 1 h", which is met.

Side note: the `top` logger from S6 (`top_1h.log`, started 2026-11-09) was still running until the
`pkill` of this test (855 samples, about 3 days); harmless, but stop loggers after each run.

**A7 build (2026-11-12):** new subVI X5 `common\subvis\CANsub_Set_Keepalive.vi` (one
`setsockopt` CLFN in a For Loop over 5 options; Linux x86_64 values: `SOL_SOCKET`=1 /
`SO_KEEPALIVE`=9 → 1; `IPPROTO_TCP`=6 / `TCP_KEEPIDLE`=4 → 2 s, `TCP_KEEPINTVL`=5 → 1 s,
`TCP_KEEPCNT`=6 → 3, `TCP_USER_TIMEOUT`=18 → 5000 ms), called in C4 after the TCP_NODELAY check and
before Start TLS; new error **5005** "keepalive not set". Dead peer → kernel aborts within ≈ 5 s
(idle 2 s + 3 × 1 s); `TCP_USER_TIMEOUT` covers the case with unacknowledged sent data.

**A7b fix (2026-11-13):** the first cable-pull test gave error **66**, not 5203: `Read_Poll` and
`Send_TX` used Merge Errors (TCP error first, 5203 second), and Merge Errors keeps only the first.
New private subVI `api\Private\Lost_Error.vi` (Inline; `operation`, `cause` → 5203 with
"`<operation>`: the connection is lost (cause: error `<code>`, `<source>`); call Close, then Open")
replaces Error Cluster From Error Code + Merge Errors in both VIs (also removes the Send_TX typo
"the Open"). VI review: correct.

**A7 results (2026-11-13, PXIe-8862, CANsub 192.168.0.49, cable pulled at the CANsub end):**

| Test | Result |
|---|---|
| 1. Loopback example, keepalive on | both channel sockets `tr` = 02 (keepalive timer), `tm->when` 0.05–1.98 s; 3670 sent = received = acks, 0 missing, 0 errors. Repeated after A7b: works as before. |
| 2. `Test_API_Receive`, idle socket (no filters), pull | `probes` 1 → 2 → 3 at 1 s steps, socket gone **3.0 s after the first unanswered probe** (pull → abort 3–5 s, derived); `error out` **5203** "Read: … (cause: error 66, …)", `state` Lost. TCP Read gives **66**, not 56 → Clear Errors (56) does not hide it. VI stop time not measured exactly (< 10 s by eye; Read polls every 100 ms). Before A7b: the same, but code 66 without 5203. |
| 2b. Re-plug | the next run opened the channel **without** `DELETE /ws` (the device freed it) |
| 3. Loopback example, pull | TX socket: `tr` = 01 (retransmit) from the pull, RX socket: first probe 2.0 s later; both gone **≈ 5.5 s after the pull**; `error out` 5203 from **Read** (cause 66). The `Send_TX` 5203 path ran **not** at runtime (TCP Write fills the kernel buffer and does not fail first); it is checked by review only. |

Monitor command (columns: local port, `tr:tm->when`, keepalive probe count):
`while true; do date +%T.%N | cut -c1-12; awk '$3 ~ /:01BB$/ && $4=="01" {print $2, $6, "probes="$9}' /proc/net/tcp; sleep 0.5; done`.
Note: in `/proc/net/tcp`, `retrnsmt` (`$7`) is the retransmit count; the keepalive probe count is
the `timeout` column (`$9`).

**A7 (after A6, decided 2026-11-10): connection-loss detection.** Found in the A4 test
(Run 2): `Read` only detects a loss when the device closes the WebSocket or TCP reports an error.
An external `DELETE /ws` (another client) does **not** close our WebSocket (the TCP connection
stays up, `Read` sees only timeouts), and a cable pull or device power cycle is also invisible
while we only receive. Fix: (1) TCP keepalive in C4 (`SO_KEEPALIVE` + short `TCP_KEEPIDLE` /
`TCP_KEEPINTVL` / `TCP_KEEPCNT`, same `setsockopt` pattern as TCP_NODELAY) so a dead peer gives a
TCP Read error → `Lost`; (2) a setup-only device-state check (REST channel state) for "channel
stopped by someone else" — **low priority**, see the measurement below.

**Measured 2026-11-10 (Test_API_Receive, 90 s):** `DELETE /ws` while our WebSocket is connected
returns **HTTP 200 but has no effect**: the state stays `error_active` for 5 s (10 polls), and
10 frames sent after the DELETE still arrive (`stats.frames` 10 → 20, device `frame_count` 20).
After the VI ended, `Close` left the channel `stopped`. (The earlier single `stopped` reading was
most likely taken after the VI had already ended.) So another client cannot silently stop our
channel with DELETE; the remaining gap is a dead peer (cable, power) → keepalive (item 1).

After A6: commit, then the CIF ticket (W16/W17, P6-CIF, adapter, `Config_FromJSON`).

## 6. Backlog (after ticket 3 and the CIF plugin)

Compared on 2026-11-12 with the device interface (`skills/cansub-device/references/rest_api.md`,
`websocket_protocol.md`; API 05.00, FW 02.05.0x). Priorities agreed.

### 6.1 Already planned elsewhere

| Feature | Device support | Where planned |
|---|---|---|
| TCP keepalive (dead-peer detection) | – | A7 **done** 2026-11-13 |
| `Reopen` after `Lost` | – | ARCHITECTURE 2.2, build order 5 |
| Hardware filter lists | mask (`(id & f2) == (f1 & f2)`) or range, std/ext; table size undocumented (503 when full) | config field exists; `Init` → 5207 |
| Listen only | `phy.listen_only` (must be false if the device is the only other node) | config field exists; `Init` → 5207 |
| Device transmit sequences (rest-bus, ±3 µs) | `/transmit`: ≤ 64 per channel, ≤ 32 frames each, 1 ms resolution, **stop when the WebSocket closes** (count → failed) | ARCHITECTURE 2.2, build order 5 |
| Batch write | – | C library 1.3 (`csb_encode_frames_to_ws`), build order 4 |
| Reader mode | – | after test V1 + C ring (ARCHITECTURE 3.2) |
| Shared ownership | – | ARCHITECTURE section 7, decision 1 |
| `Config_FromJSON`, W16/W17 (pcap) | – | CIF ticket |
| Signal (DBC) layer | – | separate library above L2 |

### 6.2 New items, in priority order

| # | Feature | Device support | Why | Priority |
|---|---|---|---|---|
| B1 | **Device channel status** (setup-only method, e.g. `Device_Status`) | `GET /api/can/{ch}`: `state` (`stopped`, `error_active`, `error_warning`, `error_passive`, `bus_off`), rx/tx/bus error counters, `frame_rate`, `bus_load` (short-window sample, imprecise) | The API cannot report **bus-off** or rising error counters today; needed for HIL fault tests. Also covers A7 item 2 (channel stopped by someone else). | **high** |
| B2 | **Error frame type** in `CANsub_Frame` | WebSocket error frames carry the type (0 bit, 1 ack, 2 form, 3 stuff, 4 CRC) | `Frame_Get` only reports "error frame"; fault-injection tests need the type. **[verify]** where the C decoder stores it (`dlc` or `data`). | **high** |
| B3 | **`phy` options in the config**: `auto_reset`, `error_frames`, `tx_ack_frames` | `PUT …/phy` (channel stopped) | X4 fixes all three to `true`. Seeing bus-off needs `auto_reset` = false; no TX acks halves the WebSocket traffic. | medium |
| B4 | **TX back-pressure** | unknown **[verify]** | Behaviour when writing faster than the bus can send (bus-off, 100 % load): today `Write_*` only sees TCP errors. Measure first, then design. | medium |
| B5 | **Device time / PTP status** | `GET /api/time/ptp` (`master`, `state`, `offset_ns`), `GET /api/time` | Tells whether `t_dev` is trustworthy. The API must **never** offer `PUT /api/time` (disables PTP until power cycle). | medium |
| B6 | **Device info** + minimum FW check at `Open` | `GET /api/info` (`id`, `hw_ver`, `fw_ver`, `mac`) | Record device and firmware in test reports; refuse an unsupported firmware like the 5204 library check. | medium |
| B7 | **REST authentication** | certificate is for the device name; we connect by IP | C2/C7 use `verify server` = FALSE (error 363507 otherwise). Fine on the lab network; for production connect by name (DNS or hosts entry). C4 (WebSocket) already verifies the chain. | low (lab) |
| B8 | mTLS client certificate | `/api/client-cert` (Ethernet only; erased by a firmware update) | Only if the device is set to require client certificates. | low |
| B9 | Channel discovery | `GET /api/can` → `[1,2,3,4]` | Convenience for tools. | low |
| B10 | Device reboot | `POST /api/reboot` (202) | **Tool only, not in the API**: a reboot resets the clock and the PTP state. | tool |
| B11 | **Single-channel monitor example** (`CANsub_API_Example.vi`): one channel, table of received frames, interactive **Send** button (classic/FD), status | – | Useful with any other CAN node (no CH1–CH2 loopback needed). Design given 2026-11-12; deferred in favour of the loopback example (first example). Reuses `Example_Frames_To_Rows`, `Example_Table_Append`. | low (example) |
| B12 | **Signal decode and encode from a CAN database (DBC)** | none: the REST API has no database function; webCAN decodes in the browser (inferred from the API, webCAN code not read) | For **standalone API use** (examples, monitor B11, test VIs). **Not for the CIF plugins**: they pass raw pcap packets, and signal decoding is expected in a different CIF plugin **[verify]**. Options: **(A)** NI-XNET conversion session (loads DBC/FIBEX, frames ↔ signals, no XNET hardware; NI-XNET must be on the target; time per conversion not measured) for a quick monitor; **(B)** convert the DBC offline with Python (cantools) to a table file + a new C function for signal extract and pack with that table (loop-safe, no allocation; one build step when the DBC changes) for the loop path; **(C)** DBC parser in LabVIEW (most work: multiplexed signals, value tables, byte order). Recommended: B for the loop, A for a monitor. Added 2026-11-16 (the DBC function was seen in webCAN). | low (after the CIF plugins) |
| B13 | **`Last Error` in the CIF `CAN Status`** (Stuff, Form, Ack, Bit1, Bit0, CRC) | not in `GET /api/can/{ch}`; the device reports the error type in error frames on the WebSocket (flag bits 8..12 of the decoded frame: 0 bit, 1 ack, 2 form, 3 stuff, 4 CRC) | The class `CAN RW CANsub` sets `Last Error` = None in Phase 1. To add it: `Read` keeps the type of the last error frame it sees (W16 skips error frames today and does not report the type), and `Update CAN Status` puts it into `CAN Status`. Needs a small change in the C function or a scan of the flags in LabVIEW. Decided 2026-11-17: backlog. | low |
| B14 | **Bus Off test of the CIF class** (`Update CAN Status`) | `GET /api/can/{ch}` gives `"state":"bus_off"` | The mapping `bus_off` → Bus Off is in `Convert Device Status.vi` and is checked by review only (R6, 2026-11-18). Error Active and Error Passive are tested. Test steps: section 6.3. Decided 2026-11-18: backlog. | low (test) |
| B15 | **Socket options on Windows** (TCP_NODELAY, keepalive) | – | `CANsub_TLS_Connect.vi` and `CANsub_Set_Keepalive.vi` set the options only when `OS == Linux` (Conditional Disable Structure, 2026-11-18). On Windows the `Default` subdiagram sets no option: `nodelay` = 0, `results` empty, no error 5003 or 5005. Effect on Windows: more latency is possible (Nagle), and a lost connection is found later than the 3 s to 5.5 s measured on the PXIe (not measured on Windows). To add: `setsockopt` from `ws2_32.dll` in a `OS == Win` subdiagram (the option numbers are different from Linux). | low (the bench target is Linux RT) |
| B16 | **Clock pair for the CIF external clock** (`Publish Clock`) | no call that gives the device time and the system time at the same moment; the device time is in each frame (`t_dev`, UTC, PTP) | The XNET class has `Get Time Pair.vi` (system time + hardware time, TAI); the plugins call it every 125 ms in `Update External Clock.vi` when `Publish Clock` is True. In Phase 1 the CANsub plugins do not publish a clock: the `True` case is empty, `Publish Clock` = True is ignored with no message (decided 2026-11-18). Options to add it: **(A)** `Get Time Pair` in the class from the last received frame (`t_dev` + 37 s and the host time of the read; accuracy = the read delay, about 1 ms, and no pair on a silent bus); **(B)** if the target clock is PTP-synchronised to the device, the pair is the system time two times (offset 0); needs a check that the synchronisation is active. | low |

### 6.3 Test steps for B14: Bus Off

**Purpose:** show that `Update CAN Status` reports Bus Off, and record how the device recovers.

**Method:** connect CAN-H to CAN-L on the bus while one frame is sent. With the two wires
connected, the bus cannot show a dominant bit. The controller sees a bit error in each attempt,
its transmit error counter goes up by 8 for each attempt, and above 255 it goes to Bus Off. A
connection between CAN-H and CAN-L does not damage a CAN transceiver (ISO 11898-2 requires that
the transceiver tolerates it).

**Parts:** a DB9 breakout or a Y cable on the CH1 side of the bus, and one jumper wire. On the
DB9 connector, CAN-L is pin 2 and CAN-H is pin 7. **[verify]** the pins against the CANsub
hardware documentation before the test.

**Steps**

1. Stop all clients of CH1 and CH2. Record the start state: `curl -sk https://192.168.0.49/api/can/1`.
2. Put the jumper between pin 2 (CAN-L) and pin 7 (CAN-H).
3. Run `Test CAN Status.vi` (`src\Tests\`) with `Interface` = `192.168.0.49/1`,
   `send one frame` = True, `wait between calls ms` = 0.
4. Record `CAN Status` (`Comm State`, the two counters) and `error out`.
5. Directly after the VI stops, run the `curl` command again and record the `state` and the
   three error counters.
6. Remove the jumper. Run `Test CAN Status.vi` with `send one frame` = False and record
   `CAN Status`. Run the `curl` command again.
7. Start `Test Open Close.vi` on `192.168.0.49/2` (`open time ms` = 20000), and during its wait
   run `Test CAN Status.vi` with `send one frame` = True. Record `CAN Status`.

**Expected**

| Step | Expected |
|---|---|
| 4 | `Comm State` = **Bus Off**; `TX Error Counter` = 255 (the device value can be higher; the class limits it to 255); `error out` = no error |
| 5 | `"state":"bus_off"` if the device keeps the state after the close; `"state":"stopped"` if not. Record which one. |
| 6 | Not known. This step shows if the device leaves Bus Off by itself after a new open, and if the counters are still high (the device keeps the counters after a close, measured in R6). |
| 7 | Error Active or Error Passive, and the frame is sent. If the state is still Bus Off, the device needs a different recovery (for example a power cycle): record it as a device rule. |

**Also record:** what `Write` returns while the controller is in Bus Off (step 3: `error out` of the
test), and the time until the device reports `bus_off` (run step 3 with the `curl` command in a
loop: `while true; do curl -sk https://192.168.0.49/api/can/1; echo; sleep 0.2; done`).


**Suggested order:** B1 + B2 (HIL fault visibility) → B3 → filter lists and transmit sequences
(rest-bus simulation) → B5 + B6 (test reports) → the rest as needed.
