# CANsub common LabVIEW library: build instructions

> **Working document for step S3** of `labview\c_client\README.md` (shared VIs C1–C7). The
> library and the shared typedefs already exist (S0, done). Start at "C1 `REST_Call.vi`".

`CANsub_Common.lvlib` holds everything that is the same in both clients:
- the **shared typedefs**;
- **REST setup**, **TLS connect** (with Nagle off), the **WebSocket handshake** and the **close**.

It is used by:
- `labview\c_client\CANsub_C_Client.lvproj`: the C-wrapper client (**active**);
- `labview\native_client\`: the native LabVIEW client (**parked** since 2026-10-23).

Created 2026-10-23 from the native build doc (`hil_test\LABVIEW_NATIVE_CLIENT_BUILD.md`, sections
2.1–2.3, 3.1–3.2 and 4.1). The text of those sections is unchanged here, except for three things:
- the host is a parameter;
- the sections are numbered C1–C7;
- `CANsub_WS_Close` (C7) is new. It uses no codec VIs, so both clients can use it.

**This file is now the reference for these VIs.** The native build doc points here.

**[verify]** = not confirmed on this target. Check it on the first run and tell me the result.

## Device address: a parameter everywhere

No document and no VI contains a fixed IP address:
- **VIs:** every VI that talks to the device has a `host` control (string).
- **Shell commands on the target:** set the address once per shell, then the commands use
  `$CANSUB_HOST`:
  ```
  export CANSUB_HOST=<device IP>     # see labview\c_client\README.md, "Bench settings"
  ```

## Library, folder, settings

- Folder: `C:\Dev\CANSub\labview\common\`. Library: `CANsub_Common.lvlib` in that folder. Both
  projects add the library under the RT target.
- `cansub_root_cert.crt` is in this folder, as a reference copy. On the target it is
  `/home/lvuser/cansub_root_cert.crt`.
- The rules for byte data, error terminals and string constants with `\r\n` are the same as in
  the native build doc ("Common rules"). In short:
  - byte data is a **U8 array** (strings only at TCP Write and in the HTTP text);
  - every VI here has `error in` / `error out` on the bottom corners, with the error wire chained
    through every node;
  - a string constant with `\r\n` needs **'\' Codes Display**.
- **VI Properties → Execution**, two groups only:

| Group | Used for | Reentrancy | Priority | Inline | Execution system | Auto error handling |
|---|---|---|---|---|---|---|
| **L** | library VIs C1, C2, C4, C6, C7 | Preallocated clone | Normal | No | Same as caller | Off |
| **T** | test VIs C3, C5 | Non-reentrant | Normal | No | Standard | Off |

### LabVIEW style (both clients, from 2026-11-06)

- **Shift registers: few, and grouped.** Values that travel together through a loop (results,
  counters, statistics) go in **one cluster shift register**: one **Unbundle By Name** at the
  start of the loop, one **Bundle By Name** at the end. The `input cluster` of the Bundle comes
  from the left shift register wire, and **every** field is wired (an unwired field keeps its old
  value without a warning). A test-only cluster needs no typedef: make its start value with
  **Create » Constant** on the indicator. If a loop needs more than about 5 shift registers,
  group them.
- **Separate shift registers stay for:**
  - the **error wire**;
  - **RT buffers** that a C wrapper changes in place (for example `C_RX_Buffers` through W13).
    Nothing goes between the shift register and the wrapper: this is the pattern that P3-RT
    measured with no buffer copy;
  - **input data-path state** (for example `pending` bytes).
- **Configuration that travels together is a typedef** (for example `CANsub_connection.ctl`,
  `CANsub_Bit_Rates.ctl`). Its labels are the same as the subVI terminal names, so
  **Unbundle By Name** shows clear names. Larger configuration typedefs contain these as parts.
- **Wire clusters, not unbundled arrays, into a For Loop**, and unbundle inside. A cluster makes a
  non-indexing tunnel; an array auto-indexes by default (a common wiring error). Unbundle only
  reads, so there is no copy.
- **Split a loop body into small subVIs by responsibility.** Pattern: a *classify* subVI returns
  a cluster with an **enum typedef** `kind`; a Case Structure on `kind` calls **one subVI per
  case** (S5: `Echo_Classify` → `Test`: `Echo_Send_Echo`, `Ack`: `Echo_Send_Report`, `Other`:
  pass through). Use enums, not I32 Select chains, for case selectors. Put each in-place pair on
  the same connector row (input left, output right). Make subVIs testable without the device.
- **Do not change the connector of a tested low-level VI** only to take a cluster. The caller
  unbundles the cluster once, and wires the fields.
- **Gate a start time with the error wire.** Put the **Tick Count (ms)** for `t0` in a one-frame
  **Flat Sequence** and wire the previous error through the frame. Otherwise it can run before
  the previous step ends.
- **Timeouts in a read loop:** clear error 56 **inside** the loop when the loop must wait for
  data (C6, C7, S4). Clear it **after** the loop when a timeout must stop the loop (C5).

### Shared typedefs (in `CANsub_Common.lvlib`)

| File | Type | Used by |
|---|---|---|
| `Bytes.ctl` | U8 array | all array typedefs below |
| `Payload.ctl` | cluster: `payload` (`Bytes.ctl`) | decoders (both clients) |
| `CAN_Frame.ctl` | cluster: `t_dev` (DBL), `id` (U32), `ide`, `fdf`, `rtr`, `brs`, `esi`, `tx_ack`, `error` (Bool), `err type` (U8), `dlc` (U8), `data` (`Bytes.ctl`) | decoders, main VIs |
| `WS_Frame.ctl` | cluster: `opcode` (U8), `payload` (`Bytes.ctl`) | WS parsers |
| `Echo_Report.ctl` | cluster, in this order: `seq` (U16), `idx` (U8), `pad` (U8), `t_rx2_dev`, `t_echo_txack2_dev`, `t_rx2_lv`, `t_echo_send_lv` (DBL) | main VIs |
| `HTTP_Method.ctl` | enum `GET`, `PUT`, `POST`, `DELETE` | C1 |
| `REST_Phase.ctl` | enum `before_ws`, `after_ws` | C2 |
| `Bit_Timing.ctl` | cluster (all U16): `brp`, `seg1`, `seg2`, `sjw`, `data brp`, `data seg1`, `data seg2`, `data sjw` | C2, the bit-timing VIs |
| `CANsub_connection.ctl` (lowercase "c", kept on purpose) | cluster: `host` (String, empty), `channel` (U8, 2), `cert path` (Path, `/home/lvuser/cansub_root_cert.crt`) | S4 test, later `CANsub_Config.ctl` (API) |
| `CANsub_Bit_Rates.ctl` | cluster: `nominal bit rate` (U32, 500000), `nominal sample point` (DBL, 80 %), `data bit rate` (U32, 2000000), `data sample point` (DBL, 80 %) | S4 test (→ W9 `C_Bit_Timing`), later `CANsub_Config.ctl` |

**Native only:** `HDLC_State.ctl` and `WS_State.ctl` stay in `labview\native_client\Typedefs\`.
**C only:** `C_State.ctl` lives in the C client project.

### Move the existing typedefs into the common library (DONE in S0, 2026-10-28; kept for reference)

The native subVIs link to the typedefs by path. A move in Windows Explorer or with git breaks
these links. Let LabVIEW do the move:

1. Open `labview\CANsub_Loopback.lvproj` (the native project), so that every caller is in memory.
2. Under the RT target: **New → Library**, save it as `labview\common\CANsub_Common.lvlib`.
3. Project Explorer, **Files** tab: drag `Bytes.ctl`, `Payload.ctl`, `CAN_Frame.ctl`,
   `WS_Frame.ctl`, `Echo_Report.ctl`, `HTTP_Method.ctl` and `REST_Phase.ctl` from
   `native_client\Typedefs` to `common\Typedefs`. LabVIEW asks to update the callers: answer yes.
   **[verify]** On the Files tab, LabVIEW moves the file on disk and relinks the open callers.
   If your version does not offer this, use **File → Save As → Rename** on each typedef, with
   all callers open.
4. **Items** tab: add the moved typedefs to `CANsub_Common.lvlib`. Create `Bit_Timing.ctl` new,
   in `common\Typedefs`, inside the library.
5. **File → Save All.** Then open each native subVI once: none may show a missing-typedef error.
6. Commit: git sees the moves as renames.

---

### C1 `REST_Call.vi`

**Settings:** group **L**: Preallocated clone, Normal, Inline No, same as caller, automatic error handling off.

One helper for all REST calls, so that status handling exists in one place.

Connector: `client handle` (in), `method` (enum typedef `HTTP_Method.ctl`: `GET`, `PUT`, `POST`,
`DELETE`), `url` (string), `body` (string), `error in` → `client handle out`, `status` (I32),
`response` (string), `error out`.

1. **Case Structure** on `method`. In each case, use one HTTP Client VI. Wire `client handle`,
   `url`, timeout `5000`, `error in`:
   - `GET` → **GET.vi**. `DELETE` → **DELETE.vi**.
   - `PUT` → **PUT.vi** (polymorphic instance **PUTBuffer**), `buffer` = `body`.
   - `POST` → **POST.vi** (instance **POSTBuffer**), `buffer` = `body`.
   - Each case gives `headers` and `body` to output tunnels.
2. Status: **Match Regular Expression** (`headers`, regex `^HTTP/\S+ (\d+)`). Resize the node to
   show the first submatch → **Decimal String To Number** (I32) → `status`.
   **Confirmed by probe A (2026-10-08):** `headers` starts with the status line, e.g.
   `HTTP/1.1 200` (the device sends no reason phrase) or `HTTP/1.1 500 Internal Server Error`.
   **HTTP errors do not set `error out`** (the 500 came back with no error), so this status
   check is the only way to detect a failed call.
3. `body` → `response`.

### C2 `CANsub_REST_Setup.vi`

**Settings:** group **L**: Preallocated clone, Normal, Inline No, same as caller, automatic error handling off.

Connector: `host` (string, the device address; no default IP), `channel` (U8, `2`), `cert path` (path), `phase`
(`REST_Phase.ctl`), `bit timing` (`Bit_Timing.ctl`, from `C_Bit_Timing` in the C client or `CANsub_Bit_Timing` in the native client), `error in` →
`api version` (string), `error out`.

**Common part (both phases)**

1. **OpenHandle.vi** (`verify server` = **FALSE**) → **ConfigSSL.vi** (`CA certificate file` = `cert
   path`, `verify server` = **FALSE**) → **AddHeader.vi** (`header` = `Content-Type`, `value` = `application/json`).
   **Measured 2026-11-04 (C1 test on the PXIe):** with TRUE, every call fails with error **363507**
   ("could not verify the authenticity of the server"): we connect by IP and the certificate is for
   the device name; ConfigSSL has no hostname input. With FALSE: status 200, `"05.00"`. The REST
   calls stay encrypted but unauthenticated (accepted for the private bench network; noted in the
   S3 ticket). The WebSocket connection (C4, Start TLS) keeps the certificate-chain check.
2. Base URL: **Format Into String**, format `https://%s/api/can/%d`, inputs `host`, `channel`
   → `ch url`.
