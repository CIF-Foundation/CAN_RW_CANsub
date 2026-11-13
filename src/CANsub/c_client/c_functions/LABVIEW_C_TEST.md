# S2b: wrapper VIs and their tests (C client)

**Working document for step S2b** of `labview\c_client\README.md` (the list of all steps).

S2a is done: the 18 generated VIs `codec_vis\VIs\csb <name>.vi` hold the only Call Library
Function Nodes (CLFNs). In S2b you build **wrapper VIs** on top of them and test them on the
PXIe. Look-ups: the generated VIs' terminals are in `WIZARD.md` section 3; the C functions are in
`cansub_codec.h`; the reference tables for row 3 are in `hil_test\LABVIEW_NATIVE_CLIENT_BUILD.md`.

## 1. Status and build order

Do the rows in order. Stop at each test, send me the values, and save (I check the VIs with
lvkit before the next row).

| Row | Build | Test | Pass | Status |
|---|---|---|---|---|
| 1 | typedefs (section 3) | – | – | **done** 2026-10-30 |
| 2 | W1 `C_Info`, W2 `C_CRC32`, W9 `C_Bit_Timing` | P1 smoke test | 6 values | **passed** 2026-10-30 |
| 3 | helper `C_Check_Return`; W3–W7 (W8 optional) | P2 (reduced) | 2 rows per table; round trip 0 mismatches | **passed** 2026-11-02 (W3–W7 front-panel rows + round trip, reported by Marcus; W8 skipped) |
| 4 | W10 `C_Encode_CAN_To_WS`, W11 `C_Decode_WS_To_CAN` | P3 helper round trip | 500 frames, 0 mismatches, ping + close | **passed** 2026-11-03 (500 / 0, ping `AB CD`, close; reported by Marcus) |
| 5 | real-time layer: W12 `C_RX_Buffers_Init`, W13 `C_Decode_RT`, W14 `C_Echo_RT`, W15 `C_Encode_RT` | P3-RT | 500 frames, 0 mismatches, echoes and encodes equal to W10 | **passed** 2026-11-04 (reported by Marcus; Show Buffer Allocations: no copy on `buffers` in W13) |
| 6 | CIF wrappers (library 1.2.0, `WIZARD.md` section 6 first): W16 `C_To_CIF_RT`, W17 `C_CIF_Encode_RT` | P6-CIF | 500 frames through CIF and back, equal to W10; NI's `Parse Next Socket CAN.vi` reads the packets | to do (for the CIF adapter) |

P4 (C vs native G) is deferred with the native client. P5 (timing) is optional (section 9).

## 2. Design: two layers of wrappers

Who allocates the buffers decides where a wrapper may be used.

| Layer | VIs | Buffers | Use |
|---|---|---|---|
| **Test / setup layer** | W1–W11 | each wrapper makes its own buffers with **Initialize Array, on every call**; W4, W5, W8, W11 also build arrays of clusters | tests, setup before a loop, rare events (a pong). **Never inside the real-time loop.** Simple connectors, same data terminals as the native G VIs, so the reference tables apply |
| **Real-time layer** | W12–W15 | **W12 allocates every buffer once**, before the loop. The caller keeps them in shift registers and passes them in; the C code writes **in place**. No allocation, no copy, no cluster arrays | the receive / echo / write loop, and the plugin API's Read and Write |

So: an "initialization level" exists, but only for the real-time layer (W12). The test layer keeps
its own buffers on purpose: it is easier to call and to compare with the reference tables, and it
never runs in the loop. Giving W3–W11 caller-owned buffers would add wiring to every test without
any run-time gain.

**W15 `C_Encode_RT` is new (2026-10-30):** the plugin API's `Write` encodes frames in the loop,
so it needs an encoder with a caller-owned `tx` buffer (W10 allocates on every call). W14 does the
same for the echo, from the receive arrays.

The only allocations left in the loop are inside LabVIEW's TCP Read (new string per read) and the
string for TCP Write (< 174 B per frame). Both are bounded and unavoidable with LabVIEW TCP.

## 3. Typedefs (all in `CANsub_C.lvlib`, folder `labview\c_client\`; done)

