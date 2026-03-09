# Skyrim VR ESL

Adds ESL support to SkyrimVR.

- [Nexus](https://www.nexusmods.com/skyrimspecialedition/mods/106712)

## Requirements

- [CMake](https://cmake.org/)
  - Add this to your `PATH`
- [PowerShell](https://github.com/PowerShell/PowerShell/releases/latest)
- [Vcpkg](https://github.com/microsoft/vcpkg)
  - Add the environment variable `VCPKG_ROOT` with the value as the path to the folder containing vcpkg
- [Visual Studio Community 2022](https://visualstudio.microsoft.com/)
  - Desktop development with C++
- [CommonLibVR](https://github.com/alandtse/CommonLibVR/tree/vr)
  - Add this as as an environment variable `CommonLibVRPath`

## User Requirements

- [VR Address Library for SKSEVR](https://www.nexusmods.com/skyrimspecialedition/mods/58101)
  - Needed for VR

## Local Development Setup

After cloning, install the pre-commit hooks (requires [pre-commit](https://pre-commit.com/)):
```
pre-commit install
```
This runs `clang-format` on staged files before each commit, matching the CI format check.

## Register Visual Studio as a Generator

- Open `x64 Native Tools Command Prompt`
- Run `cmake`
- Close the cmd window

## Building

```
git clone https://github.com/Nightfallstorm/SkyrimVRESL.git
cd skyrimvresl
# pull submodules
git submodule update --init --recursive
```

### VR

```
# run preset
cmake --preset vs2022-windows-vcpkg-vr
# see CMakeUserPresets.json.template to customize presets
cmake --build buildvr --config Release
```

## License

[GPL-3.0-or-later](LICENSE) WITH [Modding Exception AND GPL-3.0 Linking Exception (with Corresponding Source)](EXCEPTIONS.md).  
Specifically, the Modded Code is Skyrim (and its variants) and Modding Libraries include [SKSE](https://skse.silverlock.org/) and Commonlib (and variants).

Use of the API headers ([SkyrimVRESLAPI.h](src/SkyrimVRESLAPI.h), [SkyrimVRESLAPI.cpp](src/SkyrimVRESLAPI.cpp), [SkyrimVRESLAPI_SKSE.h](src/SkyrimVRESLAPI_SKSE.h), [SkyrimVRESLAPI_SKSE.cpp](src/SkyrimVRESLAPI_SKSE.cpp)) is under [MIT](https://opensource.org/license/mit/). In other words, using the SKSE messaging interface to call this dll's published API will not require application of the GPL-3.0 terms to the calling dll. In addition, using SKSEVR or CommonLib with SkyrimVRESL support will not require application of the GPL-3.0 terms.

## Credits

Skyrim modding is built on the community. While there are too many to count, we couldn't have done this without foundational work since the original Skyrim release.

- Contributors to [CommonLib (check fork network and all the credits)](https://github.com/alandtse/CommonLibVR/tree/vr), including the original author, [Ryan](https://github.com/Ryan-rsm-McKenzie).
- [SKSE Team](https://skse.silverlock.org/)
- [Powerof3](https://github.com/powerof3)

## Developer Use

See [Wiki](https://github.com/Nightfallstorm/SkyrimVRESL/wiki/Developers)

### API

No link-time dependency on SkyrimVRESL is required — the interface is retrieved at runtime via SKSE's messaging system.

**Option A — vcpkg overlay port (recommended)**

Add the overlay port path to your `CMakePresets.json`:
```json
"VCPKG_OVERLAY_PORTS": "/path/to/SkyrimVRESL/cmake/ports"
```
Add the dependency to your `vcpkg.json`:
```json
{ "name": "skyrimvresl" }
```
For the legacy SKSE64 SDK variant add `"features": ["legacy"]`. Then in CMake:
```cmake
find_package(skyrimvresl CONFIG REQUIRED)
target_link_libraries(my_plugin PRIVATE SkyrimVRESL::CommonLib)  # or SkyrimVRESL::SKSE
```
The API source is compiled into your plugin automatically via the CMake target.

**Option B — manual copy**

Copy `SkyrimVRESLAPI.h` + `SkyrimVRESLAPI.cpp` (CommonLib) or `SkyrimVRESLAPI_SKSE.h` + `SkyrimVRESLAPI_SKSE.cpp` (legacy SKSE64 SDK) directly into your project and add the `.cpp` to your source list.

**ISkyrimVRESLInterface001** — `GetSkyrimVRESLInterface001()`
- `GetBuildNumber()` — VRESL build number
- `GetCompiledFileCollection()` — SSE-compatible `TESFileCollection` (populated after `kDataLoaded`)

**ISkyrimVRESLInterface002** — `GetSkyrimVRESLInterface002()` (extends 001)
- `GetPluginLoadTimings(uint32_t* outCount)` — array of `VRESLPluginLoadTiming`, one entry per plugin, with `totalNs` (ConstructObjectList) and `openNs` (OpenTES / file I/O) durations. Stable for the lifetime of the DLL; call after `kDataLoaded`.