3. At the end of each phase: **CloseHandle.vi**.

If every call fails with an SSL error: set `verify server` = FALSE on **both** VIs, **write it in
the report**, and
continue. The certificate cannot match an IP address, and `ConfigSSL` has no hostname input.

**Case `before_ws`** (calls in this order, error wire in series):

| # | Call | Check | On failure |
|---|---|---|---|
| 1 | `GET https://<host>/api/version` | `status` = 200. **Search/Split String** (`response`, search `05.00`) → `offset of match` **≥** `0`. `api version` = `response`. (The body is the JSON string `"05.00"` with the quotes, 7 bytes, confirmed by probe A.) | error out "API version" + `response` |
| 2 | `GET <ch url>` | **Unflatten From JSON**, `type and defaults` = cluster {`state` (string)}, `strict validation?` = FALSE (the default; extra JSON fields are then allowed). `state` = `stopped` | error out "channel busy: " + `state`. Close webCAN or the other client. If a previous run of this VI did not close: `curl -sk -X DELETE https://$CANSUB_HOST/api/can/2/ws` |
| 3 | `GET <ch url>/filter` | Unflatten From JSON, `type and defaults` = I32 array → `ids` | |
| 4 | For Loop over `ids`: `DELETE <ch url>/filter/<id>` | `status` = 200 each | |
| 5 | `GET <ch url>/transmit` → For Loop: `DELETE <ch url>/transmit/<id>` | 200 each | |
| 6 | `PUT <ch url>/phy`, body = built from `bit timing` (below) | `status` = 200 | error out "phy rejected" + `response` (the device did not accept the timing) |

