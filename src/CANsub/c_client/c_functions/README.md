# libcansub_codec.so: the native-client codecs in C

Part of the **C client** project (`labview\c_client\README.md`). C versions of the build-doc VIs
(`hil_test\LABVIEW_NATIVE_CLIENT_BUILD.md`, steps 1.1–1.7 and
2.0), plus two one-call helpers for the echo loop. Same algorithms, same byte layouts, same test
vectors. Use it to compare with the G versions, or instead of them.

| File | Purpose |
|---|---|
| `cansub_codec.h` / `cansub_codec.c` | the library (C11 with GNU extensions; no malloc, no global state, thread-safe) |
| `crc32_table.inc` | CRC32 lookup table (generated) |
| `test_cansub_codec.c` | generated test program: CSS TV-01…TV-20, every build-doc table, the helpers, the CIF conversion (1612 checks) |
| `bench_cansub_codec.c` | execution time per call (median / p99 / p99.9 / max), optional SCHED_FIFO + mlockall |
| `gen\` | the test generator: `python3 gen/gen_ctest.py` rewrites `test_cansub_codec.c` from the Python models (`models.py`, `bt.py`), the CSS vectors (`hil_test\codec_ref_tv.py`) and `helpers_test.c` |
| `crosscheck.py` | fuzz test of the `.so` with ctypes against zlib, the build-doc models and python-can |
| `Makefile` | `make`, `make test`, `make check` (sanitizers), `make bench`, `make dist`, `make clean` |
| `LABVIEW_C_TEST.md` | **LabVIEW side:** wrapper VIs (one CLFN each, CLFN tables), test VIs and the test procedure P0–P5 |

## Status (2026-11-13, version 1.3.0, built and tested on desktop Linux x86_64, gcc 11.4)

- **1.3.0 (2026-11-13):** `csb_frames_to_pcap_range` for the CIF `CAN RW CANsub` class: converts a
  range of frames (`first`, `max_packets`, returns `next`) and applies allow and deny ID lists
  (the rule of `Filter Packets` in `CAN_RW_CORE`). All earlier functions are unchanged
  (`csb_frames_to_pcap` now calls the same internal code; its 1612 checks still pass).
  `make test`: **2221 checks, 0 failures**, also with AddressSanitizer + UBSan; `crosscheck.py`:
  new line "csb_frames_to_pcap_range vs model", 2000 cases, 0 mismatches. 5 mutations of the new
  code were each detected by the tests. `csb_version()` = **66304** (`0x00010300`).
  **Not yet on the PXIe** (1.2.0 is there): copy `dist/` again.

- **1.2.0 (2026-11-02):** pcap CAN packets (`LINKTYPE_CAN_SOCKETCAN`, the NI CIF format), aligned
  with NI's CIF VIs (section "pcap CAN packets" below). The 1.1.0 functions are unchanged. **Not yet on the PXIe**: copy `dist/` again.
- `make`: 0 warnings with `-Wall -Wextra -Wpedantic -Wconversion`.
- `make test`: **1612 checks, 0 failures**. `make check` (AddressSanitizer + UBSan): 0 failures.
- `crosscheck.py`: 0 mismatches. CRC32 vs zlib 3000 cases; HDLC encode 3000; CAN serialize 3000
  (classic/FD, std/ext); HDLC decode + CAN deserialize ~2900 frames in random split streams with
  garbage; bit timing vs python-can 4.6.1: identical in all 672 one-prescaler cases (224
  two-prescaler cases return `CSB_ERR_NO_SOLUTION` by design); `csb_frames_to_pcap` vs a Python
  copy of NI's Build Socket CAN.vi: 2000 random multi-frame cases, 0 mismatches.
- 1.1.0 ran on the PXIe-8862 on 2026-10-28 (all checks passed, bench below).

## Deploy: build on the desktop, run on the target

The `.so` built on desktop Linux runs on NI Linux RT x64. Checked on the binary (2026-10-23):

| Property | Value | Why it matters |
|---|---|---|
| glibc symbol versions needed | ≤ **GLIBC_2.14** (`memcpy`); `memset`, `nearbyint` 2.2.5; `__stack_chk_fail` 2.4 | any glibc since 2011 has them; NI Linux RT's is much newer |
| libraries | `libc.so.6`, `libm.so.6` only | present on every NI Linux RT target |
| CPU | `-march=x86-64` (baseline), no AVX / SSE4 instructions | runs on any x86-64, including the PXIe-8862 |
| imported functions | `memcpy`, `memset`, `nearbyint`, `__stack_chk_fail` | **no malloc, no system calls, no locks, no I/O** |

**On the desktop** (any x86-64 Linux with gcc):
```
make dist      # -> dist/: libcansub_codec.so, test_cansub_codec_static, bench_cansub_codec_static,
               #           crosscheck.py, SHA256SUMS
