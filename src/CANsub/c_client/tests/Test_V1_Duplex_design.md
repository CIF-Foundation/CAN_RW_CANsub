# `Test_V1_Duplex.vi`: proposed block diagram

Status: **PROPOSED** (not built). Source: `ARCHITECTURE.md` section 9. Drawn in the lvkit netlist
style, so it can be compared with `render` after the VI is built.

Conventions: one box per node. SubVIs by file name, primitives by LabVIEW name. Structures are
subgraphs (`loop1`, `for2`, `case3`, `seq4`). Edge labels are data types. `shift` = shift
register. The error wire is drawn on every chain.

**Settings:** group **T**: Non-reentrant, Normal priority, Inline No, Standard execution system,
automatic error handling off. **Run it on the PXIe target**, not on Windows: the question is
about LabVIEW TLS on NI Linux RT.

**One design choice (mine, not in section 9):** phase 1 and phase 2 are **two runs** of the same
VI, with different controls. This keeps the VI simple and gives separate numbers per phase.

| Run | `period ms` | `duration s` | Frames `N` |
|---|---|---|---|
| Phase 1 | `1` | `600` | 600 000 |
| Phase 2 | `20` | `300` | 15 000 |

Keep the CH1 Python receiver running for **both** runs (section 9 setup; set its time limit to
cover both runs plus the gap).

---

## 1. Before the loops

```mermaid
flowchart LR
  BT["C_Bit_Timing.vi"] -->|"Bit_Timing.ctl"| RS1["CANsub_REST_Setup.vi<br/>phase = before_ws"]
  RS1 -->|"Error"| TLS["CANsub_TLS_Connect.vi<br/>port 443"]
  TLS -->|"TLS refnum"| HS["WS_Handshake.vi"]
  TLS -->|"Error"| HS
  HS -->|"Error"| RS2["CANsub_REST_Setup.vi<br/>phase = after_ws"]
  RS2 -->|"Error"| W12["C_RX_Buffers_Init.vi"]
  W12 -->|"C_RX_Buffers.ctl"| W13a["C_Decode_RT.vi<br/>(leftover only)"]
  HS -->|"leftover U8[]"| W13a
  W12 -->|"Error"| W13a
  DIV["Quotient & Remainder<br/>duration s × 1000 / period ms"] -->|"I32 N"| INIT["Initialize Array<br/>DBL, 0, N"]
  W13a -->|"C_RX_Buffers.ctl"| L1(["to loop1 shift"])
  W13a -->|"Error"| L1
  HS -->|"TLS refnum"| L1
  HS -->|"TLS refnum"| L2(["to loop2"])
  INIT -->|"DBL[]"| L2
  W13a -->|"Error"| L2
```

- Order matters: `after_ws` (the two filter POSTs) runs **after** the handshake. Without the
  filters the device sends nothing, also no TX acks.
- `C_Decode_RT` on `leftover` keeps the HDLC state in step. It finds no acks (nothing is written
  yet).
- `C_Bit_Timing` inputs: 500 kbit/s, 2 Mbit/s, 80 % (the bench defaults).
- The TLS refnum branches to both loops. That branch is the subject of the test.

## 2. `loop1`: reader (While Loop)

```mermaid
flowchart LR
  subgraph loop1["while loop (reader)"]
    RD["TCP Read<br/>Immediate, 4096, 100 ms"] -->|"string"| S2B["String To Byte Array"]
    S2B -->|"U8[]"| W13["C_Decode_RT.vi"]
    RD -->|"Error"| E56["Unbundle By Name (code)<br/>Equal? 56"]
    E56 -->|"Bool is56"| SEL["Select<br/>is56 ? no error : error"]
    RD -->|"Error"| SEL
    SEL -->|"Error"| W13
    W13 -->|"C_Decode_Result.ctl"| RES["Unbundle By Name<br/>n frames, ping len,<br/>crc errors, decode errors"]
    RES -->|"I32 n frames"| FOR2
    W13 -->|"C_RX_Buffers.ctl"| UNB["Unbundle By Name<br/>flags, data"]
    subgraph FOR2["for2 (N = n frames)"]
      IXF["Index Array flags[i]"] -->|"U16"| AND["And 0x0020"]
      AND -->|"U16"| NZ["Not Equal To 0?"]
      NZ -->|"Bool"| CASE3
      subgraph CASE3["case3 (True = TX ack)"]
        SUB["Array Subset<br/>data, i×64, 4"] -->|"U8[4]"| TC["Type Cast → U32"]
        TC -->|"U32 value"| EQ["Equal? expected"]
        EQ -->|"Bool"| INC["Select +1 / +0<br/>ack data errors"]
        TC -->|"U32"| NXT["Increment<br/>expected = value + 1"]
        ACK["Increment acks"]
      end
    end
    UNB -->|"U16[] flags"| IXF
    UNB -->|"U8[] data"| SUB
    RES -->|"U32 crc errors, I32 decode errors, I32 ping len"| SUM["Add to totals<br/>ping len ≥ 0 → pings +1"]
    W13 -->|"Error"| CHK["Error code ≠ 0?<br/>count, keep first code"]
    CHK -->|"Bool fatal"| OR["Or (3 inputs)"]
    STOP["Stop (local var)"] -->|"Bool"| OR
    DONE["done (local var) And is56"] -->|"Bool"| OR
    OR -->|"Bool"| COND(("stop"))
  end
  W13 -.->|"shift C_RX_Buffers.ctl"| W13
  NXT -.->|"shift U32 expected (init 0)"| EQ
  INC -.->|"shift U32 ack data errors"| INC
  ACK -.->|"shift U32 acks"| ACK
  SUM -.->|"shift totals"| SUM
```