Body for call 6: **Unbundle By Name** (`bit timing`, all 8 elements) → **Format Into String**
with this format string (one line):
```
{"listen_only":false,"auto_reset":true,"error_frames":true,"tx_ack_frames":true,"timing":{"brp":%d,"seg1":%d,"seg2":%d,"sjw":%d},"timing_data":{"brp":%d,"seg1":%d,"seg2":%d,"sjw":%d}}
```
Inputs in this order: `brp`, `seg1`, `seg2`, `sjw`, `data brp`, `data seg1`, `data seg2`,
`data sjw`. With the defaults (500 kbit/s, 2 Mbit/s, 80 %) the body is
`…"timing":{"brp":2,"seg1":63,"seg2":16,"sjw":16},"timing_data":{"brp":2,"seg1":15,"seg2":4,"sjw":4}}`.
`timing_data` is always sent; it only matters for FD frames with BRS.

To make a status check: **Not Equal?** (`status`, `expected`) → **Case Structure**, with
`expected` = `200` for GET, PUT and DELETE, and `201` for the two filter POSTs in `after_ws`. In the True case,
**Error Cluster From Error Code.vi** (code `5000`, message from the table) → **Merge Errors** with
the error wire.

**Case `after_ws`**:

| # | Call | Check |
|---|---|---|
| 1 | `POST <ch url>/filter`, body `{"is_extended":false,"is_range":false,"f1":0,"f2":0}` | `status` = 201 |
| 2 | `POST <ch url>/filter`, body `{"is_extended":true,"is_range":false,"f1":0,"f2":0}` | `status` = 201 |

