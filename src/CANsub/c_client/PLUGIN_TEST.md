# Plugin tests on the PXIe (R7, R8, R9)

**Status 2026-11-20:** stage 1 (reader plugin alone) passed with package 0.1.0.2. Stage 2 (writer
and reader together) passed on 2026-11-20 after a restart of the device.

Repos: `C:\Dev\CIF\Plugin_CAN_Reader_CANsub`, `C:\Dev\CIF\Plugin_CAN_Writer_CANsub`, branch
`feature/cansub-rw-class` in each. Bench: CANsub.4 at `192.168.0.49`, CH1 and CH2 connected with
a loopback cable.

## Terms

- **Launch.vi** (`Callable Library\Core\`): starts the plugin and its gRPC server. It runs until
  the plugin is unloaded. It shows `gRPC Server Port` only after it stops.
- **Channel Read Write.vi** (`Tests\Test Helpers\`): the test client of a plugin. It connects to
  the plugin, links test channels, starts the plugin, and exchanges data with the CAN FIFO of the
  plugin. The reader version shows the received frames. The writer version sends the frames of
  its `CAN Data` control every 100 ms.
- **Sender**: the script `cansub_send.py` (python-can with the `cansub` interface), run on the
  PXIe in the Python environment `/home/admin/cansub-venv`. The PC has no environment with
  python-can-cansub. Frame k has the ID `0x100 + k mod 4`, and the data is k as 8 bytes,
  big-endian.

## Bench project (used by all stages)

`Plugin_CAN_Reader_CANsub\Tests\CANsub Plugins Bench Test.lvproj` holds the reader and the writer
plugin under one real-time target, so that one project deploys all test VIs. The two plugin repos
must be next to each other in one folder. Make it one time:

1. LabVIEW 2025: **File** > **Create Project...** > **Blank Project**. Save it at the path above.
2. **Project** > **Properties** > **Save version** = `21.0`.
3. Right-click the project root > **New** > **Targets and Devices...** > add the PXIe.
4. Right-click the target > **Add** > **File...** and add these 14 files:

| Reader repo | Writer repo |
|---|---|
| `Callable Library\CAN_Reader_CANsub_Plugin.lvlib` | `Callable Library\CAN_Writer_CANsub_Plugin.lvlib` |
| `Class\CAN_Reader_CANsub_Plugin.lvclass` | `Class\CAN_Writer_CANsub_Plugin.lvclass` |
| `Common\CAN_Reader_CANsub_Common.lvlib` | `Common\CAN_Writer_CANsub_Common.lvlib` |
| `Grpc\CAN_Reader_CANsub_wrapper\CAN_Reader_CANsub_Wrapper.lvlib` | `Grpc\CAN_Writer_CANsub_wrapper\CAN_Writer_CANsub_Wrapper.lvlib` |
| `Grpc\CanReaderCansub_server\CanReaderCansub_server.lvlib` | `Grpc\CanWriterCansub_server\CanWriterCansub_server.lvlib` |
| `Grpc\CanReaderCansub_client\CanReaderCansub_client.lvlib` | `Grpc\CanWriterCansub_client\CanWriterCansub_client.lvlib` |
| `Tests\Test Helpers\CAN_Reader_CANsub_Test_Helpers.lvlib` | `Tests\Test Helpers\CAN_Writer_CANsub_Test_Helpers.lvlib` |

5. Do not add the two UI libraries. No item may show a conflict mark.
6. Right-click the target > **Connect**. Open the two `Launch.vi` and the two
   `Channel Read Write.vi` under the target: all four Run arrows must be good. If one is broken,
   record the Error List text and stop.
7. **File** > **Save Project** (the project file only).

Rules: do not save plugin VIs or classes from this project (items under a real-time target are
saved in the editor format, not in 2021 format; at close use **Don't Save - All**). Edit the
plugins only in their own test projects, under My Computer. Close the other plugin projects
before you connect this one to the target.

## Stage 1: reader plugin alone

The sender owns CH1. The reader plugin owns CH2. Use the reader `Launch.vi` and the reader
`Channel Read Write.vi` of the bench project. (The first pass of this stage, 2026-11-20, used the
reader items under the RT target of `CAN_Reader_CANsub Interactive Test.lvproj`.)

### Part 1. Before each test session

1. Stop all other VIs on the PXIe that use the CANsub (one client for each channel).
2. Make sure that `ptp4l` and `phc2sys` run on the PXIe.
3. On the PC: `curl -k https://192.168.0.49/api/can/2` must show `"state":"stopped"`.

### Part 2. Runs

**Run 1: start with no traffic**

1. In `Launch.vi`, set `Plugin Name` to `CAN_Reader_CANsub`. Run it.
2. In `Channel Read Write.vi`, write down the default of `plugin gRPC address` and keep it. Run.
3. Wait 5 s. Then do the `curl` command again.

| Item | Expected |
|---|---|
| The two VIs | continue to run, no error dialog |
| `Node Status` | the state shows that the plugin runs |
| `curl` result | `"state":"error_active"` (the plugin opened CH2) |
| `XNET CAN Data` | empty |

**Run 2: 50 frames** (the two VIs still run)

First, on the PXIe, with the plugin running: `curl -m 5 -sk https://192.168.0.49/api/version`
must answer `"05.00"`. This proves that the plugin does not block the REST server of the device
(it did before the status rate limit of package 0.1.0.2).

One time, from the PC (copy the script to the PXIe):

```
scp cansub_send.py admin@<rt-ip>:/home/admin/
```

Then in an SSH session on the PXIe:

```
source /home/admin/cansub-venv/bin/activate
python /home/admin/cansub_send.py --channel 192.168.0.49@1 --count 50 --period-ms 100
```

| Item | Expected |
|---|---|
| Script output | `sent 50 frames, IDs 0x100 to 0x103` |
| `XNET CAN Data` | changes during the 5 s; IDs 0x100 to 0x103, 8 data bytes, the last byte counts up |
| `Header` | changes with each packet |
| `error out` | no error |

**Run 3: stop**

1. In `Channel Read Write.vi`, set **Unload**.
2. Do the `curl` command again.

| Item | Expected |
|---|---|
| `Channel Read Write.vi` | stops, `error out` has no error |
| `Launch.vi` | stops, no error, `gRPC Server Port` shows a number. If it continues after 10 s, abort it |
| `curl` result | `"state":"stopped"` (the plugin closed CH2) |

### If a run fails

| Symptom | Do |
|---|---|
| Deploy error or broken Run arrow | Record the Error List text. Do not change code |
| `Channel Read Write.vi` stops at once with an error | Record the error code and text. The usual cause is the gRPC address |
| Error 5301 | The plugin got an `RX Interface` that is not `<host>/<channel>`. A configuration file on the PXIe has a different value than the default `192.168.0.49/2` |
| Error 5004 or a TLS error | The certificate `/home/lvuser/cansub_root_cert.crt` or the network path to the device |
| Run 2: no frames, no error | Do the `curl` command for channel 1 and channel 2 and record the two results |

### Record after the test

1. The results of runs 1 to 3, and screenshots of the two front panels after run 2.
2. The default of `plugin gRPC address` and the `gRPC Server Port` value.
3. The value of `x-y` (no prediction).
4. Each dialog that LabVIEW showed at the deploy (for example a missing component on the target).

### Not known before the test

- The default of `plugin gRPC address` (not read from the VI file).
- Where the plugin gets its configuration at start. If a configuration file for the plugin name
  is on the PXIe, its `RX Interface` is used and not the default.
- If the CIF and NI-XNET components that the two VIs need are on the PXIe.
  `Channel Read Write.vi` uses an XNET conversion VI for the display.

### Result of stage 1 (2026-11-20, package 0.1.0.2): PASS

| Run | Result |
|---|---|
| 1 | The two VIs ran with no error. `curl` for channel 2 showed `"state":"error_active"` while the plugin ran (so the REST server of the device answered). `XNET CAN Data` empty, `Node Status` updated |
| 2 | 50 frames sent, 50 received: `Ctr` = 50, monitor values Iterations 50 and Read Frames 50. Last frame ID 0x101, data `00 00 00 00 00 00 00 31` (frame 49, correct). One frame in each packet |
| 3 | Stopped with no error; channel 2 showed `"state":"stopped"` |

Facts found: `Launch.vi` shows `gRPC Server Port` while it runs, and the port is assigned at the
start (45669 in this run; the default of `plugin gRPC address` was `localhost:51930` and had to be
changed). The packet time in the raw bytes was 1795184259.494787 s (TAI); the display showed
08:17:02.4947869 local time, which is the same instant in UTC minus 37 s. Period average 94.7 ms,
Execute maximum 101 ms (the 100 ms read timeout). `x-y` showed -1.7E+18 (not a useful value with
this class).

## Stage 2: writer and reader together

The writer test helper sends the frames of its `CAN Data` control to the writer plugin every
100 ms. The writer plugin sends them on CH1 (`TX Interface` = `192.168.0.49/1`). The reader
plugin receives them on CH2 and its test helper shows them. All four VIs run from the bench
project.

### Part 1. Before the session

`curl -m 5 -sk https://192.168.0.49/api/can/1` and `.../can/2` must show `"state":"stopped"`.
No other client on CH1 or CH2 (no sender script).

### Part 2. Runs

**Run 1: start the reader, then the writer**

1. Reader `Launch.vi` (`Plugin Name` = `CAN_Reader_CANsub`): run, read `gRPC Server Port`.
2. Reader `Channel Read Write.vi`: set `plugin gRPC address` to `localhost:<reader port>`, run.
3. Writer `Launch.vi` (`Plugin Name` = `CAN_Writer_CANsub`): run, read its `gRPC Server Port`.
4. Writer `Channel Read Write.vi`: set `plugin gRPC address` to `localhost:<writer port>`. In
   `CAN Data`, make one element: `identifier` = 0x200, `extended?` off, `type` = CAN Data,
   `payload` = `01 02 03 04 05 06 07 08`. Keep `Offset (ns)` at its default. Run.
5. Wait 10 s. Then on the PXIe: `curl -m 5 -sk https://192.168.0.49/api/can/1` and `.../can/2`.

| Item | Expected |
|---|---|
| The four VIs | run, no error |
| Reader `XNET CAN Data` | ID 0x200, payload `01 ... 08`, a new packet about 10 times in each second |
| Reader `Ctr` | counts up by about 10 in each second |
| `curl` channel 1 | `"state":"error_active"`, `"tx_error_count":0`, `frame_count` goes up |
| `curl` channel 2 | `"state":"error_active"`, `"rx_error_count":0` |

**Run 2: change the data while it runs**

In the writer helper, change `payload` byte 1 to `AA`. Expected: the reader shows `AA` in less
than 1 s.

**Run 3: stop**

Set **Unload** in the writer helper, then in the reader helper. Expected: all four VIs stop with
no error; `curl` shows `"state":"stopped"` for the two channels.

### Record after the test

The results of the three runs, a screenshot of each helper during run 1, the writer monitor
values (Iterations, Written Frames, FIFO Read Timeouts, FIFO Drops, ACK Errors), and the reader
monitor values.

### If a run fails

| Symptom | Do |
|---|---|
| The writer helper stops with error 5302 | The pcap bytes from the writer helper are not in the format that the class expects. Record the error text |
| Reader shows nothing, channel 1 `tx_error_count` goes up | No acknowledgement on the bus: check the cable CH1 to CH2 and that the reader runs |
| A deploy conflict dialog | Record the text. Do not select an option that removes running VIs without reading it |
| `curl` times out while the two plugins run | Stop the two plugins and record it: two plugins make two REST status calls in each second |

### Result of stage 2 (2026-11-20, package 0.1.0.2, device restarted before the test): PASS

| Test | Result |
|---|---|
| 10 minutes, writer on CH1 and reader on CH2 | reader `Ctr` 6333 at the stop; REST answered each 2 minutes (5 of 5, 73 ms to 79 ms) |
| 5 start and stop cycles, reader unloaded first | no failure |
| 5 start and stop cycles, writer unloaded first | no failure |
| second 10 minutes | reader `Ctr` 6055 at the stop; REST 5 of 5 (74 ms to 82 ms) |
| 10 minutes with package 0.1.0.3 (status failure policy, pause in the channel open) | no failure, about 6,000 frames; REST answered during the run; clean stop |

Before the restart of the device, the same test failed at random times (4 of 12 cycles, then 1 of
10): the REST server of the device stopped for 20 s to 30 s and the device closed the open
channels. Two explanations were tested with `cansub_rest_overlap_test.py` (CANSub repo,
`hil_test\`) and are **not** the cause: two REST requests at the same moment (600 pairs, no
fault), and the number of requests or of new TLS connections after a restart (9000 requests and
3000 connections, no fault). The failures started after two tests that sent REST calls with no
pause (the first plugin run with no rate limit, and a polling test), and they ended with the
restart. Probable, not proven: after such an overload the REST server stays in a weak state until
the device is restarted. Rule: after each stop of the REST server, restart the device
(`POST /api/reboot` or power cycle) before the next test, and check PTP (`GET /api/time/ptp`,
`"state":"tracking"`).

Phase 1 limits of the plugins: `Publish Clock` is ignored (backlog B16), `One Shot` has no
effect, `GetDevices` returns an empty list, `SetInterfaceBlink` returns an error status.

## Stage 3 (R9): bench test with the CIF Manager

**Status 2026-11-20: preparation.** Decisions: the CIF Manager loads the reader and the writer
plugin; the test helpers feed the writer and read the reader (no third plugin). Latency to
measure: the reader path, the writer path, and the full loop. Run time 1 h.

### Facts about the CIF Manager on the target (from the `CIF_Manager` repo)

| Item | Detail |
|---|---|
| Install package | `cif-lvcore` (`.ipk` for NI Linux RT) |
| Manager program | `/usr/local/cif/manager/cif_manager_startup.rtexe`, a link to the version for the LabVIEW runtime on the target (21, 23 or 25) |
| How it runs | a second LabVIEW runtime process (`lvrt`), started with `launch_lvrt_cif.sh` or `launch_rt.py` |
| Network port | gRPC on TCP port 15882 |
| Settings file | `/home/lvuser/CIF/cif.conf` |
| Logs | `/var/local/cif/logs/` (`cif_info.log`, `cif_error.log`, `cif_config.log`), time in UTC |

### Step 1. Is the CIF Manager installed and running? (SSH session on the target)

```
opkg list-installed | grep -i cif
ls -l /usr/local/cif/manager/
ls -l /usr/local/cif/manager/loaders/
opkg list-installed | grep labview-realtime
ps aux | grep -E "lvrt|cif" | grep -v grep
netstat -ltnp 2>/dev/null | grep 15882
tail -n 20 /var/local/cif/logs/cif_info.log
ls /usr/local/cif/
ls /home/lvuser/CIF/ 2>/dev/null
```

From the PC: `Test-NetConnection <rt-ip> -Port 15882`.

| Result | Meaning |
|---|---|
| a line with `cif-lvcore` | the package is installed |
| `cif_manager_startup.rtexe -> ..._25.rtexe` | the link agrees with a LabVIEW 2025 runtime |
| a `LISTEN` line for `:15882`, two `lvrt` processes | the manager runs |
| no line for 15882 | the manager does not run |

### How the latency is measured (plan)

- Reader path: the time of the frame on the bus (in the packet, device clock) against the time
  when the packet is in the CIF FIFO (target clock, PTP).
- Writer path: the time when the helper puts the packet into the FIFO against the time of the
  frame on the bus (transmit acknowledgement of CH1, or the receive time on CH2).
- Full loop: writer FIFO in to reader FIFO out, both on the target clock.
- Check `GET /api/time/ptp` (`"state":"tracking"`) before each run; the first two values depend
  on the PTP offset between the target and the device.

### Open before the steps can be written

- The output of step 1.
- How a plugin must be built and where its files must be on the target (plugin build projects,
  `Build Library\`; the plugin build projects have no save version setting, and the plugins
  depend on the installed `CIF CAN RW CANsub` package).
- Does the manager run each plugin in its own process? (also the open Phase 2 question; it
  decides how the plugins find `libcansub_codec.so` and the certificate)
