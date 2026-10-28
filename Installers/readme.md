Instructions to build installers for RT (ipk), Windows (NSIS), and Ubuntu (deb).

## lv_cansub_ipk (LabVIEW RT)

1. Update the version in `Installers/lv_cansub_ipk/CONTROL/control` when needed.
2. Place `libcansub_lv_wrapper.so` in `Installers/lv_cansub_ipk/usr/lib/cif/` before building (the `.so` is gitignored). Alternatively, build from C++ and set `SO_SOURCE`:

   `cd src/Wrapper && cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build`

   `SO_SOURCE=src/Wrapper/build/bin/libcansub_lv_wrapper.so bash Installers/build_lv_cansub_ipk.sh`

3. The build host needs `binutils`, `tar`, `gzip`, and `python3` (for example `sudo apt install binutils tar gzip python3`). The build uses the bundled `Installers/opkg-utils/opkg-build` script. Ubuntu/WSL does not provide an `opkg-utils` apt package.
4. From Windows, run:

   `Installers\build_lv_cansub_ipk.bat`

   Or from WSL/Linux:

   `bash Installers/build_lv_cansub_ipk.sh`

The build stages `Installers/lv_cansub_ipk/`, normalizes text files to LF line endings, packages with `opkg-build`, and writes `lv-cansub_<version>_<arch>.ipk` to `Installers/output/`.

`postinst` installs the library under `/usr/lib/cif/` and creates a symlink at `/usr/lib/libcansub_lv_wrapper.so`.

## lv_cansub Windows (NSIS)

1. Update the version in `Installers/Windows/lv-cansub.nsi` when needed.
2. Copy `src/Wrapper/build/bin/cansub_lv_wrapper.dll` to `Installers/Windows/resource/`.
3. Run:

   `Installers\Windows\build_nsis.bat`

## lv_cansub_deb (Ubuntu)

Update the version in `Installers/lv_cansub_deb/DEBIAN/control`.

Copy `libcansub_lv_wrapper.so` to `Installers/lv_cansub_deb/usr/lib/cif/`.

From WSL/Linux:

`bash Installers/build_deb.sh`

The `.deb` is created in `Installers/`.
