# CANsub CIF backlog

**Updated 2026-11-24.** Open items after the bench test with the CIF Manager (stage 3).
Sizes are estimates, not measured values.

Priority: **1** = needed to close the plugin task, **2** = robustness, **3** = next tasks,
**4** = later or optional. Section 5 is the map of all device API calls. The overview table
below gives the type of work (LabVIEW, C, test) and the time for each item.

## Where things are

| Short name | Where |
|---|---|
| class repo | this repo (`CAN_RW_CANsub`) |
| reader repo, writer repo | `Plugin_CAN_Reader_CANsub`, `Plugin_CAN_Writer_CANsub` |
| CIF core source | repo `CIF-Core-LabVIEW`, `src\CIFCore\Class\` |
| docs | this folder: `HANDOFF.md` (state, device rules), `PLUGIN_TEST.md` (test steps and results), `API_BUILD.md` (API plan, items B1 to B16), `CIF_BUILD.md` (class design), `ARCHITECTURE.md` (layers, error codes) |
| bench project | reader repo, `Tests\CANsub Plugins Bench Test.lvproj` (both plugins under one real-time target) |
| latency helpers | reader repo `Tests\Test Helpers\Latency Reader.vi`; writer repo `Tests\Test Helpers\Latency Writer.vi` |
| bench | CANsub.4 (firmware 02.05.00), CH1 connected to CH2; NI PXIe-8862 (NI Linux RT); CIF error log `/var/local/cif/logs/cif_error.log` on the target |

**Rules to remember before any work**

- Save LabVIEW files from **My Computer**, not from under the real-time target (the save writes
  2025 format; the repos are in 2021 format). `HANDOFF.md`, section 8.
- A change in the class needs a new VI package build and install before a plugin test
  (`VIPM\CAN RW CANsub.vipb`, now at 0.1.0.4).
- Do not send REST requests with no pause to the device. Restart the device after a stop of its
  REST server. `HANDOFF.md`, section 7.

---

## Overview: type of work and time for each item

The times are estimates for one engineer, not measured values. No item has a detailed design
yet. "Create" is design and build. "Test" includes the bench run and the record of the result.
Times are in hours (h) or working days (d, 8 h). A change in the class adds about 1 h for a new
VI package build, install and plugin build; this is not in the numbers.

Types: **Test** = bench test or measurement, no code change. **LV** = LabVIEW change.
**C** = new C code in `libcansub_codec`. **Build** = C build and installers, no new C function.
**Python** = script. **None** = no engineering work.

| Item | Short name | Type | Create | Test | Total |
|---|---|---|---|---|---|
| 1.1 | Test sender counts FIFO time-outs | LV (small) | 0.5 h | 0.5 h | 1 h |
| 1.2 | 1 h run at a real 100 frames/s | Test | – | 1.5 h | 1.5 h |
| 1.3 | Depth of the writer FIFO | Test (read the source) | – | 0.5 h | 0.5 h |
| 1.4 | Memory test of the plugin loader | Test | – | 1 h | 1 h |
| 1.5 | Two slow frames in the reader path | LV (small) + Test | 1 h | 1 to 3 h | 2 to 4 h |
| 1.6 | Send channel counter 27 frames low | Test | – | with 1.2 | – |
| | **Section 1** | | | | **about 1 d** |
| 2.1 | Class test with a REST server that stops | LV (small) + Test | 1 h | 3 h | 4 h |
| 2.2 | Four plugins start at the same moment | Test | 1 h (cable, setup) | 3 h | 4 h |
| 2.3 | Report to CSS Electronics | None | 0.5 h | – | 0.5 h |
| 2.4 | Automatic reconnect | LV | 1.5 d (0.5 d design) | 1 d | 2.5 d |
| 2.5 | Bus Off test | Test | – | 4 h | 4 h |
| 2.6 | Device clock offset log | Python + Test | 1 h | 2 h | 3 h |
| | **Section 2** | | | | **about 4.5 d** |
| 3.1 | Packaging of the native library | Build + LV (small) | 1.5 d | 0.5 d | 2 d |
| 3.2 | Phase 2: reader and writer on one channel | LV; C possible (0 to 2 d) | 2.5 to 3.5 d (0.5 d design) | 1 d | 3.5 to 4.5 d |
| 3.3 | Remove the remaining XNET items | LV | 0.5 d | 0.5 d | 1 d |
| 3.4 | Clock pair, `Publish Clock` | LV | 1 d (0.5 d design) | 0.5 d | 1.5 d |
| 3.5 | VI package from `main` after the merge | None | – | – | – |
| | **Section 3** | | | | **8 to 9 d** |
| 4.1 | Writer transport time | Test | – | 0.5 h | 0.5 h |
| 4.2 | Neutral default interface address | LV (small) | 0.5 h | 0.5 h | 1 h |
| 4.3 | `Last Error` in the CAN status | C + LV | with 5.3 | with 5.3 | – |
| 4.4 | `One Shot`, `ACK Errors` | Test (investigation) | – | 2 h | 2 h |
| 4.5 | Socket options on Windows | LV | 0.5 d | 0.5 d (needs a Windows test) | 1 d |
| | **Section 4** | | | | **about 1.5 d** |
| 5.1 | Hardware filter lists | LV | 1 d | 0.5 d | 1.5 d |
| 5.2 | `phy` options and read-back | LV | 0.5 d | 0.5 d | 1 d |
| 5.3 | Error frame type | C (0.5 d) + LV (0.5 d) | 1 d | 0.5 d | 1.5 d |
| 5.4 | Device transmit sequences | LV | 1.5 d | 1 d | 2.5 d |
| 5.5 | Device info, time and PTP status | LV (small) | 0.5 d | 2 h | 0.75 d |
| 5.6 | Channel discovery | LV (small) | 2 h | 1 h | 3 h |
| 5.7 | REST certificate check and mTLS | LV + device setup | 0.5 d | 0.5 d | 1 d |
| 5.8 | Device reboot tool | Python | 0.5 h | 0.5 h | 1 h |
| 5.9 | TX back-pressure | Test (design later) | – | 0.5 d | 0.5 d |
| 5.10a | Single-channel monitor example | LV | 1 d | 0.5 d | 1.5 d |
| 5.10b | Signal decode from a DBC file | C (2 d) + Python (0.5 d) + LV (1 d) | 3.5 d | 1 d | 4.5 d |
| | **Section 5** | | | | **about 16 d** |
| | **All items** | | | | **about 31 to 32 d** |

**Split by type (all items):** C code about 2.5 d (5.3 and 5.10b), and 0 to 2 d more if Phase 2
uses a C buffer. C build and installers about 1.5 d. Tests with no code change about 3 d.
The remaining time, about 23 d, is LabVIEW work with its tests.

---

## 1. Close the limits of the 1 h bench test

### 1.1 Test sender: count the FIFO writes that time out (0.5 h)

- **Problem:** the 1 h run was at 64 frames/s, not 100. `sent` was 362,265; 231,289 frames
  reached the bus.
- **Known (read from the VIs):** `Latency Writer.vi` calls `Write CAN FIFO.vi` with
  `Timeout (ms)` not wired (0) and `timed out?` not wired. `sent` = loop count + 1. A write to
  a full FIFO is dropped with no error. The plugin monitor **FIFO Drops** stays 0.
- **Change:** in the loop of `Latency Writer.vi`, wire `timed out?`. Add 1 to `sent` only when
  it is False. Add 1 to a new indicator `dropped` when it is True. Keep both in the loop shift
  register.
- **Done when:** a run with `Offset (ns)` = 1E9 and `period (ms)` = 10 shows `dropped` > 0, and
  `sent` = reader `frames`.

### 1.2 Repeat the 1 h run at a real 100 frames/s (1.5 h)

- **Settings:** `period (ms)` = 10, `Offset (ns)` = 1E8 (a short run with these values gave
  100 frames/s on the bus), writer `Wake Offset (us)` = 100.
- **Steps:** `PLUGIN_TEST.md`, stage 3. Read the two device counters, the loader RSS and
  `GET /api/time/ptp` each 10 minutes; read the counters again after the stop.
- **Done when:** `sent` = reader `frames` = `frame_count` of channel 2; the latency table is in
  `PLUGIN_TEST.md`.
- **Compare with:** full loop median 640 us, 99.9 % 1,010 us, maximum 17,074 us (run at 64/s).

### 1.3 Confirm the depth of the writer FIFO (0.5 h)

- **Known:** 64 frames/s with a 1 s offset points to 64 elements. Not confirmed.
- **Start:** `Create Test Channels.vi` and `Link Test Channels.vi` in the writer repo
  (`Tests\Test Helpers\SubVIs\`), then the FIFO create VIs in the CIF core source.
- **Done when:** the depth and where it is set are in `PLUGIN_TEST.md`, with the rule
  "rate x offset must be below the depth".

### 1.4 Memory test of the plugin loader process (1 h)

- **Observed:** RSS of the loader: 212 MB with no plugin, 774 MB after the first load of the two
  plugins, 1,165 MB before the 1 h run. During the run: flat for 40 minutes, then one step of
  12 MB.
- **Steps:** restart the CIF Manager for a clean start. Then 5 times: load the two plugins, run
  1 minute, stop them, read `ps -o pid,rss,cmd -C lvrt`.
- **Done when:** a table of RSS against load count exists. If the RSS goes up each time, repeat
  with a stock CIF plugin: growth with a stock plugin = CIF loader; growth only with our plugins =
  our class (first suspects: the frame and pcap buffers in the class private data, about 166 kB
  for each object, and the TLS connection).

### 1.5 Cause of the two slow frames in the reader path (2 to 4 h)

- **Observed:** two frames in 231,289 took about 7 ms and 17 ms from the bus to the reader FIFO.
  The writer was on time for them. 99.9 % value: 920 us.
- **Not known:** the time of the two frames in the run. The helper keeps only histogram bins.
- **Start:** add to `Latency Reader.vi` a small log of the frames above 5 ms (frame index, `T0`,
  `t_bus`). Compare the times with the 1 s status call of `Update CAN Status` (a call on a closed
  HTTPS connection needs a new TLS handshake, about 80 ms, in the same loop) and with other load
  on the PXIe.
- **Done when:** the cause is named, or 3 runs of 1 h show no relation and the item is closed
  as "not reproduced".

### 1.6 Device counter of the send channel 27 frames low after the stop (with 1.2)

- **Observed:** after the stop, `frame_count` was 231,262 on channel 1 and 231,289 on channel 2.
  The six readings during the run were equal.
- **Probable:** the device updates the counter at an interval, and the close came first.
- **Done when:** seen again or not in run 1.2. If seen again, add it to the device rules in
  `HANDOFF.md`, section 7.

---

## 2. Robustness

### 2.1 Test of the complete class with a REST server that stops (0.5 d)

- **Known:** `Status Policy.vi` (class repo, `src\Utilities\`) passed its table test as a pure
  function. Errors 5303 (no status for 60 s) and 5304 (device reports the channel as stopped)
  were not seen on the bench.
- **Idea for the test:** block only the REST path while the WebSocket stays open. Both use
  port 443 on the device, so a firewall rule cannot split them. Options: (a) a test input in
  `Update CAN Status.vi` that forces the call error; (b) stop the device REST server on purpose
  with fast calls from a REST test script (the fault that we want to survive; restart the
  device afterwards).
- **Done when:** frames continue while the status call fails, the CAN status keeps its last
  value, and error 5303 comes after 60 s.

### 2.2 Start of four plugins at the same moment (0.5 d)

- **Known:** one channel open makes about 9 REST calls with a 20 ms pause after each
  (`REST_Call_Paced.vi`, used in `CANsub_REST_Setup.vi`). Two plugins at the same time gave no
  fault. Four were not tested; the bench has two channels in loopback.
- **Needs:** channels 3 and 4 connected (a second loopback cable), 2 readers and 2 writers.
- **Done when:** 10 starts of four plugins give no REST fault and no entry in `cif_error.log`.

### 2.3 Send the report about the REST server fault to CSS Electronics (0.5 h)

- **Known:** REST calls with no pause on one HTTPS connection stopped the HTTP server after
  185 calls; ping stayed good; open channels were closed. Details: `HANDOFF.md`, section 7.
- **Done when:** sent; the answer is recorded in `HANDOFF.md`.

### 2.4 Automatic reconnect after a lost connection (1 to 2 d, design first)

- **Today:** a lost connection gives error 5203 (found in 3 s to 5.5 s by TCP keepalive); the
  CIF core then takes the plugin out of Execute. A person must stop and load the plugin again.
- **Decision needed:** reconnect in the class (`Read`/`Write` open the channel again) or in the
  plugin (state machine). `ARCHITECTURE.md`, section 2.2, has the planned API method `Reopen`.

### 2.5 Bus Off test of `Update CAN Status` (0.5 d)

- **Known:** the mapping `bus_off` to Bus Off is in `Convert Device Status.vi` and is checked by
  review only. Error Active and Error Passive are tested.
- **Steps:** `API_BUILD.md`, section 6.3 (connect CAN-H to CAN-L while a frame is sent; check the
  connector pins first). Same as item B14 there.

### 2.6 Device clock offset (0.5 d)

- **Observed:** `GET /api/time/ptp` gave `offset_ns` between 292 and 1,204 in four readings, and
  -3,235, -9,530 and 92,689 in three. The PXIe was within about 0.3 us of the same master.
- **Effect:** the offset moves time between the reader path and the writer path values. The full
  loop value is not affected.
- **Steps:** log `offset_ns` each 10 s for 1 h (one call each 10 s is safe for the device).
- **Done when:** the spread is known and stated with the latency numbers; add to report 2.3 if
  the spikes are real.

---

## 3. Next tasks

### 3.1 Packaging of the native library (2 d)

- **Today:** `libcansub_codec.so` 1.3.0 is built and copied by hand to `/usr/local/lib` on the
  PXIe; the device root certificate is at `/home/lvuser/cansub_root_cert.crt`. Build steps:
  `c_functions\README.md`.
- **Open decision:** the name and path. The repo skeleton expects `libcansub_lv_wrapper.so` in
  `/usr/lib/cif/`. Agree with the owner of the skeleton.
- **Work:** replace the placeholder C++ wrapper in `src\Wrapper` with the C source and its CMake
  build; installers `.ipk` (NI Linux RT), NSIS (Windows), `.deb` (Ubuntu); dependency in the VI
  package. `CIF_BUILD.md`, step R10.

### 3.2 Phase 2: reader and writer on the same CANsub channel (3 to 5 d)

- **Why:** the device permits one WebSocket for each channel, so today one plugin uses one
  channel.
- **Known since 2026-11-23:** the two plugins run in one process (the default plugin loader of
  the CIF Manager). So a shared connection inside the class is possible (one owner of the
  WebSocket, hand-over of frames to the second user).
- **Start:** `ARCHITECTURE.md`, section 7 ("Shared ownership"), and `CIF_BUILD.md`.
- **To define:** the behaviour when the owner stops first.

### 3.3 Remove the remaining XNET items from the plugins (1 d)

- Disabled code in `gRPC Server` (both plugins) and in `Update External Clock` (reader).
- The `XNET Devices` element in the plugin data.
- Test helpers `XNET CAN Write.vi` (reader) and `Read CAN.vi` (writer).
- `Latency Writer.vi` and `Channel Read Write.vi` use the XNET frame typedef and
  `Flatten XNET CAN to pcap CAN.vi`. Replace with the CANsub frame VIs of the class repo.
- **Done when:** the plugins and their tests load with no `CIF XNET CAN RW` package installed.

### 3.4 Clock pair for the CIF external clock, `Publish Clock` (design first)

- **Today:** the True case of `Update External Clock.vi` is empty; `Publish Clock` = True is
  ignored with no message.
- **Options:** `API_BUILD.md`, item B16 (pair from the last received frame, or system time two
  times when the target is PTP-synchronised to the device).

### 3.5 After the merge of the pull requests

- Build and publish the VI package from `main`, then set the package version in the plugin
  requirements.

---

## 4. Later or optional

| # | Item | Detail |
|---|---|---|
| 4.1 | Writer transport time | A run with writer `Wake Offset (us)` = 0. Then "bus time - planned time" is the real time from the wake to the bus. Today (offset 100) the value is the error against the plan: mean -0.8 us. |
| 4.2 | Neutral default interface address | The plugin controls have the defaults `192.168.0.49/1` (writer) and `/2` (reader). The repos are public. Change the control defaults in the two configuration typedefs. |
| 4.3 | `Last Error` in the CIF `CAN Status` | Always None today. The device gives the error type only in error frames on the WebSocket (flag bits 8 to 12). `Read` must keep the type of the last error frame. `API_BUILD.md`, item B13. |
| 4.4 | `One Shot`, `ACK Errors` | Writer plugin: `One Shot` has no effect (the device has no such option found). The statistic `ACK Errors` is not confirmed by a test. |
| 4.5 | Socket options on Windows | `CANsub_TLS_Connect.vi` and `CANsub_Set_Keepalive.vi` set TCP_NODELAY and keepalive only on Linux. On Windows: more latency is possible and a lost connection is found later. Add `setsockopt` from `ws2_32.dll`. `API_BUILD.md`, item B15. |
| 4.7 | Device functions that the client does not use | See section 5: each row with "not used" is a possible feature. |

---

## 5. Device API calls: used, planned, not planned

Source of the call list: the device documents (REST and WebSocket API, API version 05.00) and
the notes in `COMMON_BUILD.md`. "B" numbers are the items in `API_BUILD.md`, section 6.2.

| Device call | Purpose | State in the client | Backlog |
|---|---|---|---|
| `GET /api/version` | API version | **used** at the channel open | – |
| `GET /api/can/{ch}` | channel state, error counters, frame count, frame rate, bus load | **used**: busy check at the open; CAN status once each second | Bus Off test: 2.5 |
| `GET /api/can/{ch}/filter` | list of filter IDs | **used** at the open | – |
| `DELETE /api/can/{ch}/filter/{id}` | remove a filter | **used** at the open (removes old filters) | – |
| `POST /api/can/{ch}/filter` | add a filter | **used**: two accept-all filters (standard and extended) | 5.1 |
| `PUT /api/can/{ch}/phy` | bit timing and options (channel stopped) | **used**: bit rates and sample points; `auto_reset`, `error_frames`, `tx_ack_frames` are fixed to true; `listen_only` false | 5.2 |
| `GET /api/can/{ch}/ws` (WebSocket upgrade) | frame stream, both directions | **used** | – |
| `DELETE /api/can/{ch}/ws` | free the channel | **used** at the close and as a fallback | – |
| WebSocket: data frames, transmit acknowledgements | – | **used** | – |
| WebSocket: error frames with the error type | bit, ack, form, stuff, CRC | received, the type is **not reported** | 5.3 |
| `GET /api/can/{ch}/phy` | read the settings back | not used | 5.2 |
| `GET /api/can` | list of channels | not used | 5.6 |
| `POST /api/can/{ch}/transmit`, `GET .../transmit`, `DELETE .../transmit/{id}`, `GET/PUT .../transmit/{id}/count` | periodic transmit sequences in the device | not used | 5.4 |
| `GET /api/info` | device ID, hardware and firmware version, MAC | not used (not tested on firmware 02.05.00) | 5.5 |
| `GET /api/time/ptp` | PTP master, state, offset | not used by the client; used by hand with `curl` | 5.5 |
| `GET /api/time` | device time | not used | 5.5 |
| `PUT /api/time` | set the device time | **never**: it stops PTP until a power cycle | – |
| `GET/PUT/DELETE /api/client-cert` | client certificate (mTLS) | not used | 5.7 |
| `POST /api/reboot` | restart the device | not used (not tested on firmware 02.05.00) | 5.8 |

Not a device call, but related: TX back-pressure (5.9), verification of the server certificate
for REST (5.7), signal decode from a CAN database (5.10).

### Items for the calls that are not used

| # | Item | Detail | Priority |
|---|---|---|---|
| 5.1 | Hardware filter lists | The device has mask filters (`(id & f2) == (f1 & f2)`) and range filters, standard and extended. The filter table size is not documented (503 when full). The API config has the field, `Init` returns error 5207 for it. The CIF class filters in software in the C library today. | low |
| 5.2 | `phy` options in the config and read-back | Make `auto_reset`, `error_frames`, `tx_ack_frames` and `listen_only` config items (B3). To see Bus Off for a longer time, `auto_reset` must be false. No transmit acknowledgements halves the WebSocket traffic of a writer. `listen_only` must be false when the device is the only other node. Read back with `GET .../phy` to check what the device accepted. | medium |
| 5.3 | Error frame type | Report the type in the API frame (B2) and as `Last Error` in the CIF status (B13, item 4.3). Needed for fault-injection tests. | medium |
| 5.4 | Device transmit sequences | Periodic frames sent by the device itself (rest-bus), timing within about 3 us. Limits: 64 sequences for each channel, 32 frames each, 1 ms resolution. A sequence stops when the WebSocket closes. Planned in `ARCHITECTURE.md`, section 2.2. Useful for a writer with cyclic frames: no network jitter. | medium |
| 5.5 | Device info, time and PTP status | `GET /api/info` at the open: record the firmware in reports and refuse an unsupported one (B6). `GET /api/time/ptp` as a setup-only method: tells if the frame times can be trusted (B5); see item 2.6. | medium |
| 5.6 | Channel discovery | `GET /api/can` gives `[1,2,3,4]`. Convenience for tools and for the plugin `GetDevices` call, which returns an empty list today (B9). | low |
| 5.7 | REST certificate check and mTLS | REST calls run with "verify server" off, because the certificate is for the device name and we connect by IP address. For production: connect by name (B7). Client certificate only if the device is set to require one; it is erased by a firmware update (B8). | low |
| 5.8 | Device reboot | Tool only, not in the API: a reboot resets the clock and the PTP state (B10). Possible recovery for a stopped REST server; test it first. | tool |
| 5.9 | TX back-pressure | What happens when the client writes faster than the bus can send (Bus Off, 100 % load) is not known. Measure first, then design (B4). | medium |
| 5.10 | Examples and signal layer | Single-channel monitor example (B11). Signal decode and encode from a DBC file for standalone use, not for the CIF plugins (B12, with three options). | low |

**Answer to "are all device API calls in the backlog?"** Yes, with this section. Before
2026-11-24 three groups were only in the "deferred" table of `API_BUILD.md`, section 6.1, and
not in a numbered item: hardware filter lists, listen only, and transmit sequences. The
read-back `GET .../phy` was in no list.
