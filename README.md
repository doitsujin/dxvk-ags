## AGS D3D11 extensions for [DXVK](https://github.com/doitsujin/dxvk)

Provides a rudimentary proof-of-concept implementation of some of the D3D11 extensions available in the [AMD AGS SDK](https://github.com/GPUOpen-LibrariesAndSDKs/AGS_SDK) for DXVK. Requires DXVK Version 1.2 or later.

The currently supported features include:
- Depth bounds test
- Multi-Draw Indirect
- Multi-Draw Indirect with Indirect Count
- UAV Overlap

### Motivation
This project was started as an experiment to test whether DXVK can benefit from AMD [optimizations](https://gpuopen.com/gdc-presentations/2019/gdc-2019-s4-optimization-techniques-re2-dmc5.pdf) in Capcom's RE Engine, specifically in **Resident Evil 2** and **Devil May Cry 5**.

### Build instructions
Like DXVK, this library is being built as a Windows DLL using MinGW, and has essentially the same build requirements.

In order to build the DLL for the default AGS version, currently 5.2, run:
```
meson --cross-file build-win64.txt --buildtype release --prefix /your/path build
cd build
ninja install
```

The compiled DLL will be located in `/your/path/bin/amd_ags_x64.dll`.

A different version can be built by setting the `ags-version` build option as follows:
```
cd build
meson configure -Dags-version=<version>
```

32-bit builds, as well as winelib builds and MSVC are not supported, and will not be supported due to the experimental nature of the project.

### How to use
Games that support AGS usually come with their own copy of `amd_ags_x64.dll`. This file needs to be replaced with your built DLL.

Note that the version you build **must** match the version of the DLL that the game ships. AGS versions are not backwards-compatible. Version 5.2 is used in the aforementioned RE Engine games.

**Note**: The current implementation is very crude and may cause bugs or crashes in some games.

### Expected results
On an RX 480, depending on the graphics settings and resolution, performance in Resident Evil 2 improves by 1-3% with AGS optimizations enabled.
