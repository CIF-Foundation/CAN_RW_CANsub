# CANsub work: hand-over checkpoint

**Written 2026-11-13.** Read this file first if the work moves to a different person, PC or
assistant session. It tells you the state, where each thing is, and how to continue. The step
list with the status is in `README.md`, "Where you are".

## 1. State

| Area | State |
|---|---|
| Device evaluation (Python harness, report) | done (October 2026). Files: `hil_test\` in the CANSub repo |
| C codec library `libcansub_codec` 1.2.0 | done, deployed on the PXIe (`/usr/local/lib`), 1612 unit checks pass |
| LabVIEW wrappers, shared REST/TLS/WebSocket VIs | done and tested |
| LabVIEW API `CANsub_API.lvlib` (Direct ownership, Poll mode) | done; 1 h A/B proof passed (`API_BUILD.md`) |
| Loopback example | done (`examples\CANsub_API_Loopback_Example.vi`) |
| Connection-loss detection (TCP keepalive, error 5203 with cause) | done and tested 2026-11-13 (`API_BUILD.md`, A7) |
| CIF plugins | **plan written, build not started.** Next step: M0 (`CIF_BUILD.md` section 4) |

Last commits in the CANSub repo (branch `c-client`): `4f29add` A7, `afd71af` CIF plan,
`6d96534` Jira tasks.

## 2. Next steps (in order)

1. **M0:** move `labview\common` and `labview\c_client` to
   `C:\Dev\CIF\CAN_RW_CANsub\src\CANsub\`, on a new branch `feature/cansub-rw-class`.
   Commands: `CIF_BUILD.md` section 4 and the list below. The branch does not exist yet.
2. R1 to R10 in `CIF_BUILD.md` section 6 (C library 1.3.0, pcap wrapper VIs, `Write_Raw`,
   the class `CAN RW CANsub.lvclass`, CAN status, the two plugins, bench test, packaging).
3. Phase 2 (reader and writer on the same channel) after Phase 1.

M0 commands (PowerShell):

```
cd C:\Dev\CIF\CAN_RW_CANsub
git status
git checkout -b feature/cansub-rw-class
git -C C:\Dev\CANSub archive -o "$env:TEMP\cansub_move.zip" HEAD labview/common labview/c_client
Expand-Archive "$env:TEMP\cansub_move.zip" "$env:TEMP\cansub_move"
New-Item -ItemType Directory src\CANsub
Move-Item "$env:TEMP\cansub_move\labview\common" src\CANsub\common
Move-Item "$env:TEMP\cansub_move\labview\c_client" src\CANsub\c_client
Copy-Item C:\Dev\CANSub\labview\c_client\codec_vis\VIs\*.dll src\CANsub\c_client\codec_vis\VIs\
```

Check (LabVIEW closed first): `Rename-Item C:\Dev\CANSub\labview labview_old`, open
`src\CANsub\c_client\CANsub_C_Client.lvproj`, no missing items and no conflicts, run
`Test_C_Smoke.vi` and the loopback example on the PXIe. Then commit in `CAN_RW_CANsub`, rename
`labview_old` back, `git rm -r labview/common labview/c_client` in CANSub, commit, and delete the
untracked remains of the two folders.

## 3. Open items that can block

| Item | Who |
|---|---|
| Is the code permitted in `CAN_RW_CANsub` (CIF Foundation, Apache 2.0, public?) Do not push the branch before the answer. | Marcus |
| Native library name and install path (`libcansub_codec.so` in `/usr/local/lib` today; the repo skeleton expects `libcansub_lv_wrapper.so` in `/usr/lib/cif/`) | Marcus + the skeleton author |
| Do the reader plugin and the writer plugin run in one LabVIEW process? (needed for Phase 2) | to find out |

## 4. Where each thing is

| What | Where |
|---|---|
| Step list and status | `README.md` (this folder), "Where you are" |
| CIF plan, class design, design points P1 to P9, steps | `CIF_BUILD.md` |
| API plan, A6 and A7 results, backlog B1 to B11 | `API_BUILD.md` |
| Layers, rules, error codes | `ARCHITECTURE.md` |
| Shared VIs C1 to C7, LabVIEW style rules | `..\common\COMMON_BUILD.md` |
| C library, wrappers, tests | `c_functions\README.md`, `c_functions\LABVIEW_C_TEST.md`, `c_functions\WIZARD.md` |
| Jira tasks for the CIF build (7 tasks, Markdown) | CANSub repo `hil_test\jira_cif_tasks.txt` |
| Evaluation report, harness, device notes | CANSub repo `hil_test\` |
| CIF repos | `C:\Dev\CIF\`: `CAN_RW_Core`, `CAN_RW_XNET`, `CAN_RW_CANsub`, `Plugin_CAN_Reader_CANsub`, `Plugin_CAN_Writer_CANsub` |
| Presentation "CANsub HIL Integration" (6 slides) | Claude artifact, owned by Marcus (private until shared) |
| Test results of the 1 h runs | `C:\Users\mmonroe\Downloads\A6 Test Results` |

## 5. Bench

| Item | Value |
|---|---|
| Device | CANsub.4, firmware 02.05.00, API 05.00, `192.168.0.49`, CH1 and CH2 connected with a loopback cable |
| Target | NI PXIe-8862, NI Linux RT, LabVIEW 2025, interface `eno0` |
| Certificate on the target | `/home/lvuser/cansub_root_cert.crt` |
| PTP client on the target (manual start, hardware mode) | `ptp4l -i eno0 -s -m` and `phc2sys -s eno0 -c CLOCK_REALTIME -w -m` |
| Bit rates used | 500 kbit/s nominal, 2 Mbit/s data, sample points 80 % |

## 6. Decisions that stay valid

- API: caller-owned frame buffers (`CANsub_Frames`), abstract `CANsub_Channel` + `CANsub_DirectChannel`, `Write_Frame` + `Write_Echo`.
- No packed library for the API; applications and plugins use the source libraries.
- CIF: one new class `CAN RW CANsub.lvclass` (child of `CAN_RW_CORE`); the plugins only replace the class. Interface string `<host>/<channel>`. Phase 1 = one plugin per channel.
- One copy of the code only (after M0: in `CAN_RW_CANsub`). Backlog work is done there.

## 7. Device rules found by test

- One WebSocket client per channel.
- The device sends one WebSocket message per channel every 0.5 ms (fixed in firmware 02.05.00).
- Never `PUT /api/time` (it stops PTP). `t_dev` is UTC.
- A channel that sends must also be read (one transmit acknowledgement per frame).
- `DELETE /api/can/{ch}/ws` while a WebSocket is connected returns 200 but has no effect.
- After a cable pull, the device frees the channel itself; no DELETE is necessary.
- REST calls need "verify server" off (the certificate name does not match the IP address).

## 8. How the work is done

- Marcus builds the VIs in LabVIEW. The assistant gives build steps (front panel table with the connector position, numbered block diagram steps with palette names, VI properties, checks, test) and reviews each saved VI with the lvkit reader.
- Technical text in ASD-STE100 simplified English. Jira text in Markdown.
- Commit messages start with the placeholder `<KEY>` for the Jira key. Marcus does the commits.
- lvkit limits: it cannot see CLFN pass modes, the wire to a For Loop N terminal, or control defaults. Ask for a front-panel check for these. `index` and `query` time out on the CIF plugin repos; `read_vi` and `read_ctl` by path work.
