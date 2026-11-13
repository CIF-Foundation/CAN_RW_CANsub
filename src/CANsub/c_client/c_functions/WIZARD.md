# S2a: generate the codec VIs with the Import Shared Library wizard

> **STATUS: DONE (2026-10-29), reviewed with lvkit.** Do not repeat this step. Use this file only
> to **look up** the generated VIs (section 3: parameters by function; section 5: the library
> path) or when `cansub_codec.h` changes ("Keep in step" at the end). The next step is S2b:
> `LABVIEW_C_TEST.md`.
>
> **Result:** `codec_vis\CANsub_Codec.lvlib` (under the **PXIe target**), with 18 VIs
> `codec_vis\VIs\csb <name>.vi` + helper `CANsub_Codec_LibPath.vi` + `libcansub_codec.dll`
> (Windows) in `codec_vis\VIs\`. Every CLFN: **Specify path on diagram** (from the helper), C,
> Run in any thread; all VIs preallocated clone, inline, debugging off.

The wizard makes one VI for each of the **18** functions of 1.1.0 in `cansub_codec.h` (1.2.0 adds 3: section 6). These VIs contain
only the Call Library Function Node (CLFN). The sizing and real-time logic of W1–W14
(`LABVIEW_C_TEST.md`) stays in the W wrappers, which then call these VIs.

## 1. Files

`make wizard` (Linux, needs `pip install ziglang==0.13.0`) writes `c_functions\wizard\`
(git-ignored):

| File | Use |
|---|---|
| `libcansub_codec.dll` | Windows x64 build of the same source. **Only** for the wizard and Windows desktop tests. |
| `libcansub_codec.so` | The NI Linux RT build (the same file as `dist\`). |
| `cansub_codec_lv.h` | Header for the wizard: the prototypes of `cansub_codec.h`, without `#include <stdint.h>`. The wizard on Windows cannot find `stdint.h`, so this file defines `uint8_t`, `uint16_t`, `int32_t`, `uint32_t` itself. Generated, do not edit. |
| `test_cansub_codec_win.exe` | The 927 C checks, linked against the DLL. |
| `SHA256SUMS` | Checksums. |

## 2. Check the DLL on Windows (once)

```powershell
cd C:\Dev\CANSub\labview\c_client\c_functions\wizard
.\test_cansub_codec_win.exe
```

Expect `ALL PASS` (927 checks). If it fails, stop and send me the output. The wizard would then
wrap a DLL that behaves differently from the .so.

## 3. Run the wizard

LabVIEW: **Tools » Import » Shared Library (.dll)…** → **Create VIs for a shared library**.

