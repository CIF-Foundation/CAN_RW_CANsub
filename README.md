# THIS IS WORK IN PROGRESS

# CAN_RW_CANsub

LabVIEW class that reads and writes CAN frames with CCS CANsub devices. `CAN RW CANsub.lvclass` inherits from [CAN_RW_Core](https://github.com/CIF-Foundation/CAN_RW_Core), in parallel with [CAN_RW_XNET](https://github.com/CIF-Foundation/CAN_RW_XNET).

The repository also contains the C++ library LabVIEW calls (`cansub_lv_wrapper`) and native installers for Windows, Linux, and Linux RT.

## Installation

Install [CIF CAN RW Core](https://github.com/CIF-Foundation/CAN_RW_Core) first. This package requires `CIF_Foundation_lib_CIF_CAN_RW_Core` 1.4.0.3 or later.

Build or install the LabVIEW package using the VIPM project in [`VIPM/CAN RW CANsub.vipb`](VIPM/CAN%20RW%20CANsub.vipb).

Package-specific documentation is in [`VIPM/README.md`](VIPM/README.md).

Windows requires `cansub_lv_wrapper.dll` to be installed. Linux and Linux RT require `libcansub_lv_wrapper.so` on the target. Native library installers are under [`Installers/`](Installers/readme.md). Build the library from [`src/Wrapper/`](src/Wrapper/readme.md) before packaging those installers.

## Repository Layout

| Path | Description |
| --- | --- |
| `src/` | LabVIEW source for `CAN RW CANsub.lvclass`, including core, accessors, utilities, typedefs, and tests |
| `src/Wrapper/` | C++ shared library loaded by LabVIEW |
| `src/Libraries/` | Built native libraries for local LabVIEW use (Windows, Linux, Linux RT) |
| `VIPM/` | VI Package Manager build specification and package readme |
| `Installers/` | Windows (NSIS), Ubuntu (deb), and Linux RT (ipk) installers for the native library |
| `LICENSE` | Apache License 2.0 |

## License

Copyright 2025-2026 CIF Foundation

Licensed under the Apache License, Version 2.0 (the "License"); you may not use this project except in compliance with the License. You may obtain a copy of the License at:

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.

The full license text is in [`LICENSE`](LICENSE).
