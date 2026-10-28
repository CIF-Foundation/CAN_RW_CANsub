# CANsub LabVIEW wrapper

This CMake project builds the native library LabVIEW loads for CCS CANsub devices (`cansub_lv_wrapper`).

Run CMake from this directory (`src/Wrapper`).

## Prerequisites

### Windows (64-bit DLL)

- Visual Studio 2022 with the **Desktop development with C++** workload.
- CMake 3.16 or later on `PATH`.
- Open **x64 Native Tools Command Prompt for VS 2022**.

### Linux (shared object)

```bash
sudo apt update
sudo apt install build-essential cmake
```

## Build (Windows)

```bat
cd /d <repo>\src\Wrapper
cmake -S . -B build -A x64
cmake --build build --config Release
```

Output:

```text
build\bin\cansub_lv_wrapper.dll
```

Copy `cansub_lv_wrapper.dll` to `Installers\Windows\resource\` before running `Installers\Windows\build_nsis.bat`. The DLL is gitignored.

A debug build (`cmake --build build --config Debug`) writes to the same `build\bin\` folder and should not be installed on a target.

## Build (Linux / WSL)

```bash
cd src/Wrapper
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

Output:

```text
build/bin/libcansub_lv_wrapper.so
```

That `.so` is for the OS and architecture you built on (typically `x86_64` glibc Linux). Use it for the Ubuntu `.deb`. Copy it to:

```text
Installers/lv_cansub_deb/usr/lib/cif/libcansub_lv_wrapper.so
```

then run `bash Installers/build_deb.sh`. See [`Installers/readme.md`](../../Installers/readme.md).

### LabVIEW RT (IPK)

The RT package architecture is `core2-64`. The `.so` inside the IPK must be built for NI Linux RT.

Place `libcansub_lv_wrapper.so` at:

```text
Installers/lv_cansub_ipk/usr/lib/cif/libcansub_lv_wrapper.so
```

or point the IPK build at a CMake output:

```bash
SO_SOURCE=src/Wrapper/build/bin/libcansub_lv_wrapper.so bash Installers/build_lv_cansub_ipk.sh
```

From Windows, `Installers\build_lv_cansub_ipk.bat` runs that script through WSL. A desktop Linux `.so` will not load on RT unless you have confirmed the ABIs match.

## Verify

Windows:

```bat
build\bin\cansub_lv_smoke.exe
```

Linux:

```bash
./build/bin/cansub_lv_smoke
file build/bin/libcansub_lv_wrapper.so
```

`cansub_lv_smoke` prints `cansub_lv_wrapper 1.0.0.0`. `file` should report a 64-bit shared object.