| Typedef | Fields, as built | Used by |
|---|---|---|
| `C_State.ctl` | U8 array (an opaque C state) | W4, W8, W11 |
| `C_Info.ctl` | `version` (U32), `hdlc state size` (I32), `ws state size` (I32), `max frames (4096)` (I32) | W1 |
| `C_Decode_Result.ctl` | `n frames` (I32), `peer close` (Bool), `ping len` (I32; ≥ 0 = a ping arrived, −1 = none), `crc errors` (U32), `decode errors` (I32) | W11, W13 |
| `C_RX_Buffers.ctl` | `t_dev` (DBL[]), `ws` (U8[]), `id` (U32[]), `flags` (U16[]), `hdlc` (U8[]), `dlc` (U8[]), `data len` (U8[]), `data` (U8[]), `ping` (U8[]) | W12, W13, caller |

Shared typedefs from `CANsub_Common.lvlib`: `Payload`, `CAN_Frame`, `WS_Frame`, `Bit_Timing`.

**Rule:** a wrapper with **3 or more scalar results** returns them in one typedef cluster.
Always use **Bundle / Unbundle By Name** with a typedef constant (never by position): a new field
then does not break the callers.

## 4. Rules for every wrapper

- **Location:** `labview\c_client\wrappers\`, in `CANsub_C.lvlib`, under the PXIe target.
- **No CLFN in a wrapper.** Call the generated VI `csb <name>.vi`. Its library path, types,
  `Constant` flags and thread setting are already set; you set none of them again. Its terminals
  have the C parameter names; array outputs are `<name> out`.
- **Terminals:** data terminals as below; `error in` / `error out` on the bottom corners of every
  wrapper.
- **Errors:** `error in` → the generated VI's `error in`. Its `function return` and `error out`
  → **`C_Check_Return.vi`** (section 6) → the wrapper's `error out`. Exception: W9 (its own codes
  5001/5002).
- **Sizes:** every `*_cap`, `*_size`, `len`, `max_frames` comes from **Array Size** of the same
  array. **Never type a capacity constant**: a capacity larger than the array lets the C code
  write past LabVIEW's memory and crash `lvrt`.
- **Output length:** cut each output array to its real length with **Array Subset** (`… out`,
  `0`, length).
- **States pass through on an error:** a wrapper with `state in` / `state out` (W4b, W8, W11)
  wires `state in` → `state out` in its "Error" case. An empty state would break every later call
  of the caller's loop (error 5102).
- **For Loops over C results:** wire the count output (`n_payloads`, `n_frames`) to the loop's
  **N**. An indexing tunnel alone iterates over the whole buffer and adds empty elements.
- **Coercion dots are expected** where a typedef (`C_State.ctl`, `Bytes.ctl`) meets a plain U8
  array terminal of a generated VI. They are cosmetic: **Show Buffer Allocations** showed no copy
  (measured 2026-11-02 on `C_HDLC_Decode.vi`). Keep the generated VIs on plain C types.
- **Settings (VI Properties → Execution):** preallocated clone, priority Normal, same as caller,
  **Inline on**, **Allow debugging off**, automatic error handling off.
- **VI description:** paste the text given under each wrapper into **VI Properties →
  Documentation → VI description**.
- **Tests:** in `labview\c_client\tests\`, not in a library, under the PXIe target, run on the
  target. Settings: non-reentrant, Normal, Standard, automatic error handling off.

## 5. Rows 1–2 (done): W1, W2, W9 and the smoke test P1

**W1 `C_Info.vi`**: `error in` → `info` (`C_Info.ctl`), `error out`. Error wire in series through
`csb version.vi`, `csb hdlc state size.vi`, `csb ws state size.vi`, `csb max frames.vi` (`in_len`
= `4096`); the four `function return` values → Bundle By Name → `info`.

> *Returns the libcansub_codec version and its buffer sizes. A wrong version means the target has
> another library build.*

**W2 `C_CRC32.vi`**: `data` (U8[]), `error in` → `crc32` (U32), `error out`. `csb crc32.vi`
(`len` = Array Size (`data`)); `function return` → `crc32`.

> *IEEE 802.3 / zlib CRC32 of a byte array (the CRC of the CANsub HDLC frames).*

**W9 `C_Bit_Timing.vi`**: `nominal bit rate` (U32, 500000), `nominal sample point` (DBL, 80),
`data bit rate` (U32, 2000000), `data sample point` (DBL, 80), `error in` → `bit timing`
(`Bit_Timing.ctl`), `nominal sample point out`, `data sample point out`, `error out`.
1. Initialize Array U16 × **8** → `timing` (the C function has no size parameter; it writes 8).
2. `csb bit timing.vi`; `timing out` → Index Array (0–7) → Bundle By Name (`Bit_Timing.ctl`).
3. Case Structure on `function return`: `0` pass; `-2` → error **5001** "invalid bit rate or
   sample point input"; `-3` → **5002** "no bit timing with one prescaler"; `Default` → 5100 −
   value, "csb_bit_timing returned %d". **Merge Errors** (generated VI's `error out` first).

> *CAN controller timing for the CANsub (80 MHz clock, one prescaler for both phases) from the bit
> rates and sample points; same result as python-can BitTimingFd.from_sample_point. Error 5001:
> invalid input; 5002: no solution with one prescaler.*

**P1 `Test_C_Smoke.vi`** (passed 2026-10-30): W1; W2 with `31 32 … 39` ("123456789"); W9 with
the defaults. Expected: version **65792** with library 1.1.0, **66048** after the 1.2.0 update, sizes **4112** / **16432**, max frames **1171**, CRC
**CBF43926**, timing **2, 63, 16, 16, 2, 15, 4, 4**, sample points 80 / 80, no error. If it does
not run: error 7 or 13 = the library did not load (check `/usr/local/lib/libcansub_codec.so` and
the Linux frame of `CANsub_Codec_LibPath.vi`).

## 6. Row 3: helper, codec wrappers W3–W8, test P2

### Helper `C_Check_Return.vi` (build first)

`function return` (I32), `function name` (string), `error in` → `ok` (Bool), `length` (I32),
`error out`.
1. `ok` = `function return` ≥ `0`.
2. `length` = **Max & Min** (`function return`, `0`) → max: the result size, or 0 on an error.
   Wire it to Array Subset in the callers (an error then gives an empty output).
3. Case Structure on `ok`. True: `error in` → `error out`. False: **Format Into String** `%s
   returned %d` (`function name`, `function return`) → message; code = `5100` − `function
   return`; **Error Cluster From Error Code.vi** (code, message) → **Merge Errors** (`error in`
   first) → `error out`.

> *Checks the return value of a libcansub_codec function. ≥ 0: success, length = the value (a
> byte count or result size). < 0: LabVIEW error with code 5100 − value (5101 buffer too small,
> 5102 invalid input, 5103 no solution, 5104 state not initialised), and length = 0. An incoming
> error is kept first.*

### Pattern for W3–W8

1. Make the buffers with **Initialize Array** (sizes below, from Array Size of the inputs).
2. Call the generated VI; `error in` → its `error in`.
3. Error check with **`C_Check_Return.vi`**. Wire:
   - the generated VI's `function return` → `function return`;
   - a **string constant** with the C function name (for example `csb_hdlc_encode`) →
     `function name`. It only appears in the error message;
   - the generated VI's `error out` → `error in`;
   - `C_Check_Return.vi`'s `error out` → the wrapper's `error out`.

   In the steps below this is written as "`C_Check_Return.vi` (`function name` = "…")".
4. Cut the outputs with **Array Subset**, length = `C_Check_Return.vi`'s **`length`** (written
   below as "helper `length`"). The generated VI's `function return` then has one wire only.

### W3 `C_HDLC_Encode.vi`

`payload` (U8[]), `error in` → `frame` (U8[]), `error out`.
1. `n` = Array Size (`payload`); buffer U8 × (2 × (`n` + 4) + 2) (worst case: every byte escaped).
2. `csb hdlc encode.vi`: `payload`; `len` = `n`; `frame` = buffer; `frame_cap` = Array Size (buffer).
3. `C_Check_Return.vi` (`function name` = "csb_hdlc_encode"). `frame` = Array Subset (`frame out`, 0, helper `length`).

> *HDLC-encodes one payload for the CANsub: appends the CRC32 (big-endian), escapes 7E and 7D,
> and adds the 7E flags at both ends. Allocates on every call: tests and setup only.*

### W4 `C_HDLC_State_Init.vi` and `C_HDLC_Decode.vi`

**State init**: `error in` → `state` (`C_State.ctl`), `error out`. Initialize Array U8 × **4112**
→ `csb hdlc state init.vi` (`state_size` = Array Size) → `state out` → `state`; helper
(`csb_hdlc_state_init`).

> *Creates a new HDLC decoder state (4112 bytes) for C_HDLC_Decode. Use one state per byte stream
> and keep it in a shift register.*

**Decode**: `bytes in` (U8[]), `state in` (`C_State.ctl`), `error in` → `payloads` (array of
`Payload.ctl`), `state out`, `crc errors` (U32), `error out`.
1. `n` = Array Size (`bytes in`). Buffers: `out` U8 × (`n` + 4096); `lens` **I32** × (`n` / 6 + 1).
2. `csb hdlc decode.vi`:

   | Terminal | Wire |
   |---|---|
   | `state`, `state_size` | `state in`, Array Size (`state in`); `state out` → `state out` |
   | `in`, `in_len` | `bytes in`, `n` |
   | `out`, `out_cap` | buffer `out`, Array Size (`out`) |
   | `lens`, `lens_cap` | buffer `lens`, Array Size (`lens`) |
   | `n_payloads` | N of the For Loop |
   | `crc_errors` | → `crc errors` |
   | `function return` | → `C_Check_Return.vi` (`function name` = "csb_hdlc_decode") |

3. **For Loop**, N = `n_payloads`; `lens out` on an indexing tunnel; shift register `offset` (I32,
   0). Array Subset (`out out`, `offset`, `lens[i]`) → Bundle By Name (`Payload.ctl`) → indexing
   tunnel → `payloads`; `offset` + `lens[i]` → shift register.

> *Feeds received bytes into the HDLC decoder and returns every complete payload with a valid
> CRC. Bytes of an incomplete frame stay in the state for the next call. crc errors is cumulative.
> Allocates on every call: tests only.*

### W5 `C_CAN_Deserialize.vi`

`payload` (U8[]), `error in` → `frames` (array of `CAN_Frame.ctl`), `decode errors` (U32), `error
out`.
1. `m` = Array Size (`payload`) / 7 + 1. Buffers: `t_dev` DBL × `m`; `id` U32 × `m`; `flags` U16 ×
   `m`; `dlc` U8 × `m`; `data_len` U8 × `m`; `data` U8 × (`m` × 64).
2. `csb can deserialize.vi`: `len` = Array Size (`payload`); the six buffers; `max_frames` = Array
   Size (`id`). `C_Check_Return.vi` (`function name` = "csb_can_deserialize"). `decode_errors` → To U32 → `decode errors`.
3. **For Loop**, N = `n_frames`. Frame `i` →
   Bundle By Name (`CAN_Frame.ctl`):
   - `data` = Array Subset (`data out`, `i` × 64, `data_len[i]`);
   - the 7 Booleans: `flags[i]` → **Number To Boolean Array** (element k = bit k) → **one Index
     Array** with 7 outputs, first index `0` → `ide` (bit 0, mask 0001), `fdf` (1, 0002), `rtr`
     (2, 0004), `brs` (3, 0008), `esi` (4, 0010), `tx_ack` (5, 0020), `error` (6, 0040). Same
     result as And + Not Equal To 0? per mask, with 2 nodes instead of 21;
   - the five per-frame arrays (`t_dev`, `id`, `flags`, `dlc`, `data_len`) on **indexing**
     tunnels (element `i` per iteration); only `data out` on a non-indexing tunnel;
   - `err type` = Logical Shift (`flags[i]`, −8) **And** 001F → To U8.

> *Decodes all CAN frames of one HDLC payload from the CANsub into CAN_Frame clusters (device
> time, ID, flags, DLC, data). decode errors = 1 if the payload ended in a truncated frame.
> Allocates on every call: tests only.*

### W6 `C_CAN_Serialize.vi`

`id` (U32), `ide`, `fdf`, `brs` (Bool), `data` (U8[]), `error in` → `payload` (U8[]), `error out`.
1. Buffer U8 × **75** (11-byte header + 64 data bytes).
2. `csb can serialize.vi`: each Bool → **Boolean To (0,1)** → To U8; `len` = Array Size (`data`);
   `payload_cap` = Array Size (buffer). `C_Check_Return.vi` (`function name` = "csb_can_serialize").
3. `payload` = Array Subset (`payload out`, 0, helper `length`).

> *Serializes one CAN or CAN FD frame into the CANsub transmit payload (header + data). Classic:
> at most 8 data bytes. FD: at most 64, zero-padded to the next FD length. Standard IDs are masked
> to 11 bits.*

### W7 `C_WS_BuildFrame.vi`

`payload` (U8[]), `opcode` (U8, default `2`), `error in` → `frame` (U8[]), `error out`.
1. Buffer U8 × (Array Size (`payload`) + 14).
2. `csb ws build frame.vi`: `mask` = U32 constant `12345678` (hex display); `frame_cap` = Array
   Size (buffer). `C_Check_Return.vi` (`function name` = "csb_ws_build_frame").
3. `frame` = Array Subset (`frame out`, 0, helper `length`).

Used for the pong (opcode 10, rare: allocation acceptable) and in tests.

> *Builds one masked WebSocket client frame (FIN set) around a payload, ready for TCP Write.
> opcode: 2 binary, 8 close, 9 ping, 10 pong. Fixed mask 12 34 56 78 (the device does not check
> it).*

### W8 `C_WS_State_Init.vi` and `C_WS_ParseFrames.vi` (optional)

Nothing else uses W8 (the receive path uses W11 / W13). Build it only for a LabVIEW test of the
WebSocket parser.

**State init**: as W4, with **16432** bytes and `csb ws state init.vi`.

> *Creates a new WebSocket parser state (16432 bytes) for C_WS_ParseFrames. Do not use it with
> C_Decode_WS_To_CAN: the library refuses a state of the other API (error 5104).*

**Parse**: `bytes in`, `state in`, `error in` → `frames` (array of `WS_Frame.ctl`), `state out`,
`error out`. `k` = Array Size (`bytes in`) + 16384; buffers `out` U8 × `k`, `opcodes` U8 × (`k` / 2
+ 1), `lens` **I32** × (`k` / 2 + 1); `max_frames` = Array Size (`lens`). For Loop as in W4,
building `WS_Frame.ctl` (`opcode` = `opcodes[i]`, `payload` = Array Subset).

> *Feeds received bytes into the WebSocket parser and returns every complete server frame
> (opcode + payload). Bytes of an incomplete frame stay in the state. Allocates on every call:
> tests only.*

### Test P2 (reduced, sprint decision 2026-10-30)

The 927 C checks already test the codec logic. P2 checks the wrapper wiring.

1. **Front-panel rows**: run the wrapper on its front panel with the inputs and expected values of
   the native build doc. Two rows per table: the first row and one edge or error row.

   | Build doc table | Wrapper |
   |---|---|
   | 1.1 | W2 `C_CRC32` |
   | 1.2 | W3 `C_HDLC_Encode` |
   | 1.5 | W6 `C_CAN_Serialize` |
   | 1.6 | W7 `C_WS_BuildFrame` |
   | 2.0 | W9 `C_Bit_Timing` (errors 5001 / 5002) |

2. **`Test_C_Codec_RoundTrip.vi`**: the structure of build doc 1.5, "Round trip", with W6 → W3 →
   W4 → W5. Make the HDLC state with `C_HDLC_State_Init` **inside** the loop (a new state per
   case). Expected: `mismatches` = 0.

Optional (not in the sprint): `Test_C_HDLC_Decode.vi`, `Test_C_CAN_Deserialize.vi`,
`Test_C_WS_ParseFrames.vi` with the build doc 1.3, 1.4 and 1.7 tables (all columns except the
native state columns).

## 7. Row 4: helper wrappers W10, W11, test P3

### W10 `C_Encode_CAN_To_WS.vi`

`id` (U32), `ide`, `fdf`, `brs` (Bool), `data` (U8[]), `error in` → `ws frame` (U8[]), `error out`.
1. Buffer `out` U8 × **174**.
2. `csb encode can to ws.vi`: Bools → Boolean To (0,1) → To U8; `len` = Array Size (`data`);
   `mask` = `12345678` hex; `out_cap` = Array Size (buffer). `C_Check_Return.vi` (`function name` = "csb_encode_can_to_ws").
3. `ws frame` = Array Subset (`out out`, 0, helper `length`).

> *Encodes one CAN or CAN FD frame into a complete WebSocket frame for the CANsub (serialize +
> HDLC + WebSocket), ready for TCP Write. Allocates on every call: tests and setup; in the loop use
> C_Encode_RT.*

### W11 `C_Decode_WS_To_CAN.vi`

`bytes in` (U8[]), `ws state in`, `hdlc state in` (`C_State.ctl`), `error in` → `frames` (array
of `CAN_Frame.ctl`), `ws state out`, `hdlc state out`, `result` (`C_Decode_Result.ctl`), `ping
payload` (U8[]), `error out`.
1. `m` = `csb max frames.vi` (`in_len` = Array Size (`bytes in`)). The six frame buffers as W5 with
   `m`; `ping` U8 × 125.
2. `csb decode ws to can.vi` (it computes its sizes inside):

   | Terminal | Wire |
   |---|---|
   | `ws_state`, `hdlc_state` | the state inputs; `… out` → the state outputs |
   | `in` | `bytes in` |
   | `t_dev`, `id`, `flags`, `dlc`, `data_len`, `data` | the buffers |
   | `ping_payload` | buffer `ping` |
   | `n_frames`, `ping_len`, `crc_errors`, `decode_errors` | → Bundle By Name `C_Decode_Result.ctl` |
   | `peer_close` | → Not Equal To 0? → `peer close` (same bundle) |
   | `function return` | → `C_Check_Return.vi` (`function name` = "csb_decode_ws_to_can") |

3. `ping payload` = Array Subset (`ping_payload out`, 0, `ping_len`) (empty when −1).
4. The For Loop of W5 builds the `CAN_Frame.ctl` array.

Use separate WS states for W11 and W8: the library refuses a state of the other API (5104).

> *Decodes received TCP bytes (WebSocket + HDLC + CAN) into CAN frames. Incomplete data stays in
> the two states. result: frame count, peer close, ping length (−1 = none), CRC and decode errors.
> Allocates on every call: tests and setup; in the loop use C_Decode_RT.*

### Test P3 `Test_C_Helpers.vi` (no device)

1. **Frames**: For Loop N = 500, random `fdf`, `brs`, `ide`, `id`, `n` (0–8 or 0–64) and `data`
   (as in build doc 1.5, round trip). Keep the inputs (indexing tunnels). Each → W10 →
   concatenating tunnel → `stream`.
2. **Control frames**: Build Array (Concatenate) `stream` + W7 (`AB CD`, opcode 9, a ping) + W7
   (`03 E8`, opcode 8, a close). W11 accepts masked frames, so no server frames are needed.
3. **Decode in random pieces**: states: `ws` = Initialize Array U8 × 16432 → `csb ws state
   init.vi` (or `C_WS_State_Init` if you built W8; a new state, not one used by W8); `hdlc` =
   `C_HDLC_State_Init`. While Loop
   with shift registers `pos`, both states, `frames`. Per iteration: `chunk` = Random × 700 + 1,
   at most the rest; Array Subset (`stream`, `pos`, `chunk`) → W11; append `frames`; from
   `result`: Or `peer close`, Or (`ping len` ≥ 0); keep the last non-empty `ping payload`. Stop
   when `pos` ≥ Array Size (`stream`).
4. **Compare** the 500 inputs: `id`, `ide`, `fdf`, `brs` equal; Array Subset (`data`, 0, `n`)
   equal (Compare Aggregates); bytes after `n` all 0 (FD padding). Count mismatches.

| Value | Expected |
|---|---|
| frames received / mismatches | 500 / 0 |
| peer close | True |
| ping received / ping payload | True / `AB CD` |
| crc errors / decode errors | 0 / 0 |

## 8. Row 5: real-time layer W12–W15, test P3-RT

**Size rule:** the loop decodes at most **8192 bytes per call** (TCP Read of `4096` plus the
handshake `leftover` < 1024 B on the first iteration). So the frame arrays hold
`csb_max_frames(8192)` = **1756** frames. The TCP Read size and this number belong together.

### W12 `C_RX_Buffers_Init.vi` (once, before the loop)

`error in` → `buffers` (`C_RX_Buffers.ctl`), `tx` (U8[]), `error out`.
1. `m` = `csb max frames.vi` (`in_len` = 8192) → 1756.
2. Initialize Array (0): `ws` U8 × 16432; `hdlc` U8 × 4112; `t_dev` DBL × `m`; `id` U32 × `m`;
   `flags` U16 × `m`; `dlc` U8 × `m`; `data len` U8 × `m`; `data` U8 × (`m` × 64); `ping` U8 × 125;
   `tx` U8 × 174 (separate output: W14 / W15 write it while the RX arrays are read).
3. `csb warmup.vi` (`ws_state`, `hdlc_state`, sizes = Array Size). `function return` →
   `C_Check_Return.vi` (`function name` = "csb_warmup"); then helper `length` **Not Equal?** 2 →
   if True (and no error yet), error **5110**
   "warmup decoded %d frames, expected 2". Take `ws` and `hdlc` from `ws_state out` /
   `hdlc_state out` (initialised and touched).
4. Bundle By Name (`C_RX_Buffers.ctl` constant) → `buffers`.

> *Allocates every receive buffer once (frame arrays for 1756 frames, both decoder states, ping,
> and the 174-byte tx buffer) and runs the library warm-up, so the real-time loop has no
> allocation and no page faults. Call once before the loop; keep buffers and tx in shift
> registers.*

### W13 `C_Decode_RT.vi` (once per TCP Read)

`bytes in` (U8[]), `buffers in` (`C_RX_Buffers.ctl`), `error in` → `buffers out`, `result`
(`C_Decode_Result.ctl`), `error out`.
1. **In Place Element Structure** on `buffers in`, one **Unbundle / Bundle Elements** border node
   for all 9 elements (the arrays are the cluster's own memory: no copy).
2. Inside: `csb decode ws to can.vi`: `in` = `bytes in`; each element into the terminal of the
   same name and its `… out` back to the same element (`ws` → `ws_state`, `hdlc` → `hdlc_state`,
   `ping` → `ping_payload`, `data len` → `data_len`). No sizes.
3. `n_frames`, `ping_len`, `crc_errors`, `decode_errors`, `peer_close` (→ Not Equal To 0?) →
   Bundle By Name `C_Decode_Result.ctl` → `result`. `C_Check_Return.vi` (`function name` = "csb_decode_ws_to_can").
4. **Do not** build `CAN_Frame` clusters. The caller reads `id[i]`, `flags[i]`, `t_dev[i]`,
   `data[i·64 + k]` with Index Array.

> *Real-time decode: feeds received TCP bytes into the decoder in place, in the buffers from
> C_RX_Buffers_Init. No allocation, no copy. Frames are in the buffer arrays (index 0 … n frames −
> 1).*

### W14 `C_Echo_RT.vi` (once per echoed frame)

`new id` (U32), `index` (I32), `n frames` (I32), `flags` (U16[]), `data len` (U8[]), `data` (U8[]),
`tx in` (U8[]), `error in` → `tx out`, `length` (I32), `error out`.

`csb encode rx frame to ws.vi`: `new_id`, `index`, `n_frames`; `flags`, `data_len`, `data` = the RX
arrays; `mask` = `12345678` hex; `out` = `tx in`, `out out` → `tx out`; `out_cap` = Array Size
(`tx in`). `function return` → `C_Check_Return.vi` (`function name` = "csb_encode_rx_frame_to_ws"); helper `length` → `length`.

> *Real-time echo: encodes received frame number index with a new ID (IDE, FDF, BRS and data
> kept) into the caller's tx buffer. No allocation. Send tx bytes 0 … length − 1.*

### W15 `C_Encode_RT.vi` (once per transmitted frame; new)

`id` (U32), `ide`, `fdf`, `brs` (Bool), `data` (U8[]), `tx in` (U8[]), `error in` → `tx out`,
`length` (I32), `error out`.

`csb encode can to ws.vi`: Bools → Boolean To (0,1) → To U8; `data`; `len` = Array Size (`data`);
`mask` = `12345678` hex; `out` = `tx in`, `out out` → `tx out`; `out_cap` = Array Size (`tx in`).
`function return` → `C_Check_Return.vi` (`function name` = "csb_encode_can_to_ws"); helper `length` → `length`.

> *Real-time encode: one CAN or CAN FD frame into a WebSocket frame, written into the caller's tx
> buffer (174 bytes, from C_RX_Buffers_Init). No allocation. Send tx bytes 0 … length − 1.*

**Sending from W14 / W15:** Array Subset (`tx out`, 0, `length`) → Byte Array To String → TCP
Write (a small string per frame; LabVIEW TCP Write needs a string). Keep `tx` in its own shift
register, never branched to an indicator or probe (a branch makes LabVIEW copy it every time).

### Test P3-RT `Test_C_RT_Path.vi` (no device)

Same input stream as P3 (500 random frames from W10, then the ping and the close from W7).
1. Before the loop: W12 → `buffers`, `tx` (shift registers).
2. While Loop over random pieces (1–700 bytes): W13 → Unbundle By Name (`result`) → `n frames`.
   Unbundle the arrays once; For Loop N = `n frames`:
   - compare frame `i` with input number `count` (as P3);
   - W14 (`new id` = `id[i]`, `index` = `i`) → compare its `length` bytes with W10 of the same
     frame;
   - W15 with the **input** frame number `count` → compare with W10 of the same frame;
   - `count` + 1.

| Value | Expected |
|---|---|
| frames / frame mismatches | 500 / 0 |
| echo mismatches (W14 vs W10) | 0 |
| encode mismatches (W15 vs W10) | 0 |
| peer close / ping len | True / 2 |
| crc errors / decode errors | 0 / 0 |

## 9. Row 6: CIF wrappers W16, W17, test P6-CIF (for the CIF adapter)

Needs library 1.2.0 and its 3 generated VIs (`WIZARD.md` section 6). The packet format is in
`README.md` (this folder), "pcap CAN packets": NI's CIF CAN packet (pcap header big-endian, TAI time,
SocketCAN header, no padding). Both wrappers are real-time layer: caller-owned buffers, no
allocation.

**W12 gets one more output:** `cif` = U8 × `csb pcap max bytes.vi` (1756) = 154 528 bytes, for
W16. (`tx` stays 174 bytes; W17 needs its own output buffer, sized by the caller for the largest
write batch: 174 × frames per write.)

### W16 `C_To_CIF_RT.vi` (once per TCP Read, after W13)

`buffers` (`C_RX_Buffers.ctl`), `n frames` (I32), `options` (I32, default 0), `cif in` (U8[]),
`error in` → `cif out`, `length` (I32), `n packets` (I32), `error out`.
1. In Place Element Structure on `buffers` (read only: Unbundle Elements, no Bundle needed), or
   Unbundle By Name on a non-branched wire.
2. `csb frames to pcap.vi`: `n_frames`; `t_dev`, `id`, `flags`, `data_len` (`data len`), `data`;
   `tai_minus_utc_s` = constant **37**; `options`; `out` = `cif in`, `out out` → `cif out`;
   `out_cap` = Array Size (`cif in`). `function return` → `C_Check_Return.vi` (`function name` =
   "csb_frames_to_pcap"); helper `length` → `length`; `n_packets` → `n packets`.

The CIF stream is `cif out` bytes 0 … `length` − 1 (Array Subset, when handing it to CIF).

> *Converts the frames of the last decode (C_Decode_RT) into NI CIF CAN packets (pcap header
> big-endian with TAI time, SocketCAN header, payload) in the caller's cif buffer. TX acks and
> error frames are skipped unless options says otherwise. No allocation.*

### W17 `C_CIF_Encode_RT.vi` (once per write from CIF)

`cif` (U8[], packets from CIF), `tx in` (U8[]), `error in` → `tx out`, `length` (I32), `n
packets` (I32), `n rejected` (I32), `consumed` (I32), `error out`.

`csb encode pcap to ws.vi`: `in` = `cif`; `in_len` = Array Size (`cif`); `options` = 0; `mask` =
`12345678` hex; `out` = `tx in`, `out out` → `tx out`; `out_cap` = Array Size (`tx in`).
`function return` → `C_Check_Return.vi` (`function name` = "csb_encode_pcap_to_ws"); helper
`length` → `length`; the three counts out.

Send `tx` bytes 0 … `length` − 1 with **one** TCP Write. If `consumed` < Array Size (`cif`), keep
the rest (an incomplete packet, or more packets than `tx` holds) for the next call.

> *Encodes NI CIF CAN packets into WebSocket frames for one TCP Write, in the caller's tx buffer.
> Remote and error frames (the CANsub cannot send them) are skipped and counted in n rejected.
> No allocation.*

### Test P6-CIF `Test_C_CIF.vi` (no device)

1. 500 random frames as in P3 (W10 → stream) → W13 in random pieces → after each piece W16.
   Concatenate the `cif out` pieces (test only) → `cif stream`.
2. Read `cif stream` with NI's **`Parse Next Socket CAN.vi`** in a While Loop (`CAN pcap out` →
   next call) until empty: count the packets, compare `identifier`, `extended?`, `Interface Mode`
   and `Payload` with the 500 inputs.
3. `cif stream` → W17 (in pieces of random size, keep the rest by `consumed`) → compare the
   concatenated `tx` bytes with the W10 frames of the same inputs.

| Value | Expected |
|---|---|
| packets read by NI's parser | 500 |
| field mismatches (NI parser vs inputs) | 0 |
| W17 output vs W10 | identical |
| `n rejected` | 0 |

## 10. Deferred and optional tests

- **P4 `Test_C_vs_G.vi`** (deferred while the native client is parked): random inputs through the
  native G VIs and the C wrappers (CRC, HDLC encode/decode, CAN serialize/deserialize, WS build);
  0 mismatches expected.
- **P5 `Test_C_Timing.vi`** (optional): one realistic message (70 classic frames: 70 × W6 → W3 →
  W7, opcode 2); 10 000 calls of W13 (decode) and of W15 (encode, one 8-byte frame), each timed
  with High Resolution Relative Seconds into a pre-allocated array; report median and max µs.
  Debugging off on all VIs involved. The harness number `echo_node_host` remains the number for
  the comparison.

## 11. Checkpoints to report back

| After | Send me |
|---|---|
| P1 | done |
| P2 | the rows that failed, if any; round-trip `mismatches` |
| P3 | the P3 table values |
| P3-RT | the P3-RT table values |
| P6-CIF | the P6-CIF table values |
| P5 (optional) | median / max µs, decode and encode |