Rules for this loop:
- **Error 56** (read timeout) is normal. Clear it and continue.
- Any other error: `read errors` + 1, keep the first code in `first read error`, **stop**. A
  broken connection makes more reads useless.
- **Stop** when: the Stop button, or a fatal error, or (`done` = True **and** this read timed out).
  The last condition drains the acks that arrive after the last write.
- **Pings:** count them, **do not** send a pong. A pong is a second writer, and that would change
  the test. **[verify]** If `pings seen` > 0 and the run ends with error 66, the device needs a
  pong; then we add it.
- `expected` starts at `0`. Each ack must hold `expected` in bytes 0–3, else `ack data errors` + 1.
  Then `expected` = value + 1, so one bad ack counts once and not for every later ack.

## 3. `loop2`: writer (While Loop)

```mermaid
flowchart LR
  subgraph loop2["while loop (writer)"]
    I["i (iteration)"] -->|"I32"| U32["To Unsigned Long Integer"]
    U32 -->|"U32 counter"| TCW["Type Cast → U8[]"]
    TCW -->|"U8[4]"| BA["Build Array (concatenate)<br/>+ U8[4] zeros"]
    BA -->|"U8[8]"| W10["C_Encode_CAN_To_WS.vi<br/>id 0x7FF, ide F, fdf F, brs F"]
    W10 -->|"U8[]"| B2S["Byte Array To String"]
    subgraph seq4["flat sequence"]
      direction LR
      F0["frame 0: Wait Until Next ms Multiple<br/>period ms"] --> F1["frame 1: High Resolution<br/>Relative Seconds → t0"]
      F1 --> F2["frame 2: TCP Write<br/>timeout 1000 ms"]
      F2 --> F3["frame 3: High Resolution<br/>Relative Seconds → t1"]
    end
    B2S -->|"string"| F2
    W10 -->|"Error"| F2
    F3 -->|"DBL t1"| SUBT["Subtract t1 − t0"]
    F1 -->|"DBL t0"| SUBT
    SUBT -->|"DBL seconds"| RAS["Replace Array Subset<br/>index i"]
    F2 -->|"Error"| WCHK["Error code ≠ 0?<br/>count, keep first code"]
    WCHK -->|"Bool"| OR2["Or (3 inputs)"]
    LAST["i + 1 ≥ N"] -->|"Bool"| OR2
    STOP2["Stop (terminal)"] -->|"Bool"| OR2
    OR2 -->|"Bool"| STP2(("stop"))
  end
  RAS -.->|"shift DBL[] durations"| RAS
  WCHK -.->|"shift Error"| W10
```

Rules for this loop:
- The **flat sequence** forces the order wait → `t0` → TCP Write → `t1`. Without it, LabVIEW can
  run High Resolution Relative Seconds at any time, because it has no error terminals.
- Only the TCP Write call is inside `t0`…`t1`. Encoding is outside the measurement.
- `durations` is pre-allocated (section 1). **Replace Array Subset** writes in place. No
  allocation in the loop except W10 and the TCP Write string (both small, accepted for a test).
- Any write error: `write errors` + 1, keep `first write error`, **stop**.
- `frames written` = `i` + 1 at exit. After the loop: write `done` = True (the reader reads it
  with a local variable; local variables are acceptable in this test only).

## 4. After the loops

```mermaid
flowchart LR
  R1(["loop1: Error"]) -->|"Error"| ME["Merge Errors"]
  R2(["loop2: Error"]) -->|"Error"| ME
  ME -->|"Error"| C7["CANsub_WS_Close.vi"]
  REF(["TLS refnum"]) -->|"TLS refnum"| C7
  C7 -->|"Error"| SEH["Simple Error Handler.vi"]
  D(["loop2: DBL[] durations"]) -->|"DBL[]"| AS["Array Subset<br/>0, frames written"]
  AS -->|"DBL[]"| SORT["Sort 1D Array"]
  SORT -->|"DBL[]"| IX["Index Array<br/>⌊0.5·n⌋, ⌊0.99·n⌋, n−1"]
  IX -->|"DBL ×3"| US["Multiply × 1e6"]
  US -->|"DBL µs"| OUT["median / p99 / max indicators"]
```

C7 runs also when there is an error (it clears `error in` itself). Its error output is merged, so
the first error stays visible.

## Connector pane / front panel

Top-level test VI: these are front-panel terminals. Connector pane is optional.

| Inputs | Outputs |
|---|---|
| `host` (string, no default) | `frames written` (I32) |
| `cert path` (path) | `write median µs`, `write p99 µs`, `write max µs` (DBL) |
| `channel` (U8, `2`) | `write errors` (U32), `first write error` (I32) |
| `period ms` (U32, `1`) | `read errors` (U32), `first read error` (I32) |
| `duration s` (U32, `600`) | `acks counted` (U32), `ack data errors` (U32) |
| `read timeout ms` (I32, `100`) | `crc errors` (U32), `decode errors` (I32), `pings seen` (U32) |
| `Stop` (Bool) | `durations` (DBL[]), `done` (Bool), `error out` |

## Expected values (phase 1, parallel case)

| Value | Expected |
|---|---|
| `frames written` | `600000` |
| `acks counted` | = `frames written` |
| `ack data errors`, `crc errors`, `decode errors` | `0` |
| `write errors`, `read errors` | `0` |
| `write p99 µs` | ≪ 1000 |
| CH1 count | = `frames written` |

Interpretation: the table in `ARCHITECTURE.md` section 9. The key comparison is **phase 2
`write p99`**: near 100 000 µs (the read timeout) means **serialized**.

## After you build it

I run `index` (refresh), `unresolved`, then `render` on the VI, and compare with this design.
Known blind spots for lvkit: TCP Open Connection, TCP Get Raw Net Object (inside C4), and CLFNs
(untested).