```
The test and bench programs are linked **statically**: they run on the target without a compiler
and without depending on the target's library versions.

**Copy to the target** (PC, PowerShell). Remove the old folder on the target first
(`rm -rf /home/admin/csb_dist`): if it exists, `scp -r` puts the new files in `csb_dist/dist` and
the old files stay in `csb_dist` (found 2026-11-16: the old 927-check test ran).
```
scp -r C:\Dev\CIF\CAN_RW_CANsub\src\CANsub\c_client\c_functions\dist admin@<rt-ip>:/home/admin/csb_dist
```
**On the target** (as admin):
```
cd /home/admin/csb_dist
chmod +x *_static                          # scp from Windows drops the execute bit
sha256sum -c SHA256SUMS                    # 4 x OK: the copy is complete
./test_cansub_codec_static                 # 2221 checks, 0 failures / ALL PASS
python3 crosscheck.py ./libcansub_codec.so # CROSSCHECK PASS: the .so itself loads and works here
./bench_cansub_codec_static --fifo 80      # timing under SCHED_FIFO + mlockall (see below)
cp libcansub_codec.so /usr/local/lib/ && ldconfig
```
`crosscheck.py` is the check that the **shared library** loads on this target (ctypes → `dlopen`,
the same way LabVIEW loads it). The bit-timing part needs python-can; run it in `cansub-venv`, or
it is skipped.

Building on the target also works (`opkg install packagegroup-core-buildessential` **[verify]**,
then `make`, `make test`), but it is not needed.

## Real-time properties

| RT rule | How the library meets it |
|---|---|
| No dynamic memory | no `malloc` anywhere; all buffers are the caller's (LabVIEW arrays) |
| No blocking, no system calls | imports only `memcpy`, `memset`, `nearbyint`; no I/O, no locks, no sleeps |
| Bounded execution time | every function is O(input length), with fixed limits: HDLC frame ≤ 4096 B, WS buffer ≤ 16384 B, ≤ 64 data bytes per frame; bit timing 256 fixed iterations (setup only) |
| Small, fixed stack | ≤ 320 B per function (`-fstack-usage`), < 1 kB call depth |
| Thread-safe / reentrant | no global state (only the constant CRC table); "Run in any thread" in the CLFN |
| No page faults in the loop | `csb_warmup()` before the loop runs the whole encode → decode path once (code pages, CRC table, state buffers), then resets the states |
| Zero-copy echo | `csb_encode_rx_frame_to_ws()` encodes straight from the decode buffers: no copy of the frame data in LabVIEW |

**Measured on the desktop** (`make bench`, normal priority; the max values are this machine's
scheduling, not the code):

| Call | Median | p99 |
|---|---|---|
| decode: one device message (1200 B, 70 CAN frames) | 8.6 µs | 13–15 µs |
| echo: `csb_encode_rx_frame_to_ws`, one frame | 0.07 µs | 0.1 µs |
| CRC32 over 1200 B | 2.3 µs | 2.4–3.6 µs |

The device sends one WebSocket message per channel every 0.5 ms; decoding it takes < 2 % of that.

**Target, measured 2026-10-28** (PXIe-8862, NI Linux RT, `bench_cansub_codec_static --fifo 80`:
SCHED_FIFO 80 + mlockall, 100 000 calls each, `dist\` build 1.1.0; no LabVIEW application
running, assumed: not recorded):

| Call | Median | p99 | p99.9 | Max |
|---|---|---|---|---|
| decode: one device message (1200 B, 70 CAN frames) | 8.55 µs | 8.96 µs | 9.20 µs | 12.13 µs |
| echo: `csb_encode_rx_frame_to_ws`, one frame | 0.07 µs | 0.08 µs | 0.08 µs | 0.83 µs |
| CRC32 over 1200 B | 2.31 µs | 2.33 µs | 2.33 µs | 3.79 µs |

`clock_gettime` overhead 16 ns per call, included. Worst case decode = 12.1 µs = 2.4 % of the
0.5 ms device tick. Same run: static test ALL PASS (927), `crosscheck.py` PASS (bit-timing part
skipped: no python-can in that Python), Windows DLL test ALL PASS. These are the numbers for the
report.

## Calling from LabVIEW (Call Library Function Node)

Same for every function:
- **Function** tab: Library name or path = `/usr/local/lib/libcansub_codec.so` (full path, with the
  leading `/`). Or, after `ldconfig`, just `libcansub_codec.so` with **no** leading `/`.
  Calling convention **C**. Thread **Run in any thread** (all functions are thread-safe).
- **Parameters** tab, by C type:

| C type | LabVIEW type | Pass |
|---|---|---|
| `const uint8_t *` / `uint8_t *` (array) | Array, U8, 1 dimension | **Array Data Pointer** |
| `uint32_t *` / `uint16_t *` / `double *` / `int32_t *` (array) | Array, U32 / U16 / DBL / I32 | **Array Data Pointer** |
| `int32_t *` / `uint32_t *` / `double *` (one output value) | Numeric I32 / U32 / DBL | **Pointer to Value** |
| `int32_t`, `uint32_t`, `uint8_t`, `double` (input value) | Numeric I32 / U32 / U8 / DBL | **Value** |
| return `int32_t` / `uint32_t` | Numeric I32 / U32 | – |

- **Output arrays must exist before the call**, with at least the size given below. Make them
  with **Initialize Array** outside the loop, keep them in shift registers, and never resize them
  in the loop. The function only writes into them. After the call, take the valid part with
  **Array Subset** (length = the returned length or count).
- **Return value < 0** = error: `-1` buffer too small, `-2` invalid input, `-3` no bit-timing
  solution, `-4` state not initialised. Check it after every call.

### State buffers (HDLC decode and WS parse are stateful)

A state is a U8 array that the C code uses as its memory between calls:
1. **Initialize Array** (U8, size from the table) once, before the loop.
2. Call `csb_hdlc_state_init` / `csb_ws_state_init` once on it.
3. Keep it in a shift register and pass it to every call (Array Data Pointer). Do not modify it.

| State | Size (U8 elements) |
|---|---|
| HDLC | 4112 (`csb_hdlc_state_size()`) |
| WebSocket | 16432 (`csb_ws_state_size()`) |

Use **one WS state per API**. A WS state that was used with `csb_ws_parse_frames` returns `-4`
in `csb_decode_ws_to_can`, and the reverse.

## Functions

| Function | Build doc | Inputs → outputs | Output sizes |
|---|---|---|---|
| `uint32 csb_crc32(data, len)` | 1.1 | bytes → CRC32 | – |
| `int32 csb_hdlc_encode(payload, len, frame, frame_cap)` | 1.2 | payload → frame; returns frame length | `frame` ≥ 2·(len+4)+2 |
| `int32 csb_hdlc_state_init(state, size)` | 1.3 | – | state 4112 |
| `int32 csb_hdlc_decode(state, size, in, in_len, out, out_cap, lens, lens_cap, *n_payloads, *crc_errors)` | 1.3 | bytes → payloads concatenated in `out`, length of each in `lens` | `out` ≥ in_len + 4096; `lens` ≥ in_len/6 + 1 |
| `int32 csb_can_deserialize(payload, len, t_dev, id, flags, dlc, data_len, data, max_frames, *n_frames, *decode_errors)` | 1.4 | payload → frames (parallel arrays) | each array ≥ len/7 + 1; `data` ≥ 64 × that |
| `int32 csb_can_serialize(id, ide, fdf, brs, data, len, payload, cap)` | 1.5 | frame → payload; returns length | `payload` ≥ 75 |
| `int32 csb_ws_build_frame(payload, len, opcode, mask, frame, cap)` | 1.6 | payload → masked client frame; returns length | `frame` ≥ len + 14 |
| `int32 csb_ws_state_init(state, size)` | 1.7 | – | state 16432 |
| `int32 csb_ws_parse_frames(state, size, in, in_len, opcodes, lens, max_frames, out, out_cap, *n_frames)` | 1.7 | bytes → complete frames | `out` ≥ in_len + 16384; arrays ≥ (in_len + 16384)/2 + 1 |
| `int32 csb_bit_timing(nom_rate, nom_sp, data_rate, data_sp, timing[8], *nom_sp_out, *data_sp_out)` | 2.0 | bit rates → `brp, seg1, seg2, sjw, data_brp, data_seg1, data_seg2, data_sjw` (U16) | `timing` 8 × U16 |
| `int32 csb_encode_can_to_ws(id, ide, fdf, brs, data, len, mask, out, cap)` | helper | CAN frame → WS frame for TCP Write (serialize + HDLC + WS, opcode 2) | `out` ≥ 174 |
| `int32 csb_decode_ws_to_can(ws_state, ws_size, hdlc_state, hdlc_size, in, in_len, t_dev, id, flags, dlc, data_len, data, max_frames, *n_frames, *peer_close, ping_payload, *ping_len, *crc_errors, *decode_errors)` | helper | TCP Read bytes → CAN frames, plus close/ping | arrays ≥ `csb_max_frames(in_len)` = (in_len + 4096)/7 + 1; `ping_payload` 125 |
| `int32 csb_max_frames(in_len)`, `csb_version()`, `csb_*_state_size()` | – | sizes and version | – |
| `int32 csb_encode_rx_frame_to_ws(new_id, index, n_frames, flags, data_len, data, mask, out, cap)` | 1.1.0 | frame `index` of the decode arrays → WS frame with `new_id` (zero-copy echo); error frames refused | `out` ≥ 174 |
| `int32 csb_warmup(ws_state, ws_size, hdlc_state, hdlc_size)` | 1.1.0 | pre-fault the whole path once, then reset both states; returns 2 | – |
| `int32 csb_frames_to_pcap(n_frames, t_dev, id, flags, data_len, data, tai_minus_utc_s, options, out, cap, *n_packets, *n_skipped)` | 1.2.0 | decode arrays → concatenated pcap CAN packets (pcap header + SocketCAN header + payload); skips TX acks and error frames by default | `out` ≥ `csb_pcap_max_bytes(n_frames)` = 88 × n |
| `int32 csb_encode_pcap_to_ws(in, in_len, options, mask, out, cap, *n_packets, *n_rejected, *consumed)` | 1.2.0 | concatenated pcap CAN packets → concatenated WS frames for one TCP Write; keeps an incomplete last packet (`consumed`) | `out` ≥ 174 per packet |
| `int32 csb_pcap_max_bytes(n_frames)` | 1.2.0 | capacity for `csb_frames_to_pcap` | – |
| `int32 csb_frames_to_pcap_range(n_frames, first, max_packets, t_dev, id, flags, data_len, data, allow, n_allow, deny, n_deny, tai_minus_utc_s, options, out, cap, *n_packets, *n_filtered, *n_skipped, *next)` | 1.3.0 | as `csb_frames_to_pcap` for frames `first` …, at most `max_packets` packets, with allow / deny ID lists; `*next` = first frame not examined. Capacity: min(`max_packets`, `n_frames` − `first`) × 88 | – |

## pcap CAN packets (1.2.0): the NI CIF format

Checked with lvkit on 2026-11-02 against LabVIEW 2025 `vi.lib\CIF Foundation\CIF PCAP Parse and
Build`: `Example\Build Socket CAN.vi`, `Build\SubVIs\Flatten SocketCAN Header.vi`, `Build\Build
Packet Header.vi` (+ `Flatten Packet Header.vi`) and `Example\Parse Next Socket CAN.vi`. Format:
[LINKTYPE_CAN_SOCKETCAN](https://www.tcpdump.org/linktypes/LINKTYPE_CAN_SOCKETCAN.html).

**Naming:** the functions are called `pcap`, not `socketcan` or `cif` (decision 2026-11-02): the
bytes are a pcap packet record + a `LINKTYPE_CAN_SOCKETCAN` frame (big-endian `can_id`, no
padding), not the Linux `struct can_frame`; and they are a standard format, with CIF-specific
choices only in the defaults. The LabVIEW wrappers (W16, W17) keep CIF names.

One packet per CAN frame, packets concatenated (the stream that `Parse Next Socket CAN.vi` reads):

| Bytes | Field | Value |
|---|---|---|
| 0–3 | `ts_sec` | TAI seconds (U32; byte order of the pcap header: big-endian by default in this library, **little-endian in the CIF CAN plugins**, option `CSB_PCAP_LITTLE_ENDIAN`) |
| 4–7 | `ts_nsec` | nanoseconds (U32, same byte order) |
| 8–11, 12–15 | `incl_len`, `orig_len` | 8 + payload length (same byte order) |
| 16–19 | CAN ID + flags | **always big-endian**: `id AND 1FFFFFFF`, `OR 80000000` extended, `OR 40000000` remote |
| 20 | payload length | data bytes; **0 for remote** |
| 21 | FD flags | 0 classic, `04` FD, `05` FD + BRS (ESI is not set, as NI) |
| 22, 23 | reserved, len8_dlc | 0, 0 |
| 24 … | payload | the data bytes, **no padding** |

**Correction 2026-11-16:** the CIF CAN plugins use a **little-endian** pcap header (`CAN_RW_CORE` sets the NI pcap class to Little Endian; measured with NI's `Parse Next Socket CAN.vi`). Use `CSB_PCAP_LITTLE_ENDIAN` for CIF. The library default stays big-endian.

Decisions (Marcus, 2026-11-02): pcap header **big-endian** (superseded for CIF, see above); time base **TAI** (TAI = UTC + 37 s;
the C functions take the offset as a parameter, `CSB_TAI_MINUS_UTC`); **TX acks and error frames
are skipped** by default (options `CSB_PCAP_KEEP_TX_ACK`, `CSB_PCAP_KEEP_ERRORS`; CIF's parser has
no error frames and ignores the error flag). The CANsub cannot transmit remote or error frames, so
`csb_encode_pcap_to_ws` skips them and counts them in `n_rejected`.

Desktop bench (VM, normal priority): `csb_frames_to_pcap` for the 70-frame message 0.79 µs median;
`csb_encode_pcap_to_ws` for one 8-byte packet 0.08 µs median.

**Frame arrays** (outputs of the two decoders), one element per frame `i`:
- `t_dev[i]` (DBL, s since 1970 UTC)
- `id[i]` (U32)
- `flags[i]` (U16): bit 0 IDE, 1 FDF, 2 RTR, 3 BRS, 4 ESI, 5 TX ack, 6 error frame; bits 8–12 error type
- `dlc[i]` (U8, the code 0–15)
- `data_len[i]` (U8, 0–64)
- data bytes in `data[i·64 … i·64 + data_len[i] − 1]`

In LabVIEW, get the data with **Array Subset** (`data`, `i` × 64, `data_len[i]`), and each flag
with **And** + **Not Equal To 0?**, as in 1.4.

## The echo loop (real-time pattern)

The LabVIEW side is in `LABVIEW_C_TEST.md` (wrappers W12–W14) and `..\README.md` (S5). In short:
1. Before the loop: allocate every buffer **once** (W12 `C_RX_Buffers_Init`: the two states, the
   frame arrays for `csb_max_frames(4096)` = 1171 frames, ping 125 B, tx 174 B), then
   `csb_warmup`.
2. In the loop: TCP Read (Immediate, max 4096) → `csb_decode_ws_to_can` into the **same** buffers
   (W13, in place) → For Loop over `n_frames`, reading `id[i]`, `flags[i]` and `data[i·64 + k]`
   directly → echo with `csb_encode_rx_frame_to_ws` (W14, index `i`) → TCP Write.
3. Ping (`ping_len` ≥ 0) → `csb_ws_build_frame` (opcode 10). `peer_close` = 1 → leave the loop.

Error 56 on TCP Read: clear it and skip the decode (no bytes). The states carry partial frames
to the next read.

## Differences from the G VIs (on purpose)

- **HDLC frame limit:** an HDLC frame longer than 4096 unstuffed bytes is dropped and counted as a
  CRC error (the G VI grows its buffer without limit). Device frames are below ~1400 bytes.
- **WS parse:** a buffered frame longer than 16384 bytes returns `-1` and resets the state. In
  `csb_decode_ws_to_can`, binary frames of any length are streamed (no limit).
- **WS frames:** handles masked server frames and the 8-byte length (the G VI does not; the
  device sends neither).
- **Bit timing:** identical to `CANsub_Bit_Timing.vi` and python-can (one prescaler).
- **Mask:** `csb_ws_build_frame` takes the mask as an input (U32 `0x12345678` = bytes `12 34 56
  78`), so a random mask per frame is possible later.