**Without these two POSTs, the device sends no frames and gives no error.**

### C3 Test: REST setup

Stop the harness. Close webCAN. Run `before_ws` alone. Then, on the RT target shell:

```
curl -sk https://$CANSUB_HOST/api/can/2/phy; echo
curl -sk https://$CANSUB_HOST/api/can/2/filter; echo
```

Expect (default bit rates 500 kbit/s / 2 Mbit/s / 80 %, from the bit-timing VI): `phy` shows
`"timing":{"brp":2,"seg1":63,"seg2":16,"sjw":16}`, `"timing_data":{"brp":2,"seg1":15,"seg2":4,"sjw":4}`
and `"tx_ack_frames":true`. The filter
list is `[]`. Run `before_ws` again: no error (the VI is idempotent).

**Result 2026-11-05 (PXIe-8862, CH2, device 192.168.0.49): PASS.** Run 1: `api version` = `05.00`,
no error. Run 2: no error. `phy` = the expected timing, `"tx_ack_frames":true`. `filter` = `[]`.
`transmit` = `[]`. Still **[verify]**: the format of a `/transmit` list that is not empty (X3 assumes
a plain ID list, the same as `/filter`). Check it the first time a sequence exists on the device.

After the test, CH2 has no filters. python-can-cansub (the harness) installs its own filters when
it opens a channel, so harness runs are not affected. webCAN does not do this: it shows no frames
on CH2 until a filter exists. To restore them, open CH2 once with python-can-cansub. Running
`after_ws` alone while no WebSocket is open may not work **[verify]**.

### C4 `CANsub_TLS_Connect.vi`

**Settings:** group **L**: Preallocated clone, Normal, Inline No, same as caller, automatic error handling off.

Connector: `host`, `port` (U16, `443`), `cert path`, `error in` → `TLS connection` (network
refnum), `nodelay` (I32, read back), `error out`.

