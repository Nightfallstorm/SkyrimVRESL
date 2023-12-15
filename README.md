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

## Developer Use

### General

While we did not break legacy behavior of SkyrimVR data handlers, if your DLL requires accessing the list of plugins or formids, you will likely need to update as users start adding ESL mods. 

SkyrimVR ESL support can be added to your SKSEVR dlls in three ways:

1. **RECOMMENDED** [Automatically](#automatically) using ESL-enabled [CommonlibVR](https://github.com/alandtse/commonlibvr) or [CommonLibNG](https://github.com/alandtse/CommonLibVR/tree/ng). Functions have been modified to automatically detect and use.
2. Registering for the VR version of `RE::TESFileCollection*` via [SKSE's Messaging Interface](#skse-messaging-interface)
3. [Using `GetModuleHandle` and `GetProcAddress`](#getprocaddress) to grab the exported function `GetCompiledFileCollectionExtern`. _This is **not recommended** and will require your dll to be GPL-3.0 if used._

### Examples

#### Automatically

Build and compile like normal with the latest builds of commonlib. Yes, it's supposed to be that easy.

```cpp
// Get via CommonLibVR or NG handle (recommended)
// Existing functions have been converted.
auto dh = RE::TESDataHandler::GetSingleton();
logger::info("Commonlib TESDataHandler 0x{:x} "sv, reinterpret_cast<uintptr_t>(dh)); // Get Address of TESDataHandler
if (dh)
				logger::info("Commonlib TESDataHandler {} regular {} light"sv, dh->GetLoadedModCount(), dh->GetLoadedLightModCount()); // NG functions with built in support
```

#### SKSE Messaging Interface

1. Copy [src/SkyrimVRESLAPI.h](src/SkyrimVRESLAPI.h) and [src/SkyrimVRESLAPI.cpp](src/SkyrimVRESLAPI.cpp) into your project.
   - We recommend you use [vcpkg to manage](#manage-dependency-with-vcpkg)
2. Include the header `#include "SkyrimVRESLAPI.h"`
3. Get API doing SKSE PostDataLoaded (any earlier and no data will be available) with `SkyrimVRESLPluginAPI::GetSkyrimVRESLInterface001()`. By default the handle is in the global `g_SkyrimVRESLInterface`.

```cpp
#include "SkyrimVRESLAPI.h"
...
void MessageHandler(SKSE::MessagingInterface::Message* a_message)
{
	if (a_message->type == SKSE::MessagingInterface::kDataLoaded) { // Need to wait till mod data is loaded!
    SkyrimVRESLPluginAPI::GetSkyrimVRESLInterface001()	;  // Request interface
    if (g_SkyrimVRESLInterface) {                        // Use Interface, requires #include "SkyrimVRESLAPI.h"
      const auto version = g_SkyrimVRESLInterface->GetBuildNumber();
      logger::info("Got SkyrimVRESL interface buildnumber {}", version);
      const auto& vrCompiledFiles = g_SkyrimVRESLInterface->GetCompiledFileCollection(); // Grab file collection
      if (vrCompiledFiles) {
        logger::info("Got vrCompiledFiles via SKSE {} regular {} light"sv, vrCompiledFiles->files.size(), vrCompiledFiles->smallFiles.size());
        logger::info("Printing loaded files");
        for (const auto& file : vrCompiledFiles->files) {
          logger::info("Regular file {} recordFlags: {:x} index {:x}",
            std::string(file->fileName),
            file->recordFlags.underlying(),
            file->compileIndex);
        }
        for (const auto& file : vrCompiledFiles->smallFiles) {
          logger::info("Small file {} recordFlags: {:x} index {:x}",
            std::string(file->fileName),
            file->recordFlags.underlying(),
            file->smallFileCompileIndex);
        }
      }
    } else {
            logger::info("SkyrimVRESL not detected");
    }
}
```

##### Manage dependency with vcpkg

VCPKG is supported using a custom port found in [cmake/ports/SkyrimVRESL](cmake/ports/SkyrimVRESL).

1. Edit vcpkg.json to add `skyrimvresl`. This is case-sensitive.

```json
{
  ...
  "dependencies": [
    "skyrimvresl",
    ...
  ]
}
```

2. Ensure cmake support [custom ports](https://github.com/microsoft/vcpkg/blob/master/docs/users/config-environment.md#vcpkg_overlay_ports). Example for setting `CmakePresets.json`.

```json
        "VCPKG_OVERLAY_PORTS": {
          "type": "STRING",
          "value": "${sourceDir}/cmake/ports/"
        }
```

3. Copy [cmake/ports/SkyrimVRESL](cmake/ports/SkyrimVRESL) to `/cmake/ports` in your project.
4. Modify CMakeFiles.txt to include the files.

```cmake
#... Populate the SKYRIMVRESL_INCLUDE_DIRS directory
find_path(SKYRIMVRESL_INCLUDE_DIRS "SkyrimVRESLAPI.h")
#... SkyrimVRESLAPI.CPP is necessary to avoid LNK2001 unresolved external symbol errors
add_library(
	${PROJECT_NAME}
	SHARED
	${SKYRIMVRESL_INCLUDE_DIRS}/SkyrimVRESLAPI.cpp
)
#... Needed so SkyrimVRESLAPI.h can be found
target_include_directories(
	${PROJECT_NAME}
	PRIVATE
    #...
		${SKYRIMVRESL_INCLUDE_DIRS}
)
```

#### GetProcAddress

This was added for CommonLib and SKSEVR use since they cannot use [SKSE Messaging Interface](#skse-messaging-interface). While this requires no dependency setup, _this is **not recommended** and will require your dll to be GPL-3.0 if used._

```cpp
// Get via GetModuleHandle and GetProcAddress
// This is not intended for regular use and should not be considered stable.
static const RE::TESFileCollection* VRcompiledFileCollection = nullptr;
const auto VRhandle = GetModuleHandle("skyrimvresl");
if (!VRcompiledFileCollection && VRhandle != NULL) {
  const auto GetCompiledFileCollection = reinterpret_cast<const RE::TESFileCollection* (*)()>(GetProcAddress(VRhandle, "GetCompiledFileCollectionExtern"));
  if (GetCompiledFileCollection != nullptr) {
    VRcompiledFileCollection = GetCompiledFileCollection();
  }
  logger::info("Found VRcompiledFileCollection 0x{:x}", reinterpret_cast<std::uintptr_t>(VRcompiledFileCollection));
  if (VRcompiledFileCollection)
    logger::info("VRcompiledFileCollection {} regular {} light"sv, VRcompiledFileCollection->files.size(), VRcompiledFileCollection->smallFiles.size());
}
```
