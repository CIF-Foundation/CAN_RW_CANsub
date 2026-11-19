# Plugin tests on the PXIe (R7, R8, R9)

**Status 2026-11-18:** the reader plugin (R7) and the writer plugin (R8) are built and reviewed.
No test is done yet. Stage 1 is written in full. Stage 2 is an outline; the full steps depend on
two results of stage 1 (see "Stage 2").

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

## Stage 1: reader plugin alone

The sender owns CH1. The reader plugin owns CH2.

### Part 1. Project setup (one time)

1. Open `Tests\CAN_Reader_CANsub Interactive Test.lvproj`.
2. Right-click **RT PXI Target** > **Properties** > **General**. Set the IP address of the PXIe.
3. Right-click **RT PXI Target** > **Add** > **File...**. Add these files:

| File | Folder in the reader repo |
|---|---|
| `CAN_Reader_CANsub_Plugin.lvlib` | `Callable Library\` |
| `CAN_Reader_CANsub_Plugin.lvclass` | `Class\` |
| `CAN_Reader_CANsub_Common.lvlib` | `Common\` |
| `CAN_Reader_CANsub_Wrapper.lvlib` | `Grpc\CAN_Reader_CANsub_wrapper\` |
| `CanReaderCansub_server.lvlib` | `Grpc\CanReaderCansub_server\` |
| `CanReaderCansub_client.lvlib` | `Grpc\CanReaderCansub_client\` |
| `CAN_Reader_CANsub_Test_Helpers.lvlib` | `Tests\Test Helpers\` |

4. Do not add the UI library.
5. Right-click the target > **Connect**.
6. Under the RT target, open `Launch.vi` and `Channel Read Write.vi`. The two Run arrows must be
   good. If one is broken, record the Error List text and stop.
7. Save the project file only (**File** > **Save Project**).

### Part 2. Before each test session

1. Stop all other VIs on the PXIe that use the CANsub (one client for each channel).
2. Make sure that `ptp4l` and `phc2sys` run on the PXIe.
3. On the PC: `curl -k https://192.168.0.49/api/can/2` must show `"state":"stopped"`.

### Part 3. Runs

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

## Stage 2: writer and reader together (outline)

Do it after stage 1 passes. The writer test helper sends the frames of its `CAN Data` control to
the writer plugin every 100 ms. The writer plugin sends them on CH1 (`TX Interface` =
`192.168.0.49/1`). The reader plugin receives them on CH2 and its test helper shows them.

The full steps need two results of stage 1:

- How the gRPC port of a plugin is assigned. Two plugins on one target need two ports.
- If a second project can deploy to the PXIe while the first one runs. If not, the writer items
  go into the reader test project (under the RT target).

Expected result (to confirm in the steps): the reader shows the ID and the data of `CAN Data`,
about 10 packets in each second; `curl -k https://192.168.0.49/api/can/1` shows
`"state":"error_active"` and `"tx_error_count":0`; after **Unload** of the two helpers, the two
channels show `"state":"stopped"`.

Phase 1 limits of the plugins: `Publish Clock` is ignored (backlog B16), `One Shot` has no
effect, `GetDevices` returns an empty list, `SetInterfaceBlink` returns an error status.

## Stage 3 (R9): bench test with the CIF Manager

Not written yet: both plugins loaded by the CIF Manager, 1 h run, latency numbers.
