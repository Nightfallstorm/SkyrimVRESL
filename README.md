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

Use of the [SkyrimVRESLAPI.h](cmake/ports/SkyrimVRESL/SkyrimVRESLAPI.h) and [SkyrimVRESLAPI.cpp](cmake/ports/SkyrimVRESL/SkyrimVRESLAPI.cpp) is under [MIT](https://opensource.org/license/mit/). In other words, using the SKSE messaging interface to call this dll's published API will not require application of the GPL-3.0 terms to the calling dll. In addition, using SKSEVR or CommonLib with SkyrimVRESL support will not require application of the GPL-3.0 terms.

## Credits

Skyrim modding is built on the community. While there are too many to count, we couldn't have done this without foundational work since the original Skyrim release.

- Contributors to [CommonLib (check fork network and all the credits)](https://github.com/alandtse/CommonLibVR/tree/vr), including the original author, [Ryan](https://github.com/Ryan-rsm-McKenzie).
- [SKSE Team](https://skse.silverlock.org/)
- [Powerof3](https://github.com/powerof3)