The order is important. Nagle must be set off on the plain TCP connection, **before** Start TLS.

**Changed 2026-11-05 (before the build):** the TLS configuration (step 3 below) is made **first**,
before TCP Open. Reasons: a bad `cert path` then gives an error before a socket exists, and the
device closes a port-443 connection without TLS after a short time, so there must be no delay
between TCP Open and Start TLS. Nagle check: `setsockopt` return 0, `getsockopt` return 0 and
`nodelay` 1, else error **5003**. On any error before Start TLS, the VI closes the TCP connection
itself (Case Structure on the error wire). The two CLFNs can be copied from
`native_client\test vis\Probe_Socket.vi` (tested on the PXIe, probe B).

1. **TCP Open Connection** (`address` = `host`, `remote port or service name` = `port`,
   `timeout ms` = `5000`)
   → `TCP connection`.
2. **Nagle off**:
   - **TCP Get Raw Net Object.vi** (`TCP connection`) → `raw net object` (U64) → **To Long
     Integer** (I32) → `fd`. Probe B confirmed that the U64 holds the fd. A Linux file
     descriptor is a small `int`, so I32 is the correct type for `setsockopt`.
   - **Call Library Function Node**: library `libc.so.6` (no leading `/`), function
     `setsockopt`, calling convention **C**, thread **Run in any thread**. Return type: Numeric, Signed 32-bit.
     Parameters, in this order:

     | Name | Type | Pass | Value |
     |---|---|---|---|
     | `sockfd` | Numeric I32 | Value | `fd` |
     | `level` | Numeric I32 | Value | `6` (IPPROTO_TCP) |
     | `optname` | Numeric I32 | Value | `1` (TCP_NODELAY) |
     | `optval` | Numeric I32 | **Pointer to Value** | `1` |
     | `optlen` | Numeric U32 | Value | `4` |

     The return value must be `0`.
   - **Read it back** (this is the proof for the report): a second Call Library Function Node,
     function `getsockopt`, same library and settings. Parameters: `sockfd` I32 value, `level`
     I32 `6`, `optname` I32 `1`, `optval` I32 **Pointer to Value** (input `0`), `optlen` U32
     **Pointer to Value** (input `4`). Return must be `0`. The `optval` output → `nodelay`
     indicator. **It must be `1`.**
3. **TLS configuration**:
   - **New TLS Configuration** (`load OS trusted CAs?` = FALSE).
   - **Load Certificates Into Memory** (`path` = `cert path`, `format` = X.509 (PEM)) →
     `certificates` (array of DER byte arrays).
   - **For Loop** over `certificates`. A shift register carries the configuration through
     **Add Trusted Certificate To TLS Configuration**.
   - **Make TLS Configuration Immutable**.
4. **Start TLS**: `timeout ms` = `5000`, `TCP connection`, `immutable TLS configuration`,
   `server certificate validation` = **Any Trusted CA**, **`server hostname` unwired**.
   → `TLS connection`. From now on, use `TLS connection` with **TCP Read** / **TCP Write**.
5. **Close TLS Configuration** on the immutable configuration, after Start TLS. Wire its `error in`
   from a **Clear Errors** of the error wire, so it also runs when Start TLS failed. Then **Merge
   Errors** with the main error wire.

**Result 2026-11-05 (PXIe-8862, 192.168.0.49): PASS.** Built as above; VI review OK. Quick test 1:
no error, `nodelay` 1, valid refnum. Quick test 2 (cert path does not exist): error 7 from Load
Certificates Into Memory, `nodelay` 0, refnum invalid (no socket opened). NI help does not say whether Make
TLS Configuration Immutable leaves the mutable configuration open. **Measured 2026-11-05:** a second
Close TLS Configuration on the mutable configuration (the For Loop shift register) returns **error 1**
(invalid refnum): Make TLS Configuration Immutable consumes the mutable configuration. **C4 closes
only the immutable configuration.** (A first attempt wired the second Close to the immutable output
by mistake and gave no error; that is a double close of the immutable refnum, not evidence for the
mutable one.)

