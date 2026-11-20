# CANsub work: status checkpoint

**Updated 2026-11-19.** Read this file first if the work moves to a different person or PC. It
tells you the state, where each thing is, and how to continue. The step list with the status is
in `README.md`, "Where you are", and in `CIF_BUILD.md`, section 6.

## 1. State

| Area | State |
|---|---|
| C codec library `libcansub_codec` 1.3.0 | done, 2221 unit checks pass on desktop Linux and on the PXIe |
| LabVIEW wrappers, shared REST/TLS/WebSocket VIs | done and tested |
| LabVIEW API `CANsub_API.lvlib` (Direct ownership, Poll mode) | done; 1 h proof run passed (`API_BUILD.md`) |
| Loopback example | done (`examples\CANsub_API_Loopback_Example.vi`) |
| Connection-loss detection (TCP keepalive, error 5203 with cause) | done and tested |
| Class `CAN RW CANsub.lvclass` (child of `CAN_RW_CORE`) | done and tested on the PXIe: open/close, streaming, CAN status |
| Save version | all libraries, classes, VIs and controls are in LabVIEW 2021 format |
| VI package | `VIPM\CAN RW CANsub.vipb`, version 0.1.0, builds with LabVIEW 2021 (64-bit) |
| Reader and writer plugins (separate repos) | built and reviewed; not tested on the target yet |

## 2. Next steps (in order)

1. Plugin tests on the target: `PLUGIN_TEST.md` (stage 1 reader alone, stage 2 writer and reader).
2. Bench test with the CIF Manager, both plugins, 1 h, latency numbers.
3. Packaging of the native library: name, install path, installers (`CIF_BUILD.md`, R10).
4. Phase 2: reader and writer on the same channel (shared connection).

## 3. Open items that can block

| Item |
|---|
| Native library name and install path (`libcansub_codec.so` in `/usr/local/lib` today; the repo skeleton expects `libcansub_lv_wrapper.so` in `/usr/lib/cif/`) |
| Do the reader plugin and the writer plugin run in one LabVIEW process? (needed for Phase 2) |

## 4. Where each thing is

| What | Where |
|---|---|
| Step list and status | `README.md` (this folder), "Where you are" |
| CIF plan, class design, design points, steps | `CIF_BUILD.md` |
| Plugin test plan | `PLUGIN_TEST.md` |
| API plan, proof results, backlog B1 to B16 | `API_BUILD.md` |
| Layers, rules, error codes | `ARCHITECTURE.md` |
| Shared VIs C1 to C7, LabVIEW style rules | `..\common\COMMON_BUILD.md` |
| C library, wrappers, tests | `c_functions\README.md`, `c_functions\LABVIEW_C_TEST.md`, `c_functions\WIZARD.md` |

## 5. Bench used for the tests

| Item | Value |
|---|---|
| Device | CANsub.4, firmware 02.05.00, API 05.00, CH1 and CH2 connected with a loopback cable |
| Target | NI PXIe-8862, NI Linux RT, LabVIEW 2025 |
| Certificate on the target | `/home/lvuser/cansub_root_cert.crt` |
| PTP client on the target (manual start, hardware mode) | `ptp4l -i eno0 -s -m` and `phc2sys -s eno0 -c CLOCK_REALTIME -w -m` |
| Bit rates used | 500 kbit/s nominal, 2 Mbit/s data, sample points 80 % |

## 6. Decisions that stay valid

- API: caller-owned frame buffers (`CANsub_Frames`), abstract `CANsub_Channel` + `CANsub_DirectChannel`, `Write_Frame` + `Write_Echo`.
- No packed library for the API; applications and plugins use the source libraries (installed by the VI package).
- CIF: one class `CAN RW CANsub.lvclass` (child of `CAN_RW_CORE`); the plugins only replace the class. Interface string `<host>/<channel>`. Phase 1 = one plugin per channel.
- Edit the class in this repo only, not in the installed `vi.lib` copy. A fix needs a new package build and install.

## 7. Device rules found by test

- One WebSocket client per channel.
- The device sends one WebSocket message per channel every 0.5 ms (fixed in firmware 02.05.00).
- Never `PUT /api/time` (it stops PTP). `t_dev` is UTC.
- A channel that sends must also be read (one transmit acknowledgement per frame).
- `DELETE /api/can/{ch}/ws` while a WebSocket is connected returns 200 but has no effect.
- After a cable pull, the device frees the channel itself; no DELETE is necessary.
- REST calls need "verify server" off (the certificate name does not match the IP address).
- The device keeps the CAN error counters when a channel is closed and opened again (measured 2026-11-17: TX counter 128 after a frame with no acknowledgement, still 135 / Error Passive in the next session with no frame sent). The counter goes down by 1 for each frame sent with an acknowledgement.
- **No fast REST polling (measured 2026-11-19).** REST calls with no pause on one open HTTPS connection stopped the HTTP server of the device after 185 calls: the open connection got no answer for more than 5 s, and new clients failed (TLS handshake not completed, or header received and no body). Ping stayed good. The server answered again by itself in a few minutes, but it then stopped again at random times in later tests (also with slow requests) until the device was restarted; after the restart, 9000 requests, 3000 new TLS connections and 20 minutes with two plugins gave no fault. Restart the device after each stop of the REST server. At 2 calls in each second on an open connection, a second client got its answers in 77 ms. The reader plugin caused the same stop, because the plugin `Logic.vi` calls `Update CAN Status` in each cycle; so the class limits the REST status call to one in each second.
- **A status call must not be able to stop the data path (found in `cif_error.log`, 2026-11-20).** In 7 failed plugin runs the first error was the REST status call (`GET`, error 56 after its 5 s time limit), in the reader and the writer in the same second. The CIF core then took the plugins out of the Execute state, so the frame counts stopped; no entry showed a lost WebSocket. Since package 0.1.0.3: a failed status call gives no error (the last status stays, next try after 5 s, time limit 500 ms); error 5303 after 60 s with no status; error 5304 if the device reports the channel as stopped. The channel open has a pause of 20 ms after each REST call. The failure path of the complete class is tested only as a pure function (`Status Policy.vi`); a test with a REST server that stops is open.
- A TLS handshake with the device takes about 80 ms. A request on an open HTTPS connection takes about 0.6 ms. The connection is closed when idle for a time between 5 s and 30 s.

## 8. Conventions

- Technical text in ASD-STE100 simplified English.
- LabVIEW style rules: `..\common\COMMON_BUILD.md`.
- **File format rule (found by test, 2026-11-19).** All LabVIEW files of this repo are in LabVIEW 2021 format (save version `21.0`). A save from under a real-time target in a project writes the editor format (2025) and ignores the save version, although the toolbar shows `21.0`. A save from the main context (the file opened with no project) or from My Computer writes the 2021 format. So: edit and run under the target, then open each changed file with no project, make it dirty and save it, before a commit and before a package build. A package build with LabVIEW 2021 cannot load a file in 2025 format.
- Limits of a file-based VI review (no LabVIEW open): the pass modes of a Call Library Function Node, the wire to a For Loop N terminal, control defaults, and the wires inside a Conditional or Diagram Disable Structure cannot be read from the VI file. Check these on the diagram.