| Page | Setting |
|---|---|
| Shared library | `C:\Dev\CANSub\labview\c_client\c_functions\wizard\libcansub_codec.dll` |
| Header file | `…\wizard\cansub_codec_lv.h` |
| Include paths, preprocessor definitions | empty |
| Functions | all 18. If the wizard marks one as not parsed, stop and send me its message. |
| Project library | name `CANsub_Codec.lvlib`, folder `C:\Dev\CANSub\labview\c_client\codec_vis\` |
| Error handling | **Simple Error Handling** (error in / error out; the call is skipped on error in) **[verify]** the exact label in LabVIEW 2025 |
| Configure VIs and Controls | the parameter rules below, for each function. Tick **Reentrant** (this sets **Run in any thread**). |

### Parameter rules

Every pointer parameter needs a choice. The wizard does not know if a pointer is an array or
one value. Use these three rules:

The three lists below are generated from `cansub_codec.h` and cover every pointer parameter
(12 + 12 + 32 = 56). Element types: `uint8_t` = U8, `uint16_t` = U16, `uint32_t` = U32,
`int32_t` = I32, `double` = DBL. Functions with no pointer parameter (`csb_version`,
`csb_hdlc_state_size`, `csb_ws_state_size`, `csb_max_frames`) need nothing.

1. **`const` pointer** (for example `const uint8_t *data`) → **Array**, **Input**, Array Data
   Pointer. After the wizard, tick **Constant** on each. These 12:

   | Function | Parameters (element type) |
   |---|---|
   | `csb_crc32` | `data` (U8) |
   | `csb_hdlc_encode` | `payload` (U8) |
   | `csb_hdlc_decode` | `in` (U8) |
   | `csb_can_deserialize` | `payload` (U8) |
   | `csb_can_serialize` | `data` (U8) |
   | `csb_ws_build_frame` | `payload` (U8) |
   | `csb_ws_parse_frames` | `in` (U8) |
   | `csb_encode_can_to_ws` | `data` (U8) |
   | `csb_decode_ws_to_can` | `in` (U8) |
   | `csb_encode_rx_frame_to_ws` | `flags` (U16), `data_len` (U8), `data` (U8) |
2. **Pointer to one value** (the function writes one number) → **Numeric**, **Output**, **Pointer
   to Value**. These are the only 12 such parameters:

   | Function | Parameters (numeric type) |
   |---|---|
   | `csb_hdlc_decode` | `n_payloads` (I32), `crc_errors` (U32) |
   | `csb_can_deserialize` | `n_frames` (I32), `decode_errors` (I32) |
   | `csb_ws_parse_frames` | `n_frames` (I32) |
   | `csb_bit_timing` | `nominal_sp_out` (DBL), `data_sp_out` (DBL) |
   | `csb_decode_ws_to_can` | `n_frames` (I32), `peer_close` (I32), `ping_len` (I32), `crc_errors` (U32), `decode_errors` (I32) |

   Output is enough: LabVIEW creates the scalar for an unwired Pointer to Value, and the C code
   only writes these (it never reads the old value).
3. **Every other pointer** (state buffers, `out`, `frame`, `payload`, `t_dev`, `id`, `flags`,
   `dlc`, `data_len`, `data`, `lens`, `opcodes`, `ping_payload`, `timing`) → **Array**,
   **Input/Output**, Array Data Pointer. **Do not** let the wizard create the array inside the
   VI. The caller makes the array once (Initialize Array) and passes it in. It comes back out
   filled. This keeps allocation out of the real-time loop. An unwired array is empty, so
   **Output** is wrong for arrays. These 32:

   | Function | Parameters (element type) |
   |---|---|
   | `csb_hdlc_encode` | `frame` (U8) |
   | `csb_hdlc_state_init` | `state` (U8) |
   | `csb_hdlc_decode` | `state` (U8), `out` (U8), `lens` (I32) |
   | `csb_can_deserialize` | `t_dev` (DBL), `id` (U32), `flags` (U16), `dlc` (U8), `data_len` (U8), `data` (U8) |
   | `csb_can_serialize` | `payload` (U8) |
   | `csb_ws_build_frame` | `frame` (U8) |
   | `csb_ws_state_init` | `state` (U8) |
   | `csb_ws_parse_frames` | `state` (U8), `opcodes` (U8), `lens` (I32), `out` (U8) |
   | `csb_bit_timing` | `timing` (U16, **array size 8**) |
   | `csb_encode_can_to_ws` | `out` (U8) |
   | `csb_decode_ws_to_can` | `ws_state` (U8), `hdlc_state` (U8), `t_dev` (DBL), `id` (U32), `flags` (U16), `dlc` (U8), `data_len` (U8), `data` (U8), `ping_payload` (U8) |
   | `csb_encode_rx_frame_to_ws` | `out` (U8) |
   | `csb_warmup` | `ws_state` (U8), `hdlc_state` (U8) |

**Array size** (the wizard's field, default `4097`; it probably becomes the CLFN parameter's
**Minimum size**, **[verify]**): LabVIEW makes a shorter array longer before every call, which is
an allocation.

| Array | Array size |
|---|---|
| `csb_bit_timing` → `timing` (U16) | **8**. This function has no size parameter: C always writes 8 values. Never Pointer to Value. |
| every other array | **none** (empty field; `0` if the wizard needs a number). Never `4097`. |

Value parameters (`int32_t len`, `uint32_t id`, `double nominal_sp`, …) → Input, by value. The
wizard sets them correctly.

**Call node settings (all 18):** calling convention **C**, thread **Run in any thread** (the
library has no writable global data, no malloc, no locks: checked with `nm`; the only rule is
one state buffer per loop), error checking **Default**.

## 4. After the wizard

1. **Library path (decided by section 5: B).** In each of the 18 generated VIs: open the CLFN →
   Function tab → tick **Specify path on diagram** → OK. Place `CANsub_Codec_LibPath.vi`
   (inlined) in the VI and wire `lib path` → `path in`. Leave `path out` unwired. Thread stays
   **Run in any thread**. Copy the DLL next to the VIs for Windows and commit it:
   `Copy-Item …\c_functions\wizard\libcansub_codec.dll …\codec_vis\VIs\`.
2. **CLFN `Constant` flag.** The wizard does not set it. For each `const` array (rule 1), open
   the CLFN, select the parameter, tick **Constant**. Without it, LabVIEW can copy the array on
   every call.
3. **VI settings** for all 18 VIs (VI Properties → Execution): **Preallocated clone**;
   **Inline subVI into callers** on if LabVIEW allows it; **Allow debugging** off; **automatic
   error handling** off; priority Normal; execution system Same as caller. If LabVIEW refuses
   Inline, note the reason it gives.
4. **Check two VIs** (`csb_bit_timing.vi`, `csb_encode_can_to_ws.vi`): CLFN → Parameters →
   **Minimum size** = 8 for `timing`, **<None>** for the other arrays; no **Initialize Array**
   on the block diagram.
5. Tell me when it is saved. I run `index` (refresh), `unresolved` and `read_vi` on
   `codec_vis\` and check every CLFN against `cansub_codec.h`: type, pass mode, direction,
   `Constant`, thread, library name. This is also the first test of lvkit on a CLFN.

## 5. Library path: static name or path on the diagram (decide by measurement)

**Decision (Marcus, 2026-10-28):** on the target, the `.so` lives in **`/usr/local/lib`**,
installed once for all plugins (README, "Deploy"). On Windows (editing and desktop tests), the
DLL lives in `codec_vis\VIs\`, next to the generated VIs; commit it there (about 30 KB).

Two ways to tell a CLFN where the library is:

| | A: static name `libcansub_codec.*` | B: **Specify path on diagram** + helper VI (NI's advice) |
|---|---|---|
| errors | at edit time (broken arrow) | at run time, first call (error 7/13); Init's warmup makes that call |
| platform | `*` → `dll` / `so` | helper VI, Conditional Disable Structure |
| cost per call | none | LabVIEW compares the path on every call: **unknown, measure it** |

The echo call costs 0.07 µs on the target (bench 2026-10-28), so the path check can matter.

**Measured 2026-10-29:** A works **only in the RT context**. With `CANsub_Codec.lvlib` under the
**PXIe target** in the project, the CLFN accepts `libcansub_codec.*` and `csb_version.vi` returns
`65792` on the target. Under My Computer (or outside the project), LabVIEW refuses the name
(Windows cannot resolve it); there only B works. So: **keep `CANsub_Codec.lvlib` under the PXIe
target.** A has no Windows desktop use; B works in both contexts.

### 5.1 Helper `CANsub_Codec_LibPath.vi` (in `CANsub_Codec.lvlib`, folder `codec_vis\VIs\`)

Connector: → `lib path` (path). No error terminals.

**Conditional Disable Structure**, two frames:
- **`OS == "Linux"`** (the PXIe): path constant **`/usr/local/lib/libcansub_codec.so`** → `lib path`.
- **Default** (Windows): **Current VI's Path** → **Strip Path** → **Build Path** (name
  `libcansub_codec.dll`) → `lib path`.

**[verify]** the symbol and its value in LabVIEW 2025: right-click the structure → Edit
Condition; NI Linux RT targets should list `OS` = `Linux`. If not, use `TARGET_TYPE == "RT"`.

Settings: Preallocated clone, **Inline** on, debugging off, auto error handling off. Inline
matters: on the PXIe the VI then becomes a constant in its caller. When inlined, Current VI's
Path gives the **caller's** path; every caller is in `codec_vis\VIs\`, so the Windows result is the
same.

### 5.2 Test `Test_CLFN_Path_Cost.vi` (folder `c_client\tests\`, under the **PXIe target**)

Settings: group T (Non-reentrant, Standard), priority **high**. Run it with no other VI running.

1. Inputs to the call (once, before the loops): `flags` = U16 array `[0]`; `data_len` = U8 array
   `[8]`; `data` = U8 × 64 (Initialize Array, 0); `out` = U8 × 174 (Initialize Array, 0).
2. Two **Call Library Function Nodes**, both `csb_encode_rx_frame_to_ws`, configured as in rule
   1 and rule 3 (return I32; `new_id` U32 `0x123`; `index` I32 `0`; `n_frames` I32 `1`; `flags`,
   `data_len`, `data` Array ADP **Constant**; `mask` U32 `0x12345678`; `out` Array ADP; `out_cap`
   I32 = Array Size(`out`)); C, **Run in any thread**:
   - **A**: library name `libcansub_codec.*`;
   - **B**: **Specify path on diagram**, `path in` wired from `CANsub_Codec_LibPath.vi`.
3. **For Loop**, 20 iterations (blocks). Inside, in a **Flat Sequence**:
   1. High Resolution Relative Seconds → `t0`;
   2. **For Loop** N = `100000`: CLFN **A** (`out` through a shift register);
   3. High Resolution Relative Seconds → `t1`;
   4. **For Loop** N = `100000`: CLFN **B** (`out` through a shift register);
   5. High Resolution Relative Seconds → `t2`.

   Per block: `a ns` = (`t1` − `t0`) / 100000 × 1e9, `b ns` = (`t2` − `t1`) / 100000 × 1e9.
   Auto-index both into DBL arrays.
4. Outputs: **median** and **min** of `a ns` and of `b ns` (Sort 1D Array → Index Array), and
   `b − a` of the medians. Also check: return value of the last A and B call = **29** (the WS
   frame length for these inputs; measured with the .so via ctypes); else the test is invalid.

Timing per block, not per call: High Resolution Relative Seconds itself costs more than the
call, so a per-call measurement would show the timer.

### 5.3 Decision

| `b − a` (median) | Do this on all 18 CLFNs |
|---|---|
| ≤ **50 ns** | **B**: Specify path on diagram, `path in` from `CANsub_Codec_LibPath.vi` inside each generated VI |
| > 50 ns | **A** on the hot path (`csb_decode_ws_to_can`, `csb_encode_rx_frame_to_ws`, `csb_encode_can_to_ws`), **B** on the other 15 |

Then: run `csb_version.vi` under **My Computer** and under the **PXIe target**; both return
**65792**.

**Result (PXIe, 2026-10-29, `Test_CLFN_Path_Cost.vi`, 20 blocks, return 29 for A and B):**

| ns per call | median | min |
|---|---|---|
| A static name | 85.92 | 84.55 |
| B path on diagram | 96.19 | 95.24 |
| **B − A** | **10.27** | |

Measured, inner N = `100000`, factor `10000`. A first run with N = `10000` and the same factor
showed 10× too small values; corrected × 10 it gave 85.6 / 96.3 / 10.7, which this run confirms.
Plausible: the C bench gives 70 ns per echo call including its timer; the rest is LabVIEW's CLFN
cost. **Decision: B − A = 10.3 ns ≤ 50 ns → B on all 18 CLFNs.** Cost of B on the echo path:
10.3 ns per echoed frame, 0.72 µs for a 70-frame message, 0.14 % of the 0.5 ms device tick.
Spread is small (min within 2 % of median).

## 6. Update to 1.2.0: three new functions (CIF / SocketCAN), 2026-11-02

The 18 existing functions are unchanged: their generated VIs stay as they are. Add only the 3 new
ones.

1. **Target:** copy the new `dist\` and run it (`README.md`, "Deploy"): `1612 checks` ALL PASS,
   `CROSSCHECK PASS` (now with a CIF line), then `cp libcansub_codec.so /usr/local/lib/ &&
   ldconfig`. Restart the LabVIEW application on the target.
2. **Windows:** `wizard\test_cansub_codec_win.exe` → ALL PASS (1612). Copy the new
   `wizard\libcansub_codec.dll` over `codec_vis\VIs\libcansub_codec.dll`.
3. **Wizard**, same settings as section 3, header `wizard\cansub_codec_lv.h` (regenerated), and on
   the Functions page select **only** `csb_frames_to_pcap`, `csb_encode_pcap_to_ws`,
   `csb_pcap_max_bytes`. Same library `CANsub_Codec.lvlib`, folder `codec_vis\`.

   | Function | Rule 1: Input array, Constant | Rule 2: Output, Pointer to Value | Rule 3: Input/Output array | Values |
   |---|---|---|---|---|
   | `csb_frames_to_pcap` | `t_dev` (DBL), `id` (U32), `flags` (U16), `data_len` (U8), `data` (U8) | `n_packets`, `n_skipped` (I32) | `out` (U8) | `n_frames`, `tai_minus_utc_s`, `options` (I32), `out_cap` (I32) |
   | `csb_encode_pcap_to_ws` | `in` (U8) | `n_packets`, `n_rejected`, `consumed` (I32) | `out` (U8) | `in_len`, `options` (I32), `mask` (U32), `out_cap` (I32) |
   | `csb_pcap_max_bytes` | – | – | – | `n_frames` (I32) |

   Array size: **none** for every array.
4. **After the wizard**, as section 4 for the 3 new VIs: **Specify path on diagram** with
   `CANsub_Codec_LibPath.vi`; **Constant** on the rule-1 arrays; preallocated clone, inline,
   debugging off. Save All.
5. Check: `csb version.vi` on the PXIe now returns **66048** (= `0x00010200`). Then I review the 3
   new VIs with lvkit.

## Keep in step

After any change to `cansub_codec.h`: `make wizard` again, and re-run the wizard only for the
changed functions (or edit those CLFNs by hand). Add new functions to `cansub_codec.def`, else
the DLL does not export them.