If Start TLS fails: the TCP connection is closed (NI docs). Run it again once with
`server certificate validation` = **None**. If that works, the certificate file is the problem,
not the TLS stack. For the comparison you can continue with None; **write it in the report**.

### C5 Test: `Test_TLS.vi`

**Settings:** group **T**: Non-reentrant, Normal, Inline No, Standard, automatic error handling off.

1. Front panel: `host` (string), `cert path` (path). `CANsub_TLS_Connect` → `TLS connection`.
2. **TCP Write** of: **Format Into String** with format `GET /api/version HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n`
   ('\' codes display) and input `host`
3. **While Loop**: **TCP Read**, mode **Immediate**, `bytes to read` `1024`, `timeout ms` `500` →
   **Concatenate Strings** (shift register) → string indicator. Stop on any error, or after 2 s
   (Tick Count). Error 66 (connection closed by the peer, expected with `Connection: close`) and
   56 (timeout) at the end are normal. One read is not enough: the device can send the headers
   and the body in separate TLS records.
4. **TCP Close**.

Expect: the reply starts with `HTTP/1.1 200` and contains `"05.00"`. `nodelay` = 1.

**Result 2026-11-05 (PXIe-8862, 192.168.0.49): PASS** (`pass` TRUE, no error). As built: a `pass`
LED = AND of `nodelay` 1, Match Pattern `^HTTP/1\.1 200` and `"05\.00"`; the start Tick Count is
in a one-frame Flat Sequence gated by the TCP Write error; Clear Errors 66 then 56 after the loop.

### C6 `WS_Handshake.vi`

**Settings:** group **L**: Preallocated clone, Normal, Inline No, same as caller, automatic error handling off.

Connector: `TLS connection`, `host`, `channel` (U8), `error in` → `TLS connection out`,
`leftover` (U8 array), `error out`.

1. **Format Into String**. Format (`\` codes display):
   ```
   GET /api/can/%d/ws HTTP/1.1\r\nHost: %s\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\nSec-WebSocket-Version: 13\r\n\r\n
   ```
   Inputs: `channel`, `host`. → **TCP Write**.
   The key is the RFC 6455 example. We do not check `Sec-WebSocket-Accept`. Write both facts in
   the report.
2. **While Loop**, shift register `rx` (string, init empty). Before the loop: **Tick Count (ms)** →
   `t0`.
   - **TCP Read**, mode **Immediate**, `1024`, `timeout ms` `200` → **Concatenate Strings**
     (`rx`, data) → `rx`.
   - If error code = 56 (timeout), clear it: **Unbundle By Name** (`code`) → **Equal?** `56` →
     **Select** (`is 56` ? no-error cluster constant : the error wire).
   - `found` = **Search/Split String** (`rx`, search `\r\n\r\n`) → `offset of match` **≥** `0`.
     The search string constant needs '\' Codes Display.
   - Stop if: `found` **Or** (**Tick Count** − `t0` **>** `2000`) **Or** error.
3. After the loop:
   - **Search/Split String** (`rx`, `\r\n\r\n`) → `substring before match` = `headers`;
     `match + rest of string` → **String Subset** (offset `4`) → **String To Byte Array** →
     `leftover`.
   - **Match Pattern** (`headers`, `^HTTP/1[.]1 101`). If there is no match, error out with
     `headers`.

**Changed 2026-11-05 (before the build):** extra output `headers` (string, for the report); the 56
timeout is cleared with **Clear Errors** (code 56) instead of Select; the start Tick Count is gated by
the TCP Write error (Flat Sequence, as in C5); no `101` → error **5004** `WS handshake failed: %s`
(headers). Quick test `c_client\tests\Test_WS_Handshake.vi`: C2 `before_ws` → C4 → C6 → C2
`after_ws` → wait for Stop → TCP Close; curl `/filter` shows 2 filters while it runs; free the channel
afterwards with `curl -sk -X DELETE https://$CANSUB_HOST/api/can/2/ws` until C7 exists.

**Result 2026-11-05 (PXIe-8862, 192.168.0.49, CH2): PASS.** `headers` = `HTTP/1.1 101 Switching
Protocols`, `Connection: Upgrade`, `Upgrade: websocket`, `Sec-WebSocket-Accept:
s3pPLMBiTxaQ9kYGzzhZRbK+xOo=` (the RFC 6455 section 1.3 value for the example key, so the device
computes it correctly), `User-Agent: Zephyr`. `leftover` = 0 bytes. With the WebSocket open:
`/filter` = `[0,64]` (a non-empty list is a plain ID list: X3 format confirmed for `/filter`; IDs are
not consecutive), and `/api/can/2` = `{"state":"error_active","frame_count":0,...}`: while open, `state`
is the CAN controller error state (`error_active` = normal), not "running". Also: `api version`
`05.00`, `nodelay` 1, no error. After Stop (TCP close only), `curl -X DELETE .../api/can/2/ws` freed
the channel: `"state":"stopped"`. This confirms C7 step 5 (DELETE /ws) frees the channel.

`leftover` holds the first WebSocket bytes if the device sent frames in the same TCP segment.
The main loop must not lose them: feed them to the decoder before the first TCP Read.

### C7 `CANsub_WS_Close.vi` (codec-free)

**Settings:** group **L**: Preallocated clone, Normal, Inline No, same as caller, automatic error handling off.

This replaces 4.2 of the native build doc. That version used `WS_BuildFrame` and
`WS_ParseFrames` (G codecs). This one needs no codec, so both clients can use it.

Connector: `TLS connection`, `host`, `channel` (U8), `cert path`, `error in` → `error out`.

1. **Clear Errors** on `error in` (the close must always run). Keep the original error for step 6.
2. Close frame (opcode 8, close code 1000, client mask `12 34 56 78`): U8 array constant
   `88 82 12 34 56 78 11 DC` (hex). This is the expected output of test b in build doc 1.6, and of
   `csb_ws_build_frame(03 E8, opcode 8, mask 0x12345678)`. → **Byte Array To String** → **TCP Write**.
3. **While Loop**, max 2 s (**Tick Count**, as in C6): **TCP Read**, Immediate, `4096`, `200` ms.
   Ignore the bytes: these are the device's last frames and its close reply. Stop when:
   - **error 66** (the device closed the TCP connection: the normal end of the close handshake), or
   - **2 s** have passed, or
   - any other error.

   Note in a Boolean `closed by device` whether the loop ended with error 66. Clear the 56
   timeouts, as in C6. **[verify]** The device closes TCP after its close reply (RFC 6455: the
   server closes first). If you see only timeouts, C7 still works: step 5 handles it.
4. **TCP Close Connection** (it runs also with an error in).
5. If NOT `closed by device`: **OpenHandle** / **ConfigSSL** (as in C2) → **REST_Call** `DELETE`
   `https://<host>/api/can/<channel>/ws` (**Format Into String** `https://%s/api/can/%d/ws`) →
   **CloseHandle**. This frees the channel on the device and discards its queued frames. Status
   200 or 404 (no connection any more) are both fine.
6. **Merge Errors**: the original error first, then the errors of steps 2–5.

**Result 2026-11-06 (PXIe-8862, 192.168.0.49, CH2): PASS.** Quick test `c_client\tests\Test_WS_Close.vi`
(copy of `Test_WS_Handshake.vi`, TCP Close replaced by C7), 2 runs in a row: `closed by device` = True,
no error, the VI ended < 1 s after Stop, `/api/can/2` = `"state":"stopped"`; run 2 `before_ws` passed,
so C7 freed the channel. **[verify] closed:** the device closes TCP after the WebSocket close
(RFC 6455), so the DELETE fallback did not run (that path was tested by hand with curl in C6).
As built: the refnum goes through the loop tunnel to TCP Close. After the close, `/filter` =
`[0,64]`: **filters stay on the device after a WebSocket close.** `before_ws` must keep deleting them
(it does); a client that skips `before_ws` gets the previous session's filters.

**Test:** at the end of `Test_Receive` (C client step 3). After C7, `curl -sk
https://$CANSUB_HOST/api/can/2` must show `"state":"stopped"`. Note whether `closed by device` was
True: that confirms the [verify] above.
